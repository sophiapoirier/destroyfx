/*-------------- by Marc Poirier  ][  February 2002 -------------*/

#ifndef __scrubby
#define __scrubby

#include <stdio.h>

#include "dfxmisc.h"
#include "vstmidi.h"
#include "vstchunk.h"
#include "temporatetable.h"


//----------------------------------------------------------------------------- 
// these are the plugin parameters:
enum
{
	kSeekRange,
	kFreeze,

	kSeekRate,
	kSeekRateRandMin,
	kTempoSync,
	kSeekDur,
	kSeekDurRandMin,

	kPortamento,
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
	kPredelay,

	NUM_PARAMETERS
};


//----------------------------------------------------------------------------- 
// constants & macros

#define SEEK_RANGE_MIN 0.3f
#define SEEK_RANGE_MAX 6000.0f
#define seekRangeScaled(A)   (paramRangeSquaredScaled((A), SEEK_RANGE_MIN, SEEK_RANGE_MAX))
#define seekRangeUnscaled(A)   (paramRangeSquaredUnscaled((A), SEEK_RANGE_MIN, SEEK_RANGE_MAX))

#define SEEK_RATE_MIN 0.3f
#define SEEK_RATE_MAX 810.0f
#define seekRateScaled(A)   (paramRangeCubedScaled((A), SEEK_RATE_MIN, SEEK_RATE_MAX))
#define seekRateUnscaled(A)   (paramRangeCubedUnscaled((A), SEEK_RATE_MIN, SEEK_RATE_MAX))

#define SEEK_DUR_MIN 0.03f
#define SEEK_DUR_MAX 1.0f
#define seekDurScaled(A)   (paramRangeScaled((A), SEEK_DUR_MIN, SEEK_DUR_MAX))
#define seekDurUnscaled(A)   (paramRangeUnscaled((A), SEEK_DUR_MIN, SEEK_DUR_MAX))

#define OCTAVE_MIN (-5)
#define OCTAVE_MAX 7
#define octaveMinScaled(f)   paramRangeIntScaled(f, OCTAVE_MIN, 0)
#define octaveMaxScaled(f)   paramRangeIntScaled(f, 0, OCTAVE_MAX)
#define octaveMinUnscaled(i)   paramRangeIntUnscaled(i, OCTAVE_MIN, 0)
#define octaveMaxUnscaled(i)   paramRangeIntUnscaled(i, 0, OCTAVE_MAX)

#define TEMPO_MIN 39.0f
#define TEMPO_MAX 480.0f
#define tempoScaled(A)   paramRangeScaled((A), TEMPO_MIN, TEMPO_MAX)
#define tempoUnscaled(A)   paramRangeUnscaled((A), TEMPO_MIN, TEMPO_MAX)

// the number of semitones in an octave
#define NUM_PITCH_STEPS 12

// this is a necessary value for calculating the pitches related to the playback speeds
const double LN2TO1_12TH = log(pow(2.0, 1.0/12.0));	// why doesn't this work?
//#define LN2TO1_12TH   0.05776226504666215

//#define USE_LINEAR_ACCELERATION

#define NUM_PROGRAMS 16
#define PLUGIN_VERSION 1000
#define PLUGIN_ID 'scub'

//----------------------------------------------------------------------------- 
class ScrubbyProgram
{
friend class Scrubby;
public:
	ScrubbyProgram();
	~ScrubbyProgram();
private:
	float *param;
	char *name;
};


//----------------------------------------------------------------------------- 
class ScrubbyChunk : public VstChunk
{
public:
	ScrubbyChunk(long numParameters, long numPrograms, long magic, AudioEffectX *effect);
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


//----------------------------------------------------------------------------- 

class Scrubby : public AudioEffectX
{
friend class ScrubbyEditor;
public:
	Scrubby(audioMasterCallback audioMaster);
	~Scrubby();

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

	virtual void setProgram(long programNum);
	virtual void setProgramName(char *name);
	virtual void getProgramName(char *name);
	virtual bool getProgramNameIndexed(long category, long index, char *text);
	virtual bool copyProgram(long destination);

	virtual long setChunk(void *data, long byteSize, bool isPreset);
	virtual long getChunk(void **data, bool isPreset);

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
	friend class ScrubbyEditor;

	void doTheProcess(float **inputs, float **outputs, long sampleFrames, bool replacing);
	void generateNewTarget(int channel);
	double processPitchConstraint(double readStep);
	void checkTempoSyncStuff();
	void getRealValues();
	void initPresets();
	void createAudioBuffers();

	// the parameters
	float fSeekRange, fFreeze, fSeekRate, fSeekRateRandMin, fTempoSync;
	float fSeekDur, fSeekDurRandMin, fPortamento, fSplitStereo, fTempo, fPredelay;
	float fPitchConstraint, *fPitchSteps, fOctaveMin, fOctaveMax;
	// some useful converted parameter booleans
	bool useSeekRateRandMin, useSeekDurRandMin, freeze;
	bool portamento, splitStereo, tempoSync, pitchConstraint;
	bool *pitchSteps;

	ScrubbyProgram *programs;
	ScrubbyChunk *chunk;

	// buffers & associated position values/counters/etc.
	float *buffer1, *buffer2;
	long writePos;
	double readPos1, readPos2, readStep1, readStep2, portamentoStep1, portamentoStep2;
	long movecount1, movecount2, seekcount1, seekcount2;

	float SAMPLERATE;
	long MAX_BUFFER;	// the maximum size (in samples) of the audio buffer
	double MAX_BUFFER_FLOAT;	// for avoiding casting

	// tempo sync stuff
	TempoRateTable *tempoRateTable;	// a table of tempo rate values
	VstTimeInfo *timeInfo;
	float currentTempoBPS;	// tempo in beats per second
	long hostCanDoTempo;	// my semi-booly dude who knows something about the host's VstTimeInfo implementation
	bool needResync1, needResync2;	// true when playback has just started up again

	// MIDI note control stuff
	VstMidi *midistuff;	// all of the MIDI everythings
	long *activeNotesTable;	// how many voices of each note in the octave are being played
	bool keyboardWasPlayedByMidi;	// tells the GUI to update the keyboard display
	bool notesWereAlreadyActive;	// says whether any notes were active in the previous block

FILE *f;
long sinecount;
float showme;
};


#endif
