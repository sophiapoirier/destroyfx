/*------------------------------------------------------------------------
Copyright (C) 2001-2021  Tom Murphy 7 and Sophia Poirier

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

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "transverb.h"

#include <algorithm>
#include <cassert>
#include <cmath>

#include "dfxmisc.h"
#include "firfilter.h"


// these are macros that do boring entry point stuff for us
DFX_EFFECT_ENTRY(Transverb)
DFX_CORE_ENTRY(TransverbDSP)

using namespace dfx::TV;



Transverb::Transverb(TARGET_API_BASE_INSTANCE_TYPE inInstance)
  : DfxPlugin(inInstance, kNumParameters, kNumPresets) {

  initparameter_f(kBsize, {"buffer size", "BufSize", "BufSiz", "BfSz"}, 2700.0, 333.0, 1.0, 3000.0, DfxParam::Unit::MS);
  initparameter_f(kDrymix, {"dry mix", "DryMix", "Dry"}, 1.0, 1.0, 0.0, 1.0, DfxParam::Unit::LinearGain, DfxParam::Curve::Squared);
  initparameter_f(kMix1, {"1:mix", "1mix"}, 1.0, 1.0, 0.0, 1.0, DfxParam::Unit::LinearGain, DfxParam::Curve::Squared);
  initparameter_f(kDist1, {"1:dist", "1dst"}, 0.90009, 0.5, 0.0, 1.0, DfxParam::Unit::Scalar);
  initparameter_f(kSpeed1, {"1:speed", "1speed", "1spd"}, 0.0, 0.0, -3.0, 6.0, DfxParam::Unit::Octaves);
  initparameter_f(kFeed1, {"1:feedback", "1feedbk", "1fedbk", "1fdb"}, 0.0, 33.3, 0.0, 100.0, DfxParam::Unit::Percent);
  initparameter_f(kMix2, {"2:mix", "2mix"}, 0.0, 1.0, 0.0, 1.0, DfxParam::Unit::LinearGain, DfxParam::Curve::Squared);
  initparameter_f(kDist2, {"2:dist", "2dst"}, 0.1, 0.5, 0.0, 1.0, DfxParam::Unit::Scalar);
  initparameter_f(kSpeed2, {"2:speed", "2speed", "2spd"}, 1.0, 0.0, -3.0, 6.0, DfxParam::Unit::Octaves);
  initparameter_f(kFeed2, {"2:feedback", "2feedbk", "2fedbk", "2fdb"}, 0.0, 33.3, 0.0, 100.0, DfxParam::Unit::Percent);
  initparameter_list(kQuality, {"quality", "Qualty", "Qlty"}, kQualityMode_UltraHiFi, kQualityMode_UltraHiFi, kQualityMode_NumModes);
  initparameter_b(kTomsound, {"TOMSOUND", "TomSnd", "Tom7"}, false, false);
  initparameter_b(kFreeze, dfx::MakeParameterNames(dfx::kParameterNames_Freeze), false, false);

  setparametervaluestring(kQuality, kQualityMode_DirtFi, "dirt-fi");
  setparametervaluestring(kQuality, kQualityMode_HiFi, "hi-fi");
  setparametervaluestring(kQuality, kQualityMode_UltraHiFi, "ultra hi-fi");

  // distance parameters only have meaningful effect at zero speed which probably will never occur randomly,
  // and otherwise all they do is glitch a lot, so omit them
  addparameterattributes(kDist1, DfxParam::kAttribute_OmitFromRandomizeAll);
  addparameterattributes(kDist2, DfxParam::kAttribute_OmitFromRandomizeAll);
  addparameterattributes(kFreeze, DfxParam::kAttribute_OmitFromRandomizeAll);

  addparametergroup("head #1", {kSpeed1, kFeed1, kDist1});
  addparametergroup("head #2", {kSpeed2, kFeed2, kDist2});
  addparametergroup("mix", {kDrymix, kMix1, kMix2});


  settailsize_seconds(getparametermax_f(kBsize) * 0.001);

  setpresetname(0, PLUGIN_NAME_STRING);  // default preset name
  initPresets();

  addchannelconfig(kChannelConfig_AnyMatchedIO);  // N-in/N-out
  addchannelconfig(1, kNumDelays);  // mono-input / per-head-output

  speedModeStates.fill(kSpeedMode_Fine);
}

void Transverb::dfx_PostConstructor() {

#if TARGET_PLUGIN_USES_MIDI
  // since we don't use notes for any specialized control of Transverb, 
  // allow them to be assigned to control parameters via MIDI learn
  getsettings().setAllowPitchbendEvents(true);
  getsettings().setAllowNoteEvents(true);
#endif
}



TransverbDSP::TransverbDSP(DfxPlugin* inDfxPlugin)
  : DfxPluginCore(inDfxPlugin),
    firCoefficientsWindow(dfx::FIRFilter::generateKaiserWindow(kNumFIRTaps, 60.0f)) {

  firCoefficients1.assign(kNumFIRTaps, 0.0f);
  firCoefficients2.assign(kNumFIRTaps, 0.0f);

  registerSmoothedAudioValue(&speed1);
  registerSmoothedAudioValue(&speed2);
  registerSmoothedAudioValue(&drymix);
  registerSmoothedAudioValue(&mix1);
  registerSmoothedAudioValue(&feed1);
  registerSmoothedAudioValue(&mix2);
  registerSmoothedAudioValue(&feed2);
}


void TransverbDSP::reset() {

  smoothcount1 = smoothcount2 = 0;
  lastr1val = lastr2val = 0.0f;
  filter1.reset();
  filter2.reset();
  speed1hasChanged = speed2hasChanged = true;

  filter1.setSampleRate(getsamplerate());
  filter2.setSampleRate(getsamplerate());
}

void TransverbDSP::createbuffers() {

  MAXBUF = (int) (getparametermax_f(kBsize) * 0.001 * getsamplerate());

  buf1.assign(MAXBUF, 0.0f);
  buf2.assign(MAXBUF, 0.0f);
}

void TransverbDSP::clearbuffers() {

  std::fill(buf1.begin(), buf1.end(), 0.0f);
  std::fill(buf2.begin(), buf2.end(), 0.0f);
}

void TransverbDSP::releasebuffers() {

  MAXBUF = 0;
  buf1.clear();
  buf2.clear();
}


void TransverbDSP::processparameters() {

  if (auto const value = getparameterifchanged_f(kDrymix))
  {
    // balance the audio energy when mono input is fanned out to multiple output channels
    auto const dryGainScalar = std::sqrt(static_cast<double>(getplugin()->getnuminputs()) / 
                                         static_cast<double>(getplugin()->getnumoutputs()));
    drymix = *value * dryGainScalar;
  }
  if (auto const value = getparameterifchanged_f(kMix1))
    mix1 = *value;
  if (auto const value = getparameterifchanged_f(kSpeed1))
  {
    speed1 = std::pow(2.0, *value);
    speed1hasChanged = true;
  }
  if (auto const value = getparameterifchanged_scalar(kFeed1))
    feed1 = *value;
  dist1 = getparameter_f(kDist1);
  if (auto const value = getparameterifchanged_f(kMix2))
    mix2 = *value;
  if (auto const value = getparameterifchanged_f(kSpeed2))
  {
    speed2 = std::pow(2.0, *value);
    speed2hasChanged = true;
  }
  if (auto const value = getparameterifchanged_scalar(kFeed2))
    feed2 = *value;
  dist2 = getparameter_f(kDist2);
  quality = getparameter_i(kQuality);
  tomsound = getparameter_b(kTomsound);

  if (auto const value = getparameterifchanged_f(kBsize))
  {
    auto const entryBsize = std::exchange(bsize, std::clamp((int) (*value * getsamplerate() * 0.001), 1, MAXBUF));
    // the commented-out logic below is the beginning of an attempt to clean up the buffer states
    // when the buffer resizes, however the tidiness maybe feels counter to the spirit of Transverb?
    if (bsize > entryBsize)
    {
      //std::fill(std::next(buf1.begin(), entryBsize), std::next(buf1.begin(), bsize), 0.0f);
      //std::fill(std::next(buf2.begin(), entryBsize), std::next(buf2.begin(), bsize), 0.0f);
    }
    else if (writer > bsize)
    {
      //auto const entryWriter = writer;
      writer %= bsize;
      //auto const copyCount = std::min(entryBsize - bsize, writer);
      //std::copy_n(std::next(buf1.cbegin(), entryWriter - copyCount), copyCount, std::next(buf1.begin(), writer - copyCount));
      //std::copy_n(std::next(buf2.cbegin(), entryWriter - copyCount), copyCount, std::next(buf2.begin(), writer - copyCount));
    }
    read1 = std::fmod(std::fabs(read1), (double)bsize);
    read2 = std::fmod(std::fabs(read2), (double)bsize);
  }

  if (getparameterchanged(kDist1))
    read1 = std::fmod(std::fabs((double)writer + (double)dist1 * (double)bsize), (double)bsize);

  if (getparameterchanged(kDist2))
    read2 = std::fmod(std::fabs((double)writer + (double)dist2 * (double)bsize), (double)bsize);

  if (getparameterchanged(kQuality) || getparameterchanged(kTomsound))
    speed1hasChanged = speed2hasChanged = true;

  // stereo-split-heads mode (head 1 goes to left output and 2 to right)
  if (getplugin()->asymmetricalchannels())
  {
    if (GetChannelNum() == 0)
      mix2.setValueNow(getparametermin_f(kMix2));
    else if (GetChannelNum() == 1)
      mix1.setValueNow(getparametermin_f(kMix1));
  }
}



//--------- presets --------

void Transverb::initPresets() {

	long i = 1;

	setpresetname(i, "phaser up");
	setpresetparameter_f(i, kBsize, 48.687074827);
	setpresetparameter_f(i, kDrymix, 0.45);
	setpresetparameter_f(i, kMix1, 0.5);
	setpresetparameter_f(i, kDist1, 0.9);
	setpresetparameter_f(i, kSpeed1, 0.048406605 / 12.);
	setpresetparameter_f(i, kFeed1, 67.0);
	setpresetparameter_f(i, kMix2, 0.0);
	setpresetparameter_f(i, kDist2, 0.0);
	setpresetparameter_f(i, kSpeed2, getparametermin_f(kSpeed2));
	setpresetparameter_f(i, kFeed2, 0.0);
	setpresetparameter_i(i, kQuality, kQualityMode_UltraHiFi);
	setpresetparameter_b(i, kTomsound, false);
	setpresetparameter_b(i, kFreeze, false);
	i++;

	setpresetname(i, "phaser down");
	setpresetparameter_f(i, kBsize, 27.0);
	setpresetparameter_f(i, kDrymix, 0.45);
	setpresetparameter_f(i, kMix1, 0.5);
	setpresetparameter_f(i, kDist1, 0.0);
	setpresetparameter_f(i, kSpeed1, -0.12 / 12.);//-0.048542333 / 12.);
	setpresetparameter_f(i, kFeed1, 76.0);
	setpresetparameter_f(i, kMix2, 0.0);
	setpresetparameter_f(i, kDist2, 0.0);
	setpresetparameter_f(i, kSpeed2, getparametermin_f(kSpeed2));
	setpresetparameter_f(i, kFeed2, 0.0);
	setpresetparameter_i(i, kQuality, kQualityMode_UltraHiFi);
	setpresetparameter_b(i, kTomsound, false);
	setpresetparameter_b(i, kFreeze, false);
	i++;

	setpresetname(i, "aquinas");
	setpresetparameter_f(i, kBsize, 2605.1);
	setpresetparameter_f(i, kDrymix, 0.6276);
	setpresetparameter_f(i, kMix1, 1.0);
	setpresetparameter_f(i, kDist1, 0.993);
	setpresetparameter_f(i, kSpeed1, -4.665660556);
	setpresetparameter_f(i, kFeed1, 54.0);
	setpresetparameter_f(i, kMix2, 0.757);
	setpresetparameter_f(i, kDist2, 0.443);
	setpresetparameter_f(i, kSpeed2, 2.444534569);
	setpresetparameter_f(i, kFeed2, 46.0);
	setpresetparameter_i(i, kQuality, kQualityMode_UltraHiFi);
	setpresetparameter_b(i, kTomsound, false);
	setpresetparameter_b(i, kFreeze, false);
	i++;

	setpresetname(i, "glup drums");
	setpresetparameter_f(i, kBsize, 184.27356);
	setpresetparameter_f(i, kDrymix, 0.157);
	setpresetparameter_f(i, kMix1, 1.0);
	setpresetparameter_f(i, kDist1, 0.0945);
	setpresetparameter_f(i, kSpeed1, -8. / 12.);
	setpresetparameter_f(i, kFeed1, 97.6);
	setpresetparameter_f(i, kMix2, 0.0);
	setpresetparameter_f(i, kDist2, 0.197);
	setpresetparameter_f(i, kSpeed2, 0.978195651);
	setpresetparameter_f(i, kFeed2, 0.0);
	setpresetparameter_i(i, kQuality, kQualityMode_UltraHiFi);
	setpresetparameter_b(i, kTomsound, false);
	setpresetparameter_b(i, kFreeze, false);
	i++;

	setpresetname(i, "space invaders");
	setpresetparameter_f(i, kSpeed1, -0.23 / 12.);
	setpresetparameter_f(i, kFeed1, 73.0);
	setpresetparameter_f(i, kDist1, 0.1857);
	setpresetparameter_f(i, kSpeed2, 4.3 / 12.);
	setpresetparameter_f(i, kFeed2, 41.0);
	setpresetparameter_f(i, kDist2, 0.5994);
	setpresetparameter_f(i, kBsize, 16.8);
	setpresetparameter_f(i, kDrymix, 0.000576);
	setpresetparameter_f(i, kMix1, 1.0);
	setpresetparameter_f(i, kMix2, 0.1225);
	setpresetparameter_i(i, kQuality, kQualityMode_UltraHiFi);
	setpresetparameter_b(i, kTomsound, false);
	setpresetparameter_b(i, kFreeze, false);
	i++;

	setpresetname(i, "mudslap (by Styrofoam)");
	setpresetparameter_f(i, kSpeed1, -0.9748);
	setpresetparameter_f(i, kFeed1, 45.6285);
	setpresetparameter_f(i, kDist1, 0.02345);
	setpresetparameter_f(i, kSpeed2, -1.0252);
	setpresetparameter_f(i, kFeed2, 46.4819);
	setpresetparameter_f(i, kDist2, 0.0192);
	setpresetparameter_f(i, kBsize, 122.4947);
	setpresetparameter_f(i, kDrymix, 0.);
	setpresetparameter_f(i, kMix1, 1.);
	setpresetparameter_f(i, kMix2, 1.);
	setpresetparameter_i(i, kQuality, kQualityMode_DirtFi);
	setpresetparameter_b(i, kTomsound, false);
	setpresetparameter_b(i, kFreeze, false);
	i++;

	setpresetname(i, "subverb (by Styrofoam)");
	setpresetparameter_f(i, kSpeed1, -2.9232);
	setpresetparameter_f(i, kFeed1, 49.6802);
	setpresetparameter_f(i, kDist1, 0.05756);
	setpresetparameter_f(i, kSpeed2, -2.90405);
	setpresetparameter_f(i, kFeed2, 47.5475);
	setpresetparameter_f(i, kDist2, 0.032);
	setpresetparameter_f(i, kBsize, 2341.3708);
	setpresetparameter_f(i, kDrymix, 0.);
	setpresetparameter_f(i, kMix1, 1.);
	setpresetparameter_f(i, kMix2, 1.);
	setpresetparameter_i(i, kQuality, kQualityMode_UltraHiFi);
	setpresetparameter_b(i, kTomsound, true);
	setpresetparameter_b(i, kFreeze, false);
	i++;

	setpresetname(i, "vocoder beat (by Styrofoam)");
	setpresetparameter_f(i, kSpeed1, 2.);
	setpresetparameter_f(i, kFeed1, 57.3561);
	setpresetparameter_f(i, kDist1, 0.0085);
	setpresetparameter_f(i, kSpeed2, 1.);
	setpresetparameter_f(i, kFeed2, 27.5053);
	setpresetparameter_f(i, kDist2, 0.0021);
	setpresetparameter_f(i, kBsize, 64.9446);
	setpresetparameter_f(i, kDrymix, 0.);
	setpresetparameter_f(i, kMix1, 1.);
	setpresetparameter_f(i, kMix2, 1.);
	setpresetparameter_i(i, kQuality, kQualityMode_DirtFi);
	setpresetparameter_b(i, kTomsound, false);
	setpresetparameter_b(i, kFreeze, false);
	i++;

	setpresetname(i, "yo pitch! (by Styrofoam)");
	setpresetparameter_f(i, kSpeed1, -2.);
	setpresetparameter_f(i, kFeed1, 80.3769);
	setpresetparameter_f(i, kDist1, 0.022362);
	setpresetparameter_f(i, kSpeed2, 2.);
	setpresetparameter_f(i, kFeed2, 29.21106);
	setpresetparameter_f(i, kDist2, 0.010665);
	setpresetparameter_f(i, kBsize, 1938.5203);
	setpresetparameter_f(i, kDrymix, 0.);
	setpresetparameter_f(i, kMix1, 1.);
	setpresetparameter_f(i, kMix2, 0.6483);
	setpresetparameter_i(i, kQuality, kQualityMode_UltraHiFi);
	setpresetparameter_b(i, kTomsound, false);
	setpresetparameter_b(i, kFreeze, false);
	i++;

/*
	setpresetname(i, "");
	setpresetparameter_f(i, kSpeed1, );
	setpresetparameter_f(i, kFeed1, );
	setpresetparameter_f(i, kDist1, 0.0);
	setpresetparameter_f(i, kSpeed2, );
	setpresetparameter_f(i, kFeed2, );
	setpresetparameter_f(i, kDist2, 0.0);
	setpresetparameter_f(i, kBsize, );
	setpresetparameter_f(i, kDrymix, 0.);
	setpresetparameter_f(i, kMix1, 0.);
	setpresetparameter_f(i, kMix2, 0.);
	setpresetparameter_i(i, kQuality, );
	setpresetparameter_b(i, kTomsound, );
	setpresetparameter_b(i, kFreeze, false);
	i++;
*/

	// special randomizing "preset"
	setpresetname(getnumpresets() - 1, "random");
}

//-----------------------------------------------------------------------------
bool Transverb::loadpreset(long index)
{
	if (!presetisvalid(index))
		return false;

	if (getpresetname(index) == "random")
	{
		randomizeparameters();
		return true;
	}

	return DfxPlugin::loadpreset(index);
}

//--------- presets (end) --------



/* this randomizes the values of all of Transverb's parameters, sometimes in smart ways */
void Transverb::randomizeparameters()
{
	// randomize the non-mix-level parameters

	for (long i = 0; i < kDrymix; i++)
	{
		if (hasparameterattribute(i, DfxParam::kAttribute_OmitFromRandomizeAll))
		{
			continue;
		}
		// make slow speeds more probable (for fairer distribution)
		if ((i == kSpeed1) || (i == kSpeed1))
		{
			auto temprand = generateParameterRandomValue<double>(-1., 1.);
			if (temprand < 0.)
			{
				temprand = getparametermin_f(i) * (temprand + 1.);
			}
			else
			{
				temprand = getparametermax_f(i) * temprand;
			}
			setparameter_f(i, temprand);
		}
		// make smaller buffer sizes more probable (because they sound better), though prevent smallest
		else if (i == kBsize)
		{
			setparameter_gen(kBsize, std::pow(generateParameterRandomValue<double>(0.07, 1.), 1.38));
		}
		else
		{
			// TODO: this is not thread-safe (wrt randomizer state) given that Transverb can execute this method
			// from the main thread (GUI) or MIDI thread (though likelihood of concurrency is quite low)
			randomizeparameter(i);
		}
	}


	// do fancy mix level randomization

	// store the current total gain sum
	auto const mixSum = getparameter_f(kDrymix) + getparameter_f(kMix1) + getparameter_f(kMix2);

	// randomize the mix parameters
	auto newDrymix = expandparametervalue(kDrymix, generateParameterRandomValue<double>());
	auto newMix1 = expandparametervalue(kMix1, generateParameterRandomValue<double>());
	auto newMix2 = expandparametervalue(kMix2, generateParameterRandomValue<double>());
	// calculate a scalar to make up for total gain changes
	auto const mixDiffScalar = mixSum / (newDrymix + newMix1 + newMix2);

	// apply the scalar to the new mix parameter values
	newDrymix *= mixDiffScalar;
	newMix1 *= mixDiffScalar;
	newMix2 *= mixDiffScalar;

	// clip the the delay head mix values at unity gain so that we don't get mega-feedback blasts
	newDrymix = std::min(newDrymix, getparametermax_f(kDrymix));
	newMix1 = std::min(newMix1, 1.0);
	newMix2 = std::min(newMix2, 1.0);

	// set the new randomized mix parameter values as the new values
	setparameter_f(kDrymix, newDrymix);
	setparameter_f(kMix1, newMix1);
	setparameter_f(kMix2, newMix2);


	// randomize the state parameters

	// make higher qualities more probable (happen 4/5 of the time)
	setparameter_i(kQuality, generateParameterRandomValue<int64_t>(1, (kQualityMode_NumModes * 2) - 1) % kQualityMode_NumModes);
	// make TOMSOUND less probable (only 1/3 of the time)
	setparameter_b(kTomsound, static_cast<bool>(generateParameterRandomValue<int64_t>(0, 2) % 2));


	for (long i = 0; i < kNumParameters; i++)
	{
		if (hasparameterattribute(i, DfxParam::kAttribute_OmitFromRandomizeAll))
		{
			continue;
		}
		postupdate_parameter(i);  // inform any parameter listeners of the changes
	}
}

long Transverb::dfx_GetPropertyInfo(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned long inItemIndex, 
                                    size_t& outDataSize, dfx::PropertyFlags& outFlags)
{
  if (isSpeedModePropertyID(inPropertyID))
  {
    outDataSize = sizeof(uint8_t);  // single-byte representation allows for no concerns about transmission endianness
    outFlags = dfx::kPropertyFlag_Readable | dfx::kPropertyFlag_Writable;
    return dfx::kStatus_NoError;
  }
  return DfxPlugin::dfx_GetPropertyInfo(inPropertyID, inScope, inItemIndex, outDataSize, outFlags);
}

long Transverb::dfx_GetProperty(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned long inItemIndex, 
                                void* outData)
{
  if (isSpeedModePropertyID(inPropertyID))
  {
    *static_cast<uint8_t*>(outData) = static_cast<uint8_t>(speedModeStateFromPropertyID(inPropertyID));
    return dfx::kStatus_NoError;
  }
  return DfxPlugin::dfx_GetProperty(inPropertyID, inScope, inItemIndex, outData);
}

long Transverb::dfx_SetProperty(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned long inItemIndex, 
                                void const* inData, size_t inDataSize)
{
  if (isSpeedModePropertyID(inPropertyID))
  {
    speedModeStateFromPropertyID(inPropertyID) = *static_cast<uint8_t const*>(inData);
#ifdef TARGET_API_AUDIOUNIT
    PropertyChanged(inPropertyID, inScope, inItemIndex);
#endif
    return dfx::kStatus_NoError;
  }
  return DfxPlugin::dfx_SetProperty(inPropertyID, inScope, inItemIndex, inData, inDataSize);
}


size_t Transverb::settings_sizeOfExtendedData() const noexcept
{
	return speedModeStates.size() * sizeof(speedModeStates.front());
}

void Transverb::settings_saveExtendedData(void* outData, bool /*isPreset*/)
{
  auto speedModeStatesSerialization = speedModeStates;
  if constexpr (!DfxSettings::serializationIsNativeEndian())
  {
    dfx::ReverseBytes(speedModeStatesSerialization.data(), speedModeStatesSerialization.size());
  }
  memcpy(outData, speedModeStatesSerialization.data(), settings_sizeOfExtendedData());
}

void Transverb::settings_restoreExtendedData(void const* inData, size_t storedExtendedDataSize, 
                                             long dataVersion, bool /*isPreset*/)
{
  if (storedExtendedDataSize >= settings_sizeOfExtendedData())
  {
    memcpy(speedModeStates.data(), inData, settings_sizeOfExtendedData());
    if constexpr (!DfxSettings::serializationIsNativeEndian())
    {
      dfx::ReverseBytes(speedModeStates.data(), speedModeStates.size());
    }
  }
}

void Transverb::settings_doChunkRestoreSetParameterStuff(long tag, float value, long dataVersion, long presetNum)
{
	// prevent old speed mode 1 settings from applying to freeze
	if ((dataVersion < 0x00010501) && (tag == kFreeze))
	{
		auto const defaultValue = getparameterdefault_b(tag);
		if (presetisvalid(presetNum))
		{
			setpresetparameter_b(presetNum, tag, defaultValue);
		}
		else
		{
			setparameter_b(tag, defaultValue);
		}
	}
}



#pragma mark -

dfx::PropertyID dfx::TV::speedModeIndexToPropertyID(size_t inIndex) noexcept
{
  return kTransverbProperty_SpeedModeBase + static_cast<dfx::PropertyID>(inIndex);
}

size_t dfx::TV::speedModePropertyIDToIndex(dfx::PropertyID inPropertyID) noexcept
{
  assert(inPropertyID >= kTransverbProperty_SpeedModeBase);
  return inPropertyID - kTransverbProperty_SpeedModeBase;
}

bool dfx::TV::isSpeedModePropertyID(dfx::PropertyID inPropertyID) noexcept
{
  return (inPropertyID >= kTransverbProperty_SpeedModeBase) && (speedModePropertyIDToIndex(inPropertyID) < kNumDelays);
}
