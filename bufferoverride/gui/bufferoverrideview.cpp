/*------------------------------------------------------------------------
Copyright (C) 2021  Tom Murphy 7 and Sophia Poirier

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

#include <algorithm>
#include <cassert>
#include <cmath>
#include <utility>

#include "bufferoverride-base.h"

constexpr auto color_dark = VSTGUI::MakeCColor(0x6f, 0x3f, 0x00);
constexpr auto color_med = VSTGUI::MakeCColor(0xc8, 0x91, 0x3e);
constexpr auto color_lite = VSTGUI::MakeCColor(0xe8, 0xd0, 0xaa);
constexpr auto color_red = VSTGUI::MakeCColor(0xde, 0x7c, 0x70);

using VSTGUI::CPoint;
using VSTGUI::CColor;
using VSTGUI::CCoord;
using VSTGUI::UTF8StringPtr;

BufferOverrideView::BufferOverrideView(VSTGUI::CRect const & size)
  : VSTGUI::CView(size) {

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


  const CCoord buffer_sec = data.forced_buffer_sec;
  const CCoord minibuffer_sec = data.minibuffer_sec;

  offc->beginDraw();

  // Clear background.
  offc->setFillColor(color_dark);
  offc->drawRect(VSTGUI::CRect(-1, -1, width, height), VSTGUI::kDrawFilled);


  constexpr CCoord MARGIN_HORIZ = 8.0;
  constexpr CCoord MARGIN_VERT = 16.0;
  // We want to fit roughly one second, but:
  //   - Remove margin on left, since we don't draw there
  //   - Remove the same margin on right. We do draw there, but
  //     this lets us see the beginning of the next buffer when
  //     at the extreme buffer size of 1 sec.
  const CCoord pixels_per_second = width - (MARGIN_HORIZ * 2);

  auto DrawBox = [this](CCoord x, CCoord y, CCoord w, CCoord h,
                        CColor c) {
      offc->setFrameColor(c);
      // top
      offc->drawLine(CPoint(x, y), CPoint(x + w - 1, y));
      // bottom
      // (Note that we draw one additional pixel here for the bottom-right
      // corner, because I guess vstgui does not include the endpoint.)
      offc->drawLine(CPoint(x, y + h - 1), CPoint(x + w - 1 + 1, y + h - 1));
      // left
      offc->drawLine(CPoint(x, y), CPoint(x, y + h - 1));
      // right
      offc->drawLine(CPoint(x + w - 1, y), CPoint(x + w - 1, y + h - 1));
    };

  auto DrawFilledBox = [this](CCoord x, CCoord y, CCoord w, CCoord h,
                              CColor c) {
      offc->setFillColor(c);
      offc->drawRect(VSTGUI::CRect(x, y, x + w, y + h),
                     VSTGUI::kDrawFilled);
    };

  // draw 'major' boxes.
  const CCoord majorbox_width = std::max(pixels_per_second * buffer_sec, 4.);
  const CCoord majorbox_height = height - (MARGIN_VERT * 2);

  const CCoord minorbox_width = std::clamp(pixels_per_second * minibuffer_sec, 2., majorbox_width);
  const CCoord minorbox_height = majorbox_height - 4;
  {
    // Place boxes in float space but round to integer coordinates
    // since we leave some thin pixel borders.
    for (CCoord xpos = MARGIN_HORIZ; xpos < width; xpos += majorbox_width) {
      const int ixpos = std::lround(xpos);
      const int majw = std::lround(xpos + majorbox_width) - xpos;

      // TODO: When the major buffer size is really small, this draws
      // occasional slivers (looks ok) or nothing (probably could be
      // improved). Maybe some kind of "gitching" here.
      if (majw > 0) {
        DrawBox(ixpos, MARGIN_VERT, majw, majorbox_height, color_lite);

        // If minibuffers are too narrow to fit, pinstripes.
        // (could consider other effects here... "ERR0R" etc.)
        if (minorbox_width < 2) {
          bool first = true;

          for (int x = 2; x < majw - 1; x += 2) {
            offc->setFrameColor(first ? color_lite : color_red);
            offc->drawLine(
                CPoint(ixpos + x, MARGIN_VERT + 2),
                CPoint(ixpos + x, MARGIN_VERT + 2 + minorbox_height));
            first = false;
          }
        } else {
          bool first = true;
          for (CCoord nxpos = 2.0; nxpos < majw - 2; nxpos += minorbox_width) {
            const int inxpos = std::lround(nxpos);
            const int minw = std::lround(nxpos + minorbox_width) - inxpos;

            // Clip the width of the last minibuffer.
            // We can reuse the right margin by overlapping it with the
            // space after the last box, so just majw - 1.
            const int w = std::min(minw, (majw - 1) - inxpos);

            DrawFilledBox(ixpos + inxpos, MARGIN_VERT + 2,
                          // leave space between boxes
                          w - 1, minorbox_height,
                          first ? color_lite : color_med);

            first = false;
          }
        }
      }

    }
  }

  offc->endDraw();
  offc->copyFrom(ctx, getViewSize());

  setDirty(false);
}


void BufferOverrideView::onIdle() {
  assert(editor);
  auto const timestamp =
    editor->dfxgui_GetProperty<uint64_t>(
        kBOProperty_LastViewDataTimestamp).value_or(prevtimestamp);
  if (std::exchange(prevtimestamp, timestamp) != timestamp) {
    reflect();
  }
}


void BufferOverrideView::reflect() {
  // When idle, update copies of the parameters from the editor.
  // We don't copy the parameter values directly (the other UI elements
  // already display them, not to mention the x/y slider) but rather
  // the current effective values after the application of LFOs.
  // This also leaves open the possibility of rendering some waveform
  // data (a la GeometerView).

  assert(editor);

  size_t data_size = sizeof (data);
  [[maybe_unused]] const auto status = editor->dfxgui_GetProperty(
      kBOProperty_ViewData, dfx::kScope_Global, 0, &data, data_size);
  assert(status == dfx::kStatus_NoError);
  assert(data_size == sizeof (data));

  // TODO: Only if changed?
  invalid();
}
