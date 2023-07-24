/*------------------------------------------------------------------------
Copyright (C) 2002-2023  Tom Murphy 7

This file is part of Windowingstub.

Windowingstub is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Windowingstub is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Windowingstub.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org

Windowingstub, featuring the Super Destroy FX Windowing System!
------------------------------------------------------------------------*/

/*  */

#include "windowingstub.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>

#include "dfxmath.h"

/* this macro does boring entry point stuff for us */
DFX_ENTRY(Windowingstub);
DFX_CORE_ENTRY(WindowingstubDSP);


PLUGIN::PLUGIN(TARGET_API_BASE_INSTANCE_TYPE inInstance)
  : DfxPlugin(inInstance, NUM_PARAMS, NUM_PRESETS) {

  initparameter_indexed(P_BUFSIZE, {"wsize"}, 9, 9, std::ssize(buffersizes), kDfxParamUnit_samples);
  initparameter_indexed(P_SHAPE, {"wshape"}, WINDOW_TRIANGLE, WINDOW_TRIANGLE, MAX_WINDOWSHAPES);

  /* set up values for windowing */
  for (size_t i = 0; i < buffersizes.size(); i++) {
    std::array<char, 64> bufstr {};
    if (buffersizes[i] > 1000)
      std::snprintf(bufstr.data(), bufstr.size(), "%ld,%03ld", buffersizes[i] / 1000, buffersizes[i] % 1000);
    else
      std::snprintf(bufstr.data(), bufstr.size(), "%ld", buffersizes[i]);
    setparametervaluestring(P_BUFSIZE, static_cast<long>(i), bufstr.data());
  }

  setparametervaluestring(P_SHAPE, WINDOW_TRIANGLE, "linear");
  setparametervaluestring(P_SHAPE, WINDOW_ARROW, "arrow");
  setparametervaluestring(P_SHAPE, WINDOW_WEDGE, "wedge");
  setparametervaluestring(P_SHAPE, WINDOW_COS, "best");
  for (int i = NUM_WINDOWSHAPES; i < MAX_WINDOWSHAPES; i++)
    setparametervaluestring(P_SHAPE, i, "???");

  auto const delay_samples = dfx::math::ToUnsigned(buffersizes.at(getparameter_i(P_BUFSIZE)));
  setlatency_samples(delay_samples);
  settailsize_samples(delay_samples);

  setpresetname(0, "Windowingstub Default"); /* default preset name */
  makepresets();

  /* allow MIDI keys to be used to control parameters */
  getsettings()->setAllowPitchbendEvents(true);
  getsettings()->setAllowNoteEvents(true);

#if !TARGET_PLUGIN_USES_DSPCORE
  addchannelconfig(1, 1);	/* mono */
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
}

void PLUGINCORE::reset() {

  framesize = buffersizes.at(getparameter_i(P_BUFSIZE));
  third = framesize / 2;
  bufsize = third * 3;

  /* set up buffers. Prevmix and first frame of output are always 
     filled with zeros. */


  std::fill_n(prevmix.begin(), third, 0.f);
  std::fill_n(out0.begin(), framesize, 0.f);
  
  /* start input at beginning. Output has a frame of silence. */
  insize = 0;
  outstart = 0;
  outsize = framesize;

  getplugin().setlatency_samples(dfx::math::ToUnsigned(framesize));
  /* tail is the same as delay, of course */
  getplugin().settailsize_samples(dfx::math::ToUnsigned(framesize));
}

void PLUGINCORE::processparameters() {

  shape = getparameter_i(P_SHAPE);

  #ifdef TARGET_API_VST
    /* this tells the host to call a suspend()-resume() pair, 
      which updates initialDelay value */
  if (getparameterchanged(P_BUFSIZE))
    getplugin().setlatencychanged(true);
  #endif
}

/* this processes an individual window. Basically, this is where you
   write your DSP, and it will be always called with the same sample
   size (as long as the block size parameter stays the same) and
   automatically overlapped. */
void PLUGINCORE::processw(float const * in, float * out, long samples) {

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
   - can we use tail of out0 as prevmix, instead of copying?
   - can we use circular buffers instead of memmoving a lot?
     (probably not)
*/


void PLUGINCORE::process(std::span<float const> tin, std::span<float> tout) {

  for (size_t i = 0; i < tout.size(); i++) {

    /* copy sample in */
    in0[insize] = tin[i];
    insize ++;
 
    if (insize == framesize) {
      /* frame is full! */

      /* in0 -> process -> out0(first free space) */
      processw(in0.data(), out0.data()+outstart+outsize, framesize);

      float oneDivThird = 1.0f / (float)third;
      /* apply envelope */

      switch(shape) {

        case WINDOW_TRIANGLE:
          for(int z = 0; z < third; z++) {
            out0[z+outstart+outsize] *= ((float)z * oneDivThird);
            out0[z+outstart+outsize+third] *= (1.0f - ((float)z * oneDivThird));
          }
          break;
        case WINDOW_ARROW:
          for(int z = 0; z < third; z++) {
            float p = (float)z * oneDivThird;
            p *= p;
            out0[z+outstart+outsize] *= p;
            out0[z+outstart+outsize+third] *= (1.0f - p);
          }
          break;
        case WINDOW_WEDGE:
          for(int z = 0; z < third; z++) {
            float p = std::sqrt((float)z * oneDivThird);
            out0[z+outstart+outsize] *= p;
            out0[z+outstart+outsize+third] *= (1.0f - p);
          }
          break;
        case WINDOW_COS:
          for(int z = 0; z < third; z ++) {
            float p = 0.5f * (-std::cos(PI * ((float)z * oneDivThird)) + 1.0f);
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
    tout[i] = out0[outstart];

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
