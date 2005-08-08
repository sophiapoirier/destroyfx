/*------------------------------------------------------------------------
Destroy FX is a sovereign entity comprised of Marc Poirier & Tom Murphy 7.
This is our class for E-Z plugin-making and E-Z multiple-API support.
written by Marc Poirier, October 2002
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
PLUGIN_DOUBLE_NAME_STRING
	the plugin name prefixed with "Destroy FX: "
PLUGIN_RES_ID
	component resource ID of the base plugin
PLUGIN_EDITOR_DESCRIPTION_STRING
	a C string description of the plugin editor
PLUGIN_EDITOR_DOUBLE_NAME_STRING
	a C string name for the editor, prefixed with "Destroy FX: "
PLUGIN_EDITOR_ID
	4-byte ID for the plugin (will PLUGIN_ID if not defined)
PLUGIN_EDITOR_RES_ID
	component resource ID of the base plugin
SUPPORT_AU_VERSION_1
	0 or 1 (this is used by the AUBase classes)
------------------------------------------------------------------------*/

#ifndef __DFXPLUGIN_H
#define __DFXPLUGIN_H


#include "dfxdefines.h"


// should be pretty much implied:  
// if the plugin is an instrument, then it uses MIDI
#if TARGET_PLUGIN_IS_INSTRUMENT
	#ifndef TARGET_PLUGIN_USES_MIDI
	#define TARGET_PLUGIN_USES_MIDI 1
	#endif
#endif

// handle base header includes and class names for the target plugin API

#if (defined(TARGET_API_AUDIOUNIT) + defined(TARGET_API_VST)) != 1

   #error "You must define exactly one of TARGET_API_AUDIOUNIT, TARGET_API_VST."

#endif

// using Apple's Audio Unit API
#if defined(TARGET_API_AUDIOUNIT)
	#if TARGET_PLUGIN_IS_INSTRUMENT
		#include "MusicDeviceBase.h"
		typedef MusicDeviceBase TARGET_API_BASE_CLASS;
		typedef ComponentInstance TARGET_API_BASE_INSTANCE_TYPE;
	#elif TARGET_PLUGIN_USES_MIDI
		#include "AUMIDIEffectBase.h"
		typedef AUMIDIEffectBase TARGET_API_BASE_CLASS;
		typedef ComponentInstance TARGET_API_BASE_INSTANCE_TYPE;
	#else
		#include "AUEffectBase.h"
		typedef AUEffectBase TARGET_API_BASE_CLASS;
		typedef AudioUnit TARGET_API_BASE_INSTANCE_TYPE;
	#endif
	#if !TARGET_PLUGIN_IS_INSTRUMENT
		#define TARGET_API_CORE_CLASS	AUKernelBase
	#endif

// using Steinberg's VST API
#elif defined(TARGET_API_VST)
	#include "audioeffectx.h"
	typedef AudioEffectX TARGET_API_BASE_CLASS;
	typedef audioMasterCallback TARGET_API_BASE_INSTANCE_TYPE;
//	#define TARGET_API_CORE_CLASS 0	// none in VST
	// set numinputs and numoutputs if numchannels is defined
	#ifdef VST_NUM_CHANNELS
		#ifndef VST_NUM_INPUTS
		#define VST_NUM_INPUTS	VST_NUM_CHANNELS
		#endif
		#ifndef VST_NUM_OUTPUTS
		#define VST_NUM_OUTPUTS	VST_NUM_CHANNELS
		#endif
	#endif

#endif
// end of target API check



// include our crucial shits

#include "dfxmath.h"
#include "dfxparameter.h"

#include "temporatetable.h"

#if TARGET_PLUGIN_USES_MIDI
	#include "dfxmidi.h"
	#include "dfxsettings.h"
#endif

#ifdef TARGET_API_AUDIOUNIT
	#include "dfxpluginproperties.h"
#endif




//-----------------------------------------------------------------------------
// constants and types

#ifdef TARGET_API_AUDIOUNIT
	enum {
		kDfxErr_NoError = noErr,
		kDfxErr_InitializationFailed = kAudioUnitErr_FailedInitialization
	};
#else
	enum {
		kDfxErr_NoError = 0,
		kDfxErr_InitializationFailed = -10875
	};
#endif


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
{
friend class DfxPluginCore;
public:
	// ***
	DfxPlugin(TARGET_API_BASE_INSTANCE_TYPE inInstance, long inNumParameters, long inNumPresets = 1);
	// ***
	virtual ~DfxPlugin();

	void dfxplugin_postconstructor();
	void dfxplugin_predestructor();

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

	bool parameterisvalid(long parameterIndex)
		{	return ( (parameterIndex >= 0) && (parameterIndex < numParameters) && (parameters != NULL) );	}

	void initparameter_f(long parameterIndex, const char * initName, double initValue, double initDefaultValue, 
						double initMin, double initMax, 
						DfxParamUnit initUnit = kDfxParamUnit_undefined, 
						DfxParamCurve initCurve = kDfxParamCurve_linear, 
						const char * initCustomUnitString = NULL);
	void initparameter_i(long parameterIndex, const char * initName, int64_t initValue, int64_t initDefaultValue, 
						int64_t initMin, int64_t initMax, 
						DfxParamUnit initUnit = kDfxParamUnit_undefined, 
						DfxParamCurve initCurve = kDfxParamCurve_stepped, 
						const char * initCustomUnitString = NULL);
	void initparameter_b(long parameterIndex, const char * initName, bool initValue, bool initDefaultValue, 
						DfxParamUnit initUnit = kDfxParamUnit_undefined);
	void initparameter_indexed(long parameterIndex, const char * initName, int64_t initValue, int64_t initDefaultValue, 
						int64_t initNumItems, DfxParamUnit initUnit = kDfxParamUnit_index, 
						const char * initCustomUnitString = NULL);

	void setparameterusevaluestrings(long parameterIndex, bool newMode=true)
		{	if (parameterisvalid(parameterIndex)) parameters[parameterIndex].setusevaluestrings(newMode);	}
	bool getparameterusevaluestrings(long parameterIndex)
		{	if (parameterisvalid(parameterIndex)) return parameters[parameterIndex].getusevaluestrings();	else return false;	}
	bool setparametervaluestring(long parameterIndex, int64_t stringIndex, const char * inText);
	bool getparametervaluestring(long parameterIndex, int64_t stringIndex, char * outText);
	void getparameterunitstring(long parameterIndex, char * outText)
		{	if (parameterisvalid(parameterIndex)) parameters[parameterIndex].getunitstring(outText);	}
	void setparametercustomunitstring(long parameterIndex, const char * inText)
		{	if (parameterisvalid(parameterIndex)) parameters[parameterIndex].setcustomunitstring(inText);	}
	char * getparametervaluestring_ptr(long parameterIndex, int64_t stringIndex);
#ifdef TARGET_API_AUDIOUNIT
	CFStringRef * getparametervaluecfstrings(long parameterIndex)
		{	if (parameterisvalid(parameterIndex)) return parameters[parameterIndex].getvaluecfstrings();   else return NULL;	}
	virtual CFStringRef CopyClumpName(UInt32 inClumpID)
		{	return NULL;	}
#endif

	void setparameter(long parameterIndex, DfxParamValue newValue);
	void setparameter_f(long parameterIndex, double newValue);
	void setparameter_i(long parameterIndex, int64_t newValue);
	void setparameter_b(long parameterIndex, bool newValue);
	void setparameter_gen(long parameterIndex, double newValue);
	// ***
	virtual void randomizeparameter(long parameterIndex);
	// ***
	virtual void randomizeparameters(bool writeAutomation = false);	// randomize all parameters at once
	void update_parameter(long parameterIndex);
	void postupdate_parameter(long parameterIndex);

	DfxParamValue getparameter(long parameterIndex)
		{	if (parameterisvalid(parameterIndex)) return parameters[parameterIndex].get();   else return DfxParamValue(); }
	double getparameter_f(long parameterIndex)
		{	if (parameterisvalid(parameterIndex)) return parameters[parameterIndex].get_f();   else return 0.0;	}
	int64_t getparameter_i(long parameterIndex)
		{	if (parameterisvalid(parameterIndex)) return parameters[parameterIndex].get_i();   else return 0;	}
	bool getparameter_b(long parameterIndex)
		{	if (parameterisvalid(parameterIndex)) return parameters[parameterIndex].get_b();   else return false;	}
	double getparameter_gen(long parameterIndex)
		{	if (parameterisvalid(parameterIndex)) return parameters[parameterIndex].get_gen();   else return 0.0;	}
	// return a (hopefully) 0 to 1 scalar version of the parameter's current value
	double getparameter_scalar(long parameterIndex);

	double getparametermin_f(long parameterIndex)
		{	if (parameterisvalid(parameterIndex)) return parameters[parameterIndex].getmin_f();   else return 0.0;	}
	int64_t getparametermin_i(long parameterIndex)
		{	if (parameterisvalid(parameterIndex)) return parameters[parameterIndex].getmin_i();   else return 0;	}
	double getparametermax_f(long parameterIndex)
		{	if (parameterisvalid(parameterIndex)) return parameters[parameterIndex].getmax_f();   else return 0.0;	}
	int64_t getparametermax_i(long parameterIndex)
		{	if (parameterisvalid(parameterIndex)) return parameters[parameterIndex].getmax_i();   else return 0;	}
	double getparameterdefault_f(long parameterIndex)
		{	if (parameterisvalid(parameterIndex)) return parameters[parameterIndex].getdefault_f();   else return 0.0;	}

	void getparametername(long parameterIndex, char * text);
#ifdef TARGET_API_AUDIOUNIT
	CFStringRef getparametercfname(long parameterIndex)
		{	if (parameterisvalid(parameterIndex)) return parameters[parameterIndex].getcfname();   else return NULL;	}
#endif
	DfxParamValueType getparametervaluetype(long parameterIndex);
	DfxParamUnit getparameterunit(long parameterIndex);
	bool getparameterchanged(long parameterIndex);
	void setparameterchanged(long parameterIndex, bool newChanged = true);
	DfxParamCurve getparametercurve(long parameterIndex)
		{	if (parameterisvalid(parameterIndex)) return parameters[parameterIndex].getcurve();   else return kDfxParamCurve_undefined;	}
	void setparametercurve(long parameterIndex, DfxParamCurve newcurve)
		{	if (parameterisvalid(parameterIndex)) parameters[parameterIndex].setcurve(newcurve);	}
	double getparametercurvespec(long parameterIndex)
		{	if (parameterisvalid(parameterIndex)) return parameters[parameterIndex].getcurvespec();   else return 0.0;	}
	void setparametercurvespec(long parameterIndex, double newcurvespec)
		{	if (parameterisvalid(parameterIndex)) parameters[parameterIndex].setcurvespec(newcurvespec);	}
	unsigned long getparameterattributes(long parameterIndex)
		{	if (parameterisvalid(parameterIndex)) return parameters[parameterIndex].getattributes();   else return 0;	}
	void setparameterattributes(long parameterIndex, unsigned long inFlags)
		{	if (parameterisvalid(parameterIndex)) parameters[parameterIndex].setattributes(inFlags);	}
	void addparameterattributes(long parameterIndex, unsigned long inFlags)
	{
		if (parameterisvalid(parameterIndex))
			parameters[parameterIndex].setattributes( inFlags | parameters[parameterIndex].getattributes() );
	}

	// convenience methods for expanding and contracting parameter values 
	// using the min/max/curvetype/curvespec/etc. settings of a given parameter
	double expandparametervalue_index(long parameterIndex, double genValue);
	double contractparametervalue_index(long parameterIndex, double realValue);

	// whether or not the index is a valid preset
	bool presetisvalid(long presetIndex);
	// whether or not the index is a valid preset with a valid name
	bool presetnameisvalid(long presetIndex);
	// load the settings of a preset
	virtual bool loadpreset(long presetIndex);
	// set a parameter value in all of the empty (no name) presets 
	// to the current value of that parameter
	void initpresetsparameter(long parameterIndex);
	// set the text of a preset name
	void setpresetname(long presetIndex, const char * inText);
	// get a copy of the text of a preset name
	void getpresetname(long presetIndex, char * outText);
	// get a pointer to the text of a preset name
	char * getpresetname_ptr(long presetIndex);
#ifdef TARGET_API_AUDIOUNIT
	CFStringRef getpresetcfname(long presetIndex);
#endif
	long getcurrentpresetnum()
		{	return currentPresetNum;	}
	void setpresetparameter(long presetIndex, long parameterIndex, DfxParamValue newValue);
	void setpresetparameter_f(long presetIndex, long parameterIndex, double newValue);
	void setpresetparameter_i(long presetIndex, long parameterIndex, int64_t newValue);
	void setpresetparameter_b(long presetIndex, long parameterIndex, bool newValue);
	void setpresetparameter_gen(long presetIndex, long parameterIndex, double genValue);
	void update_preset(long presetIndex);
	DfxParamValue getpresetparameter(long presetIndex, long parameterIndex);
	double getpresetparameter_f(long presetIndex, long parameterIndex);


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

//	virtual void SetUseTimeStampedParameters(bool newmode = true)
//		{	b_usetimestampedparameters = newmode;	}
//	bool GetUseTimeStampedParameters()
//		{	return b_usetimestampedparameters;	}

	void getpluginname(char * outText);
	long getpluginversion()
		{	return PLUGIN_VERSION;	}

	#if TARGET_PLUGIN_USES_MIDI
		// ***
		// handlers for the types of MIDI events that we support
		virtual void handlemidi_noteon(int channel, int note, int velocity, long frameOffset);
		virtual void handlemidi_noteoff(int channel, int note, int velocity, long frameOffset);
		virtual void handlemidi_allnotesoff(int channel, long frameOffset);
		virtual void handlemidi_pitchbend(int channel, int valueLSB, int valueMSB, long frameOffset);
		virtual void handlemidi_cc(int channel, int controllerNum, int value, long frameOffset);
		virtual void handlemidi_programchange(int channel, int programNum, long frameOffset);

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

	bool sampleratechanged;

	#if TARGET_PLUGIN_USES_MIDI
		DfxMidi * midistuff;
		DfxSettings * dfxsettings;
	#endif

	long numParameters;
	long numPresets;
	long currentPresetNum;

	unsigned long numInputs, numOutputs;
	double samplerate;

	#ifdef TARGET_API_AUDIOUNIT
		bool auElementsHaveBeenCreated;
		// array of float pointers to input and output audio buffers, 
		// just for the sake of making processaudio(float**, float**, etc.) possible
		float ** inputsP;
		float ** outputsP;
		#if TARGET_PLUGIN_USES_MIDI
			// an array of how MIDI CCs and NRPNs map to parameters (if at all)
			// the map needs to survive throughout the Audio Unit's life
			AudioUnitMIDIControlMapping * aumidicontrolmap;
		#endif
	#endif

	#ifdef TARGET_API_VST
		bool latencychanged;
		bool isinitialized;
		#if TARGET_PLUGIN_USES_DSPCORE
			DfxPluginCore ** dspcores;	// we have to handle this ourselves because VST can't
		#endif
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


// overridden virtual methods from inherited API base classes
public:

#ifdef TARGET_API_AUDIOUNIT
	virtual void PostConstructor();
	virtual void PreDestructor();
	virtual ComponentResult Initialize();
	virtual void Cleanup();
	virtual ComponentResult Reset(AudioUnitScope inScope, AudioUnitElement inElement);

	#if TARGET_PLUGIN_IS_INSTRUMENT
		virtual ComponentResult Render(AudioUnitRenderActionFlags & ioActionFlags, 
								const AudioTimeStamp & inTimeStamp, UInt32 inFramesToProcess);
	#else
		virtual OSStatus ProcessBufferLists(AudioUnitRenderActionFlags & ioActionFlags, 
						const AudioBufferList & inBuffer, AudioBufferList & outBuffer, 
						UInt32 inFramesToProcess);
	#endif
	#if TARGET_PLUGIN_USES_DSPCORE
		virtual AUKernelBase * NewKernel();
	#endif

	virtual ComponentResult GetPropertyInfo(AudioUnitPropertyID inPropertyID, 
					AudioUnitScope inScope, AudioUnitElement inElement, 
					UInt32 & outDataSize, Boolean & outWritable);
	virtual ComponentResult GetProperty(AudioUnitPropertyID inPropertyID, 
					AudioUnitScope inScope, AudioUnitElement inElement, 
					void * outData);
	virtual ComponentResult SetProperty(AudioUnitPropertyID inPropertyID, 
					AudioUnitScope inScope, AudioUnitElement inElement, 
					const void * inData, UInt32 inDataSize);

	virtual ComponentResult	Version();
	virtual UInt32 SupportedNumChannels(const AUChannelInfo ** outInfo);
	virtual Float64 GetLatency();
	virtual Float64 GetTailTime();
	virtual bool SupportsTail()
		{	return true;	}

	virtual ComponentResult GetParameterInfo(AudioUnitScope inScope, 
					AudioUnitParameterID inParameterID, 
					AudioUnitParameterInfo & outParameterInfo);
	virtual ComponentResult GetParameterValueStrings(AudioUnitScope inScope, 
					AudioUnitParameterID inParameterID, CFArrayRef * outStrings);
	virtual ComponentResult SetParameter(AudioUnitParameterID inParameterID, 
					AudioUnitScope inScope, AudioUnitElement inElement, 
					Float32 inValue, UInt32 inBufferOffsetInFrames);

	virtual	ComponentResult ChangeStreamFormat(AudioUnitScope inScope, 
					AudioUnitElement inElement, 
					const CAStreamBasicDescription & inPrevFormat, 
					const CAStreamBasicDescription & inNewFormat);

	virtual ComponentResult SaveState(CFPropertyListRef * outData);
	virtual ComponentResult RestoreState(CFPropertyListRef inData);
	virtual ComponentResult GetPresets(CFArrayRef * outData) const;
	virtual OSStatus NewFactoryPresetSet(const AUPreset & inNewFactoryPreset);

	#if TARGET_PLUGIN_USES_MIDI
		virtual void HandleNoteOn(int inChannel, UInt8 inNoteNumber, 
						UInt8 inVelocity, long inFrameOffset);
		virtual void HandleNoteOff(int inChannel, UInt8 inNoteNumber, 
						UInt8 inVelocity, long inFrameOffset);
		virtual void HandleAllNotesOff(int inChannel);
		virtual void HandleControlChange(int inChannel, UInt8 inController, 
						UInt8 inValue, long inFrameOffset);
		virtual void HandlePitchWheel(int inChannel, UInt8 inPitchLSB, UInt8 inPitchMSB, 
						long inFrameOffset);
		virtual void HandleProgramChange(int inChannel, UInt8 inProgramNum);
		virtual void HandleChannelPressure(int inChannel, UInt8 inValue, long inFrameOffset)
			{ }
		virtual void HandlePolyPressure(int inChannel, UInt8 inKey, 
						UInt8 inValue, long inFrameOffset)
			{ }
		virtual void HandleResetAllControllers(int inChannel)
			{ }
		virtual void HandleAllSoundOff(int inChannel)
			{ }
	#endif
	#if TARGET_PLUGIN_IS_INSTRUMENT
		virtual ComponentResult PrepareInstrument(MusicDeviceInstrumentID inInstrument);
		virtual ComponentResult ReleaseInstrument(MusicDeviceInstrumentID inInstrument);
		virtual ComponentResult StartNote(MusicDeviceInstrumentID inInstrument, 
						MusicDeviceGroupID inGroupID, NoteInstanceID & outNoteInstanceID, 
						UInt32 inOffsetSampleFrame, const MusicDeviceNoteParams & inParams);
		virtual ComponentResult StopNote(MusicDeviceGroupID inGroupID, 
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

	virtual void process(float ** inputs, float ** outputs, long sampleFrames);
	virtual void processReplacing(float ** inputs, float ** outputs, long sampleFrames);

	virtual void suspend();
	virtual void resume();
	virtual long fxIdle();
	virtual void setSampleRate(float newRate);

	virtual long getTailSize();
	// there was a typo in the VST header files versions 2.0 through 2.2, 
	// so some hosts will still call this incorrectly named version...
	virtual long getGetTailSize() { return getTailSize(); }
	virtual bool getInputProperties(long index, VstPinProperties * properties);
	virtual bool getOutputProperties(long index, VstPinProperties * properties);

	virtual void setProgram(long programNum);
	virtual void setProgramName(char * name);
	virtual void getProgramName(char * name);
	virtual bool getProgramNameIndexed(long category, long index, char * name);
	virtual bool copyProgram(long destination);

	virtual void setParameter(long index, float value);
	virtual float getParameter(long index);
	virtual void getParameterName(long index, char * name);
	virtual void getParameterDisplay(long index, char * text);
	virtual void getParameterLabel(long index, char * label);

	virtual bool getEffectName(char * name);
	virtual long getVendorVersion();
	virtual bool getErrorText(char * text);
	virtual bool getVendorString(char * text);
	virtual bool getProductString(char * text);

	virtual long canDo(char * text);

	#if TARGET_PLUGIN_USES_MIDI
		virtual long processEvents(VstEvents * events);
		virtual long setChunk(void * data, long byteSize, bool isPreset);
		virtual long getChunk(void ** data, bool isPreset);
	#endif

	// DFX supplementary VST methods
	void setlatencychanged(bool newstatus = true)
		{	latencychanged = newstatus;	}
	bool getlatencychanged()
		{	return latencychanged;	}
	#if TARGET_PLUGIN_USES_DSPCORE
		DfxPluginCore * getplugincore(unsigned long channel)
		{
			if (channel >= getnumoutputs())
				return NULL;
			if (dspcores == NULL)
				return NULL;
			return dspcores[channel];
		}
	#endif
#endif
// end of VST API methods

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



//-----------------------------------------------------------------------------
// prototypes for a few handy buffer helper functions

bool createbuffer_f(float ** buffer, long currentBufferSize, long desiredBufferSize);
bool createbuffer_d(double ** buffer, long currentBufferSize, long desiredBufferSize);
bool createbuffer_i(long ** buffer, long currentBufferSize, long desiredBufferSize);
bool createbuffer_b(bool ** buffer, long currentBufferSize, long desiredBufferSize);
bool createbufferarray_f(float *** buffers, unsigned long currentNumBuffers, long currentBufferSize, 
						unsigned long desiredNumBuffers, long desiredBufferSize);
bool createbufferarrayarray_d(double **** buffers, unsigned long currentNumBufferArrays, unsigned long currentNumBuffers, 
							long currentBufferSize, unsigned long desiredNumBufferArrays, 
							unsigned long desiredNumBuffers, long desiredBufferSize);

void releasebuffer_f(float ** buffer);
void releasebuffer_d(double ** buffer);
void releasebuffer_i(long ** buffer);
void releasebuffer_b(bool ** buffer);
void releasebufferarray_f(float *** buffers, unsigned long numbuffers);
void releasebufferarrayarray_d(double **** buffers, unsigned long numbufferarrays, unsigned long numbuffers);

void clearbuffer_f(float * buffer, long buffersize, float value = 0.0f);
void clearbuffer_d(double * buffer, long buffersize, double value = 0.0);
void clearbuffer_i(long * buffer, long buffersize, long value = 0);
void clearbuffer_b(bool * buffer, long buffersize, bool value = false);
void clearbufferarray_f(float ** buffers, unsigned long numbuffers, long buffersize, float value = 0.0f);
void clearbufferarrayarray_d(double *** buffers, unsigned long numbufferarrays, unsigned long numbuffers, 
							long buffersize, double value = 0.0);

long launch_url(const char * urlstring);






//-----------------------------------------------------------------------------
// plugin entry point macros and defines and stuff

#ifdef TARGET_API_AUDIOUNIT

	#define DFX_ENTRY(PluginClass)   COMPONENT_ENTRY(PluginClass)

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
	#endif

#endif



#ifdef TARGET_API_VST

	#if BEOS
		#define main main_plugin
		extern "C" __declspec(dllexport) AEffect * main_plugin(audioMasterCallback audioMaster);
	#elif MACX
		#define main main_macho
		extern "C" AEffect * main_macho(audioMasterCallback audioMaster);
	#else
		AEffect * main(audioMasterCallback audioMaster);
	#endif

#define DFX_ENTRY(PluginClass)									\
	AEffect * main(audioMasterCallback audioMaster)				\
	{															\
		if ( !audioMaster(0, audioMasterVersion, 0, 0, 0, 0) )	\
			return NULL;										\
		DfxPlugin * effect = new PluginClass(audioMaster);		\
		if (effect == NULL)										\
			return NULL;										\
		effect->dfxplugin_postconstructor();					\
		return effect->getAeffect();							\
	}

	// we need to manage the DSP cores manually in VST
	// call this in the plugin's constructor if it uses DSP cores for processing
	#if TARGET_PLUGIN_USES_DSPCORE
		// DFX_CORE_ENTRY is not useful for VST, so it is defined as nothing
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



#endif
// __DFXPLUGIN_H
