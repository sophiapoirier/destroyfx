/*------------------- by Marc Poirier  ][  March 2001 -------------------*/


#ifndef __BUFFEROVERRIDE_H
#define __BUFFEROVERRIDE_H


#ifndef __DFXPLUGIN_H
#include "dfxplugin.h"
#endif

#include "lfo.h"


//----------------------------------------------------------------------------- 
// these are the plugin parameters:
enum
{
	kDivisor,
	kBufferSize_abs,
	kBufferSize_sync,
	kBufferTempoSync,
	kBufferInterrupt,

	kDivisorLFOrate_abs,
	kDivisorLFOrate_sync,
	kDivisorLFOdepth,
	kDivisorLFOshape,
	kDivisorLFOtempoSync,
	kBufferLFOrate_abs,
	kBufferLFOrate_sync,
	kBufferLFOdepth,
	kBufferLFOshape,
	kBufferLFOtempoSync,

	kSmooth,
	kDryWetMix,

	kPitchbend,
	kMidiMode,

	kTempo,
	kTempoAuto,

	NUM_PARAMETERS
};

//----------------------------------------------------------------------------- 
// constants & macros

#define NUM_PRESETS 16

#define DIVISOR_MIN 1.92f
#define DIVISOR_MAX 222.0f
#define bufferDivisorScaled(A) ( paramRangeSquaredScaled((A), DIVISOR_MIN, DIVISOR_MAX) )
#define bufferDivisorUnscaled(A) ( paramRangeSquaredUnscaled((A), DIVISOR_MIN, DIVISOR_MAX) )

#define BUFFER_MIN 1.0f
#define BUFFER_MAX 999.0f
#define forcedBufferSizeScaled(A) ( paramRangeSquaredScaled((1.0f-(A)), BUFFER_MIN, BUFFER_MAX) )
#define forcedBufferSizeUnscaled(A) ( 1.0f - paramRangeSquaredUnscaled((A), BUFFER_MIN, BUFFER_MAX) )
#define bufferSize_ms2samples(fMS) ( (long) ((fMS) * getsamplerate_f() * 0.001f) )

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
#define MIN_ALLOWABLE_BPS 0.7

enum {
	kMidiMode_nudge,
	kMidiMode_trigger,
	kNumMidiModes
};


//----------------------------------------------------------------------------- 
class BufferOverride : public DfxPlugin
{
friend class BufferOverrideEditor;
public:
	BufferOverride(TARGET_API_BASE_INSTANCE_TYPE inInstance);
	virtual ~BufferOverride();

	virtual long initialize();
	virtual void cleanup();
	virtual void reset();

	virtual void processaudio(const float **in, float **out, unsigned long inNumFrames, bool replacing=true);
	virtual void processparameters();

	virtual bool createbuffers();
	virtual void releasebuffers();


private:
	void updateBuffer(unsigned long samplePos);

	void heedBufferOverrideEvents(unsigned long samplePos);
	float getDivisorParameterFromNote(int currentNote);
	float getDivisorParameterFromPitchbend(int pitchbendByte);

	void initPresets();

	// the parameters
	float divisor, bufferSizeMs, bufferSizeSync;
	bool bufferTempoSync, bufferInterrupt, useHostTempo;
	float smooth, dryWetMix, userTempo;
	double pitchbendRange;
	long midiMode;

	long currentForcedBufferSize;	// the size of the larger, imposed buffer
	float **buffers;	// this stores the forced buffer
	float *outval;	// array of current audio output values (1 for each channel)
	unsigned long numBuffers;	// how many buffers we have allocated at the moment
	long writePos;	// the current sample position within the forced buffer

	long minibufferSize;	// the current size of the divided "mini" buffer
	long prevMinibufferSize;	// the previous size
	long readPos;	// the current sample position within the minibuffer
	float currentBufferDivisor;	// the current value of the divisor with LFO possibly applied

	float numLFOpointsDivSR;	// the number of LFO table points divided by the sampling rate

	float currentTempoBPS;	// tempo in beats per second
	bool needResync;

	long SUPER_MAX_BUFFER;

	long smoothDur, smoothcount;	// total duration & sample counter for the minibuffer transition smoothing period
	float smoothStep;	// the gain increment for each sample "step" during the smoothing period
	float sqrtFadeIn, sqrtFadeOut;	// square root of the smoothing gains, for equal power crossfading
	float smoothFract;

	double pitchbend, oldPitchbend;	// pitchbending scalar values
	bool oldNote;	// says if there was an old, unnatended note-on or note-off from a previous block
	int lastNoteOn, lastPitchbend;	// these carry over the last events from a previous processing block
	bool divisorWasChangedByHand;	// for MIDI trigger mode - tells us to respect the fDivisor value
	bool divisorWasChangedByMIDI;	// tells the GUI that the divisor displays need updating

	LFO *divisorLFO, *bufferLFO;

	float fadeOutGain, fadeInGain, realFadePart, imaginaryFadePart;	// for trig crossfading
};

#endif
