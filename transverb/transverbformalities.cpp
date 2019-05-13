/*------------------------------------------------------------------------
Copyright (C) 2001-2019  Tom Murphy 7 and Sophia Poirier

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

#include "transverb.h"

#include <algorithm>
#include <cmath>



// these are macros that do boring entry point stuff for us
DFX_EFFECT_ENTRY(Transverb)
#if TARGET_PLUGIN_USES_DSPCORE
  DFX_CORE_ENTRY(TransverbDSP)
#endif



Transverb::Transverb(TARGET_API_BASE_INSTANCE_TYPE inInstance)
  : DfxPlugin(inInstance, kNumParameters, kNumPresets) {

  initparameter_f(kBsize, "buffer size", 2700.0, 333.0, 1.0, 3000.0, DfxParam::Unit::MS);
  initparameter_f(kDrymix, "dry mix", 1.0, 1.0, 0.0, 1.0, DfxParam::Unit::LinearGain, DfxParam::Curve::Squared);
  initparameter_f(kMix1, "1:mix", 1.0, 1.0, 0.0, 1.0, DfxParam::Unit::LinearGain, DfxParam::Curve::Squared);
  initparameter_f(kDist1, "1:dist", 0.90009, 0.5, 0.0, 1.0, DfxParam::Unit::Scalar);
  initparameter_f(kSpeed1, "1:speed", 0.0, 0.0, -3.0, 6.0, DfxParam::Unit::Octaves);
  initparameter_f(kFeed1, "1:feedback", 0.0, 33.3, 0.0, 100.0, DfxParam::Unit::Percent);
  initparameter_f(kMix2, "2:mix", 0.0, 1.0, 0.0, 1.0, DfxParam::Unit::LinearGain, DfxParam::Curve::Squared);
  initparameter_f(kDist2, "2:dist", 0.1, 0.5, 0.0, 1.0, DfxParam::Unit::Scalar);
  initparameter_f(kSpeed2, "2:speed", 1.0, 0.0, -3.0, 6.0, DfxParam::Unit::Octaves);
  initparameter_f(kFeed2, "2:feedback", 0.0, 33.3, 0.0, 100.0, DfxParam::Unit::Percent);
  initparameter_list(kQuality, "quality", kQualityMode_UltraHiFi, kQualityMode_UltraHiFi, kQualityMode_NumModes);
  initparameter_b(kTomsound, "TOMSOUND", false, false);
  initparameter_list(kSpeed1mode, "1:speed mode", kSpeedMode_Fine, kSpeedMode_Fine, kSpeedMode_NumModes);
  initparameter_list(kSpeed2mode, "2:speed mode", kSpeedMode_Fine, kSpeedMode_Fine, kSpeedMode_NumModes);

  setparametervaluestring(kQuality, kQualityMode_DirtFi, "dirt-fi");
  setparametervaluestring(kQuality, kQualityMode_HiFi, "hi-fi");
  setparametervaluestring(kQuality, kQualityMode_UltraHiFi, "ultra hi-fi");
  for (int i=kSpeed1mode; i <= kSpeed2mode; i += kSpeed2mode-kSpeed1mode)
  {
    setparametervaluestring(i, kSpeedMode_Fine, "fine");
    setparametervaluestring(i, kSpeedMode_Semitone, "semitone");
    setparametervaluestring(i, kSpeedMode_Octave, "octave");
  }

  // these are only for the GUI, no need to reveal them to the user as parameters
  addparameterattributes(kSpeed1mode, DfxParam::kAttribute_Hidden);
  addparameterattributes(kSpeed2mode, DfxParam::kAttribute_Hidden);


  settailsize_seconds(getparametermax_f(kBsize) * 0.001);

  setpresetname(0, PLUGIN_NAME_STRING);  // default preset name
  initPresets();


#if TARGET_PLUGIN_USES_DSPCORE
  DFX_INIT_CORE(TransverbDSP);
#endif
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
  : DfxPluginCore(inDfxPlugin) {

  firCoefficients1.assign(kNumFIRTaps, 0.0f);
  firCoefficients2.assign(kNumFIRTaps, 0.0f);

  registerSmoothedAudioValue(&drymix);
  registerSmoothedAudioValue(&mix1);
  registerSmoothedAudioValue(&feed1);
  registerSmoothedAudioValue(&mix2);
  registerSmoothedAudioValue(&feed2);
}

void TransverbDSP::reset() {

  writer = 0;
  read1 = read2 = 0.0;
  smoothcount1 = smoothcount2 = 0;
  lastr1val = lastr2val = 0.0f;
  filter1.reset();
  filter2.reset();
  speed1hasChanged = speed2hasChanged = true;
  tomsound_sampoffset = GetChannelNum();

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

  buf1.clear();
  buf2.clear();
}


void TransverbDSP::processparameters() {

  if (auto const value = getparameterifchanged_f(kDrymix))
    drymix = *value;
  bsize = std::clamp((int) (getparameter_f(kBsize) * getsamplerate() * 0.001), 1, MAXBUF);
  if (auto const value = getparameterifchanged_f(kMix1))
    mix1 = *value;
  speed1 = std::pow(2.0, getparameter_f(kSpeed1));
  if (auto const value = getparameterifchanged_scalar(kFeed1))
    feed1 = *value;
  dist1 = getparameter_f(kDist1);
  if (auto const value = getparameterifchanged_f(kMix2))
    mix2 = *value;
  speed2 = std::pow(2.0, getparameter_f(kSpeed2));
  if (auto const value = getparameterifchanged_scalar(kFeed2))
    feed2 = *value;
  dist2 = getparameter_f(kDist2);
  quality = getparameter_i(kQuality);
  tomsound = getparameter_b(kTomsound);

  if (getparameterchanged(kBsize))
  {
    writer %= bsize;
    read1 = std::fmod(std::fabs(read1), (double)bsize);
    read2 = std::fmod(std::fabs(read2), (double)bsize);
  }

  if (getparameterchanged(kDist1))
    read1 = std::fmod(std::fabs((double)writer + (double)dist1 * (double)MAXBUF), (double)bsize);
  if (getparameterchanged(kSpeed1))
    speed1hasChanged = true;

  if (getparameterchanged(kDist2))
    read2 = std::fmod(std::fabs((double)writer + (double)dist2 * (double)MAXBUF), (double)bsize);
  if (getparameterchanged(kSpeed2))
    speed2hasChanged = true;

  if (getparameterchanged(kQuality) || getparameterchanged(kTomsound))
    speed1hasChanged = speed2hasChanged = true;

  // XXX is this necessary to get "true" TOMSOUND?
  if (getparameterchanged(kTomsound))
  {
    // if TOMSOUND was just activated, set up the channel offset error
    if (tomsound)
    {
      writer += tomsound_sampoffset;
      writer %= bsize;
      read1 += speed1 * (double)tomsound_sampoffset;
      read2 += speed2 * (double)tomsound_sampoffset;
      read1 = std::fmod(std::fabs(read1), (double)bsize);
      read2 = std::fmod(std::fabs(read2), (double)bsize);
    }
    // otherwise remove the channel offset error (unless everything's initialized)
    else if ((writer != 0) || (read1 != 0.0) || (read2 != 0.0))
    {
      writer -= tomsound_sampoffset;
      writer = (writer+bsize) % bsize;
      read1 -= speed1 * (double)tomsound_sampoffset;
      read2 -= speed2 * (double)tomsound_sampoffset;
      read1 = std::fmod(std::fabs(read1), (double)bsize);
      read2 = std::fmod(std::fabs(read2), (double)bsize);
    }
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
	setpresetparameter_f(i, kSpeed1, 0.048406605/12.0);
	setpresetparameter_f(i, kFeed1, 67.0);
	setpresetparameter_f(i, kMix2, 0.0);
	setpresetparameter_f(i, kDist2, 0.0);
	setpresetparameter_f(i, kSpeed2, getparametermin_f(kSpeed2));
	setpresetparameter_f(i, kFeed2, 0.0);
	setpresetparameter_i(i, kQuality, kQualityMode_UltraHiFi);
	setpresetparameter_b(i, kTomsound, false);
	i++;

	setpresetname(i, "phaser down");
	setpresetparameter_f(i, kBsize, 27.0);
	setpresetparameter_f(i, kDrymix, 0.45);
	setpresetparameter_f(i, kMix1, 0.5);
	setpresetparameter_f(i, kDist1, 0.0);
	setpresetparameter_f(i, kSpeed1, -0.12/12.0);//-0.048542333f/12.0);
	setpresetparameter_f(i, kFeed1, 76.0);
	setpresetparameter_f(i, kMix2, 0.0);
	setpresetparameter_f(i, kDist2, 0.0);
	setpresetparameter_f(i, kSpeed2, getparametermin_f(kSpeed2));
	setpresetparameter_f(i, kFeed2, 0.0);
	setpresetparameter_i(i, kQuality, kQualityMode_UltraHiFi);
	setpresetparameter_b(i, kTomsound, false);
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
	i++;

	setpresetname(i, "glup drums");
	setpresetparameter_f(i, kBsize, 184.27356);
	setpresetparameter_f(i, kDrymix, 0.157);
	setpresetparameter_f(i, kMix1, 1.0);
	setpresetparameter_f(i, kDist1, 0.0945);
	setpresetparameter_f(i, kSpeed1, -8.0/12.0);
	setpresetparameter_f(i, kFeed1, 97.6);
	setpresetparameter_f(i, kMix2, 0.0);
	setpresetparameter_f(i, kDist2, 0.197);
	setpresetparameter_f(i, kSpeed2, 0.978195651);
	setpresetparameter_f(i, kFeed2, 0.0);
	setpresetparameter_i(i, kQuality, kQualityMode_UltraHiFi);
	setpresetparameter_b(i, kTomsound, false);
	i++;

	setpresetname(i, "space invaders");
	setpresetparameter_f(i, kSpeed1, -0.23/12.0);
	setpresetparameter_f(i, kFeed1, 73.0);
	setpresetparameter_f(i, kDist1, 0.1857);
	setpresetparameter_f(i, kSpeed2, 4.3/12.0);
	setpresetparameter_f(i, kFeed2, 41.0);
	setpresetparameter_f(i, kDist2, 0.5994);
	setpresetparameter_f(i, kBsize, 16.8);
	setpresetparameter_f(i, kDrymix, 0.000576);
	setpresetparameter_f(i, kMix1, 1.0);
	setpresetparameter_f(i, kMix2, 0.1225);
	setpresetparameter_i(i, kQuality, kQualityMode_UltraHiFi);
	setpresetparameter_b(i, kTomsound, false);
	i++;

/*
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
	setpresetname(i, "");
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

	if (strcmp(getpresetname_ptr(index), "random") == 0)
	{
		randomizeparameters(false);
		return true;
	}

	return DfxPlugin::loadpreset(index);
}

//--------- presets (end) --------



/* this randomizes the values of all of Transverb's parameters, sometimes in smart ways */
void Transverb::randomizeparameters(bool writeAutomation)
{
// randomize the first 7 parameters

	for (long i = 0; i < kDrymix; i++)
	{
		// make slow speeds more probable (for fairer distribution)
		if ((i == kSpeed1) || (i == kSpeed1))
		{
			auto temprand = dfx::math::Rand<double>();
			if (temprand < 0.5)
				temprand = getparametermin_f(i) * temprand * 2.0;
			else
				temprand = getparametermax_f(i) * ((temprand - 0.5) * 2.0);
			setparameter_f(i, temprand);
		}
		// make smaller buffer sizes more probable (because they sound better)
		else if (i == kBsize)
			setparameter_gen(kBsize, std::pow((dfx::math::Rand<float>() * 0.93f) + 0.07f, 1.38f));
		else
			randomizeparameter(i);
	}


// do fancy mix level randomization

	// store the current total gain sum
	auto const mixSum = getparameter_f(kDrymix) + getparameter_f(kMix1) + getparameter_f(kMix2);

	// randomize the mix parameters
	auto newDrymix = dfx::math::Rand<float>();
	auto newMix1 = dfx::math::Rand<float>();
	auto newMix2 = dfx::math::Rand<float>();
	// square them all for squared gain scaling
	newDrymix *= newDrymix;
	newMix1 *= newMix1;
	newMix2 *= newMix2;
	// calculate a scalar to make up for total gain changes
	float const mixScalar = mixSum / (newDrymix + newMix1 + newMix2);

	// apply the scalar to the new mix parameter values
	newDrymix *= mixScalar;
	newMix1 *= mixScalar;
	newMix2 *= mixScalar;

	// clip the the mix values at 1.0 so that we don't get mega-feedback blasts
	newDrymix = std::min(newDrymix, 1.0f);
	newMix1 = std::min(newMix1, 1.0f);
	newMix2 = std::min(newMix2, 1.0f);

	// set the new randomized mix parameter values as the new values
	setparameter_f(kDrymix, newDrymix);
	setparameter_f(kMix1, newMix1);
	setparameter_f(kMix2, newMix2);


// randomize the state parameters

	// make higher qualities more probable (happen 4/5 of the time)
	setparameter_i(kQuality, ((rand() % 5) + 1) % 3);
	// make TOMSOUND less probable (only 1/3 of the time)
	setparameter_b(kTomsound, (bool)((rand() % 3) % 2));


	for (long i = 0; i < kSpeed1mode; i++)
	{
		postupdate_parameter(i);  // inform any parameter listeners of the changes
#ifdef TARGET_API_VST
		if (writeAutomation)
		{
			setParameterAutomated(i, getparameter_gen(i));
		}
#endif
	}
}
