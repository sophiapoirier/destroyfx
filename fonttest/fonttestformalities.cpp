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

#include "fonttest.h"

#include <algorithm>
#include <cassert>
#include <cmath>

#include "dfxmisc.h"


// these are macros that do boring entry point stuff for us
DFX_EFFECT_ENTRY(FontTest)
DFX_CORE_ENTRY(FontTestDSP)

using namespace dfx::FT;



FontTest::FontTest(TARGET_API_BASE_INSTANCE_TYPE inInstance)
  : DfxPlugin(inInstance, kNumParameters, kNumPresets) {

  initparameter_f(kPlaceholder, {"placeholder", "Phdr"}, 2700.0, 333.0, 1.0, 3000.0, DfxParam::Unit::MS);

  settailsize_seconds(1.0);

  setpresetname(0, PLUGIN_NAME_STRING);  // default preset name
  initPresets();

  addchannelconfig(kChannelConfig_AnyMatchedIO);  // N-in/N-out
  addchannelconfig(1, 1);
}

void FontTest::dfx_PostConstructor() {
}

FontTestDSP::FontTestDSP(DfxPlugin* inDfxPlugin)
  : DfxPluginCore(inDfxPlugin) {
}


void FontTestDSP::reset() {
}

void FontTestDSP::createbuffers() {
}

void FontTestDSP::clearbuffers() {
}

void FontTestDSP::releasebuffers() {
}


void FontTestDSP::processparameters() {
}


void FontTest::initPresets() {
}

bool FontTest::loadpreset(long index) {
	return false;
}

void FontTest::randomizeparameters() {
}

long FontTest::dfx_GetPropertyInfo(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned long inItemIndex,
                                    size_t& outDataSize, dfx::PropertyFlags& outFlags)
{
  return DfxPlugin::dfx_GetPropertyInfo(inPropertyID, inScope, inItemIndex, outDataSize, outFlags);
}

long FontTest::dfx_GetProperty(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned long inItemIndex,
                                void* outData)
{
  return DfxPlugin::dfx_GetProperty(inPropertyID, inScope, inItemIndex, outData);
}

long FontTest::dfx_SetProperty(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned long inItemIndex,
                                void const* inData, size_t inDataSize)
{
  return DfxPlugin::dfx_SetProperty(inPropertyID, inScope, inItemIndex, inData, inDataSize);
}


size_t FontTest::settings_sizeOfExtendedData() const noexcept
{
  return 0;
}

void FontTest::settings_saveExtendedData(void* outData, bool /*isPreset*/)
{
}

void FontTest::settings_restoreExtendedData(void const* inData, size_t storedExtendedDataSize,
                                             long dataVersion, bool /*isPreset*/)
{
}

void FontTest::settings_doChunkRestoreSetParameterStuff(long tag, float value, long dataVersion, long presetNum)
{
}

