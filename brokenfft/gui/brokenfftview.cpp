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
#include <memory>

constexpr auto coldwave = VSTGUI::MakeCColor(75, 151, 71);
constexpr auto cnewwave = VSTGUI::MakeCColor(240, 255, 160);
constexpr auto cpointoutside = VSTGUI::MakeCColor(0, 0, 0);
constexpr auto cpointinside = VSTGUI::MakeCColor(220, 100, 200);
constexpr auto cbackground = VSTGUI::MakeCColor(20, 50, 20);
constexpr auto zeroline = VSTGUI::MakeCColor(52, 71, 49);

using VSTGUI::CBitmapPixelAccess;
using VSTGUI::CBitmap;
using VSTGUI::CPoint;

BrokenFFTView::BrokenFFTView(VSTGUI::CRect const &size)
  : VSTGUI::CView(size) {

  setWantsIdle(true);
}


 
bool BrokenFFTView::attached(VSTGUI::CView *parent) {

  auto const success = VSTGUI::CView::attached(parent);

  if (success) {
    editor = dynamic_cast<DfxGuiEditor*>(getEditor());
    bitmap = new CBitmap(WIDTH, HEIGHT);

    reflect();
  }

  return success;
}


void BrokenFFTView::draw(VSTGUI::CDrawContext *ctx) {
  // XXX just to test pixel drawing
  static int timestamp = 0;
  
  assert(bitmap);

  ctx->setFillColor(cpointoutside);
  ctx->drawRect(VSTGUI::CRect(-1, -1, getWidth(), getHeight()), VSTGUI::kDrawFilled);
				    
  
  {
    // Premultiplied alpha? Probably doesn't matter since we'll just draw opaque pixels.
    // Premultiplied should be more efficient if we don't care.
    //
    // Note docs say that the accessor must be "forgotten" before the changes are
    // reflected. I think this means "deleted". (Note its destructor is private though,
    // so instead we initialize SharedPointer... are there any docs??)
    VSTGUI::SharedPointer<CBitmapPixelAccess> px =
      CBitmapPixelAccess::create(bitmap.get());

    // Might not be supported on this platform.
    // TODO: Some kind of fallback
    if (px.get() == nullptr)
      return;

    // XXX actually use data duh
    for (int x = 0; x < WIDTH; x++) {

      int h = ONE_HEIGHT * (1.0f + sin((x + timestamp) / 100.0f));
      for (int i = 0; i < h; i++) {
	// (XXX y=0 on top, wrong)
	int y = h;
	px->setPosition(x, y);
	// XXX using setValue should be much faster here, but we need to determine
	// byte/channel order. If the image consists of solid colors, we can probably
	// do that by computing them up front, though.
	px->setColor(cnewwave);
      }
      // And clear the rest of the column
      for (int i = h; i < ONE_HEIGHT; i++) {
	int y = h;
	px->setPosition(x, y);
	px->setColor(cbackground);
      }
    }
  }
    
  // XXX no
  timestamp++;
  timestamp %= 999999;

  // XXX What is the second argument here?
  bitmap->draw(ctx, getViewSize(), CPoint (0, 0), 1.0f);

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


void BrokenFFTView::reflect() {
  /* when idle, copy points out of BrokenFFT */

  assert(editor);
  size_t dataSize = sizeof(data);
  [[maybe_unused]] auto const status = editor->dfxgui_GetProperty(PROP_FFT_DATA, dfx::kScope_Global, 0,
                                                                  &data, dataSize);
  assert(status == dfx::kStatus_NoError);
  assert(dataSize == sizeof(data));

  invalid();
}
