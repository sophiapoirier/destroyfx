#ifndef __TOM7_TRANS_H
#define __TOM7_TRANS_H

#include "audioeffectx.h"
#include "rfftw.h"

#ifdef WIN32
/* turn off warnings about default but no cases in switch, etc. */
   #pragma warning( disable : 4065 57 4200 4244 )
#endif

#define fsign(f) ((f<0.0)?-1.0:1.0)
#define pi (3.1415926535)

#define PLUGIN Trans
#define PLUGINID 'T7tr'
#define PLUGINNAME "DFX Trans"

#define NUM_PARAMS 2

struct param {
  float * ptr;
  const char * name;
  const char * units;
};

struct PLUGIN : public AudioEffectX {

  PLUGIN(audioMasterCallback audioMaster);
  ~PLUGIN();

  virtual void process(float **inputs, float **outputs, long sampleFrames);
  virtual void processReplacing(float **inputs, float **outputs, 
				long sampleFrames);
  virtual void setParameter(long index, float value);
  virtual float getParameter(long index);
  virtual void getParameterLabel(long index, char *label);
  virtual void getParameterDisplay(long index, char *text);
  virtual void getParameterName(long index, char *text);

  virtual void suspend();

  void setup() {
    setNumInputs(1);		/* mono in/out */
    setNumOutputs(1);
    setUniqueID(PLUGINID);

    canProcessReplacing();
    strcpy(programName, PLUGINNAME);
  }

  virtual void setProgramName(char *name) {
    strcpy(programName, name);
  }

  virtual void getProgramName(char *name) {
    strcpy(name, programName);
  }

protected:
  char programName[32];

  param paramptrs[NUM_PARAMS];


  float function;
  float inverse;

  float * fftbuf;

  double deriv_last;
  double integrate_sum;

  rfftw_plan plan;
  int lastsamples;
  int olddir;

};

#define FPARAM(pname, idx, nm, init, un) do { pname = (init); paramptrs[idx].ptr = &pname; paramptrs[idx].name = (nm); paramptrs[idx].units = (un); } while (0)


#endif
