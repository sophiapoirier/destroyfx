/*------------------- by Marc Poirier  ][  January 2002 ------------------*/

#ifndef __dfx_firfilter_h
#define __dfx_firfilter_h


#define PI   3.1415926535897932384626433832795f
#define SHELF_START_FIR   0.333f


//-----------------------------------------------------------------------------
void calculateFIRidealLowpassCoefficients(float cutoff, float samplerate, 
											long numTaps, float * coefficients);
void applyKaiserWindow(long numTaps, float * coefficients, float attenuation);
float besselIzero(float input);
float besselIzero2(float input);

//-----------------------------------------------------------------------------
inline float processFIRfilter(float * inStream, long numTaps, float * coefficients, 
								long inPos, long arraySize)
{
	float outval = 0.0f;
	if ( (inPos+numTaps) > arraySize )
	{
		for (long i=0; i < numTaps; i++)
			outval += inStream[(inPos+i)%arraySize] * coefficients[i];
	}
	else
	{
		for (long i=0; i < numTaps; i++)
			outval += inStream[inPos+i] * coefficients[i];
	}
	return outval;
} 


#endif