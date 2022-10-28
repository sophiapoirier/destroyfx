/*------------------------------------------------------------------------
Copyright (C) 2001-2022  Tom Murphy 7 and Sophia Poirier

This file is part of Transverb.

Transverb is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Transverb is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Transverb.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#pragma once

#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <span>
#include <vector>

#include "dfxmath.h"
#include "dfxplugin.h"
#include "dfxsmoothedvalue.h"
#include "iirfilter.h"
#include "transverb-base.h"


class TransverbDSP final : public DfxPluginCore {

public:
  explicit TransverbDSP(DfxPlugin& inDfxPlugin);

  void process(std::span<float const> inAudio, std::span<float> outAudio) override;
  void reset() override;
  void processparameters() override;

private:
  static constexpr int kAudioSmoothingDur_samples = 42;
  static constexpr double kHighpassFilterCutoff = 39.;
  static constexpr size_t kNumFIRTaps = 23;
  static constexpr double kFIRSpeedThreshold = 5.;
  static constexpr double kUnitySpeed = 1.;

  enum class FilterMode { None, Highpass, LowpassIIR, LowpassFIR };

  struct Head {
    dfx::SmoothedValue<double> speed;
    dfx::SmoothedValue<float> mix, feed;

    double read = 0.;
    std::vector<float> buf;

    dfx::IIRFilter filter;
    std::array<float, kNumFIRTaps> firCoefficients {};
    bool speedHasChanged = false;

    int smoothcount = 0;
    float smoothstep = 0.f;
    float lastdelayval = 0.f;

    void reset();
  };

  static constexpr float interpolateHermite(std::span<float const> data, double readaddress, int writeaddress);
  // uses only the fractional portion of the address
  static constexpr float interpolateLinear(float value1, float value2, double address)
  {
    auto const posFract = static_cast<float>(dfx::math::ModF(address));
    return std::lerp(value1, value2, posFract);
  }
  static constexpr float interpolateLinear(std::span<float const> data, double readaddress/*, int writeaddress*/);

  // negative input values are bumped into non-negative range by incremements of modulo
  static constexpr int mod_bipolar(int value, int modulo);
  static inline double fmod_bipolar(double value, double modulo);

  // these store the parameter values
  int bsize = 0;
  dfx::SmoothedValue<float> drymix;
  long quality = 0;
  bool tomsound = false;

  int writer = 0;
  std::array<Head, dfx::TV::kNumDelays> heads;

  int const MAXBUF;  // the size of the audio buffer (dependent on sampling rate)

  std::vector<float> const firCoefficientsWindow;
};



class Transverb final : public DfxPlugin {

public:
  explicit Transverb(TARGET_API_BASE_INSTANCE_TYPE inInstance);

  void dfx_PostConstructor() override;

  bool loadpreset(size_t index) override;  // overridden to support the random preset
  void randomizeparameters() override;

  dfx::StatusCode dfx_GetPropertyInfo(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned int inItemIndex,
                                      size_t& outDataSize, dfx::PropertyFlags& outFlags) override;
  dfx::StatusCode dfx_GetProperty(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned int inItemIndex,
                                  void* outData) override;
  dfx::StatusCode dfx_SetProperty(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned int inItemIndex,
                                  void const* inData, size_t inDataSize) override;

protected:
  size_t settings_sizeOfExtendedData() const noexcept override;
  void settings_saveExtendedData(void* outData, bool isPreset) override;
  void settings_restoreExtendedData(void const* inData, size_t storedExtendedDataSize,
                                    unsigned int dataVersion, bool isPreset) override;
  void settings_doChunkRestoreSetParameterStuff(dfx::ParameterID parameterID, float value, unsigned int dataVersion, std::optional<size_t> presetIndex) override;

private:
  static constexpr size_t kNumPresets = 16;

  void initPresets();
  auto& speedModeStateFromPropertyID(dfx::PropertyID inPropertyID) {
    return speedModeStates.at(dfx::TV::speedModePropertyIDToIndex(inPropertyID));
  }

  std::array<uint32_t, dfx::TV::kNumDelays> speedModeStates {};
};


constexpr int TransverbDSP::mod_bipolar(int value, int modulo) {
  assert(modulo > 0);
  while (value < 0) {
    value += modulo;
  }
  return value % modulo;
}

inline double TransverbDSP::fmod_bipolar(double value, double modulo) {
  assert(modulo > 0.);
  while (value < 0.) {
    value += modulo;
  }
  return std::fmod(value, modulo);
}

constexpr float TransverbDSP::interpolateHermite(std::span<float const> data, double readaddress,
                                                 int writeaddress) {
  assert(readaddress >= 0.);
  assert(writeaddress >= 0);
  assert(!data.empty());

  auto const [posFract, pos] = dfx::math::ModF<size_t>(readaddress);
  assert(pos < data.size());
  size_t posMinus1 = 0, posPlus1 = 0, posPlus2 = 0;

  // because the readers and writer are not necessarily aligned,
  // upcoming or previous samples could be discontiguous, in which case
  // just "interpolate" with repeated samples
  switch (mod_bipolar(writeaddress - static_cast<int>(pos), std::ssize(data))) {
    case 0:  // the previous sample is bogus
      posMinus1 = pos;
      posPlus1 = (pos + 1) % data.size();
      posPlus2 = (pos + 2) % data.size();
      break;
    case 1:  // the next 2 samples are bogus
      posMinus1 = (pos == 0) ? (data.size() - 1) : (pos - 1);
      posPlus1 = posPlus2 = pos;
      break;
    case 2:  // the sample 2 steps ahead is bogus
      posMinus1 = (pos == 0) ? (data.size() - 1) : (pos - 1);
      posPlus1 = posPlus2 = (pos + 1) % data.size();
      break;
    default:  // everything's cool
      posMinus1 = (pos == 0) ? (data.size() - 1) : (pos - 1);
      posPlus1 = (pos + 1) % data.size();
      posPlus2 = (pos + 2) % data.size();
      break;
  }

  return dfx::math::InterpolateHermite(data[posMinus1], data[pos], data[posPlus1], data[posPlus2], posFract);
}

/*
constexpr float TransverbDSP::interpolateHermitePostLowpass(std::span<float const> data, float address) {
  assert(address >= 0.f);
  assert(!data.empty());

  auto const posFract = dfx::math::ModF(pos);
  return dfx::math::InterpolateHermite(data[0], data[1], data[2], data[3], posFract);
}
*/

constexpr float TransverbDSP::interpolateLinear(std::span<float const> data, double readaddress/*, int writeaddress*/) {
  assert(readaddress >= 0.);
  //assert(writeaddress >= 0);
  assert(!data.empty());

  auto const pos = static_cast<size_t>(readaddress);
  assert(pos < data.size());
#if 0
  if (mod_bipolar(writeaddress - static_cast<int>(pos), std::ssize(data)) == 1) {
    // the upcoming sample is not contiguous because
    // the write head is about to write to it
    return data[pos];
  }
#endif
  auto const posPlus1 = (pos + 1) % data.size();
  return interpolateLinear(data[pos], data[posPlus1], readaddress);
}
