/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2010  Sophia Poirier

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

/*------------------------------------------------------------------------
the following is provisional documentation (basically snippets of emails 
that I wrote to Tom):

the dspcore stuff:
This has to do with AU, basically.  In AU, the easiest way to make an 
effect is to use what are called "kernels" in AU-world, which are
classes that handle 1-in/1-out audio streams and have all of the state
variables necessary for DSP within them.  They fetch the parameter values
from the main plugin class, they get created and destroyed as needed, they
get an audio stream to process, and they just do their thing.  The
advantage is that you get a clear encapsulation of the DSP stuff and you
can handle an arbitrary number of channels with no extra work (this is
possible in AU, but not VST).  The only time when you wouldn't do this is
if there are channel dependencies (the effect needs to see more than one
channel at a time) or when you need (or just support) mismatched in/out
configs.  So that's why I made the DfxPluginCore class, that's why that
stuff is in there, because it's nice.  It's mostly handled for you in AU,
but I kind of hacked it into our VST implementation, too, so it should
work fine there.  Transverb now uses this stuff.

There are tons and tons of methods in the DfxPlugin and DfxParameter 
classes, but you actually don't have to ever touch hardly any of them.  
They are mostly all used just within the class (you'll notice that there 
are barely any virtual methods).

Regarding having a "whole lot of crap" when starting a new plugin, 
my hopes with this DfxPlugin stuff is that that is not the case anymore.  
For one thing, there are very few methods that you need to
implement (I think only 9 at most, and, depending on what you're doing, it
could be as few as 4), and within those methods, there's very little
default stuff that you need (as much as possible is handled in the
inherited classes).  If I had written a true stub, it probably would have
been about 15 lines of code.  But if you think it's still too much and you
can thin of any ways to make it even more compact, that's cool, do tell.
I think it's pretty good in that respect now, though.  Especially I hope
you'll like the DSPcore thing, it might look weird, but it can make
writing an effect pretty easy, since arbitrary numbers of channels are
handled for you.

Let me give you a little summary of the Theory Of Operation(tm):

In the constructor and destructor, you take care of whatever is needed for
specifying any properties, anything that other stuff depends on, etc.  You
can save creating/destroying stuff that is only needed for audio
processing for the initialize and cleanup methods.  Although in many
cases, you won't even need to implement those, it just depends on the
plugin in question.  One exception is audio buffers.  There are
additionaly handy functions called createbuffers, clearbuffers, and
releasebuffers.  The base DfxPlugin class knows when to call these, so you
don't need to (not from initialize or cleanup or anything).  Note that
createbuffers is also recreate buffers.  In other words, it is called when
the sampling rate changes or when the number of channels to process
changes.  So your plugin's implementation of createbuffers should be aware
of what you currently have allocated and then check for sr or numchannels
changes, if the buffers have dependencies there, and then call
releasebuffers and then reallocate if necessary.  dfxplugin-stub has an
example of this.

When using DSPcore approach, the constructor and destructor of you DSP
class are essentially what initialize and cleanup are for a plugin that
doesn't use DSPcores.  Because of this, you do need to explicitly call
do_cleanup in the DSP class' destructor.

Immediately before processaudio (or just process for DSPcore) is called,
processparameters is called.  You should override it and, in there, fetch
the current parameter values into more usable variables and react to any
parameter changes that need special reacting to.  The reason for taking
this approach is to kind of at least minimize thread safety issues, and
also to avoid having to insert reaction code directly into the
setparameter call, and therefore reduce inefficient redundant reactions
(more than 1 per processing buffer).  And I had other reasons, too, that I
can't remember...

Also immediately before audio processing, MIDI events are collected and
sorted for you and useful tempo/time/location info is stuffed into a
DfxTimeInfo struct timeinfo.

So the core routines are: constructor, destructor, processparameters, and
processaudio (or process for DSP core).  You might need initialize and
cleanup, and maybe the buffer routines if you use buffers, but that's it.
------------------------------------------------------------------------*/

/*------------------------------------------------------------------------
 when impelementing plugins derived from this stuff, you must define the following:
PLUGIN_NAME_STRING
	a C string of the name of the plugin
PLUGIN_ID
	4-byte ID for the plugin
PLUGIN_VERSION
	4-byte version of the plugin
TARGET_PLUGIN_USES_MIDI
TARGET_PLUGIN_IS_INSTRUMENT
TARGET_PLUGIN_USES_DSPCORE
TARGET_PLUGIN_HAS_GUI
	1 or 0

 you must define one of these, and no more than one:
TARGET_API_AUDIOUNIT
TARGET_API_VST

 necessary for Audio Unit:
PLUGIN_ENTRY_POINT
	a C string of the base plugin class name with "Entry" appended

 necessary for Audio Units using a custom GUI (TARGET_PLUGIN_HAS_GUI = 1):
PLUGIN_EDITOR_ENTRY_POINT
	a C string of the plugin editor class name with "Entry" appended

 necessary for VST:
VST_NUM_INPUTS
VST_NUM_OUTPUTS
or if they match, simply define
VST_NUM_CHANNELS
	integers representing how many inputs and outputs your plugin has

 optional for Audio Unit:
PLUGIN_DESCRIPTION_STRING
	a C string description of the plugin
PLUGIN_RES_ID
	component resource ID of the base plugin
PLUGIN_EDITOR_DESCRIPTION_STRING
	a C string description of the plugin editor
PLUGIN_EDITOR_ID
	4-byte ID for the plugin (will PLUGIN_ID if not defined)
PLUGIN_EDITOR_RES_ID
	component resource ID of the base plugin
------------------------------------------------------------------------*/

#ifndef __DFXPLUGIN_H
#define __DFXPLUGIN_H


#ifdef TARGET_API_RTAS
	#include "CEffectGroup.h"
	#ifdef TARGET_API_AUDIOSUITE
		#include "CEffectProcessAS.h"
		typedef CEffectProcessAS	TARGET_API_BASE_CLASS;
	#else
		#include "CEffectProcessRTAS.h"
		typedef CEffectProcessRTAS	TARGET_API_BASE_CLASS;
	#endif
	typedef void *	TARGET_API_BASE_INSTANCE_TYPE;
	#ifdef TARGET_PLUGIN_USES_VSTGUI
		#include "CTemplateNoUIView.h"
		#include "CProcessType.h"
	#endif
	#if WINDOWS_VERSION
		#include "Mac2Win.h"
	#endif
#endif



// include our crucial shits

#include "dfxdefines.h"
#include "dfxplugin-base.h"

#include "dfxmisc.h"
#include "dfxmath.h"
#include "dfxparameter.h"
#include "dfxsettings.h"
#include "dfxpluginproperties.h"

#include "temporatetable.h"

#if TARGET_PLUGIN_USES_MIDI
	#include "dfxmidi.h"
#endif




//-----------------------------------------------------------------------------
// types

typedef struct {
	double tempo, tempo_bps;
	long samplesPerBeat;
		bool tempoIsValid;
	double beatPos;
		bool beatPosIsValid;
	double barPos;
		bool barPosIsValid;
	double denominator, numerator;
		bool timeSigIsValid;
	long samplesToNextBar;
		bool samplesToNextBarIsValid;
	bool playbackChanged;	// whether or not the playback state or position just changed
	bool playbackIsOccurring;
} DfxTimeInfo;


#ifdef TARGET_API_AUDIOUNIT
	// the Audio Unit API already has an i/o configurations structure
	typedef AUChannelInfo DfxChannelConfig;
#else
	// immitate AUChannelInfo from the Audio Unit API for other APIs
	typedef struct {
		short inChannels;
		short outChannels;
	} DfxChannelConfig;
#endif



#pragma mark _________DfxPlugin_________
//-----------------------------------------------------------------------------
class DfxPlugin : public TARGET_API_BASE_CLASS
#if defined(TARGET_API_RTAS) && defined(TARGET_PLUGIN_USES_VSTGUI)
, public ITemplateProcess
#endif
{
public:
	// ***
	DfxPlugin(TARGET_API_BASE_INSTANCE_TYPE inInstance, long inNumParameters, long inNumPresets = 1);
	// ***
	virtual ~DfxPlugin();

	void dfx_PostConstructor();
	void dfx_PreDestructor();

	long do_initialize();
	// ***
	virtual long initialize()
		{	return kDfxErr_NoError;	}
	void do_cleanup();
	// ***
	virtual void cleanup()
		{ }
	void do_reset();
	// ***
	virtual void reset()
		{ }

	// ***
	// override this if the plugin uses audio buffers
	// this will be called at the correct times
	// it may be called repeatedly when the audio stream format changes 
	// (sampling rate or number of channels), so it is expected that 
	// the implementation will check to see whether or not the 
	// buffers are already allocated and whether or not they need to 
	// be destroyed and reallocated in a different size
	virtual bool createbuffers()
		{	return true;	}
	// ***
	// override this if the plugin uses audio buffers and 
	// ever requires the buffer contents to be zeroed 
	// (like when the DSP state is reset)
	// this will be called at the correct times
	virtual void clearbuffers()
		{ }
	// ***
	// override this if the plugin uses audio buffers
	// this will be called at the correct times
	// release any allocated audio buffers
	virtual void releasebuffers()
		{ }

	// insures that processparameters (and perhaps other related stuff) 
	// is called at the correct moments
	void do_processparameters();
	// ***
	// override this to handle/accept parameter values immediately before 
	// processing audio and to react to parameter changes
	virtual void processparameters()
		{ }
	// attend to things immediately before processing a block of audio
	void preprocessaudio();
	// attend to things immediately after processing a block of audio
	void postprocessaudio();
	// ***
	// do the audio processing (override with real stuff)
	// pass in arrays of float buffers for input and output ([channel][sample]), 
	// 
	virtual void processaudio(const float ** inStreams, float ** outStreams, unsigned long inNumFrames, 
						bool replacing=true)
		{ }

	bool parameterisvalid(long inParameterIndex)
		{	return ( (inParameterIndex >= 0) && (inParameterIndex < numParameters) && (parameters != NULL) );	}

	void initparameter_f(long inParameterIndex, const char * initName, double initValue, double initDefaultValue, 
						double initMin, double initMax, 
						DfxParamUnit initUnit = kDfxParamUnit_generic, 
						DfxParamCurve initCurve = kDfxParamCurve_linear, 
						const char * initCustomUnitString = NULL);
	void initparameter_i(long inParameterIndex, const char * initName, int64_t initValue, int64_t initDefaultValue, 
						int64_t initMin, int64_t initMax, 
						DfxParamUnit initUnit = kDfxParamUnit_generic, 
						DfxParamCurve initCurve = kDfxParamCurve_stepped, 
						const char * initCustomUnitString = NULL);
	void initparameter_b(long inParameterIndex, const char * initName, bool initValue, bool initDefaultValue, 
						DfxParamUnit initUnit = kDfxParamUnit_generic);
	void initparameter_list(long inParameterIndex, const char * initName, int64_t initValue, int64_t initDefaultValue, 
						int64_t initNumItems, DfxParamUnit initUnit = kDfxParamUnit_list, 
						const char * initCustomUnitString = NULL);

	void setparameterusevaluestrings(long inParameterIndex, bool newMode=true)
		{	if (parameterisvalid(inParameterIndex)) parameters[inParameterIndex].setusevaluestrings(newMode);	}
	bool getparameterusevaluestrings(long inParameterIndex)
		{	if (parameterisvalid(inParameterIndex)) return parameters[inParameterIndex].getusevaluestrings();	else return false;	}
	bool setparametervaluestring(long inParameterIndex, int64_t inStringIndex, const char * inText);
	bool getparametervaluestring(long inParameterIndex, int64_t inStringIndex, char * outText);
	void getparameterunitstring(long inParameterIndex, char * outText)
		{	if (parameterisvalid(inParameterIndex)) parameters[inParameterIndex].getunitstring(outText);	}
	void setparametercustomunitstring(long inParameterIndex, const char * inText)
		{	if (parameterisvalid(inParameterIndex)) parameters[inParameterIndex].setcustomunitstring(inText);	}
	char * getparametervaluestring_ptr(long inParameterIndex, int64_t inStringIndex);
#ifdef TARGET_API_AUDIOUNIT
	CFStringRef * getparametervaluecfstrings(long inParameterIndex)
		{	if (parameterisvalid(inParameterIndex)) return parameters[inParameterIndex].getvaluecfstrings();   else return NULL;	}
	virtual CFStringRef CopyParameterGroupName(UInt32 inParameterGroupID)
		{	return NULL;	}
#endif

	void setparameter(long inParameterIndex, DfxParamValue newValue);
	void setparameter_f(long inParameterIndex, double newValue);
	void setparameter_i(long inParameterIndex, int64_t newValue);
	void setparameter_b(long inParameterIndex, bool newValue);
	void setparameter_gen(long inParameterIndex, double newValue);
	// ***
	virtual void randomizeparameter(long inParameterIndex);
	// ***
	virtual void randomizeparameters(bool writeAutomation = false);	// randomize all parameters at once
	void update_parameter(long inParameterIndex);
	void postupdate_parameter(long inParameterIndex);

	DfxParamValue getparameter(long inParameterIndex)
		{	if (parameterisvalid(inParameterIndex)) return parameters[inParameterIndex].get();   else return DfxParamValue(); }
	double getparameter_f(long inParameterIndex)
		{	if (parameterisvalid(inParameterIndex)) return parameters[inParameterIndex].get_f();   else return 0.0;	}
	int64_t getparameter_i(long inParameterIndex)
		{	if (parameterisvalid(inParameterIndex)) return parameters[inParameterIndex].get_i();   else return 0;	}
	bool getparameter_b(long inParameterIndex)
		{	if (parameterisvalid(inParameterIndex)) return parameters[inParameterIndex].get_b();   else return false;	}
	double getparameter_gen(long inParameterIndex)
		{	if (parameterisvalid(inParameterIndex)) return parameters[inParameterIndex].get_gen();   else return 0.0;	}
	// return a (hopefully) 0 to 1 scalar version of the parameter's current value
	double getparameter_scalar(long inParameterIndex);

	double getparametermin_f(long inParameterIndex)
		{	if (parameterisvalid(inParameterIndex)) return parameters[inParameterIndex].getmin_f();   else return 0.0;	}
	int64_t getparametermin_i(long inParameterIndex)
		{	if (parameterisvalid(inParameterIndex)) return parameters[inParameterIndex].getmin_i();   else return 0;	}
	double getparametermax_f(long inParameterIndex)
		{	if (parameterisvalid(inParameterIndex)) return parameters[inParameterIndex].getmax_f();   else return 0.0;	}
	int64_t getparametermax_i(long inParameterIndex)
		{	if (parameterisvalid(inParameterIndex)) return parameters[inParameterIndex].getmax_i();   else return 0;	}
	double getparameterdefault_f(long inParameterIndex)
		{	if (parameterisvalid(inParameterIndex)) return parameters[inParameterIndex].getdefault_f();   else return 0.0;	}
	int64_t getparameterdefault_i(long inParameterIndex)
		{	if (parameterisvalid(inParameterIndex)) return parameters[inParameterIndex].getdefault_i();   else return 0;	}
	bool getparameterdefault_b(long inParameterIndex)
		{	if (parameterisvalid(inParameterIndex)) return parameters[inParameterIndex].getdefault_b();   else return false;	}

	void getparametername(long inParameterIndex, char * text);
#ifdef TARGET_API_AUDIOUNIT
	CFStringRef getparametercfname(long inParameterIndex)
		{	if (parameterisvalid(inParameterIndex)) return parameters[inParameterIndex].getcfname();   else return NULL;	}
#endif
	DfxParamValueType getparametervaluetype(long inParameterIndex);
	DfxParamUnit getparameterunit(long inParameterIndex);
	bool getparameterchanged(long inParameterIndex);
	void setparameterchanged(long inParameterIndex, bool inChanged = true);
	bool getparametertouched(long inParameterIndex);
	void setparametertouched(long inParameterIndex, bool inTouched = true);
	DfxParamCurve getparametercurve(long inParameterIndex)
		{	if (parameterisvalid(inParameterIndex)) return parameters[inParameterIndex].getcurve();   else return kDfxParamCurve_linear;	}
	void setparametercurve(long inParameterIndex, DfxParamCurve newcurve)
		{	if (parameterisvalid(inParameterIndex)) parameters[inParameterIndex].setcurve(newcurve);	}
	double getparametercurvespec(long inParameterIndex)
		{	if (parameterisvalid(inParameterIndex)) return parameters[inParameterIndex].getcurvespec();   else return 0.0;	}
	void setparametercurvespec(long inParameterIndex, double newcurvespec)
		{	if (parameterisvalid(inParameterIndex)) parameters[inParameterIndex].setcurvespec(newcurvespec);	}
	bool getparameterenforcevaluelimits(long inParameterIndex)
		{	if (parameterisvalid(inParameterIndex)) return parameters[inParameterIndex].GetEnforceValueLimits();   else return false;	}
	void setparameterenforcevaluelimits(long inParameterIndex, bool inNewMode)
		{	if (parameterisvalid(inParameterIndex)) parameters[inParameterIndex].SetEnforceValueLimits(inNewMode);	}
	unsigned long getparameterattributes(long inParameterIndex)
		{	if (parameterisvalid(inParameterIndex)) return parameters[inParameterIndex].getattributes();   else return 0;	}
	void setparameterattributes(long inParameterIndex, unsigned long inFlags)
		{	if (parameterisvalid(inParameterIndex)) parameters[inParameterIndex].setattributes(inFlags);	}
	void addparameterattributes(long inParameterIndex, unsigned long inFlags)
	{
		if (parameterisvalid(inParameterIndex))
			parameters[inParameterIndex].setattributes( inFlags | parameters[inParameterIndex].getattributes() );
	}

	// convenience methods for expanding and contracting parameter values 
	// using the min/max/curvetype/curvespec/etc. settings of a given parameter
	double expandparametervalue(long inParameterIndex, double genValue);
	double contractparametervalue(long inParameterIndex, double realValue);

	// whether or not the index is a valid preset
	bool presetisvalid(long inPresetIndex);
	// whether or not the index is a valid preset with a valid name
	bool presetnameisvalid(long inPresetIndex);
	// load the settings of a preset
	virtual bool loadpreset(long inPresetIndex);
	// set a parameter value in all of the empty (no name) presets 
	// to the current value of that parameter
	void initpresetsparameter(long inParameterIndex);
	// set the text of a preset name
	void setpresetname(long inPresetIndex, const char * inText);
	// get a copy of the text of a preset name
	void getpresetname(long inPresetIndex, char * outText);
	// get a pointer to the text of a preset name
	const char * getpresetname_ptr(long inPresetIndex);
#ifdef TARGET_API_AUDIOUNIT
	CFStringRef getpresetcfname(long inPresetIndex);
#endif
	long getcurrentpresetnum()
		{	return currentPresetNum;	}
	void setpresetparameter(long inPresetIndex, long inParameterIndex, DfxParamValue newValue);
	void setpresetparameter_f(long inPresetIndex, long inParameterIndex, double newValue);
	void setpresetparameter_i(long inPresetIndex, long inParameterIndex, int64_t newValue);
	void setpresetparameter_b(long inPresetIndex, long inParameterIndex, bool newValue);
	void setpresetparameter_gen(long inPresetIndex, long inParameterIndex, double genValue);
	void update_preset(long inPresetIndex);
	DfxParamValue getpresetparameter(long inPresetIndex, long inParameterIndex);
	double getpresetparameter_f(long inPresetIndex, long inParameterIndex);


	virtual long dfx_GetPropertyInfo(DfxPropertyID inPropertyID, DfxScope inScope, unsigned long inItemIndex, 
										size_t & outDataSize, DfxPropertyFlags & outFlags)
		{	return kDfxErr_InvalidProperty;	}
	virtual long dfx_GetProperty(DfxPropertyID inPropertyID, DfxScope inScope, unsigned long inItemIndex, 
									void * outData)
		{	return kDfxErr_InvalidProperty;	}
	virtual long dfx_SetProperty(DfxPropertyID inPropertyID, DfxScope inScope, unsigned long inItemIndex, 
									const void * inData, size_t inDataSize)
		{	return kDfxErr_InvalidProperty;	}
	long dfx_GetNumPluginProperties()
		{	return kDfxPluginProperty_NumProperties + dfx_GetNumAdditionalPluginProperties();	}
	virtual long dfx_GetNumAdditionalPluginProperties()
		{	return 0;	}


	// get the current audio sampling rate
	double getsamplerate()
		{	return DfxPlugin::samplerate;	}
	// convenience wrapper of getsamplerate to get float type value
	double getsamplerate_f()
		{	return (float) (DfxPlugin::samplerate);	}
	// change the current audio sampling rate
	void setsamplerate(double newrate);
	// force a refetching of the samplerate from the host
	void updatesamplerate();

	// react to a change in the number of audio channels
	void updatenumchannels();
	// return the number of audio inputs
	unsigned long getnuminputs();
	// return the number of audio outputs
	unsigned long getnumoutputs();

	long getnumparameters()
		{	return numParameters;	}
	long getnumpresets()
		{	return numPresets;	}

//	virtual void SetUseMusicalTimeInfo(bool newmode = true)
//		{	b_usemusicaltimeinfo = newmode;	}
//	bool GetUseMusicalTimeInfo()
//		{	return b_usemusicaltimeinfo;	}

	// get the DfxTimeInfo struct with the latest timeinfo values
	DfxTimeInfo gettimeinfo()
		{	return timeinfo;	}

	// add an audio input/output configuration to the array of i/o configurations
	void addchannelconfig(short inNumInputChannels, short inNumOutputChannels);

	void setlatency_samples(long newlatency);
	void setlatency_seconds(double newlatency);
	long getlatency_samples();
	double getlatency_seconds();
	void update_latency();

	void settailsize_samples(long newsize);
	void settailsize_seconds(double newsize);
	long gettailsize_samples();
	double gettailsize_seconds();
	void update_tailsize();

	void setAudioProcessingMustAccumulate(bool inNewMode);

//	virtual void SetUseTimeStampedParameters(bool newmode = true)
//		{	b_usetimestampedparameters = newmode;	}
//	bool GetUseTimeStampedParameters()
//		{	return b_usetimestampedparameters;	}

	void getpluginname(char * outText);
	long getpluginversion();

	#if TARGET_PLUGIN_USES_MIDI
		// ***
		// handlers for the types of MIDI events that we support
		virtual void handlemidi_noteon(int inChannel, int inNote, int inVelocity, long inFrameOffset);
		virtual void handlemidi_noteoff(int inChannel, int inNote, int inVelocity, long inFrameOffset);
		virtual void handlemidi_allnotesoff(int inChannel, long inFrameOffset);
		virtual void handlemidi_pitchbend(int inChannel, int inValueLSB, int inValueMSB, long inFrameOffset);
		virtual void handlemidi_cc(int inChannel, int inControllerNum, int inValue, long inFrameOffset);
		virtual void handlemidi_programchange(int inChannel, int inProgramNum, long inFrameOffset);

		DfxMidi * getmidistream()
			{	return midistuff;	}
		DfxSettings * getsettings_ptr()
			{	return dfxsettings;	}

		// ***
		/* - - - - - - - - - hooks for DfxSettings - - - - - - - - - */
		//
		// these allow for additional constructor or destructor stuff, if necessary
		virtual void settings_init() {}
		virtual void settings_cleanup() {}
		//
		// these can be overridden to store and restore more data during 
		// save() and restore() respectively
		// the data pointers point to the start of the extended data 
		// sections of the settings data
		virtual void settings_saveExtendedData(void * outData, bool isPreset) {}
		virtual void settings_restoreExtendedData(void * inData, unsigned long storedExtendedDataSize, 
											long dataVersion, bool isPreset) {}
		//
		// this can be overridden to react when parameter values are 
		// restored from settings that are loaded during restore()
		// for example, you might need to remap certain parameter values 
		// when loading settings stored by older versions of your plugin 
		// (that's why settings data version is one of the function arguments; 
		// it's the version number of the plugin that created the data)
		// (presetNum of -1 indicates that we're just working with the current 
		// state of the plugin)
		virtual void settings_doChunkRestoreSetParameterStuff(long tag, float value, long dataVersion, long presetNum = -1) {}
		//
		// these can be overridden to do something and extend the MIDI event processing
		virtual void settings_doLearningAssignStuff(long tag, long eventType, long eventChannel, 
											long eventNum, long delta, long eventNum2 = 0, 
											long eventBehaviourFlags = 0, 
											long data1 = 0, long data2 = 0, 
											float fdata1 = 0.0f, float fdata2 = 0.0f) {}
		virtual void settings_doMidiAutomatedSetParameterStuff(long tag, float value, long delta) {}
	#endif

	// handling of AU properties specific to Logic
	#ifdef TARGET_API_AUDIOUNIT
		UInt32 getSupportedLogicNodeOperationMode()
			{	return supportedLogicNodeOperationMode;	}
		void setSupportedLogicNodeOperationMode(UInt32 inNewMode)
			{	supportedLogicNodeOperationMode = inNewMode;	}
		UInt32 getCurrentLogicNodeOperationMode()
			{	return currentLogicNodeOperationMode;	}
		virtual void setCurrentLogicNodeOperationMode(UInt32 inNewMode)
			{	currentLogicNodeOperationMode = inNewMode;	}
		bool isLogicNodeEndianReversed()
		{
			return ( (getCurrentLogicNodeOperationMode() & kLogicAUNodeOperationMode_NodeEnabled) 
					&& (getCurrentLogicNodeOperationMode() & kLogicAUNodeOperationMode_EndianSwap) );
		}
	#endif


protected:
	DfxParam * parameters;
	DfxPreset * presets;

	DfxChannelConfig * channelconfigs;
	long numchannelconfigs;

	DfxTimeInfo timeinfo;
	bool b_usemusicaltimeinfo;	// XXX use this?
	bool hostCanDoTempo;
	TempoRateTable * tempoRateTable;	// a table of tempo rate values

	bool b_usetimestampedparameters;	// XXX use this?

	double samplerate;
	bool sampleratechanged;

	unsigned long numInputs, numOutputs;

	#if TARGET_PLUGIN_USES_MIDI
		DfxMidi * midistuff;
		DfxSettings * dfxsettings;
	#endif

	long numParameters;
	long numPresets;
	long currentPresetNum;

	#if TARGET_PLUGIN_USES_DSPCORE && !defined(TARGET_API_AUDIOUNIT)
		DfxPluginCore ** dspcores;	// we have to handle this ourselves because VST can't
	#endif

	#ifdef TARGET_API_AUDIOUNIT
		bool auElementsHaveBeenCreated;
		// array of float pointers to input and output audio buffers, 
		// just for the sake of making processaudio(float**, float**, etc.) possible
		float ** inputAudioStreams_au;
		float ** outputAudioStreams_au;
	#endif

	#ifdef TARGET_API_VST
		bool latencychanged;
		bool isinitialized;
	#endif

private:
	// try to get musical tempo/time/location information from the host
	void processtimeinfo();

	long latency_samples;
	double latency_seconds;
	bool b_uselatency_seconds;
	long tailsize_samples;
	double tailsize_seconds;
	bool b_usetailsize_seconds;
	bool audioProcessingAccumulatingOnly;
	bool audioIsRendering;

	#ifdef TARGET_API_AUDIOUNIT
		void UpdateInPlaceProcessingState();
		UInt32 supportedLogicNodeOperationMode, currentLogicNodeOperationMode;
	#endif

	#ifdef TARGET_API_RTAS
		void AddParametersToList();
		bool mMasterBypass_rtas;
		float ** inputAudioStreams_as;
		float ** outputAudioStreams_as;
		float * zeroAudioBuffer;
		long numZeroAudioBufferSamples;
	#ifdef TARGET_PLUGIN_USES_VSTGUI
		GrafPtr mMainPort;
		ITemplateCustomUI * mCustomUI_p;
		Rect mPIWinRect;
		CTemplateNoUIView * mNoUIView_p;
		void * mModuleHandle_p;
	#endif
	#endif


// overridden virtual methods from inherited API base classes
public:

#ifdef TARGET_API_AUDIOUNIT
	virtual void PostConstructor();
	virtual void PreDestructor();
	virtual OSStatus Initialize();
	virtual void Cleanup();
	virtual OSStatus Reset(AudioUnitScope inScope, AudioUnitElement inElement);

	#if TARGET_PLUGIN_IS_INSTRUMENT
		virtual OSStatus Render(AudioUnitRenderActionFlags & ioActionFlags, 
						const AudioTimeStamp & inTimeStamp, UInt32 inFramesToProcess);
	#else
		virtual OSStatus ProcessBufferLists(AudioUnitRenderActionFlags & ioActionFlags, 
						const AudioBufferList & inBuffer, AudioBufferList & outBuffer, 
						UInt32 inFramesToProcess);
	#endif
	#if TARGET_PLUGIN_USES_DSPCORE
		virtual AUKernelBase * NewKernel();
	#endif

	virtual OSStatus GetPropertyInfo(AudioUnitPropertyID inPropertyID, 
				AudioUnitScope inScope, AudioUnitElement inElement, 
				UInt32 & outDataSize, Boolean & outWritable);
	virtual OSStatus GetProperty(AudioUnitPropertyID inPropertyID, 
				AudioUnitScope inScope, AudioUnitElement inElement, 
				void * outData);
	virtual OSStatus SetProperty(AudioUnitPropertyID inPropertyID, 
				AudioUnitScope inScope, AudioUnitElement inElement, 
				const void * inData, UInt32 inDataSize);

	virtual OSStatus Version();
	virtual UInt32 SupportedNumChannels(const AUChannelInfo ** outInfo);
	virtual Float64 GetLatency();
	virtual Float64 GetTailTime();
	virtual bool SupportsTail()
		{	return true;	}
	virtual CFURLRef CopyIconLocation();

	virtual OSStatus GetParameterInfo(AudioUnitScope inScope, 
				AudioUnitParameterID inParameterID, 
				AudioUnitParameterInfo & outParameterInfo);
	virtual OSStatus GetParameterValueStrings(AudioUnitScope inScope, 
				AudioUnitParameterID inParameterID, CFArrayRef * outStrings);
	virtual OSStatus SetParameter(AudioUnitParameterID inParameterID, 
				AudioUnitScope inScope, AudioUnitElement inElement, 
				Float32 inValue, UInt32 inBufferOffsetInFrames);

	virtual	OSStatus ChangeStreamFormat(AudioUnitScope inScope, 
				AudioUnitElement inElement, 
				const CAStreamBasicDescription & inPrevFormat, 
				const CAStreamBasicDescription & inNewFormat);

	virtual OSStatus SaveState(CFPropertyListRef * outData);
	virtual OSStatus RestoreState(CFPropertyListRef inData);
	virtual OSStatus GetPresets(CFArrayRef * outData) const;
	virtual OSStatus NewFactoryPresetSet(const AUPreset & inNewFactoryPreset);

	#if TARGET_PLUGIN_USES_MIDI
		virtual OSStatus HandleNoteOn(UInt8 inChannel, UInt8 inNoteNumber, 
							UInt8 inVelocity, UInt32 inStartFrame);
		virtual OSStatus HandleNoteOff(UInt8 inChannel, UInt8 inNoteNumber, 
							UInt8 inVelocity, UInt32 inStartFrame);
		virtual OSStatus HandleAllNotesOff(UInt8 inChannel);
		virtual OSStatus HandleControlChange(UInt8 inChannel, UInt8 inController, 
							UInt8 inValue, UInt32 inStartFrame);
		virtual OSStatus HandlePitchWheel(UInt8 inChannel, UInt8 inPitchLSB, UInt8 inPitchMSB, 
							UInt32 inStartFrame);
		virtual OSStatus HandleProgramChange(UInt8 inChannel, UInt8 inProgramNum);
		virtual OSStatus HandleChannelPressure(UInt8 inChannel, UInt8 inValue, UInt32 inStartFrame)
			{	return noErr;	}
		virtual OSStatus HandlePolyPressure(UInt8 inChannel, UInt8 inKey, 
							UInt8 inValue, UInt32 inStartFrame)
			{	return noErr;	}
		virtual OSStatus HandleResetAllControllers(UInt8 inChannel)
			{	return noErr;	}
		virtual OSStatus HandleAllSoundOff(UInt8 inChannel)
			{	return noErr;	}
	#endif
	#if TARGET_PLUGIN_IS_INSTRUMENT
		virtual OSStatus PrepareInstrument(MusicDeviceInstrumentID inInstrument);
		virtual OSStatus ReleaseInstrument(MusicDeviceInstrumentID inInstrument);
		virtual OSStatus StartNote(MusicDeviceInstrumentID inInstrument,
						MusicDeviceGroupID inGroupID, NoteInstanceID * outNoteInstanceID, 
						UInt32 inOffsetSampleFrame, const MusicDeviceNoteParams & inParams);
		virtual OSStatus StopNote(MusicDeviceGroupID inGroupID, 
						NoteInstanceID inNoteInstanceID, UInt32 inOffsetSampleFrame);

		// this is a convenience function swiped from AUEffectBase, but not included in MusicDeviceBase
		Float64 GetSampleRate()
		{
			return GetOutput(0)->GetStreamFormat().mSampleRate;
		}
		// this is handled by AUEffectBase, but not in MusicDeviceBase
		virtual bool StreamFormatWritable(AudioUnitScope inScope, AudioUnitElement inElement)
		{
			return IsInitialized() ? false : true;
		}
	#endif
	#if TARGET_PLUGIN_HAS_GUI
		virtual int GetNumCustomUIComponents();
		virtual void GetUIComponentDescs(ComponentDescription * inDescArray);
	#endif
#endif
// end of Audio Unit API methods


#ifdef TARGET_API_VST
	virtual void close();

	virtual void processReplacing(float ** inputs, float ** outputs, VstInt32 sampleFrames);
	#if !VST_FORCE_DEPRECATED
	virtual void process(float ** inputs, float ** outputs, VstInt32 sampleFrames);
	#endif

	virtual void suspend();
	virtual void resume();
	#if !VST_FORCE_DEPRECATED
	virtual VstInt32 fxIdle();
	#endif
	virtual void setSampleRate(float newRate);

	virtual VstInt32 getTailSize();
	// XXX there is a typo in the VST SDK header files, so in case it gets corrected, 
	// make this call the properly-named version
	virtual VstInt32 getGetTailSize() { return getTailSize(); }
	virtual bool getInputProperties(VstInt32 index, VstPinProperties * properties);
	virtual bool getOutputProperties(VstInt32 index, VstPinProperties * properties);

	virtual void setProgram(VstInt32 inProgramNum);
	virtual void setProgramName(char * inName);
	virtual void getProgramName(char * outText);
	virtual bool getProgramNameIndexed(VstInt32 inCategory, VstInt32 inIndex, char * outText);

	virtual void setParameter(VstInt32 index, float value);
	virtual float getParameter(VstInt32 index);
	virtual void getParameterName(VstInt32 index, char * name);
	virtual void getParameterDisplay(VstInt32 index, char * text);
	virtual void getParameterLabel(VstInt32 index, char * label);

	virtual bool getEffectName(char * outText);
	virtual VstInt32 getVendorVersion();
	virtual bool getVendorString(char * outText);
	virtual bool getProductString(char * outText);

	virtual VstInt32 canDo(char * text);

	#if TARGET_PLUGIN_USES_MIDI
		virtual VstInt32 processEvents(VstEvents * events);
		virtual VstInt32 setChunk(void * data, VstInt32 byteSize, bool isPreset);
		virtual VstInt32 getChunk(void ** data, bool isPreset);
	#endif

	// DFX supplementary VST methods
	void setlatencychanged(bool newstatus = true)
		{	latencychanged = newstatus;	}
	bool getlatencychanged()
		{	return latencychanged;	}
	#if TARGET_PLUGIN_USES_DSPCORE
		DfxPluginCore * getplugincore(unsigned long channel)
		{
			if ( channel >= getnumoutputs() )
				return NULL;
			if (dspcores == NULL)
				return NULL;
			return dspcores[channel];
		}
	#endif
#endif
// end of VST API methods


#ifdef TARGET_API_RTAS
	virtual void Free();
	virtual ComponentResult ResetPlugInState();
	virtual ComponentResult Prime(Boolean inPriming);
	virtual void UpdateControlValueInAlgorithm(long inParameterIndex);
	virtual ComponentResult IsControlAutomatable(long inControlIndex, short * outItIs);
	virtual ComponentResult GetControlNameOfLength(long inParameterIndex, char * outName, long inNameLength, OSType inControllerType, FicBoolean * outReverseHighlight);
	virtual ComponentResult GetValueString(long inParameterIndex, long inValue, StringPtr outValueString, long inMaxLength);
	virtual ComponentResult SetChunk(OSType inChunkID, SFicPlugInChunk * chunk);
	virtual void DoTokenIdle();
	virtual CPlugInView * CreateCPlugInView();

	// AU->RTAS glue convenience functions
	double GetParameter_f_FromRTAS(long inParameterID);
	int64_t GetParameter_i_FromRTAS(long inParameterID);
	bool GetParameter_b_FromRTAS(long inParameterID);

#ifdef TARGET_PLUGIN_USES_VSTGUI
	virtual void SetViewPort(GrafPtr inPort);
	virtual void GetViewRect(Rect * outViewRect);
	virtual long SetControlValue(long inControlIndex, long inValue);
	virtual long GetControlValue(long inControlIndex, long * aValue);
	virtual long GetControlDefaultValue(long inControlIndex, long * outValue);
	ComponentResult UpdateControlGraphic(long inControlIndex, long inValue);
	ComponentResult SetControlHighliteInfo(long inControlIndex, short inIsHighlighted, short inColor);
	ComponentResult ChooseControl(Point inLocalCoord, long * outControlIndex);

	virtual void setEditor(void * inEditor)
		{	mCustomUI_p = (ITemplateCustomUI*)inEditor;	}
	virtual int ProcessTouchControl(long inControlIndex);
	virtual int ProcessReleaseControl(long inControlIndex);
	virtual void ProcessDoIdle();
	virtual void * ProcessGetModuleHandle()
		{	return mModuleHandle_p;	}
	virtual short ProcessUseResourceFile()
		{	return fProcessType->GetProcessGroup()->UseResourceFile();	}
	virtual void ProcessRestoreResourceFile(short resFile)
		{	fProcessType->GetProcessGroup()->RestoreResourceFile(resFile);	}

	// silliness needing for RTAS<->VSTGUI connecting
//	virtual float GetParameter_f(long inParameterID)
//		{	return GetParameter(inParameterID);	}
#endif
#endif
// end of RTAS/AudioSuite API methods


protected:
#ifdef TARGET_API_RTAS
	virtual void EffectInit();
#ifdef TARGET_API_AUDIOSUITE
	virtual UInt32 ProcessAudio(bool inIsMasterBypassed);
#endif
	virtual void RenderAudio(float ** inAudioStreams, float ** outAudioStreams, long inNumFramesToProcess);
#endif

};



#pragma mark _________DfxPluginCore_________

//-----------------------------------------------------------------------------
// Audio Unit must override NewKernel() to implement this
class DfxPluginCore
#ifdef TARGET_API_CORE_CLASS
: public TARGET_API_CORE_CLASS
#endif
{
public:
	DfxPluginCore(DfxPlugin * inDfxPlugin) :
		#ifdef TARGET_API_CORE_CLASS
		TARGET_API_CORE_CLASS(inDfxPlugin), 
		#endif
		dfxplugin(inDfxPlugin)
		{ }

	virtual ~DfxPluginCore()
		{	releasebuffers();	}	// XXX this doesn't work for child class' releasebuffers() implementation

	void dfxplugincore_postconstructor()
	{
		do_reset();
	}

	void do_process(const float * inStream, float * outStream, unsigned long inNumFrames, 
						bool replacing=true)
	{
		processparameters();
		process(inStream, outStream, inNumFrames, replacing);
	}
	virtual void process(const float * inStream, float * outStream, unsigned long inNumFrames, 
						bool replacing=true) = 0;
	void do_reset()
	{
		createbuffers();
		clearbuffers();
		reset();
	}
	virtual void reset()
		{ }
	virtual void processparameters()
		{ }

	// ***
	virtual bool createbuffers()
		{	return true;	}
	// ***
	virtual void clearbuffers()
		{ }
	// ***
	virtual void releasebuffers()
		{ }

	double getsamplerate()
		{	return dfxplugin->getsamplerate();	}
	float getsamplerate_f()
		{	return dfxplugin->getsamplerate_f();	}
//	DfxParamValue getparameter(long inParameterIndex)
//		{	return dfxplugin->getparameter(inParameterIndex);	}
	double getparameter_f(long inParameterIndex)
		{	return dfxplugin->getparameter_f(inParameterIndex);	}
	int64_t getparameter_i(long inParameterIndex)
		{	return dfxplugin->getparameter_i(inParameterIndex);	}
	bool getparameter_b(long inParameterIndex)
		{	return dfxplugin->getparameter_b(inParameterIndex);	}
	double getparameter_scalar(long inParameterIndex)
		{	return dfxplugin->getparameter_scalar(inParameterIndex);	}
	double getparameter_gen(long inParameterIndex)
		{	return dfxplugin->getparameter_gen(inParameterIndex);	}
	double getparametermin_f(long inParameterIndex)
		{	return dfxplugin->getparametermin_f(inParameterIndex);	}
	int64_t getparametermin_i(long inParameterIndex)
		{	return dfxplugin->getparametermin_i(inParameterIndex);	}
	double getparametermax_f(long inParameterIndex)
		{	return dfxplugin->getparametermax_f(inParameterIndex);	}
	int64_t getparametermax_i(long inParameterIndex)
		{	return dfxplugin->getparametermax_i(inParameterIndex);	}
	bool getparameterchanged(long inParameterIndex)
		{	return dfxplugin->getparameterchanged(inParameterIndex);	}
	bool getparametertouched(long inParameterIndex)
		{	return dfxplugin->getparametertouched(inParameterIndex);	}


protected:
	DfxPlugin * dfxplugin;


public:

#ifdef TARGET_API_AUDIOUNIT
	void Process(const Float32 * in, Float32 * out, UInt32 inNumFrames, UInt32 inNumChannels, bool & ioSilence)
	{
		do_process(in, out, inNumFrames);
		ioSilence = false;
	}
	virtual void Reset()
		{	do_reset();	}
#endif

};



#pragma mark _________DfxEffectGroup_________

#ifdef TARGET_API_RTAS
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class DfxEffectGroup : public CEffectGroup
{
public:
	DfxEffectGroup();

protected:
	virtual void CreateEffectTypes();
	virtual void Initialize();

private:
	void dfx_AddEffectType(CEffectType * inEffectType);
};
#endif



//-----------------------------------------------------------------------------
// prototypes for a few handy buffer helper functions

long DFX_CompositePluginVersionNumberValue();

#ifdef TARGET_API_RTAS
OSType DFX_IterateAlphaNumericFourCharCode(OSType inPreviousCode);
#endif






//-----------------------------------------------------------------------------
// plugin entry point macros and defines and stuff

#ifdef TARGET_API_AUDIOUNIT

	#define DFX_EFFECT_ENTRY(PluginClass)   COMPONENT_ENTRY(PluginClass)

	#if TARGET_PLUGIN_USES_DSPCORE
		#define DFX_CORE_ENTRY(PluginCoreClass)						\
			AUKernelBase * DfxPlugin::NewKernel()					\
			{														\
				DfxPluginCore * core = new PluginCoreClass(this);	\
				if (core != NULL)									\
					core->dfxplugincore_postconstructor();			\
				return core;										\
			}
//	#else
//		AUKernelBase * DfxPlugin::NewKernel()
//			{	return TARGET_API_BASE_CLASS::NewKernel();	}
		#define DFX_INIT_CORE(PluginCoreClass)
	#endif

#else

	// we need to manage the DSP cores manually in APIs other than AU
	// call this in the plugin's constructor if it uses DSP cores for processing
	#if TARGET_PLUGIN_USES_DSPCORE
		// DFX_CORE_ENTRY is not useful for APIs other than AU, so it is defined as nothing
		#define DFX_CORE_ENTRY(PluginCoreClass)
		#define DFX_INIT_CORE(PluginCoreClass)   										\
			for (unsigned long corecount=0; corecount < getnumoutputs(); corecount++)	\
			{																			\
				dspcores[corecount] = new PluginCoreClass(this);						\
				if (dspcores[corecount] != NULL)										\
					dspcores[corecount]->dfxplugincore_postconstructor();				\
			}
	#endif

#endif



#ifdef TARGET_API_VST

	/// this is mostly handled in vstplugmain.cpp in the VST 2.4 SDK and higher
	#if VST_2_4_EXTENSIONS

		#define DFX_EFFECT_ENTRY(PluginClass)										\
			AudioEffect * createEffectInstance(audioMasterCallback inAudioMaster)	\
			{																		\
				DfxPlugin * effect = new PluginClass(inAudioMaster);				\
				if (effect == NULL)													\
					return NULL;													\
				effect->dfx_PostConstructor();										\
				return effect;														\
			}

	#else

		#if BEOS
			#define main main_plugin
			extern "C" __declspec(dllexport) AEffect * main_plugin(audioMasterCallback audioMaster);
		#elif (MACX && __ppc__)
			#define main main_macho
			extern "C" AEffect * main_macho(audioMasterCallback audioMaster);
		#else
			AEffect * main(audioMasterCallback audioMaster);
		#endif

		#define DFX_EFFECT_ENTRY(PluginClass)								\
			AEffect * main(audioMasterCallback inAudioMaster)				\
			{																\
				if ( !inAudioMaster(0, audioMasterVersion, 0, 0, 0, 0) )	\
					return NULL;											\
				DfxPlugin * effect = new PluginClass(inAudioMaster);		\
				if (effect == NULL)											\
					return NULL;											\
				effect->dfx_PostConstructor();								\
				return effect->getAeffect();								\
			}

	#endif

#endif



#ifdef TARGET_API_RTAS

	#ifdef TARGET_API_AUDIOSUITE
		#define DFX_NewEffectProcess	DFX_NewEffectProcessAS
	#endif
	#define DFX_EFFECT_ENTRY(PluginClass)		\
		CEffectProcess * DFX_NewEffectProcess()	\
		{										\
			return new PluginClass(NULL);		\
		}

#endif



#endif
// __DFXPLUGIN_H
