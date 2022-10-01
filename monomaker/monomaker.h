/*------------------------------------------------------------------------
Copyright (C) 2001-2022  Sophia Poirier

This file is part of Monomaker.

Monomaker is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Monomaker is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Monomaker.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#pragma once

#include <vector>

#include "dfxplugin.h"
#include "dfxsmoothedvalue.h"


//-----------------------------------------------------------------------------
// these are the plugin parameters:
enum : dfx::ParameterID
{
	kInputSelection,
	kMonomerge,
	kMonomergeMode,
	kPan,
	kPanMode,
	kPanLaw,
	kPhaseInvert_LeftChannel,
	kPhaseInvert_RightChannel,

	kNumParameters
};

enum
{
	kInputSelection_Stereo,
	kInputSelection_Swap,
	kInputSelection_Left,
	kInputSelection_Right,
	kNumInputSelections
};

enum
{
	kMonomergeMode_Linear,
	kMonomergeMode_EqualPower,
	kNumMonomergeModes
};

enum
{
	kPanMode_Recenter,
	kPanMode_Balance,
	kNumPanModes
};

enum
{
	kPanLaw_0dB,
	kPanLaw_Minus3dB,
	kPanLaw_Minus6dB,
	kPanLaw_SineCosine,
	kNumPanLaws
};


//-----------------------------------------------------------------------------
class Monomaker final : public DfxPlugin
{
public:
	explicit Monomaker(TARGET_API_BASE_INSTANCE_TYPE inInstance);

	long initialize() override;
	void cleanup() override;

	void processparameters() override;
	void processaudio(float const* const* inAudio, float* const* outAudio, size_t inNumFrames) override;

private:
	dfx::SmoothedValue<float> mInputSelection_left2left, mInputSelection_left2right, mInputSelection_right2left, mInputSelection_right2right;
	dfx::SmoothedValue<float> mMonomerge_main, mMonomerge_other;
	dfx::SmoothedValue<float> mPan_left1, mPan_left2, mPan_right1, mPan_right2;

	std::vector<float> mAsymmetricalInputAudioBuffer;
};
