/*--------------- by Marc Poirier  ][  March 2001 + October 2002 ---------------*/

#ifndef __MONOMAKER_H
#include "monomaker.hpp"
#endif

#if TARGET_API_VST && TARGET_PLUGIN_HAS_GUI
	#ifndef __MONOMAKEREDITOR_H
	#include "monomakereditor.hpp"
	#endif
#endif


// this macro does boring entry point stuff for us
DFX_ENTRY(Monomaker);

//-----------------------------------------------------------------------------
// initializations & such
Monomaker::Monomaker(TARGET_API_BASE_INSTANCE_TYPE inInstance)
	: DfxPlugin(inInstance, NUM_PARAMETERS, 1)	// 2 parameters, 1 preset
{
	// initialize the parameters
	initparameter_f(kMonomerge, "monomix", 0.0f, 100.0f, 0.0f, 100.0f, kDfxParamCurve_linear, kDfxParamUnit_percent);
	initparameter_indexed(kMonomergeMode, "monomix mode", kMonomergeMode_equalpower, kMonomergeMode_linear, kNumMonomergeModes);
	initparameter_f(kPan, "pan", 0.0f, 0.0f, -1.0f, 1.0f, kDfxParamCurve_linear, kDfxParamUnit_pan);
	initparameter_indexed(kPanMode, "pan mode", kPanMode_recenter, kPanMode_recenter, kNumPanModes);

	// set the parameter value display strings
	setparametervaluestring(kMonomergeMode, kMonomergeMode_linear, "linear");
	setparametervaluestring(kMonomergeMode, kMonomergeMode_equalpower, "equal power");
	setparametervaluestring(kPanMode, kPanMode_recenter, "recenter");
	setparametervaluestring(kPanMode, kPanMode_balance, "balance");

	setpresetname(0, "let's merge");	// default preset name

	addchannelconfig(2, 2);	// 2-in/2-out
	addchannelconfig(1, 2);	// 1-in/2-out


	#if TARGET_API_AUDIOUNIT
		// XXX is there a better way to do this?
		update_preset(0);	// make host see that current preset is 0
	#endif

	#if TARGET_API_VST && TARGET_PLUGIN_HAS_GUI
		editor = new MonomakerEditor(this);
	#endif
}

//-----------------------------------------------------------------------------------------
void Monomaker::processaudio(const float **inputs, float **outputs, unsigned long inNumFrames, bool replacing)
{
	// temp input value holders
	float in1, in2, out1, out2;
	// input signal pointers, in order to support mono or stereo input
	const float *input1 = inputs[0], *input2 = (getnuminputs() < 2) ? inputs[0] : inputs[1];
	// parameter values
	float monomerge = getparameter_scalar(kMonomerge);
	long monomergemode = getparameter_i(kMonomergeMode);
	float pan = getparameter_f(kPan);
	long panmode = getparameter_i(kPanMode);

	// calculate monomerge gain scalars
	float monomerge_main = 1.0f - (monomerge * 0.5f);
	float monomerge_other = monomerge * 0.5f;
	// square root for equal power blending
	if (monomergemode == kMonomergeMode_equalpower)
	{
		monomerge_main = sqrtf(monomerge_main);
		monomerge_other = sqrtf(monomerge_other);
	}

	// calculate pan gain scalars
	float pan_left1, pan_left2, pan_right1, pan_right2;
	// when pan > 0.0, then we are panning to the right...
	if (pan > 0.0f)
	{
		pan_left1 = 2.0f - (pan + 1.0f);
		pan_left2 = 0.0f;
		pan_right1 = pan;
		pan_right2 = 1.0f;
	}
	// ...otherwise we are panning to the left
	else
	{
		pan_left1 = 1.0f;
		pan_left2 = 0.0f - pan;
		pan_right1 = 0.0f;
		pan_right2 = pan + 1.0f;
	}
	// no mixing of channels in balance mode
	if (panmode == kPanMode_balance)
	{
		pan_left1 += pan_left2;
		pan_left2 = 0.0f;
		pan_right2 += pan_right1;
		pan_right1 = 0.0f;
	}

	// process the audio streams
	for (unsigned long samplecount=0; samplecount < inNumFrames; samplecount++)
	{
		// store the input values into temp variables for shorter math below
		in1 = input1[samplecount];
		in2 = input2[samplecount];

		// do monomerging
		out1 = (in1 * monomerge_main) + (in2 * monomerge_other);
		out2 = (in2 * monomerge_main) + (in1 * monomerge_other);

		// do panning into the output stream
	#if TARGET_API_VST
		if (replacing)
		{
	#endif
			outputs[0][samplecount] = (out1 * pan_left1) + (out2 * pan_left2);
			outputs[1][samplecount] = (out2 * pan_right2) + (out1 * pan_right1);
	#if TARGET_API_VST
		}
		else
		{
			outputs[0][samplecount] += (out1 * pan_left1) + (out2 * pan_left2);
			outputs[1][samplecount] += (out2 * pan_right2) + (out1 * pan_right1);
		}
	#endif
	}
}
