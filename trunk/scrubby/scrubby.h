/*-------------- by Marc Poirier  ][  February 2002 -------------*/

#ifndef __SCRUBBY_H
#define __SCRUBBY_H


#include "dfxplugin.h"


//----------------------------------------------------------------------------- 
// these are the plugin parameters:
enum
{
	kSeekRange,
	kFreeze,

	kSeekRate_abs,
	kSeekRate_sync,
	kSeekRateRandMin_abs,
	kSeekRateRandMin_sync,
	kTempoSync,
	kSeekDur,
	kSeekDurRandMin,

	kSpeedMode,
	kSplitStereo,

	kPitchConstraint,
	kPitchStep0,
	kPitchStep1,
	kPitchStep2,
	kPitchStep3,
	kPitchStep4,
	kPitchStep5,
	kPitchStep6,
	kPitchStep7,
	kPitchStep8,
	kPitchStep9,
	kPitchStep10,
	kPitchStep11,
	kOctaveMin,
	kOctaveMax,

	kTempo,
	kTempoAuto,
	kPredelay,

	NUM_PARAMETERS
};


//----------------------------------------------------------------------------- 
// constants

const long OCTAVE_MIN = -5;
const long OCTAVE_MAX = 7;

// the number of semitones in an octave
const long NUM_PITCH_STEPS = 12;

// this is a necessary value for calculating the pitches related to the playback speeds
const double LN2TO1_12TH = log(pow(2.0, 1.0/12.0));
//const double LN2TO1_12TH = 0.05776226504666215;

#define USE_LINEAR_ACCELERATION 0

const long NUM_PRESETS = 16;

enum {
	kSpeedMode_robot,
	kSpeedMode_dj,
	kNumSpeedModes
};


/*
class ScrubbyChunk : public VstChunk
{
public:
	ScrubbyChunk(long numParameters, long numPrograms, long magic, AudioEffectX * effect);
	virtual void doLearningAssignStuff(long tag, long eventType, long eventChannel, 
										long eventNum, long delta, 
										long eventNum2 = 0, 
										long eventBehaviourFlags = 0, 
										long data1 = 0, long data2 = 0, 
										float fdata1 = 0.0f, float fdata2 = 0.0f);
	virtual void unassignParam(long tag);

	// true for unified single-point automation of both parameter range values
	bool seekRateDoubleAutomate, seekDurDoubleAutomate;
};
*/


//----------------------------------------------------------------------------- 
class Scrubby : public DfxPlugin
{
friend class ScrubbyEditor;
public:
	Scrubby(TARGET_API_BASE_INSTANCE_TYPE inInstance);
	virtual ~Scrubby();

	virtual long initialize();
	virtual void cleanup();
	virtual void reset();

	virtual void processparameters();
	virtual void processaudio(const float ** in, float ** out, unsigned long inNumFrames, bool replacing=true);

	virtual bool createbuffers();
	virtual void releasebuffers();
	virtual void clearbuffers();


private:
	void initPresets();

	void generateNewTarget(unsigned long channel);
	double processPitchConstraint(double readStep);
	void checkTempoSyncStuff();
	void processMidiNotes();

	// the parameters
	float seekRangeSeconds, seekDur, seekDurRandMin;
	float seekRateHz, seekRateSync, seekRateRandMinHz, seekRateRandMinSync;
	long seekRateIndex, seekRateRandMinIndex;
	double userTempo;
	long speedMode, octaveMin, octaveMax;
	bool freeze, splitStereo, pitchConstraint, tempoSync, useHostTempo;
	bool * pitchSteps;

	// generic versions of these parameters for curved randomization
	float seekRateHz_gen, seekRateRandMinHz_gen;

	bool useSeekRateRandMin, useSeekDurRandMin;

	// buffers and associated position values/counters/etc.
	float ** buffers;
	long writePos;
	double * readPos, * readStep, * portamentoStep;
	long * movecount, * seekcount;
	unsigned long numBuffers;	// how many buffers we have allocated at the moment

	long MAX_BUFFER;	// the maximum size (in samples) of the audio buffer
	double MAX_BUFFER_FLOAT;	// for avoiding casting

	// tempo sync stuff
	double currentTempoBPS;	// tempo in beats per second
	bool * needResync;	// true when playback has just started up again

	// MIDI note control stuff
	long * activeNotesTable;	// how many voices of each note in the octave are being played
	bool notesWereAlreadyActive;	// says whether any notes were active in the previous block

long sinecount;
float showme;
};


#endif
