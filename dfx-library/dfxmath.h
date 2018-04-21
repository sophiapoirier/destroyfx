/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2001-2018  Sophia Poirier

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


#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdlib.h>  // for RAND_MAX
#include <type_traits>


namespace dfx::math
{


//-----------------------------------------------------------------------------
// constants
//-----------------------------------------------------------------------------

template <typename T, typename = std::enable_if_t<std::is_floating_point<T>::value>>
constexpr T kPi(M_PI);

template <typename T, typename = std::enable_if_t<std::is_floating_point<T>::value>>
constexpr T kLn2(M_LN2);



//-----------------------------------------------------------------------------
// inline functions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
template <typename T>
T Rand()
{
	static_assert(std::is_floating_point<T>::value);
	static constexpr T oneDivRandMax = T(1) / T(RAND_MAX);  // reduces wasteful casting and division
	return static_cast<T>(rand()) * oneDivRandMax;
}

//-----------------------------------------------------------------------------
template <typename OutputT = size_t, typename InputT>
OutputT RoundToIndex(InputT inValue)
{
	static_assert(std::is_floating_point<InputT>::value);
	static_assert(std::is_unsigned<OutputT>::value);
	return static_cast<OutputT>(std::max(std::lround(inValue), 0L));
}

//-----------------------------------------------------------------------------
// return the parameter with larger magnitude
template <typename T>
T MagnitudeMax(T inValue1, T inValue2)
{
	static_assert(std::is_floating_point<T>::value);
	return (std::fabs(inValue1) > std::fabs(inValue2)) ? inValue1 : inValue2;
}

//-----------------------------------------------------------------------------
//inline void Undenormalize(float& ioValue) { if (std::fabs(ioValue) < 1.0e-15f) ioValue = 0.0f; }
//inline void Undenormalize(float& ioValue) { if (((*reinterpret_cast<unsigned int*>(&ioValue)) & 0x7f800000) == 0) ioValue = 0.0f; }

//-----------------------------------------------------------------------------
template <typename T>
inline T ClampDenormalValue(T inValue)
{
	static_assert(std::is_floating_point<T>::value);
#ifndef TARGET_API_AUDIOUNIT  // the AU SDK handles denormals for us
	// clamp down any very small values (below -300 dB) to zero to hopefully avoid any denormal values
	static constexpr T verySmallButNotDenormalValue = std::numeric_limits<T>::min() * T(1.0e20);
	if (std::fabs(inValue) < verySmallButNotDenormalValue)
	{
		return T(0);
	}
#endif
	return inValue;
}

//-----------------------------------------------------------------------------
template <typename T>
T Linear2dB(T inLinearValue)
{
	static_assert(std::is_floating_point<T>::value);
	return T(20) * std::log10(inLinearValue);
}

//-----------------------------------------------------------------------------
template <typename T>
T Db2Linear(T inDecibalValue)
{
	static_assert(std::is_floating_point<T>::value);
	return std::pow(T(10), inDecibalValue / T(20));
}

//-----------------------------------------------------------------------------
template <typename T>
T FrequencyScalarBySemitones(T inSemitones)
{
	static_assert(std::is_floating_point<T>::value);
	return std::exp2(inSemitones / T(12));
}

//-----------------------------------------------------------------------------
inline float InterpolateHermite(float const* inData, double inAddress, long inBufferSize)
{
	auto const pos = static_cast<long>(inAddress);
	auto const posFract = static_cast<float>(inAddress - static_cast<double>(pos));

#if 0  // XXX test performance using fewer variables/registers
	long const posMinus1 = (pos == 0) ? (inBufferSize - 1) : (pos - 1);
	long const posPlus1 = (pos + 1) % inBufferSize;
	long const posPlus2 = (pos + 2) % inBufferSize;

	return (((((((3.0f * (inData[pos] - inData[posPlus1])) - inData[posMinus1] + inData[posPlus2]) * 0.5f) * posFract)
				+ ((2.0f * inData[posPlus1]) + inData[posMinus1] - (2.5f * inData[pos]) - (inData[posPlus2] * 0.5f))) * 
				posFract + ((inData[posPlus1] - inData[posMinus1]) * 0.5f)) * posFract) + inData[pos];

#elif 1  // XXX also test using float variables of inData[] rather than looking up with posMinus1, etc.
	auto const dataPosMinus1 = inData[(pos == 0) ? (inBufferSize - 1) : (pos - 1)];
	auto const dataPosPlus1 = inData[(pos + 1) % inBufferSize];
	auto const dataPosPlus2 = inData[(pos + 2) % inBufferSize];

	float const a = ((3.0f * (inData[pos] - dataPosPlus1)) - dataPosMinus1 + dataPosPlus2) * 0.5f;
	float const b = (2.0f * dataPosPlus1) + dataPosMinus1 - (2.5f * inData[pos]) - (dataPosPlus2 * 0.5f);
	float const c = (dataPosPlus1 - dataPosMinus1) * 0.5f;

	return ((((a * posFract) + b) * posFract + c) * posFract) + inData[pos];

#else
	long const posMinus1 = (pos == 0) ? (inBufferSize - 1) : (pos - 1);
	long const posPlus1 = (pos + 1) % inBufferSize;
	long const posPlus2 = (pos + 2) % inBufferSize;

	float const a = ((3.0f * (inData[pos] - inData[posPlus1])) - inData[posMinus1] + inData[posPlus2]) * 0.5f;
	float const b = (2.0f * inData[posPlus1]) + inData[posMinus1] - (2.5f * inData[pos]) - (inData[posPlus2] * 0.5f);
	float const c = (inData[posPlus1] - inData[posMinus1]) * 0.5f;

	return ((((a * posFract) + b) * posFract + c) * posFract) + inData[pos];
#endif
}

//-----------------------------------------------------------------------------
inline float InterpolateHermite_NoWrap(float* inData, double inAddress, long inBufferSize)
{
	auto const pos = static_cast<long>(inAddress);
	auto const posFract = static_cast<float>(inAddress - static_cast<double>(pos));

	float const dataPosMinus1 = (pos == 0) ? 0.0f : inData[pos - 1];
	float const dataPosPlus1 = ((pos + 1) == inBufferSize) ? 0.0f : inData[pos + 1];
	float const dataPosPlus2 = ((pos + 2) >= inBufferSize) ? 0.0f : inData[pos + 2];

	float const a = ((3.0f * (inData[pos] - dataPosPlus1)) - dataPosMinus1 + dataPosPlus2) * 0.5f;
	float const b = (2.0f * dataPosPlus1) + dataPosMinus1 - (2.5f * inData[pos]) - (dataPosPlus2 * 0.5f);
	float const c = (dataPosPlus1 - dataPosMinus1) * 0.5f;

	return ((((a * posFract) + b) * posFract + c) * posFract) + inData[pos];
}

//-----------------------------------------------------------------------------
inline float InterpolateLinear(float* inData, double inAddress, long inBufferSize)
{
	auto const pos = static_cast<long>(inAddress);
	auto const posFract = static_cast<float>(inAddress - static_cast<double>(pos));
	return (inData[pos] * (1.0f - posFract)) + (inData[(pos + 1) % inBufferSize] * posFract);
}

//-----------------------------------------------------------------------------
inline float InterpolateLinear(float inValue1, float inValue2, double inAddress)
{
	auto const posFract = static_cast<float>(inAddress - static_cast<double>(static_cast<long>(inAddress)));
	return (inValue1 * (1.0f - posFract)) + (inValue2 * posFract);
}

//-----------------------------------------------------------------------------
inline float InterpolateRandom(float inMinValue, float inMaxValue)
{
	return ((inMaxValue - inMinValue) * Rand<float>()) + inMinValue;
}

//-----------------------------------------------------------------------------
// computes the principle branch of the Lambert W function
// { LambertW(x) = W(x), where W(x) * exp(W(x)) = x }
inline double LambertW(double inValue)
{
	auto const x = std::fabs(inValue);
	if (x <= 500.0)
	{
		return 0.665 * (1.0 + (0.0195 * std::log(x + 1.0))) * std::log(x + 1.0) + 0.04;
	}
	else
	{
		return std::log(x - 4.0) - ((1.0 - 1.0 / std::log(x)) * log(std::log(x)));
	}
}


}  // namespace
