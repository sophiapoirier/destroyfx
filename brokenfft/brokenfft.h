/*------------------------------------------------------------------------
Copyright (C) 2002-2020  Tom Murphy 7 and Sophia Poirier

This file is part of BrokenFFT.

BrokenFFT is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

BrokenFFT is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with BrokenFFT.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#ifndef _DFX_BROKENFFT_H
#define _DFX_BROKENFFT_H

#include <array>
#include <atomic>
#include <vector>

#include "dfxmutex.h"
#include "dfxplugin.h"
#include "brokenfft-base.h"

#include "rfftw.h"

// XXX maybe we should just inline this?
// #define PLUGIN PLUGIN_CLASS_NAME

class BrokenFFT final : public DfxPlugin {
public:
  static constexpr long BUFFERSIZESSIZE = 14;

  // (XXX these are actually "framesizes"; mismatch between
  // internal and external nomenclature..)
  static constexpr std::array<int, BUFFERSIZESSIZE> buffersizes {{
    4, 8, 16, 32, 64, 128, 256, 512,
    1024, 2048, 4096, 8192, 16384, 32768,
  }};

  BrokenFFT(TARGET_API_BASE_INSTANCE_TYPE inInstance);

  void dfx_PostConstructor() override;

  long dfx_GetPropertyInfo(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned long inItemIndex,
                           size_t& outDataSize, dfx::PropertyFlags& outFlags) override;
  long dfx_GetProperty(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned long inItemIndex,
                       void* outData) override;

  void randomizeparameter(long inParameterIndex) override;

  void clearwindowcache();
  void updatewindowcache(class BrokenFFTDSP const * brokenfftcore);

protected:
  std::optional<dfx::ParameterAssignment> settings_getLearningAssignData(long inParameterIndex) const override;

private:
  static constexpr long NUM_PRESETS = 16;

  /* set up the built-in presets */
  void makepresets();

  BrokenFFTViewData windowcache;
  /* passed to processw for window cache */
  std::array<int, BrokenFFTViewData::arraysize> tmpx;
  std::array<float, BrokenFFTViewData::arraysize> tmpy;
  dfx::SpinLock windowcachelock;
  std::atomic<uint64_t> lastwindowtimestamp {0};
  static_assert(decltype(lastwindowtimestamp)::is_always_lock_free);
};

class BrokenFFTDSP final : public DfxPluginCore {
public:
  BrokenFFTDSP(DfxPlugin* inDfxPlugin);

  void reset() override;
  void processparameters() override;
  void process(float const* inAudio, float* outAudio, unsigned long inNumFrames) override;

  /* several of these are needed by brokenfftview. */
public:

  void FFTOps(long samples);
  void ProcessW(float const * in, float * out, int samples);

  int getframesize() const noexcept { return framesize; }
  float const* getinput() const noexcept { return in0.data(); }

  struct AmplEntry {
    float a = 0.0f;
    int i = 0;
    AmplEntry(float a, int i) : a(a), i(i) {}
  };

private:

  void Normalize(long samples, float much);

  void updatewindowsize();
  void updatewindowshape();

  bool IsFFTDataSource() { return GetChannelNum() == 0; }
  void clearwindowcache();
  void updatewindowcache(BrokenFFTDSP const * brokenfftcore);

  BrokenFFT *const brokenfft = nullptr;

  // input and output buffers. out is framesize*2 samples long, in is
  // framesize samples long. (for maximum framesize)
  std::vector<float> in0, out0;

  // buffersize is 3 * third, framesize is 2 * third
  // buffersize is used for outbuf.
  int bufsize = 0, framesize = 0, third = 0;

  // third-sized tail of previous processed frame. already has mixing
  // envelope applied.
  std::vector<float> prevmix;

  // number of samples in in0
  int insize = 0;

  // number of samples and starting position of valid samples in out0
  int outsize = 0;
  int outstart = 0;

  // shape of envelope (XXX enum?)
  long shape = 0;
  std::vector<float> windowenvelope;

  // BrokenFFT stufff...

  void fftops(long samples);

  // Parameters, set in processparameters.
  int fft_method = METHOD_WEST;

  float destruct = 0.0f;

  float perturb = 0.0f, quant = 0.0f, rotate = 0.0f;
  float binquant = 0.0f;
  float spike = 0.0f, spikehold = 0.0f;

  float compress = 0.0f;
  float makeupgain = 0.0f;

  float echomix = 0.0f, echotime = 0.0f;
  float echomodf = 0.0f, echomodw = 0.0f;
  float echofb = 0.0f;
  float echolow = 0.0f, echohi = 0.0f;

  float postrot = 0.0f;

  float lowpass = 0.0f;

  float moments = 0.0f;

  float bride = 0.0f;
  float blow = 0.0f;
  float convolve = 0.0f;
  float harm = 0.0f;
  float afterlow = 0.0f;
  float norm = 0.0f;

  // ?
  int amplhold = 0, stopat = 0;
  // XXX vectors?
  AmplEntry *ampl = nullptr;
  float *echor = nullptr;
  float *echoc = nullptr;
  int echoctr = 0;

  // XXX ded? vectors?
  float *left_buffer = nullptr;
  float *right_buffer = nullptr;

  int lastblocksize = 0;

  // Various state, pls document
  float sampler = 0.0f, samplei = 0.0f;
  float bit1 = 0.0f, bit2 = 0.0f;
  int samplesleft = 0;

  // fft work area
  // XXX vectors?
  // XXX tmp appears to be a copy of the input
  // that we don't modify; do we need it?
  float *tmp = nullptr;
  float *oot = nullptr;
  float *fftr = nullptr;
  float *ffti = nullptr;


  // FIXME! LEAK! create_plan allocates a tree data structure.
  // We need to rfftw_destroy_plan these when we
  // replace them.
  rfftw_plan plan = nullptr;
  rfftw_plan rplan = nullptr;
};

#endif
