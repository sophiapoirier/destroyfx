/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2001-2021  Sophia Poirier

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

To contact the author, use the contact form at http://destroyfx.org/

Destroy FX is a sovereign entity comprised of Sophia Poirier and Tom Murphy 7.
Welcome to our Infinite Impulse Response filter.
------------------------------------------------------------------------*/

#include "iirfilter.h"

#include <algorithm>
#include <cassert>
#include <numeric>

#include "dfxmath.h"


//------------------------------------------------------------------------
static double const kDefaultQ_LP_HP = std::sqrt(2.0) / 2.0;  // C++20 constexpr 1.0 / std::numbers::sqrt2_v<double>
constexpr double kUnityGain = 1.;

//------------------------------------------------------------------------
struct PreCoeff
{
	double const mOmega;  // radians per sample
	double const mSinOmega;
	double const mCosOmega;
	double const mAlpha;
	double const mA;
	double const mBeta;

	PreCoeff(double inFrequency, double inQ, double inGain, double inSampleRate)
	:	mOmega(2. * dfx::math::kPi<double> * inFrequency / inSampleRate),
		mSinOmega(std::sin(mOmega)),
		mCosOmega(std::cos(mOmega)),
		mAlpha(mSinOmega / (2. * inQ)),
		//mAlpha(mSinOmega * std::sinh(std::numbers::ln2 / 2. * inQ * mOmega / mSinOmega))  // http://musicdsp.org/showone.php?id=64
		mA(std::sqrt(inGain)),
		mBeta(std::sqrt(std::max((((mA * mA) + 1.) / inQ) - ((mA - 1.) * (mA - 1.)), 0.)))
	{
	}
};

//------------------------------------------------------------------------
static dfx::IIRFilter::Coefficients CalculateCoefficients(dfx::IIRFilter::FilterType inFilterType, PreCoeff const& inPreCoeff)
{
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
			assert(false);
			return dfx::IIRFilter::kZeroCoeff;
	}

	if (b0 != 0.)
	{
		coeff.mIn /= b0;
		coeff.mPrevIn /= b0;
		coeff.mPrevPrevIn /= b0;
		coeff.mPrevOut /= b0;
		coeff.mPrevPrevOut /= b0;
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
	assert(inQ > 0.);

	mFilterType = inFilterType;
	mFilterFrequency = inFrequency;
	mFilterQ = inQ;
	mFilterGain = inGain;

	mCoeff = CalculateCoefficients(inFilterType, PreCoeff(inFrequency, inQ, inGain, mSampleRate));
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
	return setCoefficients(FilterType::Lowpass, inCutoffFrequency, kDefaultQ_LP_HP, kUnityGain);
}

//------------------------------------------------------------------------
dfx::IIRFilter::Coefficients const& dfx::IIRFilter::setHighpassCoefficients(double inCutoffFrequency)
{
	return setCoefficients(FilterType::Highpass, inCutoffFrequency, kDefaultQ_LP_HP, kUnityGain);
}

//------------------------------------------------------------------------
dfx::IIRFilter::Coefficients const& dfx::IIRFilter::setBandpassCoefficients(double inCenterFrequency, double inQ)
{
	return setCoefficients(FilterType::Bandpass, inCenterFrequency, inQ, kUnityGain);
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
dfx::Crossover::Crossover(unsigned long inChannelCount, double inSampleRate, double inFrequency)
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
	assert(inFrequency <= (inSampleRate / 2.));
	inFrequency = std::min(inFrequency, inSampleRate / 2.);  // upper-limit to Nyquist

#if !DFX_CROSSOVER_LINKWITZ_RILEY_MUSICDSP
	auto const initFilters = [inSampleRate](auto& channelFilters)
	{
		std::for_each(channelFilters.begin(), channelFilters.end(), [inSampleRate](auto& chainedFilters)
		{
			std::for_each(chainedFilters.begin(), chainedFilters.end(), [inSampleRate](auto& filter)
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
	double const wc = 2. * dfx::math::kPi<double> * inFrequency;
	auto const wc2 = wc * wc;
	auto const wc3 = wc2 * wc;
	auto const wc4 = wc2 * wc2;
	double const k = wc / std::tan(dfx::math::kPi<double> * inFrequency / mSampleRate);
	auto const k2 = k * k;
	auto const k3 = k2 * k;
	auto const k4 = k2 * k2;
	static double const sqrt2 = std::sqrt(2.);
	auto const sq_tmp1 = sqrt2 * wc3 * k;
	auto const sq_tmp2 = sqrt2 * wc * k3;
	auto const a_tmp = (4. * wc2 * k2) + (2. * sq_tmp1) + k4 + (2. * sq_tmp2) + wc4;
	auto const a_tmp_inv = (a_tmp != 0.) ? (1. / a_tmp) : 1.;

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
		std::for_each(channelFilters.begin(), channelFilters.end(), [&coeff](auto& chainedFilters)
		{
			std::for_each(chainedFilters.begin(), chainedFilters.end(), [&coeff](auto& filter)
			{
				filter.setCoefficients(coeff);
			});
		});
	};
	PreCoeff const preCoeff(inFrequency, kDefaultQ_LP_HP, kUnityGain, mSampleRate);
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
	std::for_each(mLowpassHistories.begin(), mLowpassHistories.end(), clearHistory);
	std::for_each(mHighpassHistories.begin(), mHighpassHistories.end(), clearHistory);
#else
	auto const resetFilters = [](auto& channelFilters)
	{
		std::for_each(channelFilters.begin(), channelFilters.end(), [](auto& chainedFilters)
		{
			std::for_each(chainedFilters.begin(), chainedFilters.end(), [](auto& filter)
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
std::pair<float, float> dfx::Crossover::process(unsigned long inChannel, float inSample)
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
