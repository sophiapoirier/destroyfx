
/* Geometer, starring the Super Destroy FX Windowing System! */

#ifndef __DFX_GEOMETER_H
#define __DFX_GEOMETER_H

#include "dfxmisc.h"
#include "vstchunk.h"

#ifdef WIN32
/* turn off warnings about default but no cases in switch, etc. */
   #pragma warning( disable : 4065 57 4200 4244 )
   #include <windows.h>
#endif

#define fsign(f) ((f<0.0)?-1.0:1.0)
#define pi (3.1415926535)

/* change these for your plugins */
#define PLUGIN Geometer
#define PLUGINID 'DFgr'
#define PLUGINNAME "DFX GEOMETER"
#define PLUGINPROGRAM GeometerProg

#define NUM_PROGRAMS 16

#define GUI

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


struct param {
  float * ptr;
  const char * name;
  const char * units;
  float def;
};

class PLUGINPROGRAM {
  friend class PLUGIN;
public:
  PLUGINPROGRAM() { strcpy(name, "default"); }
  ~PLUGINPROGRAM() { }
  void init(class PLUGIN *);

private:

  char name[32];
  float param[NUM_PARAMS];
};

class PLUGIN : public AudioEffectX {
  friend class PLUGINPROGRAM;
public:
  PLUGIN(audioMasterCallback audioMaster);
  ~PLUGIN();

  virtual void processX(float **inputs, float **outputs, long sampleFrames,
                       int replacing);
  virtual void process(float **inputs, float **outputs, long sampleFrames);
  virtual void processReplacing(float **inputs, float **outputs, 
                                long sampleFrames);

  virtual void setParameter(long index, float value);
  virtual float getParameter(long index);
  virtual void getParameterLabel(long index, char *label);
  virtual void getParameterDisplay(long index, char *text);
  virtual void getParameterName(long index, char *text);

  virtual void setProgram(long programNum);
  virtual void setProgramName(char *name);
  virtual void getProgramName(char *name);
  virtual bool getProgramNameIndexed(long category, long index, char *text);
  virtual bool copyProgram(long destination);

  virtual long getTailSize();
  /* there was a typo in the VST header files versions 2.0 through 2.2, 
     so some hosts will still call this incorrectly named version... */
  virtual long getGetTailSize() { return getTailSize(); }

  virtual long canDo(char* text);

  virtual void suspend();
  virtual void resume();

  virtual long processEvents(VstEvents* events);
  virtual long getChunk(void **data, bool isPreset);
  virtual long setChunk(void *data, long byteSize, bool isPreset);

  bool getVendorString(char *text) {
    strcpy (text, "Destroy FX");
    return true; 
  }

  bool getProductString(char *text) {
    strcpy (text, "Super Destroy FX bipolar VST plugin pack");
    return true; 
  }

  bool getEffectName(char *name) {
    strcpy (name, PLUGINNAME);
    return true; 
  }

  void setup() {
    programs = new PLUGINPROGRAM[NUM_PROGRAMS];
    for(int i = 0; i < NUM_PROGRAMS; i++) programs[i].init(this);
    setProgram(0);
    strcpy(programs[0].name, "Geometer LoFi");
    makepresets();

    setNumInputs(1);            /* mono in/out */
    setNumOutputs(1);
    setUniqueID(PLUGINID);

    canProcessReplacing();
  }

  /* this stuff is public so that the GUI can see it */
  VstChunk *chunk;      // chunky data full of parameter settings & stuff

protected:
  /* stores info for each parameter */
  param paramptrs[NUM_PARAMS];

  /* stores programs */
  PLUGINPROGRAM *programs;

  /* shape of envelope */
  float shape;

  /* size of buffer (param) */
  float bufsizep;

public:
  /* several of these are needed by geometerview. Maybe should use accessors... */

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
  static const int buffersizes[BUFFERSIZESSIZE];

  /* 1 if need to do ioChanged since buffer settings are different now */
  int changed;

  /* set up the built-in presets */
  void makepresets();

  /* ---------- geometer stuff ----------- */

  static int pointops(float pop, int npts, float * op_param, int samps,
		      int * px, float * py, int maxpts,
		      int * tempx, float * tempy);

  float pointstyle;
  float pointparam[MAX_POINTSTYLES];

  float interpstyle;
  float interparam[MAX_INTERPSTYLES];

  float pointop1;
  float pointop2;
  float pointop3;

  float oppar1[MAX_OPS];
  float oppar2[MAX_OPS];
  float oppar3[MAX_OPS];

  int lastx;
  float lasty;

  int * storex;
  float * storey;


public:
  int * pointx;
  float * pointy;

  int processw(float * in, float * out, long samples,
	       int * px, float * py, int maxpts,
	       int * tx, float * ty);
};

#define FPARAM(pname, idx, nm, init, un) do { paramptrs[idx].ptr = &pname; paramptrs[idx].name = (nm); paramptrs[idx].units = (un); paramptrs[idx].def = (init); pname = paramptrs[idx].def; } while (0)

#endif
