/*------------------------------------------------------------------------
Copyright (C) 2001-2022  Sophia Poirier and Tom Murphy VII

This file is part of Buffer Override.

Buffer Override is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Buffer Override is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Buffer Override.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/


#pragma once

#include <atomic>
#include <cassert>
#include <cmath>
#include <concepts>
#include <type_traits>

#include "dfx-base.h"
#include "dfxmath.h"
#include "dfxmisc.h"
#include "dfxpluginproperties.h"

//-----------------------------------------------------------------------------
// these are the plugin parameters:
enum : dfx::ParameterID
{
	kDivisor,
	kBufferSize_MS,
	kBufferSize_Sync,
	kBufferTempoSync,
	kBufferInterrupt,

	kDivisorLFORate_Hz,
	kDivisorLFORate_Sync,
	kDivisorLFODepth,
	kDivisorLFOShape,
	kDivisorLFOTempoSync,
	kBufferLFORate_Hz,
	kBufferLFORate_Sync,
	kBufferLFODepth,
	kBufferLFOShape,
	kBufferLFOTempoSync,

	kSmooth,
	kDryWetMix,

	kPitchBendRange,
	kMidiMode,

	kTempo,
	kTempoAuto,

	kMinibufferPortion,
	kMinibufferPortionRandomMin,

	kDecayDepth,
	kDecayMode,
	kDecayShape,

	kNumParameters
};

enum
{
	kMidiMode_Nudge,
	kMidiMode_Trigger,
	kNumMidiModes
};

enum
{
	kDecayMode_Gain,
	kDecayMode_Lowpass,
	kDecayMode_Highpass,
	kDecayMode_LP_HP_Alternating,
	kDecayModeCount
};

enum
{
	kDecayShape_Ramp,
	kDecayShape_Oscillate,
	kDecayShape_Random,
	kDecayShapeCount
};
using DecayShape = int;

enum : dfx::PropertyID
{
	kBOProperty_LastViewDataTimestamp = dfx::kPluginProperty_EndOfList,
	kBOProperty_ViewData
};

// Current effective buffer/divisor/etc. after LFOs. Used for
// the visualization.
struct BufferOverrideViewData
{
	struct Segment
	{
		float mForcedBufferSeconds {};
		float mMinibufferSeconds {};
	} mPreLFO, mPostLFO;
};

static_assert(dfx::IsTriviallySerializable<BufferOverrideViewData>);

namespace detail
{
using UnifiedT = std::atomic<BufferOverrideViewData>;

// where 16-byte lock-free atomics are not supported, we slice it into halves (Clang can but GCC cannot)
struct CompositeT
{
	using SegmentT = std::atomic<BufferOverrideViewData::Segment>;
	SegmentT mPreLFO, mPostLFO;
	static_assert(SegmentT::is_always_lock_free);

	void store(BufferOverrideViewData value, std::memory_order order) noexcept
	{
		mPreLFO.store(value.mPreLFO, order);
		mPostLFO.store(value.mPostLFO, order);
	}
	BufferOverrideViewData load(std::memory_order order) const noexcept
	{
		return {.mPreLFO = mPreLFO.load(order), .mPostLFO = mPostLFO.load(order)};
	}
};
}  // namespace detail

using AtomicBufferOverrideViewData = std::conditional_t<detail::UnifiedT::is_always_lock_free, detail::UnifiedT, detail::CompositeT>;

template <std::floating_point T>
T GetBufferDecay(T normalizedPosition, T depth, DecayShape shape, dfx::math::RandomEngine& randomEngine)
{
	constexpr T maxValue = 1;
	assert(normalizedPosition >= T(0));
	assert(normalizedPosition <= maxValue);
	auto const negativeDepth = std::signbit(depth);
	switch (shape)
	{
		case kDecayShape_Ramp:
			return std::lerp(maxValue - std::abs(depth), maxValue,
							 negativeDepth ? normalizedPosition : (maxValue - normalizedPosition));
		case kDecayShape_Oscillate:
		{
			auto const oscillatingPosition = T(1) - std::fabs(T(1) - (normalizedPosition * T(2)));
			return GetBufferDecay(oscillatingPosition, depth, kDecayShape_Ramp, randomEngine);
		}
		case kDecayShape_Random:
			// always maintain full level for the first minibuffer iteration
			// (like accent on the downbeat) when depth is nonnegative
			if (!negativeDepth && (normalizedPosition <= T(0)))
			{
				return maxValue;
			}
			return randomEngine.next(maxValue - std::abs(depth), maxValue);
		default:
			assert(false);
			return maxValue;
	}
}
