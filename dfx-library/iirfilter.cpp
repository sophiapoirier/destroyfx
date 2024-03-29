/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2001-2023  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 2 of the License, or 
(at your option) any later version.

Destroy FX Library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Destroy FX Library.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org

Destroy FX is a sovereign entity comprised of Sophia Poirier and Tom Murphy 7.
Welcome to our Infinite Impulse Response filter.
------------------------------------------------------------------------*/

#include "iirfilter.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>
#include <numeric>
#include <utility>

#include "dfxmath.h"
#include "dfxparameter.h"


//------------------------------------------------------------------------
constexpr double kDefaultQ_LP_HP = std::numbers::sqrt2_v<double> / 2.; 
constexpr double kUnityGain = 1.;
constexpr double kGateFrequencyMin = 20.;
constexpr double kGateFrequencyMax = 20'000.;

//------------------------------------------------------------------------
struct PreCoeff
{
	double const mOmega;  // radians per sample
	double const mSinOmega;
	double const mCosOmega;
	double const mAlpha;  // HP LP BP peak notch
	double mA {};  // peak shelf
	double mBeta {};  // shelf

	PreCoeff(double inFrequency, double inQ, double inSampleRate)
	:	mOmega(2. * std::numbers::pi_v<double> * inFrequency / inSampleRate),
		mSinOmega(std::sin(mOmega)),
		mCosOmega(std::cos(mOmega)),
		mAlpha(mSinOmega / (2. * inQ))
		//mAlpha(mSinOmega * std::sinh(std::numbers::ln2_v<double> / 2. * inQ * mOmega / mSinOmega))  // http://musicdsp.org/showone.php?id=64
	{
	}

	PreCoeff(double inFrequency, double inQ, double inGain, double inSampleRate)
	:	PreCoeff(inFrequency, inQ, inSampleRate)
	{
		mA = std::sqrt(inGain);
		mBeta = std::sqrt(std::max((((mA * mA) + 1.) / inQ) - ((mA - 1.) * (mA - 1.)), 0.));
	}
};

//------------------------------------------------------------------------
static dfx::IIRFilter::Coefficients CalculateCoefficients(dfx::IIRFilter::FilterType inFilterType, PreCoeff const& inPreCoeff)
{
#ifdef DFX_IIRFILTER_USE_OPTIMIZATION_FOR_EXCLUSIVELY_LP_HP_NOTCH
	assert((inFilterType == dfx::IIRFilter::FilterType::Lowpass) ||
		   (inFilterType == dfx::IIRFilter::FilterType::Highpass) ||
		   (inFilterType == dfx::IIRFilter::FilterType::Notch));
#endif

	dfx::IIRFilter::Coefficients coeff;
	double b0 {};

	// calculate filter coefficients
	switch (inFilterType)
	{
		case dfx::IIRFilter::FilterType::Lowpass:
			b0 = 1. + inPreCoeff.mAlpha;
			coeff.mIn = coeff.mPrevPrevIn = (1. - inPreCoeff.mCosOmega) * 0.5;
			coeff.mPrevIn = 1. - inPreCoeff.mCosOmega;
			coeff.mPrevOut = -2. * inPreCoeff.mCosOmega;
			coeff.mPrevPrevOut = 1. - inPreCoeff.mAlpha;
			break;

		case dfx::IIRFilter::FilterType::Highpass:
			b0 = 1. + inPreCoeff.mAlpha;
			coeff.mIn = coeff.mPrevPrevIn = (1. + inPreCoeff.mCosOmega) * 0.5;
			coeff.mPrevIn = -1. - inPreCoeff.mCosOmega;
			coeff.mPrevOut = -2. * inPreCoeff.mCosOmega;
			coeff.mPrevPrevOut = 1. - inPreCoeff.mAlpha;
			break;

		case dfx::IIRFilter::FilterType::Bandpass:
			b0 = 1. + inPreCoeff.mAlpha;
			coeff.mIn = inPreCoeff.mAlpha;
			coeff.mPrevIn = 0.;
			coeff.mPrevPrevIn = -inPreCoeff.mAlpha;
			coeff.mPrevOut = -2. * inPreCoeff.mCosOmega;
			coeff.mPrevPrevOut = 1. - inPreCoeff.mAlpha;
			break;

		case dfx::IIRFilter::FilterType::Peak:
			assert(!dfx::math::IsZero(inPreCoeff.mA));
			b0 = 1. + (inPreCoeff.mAlpha / inPreCoeff.mA);
			coeff.mIn = 1. + (inPreCoeff.mAlpha * inPreCoeff.mA);
			coeff.mPrevIn = coeff.mPrevOut = -2. * inPreCoeff.mCosOmega;
			coeff.mPrevPrevIn = 1. - (inPreCoeff.mAlpha * inPreCoeff.mA);
			coeff.mPrevPrevOut = 1. - (inPreCoeff.mAlpha / inPreCoeff.mA);
			break;

		case dfx::IIRFilter::FilterType::Notch:
			b0 = 1. + inPreCoeff.mAlpha;
			coeff.mIn = coeff.mPrevPrevIn = 1.;
			coeff.mPrevIn = coeff.mPrevOut = -2. * inPreCoeff.mCosOmega;
			coeff.mPrevPrevOut = 1. - inPreCoeff.mAlpha;
			break;

		case dfx::IIRFilter::FilterType::LowShelf:
			b0 = (inPreCoeff.mA + 1.) + ((inPreCoeff.mA - 1.) * inPreCoeff.mCosOmega) + (inPreCoeff.mBeta * inPreCoeff.mSinOmega);
			coeff.mIn = inPreCoeff.mA * ((inPreCoeff.mA + 1.) - ((inPreCoeff.mA - 1.) * inPreCoeff.mCosOmega) + (inPreCoeff.mBeta * inPreCoeff.mSinOmega));
			coeff.mPrevIn = 2. * inPreCoeff.mA * ((inPreCoeff.mA - 1.) - ((inPreCoeff.mA + 1.) * inPreCoeff.mCosOmega));
			coeff.mPrevPrevIn = inPreCoeff.mA * ((inPreCoeff.mA + 1.) - ((inPreCoeff.mA - 1.) * inPreCoeff.mCosOmega) - (inPreCoeff.mBeta * inPreCoeff.mSinOmega));
			coeff.mPrevOut = -2. * ((inPreCoeff.mA - 1.) + ((inPreCoeff.mA + 1.) * inPreCoeff.mCosOmega));
			coeff.mPrevPrevOut = (inPreCoeff.mA + 1.) + ((inPreCoeff.mA - 1.) * inPreCoeff.mCosOmega) - (inPreCoeff.mBeta * inPreCoeff.mSinOmega);
			break;

		case dfx::IIRFilter::FilterType::HighShelf:
			b0 = (inPreCoeff.mA + 1.) - ((inPreCoeff.mA - 1.) * inPreCoeff.mCosOmega) + (inPreCoeff.mBeta * inPreCoeff.mSinOmega);
			coeff.mIn = inPreCoeff.mA * ((inPreCoeff.mA + 1.) + ((inPreCoeff.mA - 1.)) * inPreCoeff.mCosOmega + (inPreCoeff.mBeta * inPreCoeff.mSinOmega));
			coeff.mPrevIn = -2. * inPreCoeff.mA * ((inPreCoeff.mA - 1.) + ((inPreCoeff.mA + 1.) * inPreCoeff.mCosOmega));
			coeff.mPrevPrevIn = inPreCoeff.mA * ((inPreCoeff.mA + 1.) + ((inPreCoeff.mA - 1.) * inPreCoeff.mCosOmega) - (inPreCoeff.mBeta * inPreCoeff.mSinOmega));
			coeff.mPrevOut = 2. * ((inPreCoeff.mA - 1.) - ((inPreCoeff.mA + 1.) * inPreCoeff.mCosOmega));
			coeff.mPrevPrevOut = (inPreCoeff.mA + 1.) - ((inPreCoeff.mA - 1.) * inPreCoeff.mCosOmega) - (inPreCoeff.mBeta * inPreCoeff.mSinOmega);
			break;

		default:
			std::unreachable();
	}

	if (!dfx::math::IsZero(b0))
	{
		coeff.mIn /= b0;
		coeff.mPrevIn /= b0;
		coeff.mPrevPrevIn /= b0;
		coeff.mPrevOut /= b0;
		coeff.mPrevPrevOut /= b0;
	}
	else
	{
		assert(false);
	}

	return coeff;
}


#pragma mark -

//------------------------------------------------------------------------
dfx::IIRFilter::IIRFilter(double inSampleRate)
:	mSampleRate(inSampleRate)
{
	assert(inSampleRate > 0.);
}

//------------------------------------------------------------------------
void dfx::IIRFilter::reset() noexcept
{
	mPrevIn = mPrevPrevIn = mPrevOut = mPrevPrevOut = mPrevPrevPrevOut = mCurrentOut = 0.0f;
}

//------------------------------------------------------------------------
void dfx::IIRFilter::setCoefficients(Coefficients const& inCoefficients)
{
	mCoeff = inCoefficients;
}

//------------------------------------------------------------------------
dfx::IIRFilter::Coefficients const& dfx::IIRFilter::setCoefficients(FilterType inFilterType, double inFrequency, double inQ, double inGain)
{
	assert(inFrequency > 0.);
	assert(!std::isinf(inFrequency));
	assert(!std::isnan(inFrequency));
	assert(inQ > 0.);
	assert(!std::isinf(inQ));
	assert(!std::isnan(inQ));
	assert(!std::isinf(inGain));
	assert(!std::isnan(inGain));

	mFilterType = inFilterType;
	mFilterFrequency = inFrequency;
	mFilterQ = inQ;
	mFilterGain = inGain;

	mCoeff = CalculateCoefficients(inFilterType, PreCoeff(inFrequency, inQ, inGain, mSampleRate));
	return mCoeff;
}

//------------------------------------------------------------------------
dfx::IIRFilter::Coefficients const& dfx::IIRFilter::setCoefficients(FilterType inFilterType, double inFrequency, double inQ)
{
	assert((inFilterType != FilterType::Peak) && 
		   (inFilterType != FilterType::LowShelf) && 
		   (inFilterType != FilterType::HighShelf));
	assert(inFrequency > 0.);
	assert(!std::isinf(inFrequency));
	assert(!std::isnan(inFrequency));
	assert(inQ > 0.);
	assert(!std::isinf(inQ));
	assert(!std::isnan(inQ));

	mFilterType = inFilterType;
	mFilterFrequency = inFrequency;
	mFilterQ = inQ;
	mFilterGain = kUnityGain;

	mCoeff = CalculateCoefficients(inFilterType, PreCoeff(inFrequency, inQ, mSampleRate));
	return mCoeff;
}

//------------------------------------------------------------------------
void dfx::IIRFilter::setSampleRate(double inSampleRate)
{
	mSampleRate = inSampleRate;
	// update after a change in sample rate
	setCoefficients(mFilterType, mFilterFrequency, mFilterQ, mFilterGain);
}

//------------------------------------------------------------------------
dfx::IIRFilter::Coefficients const& dfx::IIRFilter::setLowpassCoefficients(double inCutoffFrequency)
{
	return setCoefficients(FilterType::Lowpass, inCutoffFrequency, kDefaultQ_LP_HP);
}

//------------------------------------------------------------------------
static void ApplyGateFadeOut(dfx::IIRFilter::Coefficients& ioCoefficients, double inLevel)
{
	// level below which the low-pass or high-pass gate begins gain-fading filter coefficients
	constexpr double levelFadeOutThreshold = 0.1;
	constexpr double levelFadeOutThresholdInverse = 1. / levelFadeOutThreshold;

	if (inLevel < levelFadeOutThreshold)
	{
		auto const fadeGain = static_cast<float>(inLevel * levelFadeOutThresholdInverse);
		ioCoefficients.mIn *= fadeGain;
		ioCoefficients.mPrevIn *= fadeGain;
		ioCoefficients.mPrevPrevIn *= fadeGain;
	}
}

//------------------------------------------------------------------------
dfx::IIRFilter::Coefficients const& dfx::IIRFilter::setLowpassGateCoefficients(double inLevel)
{
	assert(inLevel >= 0.);
	assert(inLevel <= 1.);

	auto const cutoffFrequency = DfxParam::expand(inLevel, kGateFrequencyMin, kGateFrequencyMax, DfxParam::Curve::Log);
	auto const nyquist = mSampleRate * 0.5;
	if (cutoffFrequency >= std::min(nyquist, kGateFrequencyMax))
	{
		return kUnityCoeff;
	}

	setLowpassCoefficients(cutoffFrequency);
	ApplyGateFadeOut(mCoeff, inLevel);

	return mCoeff;
}

//------------------------------------------------------------------------
dfx::IIRFilter::Coefficients const& dfx::IIRFilter::setHighpassCoefficients(double inCutoffFrequency)
{
	assert(inCutoffFrequency <= mSampleRate);
	return setCoefficients(FilterType::Highpass, inCutoffFrequency, kDefaultQ_LP_HP);
}

//------------------------------------------------------------------------
dfx::IIRFilter::Coefficients const& dfx::IIRFilter::setHighpassGateCoefficients(double inLevel)
{
	assert(inLevel >= 0.);
	assert(inLevel <= 1.);

	if (inLevel >= 1.)
	{
		return kUnityCoeff;
	}

	auto const cutoffFrequency = DfxParam::expand(1. - inLevel, kGateFrequencyMin, kGateFrequencyMax, DfxParam::Curve::Log);
	auto const nyquist = mSampleRate * 0.5;
	setHighpassCoefficients(std::min(cutoffFrequency, nyquist));
	ApplyGateFadeOut(mCoeff, inLevel);

	return mCoeff;
}

//------------------------------------------------------------------------
dfx::IIRFilter::Coefficients const& dfx::IIRFilter::setBandpassCoefficients(double inCenterFrequency, double inQ)
{
	return setCoefficients(FilterType::Bandpass, inCenterFrequency, inQ);
}

//------------------------------------------------------------------------
void dfx::IIRFilter::copyCoefficients(IIRFilter const& inSourceFilter) noexcept
{
	mCoeff.mIn = inSourceFilter.mCoeff.mIn;
	mCoeff.mPrevIn = inSourceFilter.mCoeff.mPrevIn;
	mCoeff.mPrevPrevIn = inSourceFilter.mCoeff.mPrevPrevIn;
	mCoeff.mPrevOut = inSourceFilter.mCoeff.mPrevOut;
	mCoeff.mPrevPrevOut = inSourceFilter.mCoeff.mPrevPrevOut;
}



#pragma mark -

//------------------------------------------------------------------------
dfx::Crossover::Crossover(size_t inChannelCount, double inSampleRate, double inFrequency)
:	mSampleRate(inSampleRate),
#if DFX_CROSSOVER_LINKWITZ_RILEY_MUSICDSP
	mLowpassHistories(inChannelCount),
	mHighpassHistories(inChannelCount)
#else
	mLowpassFilters(inChannelCount),
	mHighpassFilters(inChannelCount)
#endif
{
	assert(inSampleRate > 0.);
	assert(inFrequency > 0.);
	//assert(inFrequency <= (inSampleRate / 2.));
	inFrequency = std::min(inFrequency, inSampleRate / 2.);  // upper-limit to Nyquist

#if !DFX_CROSSOVER_LINKWITZ_RILEY_MUSICDSP
	auto const initFilters = [inSampleRate](auto& channelFilters)
	{
		std::ranges::for_each(channelFilters, [inSampleRate](auto& chainedFilters)
		{
			std::ranges::for_each(chainedFilters, [inSampleRate](auto& filter)
			{
				filter = dfx::IIRFilter(inSampleRate);
			});
		});
	};
	initFilters(mLowpassFilters);
	initFilters(mHighpassFilters);
#endif

	setFrequency(inFrequency);
}

//------------------------------------------------------------------------
void dfx::Crossover::setFrequency(double inFrequency)
{
#if DFX_CROSSOVER_LINKWITZ_RILEY_MUSICDSP
	// https://www.musicdsp.org/en/latest/Filters/266-4th-order-linkwitz-riley-filters.html
	double const wc = 2. * std::numbers::pi_v<double> * inFrequency;
	auto const wc2 = wc * wc;
	auto const wc3 = wc2 * wc;
	auto const wc4 = wc2 * wc2;
	double const k = wc / std::tan(std::numbers::pi_v<double> * inFrequency / mSampleRate);
	auto const k2 = k * k;
	auto const k3 = k2 * k;
	auto const k4 = k2 * k2;
	auto const sq_tmp1 = std::numbers::sqrt2_v<double> * wc3 * k;
	auto const sq_tmp2 = std::numbers::sqrt2_v<double> * wc * k3;
	auto const a_tmp = (4. * wc2 * k2) + (2. * sq_tmp1) + k4 + (2. * sq_tmp2) + wc4;
	auto const a_tmp_inv = !dfx::math::IsZero(a_tmp) ? (1. / a_tmp) : 1.;

	mB1 = (4. * (wc4 + sq_tmp1 - k4 - sq_tmp2)) * a_tmp_inv;
	mB2 = ((6. * wc4) - (8. * wc2 * k2) + (6. * k4)) * a_tmp_inv;
	mB3 = (4. * (wc4 - sq_tmp1 + sq_tmp2 - k4)) * a_tmp_inv;
	mB4 = (k4 - (2. * sq_tmp1) + wc4 - (2. * sq_tmp2) + (4. * wc2 * k2)) * a_tmp_inv;

	mLowpassCoeff.mA0 = wc4 * a_tmp_inv;
	mLowpassCoeff.mA1 = 4. * wc4 * a_tmp_inv;
	mLowpassCoeff.mA2 = 6. * wc4 * a_tmp_inv;

	mHighpassCoeff.mA0 = k4 * a_tmp_inv;
	mHighpassCoeff.mA1 = -4. * k4 * a_tmp_inv;
	mHighpassCoeff.mA2 = 6. * k4 * a_tmp_inv;

#else
	auto const setCoefficients = [](auto& channelFilters, auto const& coeff)
	{
		std::ranges::for_each(channelFilters, [&coeff](auto& chainedFilters)
		{
			std::ranges::for_each(chainedFilters, [&coeff](auto& filter)
			{
				filter.setCoefficients(coeff);
			});
		});
	};
	PreCoeff const preCoeff(inFrequency, kDefaultQ_LP_HP, mSampleRate);
	setCoefficients(mLowpassFilters, CalculateCoefficients(dfx::IIRFilter::FilterType::Lowpass, preCoeff));
	setCoefficients(mHighpassFilters, CalculateCoefficients(dfx::IIRFilter::FilterType::Highpass, preCoeff));
#endif
}

//------------------------------------------------------------------------
void dfx::Crossover::reset()
{
#if DFX_CROSSOVER_LINKWITZ_RILEY_MUSICDSP
	auto const clearHistory = [](History& history)
	{
		history.reset();
	};
	std::ranges::for_each(mLowpassHistories, clearHistory);
	std::ranges::for_each(mHighpassHistories, clearHistory);
#else
	auto const resetFilters = [](auto& channelFilters)
	{
		std::ranges::for_each(channelFilters, [](auto& chainedFilters)
		{
			std::ranges::for_each(chainedFilters, [](auto& filter)
			{
				filter.reset();
			});
		});
	};
	resetFilters(mLowpassFilters);
	resetFilters(mHighpassFilters);
#endif
}

//------------------------------------------------------------------------
std::pair<float, float> dfx::Crossover::process(size_t inChannel, float inSample)
{
#if DFX_CROSSOVER_LINKWITZ_RILEY_MUSICDSP
	auto const process = [input = inSample, this](InputCoeff const& coeff, History& history)
	{
		double const output = dfx::math::ClampDenormal((coeff.mA0 * (input + history.mX4)) + (coeff.mA1 * (history.mX1 + history.mX3)) + (coeff.mA2 * history.mX2) - (mB1 * history.mY1) - (mB2 * history.mY2) - (mB3 * history.mY3) - (mB4 * history.mY4));
		history.mX4 = history.mX3;
		history.mX3 = history.mX2;
		history.mX2 = history.mX1;
		history.mX1 = input;
		history.mY4 = history.mY3;
		history.mY3 = history.mY2;
		history.mY2 = history.mY1;
		history.mY1 = output;
		return static_cast<float>(output);
	};
	return { process(mLowpassCoeff, mLowpassHistories[inChannel]), process(mHighpassCoeff, mHighpassHistories[inChannel]) };
#else
	auto const accumulateFilters = [inSample](auto& filters)
	{
		return std::accumulate(filters.begin(), filters.end(), inSample, [](auto const value, auto& filter)
		{
			return filter.process(value);
		});
	};
	return { accumulateFilters(mLowpassFilters[inChannel]), accumulateFilters(mHighpassFilters[inChannel]) };
#endif
}
