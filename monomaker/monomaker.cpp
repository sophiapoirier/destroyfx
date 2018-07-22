/*------------------------------------------------------------------------
Copyright (C) 2001-2018  Sophia Poirier

This file is part of Monomaker.

Monomaker is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Monomaker is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Monomaker.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "monomaker.h"

#include <cmath>


// this macro does boring entry point stuff for us
DFX_EFFECT_ENTRY(Monomaker)

//-----------------------------------------------------------------------------
// initializations and such
Monomaker::Monomaker(TARGET_API_BASE_INSTANCE_TYPE inInstance)
:	DfxPlugin(inInstance, kNumParameters, 1)
{
	// initialize the parameters
	initparameter_list(kInputSelection, "input selection", kInputSelection_Stereo, kInputSelection_Stereo, kNumInputSelections);
	initparameter_f(kMonomerge, "monomix", 0.0, 100.0, 0.0, 100.0, DfxParam::Unit::Percent);
	initparameter_list(kMonomergeMode, "monomix mode", kMonomergeMode_EqualPower, kMonomergeMode_Linear, kNumMonomergeModes);
	initparameter_f(kPan, "pan", 0.0, 0.0, -1.0, 1.0, DfxParam::Unit::Pan);
	initparameter_list(kPanMode, "pan mode", kPanMode_Recenter, kPanMode_Recenter, kNumPanModes);
//	initparameter_list(kPanLaw, "pan law", kPanLaw_, kPanLaw_, kNumPanLaws);

	// set the parameter value display strings
	setparametervaluestring(kInputSelection, kInputSelection_Stereo, "left-right");
	setparametervaluestring(kInputSelection, kInputSelection_Swap, "right-left");
	setparametervaluestring(kInputSelection, kInputSelection_Left, "left-left");
	setparametervaluestring(kInputSelection, kInputSelection_Right, "right-right");
	//
	setparametervaluestring(kMonomergeMode, kMonomergeMode_Linear, "linear");
	setparametervaluestring(kMonomergeMode, kMonomergeMode_EqualPower, "equal power");
	//
	setparametervaluestring(kPanMode, kPanMode_Recenter, "recenter");
	setparametervaluestring(kPanMode, kPanMode_Balance, "balance");
	//
//	setparametervaluestring(kPanLaw, kPanLaw_, "-3 dB");
//	setparametervaluestring(kPanLaw, kPanLaw_, "-6 dB");
//	setparametervaluestring(kPanLaw, kPanLaw_, "sine-cosine");
/*
left_output = cos(p) * input
right_output = sin(p) * input
where p is 0¡ to 90¡ (or 0 to 1/2 ¹), with 45¡ center
*/
/*
Stereo Pan supports three different kinds of pan law:  0 dB, -3 dB, and -6 dB.
The 0dB setting essentially means that you have pan law turned off.
The -3 dB setting means that sounds panned in the center are reduced by 3 dB.
The -6 dB setting means that sounds panned in the center are reduced by 6 dB.
The -3 dB setting uses a constant power curve based on sin/cos, while other two settings are linear.
*/
//	setparametervaluestring(kPanLaw, kPanLaw_, "square root");
//	setparametervaluestring(kPanLaw, kPanLaw_, "0 dB");

	setpresetname(0, "let's merge");  // default preset name

	addchannelconfig(2, 2);  // 2-in/2-out
	addchannelconfig(1, 2);  // 1-in/2-out
}

//-----------------------------------------------------------------------------------------
void Monomaker::processaudio(float const* const* inAudio, float* const* outAudio, unsigned long inNumFrames, bool replacing)
{
	// fetch the current parameter values
	auto const inputSelection = getparameter_i(kInputSelection);
	auto const monomerge = static_cast<float>(getparameter_scalar(kMonomerge));
	auto const monomergeMode = getparameter_i(kMonomergeMode);
	auto const pan = static_cast<float>(getparameter_f(kPan));
	auto const panMode = getparameter_i(kPanMode);
//	auto const panLaw = getparameter_i(kPanLaw);

	// point the input signal pointers to the correct input streams, 
	// according to the input selection (or dual-left if we only have 1 input)
	float const* input1 {}, * input2 {};
	if ((inputSelection == kInputSelection_Left) || (getnuminputs() == 1))
	{
		input1 = input2 = inAudio[0];
	}
	else if (inputSelection == kInputSelection_Right)
	{
		input1 = input2 = inAudio[1];
	}
	else if (inputSelection == kInputSelection_Swap)
	{
		input1 = inAudio[1];
		input2 = inAudio[0];
	}
	else
	{
		input1 = inAudio[0];
		input2 = inAudio[1];
	}

	// calculate monomerge gain scalars
	float monomerge_main = 1.0f - (monomerge * 0.5f);
	float monomerge_other = monomerge * 0.5f;
	// square root for equal power blending
	if (monomergeMode == kMonomergeMode_EqualPower)
	{
		monomerge_main = std::sqrt(monomerge_main);
		monomerge_other = std::sqrt(monomerge_other);
	}

	// calculate pan gain scalars
	float pan_left1 {}, pan_left2 {}, pan_right1 {}, pan_right2 {};
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
	if (panMode == kPanMode_Balance)
	{
		pan_left1 += pan_left2;
		pan_left2 = 0.0f;
		pan_right2 += pan_right1;
		pan_right1 = 0.0f;
	}

	// process the audio streams
	for (unsigned long i = 0; i < inNumFrames; i++)
	{
		// do monomerging
		float const out1 = (input1[i] * monomerge_main) + (input2[i] * monomerge_other);
		float const out2 = (input2[i] * monomerge_main) + (input1[i] * monomerge_other);

		// do panning into the output stream
	#ifdef TARGET_API_VST
		if (replacing)
		{
	#endif
			outAudio[0][i] = (out1 * pan_left1) + (out2 * pan_left2);
			outAudio[1][i] = (out2 * pan_right2) + (out1 * pan_right1);
	#ifdef TARGET_API_VST
		}
		else
		{
			outAudio[0][i] += (out1 * pan_left1) + (out2 * pan_left2);
			outAudio[1][i] += (out2 * pan_right2) + (out1 * pan_right1);
		}
	#endif
	}
}
