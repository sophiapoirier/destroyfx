/*------------------------------------------------------------------------
Copyright (C) 2002-2018  Tom Murphy 7 and Sophia Poirier

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

#include <vector>

#include "dfxgui.h"


//--------------------------------------------------------------------------
class GeometerHelpBox : public DGStaticTextDisplay {
public:
  GeometerHelpBox(DfxGuiEditor * inOwnerEditor, DGRect const& inRegion, DGImage * inBackground);

  void draw(CDrawContext * inContext) override;

  void setDisplayItem(int inHelpCategory, int inItemNum);

private:
  int helpCategory;
  int itemNum;
};


//--------------------------------------------------------------------------
class GeometerEditor : public DfxGuiEditor {
public:
  GeometerEditor(DGEditorListenerInstance inInstance);

  long OpenEditor() override;
  void parameterChanged(long inParameterID) override;
  void mouseovercontrolchanged(DGControl * currentControlUnderMouse) override;

  void changehelp(DGControl * currentControlUnderMouse);

private:
  long choose_multiparam(long baseParamID) {
    return getparameter_i(baseParamID) + baseParamID + 1;
  }
  static long get_base_param_for_slider(size_t sliderIndex) noexcept;

  std::vector<DGSlider *> sliders;
  std::vector<DGTextDisplay *> displays;
  std::vector<DGFineTuneButton *> finedownbuttons;
  std::vector<DGFineTuneButton *> fineupbuttons;

  std::vector<DGControl *> genhelpitemcontrols;
  std::vector<VSTGUI::SharedPointer<DGImage>> g_helpicons;
  DGButton * helpicon = nullptr;
  GeometerHelpBox * helpbox = nullptr;
};
