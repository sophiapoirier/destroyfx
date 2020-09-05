/*------------------------------------------------------------------------
Copyright (C) 2001-2020  Sophia Poirier

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

#include <algorithm>
#include <cassert>
#include <cmath>


// this macro does boring entry point stuff for us
DFX_EFFECT_ENTRY(Monomaker)

//-----------------------------------------------------------------------------
// initializations and such
Monomaker::Monomaker(TARGET_API_BASE_INSTANCE_TYPE inInstance)
:	DfxPlugin(inInstance, kNumParameters, 1)
{
	// initialize the parameters
	initparameter_list(kInputSelection, {"input selection", "Input", "Inpt"}, kInputSelection_Stereo, kInputSelection_Stereo, kNumInputSelections);
	initparameter_f(kMonomerge, {"monomix", "MonMix", "Mix"}, 0.0, 100.0, 0.0, 100.0, DfxParam::Unit::Percent);
	initparameter_list(kMonomergeMode, {"monomix mode", "MonoMod", "MonMod", "MMod"}, kMonomergeMode_EqualPower, kMonomergeMode_Linear, kNumMonomergeModes);
	initparameter_f(kPan, {"pan"}, 0.0, 0.0, -1.0, 1.0, DfxParam::Unit::Pan);
	initparameter_list(kPanMode, {"pan mode", "PanMode", "PanMod", "PMod"}, kPanMode_Recenter, kPanMode_Recenter, kNumPanModes);
//	initparameter_list(kPanLaw, {"pan law", "PanLaw", "PLaw"}, kPanLaw_, kPanLaw_, kNumPanLaws);

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

	registerSmoothedAudioValue(&mInputSelection_left2left);
	registerSmoothedAudioValue(&mInputSelection_left2right);
	registerSmoothedAudioValue(&mInputSelection_right2left);
	registerSmoothedAudioValue(&mInputSelection_right2right);
	registerSmoothedAudioValue(&mMonomerge_main);
	registerSmoothedAudioValue(&mMonomerge_other);
	registerSmoothedAudioValue(&mPan_left1);
	registerSmoothedAudioValue(&mPan_left2);
	registerSmoothedAudioValue(&mPan_right1);
	registerSmoothedAudioValue(&mPan_right2);
}

//-----------------------------------------------------------------------------------------
void Monomaker::createbuffers()
{
	mAsymmetricalInputAudioBuffer.assign(getmaxframes(), 0.0f);
}

//-----------------------------------------------------------------------------------------
void Monomaker::releasebuffers()
{
	mAsymmetricalInputAudioBuffer.clear();
}

//-----------------------------------------------------------------------------------------
void Monomaker::processparameters()
{
	if (auto const inputSelection = getparameterifchanged_i(kInputSelection))
	{
		switch (*inputSelection)
		{
			case kInputSelection_Stereo:
				mInputSelection_left2left = 1.0f;
				mInputSelection_left2right = 0.0f;
				mInputSelection_right2left = 0.0f;
				mInputSelection_right2right = 1.0f;
				break;
			case kInputSelection_Left:
				mInputSelection_left2left = 1.0f;
				mInputSelection_left2right = 1.0f;
				mInputSelection_right2left = 0.0f;
				mInputSelection_right2right = 0.0f;
				break;
			case kInputSelection_Right:
				mInputSelection_left2left = 0.0f;
				mInputSelection_left2right = 0.0f;
				mInputSelection_right2left = 1.0f;
				mInputSelection_right2right = 1.0f;
				break;
			case kInputSelection_Swap:
				mInputSelection_left2left = 0.0f;
				mInputSelection_left2right = 1.0f;
				mInputSelection_right2left = 1.0f;
				mInputSelection_right2right = 0.0f;
				break;
			default:
				assert(false);
				break;
		}
	}

	if (getparameterchanged(kMonomerge) || getparameterchanged(kMonomergeMode))
	{
		auto const monomerge = static_cast<float>(getparameter_scalar(kMonomerge));
		auto const monomergeMode = getparameter_i(kMonomergeMode);

		// calculate monomerge gain scalars
		auto const mapMonomergeMode = (monomergeMode == kMonomergeMode_EqualPower) ? sqrtf : [](float a){ return a; };
		mMonomerge_main = mapMonomergeMode(1.0f - (monomerge * 0.5f));
		mMonomerge_other = mapMonomergeMode(monomerge * 0.5f);
	}

	if (getparameterchanged(kPan) || getparameterchanged(kPanMode))
	{
		auto const pan = static_cast<float>(getparameter_f(kPan));
		auto const panMode = getparameter_i(kPanMode);
//		auto const panLaw = getparameter_i(kPanLaw);

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

		mPan_left1 = pan_left1;
		mPan_left2 = pan_left2;
		mPan_right1 = pan_right1;
		mPan_right2 = pan_right2;
	}
}

//-----------------------------------------------------------------------------------------
void Monomaker::processaudio(float const* const* inAudio, float* const* outAudio, unsigned long inNumFrames)
{
	// point the input signal pointers to the correct input streams, 
	// according to the input selection (or dual-left if we only have 1 input)
	float const* inAudioL {}, * inAudioR {};
	if (getnuminputs() == getnumoutputs())
	{
		inAudioL = inAudio[0];
		inAudioR = inAudio[1];
	}
	else
	{
		// copy to an intermediate input buffer in case processing in-place
		std::copy_n(inAudio[0], inNumFrames, mAsymmetricalInputAudioBuffer.data());
		inAudioL = inAudioR = mAsymmetricalInputAudioBuffer.data();
	}

	// process the audio streams
	for (unsigned long i = 0; i < inNumFrames; i++)
	{
		// apply input selection matrix
		float const inL = (inAudioL[i] * mInputSelection_left2left.getValue()) + (inAudioR[i] * mInputSelection_right2left.getValue());
		float const inR = (inAudioL[i] * mInputSelection_left2right.getValue()) + (inAudioR[i] * mInputSelection_right2right.getValue());

		// do monomerging
		float const outL = (inL * mMonomerge_main.getValue()) + (inR * mMonomerge_other.getValue());
		float const outR = (inR * mMonomerge_main.getValue()) + (inL * mMonomerge_other.getValue());

		// do panning into the output stream
		outAudio[0][i] = (outL * mPan_left1.getValue()) + (outR * mPan_left2.getValue());
		outAudio[1][i] = (outR * mPan_right2.getValue()) + (outL * mPan_right1.getValue());

		incrementSmoothedAudioValues();
	}
}
