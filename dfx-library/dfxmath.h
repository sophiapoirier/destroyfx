/*------------------------------------------------------------------------
Destroy FX is a sovereign entity comprised of Marc Poirier & Tom Murphy 7.  
This is our math shit.
------------------------------------------------------------------------*/


#ifndef __DFXMATH_H
#define __DFXMATH_H

#include <math.h>
#include <stdlib.h>	// for RAND_MAX


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

	long posMinus1 = (pos == 0) ? arraysize-1 : pos-1;
	long posPlus1 = (pos+1) % arraysize;
	long posPlus2 = (pos+2) % arraysize;

	float a = ( (3.0f*(data[pos]-data[posPlus1])) - data[posMinus1] + data[posPlus2] ) * 0.5f;
	float b = (2.0f*data[posPlus1]) + data[posMinus1] - (2.5f*data[pos]) - (data[posPlus2]*0.5f);
	float c = (data[posPlus1] - data[posMinus1]) * 0.5f;

	return (( ((a*posFract)+b) * posFract + c ) * posFract) + data[pos];
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