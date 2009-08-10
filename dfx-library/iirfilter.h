/*------------------------------------------------------------------------
Destroy FX Library (version 1.0) is a collection of foundation code 
for creating audio software plug-ins.  
Copyright (C) 2001-2009  Sophia Poirier

This program is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

This program is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with this program.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, please visit http://destroyfx.org/ 
and use the contact form.

Destroy FX is a sovereign entity comprised of Sophia Poirier and Tom Murphy 7.
Welcome to our IIR filter.
by Sophia Poirier  ][  December 2001
------------------------------------------------------------------------*/

#ifndef __DFX_IIR_FILTER_H
#define __DFX_IIR_FILTER_H


#include "dfxmath.h"


const double SHELF_START_IIR_LOWPASS = 0.333;
extern const double kDfxIIRFilter_DefaultQ_LP_HP;

typedef enum {
	kDfxIIRFilterType_Lowpass,
	kDfxIIRFilterType_Highpass,
	kDfxIIRFilterType_Bandpass,
	kDfxIIRFilterType_Peak,
	kDfxIIRFilterType_Notch,
	kDfxIIRFilterType_LowShelf,
	kDfxIIRFilterType_HighShelf,
	kDfxIIRFilterType_NumTypes
} DfxIIRFilterType;


class DfxIIRfilter
{
public:
	DfxIIRfilter();

	void calculateCoefficients(DfxIIRFilterType inFilterType, double inFreq, double inQ, double inGain);
	void calculateLowpassCoefficients(double inCutoffFreq, double inQ = kDfxIIRFilter_DefaultQ_LP_HP);
	void calculateHighpassCoefficients(double inCutoffFreq, double inQ = kDfxIIRFilter_DefaultQ_LP_HP);
	void calculateBandpassCoefficients(double inCenterFreq, double inQ);
	void setCoefficients(float inA0, float inA1, float inA2, float inB1, float inB2);
	void copyCoefficients(DfxIIRfilter * inSourceFilter);
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
		mCurrentOut = ((inSample+mPrevPrevIn)*mInCoeff) + (mPrevIn*mPrevInCoeff) 
						- (mPrevOut*mPrevOutCoeff) - (mPrevPrevOut*mPrevPrevOutCoeff);
#else
		mCurrentOut = (inSample*mInCoeff) + (mPrevIn*mPrevInCoeff) + (mPrevPrevIn*mPrevPrevInCoeff) 
						- (mPrevOut*mPrevOutCoeff) - (mPrevPrevOut*mPrevPrevOutCoeff);
#endif
		mCurrentOut = DFX_ClampDenormalValue(mCurrentOut);

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
		mCurrentOut = ( (inSample+mPrevPrevIn) * mInCoeff ) + (mPrevIn  * mPrevInCoeff)
						- (mPrevOut * mPrevOutCoeff) - (mPrevPrevOut * mPrevPrevOutCoeff);
		mCurrentOut = DFX_ClampDenormalValue(mCurrentOut);
		//
		mPrevPrevIn = mPrevIn;
		mPrevIn = inSample;
	}

	void processH2(float * inAudio, long inPos, long inBufferSize)
	{
	  float in0 = inAudio[inPos];
	  float in1 = inAudio[ (inPos + 1) % inBufferSize ];

		mPrevPrevPrevOut = mPrevPrevOut;
		mPrevPrevOut = mPrevOut;
		mPrevOut = mCurrentOut;
		// XXX this uses an optimization that only works for LP, HP, and notch filters
		mCurrentOut = ( (in0+mPrevPrevIn) * mInCoeff ) + (mPrevIn * mPrevInCoeff)
						- (mPrevOut * mPrevOutCoeff) - (mPrevPrevOut * mPrevPrevOutCoeff);
		//
		mPrevPrevPrevOut = mPrevPrevOut;
		mPrevPrevOut = mPrevOut;
		mPrevOut = mCurrentOut;
		mCurrentOut = ( (in1+mPrevIn) * mInCoeff ) + (in0 * mPrevInCoeff)
						- (mPrevOut * mPrevOutCoeff) - (mPrevPrevOut * mPrevPrevOutCoeff);
		//
		mCurrentOut = DFX_ClampDenormalValue(mCurrentOut);
		mPrevPrevIn = in0;
		mPrevIn = in1;
	}

	void processH3(float * inAudio, long inPos, long inBufferSize)
	{
	  float in0 = inAudio[inPos];
	  float in1 = inAudio[ (inPos + 1) % inBufferSize ];
	  float in2 = inAudio[ (inPos + 2) % inBufferSize ];

		// XXX this uses an optimization that only works for LP, HP, and notch filters
		mPrevPrevPrevOut = ( (in0+mPrevPrevIn) * mInCoeff ) + (mPrevIn * mPrevInCoeff)
							- (mCurrentOut * mPrevOutCoeff) - (mPrevOut * mPrevPrevOutCoeff);
		mPrevPrevOut = ((in1+mPrevIn) * mInCoeff) + (in0 * mPrevInCoeff)
						- (mPrevPrevPrevOut * mPrevOutCoeff) - (mCurrentOut * mPrevPrevOutCoeff);
		mPrevOut = ((in2+in0) * mInCoeff) + (in1 * mPrevInCoeff)
					- (mPrevPrevOut * mPrevOutCoeff) - (mPrevPrevPrevOut * mPrevPrevOutCoeff);
		//
		mCurrentOut = mPrevOut;
		mCurrentOut = DFX_ClampDenormalValue(mCurrentOut);
		mPrevOut = mPrevPrevOut;
		mPrevPrevOut = mPrevPrevPrevOut;
		mPrevPrevPrevOut = mCurrentOut;
		//
		mPrevPrevIn = in1;
		mPrevIn = in2;
	}

	void processH4(float * inAudio, long inPos, long inBufferSize)
	{
	  float in0 = inAudio[inPos];
	  float in1 = inAudio[ (inPos + 1) % inBufferSize ];
	  float in2 = inAudio[ (inPos + 2) % inBufferSize ];
	  float in3 = inAudio[ (inPos + 3) % inBufferSize ];

		// XXX this uses an optimization that only works for LP, HP, and notch filters
		mPrevPrevPrevOut = ( (in0+mPrevPrevIn) * mInCoeff ) + (mPrevIn * mPrevInCoeff)
							- (mCurrentOut * mPrevOutCoeff) - (mPrevOut * mPrevPrevOutCoeff);
		mPrevPrevOut = ((in1+mPrevIn) * mInCoeff) + (in0 * mPrevInCoeff)
						- (mPrevPrevPrevOut * mPrevOutCoeff) - (mCurrentOut * mPrevPrevOutCoeff);
		mPrevOut = ((in2+in0) * mInCoeff) + (in1 * mPrevInCoeff)
					- (mPrevPrevOut * mPrevOutCoeff) - (mPrevPrevPrevOut * mPrevPrevOutCoeff);
		mCurrentOut = ((in3+in1) * mInCoeff) + (in2 * mPrevInCoeff)
						- (mPrevOut * mPrevOutCoeff) - (mPrevPrevOut * mPrevPrevOutCoeff);
		mCurrentOut = DFX_ClampDenormalValue(mCurrentOut);
		//
		mPrevPrevIn = in2;
		mPrevIn = in3;
	}

	// 4-point Hermite spline interpolation for use with IIR filter output histories
	float interpolateHermitePostFilter(double inPos)
	{
		float posFract = (float) (inPos - floor(inPos));	// XXX or fmod(inPos, 1.0) ?

		float a = ( (3.0f*(mPrevPrevOut-mPrevOut)) - 
					mPrevPrevPrevOut + mCurrentOut ) * 0.5f;
		float b = (2.0f*mPrevOut) + mPrevPrevPrevOut - 
					(2.5f*mPrevPrevOut) - (mCurrentOut*0.5f);
		float c = (mPrevOut - mPrevPrevPrevOut) * 0.5f;

		return (( ((a*posFract)+b) * posFract + c ) * posFract) + mPrevPrevOut;
	}

#endif
// end of USING_HERMITE batch of functions


private:
	DfxIIRFilterType mFilterType;
	double mFilterFreq, mFilterQ, mFilterGain;
	double mSampleRate;

	float mPrevIn, mPrevPrevIn, mPrevOut, mPrevPrevOut, mPrevPrevPrevOut, mCurrentOut;
	float mPrevOutCoeff, mPrevPrevOutCoeff, mPrevInCoeff, mPrevPrevInCoeff, mInCoeff;
};


#endif
