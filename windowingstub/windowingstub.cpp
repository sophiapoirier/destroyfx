
/* Super Destroy FX Windowing System! */

#include "windowingstub.hpp"

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

  FPARAM(bufsizep, 0, "wsize", 0.5, "samples");
  FPARAM(shape, 1, "shape", 0.0, "");

  long maxframe = 0;
  for (long i=0; i<BUFFERSIZESSIZE; i++)
    maxframe = ( buffersizes[i] > maxframe ? buffersizes[i] : maxframe );
  int maxbufsize = (maxframe / 2) * 3;
  setup();

  in0 = (float*)malloc(maxframe * sizeof (float));
  out0 = (float*)malloc(maxbufsize * sizeof (float));

  /* prevmix is only a single third long */
  prevmix = (float*)malloc(maxframe / 2 * sizeof (float));

  /* resume sets up buffers and sizes */
  resume ();
}

PLUGIN::~PLUGIN() {

  free (in0);
  free (out0);

  free (prevmix);
}

void PLUGIN::resume() {

  framesize = MKBUFSIZE(bufsizep);
  third = framesize / 2;
  bufsize = third * 3;

  /* set up buffers. Prevmix and first third of output are always filled with zeros. */

  for (int i = 0; i < third; i ++) {
    prevmix[i] = 0.0;
    out0[i] = 0.0;
  }
  
  /* start input at beginning. Output has a third of silence. */
  insize = 0;
  outstart = 0;
  outsize = framesize;

}

void PLUGIN::suspend () {
  /* nothing to do here. */

}

void PLUGIN::setParameter(long index, float value) {
  switch (index) {
  default:
    if (index >= 0 && index < NUM_PARAMS)
      *paramptrs[index].ptr = value;
    /* otherwise, ??? */
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
    sprintf(text, "%d", MKBUFSIZE(bufsizep));
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

/* this is where you should write your real processing
   function. It will be called on overlapping input frames,
   but it doesn't need to know about that.

   Keep in mind that, for instance, a delay-like plug
   that keeps a history of samples it's seen will not work
   right here, since processw does not receive adjacent
   frames. It might be possible to use two delay buffers,
   toggling between them with each call, but it's probably
   best to just use the windowing setup for functions that
   operate entirely within one buffer.
*/
void PLUGIN::processw(float * in, float * out, long samples) {

  static int ss;
  static int t;
  ss = !ss;

#if 1
  for (int i =0 ; i < samples; i++) out[i] = (float)ss;

#else

#if 1
  /* test with sine tones */
  if (ss) {
    for(int i = 0; i < samples; i ++) {
      t++;
      out[i] = sin(t/110.0);
    }
  } else {
    for(int i = 0; i < samples; i ++) {
      t++;
      out[i] = sin(t/37.0);
    }
  }
#else

  /* reverse */

  for(int i = 0; i < samples; i ++) out[i] = in[samples - (i + 1)];
#endif
#endif
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
   - new, smoother interpolation shapes.
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

      /* apply envelope */

      if (shape < 0.33) {
	for(int z = 0; z < third; z++) {
	  float p = sqrt(z / (float)third);
	  out0[z+outstart+outsize] *= p;
	  out0[z+outstart+outsize+third] *= (1.0 - p);
	}
      } else if (shape < 0.66) {
	for(int z = 0; z < third; z++) {
	  float p = z / (float)third;
	  out0[z+outstart+outsize] *= (p*p);
	  out0[z+outstart+outsize+third] *= (1.0 - (p*p));
	}
      } else {
	for(int z = 0; z < third; z++) {
	  out0[z+outstart+outsize] *= (z / (float)third);
	  out0[z+outstart+outsize+third] *= (1.0 - (z / (float)third));
	}
      }

      /* mix in prevmix */
      for(int u = 0; u < third; u ++)
	out0[u+outstart+outsize] += prevmix[u];

      /* prevmix becomes out1 */
      memcpy(prevmix, out0 + outstart + outsize + third, third * sizeof (float));

      /* XXX destroy old out1 - debug only. */
      for(int q = 0; q < third; q ++)
	out0[outstart+outsize+third+q] = rand();

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

void PLUGIN::processReplacing(float **inputs, float **outputs, long samples) {
  processX(inputs,outputs,samples, 1);
}

void PLUGIN::process(float **inputs, float **outputs, long samples) {
  processX(inputs,outputs,samples, 0);
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
  void* hInstance;
  BOOL WINAPI DllMain (HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved) {
    hInstance = hInst;
    return 1;
  }
#endif
