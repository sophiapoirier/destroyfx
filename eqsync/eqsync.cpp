/*------------------------------------------------------------------------
Copyright (C) 2001-2021  Sophia Poirier

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

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "eqsync.h"

#include <algorithm>
#include <cmath>

#include "dfxmath.h"
#include "dfxmisc.h"


// this macro does boring entry point stuff for us
DFX_EFFECT_ENTRY(EQSync)

//-----------------------------------------------------------------------------
EQSync::EQSync(TARGET_API_BASE_INSTANCE_TYPE inInstance)
:	DfxPlugin(inInstance, kNumParameters, 1)
{
	auto const numTempoRates = mTempoRateTable.getNumRates();
	auto const unitTempoRateIndex = mTempoRateTable.getNearestTempoRateIndex(1.0f);
	initparameter_list(kRate_Sync, {"rate"}, unitTempoRateIndex, unitTempoRateIndex, numTempoRates, DfxParam::Unit::Beats);
	initparameter_f(kSmooth, {"smooth", "Smth"}, 3.0, 33.333, 0.0, 100.0, DfxParam::Unit::Percent);  // % of cycle
	initparameter_f(kTempo, dfx::MakeParameterNames(dfx::kParameterNames_Tempo), 120.0, 120.0, 39.0, 480.0, DfxParam::Unit::BPM);
	initparameter_b(kTempoAuto, dfx::MakeParameterNames(dfx::kParameterNames_TempoAuto), true, true);
//	for (long i = kA0; i <= kB2; i++)
	{
//		initparameter_f(i, {""}, 0.5, 0.5, 0.0, 1.0, DfxParam::Unit::Generic);
	}
	// okay, giving in and providing actual parameter names because Final Cut Pro folks say that it was causing problems...
	initparameter_f(kA0, {"a0"}, 0.5, 0.5, 0.0, 1.0, DfxParam::Unit::Generic);
	initparameter_f(kA1, {"a1"}, 0.5, 0.5, 0.0, 1.0, DfxParam::Unit::Generic);
	initparameter_f(kA2, {"a2"}, 0.5, 0.5, 0.0, 1.0, DfxParam::Unit::Generic);
	initparameter_f(kB1, {"b1"}, 0.5, 0.5, 0.0, 1.0, DfxParam::Unit::Generic);
	initparameter_f(kB2, {"b2"}, 0.5, 0.5, 0.0, 1.0, DfxParam::Unit::Generic);

	// set the value strings for the sync rate parameters
	for (long i = 0; i < numTempoRates; i++)
	{
		setparametervaluestring(kRate_Sync, i, mTempoRateTable.getDisplay(i));
	}

	addparametergroup("coefficients", {kA0, kA1, kA2, kB1, kB2});


	setpresetname(0, "with motors");  // default preset name

	// give mCurrentTempoBPS a value in case that's useful for a freshly opened GUI
	mCurrentTempoBPS = getparameter_f(kTempo) / 60.0;
}

//-----------------------------------------------------------------------------
long EQSync::initialize()
{
	auto const numChannels = getnumoutputs();

	mPrevIn.assign(numChannels, 0.0f);
	mPrevPrevIn.assign(numChannels, 0.0f);
	mPrevOut.assign(numChannels, 0.0f);
	mPrevPrevOut.assign(numChannels, 0.0f);

	return dfx::kStatus_NoError;
}

//-----------------------------------------------------------------------------
void EQSync::cleanup()
{
	mPrevIn = {};
	mPrevPrevIn = {};
	mPrevOut = {};
	mPrevPrevOut = {};
}

//-----------------------------------------------------------------------------------------
void EQSync::reset()
{
	mCycleSamples = 1;
	mSmoothSamples = 1;
	mSmoothDur = 1;

	std::fill(mPrevIn.begin(), mPrevIn.end(), 0.0f);
	std::fill(mPrevPrevIn.begin(), mPrevPrevIn.end(), 0.0f);
	std::fill(mPrevOut.begin(), mPrevOut.end(), 0.0f);
	std::fill(mPrevPrevOut.begin(), mPrevPrevOut.end(), 0.0f);

	mPrevA0 = mPrevA1 = mPrevA2 = mPrevB1 = mPrevB2 = 0.0f;
	mCurA0 = mCurA1 = mCurA2 = mCurB1 = mCurB2 = 0.0f;

	mNeedResync = true;  // some hosts may call resume when restarting playback
}

//-----------------------------------------------------------------------------
void EQSync::processparameters()
{
	mRate = mTempoRateTable.getScalar(getparameter_i(kRate_Sync));
	mSmooth = getparameter_scalar(kSmooth);
	mUserTempo = getparameter_f(kTempo);
	mUseHostTempo = getparameter_b(kTempoAuto);
	mA0 = getparameter_f(kA0);
	mA1 = getparameter_f(kA1);
	mA2 = getparameter_f(kA2);
	mB1 = getparameter_f(kB1);
	mB2 = getparameter_f(kB2);

	if (getparameterchanged(kRate_Sync))
	{
		mNeedResync = true;  // make sure the cycles match up if the tempo rate has changed
	}
}


//-----------------------------------------------------------------------------
void EQSync::processaudio(float const* const* inAudio, float* const* outAudio, unsigned long inNumFrames)
{
	auto const numChannels = getnumoutputs();
	bool eqChanged = false;

	// . . . . . . . . . . . tempo stuff . . . . . . . . . . . . .
	// calculate the tempo at the current processing buffer
	if (mUseHostTempo && hostCanDoTempo() && gettimeinfo().mTempoIsValid)  // get the tempo from the host
	{
		mCurrentTempoBPS = gettimeinfo().mTempo_BPS;
		// check if audio playback has just restarted and reset buffer stuff if it has (for measure sync)
		if (gettimeinfo().mPlaybackChanged)
		{
			mNeedResync = true;
			mCycleSamples = 0;
		}
	}
	else  // get the tempo from the user parameter
	{
		mCurrentTempoBPS = mUserTempo / 60.0;
		mNeedResync = false;  // we don't want it true if we're not syncing to host tempo
	}
	auto const latestCycleDur = std::lround((getsamplerate() / mCurrentTempoBPS) / mRate);


	for (unsigned long sampleCount = 0; sampleCount < inNumFrames; sampleCount++)
	{
		mCycleSamples--;  // decrement our EQ cycle counter
		if (mCycleSamples <= 0)
		{
			// calculate the lengths of the next cycle and smooth portion
			mCycleSamples = latestCycleDur;
			// see if we need to adjust this cycle so that an EQ change syncs with the next measure
			if (std::exchange(mNeedResync, false))
			{
				mCycleSamples = gettimeinfo().mSamplesToNextBar % mCycleSamples;
			}

			mSmoothSamples = std::lround(static_cast<double>(mCycleSamples) * mSmooth);
			// if mSmoothSamples is 0, make mSmoothDur = 1 to avoid dividing by zero later on
			mSmoothDur = std::max(mSmoothSamples, 1L);

			// refill the "previous" filter parameter containers
			mPrevA0 = mA0;
			mPrevA1 = mA1;
			mPrevA2 = mA2;
			mPrevB1 = mB1;
			mPrevB2 = mB2;

			// generate "current" filter parameter values
			mCurA0 = mRandomGenerator.next();
			mCurA1 = mRandomGenerator.next();
			mCurA2 = mRandomGenerator.next();
			mCurB1 = mRandomGenerator.next();
			mCurB2 = mRandomGenerator.next();

			eqChanged = true;
		}

		// fade from the previous filter parameter values to the current ones
		if (mSmoothSamples >= 0)
		{
			//calculate the changing scalars for the two EQ settings
			float const smoothIn = static_cast<float>(mSmoothDur - mSmoothSamples) / static_cast<float>(mSmoothDur);
			float const smoothOut = static_cast<float>(mSmoothSamples) / static_cast<float>(mSmoothDur);

			mA0 = (mCurA0 * smoothIn) + (mPrevA0 * smoothOut);
			mA1 = (mCurA1 * smoothIn) + (mPrevA1 * smoothOut);
			mA2 = (mCurA2 * smoothIn) + (mPrevA2 * smoothOut);
			mB1 = (mCurB1 * smoothIn) + (mPrevB1 * smoothOut);
			mB2 = (mCurB2 * smoothIn) + (mPrevB2 * smoothOut);

			mSmoothSamples--;  // and decrement the counter
			eqChanged = true;
		}

//.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

		for (unsigned long ch = 0; ch < numChannels; ch++)
		{
			// audio output section -- outputs the latest sample
			auto const inputValue = inAudio[ch][sampleCount];  // because Cubase inserts are goofy
			float outputValue = (inputValue * mA0) + (mPrevIn[ch] * mA1) + (mPrevPrevIn[ch] * mA2) - (mPrevOut[ch] * mB1) - (mPrevPrevOut[ch] * mB2);
			outputValue = dfx::math::ClampDenormal(outputValue);

			// ...doing the complex filter thing here
			outAudio[ch][sampleCount] = outputValue;

			// update the previous sample holders and increment the i/o streams
			mPrevPrevIn[ch] = mPrevIn[ch];
			mPrevIn[ch] = inputValue;
			mPrevPrevOut[ch] = mPrevOut[ch];
			mPrevOut[ch] = outputValue;
		}

	}  // end per-sample loop


	if (eqChanged)
	{
		auto const setAndNotify = [this](auto parameterID, auto parameterValue)
		{
			setparameterquietly_f(parameterID, parameterValue);
			postupdate_parameter(parameterID);  // inform listeners of change
		};
		setAndNotify(kA0, mA0);
		setAndNotify(kA1, mA1);
		setAndNotify(kA2, mA2);
		setAndNotify(kB1, mB1);
		setAndNotify(kB2, mB2);
	}
}
