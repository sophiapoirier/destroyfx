/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2018  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Destroy FX Library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Destroy FX Library.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#pragma once


#include <stdint.h>


namespace dfx
{


//-----------------------------------------------------------------------------
#if 0
enum KeyModifiers : unsigned int
{
	kKeyModifier_Accel = 1,  // command on Macs, control on PCs
	kKeyModifier_Alt = 1 << 1,  // option on Macs, alt on PCs
	kKeyModifier_Shift = 1 << 2,
	kKeyModifier_Extra = 1 << 3  // control on Macs
};
#endif

enum Axis
{
	kAxis_Horizontal = 1,
	kAxis_Vertical = 1 << 1,
	kAxis_Omni = (kAxis_Horizontal | kAxis_Vertical)
};

enum class TextAlignment
{
	Left,
	Center,
	Right
};

static char const* const kFontName_SnootPixel10 = "snoot.org pixel10";
static constexpr float kFontSize_SnootPixel10 = 14.0f;

// VSTGUI does not define named constants for these onKeyDown and onKeyUp result values
static constexpr int32_t kKeyEventHandled = 1;
static constexpr int32_t kKeyEventNotHandled = -1;


}  // namespace dfx
