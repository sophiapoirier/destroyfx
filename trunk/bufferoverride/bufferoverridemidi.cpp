/*------------------- by Marc Poirier  ][  March 2001 -------------------*/

#include "bufferoverride.hpp"


//-----------------------------------------------------------------------------
float BufferOverride::getDivisorParameterFromNote(int currentNote)
{
	// tell the GUI to update the divisor parameter's slider and value display
	divisorWasChangedByMIDI = true;
	// this isn't true anymore - it's MIDI's turn
	divisorWasChangedByHand = false;

	// this step gets the literal value for the new divisor
	float newDivisor = (float) ((double)bufferSizeMs*0.001 * midistuff->freqTable[currentNote] * pitchbend);
//	return (newDivisor < getparametermin_f(kDivisor)) ? getparametermin_f(kDivisor) : newDivisor;
	return (newDivisor < 0.0001f) ? 0.0001f : newDivisor;	// XXX is this much even necessary?
}

//-----------------------------------------------------------------------------
float BufferOverride::getDivisorParameterFromPitchbend(int pitchbendByte)
{
	oldPitchbend = pitchbend;

	// bend pitch up
	if (pitchbendByte > 64)
	{
		// scale the MIDI value from 0.0 to 1.0
		pitchbend = (double)(pitchbendByte - 64) / 63.0;
		// then scale it according to tonal steps and the user defined range
		pitchbend = pow(NOTE_UP_SCALAR, pitchbend*pitchbendRange);
	}
	// bend pitch down
	else
	{
		// scale the MIDI value from 0.0 to 1.0
		pitchbend = (double)(64 - pitchbendByte) / 64.0;
		// then scale it according to tonal steps and the user defined range
		pitchbend = pow(NOTE_DOWN_SCALAR, pitchbend*pitchbendRange);
	}

	// only update the divisor value if we're in MIDI nudge mode or trigger mode with a note currently active
	if ( (midiMode == kMidiMode_nudge) || ((midiMode == kMidiMode_trigger) && (midistuff->noteQueue[0] >= 0)) )
	{
		// tell the GUI to update the divisor parameter's slider and value display
		divisorWasChangedByMIDI = true;

		// this step gets the literal value for the new divisor
		// you need to take into account where pitchbend is coming from, hence the division by oldPitchbend
		if (oldPitchbend == 0.0f)
			oldPitchbend = 1.0f;	// avoid division by zero <-- XXX necessary?
		float newDivisor = (float) ((double)divisor * pitchbend/oldPitchbend);
//		return (newDivisor < getparametermin_f(kDivisor)) ? getparametermin_f(kDivisor) : newDivisor;
		return (newDivisor < 0.0001f) ? 0.0001f : newDivisor;	// XXX is this much even necessary?
	}
	// otherwise return -3 (my code for "don't actually update the divisor value")
	else
		return -3.0f;
}


//-----------------------------------------------------------------------------
// this function implements the changes that new MIDI events demand
void BufferOverride::heedBufferOverrideEvents(unsigned long samplePos)
{
	// look at the events if we have any
	if (midistuff->numBlockEvents > 0)
	{
// SEARCH FOR NOTE
		// initialize these to invalid values so that we can tell later what we really found
		bool foundNote = false;
		int foundNoteOn = kInvalidMidi;
		// search events from the beginning up until the current processing block position
		for (long eventcount = 0; eventcount < midistuff->numBlockEvents; eventcount++)
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

			else if (midistuff->blockEvents[eventcount].status == kMidiCC)
			{
				if (midistuff->blockEvents[eventcount].byte1 == kMidiCC_AllNotesOff)
				{
					foundNote = true;
					midistuff->removeAllNotes();
				}
			}
		}

		// we found a valid new note, so update the divisor value if that note is still active
		if (foundNote)
		{
			if ( (midistuff->noteQueue[0] >= 0) && 
// the || !divisorWasChangedByHand part has to do with the fact that the note event may have been a note off
// which pushed an older still-playing note to be first in the queue, and so we want to use that older note
// XXX however, it is possible that the removed note wasn't first, and so in that case maybe we shouldn't do this?
					((foundNoteOn >= 0) || (!divisorWasChangedByHand)) )
			{
				// update the divisor parameter value
				divisor = getDivisorParameterFromNote(midistuff->noteQueue[0]);
				// false oldNote so it will be ignored until a new valid value is put into it
				oldNote = false;
				lastNoteOn = kInvalidMidi;
			}
			// if we're in MIDI nudge mode, allow the last note-on to update divisor even if the note is not still active
			else if ( (midiMode == kMidiMode_nudge) && (foundNoteOn >= 0) )
			{
				divisor = getDivisorParameterFromNote(foundNoteOn);
				// false oldNote so it will be ignored until a new valid value is put into it
				oldNote = false;
				lastNoteOn = kInvalidMidi;
			}
		}

// SEARCH FOR PITCHBEND
		// search events backwards again looking for the most recent valid pitchbend
		for (long eventcount = midistuff->numBlockEvents - 1; eventcount >= 0; eventcount--)
		{
			// once we're below the current block position, pitchbend messages can be considered
			if ( (midistuff->blockEvents[eventcount].delta <= (signed)samplePos) && 
					(midistuff->blockEvents[eventcount].status == kMidiPitchbend) )
			{
				// update the divisor parameter value
				float tempDivisor = getDivisorParameterFromPitchbend(midistuff->blockEvents[eventcount].byte2);
				// make sure that we ought to be updating divisor
				// the function will return -3 if we're in MIDI trigger mode and no notes are active
				if (tempDivisor > 0.0f)
					divisor = tempDivisor;

				// negate lastPitchbend so it will be ignored until a new valid value is put into it
				lastPitchbend = kInvalidMidi;

				// invalidate this and all earlier pitchbend messages so that they are not found in a future search
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

	// check for an unused note left over from a previous block
	if ( oldNote && !divisorWasChangedByHand )
	{
		// only if a note is currently active for MIDI trigger mode
		if (midistuff->noteQueue[0] >= 0)
			divisor = getDivisorParameterFromNote(midistuff->noteQueue[0]);
		// but we are more permissive if we're in MIDI nudge mode
		else if ( (midiMode == kMidiMode_nudge) && (lastNoteOn >= 0) )
			divisor = getDivisorParameterFromNote(lastNoteOn);
	}
	// negate the old note stuff so it will be ignored until a new valid value is put into it
	oldNote = false;
	lastNoteOn = kInvalidMidi;

	// check for an unused pitchbend message leftover from a previous block
	if (lastPitchbend >= 0)
	{
		// update the divisor parameter value
		float tempDivisor = getDivisorParameterFromPitchbend(lastPitchbend);
		// make sure that we ought to be updating divisor
		// the function will return -3 if we're in MIDI trigger mode and no notes are active
		if (tempDivisor > 0.0f)
			divisor = tempDivisor;
		// negate lastPitchbend so it will be ignored until a new valid value is put into it
		lastPitchbend = kInvalidMidi;
	}

	// if we're in MIDI trigger mode and no notes are active and the divisor hasn't been updated 
	// via normal parameter changes, then set divisor to its min so that we get that effect punch-out
	if ( ((midiMode == kMidiMode_trigger) && (midistuff->noteQueue[0] < 0)) && (!divisorWasChangedByHand) )
	{
		divisor = getparametermin_f(kDivisor);
		divisorWasChangedByMIDI = true;
	}
}
