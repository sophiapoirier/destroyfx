/*---------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2010-2020  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Destroy FX Library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Destroy FX Library.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
---------------------------------------------------------------*/


#include "dfxenvelope.h"

#include <algorithm>
#include <cassert>
#include <cmath>

#include "dfxmath.h"
#include "dfxparameter.h"


//-----------------------------------------------------------------------------
void DfxEnvelope::setParameters(double inAttackDur, double inDecayDur, double inSustainLevel, double inReleaseDur)
{
	mAttackDur = std::max(inAttackDur, 0.0);
	mDecayDur = std::max(inDecayDur, 0.0);
	mSustainLevel = std::clamp(inSustainLevel, 0.0, 1.0);
	mReleaseDur = std::max(inReleaseDur, 0.0);

	if (mSustainLevel >= 1.0)
	{
		mDecayDur = 0.0;
	}
	else if (mSustainLevel <= 0.0)
	{
		mReleaseDur = 0.0;
	}
}

//-----------------------------------------------------------------------------
void DfxEnvelope::setSampleRate(double inSampleRate)
{
	auto const prevSampleRate = mSampleRate;
	mSampleRate = inSampleRate;
	if ((prevSampleRate != inSampleRate) && (prevSampleRate != 0.0))
	{
		auto const srScalar = inSampleRate / prevSampleRate;
		mSectionPos = dfx::math::RoundToIndex(static_cast<double>(mSectionPos) * srScalar);
		mSectionLength = dfx::math::RoundToIndex(static_cast<double>(mSectionLength) * srScalar);
		mSectionLength_inv = 1.0 / static_cast<double>(std::max(mSectionLength, size_t(1)));
	}
}

//-----------------------------------------------------------------------------
void DfxEnvelope::setInactive() noexcept
{
	mState = State::Dormant;
}

//-----------------------------------------------------------------------------
bool DfxEnvelope::isActive() const noexcept
{
	return (getState() != State::Dormant);
}

//-----------------------------------------------------------------------------
void DfxEnvelope::beginAttack()
{
	auto const prevState = mState;
	mState = State::Attack;
	mSectionPos = 0;
	mSectionLength = dfx::math::RoundToIndex(mAttackDur * mSampleRate);
	if (mSectionLength > 0)
	{
		mSectionLength_inv = 1.0 / static_cast<double>(mSectionLength);
		mStartValue = 0.0;
		mTargetValue = 1.0;
		if (mResumedAttackMode && (prevState == State::Release))
		{
			mSectionPos = dfx::math::RoundToIndex(deriveAttackPosFromEnvValue(mLastValue) * static_cast<double>(mSectionLength));
		}
	}
	else
	{
		mSectionLength_inv = 1.0;
		(void) process();  // do this to move ahead to the next envelope segment, since there is no attack
	}
}

//-----------------------------------------------------------------------------
void DfxEnvelope::beginRelease()
{
	mState = State::Release;
	mSectionPos = 0;
	mSectionLength = dfx::math::RoundToIndex(mReleaseDur * mSampleRate);
	if (mSectionLength > 0)
	{
		mSectionLength_inv = 1.0 / static_cast<double>(mSectionLength);
		mStartValue = mLastValue;
		mTargetValue = 0.0;
	}
	else
	{
		mState = State::Dormant;
	}
}

//-----------------------------------------------------------------------------
[[nodiscard]] double DfxEnvelope::process()
{
	double outputValue = 0.0;

	switch (mState)
	{
		case State::Attack:
			outputValue = calculateRise(static_cast<double>(mSectionPos + 1) * mSectionLength_inv);
			mSectionPos++;
			if (mSectionPos >= mSectionLength)
			{
				mSectionPos = 0;
				mState = State::Decay;
				mSectionLength = dfx::math::RoundToIndex(mDecayDur * mSampleRate);
				if (mSectionLength > 0)
				{
					mSectionLength_inv = 1.0 / static_cast<double>(mSectionLength);
					mStartValue = 1.0;
					mTargetValue = mSustainLevel;
				}
				else
				{
					mState = State::Sustain;
				}
			}
			break;

		case State::Decay:
			outputValue = calculateFall(static_cast<double>(mSectionPos + 1) * mSectionLength_inv);
			mSectionPos++;
			if (mSectionPos >= mSectionLength)
			{
				mSectionPos = 0;
				mState = State::Sustain;
			}
			break;

		case State::Sustain:
			outputValue = mSustainLevel;
			if (mSustainLevel <= 0.0)
			{
				mState = State::Dormant;
			}
			break;

		case State::Release:
			outputValue = calculateFall(static_cast<double>(mSectionPos + 1) * mSectionLength_inv);
			mSectionPos++;
			if (mSectionPos >= mSectionLength)
			{
				mSectionPos = 0;
				mState = State::Dormant;
			}
			break;

		case State::Dormant:
			outputValue = 0.0;
			break;

		default:
			assert(false);
			break;
	}

	mLastValue = outputValue;
	return outputValue;
}

//-----------------------------------------------------------------------------
dfx::IIRfilter::Coefficients DfxEnvelope::getLowpassGateCoefficients(double inAmplitude) const
{
	constexpr double minFreq = 20.;
	constexpr double maxFreq = 20'000.;
	auto const cutoffFreq = DfxParam::expand(inAmplitude, minFreq, maxFreq, DfxParam::Curve::Log);
	auto const nyquist = mSampleRate * 0.5;
	if (cutoffFreq >= std::min(nyquist, maxFreq))
	{
		return dfx::IIRfilter::kUnityCoeff;
	}
	if (cutoffFreq <= minFreq)
	{
		return dfx::IIRfilter::kZeroCoeff;
	}

	double const amplitudeFadeThreshold = [this]()
	{
		constexpr double thresholdDefault = 0.1;
		if ((mState == State::Release) && (mReleaseDur > 0.))
		{
			// during release, scale the threshold at which gain fading the filter input
			// so that the amount of time spent meets a minimum duration, in an effort
			// to prevent audible filter ring-out truncation glitches at the end of release
			constexpr double minimumFadeDur = 0.100;
			return std::min(thresholdDefault * std::max(minimumFadeDur / (thresholdDefault * mReleaseDur), 1.), 1.);
		}
		return thresholdDefault;
	}();
	double const amplitudeFadeThresholdInv = 1. / amplitudeFadeThreshold;

	auto coeff = dfx::IIRfilter(mSampleRate).setLowpassCoefficients(cutoffFreq);
	if (inAmplitude < amplitudeFadeThreshold)
	{
		auto const fadeAmp = static_cast<float>(inAmplitude * amplitudeFadeThresholdInv);
		coeff.mIn *= fadeAmp;
		coeff.mPrevIn *= fadeAmp;
		coeff.mPrevPrevIn *= fadeAmp;
	}
	return coeff;
}

//-----------------------------------------------------------------------------
double DfxEnvelope::calculateRise(size_t inPos, size_t inLength) const
{
	if (inLength == 0)
	{
		return 0.0;
	}
	return calculateRise(static_cast<double>(inPos + 1) / static_cast<double>(inLength));
}

//-----------------------------------------------------------------------------
double DfxEnvelope::calculateRise(double inPosNormalized) const
{
	if (mCurveType == kCurveType_Cubed)
	{
		return inPosNormalized * inPosNormalized * inPosNormalized;
	}

	// sine fade (stupendously inefficient)
//	return (std::sin((inPosNormalized * dfx::math::kPi<double>) - (dfx::math::kPi<double> * 0.5)) + 1.0) * 0.5;

	return inPosNormalized;
}

//-----------------------------------------------------------------------------
double DfxEnvelope::calculateFall(size_t inPos, size_t inLength) const
{
	if (inLength == 0)
	{
		return 0.0;
	}
	return calculateFall(static_cast<double>(inPos + 1) / static_cast<double>(inLength));
}

//-----------------------------------------------------------------------------
double DfxEnvelope::calculateFall(double inPosNormalized) const
{
	double outputValue = 1.0 - inPosNormalized;

	if (mCurveType == kCurveType_Cubed)
	{
		outputValue = outputValue * outputValue * outputValue;
	}

	outputValue = (outputValue * (mStartValue - mTargetValue)) + mTargetValue;

	return outputValue;
}

//-----------------------------------------------------------------------------
double DfxEnvelope::deriveAttackPosFromEnvValue(double inValue) const
{
	if (mCurveType == kCurveType_Cubed)
	{
		return std::cbrt(inValue);
	}
	return inValue;
}
