
/* Windowingstub, starring the Super Destroy FX Windowing System! */

#ifndef __DFX_WINDOWINGSTUB_H
#define __DFX_WINDOWINGSTUB_H

#include "dfxplugin.h"

/* change these for your plugins */
#define PLUGIN Windowingstub
#define NUM_PRESETS 16

#define BUFFERSIZESSIZE 14
const long buffersizes[BUFFERSIZESSIZE] = { 
  4, 8, 16, 32, 64, 128, 256, 512, 
  1024, 2048, 4096, 8192, 16384, 32768, 
};


#define PLUGINCORE WindowingstubDSP

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


class PLUGIN : public DfxPlugin {
public:
  PLUGIN(TARGET_API_BASE_INSTANCE_TYPE inInstance);
  virtual ~PLUGIN();

private:
  /* set up the built-in presets */
  void makepresets();
};

class PLUGINCORE : public DfxPluginCore {
public:
  PLUGINCORE(DfxPlugin * inInstance);
  virtual ~PLUGINCORE();

  virtual void reset();
  virtual void processparameters();
  virtual void process(const float *in, float *out, unsigned long inNumFrames, bool replacing=true);

  long getwindowsize() { return third; }

 private:

  /* input and output buffers. out is framesize*2 samples long, in is framesize
     samples long. (for maximum framesize)
  */
  float * in0, * out0;

  /* bufsize is 3 * third, framesize is 2 * third 
     bufsize is used for outbuf.
  */
  long bufsize, framesize, third;

  void processw(float * in, float * out, long samples);

  int shape;

  /* third-sized tail of previous processed frame. already has mixing envelope
     applied.
   */
  float * prevmix;

  /* number of samples in in0 */
  int insize;

  /* number of samples and starting position of valid samples in out0 */
  int outsize;
  int outstart;

};

#endif
