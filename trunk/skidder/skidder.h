/*---------------------------------------------------------------

   © 2000, Marcberg Soft und Hardware GmbH, All Rights Reserved

---------------------------------------------------------------*/

#ifndef __skidder
#define __skidder

#include <math.h>
#include <stdlib.h>

#include "dfxmisc.h"
#include "vstchunk.h"
#include "temporatetable.h"

#ifdef MSKIDDER
/* begin inter-plugin audio sharing stuff */
#ifdef HUNGRY
	#include "FoodEater.h"
	const OSType magic = 'Mfud';
#endif
/* end inter-plugin audio sharing stuff */
#endif


//----------------------------------------------------------------------------- 
// these are the plugin parameters:
enum
{
	kRate,
	kTempoSync,
	kTempoRate,
	kRateRandFactor,
	kTempo,
	kPulsewidth,
	kPulsewidthRandMin,
	kSlope,
	kPan,
	kNoise,

#ifdef MSKIDDER
	kMidiMode,
	kVelocity,
#endif

	kFloor,
	kFloorRandMin,

#ifdef MSKIDDER
#ifdef HUNGRY
	kConnect,
#endif
#endif

	NUM_PARAMETERS
};

//----------------------------------------------------------------------------- 
// these are the 3 MIDI note control modes:
enum
{
	kNoMidiMode,
	kMidiTrigger,
	kMidiApply,
	NUM_MIDI_MODES
};

//----------------------------------------------------------------------------- 
// these are the 4 states of the process:
enum
{
	slopeIn,
	plateau,
	slopeOut,
	valley
};

//----------------------------------------------------------------------------- 
// constants & macros

#define RATEMIN 0.3f
#define RATEMAX 21.0f
// this is for converting from parameter entries to the real values
#define rateScaled(A)   ( paramRangePowScaled((A), RATEMIN, RATEMAX, 1.65f) )
#define rateUnscaled(A)   ( paramRangePowUnscaled((A), RATEMIN, RATEMAX, 1.65f) )

#define RATE_RAND_FACTOR_MIN 1.0f
#define RATE_RAND_FACTOR_MAX 9.0f
#define rateRandFactorScaled(A)   ( paramRangeSquaredScaled((A), RATE_RAND_FACTOR_MIN, RATE_RAND_FACTOR_MAX) )
#define rateRandFactorUnscaled(A)   ( paramRangeSquaredUnscaled((A), RATE_RAND_FACTOR_MIN, RATE_RAND_FACTOR_MAX) )

#define PULSEMIN 0.001f
#define PULSEMAX 0.999f
#define pulsewidthScaled(A)   ( paramRangeScaled((A), PULSEMIN, PULSEMAX) )
#define pulsewidthUnscaled(A)   ( paramRangeUnscaled((A), PULSEMIN, PULSEMAX) )

#define TEMPO_MIN 39.0f
#define TEMPO_MAX 480.0f
#define tempoScaled(A)   ( paramRangeScaled((A), TEMPO_MIN, TEMPO_MAX) )
#define tempoUnscaled(A)   ( paramRangeUnscaled((A), TEMPO_MIN, TEMPO_MAX) )

#define SLOPEMAX 15.0f

// this is for scaling the slider values of rupture (more emphasis on lower values, dB-style)
#define fNoise_squared (fNoise*fNoise)

#define gainScaled(A) ((A)*(A)*(A))

#ifdef MSKIDDER
#define midiModeScaled(A)   ( paramSteppedScaled((A), NUM_MIDI_MODES) )
#define midiModeUnscaled(A)   ( paramSteppedUnscaled((A), NUM_MIDI_MODES) )
// 128 midi notes
#define NUM_NOTES 128
#endif

#define NUM_PROGRAMS 16
#define PLUGIN_VERSION 1400
#define PLUGIN_ID 'skid'


//----------------------------------------------------------------------------- 
class SkidderProgram
{
friend class Skidder;
public:
	SkidderProgram();
	~SkidderProgram();
private:
	float *param;
	char *name;
};


#ifdef MSKIDDER
//----------------------------------------------------------------------------- 
class SkidderChunk : public VstChunk
{
public:
	SkidderChunk(long numParameters, long numPrograms, long magic, AudioEffectX *effect);
	virtual void doLearningAssignStuff(long tag, long eventType, long eventChannel, 
										long eventNum, long delta, 
										long eventNum2 = 0, 
										long eventBehaviourFlags = 0, 
										long data1 = 0, long data2 = 0, 
										float fdata1 = 0.0f, float fdata2 = 0.0f);
	virtual void unassignParam(long tag);

	// true for unified single-point automation of both parameter range values
	bool pulsewidthDoubleAutomate, floorDoubleAutomate;
};
#endif


//----------------------------------------------------------------------------- 

class Skidder : public AudioEffectX
{
friend class SkidderEditor;
public:
	Skidder(audioMasterCallback audioMaster);
	~Skidder();

	virtual void process(float **inputs, float **outputs, long sampleFrames);
	virtual void processReplacing(float **inputs, float **outputs, long sampleFrames);

	virtual void suspend();
	virtual void resume();
#ifdef MSKIDDER
	virtual long getChunk(void **data, bool isPreset);
	virtual long setChunk(void *data, long byteSize, bool isPreset);
	virtual long processEvents(VstEvents* events);
#endif

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

	virtual bool getInputProperties(long index, VstPinProperties* properties);
	virtual bool getOutputProperties(long index, VstPinProperties* properties);
	virtual long canDo(char* text);


protected:
	void processSlopeIn();
	void processPlateau();
	void processSlopeOut();
	void processValley(float SAMPLERATE);
	float processOutput(float in1, float in2, float pan);
	void doTheProcess(float **inputs, float **outputs, long sampleFrames, bool replacing);

	// the parameters
	float fRate, fTempoSync, fTempoRate, fRateRandFactor, fTempo;
	float fPulsewidth, fPulsewidthRandMin, fSlope, fPan, fNoise, fFloor, fFloorRandMin;
	float floor, gainRange;	// a scaled version of fFloor & the difference between that & 1.0
	float randomFloor, randomGainRange;
	bool useRandomFloor;
	SkidderProgram *programs;
	float amp; // output sample scalar
	long cycleSamples, pulseSamples, slopeSamples, slopeDur, plateauSamples, valleySamples;	// sample counters
	float slopeStep;	// the scalar for each step of the fade during a slope in or out
	float panRander;	// the actual pan value for each cycle
	int state;	// the state of the process
	float rms;
	long rmscount;

	float currentTempoBPS;	// tempo in beats per second
	float oldTempoBPS;	// holds the previous value of currentTempoBPS for comparison
	bool tempoHasChanged;	// tells the GUI that the rate random range display needs updating
	// this allows tempoHasChanged to be updated after the tempo gets recalulated 
	// after switching to auto tempo, all of which is for the sake of the GUI
	bool mustUpdateTempoHasChanged;
	TempoRateTable *tempoRateTable;	// a table of tempo rate values
	long hostCanDoTempo;	// my semi-booly dude who knows something about the host's VstTimeInfo implementation
	VstTimeInfo *timeInfo;
	bool needResync;	// true when playback has just started up again

#ifdef MSKIDDER
	float fMidiMode;	// the MIDI note control mode parameter
	float fVelocity;	// the use-note-velocity parameter
	int mostRecentVelocity;	// the velocity of the most recently played note
	void noteOn(long delta);
	void noteOff();
	void resetMidi();
	int *noteTable;
	long waitSamples;
	bool MIDIin, MIDIout;	// set when notes start or stop so that the floor goes to 0.0
	SkidderChunk *chunk;
#ifdef HUNGRY
	FoodEater *foodEater;
#endif
#endif	// end of mSkidder-specific stuff
};

#endif
