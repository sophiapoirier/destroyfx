/*------------------------------------------------------------------------
Copyright (C) 2000-2023  Sophia Poirier

This file is part of Skidder.

Skidder is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Skidder is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Skidder.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#include "skidder.h"

#include <algorithm>
#include <cmath>

#include "dfxmath.h"


//-----------------------------------------------------------------------------
void Skidder::resetMidi()
{
	mNoteTable.fill(0);  // zero out the whole note table
	mWaitSamples = 0;
	mMidiIn = mMidiOut = false;
	mMostRecentVelocity = DfxMidi::kMaxValue;
}

//-----------------------------------------------------------------------------
void Skidder::processMidiNotes()
{
	auto noteWasOn = isAnyNoteOn();

	for (size_t i = 0; i < getmidistate().getBlockEventCount(); i++)
	{
		auto const currentNote = getmidistate().getBlockEvent(i).mByte1;
		auto const velocity = getmidistate().getBlockEvent(i).mByte2;

		switch (getmidistate().getBlockEvent(i).mStatus)
		{
			// note-on status was received
			case DfxMidi::kStatus_NoteOn:
				mNoteTable[currentNote] = mMostRecentVelocity = velocity;
				// This is not very accurate.
				// If more than one note-on happens this block, only the last one is effective.
				// This is good enough for this effect, though.
				noteOn(getmidistate().getBlockEvent(i).mOffsetFrames);
				noteWasOn = true;
				break;

			// note-off status was received
			case DfxMidi::kStatus_NoteOff:
				mNoteTable[currentNote] = 0;  // turn off the note
				break;

			// all notes off
			case DfxMidi::kStatus_CC:
				if (getmidistate().getBlockEvent(i).mByte1 == DfxMidi::kCC_AllNotesOff)
				{
					mNoteTable.fill(0);  // turn off all notes
					noteOff();  // do the notes off Skidder stuff
					noteWasOn = false;
				}
				break;

			default:
				break;
		}
	}

	// check to see whether we are internally in a state of note being on but all,
	// and do the Skidder notes-off stuff if we have no notes remaining on
	if (noteWasOn && !isAnyNoteOn())
	{
		noteOff();
	}
}

//-----------------------------------------------------------------------------------------
void Skidder::noteOn(size_t offsetFrames)
{
	switch (mMidiMode)
	{
		case kMidiMode_Trigger:
		case kMidiMode_Apply:
			mWaitSamples = dfx::math::ToSigned(offsetFrames);
			mState = SkidState::Valley;
			mValleySamples = 0;
			mMidiIn = true;
			break;

		default:
			break;
	}
}

//-----------------------------------------------------------------------------------------
void Skidder::noteOff()
{
	switch (mMidiMode)
	{
		case kMidiMode_Trigger:
			switch (mState)
			{
				case SkidState::SlopeIn:
					mState = SkidState::SlopeOut;
					mSlopeSamples = mSlopeDur - mSlopeSamples;
					mWaitSamples = mSlopeSamples;
					break;

				case SkidState::Plateau:
					mPlateauSamples = 1;
					mWaitSamples = mSlopeDur + 1;
					break;

				case SkidState::SlopeOut:
					mWaitSamples = mSlopeSamples;
					break;

				case SkidState::Valley:
					mWaitSamples = 0;
					break;
			}

			mMidiOut = true;
			// in case we're still in a MIDI-in phase; it won't get falsed otherwise
			if (mMidiIn)
			{
				mMidiIn = false;
			}
			// if we're in a slope-out and there's a raised floor, the slope position needs to be rescaled
			else if ((mState == SkidState::SlopeOut) && (mFloor > 0.0f))
			{
				mSlopeSamples = std::lround(((static_cast<float>(mSlopeSamples) * mSlopeStep * mGainRange) + mFloor) * static_cast<float>(mSlopeDur));
				mWaitSamples = mSlopeSamples;
			}

			// just to prevent anything really stupid from happening
			mWaitSamples = std::max(mWaitSamples, 0L);

			break;

		case kMidiMode_Apply:
			switch (mState)
			{
				case SkidState::SlopeIn:
					mWaitSamples = mSlopeSamples;
					break;

				case SkidState::Plateau:
					mWaitSamples = 0;
					break;

				case SkidState::SlopeOut:
					mState = SkidState::SlopeIn;
					mSlopeSamples = mSlopeDur - mSlopeSamples;
					mWaitSamples = mSlopeSamples;
					break;

				case SkidState::Valley:
					mValleySamples = 1;
					mWaitSamples = mSlopeDur + 1;
					break;
			}

			mMidiOut = true;
			// in case we're still in a MIDI-in phase; it won't get falsed otherwise
			mMidiIn = false;
			// just to prevent anything really stupid from happening
			mWaitSamples = std::max(mWaitSamples, 0L);

			break;

		default:
			break;
	}
}

//-----------------------------------------------------------------------------
bool Skidder::isAnyNoteOn() const
{
	return std::ranges::any_of(mNoteTable, [](auto const& velocity){ return (velocity > 0); });
}


//-----------------------------------------------------------------------------
// this gets called when Skidder automates a parameter from CC messages.
// this is where we can link parameter automation for rangeslider points.
void Skidder::settings_doLearningAssignStuff(dfx::ParameterID parameterID, dfx::MidiEventType eventType,
											 int eventChannel, int eventNum,
											 size_t /*offsetFrames*/, int eventNum2, dfx::MidiEventBehaviorFlags eventBehaviourFlags,
											 int data1, int data2, float fdata1, float fdata2)
{
	if (getsettings().getSteal())
	{
		return;
	}

	switch (parameterID)
	{
		case kPulsewidth:
			if (mPulsewidthDoubleAutomate)
			{
				getsettings().assignParameter(kPulsewidthRandMin, eventType, eventChannel, eventNum, eventNum2,
											  eventBehaviourFlags, data1, data2, fdata1, fdata2);
			}
			break;
		case kPulsewidthRandMin:
			if (mPulsewidthDoubleAutomate)
			{
				getsettings().assignParameter(kPulsewidth, eventType, eventChannel, eventNum, eventNum2,
											  eventBehaviourFlags, data1, data2, fdata1, fdata2);
			}
			break;
		case kFloor:
			if (mFloorDoubleAutomate)
			{
				getsettings().assignParameter(kFloorRandMin, eventType, eventChannel, eventNum, eventNum2,
											  eventBehaviourFlags, data1, data2, fdata1, fdata2);
			}
			break;
		case kFloorRandMin:
			if (mFloorDoubleAutomate)
			{
				getsettings().assignParameter(kFloor, eventType, eventChannel, eventNum, eventNum2,
											  eventBehaviourFlags, data1, data2, fdata1, fdata2);
			}
			break;
		default:
			break;
	}
}

/*
//-----------------------------------------------------------------------------
void Skidder::settings_unassignParam(long tag)
{
	VstChunk::unassignParam(tag);

	switch (tag)
	{
		case kPulsewidth:
			if (mPulsewidthDoubleAutomate)
			{
				VstChunk::unassignParam(kPulsewidthRandMin);
			}
			break;
		case kPulsewidthRandMin:
			if (mPulsewidthDoubleAutomate)
			{
				VstChunk::unassignParam(kPulsewidth);
			}
			break;
		case kFloor:
			if (mFloorDoubleAutomate)
			{
				VstChunk::unassignParam(kFloorRandMin);
			}
			break;
		case kFloorRandMin:
			if (mFloorDoubleAutomate)
			{
				VstChunk::unassignParam(kFloor);
			}
			break;
		default:
			break;
	}
}
*/
