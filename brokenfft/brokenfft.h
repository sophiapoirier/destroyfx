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

// XXX maybe we should just inline these?
#define PLUGIN PLUGIN_CLASS_NAME
#define PLUGINCORE BrokenFFTDSP

class PLUGIN final : public DfxPlugin {
public:
  static constexpr long BUFFERSIZESSIZE = 14;

  static constexpr std::array<int, BUFFERSIZESSIZE> buffersizes {{ 
    4, 8, 16, 32, 64, 128, 256, 512, 
    1024, 2048, 4096, 8192, 16384, 32768, 
  }};

  PLUGIN(TARGET_API_BASE_INSTANCE_TYPE inInstance);

  void dfx_PostConstructor() override;

  long dfx_GetPropertyInfo(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned long inItemIndex, 
                           size_t& outDataSize, dfx::PropertyFlags& outFlags) override;
  long dfx_GetProperty(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned long inItemIndex, 
                       void* outData) override;

  void randomizeparameter(long inParameterIndex) override;

  void clearwindowcache();
  void updatewindowcache(class PLUGINCORE const * geometercore);

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

class PLUGINCORE final : public DfxPluginCore {
public:
  PLUGINCORE(DfxPlugin* inDfxPlugin);

  void reset() override;
  void processparameters() override;
  void process(float const* inAudio, float* outAudio, unsigned long inNumFrames) override;

  /* several of these are needed by geometerview. */
public:

  int processw(float const * in, float * out, int samples,
	       int * px, float * py, int maxpts,
	       int * tx, float * ty) const;

  int getframesize() const noexcept { return framesize; }
  float const* getinput() const noexcept { return in0.data(); }

private:

  void updatewindowsize();
  void updatewindowshape();

  bool iswaveformsource() { return (GetChannelNum() == 0); }
  void clearwindowcache();
  void updatewindowcache(PLUGINCORE const * geometercore);

  PLUGIN* const brokenfft;

  /* input and output buffers. out is framesize*2 samples long, in is framesize
     samples long. (for maximum framesize)
  */
  std::vector<float> in0, out0;

  /* buffersize is 3 * third, framesize is 2 * third 
     buffersize is used for outbuf.
  */
  int bufsize = 0, framesize = 0, third = 0;

  /* third-sized tail of previous processed frame. already has mixing envelope
     applied.
  */
  std::vector<float> prevmix;

  /* number of samples in in0 */
  int insize = 0;

  /* number of samples and starting position of valid samples in out0 */
  int outsize = 0;
  int outstart = 0;

  /* ---------- XXX geometer stuff ----------- */

  static int pointops(long pop, int npts, float op_param, int samps,
		      int * px, float * py, int maxpts,
		      int * tempx, float * tempy);

  /* shape of envelope */
  long shape {};

  long pointstyle {};
  float pointparam = 0.0f;

  long interpstyle {};
  float interparam = 0.0f;

  long pointop1 {};
  long pointop2 {};
  long pointop3 {};

  float oppar1 = 0.0f;
  float oppar2 = 0.0f;
  float oppar3 = 0.0f;

  std::vector<int> pointx;
  std::vector<float> pointy;

  std::vector<int> storex;
  std::vector<float> storey;

  std::vector<float> windowenvelope;
};

#endif
