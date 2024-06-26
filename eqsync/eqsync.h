/*------------------------------------------------------------------------
Copyright (C) 2001-2024  Sophia Poirier

This file is part of EQ Sync.

EQ Sync is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

EQ Sync is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with EQ Sync.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#pragma once


#include <vector>

#include "dfxmath.h"
#include "dfxplugin.h"
#include "temporatetable.h"


//-----------------------------------------------------------------------------
// these are the plugin parameters:
enum : dfx::ParameterID
{
	kRate_Sync,
	kSmooth,
	kTempo,
	kTempoAuto,
	kA0,
	kA1,
	kA2,
	kB1,
	kB2,

	kNumParameters
};


//-----------------------------------------------------------------------------

class EQSync final : public DfxPlugin
{
public:
	explicit EQSync(TARGET_API_BASE_INSTANCE_TYPE inInstance);

	void initialize() override;
	void cleanup() override;
	void reset() override;

	void processparameters() override;
	void processaudio(std::span<float const* const> inAudio, std::span<float* const> outAudio, size_t inNumFrames) override;

private:
	// the parameters
	double mRate = 1.0, mSmooth = 0.0, mUserTempo = 1.0;
	bool mUseHostTempo = false;
	float mA0 = 1.0f, mA1 = 0.0f, mA2 = 0.0f, mB1 = 0.0f, mB2 = 0.0f;

	long mCycleSamples = 1, mSmoothSamples = 1, mSmoothDur = 1;  // sample counters
	std::vector<float> mPrevIn, mPrevPrevIn;  // these store the previous input samples' values
	std::vector<float> mPrevOut, mPrevPrevOut;  // these store the previous output samples' values
	float mPrevA0 = 0.0f, mPrevA1 = 0.0f, mPrevA2 = 0.0f, mPrevB1 = 0.0f, mPrevB2 = 0.0f;  // these store the last random filter parameter values
	float mCurA0 = 0.0f, mCurA1 = 0.0f, mCurA2 = 0.0f, mCurB1 = 0.0f, mCurB2 = 0.0f;  // these store the current random filter parameter values
	dfx::math::RandomGenerator<float> mRandomGenerator {dfx::math::RandomSeed::Entropic};

	bool mNeedResync = false;  // true when playback has just started up again
	dfx::TempoRateTable const mTempoRateTable;
};
