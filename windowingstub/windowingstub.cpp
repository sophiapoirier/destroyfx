
/* Windowingstub,
   Featuring the Super Destroy FX Windowing System! */

#include "windowingstub.h"

#include <stdio.h>

#if defined(TARGET_API_VST) && TARGET_PLUGIN_HAS_GUI
  #ifndef __DFX_WINDOWINGSTUBEDITOR_H
  #include "windowingstubeditor.hpp"
  #endif
#endif

/* this macro does boring entry point stuff for us */
DFX_ENTRY(Windowingstub);
DFX_CORE_ENTRY(WindowingstubDSP);


PLUGIN::PLUGIN(TARGET_API_BASE_INSTANCE_TYPE inInstance)
  : DfxPlugin(inInstance, NUM_PARAMS, NUM_PRESETS) {

  initparameter_indexed(P_BUFSIZE, "wsize", 9, 9, BUFFERSIZESSIZE, kDfxParamUnit_samples);
  initparameter_indexed(P_SHAPE, "wshape", WINDOW_TRIANGLE, WINDOW_TRIANGLE, MAX_WINDOWSHAPES);

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

  setpresetname(0, "Windowingstub Default"); /* default preset name */
  makepresets();

  /* allow MIDI keys to be used to control parameters */
  dfxsettings->setAllowPitchbendEvents(true);
  dfxsettings->setAllowNoteEvents(true);

#if !TARGET_PLUGIN_USES_DSPCORE
  addchannelconfig(1, 1);	/* mono */
#endif

  #ifdef TARGET_API_VST
    #if TARGET_PLUGIN_USES_DSPCORE
      DFX_INIT_CORE(WindowingstubDSP);	/* we need to manage DSP cores manually in VST */
    #endif
    /* if you have a GUI, need an Editor class... */
    #if TARGET_PLUGIN_HAS_GUI
      editor = new WindowingstubEditor(this);
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

PLUGINCORE::PLUGINCORE(TARGET_API_CORE_INSTANCE_TYPE *inInstance)
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

  shape = getparameter_i(P_SHAPE);

  #ifdef TARGET_API_VST
    /* this tells the host to call a suspend()-resume() pair, 
      which updates initialDelay value */
  if (getparameterchanged(P_BUFSIZE))
    dfxplugin->setlatencychanged(true);
  #endif
}

/* this processes an individual window. Basically, this is where you
   write your DSP, and it will be always called with the same sample
   size (as long as the block size parameter stays the same) and
   automatically overlapped. */
void PLUGINCORE::processw(float * in, float * out, long samples) {

  memmove(out, in, samples * sizeof (float));

}


/* this fake process function reads samples one at a time
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
