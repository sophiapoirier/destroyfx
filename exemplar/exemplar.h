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

DFX Exemplar, starring the Super Destroy FX Windowing System!
------------------------------------------------------------------------*/

#pragma once

#include <algorithm>
#include <array>
#include <memory>
#include <vector>

#include "dfxmisc.h"
#include "dfxplugin.h"
#include "ANN/ANN.h"
#include "rfftw.h"

/* change these for your plugins */
#define PLUGIN Exemplar

static constexpr std::array buffersizes {
  4, 8, 16, 32, 64, 128, 256, 512, 
  1024, 2048, 4096, 8192, 16384, 32768, 
};


// PLUGIN ## DSP
#define PLUGINCORE ExemplarDSP

/* the types of window shapes available for smoothity */
enum { WINDOW_TRIANGLE, 
       WINDOW_ARROW, 
       WINDOW_WEDGE, 
       WINDOW_COS, 
       NUM_WINDOWSHAPES,
       MAX_WINDOWSHAPES=16
};

enum { MODE_CAPTURE,
       MODE_MATCH,
       NUM_MODES, };

enum { FFTR_AUDIBLE,
       FFTR_ALL,
       NUM_FFTRS, };

/* the names of the parameters */
enum : dfx::ParameterID { P_BUFSIZE, P_SHAPE, 
                          P_MODE, P_FFTRANGE,
                          P_ERRORAMOUNT,
                          NUM_PARAMS
};


class PLUGIN final : public DfxPlugin {
public:
  explicit PLUGIN(TARGET_API_BASE_INSTANCE_TYPE inInstance);

private:
  static constexpr size_t NUM_PRESETS = 16;

  /* set up the built-in presets */
  void makepresets();
};

class PLUGINCORE final : public DfxPluginCore {
public:
  explicit PLUGINCORE(DfxPlugin& inInstance);

  void reset() override;
  void processparameters() override;
  void process(std::span<float const> in, std::span<float> out) override;

  long getwindowsize() const noexcept { return third; }

private:

  /* input and output buffers. out is framesize*2 samples long, in is framesize
     samples long. (for maximum framesize)
  */
  std::vector<float> in0, out0;

  /* bufsize is 3 * third, framesize is 2 * third 
     bufsize is used for outbuf.
  */
  long bufsize = 0, framesize = 0, third = 0;

  void processw(float const * in, float * out, long samples);

  int shape = 0;

  /* third-sized tail of previous processed frame. already has mixing envelope
     applied.
   */
  std::vector<float> prevmix;

  /* number of samples in in0 */
  int insize = 0;

  /* number of samples and starting position of valid samples in out0 */
  int outsize = 0;
  int outstart = 0;



  /* Exemplar stuff */
  /* classifies a window */
  void classify(float const * in, float & scale, ANNpoint & out, long samples);

  /* specific classifiers */
  void classify_haar(float const * in, float & scale, ANNpoint & out, long samples);
  void classify_fft (float const * in, float & scale, ANNpoint & out, long samples);


  bool capturemode = true;
  float erroramount = 0.f;

  static constexpr size_t CAPBUFFER = 500000; /* 1000000 */
  float capsamples[CAPBUFFER] {};
  int ncapsamples = 0; /* < CAPBUFFER */
  
#if 0
  /* an individual capture */
  struct caplet {
    /* its classification */
    ANNpoint p;
    /* its index in capsamples */
    int i;
    /* width? */
  };
#endif

  ANNpoint cap_point[CAPBUFFER] {};
  int cap_index[CAPBUFFER] {};
  float cap_scale[CAPBUFFER] {};
  int npoints = 0;

  std::unique_ptr<ANNkd_tree> nntree;

  /* for FFTW: current analysis plan */
  dfx::UniqueOpaqueType<rfftw_plan, rfftw_destroy_plan> plan, rplan;

  /* result of ffts ( */
  float fftr[*std::ranges::max_element(buffersizes)] {};

};
