
/* DFX Exemplar! */

#include "exemplar.h"

#include <stdio.h>
#include <fstream>
// #include <fstream.h>

#define DIMENSION 12

#if defined(TARGET_API_VST) && TARGET_PLUGIN_HAS_GUI
  #ifndef __DFX_EXEMPLAREDITOR_H
  #include "exemplareditor.hpp"
  #endif
#endif

/* this macro does boring entry point stuff for us */
DFX_ENTRY(Exemplar);
DFX_CORE_ENTRY(ExemplarDSP);


PLUGIN::PLUGIN(TARGET_API_BASE_INSTANCE_TYPE inInstance)
  : DfxPlugin(inInstance, NUM_PARAMS, NUM_PRESETS) {

  initparameter_indexed(P_BUFSIZE, "wsize", 9, 9, BUFFERSIZESSIZE, kDfxParamUnit_samples);
  initparameter_indexed(P_SHAPE, "wshape", WINDOW_TRIANGLE, WINDOW_TRIANGLE, MAX_WINDOWSHAPES);

  initparameter_indexed(P_MODE, "mode", MODE_CAPTURE, MODE_CAPTURE, NUM_MODES);

  initparameter_f(P_ERRORAMOUNT, "erroramt", 0.10, 0.10, 0.0, 1.0, kDfxParamUnit_custom, kDfxParamCurve_linear, "error");
  // initparameter_f(P_ERRORAMOUNT, , 0.0, 0.0, -1.0, 1.0, kDfxParamUnit_pan);


  /* modes */
  setparametervaluestring(P_MODE, MODE_MATCH, "match");
  setparametervaluestring(P_MODE, MODE_CAPTURE, "capture");

  long i;
  /* set up values for windowing */
  char bufstr[64];
  for (i=0; i < BUFFERSIZESSIZE; i++) {
    if (buffersizes[i] > 1000)
      sprintf(bufstr, "%ld,%03ld", buffersizes[i]/1000, buffersizes[i]%1000);
    else
      sprintf(bufstr, "%ld", buffersizes[i]);
    setparametervaluestring(P_BUFSIZE, i, bufstr);
  }

  setparametervaluestring(P_SHAPE, WINDOW_TRIANGLE, "linear");
  setparametervaluestring(P_SHAPE, WINDOW_ARROW, "arrow");
  setparametervaluestring(P_SHAPE, WINDOW_WEDGE, "wedge");
  setparametervaluestring(P_SHAPE, WINDOW_COS, "best");
  for (i=NUM_WINDOWSHAPES; i < MAX_WINDOWSHAPES; i++)
    setparametervaluestring(P_SHAPE, i, "???");

  long delay_samples = buffersizes[getparameter_i(P_BUFSIZE)];
  setlatency_samples(delay_samples);
  settailsize_samples(delay_samples);

  setpresetname(0, "Exemplar Default"); /* default preset name */
  makepresets();

  /* allow MIDI keys to be used to control parameters */
  dfxsettings->setAllowPitchbendEvents(true);
  dfxsettings->setAllowNoteEvents(true);

#if !TARGET_PLUGIN_USES_DSPCORE
  addchannelconfig(1, 1);	/* mono */
#endif

  #ifdef TARGET_API_VST
    #if TARGET_PLUGIN_USES_DSPCORE
      DFX_INIT_CORE(ExemplarDSP);	/* we need to manage DSP cores manually in VST */
    #endif
    /* if you have a GUI, need an Editor class... */
    #if TARGET_PLUGIN_HAS_GUI
      editor = new ExemplarEditor(this);
    #endif
  #endif
}

PLUGIN::~PLUGIN() {

#ifdef TARGET_API_VST
  /* VST doesn't have initialize and cleanup methods like Audio Unit does, 
    so we need to call this manually here */
  do_cleanup();
#endif
}

PLUGINCORE::PLUGINCORE(DfxPlugin * inInstance)
  : DfxPluginCore(inInstance) {
  /* determine the size of the largest window size */
  long maxframe = 0;
  for (int i = 0; i < BUFFERSIZESSIZE; i++)
    maxframe = (buffersizes[i] > maxframe) ? buffersizes[i] : maxframe;

  /* add some leeway? */
  in0 = (float*)malloc(maxframe * sizeof (float));
  out0 = (float*)malloc(maxframe * 2 * sizeof (float));

  /* prevmix is only a single third long */
  prevmix = (float*)malloc((maxframe / 2) * sizeof (float));

  /* initialize nn stuff */
  capturemode = true;
  nntree = 0;
  ncapsamples = 0;
  npoints = 0;
}


PLUGINCORE::~PLUGINCORE() {
  /* windowing buffers */
  free (in0);
  free (out0);

  free (prevmix);
}

void PLUGINCORE::reset() {

  framesize = buffersizes[getparameter_i(P_BUFSIZE)];
  third = framesize / 2;
  bufsize = third * 3;


  shape = getparameter_i(P_SHAPE);

  bool newcapture = MODE_CAPTURE == getparameter_i(P_MODE);
  if (newcapture != capturemode) {
    /* switching modes. this can be expensive, since we have
       to build the nearest neighbor tree. */
    capturemode = newcapture;
    if (capturemode) {
      /* entering capture mode. discard the existing tree */
      nntree = 0; /* XXX do it... */

      ncapsamples = 0;
      npoints = 0;
    } else {
      /* entering match mode. build new tree. */

      int wsize = getwindowsize();
      /* make points */
      for (npoints = 0; npoints < (ncapsamples - wsize); npoints ++) {
	/* for now do every subwindow */
	cap_index[npoints] = npoints;
	classify(&(capsamples[npoints]), cap_point[npoints], wsize);
      }

      nntree = new ANNkd_tree(cap_point, npoints, DIMENSION);
      char msg[512];
      // sprintf(msg, "ok %p", this);
      // MessageBoxA(0, "match mode", msg, MB_OK);
      #if 0
      std::ofstream f;
      f.open("c:\\code\\vstplugins\\exemplar\\dump.ann");
      if (f) {
	nntree->Dump(ANNtrue, f);
	f.close();
      }
      #endif

      /* ok, ready! */
    }
  } /* mode changed */

  /* set up buffers. Prevmix and first frame of output are always 
     filled with zeros. XXX memset */

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

  dfxplugin->setlatency_samples(framesize);
  /* tail is the same as delay, of course */
  dfxplugin->settailsize_samples(framesize);
}

void PLUGINCORE::processparameters() {
  
  /* can safely change this whenever... */
  float erroramount = getparameter_f(P_ERRORAMOUNT);

  #ifdef TARGET_API_VST
    /* this tells the host to call a suspend()-resume() pair, 
      which updates initialDelay value */
  if (getparameterchanged(P_BUFSIZE) ||
      getparameterchanged(P_MODE))
    dfxplugin->setlatencychanged(true);
  #endif
}

/* this processes an individual window. Basically, this is where you
   write your DSP, and it will be always called with the same sample
   size (as long as the block size parameter stays the same) and
   automatically overlapped. */
void PLUGINCORE::processw(float * in, float * out, long samples) {

#if 0
  /* this sounds pretty neat, actually. */
  for(long i = 0; i < samples; i ++) {
    out[i] = in[i] * in[(i + (samples >> 1)) % samples];
  }
#endif

  /* memmove(out, in, samples * sizeof (float)); */

  /* XXX since this capture is done in windowing mode,
     our capture buffer would have overlapping regions
     and also discontinuities, which is dumb. we should
     only capture on even calls to processw.
     (this should be a bit more principled...) */
  static int parity = 0;
  
  parity = !parity;
  if (capturemode) {
    /* capture mode.. just record into capsamples */
    if (parity == 0) {
      for(int i = 0; i < samples && ncapsamples < CAPBUFFER; i ++) {
	capsamples[ncapsamples++] = in[i];
      }
    }
  } else {

    ANNpoint p;
    ANNidx res;
    ANNdist dist;
    classify(in, p, samples);

    if (1 && nntree) {
      /* match mode */
      nntree->annkSearch(p, 1, &res, &dist /* distance array -- not needed */, erroramount);
      
      annDeallocPt(p);

      /* XXX boost RMS? */
      /* now res holds the closest point index */
      if (res != ANN_NULL_IDX) {
	/* so copy that captured window into output */
	/* really should use the window size that this
	   originally represented, perhaps stretching it... */
	for(int i = 0; i < samples && (cap_index[res] + i < CAPBUFFER) ; i ++) {
	  out[i] = capsamples[cap_index[res] + i];
	}
      } /* otherwise ??? */
    } else {
      /* error noise */
      for(int i = 0; i < samples; i ++) {
	out[i] = sin((float)i / 100.0);
      }

    }
  }
}

/* classify a series of samples according to the point.
   
   XXX--right now, it uses a stationary Haar wavelet.
   So we take the dot product of 'in' with wavelets w0,...wd
   of the following form:
   
           in/2
      w0   ~~~~____
               in/2

          in/4
      w1   ~~__~~__


          in/8
      w2   ~_~_~_~_


    ... etc.
 */

/* assumes samples is a power of two. */
void PLUGINCORE::classify(float * in, ANNpoint & out, long samples) {
  out = annAllocPt(DIMENSION);

  /* dth wavelet switches from 1 to -1 each s samples. */
  int freq = samples;

  for(int d = 0; d < DIMENSION; d++) {
    freq >>= 1;
    if (freq) {
      int i = 0;
      float prod = 0.0;
      while (i < samples) {
	/* up */
	{ 
	  for(int j = 0; j < freq; j ++) {
	    prod += in[i + j];
	  }
	}
	i += freq;
	/* down */
	{
	  for(int j = 0; j < freq; j ++) {
	    prod -= in[i + j];
	  }
	}
	i += freq;
      }
      out[d] = prod;
    } else {
      /* oops, we went to zero sample-length peaks... 
	 our dimension is too high for this window size
      */
      out[d] = 0.0;
    }
  }

}


/* this windowing process function reads samples one at a time
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

/* PERF: 
   - use memcpy and arithmetic instead of
     sample-by-sample copy 
   - can we use tail of out0 as prevmix, instead of copying?
   - can we use circular buffers instead of memmoving a lot?
     (probably not)
*/


void PLUGINCORE::process(const float *tin, float *tout, unsigned long samples, bool replacing) {
  int z = 0;

  for (unsigned long ii = 0; ii < samples; ii++) {

    /* copy sample in */
    in0[insize] = tin[ii];
    insize ++;
 
    if (insize == framesize) {
      /* frame is full! */

      /* in0 -> process -> out0(first free space) */
      processw(in0, out0+outstart+outsize, framesize);

      float oneDivThird = 1.0f / (float)third;
      /* apply envelope */

      switch(shape) {

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
            float p = 0.5f * (-cosf(PI * ((float)z * oneDivThird)) + 1.0f);
            out0[z+outstart+outsize] *= p;
            out0[z+outstart+outsize+third] *= (1.0f - p);
          }
          break;
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
  #ifdef TARGET_API_VST
    if (replacing)
  #endif
      tout[ii] = out0[outstart];
  #ifdef TARGET_API_VST
    else tout[ii] += out0[outstart];
  #endif

    outstart ++;
    outsize --;

    /* make sure there is always enough room for a frame in out buffer */
    if (outstart == third) {
      memmove(out0, out0 + outstart, outsize * sizeof (float));
      outstart = 0;
    }
  }
}


void PLUGIN::makepresets() {
  /* initialize presets here, see geometer for an example. */
}
