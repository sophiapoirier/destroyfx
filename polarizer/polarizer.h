/*------------------------------------------------------------------------
Copyright (C) 2001-2022  Sophia Poirier

This file is part of Polarizer.

Polarizer is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Polarizer is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Polarizer.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#pragma once

#include "dfxplugin.h"
#include "dfxsmoothedvalue.h"


// these are Polarizer's parameters
enum : dfx::ParameterID
{
	kSkip,
	kAmount,
	kImplode,

	kNumParameters
};


//-----------------------------------------------------------------------------
class PolarizerDSP final : public DfxPluginCore
{
public:
	explicit PolarizerDSP(DfxPlugin& inDfxPlugin);
	void process(std::span<float const> inAudio, std::span<float> outAudio) override;
	void reset() override;
	void processparameters() override;

private:
	long mUnaffectedSamples = 0;  // sample counter
	dfx::SmoothedValue<float> mPolarizedAmp;
};

//-----------------------------------------------------------------------------
class Polarizer final : public DfxPlugin
{
public:
	explicit Polarizer(TARGET_API_BASE_INSTANCE_TYPE inInstance);
};
