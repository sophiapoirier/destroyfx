
/* Geometer, starring the Super Destroy FX Windowing System! */

#ifndef __DFX_GEOMETER_H
#define __DFX_GEOMETER_H

#ifndef __DFXPLUGIN_H
#include "dfxplugin.h"
#endif

#ifndef __DFXMUTEX_H
#include "dfxmutex.h"
#endif

#define fsign(f) ((f<0.0)?-1.0:1.0)
#define pi (3.1415926535)

/* change these for your plugins */
#define PLUGIN Geometer

#define NUM_PRESETS 16


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

#define MKPOINTSTYLE(f)      ( paramSteppedScaled((f),   MAX_POINTSTYLES) )
#define UNMKPOINTSTYLE(i)    ( paramSteppedUnscaled((i), MAX_POINTSTYLES) )
#define MKINTERPSTYLE(f)     ( paramSteppedScaled((f),   MAX_INTERPSTYLES) )
#define UNMKINTERPSTYLE(i)   ( paramSteppedUnscaled((i), MAX_INTERPSTYLES) )
#define MKPOINTOP(f)         ( paramSteppedScaled((f),   MAX_OPS) )
#define UNMKPOINTOP(i)       ( paramSteppedUnscaled((i), MAX_OPS) )
#define MKWINDOWSHAPE(f)     ( paramSteppedScaled((f),   MAX_WINDOWSHAPES) )
#define UNMKWINDOWSHAPE(i)   ( paramSteppedUnscaled((i), MAX_WINDOWSHAPES) )


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
  friend class GeometerEditor;
public:
  PLUGIN(TARGET_API_BASE_INSTANCE_TYPE inInstance);
  virtual ~PLUGIN();

  virtual long initialize();
  virtual void cleanup();
  virtual void reset();

  virtual void processparameters();
  virtual void processaudio(const float **in, float **out, unsigned long inNumFrames, bool replacing=true);

  /* several of the below are needed by geometerview. Maybe should use accessors... */

  long getwindowsize() { return third; }

  /* input and output buffers. out is framesize*2 samples long, in is framesize
     samples long. (for maximum framesize)
  */
  float * in0, * out0;

  /* buffersize is 3 * third, framesize is 2 * third 
     buffersize is used for outbuf.
  */
  long bufsize, framesize, third;

  /* must grab this before calling processw */
  dfxmutex * cs;

  int processw(float * in, float * out, long samples,
	       int * px, float * py, int maxpts,
	       int * tx, float * ty);

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

  #define BUFFERSIZESSIZE 14
  static const long buffersizes[BUFFERSIZESSIZE];

  /* 1 if need to do ioChanged since buffer settings are different now */
  int changed;

  /* ---------- geometer stuff ----------- */

  static int pointops(long pop, int npts, float op_param, int samps,
		      int * px, float * py, int maxpts,
		      int * tempx, float * tempy);

  /* set up the built-in presets */
  void makepresets();

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
};

#endif
