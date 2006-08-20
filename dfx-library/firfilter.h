/*------------------- by Sophia Poirier  ][  January 2002 ------------------*/

#ifndef __DFX_FIR_FILTER_H
#define __DFX_FIR_FILTER_H


const float SHELF_START_FIR = 0.333f;


//-----------------------------------------------------------------------------
void calculateFIRidealLowpassCoefficients(float inCutoff, float inSampleRate, 
											long inNumTaps, float * inCoefficients);
void applyKaiserWindow(long inNumTaps, float * inCoefficients, float inAttenuation);
float besselIzero(float input);
float besselIzero2(float input);

//-----------------------------------------------------------------------------
inline float processFIRfilter(float * inAudio, long inNumTaps, float * inCoefficients, 
								long inPos, long inBufferSize)
{
	float outval = 0.0f;
	if ( (inPos+inNumTaps) > inBufferSize )
	{
		for (long i=0; i < inNumTaps; i++)
			outval += inAudio[ (inPos + i) % arraySize] * inCoefficients[i];
	}
	else
	{
		for (long i=0; i < inNumTaps; i++)
			outval += inAudio[inPos+i] * inCoefficients[i];
	}
	return outval;
} 


#endif
