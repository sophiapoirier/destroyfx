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
TARGET_API_AUDIOUNIT
TARGET_API_VST
	0 or 1

 necessary for Audio Unit:
PLUGIN_DOUBLE_NAME_STRING
	the plugin name prefixed with "Destroy FX: "
PLUGIN_ENTRY_POINT
	a C string of the base plugin class name with "Entry" appended

 necessary for Audio Units using a custom GUI (TARGET_PLUGIN_HAS_GUI = 1):
PLUGIN_EDITOR_DOUBLE_NAME_STRING
	a C string name for the editor, prefixed with "Destroy FX: "
PLUGIN_EDITOR_ENTRY_POINT
	a C string of the plugin editor class name with "Entry" appended

 necessary for VST:
NUM_INPUTS
NUM_OUTPUTS
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
SUPPORT_AU_VERSION_1
	0 or 1 (this is used by the AUBase classes)
------------------------------------------------------------------------*/

#ifndef __DFXPLUGIN_H
#define __DFXPLUGIN_H



// should be pretty much implied:  
// if the plugin is an instrument, then it uses MIDI
#if TARGET_PLUGIN_IS_INSTRUMENT
	#ifndef TARGET_PLUGIN_USES_MIDI
	#define TARGET_PLUGIN_USES_MIDI 1
	#endif
#endif



// include our crucial shits

#ifndef __DFXMATH_H
#include "dfxmath.h"
#endif

#ifndef __DFXPARAMETER_H
#include "dfxparameter.h"
#endif

#ifndef __DFX_TEMPORATETABLE_H
#include "temporatetable.h"
#endif


#if TARGET_PLUGIN_USES_MIDI

	#ifndef __DFXMIDI_H
	#include "dfxmidi.h"
	#endif

	#ifndef __DFXSETTINGS_H
	#include "dfxsettings.h"
	#endif

#endif


#if WIN32
/* turn off warnings about default but no cases in switch, unknown pragma, etc. */
   #pragma warning( disable : 4065 57 4200 4244 4068 )
   #include <windows.h>
#endif



// handle base header includes and class names for the target plugin API

// using Apple's Audio Unit API
#if TARGET_API_AUDIOUNIT
	#define TARGET_API_CORE_CLASS AUKernelBase
	#define TARGET_API_CORE_INSTANCE_TYPE AUEffectBase

	#if TARGET_PLUGIN_IS_INSTRUMENT
		#define BASE_API_HEADER "MusicDeviceBase.h"
		#define BASE_API_HEADER_DEFINITION __MusicDeviceBase_h__
		#define TARGET_API_BASE_CLASS MusicDeviceBase
		#define TARGET_API_BASE_INSTANCE_TYPE ComponentInstance
	#elif TARGET_PLUGIN_USES_MIDI
		#define BASE_API_HEADER "AUMIDIEffectBase.h"
		#define BASE_API_HEADER_DEFINITION __AUMIDIEffectBase_h__
		#define TARGET_API_BASE_CLASS AUMIDIEffectBase
		#define TARGET_API_BASE_INSTANCE_TYPE ComponentInstance
	#else
		#define BASE_API_HEADER "AUEffectBase.h"
		#define BASE_API_HEADER_DEFINITION __AUEffectBase_h__
		#define TARGET_API_BASE_CLASS AUEffectBase
		#define TARGET_API_BASE_INSTANCE_TYPE AudioUnit
	#endif

// using Steinberg's VST API
#elif TARGET_API_VST
	#define BASE_API_HEADER "audioeffectx.h"
	#define BASE_API_HEADER_DEFINITION __audioeffectx__
	#define TARGET_API_BASE_CLASS AudioEffectX
	#define TARGET_API_BASE_INSTANCE_TYPE audioMasterCallback
//	#define TARGET_API_CORE_CLASS 0	// none in VST
	#define TARGET_API_CORE_INSTANCE_TYPE DfxPlugin

#endif
// end of target API check


#include BASE_API_HEADER



//-----------------------------------------------------------------------------
// constants & macros

#define DESTROY_FX_RULEZ

#define DESTROYFX_NAME_STRING	"Destroy FX"
#define DESTROYFX_COLLECTION_NAME	"Super Destroy FX bipolar plugin pack"
#define DESTROYFX_URL "http://www.smartelectronix.com/~destroyfx/"
#define SMARTELECTRONIX_URL "http://www.smartelectronix.com/"

#define DESTROYFX_ID 'DFX!'



//-----------------------------------------------------------------------------
//enum DfxPluginCreationFlags {
//	kDfxCreate_UseMidi = 1,
//	kDfxCreate_Instrument = 1 << 1,
//	kDfxCreate
//};


struct DfxTimeInfo {
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
	// XXX implement this for Audio Unit
	bool playbackChanged;	// whether or not the playback state or position just changed
};


#if TARGET_API_AUDIOUNIT
	// the Audio Unit API already has an i/o configurations structure
//	typedef struct DfxChannelConfig AUChannelInfo;
	#define DfxChannelConfig AUChannelInfo
#else
	// immitate AUChannelInfo from the Audio Unit API for other APIs
	struct DfxChannelConfig {
		short inChannels;
		short outChannels;
	};
#endif



class DfxPluginCore;

#pragma mark _________DfxPlugin_________
//-----------------------------------------------------------------------------
class DfxPlugin : public TARGET_API_BASE_CLASS
{
friend class DfxPluginCore;
public:
	// ***
	DfxPlugin(TARGET_API_BASE_INSTANCE_TYPE inInstance, long numParameters, long numPresets = 1);
	// ***
	~DfxPlugin();

	void dfxplugin_postconstructor();

	long do_initialize();
	// ***
	virtual long initialize()
		{	return 0;	}
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
	virtual void processaudio(const float **in, float **out, unsigned long inNumFrames, 
						bool replacing=true)
		{ }

	bool parameterisvalid(long parameterIndex)
		{	return ( (parameterIndex >= 0) && (parameterIndex < numParameters) && (parameters != NULL) );	}

	void initparameter_f(long parameterIndex, const char *initName, float initValue, float initDefaultValue, 
						float initMin, float initMax, 
						DfxParamUnit initUnit = kDfxParamUnit_undefined, 
						DfxParamCurve initCurve = kDfxParamCurve_linear);
	void initparameter_d(long parameterIndex, const char *initName, double initValue, double initDefaultValue, 
						double initMin, double initMax, 
						DfxParamUnit initUnit = kDfxParamUnit_undefined, 
						DfxParamCurve initCurve = kDfxParamCurve_linear);
	void initparameter_i(long parameterIndex, const char *initName, long initValue, long initDefaultValue, 
						long initMin, long initMax, 
						DfxParamUnit initUnit = kDfxParamUnit_undefined, 
						DfxParamCurve initCurve = kDfxParamCurve_stepped);
	void initparameter_ui(long parameterIndex, const char *initName, unsigned long initValue, unsigned long initDefaultValue, 
						unsigned long initMin, unsigned long initMax, 
						DfxParamUnit initUnit = kDfxParamUnit_undefined, 
						DfxParamCurve initCurve = kDfxParamCurve_linear);
	void initparameter_b(long parameterIndex, const char *initName, bool initValue, bool initDefaultValue, 
						DfxParamUnit initUnit = kDfxParamUnit_undefined, 
						DfxParamCurve initCurve = kDfxParamCurve_linear);
	void initparameter_indexed(long parameterIndex, const char *initName, long initValue, long initDefaultValue, long initNumItems);

	bool setparametervaluestring(long parameterIndex, long stringIndex, const char *inText);
	bool getparametervaluestring(long parameterIndex, long stringIndex, char *outText);
	char * getparametervaluestring_ptr(long parameterIndex, long stringIndex);
#if TARGET_API_AUDIOUNIT
	CFStringRef * getparametervaluecfstrings(long parameterIndex)
		{	if (parameterisvalid(parameterIndex)) return parameters[parameterIndex].getvaluecfstrings();   else return NULL;	}
#endif

	void setparameter(long parameterIndex, DfxParamValue newValue);
	void setparameter_f(long parameterIndex, float newValue);
	void setparameter_d(long parameterIndex, double newValue);
	void setparameter_i(long parameterIndex, long newValue);
	void setparameter_b(long parameterIndex, bool newValue);
	void setparameter_gen(long parameterIndex, float newValue);
	void randomizeparameter(long parameterIndex);
	void update_parameter(long parameterIndex);

	DfxParamValue getparameter(long parameterIndex)
		{	if (parameterisvalid(parameterIndex)) return parameters[parameterIndex].get();   else { DfxParamValue dummy; return dummy; }	}
	float getparameter_f(long parameterIndex)
		{	if (parameterisvalid(parameterIndex)) return parameters[parameterIndex].get_f();   else return 0.0f;	}
	double getparameter_d(long parameterIndex)
		{	if (parameterisvalid(parameterIndex)) return parameters[parameterIndex].get_d();   else return 0.0;	}
	long getparameter_i(long parameterIndex)
		{	if (parameterisvalid(parameterIndex)) return parameters[parameterIndex].get_i();   else return 0;	}
	unsigned long getparameter_ui(long parameterIndex)
		{	if (parameterisvalid(parameterIndex)) return parameters[parameterIndex].get_ui();   else return 0;	}
	bool getparameter_b(long parameterIndex)
		{	if (parameterisvalid(parameterIndex)) return parameters[parameterIndex].get_b();   else return false;	}
	char getparameter_c(long parameterIndex)
		{	if (parameterisvalid(parameterIndex)) return parameters[parameterIndex].get_c();   else return 0;	}
	unsigned char getparameter_uc(long parameterIndex)
		{	if (parameterisvalid(parameterIndex)) return parameters[parameterIndex].get_uc();   else return 0;	}
	float getparameter_gen(long parameterIndex)
		{	if (parameterisvalid(parameterIndex)) return parameters[parameterIndex].get_gen();   else return 0.0f;	}
	// return a (hopefully) 0 to 1 scalar version of the parameter's current value
	float getparameter_scalar(long parameterIndex);

	float getparametermin_f(long parameterIndex)
		{	if (parameterisvalid(parameterIndex)) return parameters[parameterIndex].getmin_f();   else return 0.0f;	}
	double getparametermin_d(long parameterIndex)
		{	if (parameterisvalid(parameterIndex)) return parameters[parameterIndex].getmin_d();   else return 0.0;	}
	long getparametermin_i(long parameterIndex)
		{	if (parameterisvalid(parameterIndex)) return parameters[parameterIndex].getmin_i();   else return 0;	}
	float getparametermax_f(long parameterIndex)
		{	if (parameterisvalid(parameterIndex)) return parameters[parameterIndex].getmax_f();   else return 0.0f;	}
	double getparametermax_d(long parameterIndex)
		{	if (parameterisvalid(parameterIndex)) return parameters[parameterIndex].getmax_d();   else return 0.0;	}
	long getparametermax_i(long parameterIndex)
		{	if (parameterisvalid(parameterIndex)) return parameters[parameterIndex].getmax_i();   else return 0;	}
	float getparameterdefault_f(long parameterIndex)
		{	if (parameterisvalid(parameterIndex)) return parameters[parameterIndex].getdefault_f();   else return 0.0f;	}

	void getparametername(long parameterIndex, char *text);
	DfxParamValueType getparametervaluetype(long parameterIndex);
	DfxParamUnit getparameterunit(long parameterIndex);
	bool getparameterchanged(long parameterIndex);
	void setparameterchanged(long parameterIndex, bool newChanged = true);
	double getparametercurvespec(long parameterIndex)
		{	if (parameterisvalid(parameterIndex)) return parameters[parameterIndex].getcurvespec();   else return 0.0;	}
	void setparametercurvespec(long parameterIndex, double newcurvespec)
		{	if (parameterisvalid(parameterIndex)) parameters[parameterIndex].setcurvespec(newcurvespec);	}

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
	void setpresetname(long presetIndex, const char *inText);
	// get a copy of the text of a preset name
	void getpresetname(long presetIndex, char *outText);
	// get a pointer to the text of a preset name
	char * getpresetname_ptr(long presetIndex);
#if TARGET_API_AUDIOUNIT
	CFStringRef getpresetcfname(long presetIndex);
#endif
	long getcurrentpresetnum()
		{	return currentPresetNum;	}
	void setpresetparameter(long presetIndex, long parameterIndex, DfxParamValue newValue);
	void setpresetparameter_f(long presetIndex, long parameterIndex, float newValue);
	void setpresetparameter_d(long presetIndex, long parameterIndex, double newValue);
	void setpresetparameter_i(long presetIndex, long parameterIndex, long newValue);
	void setpresetparameter_b(long presetIndex, long parameterIndex, bool newValue);
	void setpresetparameter_gen(long presetIndex, long parameterIndex, float genValue);
	void update_preset(long presetIndex);
	DfxParamValue getpresetparameter(long presetIndex, long parameterIndex);
	float getpresetparameter_f(long presetIndex, long parameterIndex);


	// get the current audio sampling rate
	double getsamplerate()
		{	return DfxPlugin::samplerate;	}
	// convenience wrapper of getsamplerate to get float type value
	float getsamplerate_f()
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

	void setlatency_samples(long newlatency)
	{
		latency_samples = newlatency;
		b_uselatency_seconds = false;
	}
	void setlatency_seconds(double newlatency)
	{
		latency_seconds = newlatency;
		b_uselatency_seconds = true;
	}
	long getlatency_samples()
	{
		if (b_uselatency_seconds)
			return (long) (latency_seconds * getsamplerate());
		else
			return latency_samples;
	}
	double getlatency_seconds()
	{
		if (b_uselatency_seconds)
			return latency_seconds;
		else
			return (double)latency_samples / getsamplerate();
	}

	void settailsize_samples(long newsize)
	{
		tailsize_samples = newsize;
		b_usetailsize_seconds = false;
	}
	void settailsize_seconds(double newsize)
	{
		tailsize_seconds = newsize;
		b_usetailsize_seconds = true;
	}
	long gettailsize_samples()
	{
		if (b_usetailsize_seconds)
			return (long) (tailsize_seconds * getsamplerate());
		else
			return tailsize_samples;
	}
	double gettailsize_seconds()
	{
		if (b_usetailsize_seconds)
			return tailsize_seconds;
		else
			return (double)tailsize_samples / getsamplerate();
	}

	// add an audio input/output configuration to the array of i/o configurations
	void addchannelconfig(short numin, short numout);

//	virtual void SetUseTimeStampedParameters(bool newmode = true)
//		{	b_usetimestampedparameters = newmode;	}
//	bool GetUseTimeStampedParameters()
//		{	return b_usetimestampedparameters;	}

	void getpluginname(char *name)
		{	if (name) strcpy(name, PLUGIN_NAME_STRING);	}
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
		virtual void settings_saveExtendedData(void *data, bool isPreset) {}
		virtual void settings_restoreExtendedData(void *data, unsigned long storedExtendedDataSize, 
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
	DfxParam *parameters;
	DfxPreset *presets;

	DfxChannelConfig *channelconfigs;
	long numchannelconfigs;

	DfxTimeInfo timeinfo;
	bool b_usemusicaltimeinfo;	// XXX use this?
	bool hostCanDoTempo;
	TempoRateTable *tempoRateTable;	// a table of tempo rate values

	bool b_usetimestampedparameters;	// XXX use this?

	bool audioBuffersAllocated;
	bool sampleratechanged;

	#if TARGET_PLUGIN_USES_MIDI
		DfxMidi *midistuff;
		DfxSettings *dfxsettings;
	#endif

	long numParameters;
	long numPresets;
	long currentPresetNum;

	unsigned long numInputs, numOutputs;
	double samplerate;

	#if TARGET_API_AUDIOUNIT
		// array of float pointers to input and output audio buffers, 
		// just for the sake of making processaudio(float**, float**, etc.) possible
		float **inputsP, **outputsP;
		// an array of the plugin's presets in AUPreset form (number and CFString name)
		// the preset array needs to survive throughout the Audio Unit's life
		AUPreset *aupresets;
		#if TARGET_PLUGIN_USES_MIDI
			// an array of how MIDI CCs and NRPNs map to parameters (if at all)
			// the map needs to survive throughout the Audio Unit's life
			AudioUnitMIDIControlMapping *aumidicontrolmap;
		#endif
	#endif

	#if TARGET_API_VST
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


// overridden virtual methods from inherited API base classes
public:

#if TARGET_API_AUDIOUNIT
	virtual void PostConstructor();
	virtual ComponentResult Initialize();
	virtual void Cleanup();
	virtual void Reset();

	virtual OSStatus ProcessBufferLists(AudioUnitRenderActionFlags &ioActionFlags, 
					const AudioBufferList &inBuffer, AudioBufferList &outBuffer, 
					UInt32 inFramesToProcess);
	#if TARGET_PLUGIN_USES_DSPCORE
		virtual AUKernelBase * NewKernel();
	#endif

	virtual ComponentResult GetPropertyInfo(AudioUnitPropertyID inID, 
					AudioUnitScope inScope, AudioUnitElement inElement, 
					UInt32 &outDataSize, Boolean &outWritable);
	virtual ComponentResult GetProperty(AudioUnitPropertyID inID, 
					AudioUnitScope inScope, AudioUnitElement inElement, 
					void *outData);
//	virtual ComponentResult SetProperty(AudioUnitPropertyID inID, 
//					AudioUnitScope inScope, AudioUnitElement inElement, 
//					const void *inData, UInt32 inDataSize);

	virtual UInt32 SupportedNumChannels(const AUChannelInfo **outInfo);
    virtual Float64 GetLatency();
    virtual Float64 GetTailTime();
	virtual bool SupportsRampAndTail()
		{	return true;	}

	virtual ComponentResult GetParameterInfo(AudioUnitScope inScope, 
					AudioUnitParameterID inParameterID, 
					AudioUnitParameterInfo &outParameterInfo);
	virtual ComponentResult GetParameterValueStrings(AudioUnitScope inScope, 
					AudioUnitParameterID inParameterID, CFArrayRef *outStrings);
	virtual ComponentResult SetParameter(AudioUnitParameterID inID, 
					AudioUnitScope inScope, AudioUnitElement inElement, 
					Float32 inValue, UInt32 inBufferOffsetInFrames);

	virtual	ComponentResult ChangeStreamFormat(AudioUnitScope inScope, 
					AudioUnitElement inElement, 
					const CAStreamBasicDescription &inPrevFormat, 
					const CAStreamBasicDescription &inNewFormat);

	virtual ComponentResult SaveState(CFPropertyListRef *outData);
	virtual ComponentResult RestoreState(CFPropertyListRef inData);
	virtual ComponentResult GetPresets(CFArrayRef *outData) const;
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
		virtual ComponentResult PrepareInstrument(MusicDeviceInstrumentID inInstrument) = 0;
		virtual ComponentResult ReleaseInstrument(MusicDeviceInstrumentID inInstrument) = 0;
		virtual ComponentResult StartNote(MusicDeviceInstrumentID inInstrument, 
						MusicDeviceGroupID inGroupID, NoteInstanceID *outNoteInstanceID, 
						UInt32 inOffsetSampleFrame, const MusicDeviceNoteParams *inParams) = 0;
		virtual ComponentResult StopNote(MusicDeviceGroupID inGroupID, 
						NoteInstanceID inNoteInstanceID, UInt32 inOffsetSampleFrame) = 0;
	#endif
#endif
// end of Audio Unit API methods


#if TARGET_API_VST
	virtual void process(float **inputs, float **outputs, long sampleFrames);
	virtual void processReplacing(float **inputs, float **outputs, long sampleFrames);

	virtual void suspend();
	virtual void resume();
	virtual long fxIdle();
	virtual void setSampleRate(float newRate);

	virtual long getTailSize();
	// there was a typo in the VST header files versions 2.0 through 2.2, 
	// so some hosts will still call this incorrectly named version...
	virtual long getGetTailSize() { return getTailSize(); }
	virtual bool getInputProperties(long index, VstPinProperties *properties);
	virtual bool getOutputProperties(long index, VstPinProperties *properties);

	virtual void setProgram(long programNum);
	virtual void setProgramName(char *name);
	virtual void getProgramName(char *name);
	virtual bool getProgramNameIndexed(long category, long index, char *name);
	virtual bool copyProgram(long destination);

	virtual void setParameter(long index, float value);
	virtual float getParameter(long index);
	virtual void getParameterName(long index, char *name);
	virtual void getParameterDisplay(long index, char *text);
	virtual void getParameterLabel(long index, char *label);

	virtual bool getEffectName(char *name);
	virtual long getVendorVersion();
	virtual bool getErrorText(char *text);
	virtual bool getVendorString(char *text);
	virtual bool getProductString(char *text);

	virtual long canDo(char *text);

	#if TARGET_PLUGIN_USES_MIDI
		virtual long processEvents(VstEvents *events);
		virtual long setChunk(void *data, long byteSize, bool isPreset);
		virtual long getChunk(void **data, bool isPreset);
	#endif

	#if TARGET_PLUGIN_USES_DSPCORE
		DfxPluginCore **dspcores;	// we have to handle this ourselves because VST can't
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
	DfxPluginCore(TARGET_API_CORE_INSTANCE_TYPE *inInstance) :
		#ifdef TARGET_API_CORE_CLASS
		TARGET_API_CORE_CLASS(inInstance), 
		#endif
		dfxplugin((DfxPlugin*)inInstance)
		{ }

	virtual ~DfxPluginCore()
		{	releasebuffers();	}	// XXX this doesn't work from parent class

	void dfxplugincore_postconstructor()
	{
		do_reset();
	}

	void do_process(const float *in, float *out, unsigned long inNumFrames, 
						bool replacing=true)
	{
		processparameters();
		process(in, out, inNumFrames, replacing);
	}
	virtual void process(const float *in, float *out, unsigned long inNumFrames, 
						bool replacing=true)
		{ }
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
//	DfxParam getparameter(long index)
//		{	return dfxplugin->getparameter(index);	}
	float getparameter_f(long index)
		{	return dfxplugin->getparameter_f(index);	}
	double getparameter_d(long index)
		{	return dfxplugin->getparameter_d(index);	}
	long getparameter_i(long index)
		{	return dfxplugin->getparameter_i(index);	}
	bool getparameter_b(long index)
		{	return dfxplugin->getparameter_b(index);	}
	float getparameter_scalar(long index)
		{	return dfxplugin->getparameter_scalar(index);	}
	bool getparameterchanged(long index)
		{	if (dfxplugin->parameterisvalid(index)) return dfxplugin->getparameterchanged(index);   return false;	}


protected:
	DfxPlugin *dfxplugin;


public:

#if TARGET_API_AUDIOUNIT
	void Process(const Float32 *in, Float32 *out, UInt32 inNumFrames, UInt32 inNumChannels, bool &ioSilence)
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

bool createbuffer_f(float **buffer, long currentBufferSize, long desiredBufferSize);
bool createbuffer_d(double **buffer, long currentBufferSize, long desiredBufferSize);
bool createbuffer_i(long **buffer, long currentBufferSize, long desiredBufferSize);
bool createbuffer_b(bool **buffer, long currentBufferSize, long desiredBufferSize);
bool createbufferarray_f(float ***buffers, unsigned long currentNumBuffers, long currentBufferSize, 
						unsigned long desiredNumBuffers, long desiredBufferSize);
bool createbufferarrayarray_d(double ****buffers, unsigned long currentNumBufferArrays, unsigned long currentNumBuffers, 
							long currentBufferSize, unsigned long desiredNumBufferArrays, 
							unsigned long desiredNumBuffers, long desiredBufferSize);

void releasebuffer_f(float **buffer);
void releasebuffer_d(double **buffer);
void releasebuffer_i(long **buffer);
void releasebuffer_b(bool **buffer);
void releasebufferarray_f(float ***buffers, unsigned long numbuffers);
void releasebufferarrayarray_d(double ****buffers, unsigned long numbufferarrays, unsigned long numbuffers);

void clearbuffer_f(float *buffer, long buffersize, float value = 0.0f);
void clearbuffer_d(double *buffer, long buffersize, double value = 0.0);
void clearbuffer_i(long *buffer, long buffersize, long value = 0);
void clearbuffer_b(bool *buffer, long buffersize, bool value = false);
void clearbufferarray_f(float **buffers, unsigned long numbuffers, long buffersize, float value = 0.0f);
void clearbufferarrayarray_d(double ***buffers, unsigned long numbufferarrays, unsigned long numbuffers, 
							long buffersize, double value = 0.0);






//-----------------------------------------------------------------------------
// plugin entry point macros and defines and stuff

#if TARGET_API_AUDIOUNIT

	#define DFX_ENTRY(PluginClass)   COMPONENT_ENTRY(PluginClass)

	#if TARGET_PLUGIN_USES_DSPCORE
		#define DFX_CORE_ENTRY(PluginCoreClass)						\
			AUKernelBase * DfxPlugin::NewKernel()					\
			{														\
				DfxPluginCore *core = new PluginCoreClass(this);	\
				if (core != NULL)									\
					core->dfxplugincore_postconstructor();			\
				return core;										\
			}
//	#else
//		AUKernelBase * DfxPlugin::NewKernel()
//			{	return TARGET_API_BASE_CLASS::NewKernel();	}
	#endif

#endif



#if TARGET_API_VST

	#if BEOS
		#define main main_plugin
		extern "C" __declspec(dllexport) AEffect *main_plugin(audioMasterCallback audioMaster);
	#elif MACX
		#define main main_macho
		extern "C" AEffect *main_macho(audioMasterCallback audioMaster);
	#else
		AEffect *main(audioMasterCallback audioMaster);
	#endif

#define DFX_ENTRY(PluginClass)									\
	AEffect *main(audioMasterCallback audioMaster)				\
	{															\
		if ( !audioMaster(0, audioMasterVersion, 0, 0, 0, 0) )	\
			return 0;											\
		DfxPlugin *effect = new PluginClass(audioMaster);		\
		if (effect == NULL)										\
			return 0;											\
		effect->dfxplugin_postconstructor();					\
		return effect->getAeffect();							\
	}

	#if WIN32
		void *hInstance;
		BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved)
		{
			hInstance = hInst;
			return 1;
		}
	#endif

	// we need to manage the DSP cores manually in VST
	// call this in the plugin's constructor if it uses DSP cores for processing
	#if TARGET_PLUGIN_USES_DSPCORE
		// XXX is there a better way to make an empty declaration?
		#define DFX_CORE_ENTRY(PluginCoreClass)   const char acoivaXIjdASFfiXGjsASDldkfjXPVsd = 0
		#define DFX_INIT_CORE(PluginCoreClass)   								\
			for (long corecount=0; corecount < getnumoutputs(); corecount++)	\
			{																	\
				dspcores[corecount] = new PluginCoreClass(this);				\
				if (dspcores[corecount] != NULL)								\
					dspcores[corecount]->dfxplugincore_postconstructor();		\
			}
	#endif

#endif



#endif