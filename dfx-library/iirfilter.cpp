/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2001-2018  Sophia Poirier

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


double const dfx::IIRfilter::kShelfStartLowpass = 0.333;
double const dfx::IIRfilter::kDefaultQ_LP_HP = std::sqrt(2.0) / 2.0;


//------------------------------------------------------------------------
void dfx::IIRfilter::reset()
{
	mPrevIn = mPrevPrevIn = mPrevOut = mPrevPrevOut = mPrevPrevPrevOut = mCurrentOut = 0.0f;
}

//------------------------------------------------------------------------
void dfx::IIRfilter::setCoefficients(FilterType inFilterType, double inFreq, double inQ, double inGain)
{
	mFilterType = inFilterType;
	mFilterFreq = inFreq;
	mFilterQ = inQ;
	mFilterGain = inGain;

	double const omega = 2.0 * dfx::math::kPi<double> * mFilterFreq / mSampleRate;
	auto const sn = std::sin(omega);
	auto const cs = std::cos(omega);
	double const alpha = std::sin(omega) / (2.0 * mFilterQ);
// XXX from http://musicdsp.org/showone.php?id=64
//alpha = std::sin(omega) * std::sinh(dfx::math::kLn2<double> / 2.0 * mFilterQ * omega / std::sin(omega));
	auto const A = std::sqrt(mFilterGain);
	double beta = (((A * A) + 1.0) / mFilterQ) - ((A - 1.0) * (A - 1.0));
	beta = std::max(beta, 0.0);
	beta = std::sqrt(beta);
	double b0 {};

	// calculate filter coefficients
	switch (mFilterType)
	{
		case FilterType::Lowpass:
			b0 = 1.0 + alpha;
			mInCoeff = mPrevPrevInCoeff = (1.0 - cs) * 0.5;
			mPrevInCoeff = 1.0 - cs;
			mPrevOutCoeff = -2.0 * cs;
			mPrevPrevOutCoeff = 1.0 - alpha;
			break;

		case FilterType::Highpass:
			b0 = 1.0 + alpha;
			mInCoeff = mPrevPrevInCoeff = ((1.0 + cs) * 0.5);
			mPrevInCoeff = -1.0 - cs;
			mPrevOutCoeff = -2.0 * cs;
			mPrevPrevOutCoeff = 1.0 - alpha;
			break;

		case FilterType::Bandpass:
			b0 = 1.0 + alpha;
			mInCoeff = alpha;
			mPrevInCoeff = 0.0;
			mPrevPrevInCoeff = -alpha;
			mPrevOutCoeff = -2.0 * cs;
			mPrevPrevOutCoeff = 1.0 - alpha;
			break;

		case FilterType::Peak:
			b0 = 1.0 + (alpha / A);
			mInCoeff = 1.0 + (alpha * A);
			mPrevInCoeff = mPrevOutCoeff = -2.0 * cs;
			mPrevPrevInCoeff = 1.0 - (alpha * A);
			mPrevPrevOutCoeff = 1.0 - (alpha / A);
			break;

		case FilterType::Notch:
			b0 = 1.0 + alpha;
			mInCoeff = mPrevPrevInCoeff = 1.0;
			mPrevInCoeff = mPrevOutCoeff = -2.0 * cs;
			mPrevPrevOutCoeff = 1.0 - alpha;
			break;

		case FilterType::LowShelf:
			b0 = (A + 1.0) + ((A - 1.0) * cs) + (beta * sn);
			mInCoeff = A * ((A + 1.0) - ((A - 1.0) * cs) + (beta * sn));
			mPrevInCoeff = 2.0 * A * ((A - 1.0) - ((A + 1.0) * cs));
			mPrevPrevInCoeff = A * ((A + 1.0) - ((A - 1.0) * cs) - (beta * sn));
			mPrevOutCoeff = -2.0 * ((A - 1.0) + ((A + 1.0) * cs));
			mPrevPrevOutCoeff = (A + 1.0) + ((A - 1.0) * cs) - (beta * sn);
			break;

		case FilterType::HighShelf:
			b0 = (A + 1.0) - ((A - 1.0) * cs) + (beta * sn);
			mInCoeff = A * ((A + 1.0) + ((A - 1.0)) * cs + (beta * sn));
			mPrevInCoeff = -2.0 * A * ((A - 1.0) + ((A + 1.0) * cs));
			mPrevPrevInCoeff = A * ((A + 1.0) + ((A - 1.0) * cs) - (beta * sn));
			mPrevOutCoeff = 2.0 * ((A - 1.0) - ((A + 1.0) * cs));
			mPrevPrevOutCoeff = (A + 1.0) - ((A - 1.0) * cs) - (beta * sn);
			break;

		default:
			assert(false);
			return;
	}

	if (b0 != 0.0)
	{
		mInCoeff /= b0;
		mPrevInCoeff /= b0;
		mPrevPrevInCoeff /= b0;
		mPrevOutCoeff /= b0;
		mPrevPrevOutCoeff /= b0;
	}
}

//------------------------------------------------------------------------
void dfx::IIRfilter::setSampleRate(double inSampleRate)
{
	mSampleRate = inSampleRate;
	// update after a change in sample rate
	setCoefficients(mFilterType, mFilterFreq, mFilterQ, mFilterGain);
}

//------------------------------------------------------------------------
void dfx::IIRfilter::setLowpassCoefficients(double inCutoffFreq, double inQ)
{
	setCoefficients(FilterType::Lowpass, inCutoffFreq, inQ, 1.0);
}

//------------------------------------------------------------------------
void dfx::IIRfilter::setHighpassCoefficients(double inCutoffFreq, double inQ)
{
	setCoefficients(FilterType::Highpass, inCutoffFreq, inQ, 1.0);
}

//------------------------------------------------------------------------
void dfx::IIRfilter::setBandpassCoefficients(double inCenterFreq, double inQ)
{
	setCoefficients(FilterType::Bandpass, inCenterFreq, inQ, 1.0);
}

//------------------------------------------------------------------------
void dfx::IIRfilter::setCoefficients(float inA0, float inA1, float inA2, float inB1, float inB2)
{
	mInCoeff = inA0;
	mPrevInCoeff = inA1;
	mPrevPrevInCoeff = inA2;
	mPrevOutCoeff = inB1;
	mPrevPrevOutCoeff = inB2;
}

//------------------------------------------------------------------------
void dfx::IIRfilter::copyCoefficients(IIRfilter const& inSourceFilter)
{
	mPrevOutCoeff = inSourceFilter.mPrevOutCoeff;
	mPrevPrevOutCoeff = inSourceFilter.mPrevPrevOutCoeff;
	mPrevInCoeff = inSourceFilter.mPrevInCoeff;
	mPrevPrevInCoeff = inSourceFilter.mPrevPrevInCoeff;
	mInCoeff = inSourceFilter.mInCoeff;
}
