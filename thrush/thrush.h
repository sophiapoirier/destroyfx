/*------------------------------------------------------------------------
Copyright (C) 2001-2021  Sophia Poirier and Keith Fullerton Whitman

This file is part of Thrush.

Thrush is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Thrush is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Thrush.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#pragma once

#include <vector>

#include "dfxplugin.h"
#include "lfo.h"
#include "temporatetable.h"


//-------------------------------------------------------------------------------------
// these are the plugin parameters:
enum
{
	kDelay,
	kTempo,
	kTempoAuto,

	kLFO1tempoSync,
	kLFO1Rate_Hz,
	kLFO1Rate_Sync,
	kLFO1depth,
	kLFO1shape,

	kLFO2tempoSync,
	kLFO2Rate_Hz,
	kLFO2Rate_Sync,
	kLFO2depth,
	kLFO2shape,

	kStereoLink,
	kDelay2,

	kLFO1tempoSync2,
	kLFO1Rate2_Hz,
	kLFO1Rate2_Sync,
	kLFO1depth2,
	kLFO1shape2,

	kLFO2tempoSync2,
	kLFO2Rate2_Hz,
	kLFO2Rate2_Sync,
	kLFO2depth2,
	kLFO2shape2,

	kDryWetMix,

	kParameterCount
};


//-------------------------------------------------------------------------------------
class Thrush final : public DfxPlugin
{
public:
	Thrush(TARGET_API_BASE_INSTANCE_TYPE inInstance);

	long initialize() override;
	void cleanup() override;
	void reset() override;

	void processaudio(float const* const* inAudio, float* const* outAudio, unsigned long inNumFrames) override;
	void processparameters() override;

private:
	static constexpr long kPresetCount = 16;
	static constexpr long kDelaySamplesMin = 1;
	static constexpr long kDelaySamplesMax = 128;
	static constexpr long kDelayBufferSize = kDelaySamplesMax * 2;
	static constexpr float kLFORateMin = 0.09f;
	static constexpr float kLFORateMax = 21.f;
	static constexpr float kLFO2DepthMin = 1.f;
	static constexpr float kLFO2DepthMax = 9.f;

	class ThrushLFO final : public dfx::LFO
	{
	public:
		bool isActive() const noexcept
		{
			return getDepth() != 0.f;
		}
	private:
		friend class Thrush;
		float mRateHz {};
		float mTempoRateScalar {};
		bool mTempoSync {};
		float mEffectiveRateHz {};
	};

	void initPresets();

	void calculateEffectiveTempo();
	void calculateEffectiveRate(ThrushLFO& lfo) const;
	float processLFOs(ThrushLFO& lfoLayer1, ThrushLFO& lfoLayer2) const;

	static constexpr float lfo2DepthScaled(float value)
	{
		return (value * (kLFO2DepthMax - kLFO2DepthMin)) + kLFO2DepthMin;
	}

	// parameter values
	float mDelay_gen {}, mDelay2_gen {}, mUserTempoBPM {}, mDryWetMix {};
	bool mUseHostTempo = false, mStereoLink = false;

	ThrushLFO mLFO1, mLFO2, mLFO1_2, mLFO2_2;

	// these track the input and delay buffer positions
	long mInputPosition, mDelayPosition, mDelayPosition2, mOldDelayPosition, mOldDelayPosition2;
	std::vector<float> mDelayBuffer, mDelayBuffer2; // left and right channel delay buffers

	float mOneDivSR;  // the inverse of the sampling rate

	float mLastSample, mLastSample2;

	dfx::TempoRateTable const mTempoRateTable;	// a table of tempo rate values
	float mCurrentTempoBPS;	// tempo in beats per second
//	VstTimeInfo* mTimeInfo;
	bool mNeedResync;	// true when playback has just started up again
};
