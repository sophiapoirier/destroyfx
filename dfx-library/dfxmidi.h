/*---------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2001-2023  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 2 of the License, or 
(at your option) any later version.

Destroy FX Library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Destroy FX Library.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org

This is our MIDI stuff.
---------------------------------------------------------------*/

#pragma once


#include <array>
#include <optional>
#include <utility>
#include <vector>

#include "dfxenvelope.h"
#include "dfxsmoothedvalue.h"


//-----------------------------------------------------------------------------
class DfxMidi
{
public:
	static constexpr int kNumNotes = 128;
	static constexpr int kNumNotesWithLegatoVoice = kNumNotes + 1;
	static constexpr int kLegatoVoiceNoteIndex = kNumNotesWithLegatoVoice - 1;
	static constexpr int kMaxValue = 0x7F;
	static constexpr int kMaxChannelValue = 0x0F;
	static constexpr int kMidpointValue = 64;
	static constexpr float kValueScalar = 1.0f / static_cast<float>(kMaxValue);
	static constexpr int kPitchBendMidpointValue = 0x2000;
	static constexpr int kPitchBendMaxValue = 0x3FFF;
	static constexpr double kPitchBendSemitonesMax = 36.0;

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

	DfxMidi();

	void reset();  // resets the variables
	void setSampleRate(double inSampleRate);
	void setChannelCount(size_t inChannelCount);
	void setEnvParameters(double inAttackDur, double inDecayDur, double inSustainLevel, double inReleaseDur);  // ADSR
	void setEnvParameters(double inAttackDur, double inReleaseDur);  // AR
	void setEnvCurveType(DfxEnvelope::CurveType inCurveType);
	void setResumedAttackMode(bool inNewMode);

	// handlers for the types of MIDI events that we support
	void handleNoteOn(int inMidiChannel, int inNoteNumber, int inVelocity, size_t inOffsetFrames);
	void handleNoteOff(int inMidiChannel, int inNoteNumber, int inVelocity, size_t inOffsetFrames);
	void handleAllNotesOff(int inMidiChannel, size_t inOffsetFrames);
	void handleChannelAftertouch(int inMidiChannel, int inValue, size_t inOffsetFrames);
	void handlePitchBend(int inMidiChannel, int inValueLSB, int inValueMSB, size_t inOffsetFrames);
	void handleCC(int inMidiChannel, int inControllerNumber, int inValue, size_t inOffsetFrames);
	void handleProgramChange(int inMidiChannel, int inProgramNumber, size_t inOffsetFrames);

	void preprocessEvents(size_t inNumFrames);
	void postprocessEvents();

	// this is where new MIDI events are reckoned with during audio processing
	void heedEvents(size_t inEventIndex, float inVelocityCurve, float inVelocityInfluence);

	auto getBlockEventCount() const noexcept
	{
		return mNumBlockEvents;
	}
	auto const& getBlockEvent(size_t inIndex) const
	{
		return mBlockEvents.at(inIndex);
	}
	// XXX TODO: this is a hack just for Buffer Override, maybe should rethink
	void invalidateBlockEvent(size_t inIndex)
	{
		mBlockEvents.at(inIndex).mStatus = kInvalidValue;
	}

	bool isNoteActive(int inMidiNote) const;

	// manage the ordered queue of active MIDI notes
	void insertNote(int inMidiNote);
	void removeNote(int inMidiNote);
	void removeAllNotes();
	// query the ordered queue of active MIDI notes
	bool isAnyNoteActive() const noexcept
	{
		return noteIsValid(mNoteQueue.front());
	}
	auto getLatestNote() const noexcept
	{
		auto const note = mNoteQueue.front();
		return noteIsValid(note) ? std::make_optional(note) : std::nullopt;
	}

	static constexpr bool isNote(int inMidiStatus) noexcept
	{
		return (inMidiStatus == kStatus_NoteOn) || (inMidiStatus == kStatus_NoteOff);
	}

	double getNoteFrequency(int inMidiNote) const;
	// the gain for the note, scaled with velocity, curve, and influence
	float getNoteAmplitude(int inMidiNote) const;

	auto getPitchBend() const noexcept
	{
		return mPitchBend;
	}
	void setPitchBendRange(double inSemitoneRange);

	// returns -1.0 to 1.0 normalized value 
	static double calculatePitchBendScalar(int inValueLSB, int inValueMSB) noexcept;

	bool isLegatoMode() const noexcept
	{
		return mLegatoMode;
	}
	void setLegatoMode(bool inEnable);

	// this calculates fade scalars if attack, decay, or release are happening
	float processEnvelope(int inMidiNote);
	// ...or lowpass gate coefficients and a post-filter gain
	std::pair<dfx::IIRFilter::Coefficients, float> processEnvelopeLowpassGate(int inMidiNote);

	// this writes the audio output for smoothing the tips of cut-off notes
	// by sloping down from the last sample outputted by the note
	void processSmoothingOutputSample(float* const* outAudio, size_t inNumFrames, int inMidiNote);

	// this writes the audio output for smoothing the tips of cut-off notes
	// by fading out the samples stored in the tail buffers
	void processSmoothingOutputBuffer(float* const* outAudio, size_t inNumFrames, int inMidiNote);


private:
	static constexpr size_t kEventQueueSize = 12'000;
	static constexpr int kInvalidValue = -1;  // sentinel for any MIDI value type

	//-----------------------------------------------------------------------------
	// this holds MIDI event information
	struct Event
	{
		int mStatus = 0;  // the event status MIDI byte
		int mByte1 = 0;  // the first MIDI data byte
		int mByte2 = 0;  // the second MIDI data byte
		int mChannel = 0;  // the MIDI channel
		size_t mOffsetFrames = 0;  // the delta offset (the sample position in the current block where the event occurs)
		size_t mOrderOfArrival = 0;  // the sequence within the current block in which the event arrived (hack to achieve stable sort)
	};

	//-----------------------------------------------------------------------------
	// this holds information for each MIDI note
	struct MusicNote
	{
		int mVelocity = 0;  // note velocity (7-bit MIDI value)
		dfx::SmoothedValue<float> mNoteAmp {0.0};  // the gain for the note, scaled with velocity, curve, and influence
		DfxEnvelope mEnvelope;
	};

	//-----------------------------------------------------------------------------
	struct MusicNoteAudio
	{
		MusicNoteAudio() = default;
		~MusicNoteAudio() = default;
		// deleting copy operations to prevent accidental dynamic allocation in realtime context
		MusicNoteAudio(MusicNoteAudio const&) = delete;
		MusicNoteAudio& operator=(MusicNoteAudio const&) = delete;
		MusicNoteAudio(MusicNoteAudio&&) noexcept = default;
		MusicNoteAudio& operator=(MusicNoteAudio&&) noexcept = default;

		std::vector<float> mLastOutValue;  // capture the most recent output value of each audio channel for smoothing, if necessary
		size_t mSmoothSamples = 0;  // counter for quickly fading cut-off notes, for smoothity
		std::vector<std::vector<float>> mTails;  // per-channel little buffer of output samples for smoothing a cut-off note
	};

	void fillFrequencyTable();

	bool incNumEvents();  // increment the block events counter, safely

	MusicNote const& getNoteState(int inMidiNote) const;
	MusicNote& getNoteStateMutable(int inMidiNote);
	void turnOffNote(int inMidiNote);

	void postprocessEnvelope(MusicNote& inNote);

	static constexpr bool noteIsValid(int inMidiNote) noexcept
	{
		return ((inMidiNote >= 0) && (inMidiNote <= kMaxValue));
	}

	std::array<MusicNote, kNumNotes> mNoteTable {};  // a table with important data about each note
	std::array<MusicNoteAudio, kNumNotes> mNoteAudioTable {};
	std::array<double, kNumNotes> mNoteFrequencyTable {};  // a table of the frequency corresponding to each MIDI note

	// legato is handled by its own voice/note instance, separate from the array MIDI note numbers
	MusicNote mLegatoVoice;
	// therefore the MIDI note number associated with the voice is stored independently,
	// given that it can change with new note events, but only applies to the single voice
	int mActiveLegatoMidiNote = kInvalidValue;

	std::array<int, kNumNotes> mNoteQueue {};  // a chronologically ordered queue of all active notes
	std::array<Event, kEventQueueSize> mBlockEvents {};  // the new MIDI events for a given processing block
	size_t mNumBlockEvents = 0;  // the number of new MIDI events in a given processing block

	double mPitchBendNormalized = 0.0;  // bipolar normalized value of the most recent pitchbend message
	double mPitchBend = 1.0;  // a frequency scalar value for the current pitchbend setting
	double mPitchBendRange = 0.0;  // plus/minus range of pitchbend's effect in semitones

	std::array<bool, kNumNotes> mSustainedNotes {};  // which notes have deferred note-offs for when the sustain pedal is active

	bool mSustain = false;  // whether sustain pedal is active
	bool mLegatoMode = false;

	size_t mStolenNoteFadeDur = 0;
	float mStolenNoteFadeStep = 0.0f;
};
