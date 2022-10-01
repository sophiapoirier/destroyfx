/*------------------------------------------------------------------------
Copyright (C) 2001-2022  Sophia Poirier

This file is part of MIDI Gater.

MIDI Gater is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

MIDI Gater is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with MIDI Gater.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#pragma once

#include <array>
#include <vector>

#include "dfxplugin.h"
#include "dfxsmoothedvalue.h"
#include "iirfilter.h"


//-----------------------------------------------------------------------------
// these are the plugin parameters:
enum : dfx::ParameterID
{
	kAttack,
	kRelease,
	kVelocityInfluence,
	kFloor,
	kGateMode,

	kNumParameters
};


//-----------------------------------------------------------------------------
class MIDIGater final : public DfxPlugin
{
public:
	enum
	{
		kGateMode_Amplitude,
		kGateMode_Lowpass,
		kNumGateModes
	};

	explicit MIDIGater(TARGET_API_BASE_INSTANCE_TYPE inInstance);

	long initialize() override;
	void cleanup() override;
	void reset() override;
	void processparameters() override;
	void processaudio(float const* const* inAudio, float* const* outAudio, size_t inNumFrames) override;

private:
	void resetFilters();

	// parameter values
	float mVelocityInfluence = 0.0f;
	dfx::SmoothedValue<float> mFloor;
	long mGateMode {};

	std::array<std::vector<dfx::IIRFilter>, DfxMidi::kNumNotes> mLowpassGateFilters;
};
