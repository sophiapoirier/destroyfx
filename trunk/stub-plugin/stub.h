#ifndef __TOM7_STUB_H
#define __TOM7_STUB_H

#include <audioeffectx.h>

#ifdef WIN32
/* turn off warnings about default but no cases in switch, etc. */
   #pragma warning( disable : 4065 57 4200 4244 )
   #include <windows.h>
#endif

#define fsign(f) ((f<0.0)?-1.0:1.0)
#define pi (3.1415926535)

#define PLUGIN Stub
#define PLUGINID 'STUB'
#define PLUGINNAME "DFX STUBPLUG"

#define NUM_PARAMS 0
#define NUM_PROGRAMS 1

struct param {
  float * ptr;
  const char * name;
  const char * units;
};

struct PLUGIN : public AudioEffectX {

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

  virtual void suspend();

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
    setNumInputs(1);		/* mono in/out */
    setNumOutputs(1);
    setUniqueID(PLUGINID);

    canProcessReplacing();
    strcpy(programName, PLUGINNAME);
  }

  void setProgramName(char *name) {
    strcpy(programName, name);
  }

  void getProgramName(char *name) {
    strcpy(name, programName);
  }

protected:
  char programName[32];

  param paramptrs[NUM_PARAMS];

};

#define FPARAM(pname, idx, nm, init, un) do { pname = (init); paramptrs[idx].ptr = &pname; paramptrs[idx].name = (nm); paramptrs[idx].units = (un); } while (0)

#endif
