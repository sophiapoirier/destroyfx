
/* Super Destroy FX Windowing System! */

#ifndef __DFX_WINDOWINGSTUB_H
#define __DFX_WINDOWINGSTUB_H

#include <audioeffectx.h>

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
#define PLUGINNAME "DFX WINDOWINGSTUB"
#define PLUGINPROGRAM GeometerProg

#define NUM_PROGRAMS 16

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
       P_POINTOP1, P_POINTOP2, P_POINTOP3,
       P_OPPAR1_0, 
       P_OPPAR1_1, 
       P_OPPAR1_2, 
       P_OPPAR1_3, 
       P_OPPAR1_4, 
       P_OPPAR1_5, 
       P_OPPAR1_6, 
       P_OPPAR1_7, 
       P_OPPAR2_0, 
       P_OPPAR2_1, 
       P_OPPAR2_2, 
       P_OPPAR2_3, 
       P_OPPAR2_4, 
       P_OPPAR2_5, 
       P_OPPAR2_6, 
       P_OPPAR2_7, 
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

  virtual void processw(float * in, float * out, long samples);

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
    strcpy(programs[0].name, "VST window");	// default program name

    setNumInputs(1);		/* mono in/out */
    setNumOutputs(1);
    setUniqueID(PLUGINID);

    canProcessReplacing();
  }

protected:
  /* stores info for each parameter */
  param paramptrs[NUM_PARAMS];

  /* stores programs */
  PLUGINPROGRAM *programs;

  /* size of buffer (param) */
  float bufsizep;
  /* shape of envelope */
  float shape;

  /* input and output buffers. out is framesize*2 samples long, in is framesize
     samples long. (for maximum framesize)
  */
  float * in0, * out0;

  /* third-sized tail of previous processed frame. already has mixing envelope
     applied.
   */
  float * prevmix;

  /* number of samples in in0 */
  int insize;

  /* number of samples and starting position of valid samples in out0 */
  int outsize;
  int outstart;

  #define BUFFERSIZESSIZE 15
  static const int buffersizes[BUFFERSIZESSIZE];

  /* buffersize is 3 * third, framesize is 2 * third 
     buffersize is used for outbuf.
  */
  long bufsize, framesize, third;

  /* 1 if need to do ioChanged since buffer settings are different now */
  int changed;

  /* ---------- geometer stuff ----------- */

  int pointops(float pop, int npts, float * op_param, int samps);

  float pointstyle;
  float pointparam[8];

  float interpstyle;
  float interparam[8];

  float pointop1;
  float pointop2;
  float pointop3;

  float oppar1[8];
  float oppar2[8];
  float oppar3[8];

  int lastx;
  float lasty;

  int * pointx;
  float * pointy;

  int * tempx;
  float * tempy;

};

#define FPARAM(pname, idx, nm, init, un) do { paramptrs[idx].ptr = &pname; paramptrs[idx].name = (nm); paramptrs[idx].units = (un); paramptrs[idx].def = (init); pname = paramptrs[idx].def; } while (0)

#endif
