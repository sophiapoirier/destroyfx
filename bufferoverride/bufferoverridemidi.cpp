/*------------------- by Marc Poirier  ][  March 2001 -------------------*/

#ifndef __BUFFEROVERRIDE_H
#include "bufferoverride.hpp"
#endif

#include <math.h>
#include <stdlib.h>
#if MAC
	#include <fp.h>
#endif
#if WIN32
	#include <float.h>
#endif


//-----------------------------------------------------------------------------
float BufferOverride::getDivisorParameterFromNote(int currentNote)
{
  float newDivisor;	// just a temporary value holder used in breaking down a computation into 2 steps

	// tell the GUI to update the divisor parameter's slider & value display
	divisorWasChangedByMIDI = true;
	// this isn't true anymore - it's MIDI's turn
	divisorWasChangedByHand = false;

	// this step gets the literal value for the new divisor
	newDivisor = (float) ((double)bufferSizeMs*0.001 * midistuff->freqTable[currentNote] * pitchbend);
/*
	// this step scales that literal value into the appropriate value for fDivisor, 
	// the value that will rescale correctly (back to newDivisor) in the bufferDivisorScaled() macro
	newDivisor = (newDivisor-getparametermin_f(kDivisor)) / (getparametermax_f(kDivisor)-getparametermin_f(kDivisor));
	// I don't know why, but even after checking if newDivisor>0.0 I still get not-a-numbers sometimes, so I prevent that
	if (newDivisor > 0.0f)
	#if MAC
		return ( isnan(sqrtf(newDivisor)) ? 0.0f : sqrtf(newDivisor) );
	#else
		return ( _isnan(sqrtf(newDivisor)) ? 0.0f : sqrtf(newDivisor) );
	#endif
	// but skip the square-rooting if the value is negative (pitchbend went too low)
	else
		return 0.0f;
*/
		return (newDivisor < getparametermin_f(kDivisor)) ? getparametermin_f(kDivisor) : newDivisor;
}

//-----------------------------------------------------------------------------
float BufferOverride::getDivisorParameterFromPitchbend(int pitchbendByte)
{
  float newDivisor;	// just a temporary value holder


	oldPitchbend = pitchbend;

	// bend pitch up
	if (pitchbendByte > 64)
	{
		// scale the MIDI value from 0.0 to 1.0
		pitchbend = (double)(pitchbendByte - 64) / 63.0;
		// then scale it according to tonal steps & the user defined range
		pitchbend = pow(NOTE_UP_SCALAR, pitchbend*pitchbendRange);
	}

	// bend pitch down
	else
	{
		// scale the MIDI value from 0.0 to 1.0
		pitchbend = (double)(64 - pitchbendByte) / 64.0;
		// then scale it according to tonal steps & the user defined range
		pitchbend = pow(NOTE_DOWN_SCALAR, pitchbend*pitchbendRange);
	}

	// only update the fDivisor value if we're in MIDI nudge mode or trigger mode with a note currently active
	if ( (midiMode == kMidiModeNudge) || ((midiMode == kMidiModeTrigger) && (midistuff->noteQueue[0] >= 0)) )
	{
		// tell the GUI to update the divisor parameter's slider & value display
		divisorWasChangedByMIDI = true;

		// this step gets the literal value for the new divisor
		// you need to take into account where pitchbend is coming from, hence the division by oldPitchbend
		newDivisor = (float) ((double)divisor * pitchbend/oldPitchbend);
/*
		// these step scale that literal value into the appropriate value for fDivisor, 
		// the value that will rescale correctly (back to newDivisor) in the bufferDivisorScaled() macro
		newDivisor = (newDivisor-DIVISOR_MIN) / (DIVISOR_MAX-DIVISOR_MIN);
		// I don't know why, but even after checking if newDivisor>0.0 I still get not-a-numbers sometimes, so I prevent that
		if (newDivisor > 0.0f)
		#if MAC
			return ( isnan(sqrtf(newDivisor)) ? 0.0f : sqrtf(newDivisor) );
		#else
			return ( _isnan(sqrtf(newDivisor)) ? 0.0f : sqrtf(newDivisor) );
		#endif
		// but skip the squarerooting if the value is negative (pitchbend went too low)
		else
			return 0.0f;
*/
		return (newDivisor < getparametermin_f(kDivisor)) ? getparametermin_f(kDivisor) : newDivisor;
	}
	// otherwise return -3, my code for "don't really update the fDivisor value"
	else
		return -3.0f;
}


//-----------------------------------------------------------------------------
// this function implements the changes that new MIDI events demand
void BufferOverride::heedBufferOverrideEvents(unsigned long samplePos)
{
  long eventcount;
  bool foundNote;
  float tempDivisor;
  int foundNoteOn;


	// look at the events if we have any
	if (midistuff->numBlockEvents > 0)
	{
// SEARCH FOR NOTE
		// initialize these to invalid values so that we can tell later what we really found
		foundNote = false;
		foundNoteOn = kInvalidMidi;
		// search events from the beginning up until the current processing block position
		for (eventcount = 0; eventcount < midistuff->numBlockEvents; eventcount++)
		{
			// don't search past the current processing block position
			if (midistuff->blockEvents[eventcount].delta > (signed)samplePos)
				break;

			if (isNote(midistuff->blockEvents[eventcount].status))
			{
				foundNote = true;
				// update the notes table
				if (midistuff->blockEvents[eventcount].status == kMidiNoteOn)
				{
					midistuff->insertNote(midistuff->blockEvents[eventcount].byte1);
					foundNoteOn = midistuff->blockEvents[eventcount].byte1;
				}
				else
					midistuff->removeNote(midistuff->blockEvents[eventcount].byte1);

				// I use the kInvalidMidi state in this plugin to mean that the note shouldn't be considered anymore
				midistuff->blockEvents[eventcount].status = kInvalidMidi;
			}

			else if (midistuff->blockEvents[eventcount].status == kMidiCC_AllNotesOff)
			{
				foundNote = true;
				midistuff->removeAllNotes();
			}
		}

		// we found a valid new note, so update the fDivisor value if that note is still active
		if (foundNote)
		{
			if ( (midistuff->noteQueue[0] >= 0) && ((foundNoteOn >= 0) || (!divisorWasChangedByHand)) )
			{
				// update the fDivisor parameter value
				divisor = getDivisorParameterFromNote(midistuff->noteQueue[0]);
				// false oldNote so it will be ignored until a new valid value is put into it
				oldNote = false;
				lastNoteOn = kInvalidMidi;
			}
			// if we're in MIDI nudge mode, allow the last note-on to update fDivisor even if the note is not still active
			else if ( (midiMode == kMidiModeNudge) && (foundNoteOn >= 0) )
			{
				divisor = getDivisorParameterFromNote(foundNoteOn);
				// false oldNote so it will be ignored until a new valid value is put into it
				oldNote = false;
				lastNoteOn = kInvalidMidi;
			}
		}

// SEARCH FOR PITCHBEND
		// search events backwards again looking for the most recent valid pitchbend
		for (eventcount = (midistuff->numBlockEvents-1); (eventcount >= 0); eventcount--)
		{
			// once we're below the current block position, pitchbend messages can be considered
			if ( (midistuff->blockEvents[eventcount].delta <= (signed)samplePos) && 
					(midistuff->blockEvents[eventcount].status == kMidiPitchbend) )
			{
				// update the fDivisor parameter value
				tempDivisor = getDivisorParameterFromPitchbend(midistuff->blockEvents[eventcount].byte2);
				// make sure that we ought to be updating fDivisor
				// the function will return -3 if we're in MIDI trigger mode & no notes are active
				if (tempDivisor > 0.0f)
					divisor = tempDivisor;

				// negate lastPitchbend so it will be ignored until a new valid value is put into it
				lastPitchbend = kInvalidMidi;

				// invalidate this & all earlier pitchbend messages so that they are not found in a future search
				while (eventcount >= 0)
				{
					if (midistuff->blockEvents[eventcount].status == kMidiPitchbend)
						// I use the kInvalidMidi state in this plugin to mean that the note shouldn't be considered anymore
						midistuff->blockEvents[eventcount].status = kInvalidMidi;
					eventcount--;
				}
			}
		}
	}

	// check for an unused note leftover from a previous block
	if ( (oldNote) && (!divisorWasChangedByHand) )
	{
		// only if a note is currently active
		if (midistuff->noteQueue[0] >= 0)
			divisor = getDivisorParameterFromNote(midistuff->noteQueue[0]);
		// but we are more permissive if we're in MIDI nudge mode
		else if ( (midiMode == kMidiModeNudge) && (lastNoteOn >= 0) )
			divisor = getDivisorParameterFromNote(lastNoteOn);
	}
	// negate the old note stuff so it will be ignored until a new valid value is put into it
	oldNote = false;
	lastNoteOn = kInvalidMidi;

	// check for an unused pitchbend message leftover from a previous block
	if (lastPitchbend >= 0)
	{
		// update the fDivisor parameter value
		tempDivisor = getDivisorParameterFromPitchbend(lastPitchbend);
		// make sure that we ought to be updating fDivisor
		// the function will return -3 if we're in MIDI trigger mode & no notes are active
		if (tempDivisor > 0.0f)
			divisor = tempDivisor;
		// negate lastPitchbend so it will be ignored until a new valid value is put into it
		lastPitchbend = kInvalidMidi;
	}

	// if we're in MIDI trigger mode & no notes are active & the divisor hasn't been updated 
	// via normal parameter changes, then set divisor to its min so that we get that effect punch-out
	if ( ((midiMode == kMidiModeTrigger) && (midistuff->noteQueue[0] < 0)) && (!divisorWasChangedByHand) )
	{
		divisor = getparametermin_f(kDivisor);
		divisorWasChangedByMIDI = true;
	}
}
