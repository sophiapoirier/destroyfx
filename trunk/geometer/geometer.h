
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


/* the types of landmark generation operations */
enum { POINT_EXTNCROSS, 
       POINT_FREQ, 
       POINT_RANDOM, 
       POINT_SPAN, 
       POINT_DYDX, 
       POINT_UNSUP1, 
       POINT_UNSUP2, 
       POINT_UNSUP3, 
       NUM_POINTSTYLES
};

/* the types of waveform regeneration operations */
enum { INTERP_POLYGON, 
       INTERP_WRONGYGON, 
       INTERP_SMOOTHIE, 
       INTERP_REVERSI, 
       INTERP_PULSE, 
       INTERP_FRIENDS, 
       INTERP_SING,
       INTERP_UNSUP3, 
       NUM_INTERPSTYLES
};

/* the types of operations on points */
enum { OP_DOUBLE, 
       OP_HALF, 
       OP_QUARTER, 
       OP_LONGPASS, 
       OP_SHORTPASS, 
       OP_UNSUP1, 
       OP_UNSUP2, 
       OP_UNSUP3, 
       NUM_OPS
};

/* the types of window shapes available for smoothity */
enum { WINDOW_TRIANGLE, 
       WINDOW_ARROW, 
       WINDOW_WEDGE, 
       WINDOW_COS, 
       WINDOW_COS2, 
       WINDOW_UNSUP1, 
       WINDOW_UNSUP2, 
       WINDOW_UNSUP3, 
       NUM_WINDOWSHAPES
};

#define MKPOINTSTYLE(f)   ( paramSteppedScaled((f), NUM_POINTSTYLES) )
#define UNMKPOINTSTYLE(i)   ( paramSteppedUnscaled((i), NUM_POINTSTYLES) )
#define MKINTERPSTYLE(f)   ( paramSteppedScaled((f), NUM_INTERPSTYLES) )
#define UNMKINTERPSTYLE(i)   ( paramSteppedUnscaled((i), NUM_INTERPSTYLES) )
#define MKPOINTOP(f)   ( paramSteppedScaled((f), NUM_OPS) )
#define UNMKPOINTOP(i)   ( paramSteppedUnscaled((i), NUM_OPS) )
#define MKWINDOWSHAPE(f)   ( paramSteppedScaled((f), NUM_WINDOWSHAPES) )
#define UNMKWINDOWSHAPE(i)   ( paramSteppedUnscaled((i), NUM_WINDOWSHAPES) )


/* the names of the parameters */
enum { P_BUFSIZE, P_SHAPE, 
       P_POINTSTYLE, 
        P_POINTPARAM0,
        P_POINTPARAM1,
        P_POINTPARAM2,
        P_POINTPARAM3,
        P_POINTPARAM4,
        P_POINTPARAM5,
        P_POINTPARAM6,
        P_POINTPARAM7,
       P_INTERPSTYLE,
        P_INTERPARAM0,
        P_INTERPARAM1,
        P_INTERPARAM2,
        P_INTERPARAM3,
        P_INTERPARAM4,
        P_INTERPARAM5,
        P_INTERPARAM6,
        P_INTERPARAM7,
       P_POINTOP1, 
        P_OPPAR1_0, 
        P_OPPAR1_1, 
        P_OPPAR1_2, 
        P_OPPAR1_3, 
        P_OPPAR1_4, 
        P_OPPAR1_5, 
        P_OPPAR1_6, 
        P_OPPAR1_7, 
       P_POINTOP2, 
        P_OPPAR2_0, 
        P_OPPAR2_1, 
        P_OPPAR2_2, 
        P_OPPAR2_3, 
        P_OPPAR2_4, 
        P_OPPAR2_5, 
        P_OPPAR2_6, 
        P_OPPAR2_7, 
       P_POINTOP3,
        P_OPPAR3_0,
        P_OPPAR3_1,
        P_OPPAR3_2,
        P_OPPAR3_3,
        P_OPPAR3_4,
        P_OPPAR3_5,
        P_OPPAR3_6,
        P_OPPAR3_7,
       NUM_PARAMS };


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

  /* size of buffer (param) */
  float bufsizep;
  /* shape of envelope */
  float shape;

public:
  /* input and output buffers. out is framesize*2 samples long, in is framesize
     samples long. (for maximum framesize)
  */
  float * in0, * out0;

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

  /* buffersize is 3 * third, framesize is 2 * third 
     buffersize is used for outbuf.
  */
  long bufsize, framesize, third;

  /* 1 if need to do ioChanged since buffer settings are different now */
  int changed;

  /* ---------- geometer stuff ----------- */

  static int pointops(float pop, int npts, float * op_param, int samps,
		      int * px, float * py, int maxpts,
		      int * tempx, float * tempy);

  float pointstyle;
  float pointparam[NUM_POINTSTYLES];

  float interpstyle;
  float interparam[NUM_INTERPSTYLES];

  float pointop1;
  float pointop2;
  float pointop3;

  float oppar1[NUM_OPS];
  float oppar2[NUM_OPS];
  float oppar3[NUM_OPS];

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
