/*---------------------------------------------------------------

   (c) 2001, Marcberg Soft & Hard GmbH, All Rights Reserved
   (c) 2001 TOMBORG!!!!!!!!!!!!!!!!!!!!

---------------------------------------------------------------*/

#ifndef __INTERCOM_H
#define __INTERCOM_H

#include "audioeffectx.h"


//----------------------------------------------------------------------- 
// constants & macros

const long RMS_WINDOW = 630;

// this is for shaping the parameter entries into more dB-like values
#define noiseGain (fNoiseGain*fNoiseGain*4.0f)


//--------------------------------------------------------------------- 

class Intercom : public AudioEffectX {
public:
  Intercom(audioMasterCallback audioMaster);
  ~Intercom();

  virtual void process(float **inputs, float **outputs, long samples);
  virtual void processReplacing(float **inputs, float **outputs, long samples);

  virtual void processX(float **inputs, float **outputs, 
			long samples, int replacing);

  virtual void setProgramName(char *name);
  virtual void getProgramName(char *name);
  virtual void setParameter(long index, float value);
  virtual float getParameter(long index);
  virtual void getParameterLabel(long index, char *label);
  virtual void getParameterDisplay(long index, char *text);
  virtual void getParameterName(long index, char *text);
  virtual bool getEffectName(char *name);
  virtual long getVendorVersion();
  virtual bool getErrorText(char *text);
  virtual bool getVendorString(char *text);
  virtual bool getProductString(char *text);

  // this is my stuff:
protected:
  float fNoiseGain;	// the parameter
  char programName[32];
  double rms1, rms2;
  long rmscount;
  float lastRMS1, lastRMS2, noiseAmp1, noiseAmp2;

  float specialk, specialw, specialm;
};

#endif
