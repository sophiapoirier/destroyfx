
#ifndef __DECIMATE_H
#define __DECIMATE_H

#include "audioeffectx.h"

class Decimate : public AudioEffectX {
public:
  Decimate(audioMasterCallback audioMaster);
  ~Decimate();

  virtual void process(float **inputs, float **outputs, long sampleFrames);
  virtual void processReplacing(float **inputs, 
                                float **outputs, long sampleFrames);
  virtual void setProgramName(char *name);
  virtual void getProgramName(char *name);
  virtual void setParameter(long index, float value);
  virtual float getParameter(long index);
  virtual void getParameterLabel(long index, char *label);
  virtual void getParameterDisplay(long index, char *text);
  virtual void getParameterName(long index, char *text);

protected:
  float bitres;
  float destruct;
  char programName[32];

  float bit1, bit2;
  int samplesleft;
  float samplehold;
  float bigdivisor;
};

#endif
