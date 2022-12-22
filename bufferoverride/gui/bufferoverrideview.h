/*------------------------------------------------------------------------
Copyright (C) 2021-2022  Tom Murphy 7 and Sophia Poirier

This file is part of Buffer Override.

Buffer Override is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Buffer Override is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Buffer Override.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#pragma once


#include "bufferoverride-base.h"
#include "dfxgui.h"
#include "dfxmath.h"


// Visualizes how Buffer Override overrides yer buffers.
// This only depends on the values of parameters, but could be extended
// to show the input/output waveforms too?

class BufferOverrideView final : public VSTGUI::CView {
 public:

  static constexpr int WIDTH = 240;
  static constexpr int HEIGHT = 100;

  explicit BufferOverrideView(VSTGUI::CRect const &size);

  bool attached(VSTGUI::CView *parent) override;
  bool removed(VSTGUI::CView *parent) override;
  void draw(VSTGUI::CDrawContext *pContext) override;
  void onIdle() override;

 private:

  void reflect();

  BufferOverrideViewData data;
  uint64_t prevtimestamp = 0u;
  dfx::math::RandomEngine random_engine {dfx::math::RandomSeed::Monotonic};

  DfxGuiEditor *editor = nullptr;

  // TODO: (Copied from Geometer) Does this still serve a purpose in
  // modern VSTGUI?
  VSTGUI::SharedPointer<VSTGUI::COffscreenContext> offc;
};
