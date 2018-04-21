/*------------------------------------------------------------------------
Copyright (C) 2001-2018  Sophia Poirier

This file is part of Buffer Override.

Buffer Override is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Buffer Override is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Buffer Override.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "bufferoverride.h"

#include <algorithm>

#include "dfxmath.h"


//-----------------------------------------------------------------------------
float BufferOverride::getDivisorParameterFromNote(int currentNote)
{
	// tell the GUI to update the divisor parameter's slider and value display
	mDivisorWasChangedByMIDI = true;
	// this isn't true anymore - it's MIDI's turn
	mDivisorWasChangedByHand = false;

	// this step gets the literal value for the new divisor
	auto const newDivisor = static_cast<float>(static_cast<double>(mBufferSizeMS) * 0.001 * getmidistate().getNoteFrequency(currentNote) * mPitchBend);
//	return std::max(newDivisor, getparametermin_f(kDivisor));
	return std::max(newDivisor, 0.0001f);  // XXX is this much even necessary?
}

//-----------------------------------------------------------------------------
float BufferOverride::getDivisorParameterFromPitchbend(int pitchbendByte)
{
	mOldPitchBend = mPitchBend;

	constexpr auto upperRange = static_cast<double>(DfxMidi::kMaxValue - DfxMidi::kMidpointValue);
	constexpr auto lowerRange = static_cast<double>(DfxMidi::kMidpointValue);
	pitchbendByte -= DfxMidi::kMidpointValue;
	// bend pitch up
	if (pitchbendByte > 0)
	{
		// scale the MIDI value from 0.0 to 1.0
		mPitchBend = static_cast<double>(pitchbendByte) / upperRange;
	}
	// bend pitch down
	else
	{
		// scale the MIDI value from -1.0 to 0.0
		mPitchBend = static_cast<double>(pitchbendByte) / lowerRange;
	}
	// then scale it according to tonal steps and the user defined range
	mPitchBend = dfx::math::FrequencyScalarBySemitones(mPitchBend * mPitchbendRange);

	// only update the divisor value if we're in MIDI nudge mode or trigger mode with a note currently active
	if ((mMidiMode == kMidiMode_Nudge) || ((mMidiMode == kMidiMode_Trigger) && getmidistate().isNoteActive()))
	{
		// tell the GUI to update the divisor parameter's slider and value display
		mDivisorWasChangedByMIDI = true;

		// this step gets the literal value for the new divisor
		// you need to take into account where pitchbend is coming from, hence the division by mOldPitchBend
		if (mOldPitchBend == 0.0)
		{
			mOldPitchBend = 1.0;  // avoid division by zero <-- XXX necessary?
		}
		auto const newDivisor = static_cast<float>(static_cast<double>(mDivisor) * mPitchBend / mOldPitchBend);
//		return std::max(newDivisor, getparametermin_f(kDivisor));
		return std::max(newDivisor, 0.0001f);  // XXX is this much even necessary?
	}
	// otherwise return -3 (my code for "don't actually update the divisor value")
	else
	{
		return -3.0f;
	}
}


//-----------------------------------------------------------------------------
// this function implements the changes that new MIDI events demand
void BufferOverride::heedBufferOverrideEvents(unsigned long samplePos)
{
	auto& midiState = getmidistate();
	// look at the events if we have any
	if (midiState.getBlockEventCount() > 0)
	{
// SEARCH FOR NOTE
		// initialize these to invalid values so that we can tell later what we really found
		bool foundNote = false;
		int foundNoteOn = DfxMidi::kInvalidValue;
		// search events from the beginning up until the current processing block position
		for (long eventIndex = 0; eventIndex < midiState.getBlockEventCount(); eventIndex++)
		{
			// don't search past the current processing block position
			if (midiState.getBlockEvent(eventIndex).mDelta > static_cast<long>(samplePos))
			{
				break;
			}

			if (DfxMidi::isNote(midiState.getBlockEvent(eventIndex).mStatus))
			{
				foundNote = true;
				// update the notes table
				if (midiState.getBlockEvent(eventIndex).mStatus == DfxMidi::kStatus_NoteOn)
				{
					midiState.insertNote(midiState.getBlockEvent(eventIndex).mByte1);
					foundNoteOn = midiState.getBlockEvent(eventIndex).mByte1;
				}
				else
				{
					midiState.removeNote(midiState.getBlockEvent(eventIndex).mByte1);
				}

				// the note shouldn't be considered anymore
				midiState.invalidateBlockEvent(eventIndex);
			}

			else if (midiState.getBlockEvent(eventIndex).mStatus == DfxMidi::kStatus_MidiCC)
			{
				if (midiState.getBlockEvent(eventIndex).mByte1 == DfxMidi::kCC_AllNotesOff)
				{
					foundNote = true;
					midiState.removeAllNotes();
				}
			}
		}

		// we found a valid new note, so update the divisor value if that note is still active
		if (foundNote)
		{
			if (midiState.isNoteActive() &&
// the || !mDivisorWasChangedByHand part has to do with the fact that the note event may have been a note off
// which pushed an older still-playing note to be first in the queue, and so we want to use that older note
// XXX however, it is possible that the removed note wasn't first, and so in that case maybe we shouldn't do this?
				((foundNoteOn >= 0) || !mDivisorWasChangedByHand))
			{
				// update the divisor parameter value
				mDivisor = getDivisorParameterFromNote(midiState.getLatestNote());
				// false mOldNote so it will be ignored until a new valid value is put into it
				mOldNote = false;
				mLastNoteOn = DfxMidi::kInvalidValue;
			}
			// if we're in MIDI nudge mode, allow the last note-on to update divisor even if the note is not still active
			else if ((mMidiMode == kMidiMode_Nudge) && (foundNoteOn >= 0))
			{
				mDivisor = getDivisorParameterFromNote(foundNoteOn);
				// false mOldNote so it will be ignored until a new valid value is put into it
				mOldNote = false;
				mLastNoteOn = DfxMidi::kInvalidValue;
			}
		}

// SEARCH FOR PITCHBEND
		// search events backwards again looking for the most recent valid pitchbend
		for (long eventIndex = midiState.getBlockEventCount() - 1; eventIndex >= 0; eventIndex--)
		{
			// once we're below the current block position, pitchbend messages can be considered
			if ((midiState.getBlockEvent(eventIndex).mDelta <= static_cast<long>(samplePos)) 
				&& (midiState.getBlockEvent(eventIndex).mStatus == DfxMidi::kStatus_PitchBend))
			{
				// update the divisor parameter value
				auto const tempDivisor = getDivisorParameterFromPitchbend(midiState.getBlockEvent(eventIndex).mByte2);
				// make sure that we ought to be updating divisor
				// the function will return -3 if we're in MIDI trigger mode and no notes are active
				if (tempDivisor > 0.0f)
				{
					mDivisor = tempDivisor;
				}

				// invalidate mLastPitchbend so it will be ignored until a new valid value is put into it
				mLastPitchbend = DfxMidi::kInvalidValue;

				// invalidate this and all earlier pitchbend messages so that they are not found in a future search
				while (eventIndex >= 0)
				{
					if (midiState.getBlockEvent(eventIndex).mStatus == DfxMidi::kStatus_PitchBend)
					{
						// the note shouldn't be considered anymore
						midiState.invalidateBlockEvent(eventIndex);
					}
					eventIndex--;
				}
			}
		}
	}

	// check for an unused note left over from a previous block
	if (mOldNote && !mDivisorWasChangedByHand)
	{
		// only if a note is currently active for MIDI trigger mode
		if (midiState.isNoteActive())
		{
			mDivisor = getDivisorParameterFromNote(midiState.getLatestNote());
		}
		// but we are more permissive if we're in MIDI nudge mode
		else if ((mMidiMode == kMidiMode_Nudge) && (mLastNoteOn >= 0))
		{
			mDivisor = getDivisorParameterFromNote(mLastNoteOn);
		}
	}
	// invalidate the old note stuff so it will be ignored until a new valid value is put into it
	mOldNote = false;
	mLastNoteOn = DfxMidi::kInvalidValue;

	// check for an unused pitchbend message leftover from a previous block
	if (mLastPitchbend >= 0)
	{
		// update the divisor parameter value
		auto const tempDivisor = getDivisorParameterFromPitchbend(mLastPitchbend);
		// make sure that we ought to be updating divisor
		// the function will return -3 if we're in MIDI trigger mode and no notes are active
		if (tempDivisor > 0.0f)
		{
			mDivisor = tempDivisor;
		}
		// invalidate mLastPitchbend so it will be ignored until a new valid value is put into it
		mLastPitchbend = DfxMidi::kInvalidValue;
	}

	// if we're in MIDI trigger mode and no notes are active and the divisor hasn't been updated 
	// via normal parameter changes, then set divisor to its min so that we get that effect punch-out
	if ((mMidiMode == kMidiMode_Trigger) && !midiState.isNoteActive() && !mDivisorWasChangedByHand)
	{
		mDivisor = getparametermin_f(kDivisor);
		mDivisorWasChangedByMIDI = true;
	}
}
