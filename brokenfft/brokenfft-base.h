/*------------------------------------------------------------------------
Copyright (C) 2002-2020  Tom Murphy 7 and Sophia Poirier

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

#ifndef _DFX_BROKENFFT_BASE_H
#define _DFX_BROKENFFT_BASE_H

#include <array>
#include <type_traits>

#include "dfxpluginproperties.h"


/* MAX_THING gives the maximum number of things I
   ever expect to have; this affects the way the
   parameter is stored by the host.
*/

/* the types of window shapes available for smoothity */
enum {
  WINDOW_TRIANGLE,
  WINDOW_ARROW,
  WINDOW_WEDGE,
  WINDOW_BEST,
  WINDOW_COS2,
  NUM_WINDOWSHAPES,
  MAX_WINDOWSHAPES = 16,
};

enum {
  METHOD_DC,
  METHOD_WEST,
  METHOD_WEST_BUG,
  NUM_METHODS,
  MAX_METHODS = 48,
};

// The parameters.
enum {
  P_BUFSIZE,
  P_SHAPE,
  P_METHOD,
  P_DESTRUCT,
  P_PERTURB,
  P_QUANT,
  P_ROTATE,
  P_BINQUANT,
  P_SPIKE,
  P_SPIKEHOLD,
  P_COMPRESS,
  P_MUG,
  P_ECHOMIX,
  P_ECHOTIME,
  P_ECHOMODF,
  P_ECHOMODW,
  P_ECHOFB,
  P_ECHOLOW,
  P_ECHOHI,
  P_POSTROT,
  P_LOWP,
  P_MOMENTS,
  P_BRIDE,
  P_BLOW,
  P_CONV,
  P_HARM,
  P_ALOW,
  P_NORM,
  NUM_PARAMS
};


enum : dfx::PropertyID {
  PROP_LAST_WINDOW_TIMESTAMP = dfx::kPluginProperty_EndOfList,
  PROP_FFT_DATA
};

// XXX all geometer stuff
//
// If we keep this, it would be some visualization of the frequency buckets
// (Or maybe useful would be the pre-effect buckets on top, post-effect buckets
// aligned on bottom?)
struct BrokenFFTViewData {
  static constexpr int samples = 476;
  static constexpr auto arraysize = static_cast<size_t>(samples + 3);

  int32_t numpts = 0;

  /* active points */
  int32_t apts = 0;

  std::array<float, arraysize> inputs;
  std::array<int32_t, arraysize> pointsx;
  std::array<float, arraysize> pointsy;
  std::array<float, arraysize> outputs;

  BrokenFFTViewData() {
    clear();
  }

  void clear() {
    numpts = apts = 0;
    inputs.fill(0.0f);
    pointsx.fill(0);
    pointsy.fill(0.0f);
    outputs.fill(0.0f);
  }
};

static_assert(std::is_trivially_copyable_v<BrokenFFTViewData> && std::is_standard_layout_v<BrokenFFTViewData>);

#endif
