/*------------------ by Marc Poirier  ][  January 2001 -----------------*/

#ifndef __POLARIZER_H
#include "polarizer.hpp"
#endif

#if defined(TARGET_API_VST) && TARGET_PLUGIN_HAS_GUI
	#ifndef __POLARIZEREDITOR_H
	#include "polarizereditor.hpp"
	#endif
#endif


// these are macros that do boring entry point stuff for us
DFX_ENTRY(Polarizer);
DFX_CORE_ENTRY(PolarizerDSP);


//-----------------------------------------------------------------------------
// initializations & such
Polarizer::Polarizer(TARGET_API_BASE_INSTANCE_TYPE inInstance)
	: DfxPlugin(inInstance, NUM_PARAMETERS, 1)	// 3 parameters, 1 preset
{
	initparameter_i(kSkip, "leap", 1, 3, 1, 81, kDfxParamUnit_samples, kDfxParamCurve_pow);
	initparameter_f(kAmount, "polarize", 50.0f, 50.0f, 0.0f, 100.0f, kDfxParamUnit_percent);
	initparameter_b(kImplode, "implode", false, false);
	setparametercurvespec(kSkip, 1.5);

	setpresetname(0, "twicky");	// default preset name


	#ifdef TARGET_API_VST
		#if TARGET_PLUGIN_HAS_GUI
			editor = new PolarizerEditor(this);
		#endif
		DFX_INIT_CORE(PolarizerDSP);	// we need to manage DSP cores manually in VST
	#endif
}

//-------------------------------------------------------------------------
PolarizerDSP::PolarizerDSP(TARGET_API_CORE_INSTANCE_TYPE *inInstance)
	: DfxPluginCore(inInstance)
{
}

//-----------------------------------------------------------------------------------------
void PolarizerDSP::reset()
{
	unaffectedSamples = 0;	// the first sample is polarized
}

//-----------------------------------------------------------------------------------------
void PolarizerDSP::processparameters()
{
	leapSize = getparameter_i(kSkip);
	polarizedAmp = (0.5f - getparameter_scalar(kAmount)) * 2.0f;
	implode = getparameter_b(kImplode);
}

//-----------------------------------------------------------------------------------------
void PolarizerDSP::process(const float *in, float *out, unsigned long numSampleFrames, bool replacing)
{
	for (unsigned long samplecount=0; samplecount < numSampleFrames; samplecount++)
	{
		float amp = 1.0f;
		unaffectedSamples--;
		if (unaffectedSamples < 0)	// go to polarized when the leap is done
		{
			// figure out how long the unaffected period will last this time
			unaffectedSamples = leapSize;
			// this is the polarization scalar
			amp = polarizedAmp;
		}

		// if implode is off, just do the regular polarizing thing
		float outval = in[samplecount] * amp;
		if (implode)	// if it's on, then implode the audio signal
		{
			if (outval > 0.0f)	// invert the sample between 1 & 0
				outval = 1.0f - outval;
			else	// invert the sample between -1 & 0
				outval = -1.0f - outval;
		}
	#ifdef TARGET_API_VST
		if (!replacing)
			outval += out[samplecount];
	#endif
		out[samplecount] = outval;
	}
}
