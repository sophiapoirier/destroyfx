/*------------------------------------------------------------------------
Copyright (C) 2002-2019  Tom Murphy 7 and Sophia Poirier

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
#include "geometer.h"

/* this class is for the display of the wave at the top of the plugin.
   It reads the current 'in' buffer for the plugin, runs it through
   geometer with its current settings, and then draws it up there
   whenever the plugin is idle.
*/

class GeometerView : public VSTGUI::CView {

private:
#if TARGET_PLUGIN_USES_DSPCORE
  PLUGINCORE * const geom;
#else
  PLUGIN * const geom;
#endif

  int const samples;
  int numpts = 0;

  /* active points */
  int apts = 0;

  uint32_t prevms = 0;

  std::vector<float> inputs;
  std::vector<int> pointsx;
  std::vector<float> pointsy;
  std::vector<float> outputs;

  /* passed to processw */
  std::vector<int> tmpx;
  std::vector<float> tmpy;

  VSTGUI::SharedPointer<VSTGUI::COffscreenContext> offc;

public:

  GeometerView(VSTGUI::CRect const & size, PLUGIN * listener);

  bool attached(VSTGUI::CView * parent) override;
  void draw(VSTGUI::CDrawContext * pContext) override;
  void onIdle() override;

  void reflect();
};
