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

  FPARAM(bufsizep, P_BUFSIZE, "wsize", 0.5, "samples");
  FPARAM(shape, P_SHAPE, "shape", 0.0, "");

  FPARAM(destruct, P_DESTRUCT, "very special", 1.0, "?");
  FPARAM(perturb, P_PERTURB, "perturb", 0.0, "?");
  FPARAM(quant, P_QUANT, "operation q", 1.0, "?");
  FPARAM(rotate, P_ROTATE, "rotate", 0.0, "?");
  FPARAM(binquant, P_BINQUANT, "operation bq", 0.0, "?");
  FPARAM(compress, P_COMPRESS, "comp???", 1.0, "?");
  FPARAM(spike, P_SPIKE, "spike", 1.0, "?");
  FPARAM(makeupgain, P_MUG, "M.U.G.", 0.0, "?");
  FPARAM(spikehold, P_SPIKEHOLD, "spikehold", 0.0, "?");
  FPARAM(echomix, P_ECHOMIX, "eo mix", 0.0, "?");
  FPARAM(echotime, P_ECHOTIME, "eo time", 0.5, "?");
  FPARAM(echomodf, P_ECHOMODF, "eo mod f", 2.0 * pi, "?");
  FPARAM(echomodw, P_ECHOMODW, "eo mod w", 0.0, "?");
  FPARAM(echofb, P_ECHOFB, "eo fbx", 0.50, "?");
  FPARAM(echolow, P_ECHOLOW, "eo >", 0.0, "?");
  FPARAM(echohi, P_ECHOHI, "eo <", 1.0, "?");
  FPARAM(postrot, P_POSTROT, "postrot", 0.5, "?");
  

  long maxframe = 0;
  for (long i=0; i<BUFFERSIZESSIZE; i++)
    maxframe = ( buffersizes[i] > maxframe ? buffersizes[i] : maxframe );

  setup();

  in0 = (float*)malloc(maxframe * sizeof (float));
  out0 = (float*)malloc(maxframe * 2 * sizeof (float));

  /* prevmix is only a single third long */
  prevmix = (float*)malloc((maxframe / 2) * sizeof (float));

  echor = (float*)malloc(MAXECHO * sizeof(float));
  echoc = (float*)malloc(MAXECHO * sizeof(float));

  ampl = (amplentry*)malloc(sizeof (amplentry) * maxframe);
  amplsamples = maxframe;

  fftr = (float*)malloc(maxframe * sizeof(float));
  ffti = (float*)malloc(maxframe * sizeof(float));
  tmp = (float*)malloc(maxframe * sizeof(float));
  oot = (float*)malloc(maxframe * sizeof(float));
  buffersamples = maxframe;

  /* resume sets up buffers and sizes */
  changed = 1;
  resume ();
}

PLUGIN::~PLUGIN() {
  free (in0);
  free (out0);

  free (prevmix);

  if (programs) delete[] programs;
}

void PLUGIN::resume() {

  if (changed) ioChanged();

  for(int i = 0; i < MAXECHO; i++) {
    echor[i] = echoc[i] = 0.0;
  }

  amplhold = 1;
  stopat = 1;

  echoctr = 0;

  sampler = samplei = 0.0f;

  samplesleft = 0;

  framesize = MKBUFSIZE(bufsizep);
  third = framesize / 2;
  bufsize = third * 3;

  /* set up buffers. Prevmix and first frame of output are always
     filled with zeros. */

  for (int ii = 0; ii < third; ii ++) {
    prevmix[ii] = 0.0;
  }

  for (int j = 0; j < framesize; j ++) {
    out0[j] = 0.0;
  }
  
  /* start input at beginning. Output has a frame of silence. */
  insize = 0;
  outstart = 0;
  outsize = framesize;

  if (changed) setInitialDelay(framesize);

  changed = 0;

}

void PLUGIN::suspend () {
  /* nothing to do here. */

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
    /* special cases here */
  case P_POSTROT:
    if (postrot > 0.5) {
      sprintf(text, "+%d", 
	      (int)((postrot - 0.5f) * 2.0f * MKBUFSIZE(bufsizep)));
    } else if (postrot == 0.5) {
      sprintf(text, "*NO*");
    } else {
      sprintf(text, "-%d", 
	      (int)((0.5f - postrot) * 2.0f * MKBUFSIZE(bufsizep)));
    }
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

inline float quantize(float q, float old) {
  /*	if (q >= 1.0) return old;*/
  long scale = ( long)(512 * q);
  if (scale == 0) scale = 2;
  long X = ( long)(scale * old);
	
  return (float)(X / (float)scale);
}

/* this function modifies 'samples' number of floats in
   fftr and ffti */
void PLUGIN::fftops(long samples) {
  for(int i = 0; i < samples; i ++) {

    /* operation bq */
    if (binquant > 0.0 && (samplesleft -- > 0)) {
      fftr[i] = sampler;
      ffti[i] = samplei;
    } else {
      sampler = fftr[i];
      samplei = ffti[i];
      samplesleft = 1 + (int)(binquant * binquant * binquant) * (samples >> 3);
    }

    /* very special */
    if ((rand ()/(float)RAND_MAX) > destruct) {
      /* swap with random sample */
      int j = (int)((rand()/(float)RAND_MAX) * (samples - 1));
      float u = fftr[i];
      fftr[i] = fftr[j];
      fftr[j] = u;

      u = ffti[i];
      ffti[i] = ffti[j];
      ffti[j] = u;
    }

    /* operation Q */
    if (quant <= 0.9999999) {
      fftr[i] = quantize(fftr[i], quant);
      ffti[i] = quantize(ffti[i], quant);
    }

    /* perturb */
    fftr[i] = (double)((rand()/(float)RAND_MAX) * perturb + 
		       fftr[i]*(1.0-perturb));
    ffti[i] = (double)((rand()/(float)RAND_MAX) * perturb + 
		       ffti[i]*(1.0-perturb));

    /* rotate */
    int j = (i + (int)(rotate * samples)) % samples;

    fftr[i] = fftr[j];

    /* compress */

    if (compress < 0.9999999) {
      fftr[i] = fsign(fftr[i]) * powf(fabs(fftr[i]), compress);
      ffti[i] = fsign(ffti[i]) * powf(fabs(ffti[i]), compress);
    }

    /* M.U.G. */
    if (makeupgain > 0.00001) {
      fftr[i] *= 1.0 + (3.0*makeupgain);
      ffti[i] *= 1.0 + (3.0*makeupgain);
    }

  }

  /* pretty expensive spike operation */
  if (spike < 0.999999 ) {

    double loudness = 0.0;

    for ( i=0; i<samples; i++ ) {
      loudness += fftr[i]*fftr[i] + ffti[i]*ffti[i];
    }
    
    if (! -- amplhold) {
      for ( i=0; i<samples; i++ ) {
	ampl[i].a = fftr[i]*fftr[i] + ffti[i]*ffti[i];
	ampl[i].i = i;
      }

      /* sort the amplitude bins */

      stopat = 1+(int)((spike*spike*spike)*samples);

      /* consider a special case for when abs(stopat-samples) < lg samples? */ 
      tqsort(ampl, samples, stopat);

      /* chop of everything after the first i */

      amplhold = 1 + (int)(spikehold * (20.0));

    }

    double newloudness = loudness;
    for(i = stopat - 1; i < samples; i++) {
      newloudness -= ampl[i].a;
      fftr[ampl[i].i] = 0.0;
      ffti[ampl[i].i] = 0.0;
    }

    /* boost what remains. */
    double boostby = loudness / newloudness;
    for(i = 0; i < samples; i ++) {
      fftr[i] *= boostby;
      /* XXX ffti? */
    }

  }

  /* EO FX */

  for(i = 0; i < samples; i ++) {
    echor[echoctr] = fftr[i];
    echoc[echoctr++] = ffti[i];
    echoctr %= MAXECHO;

    /* you want somma this magic?? */

    if (echomix > 0.000001) {
      
      double warble = (echomodw * cos(echomodf * (echoctr / (float)MAXECHO)));
      double echott = echotime + warble;
      if (echott > 1.0) echott = 1.0;
      int echot = (MAXECHO - (MAXECHO * echott));
      int xx = (echoctr + echot) % MAXECHO;
      int last = (echoctr + MAXECHO - 1) % MAXECHO;

      if (echofb > 0.00001) {
	echoc[last] += echoc[xx] * echofb;
	
	echor[last] += echor[xx] * echofb;
      }

      if (i >= (int)((echolow * echolow * echolow) * samples) && 
	  i <= (int)((echohi * echohi * echohi) * samples)) {
	fftr[i] = (1.0 - echomix) * fftr[i] + echomix * echor[xx];
	
	ffti[i] = (1.0 - echomix) * ffti[i] + echomix * echoc[xx];
      }
    }
  }

  /* post-processing rotate-up */
  if (postrot > 0.5) {
    int rotn = (postrot - 0.5) * 2.0 * samples;
    for(int v = samples - 1; v >= 0; v --) {
      if (v < rotn) fftr[v] = ffti[v] = 0.0;
      else { 
	fftr[v] = fftr[v - rotn];
	ffti[v] = ffti[v - rotn];
      }
    }
  } else if (postrot < 0.5) {
    int rotn = (int)((0.5f - postrot) * 2.0f * MKBUFSIZE(bufsizep));
    for(int v = 0; v < samples; v++) {
      if (v > (samples - rotn)) fftr[v] = ffti[v] = 0.0;
      else {
	fftr[v] = fftr[(samples - rotn) - v];
	ffti[v] = ffti[(samples - rotn) - v];
      }
    }
  }
      
}

/* XXX I'm probably copying more times than I need to!
   PS, use memmove!
*/
void PLUGIN::processw(float * in, float * out, long samples) {

  static int ss;
  ss = !ss;

  int i = 0;

  for (int c = 0; c < samples; c++) tmp[c] = in[c];

  fft_float(samples, 0, tmp, 0, fftr, ffti);

  /* do actual processing */
  fftops(samples);

  fft_float(samples, 1, fftr, ffti, oot, tmp);

  for (int cc = 0; cc < samples; cc++) out[cc] = oot[cc];

}


/* tqsort partitions the array (of n elements) so that the first STOP
   elements are each less than the remaining elements. */

void PLUGIN::tqsort(amplentry * low, int n, int stop) {

  /* temporaries for swaps */
  float t;
  int i;
  if (n <= 1) return;
  if (stop <= 0) return;
  if (stop >= n) return; 
  
  if (n == 2) {
    if (low->a >= low[1].a) return;
    t = low->a;
    i = low->i;
    low->a = low[1].a;
    low->i = low[1].i;
    low[1].a = t;
    low[1].i = i;
#if 0
  } else if (n == 3) {
    /* add this ... */
#endif
  } else {
    int pivot = rand() % n;
    t = low[pivot].a;
    i = low[pivot].i;
    low[pivot].a = low->a;
    low[pivot].i = low->i;
    low->a = t;
    low->i = i;

    int l = 1, r = n - 1;

    while (l < r) {
      /* move fingers. 
	 it's possible to run off the array if
	 the pivot is equal to one of the last
	 or first elements (so check that). */

      while ((l < n) && low[l].a >= low->a) l++;
      while (r && low[r].a <= low->a) r--;
      if (l >= r) break;
      /* swap */
      t = low[l].a;
      i = low[l].i;
      low[l].a = low[r].a;
      low[l].i = low[r].i;
      low[r].a = t;
      low[r].i = i;
    }

    /* put the pivot back. */
    
    t = low[l-1].a;
    i = low[l-1].i;
    low[l-1].a = low->a;
    low[l-1].i = low->i;
    low->a = t;
    low->i = i;
    
    /* recurse. */

    if (stop <= l) {
      tqsort(low, l - 1, stop);
    } else {
      tqsort(low+l, n-l, stop - l);
    }
    /* done. */
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
      processw(in0, out0+outstart+outsize, framesize);

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
