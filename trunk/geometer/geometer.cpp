
/* Geometer,
   Featuring the Super Destroy FX Windowing System! */

#include "geometer.hpp"

#ifndef __geometereditor
#include "geometereditor.hpp"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <math.h>

#define MKBUFSIZE(c) (buffersizes[(int)((c)*(BUFFERSIZESSIZE-1))])

const int PLUGIN::buffersizes[BUFFERSIZESSIZE] = { 
  4, 8, 16, 32, 64, 128, 256, 512, 
  1024, 2048, 4096, 8192, 16384, 32768, 
};

int intcompare(const void * a, const void * b);
int intcompare(const void * a, const void * b) {
  return (*(int*)a - *(int*)b);
}

PLUGIN::PLUGIN(audioMasterCallback audioMaster)
  : AudioEffectX(audioMaster, NUM_PROGRAMS, NUM_PARAMS) {

  FPARAM(bufsizep, P_BUFSIZE, "wsize", 0.7f, "samples");
  FPARAM(shape, P_SHAPE, "wshape", 0.0f, "");

  FPARAM(pointstyle, P_POINTSTYLE, "points where", 0.0f, "choose");

  FPARAM(pointparam[0], P_POINTPARAMS + 0, "point:ext'n'cross", 0.0f, "magn");
  FPARAM(pointparam[1], P_POINTPARAMS + 1, "point:freq", 0.08f, "%");
  FPARAM(pointparam[2], P_POINTPARAMS + 2, "point:rand", 0.20f, "%");
  FPARAM(pointparam[3], P_POINTPARAMS + 3, "point:span", 0.20f, "width");
  FPARAM(pointparam[4], P_POINTPARAMS + 4, "point:dydx", 0.50f, "gap");
  FPARAM(pointparam[5], P_POINTPARAMS + 5, "point:level", 0.50f, "level");

  for(int pp = NUM_POINTSTYLES; pp < MAX_POINTSTYLES; pp++) {
    FPARAM(pointparam[pp], P_POINTPARAMS + pp, 
	   "pointparam:unused", 0.04f, "??");
  }

  FPARAM(interpstyle, P_INTERPSTYLE, "interpolate how", 0.0f, "choose");

  FPARAM(interparam[0], P_INTERPARAMS + 0, "interp:polygon", 0.0f, "angle");
  FPARAM(interparam[1], P_INTERPARAMS + 1, "interp:wrongy", 0.0f, "angle");
  FPARAM(interparam[2], P_INTERPARAMS + 2, "interp:smoothie", 0.5f, "exp");
  FPARAM(interparam[3], P_INTERPARAMS + 3, "interp:reversie", 0.0f, "nothing");
  FPARAM(interparam[4], P_INTERPARAMS + 4, "interp:pulse", 0.05f, "pulse");
  FPARAM(interparam[5], P_INTERPARAMS + 5, "interp:friends", 1.0f, "width");
  FPARAM(interparam[6], P_INTERPARAMS + 6, "interp:sing", 0.8f, "mod");
  FPARAM(interparam[7], P_INTERPARAMS + 7, "interp:shuffle", 0.3f, "amount");

  for(int ip = NUM_INTERPSTYLES; ip < MAX_INTERPSTYLES; ip++) {
    FPARAM(interparam[ip], P_INTERPARAMS + ip, 
	   "inter:unused", 0.0f, "???");
  }

  FPARAM(pointop1, P_POINTOP1, "pointop1", UNMKPOINTOP(OP_NONE), "choose");
  FPARAM(pointop2, P_POINTOP2, "pointop2", UNMKPOINTOP(OP_NONE), "choose");
  FPARAM(pointop3, P_POINTOP3, "pointop3", UNMKPOINTOP(OP_NONE), "choose");

#define ALLOP(n, str, def, unit) \
  do { \
    FPARAM(oppar1[n], P_OPPAR1S + n, "op1:" str, def, unit); \
    FPARAM(oppar2[n], P_OPPAR2S + n, "op2:" str, def, unit); \
    FPARAM(oppar3[n], P_OPPAR3S + n, "op3:" str, def, unit); \
  } while (0)

  ALLOP(0, "double", 0.5f, "amp");
  ALLOP(1, "half", 0.0f, "nothing");
  ALLOP(2, "quarter", 0.0f, "nothing");
  ALLOP(3, "longpass", 0.15f, "length");
  ALLOP(4, "shortpass", 0.5f, "length");
  ALLOP(5, "slow", 0.25f, "factor");
  ALLOP(6, "fast", 0.5f, "factor");
  ALLOP(7, "none", 0.0f, "nothing");
  
  for(int op = NUM_OPS; op < MAX_OPS; op++) {
    ALLOP(op, "unused", 0.5f, "???");
  }

  long maxframe = 0;
  for (long i=0; i<BUFFERSIZESSIZE; i++)
    maxframe = ( buffersizes[i] > maxframe ? buffersizes[i] : maxframe );

  setup();

  /* add some leeway? */
  in0 = (float*)malloc(maxframe * sizeof (float));
  out0 = (float*)malloc(maxframe * 2 * sizeof (float));

  /* prevmix is only a single third long */
  prevmix = (float*)malloc((maxframe / 2) * sizeof (float));

  /* geometer buffers */
  pointx = (int*)malloc((maxframe * 2 + 3) * sizeof (int));
  storex = (int*)malloc((maxframe * 2 + 3) * sizeof (int));

  pointy = (float*)malloc((maxframe * 2 + 3) * sizeof (float));
  storey = (float*)malloc((maxframe * 2 + 3) * sizeof (float));

  /* XXX doesn't take self pointer now? */
  /* Marc, is this right? */
  chunk = new VstChunk(NUM_PARAMS, NUM_PROGRAMS, PLUGINID, this);
  /* since we don't use notes for any specialized control of Geometer, 
     allow them to be assigned to control parameters via MIDI learn */
  chunk->setAllowPitchbendEvents(true);
  chunk->setAllowNoteEvents(true);

  /* resume sets up buffers and sizes */
  changed = 1;
  
  cs = new dfxmutex();

  editor = new GeometerEditor(this);

  resume ();
}

PLUGIN::~PLUGIN() {
  free (in0);
  free (out0);

  free (prevmix);

  free(pointx);
  free(pointy);
  free(storex);
  free(storey);

  delete cs;

  if (chunk) delete chunk;

  if (programs) delete[] programs;
}

void PLUGIN::resume() {
  /* XXX - I moved ioChanged to be called whenever the param is changed.
     Do I still need it here?
  */
  //  if (changed) ioChanged();

  framesize = MKBUFSIZE(bufsizep);
  third = framesize / 2;
  bufsize = third * 3;

  /* set up buffers. Prevmix and first frame of output are always 
     filled with zeros. */

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

  if (changed) setInitialDelay(framesize);

  changed = 0;

  wantEvents(); // tells the host that we like to get MIDI
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
    ioChanged();
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

/* save & restore parameter data using chunks so that we 
   can save MIDI CC assignments */
long PLUGIN::getChunk(void **data, bool isPreset) {
  return chunk->getChunk(data, isPreset);
}

long PLUGIN::setChunk(void *data, long byteSize, bool isPreset) {
  return chunk->setChunk(data, byteSize, isPreset);
}

/* process MIDI events */
long PLUGIN::processEvents(VstEvents* events) {
  /* manage parameter automation via MIDI CCs */
  chunk->processParameterEvents(events);
  /* manage program changes via MIDI */
  processProgramChangeEvents(events, this);
  /* tells the host to keep sending events */
  return 1;
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
    /* windowing */
  case P_BUFSIZE:
    sprintf(text, "%d", MKBUFSIZE(bufsizep));
    break;
  case P_SHAPE:
    switch(MKWINDOWSHAPE(shape)) {
    case WINDOW_TRIANGLE:
      strcpy(text, "linear");
      break;
    case WINDOW_ARROW:
      strcpy(text, "arrow");
      break;
    case WINDOW_WEDGE:
      strcpy(text, "wedge");
      break;
    case WINDOW_COS:
      strcpy(text, "best");
      break;
    default:
      strcpy(text, "???");
      break;
    }
    break;
    /* geometer */
  case P_POINTSTYLE:
    switch(MKPOINTSTYLE(pointstyle)) {
    case POINT_EXTNCROSS:
      strcpy(text, "ext 'n cross");
      break;
    case POINT_LEVEL:
      strcpy(text, "at level");
      break;
    case POINT_FREQ:
      strcpy(text, "at freq");
      break;
    case POINT_RANDOM:
      strcpy(text, "randomly");
      break;
    case POINT_SPAN:
      strcpy(text, "span");
      break;
    case POINT_DYDX:
      strcpy(text, "dy/dx");
      break;
    default:
      strcpy(text, "unsup");
      break;
    }
    break;
  case P_INTERPSTYLE:
    switch(MKINTERPSTYLE(interpstyle)) {
    case INTERP_POLYGON:
      strcpy(text, "polygon");
      break;
    case INTERP_WRONGYGON:
      strcpy(text, "wrongygon");
      break;
    case INTERP_SMOOTHIE:
      strcpy(text, "smoothie");
      break;
    case INTERP_REVERSI:
      strcpy(text, "reversi");
      break;
    case INTERP_PULSE:
      strcpy(text, "pulse-debug");
      break;
    case INTERP_FRIENDS:
      strcpy(text, "friends");
      break;
    case INTERP_SING:
      strcpy(text, "sing");
      break;
    case INTERP_SHUFFLE:
      strcpy(text, "shuffle");
      break;
    default:
      strcpy(text, "unsup");
      break;
    }
    break;
  case P_POINTOP1:
  case P_POINTOP2:
  case P_POINTOP3:
    switch(MKPOINTOP(*paramptrs[index].ptr)) {
    case OP_DOUBLE:
      strcpy(text, "x2");
      break;
    case OP_HALF:
      strcpy(text, "1/2");
      break;
    case OP_QUARTER:
      strcpy(text, "1/4");
      break;
    case OP_LONGPASS:
      strcpy(text, "longpass");
      break;
    case OP_SHORTPASS:
      strcpy(text, "shortpass");
      break;
    case OP_SLOW:
      strcpy(text, "slow");
      break;
    case OP_FAST:
      strcpy(text, "fast");
      break;
    case OP_NONE:
      strcpy(text, "none");
      break;
    default:
      strcpy(text, "unsup");
      break;
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

/* operations on points. this is a separate function
   because it is called once for each operation slot.
   It's static to enforce thread-safety.
*/
int PLUGIN::pointops(float pop, int npts, float * op_param, int samples,
                     int * px, float * py, int maxpts,
                     int * tempx, float * tempy) {
  /* pointops. */

  int times = 2;
  switch(MKPOINTOP(pop)) {
  case OP_DOUBLE: {
    /* x2 points */
    int i = 0;
    int t;
    for(t = 0; i < (npts - 1) && t < (maxpts-4); i++) {
      /* always include the actual point */
      tempx[t] = px[i];
      tempy[t] = py[i];
      t++;
      /* now, only if there's room... */
      if ((i < npts) && ((px[i+1] - px[i]) > 1)) {
        /* add an extra point, right between the old points.
           (the idea is to double the frequency).
	   Pick its y coordinate according to the parameter.
        */

        tempy[t] = (op_param[OP_DOUBLE] * 2.0f - 1.0f) * py[i];
        tempx[t] = (px[i] + px[i+1]) >> 1;

        t++;
      }
    }
    /* include last if not different from previous */
    if (t > 0 && npts > 0 && tempx[t-1] != px[npts-1]) {
      tempx[t] = px[npts-1];
      tempy[t] = py[npts-1];
      t++;
    }

    for(int c = 0; c < t; c++) {
      px[c] = tempx[c];
      py[c] = tempy[c];
    }
    npts = t;
    break;
  }
  case OP_HALF:
  case OP_QUARTER: {
    times = 1;
    if (MKPOINTOP(pop) == OP_QUARTER) times = 2;
    for(int t = 0; t < times; t++) {
      int i;
      /* cut points in half. never touch first or last. */
      int q = 1;
      for(i=1; q < (npts - 1); i++) {
        px[i] = px[q];
        py[i] = py[q];
        q += 2;
      }
      px[i] = px[npts - 1];
      py[i] = py[npts - 1];
      npts = i+1;
    }
    break;
  }
  case OP_LONGPASS: {
    /* longpass. drop any point that's not at least param*samples
       past the previous. */ 
    /* XXX this can cut out the last point? */
    tempx[0] = px[0];
    tempy[0] = py[0];

    int stretch = (op_param[OP_LONGPASS] * op_param[OP_LONGPASS]) * samples;
    int np = 1;

    for(int i=1; i < (npts-1); i ++) {
      if (px[i] - tempx[np-1] > stretch) {
        tempx[np] = px[i];
        tempy[np] = py[i];
        np++;
        if (np == maxpts) break;
      }
    }

    for(int c = 1; c < np; c++) {
      px[c] = tempx[c];
      py[c] = tempy[c];
    }
    
    px[np] = px[npts-1];
    py[np] = py[npts-1];
    np++;

    npts = np;

    break;
  }
  case OP_SHORTPASS: {
    /* shortpass. If an interval is longer than the
       specified amount, zero the 2nd endpoint.
    */

    int stretch = (op_param[OP_SHORTPASS] * op_param[OP_SHORTPASS]) * samples;

    for (int i=1; i < npts; i ++) {
      if (px[i] - px[i-1] > stretch) py[i] = 0.0f;
    }

    break;
  }
  case OP_SLOW: {
    /* slow points down. stretches the points out so that
       the tail is lost, but preserves their y values. */
    
    float factor = 1.0f + op_param[OP_SLOW];

    /* We don't need to worry about maxpoints, since
       we will just be moving existing samples (and
       truncating)... */
    int i;
    for(i = 0; i < (npts-1); i ++) {
      px[i] *= factor;
      if (px[i] > samples) {
	/* this sample can't stay. */
	i --;
	break;
      }
    }
    /* but save last point */
    px[i] = px[npts-1];
    py[i] = py[npts-1];
    
    npts = i + 1;

    break;
  }
  case OP_FAST: {

    float factor = 1.0f + (op_param[OP_FAST] * 3.0f);
    float onedivfactor = 1.0f / factor;

    /* number of times we need to loop through samples */
    int times = (int)(factor + 1.0);

    int outi = 0;
    for(int rep = 0; rep < times; rep ++) {
      /* where this copy of the points begins */
      int offset = rep * (onedivfactor * samples);
      for (int s = 0; s < npts; s ++) {
	/* XXX is destx in range? */
	int destx = offset + (px[s] * onedivfactor);

	if (destx >= samples) goto op_fast_out_of_points;

	/* check if we already have one here.
	   if not, add it and advance, otherwise ignore. 
	   XXX: one possibility would be to mix...
	*/
	if (!(outi > 0 && tempx[outi-1] == destx)) {
	  tempx[outi] = destx;
	  tempy[outi] = py[s];
	  outi ++;
	} 

	if (outi > (maxpts - 2)) goto op_fast_out_of_points;
      }
    }

  op_fast_out_of_points:
    
    /* always save last sample, as usual */
    tempx[outi] = px[npts - 1];
    tempy[outi] = py[npts - 1];

    /* copy.. */

    for(int c = 1; c < outi; c++) {
      px[c] = tempx[c];
      py[c] = tempy[c];
    }

    npts = outi;

    break;
  }
  case OP_NONE:
  default:
    /* nothing ... */
    break;

  } /* end of main switch(op) statement */

  return npts;
}

/* this processes an individual window.
   1. generate points
   2. do operations on points (in slots op1, op2, op3)
   3. generate waveform
*/
int PLUGIN::processw(float * in, float * out, long samples,
                     int * px, float * py, int maxpts,
                     int * tempx, float * tempy) {

  /* collect points. */

  px[0] = 0;
  py[0] = in[0];
  int numpts = 1;

  /* MS Visual C++ is super retarded, so we have to define 
     and initialize all of these before the switch statement */
  int i = 0, extx = 0, nth = 0, ctr = 0, n = 0, sd = 0;
  float ext = 0.0f;

  switch(MKPOINTSTYLE(pointstyle)) {

  case POINT_EXTNCROSS:
    /* extremities and crossings 
       FIXME: this is broken. It generates points out of order!
       XXX: really? Am I sure?
    */

    ext = 0.0f;
    extx = 0;

    enum {SZ, SZC, SA, SB};
    int state;

    state = SZ;

    for(i = 0 ; i < samples ; i ++) {
      switch(state) {
      case SZ: {
        /* just output a zero. */
        if (in[i] <= pointparam[0] && in[i] >= -pointparam[0]) state = SZC;
        else if (in[i] < -pointparam[0]) { state = SB; ext = in[i]; extx = i; }
        else { state = SA; ext = in[i]; extx = i; }
        break;
      }
      case SZC: {
        /* continuing zeros */
        if (in[i] <= pointparam[0] && in[i] >= -pointparam[0]) break;
  
        /* push zero for last spot (we know it was a zero and not pushed). */
        if (numpts < (maxpts-1)) {
          px[numpts] = (i>0)?(i - 1):0;
          py[numpts] = 0.0f;
          numpts++;
        } 

        if (in[i] < 0.0f) { 
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

        if (in[i] <= pointparam[0]) {
          /* no longer above 0. push the highest point I reached. */
          if (numpts < (maxpts-1)) {
            px[numpts] = extx;
            py[numpts] = ext;
            numpts++;
          } 
          /* and decide state */
          if (in[i] >= -pointparam[0]) {
            if (numpts < (maxpts-1)) {
              px[numpts] = i;
              py[numpts] = 0.0f;
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

        if (in[i] >= -pointparam[0]) {
          /* no longer below 0. push the lowest point I reached. */
          if (numpts < (maxpts-1)) {
            px[numpts] = extx;
            py[numpts] = ext;
            numpts++;
          } 
          /* and decide state */
          if (in[i] <= pointparam[0]) {
            if (numpts < (maxpts-1)) {
              px[numpts] = i;
              py[numpts] = 0.0f;
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

    break;

  case POINT_LEVEL: {

    enum { ABOVE, BETWEEN, BELOW, };

    int state = BETWEEN;
    float level = (pointparam[POINT_LEVEL] * .9999f) + .00005f;
    numpts = 1;

    px[0] = 0;
    py[0] = in[0];

    for(i = 0; i < samples; i ++) {

      if (in[i] > pointparam[POINT_LEVEL]) {
	if (state != ABOVE) {
	  px[numpts] = i;
	  py[numpts] = in[i];
	  numpts ++;
	  state = ABOVE;
	}
      } else if (in[i] < -pointparam[POINT_LEVEL]) {
	if (state != BELOW) {
	  px[numpts] = i;
	  py[numpts] = in[i];
	  numpts ++;
	  state = BELOW;
	}
      } else {
	if (state != BETWEEN) {
	  px[numpts] = i;
	  py[numpts] = in[i];
	  numpts++;
	  state = BETWEEN;
	}
      }

      if (numpts > samples - 2) break;

    }

    px[numpts] = samples - 1;
    py[numpts] = in[samples - 1];

    numpts ++;

    break;
  }
  case POINT_FREQ:
    /* at frequency */

    /* XXX let the user choose hz, do conversion */
    nth = (pointparam[POINT_LEVEL] * pointparam[POINT_FREQ]) * samples;
    ctr = nth;
  
    for(i = 0; i < samples; i ++) {
      ctr--;
      if (ctr <= 0) {
        if (numpts < (maxpts-1)) {
          px[numpts] = i;
          py[numpts] = in[i];
          numpts++;
        } else break; /* no point in continuing... */
        ctr = nth;
      }
    }

    break;


  case POINT_RANDOM: {
    /* randomly */

    n = (1.0 - pointparam[2]) * samples;

    for(;n --;) {
      if (numpts < (maxpts-1)) {
        px[numpts++] = rand() % samples;
      } else break;
    }

    /* sort them */

    qsort(px, numpts, sizeof (int), intcompare);

    for (sd = 0; sd < numpts; sd++) {
      py[sd] = in[px[sd]];
    }

    break;
  }
  
  case POINT_SPAN: {
    /* next x determined by sample magnitude
       
    suggested by bram.
    */

    int span = (pointparam[3] * pointparam[3]) * samples;

    i = abs((int)(py[0] * span)) + 1;

    while (i < samples) {
      px[numpts] = i;
      py[numpts] = in[i];
      numpts++;
      i = i + abs((int)(in[i] * span)) + 1;
    }

    break;

  }
  
  case POINT_DYDX: {
    /* dy/dx */
    int lastsign = 0;
    float lasts = in[0];
    int sign;

    px[0] = 0;
    py[0] = in[0];
    numpts = 1;

    for(i = 1; i < samples; i++) {
      
      if (pointparam[4] > 0.5f) {
        float pp = pointparam[4] - 0.5f;
        sign = (in[i] - lasts) > (pp * pp);
      } else {
        float pp = 0.5f - pointparam[4];
        sign = (in[i] - lasts) < (pp * pp);
      }

      lasts = in[i];

      if (sign != lastsign) {
        px[numpts] = i;
        py[numpts] = in[i];
        numpts++;
        if (numpts > (maxpts-1)) break;
      }

      lastsign = sign;
    }

    break;

  }
  default:
    /* nothing, unsupported... */
    numpts = 1;
    break;

  } /* end of pointstyle cases */


  /* always push final point for continuity (we saved room) */
  px[numpts] = samples-1;
  py[numpts] = in[samples-1];
  numpts++;

  /* modify the points according to the three slots and
     their parameters */

  numpts = pointops(pointop1, numpts, oppar1, samples, 
                    px, py, maxpts, tempx, tempy);
  numpts = pointops(pointop2, numpts, oppar2, samples, 
                    px, py, maxpts, tempx, tempy);
  numpts = pointops(pointop3, numpts, oppar3, samples, 
                    px, py, maxpts, tempx, tempy);

  int u=1, z=0;
  switch(MKINTERPSTYLE(interpstyle)) {

  case INTERP_SHUFFLE: {
    /* mix around the intervals. The parameter determines
       how mobile an interval is. 

       I build an array of interval indices (integers).
       Then I swap elements with nearby elements (where
       nearness is determined by the parameter). Then
       I reconstruct the wave by reading the intervals
       in their new order.
    */
    
    /* fix last point at last sample -- necessary to
       preserve invariants */
    px[numpts-1] = samples - 1;

    int intervals = numpts - 1;

    /* generate table */
    for(int a = 0; a < intervals; a++) {
      tempx[a] = a;
    }

    for(int z = 0; z < intervals; z++) {
      if (randFloat() < interparam[INTERP_SHUFFLE]) {
	int t;
	int dest = z + ((interparam[INTERP_SHUFFLE] * 
			 interparam[INTERP_SHUFFLE] * (float)intervals)
			* randFloat()) - (interparam[INTERP_SHUFFLE] *
					  interparam[INTERP_SHUFFLE] *
					  0.5f * (float)intervals);
	if (dest < 0) dest = 0;
	if (dest >= intervals) dest = intervals - 1;

	t = tempx[z];
	tempx[z] = tempx[dest];
	tempx[dest] = t;
      }
    }

#if 1
    /* generate output */
    int c = 0;
    for(int u = 0; u < intervals; u++) {
      int size = px[tempx[u]+1] - px[tempx[u]];
      memcpy(out + c,
	     in + px[tempx[u]],
	     size * sizeof (float));
      c += size;
    }
#else
    memset(out, 0, samples * sizeof (float));
#endif
    break;
  }

  case INTERP_FRIENDS: {
    /* bleed each segment into next segment (its "friend"). 
       interparam controls the amount of bleeding, between 
       0 samples and next-segment-size samples. 
       suggestion by jcreed (sorta).
    */

    /* copy last block verbatim. */
    if (numpts > 2)
      for(int s=px[numpts-2]; s < px[numpts-1]; s++)
	out[s] = in[s];

    /* steady state */
    int x = numpts - 2;
    for(; x > 0; x--) {
      /* x points at the beginning of the segment we'll be bleeding
	 into. */
      int sizeright = px[x+1] - px[x];
      int sizeleft = px[x] - px[x-1];
      int sizetotal = sizeleft + sizeright;

      int tgtlen = sizeleft + (sizeright * interparam[INTERP_FRIENDS]);

      if (tgtlen > 0) {
	/* to avoid using temporary storage, copy from end of target
	   towards beginning, overwriting already used source parts on
	   the way. 
	   
	   j is an offset from p[x-1], ranging from 0 to tgtlen-1.
	   Once we reach p[x], we have to start mixing with the
	   data that's already there.
	*/
	for (int j = tgtlen - 1;
	     j >= 0;
	     j--) {

	  /* XXX. use interpolated sampling for this */
	  float wet = in[(int)(px[x-1] + sizeleft * 
			       (j/(float)tgtlen))];

	  if ((j + px[x-1]) > px[x]) {
	    /* after p[x] -- mix */

	    /* linear fade-out */
	    float pct = (j - sizeleft) / (float)(tgtlen - sizeleft);

	    out[j + px[x-1]] =
	      wet * (1.0f - pct) +
	      out[j + px[x-1]] * pct;
	  } else {
	    /* before p[x] -- no mix */
	    out[j + px[x-1]] = wet;
	  }

	}
      }
    }

    break;
  }
  case INTERP_POLYGON:
    /* linear interpolation - "polygon" 
       interparam causes dimming effect -- at 1.0 it just does
       straight lines at the median.
    */

    for(u=1; u < numpts; u ++) {
      float denom = (px[u] - px[u-1]);
      float minterparam = interparam[INTERP_POLYGON] * (py[u-1] + py[u]) * 
	0.5f;
      for(z=px[u-1]; z < px[u]; z++) {
        float pct = (float)(z-px[u-1]) / denom;
        float s = py[u-1] * (1.0f - pct) +
          py[u]   * pct;
        out[z] = minterparam + (1.0f - interparam[INTERP_POLYGON]) * s;
      }
    }

    out[samples-1] = in[samples-1];

    break;


  case INTERP_WRONGYGON:
    /* linear interpolation, wrong direction - "wrongygon" 
       same dimming effect from polygon.
    */

    for(u=1; u < numpts; u ++) {
      float denom = (px[u] - px[u-1]);
      float minterparam = interparam[INTERP_WRONGYGON] * (py[u-1] + py[u]) 
	* 0.5f;
      for(z=px[u-1]; z < px[u]; z++) {
        float pct = (float)(z-px[u-1]) / denom;
        float s = py[u-1] * pct +
          py[u]   * (1.0f - pct);
        out[z] = minterparam + (1.0f - interparam[INTERP_WRONGYGON]) * s;
      }
    }

    out[samples-1] = in[samples-1];

    break;


  case INTERP_SMOOTHIE:
    /* cosine up or down - "smoothie" */

    for(u=1; u < numpts; u ++) {
      float denom = (px[u] - px[u-1]);
      for(z=px[u-1]; z < px[u]; z++) {
        float pct = (float)(z-px[u-1]) / denom;
        
        float p = 0.5f * (-cos(float(pi * pct)) + 1.0f);
        
        if (interparam[INTERP_SMOOTHIE] > 0.5f) {
          p = powf(p, (interparam[INTERP_SMOOTHIE] - 0.16666667f) * 3.0f);
        } else {
          p = powf(p, interparam[INTERP_SMOOTHIE] * 2.0f);
        }

        float s = py[u-1] * (1.0f - p) + py[u]   * p;

        out[z] = s;
      }
    }

    out[samples-1] = in[samples-1];

    break;


  case INTERP_REVERSI:
    /* x-reverse input samples for each waveform - "reversi" */

    for(u=1; u < numpts; u ++) {
      if (px[u-1] < px[u])
        for(z = px[u-1]; z < px[u]; z++) {
          int s = (px[u] - (z + 1)) + px[u - 1];
          out[z] = in[(s>0)?s:0];
        }
    }

    break;


  case INTERP_PULSE: {

    int wid = (int)(100.0 * interparam[INTERP_PULSE]);
    
    for(i = 0; i < samples; i++) out[i] = 0.0f;

    for(z = 0; z < numpts; z ++) { 
      out[px[z]] = magmax(out[px[z]], py[z]);

      if (wid > 0) {
	/* put w samples on the left, stopping if we hit a sample
	   greater than what we're placing */
	int w = wid;
	float onedivwid = 1.0f / (float)(wid + 1);
	for(int i=px[z]-1; i >= 0 && w > 0; i--, w--) {
	  float sam = py[z] * (w * onedivwid);
	  if ((out[i] + sam) * (out[i] + sam) > (sam * sam)) out[i] = sam;
	  else out[i] += sam;
	}

	w = wid;
	for(int ii=px[z]+1; ii < samples && w > 0; ii++, w--) {
	  float sam = py[z] * (w * onedivwid);
	  out[ii] = sam;
	}

      }
    }

    break;

  }
  case INTERP_SING:

    for(u=1; u < numpts; u ++) {
      float oodenom = 1.0f / (px[u] - px[u-1]);

      for(z=px[u-1]; z < px[u]; z++) {
        float pct = (float)(z-px[u-1]) * oodenom;
        
	float wand = sin(float(2.0f * pi * pct));
	out[z] = wand * 
	  interparam[INTERP_SING] + 
	  ((1.0f-interparam[INTERP_SING]) * 
	   in[z] *
	   wand);
      }
    }

    out[samples-1] = in[samples-1];


    break;
  default:

    /* unsupported ... ! */
    for(int i = 0; i < samples; i++) out[i] = 0.0f;

    break;

  } /* end of interpstyle cases */

  return numpts;

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
  int z = 0;

  for (int ii = 0; ii < samples; ii++) {

    /* copy sample in */
    in0[insize] = tin[ii];
    insize ++;
 
    if (insize == framesize) {
      /* frame is full! */

      /* in0 -> process -> out0(first free space) */
      cs->grab();
      processw(in0, out0+outstart+outsize, framesize,
	       pointx, pointy, framesize * 2,
	       storex, storey);
      cs->release();

      float oneDivThird = 1.0f / (float)third;
      /* apply envelope */

      switch(MKWINDOWSHAPE(shape)) {

        case WINDOW_TRIANGLE:
          for(z = 0; z < third; z++) {
            out0[z+outstart+outsize] *= ((float)z * oneDivThird);
            out0[z+outstart+outsize+third] *= (1.0f - ((float)z * oneDivThird));
          }
          break;
        case WINDOW_ARROW:
          for(z = 0; z < third; z++) {
            float p = (float)z * oneDivThird;
            p *= p;
            out0[z+outstart+outsize] *= p;
            out0[z+outstart+outsize+third] *= (1.0f - p);
          }
          break;
        case WINDOW_WEDGE:
          for(z = 0; z < third; z++) {
            float p = sqrtf((float)z * oneDivThird);
            out0[z+outstart+outsize] *= p;
            out0[z+outstart+outsize+third] *= (1.0f - p);
          }
          break;
        case WINDOW_COS:
          for(z = 0; z < third; z ++) {
            float p = 0.5f * (-cos(float(pi * ((float)z * oneDivThird))) + 1.0f);
            out0[z+outstart+outsize] *= p;
            out0[z+outstart+outsize+third] *= (1.0f - p);
          }
          break;
#if 0
        case WINDOW_COS2:
        default:
          for(z = 0; z < third; z ++) {
            float p = 0.5f * (-cos(float(pi * ((float)z * oneDivThird))) + 1.0f);
            p = p * p;
            out0[z+outstart+outsize] *= p;
            out0[z+outstart+outsize+third] *= (1.0f - p);
          }
          break;
#endif
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

long PLUGIN::canDo(char* text) {
  if (strcmp(text, "receiveVstEvents") == 0)
    return 1;
  if (strcmp(text, "receiveVstMidiEvent") == 0)
    return 1;
  if (strcmp(text, "geometry") == 0)
    return 1;
  if (strcmp(text, "plugAsChannelInsert") == 0)
    return 1;
  if (strcmp(text, "plugAsSend") == 0)
    return 1;
  if (strcmp(text, "mixDryWet") == 0)
    return 1;
  if (strcmp(text, "1in1out") == 0)
    return 1;

  return -1;	// explicitly can't do; 0 => don't know
}

void PLUGIN::makepresets() {
  int i = 1;

  strcpy(programs[i].name, "atonal singing");
  programs[i].param[P_BUFSIZE] = 0.7f;
  programs[i].param[P_POINTSTYLE] = UNMKPOINTSTYLE(POINT_FREQ);
  programs[i].param[P_POINTPARAMS + POINT_FREQ] = 0.10112f;
  programs[i].param[P_INTERPSTYLE] = UNMKINTERPSTYLE(INTERP_REVERSI);

  i++;

  strcpy(programs[i].name, "robo sing");
  programs[i].param[P_BUFSIZE] = paramSteppedUnscaled((9), BUFFERSIZESSIZE);	// 2^11
  programs[i].param[P_SHAPE] = UNMKWINDOWSHAPE(WINDOW_COS);
  programs[i].param[P_POINTSTYLE] = UNMKPOINTSTYLE(POINT_DYDX);
  programs[i].param[P_POINTPARAMS + POINT_DYDX] = 0.234f;
  programs[i].param[P_INTERPSTYLE] = UNMKINTERPSTYLE(INTERP_SING);
  programs[i].param[P_INTERPARAMS + INTERP_SING] = 1.0f;
  programs[i].param[P_POINTOP1] = UNMKPOINTOP(OP_FAST);
  programs[i].param[P_OPPAR1S + OP_FAST] = 0.9157304f;
  programs[i].param[P_POINTOP2] = UNMKPOINTOP(OP_SHORTPASS);
  programs[i].param[P_OPPAR2S + OP_SHORTPASS] = 0.8595505f;
  i++;

  strcpy(programs[i].name, "sploop drums");
  programs[i].param[P_BUFSIZE] = paramSteppedUnscaled((9), BUFFERSIZESSIZE);	// 2^11
  programs[i].param[P_SHAPE] = UNMKWINDOWSHAPE(WINDOW_TRIANGLE);
  programs[i].param[P_POINTSTYLE] = UNMKPOINTSTYLE(POINT_DYDX);
  programs[i].param[P_POINTPARAMS + POINT_DYDX] = 0.528f;
  programs[i].param[P_INTERPSTYLE] = UNMKINTERPSTYLE(INTERP_SING);
  programs[i].param[P_INTERPARAMS + INTERP_SING] = 0.2921348f;
  programs[i].param[P_POINTOP1] = UNMKPOINTOP(OP_QUARTER);
  programs[i].param[P_OPPAR1S + OP_QUARTER] = 0.258427f;
  programs[i].param[P_POINTOP2] = UNMKPOINTOP(OP_DOUBLE);
  programs[i].param[P_OPPAR2S + OP_DOUBLE] = 0.5f;
  i++;

}


/* XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX */ 
/* ---------- boring stuff below this line ----------- */
/* XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX */ 

#if BEOS
#define main main_plugin
extern "C" __declspec(dllexport) AEffect *main_plugin (audioMasterCallback audioMaster);

#else
AEffect *main (audioMasterCallback audioMaster);
#endif

AEffect *main (audioMasterCallback audioMaster) {
  // get vst version
  if ( !audioMaster(0, audioMasterVersion, 0, 0, 0, 0) )
    return 0;  // old version

  AudioEffect* effect = new PLUGIN(audioMaster);
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
