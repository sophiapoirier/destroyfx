/*-------------- by Marc Poirier  ][  December 2000 -------------*/

#ifndef __skidder
#include "skidder.hpp"
#endif

#ifndef __skiddereditor
#include "skiddereditor.hpp"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>


//-----------------------------------------------------------------------------
// initializations & such

Skidder::Skidder(audioMasterCallback audioMaster)
	: AudioEffectX(audioMaster, NUM_PROGRAMS, NUM_PARAMETERS)	// 16 programs, 14 parameters
{
#ifdef HUNGRY	// inter-plugin audio sharing stuff
	foodEater = new FoodEater(this, magic, 2);
#else
	setNumInputs(2);	// stereo input   (& not a synth)
	canMono();
#endif
	setNumOutputs(2);	// stereo out
	canProcessReplacing();	// supports both accumulating and replacing output
	noTail();	// there is no audio output when the audio input is silence

	tempoRateTable = new TempoRateTable;

	programs = new SkidderProgram[NUM_PROGRAMS];
	setProgram(0);
	strcpy(programs[0].name, "thip thip thip");	// default program name

#ifdef MSKIDDER
	setUniqueID(PLUGIN_ID);	// identify Skidder
#else
	setUniqueID('_kid');	// identify old, non-MIDI Skidder
#endif

	// allocate memory for these arrays
#ifdef MSKIDDER
	noteTable = new int[NUM_NOTES];
	chunk = new SkidderChunk(NUM_PARAMETERS, NUM_PROGRAMS, PLUGIN_ID, this);
	// since we don't use pitchbend for anything special, 
	// allow it be assigned to control parameters
	chunk->setAllowPitchbendEvents(true);
#endif

	suspend();	// initialize a fresh skid cycle

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

	srand((unsigned int)time(NULL));	// sets a seed value for rand() from the system clock

	editor = new SkidderEditor(this);
}

//-----------------------------------------------------------------------------------------
Skidder::~Skidder()
{
	if (programs)
		delete[] programs;

	// deallocate the memory from these arrays
	if (tempoRateTable)
		delete tempoRateTable;
#ifdef MSKIDDER
	if (noteTable)
		delete[] noteTable;
	if (chunk)
		delete chunk;
#endif

/* begin inter-plugin audio sharing stuff */
#ifdef HUNGRY
	if (foodEater)
		delete foodEater;
#endif
/* end inter-plugin audio sharing stuff */
}

//-----------------------------------------------------------------------------------------
// Destroy FX infos

bool Skidder::getEffectName(char *name) {
	#ifdef MSKIDDER
	strcpy (name, "Skidder");	// name max 32 char
	#else
	strcpy (name, "Skidder");	// name max 32 char
	#endif
	return true; }

long Skidder::getVendorVersion() {
	return PLUGIN_VERSION; }

bool Skidder::getErrorText(char *text) {
	strcpy (text, "This is the end of this plugin.");	// max 256 char
	return true; }

bool Skidder::getVendorString(char *text) {
	strcpy (text, "Destroy FX");	// a string identifying the vendor (max 64 char)
	return true; }

bool Skidder::getProductString(char *text) {
	// a string identifying the product name (max 64 char)
	strcpy (text, "Super Destroy FX bipolar VST plugin pack");
	return true; }

//------------------------------------------------------------------------
bool Skidder::getInputProperties(long index, VstPinProperties* properties)
{
	if ( (index >= 0) && (index < 2) )
	{
		sprintf(properties->label, "Skidder input %ld", index+1);
		sprintf(properties->shortLabel, "in %ld", index+1);
		properties->flags = kVstPinIsStereo | kVstPinIsActive;
		return true;
	}
	return false;
}

//------------------------------------------------------------------------
bool Skidder::getOutputProperties(long index, VstPinProperties* properties)
{
	if ( (index >= 0) && (index < 2) )
	{
		sprintf (properties->label, "Skidder output %ld", index+1);
		sprintf (properties->shortLabel, "out %ld", index+1);
		properties->flags = kVstPinIsStereo | kVstPinIsActive;
		return true;
	}
	return false;
}

//----------------------------------------------------------------------------- 
SkidderProgram::SkidderProgram()
{
	name = new char[32];
	param = new float[NUM_PARAMETERS];

	param[kRate] = rateUnscaled(3.0f);	// 0.1304347826087
	param[kTempoSync] = 0.0f;	// default to no tempo sync; "free" control
	param[kTempoRate] = 0.333f;
	param[kRateRandFactor] = 0.0f;	// default to no randomness
	param[kTempo] = 0.0f;	// default to "auto" (i.e. get it from the host)
	param[kPulsewidth] = 0.5f;
	param[kPulsewidthRandMin] = 0.5f;
	param[kSlope] = 3.0f/SLOPEMAX;	// 0.2
	param[kFloor] = 0.0f;
	param[kFloorRandMin] = 0.0f;
	param[kPan] = 0.0f;
	param[kNoise] = 0.0f;
#ifdef MSKIDDER
	param[kMidiMode] = 0.0f;	// default to no MIDI nuthin
	param[kVelocity] = 0.0f;	// default to not using note velocity
#ifdef HUNGRY
	param[kConnect] = 0.0f;	// default to disconnected
#endif
#endif

	strcpy(name, "default");
}

//----------------------------------------------------------------------------- 
SkidderProgram::~SkidderProgram()
{
	if (name)
		delete[] name;
	if (param)
		delete[] param;
}

//-----------------------------------------------------------------------------------------
// this gets called when the plugin is de-activated

void Skidder::suspend()
{
	state = valley;
	valleySamples = 0;
	panRander = 0.0f;
	rms = 0.0f;
	rmscount = 0;
	randomFloor = 0.0f;
	randomGainRange = 1.0f;

#ifdef MSKIDDER
	resetMidi();

#ifdef HUNGRY	// inter-plugin audio sharing stuff
	foodEater->suspend();
#endif

#endif
}

//-----------------------------------------------------------------------------
// this gets called when the plugin is activated
void Skidder::resume()
{
	needResync = true;	// some hosts may call resume when restarting playback

#ifdef MSKIDDER
	wantEvents();	// tells the host that we crave MIDI
	waitSamples = 0;
#endif
}

//----------------------------------------------------------------------------- 
void Skidder::setProgram(long programNum)
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
void Skidder::setProgramName(char *name)
{
	strcpy(programs[curProgram].name, name);
}

//-----------------------------------------------------------------------------------------
void Skidder::getProgramName(char *name)
{
	if ( !strcmp(programs[curProgram].name, "default") )
		sprintf(name, "default %ld", curProgram+1);
	else
		strcpy(name, programs[curProgram].name);
}

//-----------------------------------------------------------------------------------------
bool Skidder::getProgramNameIndexed(long category, long index, char *text)
{
	if ( (index < NUM_PROGRAMS) && (index >= 0) )
	{
		strcpy(text, programs[index].name);
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------------------
bool Skidder::copyProgram(long destination)
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
void Skidder::setParameter(long index, float value)
{
	switch (index)
	{
		case kRate              : fRate = value;				break;
		case kTempoSync :
			// set needResync true if tempo sync mode has just been switched on
			if ( onOffTest(value) && !onOffTest(fTempoSync) )
				needResync = true;
			fTempoSync = value;
			// the GUI may not correctly update the rate rand range display unless 
			// we tell it to because it may update before the tempo has been calculated
			mustUpdateTempoHasChanged = true;
			break;
		case kTempoRate:
			// make sure the cycles match up if the tempo rate has changed
			if (tempoRateTable->getScalar(fTempoRate) != tempoRateTable->getScalar(value))
				needResync = true;
			fTempoRate = value;
			break;
		case kRateRandFactor    : fRateRandFactor = value;		break;
		case kTempo:
			fTempo = value;
			tempoHasChanged = true;
			break;
		case kPulsewidth        : fPulsewidth = value;			break;
		case kPulsewidthRandMin : fPulsewidthRandMin = value;	break;
		case kSlope             : fSlope = value;				break;
		case kFloor             : fFloor = value;				break;
		case kFloorRandMin      : fFloorRandMin = value;		break;
		case kPan               : fPan = value;					break;
		case kNoise             : fNoise = value;				break;

#ifdef MSKIDDER
		case kMidiMode :
			// if we've just entered a MIDI mode, zero out all notes & reset waitSamples
			if ( (midiModeScaled(fMidiMode) == kNoMidiMode) && 
					(midiModeScaled(value) != kNoMidiMode) )
			{
				for (int i=0; (i < NUM_NOTES); i++)
					noteTable[i] = 0;
				noteOff();
			}
			fMidiMode = value;
			break;

		case kVelocity          : fVelocity = value;			break;

/* begin inter-plugin audio sharing stuff */
#ifdef HUNGRY
		case kConnect :
			foodEater->setParameter(value);
			break;
#endif
/* end inter-plugin audio sharing stuff */
#endif

		default :
			break;
	}

	if ( (index >= 0) && (index < NUM_PARAMETERS) )
		programs[curProgram].param[index] = value;

	if (editor)
		((AEffGUIEditor*)editor)->setParameter(index, value);
}

//-----------------------------------------------------------------------------------------
float Skidder::getParameter(long index)
{
	switch (index)
	{
		default:
		case kRate              : return fRate;
		case kTempoSync         : return fTempoSync;
		case kTempoRate         : return fTempoRate;
		case kRateRandFactor    : return fRateRandFactor;
		case kTempo             : return fTempo;
		case kPulsewidth        : return fPulsewidth;
		case kPulsewidthRandMin : return fPulsewidthRandMin;
		case kSlope             : return fSlope;
		case kFloor             : return fFloor;
		case kFloorRandMin      : return fFloorRandMin;
		case kPan               : return fPan;
		case kNoise             : return fNoise;

		#ifdef MSKIDDER
		case kMidiMode          : return fMidiMode;
		case kVelocity          : return fVelocity;
		#ifdef HUNGRY
		case kConnect           : return foodEater->fConnect;
		#endif
		#endif
	}
}

//-----------------------------------------------------------------------------------------
// titles of each parameter

void Skidder::getParameterName(long index, char *label)
{
	switch (index)
	{
		case kRate              : strcpy(label, "rate");					break;
		case kTempoSync         : strcpy(label, "tempo sync");				break;
		case kTempoRate         : strcpy(label, "tempo rate");				break;
		case kRateRandFactor    : strcpy(label, "rate random factor");		break;
		case kTempo             : strcpy(label, "tempo");					break;
		case kPulsewidth        : strcpy(label, "pulsewidth");				break;
		case kPulsewidthRandMin : strcpy(label, "pulsewidth random min.");	break;
		case kSlope             : strcpy(label, "slope");					break;
		case kFloor             : strcpy(label, "floor");					break;
		case kFloorRandMin      : strcpy(label, "floor random min.");		break;
		case kPan               : strcpy(label, "stereo spread");			break;
		case kNoise             : strcpy(label, "rupture");					break;

		#ifdef MSKIDDER
		case kMidiMode          : strcpy(label, "MIDI mode");				break;
		case kVelocity          : strcpy(label, "velocity");				break;
		#ifdef HUNGRY
		case kConnect :
			foodEater->getParameterName(label);
			break;
		#endif
		#endif
	}
}

//-----------------------------------------------------------------------------------------
// numerical display of each parameter's gradiations

void Skidder::getParameterDisplay(long index, char *text)
{
	switch (index)
	{
	case kRate :
		if (onOffTest(fTempoSync))
			strcpy(text, "off");
		else
			sprintf(text, "%.3f", rateScaled(fRate));
		break;
	case kTempoSync :
		if (onOffTest(fTempoSync))
			strcpy(text, "yes");
		else
			strcpy(text, "no");
		break;
	case kTempoRate :
		if (onOffTest(fTempoSync))
			strcpy(text, tempoRateTable->getDisplay(fTempoRate));
		else
			strcpy(text, "off");
		break;

	case kRateRandFactor :
		sprintf(text, "%.3f", rateRandFactorScaled(fRateRandFactor));
		break;

	case kTempo :
		if ( (fTempo > 0.0f) || (hostCanDoTempo != 1) )
			sprintf(text, "%.3f", tempoScaled(fTempo));
		else
			strcpy(text, "auto");
		break;
	case kPulsewidth :
		sprintf(text, "%.3f", pulsewidthScaled(fPulsewidth));
		break;

	case kPulsewidthRandMin :
		if (fPulsewidth > fPulsewidthRandMin)
			sprintf(text, "%.3f", pulsewidthScaled(fPulsewidthRandMin));
		else
			strcpy(text, "no randoms");
		break;

	case kSlope :
		sprintf(text, "%.3f", (fSlope*SLOPEMAX));
		break;

	case kFloor :
		if (fFloor <= 0.0f)
			strcpy(text, "-oo");
		else
			sprintf(text, "%.2f", dBconvert(floor));
		break;

	case kFloorRandMin :
		if (fFloorRandMin <= 0.0f)
			strcpy(text, "-oo");
		else
			sprintf(text, "%.2f", dBconvert(gainScaled(fFloorRandMin)));
		break;

	case kPan :
		sprintf(text, "%.3f", fPan);
		break;
	case kNoise :
		sprintf(text, "%ld", (long)(fNoise_squared*18732));
		break;

#ifdef MSKIDDER
	case kMidiMode :
		switch (midiModeScaled(fMidiMode))
		{
			case kMidiTrigger:	strcpy(text, "trigger");	break;
			case kMidiApply:	strcpy(text, "apply");		break;
			default:			strcpy(text, "none");		break;
		}
		break;
	case kVelocity :
		if (onOffTest(fVelocity))
			strcpy(text, "use");
		else
			strcpy(text, "don\'t use");
		break;

#ifdef HUNGRY
	case kConnect :
		foodEater->getParameterDisplay(text);
		break;
#endif
#endif
	}
}

//-----------------------------------------------------------------------------------------
// unit of measure for each parameter

void Skidder::getParameterLabel(long index, char *label)
{
long thisSamplePosition = reportCurrentPosition();	// needed to feed the next function
long randy;

	switch (index)
	{
		case kRate              : strcpy(label, "Hz");			break;
		case kTempoSync         : strcpy(label, " ");			break;
		case kTempoRate         : strcpy(label, "cycles/beat");	break;
		case kRateRandFactor    : strcpy(label, " ");			break;
		case kTempo             : strcpy(label, "bpm");			break;
		case kPulsewidth        : strcpy(label, "% of cycle");	break;
		case kPulsewidthRandMin : strcpy(label, "% of cycle");	break;
		case kSlope             : strcpy(label, "ms");			break;
		case kFloor             : strcpy(label, "dB");			break;
		case kFloorRandMin      : strcpy(label, "dB");			break;
		case kPan               : strcpy(label, "amount");		break;
		case kNoise :
			randy = rand();
			if ( randy <= (long)(RAND_MAX*0.4) )
				getHostProductString(label);
			if ( (randy > (long)(RAND_MAX*0.4)) && (randy <= (long)(RAND_MAX*0.7)) )
				getHostVendorString(label);
			if ( (randy > (long)(RAND_MAX*0.7)) && (randy <= (long)(RAND_MAX*0.8)) )
			{
				switch (hostCanDoTempo)
					{
					case -1 : strcpy(label, "no");		break;
					case 0 : strcpy(label, "perhaps");	break;
					case 1 : strcpy(label, "yes");		break;
					}
			}
			if ( (randy > (long)(RAND_MAX*0.8)) && (randy <= (long)(RAND_MAX*0.9)) )
				sprintf(label, "%.3f", ((float)tempoAt(thisSamplePosition)) / 10000.0);	// tempo in beats per second
			if ( (randy > (long)(RAND_MAX*0.9)) && (randy <= (long)RAND_MAX) )
				sprintf(label, "%ld", getHostVendorVersion());
			break;

	#ifdef MSKIDDER
		case kMidiMode : strcpy(label, " ");	break;
		case kVelocity : strcpy(label, " ");	break;
	#ifdef HUNGRY
		case kConnect : strcpy(label, " ");		break;
	#endif
	#endif
	}
}

//-----------------------------------------------------------------------------------------
// this just tells the host that Skidder receives VstTimeInfo & mSkidder also receives MIDI data

long Skidder::canDo(char* text)
{
#ifdef MSKIDDER
	if (strcmp(text, "receiveVstEvents") == 0)
		return 1;
	if (strcmp(text, "receiveVstMidiEvent") == 0)
		return 1;
#endif

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
	if (strcmp(text, "1in2out") == 0)
		return 1;
	if (strcmp(text, "2in2out") == 0)
		return 1;

	return -1;	// explicitly can't do; 0 => don't know
}



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

	AudioEffect* effect = new Skidder(audioMaster);
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
