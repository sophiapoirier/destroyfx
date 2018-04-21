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

#pragma once


#include "dfxmath.h"


namespace dfx
{


//-----------------------------------------------------------------------------
class IIRfilter
{
public:
	enum class FilterType
	{
		Lowpass,
		Highpass,
		Bandpass,
		Peak,
		Notch,
		LowShelf,
		HighShelf
	};

	static double const kDefaultQ_LP_HP;
	static double const kShelfStartLowpass;

	void setCoefficients(FilterType inFilterType, double inFreq, double inQ, double inGain);
	void setLowpassCoefficients(double inCutoffFreq, double inQ = kDefaultQ_LP_HP);
	void setHighpassCoefficients(double inCutoffFreq, double inQ = kDefaultQ_LP_HP);
	void setBandpassCoefficients(double inCenterFreq, double inQ);
	void setCoefficients(float inA0, float inA1, float inA2, float inB1, float inB2);
	void copyCoefficients(IIRfilter const& inSourceFilter);
	void setSampleRate(double inSampleRate);

	void reset();


#ifdef USING_HERMITE
	void process(float inSample)
#else
	float process(float inSample)
#endif
	{
	#ifdef USING_HERMITE
		// store 4 samples of history if we're preprocessing for Hermite interpolation
		mPrevPrevPrevOut = mPrevPrevOut;
	#endif
		mPrevPrevOut = mPrevOut;
		mPrevOut = mCurrentOut;

#ifdef USE_OPTIMIZATION_THAT_ONLY_WORKS_FOR_LP_HP_NOTCH
		mCurrentOut = ((inSample + mPrevPrevIn) * mInCoeff) + (mPrevIn * mPrevInCoeff) 
						- (mPrevOut * mPrevOutCoeff) - (mPrevPrevOut * mPrevPrevOutCoeff);
#else
		mCurrentOut = (inSample * mInCoeff) + (mPrevIn * mPrevInCoeff) + (mPrevPrevIn * mPrevPrevInCoeff) 
						- (mPrevOut * mPrevOutCoeff) - (mPrevPrevOut * mPrevPrevOutCoeff);
#endif
		mCurrentOut = dfx::math::ClampDenormalValue(mCurrentOut);

		mPrevPrevIn = mPrevIn;
		mPrevIn = inSample;

	#ifndef USING_HERMITE
		return mCurrentOut;
	#endif
	}

#ifdef USING_HERMITE
// start of pre-Hermite-specific functions
// there are 4 versions, 3 of which unroll for loops of 2, 3, & 4 iterations

	void processH1(float inSample)
	{
		mPrevPrevPrevOut = mPrevPrevOut;
		mPrevPrevOut = mPrevOut;
		mPrevOut = mCurrentOut;
		//
		// XXX this uses an optimization that only works for LP, HP, and notch filters
		mCurrentOut = ((inSample + mPrevPrevIn) * mInCoeff) + (mPrevIn * mPrevInCoeff)
						- (mPrevOut * mPrevOutCoeff) - (mPrevPrevOut * mPrevPrevOutCoeff);
		mCurrentOut = dfx::math::ClampDenormalValue(mCurrentOut);
		//
		mPrevPrevIn = mPrevIn;
		mPrevIn = inSample;
	}

	void processH2(float * inAudio, long inPos, long inBufferSize)
	{
		auto const in0 = inAudio[inPos];
		auto const in1 = inAudio[(inPos + 1) % inBufferSize];

		mPrevPrevPrevOut = mPrevPrevOut;
		mPrevPrevOut = mPrevOut;
		mPrevOut = mCurrentOut;
		// XXX this uses an optimization that only works for LP, HP, and notch filters
		mCurrentOut = ((in0 + mPrevPrevIn) * mInCoeff) + (mPrevIn * mPrevInCoeff)
						- (mPrevOut * mPrevOutCoeff) - (mPrevPrevOut * mPrevPrevOutCoeff);
		//
		mPrevPrevPrevOut = mPrevPrevOut;
		mPrevPrevOut = mPrevOut;
		mPrevOut = mCurrentOut;
		mCurrentOut = ((in1 + mPrevIn) * mInCoeff) + (in0 * mPrevInCoeff)
						- (mPrevOut * mPrevOutCoeff) - (mPrevPrevOut * mPrevPrevOutCoeff);
		//
		mCurrentOut = dfx::math::ClampDenormalValue(mCurrentOut);
		mPrevPrevIn = in0;
		mPrevIn = in1;
	}

	void processH3(float * inAudio, long inPos, long inBufferSize)
	{
		auto const in0 = inAudio[inPos];
		auto const in1 = inAudio[(inPos + 1) % inBufferSize];
		auto const in2 = inAudio[(inPos + 2) % inBufferSize];

		// XXX this uses an optimization that only works for LP, HP, and notch filters
		mPrevPrevPrevOut = ((in0 + mPrevPrevIn) * mInCoeff) + (mPrevIn * mPrevInCoeff)
							- (mCurrentOut * mPrevOutCoeff) - (mPrevOut * mPrevPrevOutCoeff);
		mPrevPrevOut = ((in1 + mPrevIn) * mInCoeff) + (in0 * mPrevInCoeff)
						- (mPrevPrevPrevOut * mPrevOutCoeff) - (mCurrentOut * mPrevPrevOutCoeff);
		mPrevOut = ((in2 + in0) * mInCoeff) + (in1 * mPrevInCoeff)
					- (mPrevPrevOut * mPrevOutCoeff) - (mPrevPrevPrevOut * mPrevPrevOutCoeff);
		//
		mCurrentOut = mPrevOut;
		mCurrentOut = dfx::math::ClampDenormalValue(mCurrentOut);
		mPrevOut = mPrevPrevOut;
		mPrevPrevOut = mPrevPrevPrevOut;
		mPrevPrevPrevOut = mCurrentOut;
		//
		mPrevPrevIn = in1;
		mPrevIn = in2;
	}

	void processH4(float * inAudio, long inPos, long inBufferSize)
	{
		auto const in0 = inAudio[inPos];
		auto const in1 = inAudio[(inPos + 1) % inBufferSize];
		auto const in2 = inAudio[(inPos + 2) % inBufferSize];
		auto const in3 = inAudio[(inPos + 3) % inBufferSize];

		// XXX this uses an optimization that only works for LP, HP, and notch filters
		mPrevPrevPrevOut = ((in0 + mPrevPrevIn) * mInCoeff) + (mPrevIn * mPrevInCoeff)
							- (mCurrentOut * mPrevOutCoeff) - (mPrevOut * mPrevPrevOutCoeff);
		mPrevPrevOut = ((in1 + mPrevIn) * mInCoeff) + (in0 * mPrevInCoeff)
						- (mPrevPrevPrevOut * mPrevOutCoeff) - (mCurrentOut * mPrevPrevOutCoeff);
		mPrevOut = ((in2 + in0) * mInCoeff) + (in1 * mPrevInCoeff)
					- (mPrevPrevOut * mPrevOutCoeff) - (mPrevPrevPrevOut * mPrevPrevOutCoeff);
		mCurrentOut = ((in3 + in1) * mInCoeff) + (in2 * mPrevInCoeff)
						- (mPrevOut * mPrevOutCoeff) - (mPrevPrevOut * mPrevPrevOutCoeff);
		mCurrentOut = dfx::math::ClampDenormalValue(mCurrentOut);
		//
		mPrevPrevIn = in2;
		mPrevIn = in3;
	}

	// 4-point Hermite spline interpolation for use with IIR filter output histories
	float interpolateHermitePostFilter(double inPos)
	{
		auto const posFract = static_cast<float>(std::fmod(inPos, 1.0));

		float const a = ((3.0f * (mPrevPrevOut-mPrevOut)) - mPrevPrevPrevOut + mCurrentOut) * 0.5f;
		float const b = (2.0f * mPrevOut) + mPrevPrevPrevOut - (2.5f * mPrevPrevOut) - (mCurrentOut * 0.5f);
		float const c = (mPrevOut - mPrevPrevPrevOut) * 0.5f;

		return ((((a * posFract) + b) * posFract + c) * posFract) + mPrevPrevOut;
	}

#endif  // USING_HERMITE


private:
	FilterType mFilterType {};
	double mFilterFreq = 1.0;
	double mFilterQ = 1.0;
	double mFilterGain = 1.0;
	double mSampleRate = 1.0;

	float mPrevIn = 0.0f, mPrevPrevIn = 0.0f, mPrevOut = 0.0f, mPrevPrevOut = 0.0f, mPrevPrevPrevOut = 0.0f, mCurrentOut = 0.0f;
	float mPrevOutCoeff = 0.0f, mPrevPrevOutCoeff = 0.0f, mPrevInCoeff = 0.0f, mPrevPrevInCoeff = 0.0f, mInCoeff = 0.0f;
};


}  // namespace
