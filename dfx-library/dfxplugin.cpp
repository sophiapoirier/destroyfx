/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2020  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Destroy FX Library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Destroy FX Library.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/

Destroy FX is a sovereign entity comprised of Sophia Poirier and Tom Murphy 7.  
This is our class for E-Z plugin-making and E-Z multiple-API support.
------------------------------------------------------------------------*/

#include "dfxplugin.h"

#include <algorithm>
#include <bitset>
#include <cassert>
#include <cmath>
#include <functional>
#include <mutex>
#include <stdio.h>
#include <thread>
#include <time.h>	// for time(), which is used to feed srand()
#include <unordered_set>

#include "dfxmisc.h"

#ifdef TARGET_API_AUDIOUNIT
	#include "dfx-au-utilities.h"
#endif

#if defined(TARGET_API_VST) && TARGET_PLUGIN_HAS_GUI
	#include "dfxguieditor.h"
	extern AEffEditor* DFXGUI_NewEditorInstance(DfxPlugin* inEffectInstance);
#endif

#ifdef TARGET_API_RTAS
	#include "ConvertUtils.h"
	#if TARGET_PLUGIN_HAS_GUI
		extern void* gThisModule;
	#endif
#endif

//#define DFX_DEBUG_PRINT_MUSICAL_TIME_INFO
//#define DFX_DEBUG_PRINT_MUSIC_EVENTS


constexpr char const* const kPresetDefaultName = "default";
constexpr std::chrono::milliseconds kIdleTimerInterval(30);


namespace
{

static std::atomic<bool> sIdleThreadShouldRun {false};
__attribute__((no_destroy)) static std::unique_ptr<std::thread> sIdleThread;  // TODO: C++20 use std::jthread, C++23 [[no_destroy]]
__attribute__((no_destroy)) static std::mutex sIdleThreadLock;
__attribute__((no_destroy)) static std::unordered_set<DfxPlugin*> sIdleClients; 
__attribute__((no_destroy)) static std::mutex sIdleClientsLock;

//-----------------------------------------------------------------------------
static void DFX_RegisterIdleClient(DfxPlugin* const inIdleClient)
{
	{
		std::lock_guard const guard(sIdleClientsLock);
		[[maybe_unused]] auto const [element, inserted] = sIdleClients.insert(inIdleClient);
		assert(inserted);
	}

	{
		std::lock_guard const guard(sIdleThreadLock);
		if (!sIdleThread)
		{
			sIdleThreadShouldRun = true;
			sIdleThread = std::make_unique<std::thread>([]()
			{
#if TARGET_OS_MAC
				pthread_setname_np(PLUGIN_NAME_STRING " idle timer");
#endif
				while (sIdleThreadShouldRun)
				{
					{
						std::lock_guard const guard(sIdleClientsLock);
						std::for_each(sIdleClients.cbegin(), sIdleClients.cend(), 
									  [](auto&& idleClient){ idleClient->do_idle(); });
					}
					std::this_thread::sleep_for(kIdleTimerInterval);
				}
			});
		}
	}
}

//-----------------------------------------------------------------------------
static void DFX_UnregisterIdleClient(DfxPlugin* const inIdleClient)
{
	auto const allClientsCompleted = [&inIdleClient]()
	{
		std::lock_guard const guard(sIdleClientsLock);
		[[maybe_unused]] auto const eraseCount = sIdleClients.erase(inIdleClient);
		assert(eraseCount == 1);
		return sIdleClients.empty();
	}();

	if (allClientsCompleted)
	{
		sIdleThreadShouldRun = false;
		std::lock_guard const guard(sIdleThreadLock);
		if (sIdleThread)
		{
			sIdleThread->join();
			sIdleThread.reset();
		}
	}
}

}  // namespace



#pragma mark --- DfxPlugin ---

#pragma mark -
#pragma mark init

//-----------------------------------------------------------------------------
DfxPlugin::DfxPlugin(
					TARGET_API_BASE_INSTANCE_TYPE inInstance
					, long inNumParameters
					, long inNumPresets
					) :

// setup the constructors of the inherited base classes, for the appropriate API
#ifdef TARGET_API_AUDIOUNIT
	#if TARGET_PLUGIN_IS_INSTRUMENT
//	TARGET_API_BASE_CLASS(inInstance, UInt32 inNumInputs, UInt32 inNumOutputs, UInt32 inNumGroups = 0), 
	TARGET_API_BASE_CLASS(inInstance, 0, 1), 
	#else
	TARGET_API_BASE_CLASS(inInstance), 
	#endif
#endif

#ifdef TARGET_API_VST
	TARGET_API_BASE_CLASS(inInstance, inNumPresets, inNumParameters), 
#endif
// end API-specific base constructors

	mParameters(inNumParameters),
	mParametersChangedAsOfPreProcess(inNumParameters, false),
	mParametersTouchedAsOfPreProcess(inNumParameters, false),
	mParametersChangedInProcessHavePosted(inNumParameters)
{
	updatesamplerate();  // XXX have it set to something here?

	// set a seed value for rand() from the system clock
	srand(static_cast<unsigned int>(time(nullptr)));

	mPresets.reserve(inNumPresets);
	for (long i = 0; i < inNumPresets; i++)
	{
		mPresets.emplace_back(inNumParameters);
	}

	// reset pending notifications
	std::for_each(mParametersChangedInProcessHavePosted.begin(), mParametersChangedInProcessHavePosted.end(), 
				  [](auto& flag){ flag.test_and_set(); });
	mPresetChangedInProcessHasPosted.test_and_set();
	mLatencyChangeHasPosted.test_and_set();
	mTailSizeChangeHasPosted.test_and_set();

#if TARGET_PLUGIN_USES_MIDI
	mMidiLearnChangedInProcessHasPosted.test_and_set();
	mMidiLearnerChangedInProcessHasPosted.test_and_set();
#endif

#if defined(TARGET_API_AUDIOUNIT) && !TARGET_PLUGIN_IS_INSTRUMENT
	SetProcessesInPlace(true);  // XXX why the default AUEffectBase constructor argument is not applied?
#endif

#ifdef TARGET_API_VST
	#if TARGET_PLUGIN_IS_INSTRUMENT
	static_assert(VST_NUM_INPUTS >= 0);
	#else
	static_assert(VST_NUM_INPUTS > 0);
	#endif
	static_assert(VST_NUM_OUTPUTS > 0);
	mNumInputs = VST_NUM_INPUTS;
	mNumOutputs = VST_NUM_OUTPUTS;

	setUniqueID(PLUGIN_ID);
	setNumInputs(VST_NUM_INPUTS);
	setNumOutputs(VST_NUM_OUTPUTS);

	TARGET_API_BASE_CLASS::setProgram(0);  // set the current preset number to 0

	noTail(true);  // until the plugin declares otherwise

	// check to see if the host supports sending tempo and time information to VST plugins
	// Note that the VST2 SDK (probably erroneously) wants a non-const string here,
	// so we don't pass a string literal.
	char timeinfo[] = "sendVstTimeInfo";
	mHostCanDoTempo = canHostDo(timeinfo) == 1;

	#if TARGET_PLUGIN_USES_MIDI
	// tell host that we want to use special data chunks for settings storage
	programsAreChunks();
	#endif

	#if TARGET_PLUGIN_IS_INSTRUMENT
	isSynth();
	#endif

	#if TARGET_PLUGIN_HAS_GUI
	// XXX AEffGUIEditor registers itself, so we probably don't need to assign this
	// to the member variable here (and probably should be using AEffect::setEditor?).
	// Instead we can probably just call getEditor() if we need it.
	editor = DFXGUI_NewEditorInstance(this);
	#endif
#endif
// end VST stuff

#ifdef TARGET_API_RTAS
	#if TARGET_PLUGIN_HAS_GUI
	mPIWinRect.top = mPIWinRect.left = mPIWinRect.bottom = mPIWinRect.right = 0;
	#if TARGET_OS_WIN32
	mModuleHandle_p = gThisModule;  // extern from DLLMain.cpp; HINSTANCE of the DLL
	#endif
	#endif
#endif
// end RTAS stuff
}

//-----------------------------------------------------------------------------
// this is called immediately after all constructors (DfxPlugin and any derived classes) complete
void DfxPlugin::do_PostConstructor()
{
	// set up a name for the default preset if none was set
	if ((getnumpresets() > 0) && !presetnameisvalid(0))
	{
		setpresetname(0, kPresetDefaultName);
	}

#if TARGET_PLUGIN_USES_MIDI
	mDfxSettings = std::make_unique<DfxSettings>(PLUGIN_ID, this, settings_sizeOfExtendedData());
#endif

	dfx_PostConstructor();

	DFX_RegisterIdleClient(this);

#ifdef TARGET_API_VST
	// all supported channel configurations should have been added by now
	assert(ischannelcountsupported(getnuminputs(), getnumoutputs()));
#endif
}


//-----------------------------------------------------------------------------
// this is called immediately before all destructors (DfxPlugin and any derived classes) occur
void DfxPlugin::do_PreDestructor()
{
	DFX_UnregisterIdleClient(this);

	dfx_PreDestructor();

#ifdef TARGET_API_VST
	// VST doesn't have initialize and cleanup methods like Audio Unit does, 
	// so we need to call this manually here
	do_cleanup();
#endif
}


//-----------------------------------------------------------------------------
// non-virtual function that calls initialize() and insures that some stuff happens
long DfxPlugin::do_initialize()
{
	updatesamplerate();
	updatenumchannels();

	auto const result = initialize();
	if (result != dfx::kStatus_NoError)
	{
		return result;
	}

#ifndef TARGET_API_AUDIOUNIT
	mInputAudioStreams.assign(getnuminputs(), nullptr);
	if (!mInPlaceAudioProcessingAllowed)
	{
		mInputOutOfPlaceAudioBuffers.assign(getnuminputs(), std::vector<float>(getmaxframes(), 0.0f));
		std::transform(mInputOutOfPlaceAudioBuffers.cbegin(), mInputOutOfPlaceAudioBuffers.cend(), 
					   mInputAudioStreams.begin(), [](auto const& buffer){ return buffer.data(); });
	}
#endif  // !TARGET_API_AUDIOUNIT

#ifdef TARGET_API_VST
	mIsInitialized = true;
#endif

#if TARGET_PLUGIN_USES_DSPCORE
	// flag parameter changes to be picked up for DSP cores, which are all instantiated anew during plugin initialize
	for (long i = 0; i < getnumparameters(); i++)
	{
		mParameters[i].setchanged(true);
	}

	if (asymmetricalchannels())
	{
		assert(getnumoutputs() > getnuminputs());  // the only imbalance we support with DSP cores
	#ifdef TARGET_API_AUDIOUNIT
		mAsymmetricalInputBufferList.Allocate(GetStreamFormat(kAudioUnitScope_Output, 0), getmaxframes());
	#else
		mAsymmetricalInputAudioBuffer.assign(getmaxframes(), 0.0f);
	#endif
	}
#endif

	std::for_each(mSmoothedAudioValues.cbegin(), mSmoothedAudioValues.cend(), 
				  [sr = getsamplerate()](auto& value){ value.first->setSampleRate(sr); });

	createbuffers();
	do_reset();

	return dfx::kStatus_NoError;
}

//-----------------------------------------------------------------------------
// non-virtual function that calls cleanup() and insures that some stuff happens
void DfxPlugin::do_cleanup()
{
#ifdef TARGET_API_VST
	if (!mIsInitialized)
	{
		// This is normal if resume() is never called (because
		// we initialize on the first resume()), which happens
		// e.g. in Cakewalk when it probes plugins on startup.
		return;
	}
#endif

	releasebuffers();

#if TARGET_PLUGIN_USES_DSPCORE
	#ifdef TARGET_API_AUDIOUNIT
	mAsymmetricalInputBufferList.Deallocate();
	#else
	mAsymmetricalInputAudioBuffer.clear();
	#endif
#endif

#ifdef TARGET_API_AUDIOUNIT
	mInputAudioStreams_au.clear();
	mOutputAudioStreams_au.clear();
#endif

	cleanup();

	mAudioRenderThreadID = {};

#ifdef TARGET_API_VST
	mIsInitialized = false;
#endif
}

//-----------------------------------------------------------------------------
// non-virtual function that calls reset() and insures that some stuff happens
void DfxPlugin::do_reset()
{
#ifdef TARGET_API_AUDIOUNIT
	// no need to do this if we're not even in Initialized state 
	// because this will basically happen when we become initialized
	// XXX not true!  because IsInitialized() will only return true *after* the Initialize() call returns
//	if (!IsInitialized())
	{
//		return;
	}
	#if !TARGET_PLUGIN_IS_INSTRUMENT
	// resets the kernels, if any
	TARGET_API_BASE_CLASS::Reset(kAudioUnitScope_Global, AudioUnitElement(0));
	#endif
#endif

	mIsFirstRenderSinceReset = true;
	std::for_each(mSmoothedAudioValues.cbegin(), mSmoothedAudioValues.cend(), [](auto& value){ value.first->snap(); });

#if TARGET_PLUGIN_USES_MIDI
	mMidiState.reset();
#endif

#if TARGET_PLUGIN_USES_DSPCORE && !defined(TARGET_API_AUDIOUNIT)
	for (auto& dspCore : mDSPCores)
	{
		if (dspCore)
		{
			dspCore->do_reset();
		}
	}
#endif

	reset();
	clearbuffers();
}



#pragma mark -
#pragma mark parameters

//-----------------------------------------------------------------------------
void DfxPlugin::initparameter_f(long inParameterIndex, std::vector<std::string_view> const& initNames, 
								double initValue, double initDefaultValue, 
								double initMin, double initMax, 
								DfxParam::Unit initUnit, DfxParam::Curve initCurve, 
								std::string_view initCustomUnitString)
{
	if (parameterisvalid(inParameterIndex))
	{
		mParameters[inParameterIndex].init_f(initNames, initValue, initDefaultValue, initMin, initMax, initUnit, initCurve);
// XXX hmmm... maybe not here?
//		if (hasparameterattribute(inParameterIndex, DfxParam::kAttribute_Unused))  // XXX should we do it like this?
		{
//			update_parameter(inParameterIndex);  // XXX make the host aware of the parameter change
		}
		initpresetsparameter(inParameterIndex);  // default empty presets with this value
		if (!initCustomUnitString.empty())
		{
			setparametercustomunitstring(inParameterIndex, initCustomUnitString);
		}
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::initparameter_i(long inParameterIndex, std::vector<std::string_view> const& initNames, 
								int64_t initValue, int64_t initDefaultValue, 
								int64_t initMin, int64_t initMax, 
								DfxParam::Unit initUnit, DfxParam::Curve initCurve, 
								std::string_view initCustomUnitString)
{
	if (parameterisvalid(inParameterIndex))
	{
		mParameters[inParameterIndex].init_i(initNames, initValue, initDefaultValue, initMin, initMax, initUnit, initCurve);
//		update_parameter(inParameterIndex);  // XXX make the host aware of the parameter change
		initpresetsparameter(inParameterIndex);  // default empty presets with this value
		if (!initCustomUnitString.empty())
		{
			setparametercustomunitstring(inParameterIndex, initCustomUnitString);
		}
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::initparameter_b(long inParameterIndex, std::vector<std::string_view> const& initNames, 
								bool initValue, bool initDefaultValue, 
								DfxParam::Unit initUnit)
{
	if (parameterisvalid(inParameterIndex))
	{
		mParameters[inParameterIndex].init_b(initNames, initValue, initDefaultValue, initUnit);
//		update_parameter(inParameterIndex);  // XXX make the host aware of the parameter change
		initpresetsparameter(inParameterIndex);  // default empty presets with this value
	}
}

//-----------------------------------------------------------------------------
// this is a shorcut for initializing a parameter that uses integer indexes 
// into an array, with an array of strings representing its values
void DfxPlugin::initparameter_list(long inParameterIndex, std::vector<std::string_view> const& initNames, 
								   int64_t initValue, int64_t initDefaultValue, 
								   int64_t initNumItems, DfxParam::Unit initUnit, 
								   std::string_view initCustomUnitString)
{
	if (parameterisvalid(inParameterIndex))
	{
		mParameters[inParameterIndex].init_i(initNames, initValue, initDefaultValue, 0, initNumItems - 1, initUnit, DfxParam::Curve::Stepped);
		setparameterusevaluestrings(inParameterIndex, true);  // indicate that we will use custom value display strings
//		update_parameter(inParameterIndex);  // XXX make the host aware of the parameter change
		initpresetsparameter(inParameterIndex);  // default empty presets with this value
		if (!initCustomUnitString.empty())
		{
			setparametercustomunitstring(inParameterIndex, initCustomUnitString);
		}
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameter(long inParameterIndex, DfxParam::Value inValue)
{
	if (parameterisvalid(inParameterIndex))
	{
		mParameters[inParameterIndex].set(inValue);
		update_parameter(inParameterIndex);  // make the host aware of the parameter change
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameter_f(long inParameterIndex, double inValue)
{
	if (parameterisvalid(inParameterIndex))
	{
		mParameters[inParameterIndex].set_f(inValue);
		update_parameter(inParameterIndex);  // make the host aware of the parameter change
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameter_i(long inParameterIndex, int64_t inValue)
{
	if (parameterisvalid(inParameterIndex))
	{
		mParameters[inParameterIndex].set_i(inValue);
		update_parameter(inParameterIndex);  // make the host aware of the parameter change
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameter_b(long inParameterIndex, bool inValue)
{
	if (parameterisvalid(inParameterIndex))
	{
		mParameters[inParameterIndex].set_b(inValue);
		update_parameter(inParameterIndex);  // make the host aware of the parameter change
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameter_gen(long inParameterIndex, double inValue)
{
	if (parameterisvalid(inParameterIndex))
	{
		mParameters[inParameterIndex].set_gen(inValue);
		update_parameter(inParameterIndex);  // make the host aware of the parameter change
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameterquietly_f(long inParameterIndex, double inValue)
{
	if (parameterisvalid(inParameterIndex))
	{
		mParameters[inParameterIndex].setquietly_f(inValue);
		update_parameter(inParameterIndex);  // make the host aware of the parameter change
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameterquietly_i(long inParameterIndex, int64_t inValue)
{
	if (parameterisvalid(inParameterIndex))
	{
		mParameters[inParameterIndex].setquietly_i(inValue);
		update_parameter(inParameterIndex);  // make the host aware of the parameter change
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameterquietly_b(long inParameterIndex, bool inValue)
{
	if (parameterisvalid(inParameterIndex))
	{
		mParameters[inParameterIndex].setquietly_b(inValue);
		update_parameter(inParameterIndex);  // make the host aware of the parameter change
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::randomizeparameter(long inParameterIndex)
{
	if (parameterisvalid(inParameterIndex))
	{
		mParameters[inParameterIndex].randomize();
		update_parameter(inParameterIndex);  // make the host aware of the parameter change
		postupdate_parameter(inParameterIndex);  // inform any parameter listeners of the changes
	}
}

//-----------------------------------------------------------------------------
// randomize all of the parameters at once
void DfxPlugin::randomizeparameters()
{
	for (long i = 0; i < getnumparameters(); i++)
	{
		if (!hasparameterattribute(i, DfxParam::kAttribute_OmitFromRandomizeAll))
		{
			randomizeparameter(i);
		}
	}
}

//-----------------------------------------------------------------------------
// do stuff necessary to inform the host of changes, etc.
void DfxPlugin::update_parameter(long inParameterIndex)
{
#ifdef TARGET_API_AUDIOUNIT
	// make the global-scope element aware of the parameter's value
	TARGET_API_BASE_CLASS::SetParameter(inParameterIndex, kAudioUnitScope_Global, AudioUnitElement(0), getparameter_f(inParameterIndex), 0);
#endif

#ifdef TARGET_API_VST
	auto const vstPresetIndex = getProgram();
	if (presetisvalid(vstPresetIndex))
	{
		setpresetparameter(vstPresetIndex, inParameterIndex, getparameter(inParameterIndex));
	}
	#if TARGET_PLUGIN_HAS_GUI
	// the VST2 editor interface has no real listener mechanism for parameters and therefore 
	// always needs notification pushed in any circumstance where a parameter value changes
	postupdate_parameter(inParameterIndex);
	#endif
#endif

#ifdef TARGET_API_RTAS
	SetControlValue(dfx::ParameterID_ToRTAS(inParameterIndex), ConvertToDigiValue(getparameter_gen(inParameterIndex)));  // XXX yeah do this?
#endif
}

//-----------------------------------------------------------------------------
// this will broadcast a notification to anyone interested (host, GUI, etc.) 
// about a parameter change
void DfxPlugin::postupdate_parameter(long inParameterIndex)
{
	if (!parameterisvalid(inParameterIndex))
	{
		return;
	}

	// defer the notification because it is not realtime-safe
	if (std::this_thread::get_id() == mAudioRenderThreadID)
	{
		// defer listener notification to later, off the realtime thread
		mParametersChangedInProcessHavePosted[inParameterIndex].clear(std::memory_order_relaxed);
		return;
	}

#ifdef TARGET_API_AUDIOUNIT
	AUParameterChange_TellListeners(GetComponentInstance(), inParameterIndex);
#endif

#ifdef TARGET_API_VST
	#if TARGET_PLUGIN_HAS_GUI
	if (auto const guiEditor = dynamic_cast<VSTGUI::AEffGUIEditor*>(getEditor()))
	{
		guiEditor->setParameter(inParameterIndex, getparameter_gen(inParameterIndex));
	}
	#endif
#endif

#ifdef TARGET_API_RTAS
	#warning "implementation required?"
#endif
}

//-----------------------------------------------------------------------------
DfxParam::Value DfxPlugin::getparameter(long inParameterIndex) const
{
	if (parameterisvalid(inParameterIndex))
	{
		return mParameters[inParameterIndex].get();
	}
	return {};
}

//-----------------------------------------------------------------------------
// return a (hopefully) 0 to 1 scalar version of the parameter's current value
double DfxPlugin::getparameter_scalar(long inParameterIndex) const
{
	if (parameterisvalid(inParameterIndex))
	{
		switch (getparameterunit(inParameterIndex))
		{
			case DfxParam::Unit::Percent:
			case DfxParam::Unit::DryWetMix:
				return mParameters[inParameterIndex].get_f() * 0.01;
			case DfxParam::Unit::Scalar:
				return mParameters[inParameterIndex].get_f();
			// XXX should we not just use contractparametervalue() here?
			default:
				return mParameters[inParameterIndex].get_f() / mParameters[inParameterIndex].getmax_f();
		}
	}
	return 0.0;
}

//-----------------------------------------------------------------------------
std::optional<double> DfxPlugin::getparameterifchanged_f(long inParameterIndex) const
{
	if (getparameterchanged(inParameterIndex))
	{
		return getparameter_f(inParameterIndex);
	}
	return {};
}

//-----------------------------------------------------------------------------
std::optional<int64_t> DfxPlugin::getparameterifchanged_i(long inParameterIndex) const
{
	if (getparameterchanged(inParameterIndex))
	{
		return getparameter_i(inParameterIndex);
	}
	return {};
}

//-----------------------------------------------------------------------------
std::optional<bool> DfxPlugin::getparameterifchanged_b(long inParameterIndex) const
{
	if (getparameterchanged(inParameterIndex))
	{
		return getparameter_b(inParameterIndex);
	}
	return {};
}

//-----------------------------------------------------------------------------
std::optional<double> DfxPlugin::getparameterifchanged_scalar(long inParameterIndex) const
{
	if (getparameterchanged(inParameterIndex))
	{
		return getparameter_scalar(inParameterIndex);
	}
	return {};
}

//-----------------------------------------------------------------------------
std::string DfxPlugin::getparametername(long inParameterIndex) const
{
	if (parameterisvalid(inParameterIndex))
	{
		return mParameters[inParameterIndex].getname();
	}
	return {};
}

//-----------------------------------------------------------------------------
std::string DfxPlugin::getparametername(long inParameterIndex, size_t inMaxLength) const
{
	if (parameterisvalid(inParameterIndex))
	{
		return mParameters[inParameterIndex].getname(inMaxLength);
	}
	return {};
}

//-----------------------------------------------------------------------------
DfxParam::ValueType DfxPlugin::getparametervaluetype(long inParameterIndex) const
{
	if (parameterisvalid(inParameterIndex))
	{
		return mParameters[inParameterIndex].getvaluetype();
	}
	return DfxParam::ValueType::Float;
}

//-----------------------------------------------------------------------------
DfxParam::Unit DfxPlugin::getparameterunit(long inParameterIndex) const
{
	if (parameterisvalid(inParameterIndex))
	{
		return mParameters[inParameterIndex].getunit();
	}
	return DfxParam::Unit::Generic;
}

//-----------------------------------------------------------------------------
bool DfxPlugin::setparametervaluestring(long inParameterIndex, int64_t inStringIndex, std::string_view inText)
{
	if (parameterisvalid(inParameterIndex))
	{
		return mParameters[inParameterIndex].setvaluestring(inStringIndex, inText);
	}
	return false;
}

//-----------------------------------------------------------------------------
std::optional<std::string> DfxPlugin::getparametervaluestring(long inParameterIndex, int64_t inStringIndex) const
{
	if (parameterisvalid(inParameterIndex))
	{
		return mParameters[inParameterIndex].getvaluestring(inStringIndex);
	}
	return {};
}

//-----------------------------------------------------------------------------
void DfxPlugin::addparametergroup(std::string const& inName, std::vector<long> const& inParameterIndices)
{
	assert(!inParameterIndices.empty());
	assert(std::none_of(mParameterGroups.cbegin(), mParameterGroups.cend(), [&inName](auto const& item){ return (item.first == inName); }));
	assert(std::none_of(inParameterIndices.cbegin(), inParameterIndices.cend(), [this](auto index){ return getparametergroup(index).has_value(); }));
	assert(std::all_of(inParameterIndices.cbegin(), inParameterIndices.cend(), std::bind(&DfxPlugin::parameterisvalid, this, std::placeholders::_1)));
	assert(std::unordered_set<long>(inParameterIndices.cbegin(), inParameterIndices.cend()).size() == inParameterIndices.size());

	mParameterGroups.emplace_back(inName, std::set<long>(inParameterIndices.cbegin(), inParameterIndices.cend()));
}

//-----------------------------------------------------------------------------
std::optional<size_t> DfxPlugin::getparametergroup(long inParameterIndex) const
{
	auto const foundGroup = std::find_if(mParameterGroups.cbegin(), mParameterGroups.cend(), [inParameterIndex](auto const& group)
	{
		auto const& indices = group.second;
		return (indices.find(inParameterIndex) != indices.cend());
	});
	if (foundGroup != mParameterGroups.cend())
	{
		return static_cast<size_t>(std::distance(mParameterGroups.cbegin(), foundGroup));
	}
	return {};
}

//-----------------------------------------------------------------------------
bool DfxPlugin::getparameterchanged(long inParameterIndex) const
{
	if (parameterisvalid(inParameterIndex))
	{
		return mParametersChangedAsOfPreProcess[inParameterIndex];
	}
	return false;
}

//-----------------------------------------------------------------------------
bool DfxPlugin::getparametertouched(long inParameterIndex) const
{
	if (parameterisvalid(inParameterIndex))
	{
		return mParametersTouchedAsOfPreProcess[inParameterIndex];
	}
	return false;
}

//-----------------------------------------------------------------------------
bool DfxPlugin::hasparameterattribute(long inParameterIndex, DfxParam::Attribute inFlag) const
{
	assert(std::bitset<sizeof(inFlag) * CHAR_BIT>(inFlag).count() == 1);
	return getparameterattributes(inParameterIndex) & inFlag;
}

//-----------------------------------------------------------------------------
// convenience methods for expanding and contracting parameter values 
// using the min/max/curvetype/curvespec/etc. settings of a given parameter
double DfxPlugin::expandparametervalue(long inParameterIndex, double genValue) const
{
	if (parameterisvalid(inParameterIndex))
	{
		return mParameters[inParameterIndex].expand(genValue);
	}
	return 0.0;
}
//-----------------------------------------------------------------------------
double DfxPlugin::contractparametervalue(long inParameterIndex, double realValue) const
{
	if (parameterisvalid(inParameterIndex))
	{
		return mParameters[inParameterIndex].contract(realValue);
	}
	return 0.0;
}



#pragma mark -
#pragma mark presets

//-----------------------------------------------------------------------------
// whether or not the index is a valid preset
bool DfxPlugin::presetisvalid(long inPresetIndex) const
{
	return (inPresetIndex >= 0) && (inPresetIndex < getnumpresets());
}

//-----------------------------------------------------------------------------
// whether or not the index is a valid preset with a valid name
// this is mostly just for Audio Unit
bool DfxPlugin::presetnameisvalid(long inPresetIndex) const
{
	if (!presetisvalid(inPresetIndex))
	{
		return false;
	}
	if (mPresets[inPresetIndex].getname().empty())
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// load the settings of a preset
bool DfxPlugin::loadpreset(long inPresetIndex)
{
	if (!presetisvalid(inPresetIndex))
	{
		return false;
	}

#ifdef TARGET_API_VST
	// XXX this needs to be done before calls to setparameter(), cuz setparameter will 
	// call update_parameter(), which will call setpresetparameter() using getProgram() 
	// for the program index to set parameter values, which means that the currently 
	// selected program will have its parameter values overwritten by those of the 
	// program currently being loaded, unless we do this first
	TARGET_API_BASE_CLASS::setProgram(inPresetIndex);
#endif

	for (long i = 0; i < getnumparameters(); i++)
	{
		setparameter(i, getpresetparameter(inPresetIndex, i));
		postupdate_parameter(i);  // inform any parameter listeners of the changes
	}

	mCurrentPresetNum = inPresetIndex;

	// do stuff necessary to inform the host of changes, etc.
	// XXX in AU, if this resulted from a call to NewFactoryPresetSet, then PropertyChanged will be called twice
	postupdate_preset();
	return true;
}

//-----------------------------------------------------------------------------
// do stuff necessary to inform the host of changes, etc.
void DfxPlugin::postupdate_preset()
{
	assert(presetisvalid(getcurrentpresetnum()));

	if (std::this_thread::get_id() == mAudioRenderThreadID)
	{
		mPresetChangedInProcessHasPosted.clear(std::memory_order_relaxed);
		return;
	}

#ifdef TARGET_API_AUDIOUNIT
	AUPreset au_preset {};
	au_preset.presetNumber = getcurrentpresetnum();
	au_preset.presetName = getpresetcfname(getcurrentpresetnum());
	SetAFactoryPresetAsCurrent(au_preset);
	PropertyChanged(kAudioUnitProperty_PresentPreset, kAudioUnitScope_Global, AudioUnitElement(0));
	PropertyChanged(kAudioUnitProperty_CurrentPreset, kAudioUnitScope_Global, AudioUnitElement(0));
#endif

#ifdef TARGET_API_VST
	TARGET_API_BASE_CLASS::setProgram(getcurrentpresetnum());
	// XXX Cubase SX will crash if custom-GUI plugs call updateDisplay 
	// while the editor is closed, so as a workaround, only do it 
	// if the plugin has no custom GUI
	// calling updateDisplay may be a nice idea in general, though, 
	// since a generic UI might still be provided in some hosts
	#if !TARGET_PLUGIN_HAS_GUI
	// tell the host to update the generic editor display with the new settings
	AudioEffectX::updateDisplay();
	#endif
#endif
}

//-----------------------------------------------------------------------------
// default all empty (no name) presets with the current value of a parameter
void DfxPlugin::initpresetsparameter(long inParameterIndex)
{
	// first fill in the presets with the init settings 
	// so that there are no "stale" unset values
	for (long i = 0; i < getnumpresets(); i++)
	{
		if (!presetnameisvalid(i))  // only if it's an "empty" preset
		{
			setpresetparameter(i, inParameterIndex, getparameter(inParameterIndex));
		}
	}
}

//-----------------------------------------------------------------------------
DfxParam::Value DfxPlugin::getpresetparameter(long inPresetIndex, long inParameterIndex) const
{
	if (parameterisvalid(inParameterIndex) && presetisvalid(inPresetIndex))
	{
		return mPresets[inPresetIndex].getvalue(inParameterIndex);
	}
	return {};
}

//-----------------------------------------------------------------------------
double DfxPlugin::getpresetparameter_f(long inPresetIndex, long inParameterIndex) const
{
	if (parameterisvalid(inParameterIndex) && presetisvalid(inPresetIndex))
	{
		return mParameters[inParameterIndex].derive_f(mPresets[inPresetIndex].getvalue(inParameterIndex));
	}
	return 0.0;
}

//-----------------------------------------------------------------------------
void DfxPlugin::setpresetparameter(long inPresetIndex, long inParameterIndex, DfxParam::Value inValue)
{
	if (parameterisvalid(inParameterIndex) && presetisvalid(inPresetIndex))
	{
		mPresets[inPresetIndex].setvalue(inParameterIndex, inValue);
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setpresetparameter_f(long inPresetIndex, long inParameterIndex, double inValue)
{
	if (parameterisvalid(inParameterIndex) && presetisvalid(inPresetIndex))
	{
		auto const paramValue = mParameters[inParameterIndex].pack_f(inValue);
		mPresets[inPresetIndex].setvalue(inParameterIndex, paramValue);
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setpresetparameter_i(long inPresetIndex, long inParameterIndex, int64_t inValue)
{
	if (parameterisvalid(inParameterIndex) && presetisvalid(inPresetIndex))
	{
		auto const paramValue = mParameters[inParameterIndex].pack_i(inValue);
		mPresets[inPresetIndex].setvalue(inParameterIndex, paramValue);
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setpresetparameter_b(long inPresetIndex, long inParameterIndex, bool inValue)
{
	if (parameterisvalid(inParameterIndex) && presetisvalid(inPresetIndex))
	{
		auto const paramValue = mParameters[inParameterIndex].pack_b(inValue);
		mPresets[inPresetIndex].setvalue(inParameterIndex, paramValue);
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setpresetparameter_gen(long inPresetIndex, long inParameterIndex, double inValue)
{
	if (parameterisvalid(inParameterIndex) && presetisvalid(inPresetIndex))
	{
		auto const paramValue = mParameters[inParameterIndex].pack_f(expandparametervalue(inParameterIndex, inValue));
		mPresets[inPresetIndex].setvalue(inParameterIndex, paramValue);
	}
}

//-----------------------------------------------------------------------------
// set the text of a preset name
void DfxPlugin::setpresetname(long inPresetIndex, std::string_view inText)
{
	assert(!inText.empty());

	if (presetisvalid(inPresetIndex))
	{
		mPresets[inPresetIndex].setname(inText);
	}
}

//-----------------------------------------------------------------------------
// get a copy of the text of a preset name
std::string DfxPlugin::getpresetname(long inPresetIndex) const
{
	if (presetisvalid(inPresetIndex))
	{
		return mPresets[inPresetIndex].getname();
	}
	return {};
}

#ifdef TARGET_API_AUDIOUNIT
//-----------------------------------------------------------------------------
// get the CFString version of a preset name
CFStringRef DfxPlugin::getpresetcfname(long inPresetIndex) const
{
	if (presetisvalid(inPresetIndex))
	{
		return mPresets[inPresetIndex].getcfname();
	}
	return nullptr;
}
#endif



#pragma mark -
#pragma mark state

//-----------------------------------------------------------------------------
// change the current audio sampling rate
void DfxPlugin::setsamplerate(double inSampleRate)
{
	// avoid bogus values
	if (inSampleRate <= 0.0)
	{
		inSampleRate = 44100.0;
	}

	if ((mSampleRateChanged = (inSampleRate != DfxPlugin::mSampleRate)))
	{
#ifdef TARGET_API_AUDIOUNIT
		// assume that sample-specified properties change in absolute duration when sample rate changes
		if (auto const latencySamples = std::get_if<long>(&mLatency); latencySamples && (*latencySamples != 0))
		{
			postupdate_latency();
		}
		if (auto const tailSizeSamples = std::get_if<long>(&mTailSize); tailSizeSamples && (*tailSizeSamples != 0))
		{
			postupdate_tailsize();
		}
#endif
	}

	// accept the new value into our sampling rate keeper
	DfxPlugin::mSampleRate = inSampleRate;

#if TARGET_PLUGIN_USES_MIDI
	mMidiState.setSampleRate(inSampleRate);
#endif
}

//-----------------------------------------------------------------------------
// called when the sampling rate should be re-fetched from the host
void DfxPlugin::updatesamplerate()
{
#ifdef TARGET_API_AUDIOUNIT
	if (mAUElementsHaveBeenCreated)  // will crash otherwise
	{
		setsamplerate(GetSampleRate());
	}
	else
	{
		setsamplerate(kAUDefaultSampleRate);
	}
#endif
#ifdef TARGET_API_VST
	setsamplerate(static_cast<double>(getSampleRate()));
#endif
#ifdef TARGET_API_RTAS
	setsamplerate(GetSampleRate());
#endif
}

//-----------------------------------------------------------------------------
// called when the number of audio channels has changed
void DfxPlugin::updatenumchannels()
{
#if TARGET_PLUGIN_USES_MIDI
	mMidiState.setChannelCount(getnumoutputs());
#endif

#ifdef TARGET_API_AUDIOUNIT
	// the number of inputs or outputs may have changed
	mNumInputs = getnuminputs();
	mInputAudioStreams_au.assign(mNumInputs, nullptr);

	mNumOutputs = getnumoutputs();
	mOutputAudioStreams_au.assign(mNumOutputs, nullptr);
#endif
}



#pragma mark -
#pragma mark properties

//-----------------------------------------------------------------------------
std::string DfxPlugin::getpluginname() const
{
	return PLUGIN_NAME_STRING;
}

//-----------------------------------------------------------------------------
long DfxPlugin::getpluginversion() const
{
	return dfx::CompositePluginVersionNumberValue();
}

//-----------------------------------------------------------------------------
void DfxPlugin::registerSmoothedAudioValue(dfx::ISmoothedValue* smoothedValue, DfxPluginCore* owner)
{
	assert(smoothedValue);
	mSmoothedAudioValues.emplace_back(smoothedValue, owner);
}

//-----------------------------------------------------------------------------
void DfxPlugin::unregisterAllSmoothedAudioValues(DfxPluginCore* owner)
{
	mSmoothedAudioValues.erase(std::remove_if(mSmoothedAudioValues.begin(), mSmoothedAudioValues.end(), [owner](auto const& value)
											  {
												  return (owner == value.second);
											  }), mSmoothedAudioValues.cend());
}

//-----------------------------------------------------------------------------
void DfxPlugin::incrementSmoothedAudioValues(DfxPluginCore* owner)
{
	std::for_each(mSmoothedAudioValues.cbegin(), mSmoothedAudioValues.cend(), [owner](auto& value)
	{
		// TODO: is the !owner test vestigial? and now confusing, per the comment in the header declaration?
		// (very careful testing required if changed because incorrect managed smoothing stuff has insidious consequences)
		if (!owner || (owner == value.second))
		{
			value.first->inc();
		}
	});
}

//-----------------------------------------------------------------------------
void DfxPlugin::do_idle()
{
	for (size_t parameterIndex = 0; parameterIndex < mParametersChangedInProcessHavePosted.size(); parameterIndex++)
	{
		if (!mParametersChangedInProcessHavePosted[parameterIndex].test_and_set(std::memory_order_relaxed))
		{
			postupdate_parameter(parameterIndex);
		}
	}
	if (!mPresetChangedInProcessHasPosted.test_and_set(std::memory_order_relaxed))
	{
		postupdate_preset();
	}
	if (!mLatencyChangeHasPosted.test_and_set(std::memory_order_relaxed))
	{
		postupdate_latency();
	}
	if (!mTailSizeChangeHasPosted.test_and_set(std::memory_order_relaxed))
	{
		postupdate_tailsize();
	}

#if TARGET_PLUGIN_USES_MIDI
	if (!mMidiLearnChangedInProcessHasPosted.test_and_set(std::memory_order_relaxed))
	{
		postupdate_midilearn();
	}
	if (!mMidiLearnerChangedInProcessHasPosted.test_and_set(std::memory_order_relaxed))
	{
		postupdate_midilearner();
	}
#endif

	idle();
}

//-----------------------------------------------------------------------------
// return the number of audio inputs
unsigned long DfxPlugin::getnuminputs()
{
#ifdef TARGET_API_AUDIOUNIT
	if (Inputs().GetNumberOfElements() > 0)
	{
		return GetStreamFormat(kAudioUnitScope_Input, AudioUnitElement(0)).NumberChannels();
	}
	else
	{
		return 0;
	}
#endif

#ifdef TARGET_API_VST
	return mNumInputs;
#endif

#ifdef TARGET_API_RTAS
	if (IsAS() && mAudioIsRendering)  // XXX checking connections is only valid while rendering
	{
		for (SInt32 ch = 0; ch < GetNumInputs(); ch++)
		{
			DAEConnectionPtr inputConnection = GetInputConnection(ch);
			if (!inputConnection)
			{
				return static_cast<unsigned long>(ch);
			}
		}
	}
	return static_cast<unsigned long>(GetNumInputs());
#endif
}

//-----------------------------------------------------------------------------
// return the number of audio outputs
unsigned long DfxPlugin::getnumoutputs()
{
#ifdef TARGET_API_AUDIOUNIT
	if (Outputs().GetNumberOfElements() > 0)
	{
		return GetStreamFormat(kAudioUnitScope_Output, AudioUnitElement(0)).NumberChannels();
	}
	else
	{
		return 0;
	}
#endif

#ifdef TARGET_API_VST
	return mNumOutputs;
#endif

#ifdef TARGET_API_RTAS
	if (IsAS() && mAudioIsRendering)  // XXX checking connections is only valid while rendering
	{
		for (SInt32 ch = 0; ch < GetNumOutputs(); ch++)
		{
			DAEConnectionPtr outputConnection = GetOutputConnection(ch);
			if (!outputConnection)
			{
				return static_cast<unsigned long>(ch);
			}
		}
	}
	return static_cast<unsigned long>(GetNumOutputs());
#endif
}

//-----------------------------------------------------------------------------
bool DfxPlugin::asymmetricalchannels()
{
	return getnuminputs() != getnumoutputs();
}

//-----------------------------------------------------------------------------
unsigned long DfxPlugin::getmaxframes()
{
#ifdef TARGET_API_AUDIOUNIT
	return GetMaxFramesPerSlice();
#endif

#ifdef TARGET_API_VST
	return static_cast<unsigned long>(getBlockSize());
#endif

#ifdef TARGET_API_RTAS
	return static_cast<unsigned long>(GetMaximumRTASQuantum());
#endif
}

#ifdef TARGET_API_AUDIOUNIT
//-----------------------------------------------------------------------------
static bool operator==(AUChannelInfo const& a, AUChannelInfo const& b) noexcept
{
	return (a.inChannels == b.inChannels) && (a.outChannels == b.outChannels);
}
#endif

//-----------------------------------------------------------------------------
void DfxPlugin::addchannelconfig(short inNumInputs, short inNumOutputs)
{
	ChannelConfig channelConfig;
	channelConfig.inChannels = inNumInputs;
	channelConfig.outChannels = inNumOutputs;
	addchannelconfig(channelConfig);
}

//-----------------------------------------------------------------------------
void DfxPlugin::addchannelconfig(ChannelConfig inChannelConfig)
{
	assert(std::find(mChannelConfigs.cbegin(), mChannelConfigs.cend(), inChannelConfig) == mChannelConfigs.cend());
	assert((inChannelConfig == kChannelConfig_AnyInAnyOut) || ((inChannelConfig.inChannels >= kChannelConfigCount_Any) && (inChannelConfig.outChannels >= kChannelConfigCount_Any)));
#if TARGET_PLUGIN_USES_DSPCORE
	assert((inChannelConfig.inChannels == inChannelConfig.outChannels) || (inChannelConfig.inChannels == 1));
#endif

	mChannelConfigs.push_back(inChannelConfig);
}

//-----------------------------------------------------------------------------
bool DfxPlugin::ischannelcountsupported(unsigned long inNumInputs, unsigned long inNumOutputs) const
{
	if (mChannelConfigs.empty())
	{
#if TARGET_PLUGIN_IS_INSTRUMENT
		return true;  // TODO: this had been our logic in Initialize(), but is it correct?
#else
		return (inNumInputs == inNumOutputs);
#endif
	}

	for (auto const& channelConfig : mChannelConfigs)
	{
		auto const configNumInputs = channelConfig.inChannels;
		auto const configNumOutputs = channelConfig.outChannels;
		// handle the special "wildcard" cases indicated by negative channel count values
		if ((configNumInputs < 0) && (configNumOutputs < 0))
		{
			// test if any number of inputs and outputs is allowed
			if (channelConfig == kChannelConfig_AnyInAnyOut)
			{
				return true;
			}
			// test if any number of ins and outs are allowed, as long as they are equal
			if ((channelConfig == kChannelConfig_AnyMatchedIO) && (inNumInputs == inNumOutputs))
			{
				return true;
			}
			// any other pair of negative values is illegal
			assert(channelConfig == kChannelConfig_AnyMatchedIO);
		}
		// handle literal channel count values (and maybe a wildcard on one of the scopes)
		else
		{
			assert((configNumInputs >= 0) || (configNumInputs == kChannelConfigCount_Any));
			assert((configNumOutputs >= 0) || (configNumOutputs == kChannelConfigCount_Any));
			bool const inputMatch = (configNumInputs == kChannelConfigCount_Any) || (inNumInputs == static_cast<unsigned long>(configNumInputs));
			bool const outputMatch = (configNumOutputs == kChannelConfigCount_Any) || (inNumOutputs == static_cast<unsigned long>(configNumOutputs));
			// if input and output are both allowed in this I/O pair description, then we found a match
			if (inputMatch && outputMatch)
			{
				return true;
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
void DfxPlugin::setlatency_samples(long inSamples)
{
	bool changed = false;
	if (std::holds_alternative<double>(mLatency))
	{
		changed = true;
	}
	else if (*std::get_if<long>(&mLatency) != inSamples)
	{
		changed = true;
	}

	mLatency = inSamples;

	if (changed)
	{
		// defer the notification because it is not realtime-safe
		if (std::this_thread::get_id() == mAudioRenderThreadID)
		{
			mLatencyChangeHasPosted.clear(std::memory_order_relaxed);
		}
		else
		{
			postupdate_latency();
		}
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setlatency_seconds(double inSeconds)
{
	bool changed = false;
	if (std::holds_alternative<long>(mLatency))
	{
		changed = true;
	}
	else if (*std::get_if<double>(&mLatency) != inSeconds)
	{
		changed = true;
	}

	mLatency = inSeconds;

	if (changed)
	{
		// defer the notification because it is not realtime-safe
		if (std::this_thread::get_id() == mAudioRenderThreadID)
		{
			mLatencyChangeHasPosted.clear(std::memory_order_relaxed);
		}
		else
		{
			postupdate_latency();
		}
	}
}

//-----------------------------------------------------------------------------
long DfxPlugin::getlatency_samples() const
{
	if (auto const latencySamples = std::get_if<long>(&mLatency))
	{
		return *latencySamples;
	}
	else
	{
		return std::lround(*std::get_if<double>(&mLatency) * getsamplerate());
	}
}

//-----------------------------------------------------------------------------
double DfxPlugin::getlatency_seconds() const
{
	if (auto const latencySeconds = std::get_if<double>(&mLatency))
	{
		return *latencySeconds;
	}
	else
	{
		return static_cast<double>(*std::get_if<long>(&mLatency)) / getsamplerate();
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::postupdate_latency()
{
#ifdef TARGET_API_AUDIOUNIT
	PropertyChanged(kAudioUnitProperty_Latency, kAudioUnitScope_Global, AudioUnitElement(0));
#endif

#ifdef TARGET_API_VST
	ioChanged();
#endif
}

//-----------------------------------------------------------------------------
void DfxPlugin::settailsize_samples(long inSamples)
{
	bool changed = false;
	if (std::holds_alternative<double>(mTailSize))
	{
		changed = true;
	}
	else if (*std::get_if<long>(&mTailSize) != inSamples)
	{
		changed = true;
	}

	mTailSize = inSamples;

	if (changed)
	{
		// defer the notification because it is not realtime-safe
		if (std::this_thread::get_id() == mAudioRenderThreadID)
		{
			mTailSizeChangeHasPosted.clear(std::memory_order_relaxed);
		}
		else
		{
			postupdate_tailsize();
		}
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::settailsize_seconds(double inSeconds)
{
	bool changed = false;
	if (std::holds_alternative<long>(mTailSize))
	{
		changed = true;
	}
	else if (*std::get_if<double>(&mTailSize) != inSeconds)
	{
		changed = true;
	}

	mTailSize = inSeconds;

	if (changed)
	{
		// defer the notification because it is not realtime-safe
		if (std::this_thread::get_id() == mAudioRenderThreadID)
		{
			mTailSizeChangeHasPosted.clear(std::memory_order_relaxed);
		}
		else
		{
			postupdate_tailsize();
		}
	}
}

//-----------------------------------------------------------------------------
long DfxPlugin::gettailsize_samples() const
{
	if (auto const tailSizeSamples = std::get_if<long>(&mTailSize))
	{
		return *tailSizeSamples;
	}
	else
	{
		return std::lround(*std::get_if<double>(&mTailSize) * getsamplerate());
	}
}

//-----------------------------------------------------------------------------
double DfxPlugin::gettailsize_seconds() const
{
	if (auto const tailSizeSeconds = std::get_if<double>(&mTailSize))
	{
		return *tailSizeSeconds;
	}
	else
	{
		return static_cast<double>(*std::get_if<long>(&mTailSize)) / getsamplerate();
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::postupdate_tailsize()
{
#ifdef TARGET_API_AUDIOUNIT
	PropertyChanged(kAudioUnitProperty_TailTime, kAudioUnitScope_Global, AudioUnitElement(0));
#endif
#ifdef TARGET_API_VST
	noTail(gettailsize_samples() <= 0);
#endif
}

//-----------------------------------------------------------------------------
void DfxPlugin::setInPlaceAudioProcessingAllowed(bool inEnable)
{
#ifdef TARGET_API_AUDIOUNIT
	assert(!IsInitialized());
#elif defined(TARGET_API_VST)
	assert(!mIsInitialized);
#endif

	mInPlaceAudioProcessingAllowed = inEnable;

#ifdef TARGET_API_AUDIOUNIT
	UpdateInPlaceProcessingState();
#endif
}



#pragma mark -
#pragma mark processing

//-----------------------------------------------------------------------------
// this is called once per audio processing block (before doing the processing) 
// in order to try to get musical tempo/time/location information from the host
void DfxPlugin::processtimeinfo()
{
	// default these values to something reasonable in case they are not available from the host
	mTimeInfo.mTempo = 120.0;
	mTimeInfo.mTempoIsValid = false;
	mTimeInfo.mBeatPos = 0.0;
	mTimeInfo.mBeatPosIsValid = false;
	mTimeInfo.mBarPos = 0.0;
	mTimeInfo.mBarPosIsValid = false;
	mTimeInfo.mNumerator = 4.0;
	mTimeInfo.mDenominator = 4.0;
	mTimeInfo.mTimeSignatureIsValid = false;
	mTimeInfo.mSamplesToNextBar = 0;
	mTimeInfo.mSamplesToNextBarIsValid = false;
	mTimeInfo.mPlaybackChanged = false;
	mTimeInfo.mPlaybackIsOccurring = true;


#ifdef TARGET_API_AUDIOUNIT
	OSStatus status;

	Float64 tempo = 120.0;
	Float64 beat = 0.0;
	status = CallHostBeatAndTempo(&beat, &tempo);
	if (status == noErr)
	{
#ifdef DFX_DEBUG_PRINT_MUSICAL_TIME_INFO
fprintf(stderr, "\ntempo = %.2f\nbeat = %.2f\n", tempo, beat);
#endif
		mTimeInfo.mTempoIsValid = true;
		mTimeInfo.mTempo = tempo;
		mTimeInfo.mBeatPosIsValid = true;
		mTimeInfo.mBeatPos = beat;

		mHostCanDoTempo = true;
	}
#ifdef DFX_DEBUG_PRINT_MUSICAL_TIME_INFO
else fprintf(stderr, "CallHostBeatAndTempo() error %ld\n", status);
#endif

	// the number of samples until the next beat from the start sample of the current rendering buffer
//	UInt32 sampleOffsetToNextBeat = 0;
	// the number of beats of the denominator value that contained in the current measure
	Float32 timeSigNumerator = 4.0f;
	// music notational conventions (4 is a quarter note, 8 an eighth note, etc)
	UInt32 timeSigDenominator = 4;
	// the beat that corresponds to the downbeat (first beat) of the current measure
	Float64 currentMeasureDownBeat = 0.0;
	status = CallHostMusicalTimeLocation(nullptr, &timeSigNumerator, &timeSigDenominator, &currentMeasureDownBeat);
	if (status == noErr)
	{
		// get the song beat position of the beginning of the current measure
		mTimeInfo.mBarPosIsValid = true;
		mTimeInfo.mBarPos = currentMeasureDownBeat;
#ifdef DFX_DEBUG_PRINT_MUSICAL_TIME_INFO
fprintf(stderr, "time sig = %.0f/%lu\nmeasure beat = %.2f\n", timeSigNumerator, timeSigDenominator, currentMeasureDownBeat);
#endif
		// get the numerator of the time signature - this is the number of beats per measure
		mTimeInfo.mTimeSignatureIsValid = true;
		mTimeInfo.mNumerator = static_cast<double>(timeSigNumerator);
		mTimeInfo.mDenominator = static_cast<double>(timeSigDenominator);
	}
#ifdef DFX_DEBUG_PRINT_MUSICAL_TIME_INFO
else fprintf(stderr, "CallHostMusicalTimeLocation() error %ld\n", status);
#endif

	Boolean isPlaying = true;
	Boolean transportStateChanged = false;
//	Float64 currentSampleInTimeLine = 0.0;
//	Boolean isCycling = false;
//	Float64 cycleStartBeat = 0.0, cycleEndBeat = 0.0;
//	status = CallHostTransportState(&isPlaying, &transportStateChanged, &currentSampleInTimeLine, &isCycling, &cycleStartBeat, &cycleEndBeat);
	status = CallHostTransportState(&isPlaying, &transportStateChanged, nullptr, nullptr, nullptr, nullptr);
	// determine whether the playback position or state has just changed
	if (status == noErr)
	{
#ifdef DFX_DEBUG_PRINT_MUSICAL_TIME_INFO
fprintf(stderr, "is playing = %s\ntransport changed = %s\n", isPlaying ? "true" : "false", transportStateChanged ? "true" : "false");
#endif
		mTimeInfo.mPlaybackChanged = transportStateChanged;
		mTimeInfo.mPlaybackIsOccurring = isPlaying;
	}
#ifdef DFX_DEBUG_PRINT_MUSICAL_TIME_INFO
else fprintf(stderr, "CallHostTransportState() error %ld\n", status);
#endif
#endif
// TARGET_API_AUDIOUNIT
 

#ifdef TARGET_API_VST
	VstTimeInfo* vstTimeInfo = getTimeInfo(kVstTempoValid 
										   | kVstTransportChanged 
										   | kVstBarsValid 
										   | kVstPpqPosValid 
										   | kVstTimeSigValid);

 	// there's nothing we can do in that case
	if (vstTimeInfo)
	{
		if (kVstTempoValid & vstTimeInfo->flags)
		{
			mTimeInfo.mTempoIsValid = true;
			mTimeInfo.mTempo = vstTimeInfo->tempo;
		}

		// get the song beat position of our precise current location
		if (kVstPpqPosValid & vstTimeInfo->flags)
		{
			mTimeInfo.mBeatPosIsValid = true;
			mTimeInfo.mBeatPos = vstTimeInfo->ppqPos;
		}

		// get the song beat position of the beginning of the current measure
		if (kVstBarsValid & vstTimeInfo->flags)
		{
			mTimeInfo.mBarPosIsValid = true;
			mTimeInfo.mBarPos = vstTimeInfo->barStartPos;
		}

		// get the numerator of the time signature - this is the number of beats per measure
		if (kVstTimeSigValid & vstTimeInfo->flags)
		{
			mTimeInfo.mTimeSignatureIsValid = true;
			mTimeInfo.mNumerator = static_cast<double>(vstTimeInfo->timeSigNumerator);
			mTimeInfo.mDenominator = static_cast<double>(vstTimeInfo->timeSigDenominator);
		}

		// determine whether the playback position or state has just changed
		if (kVstTransportChanged & vstTimeInfo->flags)
		{
			mTimeInfo.mPlaybackChanged = true;
		}

		mTimeInfo.mPlaybackIsOccurring = (kVstTransportPlaying & vstTimeInfo->flags) ? true : false;
	}
#endif
// TARGET_API_VST


	if (mTimeInfo.mTempo <= 0.0)
	{
		mTimeInfo.mTempo = 120.0;
	}
	mTimeInfo.mTempo_BPS = mTimeInfo.mTempo / 60.0;
	mTimeInfo.mSamplesPerBeat = std::lround(getsamplerate() / mTimeInfo.mTempo_BPS);

	if (mTimeInfo.mTempoIsValid && mTimeInfo.mBeatPosIsValid && mTimeInfo.mBarPosIsValid && mTimeInfo.mTimeSignatureIsValid)
	{
		mTimeInfo.mSamplesToNextBarIsValid = true;
	}

	// it will screw up the while loop below bigtime if the numerator isn't a positive number
	if (mTimeInfo.mNumerator <= 0.0)
	{
		mTimeInfo.mNumerator = 4.0;
	}
	if (mTimeInfo.mDenominator <= 0.0)
	{
		mTimeInfo.mDenominator = 4.0;
	}

	// calculate the number of samples frames from now until the next measure begins
	if (mTimeInfo.mSamplesToNextBarIsValid)
	{
		double numBeatsToBar {};
		// calculate the distance in beats to the upcoming measure beginning point
		if (mTimeInfo.mBarPos == mTimeInfo.mBeatPos)
		{
			numBeatsToBar = 0.0;
		}
		else
		{
			numBeatsToBar = mTimeInfo.mBarPos + mTimeInfo.mNumerator - mTimeInfo.mBeatPos;
			// do this stuff because some hosts (Cubase) give kind of wacky barStartPos sometimes
			while (numBeatsToBar < 0.0)
			{
				numBeatsToBar += mTimeInfo.mNumerator;
			}
			while (numBeatsToBar > mTimeInfo.mNumerator)
			{
				numBeatsToBar -= mTimeInfo.mNumerator;
			}
		}

		// convert the value for the distance to the next measure from beats to samples
		mTimeInfo.mSamplesToNextBar = std::lround(numBeatsToBar * getsamplerate() / mTimeInfo.mTempo_BPS);
		// protect against wacky values
		mTimeInfo.mSamplesToNextBar = std::max(mTimeInfo.mSamplesToNextBar, 0L);
	}
}


//-----------------------------------------------------------------------------
// this is called immediately before processing a block of audio
void DfxPlugin::preprocessaudio()
{
	mAudioIsRendering = true;
	mAudioRenderThreadID = std::this_thread::get_id();

#if TARGET_PLUGIN_USES_MIDI
	mMidiState.preprocessEvents();
#endif

	// fetch the latest musical tempo/time/location inforomation from the host
	processtimeinfo();

	// deal with current parameter values for usage during audio processing
	for (long i = 0; i < getnumparameters(); i++)
	{
		mParametersChangedAsOfPreProcess[i] = mParameters[i].setchanged(false);
		mParametersTouchedAsOfPreProcess[i] = mParameters[i].settouched(false);
	}
	do_processparameters();
}

//-----------------------------------------------------------------------------
// this is called immediately after processing a block of audio
void DfxPlugin::postprocessaudio()
{
#if TARGET_PLUGIN_USES_MIDI
	mMidiState.postprocessEvents();
#endif

	mAudioIsRendering = false;
}

//-----------------------------------------------------------------------------
// non-virtual function called to insure that processparameters happens
void DfxPlugin::do_processparameters()
{
	processparameters();

#if TARGET_PLUGIN_USES_DSPCORE
	for (unsigned long ch = 0; ch < getnumoutputs(); ch++)
	{
		getplugincore(ch)->processparameters();
	}
#endif

	if (std::exchange(mIsFirstRenderSinceReset, false))
	{
		std::for_each(mSmoothedAudioValues.cbegin(), mSmoothedAudioValues.cend(), [](auto& value){ value.first->snap(); });
	}
}



#pragma mark -
#pragma mark MIDI

#if TARGET_PLUGIN_USES_MIDI

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_noteon(int inChannel, int inNote, int inVelocity, unsigned long inOffsetFrames)
{
#ifdef DFX_DEBUG_PRINT_MUSIC_EVENTS
fprintf(stderr, "note on:  note = %d, velocity = %d, channel = %d, sample offset = %lu\n", inNote, inVelocity, inChannel, inOffsetFrames);
#endif
	mMidiState.handleNoteOn(inChannel, inNote, inVelocity, inOffsetFrames);
	mDfxSettings->handleNoteOn(inChannel, inNote, inVelocity, inOffsetFrames);
}

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_noteoff(int inChannel, int inNote, int inVelocity, unsigned long inOffsetFrames)
{
#ifdef DFX_DEBUG_PRINT_MUSIC_EVENTS
fprintf(stderr, "note off:  note = %d, velocity = %d, channel = %d, sample offset = %lu\n", inNote, inVelocity, inChannel, inOffsetFrames);
#endif
	mMidiState.handleNoteOff(inChannel, inNote, inVelocity, inOffsetFrames);
	mDfxSettings->handleNoteOff(inChannel, inNote, inVelocity, inOffsetFrames);
}

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_allnotesoff(int inChannel, unsigned long inOffsetFrames)
{
#ifdef DFX_DEBUG_PRINT_MUSIC_EVENTS
fprintf(stderr, "all notes off:  channel = %d, sample offset = %lu\n", inChannel, inOffsetFrames);
#endif
	mMidiState.handleAllNotesOff(inChannel, inOffsetFrames);
	mDfxSettings->handleAllNotesOff(inChannel, inOffsetFrames);
}

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_channelaftertouch(int inChannel, int inValue, unsigned long inOffsetFrames)
{
#ifdef DFX_DEBUG_PRINT_MUSIC_EVENTS
fprintf(stderr, "channel aftertouch:  value = %d, channel = %d, sample offset = %lu\n", inValue, inChannel, inOffsetFrames);
#endif
	mMidiState.handleChannelAftertouch(inChannel, inValue, inOffsetFrames);
	mDfxSettings->handleChannelAftertouch(inChannel, inValue, inOffsetFrames);
}

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_pitchbend(int inChannel, int inValueLSB, int inValueMSB, unsigned long inOffsetFrames)
{
#ifdef DFX_DEBUG_PRINT_MUSIC_EVENTS
fprintf(stderr, "pitchbend:  LSB = %d, MSB = %d, channel = %d, sample offset = %lu\n", inValueLSB, inValueMSB, inChannel, inOffsetFrames);
#endif
	mMidiState.handlePitchBend(inChannel, inValueLSB, inValueMSB, inOffsetFrames);
	mDfxSettings->handlePitchBend(inChannel, inValueLSB, inValueMSB, inOffsetFrames);
}

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_cc(int inChannel, int inControllerNum, int inValue, unsigned long inOffsetFrames)
{
#ifdef DFX_DEBUG_PRINT_MUSIC_EVENTS
fprintf(stderr, "MIDI CC:  controller = 0x%02X, value = %d, channel = %d, sample offset = %lu\n", inControllerNum, inValue, inChannel, inOffsetFrames);
#endif
	mMidiState.handleCC(inChannel, inControllerNum, inValue, inOffsetFrames);
	mDfxSettings->handleCC(inChannel, inControllerNum, inValue, inOffsetFrames);
}

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_programchange(int inChannel, int inProgramNum, unsigned long inOffsetFrames)
{
#ifdef DFX_DEBUG_PRINT_MUSIC_EVENTS
fprintf(stderr, "program change:  program num = %d, channel = %d, sample offset = %lu\n", inProgramNum, inChannel, inOffsetFrames);
#endif
	mMidiState.handleProgramChange(inChannel, inProgramNum, inOffsetFrames);
}

//-----------------------------------------------------------------------------
void DfxPlugin::setmidilearner(long inParameterIndex)
{
	if (auto const assignmentData = settings_getLearningAssignData(inParameterIndex))
	{
		mDfxSettings->setLearner(inParameterIndex, assignmentData->mEventBehaviorFlags, 
								 assignmentData->mDataInt1, assignmentData->mDataInt2, 
								 assignmentData->mDataFloat1, assignmentData->mDataFloat2);
	}
	else if (getparametervaluetype(inParameterIndex) == DfxParam::ValueType::Float)
	{
		mDfxSettings->setLearner(inParameterIndex);
	}
	else
	{
		auto const numStates = getparametermax_i(inParameterIndex) - getparametermin_i(inParameterIndex) + 1;
		mDfxSettings->setLearner(inParameterIndex, dfx::kMidiEventBehaviorFlag_Toggle, numStates);
	}
}

//-----------------------------------------------------------------------------
long DfxPlugin::getmidilearner() const
{
	return mDfxSettings->getLearner();
}

//-----------------------------------------------------------------------------
void DfxPlugin::setmidilearning(bool inLearnMode)
{
	mDfxSettings->setParameterMidiLearn(inLearnMode);
}

//-----------------------------------------------------------------------------
bool DfxPlugin::getmidilearning() const
{
	return mDfxSettings->isLearning();
}

//-----------------------------------------------------------------------------
void DfxPlugin::resetmidilearn()
{
	mDfxSettings->setParameterMidiReset();
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparametermidiassignment(long inParameterIndex, dfx::ParameterAssignment const& inAssignment)
{
	if (inAssignment.mEventType == dfx::MidiEventType::None)
	{
		mDfxSettings->unassignParam(inParameterIndex);
	}
	else
	{
		mDfxSettings->assignParam(inParameterIndex, inAssignment.mEventType, inAssignment.mEventChannel,
								  inAssignment.mEventNum, inAssignment.mEventNum2, 
								  inAssignment.mEventBehaviorFlags, 
								  inAssignment.mDataInt1, inAssignment.mDataInt2, 
								  inAssignment.mDataFloat1, inAssignment.mDataFloat2);
	}
}

//-----------------------------------------------------------------------------
dfx::ParameterAssignment DfxPlugin::getparametermidiassignment(long inParameterIndex) const
{
	return mDfxSettings->getParameterAssignment(inParameterIndex);
}

//-----------------------------------------------------------------------------
void DfxPlugin::setMidiAssignmentsUseChannel(bool inEnable)
{
	mDfxSettings->setUseChannel(inEnable);
}

//-----------------------------------------------------------------------------
bool DfxPlugin::getMidiAssignmentsUseChannel() const
{
	return mDfxSettings->getUseChannel();
}

//-----------------------------------------------------------------------------
void DfxPlugin::setMidiAssignmentsSteal(bool inEnable)
{
	mDfxSettings->setSteal(inEnable);
}

//-----------------------------------------------------------------------------
bool DfxPlugin::getMidiAssignmentsSteal() const
{
	return mDfxSettings->getSteal();
}

//-----------------------------------------------------------------------------
void DfxPlugin::postupdate_midilearn()
{
#ifdef TARGET_API_AUDIOUNIT
	PropertyChanged(dfx::kPluginProperty_MidiLearn, kAudioUnitScope_Global, AudioUnitElement(0));
#endif

#if defined(TARGET_API_VST) && TARGET_PLUGIN_HAS_GUI
	if (auto const guiEditor = dynamic_cast<DfxGuiEditor*>(getEditor()))
	{
		guiEditor->HandleMidiLearnChange();
	}
#endif
}

//-----------------------------------------------------------------------------
void DfxPlugin::postupdate_midilearner()
{
#ifdef TARGET_API_AUDIOUNIT
	PropertyChanged(dfx::kPluginProperty_MidiLearner, kAudioUnitScope_Global, AudioUnitElement(0));
#endif

#if defined(TARGET_API_VST) && TARGET_PLUGIN_HAS_GUI
	if (auto const guiEditor = dynamic_cast<DfxGuiEditor*>(getEditor()))
	{
		guiEditor->HandleMidiLearnerChange();
	}
#endif
}

#endif
// TARGET_PLUGIN_USES_MIDI
