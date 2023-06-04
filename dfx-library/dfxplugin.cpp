/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2023  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 2 of the License, or 
(at your option) any later version.

Destroy FX Library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Destroy FX Library.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org

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
#include <thread>
#include <unordered_set>

#include "dfxmath.h"
#include "dfxmisc.h"

#ifdef TARGET_API_AUDIOUNIT
	#include "dfx-au-utilities.h"
#endif

#if defined(TARGET_API_VST) && TARGET_PLUGIN_HAS_GUI
	#include "dfxguieditor.h"
	[[nodiscard]] extern std::unique_ptr<DfxGuiEditor> DFXGUI_NewEditorInstance(DGEditorListenerInstance inEffectInstance);
#endif

#ifdef TARGET_API_RTAS
	#include "ConvertUtils.h"
	#if TARGET_PLUGIN_HAS_GUI
		extern void* gThisModule;
	#endif
#endif

//#define DFX_DEBUG_PRINT_MUSICAL_TIME_INFO
//#define DFX_DEBUG_PRINT_MUSIC_EVENTS
#if defined(DFX_DEBUG_PRINT_MUSICAL_TIME_INFO) || defined(DFX_DEBUG_PRINT_MUSIC_EVENTS)
	#include <cstdio>
#endif


constexpr char const* const kPresetDefaultName = "default";
constexpr std::chrono::milliseconds kIdleTimerInterval(30);


namespace
{

static std::atomic<bool> sIdleThreadShouldRun {false};
// TODO C++23: [[no_destroy]]
__attribute__((no_destroy)) static std::unique_ptr<std::thread> sIdleThread;  // TODO C++20: use std::jthread
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
			sIdleThread = std::make_unique<std::thread>([]
			{
#if TARGET_OS_MAC
				pthread_setname_np(PLUGIN_NAME_STRING " idle timer");
#endif
				while (sIdleThreadShouldRun)
				{
					{
						std::lock_guard const guard(sIdleClientsLock);
						std::ranges::for_each(sIdleClients, [](auto&& idleClient){ idleClient->do_idle(); });
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
	auto const allClientsCompleted = [&inIdleClient]
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
					, size_t inNumParameters
					, size_t inNumPresets
					) :

// setup the constructors of the inherited base classes, for the appropriate API
#ifdef TARGET_API_AUDIOUNIT
	#if TARGET_PLUGIN_IS_INSTRUMENT
	TARGET_API_BASE_CLASS(inInstance, 0 /*numInputs*/, 1 /*numOutputs*/), 
	#else
	TARGET_API_BASE_CLASS(inInstance), 
	#endif
#endif

#ifdef TARGET_API_VST
	TARGET_API_BASE_CLASS(inInstance, static_cast<VstInt32>(inNumPresets), static_cast<VstInt32>(inNumParameters)), 
#endif
// end API-specific base constructors

	mParameters(inNumParameters),
	mParametersChangedAsOfPreProcess(inNumParameters, false),
	mParametersTouchedAsOfPreProcess(inNumParameters, false),
	mParametersChangedInProcessHavePosted(inNumParameters)
#if TARGET_PLUGIN_USES_DSPCORE
	, mDSPCoreParameterValuesCache(inNumParameters)
#endif
{
	updatesamplerate();  // XXX have it set to something here?

	mPresets.reserve(inNumPresets);
	for (size_t i = 0; i < inNumPresets; i++)
	{
		mPresets.emplace_back(inNumParameters);
	}

	// reset pending notifications
	std::ranges::for_each(mParametersChangedInProcessHavePosted, [](auto& flag){ flag.test_and_set(); });
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

	TARGET_API_BASE_CLASS::setUniqueID(PLUGIN_ID);
	TARGET_API_BASE_CLASS::setNumInputs(VST_NUM_INPUTS);
	TARGET_API_BASE_CLASS::setNumOutputs(VST_NUM_OUTPUTS);

	TARGET_API_BASE_CLASS::setProgram(0);  // set the current preset number to 0

	TARGET_API_BASE_CLASS::noTail(true);  // until the plugin declares otherwise

	// check to see if the host supports sending tempo and time information to VST plugins
	// Note that the VST2 SDK (probably erroneously) wants a non-const string here,
	// so we don't pass a string literal.
	char timeinfo[] = "sendVstTimeInfo";
	mHostCanDoTempo = canHostDo(timeinfo) == 1;

	#if TARGET_PLUGIN_USES_MIDI
	// tell host that we want to use special data chunks for settings storage
	TARGET_API_BASE_CLASS::programsAreChunks();
	#endif

	#if TARGET_PLUGIN_IS_INSTRUMENT
	TARGET_API_BASE_CLASS::isSynth();
	#endif

	#if TARGET_PLUGIN_HAS_GUI
	setEditor(DFXGUI_NewEditorInstance(this).release());
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
	mDfxSettings = std::make_unique<DfxSettings>(PLUGIN_ID, *this, settings_sizeOfExtendedData());
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
void DfxPlugin::do_initialize()
{
	updatesamplerate();
	updatenumchannels();

	initialize();

#ifndef TARGET_API_AUDIOUNIT
	mInputAudioStreams.assign(getnuminputs(), nullptr);
	if (!mInPlaceAudioProcessingAllowed)
	{
		mInputOutOfPlaceAudioBuffers.assign(getnuminputs(), std::vector<float>(getmaxframes(), 0.0f));
		std::ranges::transform(mInputOutOfPlaceAudioBuffers, mInputAudioStreams.begin(),
							   [](auto const& buffer){ return buffer.data(); });
	}
#endif  // !TARGET_API_AUDIOUNIT

#ifdef TARGET_API_VST
	mIsInitialized = true;
#endif

#if TARGET_PLUGIN_USES_DSPCORE
	// flag parameter changes to be picked up for DSP cores, which are all instantiated anew during plugin initialize
	std::ranges::for_each(mParameters, [](auto& parameter){ parameter.setchanged(true); });

	if (asymmetricalchannels())
	{
		assert(getnumoutputs() > getnuminputs());  // the only imbalance we support with DSP cores
	#ifdef TARGET_API_AUDIOUNIT
		mAsymmetricalInputBufferList.Allocate(GetStreamFormat(kAudioUnitScope_Output, 0), static_cast<UInt32>(getmaxframes()));
	#else
		mAsymmetricalInputAudioBuffer.assign(getmaxframes(), 0.0f);
	#endif
	}

	#ifndef TARGET_API_AUDIOUNIT
	cacheDSPCoreParameterValues();  // AU handles this in its Initialize override, since it must occur before the AU SDK base class' implementation
	#endif

	// regenerate the DSP core instances whenever the audio I/O format may have changed
	auto const dspCoreCount = getnumoutputs();
	mDSPCores.clear();
	mDSPCores.reserve(dspCoreCount);
	for (size_t ch = 0; ch < dspCoreCount; ch++)
	{
	#ifdef TARGET_API_AUDIOUNIT
		mDSPCores.push_back(dynamic_cast<DfxPluginCore*>(GetKernel(static_cast<UInt32>(ch))));
	#else
		mDSPCores.emplace_back(dspCoreFactory(ch));
	#endif
		assert(mDSPCores.back());
	}
#endif  // TARGET_PLUGIN_USES_DSPCORE

	std::ranges::for_each(mSmoothedAudioValues, [sr = getsamplerate()](auto& value){ value.first.setSampleRate(sr); });

	do_reset();
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

#if TARGET_PLUGIN_USES_DSPCORE
	mDSPCores.clear();
	#ifdef TARGET_API_AUDIOUNIT
	mAsymmetricalInputBufferList.Deallocate();
	#else
	mAsymmetricalInputAudioBuffer = {};
	#endif
#endif

#ifdef TARGET_API_AUDIOUNIT
	mInputAudioStreams_au = {};
	mOutputAudioStreams_au = {};
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
#if TARGET_PLUGIN_USES_DSPCORE
	cacheDSPCoreParameterValues();
#endif

#ifdef TARGET_API_AUDIOUNIT
	// no need to do this if we're not even in Initialized state 
	// because this will basically happen when we become initialized
	// XXX not true!  because IsInitialized() will only return true *after* the Initialize() call returns
//	if (!IsInitialized())
	{
//		return;
	}
	TARGET_API_BASE_CLASS::Reset(kAudioUnitScope_Global, AudioUnitElement(0));
#endif

	mIsFirstRenderSinceReset = true;
	std::ranges::for_each(mSmoothedAudioValues, [](auto& value){ value.first.snap(); });

#if TARGET_PLUGIN_USES_MIDI
	mMidiState.reset();
#endif

#if TARGET_PLUGIN_USES_DSPCORE && !defined(TARGET_API_AUDIOUNIT)
	for (auto& dspCore : mDSPCores)
	{
		if (dspCore)
		{
			dspCore->reset();
		}
	}
#endif

	reset();
}



#pragma mark -
#pragma mark parameters

//-----------------------------------------------------------------------------
void DfxPlugin::initparameter_f(dfx::ParameterID inParameterID, std::vector<std::string_view> const& initNames, 
								double initValue, double initDefaultValue, 
								double initMin, double initMax, 
								DfxParam::Unit initUnit, DfxParam::Curve initCurve, 
								std::string_view initCustomUnitString)
{
	auto& parameter = getparameterobject(inParameterID);
	parameter.init_f(initNames, initValue, initDefaultValue, initMin, initMax, initUnit, initCurve);
	initpresetsparameter(inParameterID);  // default empty presets with this value
	if (!initCustomUnitString.empty())
	{
		parameter.setcustomunitstring(initCustomUnitString);
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::initparameter_i(dfx::ParameterID inParameterID, std::vector<std::string_view> const& initNames, 
								int64_t initValue, int64_t initDefaultValue, 
								int64_t initMin, int64_t initMax, 
								DfxParam::Unit initUnit, DfxParam::Curve initCurve, 
								std::string_view initCustomUnitString)
{
	auto& parameter = getparameterobject(inParameterID);
	parameter.init_i(initNames, initValue, initDefaultValue, initMin, initMax, initUnit, initCurve);
	initpresetsparameter(inParameterID);  // default empty presets with this value
	if (!initCustomUnitString.empty())
	{
		parameter.setcustomunitstring(initCustomUnitString);
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::initparameter_b(dfx::ParameterID inParameterID, std::vector<std::string_view> const& initNames, 
								bool initValue, bool initDefaultValue, 
								DfxParam::Unit initUnit)
{
	getparameterobject(inParameterID).init_b(initNames, initValue, initDefaultValue, initUnit);
	initpresetsparameter(inParameterID);  // default empty presets with this value
}

//-----------------------------------------------------------------------------
void DfxPlugin::initparameter_b(dfx::ParameterID inParameterID, std::vector<std::string_view> const& initNames, 
								bool initDefaultValue, DfxParam::Unit initUnit)
{
	initparameter_b(inParameterID, initNames, initDefaultValue, initDefaultValue, initUnit);
}

//-----------------------------------------------------------------------------
// this is a shorcut for initializing a parameter that uses integer indexes 
// into an array, with an array of strings representing its values
void DfxPlugin::initparameter_list(dfx::ParameterID inParameterID, std::vector<std::string_view> const& initNames, 
								   int64_t initValue, int64_t initDefaultValue, 
								   int64_t initNumItems, DfxParam::Unit initUnit, 
								   std::string_view initCustomUnitString)
{
	auto& parameter = getparameterobject(inParameterID);
	parameter.init_i(initNames, initValue, initDefaultValue, 0, initNumItems - 1, initUnit, DfxParam::Curve::Stepped);
	parameter.setusevaluestrings(true);  // indicate that we will use custom value display strings
	initpresetsparameter(inParameterID);  // default empty presets with this value
	if (!initCustomUnitString.empty())
	{
		parameter.setcustomunitstring(initCustomUnitString);
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameter(dfx::ParameterID inParameterID, DfxParam::Value inValue)
{
	if (parameterisvalid(inParameterID))
	{
		mParameters[inParameterID].set(inValue);
		update_parameter(inParameterID);  // make the host aware of the parameter change
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameter_f(dfx::ParameterID inParameterID, double inValue)
{
	if (parameterisvalid(inParameterID))
	{
		mParameters[inParameterID].set_f(inValue);
		update_parameter(inParameterID);  // make the host aware of the parameter change
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameter_i(dfx::ParameterID inParameterID, int64_t inValue)
{
	if (parameterisvalid(inParameterID))
	{
		mParameters[inParameterID].set_i(inValue);
		update_parameter(inParameterID);  // make the host aware of the parameter change
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameter_b(dfx::ParameterID inParameterID, bool inValue)
{
	if (parameterisvalid(inParameterID))
	{
		mParameters[inParameterID].set_b(inValue);
		update_parameter(inParameterID);  // make the host aware of the parameter change
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameter_gen(dfx::ParameterID inParameterID, double inValue)
{
	if (parameterisvalid(inParameterID))
	{
		mParameters[inParameterID].set_gen(inValue);
		update_parameter(inParameterID);  // make the host aware of the parameter change
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameterquietly_f(dfx::ParameterID inParameterID, double inValue)
{
	if (parameterisvalid(inParameterID))
	{
		mParameters[inParameterID].setquietly_f(inValue);
		update_parameter(inParameterID);  // make the host aware of the parameter change
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameterquietly_i(dfx::ParameterID inParameterID, int64_t inValue)
{
	if (parameterisvalid(inParameterID))
	{
		mParameters[inParameterID].setquietly_i(inValue);
		update_parameter(inParameterID);  // make the host aware of the parameter change
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameterquietly_b(dfx::ParameterID inParameterID, bool inValue)
{
	if (parameterisvalid(inParameterID))
	{
		mParameters[inParameterID].setquietly_b(inValue);
		update_parameter(inParameterID);  // make the host aware of the parameter change
	}
}

//-----------------------------------------------------------------------------
// randomize the current parameter value
// this takes into account the parameter curve
void DfxPlugin::randomizeparameter(dfx::ParameterID inParameterID)
{
	if (parameterisvalid(inParameterID))
	{
		auto& parameter = mParameters[inParameterID];
		switch (getparametervaluetype(inParameterID))
		{
			case DfxParam::ValueType::Float:
				parameter.set_gen(generateParameterRandomValue<double>());
				break;
			case DfxParam::ValueType::Int:
				parameter.set_i(generateParameterRandomValue(parameter.getmin_i(), parameter.getmax_i()));
				break;
			case DfxParam::ValueType::Boolean:
				// we don't need to worry about a curve for boolean values
				parameter.set_b(generateParameterRandomValue<bool>());
				break;
			default:
				assert(false);
				break;
		}

		update_parameter(inParameterID);  // make the host aware of the parameter change
		postupdate_parameter(inParameterID);  // inform any parameter listeners of the changes
	}
}

//-----------------------------------------------------------------------------
// randomize all of the parameters at once
void DfxPlugin::randomizeparameters()
{
	for (dfx::ParameterID i = 0; i < getnumparameters(); i++)
	{
		if (!hasparameterattribute(i, DfxParam::kAttribute_OmitFromRandomizeAll) && !hasparameterattribute(i, DfxParam::kAttribute_Unused))
		{
			randomizeparameter(i);
		}
	}
}

//-----------------------------------------------------------------------------
// do stuff necessary to inform the host of changes, etc.
void DfxPlugin::update_parameter(dfx::ParameterID inParameterID)
{
	if (!parameterisvalid(inParameterID) || hasparameterattribute(inParameterID, DfxParam::kAttribute_Unused))
	{
		return;
	}

#ifdef TARGET_API_AUDIOUNIT
	// make the global-scope element aware of the parameter's value
	TARGET_API_BASE_CLASS::SetParameter(inParameterID, kAudioUnitScope_Global, AudioUnitElement(0), getparameter_f(inParameterID), 0);
#endif

#ifdef TARGET_API_VST
	auto const vstPresetIndex = getProgram();
	if ((vstPresetIndex >= 0) && presetisvalid(dfx::math::ToIndex(vstPresetIndex)))
	{
		setpresetparameter(dfx::math::ToIndex(vstPresetIndex), inParameterID, getparameter(inParameterID));
	}
	#if TARGET_PLUGIN_HAS_GUI
	// the VST2 editor interface has no real listener mechanism for parameters and therefore 
	// always needs notification pushed in any circumstance where a parameter value changes
	postupdate_parameter(inParameterID);
	#endif
#endif

#ifdef TARGET_API_RTAS
	SetControlValue(dfx::ParameterID_ToRTAS(inParameterID), ConvertToDigiValue(getparameter_gen(inParameterID)));  // XXX yeah do this?
#endif

	parameterChanged(inParameterID);
}

//-----------------------------------------------------------------------------
// this will broadcast a notification to anyone interested (host, GUI, etc.) 
// about a parameter change
void DfxPlugin::postupdate_parameter(dfx::ParameterID inParameterID)
{
	if (!parameterisvalid(inParameterID) || hasparameterattribute(inParameterID, DfxParam::kAttribute_Unused))
	{
		return;
	}

	// defer the notification because it is not realtime-safe
	if (isrenderthread())
	{
		// defer listener notification to later, off the realtime thread
		mParametersChangedInProcessHavePosted[inParameterID].clear(std::memory_order_relaxed);
		return;
	}

#ifdef TARGET_API_AUDIOUNIT
	AUParameterChange_TellListeners(GetComponentInstance(), inParameterID);
#endif

#ifdef TARGET_API_VST
	#if TARGET_PLUGIN_HAS_GUI
	if (auto const guiEditor = dynamic_cast<VSTGUI::AEffGUIEditor*>(getEditor()))
	{
		guiEditor->setParameter(dfx::ParameterID_ToVST(inParameterID), getparameter_gen(inParameterID));
	}
	#endif
#endif

#ifdef TARGET_API_RTAS
	#warning "implementation required?"
#endif
}

//-----------------------------------------------------------------------------
DfxParam::Value DfxPlugin::getparameter(dfx::ParameterID inParameterID) const
{
	if (parameterisvalid(inParameterID))
	{
		return mParameters[inParameterID].get();
	}
	return {};
}

size_t DfxPlugin::getparameter_index(dfx::ParameterID inParameterID) const
{
	auto const value = getparameter_i(inParameterID);
	assert(value >= 0);
	return dfx::math::ToIndex(value);
}

//-----------------------------------------------------------------------------
// return a (hopefully) 0 to 1 scalar version of the parameter's current value
double DfxPlugin::getparameter_scalar(dfx::ParameterID inParameterID) const
{
	return getparameter_scalar(inParameterID, getparameter_f(inParameterID));
}

//-----------------------------------------------------------------------------
double DfxPlugin::getparameter_scalar(dfx::ParameterID inParameterID, double inValue) const
{
	if (parameterisvalid(inParameterID))
	{
		switch (getparameterunit(inParameterID))
		{
			case DfxParam::Unit::Percent:
			case DfxParam::Unit::DryWetMix:
				return inValue * 0.01;
			case DfxParam::Unit::Scalar:
				return inValue;
			// XXX should we not just use contractparametervalue() here?
			default:
				return inValue / mParameters[inParameterID].getmax_f();
		}
	}
	return 0.0;
}

//-----------------------------------------------------------------------------
std::optional<double> DfxPlugin::getparameterifchanged_f(dfx::ParameterID inParameterID) const
{
	if (getparameterchanged(inParameterID))
	{
		return getparameter_f(inParameterID);
	}
	return {};
}

//-----------------------------------------------------------------------------
std::optional<int64_t> DfxPlugin::getparameterifchanged_i(dfx::ParameterID inParameterID) const
{
	if (getparameterchanged(inParameterID))
	{
		return getparameter_i(inParameterID);
	}
	return {};
}

//-----------------------------------------------------------------------------
std::optional<bool> DfxPlugin::getparameterifchanged_b(dfx::ParameterID inParameterID) const
{
	if (getparameterchanged(inParameterID))
	{
		return getparameter_b(inParameterID);
	}
	return {};
}

//-----------------------------------------------------------------------------
std::optional<double> DfxPlugin::getparameterifchanged_scalar(dfx::ParameterID inParameterID) const
{
	if (getparameterchanged(inParameterID))
	{
		return getparameter_scalar(inParameterID);
	}
	return {};
}

//-----------------------------------------------------------------------------
std::optional<double> DfxPlugin::getparameterifchanged_gen(dfx::ParameterID inParameterID) const
{
	if (getparameterchanged(inParameterID))
	{
		return getparameter_gen(inParameterID);
	}
	return {};
}

//-----------------------------------------------------------------------------
std::string DfxPlugin::getparametername(dfx::ParameterID inParameterID) const
{
	if (parameterisvalid(inParameterID))
	{
		return mParameters[inParameterID].getname();
	}
	return {};
}

//-----------------------------------------------------------------------------
std::string DfxPlugin::getparametername(dfx::ParameterID inParameterID, size_t inMaxLength) const
{
	if (parameterisvalid(inParameterID))
	{
		return mParameters[inParameterID].getname(inMaxLength);
	}
	return {};
}

//-----------------------------------------------------------------------------
DfxParam::ValueType DfxPlugin::getparametervaluetype(dfx::ParameterID inParameterID) const
{
	if (parameterisvalid(inParameterID))
	{
		return mParameters[inParameterID].getvaluetype();
	}
	return DfxParam::ValueType::Float;
}

//-----------------------------------------------------------------------------
DfxParam::Unit DfxPlugin::getparameterunit(dfx::ParameterID inParameterID) const
{
	if (parameterisvalid(inParameterID))
	{
		return mParameters[inParameterID].getunit();
	}
	return DfxParam::Unit::Generic;
}

//-----------------------------------------------------------------------------
bool DfxPlugin::setparametervaluestring(dfx::ParameterID inParameterID, int64_t inStringIndex, std::string_view inText)
{
	return getparameterobject(inParameterID).setvaluestring(inStringIndex, inText);
}

//-----------------------------------------------------------------------------
std::optional<std::string> DfxPlugin::getparametervaluestring(dfx::ParameterID inParameterID, int64_t inStringIndex) const
{
	if (parameterisvalid(inParameterID))
	{
		return mParameters[inParameterID].getvaluestring(inStringIndex);
	}
	return {};
}

//-----------------------------------------------------------------------------
void DfxPlugin::addparametergroup(std::string const& inName, std::vector<dfx::ParameterID> const& inParameterIndices)
{
	assert(!inName.empty());
	assert(!inParameterIndices.empty());
	assert(std::ranges::none_of(mParameterGroups, [&inName](auto const& item){ return (item.first == inName); }));
	assert(std::ranges::none_of(inParameterIndices, [this](auto index){ return getparametergroup(index).has_value(); }));
	assert(std::ranges::all_of(inParameterIndices, std::bind_front(&DfxPlugin::parameterisvalid, this)));
	assert(std::unordered_set(inParameterIndices.cbegin(), inParameterIndices.cend()).size() == inParameterIndices.size());

	mParameterGroups.emplace_back(inName, std::set(inParameterIndices.cbegin(), inParameterIndices.cend()));
}

//-----------------------------------------------------------------------------
std::optional<size_t> DfxPlugin::getparametergroup(dfx::ParameterID inParameterID) const
{
	auto const foundGroup = std::ranges::find_if(mParameterGroups, [inParameterID](auto const& group)
	{
		auto const& indices = group.second;
		return indices.contains(inParameterID);
	});
	if (foundGroup != mParameterGroups.cend())
	{
		return static_cast<size_t>(std::ranges::distance(mParameterGroups.cbegin(), foundGroup));
	}
	return {};
}

//-----------------------------------------------------------------------------
std::string DfxPlugin::getparametergroupname(size_t inGroupIndex) const
{
	if (inGroupIndex >= mParameterGroups.size())
	{
		return {};
	}
	return mParameterGroups[inGroupIndex].first;
}

//-----------------------------------------------------------------------------
bool DfxPlugin::getparameterchanged(dfx::ParameterID inParameterID) const
{
	assert(isrenderthread());  // only valid during audio rendering
	if (parameterisvalid(inParameterID))
	{
		return mParametersChangedAsOfPreProcess[inParameterID];
	}
	return false;
}

//-----------------------------------------------------------------------------
bool DfxPlugin::getparametertouched(dfx::ParameterID inParameterID) const
{
	assert(isrenderthread());  // only valid during audio rendering
	if (parameterisvalid(inParameterID))
	{
		return mParametersTouchedAsOfPreProcess[inParameterID];
	}
	return false;
}

//-----------------------------------------------------------------------------
bool DfxPlugin::hasparameterattribute(dfx::ParameterID inParameterID, DfxParam::Attribute inFlag) const
{
	assert(std::bitset<sizeof(inFlag) * CHAR_BIT>(inFlag).count() == 1);
	return getparameterattributes(inParameterID) & inFlag;
}

//-----------------------------------------------------------------------------
void DfxPlugin::addparameterattributes(dfx::ParameterID inParameterID, DfxParam::Attribute inFlags)
{
	auto& parameter = getparameterobject(inParameterID);
	parameter.setattributes(parameter.getattributes() | inFlags);
}

//-----------------------------------------------------------------------------
// convenience methods for expanding and contracting parameter values 
// using the min/max/curvetype/curvespec/etc. settings of a given parameter
double DfxPlugin::expandparametervalue(dfx::ParameterID inParameterID, double genValue) const
{
	if (parameterisvalid(inParameterID))
	{
		return mParameters[inParameterID].expand(genValue);
	}
	return 0.0;
}
//-----------------------------------------------------------------------------
double DfxPlugin::contractparametervalue(dfx::ParameterID inParameterID, double realValue) const
{
	if (parameterisvalid(inParameterID))
	{
		return mParameters[inParameterID].contract(realValue);
	}
	return 0.0;
}

//-----------------------------------------------------------------------------
DfxParam& DfxPlugin::getparameterobject(dfx::ParameterID inParameterID)
{
	assert(parameterisvalid(inParameterID));
	return mParameters.at(inParameterID);
}



#pragma mark -
#pragma mark presets

//-----------------------------------------------------------------------------
// whether or not the index is a valid preset
bool DfxPlugin::presetisvalid(size_t inPresetIndex) const noexcept
{
	return (inPresetIndex < getnumpresets());
}

//-----------------------------------------------------------------------------
// whether or not the index is a valid preset with a valid name
// this is mostly just for Audio Unit
bool DfxPlugin::presetnameisvalid(size_t inPresetIndex) const
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
bool DfxPlugin::loadpreset(size_t inPresetIndex)
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
	TARGET_API_BASE_CLASS::setProgram(static_cast<VstInt32>(inPresetIndex));
#endif

	for (dfx::ParameterID i = 0; i < getnumparameters(); i++)
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

	if (isrenderthread())
	{
		mPresetChangedInProcessHasPosted.clear(std::memory_order_relaxed);
		return;
	}

#ifdef TARGET_API_AUDIOUNIT
	AUPreset au_preset {};
	au_preset.presetNumber = static_cast<SInt32>(getcurrentpresetnum());
	au_preset.presetName = getpresetcfname(getcurrentpresetnum());
	SetAFactoryPresetAsCurrent(au_preset);
	PropertyChanged(kAudioUnitProperty_PresentPreset, kAudioUnitScope_Global, AudioUnitElement(0));
	PropertyChanged(kAudioUnitProperty_CurrentPreset, kAudioUnitScope_Global, AudioUnitElement(0));
#endif

#ifdef TARGET_API_VST
	TARGET_API_BASE_CLASS::setProgram(static_cast<VstInt32>(getcurrentpresetnum()));
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
void DfxPlugin::initpresetsparameter(dfx::ParameterID inParameterID)
{
	// first fill in the presets with the init settings 
	// so that there are no "stale" unset values
	for (size_t i = 0; i < getnumpresets(); i++)
	{
		if (!presetnameisvalid(i))  // only if it's an "empty" preset
		{
			setpresetparameter(i, inParameterID, getparameter(inParameterID));
		}
	}
}

//-----------------------------------------------------------------------------
DfxParam::Value DfxPlugin::getpresetparameter(size_t inPresetIndex, dfx::ParameterID inParameterID) const
{
	if (parameterisvalid(inParameterID) && presetisvalid(inPresetIndex))
	{
		return mPresets[inPresetIndex].getvalue(inParameterID);
	}
	return {};
}

//-----------------------------------------------------------------------------
double DfxPlugin::getpresetparameter_f(size_t inPresetIndex, dfx::ParameterID inParameterID) const
{
	if (parameterisvalid(inParameterID) && presetisvalid(inPresetIndex))
	{
		return mParameters[inParameterID].derive_f(mPresets[inPresetIndex].getvalue(inParameterID));
	}
	return 0.0;
}

//-----------------------------------------------------------------------------
int64_t DfxPlugin::getpresetparameter_i(size_t inPresetIndex, dfx::ParameterID inParameterID) const
{
	if (parameterisvalid(inParameterID) && presetisvalid(inPresetIndex))
	{
		return mParameters[inParameterID].derive_i(mPresets[inPresetIndex].getvalue(inParameterID));
	}
	return 0;
}

//-----------------------------------------------------------------------------
bool DfxPlugin::getpresetparameter_b(size_t inPresetIndex, dfx::ParameterID inParameterID) const
{
	if (parameterisvalid(inParameterID) && presetisvalid(inPresetIndex))
	{
		return mParameters[inParameterID].derive_b(mPresets[inPresetIndex].getvalue(inParameterID));
	}
	return false;
}

//-----------------------------------------------------------------------------
void DfxPlugin::setpresetparameter(size_t inPresetIndex, dfx::ParameterID inParameterID, DfxParam::Value inValue)
{
	if (parameterisvalid(inParameterID) && presetisvalid(inPresetIndex))
	{
		mPresets[inPresetIndex].setvalue(inParameterID, inValue);
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setpresetparameter_f(size_t inPresetIndex, dfx::ParameterID inParameterID, double inValue)
{
	if (parameterisvalid(inParameterID) && presetisvalid(inPresetIndex))
	{
		auto const paramValue = mParameters[inParameterID].pack_f(inValue);
		mPresets[inPresetIndex].setvalue(inParameterID, paramValue);
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setpresetparameter_i(size_t inPresetIndex, dfx::ParameterID inParameterID, int64_t inValue)
{
	if (parameterisvalid(inParameterID) && presetisvalid(inPresetIndex))
	{
		auto const paramValue = mParameters[inParameterID].pack_i(inValue);
		mPresets[inPresetIndex].setvalue(inParameterID, paramValue);
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setpresetparameter_b(size_t inPresetIndex, dfx::ParameterID inParameterID, bool inValue)
{
	if (parameterisvalid(inParameterID) && presetisvalid(inPresetIndex))
	{
		auto const paramValue = mParameters[inParameterID].pack_b(inValue);
		mPresets[inPresetIndex].setvalue(inParameterID, paramValue);
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setpresetparameter_gen(size_t inPresetIndex, dfx::ParameterID inParameterID, double inValue)
{
	if (parameterisvalid(inParameterID) && presetisvalid(inPresetIndex))
	{
		auto const paramValue = mParameters[inParameterID].pack_f(expandparametervalue(inParameterID, inValue));
		mPresets[inPresetIndex].setvalue(inParameterID, paramValue);
	}
}

//-----------------------------------------------------------------------------
// set the text of a preset name
void DfxPlugin::setpresetname(size_t inPresetIndex, std::string_view inText)
{
	assert(!inText.empty());

	if (presetisvalid(inPresetIndex))
	{
		mPresets[inPresetIndex].setname(inText);
	}
}

//-----------------------------------------------------------------------------
// get a copy of the text of a preset name
std::string DfxPlugin::getpresetname(size_t inPresetIndex) const
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
CFStringRef DfxPlugin::getpresetcfname(size_t inPresetIndex) const
{
	if (presetisvalid(inPresetIndex))
	{
		return mPresets[inPresetIndex].getcfname();
	}
	return nullptr;
}
#endif

//-----------------------------------------------------------------------------
bool DfxPlugin::settingsMinimalValidate(void const* inData, size_t inBufferSize) const noexcept
{
#if TARGET_PLUGIN_USES_MIDI
	return mDfxSettings->minimalValidate(inData, inBufferSize);
#else
	return false;
#endif
}



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
		if (mAUElementsHaveBeenCreated)
		{
			// assume that sample-specified properties change in absolute duration when sample rate changes
			if (auto const latencySamples = std::get_if<size_t>(&mLatency); latencySamples && (*latencySamples != 0))
			{
				postupdate_latency();
			}
			if (auto const tailSizeSamples = std::get_if<size_t>(&mTailSize); tailSizeSamples && (*tailSizeSamples != 0))
			{
				postupdate_tailsize();
			}
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
unsigned int DfxPlugin::getpluginversion() const noexcept
{
	return dfx::CompositePluginVersionNumberValue();
}

//-----------------------------------------------------------------------------
void DfxPlugin::registerSmoothedAudioValue(dfx::ISmoothedValue& smoothedValue, DfxPluginCore* owner)
{
	mSmoothedAudioValues.emplace_back(smoothedValue, owner);
	if (auto const sampleRate = getsamplerate(); sampleRate > 0.)
	{
		smoothedValue.setSampleRate(sampleRate);
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::unregisterAllSmoothedAudioValues(DfxPluginCore& owner)
{
	std::erase_if(mSmoothedAudioValues, [&owner](auto const& value)
	{
		return (&owner == value.second);
	});
}

//-----------------------------------------------------------------------------
void DfxPlugin::incrementSmoothedAudioValues(DfxPluginCore* owner)
{
	std::ranges::for_each(mSmoothedAudioValues, [owner](auto& value)
	{
		// TODO: is the !owner test vestigial? and now confusing, per the comment in the header declaration?
		// (very careful testing required if changed because incorrect managed smoothing stuff has insidious consequences)
		if (!owner || (owner == value.second))
		{
			value.first.inc();
		}
	});
}

//-----------------------------------------------------------------------------
std::optional<double> DfxPlugin::getSmoothedAudioValueTime() const
{
	// HACK: arbitrarily grabbing the first value's smoothing time, which is fine enough for our use cases
	// TODO: thread safety
	return mSmoothedAudioValues.empty() ? std::nullopt : std::make_optional(mSmoothedAudioValues.front().first.getSmoothingTime());
}

//-----------------------------------------------------------------------------
void DfxPlugin::setSmoothedAudioValueTime(double inSmoothingTimeInSeconds)
{
	// TODO: thread safety
	std::ranges::for_each(mSmoothedAudioValues, [inSmoothingTimeInSeconds](auto& value)
	{
		value.first.setSmoothingTime(inSmoothingTimeInSeconds);
	});
}

//-----------------------------------------------------------------------------
void DfxPlugin::do_idle()
{
	for (dfx::ParameterID parameterIndex = 0; parameterIndex < mParametersChangedInProcessHavePosted.size(); parameterIndex++)
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
size_t DfxPlugin::getnuminputs()
{
#ifdef TARGET_API_AUDIOUNIT
	if (Inputs().GetNumberOfElements() > 0)
	{
		return GetStreamFormat(kAudioUnitScope_Input, AudioUnitElement(0)).mChannelsPerFrame;
	}
	return 0;
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
				return dfx::math::ToIndex(ch);
			}
		}
	}
	return dfx::math::ToUnsigned(GetNumInputs());
#endif
}

//-----------------------------------------------------------------------------
// return the number of audio outputs
size_t DfxPlugin::getnumoutputs()
{
#ifdef TARGET_API_AUDIOUNIT
	if (Outputs().GetNumberOfElements() > 0)
	{
		return GetStreamFormat(kAudioUnitScope_Output, AudioUnitElement(0)).mChannelsPerFrame;
	}
	return 0;
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
				return dfx::math::ToIndex(ch);
			}
		}
	}
	return dfx::math::ToUnsigned(GetNumOutputs());
#endif
}

//-----------------------------------------------------------------------------
bool DfxPlugin::asymmetricalchannels()
{
	return getnuminputs() != getnumoutputs();
}

//-----------------------------------------------------------------------------
size_t DfxPlugin::getmaxframes()
{
#ifdef TARGET_API_AUDIOUNIT
	return GetMaxFramesPerSlice();
#endif

#ifdef TARGET_API_VST
	return dfx::math::ToUnsigned(getBlockSize());
#endif

#ifdef TARGET_API_RTAS
	return dfx::math::ToUnsigned(GetMaximumRTASQuantum());
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
	// TODO C++23: std::ranges::contains
	assert(std::ranges::find(mChannelConfigs, inChannelConfig) == mChannelConfigs.cend());
	assert((inChannelConfig == kChannelConfig_AnyInAnyOut) || ((inChannelConfig.inChannels >= kChannelConfigCount_Any) && (inChannelConfig.outChannels >= kChannelConfigCount_Any)));
#if TARGET_PLUGIN_USES_DSPCORE
	assert((inChannelConfig.inChannels == inChannelConfig.outChannels) || (inChannelConfig.inChannels == 1));
#endif

	mChannelConfigs.push_back(inChannelConfig);
}

//-----------------------------------------------------------------------------
bool DfxPlugin::ischannelcountsupported(size_t inNumInputs, size_t inNumOutputs) const
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
			bool const inputMatch = (configNumInputs == kChannelConfigCount_Any) || (inNumInputs == static_cast<size_t>(configNumInputs));
			bool const outputMatch = (configNumOutputs == kChannelConfigCount_Any) || (inNumOutputs == static_cast<size_t>(configNumOutputs));
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
void DfxPlugin::setlatency_samples(size_t inSampleFrames)
{
	bool changed = false;
	if (std::holds_alternative<double>(mLatency))
	{
		changed = true;
	}
	else if (*std::get_if<size_t>(&mLatency) != inSampleFrames)
	{
		changed = true;
	}

	mLatency = inSampleFrames;

	if (changed)
	{
		// defer the notification because it is not realtime-safe
		if (isrenderthread())
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
	if (std::holds_alternative<size_t>(mLatency))
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
		if (isrenderthread())
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
size_t DfxPlugin::getlatency_samples() const
{
	if (auto const latencySamples = std::get_if<size_t>(&mLatency))
	{
		return *latencySamples;
	}
	else
	{
		return dfx::math::ToUnsigned(std::lround(*std::get_if<double>(&mLatency) * getsamplerate()));
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
		return static_cast<double>(*std::get_if<size_t>(&mLatency)) / getsamplerate();
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
void DfxPlugin::settailsize_samples(size_t inSampleFrames)
{
	bool changed = false;
	if (std::holds_alternative<double>(mTailSize))
	{
		changed = true;
	}
	else if (*std::get_if<size_t>(&mTailSize) != inSampleFrames)
	{
		changed = true;
	}

	mTailSize = inSampleFrames;

	if (changed)
	{
		// defer the notification because it is not realtime-safe
		if (isrenderthread())
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
	if (std::holds_alternative<size_t>(mTailSize))
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
		if (isrenderthread())
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
size_t DfxPlugin::gettailsize_samples() const
{
	if (auto const tailSizeSamples = std::get_if<size_t>(&mTailSize))
	{
		return *tailSizeSamples;
	}
	else
	{
		return dfx::math::ToUnsigned(std::lround(*std::get_if<double>(&mTailSize) * getsamplerate()));
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
		return static_cast<double>(*std::get_if<size_t>(&mTailSize)) / getsamplerate();
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
double DfxPlugin::TimeInfo::samplesPerBeat(double inTempoBPS, double inSampleRate)
{
	assert(inTempoBPS > 0.);
	assert(inSampleRate > 0.);
	return inSampleRate / inTempoBPS;
}

//-----------------------------------------------------------------------------
std::optional<double> DfxPlugin::TimeInfo::timeSignatureNumerator() const noexcept
{
	return mTimeSignature ? std::make_optional(mTimeSignature->mNumerator) : std::nullopt;
}

//-----------------------------------------------------------------------------
std::optional<double> DfxPlugin::TimeInfo::timeSignatureDenominator() const noexcept
{
	return mTimeSignature ? std::make_optional(mTimeSignature->mDenominator) : std::nullopt;
}

//-----------------------------------------------------------------------------
// this is called once per audio processing block (before doing the processing) 
// in order to try to get musical tempo/time/location information from the host
void DfxPlugin::processtimeinfo()
{
	mTimeInfo = {};

	constexpr auto bpmToBPS = [](double tempoBPM)
	{
		return tempoBPM / 60.;
	};


#ifdef TARGET_API_AUDIOUNIT
	Float64 tempo {};
	Float64 beat {};
	auto status = CallHostBeatAndTempo(&beat, &tempo);
	if (status == noErr)
	{
#ifdef DFX_DEBUG_PRINT_MUSICAL_TIME_INFO
std::fprintf(stderr, "\ntempo = %.2f\nbeat = %.2f\n", tempo, beat);
#endif
		mTimeInfo.mTempoBPS = bpmToBPS(tempo);
		mTimeInfo.mBeatPos = beat;

		mHostCanDoTempo = true;
	}
#ifdef DFX_DEBUG_PRINT_MUSICAL_TIME_INFO
else std::fprintf(stderr, "CallHostBeatAndTempo() error %ld\n", status);
#endif

	// the number of samples until the next beat from the start sample of the current rendering buffer
//	UInt32 sampleOffsetToNextBeat {};
	// the number of beats of the denominator value that contained in the current measure
	Float32 timeSigNumerator {};
	// music notational conventions (4 is a quarter note, 8 an eighth note, etc)
	UInt32 timeSigDenominator {};
	// the beat that corresponds to the downbeat (first beat) of the current measure
	Float64 currentMeasureDownBeat {};
	status = CallHostMusicalTimeLocation(nullptr, &timeSigNumerator, &timeSigDenominator, &currentMeasureDownBeat);
	if (status == noErr)
	{
		// get the song beat position of the beginning of the current measure
		mTimeInfo.mBarPos = currentMeasureDownBeat;
#ifdef DFX_DEBUG_PRINT_MUSICAL_TIME_INFO
std::fprintf(stderr, "time sig = %.0f/%lu\nmeasure beat = %.2f\n", timeSigNumerator, timeSigDenominator, currentMeasureDownBeat);
#endif
		// get the numerator of the time signature - this is the number of beats per measure
		mTimeInfo.mTimeSignature =
		{
			.mNumerator = static_cast<double>(timeSigNumerator),
			.mDenominator = static_cast<double>(timeSigDenominator)
		};
	}
#ifdef DFX_DEBUG_PRINT_MUSICAL_TIME_INFO
else std::fprintf(stderr, "CallHostMusicalTimeLocation() error %ld\n", status);
#endif

	Boolean isPlaying {};
	Boolean transportStateChanged {};
//	Float64 currentSampleInTimeLine {};
//	Boolean isCycling {};
//	Float64 cycleStartBeat {}, cycleEndBeat {};
	status = CallHostTransportState(&isPlaying, &transportStateChanged, nullptr, nullptr, nullptr, nullptr);
	// determine whether the playback position or state has just changed
	if (status == noErr)
	{
#ifdef DFX_DEBUG_PRINT_MUSICAL_TIME_INFO
std::fprintf(stderr, "is playing = %s\ntransport changed = %s\n", isPlaying ? "true" : "false", transportStateChanged ? "true" : "false");
#endif
		mTimeInfo.mPlaybackChanged = transportStateChanged;
		mTimeInfo.mPlaybackIsOccurring = isPlaying;
	}
#ifdef DFX_DEBUG_PRINT_MUSICAL_TIME_INFO
else std::fprintf(stderr, "CallHostTransportState() error %ld\n", status);
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
			mTimeInfo.mTempoBPS = bpmToBPS(vstTimeInfo->tempo);
		}

		// get the song beat position of our precise current location
		if (kVstPpqPosValid & vstTimeInfo->flags)
		{
			mTimeInfo.mBeatPos = vstTimeInfo->ppqPos;
		}

		// get the song beat position of the beginning of the current measure
		if (kVstBarsValid & vstTimeInfo->flags)
		{
			mTimeInfo.mBarPos = vstTimeInfo->barStartPos;
		}

		// get the numerator of the time signature - this is the number of beats per measure
		if (kVstTimeSigValid & vstTimeInfo->flags)
		{
			mTimeInfo.mTimeSignature =
			{
				.mNumerator = static_cast<double>(vstTimeInfo->timeSigNumerator),
				.mDenominator = static_cast<double>(vstTimeInfo->timeSigDenominator)
			};
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


	// check for lies
	if (mTimeInfo.mTempoBPS && (*mTimeInfo.mTempoBPS <= 0.))
	{
		mTimeInfo.mTempoBPS.reset();
	}
	if (mTimeInfo.mTimeSignature && ((mTimeInfo.mTimeSignature->mNumerator <= 0.) || (mTimeInfo.mTimeSignature->mDenominator <= 0.)))
	{
		mTimeInfo.mTimeSignature.reset();
	}

	if (mTimeInfo.mTempoBPS)
	{
		mTimeInfo.mSamplesPerBeat = TimeInfo::samplesPerBeat(*mTimeInfo.mTempoBPS, getsamplerate());
	}

	// calculate the number of samples frames from the start of this audio render until the next measure begins
	if (mTimeInfo.mSamplesPerBeat && mTimeInfo.mBeatPos && mTimeInfo.mBarPos && mTimeInfo.mTimeSignature)
	{
		// calculate the distance in beats to the upcoming measure beginning point
		if (*mTimeInfo.mBeatPos == *mTimeInfo.mBarPos)
		{
			mTimeInfo.mSamplesToNextBar = 0.;
		}
		else
		{
			auto numBeatsToBar = *mTimeInfo.mBarPos + mTimeInfo.mTimeSignature->mNumerator - *mTimeInfo.mBeatPos;
			// do this stuff because some hosts (Cubase) give kind of wacky barStartPos sometimes
			while (numBeatsToBar < 0.)
			{
				numBeatsToBar += mTimeInfo.mTimeSignature->mNumerator;
			}
			while (numBeatsToBar > mTimeInfo.mTimeSignature->mNumerator)
			{
				numBeatsToBar -= mTimeInfo.mTimeSignature->mNumerator;
			}

			// convert the value for the distance to the next measure from beats to samples
			mTimeInfo.mSamplesToNextBar = std::max(numBeatsToBar * *mTimeInfo.mSamplesPerBeat, 0.);
		}
	}
}


//-----------------------------------------------------------------------------
// this is called immediately before processing a block of audio
void DfxPlugin::preprocessaudio(size_t inNumFrames)
{
	assert(inNumFrames <= getmaxframes());
	assert(inNumFrames > 0);

	mAudioIsRendering = true;
	mAudioRenderThreadID = std::this_thread::get_id();

#if TARGET_PLUGIN_USES_MIDI
	mMidiState.preprocessEvents(inNumFrames);
#endif

#if TARGET_PLUGIN_USES_DSPCORE
	cacheDSPCoreParameterValues();
#endif

	// fetch the latest musical tempo/time/location information from the host
	processtimeinfo();

	// deal with current parameter values for usage during audio processing
	for (size_t i = 0; i < getnumparameters(); i++)
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
	for (size_t ch = 0; ch < getnumoutputs(); ch++)
	{
		getplugincore(ch)->processparameters();
	}
#endif

	if (std::exchange(mIsFirstRenderSinceReset, false))
	{
		std::ranges::for_each(mSmoothedAudioValues, [](auto& value){ value.first.snap(); });
	}
}

//-----------------------------------------------------------------------------
bool DfxPlugin::isrenderthread() const noexcept
{
	return (std::this_thread::get_id() == mAudioRenderThreadID);
}

#if TARGET_PLUGIN_USES_DSPCORE
//-----------------------------------------------------------------------------
double DfxPlugin::getdspcoreparameter_gen(dfx::ParameterID inParameterID) const
{
	if (parameterisvalid(inParameterID))
	{
		return contractparametervalue(inParameterID, mParameters[inParameterID].derive_f(mDSPCoreParameterValuesCache[inParameterID]));
	}
	return 0.;
}

//-----------------------------------------------------------------------------
double DfxPlugin::getdspcoreparameter_scalar(dfx::ParameterID inParameterID) const
{
	if (parameterisvalid(inParameterID))
	{
		return getparameter_scalar(inParameterID, mParameters[inParameterID].derive_f(mDSPCoreParameterValuesCache[inParameterID]));
	}
	return 0.;
}

//-----------------------------------------------------------------------------
void DfxPlugin::cacheDSPCoreParameterValues()
{
	for (dfx::ParameterID i = 0; i < getnumparameters(); i++)
	{
		mDSPCoreParameterValuesCache[i] = getparameter(i);
	}
}

//-----------------------------------------------------------------------------
DfxPluginCore* DfxPlugin::getplugincore(size_t inChannel) const
{
	if (inChannel < mDSPCores.size())
	{
#ifdef TARGET_API_AUDIOUNIT
		return mDSPCores[inChannel];
#else
		return mDSPCores[inChannel].get();
#endif
	}
	return nullptr;
}
#endif



#pragma mark -
#pragma mark MIDI

#if TARGET_PLUGIN_USES_MIDI

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_noteon(int inChannel, int inNote, int inVelocity, size_t inOffsetFrames)
{
#ifdef DFX_DEBUG_PRINT_MUSIC_EVENTS
std::fprintf(stderr, "note on:  note = %d, velocity = %d, channel = %d, sample offset = %zu\n", inNote, inVelocity, inChannel, inOffsetFrames);
#endif
	mMidiState.handleNoteOn(inChannel, inNote, inVelocity, inOffsetFrames);
	mDfxSettings->handleNoteOn(inChannel, inNote, inVelocity, inOffsetFrames);
}

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_noteoff(int inChannel, int inNote, int inVelocity, size_t inOffsetFrames)
{
#ifdef DFX_DEBUG_PRINT_MUSIC_EVENTS
std::fprintf(stderr, "note off:  note = %d, velocity = %d, channel = %d, sample offset = %zu\n", inNote, inVelocity, inChannel, inOffsetFrames);
#endif
	mMidiState.handleNoteOff(inChannel, inNote, inVelocity, inOffsetFrames);
	mDfxSettings->handleNoteOff(inChannel, inNote, inVelocity, inOffsetFrames);
}

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_allnotesoff(int inChannel, size_t inOffsetFrames)
{
#ifdef DFX_DEBUG_PRINT_MUSIC_EVENTS
std::fprintf(stderr, "all notes off:  channel = %d, sample offset = %zu\n", inChannel, inOffsetFrames);
#endif
	mMidiState.handleAllNotesOff(inChannel, inOffsetFrames);
	mDfxSettings->handleAllNotesOff(inChannel, inOffsetFrames);
}

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_channelaftertouch(int inChannel, int inValue, size_t inOffsetFrames)
{
#ifdef DFX_DEBUG_PRINT_MUSIC_EVENTS
std::fprintf(stderr, "channel aftertouch:  value = %d, channel = %d, sample offset = %zu\n", inValue, inChannel, inOffsetFrames);
#endif
	mMidiState.handleChannelAftertouch(inChannel, inValue, inOffsetFrames);
	mDfxSettings->handleChannelAftertouch(inChannel, inValue, inOffsetFrames);
}

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_pitchbend(int inChannel, int inValueLSB, int inValueMSB, size_t inOffsetFrames)
{
#ifdef DFX_DEBUG_PRINT_MUSIC_EVENTS
std::fprintf(stderr, "pitchbend:  LSB = %d, MSB = %d, channel = %d, sample offset = %zu\n", inValueLSB, inValueMSB, inChannel, inOffsetFrames);
#endif
	mMidiState.handlePitchBend(inChannel, inValueLSB, inValueMSB, inOffsetFrames);
	mDfxSettings->handlePitchBend(inChannel, inValueLSB, inValueMSB, inOffsetFrames);
}

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_cc(int inChannel, int inControllerNum, int inValue, size_t inOffsetFrames)
{
#ifdef DFX_DEBUG_PRINT_MUSIC_EVENTS
std::fprintf(stderr, "MIDI CC:  controller = 0x%02X, value = %d, channel = %d, sample offset = %zu\n", inControllerNum, inValue, inChannel, inOffsetFrames);
#endif
	mMidiState.handleCC(inChannel, inControllerNum, inValue, inOffsetFrames);
	mDfxSettings->handleCC(inChannel, inControllerNum, inValue, inOffsetFrames);
}

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_programchange(int inChannel, int inProgramNum, size_t inOffsetFrames)
{
#ifdef DFX_DEBUG_PRINT_MUSIC_EVENTS
std::fprintf(stderr, "program change:  program num = %d, channel = %d, sample offset = %zu\n", inProgramNum, inChannel, inOffsetFrames);
#endif
	mMidiState.handleProgramChange(inChannel, inProgramNum, inOffsetFrames);
}

//-----------------------------------------------------------------------------
void DfxPlugin::setmidilearner(dfx::ParameterID inParameterID)
{
	if (auto const assignmentData = settings_getLearningAssignData(inParameterID))
	{
		mDfxSettings->setLearner(inParameterID, assignmentData->mEventBehaviorFlags, 
								 assignmentData->mDataInt1, assignmentData->mDataInt2, 
								 assignmentData->mDataFloat1, assignmentData->mDataFloat2);
	}
	else if (getparametervaluetype(inParameterID) == DfxParam::ValueType::Float)
	{
		mDfxSettings->setLearner(inParameterID);
	}
	else
	{
		auto const numStates = static_cast<int>(getparametermax_i(inParameterID) - getparametermin_i(inParameterID) + 1);
		mDfxSettings->setLearner(inParameterID, dfx::kMidiEventBehaviorFlag_Toggle, numStates);
	}
}

//-----------------------------------------------------------------------------
dfx::ParameterID DfxPlugin::getmidilearner() const
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
void DfxPlugin::setparametermidiassignment(dfx::ParameterID inParameterID, dfx::ParameterAssignment const& inAssignment)
{
	if (inAssignment.mEventType == dfx::MidiEventType::None)
	{
		mDfxSettings->unassignParameter(inParameterID);
	}
	else
	{
		mDfxSettings->assignParameter(inParameterID, inAssignment.mEventType, inAssignment.mEventChannel,
									  inAssignment.mEventNum, inAssignment.mEventNum2, 
									  inAssignment.mEventBehaviorFlags, 
									  inAssignment.mDataInt1, inAssignment.mDataInt2, 
									  inAssignment.mDataFloat1, inAssignment.mDataFloat2);
	}
}

//-----------------------------------------------------------------------------
dfx::ParameterAssignment DfxPlugin::getparametermidiassignment(dfx::ParameterID inParameterID) const
{
	return mDfxSettings->getParameterAssignment(inParameterID);
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
