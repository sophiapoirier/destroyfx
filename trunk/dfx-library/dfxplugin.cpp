/*------------------------------------------------------------------------
Destroy FX is a sovereign entity comprised of Marc Poirier & Tom Murphy 7.  
This is our class for E-Z plugin-making and E-Z multiple-API support.
written by Marc Poirier, October 2002
------------------------------------------------------------------------*/

#include "dfxplugin.h"

#include <time.h>	// for time(), which is used to feed srand()

#ifdef TARGET_API_AUDIOUNIT
	#include <AudioToolbox/AudioUnitUtilities.h>	// for AUParameterListenerNotify
#endif

#if defined(TARGET_API_VST) && TARGET_PLUGIN_HAS_GUI && defined(TARGET_PLUGIN_USES_VSTGUI)
/* If using the VST GUI interface, we need the class definition
   for AEffGUIEditor so we can send it parameter changes.
 */
	#include "vstgui.h"
// XXX Tom, shouldn't that be "vstgui.h" like it was before?
//     I don't see any aeffguieditor.h file.
//  Yeah. AEffGUIEditor is defined in that file.

#endif



#pragma mark _________---DfxPlugin---_________

#pragma mark _________init_________

//-----------------------------------------------------------------------------
DfxPlugin::DfxPlugin(
					TARGET_API_BASE_INSTANCE_TYPE inInstance
					, long numParameters
					, long numPresets
					)

// setup the constructors of the inherited base classes, for the appropriate API
#ifdef TARGET_API_AUDIOUNIT
	#if TARGET_PLUGIN_IS_INSTRUMENT
		: TARGET_API_BASE_CLASS(inInstance, UInt32 numInputs, UInt32 numOutputs, UInt32 numGroups = 0), 
	#else
		: TARGET_API_BASE_CLASS(inInstance), 
	#endif
#endif

#ifdef TARGET_API_VST
	: TARGET_API_BASE_CLASS(inInstance, numPresets, numParameters), 
	numInputs(VST_NUM_INPUTS), numOutputs(VST_NUM_OUTPUTS), 
#endif
// end API-specific base constructors

	numParameters(numParameters), numPresets(numPresets)
{
	parameters = NULL;
	presets = NULL;
	channelconfigs = NULL;
	tempoRateTable = NULL;

	#if TARGET_PLUGIN_USES_MIDI
		midistuff = NULL;
		dfxsettings = NULL;
	#endif

	numchannelconfigs = 0;

	updatesamplerate();	// XXX have it set to something here?
	sampleratechanged = true;
	hostCanDoTempo = false;	// until proven otherwise

	latency_samples = 0;
	latency_seconds = 0.0;
	b_uselatency_seconds = false;

	tailsize_samples = 0;
	tailsize_seconds = 0.0;
	b_usetailsize_seconds = false;

	// set a seed value for rand() from the system clock
	srand((unsigned int)time(NULL));

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


#ifdef TARGET_API_AUDIOUNIT
	inputsP = outputsP = NULL;

	aupresets = NULL;
	if (numPresets > 0)
	{
		aupresets = (AUPreset*) malloc(numPresets * sizeof(AUPreset));
		for (long i=0; i < numPresets; i++)
		{
			aupresets[i].presetNumber = i;
			aupresets[i].presetName = NULL;	// XXX eh?
		}
	}
	#if TARGET_PLUGIN_USES_MIDI
		aumidicontrolmap = NULL;
		if (numParameters > 0)
			aumidicontrolmap = (AudioUnitMIDIControlMapping*) malloc(numParameters * sizeof(AudioUnitMIDIControlMapping));
	#endif

#endif
// Audio Unit stuff


#ifdef TARGET_API_VST
	setUniqueID(PLUGIN_ID);	// identify
	setNumInputs(VST_NUM_INPUTS);
	setNumOutputs(VST_NUM_OUTPUTS);
	#if VST_NUM_INPUTS == 2
		canMono();	// it's okay to feed a double-mono (fake stereo) input
	#endif

	#if TARGET_PLUGIN_IS_INSTRUMENT
		isSynth();
	#endif

	canProcessReplacing();	// supports both accumulating and replacing output
	TARGET_API_BASE_CLASS::setProgram(0);	// set the current preset number to 0

	isinitialized = false;
	setlatencychanged(false);

	// check to see if the host supports sending tempo & time information to VST plugins
	hostCanDoTempo = (canHostDo("sendVstTimeInfo") == 1);

	#if TARGET_PLUGIN_USES_DSPCORE
		dspcores = (DfxPluginCore**) malloc(getnumoutputs() * sizeof(DfxPluginCore*));
		// need to save instantiating the cores for the inheriting plugin class constructor
		for (unsigned long ii=0; ii < getnumoutputs(); ii++)
			dspcores[ii] = NULL;
	#endif

	#if TARGET_PLUGIN_USES_MIDI
		// tell host that we want to use special data chunks for settings storage
		programsAreChunks();
	#endif

#endif
// VST stuff

}

//-----------------------------------------------------------------------------
// this is called immediately after all constructors (DfxPlugin and any derived classes) complete
void DfxPlugin::dfxplugin_postconstructor()
{
	// set up a name for the default preset if none was set
	if ( !presetnameisvalid(0) )
		setpresetname(0, PLUGIN_NAME_STRING);
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

	#ifdef TARGET_API_AUDIOUNIT
		if (aupresets != NULL)
			free(aupresets);
		aupresets = NULL;
		#if TARGET_PLUGIN_USES_MIDI
			if (aumidicontrolmap != NULL)
				free(aumidicontrolmap);
			aumidicontrolmap = NULL;
		#endif
	#endif
	// end AudioUnit-specific destructor stuff

	#ifdef TARGET_API_VST
		// the child plugin class destructor should call do_cleanup for VST 
		// because VST has no initialize/cleanup sort of methods, but if 
		// the child class doesn't have its own cleanup method, then it's 
		// not necessary, and so we can do it now if it wasn't done
		if (isinitialized)
			do_cleanup();
		#if TARGET_PLUGIN_USES_DSPCORE
			if (dspcores != NULL)
			{
				for (unsigned long i=0; i < getnumoutputs(); i++)
				{
					if (dspcores[i] != NULL)
						delete dspcores[i];
					dspcores[i] = NULL;
				}
				free(dspcores);
			}
			dspcores = NULL;
		#endif
	#endif
	// end VST-specific destructor stuff
}

//-----------------------------------------------------------------------------
// this is called immediately before all destructors (DfxPlugin and any derived classes) occur
void DfxPlugin::dfxplugin_predestructor()
{
	#ifdef TARGET_API_VST
		// VST doesn't have initialize and cleanup methods like Audio Unit does, 
		// so we need to call this manually here
		do_cleanup();	// XXX need to actually implement predestructor for VST!!!
	#endif
}


//-----------------------------------------------------------------------------
// non-virtual function that calls initialize() and insures that some stuff happens
long DfxPlugin::do_initialize()
{
	updatesamplerate();
	updatenumchannels();

	long result = initialize();
	if (result != 0)
		return result;

	#ifdef TARGET_API_VST
		isinitialized = true;
	#endif

	bool buffersCreated = createbuffers();
	if (buffersCreated == false)
		return kDfxErr_InitializationFailed;

	do_reset();

	return kDfxErr_NoError;	// no error
}

//-----------------------------------------------------------------------------
// non-virtual function that calls cleanup() and insures that some stuff happens
void DfxPlugin::do_cleanup()
{
	releasebuffers();

	#ifdef TARGET_API_AUDIOUNIT
		if (inputsP != NULL)
			free(inputsP);
		inputsP = NULL;
		if (outputsP != NULL)
			free(outputsP);
		outputsP = NULL;
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
	#if TARGET_PLUGIN_USES_MIDI
		if (midistuff != NULL)
			midistuff->reset();
	#endif

	reset();
	clearbuffers();
}



#pragma mark _________parameters_________

//-----------------------------------------------------------------------------
void DfxPlugin::initparameter_f(long parameterIndex, const char *initName, float initValue, 
						float initDefaultValue, float initMin, float initMax, 
						DfxParamUnit initUnit, DfxParamCurve initCurve, 
						const char *initCustomUnitString)
{
	if (parameterisvalid(parameterIndex))
	{
		parameters[parameterIndex].init_f(initName, initValue, initDefaultValue, initMin, initMax, initUnit, initCurve);
		update_parameter(parameterIndex);	// make the host aware of the parameter change
		initpresetsparameter(parameterIndex);	// default empty presets with this value
		// set the custom unit string, if there is one
		if (initCustomUnitString != NULL)
			setparametercustomunitstring(parameterIndex, initCustomUnitString);
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::initparameter_d(long parameterIndex, const char *initName, double initValue, 
						double initDefaultValue, double initMin, double initMax, 
						DfxParamUnit initUnit, DfxParamCurve initCurve, 
						const char *initCustomUnitString)
{
	if (parameterisvalid(parameterIndex))
	{
		parameters[parameterIndex].init_d(initName, initValue, initDefaultValue, initMin, initMax, initUnit, initCurve);
		update_parameter(parameterIndex);	// make the host aware of the parameter change
		initpresetsparameter(parameterIndex);	// default empty presets with this value
		// set the custom unit string, if there is one
		if (initCustomUnitString != NULL)
			setparametercustomunitstring(parameterIndex, initCustomUnitString);
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::initparameter_i(long parameterIndex, const char *initName, long initValue, 
						long initDefaultValue, long initMin, long initMax, 
						DfxParamUnit initUnit, DfxParamCurve initCurve, 
						const char *initCustomUnitString)
{
	if (parameterisvalid(parameterIndex))
	{
		parameters[parameterIndex].init_i(initName, initValue, initDefaultValue, initMin, initMax, initUnit, initCurve);
		update_parameter(parameterIndex);	// make the host aware of the parameter change
		initpresetsparameter(parameterIndex);	// default empty presets with this value
		// set the custom unit string, if there is one
		if (initCustomUnitString != NULL)
			setparametercustomunitstring(parameterIndex, initCustomUnitString);
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::initparameter_ui(long parameterIndex, const char *initName, unsigned long initValue, 
						unsigned long initDefaultValue, unsigned long initMin, unsigned long initMax, 
						DfxParamUnit initUnit, DfxParamCurve initCurve, 
						const char *initCustomUnitString)
{
	if (parameterisvalid(parameterIndex))
	{
		parameters[parameterIndex].init_ui(initName, initValue, initDefaultValue, initMin, initMax, initUnit, initCurve);
		update_parameter(parameterIndex);	// make the host aware of the parameter change
		initpresetsparameter(parameterIndex);	// default empty presets with this value
		// set the custom unit string, if there is one
		if (initCustomUnitString != NULL)
			setparametercustomunitstring(parameterIndex, initCustomUnitString);
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::initparameter_b(long parameterIndex, const char *initName, bool initValue, bool initDefaultValue, 
						DfxParamUnit initUnit)
{
	if (parameterisvalid(parameterIndex))
	{
		parameters[parameterIndex].init_b(initName, initValue, initDefaultValue, initUnit);
		update_parameter(parameterIndex);	// make the host aware of the parameter change
		initpresetsparameter(parameterIndex);	// default empty presets with this value
	}
}

//-----------------------------------------------------------------------------
// this is a shorcut for initializing a parameter that uses integer indexes 
// into an array, with an array of strings representing its values
void DfxPlugin::initparameter_indexed(long parameterIndex, const char *initName, long initValue, long initDefaultValue, long initNumItems, 
						DfxParamUnit initUnit, const char *initCustomUnitString)
{
	if (parameterisvalid(parameterIndex))
	{
		parameters[parameterIndex].init_i(initName, initValue, initDefaultValue, 0, initNumItems-1, initUnit, kDfxParamCurve_stepped);
		setparameterusevaluestrings(parameterIndex, true);	// indicate that we will use custom value display strings
		update_parameter(parameterIndex);	// make the host aware of the parameter change
		initpresetsparameter(parameterIndex);	// default empty presets with this value
		// set the custom unit string, if there is one
		if (initCustomUnitString != NULL)
			setparametercustomunitstring(parameterIndex, initCustomUnitString);
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameter(long parameterIndex, DfxParamValue newValue)
{
	if (parameterisvalid(parameterIndex))
	{
		parameters[parameterIndex].set(newValue);
		update_parameter(parameterIndex);	// make the host aware of the parameter change
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameter_f(long parameterIndex, float newValue)
{
	if (parameterisvalid(parameterIndex))
	{
		parameters[parameterIndex].set_f(newValue);
		update_parameter(parameterIndex);	// make the host aware of the parameter change
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameter_d(long parameterIndex, double newValue)
{
	if (parameterisvalid(parameterIndex))
	{
		parameters[parameterIndex].set_d(newValue);
		update_parameter(parameterIndex);	// make the host aware of the parameter change
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameter_i(long parameterIndex, long newValue)
{
	if (parameterisvalid(parameterIndex))
	{
		parameters[parameterIndex].set_i(newValue);
		update_parameter(parameterIndex);	// make the host aware of the parameter change
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameter_b(long parameterIndex, bool newValue)
{
	if (parameterisvalid(parameterIndex))
	{
		parameters[parameterIndex].set_b(newValue);
		update_parameter(parameterIndex);	// make the host aware of the parameter change
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameter_gen(long parameterIndex, float newValue)
{
	if (parameterisvalid(parameterIndex))
	{
		parameters[parameterIndex].set_gen(newValue);
		update_parameter(parameterIndex);	// make the host aware of the parameter change
	}
}

//-----------------------------------------------------------------------------
void DfxPlugin::randomizeparameter(long parameterIndex)
{
	if (parameterisvalid(parameterIndex))
	{
		parameters[parameterIndex].randomize();
		update_parameter(parameterIndex);	// make the host aware of the parameter change
	}
}

//-----------------------------------------------------------------------------
// randomize all of the parameters at once
void DfxPlugin::randomizeparameters(bool writeAutomation)
{
	for (long i=0; i < numParameters; i++)
	{
		randomizeparameter(i);
		postupdate_parameter(i);	// inform any parameter listeners of the changes

	#ifdef TARGET_API_VST
		if (writeAutomation)
			setParameterAutomated(i, getparameter_gen(i));
	#endif
	}
}

//-----------------------------------------------------------------------------
// do stuff necessary to inform the host of changes, etc.
void DfxPlugin::update_parameter(long parameterIndex)
{
	#ifdef TARGET_API_AUDIOUNIT
		// make the global-scope element aware of the parameter's value
		AUBase::SetParameter(parameterIndex, kAudioUnitScope_Global, (AudioUnitElement)0, getparameter_f(parameterIndex), 0);
	#endif

	#ifdef TARGET_API_VST
		long vstpresetnum = TARGET_API_BASE_CLASS::getProgram();
		if (presetisvalid(vstpresetnum))
			setpresetparameter(vstpresetnum, parameterIndex, getparameter(parameterIndex));
		#if TARGET_PLUGIN_HAS_GUI
			#ifdef TARGET_PLUGIN_USES_VSTGUI
			if (editor != NULL)	/* XXX can't assume it's a GUI (ie, vstgui) editor! */
				((AEffGUIEditor*)editor)->setParameter(parameterIndex, getparameter_gen(parameterIndex));
			#else
			// XXX we will need something for our GUI class here
			#endif
		#endif

	#endif
}

//-----------------------------------------------------------------------------
// this will broadcast a notification to anyone interested (host, GUI, etc.) 
// about a parameter change
void DfxPlugin::postupdate_parameter(long parameterIndex)
{
	if ( !parameterisvalid(parameterIndex) )
		return;

	#ifdef TARGET_API_AUDIOUNIT
		AudioUnitParameter dirtyparam;
		dirtyparam.mAudioUnit = GetComponentInstance();
		dirtyparam.mParameterID = parameterIndex;
		dirtyparam.mScope = kAudioUnitScope_Global;
		dirtyparam.mElement = 0;
		AUParameterListenerNotify(NULL, NULL, &dirtyparam);
	#endif
}

//-----------------------------------------------------------------------------
// return a (hopefully) 0 to 1 scalar version of the parameter's current value
float DfxPlugin::getparameter_scalar(long parameterIndex)
{
	if (parameterisvalid(parameterIndex))
	{
		switch (getparameterunit(parameterIndex))
		{
			case kDfxParamUnit_percent:
			case kDfxParamUnit_drywetmix:
				return parameters[parameterIndex].get_f() / 100.0f;
			case kDfxParamUnit_portion:
			case kDfxParamUnit_scalar:
				return parameters[parameterIndex].get_f();
			default:
				return parameters[parameterIndex].get_f() / parameters[parameterIndex].getmax_f();
		}
	}
	else
		return 0.0f;
}

//-----------------------------------------------------------------------------
void DfxPlugin::getparametername(long parameterIndex, char *text)
{
	if (text != NULL)
	{
		if (parameterisvalid(parameterIndex))
			parameters[parameterIndex].getname(text);
		else
			text[0] = 0;
	}
}

//-----------------------------------------------------------------------------
DfxParamValueType DfxPlugin::getparametervaluetype(long parameterIndex)
{
	if (parameterisvalid(parameterIndex))
		return parameters[parameterIndex].getvaluetype();
	else
		return kDfxParamValueType_undefined;
}

//-----------------------------------------------------------------------------
DfxParamUnit DfxPlugin::getparameterunit(long parameterIndex)
{
	if (parameterisvalid(parameterIndex))
		return parameters[parameterIndex].getunit();
	else
		return kDfxParamUnit_undefined;
}

//-----------------------------------------------------------------------------
bool DfxPlugin::setparametervaluestring(long parameterIndex, long stringIndex, const char *inText)
{
	if (parameterisvalid(parameterIndex))
		return parameters[parameterIndex].setvaluestring(stringIndex, inText);
	else
		return false;
}

//-----------------------------------------------------------------------------
bool DfxPlugin::getparametervaluestring(long parameterIndex, long stringIndex, char *outText)
{
	if (parameterisvalid(parameterIndex))
		return parameters[parameterIndex].getvaluestring(stringIndex, outText);
	else
		return false;
}

//-----------------------------------------------------------------------------
char * DfxPlugin::getparametervaluestring_ptr(long parameterIndex, long stringIndex)
{	if (parameterisvalid(parameterIndex))
		return parameters[parameterIndex].getvaluestring_ptr(stringIndex);
	else
		return 0;
}

//-----------------------------------------------------------------------------
bool DfxPlugin::getparameterchanged(long parameterIndex)
{
	if (parameterisvalid(parameterIndex))
		return parameters[parameterIndex].getchanged();
	else
		return false;
}

//-----------------------------------------------------------------------------
void DfxPlugin::setparameterchanged(long parameterIndex, bool newChanged)
{
	if (parameterisvalid(parameterIndex))
		parameters[parameterIndex].setchanged(newChanged);
}

//-----------------------------------------------------------------------------
// convenience methods for expanding and contracting parameter values 
// using the min/max/curvetype/curvespec/etc. settings of a given parameter
double DfxPlugin::expandparametervalue_index(long parameterIndex, float genValue)
{
	if (parameterisvalid(parameterIndex))
		return expandparametervalue((double)genValue, getparametermin_d(parameterIndex), getparametermax_d(parameterIndex), 
									getparametercurve(parameterIndex), getparametercurvespec(parameterIndex));
	else
		return 0.0;
}
//-----------------------------------------------------------------------------
float DfxPlugin::contractparametervalue_index(long parameterIndex, double realValue)
{
	if (parameterisvalid(parameterIndex))
		return (float) contractparametervalue(realValue, getparametermin_d(parameterIndex), getparametermax_d(parameterIndex), 
									getparametercurve(parameterIndex), getparametercurvespec(parameterIndex));
	else
		return 0.0f;
}



#pragma mark _________presets_________

//-----------------------------------------------------------------------------
// whether or not the index is a valid preset
bool DfxPlugin::presetisvalid(long presetIndex)
{
	if (presets == NULL)
		return false;
	if ( (presetIndex < 0) || (presetIndex >= numPresets) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// whether or not the index is a valid preset with a valid name
// this is mostly just for Audio Unit
bool DfxPlugin::presetnameisvalid(long presetIndex)
{
	// still do this check to avoid bad pointer access
	if (!presetisvalid(presetIndex))
		return false;

	if (presets[presetIndex].getname_ptr() == NULL)
		return false;
	if ( (presets[presetIndex].getname_ptr())[0] == 0 )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// load the settings of a preset
bool DfxPlugin::loadpreset(long presetIndex)
{
	if ( !presetisvalid(presetIndex) )
		return false;

	for (long i=0; i < numParameters; i++)
	{
		setparameter(i, getpresetparameter(presetIndex, i));
		postupdate_parameter(i);	// inform any parameter listeners of the changes
	}

	// do stuff necessary to inform the host of changes, etc.
	update_preset(presetIndex);
	return true;
}

//-----------------------------------------------------------------------------
// do stuff necessary to inform the host of changes, etc.
void DfxPlugin::update_preset(long presetIndex)
{
	currentPresetNum = presetIndex;

	#ifdef TARGET_API_AUDIOUNIT
		AUPreset au_preset;
		au_preset.presetNumber = presetIndex;
		au_preset.presetName = getpresetcfname(presetIndex);
		SetAFactoryPresetAsCurrent(au_preset);
		PropertyChanged(kAudioUnitProperty_CurrentPreset, kAudioUnitScope_Global, (AudioUnitElement)0);
	#endif

	#ifdef TARGET_API_VST
		TARGET_API_BASE_CLASS::setProgram(presetIndex);
		// tell the host to update the generic editor display with the new settings
		AudioEffectX::updateDisplay();

	#endif
}

//-----------------------------------------------------------------------------
// default all empty (no name) presets with the current value of a parameter
void DfxPlugin::initpresetsparameter(long parameterIndex)
{
	// first fill in the presets with the init settings 
	// so that there are no "stale" unset values
	for (long i=0; i < numPresets; i++)
	{
		if ( !presetnameisvalid(i) )	// only if it's an "empty" preset
			setpresetparameter(i, parameterIndex, getparameter(parameterIndex));
	}
}

//-----------------------------------------------------------------------------
DfxParamValue DfxPlugin::getpresetparameter(long presetIndex, long parameterIndex)
{
	if ( parameterisvalid(parameterIndex) && presetisvalid(presetIndex) )
		return presets[presetIndex].values[parameterIndex];
	else
	{
		DfxParamValue dummy;
		memset(&dummy, 0, sizeof(DfxParamValue));
		return dummy;
	}
}

//-----------------------------------------------------------------------------
float DfxPlugin::getpresetparameter_f(long presetIndex, long parameterIndex)
{
	if ( parameterisvalid(parameterIndex) && presetisvalid(presetIndex) )
		return parameters[parameterIndex].derive_f(presets[presetIndex].values[parameterIndex]);
	else
		return 0.0f;
}

//-----------------------------------------------------------------------------
void DfxPlugin::setpresetparameter(long presetIndex, long parameterIndex, DfxParamValue newValue)
{
	if ( parameterisvalid(parameterIndex) && presetisvalid(presetIndex) )
		presets[presetIndex].values[parameterIndex] = newValue;
}

//-----------------------------------------------------------------------------
void DfxPlugin::setpresetparameter_f(long presetIndex, long parameterIndex, float newValue)
{
	if ( parameterisvalid(parameterIndex) && presetisvalid(presetIndex) )
		parameters[parameterIndex].accept_f(newValue, presets[presetIndex].values[parameterIndex]);
}

//-----------------------------------------------------------------------------
void DfxPlugin::setpresetparameter_d(long presetIndex, long parameterIndex, double newValue)
{
	if ( parameterisvalid(parameterIndex) && presetisvalid(presetIndex) )
		parameters[parameterIndex].accept_d(newValue, presets[presetIndex].values[parameterIndex]);
}

//-----------------------------------------------------------------------------
void DfxPlugin::setpresetparameter_i(long presetIndex, long parameterIndex, long newValue)
{
	if ( parameterisvalid(parameterIndex) && presetisvalid(presetIndex) )
		parameters[parameterIndex].accept_i(newValue, presets[presetIndex].values[parameterIndex]);
}

//-----------------------------------------------------------------------------
void DfxPlugin::setpresetparameter_b(long presetIndex, long parameterIndex, bool newValue)
{
	if ( parameterisvalid(parameterIndex) && presetisvalid(presetIndex) )
		parameters[parameterIndex].accept_b(newValue, presets[presetIndex].values[parameterIndex]);
}

//-----------------------------------------------------------------------------
void DfxPlugin::setpresetparameter_gen(long presetIndex, long parameterIndex, float genValue)
{
	if ( parameterisvalid(parameterIndex) && presetisvalid(presetIndex) )
		parameters[parameterIndex].accept_d(expandparametervalue_index(parameterIndex, genValue), presets[presetIndex].values[parameterIndex]);
}

//-----------------------------------------------------------------------------
// set the text of a preset name
void DfxPlugin::setpresetname(long presetIndex, const char *inText)
{
	if (presetisvalid(presetIndex))
		presets[presetIndex].setname(inText);
}

//-----------------------------------------------------------------------------
// get a copy of the text of a preset name
void DfxPlugin::getpresetname(long presetIndex, char *outText)
{
	if (presetisvalid(presetIndex))
		presets[presetIndex].getname(outText);
}

//-----------------------------------------------------------------------------
// get a pointer to the text of a preset name
char * DfxPlugin::getpresetname_ptr(long presetIndex)
{
	if (presetisvalid(presetIndex))
		return presets[presetIndex].getname_ptr();
	else
		return NULL;
}

#ifdef TARGET_API_AUDIOUNIT
//-----------------------------------------------------------------------------
// get the CFString version of a preset name
CFStringRef DfxPlugin::getpresetcfname(long presetIndex)
{
	if (presetisvalid(presetIndex))
		return presets[presetIndex].getcfname();
	else
		return NULL;
}
#endif



#pragma mark _________state_________

//-----------------------------------------------------------------------------
// change the current audio sampling rate
void DfxPlugin::setsamplerate(double newrate)
{
	// avoid bogus values
	if (newrate <= 0.0)
		newrate = 44100.0;

	if (newrate != DfxPlugin::samplerate)
		sampleratechanged = true;

	// accept the new value into our sampling rate keeper
	DfxPlugin::samplerate = newrate;
}

//-----------------------------------------------------------------------------
// called when the sampling rate should be re-fetched from the host
void DfxPlugin::updatesamplerate()
{
#ifdef TARGET_API_AUDIOUNIT
	if (IsInitialized())	// will crash if not initialized
		setsamplerate(GetSampleRate());
	else
		setsamplerate(44100.0);
#endif
#ifdef TARGET_API_VST
	setsamplerate((double)getSampleRate());
#endif
}

//-----------------------------------------------------------------------------
// called when the number of audio channels has changed
void DfxPlugin::updatenumchannels()
{
	#ifdef TARGET_API_AUDIOUNIT
		// the number of inputs or outputs may have changed
		numInputs = getnuminputs();
		if (inputsP != NULL)
			free(inputsP);
		inputsP = (float**) malloc(numInputs * sizeof(float*));

		numOutputs = getnumoutputs();
		if (outputsP != NULL)
			free(outputsP);
		outputsP = (float**) malloc(numOutputs * sizeof(float*));
	#endif
}



#pragma mark _________properties_________

//-----------------------------------------------------------------------------
void DfxPlugin::getpluginname(char *outText)
{
	if (outText != NULL)
		strcpy(outText, PLUGIN_NAME_STRING);
}

//-----------------------------------------------------------------------------
// return the number of audio inputs
unsigned long DfxPlugin::getnuminputs()
{
#ifdef TARGET_API_AUDIOUNIT
	return GetInput(0)->GetStreamFormat().mChannelsPerFrame;
#endif
#ifdef TARGET_API_VST
	return numInputs;
#endif
}

//-----------------------------------------------------------------------------
// return the number of audio outputs
unsigned long DfxPlugin::getnumoutputs()
{
#ifdef TARGET_API_AUDIOUNIT
	return GetOutput(0)->GetStreamFormat().mChannelsPerFrame;
#endif
#ifdef TARGET_API_VST
	return numOutputs;
#endif
}

//-----------------------------------------------------------------------------
// add an audio input/output configuration to the array of i/o configurations
void DfxPlugin::addchannelconfig(short numin, short numout)
{
	if (channelconfigs != NULL)
	{
		DfxChannelConfig *swapconfigs = (DfxChannelConfig*) malloc(sizeof(DfxChannelConfig) * numchannelconfigs);
		memcpy(swapconfigs, channelconfigs, sizeof(DfxChannelConfig) * numchannelconfigs);
		free(channelconfigs);
		channelconfigs = (DfxChannelConfig*) malloc(sizeof(DfxChannelConfig) * (numchannelconfigs+1));
		memcpy(channelconfigs, swapconfigs, sizeof(DfxChannelConfig) * numchannelconfigs);
		free(swapconfigs);
		channelconfigs[numchannelconfigs].inChannels = numin;
		channelconfigs[numchannelconfigs].outChannels = numout;
		numchannelconfigs++;
	}
	else
	{
		channelconfigs = (DfxChannelConfig*) malloc(sizeof(DfxChannelConfig));
		channelconfigs[0].inChannels = numin;
		channelconfigs[0].outChannels = numout;
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



#pragma mark _________processing_________

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


#ifdef TARGET_API_AUDIOUNIT
	if (mHostCallbackInfo.beatAndTempoProc != NULL)
	{
		Float64 tempo, beat;
		if ( mHostCallbackInfo.beatAndTempoProc(mHostCallbackInfo.hostUserData, &beat, &tempo) == noErr )
		{
			timeinfo.tempoIsValid = true;
			timeinfo.tempo = tempo;
			timeinfo.beatPosIsValid = true;
			timeinfo.beatPos = beat;

			hostCanDoTempo = true;
		}
	}

	if (mHostCallbackInfo.musicalTimeLocationProc != NULL)
	{
		// the number of samples until the next beat from the start sample of the current rendering buffer
		UInt32 sampleOffsetToNextBeat;
		// the number of beats of the denominator value that contained in the current measure
		Float32 timeSigNumerator;
		// music notational conventions (4 is a quarter note, 8 an eigth note, etc)
		UInt32 timeSigDenominator;
		// the beat that corresponds to the downbeat (first beat) of the current measure
		Float64 currentMeasureDownBeat;
		if ( mHostCallbackInfo.musicalTimeLocationProc(mHostCallbackInfo.hostUserData, 
				&sampleOffsetToNextBeat, &timeSigNumerator, &timeSigDenominator, &currentMeasureDownBeat) == noErr )
		{
			// get the song beat position of the beginning of the previous measure
			timeinfo.barPosIsValid = true;
			timeinfo.barPos = currentMeasureDownBeat;

			// get the numerator of the time signature - this is the number of beats per measure
			timeinfo.timeSigIsValid = true;
			timeinfo.numerator = (double) timeSigNumerator;
			timeinfo.denominator = (double) timeSigDenominator;
		}

		// determine whether the playback position or state has just changed
// XXX implement this
//		if ()
//			timeinfo.playbackChanged = true;
	}
#endif
// TARGET_API_AUDIOUNIT
 

#ifdef TARGET_API_VST
	VstTimeInfo *vstTimeInfo = getTimeInfo(kVstTempoValid 
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

		// get the song beat position of the beginning of the previous measure
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
	// XXX turn off all parameterchanged flags?
	for (long i=0; i < numParameters; i++)
		setparameterchanged(i, false);

	#if TARGET_PLUGIN_USES_MIDI
		midistuff->postprocessEvents();
	#endif
}

//-----------------------------------------------------------------------------
// non-virtual function called to insure that processparameters happens
void DfxPlugin::do_processparameters()
{
	processparameters();
}



#pragma mark _________MIDI_________

#if TARGET_PLUGIN_USES_MIDI

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_noteon(int channel, int note, int velocity, long frameOffset)
{
	if (midistuff != NULL)
		midistuff->handleNoteOn(channel, note, velocity, frameOffset);
	if (dfxsettings != NULL)
		dfxsettings->handleNoteOn(channel, note, velocity, frameOffset);
}

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_noteoff(int channel, int note, int velocity, long frameOffset)
{
	if (midistuff != NULL)
		midistuff->handleNoteOff(channel, note, velocity, frameOffset);
	if (dfxsettings != NULL)
		dfxsettings->handleNoteOff(channel, note, velocity, frameOffset);
}

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_allnotesoff(int channel, long frameOffset)
{
	if (midistuff != NULL)
		midistuff->handleAllNotesOff(channel, frameOffset);
}

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_pitchbend(int channel, int valueLSB, int valueMSB, long frameOffset)
{
	if (midistuff != NULL)
		midistuff->handlePitchBend(channel, valueLSB, valueMSB, frameOffset);
	if (dfxsettings != NULL)
		dfxsettings->handlePitchBend(channel, valueLSB, valueMSB, frameOffset);
}

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_cc(int channel, int controllerNum, int value, long frameOffset)
{
	if (midistuff != NULL)
		midistuff->handleCC(channel, controllerNum, value, frameOffset);
	if (dfxsettings != NULL)
		dfxsettings->handleCC(channel, controllerNum, value, frameOffset);
}

//-----------------------------------------------------------------------------
void DfxPlugin::handlemidi_programchange(int channel, int programNum, long frameOffset)
{
	if (midistuff != NULL)
		midistuff->handleProgramChange(channel, programNum, frameOffset);
}

#endif
// TARGET_PLUGIN_USES_MIDI






#pragma mark _________---helper-functions---_________

//-----------------------------------------------------------------------------
// handy helper function for creating an array of float values
// returns true if allocation was successful, false if allocation failed
bool createbuffer_f(float **buffer, long currentBufferSize, long desiredBufferSize)
{
	// if the size of the buffer has changed, 
	// then delete & reallocate the buffes according to the new size
	if (desiredBufferSize != currentBufferSize)
		releasebuffer_f(buffer);

	if (*buffer == NULL)
		*buffer = (float*) malloc(desiredBufferSize * sizeof(float));

	// check if allocation was successful
	if (*buffer == NULL)
		return false;
	// we were successful if we reached this point
	return true;
}

//-----------------------------------------------------------------------------
// handy helper function for creating an array of double float values
// returns true if allocation was successful, false if allocation failed
bool createbuffer_d(double **buffer, long currentBufferSize, long desiredBufferSize)
{
	// if the size of the buffer has changed, 
	// then delete & reallocate the buffes according to the new size
	if (desiredBufferSize != currentBufferSize)
		releasebuffer_d(buffer);

	if (*buffer == NULL)
		*buffer = (double*) malloc(desiredBufferSize * sizeof(double));

	// check if allocation was successful
	if (*buffer == NULL)
		return false;
	// we were successful if we reached this point
	return true;
}

//-----------------------------------------------------------------------------
// handy helper function for creating an array of long int values
// returns true if allocation was successful, false if allocation failed
bool createbuffer_i(long **buffer, long currentBufferSize, long desiredBufferSize)
{
	// if the size of the buffer has changed, 
	// then delete & reallocate the buffes according to the new size
	if (desiredBufferSize != currentBufferSize)
		releasebuffer_i(buffer);

	if (*buffer == NULL)
		*buffer = (long*) malloc(desiredBufferSize * sizeof(long));

	// check if allocation was successful
	if (*buffer == NULL)
		return false;
	// we were successful if we reached this point
	return true;
}

//-----------------------------------------------------------------------------
// handy helper function for creating an array of boolean values
// returns true if allocation was successful, false if allocation failed
bool createbuffer_b(bool **buffer, long currentBufferSize, long desiredBufferSize)
{
	// if the size of the buffer has changed, 
	// then delete & reallocate the buffes according to the new size
	if (desiredBufferSize != currentBufferSize)
		releasebuffer_b(buffer);

	if (*buffer == NULL)
		*buffer = (bool*) malloc(desiredBufferSize * sizeof(bool));

	// check if allocation was successful
	if (*buffer == NULL)
		return false;
	// we were successful if we reached this point
	return true;
}

//-----------------------------------------------------------------------------
// handy helper function for creating an array of arrays of float values
// returns true if allocation was successful, false if allocation failed
bool createbufferarray_f(float ***buffers, unsigned long currentNumBuffers, long currentBufferSize, 
						unsigned long desiredNumBuffers, long desiredBufferSize)
{
	// if the size of each buffer or the number of buffers have changed, 
	// then delete & reallocate the buffers according to the new sizes
	if ( (desiredBufferSize != currentBufferSize) || (desiredNumBuffers != currentNumBuffers) )
		releasebufferarray_f(buffers, currentNumBuffers);

	if (desiredNumBuffers <= 0)
		return true;	// XXX true?

	if (*buffers == NULL)
	{
		*buffers = (float**) malloc(desiredNumBuffers * sizeof(float*));
		// out of memory or something
		if (*buffers == NULL)
			return false;
		for (unsigned long i=0; i < desiredNumBuffers; i++)
			(*buffers)[i] = NULL;
	}
	for (unsigned long i=0; i < desiredNumBuffers; i++)
	{
		if ((*buffers)[i] == NULL)
			(*buffers)[i] = (float*) malloc(desiredBufferSize * sizeof(float));
		// check if the allocation was successful
		if ((*buffers)[i] == NULL)
			return false;
	}

	// we were successful if we reached this point
	return true;
}

//-----------------------------------------------------------------------------
// handy helper function for creating an array of arrays of double float values
// returns true if allocation was successful, false if allocation failed
bool createbufferarrayarray_d(double ****buffers, unsigned long currentNumBufferArrays, unsigned long currentNumBuffers, 
							long currentBufferSize, unsigned long desiredNumBufferArrays, 
							unsigned long desiredNumBuffers, long desiredBufferSize)
{
	// if the size of each buffer or the number of buffers have changed, 
	// then delete & reallocate the buffers according to the new sizes
	if ( (desiredBufferSize != currentBufferSize) 
			|| (desiredNumBuffers != currentNumBuffers) 
			|| (desiredNumBufferArrays != currentNumBufferArrays) )
		releasebufferarrayarray_d(buffers, currentNumBufferArrays, currentNumBuffers);

	if (desiredNumBufferArrays <= 0)
		return true;	// XXX true?

	unsigned long i, j;
	if (*buffers == NULL)
	{
		*buffers = (double***) malloc(desiredNumBufferArrays * sizeof(double**));
		// out of memory or something
		if (*buffers == NULL)
			return false;
		for (i=0; i < desiredNumBufferArrays; i++)
			(*buffers)[i] = NULL;
	}
	for (i=0; i < desiredNumBufferArrays; i++)
	{
		if ((*buffers)[i] == NULL)
			(*buffers)[i] = (double**) malloc(desiredNumBuffers * sizeof(double*));
		// check if the allocation was successful
		if ((*buffers)[i] == NULL)
			return false;
		for (j=0; j < desiredNumBuffers; j++)
			(*buffers)[i][j] = NULL;
	}
	for (i=0; i < desiredNumBufferArrays; i++)
	{
		for (j=0; j < desiredNumBuffers; j++)
		{
			if ((*buffers)[i][j] == NULL)
				(*buffers)[i][j] = (double*) malloc(desiredBufferSize * sizeof(double));
			// check if the allocation was successful
			if ((*buffers)[i][j] == NULL)
				return false;
		}
	}

	// we were successful if we reached this point
	return true;
}


//-------------------------------------------------------------------------
// handy helper function for safely deallocating an array of float values
void releasebuffer_f(float **buffer)
{
	if (*buffer != NULL)
	{
		free(*buffer);
	}
	*buffer = NULL;
}

//-------------------------------------------------------------------------
// handy helper function for safely deallocating an array of double float values
void releasebuffer_d(double **buffer)
{
	if (*buffer != NULL)
	{
		free(*buffer);
	}
	*buffer = NULL;
}

//-------------------------------------------------------------------------
// handy helper function for safely deallocating an array of long int values
void releasebuffer_i(long **buffer)
{
	if (*buffer != NULL)
	{
		free(*buffer);
	}
	*buffer = NULL;
}

//-------------------------------------------------------------------------
// handy helper function for safely deallocating an array of boolean values
void releasebuffer_b(bool **buffer)
{
	if (*buffer != NULL)
	{
		free(*buffer);
	}
	*buffer = NULL;
}

//-------------------------------------------------------------------------
// handy helper function for safely deallocating an array of arrays of float values
void releasebufferarray_f(float ***buffers, unsigned long numbuffers)
{
	if (*buffers != NULL)
	{
		for (unsigned long i=0; i < numbuffers; i++)
		{
			if ((*buffers)[i] != NULL)
				free((*buffers)[i]);
			(*buffers)[i] = NULL;
		}
		free(*buffers);
	}
	*buffers = NULL;
}

//-------------------------------------------------------------------------
// handy helper function for safely deallocating an array of arrays of double float values
void releasebufferarrayarray_d(double ****buffers, unsigned long numbufferarrays, unsigned long numbuffers)
{
	if (*buffers != NULL)
	{
		for (unsigned long i=0; i < numbufferarrays; i++)
		{
			if ((*buffers)[i] != NULL)
			{
				for (unsigned long j=0; j < numbuffers; j++)
				{
					if ((*buffers)[i][j] != NULL)
						free((*buffers)[i][j]);
					(*buffers)[i][j] = NULL;
				}
				free((*buffers)[i]);
			}
			(*buffers)[i] = NULL;
		}
		free(*buffers);
	}
	*buffers = NULL;
}


//-----------------------------------------------------------------------------
// handy helper function for safely zeroing the contents of an array of float values
void clearbuffer_f(float *buffer, long buffersize, float value)
{
	if (buffer != NULL)
	{
		for (long i=0; i < buffersize; i++)
			buffer[i] = value;
	}
}

//-----------------------------------------------------------------------------
// handy helper function for safely zeroing the contents of an array of double float values
void clearbuffer_d(double *buffer, long buffersize, double value)
{
	if (buffer != NULL)
	{
		for (long i=0; i < buffersize; i++)
			buffer[i] = value;
	}
}

//-----------------------------------------------------------------------------
// handy helper function for safely zeroing the contents of an array of long int values
void clearbuffer_i(long *buffer, long buffersize, long value)
{
	if (buffer != NULL)
	{
		for (long i=0; i < buffersize; i++)
			buffer[i] = value;
	}
}

//-----------------------------------------------------------------------------
// handy helper function for safely zeroing the contents of an array of boolean values
void clearbuffer_b(bool *buffer, long buffersize, bool value)
{
	if (buffer != NULL)
	{
		for (long i=0; i < buffersize; i++)
			buffer[i] = value;
	}
}

//-----------------------------------------------------------------------------
// handy helper function for safely zeroing the contents of an array of arrays of float values
void clearbufferarray_f(float **buffers, unsigned long numbuffers, long buffersize, float value)
{
	if (buffers != NULL)
	{
		for (unsigned long i=0; i < numbuffers; i++)
		{
			if (buffers[i] != NULL)
			{
				for (long j=0; j < buffersize; j++)
					buffers[i][j] = value;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// handy helper function for safely zeroing the contents of an array of arrays of float values
void clearbufferarrayarray_d(double ***buffers, unsigned long numbufferarrays, unsigned long numbuffers, 
							long buffersize, double value)
{
	if (buffers != NULL)
	{
		for (unsigned long i=0; i < numbufferarrays; i++)
		{
			if (buffers[i] != NULL)
			{
				for (unsigned long j=0; j < numbuffers; j++)
				{
					if (buffers[i][j] != NULL)
					{
						for (long k=0; k < buffersize; k++)
							buffers[i][j][k] = value;
					}
				}
			}
		}
	}
}


#if WIN32
	// for ShellExecute
	#include <shellapi.h>
	#include <shlobj.h>
#endif

//-----------------------------------------------------------------------------
// handy function to open up an URL in the user's default web browser
//  * Mac OS
// returns noErr (0) if successful, otherwise a non-zero error code is returned
//  * Windows
// returns a meaningless value greater than 32 if successful, 
// otherwise an error code ranging from 0 to 32 is returned
long launch_url(const char *urlstring)
{
	if (urlstring == NULL)
		return 3;

#if MAC && defined(__MACH__)
	CFURLRef urlcfurl = CFURLCreateWithBytes(kCFAllocatorDefault, (const UInt8*)urlstring, strlen(urlstring), kCFStringEncodingASCII, NULL);
	if (urlcfurl != NULL)
	{
		OSStatus status = LSOpenCFURLRef(urlcfurl, NULL);	// try to launch the URL
		CFRelease(urlcfurl);
		return status;
	}
	return paramErr;	// couldn't create the CFURL, so return some error code
#endif

#if WIN32
	return (long) ShellExecute(NULL, "open", urlstring, NULL, NULL, SW_SHOWNORMAL);
#endif
}
