/*------------------------------------------------------------------------
Copyright (C) 2001-2021  Tom Murphy 7 and Sophia Poirier

This file is part of FontTest.

FontTest is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

FontTest is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with FontTest.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "dfxplugin.h"
#include "dfxsmoothedvalue.h"
#include "iirfilter.h"
#include "fonttest-base.h"


class FontTestDSP final : public DfxPluginCore {

public:
  explicit FontTestDSP(DfxPlugin* inDfxPlugin);

  void process(float const* inAudio, float* outAudio, unsigned long inNumFrames) override;
  void reset() override;
  void processparameters() override;
  void createbuffers() override;
  void clearbuffers() override;
  void releasebuffers() override;

private:
};



class FontTest final : public DfxPlugin {

public:
  explicit FontTest(TARGET_API_BASE_INSTANCE_TYPE inInstance);

  void dfx_PostConstructor() override;

  bool loadpreset(long index) override;  // overridden to support the random preset
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
};
