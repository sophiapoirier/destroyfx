/*---------------------------------------------------------------
Destroy FX Library (version 1.0) is a collection of foundation code 
for creating audio software plug-ins.  
Copyright (C) 2001-2009  Sophia Poirier

This program is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

This program is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with this program.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, please visit http://destroyfx.org/ 
and use the contact form.

Sophia's Destroy FX MIDI stuff --- happened February 2001
---------------------------------------------------------------*/

#include "dfxmidi.h"

#include <stdlib.h>
#include <math.h>


//------------------------------------------------------------------------
DfxMidi::DfxMidi()
{
	// allocate memory for these arrays
	noteTable = (NoteTable*) malloc(sizeof(NoteTable) * NUM_NOTES);
	noteQueue = (int*) malloc(sizeof(int) * NUM_NOTES);
	sustainQueue = (bool*) malloc(sizeof(bool) * NUM_NOTES);
	freqTable = (double*) malloc(sizeof(double) * NUM_NOTES);
	fadeTable = (float*) malloc(sizeof(float) * NUM_FADE_POINTS);
	blockEvents = (DfxMidiEvent*) malloc(sizeof(DfxMidiEvent) * EVENTS_QUEUE_SIZE);

	for (int i=0; i < NUM_NOTES; i++)
	{
		noteTable[i].tail1 = (float*) malloc(sizeof(float) * STOLEN_NOTE_FADE_DUR);
		noteTable[i].tail2 = (float*) malloc(sizeof(float) * STOLEN_NOTE_FADE_DUR);
	}

	fillFrequencyTable();
	fillFadeTable();

	lazyAttackMode = false;

	reset();
}

//------------------------------------------------------------------------
DfxMidi::~DfxMidi()
{
	for (int i=0; i < NUM_NOTES; i++)
	{
		if (noteTable[i].tail1 != NULL)
			free(noteTable[i].tail1);
		noteTable[i].tail1 = NULL;
		if (noteTable[i].tail2 != NULL)
			free(noteTable[i].tail2);
		noteTable[i].tail2 = NULL;
	}

	// deallocate the memory from these arrays
	if (noteTable != NULL)
		free(noteTable);
	noteTable = NULL;
	if (noteQueue != NULL)
		free(noteQueue);
	noteQueue = NULL;
	if (sustainQueue != NULL)
		free(sustainQueue);
	sustainQueue = NULL;
	if (freqTable != NULL)
		free(freqTable);
	freqTable = NULL;
	if (fadeTable != NULL)
		free(fadeTable);
	fadeTable = NULL;
	if (blockEvents != NULL)
		free(blockEvents);
	blockEvents = NULL;
}

//------------------------------------------------------------------------
void DfxMidi::reset()
{
	// zero out the note table, or what's important at least
	for (int i=0; i < NUM_NOTES; i++)
	{
		noteTable[i].velocity = 0;
		noteTable[i].attackSamples = 0;
		noteTable[i].attackDur = 0;
		noteTable[i].releaseSamples = 0;
		noteTable[i].releaseDur = 0;
		noteTable[i].lastOutValue = 0.0f;
		noteTable[i].smoothSamples = 0;
		clearTail(i);
		sustainQueue[i] = false;
	}

	// clear the ordered note queue
	removeAllNotes();

	// reset this counter since processEvents may not be called during the first block
	numBlockEvents = 0;
	// reset the pitchbend value to no bend
	pitchbend = 1.0;
	// turn sustain pedal off
	sustain = false;
}

//------------------------------------------------------------------------
void DfxMidi::preprocessEvents()
{
	// Sort the events in our queue so that their in chronological order.  (bubble sort)
	// The host is supposed to send them in order, but just in case...
	for (long i=0; i < (numBlockEvents-1); i++)
	{
		// default it to true and change it to false if the next loop finds unsorted items
		bool sorted = true;
		//
		for (long j=0; j < (numBlockEvents-1-i); j++)
		{
			// swap the neighbors if they're out of order
			if (blockEvents[j+1].delta < blockEvents[j].delta)
			{
				DfxMidiEvent tempEvent = blockEvents[j];
				blockEvents[j] = blockEvents[j+1];
				blockEvents[j+1] = tempEvent;
				sorted = false;
			}
		}
		//
		// no need to go through all (numBlockEvents-1)! iterations if the array is fully sorted already
		if (sorted)
			break;
	}
}

//------------------------------------------------------------------------
// zeroing numBlockEvents must be done at the end of each audio processing block 
// so that events are not reused in the next block(s) if no new events arrive
void DfxMidi::postprocessEvents()
{
	numBlockEvents = 0;
}

//------------------------------------------------------------------------
void DfxMidi::clearTail(int inCurrentNote)
{
	for (int i=0; i < STOLEN_NOTE_FADE_DUR; i++)
		noteTable[inCurrentNote].tail1[i] = 0.0f;
	for (int j=0; j < STOLEN_NOTE_FADE_DUR; j++)
		noteTable[inCurrentNote].tail2[j] = 0.0f;
}

//-----------------------------------------------------------------------------------------
// this function fills a table with the correct frequency for every MIDI note
void DfxMidi::fillFrequencyTable()
{
	double baseNote = 6.875;	// A
	baseNote *= NOTE_UP_SCALAR;	// A#
	baseNote *= NOTE_UP_SCALAR;	// B
	baseNote *= NOTE_UP_SCALAR;	// C, frequency of midi note 0
	for (int i = 0; i < NUM_NOTES; i++)	// 128 midi notes
	{
		freqTable[i] = baseNote;
		baseNote *= NOTE_UP_SCALAR;
	}
}


//-----------------------------------------------------------------------------------------
// this function makes a fade curve table for using when scaling during attack and release
void DfxMidi::fillFadeTable()
{
	double fadeCurveStep = 1.0 / (double)(NUM_FADE_POINTS-1);
	for (long i = 0; i < NUM_FADE_POINTS; i++)
	{
		fadeTable[i] = (float) pow( (double)i * fadeCurveStep, FADE_CURVE );
		// zero any near-denormal values
		if (fadeTable[i] < 1e-15)
			fadeTable[i] = 0.0f;
	}
}


//-----------------------------------------------------------------------------
// this function inserts a new note into the beginning of the active notes queue
void DfxMidi::insertNote(int inCurrentNote)
{
	// first check whether this note is already active (could happen in weird sequencers, like Max for example)
	for (int notecount = 0; notecount < NUM_NOTES; notecount++)
	{
		// we've looked at all active notes and didn't find the current one, so escape this for loop...
		if (noteQueue[notecount] < 0)
			break;
		// the current note is already active ...
		if (noteQueue[notecount] == inCurrentNote)
		{
			// ... so shift all of the notes before it up one position ...
			while (notecount > 0)
			{
				noteQueue[notecount] = noteQueue[notecount-1];
				notecount--;
			}
			// ... and then re-insert the current note as the first note
			noteQueue[0] = inCurrentNote;
			return;
		}
	}

	// shift every note up a position   (normal scenario)
	for (int nc = NUM_NOTES - 1; nc > 0; nc--)
		noteQueue[nc] = noteQueue[nc - 1];
	// then place the new note into the first position
	noteQueue[0] = inCurrentNote;
}

//-----------------------------------------------------------------------------
// this function removes a note from the active notes queue
void DfxMidi::removeNote(int inCurrentNote)
{
	bool doShift = false;
	for (int notecount = 0; notecount < (NUM_NOTES-1); notecount++)
	{
		// don't do anything until the note to delete is found
		if (noteQueue[notecount] == inCurrentNote)
			doShift = true;
		// start shifting notes down past the point of the deleted note
		if (doShift)
			noteQueue[notecount] = noteQueue[notecount+1];
		// we've reached the last active note in the table, so there's no need to shift notes down anymore
		if (noteQueue[notecount] < 0)
			break;
	}

	// this much must be true if we've just deleted a note, and it can't happen in the previous loop
	noteQueue[NUM_NOTES-1] = kInvalidMidi;
}

//-----------------------------------------------------------------------------
// this function cancels all of the notes in the active notes queue
void DfxMidi::removeAllNotes()
{
	for (int notecount = 0; notecount < NUM_NOTES; notecount++)
		noteQueue[notecount] = kInvalidMidi;
}


//-----------------------------------------------------------------------------
bool DfxMidi::incNumEvents()
{
	numBlockEvents++;
	// don't go past the allocated space for the events queue
	if (numBlockEvents >= EVENTS_QUEUE_SIZE)
	{
		numBlockEvents = EVENTS_QUEUE_SIZE - 1;
		return false;	// ! abort !
	}
	return true;	// successful increment
}

//-----------------------------------------------------------------------------
void DfxMidi::handleNoteOn(int inMidiChannel, int inNoteNumber, int inVelocity, long inBufferOffset)
{
	blockEvents[numBlockEvents].status = kMidiNoteOn;
	blockEvents[numBlockEvents].channel = inMidiChannel;
	blockEvents[numBlockEvents].byte1 = inNoteNumber;
	blockEvents[numBlockEvents].byte2 = inVelocity;
	blockEvents[numBlockEvents].delta = inBufferOffset;
	incNumEvents();
}

//-----------------------------------------------------------------------------
void DfxMidi::handleNoteOff(int inMidiChannel, int inNoteNumber, int inVelocity, long inBufferOffset)
{
	blockEvents[numBlockEvents].status = kMidiNoteOff;
	blockEvents[numBlockEvents].channel = inMidiChannel;
	blockEvents[numBlockEvents].byte1 = inNoteNumber;
	blockEvents[numBlockEvents].byte2 = inVelocity;
	blockEvents[numBlockEvents].delta = inBufferOffset;
	incNumEvents();
}

//-----------------------------------------------------------------------------
void DfxMidi::handleAllNotesOff(int inMidiChannel, long inBufferOffset)
{
	blockEvents[numBlockEvents].status = kMidiCC;
	blockEvents[numBlockEvents].byte1 = kMidiCC_AllNotesOff;
	blockEvents[numBlockEvents].channel = inMidiChannel;
	blockEvents[numBlockEvents].delta = inBufferOffset;
	incNumEvents();
}

//-----------------------------------------------------------------------------
void DfxMidi::handlePitchBend(int inMidiChannel, int inValueLSB, int inValueMSB, long inBufferOffset)
{
	blockEvents[numBlockEvents].status = kMidiPitchbend;
	blockEvents[numBlockEvents].channel = inMidiChannel;
	blockEvents[numBlockEvents].byte1 = inValueLSB;
	blockEvents[numBlockEvents].byte2 = inValueMSB;
	blockEvents[numBlockEvents].delta = inBufferOffset;
	incNumEvents();
}

//-----------------------------------------------------------------------------
void DfxMidi::handleCC(int inMidiChannel, int inControllerNumber, int inValue, long inBufferOffset)
{
	// only handling sustain pedal for now...
	if (inControllerNumber == kMidiCC_SustainPedalOnOff)
	{
		blockEvents[numBlockEvents].status = kMidiCC;
		blockEvents[numBlockEvents].byte1 = inControllerNumber;
		blockEvents[numBlockEvents].channel = inMidiChannel;
		blockEvents[numBlockEvents].byte2 = inValue;	// <= 63 is off, >= 64 is on
		blockEvents[numBlockEvents].delta = inBufferOffset;
		incNumEvents();
	}
}

//-----------------------------------------------------------------------------
void DfxMidi::handleProgramChange(int inMidiChannel, int inProgramNumber, long inBufferOffset)
{
	blockEvents[numBlockEvents].status = kMidiProgramChange;
	blockEvents[numBlockEvents].channel = inMidiChannel;
	blockEvents[numBlockEvents].byte1 = inProgramNumber;
	blockEvents[numBlockEvents].delta = inBufferOffset;
	incNumEvents();
}


//-----------------------------------------------------------------------------------------
// this function is called during process() when MIDI events need to be attended to
void DfxMidi::heedEvents(long inEventNum, float inSampleRate, double inPitchbendRange, float inAttackDur, 
							float inReleaseDur, bool inLegato, float inVelocityCurve, float inVelocityInfluence)
{
	switch (blockEvents[inEventNum].status)
	{

// --- NOTE-ON RECEIVED ---
		case kMidiNoteOn:
			{
				int currentNote = blockEvents[inEventNum].byte1;
				noteTable[currentNote].velocity = blockEvents[inEventNum].byte2;
				noteTable[currentNote].noteAmp = ( (float)pow(MIDI_SCALAR * (float)(noteTable[currentNote].velocity), inVelocityCurve) * 
												inVelocityInfluence ) + (1.0f - inVelocityInfluence);
				//
				if (inLegato)	// legato is on, fade out the last not and fade in the new one, supershort
				{
					// this is false until we find some already active note
					bool legatoNoteFound = false;
					// find the previous note and set it to fade out
					for (int notecount=0; notecount < NUM_NOTES; notecount++)
					{
						// we want to find the active note, but not this new one
						if ( (noteTable[notecount].velocity) && (notecount != currentNote) && (noteTable[notecount].releaseDur == 0) )
						{
							// if the note is currently fading in, pick up where it left off
							if (noteTable[notecount].attackDur)
								noteTable[notecount].releaseSamples = noteTable[notecount].attackSamples;
							// otherwise do the full fade out duration, if the note is not already fading out
							else if ( (noteTable[notecount].releaseSamples) <= 0 )
								noteTable[notecount].releaseSamples = LEGATO_FADE_DUR;
							noteTable[notecount].releaseDur = LEGATO_FADE_DUR;
							noteTable[notecount].attackDur = 0;
							noteTable[notecount].attackSamples = 0;
							noteTable[notecount].fadeTableStep = (float)NUM_FADE_POINTS / (float)LEGATO_FADE_DUR;
							noteTable[notecount].linearFadeStep = LEGATO_FADE_STEP;
							// we found an active note (that's the same as the new incoming note)
							legatoNoteFound = true;
						}
					}
					// don't start a new note fade-in if the currently active note is the same as this new note
					if (! ((!legatoNoteFound) && (noteTable[currentNote].velocity)) )
					{
						// legato mode always uses this short fade
						noteTable[currentNote].attackDur = LEGATO_FADE_DUR;
						// attackSamples starts counting from zero, so set it to zero
						noteTable[currentNote].attackSamples = 0;
						// calculate how far this fade must "step" through the fade table at each sample.
						// Since legato mode overrides the fades parameter and only does cheap fades, this 
						// isn't really necessary, but since I don't trust that everything will work right...
						noteTable[currentNote].fadeTableStep = (float)NUM_FADE_POINTS / (float)LEGATO_FADE_DUR;
						noteTable[currentNote].linearFadeStep = LEGATO_FADE_STEP;
					}
				}
				//
				else	// legato is off, so set up for the attack envelope
				{
					// calculate the duration, in samples, for the attack
					long attackdur = (long) (inAttackDur * inSampleRate);
					noteTable[currentNote].attackDur = attackdur;
					if (attackdur)	// avoid potential division by zero
					{
						// calculate how far this fade must "step" through the fade table at each sample
						noteTable[currentNote].fadeTableStep = (float)NUM_FADE_POINTS / (float)attackdur;
						noteTable[currentNote].linearFadeStep = 1.0f / (float)attackdur;
					}
					// if using lazyAttackMode and this note is already sounding and in release, pick up from where it is
					if ( lazyAttackMode && (noteTable[currentNote].releaseDur > 0) )
						noteTable[currentNote].attackSamples = (long) ( (float)(noteTable[currentNote].releaseSamples) / 
																(float)(noteTable[currentNote].releaseDur) * (float)attackdur );
					else	// regular
					{
						// attackSamples starts counting from zero, so set it to zero
						noteTable[currentNote].attackSamples = 0;
						// if the note is still sounding and in release, then kick smooth the end of that last note
						if (noteTable[currentNote].releaseDur > 0)
							noteTable[currentNote].smoothSamples = STOLEN_NOTE_FADE_DUR;
					}
				}
				// now we've checked the fade state, so we can zero these out to turn this note's release
				noteTable[currentNote].releaseDur = 0;
				noteTable[currentNote].releaseSamples = 0;
			}
			break;



// --- NOTE-OFF RECEIVED ---
		case kMidiNoteOff:
			{
				int currentNote = blockEvents[inEventNum].byte1;
				// don't process this note off, but do remember it, if the sustain pedal is on
				if (sustain)
					sustainQueue[currentNote] = true;
				else
					turnOffNote(currentNote, inReleaseDur, inLegato, inSampleRate);
			}
			break;



// --- PITCHBEND RECEIVED ---
		case kMidiPitchbend:
			{
				long pitchbend14bit = (blockEvents[inEventNum].byte2 * 0x80) + blockEvents[inEventNum].byte1;
				// no bend
				if (pitchbend14bit == kDfxMidi_PitchbendMiddleValue)
				{
					pitchbend = 1.0;
				}
				// bend pitch up
				else if (pitchbend14bit > kDfxMidi_PitchbendMiddleValue)
				{
					// scale the MIDI value from 0.0 to 1.0
					pitchbend = (double)(pitchbend14bit - kDfxMidi_PitchbendMiddleValue) / (double)(kDfxMidi_PitchbendMiddleValue - 1);
					// then scale it according to tonal steps and the user defined range
					pitchbend = pow(NOTE_UP_SCALAR, pitchbend*inPitchbendRange);
				}
				// bend pitch down
				else
				{
					// scale the MIDI value from 1.0 to 0.0
					pitchbend = (double)(-(pitchbend14bit - kDfxMidi_PitchbendMiddleValue)) / (double)kDfxMidi_PitchbendMiddleValue;
					// then scale it according to tonal steps and the user defined range
					pitchbend = pow(NOTE_DOWN_SCALAR, pitchbend*inPitchbendRange);
				}
			}
			break;



// --- CONTROLLER CHANGE RECEIVED ---
		case kMidiCC:
		{
			switch (blockEvents[inEventNum].byte1)
			{
	// --- SUSTAIN PEDAL RECEIVED ---
				case kMidiCC_SustainPedalOnOff:
					if ( sustain && (blockEvents[inEventNum].byte2 <= 63) )
					{
						for (int i=0; i < NUM_NOTES; i++)
						{
							if (sustainQueue[i])
							{
								turnOffNote(i, inReleaseDur, inLegato, inSampleRate);
								sustainQueue[i] = false;
							}
						}
					}
					sustain = (blockEvents[inEventNum].byte2 >= 64);
					break;

	// --- ALL-NOTES-OFF RECEIVED ---
				// all sound off, so call suspend() to wipe out all of the notes and buffers
				case kMidiCC_AllNotesOff:
					{
						// and zero out the note table, or what's important at least
						for (int i=0; i < NUM_NOTES; i++)
						{
							noteTable[i].velocity = 0;
							noteTable[i].attackSamples = 0;
							noteTable[i].attackDur = 0;
							noteTable[i].releaseSamples = 0;
							noteTable[i].releaseDur = 0;
						}
					}
					break;
			}
			break;	// kMidiCC
		}

// --- nothingness ---
		default:
			break;
	}
}


//-----------------------------------------------------------------------------------------
void DfxMidi::turnOffNote(int inCurrentNote, float inReleaseDur, bool inLegato, float inSampleRate)
{
	// legato is off (note-offs are ignored when it's on)
	// go into the note release if legato is off and the note isn't already off
	if ( (!inLegato) && (noteTable[inCurrentNote].velocity > 0) )
	{
		// calculate the duration, in samples, for the release
		long releasedur = (long)(inReleaseDur * inSampleRate);
		noteTable[inCurrentNote].releaseDur = releasedur;
		// this note is already sounding and in attack, so pick up from where it is
		if (noteTable[inCurrentNote].attackDur)
			noteTable[inCurrentNote].releaseSamples = (long) 
				( (float)(noteTable[inCurrentNote].attackSamples) / (float)(noteTable[inCurrentNote].attackDur) * (float)releasedur );
		else	// regular
			noteTable[inCurrentNote].releaseSamples = releasedur;
		if (releasedur)	// avoid potential division by zero
		{
			// calculate how far this fade must "step" through the fade table at each sample
			noteTable[inCurrentNote].fadeTableStep = (float)NUM_FADE_POINTS / (float)releasedur;
			noteTable[inCurrentNote].linearFadeStep = 1.0f / (float)releasedur;
		}
		// make sure to turn the note off NOW if there is no release
		else
			noteTable[inCurrentNote].velocity = 0;
	}
	// we're at note off, so wipe out the attack info
	noteTable[inCurrentNote].attackDur = 0;
	noteTable[inCurrentNote].attackSamples = 0;
}



//-----------------------------------------------------------------------------------------



/* I wrote this in an email to someone explaining how my MIDI handling works.  
   I figured it was worth throwing in here for anyone else who might look at my source code.
   It's a step by step explanation of what happens.

NOTE:  I've made many changes to this code but haven't updated the info below, 
so don't be surprised if it doesn't match up with the current code.

	In processEvents(), I receive a VstEvent array (which includes a 
counter value for how many items there are in that particular array) and 
then I look at every item.  First I check if it's a MIDI event (like in 
the SDK).  If it is, then I cast it into my VstMidiEvent variable (like in 
the SDK) and then start examining it.  I only want to deal with it if is 
one of 3 types of MIDI events:  a note, a pitchbend message, or a panic 
message.  That is what each of the next three "if"s are all about.  If the 
event is any one of those 3 things, then it gets worked on.
	You are right that the struct array blockEvents[] is my queue.  I fill 
up one item in that array for each interesting event that I receive during 
that processing block.  I store the status in my own way (using an enum 
that's in my header) as either kMidiNoteOn, kMidiNoteOff, kMidiPitchbend, 
or kMidiCC_AllNotesOff.  This is what goes in the blockEvents.status field.  
Then I take MIDI bytes 1 & 2 and put them into blockEvents.byte1 & .byte2.  
If it's a note off message, then I put 0 into byte2 (velocity).  By the 
way, with pitchbend, byte 1 is the LSB, but since LSB is, so far as I know, 
inconsistantly implemented by different MIDI devices, my plugin doesn't 
actually use that value in any way.  At the end of each of these "if"s, I 
update numBlockEvents because that is my counter that I look at during 
process() to see how many events I have to deal with during that block.
	And for each event, I store the deltaFrames value.  deltaFrames is the 
number of samples into that processing block that the event occurs.  This 
is what makes sample accurate timing (on MIDI data playback, not with live 
playing, of course) possible.  A VST host will send all of the upcoming 
events for a giving processing block to processEvents() and then the exact 
position within that processing block is given by deltaFrames.  If 
deltaFrames is 0, then the event occurs at the very beginning of the block.  
If deltaFrames is 333, then it occurs 333 samples into that processing 
block.  While it is not required, the SDK claims that hosts generally, as 
a matter of convention, send VstEvents to processEvents() in chronological 
order.  My plugin assumes that they are received in order.  There is no 
sorting done in my plugin.
	Now on to process().  Basically, I divide my process up into 
sub-chunks according to when events occur (which means according to the 
deltaFrames values).  I have two important variables here:   eventcount 
and currentBlockPosition.  The eventcount keeps track of how many of the 
events for that block I have addressed.  I initialize it to -1 so that 
first it will do processing up until the first event and then it will 
start counting events at 0 with the first event.  This is because there 
most likely will be audio to process before any events occur during that 
block (unless the block began with all notes off).  currentBlockPosition 
stores the sample start position of the current sub-chunk.  Basically it 
is the deltaFrames values of the latest event that I am working on.  It 
obviously starts out at 0.
	Next I start a "do" loop that cycles for every event.  First it 
evaluates the duration of the current processing sub-chunk, so it first 
checks to see if there are any more upcoming events in the queue.  If not, 
then the current chunk processes to the end of the processing block, if 
yes, then the current sub-chunk position is subtracted from the upcoming 
event's deltaFrames value.  I move up inputs and outputs accordingly, etc.
	Next comes a "for" loop that goes through my noteTable[] struct array 
and looks for any active notes (i.e. notes with a non-zero velocity value).  
All that I do during this loop is check the velocity of each note and then 
process the audio for that note if it has a non-zero velocity.
	After that "for" loop, I increment eventcount, leave the events loop 
if events are done, update currentBlockPosition, and then call heedEvents().  
heedEvents() is important.  heedEvents() is where I take in the effects of 
the next MIDI event.  Basically I tell heedEvents() which event number I 
am looking at and then it updates any vital stuff so that, when going 
through the next processing sub-chunk, all necessary changes have been 
made to make the next batch of processing take into account the impact of 
the latest event.  So heedEvents() is pretty much just a switch statement 
checking out which type of event is being analyzed and then implementing it 
in the appropriate way.  It would take quite a while to fully explain what 
happens in there and why, so I'll leave it up to you to determine whichever 
parts of it you want to figure out.  I don't think that it is very 
complicated (just many steps), but I could be wrong and if you can't figure 
out any parts of it, let me know and I'll explain.
	There is one more really important thing in process().  At the of the 
whole function, I reset numBlockEvents (the global events counter) to 0.  
This is extremely important because processEvents() only gets called if 
new events are received during a processing block.  If not, then 
processEvents() does not get called, numBlockEvents does not get zeroed 
at the beginning of processEvents(), and process() will process the same 
MIDI events over and over and over for every processing block until a new 
MIDI event is received.  This fact about processEvents() is not explained 
in the SDK and I spent FOREVER with a malfunctioning plugin until I 
figured this out.
*/
