/*------------------- by Marc Poirier  ][  March 2001 -------------------*/

#ifndef __bufferoverride
#include "bufferoverride.hpp"
#endif

#include "vstgui.h"		// for AEffGuiEditor::setParameter()

#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#pragma mark _________init_________

//-----------------------------------------------------------------------------
// initializations & such

BufferOverride::BufferOverride(audioMasterCallback audioMaster)
	: AudioEffectX(audioMaster, NUM_PROGRAMS, NUM_PARAMETERS)	// 16 programs, 17 parameters
{
	buffer1 = NULL;
#ifdef BUFFEROVERRIDE_STEREO
	buffer2 = NULL;
#endif
	// default this to something, for the sake of getTailSize()
	SUPER_MAX_BUFFER = (long) ((44100.0f / MIN_ALLOWABLE_BPS) * 4.0f);

/* begin inter-plugin audio sharing stuff */
#ifdef HUNGRY
	#ifdef BUFFEROVERRIDE_STEREO
	foodEater = new FoodEater(this, magic, 2);
	#else
	foodEater = new FoodEater(this, magic, 1);
	#endif
#else
/* end inter-plugin audio sharing stuff */
	#ifdef BUFFEROVERRIDE_STEREO
	setNumInputs(2);	// stereo inputs; not a synth
	canMono();	// it's okay to feed both inputs with the same signal
	#else
	setNumInputs(1);	// mono inputs; not a synth
	#endif
#endif

#ifdef BUFFEROVERRIDE_STEREO
	setNumOutputs(2);	// stereo out
	setUniqueID(PLUGIN_ID);	// identify Buffer Override stereo   'bufS'
#else
	setNumOutputs(1);	// mono out
	setUniqueID('bufM');	// identify Buffer Override mono
#endif

	canProcessReplacing();	// supports both accumulating and replacing output

	// allocate memory for these structures
	midistuff = new VstMidi;
	tempoRateTable = new TempoRateTable;
	divisorLFO = new LFO;
	bufferLFO = new LFO;

	chunk = new VstChunk(NUM_PARAMETERS, NUM_PROGRAMS, PLUGIN_ID, this);
	programs = new BufferOverrideProgram[NUM_PROGRAMS];
	setProgram(0);
	strcpy(programs[0].name, "self-determined");	// default program name
	initPresets();

	suspend();

	// check to see if the host supports sending tempo & time information to VST plugins
	hostCanDoTempo = canHostDo("sendVstTimeInfo");
	// default the tempo to something more reasonable than 39 bmp
	// also give currentTempoBPS a value in case that's useful for a freshly opened GUI
	if (hostCanDoTempo == 1)
		currentTempoBPS = (float)tempoAt(0) / 600000.0f;
	if ( (hostCanDoTempo != 1) || (currentTempoBPS <= 0.0f) )
	{
		setParameter( kTempo, tempoUnscaled(120.0f) );
		currentTempoBPS = tempoScaled(fTempo) / 60.0f;
	}

	editor = new BufferOverrideEditor(this);
}

//-------------------------------------------------------------------------
BufferOverride::~BufferOverride()
{
	if (programs)
		delete[] programs;
	if (chunk)
		delete chunk;

	// deallocate the memory from these arrays
	if (buffer1)
		delete[] buffer1;
#ifdef BUFFEROVERRIDE_STEREO
	if (buffer2)
		delete[] buffer2;
#endif
	if (midistuff)
		delete midistuff;
	if (tempoRateTable)
		delete tempoRateTable;
	if (divisorLFO)
		delete divisorLFO;
	if (bufferLFO)
		delete bufferLFO;

/* begin inter-plugin audio sharing stuff */
#ifdef HUNGRY
	if (foodEater)
		delete foodEater;
#endif
/* end inter-plugin audio sharing stuff */
}

//-------------------------------------------------------------------------
void BufferOverride::suspend()
{
	// setting the values like this will restart the forced buffer in the next process()
	currentForcedBufferSize = 1;
	writePos = readPos = 1;
	minibufferSize = 1;
	prevMinibufferSize = 0;
	smoothcount = smoothDur = 0;
	sqrtFadeIn = sqrtFadeOut = 1.0f;

	divisorLFO->reset();
	bufferLFO->reset();

	oldNote = false;
	lastNoteOn = kInvalidMidi;
	lastPitchbend = kInvalidMidi;
	pitchbend = 1.0;
	oldPitchbend = 1.0;
	divisorWasChangedByMIDI = divisorWasChangedByHand = false;
	midistuff->reset();

#ifdef HUNGRY	// inter-plugin audio sharing stuff
	foodEater->suspend();
#endif
}

//-----------------------------------------------------------------------------
// this gets called when the plugin is activated
void BufferOverride::resume()
{
	needResync = true;	// some hosts may call resume when restarting playback
	wantEvents();

	createAudioBuffers();
}

//-------------------------------------------------------------------------
void BufferOverride::createAudioBuffers()
{
	// update the sample rate value
	SAMPLERATE = getSampleRate();
	// just in case the host responds with something wacky
	if (SAMPLERATE <= 0.0f)
		SAMPLERATE = 44100.0f;
	long oldMax = SUPER_MAX_BUFFER;
	SUPER_MAX_BUFFER = (long) ((SAMPLERATE / MIN_ALLOWABLE_BPS) * 4.0f);

	// if the sampling rate (& therefore the max buffer size) has changed, 
	// then delete & reallocate the buffers according to the sampling rate
	if (SUPER_MAX_BUFFER != oldMax)
	{
		if (buffer1 != NULL)
			delete[] buffer1;
		buffer1 = NULL;
	#ifdef BUFFEROVERRIDE_STEREO
		if (buffer2 != NULL)
			delete[] buffer2;
		buffer2 = NULL;
	#endif
	}
	if (buffer1 == NULL)
		buffer1 = new float[SUPER_MAX_BUFFER];
	#ifdef BUFFEROVERRIDE_STEREO
	if (buffer2 == NULL)
		buffer2 = new float[SUPER_MAX_BUFFER];
	#endif
}


#pragma mark _________info_________

//-------------------------------------------------------------------------
// Destroy FX infos

bool BufferOverride::getEffectName(char *name) {
	#ifdef BUFFEROVERRIDE_STEREO
	strcpy(name, "Buffer Override (stereo)");	// name max 32 char
	#else
	strcpy(name, "Buffer Override (mono)");	// name max 32 char
	#endif
	return true; }

long BufferOverride::getVendorVersion() {
	return PLUGIN_VERSION; }

bool BufferOverride::getErrorText(char *text) {
	strcpy(text, "This is hopeless.");	// max 256 char
	return true; }

bool BufferOverride::getVendorString(char *text) {
	strcpy(text, "Destroy FX");	// a string identifying the vendor (max 64 char)
	return true; }

bool BufferOverride::getProductString(char *text) {
	// a string identifying the product name (max 64 char)
	strcpy(text, "Super Destroy FX bipolar VST plugin pack");
	return true; }

//-----------------------------------------------------------------------------------------
// this tells the host to keep calling process() for the duration of one forced buffer 
// even if the audio input has ended

long BufferOverride::getTailSize()
{
	return SUPER_MAX_BUFFER;
}

//------------------------------------------------------------------------
bool BufferOverride::getInputProperties(long index, VstPinProperties* properties)
{
#ifdef BUFFEROVERRIDE_STEREO
	long numIns = 2;
	long ioFlags = kVstPinIsStereo | kVstPinIsActive;
#else
	long numIns = 1;
	long ioFlags = kVstPinIsActive;
#endif

	if ( (index >= 0) && (index < numIns) )
	{
		sprintf(properties->label, "Buffer Override input %ld", index+1);
		sprintf(properties->shortLabel, "in %ld", index+1);
		properties->flags = ioFlags;
		return true;
	}
	return false;
}

//------------------------------------------------------------------------
bool BufferOverride::getOutputProperties(long index, VstPinProperties* properties)
{
#ifdef BUFFEROVERRIDE_STEREO
	long numOuts = 2;
	long ioFlags = kVstPinIsStereo | kVstPinIsActive;
#else
	long numOuts = 1;
	long ioFlags = kVstPinIsActive;
#endif

	if ( (index >= 0) && (index < numOuts) )
	{
		sprintf (properties->label, "Buffer Override output %ld", index+1);
		sprintf (properties->shortLabel, "out %ld", index+1);
		properties->flags = ioFlags;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------------------
// this tells the host what BufferOverride can do

long BufferOverride::canDo(char* text)
{
	if (strcmp(text, "receiveVstEvents") == 0)
		return 1;
	if (strcmp(text, "receiveVstMidiEvent") == 0)
		return 1;
	if (strcmp(text, "receiveVstTimeInfo") == 0)
		return 1;
	if (strcmp(text, "plugAsChannelInsert") == 0)
		return 1;
	if (strcmp(text, "plugAsSend") == 0)
		return 1;
	if (strcmp(text, "mixDryWet") == 0)
		return 1;
	if (strcmp(text, "1in1out") == 0)
		return 1;
#ifdef BUFFEROVERRIDE_STEREO
	if (strcmp(text, "1in2out") == 0)
		return 1;
	if (strcmp(text, "2in2out") == 0)
		return 1;
#endif

	return -1;	// explicitly can't do; 0 means "I don't know."
}


#pragma mark _________programs_________

//----------------------------------------------------------------------------- 
BufferOverrideProgram::BufferOverrideProgram()
{
	name = new char[32];
	param = new float[NUM_PARAMETERS];

	param[kDivisor] = 0.0f;
	param[kBuffer] = forcedBufferSizeUnscaled(90.0f);
	param[kBufferTempoSync] = 0.0f;	// default to no tempo sync
	param[kBufferInterrupt] = 1.0f;	// default to on, use new forced buffer behaviour
	param[kDivisorLFOrate] = LFOrateUnscaled(0.3f);
	param[kDivisorLFOdepth] = 0.0f;
	param[kDivisorLFOshape] = 0.0f;
	param[kDivisorLFOtempoSync] = 0.0f;
	param[kBufferLFOrate] = LFOrateUnscaled(3.0f);
	param[kBufferLFOdepth] = 0.0f;
	param[kBufferLFOshape] = 0.0f;
	param[kBufferLFOtempoSync] = 0.0f;
	param[kSmooth] = 0.09f;
	param[kDryWetMix] = 1.0f;	// default to all wet
	param[kPitchbend] = 6.0f / (float)PITCHBEND_MAX;
	param[kMidiMode] = 0.0f;	// default to "nudge" mode
	param[kTempo] = 0.0f;	// default to "auto" (i.e. get it from the host)
#ifdef HUNGRY
	param[kConnect] = 0.0f;	// default to disconnected
#endif

	strcpy(name, "default");
}

//----------------------------------------------------------------------------- 
BufferOverrideProgram::~BufferOverrideProgram()
{
	if (name)
		delete[] name;
	if (param)
		delete[] param;
}

//-------------------------------------------------------------------------
void BufferOverride::initPresets()
{
  int i = 1;

	programs[i].param[kDivisor] = bufferDivisorUnscaled(4.0f);
	programs[i].param[kBuffer] = paramSteppedUnscaled(8.7f, NUM_TEMPO_RATES);
	programs[i].param[kBufferTempoSync] = 1.0f;
	programs[i].param[kSmooth] = 0.09f;
	programs[i].param[kDryWetMix] = 1.0f;
	programs[i].param[kMidiMode] = 0.0f;
	strcpy(programs[i].name, "drum roll");
	i++;

	programs[i].param[kDivisor] = bufferDivisorUnscaled(37.0f);
	programs[i].param[kBuffer] = forcedBufferSizeUnscaled(444.0f);
	programs[i].param[kBufferTempoSync] = 0.0f;
	programs[i].param[kBufferInterrupt] = 1.0f;
	programs[i].param[kDivisorLFOrate] = LFOrateUnscaled(0.3f);
	programs[i].param[kDivisorLFOdepth] = 0.72f;
	programs[i].param[kDivisorLFOshape] = LFOshapeUnscaled(kSawLFO);
	programs[i].param[kDivisorLFOtempoSync] = 0.0f;
	programs[i].param[kBufferLFOrate] = LFOrateUnscaled(0.27f);
	programs[i].param[kBufferLFOdepth] = 0.63f;
	programs[i].param[kBufferLFOshape] = LFOshapeUnscaled(kSawLFO);
	programs[i].param[kBufferLFOtempoSync] = 0.0f;
	programs[i].param[kSmooth] = 0.042f;
	programs[i].param[kDryWetMix] = 1.0f;
	programs[i].param[kMidiMode] = 0.0f;
	strcpy(programs[i].name, "arpeggio");
	i++;

	programs[i].param[kDivisor] = bufferDivisorUnscaled(170.0f);
	programs[i].param[kBuffer] = forcedBufferSizeUnscaled(128.0f);
	programs[i].param[kBufferTempoSync] = 0.0f;
	programs[i].param[kBufferInterrupt] = 1.0f;
	programs[i].param[kDivisorLFOrate] = LFOrateUnscaled(9.0f);
	programs[i].param[kDivisorLFOdepth] = 0.87f;
	programs[i].param[kDivisorLFOshape] = LFOshapeUnscaled(kThornLFO);
	programs[i].param[kDivisorLFOtempoSync] = 0.0f;
	programs[i].param[kBufferLFOrate] = LFOrateUnscaled(5.55f);
	programs[i].param[kBufferLFOdepth] = 0.69f;
	programs[i].param[kBufferLFOshape] = LFOshapeUnscaled(kReverseSawLFO);
	programs[i].param[kBufferLFOtempoSync] = 0.0f;
	programs[i].param[kSmooth] = 0.201f;
	programs[i].param[kDryWetMix] = 1.0f;
	programs[i].param[kMidiMode] = 0.0f;
	strcpy(programs[i].name, "laser");
	i++;

	programs[i].param[kDivisor] = bufferDivisorUnscaled(42.0f);
	programs[i].param[kBuffer] = forcedBufferSizeUnscaled(210.0f);
	programs[i].param[kBufferTempoSync] = 0.0f;
	programs[i].param[kBufferInterrupt] = 1.0f;
	programs[i].param[kDivisorLFOrate] = LFOrateUnscaled(3.78f);
	programs[i].param[kDivisorLFOdepth] = 0.9f;
	programs[i].param[kDivisorLFOshape] = LFOshapeUnscaled(kRandomLFO);
	programs[i].param[kDivisorLFOtempoSync] = 0.0f;
	programs[i].param[kBufferLFOdepth] = 0.0f;
	programs[i].param[kSmooth] = 0.039f;
	programs[i].param[kDryWetMix] = 1.0f;
	programs[i].param[kMidiMode] = 0.0f;
	strcpy(programs[i].name, "sour melodies");
	i++;

	programs[i].param[kDivisor] = bufferDivisorUnscaled(9.0f);
	programs[i].param[kBuffer] = forcedBufferSizeUnscaled(747.0f);
	programs[i].param[kBufferTempoSync] = 0.0f;
	programs[i].param[kDivisorLFOrate] = 0.0f;
	programs[i].param[kDivisorLFOdepth] = 0.0f;
	programs[i].param[kDivisorLFOshape] = LFOshapeUnscaled(kTriangleLFO);
	programs[i].param[kDivisorLFOtempoSync] = 0.0f;
	programs[i].param[kBufferLFOrate] = LFOrateUnscaled(0.174f);
	programs[i].param[kBufferLFOdepth] = 0.21f;
	programs[i].param[kBufferLFOshape] = LFOshapeUnscaled(kTriangleLFO);
	programs[i].param[kBufferLFOtempoSync] = 0.0f;
	programs[i].param[kSmooth] = 0.081f;
	programs[i].param[kDryWetMix] = 1.0f;
	programs[i].param[kMidiMode] = 0.0f;
	strcpy(programs[i].name, "rerun");
	i++;

	programs[i].param[kDivisor] = bufferDivisorUnscaled(2.001f);
	programs[i].param[kBuffer] = forcedBufferSizeUnscaled(603.0f);
	programs[i].param[kBufferTempoSync] = 0.0f;
	programs[i].param[kDivisorLFOdepth] = 0.0f;
	programs[i].param[kBufferLFOdepth] = 0.0f;
	programs[i].param[kSmooth] = 1.0f;
	programs[i].param[kDryWetMix] = 1.0f;
	programs[i].param[kMidiMode] = 0.0f;
	strcpy(programs[i].name, "\"echo\"");
	i++;

	programs[i].param[kDivisor] = bufferDivisorUnscaled(27.0f);
	programs[i].param[kBuffer] = forcedBufferSizeUnscaled(81.0f);
	programs[i].param[kBufferTempoSync] = 0.0f;
	programs[i].param[kBufferInterrupt] = 1.0f;
	programs[i].param[kDivisorLFOrate] = paramSteppedUnscaled(6.6f, NUM_TEMPO_RATES);
	programs[i].param[kDivisorLFOdepth] = 0.333f;
	programs[i].param[kDivisorLFOshape] = LFOshapeUnscaled(kSineLFO);
	programs[i].param[kDivisorLFOtempoSync] = 1.0f;
	programs[i].param[kBufferLFOrate] = 0.0f;
	programs[i].param[kBufferLFOdepth] = 0.06f;
	programs[i].param[kBufferLFOshape] = LFOshapeUnscaled(kSawLFO);
	programs[i].param[kBufferLFOtempoSync] = 1.0f;
	programs[i].param[kSmooth] = 0.06f;
	programs[i].param[kDryWetMix] = 1.0f;
	programs[i].param[kMidiMode] = 0.0f;
	programs[i].param[kTempo] = 0.0f;
	strcpy(programs[i].name, "squeegee");
	i++;

/*
	programs[i].param[kDivisor] = bufferDivisorUnscaled(f);
	programs[i].param[kBuffer] = forcedBufferSizeUnscaled(f);
	programs[i].param[kBufferTempoSync] = .0f;
	programs[i].param[kBufferInterrupt] = .0f;
	programs[i].param[kDivisorLFOrate] = LFOrateUnscaled(.f);
	programs[i].param[kDivisorLFOdepth] = f;
	programs[i].param[kDivisorLFOshape] = LFOshapeUnscaled(kLFO);
	programs[i].param[kDivisorLFOtempoSync] = .0f;
	programs[i].param[kBufferLFOrate] = LFOrateUnscaled(.f);
	programs[i].param[kBufferLFOdepth] = f;
	programs[i].param[kBufferLFOshape] = LFOshapeUnscaled(kLFO);
	programs[i].param[kBufferLFOtempoSync] = .0f;
	programs[i].param[kSmooth] = f;
	programs[i].param[kDryWetMix] = f;
	programs[i].param[kPitchbend] = f / PITCHBEND_MAX;
	programs[i].param[kMidiMode] = .0f;
	programs[i].param[kTempo] = (f - TEMPO_MIN) / TEMPO_RANGE;
	strcpy(programs[i].name, "");
	i++;
*/
}

//-----------------------------------------------------------------------------
long BufferOverride::getChunk(void **data, bool isPreset)
{	return chunk->getChunk(data, isPreset);	}

long BufferOverride::setChunk(void *data, long byteSize, bool isPreset)
{	return chunk->setChunk(data, byteSize, isPreset);	}

//----------------------------------------------------------------------------- 
void BufferOverride::setProgram(long programNum)
{
	if ( (programNum < NUM_PROGRAMS) && (programNum >= 0) )
	{
		AudioEffectX::setProgram(programNum);

		for (int i=0; i < NUM_PARAMETERS; i++)
			setParameter(i, programs[programNum].param[i]);

		// tell the host to update the default editor display with the new settings
		AudioEffectX::updateDisplay();
		// tells the GUI to update its display
//		editor->postUpdate();
	}
}

//-------------------------------------------------------------------------
void BufferOverride::setProgramName(char *name)
{
	strcpy(programs[curProgram].name, name);
}

//-------------------------------------------------------------------------
void BufferOverride::getProgramName(char *name)
{
	if ( !strcmp(programs[curProgram].name, "default") )
		sprintf(name, "default %ld", curProgram+1);
	else
		strcpy(name, programs[curProgram].name);
}

//-------------------------------------------------------------------------
bool BufferOverride::getProgramNameIndexed(long category, long index, char *text)
{
	if ( (index < NUM_PROGRAMS) && (index >= 0) )
	{
		strcpy(text, programs[index].name);
		return true;
	}
	return false;
}

//-------------------------------------------------------------------------
bool BufferOverride::copyProgram(long destination)
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


#pragma mark _________parameters_________

//-------------------------------------------------------------------------
void BufferOverride::setParameter(long index, float value)
{
	switch (index)
	{
		case kDivisor :
			fDivisor = value;
			// tell MIDI trigger mode to respect this change
			divisorWasChangedByHand = true;
			break;

		case kBuffer:
			// make sure the cycles match up if the tempo rate has changed
			if (tempoRateTable->getScalar(fBuffer) != tempoRateTable->getScalar(value))
				needResync = true;
			fBuffer = value;
			break;

		case kBufferTempoSync :
			// set needResync true if tempo sync mode has just been switched on
			if ( onOffTest(value) && !onOffTest(fBufferTempoSync) )
				needResync = true;
			fBufferTempoSync = value;
			break;

		case kBufferInterrupt     : fBufferInterrupt = value;			break;
		case kDivisorLFOrate      : divisorLFO->fRate = value;		break;
		case kDivisorLFOdepth     : divisorLFO->fDepth = value;		break;
		case kDivisorLFOshape     : divisorLFO->fShape = value;		break;
		case kDivisorLFOtempoSync : divisorLFO->fTempoSync = value;	break;
		case kBufferLFOrate       : bufferLFO->fRate = value;		break;
		case kBufferLFOdepth      : bufferLFO->fDepth = value;		break;
		case kBufferLFOshape      : bufferLFO->fShape = value;		break;
		case kBufferLFOtempoSync  : bufferLFO->fTempoSync = value;	break;
		case kSmooth              : fSmooth = value;				break;
		case kDryWetMix           : fDryWetMix = value;				break;
		case kPitchbend           : fPitchbend = value;				break;
		case kMidiMode :
			// reset all notes to off if we're switching into MIDI trigger mode
			if ( (onOffTest(value) == true) && (onOffTest(fMidiMode) == false) )
			{
				midistuff->removeAllNotes();
				divisorWasChangedByHand = false;
			}
			fMidiMode = value;
			break;
		case kTempo               : fTempo = value;					break;

/* begin inter-plugin audio sharing stuff */
#ifdef HUNGRY
		case kConnect :
			foodEater->setParameter(value);
			break;
#endif
/* end inter-plugin audio sharing stuff */

		default :
			break;
	}

	if ( (index >= 0) && (index < NUM_PARAMETERS) )
		programs[curProgram].param[index] = value;

	if (editor)
		((AEffGUIEditor*)editor)->setParameter(index, value);
}

//-------------------------------------------------------------------------
float BufferOverride::getParameter(long index)
{
	switch (index)
	{
		default:
		case kDivisor             : return fDivisor;
		case kBuffer              : return fBuffer;
		case kBufferTempoSync     : return fBufferTempoSync;
		case kBufferInterrupt     : return fBufferInterrupt;
		case kDivisorLFOrate      : return divisorLFO->fRate;
		case kDivisorLFOdepth     : return divisorLFO->fDepth;
		case kDivisorLFOshape     : return divisorLFO->fShape;
		case kDivisorLFOtempoSync : return divisorLFO->fTempoSync;
		case kBufferLFOrate       : return bufferLFO->fRate;
		case kBufferLFOdepth      : return bufferLFO->fDepth;
		case kBufferLFOshape      : return bufferLFO->fShape;
		case kBufferLFOtempoSync  : return bufferLFO->fTempoSync;
		case kSmooth              : return fSmooth;
		case kDryWetMix           : return fDryWetMix;
		case kPitchbend           : return fPitchbend;
		case kMidiMode            : return fMidiMode;
		case kTempo               : return fTempo;

		#ifdef HUNGRY
		case kConnect             : return foodEater->fConnect;
		#endif
	}
}

//-------------------------------------------------------------------------
// titles of each parameter

void BufferOverride::getParameterName(long index, char *label)
{
	switch (index)
	{
		case kDivisor             : strcpy(label, "buffer divisor");			break;
		case kBuffer              : strcpy(label, "forced buffer size");		break;
		case kBufferTempoSync     : strcpy(label, "forced buffer tempo sync");	break;
		case kBufferInterrupt     : strcpy(label, "stuck buffer");				break;
		case kDivisorLFOrate      : strcpy(label, "divisor LFO rate");			break;
		case kDivisorLFOdepth     : strcpy(label, "divisor LFO depth");			break;
		case kDivisorLFOshape     : strcpy(label, "divisor LFO shape");			break;
		case kDivisorLFOtempoSync : strcpy(label, "divisor LFO tempo sync");	break;
		case kBufferLFOrate       : strcpy(label, "buffer LFO rate");			break;
		case kBufferLFOdepth      : strcpy(label, "buffer LFO depth");			break;
		case kBufferLFOshape      : strcpy(label, "buffer LFO shape");			break;
		case kBufferLFOtempoSync  : strcpy(label, "buffer LFO tempo sync");		break;
		case kSmooth              : strcpy(label, "smooth");					break;
		case kDryWetMix           : strcpy(label, "dry/wet mix");				break;
		case kPitchbend           : strcpy(label, "pitchbend");					break;
		case kMidiMode            : strcpy(label, "MIDI mode");					break;
		case kTempo               : strcpy(label, "tempo");						break;

		#ifdef HUNGRY
		case kConnect :
			foodEater->getParameterName(label);
			break;
		#endif
	}
}

//-------------------------------------------------------------------------
// numerical display of each parameter's gradiations

void BufferOverride::getParameterDisplay(long index, char *text)
{
	switch (index)
	{
		case kDivisor :
			if (bufferDivisorScaled(fDivisor) < 2.0f)
				sprintf(text, "%.3f", 1.0f);
			else
				sprintf(text, "%.3f", bufferDivisorScaled(fDivisor));
			break;
		case kBuffer :
			if (onOffTest(fBufferTempoSync))
				strcpy(text, tempoRateTable->getDisplay(fBuffer));
			else
				sprintf(text, "%.1f", forcedBufferSizeScaled(fBuffer));
			break;
		case kBufferTempoSync :
			if (onOffTest(fBufferTempoSync))
				strcpy(text, "yes");
			else
				strcpy(text, "no");
			break;
		case kBufferInterrupt :
			if (onOffTest(fBufferInterrupt))
				strcpy(text, "yes");
			else
				strcpy(text, "no");
			break;
		case kDivisorLFOrate :
			if (onOffTest(divisorLFO->fTempoSync))
				strcpy(text, tempoRateTable->getDisplay(divisorLFO->fRate));
			else
				sprintf(text, "%.1f", LFOrateScaled(divisorLFO->fRate));
			break;
		case kDivisorLFOdepth :
			sprintf(text, "%ld %%", (long)(divisorLFO->fDepth * 100.0f));
			break;
		case kDivisorLFOshape :
			divisorLFO->getShapeName(text);
			break;
		case kDivisorLFOtempoSync :
			if (onOffTest(divisorLFO->fTempoSync))
				strcpy(text, "yes");
			else
				strcpy(text, "no");
			break;
		case kBufferLFOrate :
			if (onOffTest(bufferLFO->fTempoSync))
				strcpy(text, tempoRateTable->getDisplay(bufferLFO->fRate));
			else
				sprintf(text, "%.1f", LFOrateScaled(bufferLFO->fRate));
			break;
		case kBufferLFOdepth :
			sprintf(text, "%ld %%", (long)(bufferLFO->fDepth * 100.0f));
			break;
		case kBufferLFOshape :
			bufferLFO->getShapeName(text);
			break;
		case kBufferLFOtempoSync :
			if (onOffTest(bufferLFO->fTempoSync))
				strcpy(text, "yes");
			else
				strcpy(text, "no");
			break;
		case kSmooth :
			sprintf(text, "%.1f %%", (fSmooth*100.0f));
			break;
		case kDryWetMix :
			sprintf(text, "%ld %%", (long)(fDryWetMix*100.0f));
			break;
		case kPitchbend :
			sprintf(text, "\xB1%.2f", fPitchbend*PITCHBEND_MAX);
			break;
		case kMidiMode :
			if (onOffTest(fMidiMode))
				sprintf(text, "trigger");
			else
				strcpy(text, "nudge");
			break;
		case kTempo :
			if ( (fTempo > 0.0f) || (hostCanDoTempo != 1) )
				sprintf(text, "%.3f", tempoScaled(fTempo));
			else
				strcpy(text, "auto");
			break;

#ifdef HUNGRY
		case kConnect :
			foodEater->getParameterDisplay(text);
			break;
#endif
	}
}

//-------------------------------------------------------------------------
// unit of measure for each parameter

void BufferOverride::getParameterLabel(long index, char *label)
{
	switch (index)
	{
		case kDivisor         : strcpy(label, " ");			break;
		case kBuffer :
			if (onOffTest(fBufferTempoSync))
				strcpy(label, "buffers/beat");
			else
				strcpy(label, "samples");
			break;
		case kBufferTempoSync     : strcpy(label, " ");			break;
		case kBufferInterrupt     : strcpy(label, " ");			break;
		case kDivisorLFOrate      : strcpy(label, "Hz");		break;
		case kDivisorLFOdepth     : strcpy(label, " ");			break;
		case kDivisorLFOshape     : strcpy(label, " ");			break;
		case kDivisorLFOtempoSync : strcpy(label, " ");			break;
		case kBufferLFOrate       : strcpy(label, "Hz");		break;
		case kBufferLFOdepth      : strcpy(label, " ");			break;
		case kBufferLFOshape      : strcpy(label, " ");			break;
		case kBufferLFOtempoSync  : strcpy(label, " ");			break;
		case kSmooth              : strcpy(label, " ");			break;
		case kDryWetMix           : strcpy(label, " ");			break;
		case kPitchbend           : strcpy(label, "semitones");	break;
		case kMidiMode            : strcpy(label, " ");			break;
		case kTempo               : strcpy(label, "bpm");		break;
		default: strcpy(label, " ");	break;
	}
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

	AudioEffect* effect = new BufferOverride(audioMaster);
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
