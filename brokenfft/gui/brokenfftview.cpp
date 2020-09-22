/*------------------------------------------------------------------------
Copyright (C) 2002-2019  Tom Murphy 7 and Sophia Poirier

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

#include "brokenfftview.h"

#include <cassert>
#include <cmath>
#include <utility>

constexpr auto coldwave = VSTGUI::MakeCColor(75, 151, 71);
constexpr auto cnewwave = VSTGUI::MakeCColor(240, 255, 160);
constexpr auto cpointoutside = VSTGUI::MakeCColor(0, 0, 0);
constexpr auto cpointinside = VSTGUI::MakeCColor(220, 100, 200);
constexpr auto cbackground = VSTGUI::MakeCColor(20, 50, 20);
constexpr auto zeroline = VSTGUI::MakeCColor(52, 71, 49);

BrokenFFTView::BrokenFFTView(VSTGUI::CRect const & size)
  : VSTGUI::CView(size) {

  setWantsIdle(true);
}


 
bool BrokenFFTView::attached(VSTGUI::CView * parent) {

  auto const success = VSTGUI::CView::attached(parent);

  if (success) {
    editor = dynamic_cast<DfxGuiEditor*>(getEditor());
    offc = VSTGUI::COffscreenContext::create(getFrame(), getWidth(), getHeight());

    reflect();
  }

  return success;
}


void BrokenFFTView::draw(VSTGUI::CDrawContext * ctx) {

  assert(offc);

  auto const signedlinear2y = [height = getHeight()](float value) -> VSTGUI::CCoord {
    return height * (-value + 1.0) * 0.5;
  };

  offc->beginDraw();

  offc->setFillColor(cbackground);
  offc->drawRect(VSTGUI::CRect(-1, -1, getWidth(), getHeight()), VSTGUI::kDrawFilled);

  offc->setFrameColor(zeroline);
  VSTGUI::CCoord const centery = std::floor(getHeight() / 2.0);
  offc->drawLine(VSTGUI::CPoint(0, centery), VSTGUI::CPoint(getWidth(), centery));

  VSTGUI::CCoord const start = (data.apts < getWidth()) ? ((std::lround(getWidth()) - data.apts) >> 1) : 0;
  VSTGUI::CDrawContext::LinePair line;
//  VSTGUI::SharedPointer const path(ctx->createGraphicsPath(), false);

  offc->setFrameColor(coldwave);
  line.first = line.second = VSTGUI::CPoint(start, signedlinear2y(data.inputs.front()));
  for (int i = 1; i < data.apts; i ++) {
    line.first = std::exchange(line.second, VSTGUI::CPoint(start + i, signedlinear2y(data.inputs[i])));
    offc->drawLine(line);
  }

  offc->setFrameColor(cnewwave);
  line.first = line.second = VSTGUI::CPoint(start, signedlinear2y(data.outputs.front()));
  for (int i = 1; i < data.apts; i ++) {
    line.first = std::exchange(line.second, VSTGUI::CPoint(start + i, signedlinear2y(data.outputs[i])));
    offc->drawLine(line);
  }

  for (int i = 0; i < data.numpts; i ++) {
    constexpr int bordersize = 1;
    constexpr int pointsize = 1;
    auto const yy = static_cast<int>(signedlinear2y(data.pointsy[i]));

    VSTGUI::CRect box(start + data.pointsx[i] - bordersize, yy - bordersize,
                      start + data.pointsx[i] + bordersize + pointsize, yy + bordersize + pointsize);
    offc->setFillColor(cpointoutside);
    offc->drawRect(box, VSTGUI::kDrawFilled);

    box.inset(bordersize, bordersize);
    offc->setFillColor(cpointinside);
    offc->drawRect(box, VSTGUI::kDrawFilled);
  }

  offc->endDraw();
  offc->copyFrom(ctx, getViewSize());

  setDirty(false);
}


void BrokenFFTView::onIdle() {

  /* XXX reevaluate when I should do this. */
  /* maybe I don't need to do this every frame... */
  assert(editor);
  auto const timestamp = editor->dfxgui_GetProperty<uint64_t>(PROP_LAST_WINDOW_TIMESTAMP).value_or(prevtimestamp);
  if (std::exchange(prevtimestamp, timestamp) != timestamp) {
    reflect();
  }
}


/* XXX use memcpy where applicable. */
/* XXX don't bother running processw unless the input data have changed. */
void BrokenFFTView::reflect() {
  /* when idle, copy points out of BrokenFFT */

  assert(editor);
  size_t dataSize = sizeof(data);
  [[maybe_unused]] auto const status = editor->dfxgui_GetProperty(PROP_WAVEFORM_DATA, dfx::kScope_Global, 0,
                                                                  &data, dataSize);
  assert(status == dfx::kStatus_NoError);
  assert(dataSize == sizeof(data));

  invalid();
}
