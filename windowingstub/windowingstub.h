
/* Windowingstub, starring the Super Destroy FX Windowing System! */

#pragma once

#include "dfxplugin.h"

#include <vector>

/* change these for your plugins */
#define PLUGIN Windowingstub
#define PLUGINCORE WindowingstubDSP


static constexpr long buffersizes[] = { 
  4, 8, 16, 32, 64, 128, 256, 512, 
  1024, 2048, 4096, 8192, 16384, 32768, 
};
static constexpr long BUFFERSIZESSIZE = std::size(buffersizes);

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
       NUM_PARAMS,
};


class PLUGIN final : public DfxPlugin {
public:
  explicit PLUGIN(TARGET_API_BASE_INSTANCE_TYPE inInstance);

private:
  static constexpr long NUM_PRESETS = 16;

  /* set up the built-in presets */
  void makepresets();
};

class PLUGINCORE final : public DfxPluginCore {
public:
  explicit PLUGINCORE(DfxPlugin * inInstance);

  void reset() override;
  void processparameters() override;
  void process(float const * in, float * out, unsigned long inNumFrames) override;

  long getwindowsize() const noexcept { return third; }

 private:

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

};
