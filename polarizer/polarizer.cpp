/*------------------ by Marc Poirier  ][  January 2001 -----------------*/

#include "polarizer.hpp"

#if defined(TARGET_API_VST) && TARGET_PLUGIN_HAS_GUI
	#include "polarizereditor.hpp"
#endif


// these are macros that do boring entry point stuff for us
DFX_ENTRY(Polarizer);
DFX_CORE_ENTRY(PolarizerDSP);

//-----------------------------------------------------------------------------
// initializations and such
Polarizer::Polarizer(TARGET_API_BASE_INSTANCE_TYPE inInstance)
	: DfxPlugin(inInstance, NUM_PARAMETERS, 1)	// 3 parameters, 1 preset
{
	initparameter_i(kSkip, "leap", 1, 3, 1, 81, kDfxParamUnit_samples, kDfxParamCurve_pow);
	initparameter_f(kAmount, "polarize", 50.0f, 50.0f, 0.0f, 100.0f, kDfxParamUnit_percent);
	initparameter_b(kImplode, "implode", false, false);
	setparametercurvespec(kSkip, 1.5);

	setpresetname(0, "twicky");	// default preset name

	#ifdef TARGET_API_VST
		DFX_INIT_CORE(PolarizerDSP);	// we need to manage DSP cores manually in VST
		#if TARGET_PLUGIN_HAS_GUI
			editor = new PolarizerEditor(this);
		#endif
	#endif
}

//-------------------------------------------------------------------------
PolarizerDSP::PolarizerDSP(DfxPlugin *inDfxPlugin)
	: DfxPluginCore(inDfxPlugin)
{
}

//-----------------------------------------------------------------------------------------
void PolarizerDSP::reset()
{
	unaffectedSamples = 0;	// the first sample is polarized
}

//-----------------------------------------------------------------------------------------
void PolarizerDSP::process(const float *in, float *out, unsigned long numSampleFrames, bool replacing)
{
	// fetch the current parameter values
	long leapSize = getparameter_i(kSkip);
	float polarizedAmp = (0.5f - getparameter_scalar(kAmount)) * 2.0f;
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
