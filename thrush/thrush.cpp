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

#include "thrush.h"

#include <algorithm>
#include <cmath>

#include "dfxmisc.h"


DFX_EFFECT_ENTRY(Thrush)

//-------------------------------------------------------------------------
Thrush::Thrush(TARGET_API_BASE_INSTANCE_TYPE inInstance)
:	DfxPlugin(inInstance, kParameterCount, kPresetCount),
	mTempoRateTable(dfx::TempoRateTable::Rates::Slow)
{
	auto const numTempoRates = mTempoRateTable.getNumRates();
	auto const unitTempoRateIndex = mTempoRateTable.getNearestTempoRateIndex(1.f);

	initparameter_i(kDelay, {"inverse delay", "seven", "six", "four"}, 9, 9, kDelaySamplesMin, kDelaySamplesMax, DfxParam::Unit::Samples);
	initparameter_f(kTempo, dfx::MakeParameterNames(dfx::kParameterNames_Tempo), 120., 120., 39.f, 480.f, DfxParam::Unit::BPM);
	initparameter_b(kTempoAuto, dfx::MakeParameterNames(dfx::kParameterNames_TempoAuto), true, true);
	initparameter_f(kLFO1Rate_Hz, {"LFO1 rate (free)", "seven", "six", "four"}, 3., 3., kLFORateMin, kLFORateMax, DfxParam::Unit::Hz, DfxParam::Curve::Squared);
	initparameter_list(kLFO1Rate_Sync, {"LFO1 rate (sync)", "seven", "six", "four"}, unitTempoRateIndex, unitTempoRateIndex, numTempoRates, DfxParam::Unit::Beats);
	initparameter_b(kLFO1tempoSync, {"LFO1 tempo sync", "seven", "six", "four"}, false, false);
	initparameter_f(kLFO1depth, {"LFO1 depth", "seven", "six", "four"}, 81., 0., 0., 100., DfxParam::Unit::Percent);
	initparameter_list(kLFO1shape, {"LFO1 shape", "seven", "six", "four"}, dfx::LFO::kShape_Thorn, dfx::LFO::kShape_Thorn, dfx::LFO::kNumShapes);
	initparameter_f(kLFO2Rate_Hz, {"LFO2 rate (free)", "seven", "six", "four"}, 6., 6., kLFORateMin, kLFORateMax, DfxParam::Unit::Hz, DfxParam::Curve::Squared);
	initparameter_list(kLFO2Rate_Sync, {"LFO2 rate (sync)", "seven", "six", "four"}, unitTempoRateIndex, unitTempoRateIndex, numTempoRates, DfxParam::Unit::Beats);
	initparameter_b(kLFO2tempoSync, {"LFO2 tempo sync", "seven", "six", "four"}, false, false);
	initparameter_f(kLFO2depth, {"LFO2 depth", "seven", "six", "four"}, 0., 0., kLFO2DepthMin, kLFO2DepthMax, DfxParam::Unit::Scalar);
	initparameter_list(kLFO2shape, {"LFO2 shape", "seven", "six", "four"}, dfx::LFO::kShape_Triangle, dfx::LFO::kShape_Triangle, dfx::LFO::kNumShapes);
	initparameter_b(kStereoLink, {"stereo link", "seven", "six", "four"}, true, true);
	initparameter_i(kDelay2, {"inverse delay R", "seven", "six", "four"}, 3, 3, kDelaySamplesMin, kDelaySamplesMax, DfxParam::Unit::Samples);
	initparameter_f(kLFO1Rate2_Hz, {"LFO1 rate R (free)", "seven", "six", "four"}, 9., 9., kLFORateMin, kLFORateMax, DfxParam::Unit::Hz, DfxParam::Curve::Squared);
	initparameter_list(kLFO1Rate2_Sync, {"LFO1 rate R (sync)", "seven", "six", "four"}, unitTempoRateIndex, unitTempoRateIndex, numTempoRates, DfxParam::Unit::Beats);
	initparameter_b(kLFO1tempoSync2, {"LFO1 tempo sync R", "seven", "six", "four"}, false, false);
	initparameter_f(kLFO1depth2, {"LFO1 depth R", "seven", "six", "four"}, 30., 0., 0., 100., DfxParam::Unit::Percent);
	initparameter_list(kLFO1shape2, {"LFO1 shape R", "seven", "six", "four"}, dfx::LFO::kShape_Square, dfx::LFO::kShape_Square, dfx::LFO::kNumShapes);
	initparameter_f(kLFO2Rate2_Hz, {"LFO2 rate R (free)", "seven", "six", "four"}, 12., 12., kLFORateMin, kLFORateMax, DfxParam::Unit::Hz, DfxParam::Curve::Squared);
	initparameter_list(kLFO2Rate2_Sync, {"LFO2 rate R (sync)", "seven", "six", "four"}, unitTempoRateIndex, unitTempoRateIndex, numTempoRates, DfxParam::Unit::Beats);
	initparameter_b(kLFO2tempoSync2, {"LFO2 tempo sync R", "seven", "six", "four"}, false, false);
	initparameter_f(kLFO2depth2, {"LFO2 depth R", "seven", "six", "four"}, 0., 0., kLFO2DepthMin, kLFO2DepthMax, DfxParam::Unit::Scalar);
	initparameter_list(kLFO2shape2, {"LFO2 shape R", "seven", "six", "four"}, dfx::LFO::kShape_Saw, dfx::LFO::kShape_Saw, dfx::LFO::kNumShapes);
	initparameter_f(kDryWetMix, dfx::MakeParameterNames(dfx::kParameterNames_DryWetMix), 100., 50., 0.0, 100.0, DfxParam::Unit::DryWetMix);

	// set the value strings for the LFO shape parameters
	for (dfx::LFO::Shape i = 0; i < dfx::LFO::kNumShapes; i++)
	{
		auto const shapeName = dfx::LFO::getShapeName(i);
		setparametervaluestring(kLFO1shape, i, shapeName);
		setparametervaluestring(kLFO2shape, i, shapeName);
		setparametervaluestring(kLFO1shape2, i, shapeName);
		setparametervaluestring(kLFO2shape2, i, shapeName);
	}
	// set the value strings for the sync rate parameters
	for (long i = 0; i < mTempoRateTable.getNumRates(); i++)
	{
		auto const& tempoRateName = mTempoRateTable.getDisplay(i);
		setparametervaluestring(kLFO1Rate_Sync, i, tempoRateName);
		setparametervaluestring(kLFO2Rate_Sync, i, tempoRateName);
		setparametervaluestring(kLFO1Rate2_Sync, i, tempoRateName);
		setparametervaluestring(kLFO2Rate2_Sync, i, tempoRateName);
	}

	addparametergroup("global/left",
	{
		kDelay,
		kLFO1Rate_Hz, kLFO1Rate_Sync, kLFO1tempoSync, kLFO1depth, kLFO1shape,
		kLFO2Rate_Hz, kLFO2Rate_Sync, kLFO2tempoSync, kLFO2depth, kLFO2shape
	});
	addparametergroup("right",
	{
		kStereoLink, kDelay2,
		kLFO1Rate2_Hz, kLFO1Rate2_Sync, kLFO1tempoSync2, kLFO1depth2, kLFO1shape2,
		kLFO2Rate2_Hz, kLFO2Rate2_Sync, kLFO2tempoSync2, kLFO2depth2, kLFO2shape2
	});

	addchannelconfig(2, 2);  // stereo in / stereo out
	//addchannelconfig(1, 2);  // it's okay to feed both inputs with the same signal

	settailsize_samples(kDelayBufferSize);

	mCurrentTempoBPS = getparameter_f(kTempo) / 60.f;

	setpresetname(0, "thrush");  // default preset name
	initPresets();
}

//-------------------------------------------------------------------------
long Thrush::initialize()
{
	mDelayBuffer.assign(kDelayBufferSize, 0.f);
	mDelayBuffer2.assign(kDelayBufferSize, 0.f);

	// this is a handy value to have during LFO calculations and wasteful to recalculate at every sample
	mOneDivSR = 1.f / getsamplerate_f();

	return dfx::kStatus_NoError;
}

//-------------------------------------------------------------------------
void Thrush::cleanup()
{
	mDelayBuffer = {};
	mDelayBuffer2 = {};
}

//-------------------------------------------------------------------------
void Thrush::reset()
{
	mInputPosition = 0;
	mLFO1.reset();
	mLFO2.reset();
	mLFO1_2.reset();
	mLFO2_2.reset();
	mDelayPosition = mDelayPosition2 = mOldDelayPosition = mOldDelayPosition2 = 0;

	std::fill(mDelayBuffer.begin(), mDelayBuffer.end(), 0.f);
	std::fill(mDelayBuffer2.begin(), mDelayBuffer2.end(), 0.f);

	mNeedResync = true;
}

//-------------------------------------------------------------------------
void Thrush::initPresets()
{
	long i = 1;

	setpresetname(i, "vibber");
//	setpresetparameter_i(i, kDelay, );
	setpresetparameter_b(i, kLFO1tempoSync, false);
//	setpresetparameter_f(i, kLFO1Rate_Hz, );
//	setpresetparameter_f(i, kLFO1depth, );
	setpresetparameter_i(i, kLFO1shape, dfx::LFO::kShape_Saw);
	setpresetparameter_b(i, kStereoLink, false);
//	setpresetparameter_i(i, kDelay2, );
	setpresetparameter_b(i, kLFO1tempoSync2, false);
//	setpresetparameter_f(i, kLFO1Rate2_Hz, );
//	setpresetparameter_f(i, kLFO1depth2, );
	setpresetparameter_i(i, kLFO1shape2, dfx::LFO::kShape_Saw);
	setpresetparameter_f(i, kDryWetMix, 100.);
	i++;
};


#pragma mark -

//-------------------------------------------------------------------------
void Thrush::calculateEffectiveTempo()
{
	// figure out the current tempo if any of our LFOs are tempo synced
	if ((mLFO1.mTempoSync || mLFO2.mTempoSync) || 
		 (!mStereoLink && (mLFO1_2.mTempoSync || mLFO2_2.mTempoSync)))
	{
		// calculate the tempo at the current processing buffer
		if (mUseHostTempo && hostCanDoTempo() && gettimeinfo().mTempoIsValid)  // get the tempo from the host
		{
			mCurrentTempoBPS = gettimeinfo().mTempo_BPS;
			// check if audio playback has just restarted and reset cycle state stuff if it has (for measure sync)
			mNeedResync |= gettimeinfo().mPlaybackChanged;
		}
		// get the tempo from the user parameter
		else
		{
			mCurrentTempoBPS = mUserTempoBPM / 60.f;
			mNeedResync = false;  // we don't want it true if we're not syncing to host tempo
		}
	}
}

//-------------------------------------------------------------------------
void Thrush::calculateEffectiveRate(ThrushLFO& lfo) const
{
	lfo.mEffectiveRateHz = lfo.mTempoSync ? (mCurrentTempoBPS * lfo.mTempoRateScalar) : lfo.mRateHz;
	// get the increment for each step through the LFO's cycle phase at its current frequency
	lfo.setStepSize(lfo.mEffectiveRateHz * mOneDivSR);
}

//-------------------------------------------------------------------------
// modulate the first layer LFO's rate with the second layer LFO and then output the first layer LFO output
float Thrush::processLFOs(ThrushLFO& lfoLayer1, ThrushLFO& lfoLayer2) const
{
	// do beat sync if it ought to be done
	if (mNeedResync && lfoLayer2.mTempoSync && gettimeinfo().mSamplesToNextBarIsValid)
	{
		lfoLayer2.syncToTheBeat(gettimeinfo().mSamplesToNextBar);
	}

	lfoLayer2.updatePosition();  // move forward another sample position
	float lfoOffset = lfoLayer2.process();  // this is the offset from the first layer LFO's rate, caused by the second layer LFO

	// scale the 0 - 1 LFO output value to the depth range of the second layer LFO
	lfoOffset = lfo2DepthScaled(lfoOffset);

	// update the first layer LFO's cycle phase step size as modulated by the second layer LFO
	lfoLayer1.setStepSize(lfoLayer1.mEffectiveRateHz * lfoOffset * mOneDivSR);
	// do beat sync if it must be done (don't do it if the 2nd layer LFO is active; that's too much to deal with)
	if (mNeedResync && (lfoLayer1.mTempoSync && !lfoLayer2.isActive()) && gettimeinfo().mSamplesToNextBarIsValid)
	{
		lfoLayer1.syncToTheBeat(gettimeinfo().mSamplesToNextBar);
	}

	lfoLayer1.updatePosition();  // move forward another sample position
	// scale the 0 - 1 LFO output value to 0 - 2 (oscillating around 1)
	return lfoLayer1.processZeroToTwo();
}

//-------------------------------------------------------------------------
void Thrush::processparameters()
{
	mDelay_gen = getparameter_gen(kDelay);
	mUserTempoBPM = getparameter_f(kTempo);
	mUseHostTempo = getparameter_b(kTempoAuto);

	mLFO1.mRateHz = getparameter_f(kLFO1Rate_Hz);
	// make sure the cycles match up if the tempo rate has changed
	if (auto const value = getparameterifchanged_f(kLFO1Rate_Sync))
	{
		mLFO1.mTempoRateScalar = *value;
		mNeedResync = true;
	}

	if (auto const value = getparameterifchanged_b(kLFO1tempoSync))
	{
		mLFO1.mTempoSync = *value;
		// need to resync if tempo sync mode has just been switched on
		mNeedResync |= *value;
	}

	mLFO1.setDepth(getparameter_scalar(kLFO1depth));
	mLFO1.setShape(getparameter_i(kLFO1shape));

	mLFO2.mRateHz = getparameter_f(kLFO2Rate_Hz);
	// make sure the cycles match up if the tempo rate has changed
	if (auto const value = getparameterifchanged_f(kLFO2Rate_Sync))
	{
		mLFO2.mTempoRateScalar = *value;
		mNeedResync = true;
	}

	if (auto const value = getparameterifchanged_b(kLFO2tempoSync))
	{
		mLFO2.mTempoSync = *value;
		// need to resync if tempo sync mode has just been switched on
		mNeedResync |= *value;
	}

	mLFO2.setDepth(getparameter_gen(kLFO2depth));
	mLFO2.setShape(getparameter_i(kLFO2shape));

	mStereoLink = getparameter_b(kStereoLink);

	mDelay2_gen = getparameter_gen(kDelay2);
	mLFO1_2.mRateHz = getparameter_f(kLFO1Rate2_Hz);
	mLFO1_2.mTempoRateScalar = mTempoRateTable.getScalar(getparameter_i(kLFO1Rate2_Sync));

	if (auto const value = getparameterifchanged_b(kLFO1tempoSync2))
	{
		mLFO1_2.mTempoSync = *value;
		// need to resync if tempo sync mode has just been switched on
		mNeedResync |= (!mStereoLink && *value);
	}

	mLFO1_2.setDepth(getparameter_scalar(kLFO1depth2));
	mLFO1_2.setShape(getparameter_i(kLFO1shape2));
	mLFO2_2.mRateHz = getparameter_f(kLFO2Rate2_Hz);
	mLFO2_2.mTempoRateScalar = mTempoRateTable.getScalar(getparameter_i(kLFO2Rate2_Sync));

	if (auto const value = getparameterifchanged_b(kLFO2tempoSync2))
	{
		mLFO2_2.mTempoSync = *value;
		// need to resync if tempo sync mode has just been switched on
		mNeedResync |= (!mStereoLink && *value);
	}

	mLFO2_2.setDepth(getparameter_gen(kLFO2depth2));
	mLFO2_2.setShape(getparameter_i(kLFO2shape2));
	mDryWetMix = getparameter_scalar(kDryWetMix);
}

//-------------------------------------------------------------------------
void Thrush::processaudio(float const* const* inAudio, float* const* outAudio, unsigned long inNumFrames)
{
	float const inputGain = 1.f - (mDryWetMix * 0.5f);
	float const inverseGain = mDryWetMix * 0.5f;


	// set up the basic startup conditions of all of the LFOs if any LFOs are turned on
	calculateEffectiveTempo();
	calculateEffectiveRate(mLFO1);
	calculateEffectiveRate(mLFO2);
	calculateEffectiveRate(mLFO1_2);
	calculateEffectiveRate(mLFO2_2);

	for (unsigned long samplecount = 0; samplecount < inNumFrames; samplecount++)
	{
		auto const normalizedOffset = [this](long parameterID, float normalizedValue, float offset)
		{
			return static_cast<long>(expandparametervalue(parameterID, normalizedValue * offset) + DfxParam::kIntegerPadding);
		};
		// evaluate the sample-by-sample output of the LFOs
		auto delayOffset = processLFOs(mLFO1, mLFO2);
		// update the delay position(s)   (this is done every sample in case LFOs are active)
		mDelayPosition = (mInputPosition - normalizedOffset(kDelay, mDelay_gen, delayOffset) + kDelayBufferSize) % kDelayBufferSize;
		if (mStereoLink)
		{
			mDelayPosition2 = mDelayPosition;
		}
		else
		{
			delayOffset = processLFOs(mLFO1_2, mLFO2_2);
			mDelayPosition2 = (mInputPosition - normalizedOffset(kDelay2, mDelay2_gen, delayOffset) + kDelayBufferSize) % kDelayBufferSize;
		}
		mNeedResync = false;  // make sure it gets set false so that it doesn't happen again when it shouldn't

		// audio output ... first reckon with the anything that needs to be smoothed (this is a mess)
		if ((mLFO1.mSmoothSamples > 0) || (!mStereoLink && (mLFO1_2.mSmoothSamples > 0)))
		{
			// give the right channel the same fade position if stereo link is on
			if (mStereoLink)
			{
				mLFO1_2.mSmoothSamples = mLFO1.mSmoothSamples;
			}

			////////////   doing the left channel first   ////////////
			// If the fade is at the very beginning, don't actually start fading anything yet.
			// Instead take the current output sample and store it as the blending out value.
			if (mLFO1.mSmoothSamples == ThrushLFO::kSmoothDur)
			{
				mOldDelayPosition = mDelayPosition;
				mLastSample = (inAudio[0][samplecount] * inputGain) - (mDelayBuffer[mDelayPosition] * inverseGain);
				outAudio[0][samplecount] = mLastSample;
			}
			// do regular fade-out/mixing stuff
			else if (mLFO1.mSmoothSamples > 0)
			{
				mOldDelayPosition = (mOldDelayPosition + 1) % kDelayBufferSize;
				auto const oldGain = static_cast<float>(mLFO1.mSmoothSamples) * ThrushLFO::kSmoothStep;
				auto const newGain = 1.f - oldGain;
				outAudio[0][samplecount] = (inAudio[0][samplecount] * inputGain) - 
											((mDelayBuffer[mDelayPosition] * inverseGain) * newGain) - 
											((mDelayBuffer[mOldDelayPosition] * inverseGain) * oldGain);
			}
			// if neither of the first two conditions were true, then the left channel 
			// is not fading/blending at all - the right channel must be - do regular output
			else
			{
				outAudio[0][samplecount] = (inAudio[0][samplecount] * inputGain) - 
											(mDelayBuffer[mDelayPosition] * inverseGain);
			}

			////////////   now the right channel - same stuff as above   ////////////
			if (mLFO1_2.mSmoothSamples == ThrushLFO::kSmoothDur)
			{
				mOldDelayPosition2 = mDelayPosition2;
				mLastSample2 = (inAudio[1][samplecount] * inputGain) - (mDelayBuffer2[mDelayPosition2] * inverseGain);
				outAudio[1][samplecount] = mLastSample2;
			}
			else if (mLFO1_2.mSmoothSamples > 0)
			{
				mOldDelayPosition2 = (mOldDelayPosition2 + 1) % kDelayBufferSize;
				auto const oldGain = static_cast<float>(mLFO1_2.mSmoothSamples) * ThrushLFO::kSmoothStep;
				auto const newGain = 1.f - oldGain;
				outAudio[1][samplecount] = (inAudio[1][samplecount] * inputGain) - 
											((mDelayBuffer2[mDelayPosition2] * inverseGain) * newGain) - 
											((mDelayBuffer2[mOldDelayPosition2] * inverseGain) * oldGain);
			}
			else
			{
				outAudio[1][samplecount] = (inAudio[1][samplecount] * inputGain) - 
											(mDelayBuffer2[mDelayPosition2] * inverseGain);
			}

			// decrement the counters if they are currently counting
			if (mLFO1.mSmoothSamples > 0)
			{
				mLFO1.mSmoothSamples--;
			}
			if (mLFO1_2.mSmoothSamples > 0)
			{
				mLFO1_2.mSmoothSamples--;
			}
		}


		// no smoothing is going on   (this is much simpler)
		else
		{
			outAudio[0][samplecount] = (inAudio[0][samplecount] * inputGain) - 
									(mDelayBuffer[mDelayPosition] * inverseGain);
			outAudio[1][samplecount] = (inAudio[1][samplecount] * inputGain) - 
									(mDelayBuffer2[mDelayPosition2] * inverseGain);
		}

		// put the latest input samples into the delay buffer
		mDelayBuffer[mInputPosition] = inAudio[0][samplecount];
		mDelayBuffer2[mInputPosition] = inAudio[1][samplecount];

		// incremement the input position in the delay buffer and wrap it around 
		// if it has reached the end of the buffer
		mInputPosition = (mInputPosition + 1) % kDelayBufferSize;
	}
}