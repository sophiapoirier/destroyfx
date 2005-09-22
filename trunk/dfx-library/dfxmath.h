/*------------------------------------------------------------------------
Destroy FX is a sovereign entity comprised of Marc Poirier & Tom Murphy 7.  
This is our math shit.
written by Tom Murphy 7 and Marc Poirier, 2001 - 2003
------------------------------------------------------------------------*/


#ifndef __DFXMATH_H
#define __DFXMATH_H

#include <math.h>
#include <stdlib.h>	// for RAND_MAX


// XXX figure out another way
#ifdef __APPLE_CC__
	#define sqrtf	(float)sqrt
	#define powf	(float)pow
	#define sinf	(float)sin
	#define cosf	(float)cos
	#define log10f	(float)log10
#endif



//-----------------------------------------------------------------------------
// constants & macros

inline float linear2dB(float linearValue)
{
	return 20.0f * log10f(linearValue);
}

#ifndef PI
#define PI 3.1415926535897932384626433832795f
#endif
#ifndef PId
#define PId 3.1415926535897932384626433832795
#endif

// reduces wasteful casting & division
const float ONE_DIV_RAND_MAX = 1.0f / (float)RAND_MAX;
#define randFloat()   ( (float)rand() * ONE_DIV_RAND_MAX )
const double ONE_DIV_RAND_MAX_D = 1.0 / (double)RAND_MAX;
#define randDouble()   ( (double)rand() * ONE_DIV_RAND_MAX_D )

#ifndef clip
#define clip(fval)   (if (fval < -1.0f) fval = -1.0f; else if (fval > 1.0f) fval = 1.0f)
#endif

inline float fsign(float fval)
{
	return (fval < 0.0f) ? -1.0f : 1.0f;
}

#ifndef undenormalize
#define undenormalize(dval)   if (fabs(dval) < 1.0e-15)   dval = 0.0
//#define undenormalize(fval)	if ( ((*((unsigned int*)&fval)) & 0x7f800000) == 0 )   fval = 0.0f
//#define IS_DENORMAL(fval)   ( (*((unsigned int*)&fval)) & 0x7f800000) == 0 )
#endif



//-----------------------------------------------------------------------------
// inline functions

inline float interpolateHermite(float * inData, double inAddress, long inArraySize)
{
	long pos = (long)inAddress;
	float posFract = (float) (inAddress - (double)pos);

#if 0	// XXX test performance using fewer variables/registers
	long posMinus1 = (pos == 0) ? inArraySize-1 : pos-1;
	long posPlus1 = (pos+1) % inArraySize;
	long posPlus2 = (pos+2) % inArraySize;

	return (( (((((3.0f*(inData[pos]-inData[posPlus1])) - inData[posMinus1] + inData[posPlus2]) * 0.5f) * posFract)
				+ ((2.0f*inData[posPlus1]) + inData[posMinus1] - (2.5f*inData[pos]) - (inData[posPlus2]*0.5f))) * 
				posFract + ((inData[posPlus1] - inData[posMinus1]) * 0.5f) ) * posFract) + inData[pos];

#elif 1	// XXX also test using float variables of inData[] rather than looking up with posMinus1, etc.
	float dataPosMinus1 = inData[ (pos == 0) ? inArraySize-1 : pos-1 ];
	float dataPosPlus1 = inData[ (pos+1) % inArraySize ];
	float dataPosPlus2 = inData[ (pos+2) % inArraySize ];

	float a = ( (3.0f*(inData[pos]-dataPosPlus1)) - dataPosMinus1 + dataPosPlus2 ) * 0.5f;
	float b = (2.0f*dataPosPlus1) + dataPosMinus1 - (2.5f*inData[pos]) - (dataPosPlus2*0.5f);
	float c = (dataPosPlus1 - dataPosMinus1) * 0.5f;

	return (( ((a*posFract)+b) * posFract + c ) * posFract) + inData[pos];

#else
	long posMinus1 = (pos == 0) ? inArraySize-1 : pos-1;
	long posPlus1 = (pos+1) % inArraySize;
	long posPlus2 = (pos+2) % inArraySize;

	float a = ( (3.0f*(inData[pos]-inData[posPlus1])) - inData[posMinus1] + inData[posPlus2] ) * 0.5f;
	float b = (2.0f*inData[posPlus1]) + inData[posMinus1] - (2.5f*inData[pos]) - (inData[posPlus2]*0.5f);
	float c = (inData[posPlus1] - inData[posMinus1]) * 0.5f;

	return (( ((a*posFract)+b) * posFract + c ) * posFract) + inData[pos];
#endif
}

inline float interpolateHermite_noWrap(float * inData, double inAddress, long inArraySize)
{
	long pos = (long)inAddress;
	float posFract = (float) (inAddress - (double)pos);

	float dataPosMinus1 = (pos == 0) ? 0.0f : inData[pos-1];
	float dataPosPlus1 = ((pos+1) == inArraySize) ? 0.0f : inData[pos+1];
	float dataPosPlus2 = ((pos+2) >= inArraySize) ? 0.0f : inData[pos+2];

	float a = ( (3.0f*(inData[pos]-dataPosPlus1)) - dataPosMinus1 + dataPosPlus2 ) * 0.5f;
	float b = (2.0f*dataPosPlus1) + dataPosMinus1 - (2.5f*inData[pos]) - (dataPosPlus2*0.5f);
	float c = (dataPosPlus1 - dataPosMinus1) * 0.5f;

	return (( ((a*posFract)+b) * posFract + c ) * posFract) + inData[pos];
}

inline float interpolateLinear(float * inData, double inAddress, long inArraySize)
{
	long pos = (long)inAddress;
	float posFract = (float) (inAddress - (double)pos);
	return (inData[pos] * (1.0f-posFract)) + (inData[(pos+1)%inArraySize] * posFract);
}

inline float interpolateRandom(float randMin, float randMax)
{
	float randy = (float)rand() * ONE_DIV_RAND_MAX;
	return ((randMax-randMin) * randy) + randMin;
}

inline float interpolateLinear2values(float point1, float point2, double inAddress)
{
	float posFract = (float) (inAddress - (double)((long)inAddress));
	return (point1 * (1.0f-posFract)) + (point2 * posFract);
}

// return the parameter with larger magnitude
inline float magmax(float a, float b) {
  if (fabsf(a) > fabsf(b)) return a;
  else return b;
}

// computes the principle branch of the Lambert W function
//    { LambertW(x) = W(x), where W(x) * exp(W(x)) = x }
inline double LambertW(double input)
{
  double x = fabs(input);

	if (x <= 500.0)
		return 0.665 * ( 1.0 + (0.0195 * log(x+1.0)) ) * log(x+1.0) + 0.04;
	else
		return log(x-4.0) - ( (1.0 - 1.0/log(x)) * log(log(x)) );
}

//-----------------------------------------------------------------------------
// I found this somewhere on the internet
/*
inline float anotherSqrt(float x)
{
	float result = 1.0f;
	float store = 0.0f;
	while (store != result)
	{
		store = result;
		result = 0.5f * (result + (x / result));
	}
}
*/


#endif