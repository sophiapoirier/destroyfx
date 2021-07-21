/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2021  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 2 of the License, or 
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


#include <cstdint>

#ifdef __MACH__
	#include <TargetConditionals.h>
#endif

#ifdef TARGET_API_VST
	// XXX probably not right to do this here, but vstgui needs aeffectx.h to
	// be included before it when targeting vst, because it looks for __aeffectx__
	// include guards to avoid redefining symbols (ugh)
	#include "dfxplugin-base.h"
#endif

#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wunused-parameter"
	#pragma clang diagnostic ignored "-Wnon-virtual-dtor"
	#pragma clang diagnostic ignored "-Wfour-char-constants"
	#include "vstgui.h"
#pragma clang diagnostic pop


namespace dfx
{


//-----------------------------------------------------------------------------
enum Axis : unsigned int
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

static constexpr char const* const kFontName_Snooty10px = "DFX Snooty 10px";
static constexpr char const* const kFontName_Pasement9px = "DFX Pasement 9px";
// These are each the one size where the fonts look good (they are bitmap fonts).
static constexpr float kFontSize_Snooty10px = 10.0f;
static constexpr float kFontSize_Pasement9px = 9.0f;

static constexpr char const* const kFontName_BoringBoron = "Boring Boron";


// VSTGUI does not define named constants for these onKeyDown and onKeyUp result values
static constexpr int32_t kKeyEventHandled = 1;
static constexpr int32_t kKeyEventNotHandled = -1;


}  // namespace dfx
