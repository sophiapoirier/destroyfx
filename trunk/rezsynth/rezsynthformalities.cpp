/*------------------------------------------------------------------------
Copyright (C) 2001-2019  Sophia Poirier

This file is part of Rez Synth.

Rez Synth is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Rez Synth is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Rez Synth.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "rezsynth.h"

#include <algorithm>

#include "dfxmath.h"


// this macro does boring entry point stuff for us
DFX_EFFECT_ENTRY(RezSynth)

//-----------------------------------------------------------------------------------------
// initializations
RezSynth::RezSynth(TARGET_API_BASE_INSTANCE_TYPE inInstance)
	: DfxPlugin(inInstance,	kNumParameters, kNumPresets)
{
	initparameter_f(kBandwidthAmount_Hz, "bandwidth (Hz)", 3.0, 3.0, 0.1, 300.0, DfxParam::Unit::Hz, DfxParam::Curve::Log);
	initparameter_f(kBandwidthAmount_Q, "bandwidth (Q)", 69.0, 30.0, 0.01, 500.0, DfxParam::Unit::Generic, DfxParam::Curve::Cubed);
	initparameter_list(kBandwidthMode, "bandwidth mode", kBandwidthMode_Q, kBandwidthMode_Q, kNumBandwidthModes);
	initparameter_list(kResonAlgorithm, "resonance algorithm", kResonAlg_2Pole2ZeroR, kResonAlg_2Pole2ZeroR, kNumResonAlgs);
	initparameter_i(kNumBands, "bands per note", 1, 1, 1, kMaxBands, DfxParam::Unit::Generic);
	initparameter_f(kSepAmount_Octaval, "band separation (octaval)", 12.0, 12.0, 0.0, 36.0, DfxParam::Unit::Semitones);
	initparameter_f(kSepAmount_Linear, "band separation (linear)", 1.0, 1.0, 0.0, 3.0, DfxParam::Unit::Scalar);  // % of center frequency
	initparameter_list(kSepMode, "separation mode", kSeparationMode_Octaval, kSeparationMode_Octaval, kNumSeparationModes);
	initparameter_b(kFoldover, "filter frequency aliasing", true, false);
	initparameter_f(kEnvAttack, "attack", 3.0, 3.0, 0.0, 3000.0, DfxParam::Unit::MS, DfxParam::Curve::Squared);
	initparameter_f(kEnvDecay, "decay", 30.0, 30.0, 0.0, 3000.0, DfxParam::Unit::MS, DfxParam::Curve::Squared);
	initparameter_f(kEnvSustain, "sustain", 100.0, 50.0, 0.0, 100.0, DfxParam::Unit::Percent, DfxParam::Curve::Cubed);
	initparameter_f(kEnvRelease, "release", 300.0, 300.0, 0.0, 3000.0, DfxParam::Unit::MS, DfxParam::Curve::Squared);
	initparameter_list(kFadeType, "envelope fades", DfxEnvelope::kCurveType_Cubed, DfxEnvelope::kCurveType_Cubed, DfxEnvelope::kCurveType_NumTypes);
	initparameter_b(kLegato, "legato", false, false);
//	initparameter_f(kVelocityInfluence, "velocity influence", 0.6, 1.0, 0.0, 1.0, DfxParam::Unit::Scalar);
	initparameter_f(kVelocityInfluence, "velocity influence", 60.0, 100.0, 0.0, 100.0, DfxParam::Unit::Percent);
	initparameter_f(kVelocityCurve, "velocity curve", 2.0, 1.0, 0.3, 3.0, DfxParam::Unit::Exponent);
	initparameter_f(kPitchBendRange, "pitch bend range", 3.0, 3.0, 0.0, DfxMidi::kPitchBendSemitonesMax, DfxParam::Unit::Semitones);
	initparameter_list(kScaleMode, "filter response scaling mode", kScaleMode_RMS, kScaleMode_None, kNumScaleModes);
	initparameter_f(kFilterOutputGain, "output gain", 1.0, 1.0, 0.0, dfx::math::Db2Linear(12.0), DfxParam::Unit::LinearGain, DfxParam::Curve::Cubed);
	initparameter_f(kBetweenGain, "between gain", 0.0, 1.0, 0.0, dfx::math::Db2Linear(12.0), DfxParam::Unit::LinearGain, DfxParam::Curve::Cubed);
	initparameter_f(kDryWetMix, "dry/wet mix", 100.0, 50.0, 0.0, 100.0, DfxParam::Unit::DryWetMix);
	initparameter_list(kDryWetMixMode, "dry/wet mix mode", kDryWetMixMode_EqualPower, kDryWetMixMode_Linear, kNumDryWetMixModes);
	initparameter_b(kWiseAmp, "careful", true, true);

	setparametervaluestring(kBandwidthMode, kBandwidthMode_Hz, "Hz");
	setparametervaluestring(kBandwidthMode, kBandwidthMode_Q, "Q");
	setparametervaluestring(kResonAlgorithm, kResonAlg_2PoleNoZero, "no zero");
	setparametervaluestring(kResonAlgorithm, kResonAlg_2Pole2ZeroR, "2-zero (radius)");
	setparametervaluestring(kResonAlgorithm, kResonAlg_2Pole2Zero1, "2-zero (1)");
	setparametervaluestring(kSepMode, kSeparationMode_Octaval, "octaval");
	setparametervaluestring(kSepMode, kSeparationMode_Linear, "linear");
	setparametervaluestring(kScaleMode, kScaleMode_None, "no scaling");
	setparametervaluestring(kScaleMode, kScaleMode_RMS, "RMS normalize");
	setparametervaluestring(kScaleMode, kScaleMode_Peak, "peak normalize");
	setparametervaluestring(kDryWetMixMode, kDryWetMixMode_Linear, "linear");
	setparametervaluestring(kDryWetMixMode, kDryWetMixMode_EqualPower, "equal power");
	setparametervaluestring(kFadeType, DfxEnvelope::kCurveType_Linear, "linear");
	setparametervaluestring(kFadeType, DfxEnvelope::kCurveType_Cubed, "exponential");
//	setparametervaluestring(kFoldover, 0, "resist");
//	setparametervaluestring(kFoldover, 1, "allow");

	addparametergroup("envelope", {kEnvAttack, kEnvDecay, kEnvSustain, kEnvRelease, kFadeType});
	addparametergroup("mix", {kFilterOutputGain, kBetweenGain, kDryWetMix, kDryWetMixMode});


	settailsize_seconds(getparametermax_f(kEnvRelease) * 0.001);
	setAudioProcessingMustAccumulate(true);  // only support accumulating output
	getmidistate().setResumedAttackMode(true);  // this enables the lazy note attack mode

	setpresetname(0, "feminist synth");  // default preset name

	registerSmoothedAudioValue(&mOutputGain);
	registerSmoothedAudioValue(&mBetweenGain);
	registerSmoothedAudioValue(&mDryGain);
	registerSmoothedAudioValue(&mWetGain);
}

//-----------------------------------------------------------------------------------------
long RezSynth::initialize()
{
	// these are values that are always needed during calculateCoefficients
	mPiDivSR = dfx::math::kPi<double> / getsamplerate();
	mTwoPiDivSR = mPiDivSR * 2.0;
	mNyquist = getsamplerate() / 2.0;

	return dfx::kStatus_NoError;
}

//-----------------------------------------------------------------------------------------
void RezSynth::reset()
{
	// reset the unaffected between audio stuff
	mUnaffectedState = UnaffectedState::FadeIn;
	mUnaffectedFadeSamples = 0;

	mInputAmp.fill(0.0);
	mPrevOutCoeff.fill(0.0);
	mPrevPrevOutCoeff.fill(0.0);
	mPrevPrevInCoeff.fill(0.0);
}

//-----------------------------------------------------------------------------------------
void RezSynth::createbuffers()
{
	auto const numChannels = getnumoutputs();

	mPrevOutValue.assign(numChannels, {});
	mPrevPrevOutValue.assign(numChannels, {});
	mPrevInValue.assign(numChannels, {});
	mPrevPrevInValue.assign(numChannels, {});
}

//-----------------------------------------------------------------------------------------
void RezSynth::releasebuffers()
{
	mPrevOutValue.clear();
	mPrevPrevOutValue.clear();
	mPrevInValue.clear();
	mPrevPrevInValue.clear();
}

//-----------------------------------------------------------------------------------------
void RezSynth::clearbuffers()
{
	clearChannelsOfNotesOfBands(mPrevOutValue);
	clearChannelsOfNotesOfBands(mPrevPrevOutValue);
	for (auto& values : mPrevInValue)
	{
		values.fill(0.0);
	}
	for (auto& values : mPrevPrevInValue)
	{
		values.fill(0.0);
	}
}

//-----------------------------------------------------------------------------------------
void RezSynth::processparameters()
{
	auto const oldNumBands = mNumBands;

	mBandwidthAmount_Hz = getparameter_f(kBandwidthAmount_Hz);
	mBandwidthAmount_Q = getparameter_f(kBandwidthAmount_Q);
	mBandwidthMode = getparameter_i(kBandwidthMode);
	mNumBands = std::min(getparameter_i(kNumBands), kMaxBands);
	mSepAmount_Octaval = getparameter_f(kSepAmount_Octaval) / 12.0;
	mSepAmount_Linear = getparameter_f(kSepAmount_Linear);
	mSepMode = getparameter_i(kSepMode);
	mFoldover = getparameter_b(kFoldover);  // true for allow, false for resist
	mAttack_Seconds = getparameter_f(kEnvAttack) * 0.001;
	mDecay_Seconds = getparameter_f(kEnvDecay) * 0.001;
	mSustain = getparameter_scalar(kEnvSustain);
	mRelease_Seconds = getparameter_f(kEnvRelease) * 0.001;
	mFadeType = static_cast<DfxEnvelope::CurveType>(getparameter_i(kFadeType));
	mLegato = getparameter_b(kLegato);
	mVelocityInfluence = getparameter_scalar(kVelocityInfluence);
	mVelocityCurve = getparameter_f(kVelocityCurve);
	mPitchBendRange = getparameter_f(kPitchBendRange);
	mScaleMode = getparameter_i(kScaleMode);
	mResonAlgorithm = getparameter_i(kResonAlgorithm);
	if (auto const value = getparameterifchanged_f(kFilterOutputGain))
	{
		mOutputGain = *value;
	}
	if (auto const value = getparameterifchanged_f(kBetweenGain))
	{
		mBetweenGain = *value;
	}
	if (getparameterchanged(kDryWetMix) || getparameterchanged(kDryWetMixMode))
	{
		auto const dryWetMix = static_cast<float>(getparameter_scalar(kDryWetMix));
		auto const dryWetMixMode = getparameter_i(kDryWetMixMode);
		auto dryGain = 1.0f - dryWetMix;
		auto wetGain = dryWetMix;
		if (dryWetMixMode == kDryWetMixMode_EqualPower)
		{
			// sqare root for equal power blending of non-correlated signals
			dryGain = std::sqrt(dryGain);
			wetGain = std::sqrt(wetGain);
		}
		mDryGain = dryGain;
		mWetGain = wetGain;
	}
	mWiseAmp = getparameter_b(kWiseAmp);

	if (getparameterchanged(kNumBands))
	{
		// clear the output buffers of abandoned bands when the number decreases
		if (mNumBands < oldNumBands)
		{
			for (size_t ch = 0; ch < mPrevOutValue.size(); ch++)
			{
				for (size_t note = 0; note < mPrevOutValue[ch].size(); note++)
				{
					for (int band = mNumBands; band < oldNumBands; band++)
					{
						mPrevOutValue[ch][note][band] = 0.0;
						mPrevPrevOutValue[ch][note][band] = 0.0;
					}
				}
			}
		}
	}

	// feedback buffers need to be cleared
	if (getparameterchanged(kScaleMode) || getparameterchanged(kResonAlgorithm))
	{
		clearChannelsOfNotesOfBands(mPrevOutValue);
		clearChannelsOfNotesOfBands(mPrevPrevOutValue);
	}

	if (getparameterchanged(kEnvAttack) || getparameterchanged(kEnvDecay) 
		|| getparameterchanged(kEnvSustain) || getparameterchanged(kEnvRelease))
	{
		getmidistate().setEnvParameters(mAttack_Seconds, mDecay_Seconds, mSustain, mRelease_Seconds);
	}

	if (getparameterchanged(kFadeType))
	{
		getmidistate().setEnvCurveType(mFadeType);
	}

/* XXX implement real legato
	// if we have just exited legato mode, we must end any active notes so that 
	// they don't hang in legato mode (remember, legato mode ignores note-offs)
	if (getparameterchanged(kLegato) && !mLegato)
	{
		for (int notecount = 0; notecount < DfxMidi::kNumNotes; notecount++)
		{
			if (getmidistate().getNoteState(notecount).mVelocity)
			{
				// if the note is currently fading in, pick up where it left off
				if (getmidistate().getNoteState(notecount).attackDur)
				{
					getmidistate().getNoteState(notecount).releaseSamples = getmidistate().getNoteState(notecount).attackSamples;
				}
				// otherwise do the full fade out duration (if the note is not already fading out)
				else if ((getmidistate().getNoteState(notecount).releaseSamples) <= 0)
				{
					getmidistate().noteTable[notecount].releaseSamples = DfxMidi::kLegatoFadeDur;
				}
				getmidistate().getNoteState(notecount).releaseDur = DfxMidi::kLegatoFadeDur;
				getmidistate().getNoteState(notecount).attackDur = 0;
				getmidistate().getNoteState(notecount).attackSamples = 0;
				getmidistate().getNoteState(notecount).linearFadeStep = DfxMidi::kLegatoFadeStep;
			}
		}
	}
*/
}
