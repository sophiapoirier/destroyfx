/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2011  Sophia Poirier

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

#include <stdio.h>
#include <time.h>	// for time(), which is used to feed srand()

#ifdef TARGET_API_AUDIOUNIT
	#include "dfx-au-utilities.h"
#endif

#if defined(TARGET_API_VST) && TARGET_PLUGIN_HAS_GUI && defined(TARGET_PLUGIN_USES_VSTGUI)
	// If using the VST GUI interface, we need the class definition
	// for AEffGUIEditor so that we can send it parameter changes.
	#include "aeffguieditor.h"
	extern AEffEditor * DFXGUI_NewEditorInstance(DfxPlugin * inEffectInstance);
#endif

#ifdef TARGET_API_RTAS
	#include "ConvertUtils.h"
	#if TARGET_PLUGIN_HAS_GUI && defined(TARGET_PLUGIN_USES_VSTGUI)
		extern void * gThisModule;
	#endif
#endif

#if TARGET_OS_MAC
	#include <Carbon/Carbon.h>
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
//		TARGET_API_BASE_CLASS(inInstance, UInt32 inNumInputs, UInt32 inNumOutputs, UInt32 inNumGroups = 0), 
		TARGET_API_BASE_CLASS(inInstance, 0, 1), 
	#else
		TARGET_API_BASE_CLASS(inInstance), 
	#endif
#endif

#ifdef TARGET_API_VST
	TARGET_API_BASE_CLASS(inInstance, inNumPresets, inNumParameters), 
	numInputs(VST_NUM_INPUTS), numOutputs(VST_NUM_OUTPUTS), 
#endif
// end API-specific base constructors

	numParameters(inNumParameters), numPresets(inNumPresets)
{
	parameters = NULL;
	presets = NULL;
	channelconfigs = NULL;
	tempoRateTable = NULL;

	#if TARGET_PLUGIN_USES_MIDI
		midistuff = NULL;
		dfxsettings = NULL;
	#endif

	#if TARGET_PLUGIN_USES_DSPCORE && !defined(TARGET_API_AUDIOUNIT)
		dspcores = NULL;
	#endif

	numchannelconfigs = 0;

#ifdef TARGET_API_AUDIOUNIT
	auElementsHaveBeenCreated = false;
	inputAudioStreams_au = outputAudioStreams_au = NULL;

	supportedLogicNodeOperationMode = kLogicAUNodeOperationMode_FullSupport;
	currentLogicNodeOperationMode = 0;
#endif
// end Audio Unit stuff

	updatesamplerate();	// XXX have it set to something here?
	sampleratechanged = true;
	hostCanDoTempo = false;	// until proven otherwise

	latency_samples = 0;
	latency_seconds = 0.0;
	b_uselatency_seconds = false;

	tailsize_samples = 0;
	tailsize_seconds = 0.0;
	b_usetailsize_seconds = false;

	audioProcessingAccumulatingOnly = false;
	audioIsRendering = false;

	// set a seed value for rand() from the system clock
	srand( (unsigned int)time(NULL) );

	currentPresetNum = 0;	// XXX eh?


	if (numParameters > 0)
		parameters = new DfxParam[numParameters];
	if (numPresets > 0)
	{
		presets = new DfxPreset[numPresets];
		for (int i=0; i < numPresets; i++)
			presets[i].PostConstructor(numParameters);	// allocate for parameter values
	}

	#if TARGET_PLUGIN_USES_MIDI
		midistuff = new DfxMidi;
		dfxsettings = new DfxSettings(PLUGIN_ID, this);
	#endif


#ifdef TARGET_API_VST
	setUniqueID(PLUGIN_ID);	// identify
	setNumInputs(VST_NUM_INPUTS);
	setNumOutputs(VST_NUM_OUTPUTS);

	#if TARGET_PLUGIN_IS_INSTRUMENT
		isSynth();
	#endif

	#if !VST_FORCE_DEPRECATED
	canProcessReplacing();	// supports both accumulating and replacing output
	#endif
	TARGET_API_BASE_CLASS::setProgram(0);	// set the current preset number to 0

	isinitialized = false;
	setlatencychanged(false);

	// check to see if the host supports sending tempo & time information to VST plugins
	hostCanDoTempo = (canHostDo("sendVstTimeInfo") == 1);

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
	inputAudioStreams_as = NULL;
	outputAudioStreams_as = NULL;
	zeroAudioBuffer = NULL;
	numZeroAudioBufferSamples = 0;
	mMasterBypass_rtas = false;

#ifdef TARGET_PLUGIN_USES_VSTGUI
	mCustomUI_p = NULL;
	mNoUIView_p = NULL;
	mModuleHandle_p = NULL;
	mLeftOffset = mTopOffset = 0;
	mPIWinRect.top = mPIWinRect.left = mPIWinRect.bottom = mPIWinRect.right = 0;
	#if WINDOWS_VERSION
		mModuleHandle_p = (void*)gThisModule;	// extern from DLLMain.cpp; HINSTANCE of the DLL
	#elif MAC_VERSION
		mModuleHandle_p = NULL;	// not needed by Mac VST calls
	#endif
#endif
#endif
// end RTAS stuff

}

//-----------------------------------------------------------------------------
// this is called immediately after all constructors (DfxPlugin and any derived classes) complete
void DfxPlugin::dfx_PostConstructor()
{
	// set up a name for the default preset if none was set
	if ( !presetnameisvalid(0) )
		setpresetname(0, DFX_PRESET_DEFAULT_NAME);

	#if TARGET_PLUGIN_USES_DSPCORE && !defined(TARGET_API_AUDIOUNIT)
		dspcores = (DfxPluginCore**) malloc(getnumoutputs() * sizeof(DfxPluginCore*));
		// need to save instantiating the cores for the inheriting plugin class constructor
		for (unsigned long ch=0; ch < getnumoutputs(); ch++)
			dspcores[ch] = NULL;
	#endif
}


//-----------------------------------------------------------------------------
DfxPlugin::~DfxPlugin()
{
	if (parameters != NULL)
		delete[] parameters;
	parameters = NULL;

	if (presets != NULL)
		delete[] presets;
	presets = NULL;

	if (channelconfigs != NULL)
		free(channelconfigs);
	channelconfigs = NULL;

	if (tempoRateTable != NULL)
		delete tempoRateTable;
	tempoRateTable = NULL;

	#if TARGET_PLUGIN_USES_MIDI
		if (midistuff != NULL)
			delete midistuff;
		midistuff = NULL;
		if (dfxsettings != NULL)
			delete dfxsettings;
		dfxsettings = NULL;
	#endif

	#if TARGET_PLUGIN_USES_DSPCORE && !defined(TARGET_API_AUDIOUNIT)
		if (dspcores != NULL)
		{
			for (unsigned long ch=0; ch < getnumoutputs(); ch++)
			{
				if (dspcores[ch] != NULL)
					delete dspcores[ch];
				dspcores[ch] = NULL;
			}
			free(dspcores);
		}
		dspcores = NULL;
	#endif

#ifdef TARGET_API_RTAS
	if (inputAudioStreams_as != NULL)
		free(inputAudioStreams_as);
	inputAudioStreams_as = NULL;
	if (outputAudioStreams_as != NULL)
		free(outputAudioStreams_as);
	outputAudioStreams_as = NULL;
	if (zeroAudioBuffer != NULL)
		free(zeroAudioBuffer);
	zeroAudioBuffer = NULL;

	#ifdef TARGET_PLUGIN_USES_VSTGUI
		if (mCustomUI_p != NULL)
			delete mCustomUI_p;
		mCustomUI_p = NULL;

		mNoUIView_p = NULL;
	#endif
#endif
// end RTAS-specific destructor stuff
}

//-----------------------------------------------------------------------------
// this is called immediately before all destructors (DfxPlugin and any derived classes) occur
void DfxPlugin::dfx_PreDestructor()
{
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

	long result = initialize();
	if (result != kDfxErr_NoError)
		return result;

	#ifdef TARGET_API_VST
		isinitialized = true;
	#endif

	bool buffersCreated = createbuffers();
	if (!buffersCreated)
		return kDfxErr_InitializationFailed;

	do_reset();

	return kDfxErr_NoError;	// no error
}

//-----------------------------------------------------------------------------
// non-virtual function that calls cleanup() and insures that some stuff happens
void DfxPlugin::do_cleanup()
{
	#ifdef TARGET_API_VST
		if (!isinitialized)
			return;
	#endif

	releasebuffers();

	#ifdef TARGET_API_AUDIOUNIT
		if (inputAudioStreams_au != NULL)
			free(inputAudioStreams_au);
		inputAudioStreams_au = NULL;
		if (outputAudioStreams_au != NULL)
			free(outputAudioStreams_au);
		outputAudioStreams_au = NULL;
	#endif

	cleanup();

	#ifdef TARGET_API_VST
		isinitialized = false;
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
//		if (! IsInitialized() )
//			return;
		#if !TARGET_PLUGIN_IS_INSTRUMENT
			// resets the kernels, if any
			AUEffectBase::Reset(kAudioUnitScope_Global, (AudioUnitElement)0);
		#endif
	#endif

	#if TARGET_PLUGIN_USES_MIDI
		if (midistuff != NULL)
			midistuff->reset();
	#endif

	#if TARGET_PLUGIN_USES_DSPCORE && !defined(TARGET_API_AUDIOUNIT)
		for (unsigned long i=0; i < getnumoutputs(); i++)
		{
			if (dspcores[i] != NULL)
				dspcores[i]->do_reset();
		}
	#endif

	reset();
	clearbuffers();
}



#pragma mark -
#pragma mark parameters

//-----------------------------------------------------------------------------
void DfxPlugin::initparameter_f(long inParameterIndex, const char * initName, double initValue, 
						double initDefaultValue, double initMin, double initMax, 
						DfxParamUnit initUnit, DfxParamCurve initCurve, 
						const char * initCustomUnitString)
{
	if (parameterisvalid(inParameterIndex))
	{
		parameters[inParameterIndex].init_f(initName, initValue, initDefaultValue, initMin, initMax, initUnit, initCurve);
// XXX hmmm... maybe not here?
//		if (getparameterattributes(inParameterIndex) & kDfxParamAttribute_unused)	// XXX should we do it like this?
//			update_parameter(inParameterIndex);	// XXX make the host aware of the parameter change
		initpresetsparameter(inParameterIndex);	// default empty presets with this value
		// set the custom unit string, if there is one
		if (initCustomUnitString != NULL)
			setparametercustomunitstring(inParameterIndex, initCustomUnitString);
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::initparameter_i(long inParameterIndex, const char * initName, int64_t initValue, 
						int64_t initDefaultValue, int64_t initMin, int64_t initMax, 
						DfxParamUnit initUnit, DfxParamCurve initCurve, 
						const char * initCustomUnitString)
{
	if (parameterisvalid(inParameterIndex))
	{
		parameters[inParameterIndex].init_i(initName, initValue, initDefaultValue, initMin, initMax, initUnit, initCurve);
//		update_parameter(inParameterIndex);	// XXX make the host aware of the parameter change
		initpresetsparameter(inParameterIndex);	// default empty presets with this value
		// set the custom unit string, if there is one
		if (initCustomUnitString != NULL)
			setparametercustomunitstring(inParameterIndex, initCustomUnitString);
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::initparameter_b(long inParameterIndex, const char * initName, bool initValue, bool initDefaultValue, 
						DfxParamUnit initUnit)
{
	if (parameterisvalid(inParameterIndex))
	{
		parameters[inParameterIndex].init_b(initName, initValue, initDefaultValue, initUnit);
//		update_parameter(inParameterIndex);	// XXX make the host aware of the parameter change
		initpresetsparameter(inParameterIndex);	// default empty presets with this value
	}
}

//-----------------------------------------------------------------------------
// this is a shorcut for initializing a parameter that uses integer indexes 
// into an array, with an array of strings representing its values
void DfxPlugin::initparameter_list(long inParameterIndex, const char * initName, int64_t initValue, int64_t initDefaultValue, 
						int64_t initNumItems, DfxParamUnit initUnit, const char * initCustomUnitString)
{
	if (parameterisvalid(inParameterIndex))
	{
		parameters[inParameterIndex].init_i(initName, initValue, initDefaultValue, 0, initNumItems-1, initUnit, kDfxParamCurve_stepped);
		setparameterusevaluestrings(inParameterIndex, true);	// indicate that we will use custom value display strings
//		update_parameter(inParameterIndex);	// XXX make the host aware of the parameter change
		initpresetsparameter(inParameterIndex);	// default empty presets with this value
		// set the custom unit string, if there is one
		if (initCustomUnitString != NULL)
			setparametercustomunitstring(inParameterIndex, initCustomUnitString);
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameter(long inParameterIndex, DfxParamValue newValue)
{
	if (parameterisvalid(inParameterIndex))
	{
		parameters[inParameterIndex].set(newValue);
		update_parameter(inParameterIndex);	// make the host aware of the parameter change
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameter_f(long inParameterIndex, double newValue)
{
	if (parameterisvalid(inParameterIndex))
	{
		parameters[inParameterIndex].set_f(newValue);
		update_parameter(inParameterIndex);	// make the host aware of the parameter change
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameter_i(long inParameterIndex, int64_t newValue)
{
	if (parameterisvalid(inParameterIndex))
	{
		parameters[inParameterIndex].set_i(newValue);
		update_parameter(inParameterIndex);	// make the host aware of the parameter change
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameter_b(long inParameterIndex, bool newValue)
{
	if (parameterisvalid(inParameterIndex))
	{
		parameters[inParameterIndex].set_b(newValue);
		update_parameter(inParameterIndex);	// make the host aware of the parameter change
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameter_gen(long inParameterIndex, double newValue)
{
	if (parameterisvalid(inParameterIndex))
	{
		parameters[inParameterIndex].set_gen(newValue);
		update_parameter(inParameterIndex);	// make the host aware of the parameter change
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::randomizeparameter(long inParameterIndex)
{
	if (parameterisvalid(inParameterIndex))
	{
		parameters[inParameterIndex].randomize();
		update_parameter(inParameterIndex);	// make the host aware of the parameter change
		postupdate_parameter(inParameterIndex);	// inform any parameter listeners of the changes
	}
}

//-----------------------------------------------------------------------------
// randomize all of the parameters at once
void DfxPlugin::randomizeparameters(bool writeAutomation)
{
	for (long i=0; i < numParameters; i++)
	{
		randomizeparameter(i);

	#ifdef TARGET_API_VST
		if (writeAutomation)
			setParameterAutomated(i, getparameter_gen(i));
	#endif
	}
}

//-----------------------------------------------------------------------------
// do stuff necessary to inform the host of changes, etc.
void DfxPlugin::update_parameter(long inParameterIndex)
{
	#ifdef TARGET_API_AUDIOUNIT
		// make the global-scope element aware of the parameter's value
		AUBase::SetParameter(inParameterIndex, kAudioUnitScope_Global, (AudioUnitElement)0, getparameter_f(inParameterIndex), 0);
	#endif

	#ifdef TARGET_API_VST
		long vstpresetnum = TARGET_API_BASE_CLASS::getProgram();
		if (presetisvalid(vstpresetnum))
			setpresetparameter(vstpresetnum, inParameterIndex, getparameter(inParameterIndex));
		#if TARGET_PLUGIN_HAS_GUI
			#ifdef TARGET_PLUGIN_USES_VSTGUI
			if (editor != NULL)
				((AEffGUIEditor*)editor)->setParameter(inParameterIndex, getparameter_gen(inParameterIndex));
			#else
			// XXX we will need something for our GUI class here
			#endif
		#endif
	#endif

	#ifdef TARGET_API_RTAS
		SetControlValue( DFX_ParameterID_ToRTAS(inParameterIndex), ConvertToDigiValue(getparameter_gen(inParameterIndex)) );	// XXX yeah do this?
	#endif
}

//-----------------------------------------------------------------------------
// this will broadcast a notification to anyone interested (host, GUI, etc.) 
// about a parameter change
void DfxPlugin::postupdate_parameter(long inParameterIndex)
{
	if ( !parameterisvalid(inParameterIndex) )
		return;

	#ifdef TARGET_API_AUDIOUNIT
		AUParameterChange_TellListeners(GetComponentInstance(), inParameterIndex);
	#endif
}

//-----------------------------------------------------------------------------
// return a (hopefully) 0 to 1 scalar version of the parameter's current value
double DfxPlugin::getparameter_scalar(long inParameterIndex)
{
	if (parameterisvalid(inParameterIndex))
	{
		switch (getparameterunit(inParameterIndex))
		{
			case kDfxParamUnit_percent:
			case kDfxParamUnit_drywetmix:
				return parameters[inParameterIndex].get_f() * 0.01;
			case kDfxParamUnit_scalar:
				return parameters[inParameterIndex].get_f();
			// XXX should we not just use contractparametervalue() here?
			default:
				return parameters[inParameterIndex].get_f() / parameters[inParameterIndex].getmax_f();
		}
	}
	else
		return 0.0;
}

//-----------------------------------------------------------------------------
void DfxPlugin::getparametername(long inParameterIndex, char * outText)
{
	if (outText != NULL)
	{
		if (parameterisvalid(inParameterIndex))
			parameters[inParameterIndex].getname(outText);
		else
			outText[0] = 0;
	}
}

//-----------------------------------------------------------------------------
DfxParamValueType DfxPlugin::getparametervaluetype(long inParameterIndex)
{
	if (parameterisvalid(inParameterIndex))
		return parameters[inParameterIndex].getvaluetype();
	else
		return kDfxParamValueType_float;
}

//-----------------------------------------------------------------------------
DfxParamUnit DfxPlugin::getparameterunit(long inParameterIndex)
{
	if (parameterisvalid(inParameterIndex))
		return parameters[inParameterIndex].getunit();
	else
		return kDfxParamUnit_generic;
}

//-----------------------------------------------------------------------------
bool DfxPlugin::setparametervaluestring(long inParameterIndex, int64_t inStringIndex, const char * inText)
{
	if (parameterisvalid(inParameterIndex))
		return parameters[inParameterIndex].setvaluestring(inStringIndex, inText);
	else
		return false;
}

//-----------------------------------------------------------------------------
bool DfxPlugin::getparametervaluestring(long inParameterIndex, int64_t inStringIndex, char * outText)
{
	if (parameterisvalid(inParameterIndex))
		return parameters[inParameterIndex].getvaluestring(inStringIndex, outText);
	else
		return false;
}

//-----------------------------------------------------------------------------
char * DfxPlugin::getparametervaluestring_ptr(long inParameterIndex, int64_t inStringIndex)
{
	if (parameterisvalid(inParameterIndex))
		return parameters[inParameterIndex].getvaluestring_ptr(inStringIndex);
	else
		return 0;
}

//-----------------------------------------------------------------------------
bool DfxPlugin::getparameterchanged(long inParameterIndex)
{
	if (parameterisvalid(inParameterIndex))
		return parameters[inParameterIndex].getchanged();
	else
		return false;
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameterchanged(long inParameterIndex, bool inChanged)
{
	if (parameterisvalid(inParameterIndex))
		parameters[inParameterIndex].setchanged(inChanged);
}

//-----------------------------------------------------------------------------
bool DfxPlugin::getparametertouched(long inParameterIndex)
{
	if (parameterisvalid(inParameterIndex))
		return parameters[inParameterIndex].gettouched();
	else
		return false;
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparametertouched(long inParameterIndex, bool inTouched)
{
	if (parameterisvalid(inParameterIndex))
		parameters[inParameterIndex].settouched(inTouched);
}

//-----------------------------------------------------------------------------
// convenience methods for expanding and contracting parameter values 
// using the min/max/curvetype/curvespec/etc. settings of a given parameter
double DfxPlugin::expandparametervalue(long inParameterIndex, double genValue)
{
	if (parameterisvalid(inParameterIndex))
		return parameters[inParameterIndex].expand(genValue);
	else
		return 0.0;
}
//-----------------------------------------------------------------------------
double DfxPlugin::contractparametervalue(long inParameterIndex, double realValue)
{
	if (parameterisvalid(inParameterIndex))
		return parameters[inParameterIndex].contract(realValue);
	else
		return 0.0;
}



#pragma mark -
#pragma mark presets

//-----------------------------------------------------------------------------
// whether or not the index is a valid preset
bool DfxPlugin::presetisvalid(long inPresetIndex)
{
	if (presets == NULL)
		return false;
	if ( (inPresetIndex < 0) || (inPresetIndex >= numPresets) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// whether or not the index is a valid preset with a valid name
// this is mostly just for Audio Unit
bool DfxPlugin::presetnameisvalid(long inPresetIndex)
{
	// still do this check to avoid bad pointer access
	if (!presetisvalid(inPresetIndex))
		return false;

	if (presets[inPresetIndex].getname_ptr() == NULL)
		return false;
	if ( (presets[inPresetIndex].getname_ptr())[0] == 0 )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// load the settings of a preset
bool DfxPlugin::loadpreset(long inPresetIndex)
{
	if ( !presetisvalid(inPresetIndex) )
		return false;

	#ifdef TARGET_API_VST
		// XXX this needs to be done before calls to setparameter(), cuz setparameter will 
		// call update_parameter(), which will call setpresetparameter() using getProgram() 
		// for the program index to set parameter values, which means that the currently 
		// selected program will have its parameter values overwritten by those of the 
		// program currently being loaded, unless we do this first
		TARGET_API_BASE_CLASS::setProgram(inPresetIndex);
	#endif

	for (long i=0; i < numParameters; i++)
	{
		setparameter(i, getpresetparameter(inPresetIndex, i));
		postupdate_parameter(i);	// inform any parameter listeners of the changes
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
	currentPresetNum = inPresetIndex;

	#ifdef TARGET_API_AUDIOUNIT
		AUPreset au_preset;
		au_preset.presetNumber = inPresetIndex;
		au_preset.presetName = getpresetcfname(inPresetIndex);
		SetAFactoryPresetAsCurrent(au_preset);
		PropertyChanged(kAudioUnitProperty_PresentPreset, kAudioUnitScope_Global, (AudioUnitElement)0);
		PropertyChanged(kAudioUnitProperty_CurrentPreset, kAudioUnitScope_Global, (AudioUnitElement)0);
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
	for (long i=0; i < numPresets; i++)
	{
		if ( !presetnameisvalid(i) )	// only if it's an "empty" preset
			setpresetparameter(i, inParameterIndex, getparameter(inParameterIndex));
	}
}

//-----------------------------------------------------------------------------
DfxParamValue DfxPlugin::getpresetparameter(long inPresetIndex, long inParameterIndex)
{
	if ( parameterisvalid(inParameterIndex) && presetisvalid(inPresetIndex) )
		return presets[inPresetIndex].values[inParameterIndex];
	else
	{
		DfxParamValue dummy = {0};
		return dummy;
	}
}

//-----------------------------------------------------------------------------
double DfxPlugin::getpresetparameter_f(long inPresetIndex, long inParameterIndex)
{
	if ( parameterisvalid(inParameterIndex) && presetisvalid(inPresetIndex) )
		return parameters[inParameterIndex].derive_f(presets[inPresetIndex].values[inParameterIndex]);
	else
		return 0.0;
}

//-----------------------------------------------------------------------------
void DfxPlugin::setpresetparameter(long inPresetIndex, long inParameterIndex, DfxParamValue newValue)
{
	if ( parameterisvalid(inParameterIndex) && presetisvalid(inPresetIndex) )
		presets[inPresetIndex].values[inParameterIndex] = newValue;
}

//-----------------------------------------------------------------------------
void DfxPlugin::setpresetparameter_f(long inPresetIndex, long inParameterIndex, double newValue)
{
	if ( parameterisvalid(inParameterIndex) && presetisvalid(inPresetIndex) )
		parameters[inParameterIndex].accept_f(newValue, presets[inPresetIndex].values[inParameterIndex]);
}

//-----------------------------------------------------------------------------
void DfxPlugin::setpresetparameter_i(long inPresetIndex, long inParameterIndex, int64_t newValue)
{
	if ( parameterisvalid(inParameterIndex) && presetisvalid(inPresetIndex) )
		parameters[inParameterIndex].accept_i(newValue, presets[inPresetIndex].values[inParameterIndex]);
}

//-----------------------------------------------------------------------------
void DfxPlugin::setpresetparameter_b(long inPresetIndex, long inParameterIndex, bool newValue)
{
	if ( parameterisvalid(inParameterIndex) && presetisvalid(inPresetIndex) )
		parameters[inParameterIndex].accept_b(newValue, presets[inPresetIndex].values[inParameterIndex]);
}

//-----------------------------------------------------------------------------
void DfxPlugin::setpresetparameter_gen(long inPresetIndex, long inParameterIndex, double genValue)
{
	if ( parameterisvalid(inParameterIndex) && presetisvalid(inPresetIndex) )
		parameters[inParameterIndex].accept_f(expandparametervalue(inParameterIndex, genValue), presets[inPresetIndex].values[inParameterIndex]);
}

//-----------------------------------------------------------------------------
// set the text of a preset name
void DfxPlugin::setpresetname(long inPresetIndex, const char * inText)
{
	if (presetisvalid(inPresetIndex))
		presets[inPresetIndex].setname(inText);
}

//-----------------------------------------------------------------------------
// get a copy of the text of a preset name
void DfxPlugin::getpresetname(long inPresetIndex, char * outText)
{
	if (presetisvalid(inPresetIndex))
		presets[inPresetIndex].getname(outText);
}

//-----------------------------------------------------------------------------
// get a pointer to the text of a preset name
const char * DfxPlugin::getpresetname_ptr(long inPresetIndex)
{
	if (presetisvalid(inPresetIndex))
		return presets[inPresetIndex].getname_ptr();
	else
		return NULL;
}

#ifdef TARGET_API_AUDIOUNIT
//-----------------------------------------------------------------------------
// get the CFString version of a preset name
CFStringRef DfxPlugin::getpresetcfname(long inPresetIndex)
{
	if (presetisvalid(inPresetIndex))
		return presets[inPresetIndex].getcfname();
	else
		return NULL;
}
#endif



#pragma mark -
#pragma mark state

//-----------------------------------------------------------------------------
// change the current audio sampling rate
void DfxPlugin::setsamplerate(double newrate)
{
	// avoid bogus values
	if (newrate <= 0.0)
		newrate = 44100.0;

	if (newrate != samplerate)
		sampleratechanged = true;

	// accept the new value into our sampling rate keeper
	samplerate = newrate;

	#if TARGET_PLUGIN_USES_MIDI
		if (midistuff != NULL)
			midistuff->setSampleRate(newrate);
	#endif
}

//-----------------------------------------------------------------------------
// called when the sampling rate should be re-fetched from the host
void DfxPlugin::updatesamplerate()
{
#ifdef TARGET_API_AUDIOUNIT
	if (auElementsHaveBeenCreated)	// will crash otherwise
		setsamplerate( GetSampleRate() );
	else
		setsamplerate(kAUDefaultSampleRate);
#endif
#ifdef TARGET_API_VST
	setsamplerate( (double)getSampleRate() );
#endif
#ifdef TARGET_API_RTAS
	setsamplerate( GetSampleRate() );
#endif
}

//-----------------------------------------------------------------------------
// called when the number of audio channels has changed
void DfxPlugin::updatenumchannels()
{
	#ifdef TARGET_API_AUDIOUNIT
		// the number of inputs or outputs may have changed
		numInputs = getnuminputs();
		if (inputAudioStreams_au != NULL)
			free(inputAudioStreams_au);
		inputAudioStreams_au = NULL;
		if (numInputs > 0)
			inputAudioStreams_au = (float**) malloc(numInputs * sizeof(float*));

		numOutputs = getnumoutputs();
		if (outputAudioStreams_au != NULL)
			free(outputAudioStreams_au);
		outputAudioStreams_au = NULL;
		if (numOutputs > 0)
			outputAudioStreams_au = (float**) malloc(numOutputs * sizeof(float*));
	#endif
}



#pragma mark -
#pragma mark properties

//-----------------------------------------------------------------------------
void DfxPlugin::getpluginname(char * outText)
{
	if (outText != NULL)
		strcpy(outText, PLUGIN_NAME_STRING);
}

//-----------------------------------------------------------------------------
long DfxPlugin::getpluginversion()
{
	return DFX_CompositePluginVersionNumberValue();
}

//-----------------------------------------------------------------------------
// return the number of audio inputs
unsigned long DfxPlugin::getnuminputs()
{
#ifdef TARGET_API_AUDIOUNIT
	if ( Inputs().GetNumberOfElements() > 0 )
		return GetStreamFormat(kAudioUnitScope_Input, (AudioUnitElement)0).NumberChannels();
	else
		return 0;
#endif

#ifdef TARGET_API_VST
	return numInputs;
#endif

#ifdef TARGET_API_RTAS
	if ( IsAS() && audioIsRendering )	// XXX checking connections is only valid while rendering
	{
		for (SInt32 ch=0; ch < GetNumInputs(); ch++)
		{
			DAEConnectionPtr inputConnection = GetInputConnection(ch);
			if (inputConnection == NULL)
				return (unsigned int)ch;
		}
	}
	return (unsigned long) GetNumInputs();
#endif
}

//-----------------------------------------------------------------------------
// return the number of audio outputs
unsigned long DfxPlugin::getnumoutputs()
{
#ifdef TARGET_API_AUDIOUNIT
	if ( Outputs().GetNumberOfElements() > 0 )
		return GetStreamFormat(kAudioUnitScope_Output, (AudioUnitElement)0).NumberChannels();
	else
		return 0;
#endif

#ifdef TARGET_API_VST
	return numOutputs;
#endif

#ifdef TARGET_API_RTAS
	if ( IsAS() && audioIsRendering )	// XXX checking connections is only valid while rendering
	{
		for (SInt32 ch=0; ch < GetNumOutputs(); ch++)
		{
			DAEConnectionPtr outputConnection = GetOutputConnection(ch);
			if (outputConnection == NULL)
				return (unsigned int)ch;
		}
	}
	return (unsigned long) GetNumOutputs();
#endif
}

//-----------------------------------------------------------------------------
// add an audio input/output configuration to the array of i/o configurations
void DfxPlugin::addchannelconfig(short inNumInputChannels, short inNumOutputChannels)
{
	if (channelconfigs != NULL)
	{
		DfxChannelConfig * swapconfigs = (DfxChannelConfig*) malloc(sizeof(*channelconfigs) * (numchannelconfigs+1));
		memcpy(swapconfigs, channelconfigs, sizeof(*channelconfigs) * numchannelconfigs);
		free(channelconfigs);
		channelconfigs = swapconfigs;
		channelconfigs[numchannelconfigs].inChannels = inNumInputChannels;
		channelconfigs[numchannelconfigs].outChannels = inNumOutputChannels;
		numchannelconfigs++;
	}
	else
	{
		channelconfigs = (DfxChannelConfig*) malloc(sizeof(*channelconfigs));
		channelconfigs[0].inChannels = inNumInputChannels;
		channelconfigs[0].outChannels = inNumOutputChannels;
		numchannelconfigs = 1;
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setlatency_samples(long newlatency)
{
	bool changed = false;
	if (b_uselatency_seconds)
		changed = true;
	else if (latency_samples != newlatency)
		changed = true;

	latency_samples = newlatency;
	b_uselatency_seconds = false;

	if (changed)
		update_latency();
}

//-----------------------------------------------------------------------------
void DfxPlugin::setlatency_seconds(double newlatency)
{
	bool changed = false;
	if (!b_uselatency_seconds)
		changed = true;
	else if (latency_seconds != newlatency)
		changed = true;

	latency_seconds = newlatency;
	b_uselatency_seconds = true;

	if (changed)
		update_latency();
}

//-----------------------------------------------------------------------------
long DfxPlugin::getlatency_samples()
{
	if (b_uselatency_seconds)
		return (long) (latency_seconds * getsamplerate());
	else
		return latency_samples;
}

//-----------------------------------------------------------------------------
double DfxPlugin::getlatency_seconds()
{
	if (b_uselatency_seconds)
		return latency_seconds;
	else
		return (double)latency_samples / getsamplerate();
}

//-----------------------------------------------------------------------------
void DfxPlugin::update_latency()
{
	#ifdef TARGET_API_AUDIOUNIT
		PropertyChanged(kAudioUnitProperty_Latency, kAudioUnitScope_Global, (AudioUnitElement)0);
	#endif
}

//-----------------------------------------------------------------------------
void DfxPlugin::settailsize_samples(long newsize)
{
	bool changed = false;
	if (b_usetailsize_seconds)
		changed = true;
	else if (tailsize_samples != newsize)
		changed = true;

	tailsize_samples = newsize;
	b_usetailsize_seconds = false;

	if (changed)
		update_tailsize();
}

//-----------------------------------------------------------------------------
void DfxPlugin::settailsize_seconds(double newsize)
{
	bool changed = false;
	if (!b_usetailsize_seconds)
		changed = true;
	else if (tailsize_seconds != newsize)
		changed = true;

	tailsize_seconds = newsize;
	b_usetailsize_seconds = true;

	if (changed)
		update_tailsize();
}

//-----------------------------------------------------------------------------
long DfxPlugin::gettailsize_samples()
{
	if (b_usetailsize_seconds)
		return (long) (tailsize_seconds * getsamplerate());
	else
		return tailsize_samples;
}

//-----------------------------------------------------------------------------
double DfxPlugin::gettailsize_seconds()
{
	if (b_usetailsize_seconds)
		return tailsize_seconds;
	else
		return (double)tailsize_samples / getsamplerate();
}

//-----------------------------------------------------------------------------
void DfxPlugin::update_tailsize()
{
	#ifdef TARGET_API_AUDIOUNIT
		PropertyChanged(kAudioUnitProperty_TailTime, kAudioUnitScope_Global, (AudioUnitElement)0);
	#endif
}

//-----------------------------------------------------------------------------
void DfxPlugin::setAudioProcessingMustAccumulate(bool inNewMode)
{
	audioProcessingAccumulatingOnly = inNewMode;
	if (inNewMode)
	{
	#ifdef TARGET_API_AUDIOUNIT
		#if !TARGET_PLUGIN_IS_INSTRUMENT
			SetProcessesInPlace(false);
		#endif
	#endif
	#if defined(TARGET_API_VST) && !VST_FORCE_DEPRECATED
		canProcessReplacing(false);	// XXX can't depend on this anymore
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
	timeinfo.tempo = 120.0;
	timeinfo.tempoIsValid = false;
	timeinfo.beatPos = 0.0;
	timeinfo.beatPosIsValid = false;
	timeinfo.barPos = 0.0;
	timeinfo.barPosIsValid = false;
	timeinfo.numerator = 4.0;
	timeinfo.denominator = 4.0;
	timeinfo.timeSigIsValid = false;
	timeinfo.samplesToNextBar = 0;
	timeinfo.samplesToNextBarIsValid = false;
	timeinfo.playbackChanged = false;
	timeinfo.playbackIsOccurring = true;


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
		timeinfo.tempoIsValid = true;
		timeinfo.tempo = tempo;
		timeinfo.beatPosIsValid = true;
		timeinfo.beatPos = beat;

		hostCanDoTempo = true;
	}
#ifdef DFX_DEBUG_PRINT_MUSICAL_TIME_INFO
else fprintf(stderr, "CallHostBeatAndTempo() error %ld\n", status);
#endif

	// the number of samples until the next beat from the start sample of the current rendering buffer
//	UInt32 sampleOffsetToNextBeat = 0;	// XXX should I just send NULL since we don't use this?
	// the number of beats of the denominator value that contained in the current measure
	Float32 timeSigNumerator = 4.0f;
	// music notational conventions (4 is a quarter note, 8 an eighth note, etc)
	UInt32 timeSigDenominator = 4;
	// the beat that corresponds to the downbeat (first beat) of the current measure
	Float64 currentMeasureDownBeat = 0.0;
	status = CallHostMusicalTimeLocation(NULL, &timeSigNumerator, &timeSigDenominator, &currentMeasureDownBeat);
	if (status == noErr)
	{
		// get the song beat position of the beginning of the current measure
		timeinfo.barPosIsValid = true;
		timeinfo.barPos = currentMeasureDownBeat;
#ifdef DFX_DEBUG_PRINT_MUSICAL_TIME_INFO
fprintf(stderr, "time sig = %.0f/%lu\nmeasure beat = %.2f\n", timeSigNumerator, timeSigDenominator, currentMeasureDownBeat);
#endif
		// get the numerator of the time signature - this is the number of beats per measure
		timeinfo.timeSigIsValid = true;
		timeinfo.numerator = (double) timeSigNumerator;
		timeinfo.denominator = (double) timeSigDenominator;
	}
#ifdef DFX_DEBUG_PRINT_MUSICAL_TIME_INFO
else fprintf(stderr, "CallHostMusicalTimeLocation() error %ld\n", status);
#endif

	// check if the host is a buggy one that will crash TransportStateProc
	static const bool auTransportStateIsSafe = IsTransportStateProcSafe();
	if (auTransportStateIsSafe)
	{
		Boolean isPlaying = true;
		Boolean transportStateChanged = false;
//		Float64 currentSampleInTimeLine = 0.0;
//		Boolean isCycling = false;
//		Float64 cycleStartBeat = 0.0, cycleEndBeat = 0.0;
//		status = CallHostTransportState(&isPlaying, &transportStateChanged, &currentSampleInTimeLine, &isCycling, &cycleStartBeat, &cycleEndBeat);
		status = CallHostTransportState(&isPlaying, &transportStateChanged, NULL, NULL, NULL, NULL);
		// determine whether the playback position or state has just changed
		if (status == noErr)
		{
#ifdef DFX_DEBUG_PRINT_MUSICAL_TIME_INFO
fprintf(stderr, "is playing = %s\ntransport changed = %s\n", isPlaying ? "true" : "false", transportStateChanged ? "true" : "false");
#endif
			timeinfo.playbackChanged = transportStateChanged;
			timeinfo.playbackIsOccurring = isPlaying;
		}
#ifdef DFX_DEBUG_PRINT_MUSICAL_TIME_INFO
else fprintf(stderr, "CallHostTransportState() error %ld\n", status);
#endif
	}
#endif
// TARGET_API_AUDIOUNIT
 

#ifdef TARGET_API_VST
	VstTimeInfo * vstTimeInfo = getTimeInfo(kVstTempoValid 
										| kVstTransportChanged 
										| kVstBarsValid 
										| kVstPpqPosValid 
										| kVstTimeSigValid);

 	// there's nothing we can do in that case
	if (vstTimeInfo != NULL)
	{
		if (kVstTempoValid & vstTimeInfo->flags)
		{
			timeinfo.tempoIsValid = true;
			timeinfo.tempo = vstTimeInfo->tempo;
		}

		// get the song beat position of our precise current location
		if (kVstPpqPosValid & vstTimeInfo->flags)
		{
			timeinfo.beatPosIsValid = true;
			timeinfo.beatPos = vstTimeInfo->ppqPos;
		}

		// get the song beat position of the beginning of the current measure
		if (kVstBarsValid & vstTimeInfo->flags)
		{
			timeinfo.barPosIsValid = true;
			timeinfo.barPos = vstTimeInfo->barStartPos;
		}

		// get the numerator of the time signature - this is the number of beats per measure
		if (kVstTimeSigValid & vstTimeInfo->flags)
		{
			timeinfo.timeSigIsValid = true;
			timeinfo.numerator = (double) vstTimeInfo->timeSigNumerator;
			timeinfo.denominator = (double) vstTimeInfo->timeSigDenominator;
		}

		// determine whether the playback position or state has just changed
		if (kVstTransportChanged & vstTimeInfo->flags)
			timeinfo.playbackChanged = true;

		if (kVstTransportPlaying & vstTimeInfo->flags)
			timeinfo.playbackIsOccurring = true;
	}
#endif
// TARGET_API_VST


	if (timeinfo.tempo <= 0.0)
		timeinfo.tempo = 120.0;
	timeinfo.tempo_bps = timeinfo.tempo / 60.0;
	timeinfo.samplesPerBeat = (long) (getsamplerate() / timeinfo.tempo_bps);

	if (timeinfo.tempoIsValid && timeinfo.beatPosIsValid && 
			timeinfo.barPosIsValid && timeinfo.timeSigIsValid)
		timeinfo.samplesToNextBarIsValid = true;

	// it will screw up the while loop below bigtime if the numerator isn't a positive number
	if (timeinfo.numerator <= 0.0)
		timeinfo.numerator = 4.0;
	if (timeinfo.denominator <= 0.0)
		timeinfo.denominator = 4.0;

	// calculate the number of samples frames from now until the next measure begins
	if (timeinfo.samplesToNextBarIsValid)
	{
		double numBeatsToBar;
		// calculate the distance in beats to the upcoming measure beginning point
		if (timeinfo.barPos == timeinfo.beatPos)
			numBeatsToBar = 0.0;
		else
		{
			numBeatsToBar = timeinfo.barPos + timeinfo.numerator - timeinfo.beatPos;
			// do this stuff because some hosts (Cubase) give kind of wacky barStartPos sometimes
			while (numBeatsToBar < 0.0)
				numBeatsToBar += timeinfo.numerator;
			while (numBeatsToBar > timeinfo.numerator)
				numBeatsToBar -= timeinfo.numerator;
		}
	
		// convert the value for the distance to the next measure from beats to samples
		timeinfo.samplesToNextBar = (long) (numBeatsToBar * getsamplerate() / timeinfo.tempo_bps);
		// protect against wacky values
		if (timeinfo.samplesToNextBar < 0)
			timeinfo.samplesToNextBar = 0;
	}
}


//-----------------------------------------------------------------------------
// this is called immediately before processing a block of audio
void DfxPlugin::preprocessaudio()
{
	audioIsRendering = true;

	#if TARGET_PLUGIN_USES_MIDI
		midistuff->preprocessEvents();
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
	for (long i=0; i < numParameters; i++)
	{
		setparameterchanged(i, false);
		setparametertouched(i, false);
	}

	#if TARGET_PLUGIN_USES_MIDI
		midistuff->postprocessEvents();
	#endif

	audioIsRendering = false;
}

//-----------------------------------------------------------------------------
// non-virtual function called to insure that processparameters happens
void DfxPlugin::do_processparameters()
{
	processparameters();
}



#pragma mark -
#pragma mark MIDI

#if TARGET_PLUGIN_USES_MIDI

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_noteon(int inChannel, int inNote, int inVelocity, long inFrameOffset)
{
#ifdef DFX_DEBUG_PRINT_MUSIC_EVENTS
fprintf(stderr, "note on:  note = %d, velocity = %d, channel = %d, sample offset = %ld\n", inNote, inVelocity, inChannel, inFrameOffset);
#endif
	if (midistuff != NULL)
		midistuff->handleNoteOn(inChannel, inNote, inVelocity, inFrameOffset);
	if (dfxsettings != NULL)
		dfxsettings->handleNoteOn(inChannel, inNote, inVelocity, inFrameOffset);
}

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_noteoff(int inChannel, int inNote, int inVelocity, long inFrameOffset)
{
#ifdef DFX_DEBUG_PRINT_MUSIC_EVENTS
fprintf(stderr, "note off:  note = %d, velocity = %d, channel = %d, sample offset = %ld\n", inNote, inVelocity, inChannel, inFrameOffset);
#endif
	if (midistuff != NULL)
		midistuff->handleNoteOff(inChannel, inNote, inVelocity, inFrameOffset);
	if (dfxsettings != NULL)
		dfxsettings->handleNoteOff(inChannel, inNote, inVelocity, inFrameOffset);
}

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_allnotesoff(int inChannel, long inFrameOffset)
{
#ifdef DFX_DEBUG_PRINT_MUSIC_EVENTS
fprintf(stderr, "all notes off:  channel = %d, sample offset = %ld\n", inChannel, inFrameOffset);
#endif
	if (midistuff != NULL)
		midistuff->handleAllNotesOff(inChannel, inFrameOffset);
	if (dfxsettings != NULL)
		dfxsettings->handleAllNotesOff(inChannel, inFrameOffset);
}

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_pitchbend(int inChannel, int inValueLSB, int inValueMSB, long inFrameOffset)
{
#ifdef DFX_DEBUG_PRINT_MUSIC_EVENTS
fprintf(stderr, "pitchbend:  LSB = %d, MSB = %d, channel = %d, sample offset = %ld\n", inValueLSB, inValueMSB, inChannel, inFrameOffset);
#endif
	if (midistuff != NULL)
		midistuff->handlePitchBend(inChannel, inValueLSB, inValueMSB, inFrameOffset);
	if (dfxsettings != NULL)
		dfxsettings->handlePitchBend(inChannel, inValueLSB, inValueMSB, inFrameOffset);
}

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_cc(int inChannel, int inControllerNum, int inValue, long inFrameOffset)
{
#ifdef DFX_DEBUG_PRINT_MUSIC_EVENTS
fprintf(stderr, "MIDI CC:  controller = 0x%02X, value = %d, channel = %d, sample offset = %ld\n", inControllerNum, inValue, inChannel, inFrameOffset);
#endif
	if (midistuff != NULL)
		midistuff->handleCC(inChannel, inControllerNum, inValue, inFrameOffset);
	if (dfxsettings != NULL)
		dfxsettings->handleCC(inChannel, inControllerNum, inValue, inFrameOffset);
}

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_programchange(int inChannel, int inProgramNum, long inFrameOffset)
{
#ifdef DFX_DEBUG_PRINT_MUSIC_EVENTS
fprintf(stderr, "program change:  program num = %d, channel = %d, sample offset = %ld\n", inProgramNum, inChannel, inFrameOffset);
#endif
	if (midistuff != NULL)
		midistuff->handleProgramChange(inChannel, inProgramNum, inFrameOffset);
}

#endif
// TARGET_PLUGIN_USES_MIDI






#pragma mark -
#pragma mark --- helper functions ---

#ifndef TARGET_API_AUDIOSUITE

//-----------------------------------------------------------------------------
long DFX_CompositePluginVersionNumberValue()
{
	return (PLUGIN_VERSION_MAJOR << 16) | (PLUGIN_VERSION_MINOR << 8) | PLUGIN_VERSION_BUGFIX;
}

#endif
// !TARGET_API_AUDIOSUITE
