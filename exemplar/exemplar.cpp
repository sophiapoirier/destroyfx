/*------------------------------------------------------------------------
Copyright (C) 2006-2023  Tom Murphy 7

This file is part of Exemplar.

Exemplar is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Exemplar is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Exemplar.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org

DFX Exemplar, featuring the Super Destroy FX Windowing System!
------------------------------------------------------------------------*/

#include "exemplar.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>

#include "dfxmath.h"


#define NORMALIZE 1

static constexpr int DIMENSION = 10;
// should be param */
static constexpr int STRIDE = 128;

/* this macro does boring entry point stuff for us */
DFX_ENTRY(Exemplar);
DFX_CORE_ENTRY(ExemplarDSP);


PLUGIN::PLUGIN(TARGET_API_BASE_INSTANCE_TYPE inInstance)
  : DfxPlugin(inInstance, NUM_PARAMS, NUM_PRESETS) {

  initparameter_indexed(P_BUFSIZE, {"wsize"}, 9, 9, std::ssize(buffersizes), kDfxParamUnit_samples);
  initparameter_indexed(P_SHAPE, {"wshape"}, WINDOW_TRIANGLE, WINDOW_TRIANGLE, MAX_WINDOWSHAPES);

  initparameter_indexed(P_MODE, {"mode"}, MODE_CAPTURE, MODE_CAPTURE, NUM_MODES);

  initparameter_f(P_ERRORAMOUNT, {"erroramt"}, 0.10, 0.10, 0.0, 1.0, kDfxParamUnit_custom, kDfxParamCurve_linear, "error");

  initparameter_indexed(P_FFTRANGE, {"fft range"}, FFTR_AUDIBLE, FFTR_ALL, NUM_FFTRS);


  /* fft ranges */
  setparametervaluestring(P_FFTRANGE, FFTR_AUDIBLE, "audible");
  setparametervaluestring(P_FFTRANGE, FFTR_ALL, "all");

  /* modes */
  setparametervaluestring(P_MODE, MODE_MATCH, "match");
  setparametervaluestring(P_MODE, MODE_CAPTURE, "capture");

  /* set up values for windowing */
  std::array<char, 64> bufstr {};
  for (size_t i=0; i < buffersizes.size(); i++) {
    if (buffersizes[i] > 1000)
      std::snprintf(bufstr.data(), bufstr.size(), "%ld,%03ld", buffersizes[i]/1000, buffersizes[i]%1000);
    else
      std::snprintf(bufstr.data(), bufstr.size(), "%ld", buffersizes[i]);
    setparametervaluestring(P_BUFSIZE, static_cast<long>(i), bufstr.data());
  }

  setparametervaluestring(P_SHAPE, WINDOW_TRIANGLE, "linear");
  setparametervaluestring(P_SHAPE, WINDOW_ARROW, "arrow");
  setparametervaluestring(P_SHAPE, WINDOW_WEDGE, "wedge");
  setparametervaluestring(P_SHAPE, WINDOW_COS, "best");
  for (int i=NUM_WINDOWSHAPES; i < MAX_WINDOWSHAPES; i++)
    setparametervaluestring(P_SHAPE, i, "???");

  auto const delay_samples = dfx::math::ToUnsigned(buffersizes.at(getparameter_i(P_BUFSIZE)));
  setlatency_samples(delay_samples);
  settailsize_samples(delay_samples);

  setpresetname(0, "Exemplar Default"); /* default preset name */
  makepresets();

  /* allow MIDI keys to be used to control parameters */
  dfxsettings->setAllowPitchbendEvents(true);
  dfxsettings->setAllowNoteEvents(true);

#if !TARGET_PLUGIN_USES_DSPCORE
  addchannelconfig(1, 1);       /* mono */
#endif
}

PLUGINCORE::PLUGINCORE(DfxPlugin& inInstance)
  : DfxPluginCore(inInstance) {
  /* determine the size of the largest window size */
  constexpr auto maxframe = *std::ranges::max_element(buffersizes);

  /* add some leeway? */
  in0.assign(maxframe, 0.f);
  out0.assign(maxframe * 2, 0.f);

  /* prevmix is only a single third long */
  prevmix.assign(maxframe / 2, 0.f);

  /* initialize FFT stuff */
  plan.reset(rfftw_create_plan(framesize, FFTW_FORWARD, FFTW_ESTIMATE));
  // rplan.reset(rfftw_create_plan(framesize, FFTW_BACKWARD, FFTW_ESTIMATE));

}


void PLUGINCORE::reset() {

  framesize = buffersizes.at(getparameter_i(P_BUFSIZE));
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
      nntree.reset(); /* XXX do it... */

      ncapsamples = 0;
      npoints = 0;
    } else {
      /* entering match mode. build new tree. */

      int wsize = getwindowsize();
      int exstart = 0;
      /* make points */
      for (exstart = 0; exstart < (ncapsamples - wsize); exstart += STRIDE) {
        /* save which start point this is */
        cap_index[npoints] = exstart;
        classify(&(capsamples[exstart]), 
                 cap_scale[npoints],
                 cap_point[npoints], wsize);
        npoints++;
      }

      nntree = std::make_unique<ANNkd_tree>(cap_point, npoints, DIMENSION);
      std::array<char, 512> msg {};
      // std::snprintf(msg.data(), msg.size(), "ok %p", this);
      // MessageBoxA(0, "match mode", msg.data(), MB_OK);
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

  std::fill_n(prevmix.begin(), third, 0.f);
  std::fill_n(out0.begin(), framesize, 0.f)

  /* start input at beginning. Output has a frame of silence. */
  insize = 0;
  outstart = 0;
  outsize = framesize;

  /* restore FFT plans */
  plan.reset(rfftw_create_plan(framesize, FFTW_FORWARD, FFTW_ESTIMATE));
  // rplan.reset(rfftw_create_plan(framesize, FFTW_BACKWARD, FFTW_ESTIMATE));

  dfxplugin->setlatency_samples(dfx::math::ToUnsigned(framesize));
  /* tail is the same as delay, of course */
  dfxplugin->settailsize_samples(dfx::math::ToUnsigned(framesize));
}

void PLUGINCORE::processparameters() {
  
  /* can safely change this whenever... */
  erroramount = getparameter_f(P_ERRORAMOUNT);

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
void PLUGINCORE::processw(float const * in, float * out, long samples) {

#if 0
  /* this sounds pretty neat, actually. */
  for(long i = 0; i < samples; i ++) {
    out[i] = in[i] * in[(i + (samples >> 1)) % samples];
  }
#endif

  /* memmove(out, in, samples * sizeof (float)); */

  /* XXX since this capture is done in windowing mode,
     our capture buffer would have overlapping regions
     and also discontinuities, which is undesirable.
     we should only capture on even calls to processw.
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
    float scale;
    classify(in, scale, p, samples);

    if (1 && nntree) {
      /* match mode */
      nntree->annkSearch(p, 1, &res, &dist /* distance array -- not needed */, erroramount);
      
      annDeallocPt(p);

      /* now res holds the closest point index */
      if (res != ANN_NULL_IDX) {
        /* so copy that captured window into output */
        /* really should use the window size that this
           originally represented, perhaps stretching it... */

        /* scale is the boost we would need to apply to
           normalize the existing sample.

           cap_scale[res] is the boost we would apply
           to normalize the match sample.

           so we boost the match sample
           ( capsamples[i] * cap_scale[res] ) and then
           dim the result to the volume of the existing
           sample ( .. / scale ).
        */

        float matchvol = cap_scale[res] / scale;

        for(int i = 0; i < samples && (cap_index[res] + i < CAPBUFFER) ; i ++) {
          out[i] = capsamples[cap_index[res] + i] * matchvol;
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
void PLUGINCORE::classify_haar(float const * in, float & scale, 
                               ANNpoint & out, long samples) {
  out = annAllocPt(DIMENSION);

  /* first we normalize, since we are not really trying to
     match by scale but by the characteristic of the wave. */

  /* XXX the scale we return should probably be based on
     RMS, not the max sample seen. */
  scale = 1.0;
# if NORMALIZE
  {
    float max = 0.0;
    /* PERF maybe a slicker way exists for this? */
    for(int i = 0; i < samples; i ++) {
      float s = in[i];
      /* cheaper than fabs? */
      s *= s;
      if (s > max) max = s;
    }
    if (max > 0.0) {
      scale = 1.0 / sqrt(max);
    } else scale = 1.0;
  }
# endif

  /* dth wavelet switches from 1 to -1 each s samples. */
  int freq = samples;

  /* often, the wave size is too low for the kinds of
     dimensions we want to analyze. (a stationary haar
     wavelet degenerates too quickly, because the peaks
     shrink logarithmically). So, shrink at a smaller
     factor. */

  for(int d = 0; d < DIMENSION; d++) {
    freq = freq * 4 / 5;  // shrink
    if (freq) {
      int i = 0;
      float prod = 0.0;
      /* XXX now that freq isn't a power of two,
         we might drop the last peak or two
         entirely... */
      while (i < samples) {
        /* up */
        if (i + freq < samples) { 
          for(int j = 0; j < freq; j ++) {
            prod += in[i + j] * scale;
          }
        }
        i += freq;
        /* down */
        if (i + freq < samples) {
          for(int j = 0; j < freq; j ++) {
            prod -= in[i + j] * scale;
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

void PLUGINCORE::classify_fft(float const * in, float & scale, 
                              ANNpoint & out, long samples) {
  out = annAllocPt(DIMENSION);

  /* first we normalize, since we are not really trying to
     match by scale but by the characteristic of the wave. */

  /* XXX the scale we return should probably be based on
     RMS, not the max sample seen. */
  scale = 1.0;
# if NORMALIZE
  {
    float max = 0.0;
    /* PERF maybe a slicker way exists for this? */
    for(int i = 0; i < samples; i ++) {
      float s = in[i];
      /* cheaper than fabs? */
      s *= s;
      if (s > max) max = s;
    }
    if (max > 0.0) {
      scale = 1.0 / sqrt(max);
    } else scale = 1.0;
  }
# endif

  /* do the fft */
  rfftw_one(plan.get(), in, fftr);

  /* what we've got now is frequency/amplitude pairs.
     we want to represent the characteristics of this
     sound so that we can later find sounds 'near' to it.

     ** maybe should implement several of these, with
        a parameter **

     idea 1: count the n loudest frequencies by rank.

     this is bad because something that matches closely
     the last two faint frequencies might end up closer than
     something that matches the very strong major frequency.
     
     idea 2: count the n loudest frequencies, but scale
             them so that variations in the loudest frequency
             count more than variations in the less loud freqs.

     better, but arbitrary when the the n loudest frequencies
     are close to one another in amplitude.

     idea 3: count the n loudest frequencies, but scale
             each one by the inverse of its amplitude.

     bad, because we might confuse a 50hz peak at .5 amplitude
     with a 25hz peak at 1.0 amplitude, which is wrong.

     idea 4: classifier is n/2 pairs of (freq, amplitude)
             sorted by amplitude.

     bad because it has less frequency information, but it
     is unlikely to cause false matches. Has the same drawback
     as idea 1.

     idea 5: classifier is n/2 pairs of (scaled freq, amplitude),
             sorted by amplitude

     seems to still have the same problem as #3, although it is
     tempered by the fact that the amplitudes will not match. Maybe 
     we can adjust the extent to which we care about exact matches 
     by scaling the amplitude by a constant?

     idea 6: classifier is n/2 pairs of (freq, amplitude), where
             each pair is scaled by a constant determined by its
             index.

     only bad when the n loudest frequencies are close to one
     another in amplitude.

     
     Overall I think 6 is best, but 2 is easiest to implement
     and might get best results anyway because it has twice as
     many dimensions.
  */
  
  /* METHOD 3.
     collect the DIMENSION highest bins,
     sorted in descending order. */

  int best[DIMENSION] {};
  { 
    for(int j = 0; j < DIMENSION; j ++) best[j] = -1;
  }

  /* loop and insert */
  {
    int low = 0;
    int hi = samples;

    if (getparameter_i(P_FFTRANGE) == FFTR_AUDIBLE) {
      /* XXX just a guess. Need to know the sample rate.
         But, whatever. */
      low = .005 * samples;
      hi = samples * 0.95;
    }
    
    /* sanity check in case buffer is really small */
    if (low < 0 || low >= samples) low = 0;
    if (hi > samples) hi = samples;

    for(int i = low; i < hi; i ++) {

      int m = i;
      /* m will be inserted if larger than fftr[best[j]]
         or best[j] == -1 */
      for(int j = 0; j < DIMENSION; j ++) {
        if (best[j] == -1) {
          best[j] = m; break;
        } else {
          /* better than current best */
          if (fftr[m] > fftr[best[j]]) {
            int tmp = best[j];
            best[j] = m;
            m = tmp;
          } else break;
        }
      }

    }
  }

  /* now we have the top DIMENSION frequencies */

  {
    /* nb. if we read fftr[best[i]] we had better
       check that best[i] <> -1 */
    for(int i = 0; i < DIMENSION; i ++) {
      out[i] = (float)best[i];
    }

    /* then scale */
    float sc = 0.25;
    for(int j = DIMENSION - 1; j >= 0; j --) {
      out[j] *= sc;
      sc *= 2.0;
    }
  }
}

void PLUGINCORE::classify(float const * in, float & scale, 
                          ANNpoint & out, long samples) {
  classify_fft(in, scale, out, samples);
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


void PLUGINCORE::process(std::span<float const> tin, std::span<float> tout) {
  int z = 0;

  for (size_t ii = 0; ii < tout.size(); ii++) {

    /* copy sample in */
    in0[insize] = tin[ii];
    insize ++;
 
    if (insize == framesize) {
      /* frame is full! */

      /* in0 -> process -> out0(first free space) */
      processw(in0.data(), out0.data()+outstart+outsize, framesize);

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
      std::copy_n(std::next(out0.cbegin(), outstart + outsize + third), third, prevmix.begin());

      /* copy 2nd third of input over in0 (need to re-use it for next frame), 
         now insize = third */
      std::copy_n(std::next(in0.cbegin(), third), third, in0.begin());

      insize = third;
      
      outsize += third;
    }

    /* send sample out */
    tout[ii] = out0[outstart];

    outstart ++;
    outsize --;

    /* make sure there is always enough room for a frame in out buffer */
    if (outstart == third) {
      memmove(out0.data(), out0.data() + outstart, outsize * sizeof (float));
      outstart = 0;
    }
  }
}


void PLUGIN::makepresets() {
  /* initialize presets here, see geometer for an example. */
}
