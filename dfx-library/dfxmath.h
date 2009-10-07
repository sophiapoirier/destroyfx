/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2001-2009  Sophia Poirier

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
This is our math shit.
------------------------------------------------------------------------*/


#ifndef __DFX_MATH_H
#define __DFX_MATH_H

#include <math.h>
#include <stdlib.h>	// for RAND_MAX


// the Mac OS X system math.h up through Mac OS X 10.2 was missing the float variations of these functions
#if defined(MAC_OS_X_VERSION_MAX_ALLOWED) && (MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_3)
	#define sqrtf	(float)sqrt
	#define powf	(float)pow
	#define sinf	(float)sin
	#define cosf	(float)cos
	#define log10f	(float)log10
#endif



//-----------------------------------------------------------------------------
// constants and macros
//-----------------------------------------------------------------------------

const float kDFX_PI_f = 3.1415926535897932384626433832795f;	// XXX 4.0f * atanf(1.0f);
const double kDFX_PI_d = 3.1415926535897932384626433832795;	// XXX 4.0 * atan(1.0);
const float kDFX_LN2_f = 0.69314718055994530942f;
const double kDFX_LN2_d = 0.69314718055994530942;

// reduces wasteful casting and division
const float kDFX_OneDivRandMax = 1.0f / (float)RAND_MAX;
const double kDFX_OneDivRandMax_d = 1.0 / (double)RAND_MAX;

#ifndef DFX_UNDENORMALIZE
	#define DFX_UNDENORMALIZE(fval)   do { if (fabsf(fval) < 1.0e-15f) fval = 0.0f; } while (0)
//	#define DFX_UNDENORMALIZE(fval)	do { if ( ((*((unsigned int*)&fval)) & 0x7f800000) == 0 ) fval = 0.0f; } while (0)
//	#define DFX_IS_DENORMAL(fval)   ( (*((unsigned int*)&fval)) & 0x7f800000) == 0 )
#endif



//-----------------------------------------------------------------------------
// inline functions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
inline float DFX_Rand_f()
{
	return (float)rand() * kDFX_OneDivRandMax;
}
inline double DFX_Rand_d()
{
	return (double)rand() * kDFX_OneDivRandMax_d;
}

//-----------------------------------------------------------------------------
inline float DFX_ClipAudio(float inValue)
{
	if (inValue < -1.0f)
		return -1.0f;
	else if (inValue > 1.0f)
		return 1.0f;
	return inValue;
}

//-----------------------------------------------------------------------------
inline float DFX_Sign_f(float inValue)
{
	return (inValue < 0.0f) ? -1.0f : 1.0f;
}

//-----------------------------------------------------------------------------
inline float DFX_ClampDenormalValue(float inValue)
{
#ifndef TARGET_API_AUDIOUNIT	// the AU SDK handles denormals for us
	// clamp down any very small values (below -300 dB) to zero to hopefully avoid any denormal values
	const float verySmallButNotDenormalValue = 1.0e-15f;
	if (fabsf(inValue) < verySmallButNotDenormalValue)
		return 0.0f;
#endif
	return inValue;
}

//-----------------------------------------------------------------------------
inline double DFX_Linear2dB(double inLinearValue)
{
	return 20.0 * log10(inLinearValue);
}

//-----------------------------------------------------------------------------
inline double DFX_dB2Linear(double inDecibalValue)
{
	return pow(10.0, inDecibalValue/20.0);
}

//-----------------------------------------------------------------------------
inline float DFX_InterpolateHermite(float * inData, double inAddress, long inBufferSize)
{
	long pos = (long)inAddress;
	float posFract = (float) (inAddress - (double)pos);

#if 0	// XXX test performance using fewer variables/registers
	long posMinus1 = (pos == 0) ? inBufferSize-1 : pos-1;
	long posPlus1 = (pos+1) % inBufferSize;
	long posPlus2 = (pos+2) % inBufferSize;

	return (( (((((3.0f*(inData[pos]-inData[posPlus1])) - inData[posMinus1] + inData[posPlus2]) * 0.5f) * posFract)
				+ ((2.0f*inData[posPlus1]) + inData[posMinus1] - (2.5f*inData[pos]) - (inData[posPlus2]*0.5f))) * 
				posFract + ((inData[posPlus1] - inData[posMinus1]) * 0.5f) ) * posFract) + inData[pos];

#elif 1	// XXX also test using float variables of inData[] rather than looking up with posMinus1, etc.
	float dataPosMinus1 = inData[ (pos == 0) ? inBufferSize-1 : pos-1 ];
	float dataPosPlus1 = inData[ (pos+1) % inBufferSize ];
	float dataPosPlus2 = inData[ (pos+2) % inBufferSize ];

	float a = ( (3.0f*(inData[pos]-dataPosPlus1)) - dataPosMinus1 + dataPosPlus2 ) * 0.5f;
	float b = (2.0f*dataPosPlus1) + dataPosMinus1 - (2.5f*inData[pos]) - (dataPosPlus2*0.5f);
	float c = (dataPosPlus1 - dataPosMinus1) * 0.5f;

	return (( ((a*posFract)+b) * posFract + c ) * posFract) + inData[pos];

#else
	long posMinus1 = (pos == 0) ? inBufferSize-1 : pos-1;
	long posPlus1 = (pos+1) % inBufferSize;
	long posPlus2 = (pos+2) % inBufferSize;

	float a = ( (3.0f*(inData[pos]-inData[posPlus1])) - inData[posMinus1] + inData[posPlus2] ) * 0.5f;
	float b = (2.0f*inData[posPlus1]) + inData[posMinus1] - (2.5f*inData[pos]) - (inData[posPlus2]*0.5f);
	float c = (inData[posPlus1] - inData[posMinus1]) * 0.5f;

	return (( ((a*posFract)+b) * posFract + c ) * posFract) + inData[pos];
#endif
}

//-----------------------------------------------------------------------------
inline float DFX_InterpolateHermite_NoWrap(float * inData, double inAddress, long inBufferSize)
{
	long pos = (long)inAddress;
	float posFract = (float) (inAddress - (double)pos);

	float dataPosMinus1 = (pos == 0) ? 0.0f : inData[pos-1];
	float dataPosPlus1 = ((pos+1) == inBufferSize) ? 0.0f : inData[pos+1];
	float dataPosPlus2 = ((pos+2) >= inBufferSize) ? 0.0f : inData[pos+2];

	float a = ( (3.0f*(inData[pos]-dataPosPlus1)) - dataPosMinus1 + dataPosPlus2 ) * 0.5f;
	float b = (2.0f*dataPosPlus1) + dataPosMinus1 - (2.5f*inData[pos]) - (dataPosPlus2*0.5f);
	float c = (dataPosPlus1 - dataPosMinus1) * 0.5f;

	return (( ((a*posFract)+b) * posFract + c ) * posFract) + inData[pos];
}

//-----------------------------------------------------------------------------
inline float DFX_InterpolateLinear(float * inData, double inAddress, long inBufferSize)
{
	long pos = (long)inAddress;
	float posFract = (float) (inAddress - (double)pos);
	return (inData[pos] * (1.0f-posFract)) + (inData[(pos+1)%inBufferSize] * posFract);
}

//-----------------------------------------------------------------------------
inline float DFX_InterpolateRandom(float inMinValue, float inMaxValue)
{
	return ((inMaxValue-inMinValue) * DFX_Rand_f()) + inMinValue;
}

//-----------------------------------------------------------------------------
inline float DFX_InterpolateLinear_Values(float inValue1, float inValue2, double inAddress)
{
	float posFract = (float) (inAddress - (double)((long)inAddress));
	return (inValue1 * (1.0f-posFract)) + (inValue2 * posFract);
}

//-----------------------------------------------------------------------------
// return the parameter with larger magnitude
inline float DFX_MagnitudeMax(float inValue1, float inValue2)
{
	if ( fabsf(inValue1) > fabsf(inValue2) )
		return inValue1;
	else
		return inValue2;
}

//-----------------------------------------------------------------------------
// computes the principle branch of the Lambert W function
//    { LambertW(x) = W(x), where W(x) * exp(W(x)) = x }
inline double DFX_LambertW(double inValue)
{
	double x = fabs(inValue);
	if (x <= 500.0)
		return 0.665 * ( 1.0 + (0.0195 * log(x+1.0)) ) * log(x+1.0) + 0.04;
	else
		return log(x-4.0) - ( (1.0 - 1.0/log(x)) * log(log(x)) );
}

/*
//-----------------------------------------------------------------------------
// I found this somewhere on the internet
inline float DFX_AnotherSqrt(float inValue)
{
	float result = 1.0f;
	float store = 0.0f;
	while (store != result)
	{
		store = result;
		result = 0.5f * (result + (inValue / result));
	}
}
*/


#endif
