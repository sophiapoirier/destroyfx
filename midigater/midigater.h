/*------------------------------------------------------------------------
Copyright (C) 2001-2020  Sophia Poirier

This file is part of MIDI Gater.

MIDI Gater is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

MIDI Gater is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with MIDI Gater.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#pragma once

#include "dfxplugin.h"
#include "dfxsmoothedvalue.h"


//----------------------------------------------------------------------------- 
// these are the plugin parameters:
enum
{
	kAttack,
	kRelease,
	kVelocityInfluence,
	kFloor,

	kNumParameters
};


//----------------------------------------------------------------------------- 
class MIDIGater final : public DfxPlugin
{
public:
	MIDIGater(TARGET_API_BASE_INSTANCE_TYPE inInstance);

	void reset() override;
	void processparameters() override;
	void processaudio(float const* const* inAudio, float* const* outAudio, unsigned long inNumFrames) override;

private:
	// these are the states of the unaffected audio input between notes
	enum class UnaffectedState
	{
		FadeIn,
		Flat,
		FadeOut
	};

	void processUnaffected(float const* const* inAudio, float* const* outAudio, 
						   unsigned long inNumFramesToProcess, unsigned long inOffsetFrames, unsigned long inNumChannels);

	// parameter values
	float mVelocityInfluence = 0.0f;
	dfx::SmoothedValue<float> mFloor;

	UnaffectedState mUnaffectedState {};
	long mUnaffectedFadeSamples = 0;
};
