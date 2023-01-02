/*------------------------------------------------------------------------
Copyright (C) 2021-2023  Tom Murphy 7 and Sophia Poirier

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

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#include "fonttesteditor.h"

#include "dfxmisc.h"

// Make adjustments to the constants in dfxgui-base.h.

// TODO: We should test the help display here too since it does some hacks
// to do and undo tweaks.

constexpr auto kDisplayTextColor = DGColor::kBlack;

//-----------------------------------------------------------------------------
// positions
enum
{
  kEvenTextX = 16,
  kEvenWetarY = 64 + dfx::kFontYOffset_Wetar16px,
  kEvenSnootyY = 128 + dfx::kFontYOffset_Snooty10px,
  kEvenPasementY = 192 + dfx::kFontYOffset_Pasement9px,

  kOddTextX = 160,
  kOddWetarY = 63 + dfx::kFontYOffset_Wetar16px,
  kOddSnootyY = 127 + dfx::kFontYOffset_Snooty10px,
  kOddPasementY = 191 + dfx::kFontYOffset_Pasement9px,

  kTextWidth = 128,
};

//-----------------------------------------------------------------------------
DFX_EDITOR_ENTRY(FontTestEditor)

//-----------------------------------------------------------------------------
void FontTestEditor::OpenEditor()
{
  std::string text =
    (std::string)"Wey go 9%~ " +
    (std::string)dfx::kInfinityUTF8;

  VSTGUI::UTF8String text8(text);

  // Even positions
  {
    constexpr DGRect pos(kEvenTextX, kEvenWetarY, kTextWidth,
                         dfx::kFontContainerHeight_Wetar16px);

    emplaceControl<DGStaticTextDisplay>(
        this, pos, /* background */ nullptr,
        dfx::TextAlignment::Left,
        dfx::kFontSize_Wetar16px,
        kDisplayTextColor,
        dfx::kFontName_Wetar16px)->setText(text8);
  }

  {
    constexpr DGRect pos(kEvenTextX, kEvenSnootyY, kTextWidth,
                         dfx::kFontContainerHeight_Snooty10px);

    emplaceControl<DGStaticTextDisplay>(
        this, pos, /* background */ nullptr,
        dfx::TextAlignment::Left,
        dfx::kFontSize_Snooty10px,
        kDisplayTextColor,
        dfx::kFontName_Snooty10px)->setText(text8);
  }

  {
    constexpr DGRect pos(kEvenTextX, kEvenPasementY, kTextWidth,
                         dfx::kFontContainerHeight_Pasement9px);

    emplaceControl<DGStaticTextDisplay>(
        this, pos, /* background */ nullptr,
        dfx::TextAlignment::Left,
        dfx::kFontSize_Pasement9px,
        kDisplayTextColor,
        dfx::kFontName_Pasement9px)->setText(text8);
  }

  // Odd positions
  {
    constexpr DGRect pos(kOddTextX, kOddWetarY, kTextWidth,
                         dfx::kFontContainerHeight_Wetar16px);

    emplaceControl<DGStaticTextDisplay>(
        this, pos, /* background */ nullptr,
        dfx::TextAlignment::Left,
        dfx::kFontSize_Wetar16px,
        kDisplayTextColor,
        dfx::kFontName_Wetar16px)->setText(text8);
  }

  {
    constexpr DGRect pos(kOddTextX, kOddSnootyY, kTextWidth,
                         dfx::kFontContainerHeight_Snooty10px);

    emplaceControl<DGStaticTextDisplay>(
        this, pos, /* background */ nullptr,
        dfx::TextAlignment::Left,
        dfx::kFontSize_Snooty10px,
        kDisplayTextColor,
        dfx::kFontName_Snooty10px)->setText(text8);
  }

  {
    constexpr DGRect pos(kOddTextX, kOddPasementY, kTextWidth,
                         dfx::kFontContainerHeight_Pasement9px);

    emplaceControl<DGStaticTextDisplay>(
        this, pos, /* background */ nullptr,
        dfx::TextAlignment::Left,
        dfx::kFontSize_Pasement9px,
        kDisplayTextColor,
        dfx::kFontName_Pasement9px)->setText(text8);
  }
}
