/*--------------- by Marc Poirier  ][  March 2001 + October 2002 ---------------*/

#include "monomaker.hpp"

#if defined(TARGET_API_VST) && TARGET_PLUGIN_HAS_GUI
	#include "monomakereditor.hpp"
#endif


// this macro does boring entry point stuff for us
DFX_ENTRY(Monomaker);

//-----------------------------------------------------------------------------
// initializations & such
Monomaker::Monomaker(TARGET_API_BASE_INSTANCE_TYPE inInstance)
	: DfxPlugin(inInstance, NUM_PARAMETERS, 1)	// 2 parameters, 1 preset
{
	// initialize the parameters
	initparameter_indexed(kInputSelection, "input selection", kInputSelection_stereo, kInputSelection_stereo, kNumInputSelections);
	initparameter_f(kMonomerge, "monomix", 0.0f, 100.0f, 0.0f, 100.0f, kDfxParamUnit_percent);
	initparameter_indexed(kMonomergeMode, "monomix mode", kMonomergeMode_equalpower, kMonomergeMode_linear, kNumMonomergeModes);
	initparameter_f(kPan, "pan", 0.0f, 0.0f, -1.0f, 1.0f, kDfxParamUnit_pan);
	initparameter_indexed(kPanMode, "pan mode", kPanMode_recenter, kPanMode_recenter, kNumPanModes);

	// set the parameter value display strings
	setparametervaluestring(kInputSelection, kInputSelection_stereo, "left-right");
	setparametervaluestring(kInputSelection, kInputSelection_swap, "right-left");
	setparametervaluestring(kInputSelection, kInputSelection_left, "left-left");
	setparametervaluestring(kInputSelection, kInputSelection_right, "right-right");
	//
	setparametervaluestring(kMonomergeMode, kMonomergeMode_linear, "linear");
	setparametervaluestring(kMonomergeMode, kMonomergeMode_equalpower, "equal power");
	//
	setparametervaluestring(kPanMode, kPanMode_recenter, "recenter");
	setparametervaluestring(kPanMode, kPanMode_balance, "balance");

	setpresetname(0, "let's merge");	// default preset name

	addchannelconfig(2, 2);	// 2-in/2-out
	addchannelconfig(1, 2);	// 1-in/2-out


	#if defined(TARGET_API_VST) && TARGET_PLUGIN_HAS_GUI
		editor = new MonomakerEditor(this);
	#endif
}

//-----------------------------------------------------------------------------------------
void Monomaker::processaudio(const float **inputs, float **outputs, unsigned long inNumFrames, bool replacing)
{
	// fetch the current parameter values
	long inputselection = getparameter_i(kInputSelection);
	float monomerge = getparameter_scalar(kMonomerge);
	long monomergemode = getparameter_i(kMonomergeMode);
	float pan = getparameter_f(kPan);
	long panmode = getparameter_i(kPanMode);

	// point the input signal pointers to the correct input streams, 
	// according to the input selection (or dual-left if we only have 1 input)
	const float *input1, *input2;
	if ( (inputselection == kInputSelection_left) || (getnuminputs() == 1) )
	{
		input1 = input2 = inputs[0];
	}
	else if (inputselection == kInputSelection_right)
	{
		input1 = input2 = inputs[1];
	}
	else if (inputselection == kInputSelection_swap)
	{
		input1 = inputs[1];
		input2 = inputs[0];
	}
	else
	{
		input1 = inputs[0];
		input2 = inputs[1];
	}

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
	for (unsigned long i=0; i < inNumFrames; i++)
	{
		// do monomerging
		const float out1 = (input1[i] * monomerge_main) + (input2[i] * monomerge_other);
		const float out2 = (input2[i] * monomerge_main) + (input1[i] * monomerge_other);

		// do panning into the output stream
	#ifdef TARGET_API_VST
		if (replacing)
		{
	#endif
			outputs[0][i] = (out1 * pan_left1) + (out2 * pan_left2);
			outputs[1][i] = (out2 * pan_right2) + (out1 * pan_right1);
	#ifdef TARGET_API_VST
		}
		else
		{
			outputs[0][i] += (out1 * pan_left1) + (out2 * pan_left2);
			outputs[1][i] += (out2 * pan_right2) + (out1 * pan_right1);
		}
	#endif
	}
}
