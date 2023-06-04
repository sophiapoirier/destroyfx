/*------------------------------------------------------------------------
Copyright (C) 2001-2023  Sophia Poirier

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

#include "polarizer.h"

#include <algorithm>


// these are macros that do boring entry point stuff for us
DFX_EFFECT_ENTRY(Polarizer)
DFX_CORE_ENTRY(PolarizerDSP)

//-----------------------------------------------------------------------------------------
// initializations and such
Polarizer::Polarizer(TARGET_API_BASE_INSTANCE_TYPE inInstance)
:	DfxPlugin(inInstance, kNumParameters, 1)
{
	initparameter_i(kSkip, {"leap"}, 1, 3, 1, 81, DfxParam::Unit::Samples, DfxParam::Curve::Pow);
	initparameter_f(kAmount, {"polarization strength", "Amount", "Amnt"}, 50.0, 50.0, 0.0, 100.0, DfxParam::Unit::Percent);
	initparameter_b(kImplode, {"implode", "Implod", "mpld"}, false);
	setparametercurvespec(kSkip, 1.5);

	setpresetname(0, "twicky");  // default preset name
}

//-----------------------------------------------------------------------------------------
PolarizerDSP::PolarizerDSP(DfxPlugin& inDfxPlugin)
:	DfxPluginCore(inDfxPlugin)
{
	registerSmoothedAudioValue(mPolarizedAmp);
}

//-----------------------------------------------------------------------------------------
void PolarizerDSP::reset()
{
	mUnaffectedSamples = 0;  // the first sample is polarized
}

//-----------------------------------------------------------------------------------------
void PolarizerDSP::processparameters()
{
	if (auto const value = getparameterifchanged_scalar(kAmount))
	{
		mPolarizedAmp = (0.5f - static_cast<float>(*value)) * 2.0f;
	}
}

//-----------------------------------------------------------------------------------------
void PolarizerDSP::process(std::span<float const> inAudio, std::span<float> outAudio)
{
	// fetch the current parameter values
	auto const leapSize = static_cast<decltype(mUnaffectedSamples)>(getparameter_i(kSkip));
	auto const implode = getparameter_b(kImplode);

	// catch up if leap size decreased
	mUnaffectedSamples = std::min(mUnaffectedSamples, leapSize);

	std::ranges::transform(inAudio, outAudio.begin(), [this, leapSize, implode](auto const inputValue)
	{
		auto outputValue = inputValue;
		mUnaffectedSamples--;
		if (mUnaffectedSamples < 0)  // go to polarized when the leap is done
		{
			// figure out how long the unaffected period will last this time
			mUnaffectedSamples = leapSize;
			// this is the polarization scalar
			outputValue *= mPolarizedAmp.getValue();
		}

		// if it's on, then implode the audio signal
		if (implode)
		{
			if (outputValue > 0.0f)  // invert the sample between 1 and 0
			{
				outputValue = 1.0f - outputValue;
			}
			else  // invert the sample between -1 and 0
			{
				outputValue = -1.0f - outputValue;
			}
		}

		incrementSmoothedAudioValues();

		return outputValue;
	});
}
