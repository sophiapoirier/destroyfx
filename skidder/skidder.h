/*-------------- by Sophia Poirier  ][  December 2000 -------------*/

#ifndef __SKIDDER_H
#define __SKIDDER_H


#include "dfxplugin.h"


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

	kNumParameters
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
	kSkidState_SlopeIn,
	kSkidState_Plateau,
	kSkidState_SlopeOut,
	kSkidState_Valley
};

//----------------------------------------------------------------------------- 
// constants

const long kNumPresets = 16;


//----------------------------------------------------------------------------- 
class Skidder : public DfxPlugin
{
public:
	Skidder(TARGET_API_BASE_INSTANCE_TYPE inInstance);
	virtual ~Skidder();

	virtual void reset();
	virtual void processparameters();
	virtual void processaudio(const float ** inStreams, float ** outStreams, unsigned long inNumFrames, bool replacing=true);

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
	float panWidth, floor, floorRandMin, noise;
	double slopeSeconds, userTempo;
	long midiMode;
	bool tempoSync, useHostTempo, useVelocity;

	float gainRange;	// a scaled version of fFloor and the difference between that and 1.0
	float randomFloor, randomGainRange;
	// generic versions of these parameters for curved randomization
	float rateHz_gen, rateRandMinHz_gen, floor_gen, floorRandMin_gen;
	bool useRandomRate, useRandomPulsewidth, useRandomFloor;
	float sampleAmp; // output sample scalar
	long cycleSamples, pulseSamples, slopeSamples, slopeDur, plateauSamples, valleySamples;	// sample counters
	float slopeStep;	// the scalar for each step of the fade during a slope in or out
	float panGainL, panGainR;	// the actual pan gain values for each stereo channel during each cycle
	int state;	// the state of the process
	double rms;
	long rmscount;

	double currentTempoBPS;	// tempo in beats per second
	double oldTempoBPS;	// holds the previous value of currentTempoBPS for comparison
	bool needResync;	// true when playback has just started up again

	float fMidiMode;	// the MIDI note control mode parameter
	float fVelocity;	// the use-note-velocity parameter
	int mostRecentVelocity;	// the velocity of the most recently played note
	void noteOn(long delta);
	void noteOff();
	void resetMidi();
	int * noteTable;
	long waitSamples;
	bool MIDIin, MIDIout;	// set when notes start or stop so that the floor goes to 0.0
};

#endif
