/*------------------------------------------------------------------------
Copyright (C) 2001-2021  Tom Murphy 7 and Sophia Poirier

This file is part of FontTest.

FontTest is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

FontTest is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with FontTest.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "fonttesteditor.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string_view>

#include "dfxmisc.h"

using namespace dfx::FT;

// Even though a font may be actually 16 pixels high, we need some
// leeway to account for platform-specific rendering differences.
// VSTGUI clips the text if it exceeds this height; the additional
// clearance makes sure that descenders in characters like 'g' don't
// get cut off. Every time we render the font we should use this
// height for the container element.
//
// These are empirically determined from the results of rendering
// on Mac, which is the reference platform. Windows has additional
// tweaks off these. (Or perhaps we could just make these constants
// differ by platform?)
// constexpr int kFontContainerHeight_Wetar16px = 20;
// constexpr int kFontContainerHeight_Snooty10px = 12;
constexpr int kFontContainerHeight_Wetar16px = 18;
constexpr int kFontContainerHeight_Snooty10px = 11;
constexpr int kFontContainerHeight_Pasement9px = 10;

// These offsets allow for precise vertical positioning of the font
// against some reference pixel. For example, positioning the control
// at (x, y + kFontYOffset_whatever) will render such that the top
// left pixel of the font is at (x, y). These are optional; you can
// also just put the font where it looks good. Internal to dfxgui, the
// position is adjusted to get the same result on each platform.
constexpr int kFontYOffset_Wetar16px = -2;
constexpr int kFontYOffset_Snooty10px = -1;
constexpr int kFontYOffset_Pasement9px = -1;

constexpr DGColor kDisplayTextColor(0, 0, 0);

//-----------------------------------------------------------------------------
// positions
enum
{
  kEvenTextX = 16,
  kEvenWetarY = 64 + kFontYOffset_Wetar16px,
  kEvenSnootyY = 128 + kFontYOffset_Snooty10px,
  kEvenPasementY = 192 + kFontYOffset_Pasement9px,

  kOddTextX = 160,  
  kOddWetarY = 63 + kFontYOffset_Wetar16px,
  kOddSnootyY = 127 + kFontYOffset_Snooty10px,
  kOddPasementY = 191 + kFontYOffset_Pasement9px,

  kTextWidth = 128,
};

//-----------------------------------------------------------------------------
DFX_EDITOR_ENTRY(FontTestEditor)

//-----------------------------------------------------------------------------
FontTestEditor::FontTestEditor(DGEditorListenerInstance inInstance)
:	DfxGuiEditor(inInstance)
{

}

//-----------------------------------------------------------------------------
long FontTestEditor::OpenEditor()
{
  std::string text =
    (std::string)"Wey go 9%~ " +
    (std::string)dfx::kInfinityUTF8;

  VSTGUI::UTF8String text8(text);

  // Even positions
  {
    DGRect pos;
    pos.set(kEvenTextX, kEvenWetarY, kTextWidth,
            kFontContainerHeight_Wetar16px);

    emplaceControl<DGStaticTextDisplay>(
        this, pos, /* background */ nullptr,
        dfx::TextAlignment::Left,
        dfx::kFontSize_Wetar16px,
        kDisplayTextColor,
        dfx::kFontName_Wetar16px)->setText(text8);
  }

  {
    DGRect pos;
    pos.set(kEvenTextX, kEvenSnootyY, kTextWidth,
            kFontContainerHeight_Snooty10px);

    emplaceControl<DGStaticTextDisplay>(
        this, pos, /* background */ nullptr,
        dfx::TextAlignment::Left,
        dfx::kFontSize_Snooty10px,
        kDisplayTextColor,
        dfx::kFontName_Snooty10px)->setText(text8);
  }

  {
    DGRect pos;
    pos.set(kEvenTextX, kEvenPasementY, kTextWidth,
            kFontContainerHeight_Pasement9px);

    emplaceControl<DGStaticTextDisplay>(
        this, pos, /* background */ nullptr,
        dfx::TextAlignment::Left,
        dfx::kFontSize_Pasement9px,
        kDisplayTextColor,
        dfx::kFontName_Pasement9px)->setText(text8);
  }

  // Odd positions
  {
    DGRect pos;
    pos.set(kOddTextX, kOddWetarY, kTextWidth,
            kFontContainerHeight_Wetar16px);

    emplaceControl<DGStaticTextDisplay>(
        this, pos, /* background */ nullptr,
        dfx::TextAlignment::Left,
        dfx::kFontSize_Wetar16px,
        kDisplayTextColor,
        dfx::kFontName_Wetar16px)->setText(text8);
  }

  {
    DGRect pos;
    pos.set(kOddTextX, kOddSnootyY, kTextWidth,
            kFontContainerHeight_Snooty10px);

    emplaceControl<DGStaticTextDisplay>(
        this, pos, /* background */ nullptr,
        dfx::TextAlignment::Left,
        dfx::kFontSize_Snooty10px,
        kDisplayTextColor,
        dfx::kFontName_Snooty10px)->setText(text8);
  }

  {
    DGRect pos;
    pos.set(kOddTextX, kOddPasementY, kTextWidth,
            kFontContainerHeight_Pasement9px);

    emplaceControl<DGStaticTextDisplay>(
        this, pos, /* background */ nullptr,
        dfx::TextAlignment::Left,
        dfx::kFontSize_Pasement9px,
        kDisplayTextColor,
        dfx::kFontName_Pasement9px)->setText(text8);
  }

  

  return dfx::kStatus_NoError;
}

//-----------------------------------------------------------------------------
void FontTestEditor::PostOpenEditor()
{

}

//-----------------------------------------------------------------------------
void FontTestEditor::CloseEditor()
{
}

//-----------------------------------------------------------------------------
void FontTestEditor::parameterChanged(long inParameterID)
{
}

//-----------------------------------------------------------------------------
void FontTestEditor::HandlePropertyChange(dfx::PropertyID inPropertyID, dfx::Scope /*inScope*/, unsigned long /*inItemIndex*/)
{
}
