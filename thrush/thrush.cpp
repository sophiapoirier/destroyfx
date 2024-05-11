/*------------------------------------------------------------------------
Copyright (C) 2001-2024  Sophia Poirier and Keith Fullerton Whitman

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

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#include "thrush.h"

#include <algorithm>
#include <cmath>
#include <numeric>

#include "dfxmath.h"
#include "dfxmisc.h"


DFX_EFFECT_ENTRY(Thrush)

//-------------------------------------------------------------------------
Thrush::Thrush(TARGET_API_BASE_INSTANCE_TYPE inInstance)
:	DfxPlugin(inInstance, kParameterCount, kPresetCount),
	mTempoRateTable(dfx::TempoRateTable::Rates::Slow)
{
	auto const numTempoRates = mTempoRateTable.getNumRates();
	auto const unitTempoRateIndex = mTempoRateTable.getNearestTempoRateIndex(1.);

	initparameter_i(kDelay, {"inverse delay", "seven", "six", "four"}, 9, 9, kDelaySamplesMin, kDelaySamplesMax, DfxParam::Unit::Samples);
	initparameter_f(kTempo, {dfx::kParameterNames_Tempo}, 120., 120., 39.f, 480.f, DfxParam::Unit::BPM);
	initparameter_b(kTempoAuto, {dfx::kParameterNames_TempoAuto}, true);
	initparameter_f(kLFO1Rate_Hz, {"LFO1 rate (free)", "seven", "six", "four"}, 3., 3., kLFORateMin, kLFORateMax, DfxParam::Unit::Hz, DfxParam::Curve::Squared);
	initparameter_list(kLFO1Rate_Sync, {"LFO1 rate (sync)", "seven", "six", "four"}, unitTempoRateIndex, unitTempoRateIndex, numTempoRates, DfxParam::Unit::Beats);
	initparameter_b(kLFO1TempoSync, {"LFO1 tempo sync", "seven", "six", "four"}, false);
	initparameter_f(kLFO1Depth, {"LFO1 depth", "seven", "six", "four"}, 81., 0., 0., 100., DfxParam::Unit::Percent);
	initparameter_list(kLFO1Shape, {"LFO1 shape", "seven", "six", "four"}, dfx::LFO::kShape_Thorn, dfx::LFO::kShape_Thorn, dfx::LFO::kNumShapes);
	initparameter_f(kLFO2Rate_Hz, {"LFO2 rate (free)", "seven", "six", "four"}, 6., 6., kLFORateMin, kLFORateMax, DfxParam::Unit::Hz, DfxParam::Curve::Squared);
	initparameter_list(kLFO2Rate_Sync, {"LFO2 rate (sync)", "seven", "six", "four"}, unitTempoRateIndex, unitTempoRateIndex, numTempoRates, DfxParam::Unit::Beats);
	initparameter_b(kLFO2TempoSync, {"LFO2 tempo sync", "seven", "six", "four"}, false);
	initparameter_f(kLFO2Depth, {"LFO2 depth", "seven", "six", "four"}, 0., 0., kLFO2DepthMin, kLFO2DepthMax, DfxParam::Unit::Scalar);
	initparameter_list(kLFO2Shape, {"LFO2 shape", "seven", "six", "four"}, dfx::LFO::kShape_Triangle, dfx::LFO::kShape_Triangle, dfx::LFO::kNumShapes);
	initparameter_b(kStereoLink, {"stereo link", "seven", "six", "four"}, true);
	initparameter_i(kDelay2, {"inverse delay R", "seven", "six", "four"}, 3, 3, kDelaySamplesMin, kDelaySamplesMax, DfxParam::Unit::Samples);
	initparameter_f(kLFO1Rate2_Hz, {"LFO1 rate R (free)", "seven", "six", "four"}, 9., 9., kLFORateMin, kLFORateMax, DfxParam::Unit::Hz, DfxParam::Curve::Squared);
	initparameter_list(kLFO1Rate2_Sync, {"LFO1 rate R (sync)", "seven", "six", "four"}, unitTempoRateIndex, unitTempoRateIndex, numTempoRates, DfxParam::Unit::Beats);
	initparameter_b(kLFO1TempoSync2, {"LFO1 tempo sync R", "seven", "six", "four"}, false);
	initparameter_f(kLFO1Depth2, {"LFO1 depth R", "seven", "six", "four"}, 30., 0., 0., 100., DfxParam::Unit::Percent);
	initparameter_list(kLFO1Shape2, {"LFO1 shape R", "seven", "six", "four"}, dfx::LFO::kShape_Square, dfx::LFO::kShape_Square, dfx::LFO::kNumShapes);
	initparameter_f(kLFO2Rate2_Hz, {"LFO2 rate R (free)", "seven", "six", "four"}, 12., 12., kLFORateMin, kLFORateMax, DfxParam::Unit::Hz, DfxParam::Curve::Squared);
	initparameter_list(kLFO2Rate2_Sync, {"LFO2 rate R (sync)", "seven", "six", "four"}, unitTempoRateIndex, unitTempoRateIndex, numTempoRates, DfxParam::Unit::Beats);
	initparameter_b(kLFO2TempoSync2, {"LFO2 tempo sync R", "seven", "six", "four"}, false);
	initparameter_f(kLFO2Depth2, {"LFO2 depth R", "seven", "six", "four"}, 0., 0., kLFO2DepthMin, kLFO2DepthMax, DfxParam::Unit::Scalar);
	initparameter_list(kLFO2Shape2, {"LFO2 shape R", "seven", "six", "four"}, dfx::LFO::kShape_Saw, dfx::LFO::kShape_Saw, dfx::LFO::kNumShapes);
	initparameter_f(kDryWetMix, {dfx::kParameterNames_DryWetMix}, 100., 50., 0., 100., DfxParam::Unit::DryWetMix);

	setparameterenforcevaluelimits(kDelay, true);
	setparameterenforcevaluelimits(kDelay2, true);

	// set the value strings for the LFO shape parameters
	for (dfx::LFO::Shape i = 0; i < dfx::LFO::kNumShapes; i++)
	{
		auto const shapeName = dfx::LFO::getShapeName(i);
		setparametervaluestring(kLFO1Shape, i, shapeName);
		setparametervaluestring(kLFO2Shape, i, shapeName);
		setparametervaluestring(kLFO1Shape2, i, shapeName);
		setparametervaluestring(kLFO2Shape2, i, shapeName);
	}
	// set the value strings for the sync rate parameters
	for (size_t i = 0; i < numTempoRates; i++)
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
		kLFO1Rate_Hz, kLFO1Rate_Sync, kLFO1TempoSync, kLFO1Depth, kLFO1Shape,
		kLFO2Rate_Hz, kLFO2Rate_Sync, kLFO2TempoSync, kLFO2Depth, kLFO2Shape
	});
	addparametergroup("right",
	{
		kStereoLink, kDelay2,
		kLFO1Rate2_Hz, kLFO1Rate2_Sync, kLFO1TempoSync2, kLFO1Depth2, kLFO1Shape2,
		kLFO2Rate2_Hz, kLFO2Rate2_Sync, kLFO2TempoSync2, kLFO2Depth2, kLFO2Shape2
	});

	addchannelconfig(2, 2);  // stereo in / stereo out
	//addchannelconfig(1, 2);  // it's okay to feed both inputs with the same signal

	settailsize_samples(dfx::math::ToUnsigned(kDelayBufferSize));

	mCurrentTempoBPS = getparameter_f(kTempo) / 60.;

	setpresetname(0, "thrush");  // default preset name
	initPresets();

	registerSmoothedAudioValue(mInputGain);
	registerSmoothedAudioValue(mInverseGain);
	registerSmoothedAudioValue(mDelayOffset);
	registerSmoothedAudioValue(mDelayOffset2);
}

//-------------------------------------------------------------------------
void Thrush::initialize()
{
	mDelayBuffer.assign(kDelayBufferSize, 0.f);
	mDelayBuffer2.assign(kDelayBufferSize, 0.f);

	// this is a handy value to have during LFO calculations and wasteful to recalculate at every sample
	mOneDivSR = 1. / getsamplerate();
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
	mDelayOffset.setValueNow(0.);
	mDelayOffset2.setValueNow(0.);
#if THRUSH_LFO_DISCONTINUITY_SMOOTHING
	mOldDelayPosition = mOldDelayPosition2 = 0;
#endif

	std::ranges::fill(mDelayBuffer, 0.f);
	std::ranges::fill(mDelayBuffer2, 0.f);

	mNeedResync = true;
}

//-------------------------------------------------------------------------
void Thrush::initPresets()
{
	size_t i = 1;

	setpresetname(i, "vibber");
//	setpresetparameter_i(i, kDelay, );
	setpresetparameter_b(i, kLFO1TempoSync, false);
//	setpresetparameter_f(i, kLFO1Rate_Hz, );
//	setpresetparameter_f(i, kLFO1Depth, );
	setpresetparameter_i(i, kLFO1Shape, dfx::LFO::kShape_Saw);
	setpresetparameter_b(i, kStereoLink, false);
//	setpresetparameter_i(i, kDelay2, );
	setpresetparameter_b(i, kLFO1TempoSync2, false);
//	setpresetparameter_f(i, kLFO1Rate2_Hz, );
//	setpresetparameter_f(i, kLFO1Depth2, );
	setpresetparameter_i(i, kLFO1Shape2, dfx::LFO::kShape_Saw);
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
		if (mUseHostTempo && hostCanDoTempo() && gettimeinfo().mTempoBPS)  // get the tempo from the host
		{
			mCurrentTempoBPS = *gettimeinfo().mTempoBPS;
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
double Thrush::processLFOs(ThrushLFO& lfoLayer1, ThrushLFO& lfoLayer2) const
{
	// do beat sync if it ought to be done
	if (mNeedResync && lfoLayer2.mTempoSync && gettimeinfo().mSamplesToNextBar)
	{
		lfoLayer2.syncToTheBeat(*gettimeinfo().mSamplesToNextBar);
	}

	lfoLayer2.updatePosition();  // move forward another sample position
	auto lfoOffset = lfoLayer2.process();  // this is the offset from the first layer LFO's rate, caused by the second layer LFO

	// scale the 0 - 1 LFO output value to the depth range of the second layer LFO
	lfoOffset = std::lerp(kLFO2DepthMin, kLFO2DepthMax, lfoOffset);

	// update the first layer LFO's cycle phase step size as modulated by the second layer LFO
	lfoLayer1.setStepSize(lfoLayer1.mEffectiveRateHz * lfoOffset * mOneDivSR);
	// do beat sync if it must be done (don't do it if the 2nd layer LFO is active; that's too much to deal with)
	if (mNeedResync && (lfoLayer1.mTempoSync && !lfoLayer2.isActive()) && gettimeinfo().mSamplesToNextBar)
	{
		lfoLayer1.syncToTheBeat(*gettimeinfo().mSamplesToNextBar);
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

	if (auto const value = getparameterifchanged_b(kLFO1TempoSync))
	{
		mLFO1.mTempoSync = *value;
		// need to resync if tempo sync mode has just been switched on
		mNeedResync |= *value;
	}

	mLFO1.setDepth(getparameter_scalar(kLFO1Depth));
	mLFO1.setShape(static_cast<dfx::LFO::Shape>(getparameter_i(kLFO1Shape)));

	mLFO2.mRateHz = getparameter_f(kLFO2Rate_Hz);
	// make sure the cycles match up if the tempo rate has changed
	if (auto const value = getparameterifchanged_f(kLFO2Rate_Sync))
	{
		mLFO2.mTempoRateScalar = *value;
		mNeedResync = true;
	}

	if (auto const value = getparameterifchanged_b(kLFO2TempoSync))
	{
		mLFO2.mTempoSync = *value;
		// need to resync if tempo sync mode has just been switched on
		mNeedResync |= *value;
	}

	mLFO2.setDepth(getparameter_gen(kLFO2Depth));
	mLFO2.setShape(static_cast<dfx::LFO::Shape>(getparameter_i(kLFO2Shape)));

	mStereoLink = getparameter_b(kStereoLink);

	mDelay2_gen = getparameter_gen(kDelay2);
	mLFO1_2.mRateHz = getparameter_f(kLFO1Rate2_Hz);
	mLFO1_2.mTempoRateScalar = mTempoRateTable.getScalar(getparameter_index(kLFO1Rate2_Sync));

	if (auto const value = getparameterifchanged_b(kLFO1TempoSync2))
	{
		mLFO1_2.mTempoSync = *value;
		// need to resync if tempo sync mode has just been switched on
		mNeedResync |= (!mStereoLink && *value);
	}

	mLFO1_2.setDepth(getparameter_scalar(kLFO1Depth2));
	mLFO1_2.setShape(static_cast<dfx::LFO::Shape>(getparameter_i(kLFO1Shape2)));
	mLFO2_2.mRateHz = getparameter_f(kLFO2Rate2_Hz);
	mLFO2_2.mTempoRateScalar = mTempoRateTable.getScalar(getparameter_index(kLFO2Rate2_Sync));

	if (auto const value = getparameterifchanged_b(kLFO2TempoSync2))
	{
		mLFO2_2.mTempoSync = *value;
		// need to resync if tempo sync mode has just been switched on
		mNeedResync |= (!mStereoLink && *value);
	}

	mLFO2_2.setDepth(getparameter_gen(kLFO2Depth2));
	mLFO2_2.setShape(static_cast<dfx::LFO::Shape>(getparameter_i(kLFO2Shape2)));

	if (auto const value = getparameterifchanged_scalar(kDryWetMix))
	{
		auto const dryWetMix = static_cast<float>(*value);
		mInputGain = 1.f - std::pow(dryWetMix * 0.75f, 1.5f);
		mInverseGain = std::pow(dryWetMix * 0.87f, 1.5f);
	}
}

//-------------------------------------------------------------------------
void Thrush::processaudio(std::span<float const* const> inAudio, std::span<float* const> outAudio, size_t inNumFrames)
{
	// set up the basic startup conditions of all of the LFOs if any LFOs are turned on
	calculateEffectiveTempo();
	calculateEffectiveRate(mLFO1);
	calculateEffectiveRate(mLFO2);
	calculateEffectiveRate(mLFO1_2);
	calculateEffectiveRate(mLFO2_2);

	for (size_t sampleIndex = 0; sampleIndex < inNumFrames; sampleIndex++)
	{
		// evaluate the sample-by-sample output of the LFOs
		auto const normalizedOffset = [this](dfx::ParameterID parameterID, double normalizedValue, ThrushLFO& lfoLayer1, ThrushLFO& lfoLayer2)
		{
			auto const delayOffset = processLFOs(lfoLayer1, lfoLayer2);
			return expandparametervalue(parameterID, normalizedValue * delayOffset);
		};
		mDelayOffset = normalizedOffset(kDelay, mDelay_gen, mLFO1, mLFO2);
		if (mStereoLink)
		{
			mDelayOffset2 = mDelayOffset;
		}
		else
		{
			mDelayOffset2 = normalizedOffset(kDelay2, mDelay2_gen, mLFO1_2, mLFO2_2);
		}
		// update the delay position(s) (this is done every sample in case LFOs are active)
		auto const delayedPosition = [this](auto const& delayOffset)
		{
			auto const delayOffset_i = static_cast<long>(delayOffset.getValue() + DfxParam::kIntegerPadding);
			return (mInputPosition - delayOffset_i + kDelayBufferSize) % kDelayBufferSize;
		};
		auto const delayPosition = delayedPosition(mDelayOffset);
		auto const delayPosition2 = delayedPosition(mDelayOffset2);
		mNeedResync = false;  // make sure it gets set false so that it doesn't happen again when it shouldn't

#if THRUSH_LFO_DISCONTINUITY_SMOOTHING
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
				mOldDelayPosition = delayPosition;
				mLastSample = (inAudio[0][sampleIndex] * mInputGain.getValue()) - (mDelayBuffer[delayPosition] * mInverseGain.getValue());
				outAudio[0][sampleIndex] = mLastSample;
			}
			// do regular fade-out/mixing stuff
			else if (mLFO1.mSmoothSamples > 0)
			{
				mOldDelayPosition = (mOldDelayPosition + 1) % kDelayBufferSize;
				auto const oldGain = static_cast<float>(mLFO1.mSmoothSamples) * static_cast<float>(ThrushLFO::kSmoothStep);
				auto const newGain = 1.f - oldGain;
				outAudio[0][sampleIndex] = (inAudio[0][sampleIndex] * mInputGain.getValue()) - 
											((mDelayBuffer[delayPosition] * mInverseGain.getValue()) * newGain) - 
											((mDelayBuffer[mOldDelayPosition] * mInverseGain.getValue()) * oldGain);
			}
			// if neither of the first two conditions were true, then the left channel 
			// is not fading/blending at all - the right channel must be - do regular output
			else
			{
				outAudio[0][sampleIndex] = (inAudio[0][sampleIndex] * mInputGain.getValue()) - 
											(mDelayBuffer[delayPosition] * mInverseGain.getValue());
			}

			////////////   now the right channel - same stuff as above   ////////////
			if (mLFO1_2.mSmoothSamples == ThrushLFO::kSmoothDur)
			{
				mOldDelayPosition2 = delayPosition2;
				mLastSample2 = (inAudio[1][sampleIndex] * mInputGain.getValue()) - (mDelayBuffer2[delayPosition2] * mInverseGain.getValue());
				outAudio[1][sampleIndex] = mLastSample2;
			}
			else if (mLFO1_2.mSmoothSamples > 0)
			{
				mOldDelayPosition2 = (mOldDelayPosition2 + 1) % kDelayBufferSize;
				auto const oldGain = static_cast<float>(mLFO1_2.mSmoothSamples) * static_cast<float>(ThrushLFO::kSmoothStep);
				auto const newGain = 1.f - oldGain;
				outAudio[1][sampleIndex] = (inAudio[1][sampleIndex] * mInputGain.getValue()) - 
											((mDelayBuffer2[delayPosition2] * mInverseGain.getValue()) * newGain) - 
											((mDelayBuffer2[mOldDelayPosition2] * mInverseGain.getValue()) * oldGain);
			}
			else
			{
				outAudio[1][sampleIndex] = (inAudio[1][sampleIndex] * mInputGain.getValue()) - 
											(mDelayBuffer2[delayPosition2] * mInverseGain.getValue());
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


		// no smoothing is going on (this is much simpler)
		else
#endif  // THRUSH_LFO_DISCONTINUITY_SMOOTHING
		{
			outAudio[0][sampleIndex] = (inAudio[0][sampleIndex] * mInputGain.getValue()) - 
									(mDelayBuffer[delayPosition] * mInverseGain.getValue());
			outAudio[1][sampleIndex] = (inAudio[1][sampleIndex] * mInputGain.getValue()) - 
									(mDelayBuffer2[delayPosition2] * mInverseGain.getValue());
		}

		// put the latest input samples into the delay buffer
		mDelayBuffer[mInputPosition] = inAudio[0][sampleIndex];
		mDelayBuffer2[mInputPosition] = inAudio[1][sampleIndex];

		// incremement the input position in the delay buffer and wrap it around 
		// if it has reached the end of the buffer
		mInputPosition = (mInputPosition + 1) % kDelayBufferSize;

		incrementSmoothedAudioValues();
	}
}
