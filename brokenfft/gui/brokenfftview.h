/*------------------------------------------------------------------------
Copyright (C) 2002-2020  Tom Murphy 7 and Sophia Poirier

This file is part of BrokenFFT.

BrokenFFT is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

BrokenFFT is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with BrokenFFT.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#ifndef _DFX_BROKENFFT_VIEW_H
#define _DFX_BROKENFFT_VIEW_H


#include "dfxgui.h"
#include "brokenfft-base.h"

/* this class is for the display of the wave at the top of the plugin.
   It reads the current 'in' buffer for the plugin, runs it through
   brokenfft with its current settings, and then draws it up there
   whenever the plugin is idle.
*/

class BrokenFFTView final : public VSTGUI::CView {

private:

  BrokenFFTViewData data;
  uint64_t prevtimestamp = 0;

  DfxGuiEditor * editor = nullptr;
  // TODO: does this still serve a purpose in modern VSTGUI?  
  VSTGUI::SharedPointer<VSTGUI::COffscreenContext> offc;

public:

  explicit BrokenFFTView(VSTGUI::CRect const & size);

  bool attached(VSTGUI::CView * parent) override;
  void draw(VSTGUI::CDrawContext * pContext) override;
  void onIdle() override;

  void reflect();
};

#endif
