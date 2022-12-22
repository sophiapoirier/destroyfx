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

#include "bufferoverrideview.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numeric>
#include <tuple>
#include <utility>

#include "bufferoverride-base.h"

using VSTGUI::CColor;
using VSTGUI::CCoord;
using VSTGUI::CPoint;

constexpr auto color_dark = VSTGUI::MakeCColor(0x6f, 0x3f, 0x00);
constexpr auto color_med = VSTGUI::MakeCColor(0xc8, 0x91, 0x3e);
constexpr auto color_lite = VSTGUI::MakeCColor(0xe8, 0xd0, 0xaa);
constexpr auto color_legend = VSTGUI::MakeCColor(0xc6, 0x85, 0x3d);

constexpr CCoord FORCED_BUFFER_WIDTH_MIN = 4.;
constexpr CCoord MINIBUFFER_WIDTH_MIN = 2.;

BufferOverrideView::BufferOverrideView(VSTGUI::CRect const &size)
  : VSTGUI::CView(size) {

  setWantsIdle(true);
}


bool BufferOverrideView::attached(VSTGUI::CView *parent) {

  const auto success = VSTGUI::CView::attached(parent);

  if (success) {
    editor = dynamic_cast<DfxGuiEditor*>(getEditor());
    assert(editor);
    offc = VSTGUI::COffscreenContext::create({getWidth(), getHeight()});
    if (const auto font =
        editor->CreateVstGuiFont(dfx::kFontSize_Pasement9px,
                                 dfx::kFontName_Pasement9px)) {
      offc->setFont(font);
    }

    reflect();
  }

  return success;
}


bool BufferOverrideView::removed(VSTGUI::CView *parent) {

  editor = nullptr;
  offc = {};

  return VSTGUI::CView::removed(parent);
}


static std::tuple<CCoord, CCoord, CCoord>
ScaleViewData(BufferOverrideViewData data, CCoord pixels_per_window) {

  constexpr CCoord window_sec_default = 1;
  auto window_sec = window_sec_default;

  constexpr auto Scale = [](CCoord value, CCoord window) {
    return value * (window_sec_default / window);
  };

  // calculate scale using the pre-LFO values so that it remains
  // stable when not adjusting parameters
  const CCoord forced_buffer = data.mPreLFO.mForcedBufferSeconds;
  const CCoord minibuffer = data.mPreLFO.mMinibufferSeconds;

  // zoom out when a single forced buffer exceeds the default window
  while (forced_buffer > window_sec) {
    window_sec = std::round(window_sec * 2);
  }

  // if the mini-buffers are below the pixel threshold and
  // the forced buffer size is less than the default window,
  // zoom in (but not smaller than the forced buffer size)
  if (forced_buffer < window_sec_default) {
    const auto Pixels = [=](CCoord value, CCoord window) {
      return Scale(value, window) * pixels_per_window;
    };
    while (Pixels(minibuffer, window_sec) < MINIBUFFER_WIDTH_MIN) {
      const auto next_window_sec = window_sec / 2.;
      if (next_window_sec < forced_buffer) {
        break;
      }
      window_sec = next_window_sec;
    }
  }

  return {
    Scale(data.mPostLFO.mForcedBufferSeconds, window_sec),
    Scale(data.mPostLFO.mMinibufferSeconds, window_sec),
    window_sec
  };
}

void BufferOverrideView::draw(VSTGUI::CDrawContext *ctx) {

  assert(offc);

  const auto width = getWidth();
  const auto height = getHeight();


  offc->beginDraw();
  offc->setLineWidth(1);

  // Clear background.
  offc->setFillColor(color_dark);
  offc->drawRect(VSTGUI::CRect(-1, -1, width, height), VSTGUI::kDrawFilled);


  constexpr CCoord MARGIN_HORIZ = 8.0;
  constexpr CCoord MARGIN_TOP = 9.0;
  constexpr CCoord MARGIN_BOTTOM = 23.0;
  // We want to fit roughly one second, but:
  //   - Remove margin on left, since we don't draw there
  //   - Remove the same margin on right. We do draw there, but
  //     this lets us see the beginning of the next buffer when
  //     at the extreme buffer size of 1 sec.
  const CCoord pixels_per_window = width - (MARGIN_HORIZ * 2);
  assert(pixels_per_window > 0);

  const auto DrawBox = [this](CCoord x, CCoord y, CCoord w, CCoord h,
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

  const auto DrawFilledBox = [this](CCoord x, CCoord y, CCoord w, CCoord h,
                                    CColor c) {
      offc->setFillColor(c);
      offc->drawRect(DGRect(x, y, w, h), VSTGUI::kDrawFilled);
    };

  const auto [forced_buffer, minibuffer, window_sec] =
    ScaleViewData(data, pixels_per_window);
  const auto decay_depth = editor->getparameter_f(kDecayDepth) * 0.01;
  const auto decay_shape = static_cast<DecayShape>(editor->getparameter_i(kDecayShape));

  // draw 'major' boxes.
  const CCoord majorbox_width = std::max(pixels_per_window * forced_buffer,
                                         FORCED_BUFFER_WIDTH_MIN);
  const CCoord majorbox_height = height - (MARGIN_TOP + MARGIN_BOTTOM);

  const CCoord minorbox_width = std::clamp(pixels_per_window * minibuffer,
                                           MINIBUFFER_WIDTH_MIN,
                                           majorbox_width);
  const CCoord minorbox_height = majorbox_height - 4;

  // Place boxes in float space but round to integer coordinates
  // since we leave some thin pixel borders.
  for (CCoord xpos = MARGIN_HORIZ; xpos < width; xpos += majorbox_width) {
    const auto ixpos = std::lround(xpos);
    const auto majw = std::lround(xpos + majorbox_width - xpos);

    DrawBox(ixpos, MARGIN_TOP, majw, majorbox_height, color_lite);

    auto minorbox_color = color_lite;
    constexpr CCoord xstart = 2;
    const CCoord xend = majw - xstart;
    // TODO: reflect that a fractional minibuffer extends the final one
    for (CCoord nxpos = xstart; nxpos < xend; nxpos += minorbox_width) {
      const auto inxpos = std::lround(nxpos);
      const auto minw = std::lround(nxpos + minorbox_width) - inxpos;

      // Clip the width of the last minibuffer.
      // We can reuse the right margin by overlapping it with the
      // space after the last box, so just majw - 1.
      const auto w = std::min(minw, (majw - 1) - inxpos);

      const auto pos_normalized = (nxpos - xstart) / (xend - xstart);
      const auto decay = GetBufferDecay(pos_normalized, decay_depth,
                                        decay_shape, random_engine);
      const auto decayed_height = std::round(minorbox_height * decay);
      const auto ypos = MARGIN_TOP + 2 + (minorbox_height - decayed_height);
      DrawFilledBox(ixpos + inxpos, ypos,
                    // leave space between boxes
                    w - 1, decayed_height,
                    minorbox_color);

      minorbox_color = color_med;
    }
  }

  // draw the time-scale legend
  constexpr CCoord LEGEND_MARGIN_TOP = 7;
  constexpr CCoord LEGEND_MARGIN_BOTTOM = 8;
  const auto legend_height = MARGIN_BOTTOM -
    (LEGEND_MARGIN_TOP + LEGEND_MARGIN_BOTTOM);
  assert(legend_height > 0);
  const auto legend_top = height - LEGEND_MARGIN_BOTTOM - legend_height;
  const auto legend_bottom = legend_top + legend_height;
  constexpr auto legend_left = MARGIN_HORIZ;
  const auto legend_right = width - MARGIN_HORIZ;
  const auto legend_width = legend_right - legend_left;
  constexpr CCoord LEGEND_DASH_STRIDE = 2;
  constexpr CCoord LEGEND_DASH_WIDTH = 1;
  const auto legend_dash_y = std::floor(std::midpoint(legend_top,
                                                      legend_bottom));

  // legend bounding
  offc->setFrameColor(color_legend);
  offc->drawLine({legend_left, legend_top},
                 {legend_left, legend_bottom});
  offc->drawLine({legend_right, legend_top},
                 {legend_right, legend_bottom});
  for (auto x = legend_left + LEGEND_DASH_STRIDE;
       x <= (legend_right - LEGEND_DASH_STRIDE);
       x += LEGEND_DASH_STRIDE) {
    DrawFilledBox(x, legend_dash_y,
                  LEGEND_DASH_WIDTH, LEGEND_DASH_WIDTH,
                  color_legend);
  }

  // legend label
  const auto legend_label = (window_sec < 1) ?
    (std::to_string(std::lround(window_sec * 1000)) + " ms") :
    (std::to_string(std::lround(window_sec)) + " sec");
  constexpr bool LABEL_ANTIALIAS = false;
  constexpr CCoord LABEL_PADDING = 12;
  const auto label_width = offc->getStringWidth(legend_label.c_str()) +
    (LABEL_PADDING * 2);
  const auto label_x = legend_left +
    ((legend_width - label_width) / 2);
  const DGRect text_area(label_x, legend_top + 1, label_width,
                         dfx::kFontContainerHeight_Pasement9px);
  offc->setFillColor(color_dark);
  offc->drawRect(text_area, VSTGUI::kDrawFilled);
  offc->setFontColor(color_lite);
  const auto adjusted_text_area = detail::AdjustTextViewForPlatform(
      offc->getFont()->getName(), text_area);
  offc->drawString(legend_label.c_str(), adjusted_text_area,
                   VSTGUI::kCenterText, LABEL_ANTIALIAS);


  offc->endDraw();
  offc->copyFrom(ctx, getViewSize());

  setDirty(false);
}


void BufferOverrideView::onIdle() {
  assert(editor);
  const auto timestamp =
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

  invalid();
}
