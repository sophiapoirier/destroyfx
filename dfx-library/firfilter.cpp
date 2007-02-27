/*------------------- by Sophia Poirier  ][  January 2002 ------------------*/

#include "firfilter.h"

#include "dfxmath.h"



//-----------------------------------------------------------------------------
// you're supposed to use use an odd number of taps
void calculateFIRidealLowpassCoefficients(float inCutoff, float inSampleRate, 
											long inNumTaps, float * inCoefficients)
{
	// get the cutoff as a ratio of cutoff to Nyquist, scaled from 0 to Pi
	float corner = (inCutoff / (inSampleRate*0.5f)) * kDFX_PI_f;

	long middleCoeff;
	if (inNumTaps % 2)
	{
		middleCoeff = (inNumTaps-1) / 2;
		inCoefficients[middleCoeff] = corner / kDFX_PI_f;
	}
	else
		middleCoeff = inNumTaps / 2;

	for (long n=0; n < middleCoeff; n++)
	{
		float value = (float)n - ((float)(inNumTaps-1) * 0.5f);
		inCoefficients[n] = sinf(value * corner) / (value * kDFX_PI_f);
		inCoefficients[inNumTaps - 1 - n] = inCoefficients[n];
	}
}

//-----------------------------------------------------------------------------
void applyKaiserWindow(long inNumTaps, float * inCoefficients, float inAttenuation)
{
	// beta is 0 if the attenuation is less than 21 dB
	float beta = 0.0f;
	if (inAttenuation >= 50.0f)
		beta = 0.1102f * (inAttenuation - 8.71f);
	else if ( (inAttenuation < 50.0f) && (inAttenuation >= 21.0f) )
	{
		beta = 0.5842f * powf( (inAttenuation - 21.0f), 0.4f);
		beta += 0.07886f * (inAttenuation - 21.0f);
	}

	long halfLength;
	if (inNumTaps % 2)
		halfLength = (inNumTaps + 1) / 2;
	else
		halfLength = inNumTaps / 2;

	for (long n=0; n < halfLength; n++)
	{
		inCoefficients[n] *= besselIzero(beta * 
					sqrtf(1.0f - powf( (1.0f-((2.0f*n)/((float)(inNumTaps-1)))), 2.0f ))) 
				/ besselIzero(beta);
		inCoefficients[inNumTaps-1-n] = inCoefficients[n];
	}
} 

//-----------------------------------------------------------------------------
float besselIzero(float input)
{
	float sum = 1.0f;
	float halfIn = input * 0.5f;
	float denominator = 1.0f;
	float numerator = 1.0f;
	for (int m=1; m <= 32; m++)
	{
		numerator *= halfIn;
		denominator *= (float)m;
		float term = numerator / denominator;
		sum += term * term;
	}
	return sum;
}

//-----------------------------------------------------------------------------
float besselIzero2(float input)
{
	float sum = 1.0f;
	float ds = 1.0f;
	float d = 0.0f;

	do
	{
		d += 2.0f;
		ds *= (input * input) / (d * d);
		sum += ds;
	}
	while ( ds > (1E-7f * sum) );

	return sum;
}
