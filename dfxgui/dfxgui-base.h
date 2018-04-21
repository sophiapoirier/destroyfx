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


//-----------------------------------------------------------------------------
#if 0
enum : unsigned int
{
	kDGKeyModifier_Accel = 1,  // command on Macs, control on PCs
	kDGKeyModifier_Alt = 1 << 1,  // option on Macs, alt on PCs
	kDGKeyModifier_Shift = 1 << 2,
	kDGKeyModifier_Extra = 1 << 3  // control on Macs
};
typedef unsigned int	DGKeyModifiers;
#endif

typedef enum
{
	kDGAxis_Horizontal = 1,
	kDGAxis_Vertical = 1 << 1,
	kDGAxis_Omni = (kDGAxis_Horizontal | kDGAxis_Vertical)
} DGAxis;

enum class DGTextAlignment
{
	Left,
	Center,
	Right
};

static char const* const kDGFontName_SnootPixel10 = "snoot.org pixel10";
static constexpr float kDGFontSize_SnootPixel10 = 14.0f;
