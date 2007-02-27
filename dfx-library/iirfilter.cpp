/*------------------- by Sophia Poirier  ][  December 2001 ------------------*/

#include "iirfilter.h"

#include "dfxmath.h"


//------------------------------------------------------------------------
IIRfilter::IIRfilter()
{
	reset();
}

//------------------------------------------------------------------------
void IIRfilter::calculateLowpassCoefficients(float inCutoff, float inSampleRate)
{
	float Q = 0.5f;
	float twoPiFreqDivSR = 2.0f * kDFX_PI_f * inCutoff / inSampleRate;	// ¹ ... 0
	float cosTwoPiFreqDivSR = cosf(twoPiFreqDivSR);						// 1 ... -1
	float slopeFactor = sinf(twoPiFreqDivSR) / (Q * 2.0f);				// 0 ... 1 ... 0
	float coeffScalar = 1.0f / (1.0f + slopeFactor);					// 1 ... 0.5 ... 1

	// calculate filter coefficients
	pInCoeff = (1.0f - cosTwoPiFreqDivSR) * coeffScalar;	// 0 ... 2
	inCoeff = ppInCoeff = pInCoeff * 0.5f;					// 0 ... 1
	pOutCoeff = (-2.0f * cosTwoPiFreqDivSR) * coeffScalar;	// -2 ... 2
	ppOutCoeff = (1.0f - slopeFactor) * coeffScalar;		// 1 ... 0 ... 1
}

//------------------------------------------------------------------------
void IIRfilter::calculateHighpassCoefficients(float inCutoff, float inSampleRate)
{
	float Q = 0.5f;
	float twoPiFreqDivSR = 2.0f * kDFX_PI_f * inCutoff / inSampleRate;	// ¹ ... 0
	float cosTwoPiFreqDivSR = cosf(twoPiFreqDivSR);						// 1 ... -1
	float slopeFactor = sinf(twoPiFreqDivSR) / (Q * 2.0f);				// 0 ... 1 ... 0
	float coeffScalar = 1.0f / (1.0f + slopeFactor);					// 1 ... 0.5 ... 1

	// calculate filter coefficients
	pInCoeff = (-1.0f - cosTwoPiFreqDivSR) * coeffScalar;	// 2 ... 0
	inCoeff = ppInCoeff = pInCoeff * (-0.5f);				// -1 ... 0
	pOutCoeff = (-2.0f * cosTwoPiFreqDivSR) * coeffScalar;	// -2 ... 2
	ppOutCoeff = (1.0f - slopeFactor) * coeffScalar;		// 1 ... 0 ... 1
}

//------------------------------------------------------------------------
void IIRfilter::copyCoefficients(IIRfilter * inSourceFilter)
{
	pOutCoeff = inSourceFilter->pOutCoeff;
	ppOutCoeff = inSourceFilter->ppOutCoeff;
	pInCoeff = inSourceFilter->pInCoeff;
	ppInCoeff = inSourceFilter->ppInCoeff;
	inCoeff = inSourceFilter->inCoeff;
}
