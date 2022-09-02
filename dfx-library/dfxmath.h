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

To contact the author, use the contact form at http://destroyfx.org/

Destroy FX is a sovereign entity comprised of Sophia Poirier and Tom Murphy 7.  
This is our math and numerics shit.
------------------------------------------------------------------------*/


#pragma once

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <limits>
#include <random>
#include <type_traits>


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
template <typename OutputT = size_t, typename InputT>
constexpr OutputT ToIndex(InputT inValue) noexcept
{
	static_assert(std::is_integral_v<InputT> && std::is_signed_v<InputT>);
	static_assert(std::is_integral_v<OutputT> && std::is_unsigned_v<OutputT>);
	return static_cast<OutputT>(std::max(inValue, InputT(0)));
}

//-----------------------------------------------------------------------------
template <typename OutputT = size_t, typename InputT>
constexpr OutputT RoundToIndex(InputT inValue)
{
	static_assert(std::is_floating_point_v<InputT>);
	return ToIndex<OutputT>(std::llround(inValue));
}

//-----------------------------------------------------------------------------
template <typename T>
constexpr auto ToSigned(T inValue) noexcept
{
	static_assert(std::is_integral_v<T>);
	static_assert(std::is_unsigned_v<T>);
	using DecayedT = std::decay_t<T>;
	using SignedT = std::make_signed_t<DecayedT>;
	assert(inValue <= static_cast<DecayedT>(std::numeric_limits<SignedT>::max()));
	return static_cast<SignedT>(inValue);
}

//-----------------------------------------------------------------------------
template <typename T>
constexpr auto ToUnsigned(T inValue) noexcept
{
	static_assert(std::is_integral_v<T>);
	static_assert(std::is_signed_v<T>);
	using DecayedT = std::decay_t<T>;
	assert(inValue >= DecayedT(0));
	return static_cast<std::make_unsigned_t<DecayedT>>(inValue);
}

//-----------------------------------------------------------------------------
// return the parameter with larger magnitude
template <typename T>
constexpr T MagnitudeMax(T inValue1, T inValue2)
{
	static_assert(std::is_floating_point_v<T>);
	return (std::fabs(inValue1) > std::fabs(inValue2)) ? inValue1 : inValue2;
}

//-----------------------------------------------------------------------------
//static inline void Undenormalize(float& ioValue) { if (std::fabs(ioValue) < 1.0e-15f) ioValue = 0.0f; }
//static inline void Undenormalize(float& ioValue) { if ((reinterpret_cast<unsigned int&>(ioValue) & 0x7f800000) == 0) ioValue = 0.0f; }

//-----------------------------------------------------------------------------
template <typename T>
constexpr T ClampDenormal(T inValue)
{
	static_assert(std::is_floating_point_v<T>);
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
template <typename T>
constexpr T Linear2dB(T inLinearValue)
{
	static_assert(std::is_floating_point_v<T>);
	return T(20) * std::log10(inLinearValue);
}

//-----------------------------------------------------------------------------
template <typename T>
constexpr T Db2Linear(T inDecibalValue)
{
	static_assert(std::is_floating_point_v<T>);
	return std::pow(T(10), inDecibalValue / T(20));
}

//-----------------------------------------------------------------------------
template <typename T>
constexpr T FrequencyScalarBySemitones(T inSemitones)
{
	static_assert(std::is_floating_point_v<T>);
	return std::exp2(inSemitones / T(12));
}

//-----------------------------------------------------------------------------
constexpr float InterpolateHermite(float const* inData, double inAddress, long inBufferSize)
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
constexpr float InterpolateHermite_NoWrap(float const* inData, double inAddress, long inBufferSize)
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
// computes the principle branch of the Lambert W function
// { LambertW(x) = W(x), where W(x) * exp(W(x)) = x }
static inline double LambertW(double inValue)
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
static inline size_t GetFrequencyBasedSmoothingStride(double inSamplerate)
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

namespace detail
{
template <typename T>
static constexpr void validateRandomValueType()
{
	static_assert(std::is_arithmetic_v<T>);
}

template <typename T>
static constexpr T getRandomDefaultMaximum()
{
	validateRandomValueType<T>();
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
template <typename T>
static constexpr auto getRandomDistribution(T inRangeMinimum = T(0), T inRangeMaximum = getRandomDefaultMaximum<T>())
{
	validateRandomValueType<T>();
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

	static constexpr uint32_t min() { return 0U; }
	static constexpr uint32_t max() { return std::numeric_limits<uint32_t>::max(); }
	explicit PCGRandomEngine(uint32_t s = 0x333777) : state(s + INCREMENT) {}
	void seed(uint32_t s) { state = s + INCREMENT; }
	uint32_t operator() ()
	{
		uint64_t x = state;
		const unsigned count = static_cast<unsigned>(x >> 59);
		state = x * MULTIPLIER + INCREMENT;
		x ^= x >> 18;
		return rotr32(static_cast<uint32_t>(x >> 27), count);
	}
	void discard(unsigned long long z)
	{
		// Just run operator() z times.
		while (z--) std::ignore = (*this)();
	}

private:
	static constexpr uint32_t rotr32(uint32_t x, unsigned r) {
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
	using result_type = EngineType::result_type;

	explicit RandomEngine(RandomSeed inSeedType)
	:	mEngine(getSeed(inSeedType))
	{
	}

	// inclusive range (closed interval)
	// allow dynamically using a new distribution range (creation is cheap)
	template <typename T>
	T next(T const& inRangeMinimum, T const& inRangeMaximum)
	{
		detail::validateRandomValueType<T>();
		return detail::getRandomDistribution<T>(inRangeMinimum, inRangeMaximum)(mEngine);
	}

	template <typename T>
	T next()
	{
		detail::validateRandomValueType<T>();
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
	static EngineType::result_type getSeed(RandomSeed inSeedType)
	{
		switch (inSeedType)
		{
			case RandomSeed::Static:
				return 1729;
			case RandomSeed::Monotonic:
				return static_cast<EngineType::result_type>(std::chrono::steady_clock::now().time_since_epoch().count());
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
template <typename T>
class RandomGenerator
{
public:
	// inclusive range (closed interval)
	explicit RandomGenerator(RandomSeed inSeedType, T inRangeMinimum = T(0), T inRangeMaximum = detail::getRandomDefaultMaximum<T>())
	:	mEngine(inSeedType),
		mDistribution(inRangeMinimum, inRangeMaximum)
	{
		detail::validateRandomValueType<T>();
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
