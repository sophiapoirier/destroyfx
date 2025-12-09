/*---------------------------------------------------------------
Copyright (C) 2001-2025  Tom Murphy 7

This file is part of Intercom.

Intercom is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 2 of the License, or 
(at your option) any later version.

Intercom is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Intercom.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
---------------------------------------------------------------*/

#ifndef DFX_INTERCOM_H
#define DFX_INTERCOM_H

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
