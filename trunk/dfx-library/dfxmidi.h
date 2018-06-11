/*---------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2001-2018  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Destroy FX Library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Destroy FX Library.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/

Sophia's Destroy FX MIDI stuff
---------------------------------------------------------------*/

#pragma once


#include <array>

#include "dfxenvelope.h"


//-----------------------------------------------------------------------------
class DfxMidi
{
public:
	static constexpr long kNumNotes = 128;
	static constexpr int kMaxValue = 0x7F;
	static constexpr int kMidpointValue = 64;
	static constexpr float kValueScalar = 1.0f / static_cast<float>(kMaxValue);
	static constexpr int kInvalidValue = -3;  // for whatever
	static constexpr int kPitchBendMidpointValue = 0x2000;
	static constexpr int kPitchBendMaxValue = 0x3FFF;
	static constexpr double kPitchBendSemitonesMax = 36.0;
	static constexpr long kStolenNoteFadeDur = 48;

	// these are the MIDI event status types
	enum
	{
		kStatus_NoteOff = 0x80,
		kStatus_NoteOn = 0x90,
		kStatus_PolyphonicAftertouch = 0xA0,
		kStatus_CC = 0xB0,
		kStatus_ProgramChange = 0xC0,
		kStatus_ChannelAftertouch = 0xD0,
		kStatus_PitchBend = 0xE0,

		kStatus_SysEx = 0xF0,
		kStatus_TimeCode = 0xF1,
		kStatus_SongPositionPointer = 0xF2,
		kStatus_SongSelect = 0xF3,
		kStatus_TuneRequest = 0xF6,
		kStatus_EndOfSysex = 0xF7,
		kStatus_TimingClock = 0xF8,
		kStatus_Start = 0xFA,
		kStatus_Continue = 0xFB,
		kStatus_Stop = 0xFC,
		kStatus_ActiveSensing = 0xFE,
		kStatus_SystemReset = 0xFF
	};

	// these are the MIDI continuous controller messages (CCs)
	enum
	{
		kCC_BankSelect = 0x00,
		kCC_ModWheel = 0x01,
		kCC_BreathControl = 0x02,
		kCC_FootControl = 0x04,
		kCC_PortamentoTime = 0x05,
		kCC_DataEntry = 0x06,
		kCC_ChannelVolume = 0x07,
		kCC_Balance = 0x08,
		kCC_Pan = 0x0A,
		kCC_ExpressionController = 0x0B,
		kCC_EffectControl1 = 0x0C,
		kCC_EffectControl2 = 0x0D,
		kCC_GeneralPurposeController1 = 0x10,
		kCC_GeneralPurposeController2 = 0x11,
		kCC_GeneralPurposeController3 = 0x12,
		kCC_GeneralPurposeController4 = 0x13,

		// on/off CCs   ( <= 63 is off, >= 64 is on )
		kCC_SustainPedalOnOff = 0x40,
		kCC_PortamentoOnOff = 0x41,
		kCC_SustenutoOnOff = 0x42,
		kCC_SoftPedalOnOff = 0x43,
		kCC_LegatoFootswitch = 0x44,
		kCC_Hold2 = 0x45,

		kCC_SoundController1_SoundVariation = 0x46,
		kCC_SoundController2_Timbre = 0x47,
		kCC_SoundController3_ReleaseTime = 0x48,
		kCC_SoundController4_AttackTime = 0x49,
		kCC_SoundController5_Brightness = 0x4A,
		kCC_SoundController6 = 0x4B,
		kCC_SoundController7 = 0x4C,
		kCC_SoundController8 = 0x4D,
		kCC_SoundController9 = 0x4E,
		kCC_SoundController10 = 0x4F,
		kCC_GeneralPurposeController5 = 0x50,
		kCC_GeneralPurposeController6 = 0x51,
		kCC_GeneralPurposeController7 = 0x52,
		kCC_GeneralPurposeController8 = 0x53,
		kCC_PortamentoControl = 0x54,
		kCC_Effects1Depth = 0x5B,
		kCC_Effects2Depth = 0x5C,
		kCC_Effects3Depth = 0x5D,
		kCC_Effects4Depth = 0x5E,
		kCC_Effects5Depth = 0x5F,
		kCC_DataEntryPlus1 = 0x60,
		kCC_DataEntryMinus1 = 0x61,

		// special commands
		kCC_AllSoundOff = 0x78,  // 0 only
		kCC_ResetAllControllers = 0x79,  // 0 only
		kCC_LocalControlOnOff = 0x7A,  // 0 = off, 127 = on
		kCC_AllNotesOff = 0x7B,  // 0 only
		kCC_OmniModeOff = 0x7C,  // 0 only
		kCC_OmniModeOn = 0x7D,  // 0 only
		kCC_PolyModeOnOff = 0x7E,
		kCC_PolyModeOn = 0x7F  // 0 only
	};

	//----------------------------------------------------------------------------- 
	// this holds MIDI event information
	struct Event
	{
		int mStatus;  // the event status MIDI byte
		int mByte1;  // the first MIDI data byte
		int mByte2;  // the second MIDI data byte
		long mDelta;  // the delta offset (the sample position in the current block where the event occurs)
		int mChannel;  // the MIDI channel
	};

	//-----------------------------------------------------------------------------
	// this holds information for each MIDI note
	struct MusicNote
	{
		MusicNote()
		{
			clearTail();
		}

		void clearTail()  // zero out a note's tail buffers
		{
			mTail1.fill(0.0f);
			mTail2.fill(0.0f);
		}

		int mVelocity = 0;  // note velocity (7-bit MIDI value)
		float mNoteAmp = 0.0f;  // the gain for the note, scaled with velocity, curve, and influence
		DfxEnvelope mEnvelope;
		float mLastOutValue = 0.0f;  // capture the most recent output value for smoothing, if necessary   XXX mono assumption
		long mSmoothSamples = 0;  // counter for quickly fading cut-off notes, for smoothity
		std::array<float, kStolenNoteFadeDur> mTail1;  // a little buffer of output samples for smoothing a cut-off note (left channel)
		std::array<float, kStolenNoteFadeDur> mTail2;  // (right channel) XXX wow this stereo assumption is such a bad idea
	};

	DfxMidi();

	void reset();  // resets the variables
	void setSampleRate(double inSampleRate);
	void setEnvParameters(double inAttackDur, double inDecayDur, double inSustainLevel, double inReleaseDur);
	void setEnvCurveType(DfxEnvelope::CurveType inCurveType);
	void setResumedAttackMode(bool inNewMode);

	// handlers for the types of MIDI events that we support
	void handleNoteOn(int inMidiChannel, int inNoteNumber, int inVelocity, long inBufferOffset);
	void handleNoteOff(int inMidiChannel, int inNoteNumber, int inVelocity, long inBufferOffset);
	void handleAllNotesOff(int inMidiChannel, long inBufferOffset);
	void handlePitchBend(int inMidiChannel, int inValueLSB, int inValueMSB, long inBufferOffset);
	void handleCC(int inMidiChannel, int inControllerNumber, int inValue, long inBufferOffset);
	void handleProgramChange(int inMidiChannel, int inProgramNumber, long inBufferOffset);

	void preprocessEvents();
	void postprocessEvents();

	// this is where new MIDI events are reckoned with during audio processing
	void heedEvents(long inEventNum, double inPitchBendRange, bool inLegato, 
					float inVelocityCurve, float inVelocityInfluence);

	auto getBlockEventCount() const noexcept
	{
		return mNumBlockEvents;
	}
	auto const& getBlockEvent(long inIndex) const
	{
		return mBlockEvents.at(inIndex);
	}
	// XXX TODO: this is a hack just for Buffer Override, maybe should rethink
	void invalidateBlockEvent(long inIndex)
	{
		mBlockEvents.at(inIndex).mStatus = kInvalidValue;
	}

	auto const& getNoteState(int inMidiNote) const
	{
		return mNoteTable.at(inMidiNote);
	}
	// XXX TODO: this is a hack just for Rez Synth, maybe should rethink
	void setNoteState(int inMidiNote, MusicNote const& inNoteState)
	{
		mNoteTable.at(inMidiNote) = inNoteState;
	}

	// manage the ordered queue of active MIDI notes
	void insertNote(int inMidiNote);
	void removeNote(int inMidiNote);
	void removeAllNotes();
	// query the ordered queue of active MIDI notes
	bool isNoteActive() const
	{
		return mNoteQueue.front() >= 0;
	}
	auto getLatestNote() const
	{
		return mNoteQueue.front();
	}

	static constexpr bool isNote(int inMidiStatus)
	{
		return (inMidiStatus == kStatus_NoteOn) || (inMidiStatus == kStatus_NoteOff);
	}

	auto getNoteFrequency(int inNote) const
	{
		return mNoteFrequencyTable.at(inNote);
	}

	auto getPitchBend() const noexcept
	{
		return mPitchBend;
	}

	// this calculates fade scalars if attack, decay, or release are happening
	float processEnvelope(int inMidiNote);

	// this writes the audio output for smoothing the tips of cut-off notes
	// by sloping down from the last sample outputted by the note
	void processSmoothingOutputSample(float* outAudio, long inNumSamples, int inMidiNote);

	// this writes the audio output for smoothing the tips of cut-off notes
	// by fading out the samples stored in the tail buffers
	void processSmoothingOutputBuffer(float* outAudio, long inNumSamples, int inMidiNote, int inMidiChannel);


private:
	static constexpr size_t kEventQueueSize = 12000;
	static constexpr float kStolenNoteFadeStep = 1.0f / static_cast<float>(kStolenNoteFadeDur);
	static constexpr long kLegatoFadeDur = 39;
	static constexpr float kLegatoFadeStep = 1.0f / static_cast<float>(kLegatoFadeDur);

	void fillFrequencyTable();

	bool incNumEvents();  // increment the block events counter, safely

	void turnOffNote(int inMidiNote, bool inLegato);

	std::array<MusicNote, kNumNotes> mNoteTable;  // a table with important data about each note
	std::array<double, kNumNotes> mNoteFrequencyTable;  // a table of the frequency corresponding to each MIDI note

	std::array<int, kNumNotes> mNoteQueue;  // a chronologically ordered queue of all active notes
	std::array<Event, kEventQueueSize> mBlockEvents;  // the new MIDI events for a given processing block
	long mNumBlockEvents = 0;  // the number of new MIDI events in a given processing block

	double mPitchBend = 1.0;  // a frequency scalar value for the current pitchbend setting

	std::array<bool, kNumNotes> mSustainQueue;  // a queue of note-offs for when the sustain pedal is active

	// pick up where the release left off, if it's still releasing
//	bool mLazyAttackMode;
	// sustain pedal is active
	bool mSustain = false;
};
