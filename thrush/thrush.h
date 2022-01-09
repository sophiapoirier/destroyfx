/*------------------------------------------------------------------------
Copyright (C) 2001-2022  Sophia Poirier and Keith Fullerton Whitman

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
#include "dfxsmoothedvalue.h"
#include "lfo.h"
#include "temporatetable.h"


#define THRUSH_LFO_DISCONTINUITY_SMOOTHING 0  // the current implementation is quite flawed


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
	static constexpr double kLFORateMin = 0.09;
	static constexpr double kLFORateMax = 21.;
	static constexpr double kLFO2DepthMin = 1.;
	static constexpr double kLFO2DepthMax = 9.;

	class ThrushLFO final : public dfx::LFO
	{
	public:
		bool isActive() const noexcept
		{
			return getDepth() != 0.;
		}
	private:
		friend class Thrush;
		double mRateHz {};
		double mTempoRateScalar {};
		bool mTempoSync {};
		double mEffectiveRateHz {};
	};

	void initPresets();

	void calculateEffectiveTempo();
	void calculateEffectiveRate(ThrushLFO& lfo) const;
	double processLFOs(ThrushLFO& lfoLayer1, ThrushLFO& lfoLayer2) const;

	// parameter values
	double mDelay_gen {}, mDelay2_gen {};
	double mUserTempoBPM {};
	dfx::SmoothedValue<float> mInputGain, mInverseGain;  // the effective states of the dry/wet mix
	bool mUseHostTempo = false, mStereoLink = false;

	ThrushLFO mLFO1, mLFO2, mLFO1_2, mLFO2_2;

	// these track the input and delay buffer positions
	long mInputPosition {};
	// TODO: smoothing the delay position offsets is a bit overkill because it is only really LFO
	// discontinuity points that need smoothing, and this approach causes all LFO movement to lag
	dfx::SmoothedValue<double> mDelayOffset {}, mDelayOffset2 {};
#if THRUSH_LFO_DISCONTINUITY_SMOOTHING
	long mOldDelayPosition {}, mOldDelayPosition2 {};
#endif
	std::vector<float> mDelayBuffer, mDelayBuffer2; // left and right channel delay buffers

	double mOneDivSR {};  // the inverse of the sampling rate

#if THRUSH_LFO_DISCONTINUITY_SMOOTHING
	float mLastSample {}, mLastSample2 {};  // TODO: these are not actually used in a stateful fashion
#endif

	dfx::TempoRateTable const mTempoRateTable;	// a table of tempo rate values
	double mCurrentTempoBPS {};	// tempo in beats per second
	bool mNeedResync {};	// true when playback has just started up again
};
