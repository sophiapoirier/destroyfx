/*------------------------------------------------------------------------
Destroy FX is a sovereign entity comprised of Marc Poirier & Tom Murphy 7.  
This is our math shit.
written by Tom Murphy 7 and Marc Poirier, 2001 - 2002
------------------------------------------------------------------------*/


#ifndef __DFXMATH_H
#define __DFXMATH_H

#include <math.h>
#include <stdlib.h>	// for RAND_MAX


// XXX figure out another way
#ifndef sqrtf
#define sqrtf (float)sqrt
#endif
#ifndef powf
#define powf (float)pow
#endif
#ifndef sinf
#define sinf (float)sin
#endif
#ifndef cosf
#define cosf (float)cos
#endif
#ifndef fabsf
#define fabsf (float)fabs
#endif



//-----------------------------------------------------------------------------
// constants & macros

#define dBconvert(fval) ( 20.0f * log10f((fval)) )

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

#ifndef fsign
#define fsign(fval) (((fval)<0.0f)?-1.0f:1.0f)
#endif

#ifndef undenormalize
#define undenormalize(dval)   if (fabs(dval) < 1.0e-15)   dval = 0.0
//#define undenormalize(fval)  (((*(unsigned int*)&(fval))&0x7f800000)==0)?0.0f:(fval)
#endif



//-----------------------------------------------------------------------------
// inline functions

inline float interpolateHermite(float *data, double address, long arraysize)
{
	long pos = (long)address;
	float posFract = (float) (address - (double)pos);

#if 0	// XXX test performance using fewer variables/registers
	long posMinus1 = (pos == 0) ? arraysize-1 : pos-1;
	long posPlus1 = (pos+1) % arraysize;
	long posPlus2 = (pos+2) % arraysize;

	return (( (((((3.0f*(data[pos]-data[posPlus1])) - data[posMinus1] + data[posPlus2]) * 0.5f) * posFract)
				+ ((2.0f*data[posPlus1]) + data[posMinus1] - (2.5f*data[pos]) - (data[posPlus2]*0.5f))) * 
				posFract + ((data[posPlus1] - data[posMinus1]) * 0.5f) ) * posFract) + data[pos];

#elif 1	// XXX also test using float variables of data[] rather than looking up with posMinus1, etc.
	float dataPosMinus1 = data[ (pos == 0) ? arraysize-1 : pos-1 ];
	float dataPosPlus1 = data[ (pos+1) % arraysize ];
	float dataPosPlus2 = data[ (pos+2) % arraysize ];

	float a = ( (3.0f*(data[pos]-dataPosPlus1)) - dataPosMinus1 + dataPosPlus2 ) * 0.5f;
	float b = (2.0f*dataPosPlus1) + dataPosMinus1 - (2.5f*data[pos]) - (dataPosPlus2*0.5f);
	float c = (dataPosPlus1 - dataPosMinus1) * 0.5f;

	return (( ((a*posFract)+b) * posFract + c ) * posFract) + data[pos];

#else
	long posMinus1 = (pos == 0) ? arraysize-1 : pos-1;
	long posPlus1 = (pos+1) % arraysize;
	long posPlus2 = (pos+2) % arraysize;

	float a = ( (3.0f*(data[pos]-data[posPlus1])) - data[posMinus1] + data[posPlus2] ) * 0.5f;
	float b = (2.0f*data[posPlus1]) + data[posMinus1] - (2.5f*data[pos]) - (data[posPlus2]*0.5f);
	float c = (data[posPlus1] - data[posMinus1]) * 0.5f;

	return (( ((a*posFract)+b) * posFract + c ) * posFract) + data[pos];
#endif
}

inline float interpolateLinear(float *data, double address, long arraysize)
{
	long pos = (long)address;
	float posFract = (float) (address - (double)pos);
	return (data[pos] * (1.0f-posFract)) + (data[(pos+1)%arraysize] * posFract);
}

inline float interpolateRandom(float randMin, float randMax)
{
	float randy = (float)rand() * ONE_DIV_RAND_MAX;
	return ((randMax-randMin) * randy) + randMin;
}

inline float interpolateLinear2values(float point1, float point2, double address)
{
	float posFract = (float) (address - (double)((long)address));
	return (point1 * (1.0f-posFract)) + (point2 * posFract);
}

/* return the parameter with larger magnitude */
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