/*------------------- by Marc Poirier  ][  March 2001 -------------------*/


#ifndef __bufferOverride
#define __bufferOverride

#include "vstmidi.h"
#include "dfxmisc.h"
#include "lfo.h"
#include "TempoRateTable.h"
#include "vstchunk.h"

/* begin inter-plugin audio sharing stuff */
#ifdef HUNGRY
	#include "FoodEater.h"
	const OSType magic = 'Bfud';
#endif
/* end inter-plugin audio sharing stuff */


//----------------------------------------------------------------------------- 
// these are the plugin parameters:
enum
{
	kDivisor,
	kBuffer,
	kBufferTempoSync,
	kBufferInterrupt,

	kDivisorLFOrate,
	kDivisorLFOdepth,
	kDivisorLFOshape,
	kDivisorLFOtempoSync,
	kBufferLFOrate,
	kBufferLFOdepth,
	kBufferLFOshape,
	kBufferLFOtempoSync,

	kSmooth,
	kDryWetMix,

	kPitchbend,
	kMidiMode,

	kTempo,

#ifdef HUNGRY
	kConnect,
#endif

	NUM_PARAMETERS
};

//----------------------------------------------------------------------------- 
// constants & macros

#define DIVISOR_MIN 1.92f
#define DIVISOR_MAX 222.0f
#define bufferDivisorScaled(A) ( paramRangeSquaredScaled((A), DIVISOR_MIN, DIVISOR_MAX) )
#define bufferDivisorUnscaled(A) ( paramRangeSquaredUnscaled((A), DIVISOR_MIN, DIVISOR_MAX) )

#define BUFFER_MIN 1.0f
#define BUFFER_MAX 999.0f
#define forcedBufferSizeScaled(A) ( paramRangeSquaredScaled((1.0f-(A)), BUFFER_MIN, BUFFER_MAX) )
#define forcedBufferSizeUnscaled(A) ( 1.0f - paramRangeSquaredUnscaled((A), BUFFER_MIN, BUFFER_MAX) )
#define forcedBufferSizeSamples(A) ( (long)(forcedBufferSizeScaled((A)) * SAMPLERATE * 0.001f) )

#define TEMPO_MIN 57.0f
#define TEMPO_MAX 480.0f
#define tempoScaled(A)   ( paramRangeScaled((A), TEMPO_MIN, TEMPO_MAX) )
#define tempoUnscaled(A)   ( paramRangeUnscaled((A), TEMPO_MIN, TEMPO_MAX) )

#define LFO_RATE_MIN 0.03f
#define LFO_RATE_MAX 21.0f
#define LFOrateScaled(A)   ( paramRangeSquaredScaled((A), LFO_RATE_MIN, LFO_RATE_MAX) )
#define LFOrateUnscaled(A)   ( paramRangeSquaredUnscaled((A), LFO_RATE_MIN, LFO_RATE_MAX) )

// you need this stuff to get some maximum buffer size & allocate for that
// this is 42 bpm - should be sufficient
#define MIN_ALLOWABLE_BPS 0.7f

#define NUM_PROGRAMS 16
#define PLUGIN_VERSION 2000
#define PLUGIN_ID 'bufS'


//----------------------------------------------------------------------------- 
class BufferOverrideProgram
{
friend class BufferOverride;
public:
	BufferOverrideProgram();
	~BufferOverrideProgram();
private:
	float *param;
	char *name;
};


//----------------------------------------------------------------------------- 

class BufferOverride : public AudioEffectX
{
friend class BufferOverrideEditor;
public:
	BufferOverride(audioMasterCallback audioMaster);
	~BufferOverride();

	virtual void process(float **inputs, float **outputs, long sampleFrames);
	virtual void processReplacing(float **inputs, float **outputs, long sampleFrames);

	virtual void suspend();
	virtual void resume();

	virtual long processEvents(VstEvents* events);
	virtual long getTailSize();
	// there was a typo in the VST header files versions 2.0 through 2.2, 
	// so some hosts will still call this incorrectly named version...
	virtual long getGetTailSize() { return getTailSize(); }
	virtual bool getInputProperties(long index, VstPinProperties* properties);
	virtual bool getOutputProperties(long index, VstPinProperties* properties);

	virtual long getChunk(void **data, bool isPreset);
	virtual long setChunk(void *data, long byteSize, bool isPreset);

	virtual void setProgram(long programNum);
	virtual void setProgramName(char *name);
	virtual void getProgramName(char *name);
	virtual bool getProgramNameIndexed(long category, long index, char *text);
	virtual bool copyProgram(long destination);

	virtual void setParameter(long index, float value);
	virtual float getParameter(long index);
	virtual void getParameterName(long index, char *text);
	virtual void getParameterDisplay(long index, char *text);
	virtual void getParameterLabel(long index, char *label);

	virtual bool getEffectName(char *name);
	virtual long getVendorVersion();
	virtual bool getErrorText(char *text);
	virtual bool getVendorString(char *text);
	virtual bool getProductString(char *text);

	virtual long canDo(char* text);


protected:
	void doTheProcess(float **inputs, float **outputs, long sampleFrames, bool replacing);
	void updateBuffer(long samplePos);

	void heedBufferOverrideEvents(long samplePos);
	float getDivisorParameterFromNote(int currentNote);
	float getDivisorParameterFromPitchbend(int pitchbendByte);

	void initPresets();
	void createAudioBuffers();

	// the parameters
	float fDivisor, fBuffer, fBufferTempoSync, fBufferInterrupt, fSmooth, fDryWetMix, fPitchbend, fMidiMode, fTempo;

	BufferOverrideProgram *programs;	// presets / program slots
	VstChunk *chunk;	// chunky data

	long currentForcedBufferSize;	// the size of the larger, imposed buffer
	// these store the forced buffer
	float *buffer1;
#ifdef BUFFEROVERRIDE_STEREO
	float *buffer2;
#endif
	long writePos;	// the current sample position within the forced buffer

	long minibufferSize;	// the current size of the divided "mini" buffer
	long prevMinibufferSize;	// the previous size
	long readPos;	// the current sample position within the minibuffer
	float currentBufferDivisor;	// the current value of the divisor with LFO possibly applied

	float numLFOpointsDivSR;	// the number of LFO table points divided by the sampling rate

	VstTimeInfo *timeInfo;
	float currentTempoBPS;	// tempo in beats per second
	TempoRateTable *tempoRateTable;	// a table of tempo rate values
	long hostCanDoTempo;	// my semi-booly dude who knows something about the host's VstTimeInfo implementation
	bool needResync;

	long SUPER_MAX_BUFFER;
	float SAMPLERATE;

	long smoothDur, smoothcount;	// total duration & sample counter for the minibuffer transition smoothing period
	float smoothStep;	// the gain increment for each sample "step" during the smoothing period
	float sqrtFadeIn, sqrtFadeOut;	// square root of the smoothing gains, for equal power crossfading
	float smoothFract;

	double pitchbend, oldPitchbend;	// pitchbending scalar values
	VstMidi *midistuff;	// all of the MIDI everythings
	bool oldNote;	// says if there was an old, unnatended note-on or note-off from a previous block
	int lastNoteOn, lastPitchbend;	// these carry over the last events from a previous processing block
	bool divisorWasChangedByHand;	// for MIDI trigger mode - tells us to respect the fDivisor value
	bool divisorWasChangedByMIDI;	// tells the GUI that the divisor displays need updating

	LFO *divisorLFO, *bufferLFO;

#ifdef HUNGRY
	FoodEater *foodEater;
#endif

	float fadeOutGain, fadeInGain, realFadePart, imaginaryFadePart;	// for trig crossfading
};

#endif
