/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2001-2020  Sophia Poirier

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

Destroy FX is a sovereign entity comprised of Sophia Poirier and Tom Murphy 7.
Welcome to our Infinite Impulse Response filter.
------------------------------------------------------------------------*/

#include "iirfilter.h"

#include <algorithm>
#include <cassert>
#include <numeric>

#include "dfxmath.h"


static double const kDefaultQ_LP_HP = std::sqrt(2.0) / 2.0;  // C++20 constexpr 1.0 / std::numbers::sqrt2_v<double>


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
// split out common value and filter-type-specific coefficient calculations?
dfx::IIRFilter::Coefficients const& dfx::IIRFilter::setCoefficients(FilterType inFilterType, double inFrequency, double inQ, double inGain)
{
	assert(inFrequency > 0.);
	assert(inQ > 0.);

	mFilterType = inFilterType;
	mFilterFrequency = inFrequency;
	mFilterQ = inQ;
	mFilterGain = inGain;

	double const omega = 2.0 * dfx::math::kPi<double> * inFrequency / mSampleRate;  // radians per sample
	auto const sn = std::sin(omega);
	auto const cs = std::cos(omega);
	double const alpha = std::sin(omega) / (2.0 * inQ);
// XXX from http://musicdsp.org/showone.php?id=64
//alpha = std::sin(omega) * std::sinh(std::numbers::ln2 / 2.0 * inQ * omega / std::sin(omega));
	auto const A = std::sqrt(inGain);
	auto const beta = [inQ, A]()
	{
		double result = (((A * A) + 1.0) / inQ) - ((A - 1.0) * (A - 1.0));
		result = std::max(result, 0.0);
		return std::sqrt(result);
	}();
	double b0 {};

	// calculate filter coefficients
	switch (inFilterType)
	{
		case FilterType::Lowpass:
			b0 = 1.0 + alpha;
			mCoeff.mIn = mCoeff.mPrevPrevIn = (1.0 - cs) * 0.5;
			mCoeff.mPrevIn = 1.0 - cs;
			mCoeff.mPrevOut = -2.0 * cs;
			mCoeff.mPrevPrevOut = 1.0 - alpha;
			break;

		case FilterType::Highpass:
			b0 = 1.0 + alpha;
			mCoeff.mIn = mCoeff.mPrevPrevIn = (1.0 + cs) * 0.5;
			mCoeff.mPrevIn = -1.0 - cs;
			mCoeff.mPrevOut = -2.0 * cs;
			mCoeff.mPrevPrevOut = 1.0 - alpha;
			break;

		case FilterType::Bandpass:
			b0 = 1.0 + alpha;
			mCoeff.mIn = alpha;
			mCoeff.mPrevIn = 0.0;
			mCoeff.mPrevPrevIn = -alpha;
			mCoeff.mPrevOut = -2.0 * cs;
			mCoeff.mPrevPrevOut = 1.0 - alpha;
			break;

		case FilterType::Peak:
			b0 = 1.0 + (alpha / A);
			mCoeff.mIn = 1.0 + (alpha * A);
			mCoeff.mPrevIn = mCoeff.mPrevOut = -2.0 * cs;
			mCoeff.mPrevPrevIn = 1.0 - (alpha * A);
			mCoeff.mPrevPrevOut = 1.0 - (alpha / A);
			break;

		case FilterType::Notch:
			b0 = 1.0 + alpha;
			mCoeff.mIn = mCoeff.mPrevPrevIn = 1.0;
			mCoeff.mPrevIn = mCoeff.mPrevOut = -2.0 * cs;
			mCoeff.mPrevPrevOut = 1.0 - alpha;
			break;

		case FilterType::LowShelf:
			b0 = (A + 1.0) + ((A - 1.0) * cs) + (beta * sn);
			mCoeff.mIn = A * ((A + 1.0) - ((A - 1.0) * cs) + (beta * sn));
			mCoeff.mPrevIn = 2.0 * A * ((A - 1.0) - ((A + 1.0) * cs));
			mCoeff.mPrevPrevIn = A * ((A + 1.0) - ((A - 1.0) * cs) - (beta * sn));
			mCoeff.mPrevOut = -2.0 * ((A - 1.0) + ((A + 1.0) * cs));
			mCoeff.mPrevPrevOut = (A + 1.0) + ((A - 1.0) * cs) - (beta * sn);
			break;

		case FilterType::HighShelf:
			b0 = (A + 1.0) - ((A - 1.0) * cs) + (beta * sn);
			mCoeff.mIn = A * ((A + 1.0) + ((A - 1.0)) * cs + (beta * sn));
			mCoeff.mPrevIn = -2.0 * A * ((A - 1.0) + ((A + 1.0) * cs));
			mCoeff.mPrevPrevIn = A * ((A + 1.0) + ((A - 1.0) * cs) - (beta * sn));
			mCoeff.mPrevOut = 2.0 * ((A - 1.0) - ((A + 1.0) * cs));
			mCoeff.mPrevPrevOut = (A + 1.0) - ((A - 1.0) * cs) - (beta * sn);
			break;

		default:
			assert(false);
			return kZeroCoeff;
	}

	if (b0 != 0.0)
	{
		mCoeff.mIn /= b0;
		mCoeff.mPrevIn /= b0;
		mCoeff.mPrevPrevIn /= b0;
		mCoeff.mPrevOut /= b0;
		mCoeff.mPrevPrevOut /= b0;
	}

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
	return setCoefficients(FilterType::Lowpass, inCutoffFrequency, kDefaultQ_LP_HP, 1.0);
}

//------------------------------------------------------------------------
dfx::IIRFilter::Coefficients const& dfx::IIRFilter::setHighpassCoefficients(double inCutoffFrequency)
{
	return setCoefficients(FilterType::Highpass, inCutoffFrequency, kDefaultQ_LP_HP, 1.0);
}

//------------------------------------------------------------------------
dfx::IIRFilter::Coefficients const& dfx::IIRFilter::setBandpassCoefficients(double inCenterFrequency, double inQ)
{
	return setCoefficients(FilterType::Bandpass, inCenterFrequency, inQ, 1.0);
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
#if DFX_CROSSOVER_LINKWITZ_RILEY
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

#if !DFX_CROSSOVER_LINKWITZ_RILEY
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
// https://www.musicdsp.org/en/latest/Filters/266-4th-order-linkwitz-riley-filters.html
void dfx::Crossover::setFrequency(double inFrequency)
{
#if DFX_CROSSOVER_LINKWITZ_RILEY
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
	setCoefficients(mLowpassFilters, dfx::IIRFilter(mSampleRate).setLowpassCoefficients(inFrequency));
	setCoefficients(mHighpassFilters, dfx::IIRFilter(mSampleRate).setHighpassCoefficients(inFrequency));
#endif
}

//------------------------------------------------------------------------
void dfx::Crossover::reset()
{
#if DFX_CROSSOVER_LINKWITZ_RILEY
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
#if DFX_CROSSOVER_LINKWITZ_RILEY
	auto const process = [input = inSample, this](InputCoeff const& coeff, History& history)
	{
		double const output = (coeff.mA0 * (input + history.mX4)) + (coeff.mA1 * (history.mX1 + history.mX3)) + (coeff.mA2 * history.mX2) - (mB1 * history.mY1) - (mB2 * history.mY2) - (mB3 * history.mY3) - (mB4 * history.mY4);
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
