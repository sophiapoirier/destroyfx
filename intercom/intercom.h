/*---------------------------------------------------------------

   (c) 2001, Sophberg Soft & Hard GmbH, All Rights Reserved
   (c) 2001 TOMBORG!!!!!!!!!!!!!!!!!!!!

---------------------------------------------------------------*/

#ifndef __DFX_INTERCOM_H
#define __DFX_INTERCOM_H

#include "audioeffectx.h"


//----------------------------------------------------------------------------- 
// plugin parameters
enum
{
	kParam_NoiseGain,
	kParam_SpecialK,
	kParam_SpecialW,
	kParam_SpecialM,

	kNumParameters
};


//--------------------------------------------------------------------- 

class Intercom : public AudioEffectX {
public:
  Intercom(audioMasterCallback audioMaster);

  virtual void process(float **inputs, float **outputs, long samples);
  virtual void processReplacing(float **inputs, float **outputs, long samples);
  virtual void suspend();

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
  void processX(float **inputs, float **outputs, long samples, int replacing);

  float fNoiseGain;	// the parameter
  char programName[32];
  double rms1, rms2;
  long rmscount;
  float lastRMS1, lastRMS2, noiseAmp1, noiseAmp2;

  float specialk, specialw, specialm;
  float fUseWrongRMS, fRMSceiling;
};

#endif
