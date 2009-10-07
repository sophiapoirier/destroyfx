/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2001-2009  Sophia Poirier

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

#include "dfxmath.h"


const double DfxIIRfilter::kShelfStartLowpass = 0.333;
const double DfxIIRfilter::kDefaultQ_LP_HP = sqrt(2.0) / 2.0;


//------------------------------------------------------------------------
DfxIIRfilter::DfxIIRfilter()
{
	mFilterType = kDfxIIRFilterType_Lowpass;
	mFilterFreq = 1.0;
	mFilterQ = 1.0;
	mFilterGain = 1.0;
	mSampleRate = 1.0;

	mPrevOutCoeff = mPrevPrevOutCoeff = mPrevInCoeff = mPrevPrevInCoeff = mInCoeff = 0.0f;

	reset();
}

//------------------------------------------------------------------------
void DfxIIRfilter::reset()
{
	mPrevIn = mPrevPrevIn = mPrevOut = mPrevPrevOut = mPrevPrevPrevOut = mCurrentOut = 0.0f;
}

//------------------------------------------------------------------------
void DfxIIRfilter::calculateCoefficients(DfxIIRFilterType inFilterType, double inFreq, double inQ, double inGain)
{
	mFilterType = inFilterType;
	mFilterFreq = inFreq;
	mFilterQ = inQ;
	mFilterGain = inGain;

	double omega = 2.0 * kDFX_PI_d * mFilterFreq / mSampleRate;
	double sn = sin(omega);
	double cs = cos(omega);
	double alpha = sin(omega) / (2.0 * mFilterQ);
// XXX from http://musicdsp.org/showone.php?id=64
//alpha = sin(omega) * sinh(kDFX_LN2_d / 2.0 * mFilterQ * omega / sin(omega));
	double A = sqrt(mFilterGain);
	double beta = (((A*A) + 1.0) / mFilterQ) - ((A-1.0) * (A-1.0));
	if (beta < 0.0)
		beta = 0.0;
	beta = sqrt(beta);
	double b0 = 1.0 + alpha;

	// calculate filter coefficients
	switch (mFilterType)
	{
		case kDfxIIRFilterType_Lowpass:
			b0 = 1.0 + alpha;
			mInCoeff = mPrevPrevInCoeff = ((1.0 - cs) * 0.5);
			mPrevInCoeff = (1.0 - cs);
			mPrevOutCoeff = (-2.0 * cs);
			mPrevPrevOutCoeff = (1.0 - alpha);
			break;

		case kDfxIIRFilterType_Highpass:
			b0 = 1.0 + alpha;
			mInCoeff = mPrevPrevInCoeff = ((1.0 + cs) * 0.5);
			mPrevInCoeff = (-1.0 - cs);
			mPrevOutCoeff = (-2.0 * cs);
			mPrevPrevOutCoeff = (1.0 - alpha);
			break;

		case kDfxIIRFilterType_Bandpass:
			b0 = 1.0 + alpha;
			mInCoeff = alpha;
			mPrevInCoeff = 0.0;
			mPrevPrevInCoeff = -alpha;
			mPrevOutCoeff = (-2.0 * cs);
			mPrevPrevOutCoeff = (1.0 - alpha);
			break;

		case kDfxIIRFilterType_Peak:
			b0 = 1.0 + (alpha / A);
			mInCoeff = 1.0 + (alpha * A);
			mPrevInCoeff = mPrevOutCoeff = -2.0 * cs;
			mPrevPrevInCoeff = 1.0 - (alpha * A);
			mPrevPrevOutCoeff = 1.0 - (alpha / A);
			break;

		case kDfxIIRFilterType_Notch:
			b0 = 1.0 + alpha;
			mInCoeff = mPrevPrevInCoeff = 1.0;
			mPrevInCoeff = mPrevOutCoeff = (-2.0 * cs);
			mPrevPrevOutCoeff = (1.0 - alpha);
			break;

		case kDfxIIRFilterType_LowShelf:
			b0 = (A + 1.0) + ((A - 1.0) * cs) + (beta * sn);
			mInCoeff = A * ((A + 1.0) - ((A - 1.0) * cs) + (beta * sn));
			mPrevInCoeff = 2.0 * A * ((A - 1.0) - ((A + 1.0) * cs));
			mPrevPrevInCoeff = A * ((A + 1.0) - ((A - 1.0) * cs) - (beta * sn));
			mPrevOutCoeff = -2.0 * ((A - 1.0) + ((A + 1.0) * cs));
			mPrevPrevOutCoeff = (A + 1.0) + ((A - 1.0) * cs) - (beta * sn);
			break;

		case kDfxIIRFilterType_HighShelf:
			b0 = (A + 1.0) - ((A - 1.0) * cs) + (beta * sn);
			mInCoeff = A * ((A + 1.0) + ((A - 1.0)) * cs + (beta * sn));
			mPrevInCoeff = -2.0 * A * ((A - 1.0) + ((A + 1.0) * cs));
			mPrevPrevInCoeff = A * ((A + 1.0) + ((A - 1.0) * cs) - (beta * sn));
			mPrevOutCoeff = 2.0 * ((A - 1.0) - ((A + 1.0) * cs));
			mPrevPrevOutCoeff = (A + 1.0) - ((A - 1.0) * cs) - (beta * sn);
			break;

		default:
			break;
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
void DfxIIRfilter::setSampleRate(double inSampleRate)
{
	mSampleRate = inSampleRate;
	// update after a change in sample rate
	calculateCoefficients(mFilterType, mFilterFreq, mFilterQ, mFilterGain);
}

//------------------------------------------------------------------------
void DfxIIRfilter::calculateLowpassCoefficients(double inCutoffFreq, double inQ)
{
	calculateCoefficients(kDfxIIRFilterType_Lowpass, inCutoffFreq, inQ, 1.0);
}

//------------------------------------------------------------------------
void DfxIIRfilter::calculateHighpassCoefficients(double inCutoffFreq, double inQ)
{
	calculateCoefficients(kDfxIIRFilterType_Highpass, inCutoffFreq, inQ, 1.0);
}

//------------------------------------------------------------------------
void DfxIIRfilter::calculateBandpassCoefficients(double inCenterFreq, double inQ)
{
	calculateCoefficients(kDfxIIRFilterType_Bandpass, inCenterFreq, inQ, 1.0);
}

//------------------------------------------------------------------------
void DfxIIRfilter::setCoefficients(float inA0, float inA1, float inA2, float inB1, float inB2)
{
	mInCoeff = inA0;
	mPrevInCoeff = inA1;
	mPrevPrevInCoeff = inA2;
	mPrevOutCoeff = inB1;
	mPrevPrevOutCoeff = inB2;
}

//------------------------------------------------------------------------
void DfxIIRfilter::copyCoefficients(DfxIIRfilter * inSourceFilter)
{
	mPrevOutCoeff = inSourceFilter->mPrevOutCoeff;
	mPrevPrevOutCoeff = inSourceFilter->mPrevPrevOutCoeff;
	mPrevInCoeff = inSourceFilter->mPrevInCoeff;
	mPrevPrevInCoeff = inSourceFilter->mPrevPrevInCoeff;
	mInCoeff = inSourceFilter->mInCoeff;
}
