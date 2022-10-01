/*------------------------------------------------------------------------
Copyright (C) 2002-2022  Tom Murphy 7 and Sophia Poirier

This file is part of Geometer.

Geometer is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Geometer is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Geometer.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#pragma once

#include <vector>

#include "dfxgui.h"


//--------------------------------------------------------------------------
class GeometerEditor final : public DfxGuiEditor {
public:
  explicit GeometerEditor(DGEditorListenerInstance inInstance);

  long OpenEditor() override;
  void CloseEditor() override;
  void parameterChanged(dfx::ParameterID inParameterID) override;
  void mouseovercontrolchanged(IDGControl * currentControlUnderMouse) override;

private:
  std::string changehelp(IDGControl * currentControlUnderMouse);
  static std::string helptext(size_t inHelpCategory, int inItemNum);
  dfx::ParameterID choose_multiparam(dfx::ParameterID baseParamID);
  static dfx::ParameterID get_base_param_for_slider(size_t sliderIndex) noexcept;

  std::vector<DGSlider *> sliders;
  std::vector<DGTextDisplay *> displays;
  std::vector<DGFineTuneButton *> finedownbuttons;
  std::vector<DGFineTuneButton *> fineupbuttons;

  std::vector<IDGControl *> genhelpitemcontrols;
  std::vector<VSTGUI::SharedPointer<DGImage>> g_helpicons;
  DGButton * helpicon = nullptr;
  DGHelpBox * helpbox = nullptr;
};
