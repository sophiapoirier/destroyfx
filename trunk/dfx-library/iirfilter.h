/*------------------- by Sophia Poirier  ][  December 2001 ------------------*/

#ifndef __DFX_IIR_FILTER_H
#define __DFX_IIR_FILTER_H


const float SHELF_START_IIR = 0.333f;


class IIRfilter
{
public:
	IIRfilter();

	void calculateLowpassCoefficients(float inCutoff, float inSampleRate);
	void calculateHighpassCoefficients(float inCutoff, float inSampleRate);
	void copyCoefficients(IIRfilter * inSourceFilter);

	void reset()
	{
		prevIn = prevprevIn = prevOut = prevprevOut = prevprevprevOut = currentOut = 0.0f;
	}

	float prevIn, prevprevIn, prevOut, prevprevOut, prevprevprevOut, currentOut;
	float pOutCoeff, ppOutCoeff, pInCoeff, ppInCoeff, inCoeff;


#ifdef USING_HERMITE
	void process(float inSample)
#else
	float process(float inSample)
#endif
	{
	#ifdef USING_HERMITE
		// store 4 samples of history if we're preprocessing for Hermite interpolation
		prevprevprevOut = prevprevOut;
	#endif
		prevprevOut = prevOut;
		prevOut = currentOut;

//		currentOut = (inSample*inCoeff) + (prevIn*pInCoeff) + (prevprevIn*ppInCoeff) 
//					- (prevOut*pOutCoeff) - (prevprevOut*ppOutCoeff);
		currentOut = ((inSample+prevprevIn)*inCoeff) + (prevIn*pInCoeff) 
					- (prevOut*pOutCoeff) - (prevprevOut*ppOutCoeff);

		prevprevIn = prevIn;
		prevIn = inSample;

	#ifndef USING_HERMITE
		return currentOut;
	#endif
	}

#ifdef USING_HERMITE
// start of pre-Hermite-specific functions
// there are 4 versions, 3 of which unroll for loops of 2, 3, & 4 iterations

	void processH1(float inSample)
	{
		prevprevprevOut = prevprevOut;
		prevprevOut = prevOut;
		prevOut = currentOut;
		//
		currentOut = ( (inSample+prevprevIn) * inCoeff ) + (prevIn  * pInCoeff)
					- (prevOut * pOutCoeff) - (prevprevOut * ppOutCoeff);
		//
		prevprevIn = prevIn;
		prevIn = inSample;
	}

	void processH2(float * inAudio, long inPos, long inBufferSize)
	{
	  float in0 = inAudio[inPos];
	  float in1 = inAudio[ (inPos + 1) % inBufferSize ];

		prevprevprevOut = prevprevOut;
		prevprevOut = prevOut;
		prevOut = currentOut;
		currentOut = ( (in0+prevprevIn) * inCoeff ) + (prevIn * pInCoeff)
					- (prevOut * pOutCoeff) - (prevprevOut * ppOutCoeff);
		//
		prevprevprevOut = prevprevOut;
		prevprevOut = prevOut;
		prevOut = currentOut;
		currentOut = ( (in1+prevIn) * inCoeff ) + (in0 * pInCoeff)
					- (prevOut * pOutCoeff) - (prevprevOut * ppOutCoeff);
		//
		prevprevIn = in0;
		prevIn = in1;
	}

	void processH3(float * inAudio, long inPos, long inBufferSize)
	{
	  float in0 = inAudio[inPos];
	  float in1 = inAudio[ (inPos + 1) % inBufferSize ];
	  float in2 = inAudio[ (inPos + 2) % inBufferSize ];

		prevprevprevOut = ( (in0+prevprevIn) * inCoeff ) + (prevIn * pInCoeff)
						- (currentOut * pOutCoeff) - (prevOut * ppOutCoeff);
		prevprevOut = ((in1+prevIn) * inCoeff) + (in0 * pInCoeff)
						- (prevprevprevOut * pOutCoeff) - (currentOut * ppOutCoeff);
		prevOut = ((in2+in0) * inCoeff) + (in1 * pInCoeff)
						- (prevprevOut * pOutCoeff) - (prevprevprevOut * ppOutCoeff);
		//
		currentOut = prevOut;
		prevOut = prevprevOut;
		prevprevOut = prevprevprevOut;
		prevprevprevOut = currentOut;
		//
		prevprevIn = in1;
		prevIn = in2;
	}

	void processH4(float * inAudio, long inPos, long inBufferSize)
	{
	  float in0 = inAudio[inPos];
	  float in1 = inAudio[ (inPos + 1) % inBufferSize ];
	  float in2 = inAudio[ (inPos + 2) % inBufferSize ];
	  float in3 = inAudio[ (inPos + 3) % inBufferSize ];

		prevprevprevOut = ( (in0+prevprevIn) * inCoeff ) + (prevIn * pInCoeff)
						- (currentOut * pOutCoeff) - (prevOut * ppOutCoeff);
		prevprevOut = ((in1+prevIn) * inCoeff) + (in0 * pInCoeff)
						- (prevprevprevOut * pOutCoeff) - (currentOut * ppOutCoeff);
		prevOut = ((in2+in0) * inCoeff) + (in1 * pInCoeff)
						- (prevprevOut * pOutCoeff) - (prevprevprevOut * ppOutCoeff);
		currentOut = ((in3+in1) * inCoeff) + (in2 * pInCoeff)
						- (prevOut * pOutCoeff) - (prevprevOut * ppOutCoeff);
		//
		prevprevIn = in2;
		prevIn = in3;
	}

#endif
// end of USING_HERMITE batch of functions

};	// end of IIRfilter class definition



// 4-point Hermite spline interpolation for use with IIR filter output histories
inline float interpolateHermitePostFilter(IIRfilter * inFilter, double inPos)
{
	long pos_i = (long)inPos;
	float posFract = (float) (inPos - (double)pos_i);

	float a = ( (3.0f*(inFilter->prevprevOut-inFilter->prevOut)) - 
				inFilter->prevprevprevOut + inFilter->currentOut ) * 0.5f;
	float b = (2.0f*inFilter->prevOut) + inFilter->prevprevprevOut - 
				(2.5f*inFilter->prevprevOut) - (inFilter->currentOut*0.5f);
	float c = (inFilter->prevOut - inFilter->prevprevprevOut) * 0.5f;

	return (( ((a*posFract)+b) * posFract + c ) * posFract) + inFilter->prevprevOut;
}


#endif
