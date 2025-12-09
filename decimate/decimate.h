/*---------------------------------------------------------------
Copyright (C) 2001-2025  Tom Murphy 7

This file is part of Decimate.

Decimate is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 2 of the License, or 
(at your option) any later version.

Decimate is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Decimate.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
---------------------------------------------------------------*/

#ifndef DFX_DECIMATE_H
#define DFX_DECIMATE_H

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
