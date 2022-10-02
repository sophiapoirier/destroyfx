/*------------------------------------------------------------------------
Copyright (C) 2001-2022  Tom Murphy 7 and Sophia Poirier

This file is part of Transverb.

Transverb is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Transverb is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Transverb.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#pragma once

#include <array>

#include "dfx-base.h"
#include "dfxpluginproperties.h"


namespace dfx::TV
{

//-----------------------------------------------------------------------------
// these are the plugin parameters:
enum : dfx::ParameterID
{
	kBsize,
	kSpeed1,
	kFeed1,
	kDist1,
	kSpeed2,
	kFeed2,
	kDist2,
	kDrymix,
	kMix1,
	kMix2,
	kQuality,
	kTomsound,
	kFreeze,
	kAttenuateFeedbackByMixLevel,

	kNumParameters
};


static constexpr size_t kNumDelays = 2;

static constexpr std::array<dfx::ParameterID, kNumDelays> kSpeedParameters { kSpeed1, kSpeed2 };
static constexpr std::array<dfx::ParameterID, kNumDelays> kFeedParameters { kFeed1, kFeed2 };
static constexpr std::array<dfx::ParameterID, kNumDelays> kDistParameters { kDist1, kDist2 };
static constexpr std::array<dfx::ParameterID, kNumDelays> kMixParameters { kMix1, kMix2 };

enum { kQualityMode_DirtFi, kQualityMode_HiFi, kQualityMode_UltraHiFi, kQualityMode_NumModes };

// this stuff is for the speed parameter adjustment mode switch on the GUI
enum : unsigned int { kSpeedMode_Fine, kSpeedMode_Semitone, kSpeedMode_Octave, kSpeedMode_NumModes };
static constexpr dfx::PropertyID kTransverbProperty_SpeedModeBase = dfx::kPluginProperty_EndOfList;


dfx::PropertyID speedModeIndexToPropertyID(size_t inIndex) noexcept;
size_t speedModePropertyIDToIndex(dfx::PropertyID inPropertyID) noexcept;
bool isSpeedModePropertyID(dfx::PropertyID inPropertyID) noexcept;

}  // namespace
