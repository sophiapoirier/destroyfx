/*------------------------------------------------------------------------
Copyright (C) 2002-2021  Tom Murphy 7 and Sophia Poirier

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

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "bufferoverrideview.h"

#include <cassert>
#include <cmath>
#include <utility>

constexpr auto color_dark = VSTGUI::MakeCColor(0x6f, 0x3f, 0x00);
constexpr auto color_med = VSTGUI::MakeCColor(0xc8, 0x91, 0x3e);
constexpr auto color_lite = VSTGUI::MakeCColor(0xe8, 0xd0, 0xaa);

using VSTGUI::CPoint;
using VSTGUI::UTF8StringPtr;

BufferOverrideView::BufferOverrideView(VSTGUI::CRect const & size)
  : VSTGUI::CView(size) {

  // XXX temporary
  fontDesc = VSTGUI::makeOwned<VSTGUI::CFontDesc>(VSTGUI::kSystemFont->getName(), 24);
  
  setWantsIdle(true);
}


 
bool BufferOverrideView::attached(VSTGUI::CView * parent) {

  auto const success = VSTGUI::CView::attached(parent);

  if (success) {
    editor = dynamic_cast<DfxGuiEditor*>(getEditor());
    offc = VSTGUI::COffscreenContext::create({getWidth(), getHeight()});

    reflect();
  }

  return success;
}


void BufferOverrideView::draw(VSTGUI::CDrawContext *ctx) {

  assert(offc);

  const auto width = getWidth();
  const auto height = getHeight();

  offc->beginDraw();

  offc->setFillColor(color_dark);
  offc->drawRect(VSTGUI::CRect(-1, -1, width, height), VSTGUI::kDrawFilled);

  // XXX this just demonstrates that we are getting the parameters.
  // draw something nice!
  offc->setFont(fontDesc);
  offc->setFontColor(color_med);
  
  char placeholder[128];
  sprintf(placeholder, "%.4f %.4f", divisor, buffer_ms);


  offc->drawString(VSTGUI::UTF8String(placeholder).getPlatformString(), CPoint(10, 50));

#if 0
  // from geometer, for reference
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
#endif
  
  offc->endDraw();
  offc->copyFrom(ctx, getViewSize());
  
  setDirty(false);
}


void BufferOverrideView::onIdle() {
  reflect();
}


void BufferOverrideView::reflect() {
  // When idle, update copies of the parameters from the editor.
  // Perhaps it's better to do this by registering listeners? But:
  //   - want to take LFOs into account
  //   - want to leave open the possibility of rendering some waveform data
  
  assert(editor);

  // (This should compute the current effective divisor and buffer in some
  // absolute terms. Currently ignores LFOs, and buffer_ms only works when
  // buffer sync is off)
  divisor = editor->getparameter_f(kDivisor);
  buffer_ms = editor->getparameter_f(kBufferSize_MS);    

  // Only if changed?
  invalid();
}
