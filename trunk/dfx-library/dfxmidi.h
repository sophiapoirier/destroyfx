/*---------------------------------------------------------------
    Marc's Destroy FX MIDI stuff --- happened February 2001
---------------------------------------------------------------*/

#ifndef __DFXMIDI_H
#define __DFXMIDI_H


//----------------------------------------------------------------------------- 
// enums

// these are the MIDI event status types
enum
{
	kMidiNoteOff = 0x80,
	kMidiNoteOn = 0x90,
	kMidiPolyphonicAftertouch = 0xA0,
	kMidiCC = 0xB0,
	kMidiProgramChange = 0xC0,
	kMidiChannelAftertouch = 0xD0,
	kMidiPitchbend = 0xE0,

	kMidiSysEx = 0xF0,
	kMidiTimeCode = 0xF1,
	kMidiSongPositionPointer = 0xF2,
	kMidiSongSelect = 0xF3,
	kMidiTuneRequest = 0xF6,
	kMidiEndOfSysex = 0xF7,
	kMidiTimingClock = 0xF8,
	kMidiStart = 0xFA,
	kMidiContinue = 0xFB,
	kMidiStop = 0xFC,
	kMidiActiveSensing = 0xFE,
	kMidiSystemReset = 0xFF,

	kInvalidMidi = -3	// for whatever
};

// these are the MIDI continuous controller messages (CCs)
enum
{
	kMidiCC_BankSelect = 0x00,
	kMidiCC_ModWheel = 0x01,
	kMidiCC_BreathControl = 0x02,
	kMidiCC_FootControl = 0x04,
	kMidiCC_PortamentoTime = 0x05,
	kMidiCC_DataEntry = 0x06,
	kMidiCC_ChannelVolume = 0x07,
	kMidiCC_Balance = 0x08,
	kMidiCC_Pan = 0x0A,
	kMidiCC_ExpressionController = 0x0B,
	kMidiCC_EffectControl1 = 0x0C,
	kMidiCC_EffectControl2 = 0x0D,
	kMidiCC_GeneralPurposeController1 = 0x10,
	kMidiCC_GeneralPurposeController2 = 0x11,
	kMidiCC_GeneralPurposeController3 = 0x12,
	kMidiCC_GeneralPurposeController4 = 0x13,

	// on/off CCs   ( <= 63 is off, >= 64 is on )
	kMidiCC_SustainPedalOnOff = 0x40,
	kMidiCC_PortamentoOnOff = 0x41,
	kMidiCC_SustenutoOnOff = 0x42,
	kMidiCC_SoftPedalOnOff = 0x43,
	kMidiCC_LegatoFootswitch = 0x44,
	kMidiCC_Hold2 = 0x45,

	kMidiCC_SoundController1_soundVariation = 0x46,
	kMidiCC_SoundController2_timbre = 0x47,
	kMidiCC_SoundController3_releaseTime = 0x48,
	kMidiCC_SoundController4_attackTime = 0x49,
	kMidiCC_SoundController5_brightness = 0x4A,
	kMidiCC_SoundController6 = 0x4B,
	kMidiCC_SoundController7 = 0x4C,
	kMidiCC_SoundController8 = 0x4D,
	kMidiCC_SoundController9 = 0x4E,
	kMidiCC_SoundController10 = 0x4F,
	kMidiCC_GeneralPurposeController5 = 0x50,
	kMidiCC_GeneralPurposeController6 = 0x51,
	kMidiCC_GeneralPurposeController7 = 0x52,
	kMidiCC_GeneralPurposeController8 = 0x53,
	kMidiCC_PortamentoControl = 0x54,
	kMidiCC_Effects1Depth = 0x5B,
	kMidiCC_Effects2Depth = 0x5C,
	kMidiCC_Effects3Depth = 0x5D,
	kMidiCC_Effects4Depth = 0x5E,
	kMidiCC_Effects5Depth = 0x5F,
	kMidiCC_DataEntryPlus1 = 0x60,
	kMidiCC_DataEntryMinus1 = 0x61,

	// sepcial commands
	kMidiCC_AllSoundOff = 0x78,	// 0 only
	kMidiCC_ResetAllControllers = 0x79,	// 0 only
	kMidiCC_LocalControlOnOff = 0x7A,	// 0 = off, 127 = on
	kMidiCC_AllNotesOff = 0x7B,	// 0 only
	kMidiCC_OmniModeOff = 0x7C,	// 0 only
	kMidiCC_OmniModeOn = 0x7D,	// 0 only
	kMidiCC_PolyModeOnOff = 0x7E,
	kMidiCC_PolyModeOn = 0x7F	// 0 only
};

//----------------------------------------------------------------------------- 
// constants & macros

const long NUM_FADE_POINTS = 30000;
const double FADE_CURVE = 2.7;

const double PITCHBEND_MAX = 36.0;

// 128 midi notes
const long NUM_NOTES = 128;
// 12th root of 2
const double NOTE_UP_SCALAR = 1.059463094359295264561825294946;
const double NOTE_DOWN_SCALAR = 0.94387431268169349664191315666792;
const float MIDI_SCALAR = 1.0f / 127.0f;

const long STOLEN_NOTE_FADE_DUR = 48;
const float STOLEN_NOTE_FADE_STEP = 1.0f / (float)STOLEN_NOTE_FADE_DUR;
const long LEGATO_FADE_DUR = 39;
const float LEGATO_FADE_STEP = 1.0f / LEGATO_FADE_DUR;

const long EVENTS_QUEUE_SIZE = 12000;

inline bool isNote(int midiStatus)
{
	return (midiStatus == kMidiNoteOn) || (midiStatus == kMidiNoteOff);
}


//----------------------------------------------------------------------------- 
// types

// this holds MIDI event information
typedef struct {
	int status;	// the event status MIDI byte
	int byte1;	// the first MIDI data byte
	int byte2;	// the second MIDI data byte
	long delta;	// the delta offset (the sample position in the current block where the event occurs)
	int channel;	// the MIDI channel
} DfxMidiEvent;

// this holds information for each MIDI note
typedef struct {
	int velocity;	// note velocity - 7-bit MIDI value
	float noteAmp;	// the gain for the note, scaled with velocity, curve, & influence
	long attackDur;	// duration, in samples, of the attack phase
	long attackSamples;	// current position in the attack phase
	long releaseDur;	// duration, in samples, of the release phase
	long releaseSamples;	// current position in the release phase
	float fadeTableStep;	// the step increment for each envelope step using the fade table
	float linearFadeStep;	// the step increment for each linear envelope step
	float lastOutValue;	// capture the most recent output value for smoothing, if necessary
	long smoothSamples;	// counter for quickly fading cut-off notes, for smoothity
	float * tail1;	// a little buffer of output samples for smoothing a cut-off note (left channel)
	float * tail2;	// (right channel)
} NoteTable;


//-----------------------------------------------------------------------------

class DfxMidi
{
public:
	DfxMidi();
	~DfxMidi();

	void reset();	// resets the variables
	void clearTail(int currentNote);	// zero out a note's tail buffers
	void setLazyAttack(bool newMode=true) { lazyAttackMode = newMode; };

	bool incNumEvents();	// increment the numBlockEvents counter, safely

	// handlers for the types of MIDI events that we support
	void handleNoteOn(int channel, int note, int velocity, long frameOffset);
	void handleNoteOff(int channel, int note, int velocity, long frameOffset);
	void handleAllNotesOff(int channel, long frameOffset);
	void handlePitchBend(int channel, int valueLSB, int valueMSB, long frameOffset);
	void handleCC(int channel, int controllerNum, int value, long frameOffset);
	void handleProgramChange(int channel, int programNum, long frameOffset);

	void preprocessEvents();
	void postprocessEvents();

	// this is where new MIDI events are reckoned with during audio processing
	void heedEvents(long eventNum, float SAMPLERATE, double pitchbendRange, float attack, float release, 
					bool legato, float velCurve, float velInfluence);

	// these are for manage the ordered queue of active MIDI notes
	void insertNote(int currentNote);
	void removeNote(int currentNote);
	void removeAllNotes();

	// public variables
	NoteTable * noteTable;	// a table with important data about each note
	DfxMidiEvent * blockEvents;	// the new MIDI events for a given processing block
	long numBlockEvents;	// the number of new MIDI events in a given processing block
	int * noteQueue;		// a chronologically ordered queue of all active notes
	double * freqTable;	// a table of the frequency corresponding to each MIDI note
	double pitchbend;		// a frequency scalar value for the current pitchbend setting


	//-------------------------------------------------------------------------
	// this function calculates fade scalars if attack or release are happening
	float processEnvelope(bool fades, int currentNote)
	{
		NoteTable * note = &noteTable[currentNote];

		// if attack is in progress
		if (note->attackDur > 0)
		{
			(note->attackSamples)++;
			// zero things out if the attack is over so we won't do this fade calculation next time
			if (note->attackSamples >= note->attackDur)
			{
				note->attackDur = 0;
				return 1.0f;
			}

			if (fades)	// use nice, exponential fading
				return fadeTable[ (long) ((float)(note->attackSamples) * note->fadeTableStep) ];
			else	// bad, linear fade
				return (float)(note->attackSamples) * note->linearFadeStep;
				// exponential sine fade (stupendously inefficient)
//				envAmp = ( sin((envAmp*PI)-(PI*0.5)) + 1.0 ) * 0.5;
//				envAmp *= envAmp;	// squared
		}

		// if release is in progress
		else if (note->releaseDur)
		{
			(note->releaseSamples)--;
			// zero things out if the release is over so we won't do this fade calculation next time
			// & turn this note off
			if (note->releaseSamples <= 0)
			{
				note->releaseDur = 0;
				note->velocity = 0;
				return 0.0f;
			}

			if (fades)	// use nice, exponential fading
				return fadeTable[ (long) ((float)(note->releaseSamples) * note->fadeTableStep) ];
			else	// use bad fade
				return (float)(note->releaseSamples) * note->linearFadeStep;
				// exponential sine fade
//				envAmp = ( sinf((envAmp*PI)-(PI*0.5f)) + 1.0f ) * 0.5f;
//				envAmp *= envAmp;	// squared
		}

		// since it's possible for the release to end & the note to turn off 
		// during this processing buffer, we have to check for that & then return 0.0
		else if ( (note->velocity) == 0 )
			return 0.0f;

		// just send 1.0 no fades or note-offs are happening
		return 1.0f;
	}


	//-------------------------------------------------------------------------
	// this function writes the audio output for smoothing the tips of cut-off notes
	// by sloping down from the last sample outputted by the note
	void processSmoothingOutputSample(float * out, long sampleFrames, int currentNote)
	{
		for (long samplecount=0; (samplecount < sampleFrames); samplecount++)
		{
			// add the latest sample to the output collection, scaled by the note envelope & user gain
			out[samplecount] += noteTable[currentNote].lastOutValue * 
								(float)noteTable[currentNote].smoothSamples * STOLEN_NOTE_FADE_STEP;
			// decrement the smoothing counter
			(noteTable[currentNote].smoothSamples)--;
			// exit this function if we've done all of the smoothing necessary
			if (noteTable[currentNote].smoothSamples <= 0)
				return;
		}
	}


	//-------------------------------------------------------------------------
	// this function writes the audio output for smoothing the tips of cut-off notes
	// by fading out the samples stored in the tail buffers
	void processSmoothingOutputBuffer(float * out, long sampleFrames, int currentNote, int channel)
	{
		long * smoothsamples = &(noteTable[currentNote].smoothSamples);
		float * tail = (channel == 1) ? noteTable[currentNote].tail1 : noteTable[currentNote].tail2;

		for (long samplecount=0; (samplecount < sampleFrames); samplecount++)
		{
			out[samplecount] += tail[STOLEN_NOTE_FADE_DUR-(*smoothsamples)] * 
								(float)(*smoothsamples) * STOLEN_NOTE_FADE_STEP;
			(*smoothsamples)--;
			if (*smoothsamples <= 0)
				return;
		}
	}



private:
	// initializations
	void fillFrequencyTable();
	void fillFadeTable();

	void turnOffNote(int currentNote, float release, bool legato, float SAMPLERATE);

	// a queue of note-offs for when the sustain pedal is active
	bool * sustainQueue;
	// an exponentially curved gain envelope
	float * fadeTable;

	// if the MIDI hardware uses it
	bool usePitchbendLSB;

	// pick up where the release left off, if it's still releasing
	bool lazyAttackMode;
	// sustain pedal is active
	bool sustain;

	// the offset of the most recent MIDI program change message
	long latestMidiProgramChange;
};

#endif
