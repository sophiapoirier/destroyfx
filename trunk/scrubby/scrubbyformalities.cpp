/*-------------- by Marc Poirier  ][  February 2002 -------------*/

#ifndef __scrubby
#include "scrubby.hpp"
#endif

#ifndef __scrubbyeditor
#include "scrubbyeditor.hpp"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>


#pragma mark _________init_________

//-----------------------------------------------------------------------------
// initializations & such

Scrubby::Scrubby(audioMasterCallback audioMaster)
	: AudioEffectX(audioMaster, NUM_PROGRAMS, NUM_PARAMETERS)	// 16 programs, 26 parameters
{
	buffer1 = NULL;
	buffer2 = NULL;
	// default this to something, for the sake of getTailSize()
	MAX_BUFFER = (long) (SEEK_RANGE_MAX * 0.001f * 44100.0f / SEEK_RATE_MIN);

	// allocate memory for these arrays
	fPitchSteps = new float[NUM_PITCH_STEPS];
	pitchSteps = new bool[NUM_PITCH_STEPS];
	activeNotesTable = new long[NUM_PITCH_STEPS];
	for (int i=0; i < NUM_PITCH_STEPS; i++)
		activeNotesTable[i] = 0;
	midistuff = new VstMidi;
	tempoRateTable = new TempoRateTable;
	programs = new ScrubbyProgram[NUM_PROGRAMS];
	setProgram(0);
	strcpy(programs[0].name, "scub");	// default program name
	initPresets();
	chunk = new ScrubbyChunk(NUM_PARAMETERS, NUM_PROGRAMS, PLUGIN_ID, this);
	// since we don't use pitchbend for anything special, 
	// allow it be assigned to control parameters
	chunk->setAllowPitchbendEvents(true);

	setUniqueID(PLUGIN_ID);	// identify Scrubby
	setNumInputs(2);	// stereo in
	setNumOutputs(2);	// stereo out
	canMono();
	canProcessReplacing();	// supports both accumulating and replacing output

	// check to see if the host supports sending tempo & time information to VST plugins
	hostCanDoTempo = canHostDo("sendVstTimeInfo");
	// if not, then default the tempo to something more reasonable than 39 bmp
	// also give currentTempoBPS a value in case that's useful for a freshly opened GUI
	if (hostCanDoTempo == 1)
		currentTempoBPS = (float)tempoAt(0) / 600000.0f;
	if ( (hostCanDoTempo != 1) || (currentTempoBPS <= 0.0f) )
	{
		setParameter( kTempo, tempoUnscaled(120.0f) );
		currentTempoBPS = tempoScaled(fTempo) / 60.0f;
	}

	srand((unsigned int)time(NULL));	// sets a seed value for rand() from the system clock

	predelayChanged = false;

	suspend();

	editor = new ScrubbyEditor(this);

/*
#if CALL_NOT_IN_CARBON
f = fopen("murky:Desktop Folder:scrubby shiz", "w");
#else
f = fopen("coriander:Users:marc:Desktop:scrubby shiz", "w");
#endif
*/
}

//-----------------------------------------------------------------------------------------
Scrubby::~Scrubby()
{
//fclose(f);
	// deallocate the memory from these arrays
	if (programs)   delete[] programs;
	if (fPitchSteps)   delete[] fPitchSteps;
	if (pitchSteps)   delete[] pitchSteps;
	if (activeNotesTable)   delete[] activeNotesTable;
	if (buffer1)   delete[] buffer1;
	if (buffer2)   delete[] buffer2;
	if (chunk)   delete chunk;
	if (midistuff)   delete midistuff;
	if (tempoRateTable)   delete tempoRateTable;
}

//-----------------------------------------------------------------------------------------
// this gets called when the plugin is de-activated

void Scrubby::suspend()
{
	// reset these position trackers thingies & whatnot
	writePos = 0;
	readPos1 = readPos2 = 0.001;
	seekcount1 = seekcount2 = movecount1 = movecount2 = 0;
	readStep1 = readStep2 = 1.0;
#ifdef USE_LINEAR_ACCELERATION
	portamentoStep1 = portamentoStep2 = 0.0;
#else
	portamentoStep1 = portamentoStep2 = 1.0;
#endif

	midistuff->reset();
	keyboardWasPlayedByMidi = true;
	// delete any stored active notes
	for (int i=0; i < NUM_PITCH_STEPS; i++)
	{
		if (activeNotesTable[i] > 0)
			fPitchSteps[i] = 0.0f;
		activeNotesTable[i] = 0;
	}

sinecount = 0;
}

//-----------------------------------------------------------------------------
// this gets called when the plugin is activated
void Scrubby::resume()
{
	needResync1 = needResync2 = true;	// some hosts may call resume when restarting playback
	wantEvents();	// for MIDI program changes

	createAudioBuffers();

	// tell the host what the length of delay compensation should be, in samples
	setInitialDelay((long) (seekRangeScaled(fSeekRange) * fPredelay * SAMPLERATE * 0.001f));
	needIdle();
	predelayChanged = false;
}

//-----------------------------------------------------------------------------
void Scrubby::createAudioBuffers()
{
	// update the sample rate value
	SAMPLERATE = getSampleRate();
	// just in case the host responds with something wacky
	if (SAMPLERATE <= 0.0f)
		SAMPLERATE = 44100.0f;
	long oldMax = MAX_BUFFER;
	// the number of samples in the maximum seek range, 
	// dividing by the minimum seek rate for extra leeway while moving
	MAX_BUFFER = (long) (SEEK_RANGE_MAX * 0.001f * SAMPLERATE / SEEK_RATE_MIN);
	MAX_BUFFER_FLOAT = (double)MAX_BUFFER;

	// if the sampling rate (& therefore the max buffer size) has changed, 
	// then delete & reallocate the buffers according to the sampling rate
	if (MAX_BUFFER != oldMax)
	{
		if (buffer1 != NULL)
			delete[] buffer1;
		buffer1 = NULL;
		if (buffer2 != NULL)
			delete[] buffer2;
		buffer2 = NULL;
	}
	if (buffer1 == NULL)
		buffer1 = new float[MAX_BUFFER];
	if (buffer2 == NULL)
		buffer2 = new float[MAX_BUFFER];

	// clear out the buffers
	for (long i=0; i < MAX_BUFFER; i++)
		buffer1[i] = 0.0f;
	for (long j=0; j < MAX_BUFFER; j++)
		buffer2[j] = 0.0f;
}

//-------------------------------------------------------------------------
long Scrubby::fxIdle()
{
	if (predelayChanged)
		ioChanged();
	predelayChanged = false;

	return 1;
}


#pragma mark _________info_________

//-----------------------------------------------------------------------------
// this tells the host to keep calling process() for the duration of one buffer 
// even if the audio input has ended
long Scrubby::getTailSize()
{
	return MAX_BUFFER;
}

//------------------------------------------------------------------------
bool Scrubby::getInputProperties(long index, VstPinProperties* properties)
{
	if ( (index >= 0) && (index < 2) )
	{
		sprintf(properties->label, "Scrubby input %ld", index+1);
		sprintf(properties->shortLabel, "in %ld", index+1);
		properties->flags = kVstPinIsStereo | kVstPinIsActive;
		return true;
	}
	return false;
}

//------------------------------------------------------------------------
bool Scrubby::getOutputProperties(long index, VstPinProperties *properties)
{
	if ( (index >= 0) && (index < 2) )
	{
		sprintf (properties->label, "Scrubby output %ld", index+1);
		sprintf (properties->shortLabel, "out %ld", index+1);
		properties->flags = kVstPinIsStereo | kVstPinIsActive;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------------------
// Destroy FX infos

bool Scrubby::getEffectName(char *name) {
	strcpy (name, "Scrubby");	// name max 32 char
	return true; }

long Scrubby::getVendorVersion() {
	return PLUGIN_VERSION; }

bool Scrubby::getErrorText(char *text) {
	strcpy (text, "U FUCT UP");	// max 256 char
	return true; }

bool Scrubby::getVendorString(char *text) {
	strcpy (text, "Destroy FX");	// a string identifying the vendor (max 64 char)
	return true; }

bool Scrubby::getProductString(char *text) {
	// a string identifying the product name (max 64 char)
	strcpy (text, "Super Destroy FX bipolar VST plugin pack");
	return true; }

//-----------------------------------------------------------------------------------------
// this just tells the host what Scrubby can do

long Scrubby::canDo(char* text)
{
	if (strcmp(text, "receiveVstEvents") == 0)
		return 1;
	if (strcmp(text, "receiveVstMidiEvent") == 0)
		return 1;
	if (strcmp(text, "receiveVstTimeInfo") == 0)
		return 1;
	if (strcmp(text, "fuctsoundz") == 0)
		return 1;
	if (strcmp(text, "plugAsChannelInsert") == 0)
		return 1;
	if (strcmp(text, "plugAsSend") == 0)
		return 1;
	if (strcmp(text, "mixDryWet") == 0)
		return 1;
	if (strcmp(text, "1in2out") == 0)
		return 1;
	if (strcmp(text, "2in2out") == 0)
		return 1;

	return -1;	// explicitly can't do; 0 => don't know
}


#pragma mark _________programs_________

//----------------------------------------------------------------------------- 
ScrubbyChunk::ScrubbyChunk(long numParameters, long numPrograms, long magic, AudioEffectX *effect)
	: VstChunk (numParameters, numPrograms, magic, effect)
{
	// start off with split CC automation of both range slider points
	seekRateDoubleAutomate = seekDurDoubleAutomate = false;
}

//----------------------------------------------------------------------------- 
// this gets called when Scrubby automates a parameter from CC messages.
// this is where we can link parameter automation for range slider points.
void ScrubbyChunk::doLearningAssignStuff(long tag, long eventType, long eventChannel, 
										long eventNum, long delta, long eventNum2, 
										long eventBehaviourFlags, 
										long data1, long data2, float fdata1, float fdata2)
{
	if ( getSteal() )
		return;

	switch (tag)
	{
		case kSeekRate:
			if (seekRateDoubleAutomate)
				assignParam(kSeekRateRandMin, eventType, eventChannel, eventNum, eventNum2, 
							eventBehaviourFlags, data1, data2, fdata1, fdata2);
			break;
		case kSeekRateRandMin:
			if (seekRateDoubleAutomate)
				assignParam(kSeekRate, eventType, eventChannel, eventNum, eventNum2, 
							eventBehaviourFlags, data1, data2, fdata1, fdata2);
			break;
		case kSeekDur:
			if (seekDurDoubleAutomate)
				assignParam(kSeekDurRandMin, eventType, eventChannel, eventNum, eventNum2, 
							eventBehaviourFlags, data1, data2, fdata1, fdata2);
			break;
		case kSeekDurRandMin:
			if (seekDurDoubleAutomate)
				assignParam(kSeekDur, eventType, eventChannel, eventNum, eventNum2, 
							eventBehaviourFlags, data1, data2, fdata1, fdata2);
			break;
		default:
			break;
	}
}

//----------------------------------------------------------------------------- 
void ScrubbyChunk::unassignParam(long tag)
{
	VstChunk::unassignParam(tag);

	switch (tag)
	{
		case kSeekRate:
			if (seekRateDoubleAutomate)
				VstChunk::unassignParam(kSeekRateRandMin);
			break;
		case kSeekRateRandMin:
			if (seekRateDoubleAutomate)
				VstChunk::unassignParam(kSeekRate);
			break;
		case kSeekDur:
			if (seekDurDoubleAutomate)
				VstChunk::unassignParam(kSeekDurRandMin);
			break;
		case kSeekDurRandMin:
			if (seekDurDoubleAutomate)
				VstChunk::unassignParam(kSeekDur);
			break;
		default:
			break;
	}
}


//----------------------------------------------------------------------------- 
ScrubbyProgram::ScrubbyProgram()
{
	name = new char[32];
	param = new float[NUM_PARAMETERS];

	param[kSeekRange] = seekRangeUnscaled(333.0f);
	param[kFreeze] = 0.0f;	// default to off
	param[kSeekRate] = seekRateUnscaled(9.0f);
	param[kSeekRateRandMin] = seekRateUnscaled(9.0f);
	param[kTempoSync] = 0.0f;	// default to no tempo sync; "free" control
	param[kSeekDur] = seekDurUnscaled(1.0f);	// default to 100%
	param[kSeekDurRandMin] = seekDurScaled(1.0f);
	param[kPortamento] = 0.0f;	// default to on
	param[kSplitStereo] = 0.0f;	// default to unified
	param[kPitchConstraint] = 0.0f;	// default to off
	param[kOctaveMin] = 0.0f;	// default to -infinity
	param[kOctaveMax] = 1.0f;	// default to +infinity
	param[kTempo] = 0.0f;	// default to "auto" (i.e. get it from the host)
	param[kPredelay] = 0.0f;	// default to none
	for (int i=kPitchStep0; i <= kPitchStep11; i++)
		param[i] = 0.0f;	// default all notes to off (looks better on the GUI)
	// no, I changed my mind, at least leave 1 note on so that the user isn't 
	// confused the first time turning on pitch constraint & getting silence
	param[kPitchStep0] = 1.0f;

	strcpy(name, "default");
}

//----------------------------------------------------------------------------- 
ScrubbyProgram::~ScrubbyProgram()
{
	if (name)
		delete[] name;
	if (param)
		delete[] param;
}


//-------------------------------------------------------------------------
void Scrubby::initPresets()
{
  int i = 1;

	programs[i].param[kSeekRange] = seekRangeUnscaled(603.0f);
	programs[i].param[kFreeze] = 0.0f;
	programs[i].param[kSeekRate] = programs[i].param[kSeekRateRandMin] = seekRateUnscaled(11.547f);
	programs[i].param[kTempoSync] = 0.0f;
	programs[i].param[kSeekDur] = programs[i].param[kSeekDurRandMin] = seekDurUnscaled(0.408f);
	programs[i].param[kPortamento] = 0.0f;
	programs[i].param[kSplitStereo] = 0.0f;
	programs[i].param[kPitchConstraint] = 1.0f;
	programs[i].param[kOctaveMin] = 0.0f;
	programs[i].param[kOctaveMax] = octaveMaxUnscaled(1);
	programs[i].param[kPitchStep0] = 1.0f;
	programs[i].param[kPitchStep1] = 0.0f;
	programs[i].param[kPitchStep2] = 0.0f;
	programs[i].param[kPitchStep3] = 0.0f;
	programs[i].param[kPitchStep4] = 0.0f;
	programs[i].param[kPitchStep5] = 0.0f;
	programs[i].param[kPitchStep6] = 0.0f;
	programs[i].param[kPitchStep7] = 1.0f;
	programs[i].param[kPitchStep8] = 0.0f;
	programs[i].param[kPitchStep9] = 0.0f;
	programs[i].param[kPitchStep10] = 0.0f;
	programs[i].param[kPitchStep11] = 0.0f;
	strcpy(programs[i].name, "happy machine");
	i++;

	programs[i].param[kSeekRange] = seekRangeUnscaled(3.0f);
	programs[i].param[kFreeze] = 0.0f;
	programs[i].param[kSeekRate] = programs[i].param[kSeekRateRandMin] = seekRateUnscaled(18.0f);
	programs[i].param[kTempoSync] = 0.0f;
	programs[i].param[kSeekDur] = programs[i].param[kSeekDurRandMin] = seekDurUnscaled(1.0f);
	programs[i].param[kPortamento] = 0.0f;
	programs[i].param[kSplitStereo] = 1.0f;
	programs[i].param[kPitchConstraint] = 0.0f;
	strcpy(programs[i].name, "fake chorus");
	i++;

	programs[i].param[kSeekRange] = seekRangeUnscaled(3000.0f);
	programs[i].param[kFreeze] = 0.0f;
	programs[i].param[kSeekRate] = programs[i].param[kSeekRateRandMin] = seekRateUnscaled(6.0f);
	programs[i].param[kTempoSync] = 0.0f;
	programs[i].param[kSeekDur] = programs[i].param[kSeekDurRandMin] = seekDurUnscaled(1.0f);
	programs[i].param[kPortamento] = 1.0f;
	programs[i].param[kSplitStereo] = 0.0f;
	programs[i].param[kPitchConstraint] = 0.0f;
	strcpy(programs[i].name, "broken turntable");
	i++;

	programs[i].param[kSeekRange] = seekRangeUnscaled(270.0f);
	programs[i].param[kFreeze] = 0.0f;
	programs[i].param[kSeekRate] = seekRateUnscaled(420.0f);
	programs[i].param[kSeekRateRandMin] = seekRateUnscaled(7.2f);
	programs[i].param[kTempoSync] = 0.0f;
	programs[i].param[kSeekDur] = seekDurUnscaled(57.0f);
	programs[i].param[kSeekDurRandMin] = seekDurScaled(30.0f);
	programs[i].param[kPortamento] = 0.0f;
	programs[i].param[kSplitStereo] = 0.0f;
	programs[i].param[kPitchConstraint] = 0.0f;
	strcpy(programs[i].name, "blib");
	i++;

/*
	programs[i].param[kSeekRange] = seekRangeUnscaled(f);
	programs[i].param[kFreeze] = .0f;
	programs[i].param[kSeekRate] = seekRateUnscaled(f);
	programs[i].param[kSeekRateRandMin] = seekRateUnscaled(f);
	programs[i].param[kTempoSync] = .0f;
	programs[i].param[kSeekDur] = seekDurUnscaled(f);
	programs[i].param[kSeekDurRandMin] = seekDurScaled(f);
	programs[i].param[kPortamento] = .0f;
	programs[i].param[kSplitStereo] = .0f;
	programs[i].param[kPitchConstraint] = .0f;
	programs[i].param[kOctaveMin] = octaveMinUnscaled();
	programs[i].param[kOctaveMax] = octaveMaxUnscaled();
	programs[i].param[kTempo] = f;
	programs[i].param[kPredelay] = f;
	programs[i].param[kPitchStep0] = .0f;
	programs[i].param[kPitchStep1] = .0f;
	programs[i].param[kPitchStep2] = .0f;
	programs[i].param[kPitchStep3] = .0f;
	programs[i].param[kPitchStep4] = .0f;
	programs[i].param[kPitchStep5] = .0f;
	programs[i].param[kPitchStep6] = .0f;
	programs[i].param[kPitchStep7] = .0f;
	programs[i].param[kPitchStep8] = .0f;
	programs[i].param[kPitchStep9] = .0f;
	programs[i].param[kPitchStep10] = .0f;
	programs[i].param[kPitchStep11] = .0f;
	strcpy(programs[i].name, "");
	i++;
*/
}


//----------------------------------------------------------------------------- 
void Scrubby::setProgram(long programNum)
{
	if ( (programNum < NUM_PROGRAMS) && (programNum >= 0) )
	{
		AudioEffectX::setProgram(programNum);

		for (int i=0; i < NUM_PARAMETERS; i++)
			setParameter(i, programs[programNum].param[i]);
	}
	// tell the host to update the editor display with the new settings
	AudioEffectX::updateDisplay();
}

//-----------------------------------------------------------------------------------------
void Scrubby::setProgramName(char *name)
{
	strcpy(programs[curProgram].name, name);
}

//-----------------------------------------------------------------------------------------
void Scrubby::getProgramName(char *name)
{
	if ( !strcmp(programs[curProgram].name, "default") )
		sprintf(name, "default %ld", curProgram+1);
	else
		strcpy(name, programs[curProgram].name);
}

//-----------------------------------------------------------------------------------------
bool Scrubby::getProgramNameIndexed(long category, long index, char *text)
{
	if ( (index < NUM_PROGRAMS) && (index >= 0) )
	{
		strcpy(text, programs[index].name);
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------------------
bool Scrubby::copyProgram(long destination)
{
	if ( (destination < NUM_PROGRAMS) && (destination >= 0) )
	{
		if (destination != curProgram)	// only copy it if it's a different program slot
		{
			for (long i = 0; i < NUM_PARAMETERS; i++)
				programs[destination].param[i] = programs[curProgram].param[i];
			strcpy(programs[destination].name, programs[curProgram].name);
		}
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------------------
long Scrubby::getChunk(void **data, bool isPreset)
{	return chunk->getChunk(data, isPreset);	}

//-----------------------------------------------------------------------------------------
long Scrubby::setChunk(void *data, long byteSize, bool isPreset)
{	return chunk->setChunk(data, byteSize, isPreset);	}

#pragma mark _________parameters_________

//-----------------------------------------------------------------------------------------
void Scrubby::setParameter(long index, float value)
{
	if ( (index < 0) && (index >= NUM_PARAMETERS) )
		return;

	switch (index)
	{
		case kSeekRange       : fSeekRange = value;			break;
		case kFreeze          : fFreeze = value;			break;
		case kSeekRate:
			// make sure the cycles match up if the tempo rate has changed
			if (tempoRateTable->getScalar(fSeekRate) != tempoRateTable->getScalar(value))
				needResync1 = needResync2 = true;
			fSeekRate = value;
			break;
		case kSeekRateRandMin : fSeekRateRandMin = value;	break;
		case kTempoSync:
			// set needResync true if tempo sync mode has just been switched on
			if ( onOffTest(value) && !onOffTest(fTempoSync) )
				needResync1 = needResync2 = true;
			fTempoSync = value;
			break;
		case kSeekDur         : fSeekDur = value;			break;
		case kSeekDurRandMin  : fSeekDurRandMin = value;	break;
		case kPortamento      : fPortamento = value;		break;
		case kSplitStereo     : fSplitStereo = value;		break;
		case kPitchConstraint : fPitchConstraint = value;	break;
		case kOctaveMin       : fOctaveMin = value;			break;
		case kOctaveMax       : fOctaveMax = value;			break;
		case kTempo           : fTempo = value;				break;
		case kPredelay:
			fPredelay = value;
			// this tells the host to call a suspend()-resume() pair, which updates initialDelay
			predelayChanged = true;
			break;
		default:	break;
	}

	if ( (index >= kPitchStep0) && (index <= kPitchStep11) )
	{
		fPitchSteps[index-kPitchStep0] = value;
		// reset the associated note in the notes table; manual changes override MIDI
		activeNotesTable[index-kPitchStep0] = 0;
	}

	programs[curProgram].param[index] = value;

	if (editor)
		((AEffGUIEditor*)editor)->setParameter(index, value);
}

//-----------------------------------------------------------------------------------------
float Scrubby::getParameter(long index)
{
	if ( (index >= kPitchStep0) && (index <= kPitchStep11) )
		return fPitchSteps[index-kPitchStep0];

	switch (index)
	{
		default:
		case kSeekRange       : return fSeekRange;
		case kFreeze          : return fFreeze;
		case kSeekRate        : return fSeekRate;
		case kSeekRateRandMin : return fSeekRateRandMin;
		case kTempoSync       : return fTempoSync;
		case kSeekDur         : return fSeekDur;
		case kSeekDurRandMin  : return fSeekDurRandMin;
		case kPortamento      : return fPortamento;
		case kSplitStereo     : return fSplitStereo;
		case kPitchConstraint : return fPitchConstraint;
		case kOctaveMin       : return fOctaveMin;
		case kOctaveMax       : return fOctaveMax;
		case kTempo           : return fTempo;
		case kPredelay        : return fPredelay;
	}
}

//-----------------------------------------------------------------------------------------
// titles of each parameter

void Scrubby::getParameterName(long index, char *label)
{
	switch (index)
	{
		case kSeekRange       : strcpy(label, "seek range");			break;
		case kFreeze          : strcpy(label, "freeze");				break;
		case kSeekRate        : strcpy(label, "seek rate");				break;
		case kSeekRateRandMin : strcpy(label, "seek rate rand min");	break;
		case kTempoSync       : strcpy(label, "tempo sync");			break;
		case kSeekDur         : strcpy(label, "seek duration");			break;
		case kSeekDurRandMin  : strcpy(label, "seek dur rand min");		break;
		case kPortamento      : strcpy(label, "speeds");				break;
		case kSplitStereo     : strcpy(label, "stereo");				break;
		case kPitchConstraint : strcpy(label, "pitch constraint");		break;
		case kTempo           : strcpy(label, "tempo");					break;
		case kPredelay        : strcpy(label, "predelay");				break;
		case kPitchStep0      : strcpy(label, "semi0 (unity/octave)");	break;
		case kPitchStep1      : strcpy(label, "semi1 (minor 2nd)");		break;
		case kPitchStep2      : strcpy(label, "semi2 (major 2nd)");		break;
		case kPitchStep3      : strcpy(label, "semi3 (minor 3rd)");		break;
		case kPitchStep4      : strcpy(label, "semi4 (major 3rd)");		break;
		case kPitchStep5      : strcpy(label, "semi5 (perfect 4th)");	break;
		case kPitchStep6      : strcpy(label, "semi6 (augmented 4th)");	break;
		case kPitchStep7      : strcpy(label, "semi7 (perfect 5th)");	break;
		case kPitchStep8      : strcpy(label, "semi8 (minor 6th)");		break;
		case kPitchStep9      : strcpy(label, "semi9 (major 6th)");		break;
		case kPitchStep10     : strcpy(label, "semi10 (minor 7th)");	break;
		case kPitchStep11     : strcpy(label, "semi11 (major 7th)");	break;
		case kOctaveMin       : strcpy(label, "octave minimum");		break;
		case kOctaveMax       : strcpy(label, "octave maximum");		break;
		default: break;
	}
}

//-----------------------------------------------------------------------------------------
// numerical display of each parameter's gradiations

void Scrubby::getParameterDisplay(long index, char *text)
{
	switch (index)
	{
		case kSeekRange:
			sprintf(text, "%.3f", seekRangeScaled(fSeekRange));
			break;
		case kFreeze:
			if (onOffTest(fFreeze))
				strcpy(text, "yes");
			else
				strcpy(text, "no");
			break;

		case kSeekRate:
			if (onOffTest(fTempoSync))
				strcpy(text, tempoRateTable->getDisplay(fSeekRate));
			else
				sprintf(text, "%.3f", seekRateScaled(fSeekRate));
			break;
		case kSeekRateRandMin:
			if (fSeekRate > fSeekRateRandMin)
			{
				if (onOffTest(fTempoSync))
					strcpy(text, tempoRateTable->getDisplay(fSeekRateRandMin));
				else
					sprintf(text, "%.3f", seekRateScaled(fSeekRateRandMin));
			}
			else
				strcpy(text, "no randoms");
			break;
		case kTempoSync:
			if (onOffTest(fTempoSync))
				strcpy(text, "yes");
			else
				strcpy(text, "no");
			break;

		case kSeekDur:
			sprintf(text, "%.1f%%", seekDurScaled(fSeekDur) * 100.0f);
			break;
		case kSeekDurRandMin:
			if (fSeekDur > fSeekDurRandMin)
				sprintf(text, "%.1f%%", seekDurScaled(fSeekDurRandMin) * 100.0f);
			else
				strcpy(text, "no randoms");
			break;

		case kPortamento:
			if (onOffTest(fPortamento))
				strcpy(text, "DJ");
			else
				strcpy(text, "robot");
			break;
		case kSplitStereo:
			if (onOffTest(fSplitStereo))
				strcpy(text, "split");
			else
				strcpy(text, "unified");
			break;

		case kPitchConstraint:
			if (onOffTest(fPortamento))
				strcpy(text, "irrelevant");
			else if (onOffTest(fPitchConstraint))
				strcpy(text, "on");
			else
				strcpy(text, "off");
			break;
		case kOctaveMin:
			if ( onOffTest(fPortamento) || !onOffTest(fPitchConstraint) )
				strcpy(text, "irrelevant");
			else if (octaveMinScaled(fOctaveMin) <= OCTAVE_MIN)
				strcpy(text, "no min");
			else
				sprintf(text, "%ld", octaveMinScaled(fOctaveMin));
			break;
		case kOctaveMax:
			if ( onOffTest(fPortamento) || !onOffTest(fPitchConstraint) )
				strcpy(text, "irrelevant");
			else if (octaveMaxScaled(fOctaveMax) >= OCTAVE_MAX)
				strcpy(text, "no max");
			else
				sprintf(text, "%+ld", octaveMaxScaled(fOctaveMax));
			break;

		case kTempo:
			if ( (fTempo > 0.0f) || (hostCanDoTempo != 1) )
				sprintf(text, "%.3f", tempoScaled(fTempo));
			else
				strcpy(text, "auto");
			break;
		case kPredelay:
			sprintf(text, "%d%%", (int)(fPredelay*100.0f));
			break;

		default: break;
	}

	if ( (index >= kPitchStep0) && (index <= kPitchStep11) )
	{
		if (onOffTest(fPitchSteps[index-kPitchStep0]))
			strcpy(text, "okay");
		else
			strcpy(text, "no");
	}
}

//-----------------------------------------------------------------------------------------
// unit of measure for each parameter

void Scrubby::getParameterLabel(long index, char *label)
{
	switch (index)
	{
		case kSeekRange: strcpy(label, "ms");	break;
		case kSeekRate:
		case kSeekRateRandMin:
			if (onOffTest(fTempoSync))
				strcpy(label, "seeks/beat");
			else
				strcpy(label, "Hz");
			break;
		case kSeekDur:
		case kSeekDurRandMin:
			strcpy(label, "% of range");
			break;
		case kOctaveMin:
		case kOctaveMax: strcpy(label, "octaves");	break;
		case kTempo: strcpy(label, "bpm");	break;
		case kPredelay : strcpy(label, "% of range");	break;
		default: strcpy(label, " ");		break;
	}
}

//-----------------------------------------------------------------------------------------
// convert some of the 0.0 - 1.0 parameter values into values that are actually useful
void Scrubby::getRealValues()
{
	// calculate some useful booleans
	useSeekRateRandMin = (fSeekRateRandMin < fSeekRate);
	useSeekDurRandMin = (fSeekDurRandMin < fSeekDur);
	freeze = onOffTest(fFreeze);
	splitStereo = onOffTest(fSplitStereo);
	tempoSync = onOffTest(fTempoSync);
	portamento = onOffTest(fPortamento);
	pitchConstraint = onOffTest(fPitchConstraint);
	for (int i=0; i < NUM_PITCH_STEPS; i++)
		pitchSteps[i] = onOffTest(fPitchSteps[i]);
}



#pragma mark _________main_________

//-----------------------------------------------------------------------------
//                                   main()                                   |
//-----------------------------------------------------------------------------

// prototype of the export function main
#if BEOS
#define main main_plugin
extern "C" __declspec(dllexport) AEffect *main_plugin(audioMasterCallback audioMaster);

#else
AEffect *main(audioMasterCallback audioMaster);
#endif

AEffect *main(audioMasterCallback audioMaster)
{
	// get vst version
	if ( !audioMaster(0, audioMasterVersion, 0, 0, 0, 0) )
		return 0;  // old version

	AudioEffect* effect = new Scrubby(audioMaster);
	if (!effect)
		return 0;
	return effect->getAeffect();
}

#if WIN32
void* hInstance;
BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved)
{
	hInstance = hInst;
	return 1;
}
#endif
