
/* brokenfft: $Id: brokenfft.cpp,v 1.4 2001-09-02 05:05:51 tom7 Exp $ */

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <math.h>

#include "brokenfft.hpp"
#include "fourier.h"

#define fsign(f) ((f<0.0)?-1.0:1.0)

#define pi (3.1415926535)

void tqsort(amplentry * low, int n, int stop);

int amplcomp(const void * l, const void * r);
int amplcomp(const void * l, const void * r) {
  amplentry * ll = (amplentry*) l;
  amplentry * rr = (amplentry*) r;
 
  return (int) (rr->a - ll->a);
}

/* samples */

Fft::Fft(audioMasterCallback audioMaster)
  : AudioEffectX(audioMaster, 1, 19) {
  bitres = 1.;		
  samplehold = 1.0;
  samplesleft = 0;
  destruct = 1.0;
  perturb = 0.0;
  quant = 1.0;
  rotate = 0.0;
  binquant = 0.0;
  compress = 1.0;
  sampler = 0.0;
  samplei = 0.0;
  spike = 1.0;
  makeupgain = 0.0;
  amplhold = 1;
  stopat = 1;
  ampl = 0;
  spikehold = 0.0;
  echomix = 0.0;
  echotime = 0.5;
  setNumInputs(1);		/* mono in/out */
  setNumOutputs(1);
  setUniqueID('T7BF');

  echomodf = 2.0 * pi;
  echomodw = 0.0;

  echofb = 0.50;
  echolow = 0.0;
  echohi = 1.0;

  OVERLAP = 5;

  echoctr = 0;
  echor = (float*)malloc(MAXECHO * sizeof(float));
  echoc = (float*)malloc(MAXECHO * sizeof(float));

  ampl = (amplentry*)malloc(sizeof (amplentry) * MAXSAMPLES);
  amplsamples = MAXSAMPLES;

  fftr = (float*)malloc(MAXSAMPLES * sizeof(float));
  ffti = (float*)malloc(MAXSAMPLES * sizeof(float));
  tmp = (float*)malloc(MAXSAMPLES * sizeof(float));
  oot = (float*)malloc(MAXSAMPLES * sizeof(float));
  buffersamples = MAXSAMPLES;

  for(int i = 0; i < MAXECHO; i++) {
    echor[i] = echoc[i] = 0.0;
  }

  overbuff = (float*)malloc((MAXOVERLAP+4) * sizeof (float));

  for(int ii = 0; ii < (MAXOVERLAP+4); ii++) overbuff[ii] = 0.0;

  canProcessReplacing();
  strcpy(programName, "Broken FFT");
}

Fft::~Fft() {
  free(overbuff);
  free(ampl);

  free(fftr);
  free(ffti);
  free(tmp);
  free(oot);

  free(echor);
  free(echoc);
}

void Fft::setProgramName(char *name) {
  strcpy(programName, name);
}

void Fft::getProgramName(char *name) {
  strcpy(name, programName);
}

void Fft::setParameter(long index, float value) {
  switch (index) {
  case 0: {
    bitres = value;
    int nOVERLAP = (int)(MAXOVERLAP * (1.0 - value));

    if (nOVERLAP > OVERLAP) {
      for (int i=OVERLAP; i < nOVERLAP; i++) {
	    overbuff[i] = samplehold * (rand()/(float)RAND_MAX);
      }
    }
    OVERLAP = nOVERLAP;
  }
  break;
  case 1:
    samplehold = value;
    break;
  case 2: 
    destruct = value;
    break;
  case 3:
    perturb = value;
    break;
  case 4:
    quant = value;
    break;
  case 5:
    rotate = value;
    break;
  case 10:
    binquant = value;
    break;
  case 7:
    spike = value;
    break;
  case 8:
    compress = value;
    break;
  case 9:
    makeupgain = value;
    break;
  case 6:
    spikehold = value;
    break;
  case 11:
    break; /* unused */
  case 12:
    echomix = value;
    break;
  case 13:
    echotime = value;
    break;
  case 14:
    echomodw = value;
    break;
  case 15:
    echomodf = value * 2.0 * pi;
    break;
  case 16:
    echofb = value;
    break;
  case 17:
    if (value < echohi) echolow = value;
    break;
  case 18:
    if (value > echolow) echohi = value;
    break;
  default:
    break;
  }
}

float Fft::getParameter(long index) {
  switch (index) {
  default:
  case 0: return bitres;
  case 1: return samplehold;
  case 2: return destruct;
  case 3: return perturb;
  case 4: return quant;
  case 5: return rotate;
  case 10: return binquant;
  case 7: return spike;
  case 8: return compress;
  case 9: return makeupgain;
  case 6: return spikehold;
  case 12: return echomix;
  case 13: return echotime;
  case 14: return echomodw;
  case 15: return (echomodf / (2.0 * pi));
  case 16: return echofb;
  case 17: return echolow;
  case 18: return echohi;
  }
}

void Fft::getParameterName(long index, char *label) {
  switch(index) {
  case 0:
    strcpy(label, "OVERLAP");
    break;
  case 1:
    strcpy(label, "inter-OVRLF");
    break;
  case 2:
    strcpy(label, "very special");
    break;
  case 3:
    strcpy(label, "soft noise");
    break;
  case 4:
    strcpy(label, "operation q");
    break;
  case 5:
    strcpy(label, "rotate");
    break;
  case 10:
    strcpy(label, "BQ max");
    break;
  case 7:
    strcpy(label, "spike");
    break;
  case 8:
    strcpy(label, "compress");
    break;
  case 9:
    strcpy(label, "M.U.G.");
    break;
  case 6:
    strcpy(label, "spike-X");
    break;
  case 12:
    strcpy(label, "EO mix");
    break;
  case 13:
    strcpy(label, "EO space");
    break;
  case 14:
    strcpy(label, "EO opn w");
    break;
  case 15:
    strcpy(label, "EO opn f");
    break;
  case 16:
    strcpy(label, "EO rept");
    break;
  case 17:
    strcpy(label, "EO <<");
    break;
  case 18:
    strcpy(label, "EO >>");
    break;
  default:
    strcpy(label, "?");
    break;
  }
}

void Fft::getParameterDisplay(long index, char *text) {
  switch(index) {
  case 4:
    float2string(sin(quant), text);
    break;
  case 5:
    { int uu = (int)(rotate * 0x7FFFFFFF);
    for(int i = 0;
	i < 6;
	i ++) {
      text[i] = "X1234567890qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM!@#$%^&*()"[uu&63];
      uu ^= 0x23980123;
      uu *= 0x3311;
      uu >>= 4;
      text[i+1] = 0;
    }
    }
    break;
  case 10:
    float2string(binquant * binquant * binquant, text);
    break;
  case 7:
    if (spike >= 0.999999) strcpy(text, "...");
    else float2string(spike, text);
    break;
  case 9:
    float2string(makeupgain * makeupgain, text);
    break;
  case 6:
    float2string(spikehold * 20.0, text);
    break;
  case 12:
    float2string(echomix * sqrt(echotime), text);
    break;
  default:
    float2string(getParameter(index), text);
    break;
  }
}

void Fft::getParameterLabel(long index, char *label) {
  strcpy(label, "units");
}

inline float quantize(float q, float old) {
  /*	if (q >= 1.0) return old;*/
  long scale = ( long)(512 * q);
  if (scale == 0) scale = 2;
  long X = ( long)(scale * old);
	
  return (float)(X / (float)scale);
}

void Fft::suspend () {
  memset (echoc, 0, MAXECHO * sizeof (float));
  memset (echor, 0, MAXECHO * sizeof (float));
  echoctr = 0;
}


/* tqsort partitions the array (of n elements) so that the first STOP
   elements are each less than the remaining elements. */

void tqsort(amplentry * low, int n, int stop) {

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

/* this function modifies 'samples' number of floats in
   fftr and ffti */
void Fft::fftops(long samples) {
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
      
}


/* wavelab: sampleframes = 2048 */

void Fft::processX(float **inputs, float **outputs, long samples,
		   int overwrite) {
  float * in  = *inputs;
  float * out = *outputs;

  int i = 0;
  /* maybe here we can pad up to the next biggest power of two? */
  if (samples & (samples-1)) {
    /* not a power of two */
    /*    MessageBox(0, "block size not a power of two", 
	  "can't proceed", MB_OK);*/

    /* FIXME!!!! do something...  */

    return;
  }

  if (!ampl || samples > amplsamples) { 
    amplhold = 1; /* recalc this time */
    free(ampl);
    ampl = (amplentry*)malloc(sizeof (amplentry) * samples);
    amplsamples = samples;
  }

  if (samples > buffersamples) {

    free(fftr);
    free(ffti);
    free(tmp);
    free(oot);

    samples = buffersamples;

    fftr = (float*)malloc(buffersamples * sizeof(float));
    ffti = (float*)malloc(buffersamples * sizeof(float));
    tmp =  (float*)malloc(buffersamples * sizeof(float));
    oot =  (float*)malloc(buffersamples * sizeof(float));

  }

  for (int c = 0; c < samples; c++) tmp[c] = in[c];

  fft_float(samples, 0, tmp, 0, fftr, ffti);

  /* do actual processing */
  fftops(samples);

  fft_float(samples, 1, fftr, ffti, oot, tmp);

  float overscale;
  if (samples >= OVERLAP) overscale = 1.0;
  else overscale =  (samples-OVERLAP) / (float)samples;

  /* process samples, fading at the beginning and stretching
     for an extra OVERLAP samples */
  
  if (overwrite) {
    for(i = 0; i < samples; i++) {
      if (i < OVERLAP) {
	out[i] = oot[(int)(i * overscale)] * (i / (float)OVERLAP) +
	  overbuff[i] * ((OVERLAP-i) / (float)OVERLAP);
      } else { 
	out[i] = oot[(int)(i * overscale)];
      }
    }
  } else {
    for(i = 0; i < samples; i++) {
      if (i < OVERLAP) {
	out[i] += oot[(int)(i * overscale)] * (i / (float)OVERLAP) +
	  overbuff[i] * ((OVERLAP-i) / (float)OVERLAP);
      } else { 
	out[i] += oot[(int)(i * overscale)];
      }
    }
  }

  int u = 0;
  for(i = (i * overscale) + 1; i < samples; i ++) {
    overbuff[u++] = oot[i];
  }

}

void Fft::process(float **inputs, float **outputs, long samples) {
  processX(inputs,outputs,samples,0);
}

void Fft::processReplacing(float **inputs, float **outputs, long samples) {
  processX(inputs,outputs,samples,1);
}


/* ------------------- boring! --------------------- */

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

  effect = new Fft (audioMaster);
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
