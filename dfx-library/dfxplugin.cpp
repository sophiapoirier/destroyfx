/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2019  Sophia Poirier

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
#include <cassert>
#include <cmath>
#include <stdio.h>
#include <time.h>	// for time(), which is used to feed srand()

#include "dfxmisc.h"

#ifdef TARGET_API_AUDIOUNIT
	#include "dfx-au-utilities.h"
#endif

#if defined(TARGET_API_VST) && TARGET_PLUGIN_HAS_GUI && defined(TARGET_PLUGIN_USES_VSTGUI)
	// If using the VST GUI interface, we need the class definition
	// for AEffGUIEditor so that we can send it parameter changes.
	#include "aeffguieditor.h"
	extern AEffEditor* DFXGUI_NewEditorInstance(DfxPlugin* inEffectInstance);
#endif

#ifdef TARGET_API_RTAS
	#include "ConvertUtils.h"
	#if TARGET_PLUGIN_HAS_GUI && defined(TARGET_PLUGIN_USES_VSTGUI)
		extern void* gThisModule;
	#endif
#endif

//#define DFX_DEBUG_PRINT_MUSICAL_TIME_INFO
//#define DFX_DEBUG_PRINT_MUSIC_EVENTS

#define DFX_PRESET_DEFAULT_NAME	"default"



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
	mNumInputs(VST_NUM_INPUTS), mNumOutputs(VST_NUM_OUTPUTS), 
#endif
// end API-specific base constructors

	mParameters(inNumParameters)
{
	updatesamplerate();  // XXX have it set to something here?

	// set a seed value for rand() from the system clock
	srand(static_cast<unsigned int>(time(nullptr)));

	mPresets.reserve(inNumPresets);
	for (long i = 0; i < inNumPresets; i++)
	{
		mPresets.emplace_back(inNumParameters);
	}


#ifdef TARGET_API_VST
	setUniqueID(PLUGIN_ID);  // identify
	setNumInputs(VST_NUM_INPUTS);
	setNumOutputs(VST_NUM_OUTPUTS);

	#if TARGET_PLUGIN_IS_INSTRUMENT
	isSynth();
	#endif

	#if !VST_FORCE_DEPRECATED
	canProcessReplacing();  // supports both accumulating and replacing output
	#endif
	TARGET_API_BASE_CLASS::setProgram(0);  // set the current preset number to 0

	// check to see if the host supports sending tempo and time information to VST plugins
	mHostCanDoTempo = (canHostDo("sendVstTimeInfo") == 1);

	#if TARGET_PLUGIN_USES_MIDI
	// tell host that we want to use special data chunks for settings storage
	programsAreChunks();
	#endif

	#if TARGET_PLUGIN_HAS_GUI
	editor = DFXGUI_NewEditorInstance(this);
	#endif
#endif
// end VST stuff

#ifdef TARGET_API_RTAS
#ifdef TARGET_PLUGIN_USES_VSTGUI
	mPIWinRect.top = mPIWinRect.left = mPIWinRect.bottom = mPIWinRect.right = 0;
	#if WINDOWS_VERSION
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
	if (!presetnameisvalid(0))
	{
		setpresetname(0, DFX_PRESET_DEFAULT_NAME);
	}

#if TARGET_PLUGIN_USES_DSPCORE && !defined(TARGET_API_AUDIOUNIT)
	// need to save instantiating the cores for the inheriting plugin class constructor
	mDSPCores.assign(getnumoutputs(), nullptr);
#endif

#if TARGET_PLUGIN_USES_MIDI
	mDfxSettings = std::make_unique<DfxSettings>(PLUGIN_ID, this);
#endif

	dfx_PostConstructor();
}


//-----------------------------------------------------------------------------
// this is called immediately before all destructors (DfxPlugin and any derived classes) occur
void DfxPlugin::do_PreDestructor()
{
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

#ifdef TARGET_API_VST
	mIsInitialized = true;
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
		return;
	}
#endif

	releasebuffers();

#ifdef TARGET_API_AUDIOUNIT
	mInputAudioStreams_au.clear();
	mOutputAudioStreams_au.clear();
#endif

	cleanup();

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
	AUEffectBase::Reset(kAudioUnitScope_Global, AudioUnitElement(0));
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
void DfxPlugin::initparameter_f(long inParameterIndex, char const* initName, double initValue, 
								double initDefaultValue, double initMin, double initMax, 
								DfxParam::Unit initUnit, DfxParam::Curve initCurve, 
								char const* initCustomUnitString)
{
	if (parameterisvalid(inParameterIndex))
	{
		mParameters[inParameterIndex].init_f(initName, initValue, initDefaultValue, initMin, initMax, initUnit, initCurve);
// XXX hmmm... maybe not here?
		//		if (getparameterattributes(inParameterIndex) & DfxParam::kAttribute_Unused)  // XXX should we do it like this?
		{
//			update_parameter(inParameterIndex);  // XXX make the host aware of the parameter change
		}
		initpresetsparameter(inParameterIndex);  // default empty presets with this value
		if (initCustomUnitString)
		{
			setparametercustomunitstring(inParameterIndex, initCustomUnitString);
		}
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::initparameter_i(long inParameterIndex, char const* initName, int64_t initValue, 
								int64_t initDefaultValue, int64_t initMin, int64_t initMax, 
								DfxParam::Unit initUnit, DfxParam::Curve initCurve, 
								char const* initCustomUnitString)
{
	if (parameterisvalid(inParameterIndex))
	{
		mParameters[inParameterIndex].init_i(initName, initValue, initDefaultValue, initMin, initMax, initUnit, initCurve);
//		update_parameter(inParameterIndex);  // XXX make the host aware of the parameter change
		initpresetsparameter(inParameterIndex);  // default empty presets with this value
		if (initCustomUnitString)
		{
			setparametercustomunitstring(inParameterIndex, initCustomUnitString);
		}
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::initparameter_b(long inParameterIndex, char const* initName, bool initValue, bool initDefaultValue, 
								DfxParam::Unit initUnit)
{
	if (parameterisvalid(inParameterIndex))
	{
		mParameters[inParameterIndex].init_b(initName, initValue, initDefaultValue, initUnit);
//		update_parameter(inParameterIndex);  // XXX make the host aware of the parameter change
		initpresetsparameter(inParameterIndex);  // default empty presets with this value
	}
}

//-----------------------------------------------------------------------------
// this is a shorcut for initializing a parameter that uses integer indexes 
// into an array, with an array of strings representing its values
void DfxPlugin::initparameter_list(long inParameterIndex, char const* initName, int64_t initValue, int64_t initDefaultValue, 
								   int64_t initNumItems, DfxParam::Unit initUnit, char const* initCustomUnitString)
{
	if (parameterisvalid(inParameterIndex))
	{
		mParameters[inParameterIndex].init_i(initName, initValue, initDefaultValue, 0, initNumItems - 1, initUnit, DfxParam::Curve::Stepped);
		setparameterusevaluestrings(inParameterIndex, true);  // indicate that we will use custom value display strings
//		update_parameter(inParameterIndex);  // XXX make the host aware of the parameter change
		initpresetsparameter(inParameterIndex);  // default empty presets with this value
		if (initCustomUnitString)
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
void DfxPlugin::randomizeparameters(bool writeAutomation)
{
	for (long i = 0; i < getnumparameters(); i++)
	{
		randomizeparameter(i);

#ifdef TARGET_API_VST
		if (writeAutomation)
		{
			setParameterAutomated(i, getparameter_gen(i));
		}
#endif
	}
}

//-----------------------------------------------------------------------------
// do stuff necessary to inform the host of changes, etc.
void DfxPlugin::update_parameter(long inParameterIndex)
{
#ifdef TARGET_API_AUDIOUNIT
	// make the global-scope element aware of the parameter's value
	AUBase::SetParameter(inParameterIndex, kAudioUnitScope_Global, AudioUnitElement(0), getparameter_f(inParameterIndex), 0);
#endif

#ifdef TARGET_API_VST
	auto const vstpresetnum = TARGET_API_BASE_CLASS::getProgram();
	if (presetisvalid(vstpresetnum))
	{
		setpresetparameter(vstpresetnum, inParameterIndex, getparameter(inParameterIndex));
	}
	#if TARGET_PLUGIN_HAS_GUI
	#ifdef TARGET_PLUGIN_USES_VSTGUI
	if (editor)
	{
		((AEffGUIEditor*)editor)->setParameter(inParameterIndex, getparameter_gen(inParameterIndex));
	}
	#else
	assert(false);  // XXX TODO: we will need something for our GUI class here
	#endif
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

#ifdef TARGET_API_AUDIOUNIT
	AUParameterChange_TellListeners(GetComponentInstance(), inParameterIndex);
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
void DfxPlugin::getparametername(long inParameterIndex, char* outText) const
{
	if (outText)
	{
		if (parameterisvalid(inParameterIndex))
		{
			mParameters[inParameterIndex].getname(outText);
		}
		else
		{
			outText[0] = 0;
		}
	}
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
bool DfxPlugin::setparametervaluestring(long inParameterIndex, int64_t inStringIndex, char const* inText)
{
	if (parameterisvalid(inParameterIndex))
	{
		return mParameters[inParameterIndex].setvaluestring(inStringIndex, inText);
	}
	return false;
}

//-----------------------------------------------------------------------------
bool DfxPlugin::getparametervaluestring(long inParameterIndex, int64_t inStringIndex, char* outText) const
{
	if (parameterisvalid(inParameterIndex))
	{
		return mParameters[inParameterIndex].getvaluestring(inStringIndex, outText);
	}
	return false;
}

//-----------------------------------------------------------------------------
char const* DfxPlugin::getparametervaluestring_ptr(long inParameterIndex, int64_t inStringIndex) const
{
	if (parameterisvalid(inParameterIndex))
	{
		return mParameters[inParameterIndex].getvaluestring_ptr(inStringIndex);
	}
	return nullptr;
}

//-----------------------------------------------------------------------------
bool DfxPlugin::getparameterchanged(long inParameterIndex) const
{
	if (parameterisvalid(inParameterIndex))
	{
		return mParameters[inParameterIndex].getchanged();
	}
	return false;
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameterchanged(long inParameterIndex, bool inChanged)
{
	if (parameterisvalid(inParameterIndex))
	{
		mParameters[inParameterIndex].setchanged(inChanged);
	}
}

//-----------------------------------------------------------------------------
bool DfxPlugin::getparametertouched(long inParameterIndex) const
{
	if (parameterisvalid(inParameterIndex))
	{
		return mParameters[inParameterIndex].gettouched();
	}
	return false;
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparametertouched(long inParameterIndex, bool inTouched)
{
	if (parameterisvalid(inParameterIndex))
	{
		mParameters[inParameterIndex].settouched(inTouched);
	}
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
	if (!mPresets[inPresetIndex].getname_ptr())
	{
		return false;
	}
	if ((mPresets[inPresetIndex].getname_ptr())[0] == 0)
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

	// do stuff necessary to inform the host of changes, etc.
	// XXX in AU, if this resulted from a call to NewFactoryPresetSet, then PropertyChanged will be called twice
	update_preset(inPresetIndex);
	return true;
}

//-----------------------------------------------------------------------------
// do stuff necessary to inform the host of changes, etc.
void DfxPlugin::update_preset(long inPresetIndex)
{
	mCurrentPresetNum = inPresetIndex;

#ifdef TARGET_API_AUDIOUNIT
	AUPreset au_preset {};
	au_preset.presetNumber = inPresetIndex;
	au_preset.presetName = getpresetcfname(inPresetIndex);
	SetAFactoryPresetAsCurrent(au_preset);
	PropertyChanged(kAudioUnitProperty_PresentPreset, kAudioUnitScope_Global, AudioUnitElement(0));
	PropertyChanged(kAudioUnitProperty_CurrentPreset, kAudioUnitScope_Global, AudioUnitElement(0));
#endif

#ifdef TARGET_API_VST
	TARGET_API_BASE_CLASS::setProgram(inPresetIndex);
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
		DfxParam::Value paramValue {};
		mParameters[inParameterIndex].accept_f(inValue, paramValue);
		mPresets[inPresetIndex].setvalue(inParameterIndex, paramValue);
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setpresetparameter_i(long inPresetIndex, long inParameterIndex, int64_t inValue)
{
	if (parameterisvalid(inParameterIndex) && presetisvalid(inPresetIndex))
	{
		DfxParam::Value paramValue {};
		mParameters[inParameterIndex].accept_i(inValue, paramValue);
		mPresets[inPresetIndex].setvalue(inParameterIndex, paramValue);
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setpresetparameter_b(long inPresetIndex, long inParameterIndex, bool inValue)
{
	if (parameterisvalid(inParameterIndex) && presetisvalid(inPresetIndex))
	{
		DfxParam::Value paramValue {};
		mParameters[inParameterIndex].accept_b(inValue, paramValue);
		mPresets[inPresetIndex].setvalue(inParameterIndex, paramValue);
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setpresetparameter_gen(long inPresetIndex, long inParameterIndex, double inValue)
{
	if (parameterisvalid(inParameterIndex) && presetisvalid(inPresetIndex))
	{
		DfxParam::Value paramValue {};
		mParameters[inParameterIndex].accept_f(expandparametervalue(inParameterIndex, inValue), paramValue);
		mPresets[inPresetIndex].setvalue(inParameterIndex, paramValue);
	}
}

//-----------------------------------------------------------------------------
// set the text of a preset name
void DfxPlugin::setpresetname(long inPresetIndex, char const* inText)
{
	if (presetisvalid(inPresetIndex))
	{
		mPresets[inPresetIndex].setname(inText);
	}
}

//-----------------------------------------------------------------------------
// get a copy of the text of a preset name
void DfxPlugin::getpresetname(long inPresetIndex, char* outText) const
{
	if (presetisvalid(inPresetIndex))
	{
		mPresets[inPresetIndex].getname(outText);
	}
}

//-----------------------------------------------------------------------------
// get a pointer to the text of a preset name
char const* DfxPlugin::getpresetname_ptr(long inPresetIndex) const
{
	if (presetisvalid(inPresetIndex))
	{
		return mPresets[inPresetIndex].getname_ptr();
	}
	else
	{
		return nullptr;
	}
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
	else
	{
		return nullptr;
	}
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

	if (inSampleRate != DfxPlugin::mSampleRate)
	{
		mSampleRateChanged = true;
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
void DfxPlugin::getpluginname(char* outText) const
{
	if (outText)
	{
		strcpy(outText, PLUGIN_NAME_STRING);
	}
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
		if (!owner || (owner == value.second))
		{
			value.first->inc();
		}
	});
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
// add an audio input/output configuration to the array of i/o configurations
void DfxPlugin::addchannelconfig(short inNumInputChannels, short inNumOutputChannels)
{
	ChannelConfig channelConfig;
	channelConfig.inChannels = inNumInputChannels;
	channelConfig.outChannels = inNumOutputChannels;
	mChannelconfigs.push_back(channelConfig);
}

//-----------------------------------------------------------------------------
void DfxPlugin::setlatency_samples(long inLatency)
{
	bool changed = false;
	if (mUseLatency_seconds)
	{
		changed = true;
	}
	else if (mLatency_samples != inLatency)
	{
		changed = true;
	}

	mLatency_samples = inLatency;
	mUseLatency_seconds = false;

	if (changed)
	{
		update_latency();
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setlatency_seconds(double inLatency)
{
	bool changed = false;
	if (!mUseLatency_seconds)
	{
		changed = true;
	}
	else if (mLatency_seconds != inLatency)
	{
		changed = true;
	}

	mLatency_seconds = inLatency;
	mUseLatency_seconds = true;

	if (changed)
	{
		update_latency();
	}
}

//-----------------------------------------------------------------------------
long DfxPlugin::getlatency_samples() const
{
	if (mUseLatency_seconds)
	{
		return std::lround(mLatency_seconds * getsamplerate());
	}
	else
	{
		return mLatency_samples;
	}
}

//-----------------------------------------------------------------------------
double DfxPlugin::getlatency_seconds() const
{
	if (mUseLatency_seconds)
	{
		return mLatency_seconds;
	}
	else
	{
		return static_cast<double>(mLatency_samples) / getsamplerate();
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::update_latency()
{
#ifdef TARGET_API_AUDIOUNIT
	PropertyChanged(kAudioUnitProperty_Latency, kAudioUnitScope_Global, AudioUnitElement(0));
#endif
}

//-----------------------------------------------------------------------------
void DfxPlugin::settailsize_samples(long inSize)
{
	bool changed = false;
	if (mUseTailSize_seconds)
	{
		changed = true;
	}
	else if (mTailSize_samples != inSize)
	{
		changed = true;
	}

	mTailSize_samples = inSize;
	mUseTailSize_seconds = false;

	if (changed)
	{
		update_tailsize();
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::settailsize_seconds(double inSize)
{
	bool changed = false;
	if (!mUseTailSize_seconds)
	{
		changed = true;
	}
	else if (mTailSize_seconds != inSize)
	{
		changed = true;
	}

	mTailSize_seconds = inSize;
	mUseTailSize_seconds = true;

	if (changed)
	{
		update_tailsize();
	}
}

//-----------------------------------------------------------------------------
long DfxPlugin::gettailsize_samples() const
{
	if (mUseTailSize_seconds)
	{
		return std::lround(mTailSize_seconds * getsamplerate());
	}
	else
	{
		return mTailSize_samples;
	}
}

//-----------------------------------------------------------------------------
double DfxPlugin::gettailsize_seconds() const
{
	if (mUseTailSize_seconds)
	{
		return mTailSize_seconds;
	}
	else
	{
		return static_cast<double>(mTailSize_samples) / getsamplerate();
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::update_tailsize()
{
#ifdef TARGET_API_AUDIOUNIT
	PropertyChanged(kAudioUnitProperty_TailTime, kAudioUnitScope_Global, AudioUnitElement(0));
#endif
}

//-----------------------------------------------------------------------------
void DfxPlugin::setAudioProcessingMustAccumulate(bool inMode)
{
	mAudioProcessingAccumulatingOnly = inMode;
	if (inMode)
	{
#ifdef TARGET_API_AUDIOUNIT
	#if !TARGET_PLUGIN_IS_INSTRUMENT
		SetProcessesInPlace(false);
	#endif
#endif
#if defined(TARGET_API_VST) && !VST_FORCE_DEPRECATED
		canProcessReplacing(false);  // XXX can't depend on this anymore
#endif
	}
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

#if TARGET_PLUGIN_USES_MIDI
	mMidiState.preprocessEvents();
#endif

	// fetch the latest musical tempo/time/location inforomation from the host
	processtimeinfo();
	// deal with current parameter values for usage during audio processing
	do_processparameters();
}

//-----------------------------------------------------------------------------
// this is called immediately after processing a block of audio
void DfxPlugin::postprocessaudio()
{
	// XXX turn off all parameterchanged and parametertouched flags?
	for (long i = 0; i < getnumparameters(); i++)
	{
		setparameterchanged(i, false);
		setparametertouched(i, false);
	}

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
	if (mDfxSettings)
	{
		mDfxSettings->handleNoteOn(inChannel, inNote, inVelocity, inOffsetFrames);
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_noteoff(int inChannel, int inNote, int inVelocity, unsigned long inOffsetFrames)
{
#ifdef DFX_DEBUG_PRINT_MUSIC_EVENTS
fprintf(stderr, "note off:  note = %d, velocity = %d, channel = %d, sample offset = %lu\n", inNote, inVelocity, inChannel, inOffsetFrames);
#endif
	mMidiState.handleNoteOff(inChannel, inNote, inVelocity, inOffsetFrames);
	if (mDfxSettings)
	{
		mDfxSettings->handleNoteOff(inChannel, inNote, inVelocity, inOffsetFrames);
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_allnotesoff(int inChannel, unsigned long inOffsetFrames)
{
#ifdef DFX_DEBUG_PRINT_MUSIC_EVENTS
fprintf(stderr, "all notes off:  channel = %d, sample offset = %lu\n", inChannel, inOffsetFrames);
#endif
	mMidiState.handleAllNotesOff(inChannel, inOffsetFrames);
	if (mDfxSettings)
	{
		mDfxSettings->handleAllNotesOff(inChannel, inOffsetFrames);
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_pitchbend(int inChannel, int inValueLSB, int inValueMSB, unsigned long inOffsetFrames)
{
#ifdef DFX_DEBUG_PRINT_MUSIC_EVENTS
fprintf(stderr, "pitchbend:  LSB = %d, MSB = %d, channel = %d, sample offset = %lu\n", inValueLSB, inValueMSB, inChannel, inOffsetFrames);
#endif
	mMidiState.handlePitchBend(inChannel, inValueLSB, inValueMSB, inOffsetFrames);
	if (mDfxSettings)
	{
		mDfxSettings->handlePitchBend(inChannel, inValueLSB, inValueMSB, inOffsetFrames);
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_cc(int inChannel, int inControllerNum, int inValue, unsigned long inOffsetFrames)
{
#ifdef DFX_DEBUG_PRINT_MUSIC_EVENTS
fprintf(stderr, "MIDI CC:  controller = 0x%02X, value = %d, channel = %d, sample offset = %lu\n", inControllerNum, inValue, inChannel, inOffsetFrames);
#endif
	mMidiState.handleCC(inChannel, inControllerNum, inValue, inOffsetFrames);
	if (mDfxSettings)
	{
		mDfxSettings->handleCC(inChannel, inControllerNum, inValue, inOffsetFrames);
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_programchange(int inChannel, int inProgramNum, unsigned long inOffsetFrames)
{
#ifdef DFX_DEBUG_PRINT_MUSIC_EVENTS
fprintf(stderr, "program change:  program num = %d, channel = %d, sample offset = %lu\n", inProgramNum, inChannel, inOffsetFrames);
#endif
	mMidiState.handleProgramChange(inChannel, inProgramNum, inOffsetFrames);
}

#endif
// TARGET_PLUGIN_USES_MIDI
