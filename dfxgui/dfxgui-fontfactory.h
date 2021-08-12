/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2021  Sophia Poirier and Tom Murphy VII

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

#ifndef _DFXGUI_FONTFACTORY_H
#define _DFXGUI_FONTFACTORY_H

#include <memory>

#include "dfxgui-base.h"

namespace dfx {

class FontFactory {
public:
  // Creates a font factory instance, which installs all the fonts
  // found in embedded resources.
  //
  // On macOS, any font contained within the plugin bundle resources
  // will be registered for the host process. On Windows, resources
  // of type DFX_TTF are installed. Note that on Windows, this is
  // somewhat expensive (writes temporary files).
  //
  // Note that on Windows, it seems to take a few milliseconds for
  // installed fonts to actually become available (or maybe it needs
  // to run an event loop or something). So calling CreateVstGuiFont
  // immediately after constructing the FontFactory may not actually
  // work. :( TODO: Is there some way to wait until the fonts are all
  // found before returning?
  static std::unique_ptr<FontFactory> Create();
  virtual ~FontFactory() {}

  // Create a VSTGUI font descriptor. If the font name is null, uses
  // the "system font."
  // 
  // FontFactory must outlive the returned font instance.
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
