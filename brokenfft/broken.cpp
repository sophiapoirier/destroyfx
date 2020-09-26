/* Broken FFT 2.0: Featuring the Super Destroy FX Windowing System! */

#include "brokenfft.hpp"
#include "fourier.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <math.h>

#define MKBUFSIZE(c) (buffersizes[(int)((c)*(BUFFERSIZESSIZE-1))])

const int PLUGIN::buffersizes[BUFFERSIZESSIZE] = { 
  2, 4, 8, 16, 32, 64, 128, 256, 512, 
  1024, 2048, 4096, 8192, 16384, 32768, 
};

PLUGIN::PLUGIN(audioMasterCallback audioMaster)
  : AudioEffectX(audioMaster, NUM_PROGRAMS, NUM_PARAMS) {

  FPARAM(bufsizep, P_BUFSIZE, "wins", 0.5f, "samples");
  FPARAM(shape, P_SHAPE, "shape", 0.0f, "");
  FPARAM(method, P_METHOD, "how", 0.0f, "which");

  FPARAM(destruct, P_DESTRUCT, "very special", 1.0f, "?");
  FPARAM(perturb, P_PERTURB, "perturb", 0.0f, "?");
  FPARAM(quant, P_QUANT, "operation q", 1.0f, "?");
  FPARAM(rotate, P_ROTATE, "rotate", 0.0f, "?");
  FPARAM(binquant, P_BINQUANT, "operation bq", 0.0f, "?");
  FPARAM(compress, P_COMPRESS, "comp???", 1.f, "?");
  FPARAM(spike, P_SPIKE, "spike", 1.0f, "?");
  FPARAM(makeupgain, P_MUG, "M.U.G.", 0.0f, "?");
  FPARAM(spikehold, P_SPIKEHOLD, "spikehold", 0.0f, "?");
  FPARAM(echomix, P_ECHOMIX, "eo mix", 0.0f, "?");
  FPARAM(echotime, P_ECHOTIME, "eo time", 0.5f, "?");
  FPARAM(echomodf, P_ECHOMODF, "eo mod f", float(2.0 * pi), "?");
  FPARAM(echomodw, P_ECHOMODW, "eo mod w", 0.0f, "?");
  FPARAM(echofb, P_ECHOFB, "eo fbx", 0.50f, "?");
  FPARAM(echolow, P_ECHOLOW, "eo >", 0.0f, "?");
  FPARAM(echohi, P_ECHOHI, "eo <", 1.0f, "?");
  FPARAM(postrot, P_POSTROT, "postrot", 0.5f, "?");
  FPARAM(moments, P_MOMENTS, "moments", 0.0f, "?");
  FPARAM(bride, P_BRIDE, "bride", 1.0f, "?");
  FPARAM(blow, P_BLOW, "blow", 0.0f, "?");
  FPARAM(lowpass, P_LOWP, "lowp", 1.0f, "?");
  FPARAM(convolve, P_CONV, "convolve", 0.0f, "?");
  FPARAM(harm, P_HARM, "harm", 0.0f, "?");
  FPARAM(afterlow, P_ALOW, "afterlow", 1.0f, "?");
  FPARAM(norm, P_NORM, "anorm", 0.0f, "?");
  

  long maxframe = 0;
  for (long i=0; i<BUFFERSIZESSIZE; i++)
    maxframe = ( buffersizes[i] > maxframe ? buffersizes[i] : maxframe );

  setup();


  changed = 0;
}

PLUGIN::~PLUGIN() {
  free (in0);
  free (out0);

  free (prevmix);

  if (programs) delete[] programs;
}

void PLUGIN::resume() {
  framesize = MKBUFSIZE(bufsizep);
  third = framesize / 2;
  bufsize = third * 3;


  /* set up buffers. Prevmix and first frame of output are always
     filled with zeros. */

  for (int ii = 0; ii < third; ii ++) {
    prevmix[ii] = 0.0f;
  }

  for (int j = 0; j < framesize; j ++) {
    out0[j] = 0.0f;
  }
  
  /* start input at beginning. Output has a frame of silence. */
  insize = 0;
  outstart = 0;
  outsize = framesize;

  setInitialDelay(framesize);
  changed = 0;
  needIdle();

}

void PLUGIN::suspend () {
  /* nothing to do here. */

}

long PLUGIN::fxIdle() {
  /* ioChanged() causes resume() to be called and 
     changed to initialDelay are updated for the host */
  if (changed)
    ioChanged();
  changed = 0;

  return 1;
}

/* tail is the same as delay, of course */
long PLUGIN::getTailSize() { return framesize; }

void PLUGIN::setParameter(long index, float value) {
  switch (index) {
  case P_BUFSIZE:
    changed = 1;
    /* fallthrough */
  default:
    if (index >= 0 && index < NUM_PARAMS)
      *paramptrs[index].ptr = value;
    /* otherwise, ??? */
    break;
  }

  /* copy the new value to the active program's corresponding parameter value */
  if ( (index >= 0) && (index < NUM_PARAMS) )
    programs[curProgram].param[index] = value;
}

float PLUGIN::getParameter(long index) {
  switch (index) {
    /* special cases here */
  default:
    /* otherwise pull it out of array. */
    if (index >= 0 && index < NUM_PARAMS) return *paramptrs[index].ptr;
    else return 0.0f; /* ? */
  }
}

void PLUGIN::getParameterName(long index, char *label) {
  switch(index) {
    /* special cases here */
  default:
    if (index >= 0 && index < NUM_PARAMS && paramptrs[index].name) 
      strcpy(label, paramptrs[index].name);
    else strcpy(label, "?");
    break;
  }
}

void PLUGIN::getParameterDisplay(long index, char *text) {
  switch(index) {
  case P_BUFSIZE:
    sprintf(text, "%d", MKBUFSIZE(bufsizep));
    break;
  case P_SHAPE:
    if (shape < 0.20f) {
      strcpy(text, "linear");
    } else if (shape < 0.40f) {
      strcpy(text, "arrow");
    } else if (shape < 0.60f) {
      strcpy(text, "wedge");
    } else if (shape < 0.80f) {
      strcpy(text, "best");
    } else {
      strcpy(text, "cos^2");
    }
    break;

  case P_METHOD:
    if (method < 0.25) strcpy(text, "dc");
    else if (method < 0.50) strcpy(text, "west");
    else strcpy(text, "west-bug");
    break;
    /* special cases here */
  case P_POSTROT:
    if (postrot > 0.5f) {
      sprintf(text, "+%d", 
	      (int)((postrot - 0.5f) * 1.0f * MKBUFSIZE(bufsizep)));
    } else if (postrot == 0.5f) {
      sprintf(text, "*NO*");
    } else {
      sprintf(text, "-%d", 
	      (int)((0.5f - postrot) * 1.0f * MKBUFSIZE(bufsizep)));
    }
    break;
  default:
    float2string(getParameter(index), text);
    break;
  }
}

void PLUGIN::getParameterLabel(long index, char *label) {
  switch(index) {
    /* special cases here */
  default:
    if (index >= 0 && index < NUM_PARAMS && paramptrs[index].units) 
      strcpy(label, paramptrs[index].units);
    else strcpy(label, "?");
    break;
  }
}



/* this fake processX function reads samples one at a time
   from the true input. It simultaneously copies samples from
   the beginning of the output buffer to the true output.
   We maintain that out0 always has at least 'third' samples
   in it; this is enough to pick up for the delay of input
   processing and to make sure we always have enough samples
   to fill the true output buffer.

   If the input frame is full:
    - calls wprocess on this full input frame
    - applies the windowing envelope to the tail of out0 (output frame)
    - mixes in prevmix with the first half of the output frame
    - increases outsize so that the first half of the output frame is
      now available output
    - copies the second half of the output to be prevmix for next frame.
    - copies the second half of the input buffer to the first,
      resets the size (thus we process each third-size chunk twice)

  If we have read more than 'third' samples out of the out0 buffer:
   - Slide contents to beginning of buffer
   - Reset outstart

*/

/* to improve: 
   - use memcpy and arithmetic instead of
     sample-by-sample copy 
   - can we use tail of out0 as prevmix, instead of copying?
   - can we use circular buffers instead of memmoving a lot
     (probably not)
*/

void PLUGIN::processX(float **trueinputs, float **trueoutputs, long samples, 
		      int replacing) {
  float * tin  = *trueinputs;
  float * tout = *trueoutputs;

  for (int ii = 0; ii < samples; ii++) {

    /* copy sample in */
    in0[insize] = tin[ii];
    insize ++;
 
    if (insize == framesize) {
      /* frame is full! */

      /* in0 -> process -> out0(first free space) */
      processw(in0, out0+outstart+outsize);

      float oneDivThird = 1.0f / (float)third;
      /* apply envelope */

      if (shape < 0.20f) {
	for(int z = 0; z < third; z++) {
	  float p = sqrtf((float)z * oneDivThird);
	  out0[z+outstart+outsize] *= p;
	  out0[z+outstart+outsize+third] *= (1.0f - p);
	}
      } else if (shape < 0.40f) {
	for(int z = 0; z < third; z++) {
	  float p = (float)z * oneDivThird;
	  p *= p;
	  out0[z+outstart+outsize] *= p;
	  out0[z+outstart+outsize+third] *= (1.0f - p);
	}
      } else if (shape < 0.60f) {
	for(int z = 0; z < third; z++) {
	  out0[z+outstart+outsize] *= ((float)z * oneDivThird);
	  out0[z+outstart+outsize+third] *= (1.0f - ((float)z * oneDivThird));
	}
      } else if (shape < 0.80f) {
	for(int z = 0; z < third; z ++) {
	  float p = 0.5f * (-cos(float(pi * ((float)z * oneDivThird))) + 1.0f);
	  out0[z+outstart+outsize] *= p;
	  out0[z+outstart+outsize+third] *= (1.0f - p);
	}
      } else {
	for(int z = 0; z < third; z ++) {
	  float p = 0.5f * (-cos(float(pi * ((float)z * oneDivThird))) + 1.0f);
	  p = p * p;
	  out0[z+outstart+outsize] *= p;
	  out0[z+outstart+outsize+third] *= (1.0f - p);
	}
      }

      /* mix in prevmix */
      for(int u = 0; u < third; u ++)
	out0[u+outstart+outsize] += prevmix[u];

      /* prevmix becomes out1 */
      memcpy(prevmix, out0 + outstart + outsize + third, third * sizeof (float));

      /* copy 2nd third of input over in0 (need to re-use it for next frame), 
	 now insize = third */
      memcpy(in0, in0 + third, third * sizeof (float));

      insize = third;
      
      outsize += third;
    }

    /* send sample out */
    if (replacing) tout[ii] = out0[outstart];
    else tout[ii] += out0[outstart];

    outstart ++;
    outsize --;

    /* make sure there is always enough room for a frame in out buffer */
    if (outstart == third) {
      memmove(out0, out0 + outstart, outsize * sizeof (float));
      outstart = 0;
    }
  }
}

/* these should always call the common processX function */

void PLUGIN::processReplacing(float **inputs, float **outputs, long samples) {
  processX(inputs,outputs,samples, 1);
}

void PLUGIN::process(float **inputs, float **outputs, long samples) {
  processX(inputs,outputs,samples, 0);
}


/* program stuff. Should never need to mess with this, hopefully.. */

void PLUGINPROGRAM::init(PLUGIN * p) {
  /* copy defaults from paramptrs */
  for (int i=0; i < NUM_PARAMS; i++)
    param[i] = p->paramptrs[i].def;
}

void PLUGIN::setProgram(long programNum) {
  if ( (programNum < NUM_PROGRAMS) && (programNum >= 0) ) {
      AudioEffectX::setProgram(programNum);

      curProgram = programNum;
      for (int i=0; i < NUM_PARAMS; i++)
        setParameter(i, programs[programNum].param[i]);
    }
  // tell the host to update the editor display with the new settings
  AudioEffectX::updateDisplay();
}

void PLUGIN::setProgramName(char *name) {
  strcpy(programs[curProgram].name, name);
}

void PLUGIN::getProgramName(char *name) {
  if ( !strcmp(programs[curProgram].name, "default") )
    sprintf(name, "default %ld", curProgram+1);
  else
    strcpy(name, programs[curProgram].name);
}

bool PLUGIN::getProgramNameIndexed(long category, long index, char * text) {
  if ( (index < NUM_PROGRAMS) && (index >= 0) ) {
    strcpy(text, programs[index].name);
    return true;
  }
  return false;
}

bool PLUGIN::copyProgram(long destination) {
  if ( (destination < NUM_PROGRAMS) && (destination >= 0) ) {
    programs[destination] = programs[curProgram];
    return true;
  }
  return false;
}


/* this is only compiled if not building the GUI version */
#ifndef GUI

/* XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX */ 
/* ---------- boring stuff below this line ----------- */
/* XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX */ 

static AudioEffect *effect = 0;
bool oome = false;

#if MAC
   #pragma export on
#endif

// prototype of the export function main
#if BEOS
   #define main main_plugin
   extern "C" __declspec(dllexport) AEffect 
      *main_plugin (audioMasterCallback audioMaster);

#else
   AEffect *main (audioMasterCallback audioMaster);
#endif

AEffect *main (audioMasterCallback audioMaster) {
  // get vst version
  if (!audioMaster (0, audioMasterVersion, 0, 0, 0, 0))
    return 0;  // old version

  effect = new PLUGIN (audioMaster);
  if (!effect)
    return 0;
  if (oome) {
    delete effect;
    return 0;
  }
  return effect->getAeffect ();
}

#if MAC
  #pragma export off
#endif

#if WIN32
  void* hInstance;
  BOOL WINAPI DllMain (HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved) {
    hInstance = hInst;
    return 1;
  }
#endif
#endif
