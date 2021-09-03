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


//-----------------------------------------------------------------------------
// positions
enum
{
  kTextX = 16,
  kWetarY = 64,
  kSnootyY = 128,
  kPasementY = 192,

  kTextWidth = 128,
};


constexpr DGColor kDisplayTextColor(0, 0, 0);

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

  {
    DGRect pos;
    pos.set(kTextX, kWetarY, kTextWidth, 16);

    emplaceControl<DGStaticTextDisplay>(
        this, pos, /* background */ nullptr,
        dfx::TextAlignment::Left,
        dfx::kFontSize_Wetar16px,
        kDisplayTextColor,
        dfx::kFontName_Wetar16px)->setText(text8);
  }

  {
    DGRect pos;
    pos.set(kTextX, kSnootyY, kTextWidth, 10);

    emplaceControl<DGStaticTextDisplay>(
        this, pos, /* background */ nullptr,
        dfx::TextAlignment::Left,
        dfx::kFontSize_Snooty10px,
        kDisplayTextColor,
        dfx::kFontName_Snooty10px)->setText(text8);
  }

  {
    DGRect pos;
    pos.set(kTextX, kPasementY, kTextWidth, 9);

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
