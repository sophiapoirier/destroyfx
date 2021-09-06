/*------------------------------------------------------------------------
Copyright (C) 2001-2021  Sophia Poirier

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

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#pragma once

#include <atomic>
#include <vector>

#include "bufferoverride-base.h"
#include "dfxplugin.h"
#include "dfxsmoothedvalue.h"
#include "lfo.h"
#include "temporatetable.h"


//-----------------------------------------------------------------------------
class BufferOverride final : public DfxPlugin
{
public:
	explicit BufferOverride(TARGET_API_BASE_INSTANCE_TYPE inInstance);

	void reset() override;

	void processaudio(float const* const* inAudio, float* const* outAudio, unsigned long inNumFrames) override;
	void processparameters() override;

	void createbuffers() override;
	void releasebuffers() override;

	long dfx_GetPropertyInfo(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned long inItemIndex, size_t& outDataSize, dfx::PropertyFlags& outFlags) override;
	long dfx_GetProperty(dfx::PropertyID, dfx::Scope inScope, unsigned long inItemIndex, void* outData) override;

private:
	static constexpr long kNumPresets = 16;
	// We need this stuff to get some maximum buffer size and allocate for that.
	// This is 42 bpm, which should be sufficient.
	static constexpr double kMinAllowableBPS = 0.7;

	void updateBuffer(unsigned long samplePos);

	void heedBufferOverrideEvents(unsigned long samplePos);
	float getDivisorParameterFromNote(int currentNote);
	float getDivisorParameterFromPitchbend(int valueLSB, int valueMSB);

	void initPresets();

	void updateViewDataCache();

	// the parameters
	float mDivisor = 1.0f, mBufferSizeMS = 0.0f, mBufferSizeSync = 0.0f;
	bool mBufferTempoSync = false, mBufferInterrupt = false, mUseHostTempo = false;
	float mSmoothPortion = 0.0f;
	double mPitchBendRange = 0.0, mUserTempoBPM = 0.0;
	long mMidiMode = 0;
	bool mDivisorLFOTempoSync = false, mBufferLFOTempoSync = false;
	float mDivisorLFORateHz = 0.0f, mBufferLFORateHz = 0.0f;  // LFO rate (in Hz)
	float mDivisorLFOTempoRate = 0.0f, mBufferLFOTempoRate = 0.0f;  // LFO rate (in cycles per beat)

	dfx::SmoothedValue<float> mInputGain, mOutputGain;  // the effective states of the dry/wet mix

	long mCurrentForcedBufferSize = 0;  // the size of the larger, imposed buffer
	std::vector<std::vector<float>> mBuffers;  // this stores the forced buffer
	std::vector<float> mAudioOutputValues;  // array of current audio output values (1 for each channel)
	long mWritePos = 0;  // the current sample position within the forced buffer

	long mMinibufferSize = 0;  // the current size of the divided "mini" buffer
	long mPrevMinibufferSize = 0;  // the previous size
	long mReadPos = 0;  // the current sample position within the minibuffer

	// Like mMinibufferSize, but just the size of the first repetition
	// with no complexity about the boundary case at the end. Just used for
	// visualization.
	long mMinibufferSizeForView = 0;
  
	float mOneDivSR = 0.0f;  // the inverse of the sampling rate

	double mCurrentTempoBPS = 0.0;  // tempo in beats per second
	bool mNeedResync = false;
	dfx::TempoRateTable const mTempoRateTable;

	long mSmoothDur = 0, mSmoothCount = 0;  // total duration and sample counter for the minibuffer transition smoothing period
//	float mSmoothStep = 0.0f;  // the gain increment for each sample "step" during the smoothing period
//	float mSqrtFadeIn = 0.0f, mSqrtFadeOut = 0.0f;  // square root of the smoothing gains, for equal power crossfading
//	float mSmoothFract = 0.0f;

	double mPitchBend = 0.0, mOldPitchBend = 0.0;  // pitchbending scalar values
	bool mOldNote = false;  // says if there was an old, unattended note-on or note-off from a previous block
	int mLastNoteOn = 0, mLastPitchbendLSB = 0, mLastPitchbendMSB = 0;  // these carry over the last events from a previous processing block
	bool mDivisorWasChangedByHand = false;  // for MIDI trigger mode - tells us to respect the mDivisor value
	bool mDivisorWasChangedByMIDI = false;  // tells the GUI that the divisor displays need updating

	dfx::LFO mDivisorLFO, mBufferLFO;

	float mFadeOutGain = 0.0f, mFadeInGain = 0.0f, mRealFadePart = 0.0f, mImaginaryFadePart = 0.0f;  // for trig crossfading

	BufferOverrideViewData_DSP mViewDataCache;
	std::atomic<uint64_t> mLastViewDataCacheTimestamp {0u};
	static_assert(decltype(mLastViewDataCacheTimestamp)::is_always_lock_free);
};
