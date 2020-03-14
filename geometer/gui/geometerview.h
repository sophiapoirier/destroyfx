/*------------------------------------------------------------------------
Copyright (C) 2002-2020  Tom Murphy 7 and Sophia Poirier

This file is part of Geometer.

Geometer is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Geometer is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Geometer.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#pragma once


#include "dfxgui.h"
#include "geometer-base.h"

/* this class is for the display of the wave at the top of the plugin.
   It reads the current 'in' buffer for the plugin, runs it through
   geometer with its current settings, and then draws it up there
   whenever the plugin is idle.
*/

class GeometerView final : public VSTGUI::CView {

private:

  GeometerViewData data;
  uint64_t prevtimestamp = 0;

  DfxGuiEditor * editor = nullptr;
  VSTGUI::SharedPointer<VSTGUI::COffscreenContext> offc;  // TODO: does this still serve a purpose in modern VSTGUI?

public:

  explicit GeometerView(VSTGUI::CRect const & size);

  bool attached(VSTGUI::CView * parent) override;
  void draw(VSTGUI::CDrawContext * pContext) override;
  void onIdle() override;

  void reflect();
};
