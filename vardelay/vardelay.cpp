/*---------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2023  Sophia Poirier

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

Super Destroy FX : Vardelay!
---------------------------------------------------------------*/

#include "vardelay.hpp"

#include <algorithm>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <math.h>

#define MKBUFSIZE(c) (buffersizes[(int)((c)*(buffersizes.size()-1))])


PLUGIN::PLUGIN(audioMasterCallback audioMaster)
  : AudioEffectX(audioMaster, NUM_PROGRAMS, NUM_PARAMS) {

  FPARAM(bufsizep, P_BUFSIZE, "wsize", 0.5f, "samples");
  FPARAM(shape, P_SHAPE, "shape", 0.0f, "");
  
  FPARAM(rigid, P_RIGID, "rigid", 0.0f, "?");

  FPARAM(usebands, P_USEBANDS, "usebands", 0.4f, "number");
  for(int zz=0; zz < BANDS; zz++) {
    FPARAM(band[zz], P_DELAYS + zz, "band", 0.0f, "dist");
  }

  constexpr auto maxframe = *std::ranges::max_element(buffersizes);

  setup();

  in0 = (float*)malloc(maxframe * sizeof (float));
  out0 = (float*)malloc(maxframe * 2 * sizeof (float));

  /* prevmix is only a single third long */
  prevmix = (float*)malloc((maxframe / 2) * sizeof (float));

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

  /* set up buffers. Prevmix and first frame of output are always filled with zeros. */

  for (int i = 0; i < third; i ++) {
    prevmix[i] = 0.0f;
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
long PLUGIN::getGetTailSize() { return framesize; }

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

  if (editor)
    ((AEffGUIEditor*)editor)->setParameter(index, value);
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
      strcpy(text, "chunky");
    }
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

  /* XXX memset */
  for(int u=0; u < samples; u ++) out[u] = 0.0f;

  float bandsize = 1.0f / (usebands * .99f * BANDS);
  for(int i=0; i < samples; i ++) {
    unsigned int which = fabsf(in[i]) / bandsize;
    if (which >= (BANDS-1)) which = BANDS - 2;

    /* XXX when writing here, do anti-aliasing */
    if (rigid > 0.5f) {
      /* read band data, put sample */
      unsigned int dest = i + (band[which] * (samples / 4));
      /* only write it if it falls in this frame (otherwise
	 we should get it when we do the second pass on
	 the window */
      if (dest < samples)
	out[dest] += in[i];
    } else {
      /* how far into the bin is it? */
      float leak = fabs(in[i]) - (float)which * bandsize;
      if (leak > bandsize) leak = bandsize;

      /* now mix a little bit of band and band + 1 */

      unsigned int dest1 = i + (band[which] * (samples / 4));
      if (dest1 < samples) out[dest1] += in[i] * (1.0f - (leak/bandsize));
      unsigned int dest2 = i + (band[which+1] * (samples / 4));
      if (dest2 < samples) out[dest2] += in[i] * (leak/bandsize);

    }
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
	/* 'second half */
	int z;
	for(z = 0; z < (third>>1); z++)
	  out0[z+outstart+outsize] = 0.0f;

	for(; z < third; z ++)
	  out0[z+outstart+outsize+third] = 0.0f;

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

  AudioEffect *effect = new PLUGIN (audioMaster);
  if (!effect)
    return 0;
  return effect->getAeffect();
}

#if WIN32
  void* hInstance;
  BOOL WINAPI DllMain (HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved) {
    hInstance = hInst;
    return 1;
  }
#endif
#endif
