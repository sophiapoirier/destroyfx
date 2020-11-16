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

#include "dfxmath.h"


static double const kDefaultQ_LP_HP = std::sqrt(2.0) / 2.0;  // C++20 constexpr 1.0 / std::numbers::sqrt2_v<double>


//------------------------------------------------------------------------
dfx::IIRFilter::IIRFilter(double inSampleRate)
:	mSampleRate(inSampleRate)
{
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
dfx::IIRFilter::Coefficients const& dfx::IIRFilter::setCoefficients(FilterType inFilterType, double inFreq, double inQ, double inGain)
{
	mFilterType = inFilterType;
	mFilterFreq = inFreq;
	mFilterQ = inQ;
	mFilterGain = inGain;

	double const omega = 2.0 * dfx::math::kPi<double> * inFreq / mSampleRate;  // radians per sample
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
			mCoeff.mIn = mCoeff.mPrevPrevIn = ((1.0 + cs) * 0.5);
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
	setCoefficients(mFilterType, mFilterFreq, mFilterQ, mFilterGain);
}

//------------------------------------------------------------------------
dfx::IIRFilter::Coefficients const& dfx::IIRFilter::setLowpassCoefficients(double inCutoffFreq)
{
	return setCoefficients(FilterType::Lowpass, inCutoffFreq, kDefaultQ_LP_HP, 1.0);
}

//------------------------------------------------------------------------
dfx::IIRFilter::Coefficients const& dfx::IIRFilter::setHighpassCoefficients(double inCutoffFreq)
{
	return setCoefficients(FilterType::Highpass, inCutoffFreq, kDefaultQ_LP_HP, 1.0);
}

//------------------------------------------------------------------------
dfx::IIRFilter::Coefficients const& dfx::IIRFilter::setBandpassCoefficients(double inCenterFreq, double inQ)
{
	return setCoefficients(FilterType::Bandpass, inCenterFreq, inQ, 1.0);
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
