/*------------------------------------------------------------------------
Copyright (C) 2002-2022  Sophia Poirier

This file is part of Scrubby.

Scrubby is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Scrubby is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Scrubby.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#include "scrubby.h"

#include <algorithm>
#include <array>
#include <cmath>

#include "dfxmisc.h"


#pragma mark init

// this macro does boring entry point stuff for us
DFX_EFFECT_ENTRY(Scrubby)

//-----------------------------------------------------------------------------
// initializations and such
Scrubby::Scrubby(TARGET_API_BASE_INSTANCE_TYPE inInstance)
:	DfxPlugin(inInstance, kNumParameters, kNumPresets)
{
	// initialize the parameters
	auto const numTempoRates = mTempoRateTable.getNumRates();
	auto const unitTempoRateIndex = mTempoRateTable.getNearestTempoRateIndex(1.);
	initparameter_f(kSeekRange, {"seek range", "SeekRng", "SekRng", "SkRg"}, 333.0, 333.0, 0.3, 6000.0, DfxParam::Unit::MS, DfxParam::Curve::Squared);
	initparameter_b(kFreeze, dfx::MakeParameterNames(dfx::kParameterNames_Freeze), false);
	initparameter_f(kSeekRate_Hz, {"seek rate (free)", "SekRtFr", "SkRtFr", "SkRF"}, 9.0, 3.0, 0.3, 810.0, DfxParam::Unit::Hz, DfxParam::Curve::Log);//DfxParam::Curve::Cubed
	initparameter_list(kSeekRate_Sync, {"seek rate (sync)", "SekRtSc", "SkRtSc", "SkRS"}, unitTempoRateIndex, unitTempoRateIndex, numTempoRates, DfxParam::Unit::Beats);
	initparameter_f(kSeekRateRandMin_Hz, {"seek rate rand min (free)", "SkRtMnF", "SkRtMF", "SRMF"}, 9.0, 3.0, 0.3, 810.0, DfxParam::Unit::Hz, DfxParam::Curve::Log);//DfxParam::Curve::Cubed
	initparameter_list(kSeekRateRandMin_Sync, {"seek rate rand min (sync)", "SkRtMnS", "SkRtMS", "SRMS"}, unitTempoRateIndex, unitTempoRateIndex, numTempoRates, DfxParam::Unit::Beats);
	initparameter_b(kTempoSync, dfx::MakeParameterNames(dfx::kParameterNames_TempoSync), false);
	initparameter_f(kSeekDur, {"seek duration", "SeekDur", "SekDur", "SkDr"}, 100.0, 100.0, 3.0, 100.0, DfxParam::Unit::Percent);  // percent of range
	initparameter_f(kSeekDurRandMin, {"seek dur rand min", "SekDrMn", "SekDrM", "SkDM"}, 100.0, 100.0, 3.0, 100.0, DfxParam::Unit::Percent);  // percent of range
	initparameter_list(kSpeedMode, {"speeds", "Spee"}, kSpeedMode_Robot, kSpeedMode_Robot, kNumSpeedModes);
	initparameter_b(kSplitChannels, {"channels split", "Channel", "Chanel", "Chan"}, false);
	initparameter_b(kPitchConstraint, {"pitch constraint", "PtchCon", "PtchCn", "Ptch"}, false);
	// default all notes to off (looks better on the GUI)
	// no, I changed my mind, at least leave one note on so that the user isn't
	// confused the first time turning on pitch constraint and getting silence
	initparameter_b(kPitchStep0, {"semi0 (unity/octave)"}, true, false);
	initparameter_b(kPitchStep1, {"semi1 (minor 2nd)"}, false);
	initparameter_b(kPitchStep2, {"semi2 (major 2nd)"}, false);
	initparameter_b(kPitchStep3, {"semi3 (minor 3rd)"}, false);
	initparameter_b(kPitchStep4, {"semi4 (major 3rd)"}, false);
	initparameter_b(kPitchStep5, {"semi5 (perfect 4th)"}, false);
	initparameter_b(kPitchStep6, {"semi6 (augmented 4th)"}, false);
	initparameter_b(kPitchStep7, {"semi7 (perfect 5th)"}, false);
	initparameter_b(kPitchStep8, {"semi8 (minor 6th)"}, false);
	initparameter_b(kPitchStep9, {"semi9 (major 6th)"}, false);
	initparameter_b(kPitchStep10, {"semi10 (minor 7th)"}, false);
	initparameter_b(kPitchStep11, {"semi11 (major 7th)"}, false);
	initparameter_i(kOctaveMin, {"octave minimum", "OctvMin", "OctvMn", "8va["}, kOctave_MinValue, kOctave_MinValue, kOctave_MinValue, 0, DfxParam::Unit::Octaves);
	setparameterusevaluestrings(kOctaveMin, true);
	initparameter_i(kOctaveMax, {"octave maximum", "OctvMax", "OctvMx", "8va]"}, kOctave_MaxValue, kOctave_MaxValue, 0, kOctave_MaxValue, DfxParam::Unit::Octaves);
	setparameterusevaluestrings(kOctaveMax, true);
	initparameter_f(kTempo, dfx::MakeParameterNames(dfx::kParameterNames_Tempo), 120.0, 120.0, 39.0, 480.0, DfxParam::Unit::BPM);
	initparameter_b(kTempoAuto, dfx::MakeParameterNames(dfx::kParameterNames_TempoAuto), true);
	initparameter_f(kPredelay, {"predelay", "PreDela", "PreDel", "PDel"}, 0.0, 50.0, 0.0, 100.0, DfxParam::Unit::Percent);  // percent of range
	initparameter_f(kDryLevel, {"dry level", "DryLevl", "DryLvl", "Dry"}, 0., 0.5, 0., 1., DfxParam::Unit::LinearGain, DfxParam::Curve::Squared);
	initparameter_f(kWetLevel, {"wet level", "WetLevl", "WetLvl", "Wet"}, 1., 0.5, 0., 1., DfxParam::Unit::LinearGain, DfxParam::Curve::Squared);

	setparameterenforcevaluelimits(kPredelay, true);

	// set the value strings for the sync rate parameters
	for (size_t i = 0; i < numTempoRates; i++)
	{
		auto const& tempoRateName = mTempoRateTable.getDisplay(i);
		setparametervaluestring(kSeekRate_Sync, i, tempoRateName);
		setparametervaluestring(kSeekRateRandMin_Sync, i, tempoRateName);
	}
	// set the value strings for the speed modes
	setparametervaluestring(kSpeedMode, kSpeedMode_Robot, "robot");
	setparametervaluestring(kSpeedMode, kSpeedMode_DJ, "DJ");
	// set the value strings for the octave range parameters
	setparametervaluestring(kOctaveMin, getparametermin_i(kOctaveMin), "no min");
	for (long i = getparametermin_i(kOctaveMin) + 1; i <= getparametermax_i(kOctaveMin); i++)
	{
		setparametervaluestring(kOctaveMin, i, std::to_string(i));
	}
	for (long i = getparametermin_i(kOctaveMax); i < getparametermax_i(kOctaveMax); i++)
	{
		auto const prefix = (i > 0) ? "+" : "";
		auto const octaveName = prefix + std::to_string(i);
		setparametervaluestring(kOctaveMax, i, octaveName);
	}
	setparametervaluestring(kOctaveMax, getparametermax_i(kOctaveMax), "no max");

	addparameterattributes(kFreeze, DfxParam::kAttribute_OmitFromRandomizeAll);
	addparameterattributes(kPredelay, DfxParam::kAttribute_OmitFromRandomizeAll);
	// the mix level parameters are "omitted" only in the sense that we manually handle their randomization
	addparameterattributes(kDryLevel, DfxParam::kAttribute_OmitFromRandomizeAll);
	addparameterattributes(kWetLevel, DfxParam::kAttribute_OmitFromRandomizeAll);

	addparametergroup("pitch control", {kPitchConstraint, kPitchStep0, kPitchStep1, kPitchStep2, kPitchStep3, kPitchStep4, kPitchStep5, kPitchStep6, kPitchStep7, kPitchStep8, kPitchStep9, kPitchStep10, kPitchStep11});


	settailsize_seconds(getparametermax_f(kSeekRange) * 0.001 / getparametermin_f(kSeekRate_Hz));

	setpresetname(0, "scub");  // default preset name
	initPresets();

	addchannelconfig(kChannelConfig_AnyMatchedIO);  // N-in/N-out
	addchannelconfig(1, kChannelConfigCount_Any);  // 1-in/N-out

	registerSmoothedAudioValue(mInputGain);
	registerSmoothedAudioValue(mOutputGain);

	mCurrentTempoBPS = getparameter_f(kTempo) / 60.0;
}

//-------------------------------------------------------------------------
void Scrubby::dfx_PostConstructor()
{
	// since we don't use pitchbend for anything special,
	// allow it be assigned to control parameters
	getsettings().setAllowPitchbendEvents(true);
	// can't load old VST-style settings
	getsettings().setLowestLoadableVersion(0x00010100);
}

//-------------------------------------------------------------------------
void Scrubby::initialize()
{
	// the number of samples in the maximum seek range,
	// dividing by the minimum seek rate for extra leeway while moving
	mMaxAudioBufferSize = std::lround(getparametermax_f(kSeekRange) * 0.001 * getsamplerate_f() / getparametermin_f(kSeekRate_Hz));
	mMaxAudioBufferSize_f = static_cast<double>(mMaxAudioBufferSize);
	auto const numChannels = getnumoutputs();

	mAudioBuffers.assign(numChannels, {});
	for (auto& buffer : mAudioBuffers)
	{
		buffer.assign(mMaxAudioBufferSize, 0.0f);
	}
	mReadPos.assign(numChannels, 0.0);
	mReadStep.assign(numChannels, 0.0);
	mPortamentoStep.assign(numChannels, 0.0);
	mMoveCount.assign(numChannels, 0);
	mSeekCount.assign(numChannels, 0);
	mNeedResync.assign(numChannels, false);

	mHighpassFilters.assign(numChannels, {});
	std::for_each(mHighpassFilters.begin(), mHighpassFilters.end(), [this](auto& filter)
	{
		filter.setSampleRate(getsamplerate());
		filter.setHighpassCoefficients(kHighpassFilterCutoff);
	});

	setlatency_seconds((getparameter_f(kSeekRange) * 0.001) * getparameter_scalar(kPredelay));
}

//-----------------------------------------------------------------------------
void Scrubby::cleanup()
{
	mAudioBuffers = {};
	mReadPos = {};
	mReadStep = {};
	mPortamentoStep = {};
	mMoveCount = {};
	mSeekCount = {};
	mNeedResync = {};
	mHighpassFilters = {};
}

//-------------------------------------------------------------------------
void Scrubby::reset()
{
	// clear out the buffers
	for (auto& buffer : mAudioBuffers)
	{
		std::fill(buffer.begin(), buffer.end(), 0.0f);
	}
	std::fill(mReadPos.begin(), mReadPos.end(), 0.001);
	std::fill(mReadStep.begin(), mReadStep.end(), 1.0);
#if USE_LINEAR_ACCELERATION
	std::fill(mPortamentoStep.begin(), mPortamentoStep.end(), 0.0);
#else
	std::fill(mPortamentoStep.begin(), mPortamentoStep.end(), 1.0);
#endif
	std::fill(mMoveCount.begin(), mMoveCount.end(), 0);
	std::fill(mSeekCount.begin(), mSeekCount.end(), 0);
	// some hosts may call reset when restarting playback
	std::fill(mNeedResync.begin(), mNeedResync.end(), true);

	std::for_each(mHighpassFilters.begin(), mHighpassFilters.end(), [](auto& filter){ filter.reset(); });

	// reset the position tracker
	mWritePos = 0;

	// delete any stored active notes
	for (size_t i = 0; i < mActiveNotesTable.size(); i++)
	{
		if (std::exchange(mActiveNotesTable[i], 0) > 0)
		{
			setparameterquietly_b(static_cast<dfx::ParameterID>(i) + kPitchStep0, false);
			postupdate_parameter(static_cast<dfx::ParameterID>(i) + kPitchStep0);
		}
	}

mSineCount = 0;
}


#pragma mark -
#pragma mark presets

//-------------------------------------------------------------------------
void Scrubby::initPresets()
{
	size_t i = 1;

	setpresetname(i, "happy machine");
	setpresetparameter_f(i, kSeekRange, 603.0);
	setpresetparameter_b(i, kFreeze, false);
	setpresetparameter_f(i, kSeekRate_Hz, 11.547);
	setpresetparameter_f(i, kSeekRateRandMin_Hz, 11.547);
	setpresetparameter_b(i, kTempoSync, false);
	setpresetparameter_f(i, kSeekDur, 40.8);
	setpresetparameter_f(i, kSeekDurRandMin, 40.8);
	setpresetparameter_i(i, kSpeedMode, kSpeedMode_Robot);
	setpresetparameter_b(i, kSplitChannels, false);
	setpresetparameter_b(i, kPitchConstraint, true);
	setpresetparameter_i(i, kOctaveMin, getparametermin_i(kOctaveMin));
	setpresetparameter_i(i, kOctaveMax, 1);
	setpresetparameter_b(i, kPitchStep0, true);
	setpresetparameter_b(i, kPitchStep1, false);
	setpresetparameter_b(i, kPitchStep2, false);
	setpresetparameter_b(i, kPitchStep3, false);
	setpresetparameter_b(i, kPitchStep4, false);
	setpresetparameter_b(i, kPitchStep5, false);
	setpresetparameter_b(i, kPitchStep6, false);
	setpresetparameter_b(i, kPitchStep7, true);
	setpresetparameter_b(i, kPitchStep8, false);
	setpresetparameter_b(i, kPitchStep9, false);
	setpresetparameter_b(i, kPitchStep10, false);
	setpresetparameter_b(i, kPitchStep11, false);
	setpresetparameter_f(i, kDryLevel, 0.);
	setpresetparameter_f(i, kWetLevel, 1.);
	i++;

	setpresetname(i, "fake chorus");
	setpresetparameter_f(i, kSeekRange, 3.0);
	setpresetparameter_b(i, kFreeze, false);
	setpresetparameter_f(i, kSeekRate_Hz, 18.0);
	setpresetparameter_f(i, kSeekRateRandMin_Hz, 18.0);
	setpresetparameter_b(i, kTempoSync, false);
	setpresetparameter_f(i, kSeekDur, 100.0);
	setpresetparameter_f(i, kSeekDurRandMin, 100.0);
	setpresetparameter_i(i, kSpeedMode, kSpeedMode_Robot);
	setpresetparameter_b(i, kSplitChannels, true);
	setpresetparameter_b(i, kPitchConstraint, false);
	setpresetparameter_f(i, kDryLevel, 0.);
	setpresetparameter_f(i, kWetLevel, 1.);
	i++;

	setpresetname(i, "broken turntable");
	setpresetparameter_f(i, kSeekRange, 3000.0);
	setpresetparameter_b(i, kFreeze, false);
	setpresetparameter_f(i, kSeekRate_Hz, 6.0);
	setpresetparameter_f(i, kSeekRateRandMin_Hz, 6.0);
	setpresetparameter_b(i, kTempoSync, false);
	setpresetparameter_f(i, kSeekDur, 100.0);
	setpresetparameter_f(i, kSeekDurRandMin, 100.0);
	setpresetparameter_i(i, kSpeedMode, kSpeedMode_DJ);
	setpresetparameter_b(i, kSplitChannels, false);
	setpresetparameter_b(i, kPitchConstraint, false);
	setpresetparameter_f(i, kDryLevel, 0.);
	setpresetparameter_f(i, kWetLevel, 1.);
	i++;

	setpresetname(i, "blib");
	setpresetparameter_f(i, kSeekRange, 270.0);
	setpresetparameter_b(i, kFreeze, false);
	setpresetparameter_f(i, kSeekRate_Hz, 420.0);
	setpresetparameter_f(i, kSeekRateRandMin_Hz, 7.2);
	setpresetparameter_b(i, kTempoSync, false);
	setpresetparameter_f(i, kSeekDur, 57.0);
	setpresetparameter_f(i, kSeekDurRandMin, 30.0);
	setpresetparameter_i(i, kSpeedMode, kSpeedMode_Robot);
	setpresetparameter_b(i, kSplitChannels, false);
	setpresetparameter_b(i, kPitchConstraint, false);
	setpresetparameter_f(i, kDryLevel, 0.);
	setpresetparameter_f(i, kWetLevel, 1.);
	i++;

	setpresetname(i, "DJ staccato");
	setpresetparameter_f(i, kSeekRange, 1800.0);
	setpresetparameter_b(i, kFreeze, false);
	setpresetparameter_i(i, kSeekRate_Sync, mTempoRateTable.getNearestTempoRateIndex(2.));
	setpresetparameter_i(i, kSeekRateRandMin_Sync, mTempoRateTable.getNearestTempoRateIndex(2.));
	setpresetparameter_b(i, kTempoSync, true);
	setpresetparameter_f(i, kSeekDur, 22.2);
	setpresetparameter_f(i, kSeekDurRandMin, 22.2);
	setpresetparameter_i(i, kSpeedMode, kSpeedMode_DJ);
	setpresetparameter_b(i, kSplitChannels, false);
	setpresetparameter_b(i, kPitchConstraint, false);
	setpresetparameter_b(i, kTempoAuto, true);
	setpresetparameter_f(i, kDryLevel, 0.);
	setpresetparameter_f(i, kWetLevel, 1.);
	i++;

/*
	setpresetname(i, "");
	setpresetparameter_f(i, kSeekRange, );
	setpresetparameter_b(i, kFreeze, );
	setpresetparameter_f(i, kSeekRate_Hz, );
	setpresetparameter_i(i, kSeekRate_Sync, mTempoRateTable.getNearestTempoRateIndex());
	setpresetparameter_f(i, kSeekRateRandMin_Hz, );
	setpresetparameter_i(i, kSeekRateRandMin_Sync, mTempoRateTable.getNearestTempoRateIndex());
	setpresetparameter_b(i, kTempoSync, );
	setpresetparameter_f(i, kSeekDur, );
	setpresetparameter_f(i, kSeekDurRandMin, );
	setpresetparameter_i(i, kSpeedMode, kSpeedMode_);
	setpresetparameter_b(i, kSplitChannels, );
	setpresetparameter_b(i, kPitchConstraint, );
	setpresetparameter_i(i, kOctaveMin, );
	setpresetparameter_i(i, kOctaveMax, );
	setpresetparameter_f(i, kTempo, );
	setpresetparameter_b(i, kTempoAuto, );
	setpresetparameter_f(i, kPredelay, );
	setpresetparameter_b(i, kPitchStep0, );
	setpresetparameter_b(i, kPitchStep1, );
	setpresetparameter_b(i, kPitchStep2, );
	setpresetparameter_b(i, kPitchStep3, );
	setpresetparameter_b(i, kPitchStep4, );
	setpresetparameter_b(i, kPitchStep5, );
	setpresetparameter_b(i, kPitchStep6, );
	setpresetparameter_b(i, kPitchStep7, );
	setpresetparameter_b(i, kPitchStep8, );
	setpresetparameter_b(i, kPitchStep9, );
	setpresetparameter_b(i, kPitchStep10, );
	setpresetparameter_b(i, kPitchStep11, );
	setpresetparameter_f(i, kDryLevel, );
	setpresetparameter_f(i, kWetLevel, );
	i++;
*/
}


#pragma mark -
#pragma mark parameters

//-------------------------------------------------------------------------
void Scrubby::randomizeparameters()
{
	// store the current total mix gain sum
	auto const entryDryLevel = getparameter_f(kDryLevel);
	auto const entryWetLevel = getparameter_f(kWetLevel);
	auto const entryMixSum = entryDryLevel + entryWetLevel;

	DfxPlugin::randomizeparameters();

	// randomize the mix parameters
	auto newDryLevel = expandparametervalue(kDryLevel, generateParameterRandomValue<double>());
	auto newWetLevel = expandparametervalue(kWetLevel, generateParameterRandomValue<double>());
	auto const newMixSum = newDryLevel + newWetLevel;
	if (newMixSum > 0.)
	{
		// calculate a scalar to make up for total gain changes
		auto const mixDiffScalar = entryMixSum / newMixSum;
		
		// apply the scalar to the new mix parameter values
		newDryLevel = std::min(newDryLevel * mixDiffScalar, getparametermax_f(kDryLevel));
		newWetLevel = std::min(newWetLevel * mixDiffScalar, getparametermax_f(kWetLevel));
	}
	else
	{
		newDryLevel = entryDryLevel;
		newWetLevel = entryWetLevel;
	}

	// set the new randomized mix parameter values as the new values
	setparameter_f(kDryLevel, newDryLevel);
	setparameter_f(kWetLevel, newWetLevel);
	postupdate_parameter(kDryLevel);
	postupdate_parameter(kWetLevel);
}

//-------------------------------------------------------------------------
void Scrubby::processparameters()
{
	mSeekRangeSeconds = getparameter_f(kSeekRange) * 0.001;
	mFreeze = getparameter_b(kFreeze);
	mSeekRateHz = getparameter_f(kSeekRate_Hz);
	mSeekRateIndex = getparameter_index(kSeekRate_Sync);
	mSeekRateSync = mTempoRateTable.getScalar(mSeekRateIndex);
	auto const seekRateRandMinHz = getparameter_f(kSeekRateRandMin_Hz);
	mSeekRateRandMinIndex = getparameter_index(kSeekRateRandMin_Sync);
	auto const seekRateRandMinSync = mTempoRateTable.getScalar(mSeekRateRandMinIndex);
	mTempoSync = getparameter_b(kTempoSync);
	mSeekDur = getparameter_scalar(kSeekDur);
	mSeekDurRandMin = getparameter_scalar(kSeekDurRandMin);
	mSpeedMode = getparameter_i(kSpeedMode);
	mSplitChannels = getparameter_b(kSplitChannels);
	mPitchConstraint = getparameter_b(kPitchConstraint);
	mOctaveMin = getparameter_i(kOctaveMin);
	mOctaveMax = getparameter_i(kOctaveMax);
	mUserTempo = getparameter_f(kTempo);
	mUseHostTempo = getparameter_b(kTempoAuto);
	for (size_t i = 0; i < kNumPitchSteps; i++)
	{
		dfx::ParameterID const parameterID = static_cast<dfx::ParameterID>(i) + kPitchStep0;
		// fetch latest value regardless of whether it "changed" since MIDI notes generate internal values "quietly"
		mPitchSteps[i] = getparameter_b(parameterID);
		// reset the associated note in the notes table; manual changes override MIDI
		if (getparameterchanged(parameterID))
		{
			mActiveNotesTable[i] = 0;
		}
	}
	if (auto const value = getparameterifchanged_f(kDryLevel))
	{
		mInputGain = static_cast<float>(*value);
	}
	if (auto const value = getparameterifchanged_f(kWetLevel))
	{
		mOutputGain = static_cast<float>(*value);
	}

	// get the "generic" values of these parameters for randomization
	mSeekRateHz_gen = getparameter_gen(kSeekRate_Hz);
	mSeekRateRandMinHz_gen = getparameter_gen(kSeekRateRandMin_Hz);

	// make sure the cycles match up if the tempo rate has changed
	// or if tempo sync mode has just been switched on
	if (getparameterchanged(kSeekRate_Sync) || (getparameterchanged(kTempoSync) && mTempoSync))
	{
		std::fill(mNeedResync.begin(), mNeedResync.end(), true);
	}

	if (getparameterchanged(kPredelay))
	{
		// tell the host what the length of delay compensation should be
		setlatency_seconds(mSeekRangeSeconds * getparameter_scalar(kPredelay));
	}

	auto const entryUseSeekRateRandMin = std::exchange(mUseSeekRateRandMin, mTempoSync ? (seekRateRandMinSync < mSeekRateSync) : (seekRateRandMinHz < mSeekRateHz));
	if (entryUseSeekRateRandMin && !mUseSeekRateRandMin && mTempoSync)
	{
		std::fill(mNeedResync.begin(), mNeedResync.end(), true);
	}
	mUseSeekDurRandMin = (mSeekDurRandMin < mSeekDur);
}
