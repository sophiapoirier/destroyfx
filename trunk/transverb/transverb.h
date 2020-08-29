/*------------------------------------------------------------------------
Copyright (C) 2001-2020  Tom Murphy 7 and Sophia Poirier

This file is part of Transverb.

Transverb is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Transverb is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Transverb.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#pragma once

#include <array>
#include <vector>

#include "dfxplugin.h"
#include "dfxsmoothedvalue.h"
#include "iirfilter.h"
#include "transverb-base.h"


class TransverbDSP final : public DfxPluginCore {

public:
  explicit TransverbDSP(DfxPlugin* inDfxPlugin);

  void process(float const* inAudio, float* outAudio, unsigned long inNumFrames) override;
  void reset() override;
  void processparameters() override;
  void createbuffers() override;
  void clearbuffers() override;
  void releasebuffers() override;

private:
  static constexpr long kAudioSmoothingDur_samples = 42;
  static constexpr size_t kNumFIRTaps = 23;

  enum class FilterMode { Nothing, Highpass, LowpassIIR, LowpassFIR };

  inline float InterpolateHermite(float* data, double address, int arraysize, int danger);
  inline float InterpolateLinear(float* data, double address, int arraysize, int danger);

  // these get set to the parameter values
  int bsize = 0;
  dfx::SmoothedValue<double> speed1, speed2;
  dfx::SmoothedValue<float> drymix;
  dfx::SmoothedValue<float> mix1, feed1;
  float dist1 = 0.0f;
  dfx::SmoothedValue<float> mix2, feed2;
  float dist2 = 0.0f;
  long quality = 0;
  bool tomsound = false;

  int writer = 0;
  double read1 = 0.0, read2 = 0.0;

  std::vector<float> buf1;
  std::vector<float> buf2;
  int MAXBUF = 0;  // the size of the audio buffer (dependent on sampling rate)

  dfx::IIRfilter filter1, filter2;
  bool speed1hasChanged = false, speed2hasChanged = false;

  int smoothcount1 = 0, smoothcount2 = 0;
  int smoothdur1 = 0, smoothdur2 = 0;
  float smoothstep1 = 0.0f, smoothstep2 = 0.0f;
  float lastr1val = 0.0f, lastr2val = 0.0f;

  std::vector<float> firCoefficients1, firCoefficients2;
  std::vector<float> const firCoefficientsWindow;
};



class Transverb final : public DfxPlugin {

public:
  Transverb(TARGET_API_BASE_INSTANCE_TYPE inInstance);

  void dfx_PostConstructor() override;

  bool loadpreset(long index) override;  // overriden to support the random preset
  void randomizeparameters() override;

  long dfx_GetPropertyInfo(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned long inItemIndex, 
                           size_t& outDataSize, dfx::PropertyFlags& outFlags) override;
  long dfx_GetProperty(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned long inItemIndex, 
                       void* outData) override;
  long dfx_SetProperty(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned long inItemIndex, 
                       void const* inData, size_t inDataSize) override;

protected:
  size_t settings_sizeOfExtendedData() const noexcept override;
  void settings_saveExtendedData(void* outData, bool isPreset) override;
  void settings_restoreExtendedData(void const* inData, size_t storedExtendedDataSize, 
                                    long dataVersion, bool isPreset) override;
  void settings_doChunkRestoreSetParameterStuff(long tag, float value, long dataVersion, long presetNum) override;

private:
  static constexpr long kNumPresets = 16;

  void initPresets();
  auto& speedModeStateFromPropertyID(dfx::PropertyID inPropertyID) {
    return speedModeStates.at(dfx::TV::speedModePropertyIDToIndex(inPropertyID));
  }

  std::array<int32_t, dfx::TV::kNumDelays> speedModeStates;
};


inline float TransverbDSP::InterpolateHermite(float* data, double address, 
                                              int arraysize, int danger) {
  int posMinus1 = 0, posPlus1 = 0, posPlus2 = 0;

  auto const pos = (int)address;
  auto const posFract = (float)(address - (double)pos);

  // because the readers and writer are not necessarily aligned, 
  // upcoming or previous samples could be discontiguous, in which case 
  // just "interpolate" with repeated samples
  switch (danger) {
    case 0:  // the previous sample is bogus
      posMinus1 = pos;
      posPlus1 = (pos + 1) % arraysize;
      posPlus2 = (pos + 2) % arraysize;
      break;
    case 1:  // the next 2 samples are bogus
      posMinus1 = (pos == 0) ? (arraysize - 1) : (pos - 1);
      posPlus1 = posPlus2 = pos;
      break;
    case 2:  // the sample 2 steps ahead is bogus
      posMinus1 = (pos == 0) ? (arraysize - 1) : (pos - 1);
      posPlus1 = posPlus2 = (pos + 1) % arraysize;
      break;
    default:  // everything's cool
      posMinus1 = (pos == 0) ? (arraysize - 1) : (pos - 1);
      posPlus1 = (pos + 1) % arraysize;
      posPlus2 = (pos + 2) % arraysize;
      break;
  }

  float const a = ((3.0f * (data[pos] - data[posPlus1])) - 
                   data[posMinus1] + data[posPlus2]) * 0.5f;
  float const b = (2.0f * data[posPlus1]) + data[posMinus1] - 
                  (2.5f * data[pos]) - (data[posPlus2] * 0.5f);
  float const c = (data[posPlus1] - data[posMinus1]) * 0.5f;

  return (((a * posFract) + b) * posFract + c) * posFract + data[pos];
}

/*
inline float TransverbDSP::InterpolateHermitePostLowpass(float* data, float address) {
  auto const pos = (int)address;
  float const posFract = address - (float)pos;

  float const a = ((3.0f * (data[1] - data[2])) - 
                   data[0] + data[3]) * 0.5f;
  float const b = (2.0f * data[2]) + data[0] - 
                  (2.5f * data[1]) - (data[3] * 0.5f);
  float const c = (data[2] - data[0]) * 0.5f;

  return (((a * posFract) + b) * posFract + c) * posFract + data[1];
}
*/

inline float TransverbDSP::InterpolateLinear(float* data, double address, 
                                             int arraysize, int danger) {
	int posPlus1 = 0;
	auto const pos = (int)address;
	auto const posFract = (float)(address - (double)pos);

	if (danger == 1) {
		// the upcoming sample is not contiguous because 
		// the write head is about to write to it
		posPlus1 = pos;
	} else {
		// it's alright
		posPlus1 = (pos + 1) % arraysize;
	}
	return (data[pos] * (1.0f - posFract)) + 
			(data[posPlus1] * posFract);
}
