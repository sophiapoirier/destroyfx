/*---------------------------------------------------------------
Copyright (C) 2001-2025  Tom Murphy 7

This file is part of Trans.

Trans is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 2 of the License, or 
(at your option) any later version.

Trans is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Trans.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
---------------------------------------------------------------*/

#ifndef DFX_TOM7_TRANS_H
#define DFX_TOM7_TRANS_H

#include "audioeffectx.h"
#include "rfftw.h"

// TODO: Port to DfxPlugin
#include "dfxplugin-prefix.h"

#ifdef WIN32
/* turn off warnings about default but no cases in switch, etc. */
   #pragma warning( disable : 4065 57 4200 4244 )
#endif

#define fsign(f) ((f<0.0)?-1.0:1.0)
#define pi (3.1415926535)

#define PLUGIN Trans
#define PLUGIN_ID FOURCC('D', 'F', 't', 'f')
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
    setUniqueID(PLUGIN_ID);

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
