/*------------------------------------------------------------------------
Copyright (C) 2001-2024  Sophia Poirier

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

#pragma once

#include <array>
#include <optional>
#include <span>
#include <vector>

#include "bufferoverride-base.h"
#include "dfxmath.h"
#include "dfxmisc.h"
#include "dfxplugin.h"
#include "dfxsmoothedvalue.h"
#include "iirfilter.h"
#include "lfo.h"
#include "temporatetable.h"


//-----------------------------------------------------------------------------
class BufferOverride final : public DfxPlugin
{
public:
	explicit BufferOverride(TARGET_API_BASE_INSTANCE_TYPE inInstance);

	void initialize() override;
	void cleanup() override;
	void reset() override;

	void processaudio(std::span<float const* const> inAudio, std::span<float* const> outAudio, size_t inNumFrames) override;
	void processparameters() override;
	void parameterChanged(dfx::ParameterID inParameterID) override;

	dfx::StatusCode dfx_GetPropertyInfo(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned int inItemIndex, size_t& outDataSize, dfx::PropertyFlags& outFlags) override;
	dfx::StatusCode dfx_GetProperty(dfx::PropertyID, dfx::Scope inScope, unsigned int inItemIndex, void* outData) override;

private:
	static constexpr size_t kNumPresets = 16;
	// the lowest parameter value before it behaves as one (no division)
	static constexpr float kActiveDivisorMinimum = 2.f;
	// We need this to get some maximum buffer size and allocate for that.
	// This is 42 bpm, which should be sufficient.
	static constexpr double kMinAllowableBPS = 0.7;
	static constexpr float kLFOValueDefault = 1.f;

	long ms2samples(double inSizeMS) const;
	long beat2samples(double inBeatScalar, double inTempoBPS) const;
	void updateBuffer(size_t samplePos, bool& ioViewDataChanged);

	void heedMidiEvents(size_t samplePos);
	float getDivisorParameterFromNote(int currentNote);
	float getDivisorParameterFromPitchbend(int valueLSB, int valueMSB);

	void initPresets();

	void updateViewDataCache();

	// the parameters
	float mDivisor = 1.f;
	double mBufferSizeMS = 0., mBufferSizeSync = 0.;
	bool mBufferTempoSync = false, mBufferInterrupt = false, mUseHostTempo = false;
	float mSmoothPortion = 0.0f;
	double mPitchBendRange = 0.0, mUserTempoBPM = 0.0;
	long mMidiMode {};
	bool mDivisorLFOTempoSync = false, mBufferLFOTempoSync = false;
	double mDivisorLFORateHz = 0., mBufferLFORateHz = 0.;  // LFO rate (in Hz)
	double mDivisorLFOTempoRate = 0., mBufferLFOTempoRate = 0.;  // LFO rate (in cycles per beat)
	double mMinibufferPortion = 1., mMinibufferPortionRandomMin = 1., mEffectiveMinibufferPortion = 1.;
	float mDecayDepth = 0.f;
	long mDecayMode {};
	DecayShape mDecayShape {};

	dfx::SmoothedValue<float> mInputGain, mOutputGain;  // the effective states of the dry/wet mix

	long mCurrentForcedBufferSize = 0;  // the size of the larger, imposed buffer
	std::vector<std::vector<float>> mBuffers;  // this stores the forced buffer
	std::vector<float> mAudioOutputValues;  // array of current audio output values (one for each channel)
	long mWritePos = 0;  // the current sample position within the forced buffer

	long mMinibufferSize = 0;  // the current size of the divided "mini" buffer
	long mMinibufferAudibleLength = 0;  // the audible duty cycle of the current minibuffer
	long mPrevMinibufferSize = 0;  // the previous size
	long mReadPos = 0;  // the current sample position within the minibuffer

	float mMinibufferDecayGain = 1.f, mPrevMinibufferDecayGain = 1.f;
	std::array<std::vector<dfx::IIRFilter>, 2> mDecayFilters;
	std::span<dfx::IIRFilter> mCurrentDecayFilters, mPrevDecayFilters;
	bool mDecayFilterIsLowpass = true;
	dfx::math::RandomEngine mRandomEngine {dfx::math::RandomSeed::Entropic};

	double mOneDivSR = 0.;  // the inverse of the sampling rate

	double mCurrentTempoBPS = 0.0;  // tempo in beats per second
	bool mNeedResync = false;
	dfx::TempoRateTable const mTempoRateTable;

	long mFadeInSmoothLength = 0, mFadeOutSmoothLength = 0;  // total duration and sample counter for the minibuffer transition smoothing period
	long mFadeInSmoothCountDown = 0, mFadeOutSmoothCountDown = 0;
	float mFadeInSmoothStep = 0.f, mFadeOutSmoothStep = 0.f;  // the normalized position increment for each sample "step" during the smoothing period

	double mPitchBend = 0.0, mOldPitchBend = 0.0;  // pitchbending scalar values
	bool mOldNote = false;  // says if there was an old, unattended note-on or note-off from a previous block
	std::optional<int> mLastNoteOn, mLastPitchbendLSB, mLastPitchbendMSB;  // these carry over the last events from a previous processing block
	bool mDivisorWasChangedByHand = false;  // for MIDI trigger mode - tells us to respect the mDivisor value
	bool mDivisorWasChangedByMIDI = false;  // tells the GUI that the divisor displays need updating

	dfx::LFO mDivisorLFO, mBufferLFO;

	AtomicBufferOverrideViewData mViewDataCache;
	dfx::LockFreeAtomic<uint64_t> mViewDataCacheTimestamp {0u};
	dfx::LockFreeAtomic<double> mHostTempoBPS_viewCache {0.};
	dfx::LockFreeAtomic<float> mDivisorLFOValue_viewCache {kLFOValueDefault}, mBufferLFOValue_viewCache {kLFOValueDefault};
};
