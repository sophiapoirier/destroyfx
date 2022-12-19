/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2001-2022  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 2 of the License, or 
(at your option) any later version.

Destroy FX Library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Destroy FX Library.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org

Destroy FX is a sovereign entity comprised of Sophia Poirier and Tom Murphy 7.  
This is our math and numerics shit.
------------------------------------------------------------------------*/


#pragma once

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <concepts>
#include <limits>
#include <random>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>


namespace dfx::math
{


//-----------------------------------------------------------------------------
// constants
//-----------------------------------------------------------------------------

// the AU SDK handles denormals for us, and ARM processors don't have denormal performance issues
constexpr bool kDenormalProblem = 
#if defined(TARGET_API_AUDIOUNIT) || defined(__arm__) || defined(__arm64__)
false;
#else
true;
#endif



//-----------------------------------------------------------------------------
// functions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
template <std::unsigned_integral OutputT = size_t, std::signed_integral InputT>
constexpr OutputT ToIndex(InputT inValue) noexcept
{
	return static_cast<OutputT>(std::max(inValue, InputT(0)));
}

//-----------------------------------------------------------------------------
template <std::unsigned_integral OutputT = size_t>
constexpr OutputT RoundToIndex(std::floating_point auto inValue)
{
	return ToIndex<OutputT>(std::llround(inValue));
}

//-----------------------------------------------------------------------------
// fills the gap of a 32-bit int-returning flavor of std::lround or llround
constexpr int IRound(std::floating_point auto inValue)
{
	auto const result = std::llround(inValue);
	using ResultT = int;
	assert(result >= static_cast<decltype(result)>(std::numeric_limits<ResultT>::min()));
	assert(result <= static_cast<decltype(result)>(std::numeric_limits<ResultT>::max()));
	return static_cast<ResultT>(result);
}

//-----------------------------------------------------------------------------
template <std::unsigned_integral T>
constexpr auto ToSigned(T inValue) noexcept
{
	using SignedT = std::make_signed_t<std::decay_t<T>>;
	assert(inValue <= static_cast<T>(std::numeric_limits<SignedT>::max()));
	return static_cast<SignedT>(inValue);
}

//-----------------------------------------------------------------------------
template <std::signed_integral T>
constexpr auto ToUnsigned(T inValue) noexcept
{
	assert(inValue >= T(0));
	return static_cast<std::make_unsigned_t<std::decay_t<T>>>(inValue);
}

//-----------------------------------------------------------------------------
constexpr bool IsZero(std::floating_point auto inValue) noexcept
{
	return (std::fpclassify(inValue) == FP_ZERO);
}

//-----------------------------------------------------------------------------
// return the parameter with larger magnitude
template <std::floating_point T>
constexpr T MagnitudeMax(T inValue1, T inValue2)
{
	return (std::fabs(inValue1) > std::fabs(inValue2)) ? inValue1 : inValue2;
}

//-----------------------------------------------------------------------------
//constexpr void Undenormalize(float& ioValue) { if (std::fabs(ioValue) < 1.0e-15f) ioValue = 0.f; }
//constexpr void Undenormalize(float& ioValue) { if ((std::bit_cast<unsigned int>(ioValue) & 0x7f800000) == 0) ioValue = 0.f; }

//-----------------------------------------------------------------------------
template <std::floating_point T>
constexpr T ClampDenormal(T inValue)
{
	if constexpr (kDenormalProblem)
	{
		// clamp down any very small values (below -300 dB) to zero to hopefully avoid any denormal values
		constexpr T verySmallButNotDenormalValue = std::numeric_limits<T>::min() * T(1.0e20);
		if (std::fabs(inValue) < verySmallButNotDenormalValue)
		{
			return T(0);
		}
	}
	return inValue;
}

//-----------------------------------------------------------------------------
template <std::floating_point T>
constexpr T Linear2dB(T inLinearValue)
{
	return T(20) * std::log10(inLinearValue);
}

//-----------------------------------------------------------------------------
template <std::floating_point T>
constexpr T Db2Linear(T inDecibalValue)
{
	return std::pow(T(10), inDecibalValue / T(20));
}

//-----------------------------------------------------------------------------
template <std::floating_point T>
constexpr T FrequencyScalarBySemitones(T inSemitones)
{
	return std::exp2(inSemitones / T(12));
}

//-----------------------------------------------------------------------------
// orders of magnitude faster performance than std::modf
// leaving the integral template argument unspecified means that you only wish to retrieve the fractional component
template <typename OptionalIntegralT = void, std::floating_point FloatingT>
requires std::is_integral_v<OptionalIntegralT> || std::is_void_v<OptionalIntegralT>
constexpr auto ModF(FloatingT inValue)
{
	constexpr bool integralTypeSpecified = !std::is_void_v<OptionalIntegralT>;
	using IntegralT = std::conditional_t<integralTypeSpecified, OptionalIntegralT, long>;
	if constexpr (std::is_unsigned_v<IntegralT>)
	{
		assert(inValue >= FloatingT(0));
	}
	auto const integralValue = static_cast<IntegralT>(inValue);
	auto const fractionalValue = inValue - static_cast<FloatingT>(integralValue);
	if constexpr (integralTypeSpecified)
	{
		return std::make_pair(fractionalValue, integralValue);
	}
	else
	{
		return fractionalValue;
	}
}

//-----------------------------------------------------------------------------
constexpr float InterpolateHermite(float inPrecedingValue, float inCurrentValue, float inNextValue, float inNextNextValue, float inPosition)
{
#if 0
	float const precedingMinusCurrent = inPrecedingValue - inCurrentValue;
	//float const a = ((3.f * (inCurrentValue - inNextValue)) - inPrecedingValue + inNextNextValue) * 0.5f;
	//float const b = (2.f * inNextValue) + inPrecedingValue - (2.5f * inCurrentValue) - (0.5f * inNextNextValue);
	float const a = (inCurrentValue - inNextValue) + (0.5f * (inNextNextValue - precedingMinusCurrent - inNextValue));
	float const c = (inNextValue - inPrecedingValue) * 0.5f;
	float const b = precedingMinusCurrent + c - a;
	return (((((a * inPosition) + b) * inPosition) + c) * inPosition) + inCurrentValue;
#elif 1
	// optimization by Laurent de Soras https://www.musicdsp.org/en/latest/Other/93-hermite-interpollation.html
	float const c = (inNextValue - inPrecedingValue) * 0.5f;
	float const currentMinusNext = inCurrentValue - inNextValue;
	float const w = c + currentMinusNext;
	float const a = w + currentMinusNext + ((inNextNextValue - inCurrentValue) * 0.5f);
	float const b_neg = w + a;
	return (((((a * inPosition) - b_neg) * inPosition) + c) * inPosition) + inCurrentValue;
#endif
}

//-----------------------------------------------------------------------------
constexpr float InterpolateHermite(std::span<float const> inData, double inAddress)
{
	assert(inAddress >= 0.);
	assert(!inData.empty());

	auto const [posFract, pos] = ModF<size_t>(inAddress);
	assert(pos < inData.size());

	size_t const posMinus1 = (pos == 0) ? (inData.size() - 1) : (pos - 1);
	size_t const posPlus1 = (pos + 1) % inData.size();
	size_t const posPlus2 = (pos + 2) % inData.size();

	return InterpolateHermite(inData[posMinus1], inData[pos], inData[posPlus1], inData[posPlus2], static_cast<float>(posFract));
}

//-----------------------------------------------------------------------------
constexpr float InterpolateHermite_NoWrap(std::span<float const> inData, double inAddress)
{
	assert(inAddress >= 0.);
	assert(!inData.empty());

	auto const [posFract, pos] = ModF<size_t>(inAddress);
	assert(pos < inData.size());

	float const dataPosMinus1 = (pos == 0) ? 0.f : inData[pos - 1];
	float const dataPosPlus1 = ((pos + 1) == inData.size()) ? 0.f : inData[pos + 1];
	float const dataPosPlus2 = ((pos + 2) >= inData.size()) ? 0.f : inData[pos + 2];

	return InterpolateHermite(dataPosMinus1, inData[pos], dataPosPlus1, dataPosPlus2, static_cast<float>(posFract));
}

//-----------------------------------------------------------------------------
// computes the principle branch of the Lambert W function
// { LambertW(x) = W(x), where W(x) * exp(W(x)) = x }
constexpr double LambertW(double inValue)
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

//-----------------------------------------------------------------------------
// provides a good enough parameter smoothing update sample interval for frequency-based parameters;
// this is targeting an update granularity of every 4 sample frames at a 44.1 kHz sample rate
constexpr size_t GetFrequencyBasedSmoothingStride(double inSamplerate)
{
	assert(inSamplerate > 0.);
	// TODO C++23: integer literal suffix UZ
	return std::max(static_cast<size_t>(inSamplerate) / size_t(11025), size_t(1));
}



//-----------------------------------------------------------------------------
// classes
//-----------------------------------------------------------------------------

enum class RandomSeed
{
	Static,
	Monotonic,
	Entropic  // unbounded operation (not realtime-safe)
};

template <typename T>
concept Randomizable = std::is_arithmetic_v<T>;

namespace detail
{
template <Randomizable T>
constexpr T getRandomDefaultMaximum()
{
	if constexpr (std::is_floating_point_v<T>)
	{
		return T(1);
	}
	else
	{
		return std::numeric_limits<T>::max();
	}
}

// allows to select between types at compile-time
template <Randomizable T>
constexpr auto getRandomDistribution(T inRangeMinimum = T(0), T inRangeMaximum = getRandomDefaultMaximum<T>())
{
	assert(inRangeMinimum <= inRangeMaximum);
	if constexpr (std::is_floating_point_v<T>)
	{
		return std::uniform_real_distribution<T>(inRangeMinimum, inRangeMaximum);
	}
	else if constexpr (std::is_same_v<std::decay_t<T>, bool>)
	{
		return std::uniform_int_distribution<uint8_t>(inRangeMinimum ? 1 : 0, inRangeMaximum ? 1 : 0);
	}
	else
	{
		return std::uniform_int_distribution<T>(inRangeMinimum, inRangeMaximum);
	}
}

// This is PCG-XSH-RR as a C++11 random number engine with 32 bit output
// and 64-bit state.
// https://en.wikipedia.org/wiki/Permuted_congruential_generator
struct PCGRandomEngine
{
	using result_type = uint32_t;

	static constexpr result_type min() { return 0U; }
	static constexpr result_type max() { return std::numeric_limits<result_type>::max(); }
	explicit PCGRandomEngine(result_type s = 0x333777) : state(s + INCREMENT) {}
	void seed(result_type s) { state = s + INCREMENT; }
	result_type operator() ()
	{
		uint64_t x = state;
		const unsigned count = static_cast<unsigned>(x >> 59);
		state = x * MULTIPLIER + INCREMENT;
		x ^= x >> 18;
		return rotr32(static_cast<result_type>(x >> 27), count);
	}
	void discard(unsigned long long z)
	{
		// Just run operator() z times.
		while (z--) { std::ignore = (*this)(); }
	}

private:
	static constexpr result_type rotr32(result_type x, unsigned r)
	{
		return x >> r | x << (-r & 31);
	}
	uint64_t state = 0U;
	static constexpr uint64_t MULTIPLIER = 6364136223846793005U;
	static constexpr uint64_t INCREMENT = 1442695040888963407U;

	// TODO: Technically should have overloads << >> == !=, but
	// are they really useful?
};

}

//-----------------------------------------------------------------------------
class RandomEngine
{
private:
	using EngineType = detail::PCGRandomEngine;

public:
	// XXX since next() is templated, it's a bit suspicious
	// that this is a specific type. But if any choice makes
	// sense, it's the underlying type of the engine....
	// reason: I think that it is the job of the random
	// number distribution to map the engine's raw numerical
	// output type to next's template type.
	// that said: How can a 64-bit integral distribution do
	// a good job fed with 32-bit engine values?
	using result_type = EngineType::result_type;

	explicit RandomEngine(RandomSeed inSeedType)
	:	mEngine(getSeed(inSeedType))
	{
	}

	// inclusive range (closed interval)
	// allow dynamically using a new distribution range (creation is cheap)
	template <Randomizable T>
	T next(T const& inRangeMinimum, T const& inRangeMaximum)
	{
		return detail::getRandomDistribution<T>(inRangeMinimum, inRangeMaximum)(mEngine);
	}

	template <Randomizable T>
	T next()
	{
		return detail::getRandomDistribution<T>()(mEngine);
	}

	// minimum STL-required random engine interface
	auto operator()()
	{
		return mEngine();
	}
	static constexpr auto min()
	{
		return EngineType::min();
	}
	static constexpr auto max()
	{
		return EngineType::max();
	}

private:
	static result_type getSeed(RandomSeed inSeedType)
	{
		switch (inSeedType)
		{
			case RandomSeed::Static:
				return 1729;
			case RandomSeed::Monotonic:
				return static_cast<result_type>(std::chrono::steady_clock::now().time_since_epoch().count());
			case RandomSeed::Entropic:
				return std::random_device()();
			default:
				assert(false);
				return 0;
		}
	}

	EngineType mEngine;
};

//-----------------------------------------------------------------------------
template <Randomizable T>
class RandomGenerator
{
public:
	// inclusive range (closed interval)
	explicit RandomGenerator(RandomSeed inSeedType, T inRangeMinimum = T(0), T inRangeMaximum = detail::getRandomDefaultMaximum<T>())
	:	mEngine(inSeedType),
		mDistribution(inRangeMinimum, inRangeMaximum)
	{
	}

	T next()
	{
		return mDistribution(mEngine);
	}

private:
	RandomEngine mEngine;
	decltype(detail::getRandomDistribution<T>()) mDistribution;
};


}  // namespace
