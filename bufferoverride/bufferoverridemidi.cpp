/*------------------------------------------------------------------------
Copyright (C) 2001-2022  Sophia Poirier

This file is part of Buffer Override.

Buffer Override is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Buffer Override is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Buffer Override.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#include "bufferoverride-base.h"
#include "bufferoverride.h"

#include <algorithm>
#include <optional>

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
float BufferOverride::getDivisorParameterFromPitchbend(int valueLSB, int valueMSB)
{
	mOldPitchBend = mPitchBend;

	mPitchBend = DfxMidi::calculatePitchBendScalar(valueLSB, valueMSB);
	// scale it according to tonal steps and the user defined range
	mPitchBend = dfx::math::FrequencyScalarBySemitones(mPitchBend * mPitchBendRange);

	// only update the divisor value if we're in MIDI nudge mode or trigger mode with a note currently active
	if ((mMidiMode == kMidiMode_Nudge) || ((mMidiMode == kMidiMode_Trigger) && getmidistate().isAnyNoteActive()))
	{
		// tell the GUI to update the divisor parameter's slider and value display
		mDivisorWasChangedByMIDI = true;

		// this step gets the literal value for the new divisor
		// you need to take into account where pitchbend is coming from, hence the division by mOldPitchBend
		if (dfx::math::IsZero(mOldPitchBend))
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
void BufferOverride::heedMidiEvents(size_t samplePos)
{
	auto& midiState = getmidistate();
	// look at the events if we have any
	if (midiState.getBlockEventCount() > 0)
	{
// SEARCH FOR NOTE
		// initialize these to invalid values so that we can tell later what we really found
		bool foundNote = false;
		std::optional<int> foundNoteOn;
		// search events from the beginning up until the current processing block position
		for (size_t eventIndex = 0; eventIndex < midiState.getBlockEventCount(); eventIndex++)
		{
			// don't search past the current processing block position
			if (midiState.getBlockEvent(eventIndex).mOffsetFrames > samplePos)
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

			else if (midiState.getBlockEvent(eventIndex).mStatus == DfxMidi::kStatus_CC)
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
			if (auto const latestNote = midiState.getLatestNote(); latestNote &&
// the || !mDivisorWasChangedByHand part has to do with the fact that the note event may have been a note off
// which pushed an older still-playing note to be first in the queue, and so we want to use that older note
// XXX however, it is possible that the removed note wasn't first, and so in that case maybe we shouldn't do this?
				(foundNoteOn || !mDivisorWasChangedByHand))
			{
				// update the divisor parameter value
				mDivisor = getDivisorParameterFromNote(*latestNote);
				// false mOldNote so it will be ignored until a new valid value is put into it
				mOldNote = false;
				mLastNoteOn.reset();
			}
			// if we're in MIDI nudge mode, allow the last note-on to update divisor even if the note is not still active
			else if ((mMidiMode == kMidiMode_Nudge) && foundNoteOn)
			{
				mDivisor = getDivisorParameterFromNote(*foundNoteOn);
				// false mOldNote so it will be ignored until a new valid value is put into it
				mOldNote = false;
				mLastNoteOn.reset();
			}
		}

// SEARCH FOR PITCHBEND
		// search events backwards again looking for the most recent valid pitchbend
		for (size_t eventIndex = midiState.getBlockEventCount() - 1; true; eventIndex--)
		{
			// once we're below the current block position, pitchbend messages can be considered
			if ((midiState.getBlockEvent(eventIndex).mOffsetFrames <= samplePos)
				&& (midiState.getBlockEvent(eventIndex).mStatus == DfxMidi::kStatus_PitchBend))
			{
				// update the divisor parameter value
				auto const tempDivisor = getDivisorParameterFromPitchbend(midiState.getBlockEvent(eventIndex).mByte1, midiState.getBlockEvent(eventIndex).mByte2);
				// make sure that we ought to be updating divisor
				// the function will return -3 if we're in MIDI trigger mode and no notes are active
				if (tempDivisor > 0.0f)
				{
					mDivisor = tempDivisor;
				}

				// invalidate mLastPitchbend* so they will be ignored until new valid values are put into them
				mLastPitchbendLSB.reset();
				mLastPitchbendMSB.reset();

				// invalidate this and all earlier pitchbend messages so that they are not found in a future search
				while (true)
				{
					if (midiState.getBlockEvent(eventIndex).mStatus == DfxMidi::kStatus_PitchBend)
					{
						// the note shouldn't be considered anymore
						midiState.invalidateBlockEvent(eventIndex);
					}
					if (eventIndex == 0)
					{
						break;
					}
					eventIndex--;
				}
			}
			if (eventIndex == 0)
			{
				break;
			}
		}
	}

	// check for an unused note left over from a previous block
	if (mOldNote && !mDivisorWasChangedByHand)
	{
		// only if a note is currently active for MIDI trigger mode
		if (auto const latestNote = midiState.getLatestNote())
		{
			mDivisor = getDivisorParameterFromNote(*latestNote);
		}
		// but we are more permissive if we're in MIDI nudge mode
		else if ((mMidiMode == kMidiMode_Nudge) && mLastNoteOn)
		{
			mDivisor = getDivisorParameterFromNote(*mLastNoteOn);
		}
	}
	// invalidate the old note stuff so it will be ignored until a new valid value is put into it
	mOldNote = false;
	mLastNoteOn.reset();

	// check for an unused pitchbend message leftover from a previous block
	if (mLastPitchbendLSB && mLastPitchbendMSB)
	{
		// update the divisor parameter value
		auto const tempDivisor = getDivisorParameterFromPitchbend(*mLastPitchbendLSB, *mLastPitchbendMSB);
		// make sure that we ought to be updating divisor
		// the function will return -3 if we're in MIDI trigger mode and no notes are active
		if (tempDivisor > 0.0f)
		{
			mDivisor = tempDivisor;
		}
		// invalidate mLastPitchbend* so they will be ignored until new valid values are put into them
		mLastPitchbendLSB.reset();
		mLastPitchbendMSB.reset();
	}

	// if we're in MIDI trigger mode and no notes are active and the divisor hasn't been updated
	// via normal parameter changes, then set divisor to its min so that we get that effect punch-out
	if ((mMidiMode == kMidiMode_Trigger) && !midiState.isAnyNoteActive() && !mDivisorWasChangedByHand)
	{
		mDivisor = getparametermin_f(kDivisor);
		mDivisorWasChangedByMIDI = true;
	}
}
