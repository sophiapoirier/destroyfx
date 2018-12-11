/*------------------------------------------------------------------------
Copyright (C) 2002-2018  Tom Murphy 7 and Sophia Poirier

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

Geometer, starring the Super Destroy FX Windowing System!
------------------------------------------------------------------------*/

#pragma once

#include <array>
#include <mutex>
#include <vector>

#include "dfxplugin.h"

/* change these for your plugins */
#define PLUGIN PLUGIN_CLASS_NAME

#if TARGET_PLUGIN_USES_DSPCORE
  #define PLUGINCORE GeometerDSP
#else
  #define PLUGINCORE PLUGIN_CLASS_NAME
#endif


/* MAX_THING gives the maximum number of things I
   ever expect to have; this affects the way the
   parameter is stored by the host.
*/

/* the types of landmark generation operations */
enum { POINT_EXTNCROSS, 
       POINT_FREQ, 
       POINT_RANDOM, 
       POINT_SPAN, 
       POINT_DYDX, 
       POINT_LEVEL,
       NUM_POINTSTYLES,
       MAX_POINTSTYLES=48
};

/* the types of waveform regeneration operations */
enum { INTERP_POLYGON, 
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
enum { OP_DOUBLE, 
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
enum { WINDOW_TRIANGLE, 
       WINDOW_ARROW, 
       WINDOW_WEDGE, 
       WINDOW_COS, 
       NUM_WINDOWSHAPES,
       MAX_WINDOWSHAPES=16
};

/* the names of the parameters */
enum { P_BUFSIZE, P_SHAPE, 
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


class PLUGIN : public DfxPlugin {
public:
  static constexpr long BUFFERSIZESSIZE = 14;

  static constexpr std::array<long, BUFFERSIZESSIZE> buffersizes {{ 
    4, 8, 16, 32, 64, 128, 256, 512, 
    1024, 2048, 4096, 8192, 16384, 32768, 
  }};

  PLUGIN(TARGET_API_BASE_INSTANCE_TYPE inInstance);

  void dfx_PostConstructor() override;

  void randomizeparameter(long inParameterIndex) override;
  std::optional<dfx::ParameterAssignment> settings_getLearningAssignData(long inParameterIndex) const override;

#if !TARGET_PLUGIN_USES_DSPCORE
  long initialize() override;
  void cleanup() override;
  void reset() override;

  void processparameters() override;
  void processaudio(float const* const* inAudio, float* const* outAudio, unsigned long inNumFrames, bool replacing = true) override;
#endif

private:
  static constexpr long NUM_PRESETS = 16;

  /* set up the built-in presets */
  void makepresets();
#if TARGET_PLUGIN_USES_DSPCORE
};

class PLUGINCORE : public DfxPluginCore {
public:
  PLUGINCORE(DfxPlugin* inDfxPlugin);

  void reset() override;
  void processparameters() override;
  void process(float const* inAudio, float* outAudio, unsigned long inNumFrames, bool replacing = true) override;

  long getwindowsize() { return third; }
#endif

  /* several of these are needed by geometerview. TODO: use accessors */
public:

  /* input and output buffers. out is framesize*2 samples long, in is framesize
     samples long. (for maximum framesize)
  */
  std::vector<float> in0, out0;

  /* buffersize is 3 * third, framesize is 2 * third 
     buffersize is used for outbuf.
  */
  long bufsize = 0, framesize = 0, third = 0;

  int processw(float * in, float * out, long samples,
	       int * px, float * py, int maxpts,
	       int * tx, float * ty);

  /* must grab this before calling processw */
  std::mutex cs;

private:

  /* third-sized tail of previous processed frame. already has mixing envelope
     applied.
   */
  std::vector<float> prevmix;

  /* number of samples in in0 */
  int insize = 0;

  /* number of samples and starting position of valid samples in out0 */
  int outsize = 0;
  int outstart = 0;

  /* ---------- geometer stuff ----------- */

  static int pointops(long pop, int npts, float op_param, int samps,
		      int * px, float * py, int maxpts,
		      int * tempx, float * tempy);

  /* shape of envelope */
  long shape {};

  long pointstyle {};
  float pointparam = 0.0f;

  long interpstyle {};
  float interparam = 0.0f;

  long pointop1 {};
  long pointop2 {};
  long pointop3 {};

  float oppar1 = 0.0f;
  float oppar2 = 0.0f;
  float oppar3 = 0.0f;

  std::vector<int> pointx;
  std::vector<float> pointy;

  std::vector<int> storex;
  std::vector<float> storey;

  std::vector<float> windowbuf;
};
