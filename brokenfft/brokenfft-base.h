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


// XXX these enums are all from geometer

/* MAX_THING gives the maximum number of things I
   ever expect to have; this affects the way the
   parameter is stored by the host.
*/

/* the types of landmark generation operations */
enum {
  POINT_EXTNCROSS, 
  POINT_FREQ, 
  POINT_RANDOM, 
  POINT_SPAN, 
  POINT_DYDX, 
  POINT_LEVEL,
  NUM_POINTSTYLES,
  MAX_POINTSTYLES=48
};

/* the types of waveform regeneration operations */
enum {
  INTERP_POLYGON, 
  INTERP_WRONGYGON, 
  INTERP_SMOOTHIE, 
  INTERP_REVERSI, 
  INTERP_PULSE, 
  INTERP_FRIENDS, 
  INTERP_SING,
  INTERP_SHUFFLE,
  NUM_INTERPSTYLES,
  MAX_INTERPSTYLES=48
};

/* the types of operations on points */
enum {
  OP_DOUBLE, 
  OP_HALF, 
  OP_QUARTER, 
  OP_LONGPASS, 
  OP_SHORTPASS, 
  OP_SLOW, 
  OP_FAST, 
  OP_NONE, 
  NUM_OPS,
  MAX_OPS=48
};

/* the types of window shapes available for smoothity */
enum {
  WINDOW_TRIANGLE, 
  WINDOW_ARROW, 
  WINDOW_WEDGE, 
  WINDOW_COS, 
  NUM_WINDOWSHAPES,
  MAX_WINDOWSHAPES=16
};

/* the names of the parameters */
enum {
  P_BUFSIZE, P_SHAPE, 
  P_POINTSTYLE, 
  P_POINTPARAMS,
  P_INTERPSTYLE = P_POINTPARAMS + MAX_POINTSTYLES,
  P_INTERPARAMS,
  P_POINTOP1 = P_INTERPARAMS + MAX_INTERPSTYLES,
  P_OPPAR1S,
  P_POINTOP2 = P_OPPAR1S + MAX_OPS,
  P_OPPAR2S,
  P_POINTOP3 = P_OPPAR2S + MAX_OPS,
  P_OPPAR3S,
  NUM_PARAMS = P_OPPAR3S + MAX_OPS
};

enum : dfx::PropertyID {
  PROP_LAST_WINDOW_TIMESTAMP = dfx::kPluginProperty_EndOfList,
  PROP_WAVEFORM_DATA
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
