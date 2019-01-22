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

#include "geometerview.h"

#include <algorithm>
#include <cmath>

constexpr auto coldwave = MakeCColor(75, 151, 71);
constexpr auto cnewwave = MakeCColor(240, 255, 160);
constexpr auto cpointoutside = MakeCColor(0, 0, 0);
constexpr auto cpointinside = MakeCColor(220, 100, 200);
//constexpr auto cbackground = MakeCColor(17, 25, 16);
constexpr auto cbackground = MakeCColor(20, 50, 20);
constexpr auto zeroline = MakeCColor(52, 71, 49);

GeometerView::GeometerView(CRect const & size, PLUGIN * listener)
  : CView(size),
#if TARGET_PLUGIN_USES_DSPCORE
    geom(dynamic_cast<PLUGINCORE*>(listener->getplugincore(0))),
#else
    geom(listener),
#endif
    samples(size.getWidth()) {

  assert(geom);

  inputs.assign((samples + 3), 0.0f);
  pointsx.assign((samples + 3), 0);
  pointsy.assign((samples + 3), 0.0f);
  tmpx.assign((samples + 3), 0);
  tmpy.assign((samples + 3), 0.0f);
  outputs.assign((samples + 3), 0.0f);

  setWantsIdle(true);
}


 
bool GeometerView::attached(CView * parent) {

  auto const success = CView::attached(parent);

  if (success) {
    offc = COffscreenContext::create(getFrame(), getWidth(), getHeight());

    reflect();
  }

  return success;
}


void GeometerView::draw(CDrawContext * ctx) {

  if (!offc || inputs.empty() || outputs.empty()) return; /* not ready yet */

  auto const signedlinear2y = [height = getHeight()](float value) -> CCoord {
    return height * (-value + 1.0) * 0.5;
  };

  offc->beginDraw();

  offc->setFillColor(cbackground);
  offc->drawRect(CRect(-1, -1, getWidth(), getHeight()), kDrawFilled);

  offc->setFrameColor(zeroline);
  CCoord const centery = std::floor(getHeight() / 2.0);
  offc->drawLine(CPoint(0, centery), CPoint(getWidth(), centery));

  CCoord const start = (apts < getWidth()) ? ((std::lround(getWidth()) - apts) >> 1) : 0;
  CDrawContext::LinePair line;
//  VSTGUI::SharedPointer const path(ctx->createGraphicsPath(), false);

  offc->setFrameColor(coldwave);
  line.first = line.second = CPoint(start, signedlinear2y(inputs.front()));
  for (int i = 1; i < apts; i ++) {
    line.first = std::exchange(line.second, CPoint(start + i, signedlinear2y(inputs[i])));
    offc->drawLine(line);
  }

  offc->setFrameColor(cnewwave);
  line.first = line.second = CPoint(start, signedlinear2y(outputs.front()));
  for (int i = 1; i < apts; i ++) {
    line.first = std::exchange(line.second, CPoint(start + i, signedlinear2y(outputs[i])));
    offc->drawLine(line);
  }

#if 1
  for (int i = 0; i < numpts; i ++) {
    constexpr int bordersize = 1;
    constexpr int pointsize = 1;
    auto const yy = static_cast<int>(signedlinear2y(pointsy[i]));

    CRect box(start + pointsx[i] - bordersize, yy - bordersize,
              start + pointsx[i] + bordersize + pointsize, yy + bordersize + pointsize);
    offc->setFillColor(cpointoutside);
    offc->drawRect(box, kDrawFilled);

    box.inset(bordersize, bordersize);
    offc->setFillColor(cpointinside);
    offc->drawRect(box, kDrawFilled);
  }
#endif

  offc->endDraw();
  offc->copyFrom(ctx, getViewSize());

  setDirty(false);
}


void GeometerView::onIdle() {

  /* XXX reevaluate when I should do this. */
#if 1
  /* maybe I don't need to do this every frame... */
  auto const currentms = getFrame()->getTicks();
  auto const windowsizems = static_cast<uint32_t>(static_cast<double>(geom->getwindowsize()) * 1000.0 / geom->getsamplerate());
  auto const elapsedms = currentms - prevms;
  if ((elapsedms > windowsizems) || (currentms < prevms)) {
    reflect();
    prevms = currentms;
  }
#endif
}


/* XXX use memcpy where applicable. */
/* XXX don't bother running processw unless the input data have changed. */
void GeometerView::reflect() {
  /* when idle, copy points out of Geometer */

  if (inputs.empty() || !geom || geom->in0.empty()) return; /* too soon */

#if 1
  for (int i=0; i < samples; i++) {
    inputs[i] = geom->in0[i];
  }
#else
  for (int i=0; i < samples; i++) {
    inputs[i] = std::sin((i * 10 * dfx::math::kPi<float>) / samples);
  }
#endif

  {
    std::lock_guard const guard(geom->cs);

    apts = std::min(static_cast<int>(geom->framesize), samples);

    numpts = geom->processw(inputs.data(), outputs.data(),
                            apts, pointsx.data(), pointsy.data(), samples - 1,
                            tmpx.data(), tmpy.data());
  }

  invalid();
}
