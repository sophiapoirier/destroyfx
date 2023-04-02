/*------------------------------------------------------------------------
Copyright (C) 2005-2023  Tom Murphy 7

This file is part of Slowft.

Slowft is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Slowft is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Slowft.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org

Slowft, starring the Super Destroy FX Windowing System!
------------------------------------------------------------------------*/

#pragma once

#include <array>
#include <numbers>
#include <vector>

#include "dfxplugin.h"

/* change these for your plugins */
#define PLUGIN Slowft

static constexpr std::array buffersizes {
  4, 8, 16, 32, 64, 128, 256, 512, 
  1024, 2048, 4096, 8192, 16384, 32768, 
};


#define PLUGINCORE SlowftDSP


/* the types of window shapes available for smoothity */
enum { WINDOW_TRIANGLE, 
       WINDOW_ARROW, 
       WINDOW_WEDGE, 
       WINDOW_COS, 
       NUM_WINDOWSHAPES,
       MAX_WINDOWSHAPES=16
};

/* the names of the parameters */
enum : dfx::ParameterID { P_BUFSIZE, P_SHAPE, 
                          NUM_PARAMS
};


class PLUGIN final : public DfxPlugin {
public:
  PLUGIN(TARGET_API_BASE_INSTANCE_TYPE inInstance);

private:
  static constexpr size_t NUM_PRESETS = 16;

  /* set up the built-in presets */
  void makepresets();
};

class PLUGINCORE final : public DfxPluginCore {
public:
  PLUGINCORE(DfxPlugin& inInstance);

  void reset() override;
  void processparameters() override;
  void process(std::span<float const> in, std::span<float> out) override;

  long getwindowsize() const noexcept { return third; }

private:
  static constexpr float BASE_FREQ = 27.5f;
  static constexpr size_t NUM_KEYS = 88;
  static constexpr float HALFSTEP_RATIO = 1.05946309436f;
  static constexpr float SLOWFT_2PI = std::numbers::pi_v<float> * 2.f;

  /* input and output buffers. out is framesize*2 samples long, in is framesize
     samples long. (for maximum framesize)
  */
  std::vector<float> in0, out0;

  /* bufsize is 3 * third, framesize is 2 * third 
     bufsize is used for outbuf.
  */
  long bufsize = 0, framesize = 0, third = 0;

  void processw(float const * in, float * out, long samples);

  int shape = 0;

  /* third-sized tail of previous processed frame. already has mixing envelope
     applied.
   */
  std::vector<float> prevmix;

  /* number of samples in in0 */
  int insize = 0;

  /* number of samples and starting position of valid samples in out0 */
  int outsize = 0;
  int outstart = 0;


  /* the transformed data */
  float sines[NUM_KEYS] {};
  float cosines[NUM_KEYS] {};

};
