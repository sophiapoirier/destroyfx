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
#include <type_traits>

#include "dfx-base.h"
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

	kNumParameters
};

enum
{
	kMidiMode_Nudge,
	kMidiMode_Trigger,
	kNumMidiModes
};

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
