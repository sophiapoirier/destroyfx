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

#pragma once


#include "dfxplugin.h"


//----------------------------------------------------------------------------- 
// these are the plugin parameters:
enum
{
	kInputSelection,
	kMonomerge,
	kMonomergeMode,
	kPan,
	kPanMode,
//	kPanLaw,

	kNumParameters
};

enum
{
	kInputSelection_Stereo,
	kInputSelection_Swap,
	kInputSelection_Left,
	kInputSelection_Right,
	kNumInputSelections
};

enum
{
	kMonomergeMode_Linear,
	kMonomergeMode_EqualPower,
	kNumMonomergeModes
};

enum
{
	kPanMode_Recenter,
	kPanMode_Balance,
	kNumPanModes
};


//----------------------------------------------------------------------------- 
class Monomaker : public DfxPlugin
{
public:
	Monomaker(TARGET_API_BASE_INSTANCE_TYPE inInstance);
	void processaudio(float const* const* inAudio, float* const* outAudio, unsigned long inNumFrames, bool replacing = true) override;
};
