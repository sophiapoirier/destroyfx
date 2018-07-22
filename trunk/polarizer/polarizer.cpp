/*------------------------------------------------------------------------
Copyright (C) 2001-2018  Sophia Poirier

This file is part of Polarizer.

Polarizer is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Polarizer is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Polarizer.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "polarizer.h"


// these are macros that do boring entry point stuff for us
DFX_EFFECT_ENTRY(Polarizer)
DFX_CORE_ENTRY(PolarizerDSP)

//-----------------------------------------------------------------------------------------
// initializations and such
Polarizer::Polarizer(TARGET_API_BASE_INSTANCE_TYPE inInstance)
:	DfxPlugin(inInstance, kNumParameters, 1)
{
	initparameter_i(kSkip, "leap", 1, 3, 1, 81, DfxParam::Unit::Samples, DfxParam::Curve::Pow);
	initparameter_f(kAmount, "polarization strength", 50.0, 50.0, 0.0, 100.0, DfxParam::Unit::Percent);
	initparameter_b(kImplode, "implode", false, false);
	setparametercurvespec(kSkip, 1.5);

	setpresetname(0, "twicky");  // default preset name

	DFX_INIT_CORE(PolarizerDSP);
}

//-----------------------------------------------------------------------------------------
PolarizerDSP::PolarizerDSP(DfxPlugin* inDfxPlugin)
:	DfxPluginCore(inDfxPlugin)
{
}

//-----------------------------------------------------------------------------------------
void PolarizerDSP::reset()
{
	mUnaffectedSamples = 0;  // the first sample is polarized
}

//-----------------------------------------------------------------------------------------
void PolarizerDSP::process(float const* inAudio, float* outAudio, unsigned long numSampleFrames, bool replacing)
{
	// fetch the current parameter values
	auto const leapSize = getparameter_i(kSkip);
	float const polarizedAmp = (0.5f - static_cast<float>(getparameter_scalar(kAmount))) * 2.0f;
	auto const implode = getparameter_b(kImplode);

	for (unsigned long sampleCount = 0; sampleCount < numSampleFrames; sampleCount++)
	{
		auto outputValue = inAudio[sampleCount];
		mUnaffectedSamples--;
		if (mUnaffectedSamples < 0)  // go to polarized when the leap is done
		{
			// figure out how long the unaffected period will last this time
			mUnaffectedSamples = leapSize;
			// this is the polarization scalar
			outputValue *= polarizedAmp;
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

	#ifdef TARGET_API_VST
		if (!replacing)
		{
			outputValue += outAudio[sampleCount];
		}
	#endif
		outAudio[sampleCount] = outputValue;
	}
}
