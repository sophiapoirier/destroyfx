/* Geometer,
   Featuring the Super Destroy FX Windowing System! */

#include "geometer.hpp"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <math.h>

#define MKBUFSIZE(c) (buffersizes[(int)((c)*(BUFFERSIZESSIZE-1))])

const int PLUGIN::buffersizes[BUFFERSIZESSIZE] = { 
  2, 4, 8, 16, 32, 64, 128, 256, 512, 
  1024, 2048, 4096, 8192, 16384, 32768, 
};

int intcompare(const void * a, const void * b);
int intcompare(const void * a, const void * b) {
  return (*(int*)a - *(int*)b);
}

PLUGIN::PLUGIN(audioMasterCallback audioMaster)
  : AudioEffectX(audioMaster, NUM_PROGRAMS, NUM_PARAMS) {

  FPARAM(bufsizep, P_BUFSIZE, "wsize", 0.5, "samples");
  FPARAM(shape, P_SHAPE, "wshape", 0.0, "");

  FPARAM(pointstyle, P_POINTSTYLE, "points where", 0.0, "choose");
  FPARAM(pointparam, P_POINTPARAM, "freq", 0.1f, "percent");
  FPARAM(interpstyle, P_INTERPSTYLE, "interp how", 0.0, "choose");

  FPARAM(pointop1, P_POINTOP1, "pointop1", 0.5f, "choose");
  FPARAM(pointop2, P_POINTOP2, "pointop2", 0.5f, "choose");
  FPARAM(pointop3, P_POINTOP3, "pointop3", 0.5f, "choose");

  FPARAM(oppar1, P_OPPAR1, "opparm1", 0.5f, "???");
  FPARAM(oppar2, P_OPPAR2, "opparm1", 0.5f, "???");
  FPARAM(oppar3, P_OPPAR3, "opparm1", 0.5f, "???");

  long maxframe = 0;
  for (long i=0; i<BUFFERSIZESSIZE; i++)
    maxframe = ( buffersizes[i] > maxframe ? buffersizes[i] : maxframe );

  setup();

  in0 = (float*)malloc(maxframe * sizeof (float));
  out0 = (float*)malloc(maxframe * 2 * sizeof (float));

  /* prevmix is only a single third long */
  prevmix = (float*)malloc((maxframe / 2) * sizeof (float));

  /* geometer buffers */
  pointx = (int*)malloc((maxframe * 2 + 3) * sizeof (int));
  tempx = (int*)malloc((maxframe * 2 + 3) * sizeof (int));

  pointy = (float*)malloc((maxframe * 2 + 3) * sizeof (float));
  tempy = (float*)malloc((maxframe * 2 + 3) * sizeof (float));


  /* resume sets up buffers and sizes */
  changed = 1;
  resume ();
}

PLUGIN::~PLUGIN() {
  free (in0);
  free (out0);

  free (prevmix);

  free(pointx);
  free(pointy);
  free(tempx);
  free(tempy);

  if (programs) delete[] programs;
}

void PLUGIN::resume() {

  if (changed) ioChanged();

  framesize = MKBUFSIZE(bufsizep);
  third = framesize / 2;
  bufsize = third * 3;

  /* set up buffers. Prevmix and first frame of output are always filled with zeros. */

  for (int i = 0; i < third; i ++) {
    prevmix[i] = 0.0;
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
    /* windowing */
  case P_BUFSIZE:
    sprintf(text, "%d", MKBUFSIZE(bufsizep));
    break;
  case P_SHAPE:
    if (shape < 0.20f) {
      strcpy(text, "linear");
    } else if (shape < 0.40f) {
      strcpy(text, "arrow");
    } else if (shape < 0.60f) {
      //      float s = 1.0 / (shape - shape);
      //      sprintf(text, "%f", s);
      strcpy(text, "wedge");
    } else if (shape < 0.80f) {
      strcpy(text, "best");
    } else {
      strcpy(text, "cos^2");
    }
    break;
    /* geometer */
  case P_POINTSTYLE:
    if (pointstyle < 0.25) strcpy(text, "ext 'n cross");
    else if (pointstyle < 0.50) strcpy(text, "at freq");
    else if (pointstyle < 0.75) strcpy(text, "randomly");
    else strcpy(text, "dy/dx zero-cross");
    break;
  case P_INTERPSTYLE:
    if (interpstyle < 0.10) strcpy(text, "polygon");
    else if (interpstyle < 0.20) strcpy(text, "wrongygon");
    else if (interpstyle < 0.30) strcpy(text, "reversi");
    else if (interpstyle < 0.40) strcpy(text, "smoothie");
    else if (interpstyle < 0.90) strcpy(text, "pulse-debug");
    else strcpy(text, "unsup");
    break;
  case P_POINTOP1:
  case P_POINTOP2:
  case P_POINTOP3:
    if (*paramptrs[index].ptr < 0.10) strcpy(text, "1/4");
    else if (*paramptrs[index].ptr < 0.20) strcpy(text, "1/2");
    else if (*paramptrs[index].ptr < 0.60) strcpy(text, "no");
    else strcpy(text, "x 2");
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

/* operations on points. this is a separate function
   because it is called once for each operation slot.
*/
int PLUGIN::pointops(float pop, int numpts, float op_param) {
  /* pointops. */

  int maxpts = framesize * 2;

  if (pop < 0.20) {
    int times = 1;
    if (pop < 0.10) times = 2;

    for(int t = 0; t < times; t++) {
      /* cut points in half. never touch first or last. */
      int q = 1, i = 1;
      for(i=1; q < (numpts - 1); i++) {
        pointx[i] = pointx[q];
        pointy[i] = pointy[q];
        q += 2;
      }
      pointx[i] = pointx[numpts - 1];
      pointy[i] = pointy[numpts - 1];
      numpts = i+1;
    }
  } else if (pop < 0.60) { 
    /* nothing */ 
  } else {
    /* x2 points */
    int i = 0, t;
    for(t = 0; i < (numpts - 1) && t < (maxpts-2); i++) {
      tempx[t] = pointx[i];
      tempy[t] = pointy[i];
      t++;
      /* now, only if there's room... */
      if (pointx[i+1] - pointx[i] > 1) {
	/* add an extra point. Pick it in some arbitrary weird way. 
	   my idea is to double the frequency ...
	*/
      
	tempy[t] = (op_param * 2.0 - 1.0) * pointy[i];
	tempx[t] = (pointx[i] + pointx[i+1]) >> 1;

	t++;
      }

    }
    /* always include last */
    tempx[t] = pointx[numpts-1];
    tempy[t] = pointy[numpts-1];
    t++;

    for(int c = 0; c < t; c++) {
      pointx[c] = tempx[c];
      pointy[c] = tempy[c];
    }
    numpts = t + 1;
  }


  return numpts;
}

/* this processes an individual window.
   1. generate points
   2. do operations on points (in slots op1, op2, op3)
   3. generate waveform
*/
void PLUGIN::processw(float * in, float * out, long samples) {

  /* collect points. */

  pointx[0] = 0;
  pointy[0] = in[0];
  int numpts = 1;

  int maxpts = framesize * 2;

  if (pointstyle < 0.25) {
    /* extremities and crossings */

    float ext = 0.0;
    int extx = 0;
    
    enum {SZ, SZC, SA, SB};
    int state;

    state = SZ;

    for(int i = 0 ; i < samples ; i ++) {
      switch(state) {
      case SZ: {
        /* just output a zero. */
        if (in[i] <= pointparam && in[i] >= -pointparam) state = SZC;
        else if (in[i] < -pointparam) { state = SB; ext = in[i]; extx = i; }
        else { state = SA; ext = in[i]; extx = i; }
        break;
      }
      case SZC: {
        /* continuing zeros */
        if (in[i] <= pointparam && in[i] >= -pointparam) break;
      
        /* push zero for last spot (we know it was a zero and not pushed). */
        if (numpts < (maxpts-1)) {
          pointx[numpts] = (i>0)?(i - 1):0;
          pointy[numpts] = 0.0;
          numpts++;
        } 

        if (in[i] < 0.0) { 
          state = SB;
        } else {
          state = SA;
        }

        ext = in[i];
        extx = i;
        break;
      }
      case SA: {
        /* above zero */

        if (in[i] <= pointparam) {
          /* no longer above 0. push the highest point I reached. */
          if (numpts < (maxpts-1)) {
            pointx[numpts] = extx;
            pointy[numpts] = ext;
            numpts++;
          } 
          /* and decide state */
          if (in[i] >= -pointparam) {
            if (numpts < (maxpts-1)) {
              pointx[numpts] = i;
              pointy[numpts] = 0.0;
              numpts++;
            } 
            state = SZ;
          } else {
            state = SB;
            ext = in[i];
            extx = i;
          }
        } else {
          if (in[i] > ext) {
            ext = in[i];
            extx = i;
          }
        }
        break;
      }
      case SB: {
        /* below zero */

        if (in[i] >= -pointparam) {
          /* no longer below 0. push the lowest point I reached. */
          if (numpts < (maxpts-1)) {
            pointx[numpts] = extx;
            pointy[numpts] = ext;
            numpts++;
          } 
          /* and decide state */
          if (in[i] <= pointparam) {
            if (numpts < (maxpts-1)) {
              pointx[numpts] = i;
              pointy[numpts] = 0.0;
              numpts++;
            } 
            state = SZ;
          } else {
            state = SA;
            ext = in[i];
            extx = i;
          }
        } else {
          if (in[i] < ext) {
            ext = in[i];
            extx = i;
          }
        }
        break;
      }
      default:;

      }
    }
  } else if (pointstyle < 0.50) {
    /* at frequency */
    
    /* XXX let the user choose hz, do conversion */
    int nth = (pointparam * pointparam) * samples;
    int ctr = nth;
  
    for(int i = 0; i < samples; i ++) {
      ctr--;
      if (ctr <= 0) {
        if (numpts < (maxpts-1)) {
          pointx[numpts] = i;
          pointy[numpts] = in[i];
          numpts++;
        } else break; /* no point in continuing... */
        ctr = nth;
      }
    }

  } else if (pointstyle < 0.75) {
    /* randomly */

    int n = (1.0 - pointparam) * samples;

    for(;n --;) {
      if (numpts < (maxpts-1)) {
	pointx[numpts++] = rand() % samples;
      } else break;
    }

    /* sort them */

    qsort(pointx, numpts, sizeof (int),
	  intcompare);

    for (int sd = 0; sd < numpts; sd++) {
      pointy[sd] = in[pointx[sd]];
    }

  } else {
    int lastsign = 0;
    float lasts = in[0];
    int sign;

    pointx[0] = 0;
    pointy[0] = in[0];
    numpts = 1;
    
    for(int i = 1; i < samples; i++) {
      
      if (pointparam > 0.5) {
	float pp = pointparam - 0.5;
	sign = (in[i] - lasts) > (pp * pp);
      } else {
	float pp = 0.5 - pointparam;
	sign = (in[i] - lasts) < (pp * pp);
      }

      lasts = in[i];

      if (sign != lastsign) {
	pointx[numpts] = i;
	pointy[numpts] = in[i];
	numpts++;
	if (numpts > (maxpts-1)) break;
      }

      lastsign = sign;
    }
    

  }

  /* always push final point for continuity (we saved room) */
  pointx[numpts] = samples-1;
  pointy[numpts] = in[samples-1];
  numpts++;

  numpts = pointops(pointop1, numpts, oppar1);
  numpts = pointops(pointop2, numpts, oppar2);
  numpts = pointops(pointop3, numpts, oppar3);

  if (interpstyle < 0.10) {
    /* linear interpolation - "polygon" */

    for(int u=1; u < numpts; u ++) {
      float denom = (pointx[u] - pointx[u-1]);
      for(int z=pointx[u-1]; z < pointx[u]; z++) {
        float pct = (float)(z-pointx[u-1]) / denom;
        float s = pointy[u-1] * (1.0 - pct) +
          pointy[u]   * pct;
        out[z] = s;
      }
    }

    out[samples-1] = in[samples-1];

  } else if (interpstyle < 0.20) {
    /* linear interpolation, wrong direction - "wrongygon" */

    for(int u=1; u < numpts; u ++) {
      float denom = (pointx[u] - pointx[u-1]);
      for(int z=pointx[u-1]; z < pointx[u]; z++) {
        float pct = (float)(z-pointx[u-1]) / denom;
        float s = pointy[u-1] * pct +
          pointy[u]   * (1.0 - pct);
        out[z] = s;
      }
    }

    out[samples-1] = in[samples-1];

  } else if (interpstyle < 0.30) {
    /* x-reverse input samples for each waveform - "reversi" */

    for(int u=1; u < numpts; u ++) {
      if (pointx[u-1] < pointx[u])
	for(int z = pointx[u-1]; z < pointx[u]; z++) {
	  int s = (pointx[u] - (z + 1)) + pointx[u - 1];
	  out[z] = in[(s>0)?s:0];
	}
    }

  } else if (interpstyle < 0.40) {
    /* cosine up or down - "smoothie" */

    for(int u=1; u < numpts; u ++) {
      float denom = (pointx[u] - pointx[u-1]);
      for(int z=pointx[u-1]; z < pointx[u]; z++) {
        float pct = (float)(z-pointx[u-1]) / denom;
        
        float p = 0.5f * (-cos(float(pi * pct)) + 1.0f);

        float s = pointy[u-1] * (1.0 - p) + pointy[u]   * p;

        out[z] = s;
      }
    }

    out[samples-1] = in[samples-1];

  } else if (interpstyle < 0.90) {

    /* XXX - how about generalizing this to a shaped pulse of 
       constant width */

    for(int i = 0; i < samples; i++) out[i] = 0.0;

    for(int z = 0; z < numpts; z ++) { out[pointx[z]] += pointy[z]; }

  } else {

    /* unsupported ... ! */
    for(int i = 0; i < samples; i++) out[i] = 0.0;
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
   - can we use circular buffers instead of memmoving a lot?
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
