/*------------------------------------------------------------------------
Copyright (C) 2001-2009  Sophia Poirier

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
	: DfxPlugin(inInstance, kNumParameters, 1)	// 3 parameters, 1 preset
{
	initparameter_i(kSkip, "leap", 1, 3, 1, 81, kDfxParamUnit_samples, kDfxParamCurve_pow);
	initparameter_f(kAmount, "polarization strength", 50.0, 50.0, 0.0, 100.0, kDfxParamUnit_percent);
	initparameter_b(kImplode, "implode", false, false);
	setparametercurvespec(kSkip, 1.5);

	setpresetname(0, "twicky");	// default preset name

	DFX_INIT_CORE(PolarizerDSP);
}

//-----------------------------------------------------------------------------------------
PolarizerDSP::PolarizerDSP(DfxPlugin * inDfxPlugin)
	: DfxPluginCore(inDfxPlugin)
{
}

//-----------------------------------------------------------------------------------------
void PolarizerDSP::reset()
{
	unaffectedSamples = 0;	// the first sample is polarized
}

//-----------------------------------------------------------------------------------------
void PolarizerDSP::process(const float * in, float * out, unsigned long numSampleFrames, bool replacing)
{
	// fetch the current parameter values
	long leapSize = getparameter_i(kSkip);
	float polarizedAmp = (0.5 - getparameter_scalar(kAmount)) * 2.0;
	bool implode = getparameter_b(kImplode);

	for (unsigned long samplecount=0; samplecount < numSampleFrames; samplecount++)
	{
		float outval = in[samplecount];
		unaffectedSamples--;
		if (unaffectedSamples < 0)	// go to polarized when the leap is done
		{
			// figure out how long the unaffected period will last this time
			unaffectedSamples = leapSize;
			// this is the polarization scalar
			outval *= polarizedAmp;
		}

		// if it's on, then implode the audio signal
		if (implode)
		{
			if (outval > 0.0f)	// invert the sample between 1 and 0
				outval = 1.0f - outval;
			else	// invert the sample between -1 and 0
				outval = -1.0f - outval;
		}

	#ifdef TARGET_API_VST
		if (!replacing)
			outval += out[samplecount];
	#endif
		out[samplecount] = outval;
	}
}
