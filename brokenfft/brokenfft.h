/*---------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2001-2023  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 2 of the License, or 
(at your option) any later version.

Destroy FX Library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Destroy FX Library.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org

BrokenFFT 2, featuring the Super Destroy FX Windowing System!
---------------------------------------------------------------*/

#ifndef _DFX_BROKENFFT_H
#define _DFX_BROKENFFT_H

#include <array>
#include <audioeffectx.h>
#include "rfftw.h"

#ifdef WIN32
/* turn off warnings about default but no cases in switch, etc. */
   #pragma warning( disable : 4065 57 4200 4244 )
   #include <windows.h>
#endif

#define fsign(f) ((f<0.0)?-1.0:1.0)
#define pi (3.1415926535)

#define MAXECHO 8192

#define FOURCC(a, b, c, d) (((unsigned int)(a) << 24) | ((unsigned int)(b) << 16) | \
                            ((unsigned int)(c) << 8) | ((unsigned int)(d)))

/* change these for your plugins */
#define PLUGIN BrokenFFT
#define PLUGINID FOURCC('D', 'F', 'b', 'f')
#define PLUGINNAME "DFX BROKENFFT"
#define PLUGINPROGRAM BrokenFFTProgram

#define NUM_PROGRAMS 16

/* the names of the parameters */
enum { P_BUFSIZE, P_SHAPE, P_METHOD,
       P_DESTRUCT,
       P_PERTURB,
       P_QUANT,
       P_ROTATE,
       P_BINQUANT,
       P_COMPRESS,
       P_SPIKE,
       P_MUG,
       P_SPIKEHOLD,
       P_ECHOMIX,
       P_ECHOTIME,
       P_ECHOMODF,
       P_ECHOMODW,
       P_ECHOFB,
       P_ECHOLOW,
       P_ECHOHI,
       P_POSTROT,
       P_LOWP,
       P_MOMENTS,
       P_BRIDE,
       P_BLOW,
       P_CONV,
       P_HARM,
       P_ALOW,
       P_NORM,
       NUM_PARAMS};

struct param {
  float * ptr;
  const char * name;
  const char * units;
  float def;
};

struct amplentry {
  float a;
  int i;
};

class PLUGINPROGRAM {
  friend class PLUGIN;
public:
  PLUGINPROGRAM() { strcpy(name, "Mr. Fourier"); }
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
  virtual long fxIdle();

  virtual void processw(float * in, float * out);

  void normalize(long, float);

  bool getVendorString(char *text) {
    strcpy (text, "Destroy FX");
    return true; 
  }

  bool getProductString(char *text) {
    strcpy (text, PLUGIN_COLLECTION_NAME);
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
    strcpy(programs[0].name, "Mr. Fourier");

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

  static constexpr std::array buffersizes {
    2, 4, 8, 16, 32, 64, 128, 256, 512,
    1024, 2048, 4096, 8192, 16384, 32768
  };

  static void tqsort(amplentry * low, int n, int stop);
  

  /* buffersize is 3 * third, framesize is 2 * third 
     buffersize is used for outbuf.
  */
  long bufsize, framesize, third;

  /* 1 if need to do ioChanged since buffer settings are different now */
  int changed;


  /* BAD FFT STUFF */
  void fftops(long samples);

  float destruct;

  float * left_buffer, 
        * right_buffer;

  int lastblocksize;

  float perturb, quant, rotate, spike;

  float binquant;

  float sampler, samplei;

  float compress;
  float makeupgain;

  float bit1, bit2;
  float spikehold;
  int samplesleft;

  float echomix, echotime;

  int amplhold, stopat;
  amplentry * ampl;
  float * echor;
  float * echoc;

  float echofb;

  /* fft work area */
  float * tmp, * oot, * fftr, * ffti;

  float echomodw, echomodf;
  float echolow, echohi;
  int echoctr;


  float postrot;

  float bride;

  float moments;

  float lowpass;
  float blow;
  float convolve;
  float afterlow;
  float norm;

  float harm;

  float method;

  rfftw_plan plan;
  rfftw_plan rplan;

};


#define FPARAM(pname, idx, nm, init, un) do { paramptrs[idx].ptr = &pname; paramptrs[idx].name = (nm); paramptrs[idx].units = (un); paramptrs[idx].def = (init); pname = paramptrs[idx].def; } while (0)

#endif
