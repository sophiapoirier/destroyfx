
/* Geometer, starring the Super Destroy FX Windowing System! */

#ifndef __DFX_GEOMETER_H
#define __DFX_GEOMETER_H

#include "dfxplugin.h"

#include "dfxmutex.h"

/* change these for your plugins */
#define PLUGIN Geometer
const long NUM_PRESETS = 16;

const long BUFFERSIZESSIZE = 14;
const long buffersizes[BUFFERSIZESSIZE] = { 
  4, 8, 16, 32, 64, 128, 256, 512, 
  1024, 2048, 4096, 8192, 16384, 32768, 
};

#if TARGET_PLUGIN_USES_DSPCORE
  #define PLUGINCORE GeometerDSP
#else
  #define PLUGINCORE Geometer
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
  PLUGIN(TARGET_API_BASE_INSTANCE_TYPE inInstance);
  virtual ~PLUGIN();

#if !TARGET_PLUGIN_USES_DSPCORE
  virtual long initialize();
  virtual void cleanup();
  virtual void reset();

  virtual void processparameters();
  virtual void processaudio(const float **in, float **out, unsigned long inNumFrames, bool replacing=true);
#endif

private:
  /* set up the built-in presets */
  void makepresets();
#if TARGET_PLUGIN_USES_DSPCORE
};

class PLUGINCORE : public DfxPluginCore {
public:
  PLUGINCORE(DfxPlugin *inDfxPlugin);
  virtual ~PLUGINCORE();

  virtual void reset();
  virtual void processparameters();
  virtual void process(const float *in, float *out, unsigned long inNumFrames, bool replacing=true);

  long getwindowsize() { return third; }
#endif

  /* several of these are needed by geometerview. Maybe should use accessors... */
public:

  /* input and output buffers. out is framesize*2 samples long, in is framesize
     samples long. (for maximum framesize)
  */
  float * in0, * out0;

  /* buffersize is 3 * third, framesize is 2 * third 
     buffersize is used for outbuf.
  */
  long bufsize, framesize, third;

  int processw(float * in, float * out, long samples,
	       int * px, float * py, int maxpts,
	       int * tx, float * ty);

  /* must grab this before calling processw */
  DfxMutex * cs;

private:

  /* third-sized tail of previous processed frame. already has mixing envelope
     applied.
   */
  float * prevmix;

  /* number of samples in in0 */
  int insize;

  /* number of samples and starting position of valid samples in out0 */
  int outsize;
  int outstart;

  /* ---------- geometer stuff ----------- */

  static int pointops(long pop, int npts, float op_param, int samps,
		      int * px, float * py, int maxpts,
		      int * tempx, float * tempy);

  /* shape of envelope */
  long shape;

  long pointstyle;
  float pointparam;

  long interpstyle;
  float interparam;

  long pointop1;
  long pointop2;
  long pointop3;

  float oppar1;
  float oppar2;
  float oppar3;

  int lastx;
  float lasty;

  int * pointx;
  float * pointy;

  int * storex;
  float * storey;
float * windowbuf;
};

#endif
