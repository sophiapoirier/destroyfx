/*------------------------------------------------------------------------
Destroy FX is a sovereign entity comprised of Marc Poirier & Tom Murphy 7.  
This is our shit.
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
	4-byte ID for the plugin (will base it off PLUGIN_ID if not defined)
PLUGIN_EDITOR_RES_ID
	component resource ID of the base plugin
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
	virtual bool createbuffers()
		{	return true;	}
	// ***
	virtual void clearbuffers()
		{ }
	// ***
	virtual void releasebuffers()
		{ }

	// ***
	void do_processparameters();
	virtual void processparameters()
		{ }
	void preprocessaudio();
	void postprocessaudio();
	// ***
	virtual void processaudio(const float **in, float **out, unsigned long inNumFrames, 
						bool replacing=true)
		{ }

	bool parameterisvalid(long parameterIndex)
		{	return ( (parameterIndex >= 0) && (parameterIndex < numParameters) && (parameters != NULL) );	}

	void initparameter_f(long parameterIndex, const char *initName, float initValue, float initDefaultValue, 
						float initMin, float initMax, 
						DfxParamCurve initCurve = kDfxParamCurve_linear, 
						DfxParamUnit initUnit = kDfxParamUnit_undefined);
	void initparameter_d(long parameterIndex, const char *initName, double initValue, double initDefaultValue, 
						double initMin, double initMax, 
						DfxParamCurve initCurve = kDfxParamCurve_linear, 
						DfxParamUnit initUnit = kDfxParamUnit_undefined);
	void initparameter_i(long parameterIndex, const char *initName, long initValue, long initDefaultValue, 
						long initMin, long initMax, 
						DfxParamCurve initCurve = kDfxParamCurve_linear, 
						DfxParamUnit initUnit = kDfxParamUnit_undefined);
	void initparameter_ui(long parameterIndex, const char *initName, unsigned long initValue, unsigned long initDefaultValue, 
						unsigned long initMin, unsigned long initMax, 
						DfxParamCurve initCurve = kDfxParamCurve_linear, 
						DfxParamUnit initUnit = kDfxParamUnit_undefined);
	void initparameter_b(long parameterIndex, const char *initName, bool initValue, bool initDefaultValue, 
						DfxParamCurve initCurve = kDfxParamCurve_linear, 
						DfxParamUnit initUnit = kDfxParamUnit_undefined);
	void initparameter_indexed(long parameterIndex, const char *initName, long initValue, long initDefaultValue, long initNumItems);

	bool setparametervaluestring(long parameterIndex, long stringIndex, const char *inText)
	{
		if (parameterisvalid(parameterIndex))
			return parameters[parameterIndex].setvaluestring(stringIndex, inText);
		else return false;
	}
	bool getparametervaluestring(long parameterIndex, long stringIndex, char *outText)
	{
		if (parameterisvalid(parameterIndex))
			return parameters[parameterIndex].getvaluestring(stringIndex, outText);
		else return false;
	}
	char * getparametervaluestring_ptr(long parameterIndex, long stringIndex)
	{	if (parameterisvalid(parameterIndex))
			return parameters[parameterIndex].getvaluestring_ptr(stringIndex);
		else return 0;
	}
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

	bool presetisvalid(long presetIndex);
	bool presetnameisvalid(long presetIndex);
	virtual bool loadpreset(long presetIndex);
	void initpresetsparameter(long parameterIndex);
	void setpresetname(long presetIndex, const char *inText);
	void getpresetname(long presetIndex, char *outText);
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
	void update_preset(long presetIndex);
	DfxParamValue getpresetparameter(long presetIndex, long parameterIndex);
	float getpresetparameter_f(long presetIndex, long parameterIndex);


	double getsamplerate()
		{	return DfxPlugin::samplerate;	}
	void setsamplerate(double newrate);
	// force a refetching of the samplerate from the host
	void updatesamplerate();

	unsigned long getnuminputs();
	unsigned long getnumoutputs();

	long getnumparameters()
		{	return numParameters;	}
	long getnumpresets()
		{	return numPresets;	}

//	virtual void SetUseMusicalTimeInfo(bool newmode = true)
//		{	b_usemusicaltimeinfo = newmode;	}
//	bool GetUseMusicalTimeInfo()
//		{	return b_usemusicaltimeinfo;	}

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
		DfxSettings * getsettings_ptr()
			{	return dfxsettings;	}
		// handlers for the types of MIDI events that we support
		void handlemidi_noteon(int channel, int note, int velocity, long frameOffset);
		void handlemidi_noteoff(int channel, int note, int velocity, long frameOffset);
		void handlemidi_allnotesoff(int channel, long frameOffset);
		void handlemidi_pitchbend(int channel, int valueLSB, int valueMSB, long frameOffset);
		void handlemidi_cc(int channel, int controllerNum, int value, long frameOffset);
		void handlemidi_programchange(int channel, int programNum, long frameOffset);
	#endif


protected:
	DfxParam *parameters;
	DfxPreset *presets;

	DfxChannelConfig *channelconfigs;
	long numchannelconfigs;

	DfxTimeInfo timeinfo;
	bool b_usemusicaltimeinfo;	// XXX use this?
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
		float **inputsP, **outputsP;
	#endif

	#if TARGET_API_VST
		bool latencychanged;
	#endif

private:
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
	virtual ComponentResult Initialize();
	virtual void Cleanup();
	virtual void Reset();

	virtual OSStatus ProcessBufferLists(AudioUnitRenderActionFlags &ioActionFlags, 
					const AudioBufferList &inBuffer, AudioBufferList &outBuffer, 
					UInt32 inFramesToProcess);
	virtual AUKernelBase * NewKernel();

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
	AUPreset *aupresets;	// preset data needs to survive throughout the Audio Unit's life

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
// Audio Unit must override NewKernel() to implmement this
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






#if TARGET_API_AUDIOUNIT

	#define DFX_ENTRY(PluginClass)   COMPONENT_ENTRY(PluginClass)

	#if TARGET_PLUGIN_USES_DSPCORE
		#define DFX_CORE_ENTRY(PluginCoreClass)	\
			AUKernelBase * DfxPlugin::NewKernel()		\
				{	return new PluginCoreClass(this);	}
	#else
		AUKernelBase * DfxPlugin::NewKernel()
			{	return TARGET_API_BASE_CLASS::NewKernel();	}
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
		AudioEffectX *effect = new PluginClass(audioMaster);	\
		if (!effect)											\
			return 0;											\
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
		// XXX what else could be done aside from void?
		#define DFX_CORE_ENTRY(PluginCoreClass)   void
		#define DFX_INIT_CORE(TransverbDSP)   for (long i=0; i < getnumoutputs(); i++)   dspcores[i] = new TransverbDSP(this);
	#endif

#endif



#ifdef WIN32
/* turn off warnings about default but no cases in switch, unknown pragma, etc. */
   #pragma warning( disable : 4065 57 4200 4244 4068 )
   #include <windows.h>
#endif



#endif