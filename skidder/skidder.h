/*---------------------------------------------------------------
   © 2000, Marcberg Soft und Hardware GmbH, All Rights Reserved
---------------------------------------------------------------*/

#ifndef __SKIDDER_H
#define __SKIDDER_H


#ifndef __DFXPLUGIN_H
#include "dfxplugin.h"
#endif


//----------------------------------------------------------------------------- 
// these are the plugin parameters:
enum
{
	kRate_abs,
	kRate_sync,
	kRateRandMin_abs,
	kRateRandMin_sync,
	kTempoSync,
	kPulsewidth,
	kPulsewidthRandMin,
	kSlope,
	kPan,
	kNoise,
	kMidiMode,
	kVelocity,
	kFloor,
	kFloorRandMin,
	kTempo,
	kTempoAuto,

	NUM_PARAMETERS
};

//----------------------------------------------------------------------------- 
// these are the 3 MIDI note control modes:
enum
{
	kMidiMode_none,
	kMidiMode_trigger,
	kMidiMode_apply,
	kNumMidiModes
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

#define midiModeScaled(A)   ( paramSteppedScaled((A), kNumMidiModes) )
#define midiModeUnscaled(A)   ( paramSteppedUnscaled((A), kNumMidiModes) )

#define NUM_PRESETS 16


//----------------------------------------------------------------------------- 
class Skidder : public DfxPlugin
{
friend class SkidderEditor;
public:
	Skidder(TARGET_API_BASE_INSTANCE_TYPE inInstance);
	virtual ~Skidder();

	virtual void reset();
	virtual void processparameters();
	virtual void processaudio(const float **in, float **out, unsigned long inNumFrames, bool replacing=true);

	// stuff for extending DfxSettings
	virtual void settings_doLearningAssignStuff(long tag, long eventType, long eventChannel, 
										long eventNum, long delta, long eventNum2 = 0, 
										long eventBehaviourFlags = 0, long data1 = 0, 
										long data2 = 0, float fdata1 = 0.0f, float fdata2 = 0.0f);
//	virtual void settings_unassignParam(long tag);
	// true for unified single-point automation of both parameter range values
	bool rateDoubleAutomate, pulsewidthDoubleAutomate, floorDoubleAutomate;


private:
	void processSlopeIn();
	void processPlateau();
	void processSlopeOut();
	void processValley();
	float processOutput(float in1, float in2, float panGain);
	void processMidiNotes();

	// the parameters
	float rateHz, rateSync, rateRandMinHz, rateRandMinSync, pulsewidth, pulsewidthRandMin;
	long rateIndex, rateRandMinIndex;
	float panWidth, floor, floorRandMin, noise, userTempo;
	double slopeSeconds;
	long midiMode;
	bool tempoSync, useHostTempo, useVelocity;

	float gainRange;	// a scaled version of fFloor & the difference between that & 1.0
	float randomFloor, randomGainRange;
	// generic versions of these parameters for curved randomization
	float rateHz_gen, rateRandMinHz_gen, floor_gen, floorRandMin_gen;
	bool useRandomRate, useRandomPulsewidth, useRandomFloor;
	float sampleAmp; // output sample scalar
	long cycleSamples, pulseSamples, slopeSamples, slopeDur, plateauSamples, valleySamples;	// sample counters
	float slopeStep;	// the scalar for each step of the fade during a slope in or out
	float panGainL, panGainR;	// the actual pan gain values for each stereo channel during each cycle
	int state;	// the state of the process
	float rms;
	long rmscount;

	float currentTempoBPS;	// tempo in beats per second
	float oldTempoBPS;	// holds the previous value of currentTempoBPS for comparison
	bool needResync;	// true when playback has just started up again

	float fMidiMode;	// the MIDI note control mode parameter
	float fVelocity;	// the use-note-velocity parameter
	int mostRecentVelocity;	// the velocity of the most recently played note
	void noteOn(long delta);
	void noteOff();
	void resetMidi();
	int *noteTable;
	long waitSamples;
	bool MIDIin, MIDIout;	// set when notes start or stop so that the floor goes to 0.0
};

#endif
