
#include "trans.hpp"

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <math.h>

#define MAXFRAME 8192

void dfour1(float *data, int nn, int isign);

PLUGIN::PLUGIN(audioMasterCallback audioMaster)
  : AudioEffectX(audioMaster, 1, NUM_PARAMS) {

  FPARAM(function, 0, "function", 0.0, "");
  FPARAM(inverse,  1, "inverse", 0.0, "");

  deriv_last = 0.0;
  integrate_sum = 0.0;

  fftbuf = (float*)malloc((MAXFRAME + 4) * sizeof (float));

  lastsamples = 2048;
  plan = rfftw_create_plan(2048, FFTW_FORWARD, FFTW_ESTIMATE);
  olddir = (inverse < 0.5);

  setup();
}

PLUGIN::~PLUGIN() {

  free(fftbuf);

  rfftw_destroy_plan(plan);

}

void PLUGIN::setParameter(long index, float value) {
  switch (index) {
  default:
    if (index >= 0 && index < NUM_PARAMS)
      *paramptrs[index].ptr = value;
    break;
  }
}

float PLUGIN::getParameter(long index) {
  switch (index) {
    /* special cases here */
  default:
    /* otherwise pull it out of array. */
    if (index >= 0 && index < NUM_PARAMS) return *paramptrs[index].ptr;
    else return 0.0; /* ? */
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
  case 0:
    if (function < 0.20)
      strcpy(text, "SIN");
    else if (function < 0.40)
      strcpy(text, "TAN");
    else if (function < 0.60)
      strcpy(text, "e^x");
    else if (function < 0.80)
      strcpy(text, "dy/dx");
    else if (function <= 1.0)
      strcpy(text, "fft");
    break;
  case 1:
    if (inverse > 0.5)
      strcpy(text, "YES");
    else
      strcpy(text, "NO");
    break;
    /* special cases here */
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

void PLUGIN::suspend () {
  deriv_last = 0.0;
  integrate_sum = 0.0;
}

void PLUGIN::process(float **inputs, float **outputs, long samples) {
  float * in  = *inputs;
  float * out = *outputs;

  if (function <= 0.20) {
    if (inverse < 0.5) {
      for(int i = 0; i < samples; i++) {
	out[i] += sin(sin(in[i]));
      }
    } else {
      for(int i = 0; i < samples; i++) {
	out[i] += asin(asin(in[i]));
      }
    }
  } else if (function <= 0.40) {
    if (inverse < 0.5) {
      for(int i = 0; i < samples; i++) {
	out[i] += tan(tan(in[i]));
      }
    } else {
      for(int i = 0; i < samples; i++) {
	out[i] += atan(atan(in[i]));
      }
    }
  } else if (function <= 0.60) {
    if (inverse < 0.5) {
      for(int i = 0; i < samples; i++) {
	out[i] += fsign(in[i]) * exp(fabs(in[i]));
      }
    } else {
      for(int i = 0; i < samples; i++) {
	out[i] += fsign(in[i]) * log(fabs(in[i]));
      }
    }
  } else if (function <= 0.80) {
    if (inverse < 0.5) {

      /* keep from getting out of control */
      deriv_last = (deriv_last * 0.5);

      for(int i = 0; i < samples; i++) {
	out[i] += deriv_last - in[i];
	deriv_last = in[i];
      }
    } else {

      /* keep from getting out of control */
      integrate_sum *= 0.5;
      for(int i = 0; i < samples; i++) {
	integrate_sum += in[i];
	out[i] += integrate_sum;
      }
    }
  } else if (function <= 1.0) {
    if (samples > MAXFRAME) samples = MAXFRAME;

    if (lastsamples != samples || olddir != (inverse < 0.5)) {
      rfftw_destroy_plan(plan);
      plan = rfftw_create_plan(lastsamples=samples, 
			       (inverse < 0.5)?FFTW_FORWARD:FFTW_BACKWARD, 
			       FFTW_ESTIMATE);
      olddir = (inverse < 0.5);
    }

    rfftw_one(plan, in, fftbuf);

    if (inverse < 0.5) {
      for(int i = 0; i < samples; i++) {
	out[i] += fftbuf[i];
      }
    } else {
      float div = 1.0 / samples;
      for(int i = 0; i < samples; i++) {
	out[i] += fftbuf[i] * div;
      }
    }
  }

  /* else ... */

}

/* usually this will do */
void PLUGIN::processReplacing(float **inputs, float **outputs, long samples) {
  for (int i = 0; i < samples; i++) {
    outputs[0][i] = 0.0;
  }
  process(inputs,outputs,samples);
}


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
#include <windows.h>
void* hInstance;
BOOL WINAPI DllMain (HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved) {
  hInstance = hInst;
  return 1;
}
#endif
