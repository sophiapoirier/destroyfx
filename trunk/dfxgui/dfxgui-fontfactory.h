/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2020  Sophia Poirier and Tom Murphy VII

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

#ifndef _DFXGUI_FONTFACTORY_H
#define _DFXGUI_FONTFACTORY_H

#include <memory>

#include "dfxdefines.h"
#include "dfxgui-base.h"

// XXX probably not right to do this here, but vstgui needs aeffectx.h to
// be included before it when targeting vst, because it looks for __aeffectx__
// include guards to avoid redefining symbols (ugh)
#include "dfxplugin-base.h"

#include "vstgui.h"

namespace dfx {

class FontFactory {
public:
  static std::unique_ptr<FontFactory> Create();
  virtual ~FontFactory() {}

  // Install all fonts found in the embedded resources. On mac,
  // anything that can be installed as a font is installed. On
  // Windows, resources of type DFX_TTF are installed. Note that on
  // windows, this is somewhat expensive (writes temporary files).
  // Thread-safe; OK to call this multiple times per FontFactory
  // instance.
  virtual void InstallAllFonts() = 0;
  
  // Create a VSTGUI font descriptor. If the font name is null, uses
  // the "system font."
  // 
  // Installs all fonts if that hasn't been done yet. FontFactory must
  // outlive the font instance.
  virtual VSTGUI::SharedPointer<VSTGUI::CFontDesc> CreateVstGuiFont(
      float inFontSize, char const* inFontName = nullptr) = 0;

protected:
  // Use Create().
  FontFactory() {}
private:
  FontFactory(const FontFactory&) = delete;
  void operator=(const FontFactory&) = delete;
};

}  // namespace dfx

#endif
