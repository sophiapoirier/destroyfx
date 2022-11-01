/*------------------------------------------------------------------------
Copyright (C) 2001-2022  Sophia Poirier

This file is part of Buffer Override.

Buffer Override is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Buffer Override is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Buffer Override.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#include "bufferoverride-base.h"
#include "bufferoverride.h"

#include <algorithm>
#include <cassert>
#include <cmath>

#include "dfxmisc.h"


#pragma mark _________init_________

// this macro does boring entry point stuff for us
DFX_EFFECT_ENTRY(BufferOverride)

//-----------------------------------------------------------------------------
// initializations and such
BufferOverride::BufferOverride(TARGET_API_BASE_INSTANCE_TYPE inInstance)
:	DfxPlugin(inInstance, kNumParameters, kNumPresets),
	mTempoRateTable(dfx::TempoRateTable::Rates::NoExtreme)
{
	auto const numTempoRates = mTempoRateTable.getNumRates();
	auto const unitTempoRateIndex = mTempoRateTable.getNearestTempoRateIndex(1.);
	initparameter_f(kDivisor, {"buffer divisor", "BufDiv", "BfDv"}, 1.92, 1.92, 1.92, 222.0, DfxParam::Unit::Divisor, DfxParam::Curve::Squared);
	initparameter_f(kBufferSize_MS, {"forced buffer size (free)", "BufSizF", "BufSzF", "BfSF"}, 90.0, 33.3, 1.0, 999.0, DfxParam::Unit::MS, DfxParam::Curve::Squared);
	initparameter_list(kBufferSize_Sync, {"forced buffer size (sync)", "BufSizS", "BufSzS", "BfSS"}, unitTempoRateIndex, unitTempoRateIndex, numTempoRates, DfxParam::Unit::Beats);
	initparameter_b(kBufferTempoSync, {"forced buffer tempo sync", "BufSync", "BufSnc", "BfSc"}, false);
	initparameter_b(kBufferInterrupt, {"buffer interrupt", "BufRupt", "BufInt", "BfIn"}, true);
	initparameter_f(kDivisorLFORate_Hz, {"divisor LFO rate (free)", "DvLFORF", "DLFORF", "DLRF"}, 0.3, 3.0, 0.03, 21.0, DfxParam::Unit::Hz, DfxParam::Curve::Squared);
	initparameter_list(kDivisorLFORate_Sync, {"divisor LFO rate (sync)", "DvLFORS", "DLFORS", "DLRS"}, unitTempoRateIndex, unitTempoRateIndex, numTempoRates, DfxParam::Unit::Beats);
	initparameter_f(kDivisorLFODepth, {"divisor LFO depth", "DvLFODp", "DvLFOD", "DvLD"}, 0.0, 0.0, 0.0, 100.0, DfxParam::Unit::Percent);
	initparameter_list(kDivisorLFOShape, {"divisor LFO shape", "DvLFOSh", "DLFOSh", "DLSh"}, 0, 0, dfx::LFO::kNumShapes);
	initparameter_b(kDivisorLFOTempoSync, {"divisor LFO tempo sync", "DvLFOSc", "DLFOSc", "DLSc"}, false);
	initparameter_f(kBufferLFORate_Hz, {"buffer LFO rate (free)", "BfLFORF", "BLFORF", "BLRF"}, 3.0, 3.0, 0.03, 21.0, DfxParam::Unit::Hz, DfxParam::Curve::Exp);//DfxParam::Curve::Squared);
	initparameter_list(kBufferLFORate_Sync, {"buffer LFO rate (sync)", "BfLFORS", "BLFORS", "BLRS"}, unitTempoRateIndex, unitTempoRateIndex, numTempoRates, DfxParam::Unit::Beats);
	initparameter_f(kBufferLFODepth, {"buffer LFO depth", "BfLFODp", "BfLFOD", "BfLD"}, 0.0, 0.0, 0.0, 100.0, DfxParam::Unit::Percent);
	initparameter_list(kBufferLFOShape, {"buffer LFO shape", "BfLFOSh", "BLFOSh", "BLSh"}, 0, 0, dfx::LFO::kNumShapes);
	initparameter_b(kBufferLFOTempoSync, {"buffer LFO tempo sync", "BfLFOSc", "BLFOSc", "BLSc"}, false);
	initparameter_f(kSmooth, {"smooth", "Smth"}, 9.0, 3.0, 0.0, 100.0, DfxParam::Unit::Percent);
	initparameter_f(kDryWetMix, dfx::MakeParameterNames(dfx::kParameterNames_DryWetMix), 100.0, 50.0, 0.0, 100.0, DfxParam::Unit::DryWetMix);
	initparameter_f(kPitchBendRange, dfx::MakeParameterNames(dfx::kParameterNames_PitchBendRange), 6.0, 3.0, 0.0, DfxMidi::kPitchBendSemitonesMax, DfxParam::Unit::Semitones);
	initparameter_list(kMidiMode, dfx::MakeParameterNames(dfx::kParameterNames_MidiMode), kMidiMode_Nudge, kMidiMode_Nudge, kNumMidiModes);
	initparameter_f(kTempo, dfx::MakeParameterNames(dfx::kParameterNames_Tempo), 120.0, 120.0, 57.0, 480.0, DfxParam::Unit::BPM);
	initparameter_b(kTempoAuto, dfx::MakeParameterNames(dfx::kParameterNames_TempoAuto), true);
	initparameter_f(kDecayDepth, {"decay depth", "DecaDep", "DecDep", "DecD"}, 0., 0., -100., 100., DfxParam::Unit::Percent);
	initparameter_list(kDecayMode, {"decay mode", "DecaMod", "DecMod", "DecM"}, kDecayMode_Gain, kDecayMode_Gain, kDecayModeCount);
	initparameter_list(kDecayShape, {"decay shape", "DecaShp", "DecShp", "DecS"}, kDecayShape_Ramp, kDecayShape_Ramp, kDecayShapeCount);

	// set the value strings for the LFO shape parameters
	for (dfx::LFO::Shape i = 0; i < dfx::LFO::kNumShapes; i++)
	{
		auto const shapeName = dfx::LFO::getShapeName(i);
		setparametervaluestring(kDivisorLFOShape, i, shapeName);
		setparametervaluestring(kBufferLFOShape, i, shapeName);
	}
	// set the value strings for the sync rate parameters
	for (size_t i = 0; i < numTempoRates; i++)
	{
		auto const& tempoRateName = mTempoRateTable.getDisplay(i);
		setparametervaluestring(kBufferSize_Sync, i, tempoRateName);
		setparametervaluestring(kDivisorLFORate_Sync, i, tempoRateName);
		setparametervaluestring(kBufferLFORate_Sync, i, tempoRateName);
	}
	// set the value strings for the MIDI mode parameter
	setparametervaluestring(kMidiMode, kMidiMode_Nudge, "nudge");
	setparametervaluestring(kMidiMode, kMidiMode_Trigger, "trigger");
	// decay type value strings
	setparametervaluestring(kDecayMode, kDecayMode_Gain, "gain");
	setparametervaluestring(kDecayMode, kDecayMode_Lowpass, "low-pass");
	setparametervaluestring(kDecayMode, kDecayMode_Highpass, "high-pass");
	setparametervaluestring(kDecayMode, kDecayMode_LP_HP_Alternating, "LP/HP ping pong");
	// decay shape value strings
	setparametervaluestring(kDecayShape, kDecayShape_Ramp, "ramp");
	setparametervaluestring(kDecayShape, kDecayShape_Oscillate, "oscillate");
	setparametervaluestring(kDecayShape, kDecayShape_Random, "random");

	addparameterattributes(kMidiMode, DfxParam::kAttribute_OmitFromRandomizeAll);

	addparametergroup("buffer divisor LFO", {kDivisorLFORate_Hz, kDivisorLFORate_Sync, kDivisorLFODepth, kDivisorLFOShape, kDivisorLFOTempoSync});
	addparametergroup("forced buffer size LFO", {kBufferLFORate_Hz, kBufferLFORate_Sync, kBufferLFODepth, kBufferLFOShape, kBufferLFOTempoSync});
	addparametergroup("buffer decay", {kDecayDepth, kDecayMode, kDecayShape});

	settailsize_seconds(1.0 / (mTempoRateTable.getScalar(0) * kMinAllowableBPS));


	mCurrentTempoBPS = getparameter_f(kTempo) / 60.0;

	setpresetname(0, "self-determined");  // default preset name
	initPresets();

	registerSmoothedAudioValue(mInputGain);
	registerSmoothedAudioValue(mOutputGain);

	updateViewDataCache();
}

//-------------------------------------------------------------------------
void BufferOverride::initialize()
{
	auto const numChannels = getnumoutputs();

	auto const maxAudioBufferSizeMS = ms2samples(getparametermax_f(kBufferSize_MS));
	auto const maxAudioBufferSizeSync = beat2samples(mTempoRateTable.getScalar(0), kMinAllowableBPS);
	auto const maxAudioBufferSize = static_cast<size_t>(std::max(maxAudioBufferSizeMS, maxAudioBufferSizeSync));
	mBuffers.resize(numChannels);
	for (auto& buffer : mBuffers)
	{
		buffer.assign(maxAudioBufferSize, 0.0f);
	}
	mAudioOutputValues.assign(numChannels, 0.0f);

	std::for_each(mDecayFilters.begin(), mDecayFilters.end(), [this, numChannels](auto& filters)
	{
		filters.assign(numChannels, {});
		std::for_each(filters.begin(), filters.end(), [this](auto& filter)
		{
			filter.setSampleRate(getsamplerate());
		});
	});
	mCurrentDecayFilters = mDecayFilters.front();
	mPrevDecayFilters = mDecayFilters.back();

	// this is a handy value to have during LFO calculations and wasteful to recalculate at every sample
	mOneDivSR = 1. / getsamplerate();
}

//-------------------------------------------------------------------------
void BufferOverride::cleanup()
{
	mBuffers = {};
	mAudioOutputValues = {};
	mDecayFilters.fill({});
	mCurrentDecayFilters = mPrevDecayFilters = {};
}

//-------------------------------------------------------------------------
void BufferOverride::reset()
{
	// setting the values like this will restart the forced buffer in the next process()
	mCurrentForcedBufferSize = 1;
	mWritePos = mReadPos = 1;
	mMinibufferSize = 1;
	mPrevMinibufferSize = 0;
	mSmoothCount = mSmoothDur = 0;
	mMinibufferDecayGain = 1.f;
	mPrevMinibufferDecayGain = 0.f;
//	mSqrtFadeIn = mSqrtFadeOut = 1.0f;

	mDivisorLFO.reset();
	mBufferLFO.reset();

	std::for_each(mDecayFilters.begin(), mDecayFilters.end(), [](auto& filters)
	{
		std::for_each(filters.begin(), filters.end(), [](auto& filter)
		{
			filter.reset();
			filter.setCoefficients(dfx::IIRFilter::kUnityCoeff);
		});
	});
	mDecayFilterIsLowpass = true;

	mOldNote = false;
	mLastNoteOn.reset();
	mLastPitchbendLSB.reset();
	mLastPitchbendMSB.reset();
	mPitchBend = 1.0;
	mOldPitchBend = 1.0;
	mDivisorWasChangedByMIDI = mDivisorWasChangedByHand = false;

	mNeedResync = true;  // some hosts may call resume when restarting playback

	mDivisorLFOValue_viewCache.store(kLFOValueDefault, std::memory_order_relaxed);
	mBufferLFOValue_viewCache.store(kLFOValueDefault, std::memory_order_relaxed);
	updateViewDataCache();
}


#pragma mark _________presets_________

//-------------------------------------------------------------------------
void BufferOverride::initPresets()
{
	size_t i = 1;

	setpresetname(i, "drum roll");
	setpresetparameter_f(i, kDivisor, 4.0);
	setpresetparameter_i(i, kBufferSize_Sync, mTempoRateTable.getNearestTempoRateIndex(4.));
	setpresetparameter_b(i, kBufferTempoSync, true);
	setpresetparameter_f(i, kSmooth, 9.0);
	setpresetparameter_f(i, kDryWetMix, getparametermax_f(kDryWetMix));
	setpresetparameter_i(i, kMidiMode, kMidiMode_Nudge);
	setpresetparameter_b(i, kTempoAuto, true);
	i++;

	setpresetname(i, "arpeggio");
	setpresetparameter_f(i, kDivisor, 37.0);
	setpresetparameter_f(i, kBufferSize_MS, 444.0);
	setpresetparameter_b(i, kBufferTempoSync, false);
	setpresetparameter_b(i, kBufferInterrupt, true);
	setpresetparameter_f(i, kDivisorLFORate_Hz, 0.3);
	setpresetparameter_f(i, kDivisorLFODepth, 72.0);
	setpresetparameter_i(i, kDivisorLFOShape, dfx::LFO::kShape_Saw);
	setpresetparameter_b(i, kDivisorLFOTempoSync, false);
	setpresetparameter_f(i, kBufferLFORate_Hz, 0.27);
	setpresetparameter_f(i, kBufferLFODepth, 63.0);
	setpresetparameter_i(i, kBufferLFOShape, dfx::LFO::kShape_Saw);
	setpresetparameter_b(i, kBufferLFOTempoSync, false);
	setpresetparameter_f(i, kSmooth, 4.2);
	setpresetparameter_f(i, kDryWetMix, getparametermax_f(kDryWetMix));
	setpresetparameter_i(i, kMidiMode, kMidiMode_Nudge);
	i++;

	setpresetname(i, "laser");
	setpresetparameter_f(i, kDivisor, 170.0);
	setpresetparameter_f(i, kBufferSize_MS, 128.0);
	setpresetparameter_b(i, kBufferTempoSync, false);
	setpresetparameter_b(i, kBufferInterrupt, true);
	setpresetparameter_f(i, kDivisorLFORate_Hz, 9.0);
	setpresetparameter_f(i, kDivisorLFODepth, 87.0);
	setpresetparameter_i(i, kDivisorLFOShape, dfx::LFO::kShape_Thorn);
	setpresetparameter_b(i, kDivisorLFOTempoSync, false);
	setpresetparameter_f(i, kBufferLFORate_Hz, 5.55);
	setpresetparameter_f(i, kBufferLFODepth, 69.0);
	setpresetparameter_i(i, kBufferLFOShape, dfx::LFO::kShape_ReverseSaw);
	setpresetparameter_b(i, kBufferLFOTempoSync, false);
	setpresetparameter_f(i, kSmooth, 20.1);
	setpresetparameter_f(i, kDryWetMix, getparametermax_f(kDryWetMix));
	setpresetparameter_i(i, kMidiMode, kMidiMode_Nudge);
	i++;

	setpresetname(i, "sour melodies");
	setpresetparameter_f(i, kDivisor, 42.0);
	setpresetparameter_f(i, kBufferSize_MS, 210.0);
	setpresetparameter_b(i, kBufferTempoSync, false);
	setpresetparameter_b(i, kBufferInterrupt, true);
	setpresetparameter_f(i, kDivisorLFORate_Hz, 3.78);
	setpresetparameter_f(i, kDivisorLFODepth, 90.0);
	setpresetparameter_i(i, kDivisorLFOShape, dfx::LFO::kShape_Random);
	setpresetparameter_b(i, kDivisorLFOTempoSync, false);
	setpresetparameter_f(i, kBufferLFODepth, 0.0);
	setpresetparameter_f(i, kSmooth, 3.9);
	setpresetparameter_f(i, kDryWetMix, getparametermax_f(kDryWetMix));
	setpresetparameter_i(i, kMidiMode, kMidiMode_Nudge);
	i++;

	setpresetname(i, "rerun");
	setpresetparameter_f(i, kDivisor, 9.0);
	setpresetparameter_f(i, kBufferSize_MS, 747.0);
	setpresetparameter_b(i, kBufferTempoSync, false);
	setpresetparameter_f(i, kDivisorLFORate_Hz, getparametermin_f(kDivisorLFORate_Hz));
	setpresetparameter_f(i, kDivisorLFODepth, 0.0);
	setpresetparameter_i(i, kDivisorLFOShape, dfx::LFO::kShape_Triangle);
	setpresetparameter_b(i, kDivisorLFOTempoSync, false);
	setpresetparameter_f(i, kBufferLFORate_Hz, 0.174);
	setpresetparameter_f(i, kBufferLFODepth, 21.0);
	setpresetparameter_i(i, kBufferLFOShape, dfx::LFO::kShape_Triangle);
	setpresetparameter_b(i, kBufferLFOTempoSync, false);
	setpresetparameter_f(i, kSmooth, 8.1);
	setpresetparameter_f(i, kDryWetMix, getparametermax_f(kDryWetMix));
	setpresetparameter_i(i, kMidiMode, kMidiMode_Nudge);
	i++;

	setpresetname(i, "jiggy");
	setpresetparameter_f(i, kDivisor, 4.0);
	setpresetparameter_i(i, kBufferSize_Sync, mTempoRateTable.getNearestTempoRateIndex(4.));
	setpresetparameter_b(i, kBufferTempoSync, true);
	setpresetparameter_b(i, kBufferInterrupt, true);
	setpresetparameter_i(i, kDivisorLFORate_Sync, mTempoRateTable.getNearestTempoRateIndex(0.5));
	setpresetparameter_f(i, kDivisorLFODepth, 84.0);
	setpresetparameter_i(i, kDivisorLFOShape, dfx::LFO::kShape_Square);
	setpresetparameter_b(i, kDivisorLFOTempoSync, true);
	setpresetparameter_f(i, kBufferLFODepth, 0.0);
	setpresetparameter_f(i, kSmooth, 9.0);  // eh?
	setpresetparameter_f(i, kDryWetMix, 100.0);
	setpresetparameter_i(i, kMidiMode, kMidiMode_Nudge);
	setpresetparameter_b(i, kTempoAuto, true);
	i++;

	setpresetname(i, "aliens are coming");
	setpresetparameter_f(i, kDivisor, 5.64);
	setpresetparameter_f(i, kBufferSize_MS, 31.25);
	setpresetparameter_b(i, kBufferTempoSync, false);
	setpresetparameter_b(i, kBufferInterrupt, true);
	setpresetparameter_f(i, kDivisorLFODepth, 0.0);
	setpresetparameter_f(i, kBufferLFORate_Hz, 1.53);
	setpresetparameter_f(i, kBufferLFODepth, 99.0);
	setpresetparameter_i(i, kBufferLFOShape, dfx::LFO::kShape_Sine);
	setpresetparameter_b(i, kBufferLFOTempoSync, false);
	setpresetparameter_f(i, kSmooth, 9.0);
	setpresetparameter_f(i, kDryWetMix, 100.0);
	setpresetparameter_i(i, kMidiMode, kMidiMode_Nudge);
	i++;

	setpresetname(i, "\"echo\"");
	setpresetparameter_f(i, kDivisor, 2.001);
	setpresetparameter_f(i, kBufferSize_MS, 603.0);
	setpresetparameter_b(i, kBufferTempoSync, false);
	setpresetparameter_f(i, kDivisorLFODepth, 0.0);
	setpresetparameter_f(i, kBufferLFODepth, 0.0);
	setpresetparameter_f(i, kSmooth, getparametermax_f(kSmooth));
	setpresetparameter_f(i, kDryWetMix, getparametermax_f(kDryWetMix));
	setpresetparameter_i(i, kMidiMode, kMidiMode_Nudge);
	i++;

	setpresetname(i, "squeegee");
	setpresetparameter_f(i, kDivisor, 27.0);
	setpresetparameter_f(i, kBufferSize_MS, 81.0);
	setpresetparameter_b(i, kBufferTempoSync, false);
	setpresetparameter_b(i, kBufferInterrupt, true);
	setpresetparameter_i(i, kDivisorLFORate_Sync, mTempoRateTable.getNearestTempoRateIndex(2.));
	setpresetparameter_f(i, kDivisorLFODepth, 33.3);
	setpresetparameter_i(i, kDivisorLFOShape, dfx::LFO::kShape_Sine);
	setpresetparameter_b(i, kDivisorLFOTempoSync, true);
	setpresetparameter_i(i, kBufferLFORate_Sync, 0);
	setpresetparameter_f(i, kBufferLFODepth, 6.0);
	setpresetparameter_i(i, kBufferLFOShape, dfx::LFO::kShape_Saw);
	setpresetparameter_b(i, kBufferLFOTempoSync, true);
	setpresetparameter_f(i, kSmooth, 6.0);
	setpresetparameter_f(i, kDryWetMix, getparametermax_f(kDryWetMix));
	setpresetparameter_i(i, kMidiMode, kMidiMode_Nudge);
	setpresetparameter_b(i, kTempoAuto, true);
	i++;

/*
	setpresetname(i, "");
	setpresetparameter_f(i, kDivisor, );
	setpresetparameter_f(i, kBufferSize_MS, );
	setpresetparameter_i(i, kBufferSize_Sync, mTempoRateTable.getNearestTempoRateIndex());
	setpresetparameter_b(i, kBufferTempoSync, );
	setpresetparameter_b(i, kBufferInterrupt, );
	setpresetparameter_f(i, kDivisorLFORate_Hz, );
	setpresetparameter_i(i, kDivisorLFORate_Sync, mTempoRateTable.getNearestTempoRateIndex());
	setpresetparameter_f(i, kDivisorLFODepth, );
	setpresetparameter_i(i, kDivisorLFOShape, dfx::LFO::kShape_);
	setpresetparameter_b(i, kDivisorLFOTempoSync, );
	setpresetparameter_f(i, kBufferLFORate_Hz, );
	setpresetparameter_i(i, kBufferLFORate_Sync, mTempoRateTable.getNearestTempoRateIndex());
	setpresetparameter_f(i, kBufferLFODepth, );
	setpresetparameter_i(i, kBufferLFOShape, dfx::LFO::kShape_);
	setpresetparameter_b(i, kBufferLFOTempoSync, );
	setpresetparameter_f(i, kSmooth, );
	setpresetparameter_f(i, kDryWetMix, );
	setpresetparameter_d(i, kPitchBendRange, );
	setpresetparameter_i(i, kMidiMode, kMidiMode_);
	setpresetparameter_f(i, kTempo, );
	setpresetparameter_b(i, kTempoAuto, );
	i++;
*/
}


#pragma mark _________parameters_________

//-------------------------------------------------------------------------
void BufferOverride::processparameters()
{
	mDivisor = getparameter_f(kDivisor);
	mBufferSizeMS = getparameter_f(kBufferSize_MS);
	mBufferSizeSync = mTempoRateTable.getScalar(getparameter_index(kBufferSize_Sync));
	mBufferTempoSync = getparameter_b(kBufferTempoSync);
	mBufferInterrupt = getparameter_b(kBufferInterrupt);
	mDivisorLFORateHz = getparameter_f(kDivisorLFORate_Hz);
	mDivisorLFOTempoRate = mTempoRateTable.getScalar(getparameter_index(kDivisorLFORate_Sync));
	mDivisorLFO.setDepth(getparameter_scalar(kDivisorLFODepth));
	mDivisorLFO.setShape(getparameter_i(kDivisorLFOShape));
	mDivisorLFOTempoSync = getparameter_b(kDivisorLFOTempoSync);
	mBufferLFORateHz = getparameter_f(kBufferLFORate_Hz);
	mBufferLFOTempoRate = mTempoRateTable.getScalar(getparameter_index(kBufferLFORate_Sync));
	mBufferLFO.setDepth(getparameter_scalar(kBufferLFODepth));
	mBufferLFO.setShape(getparameter_i(kBufferLFOShape));
	mBufferLFOTempoSync = getparameter_b(kBufferLFOTempoSync);
	mSmoothPortion = getparameter_scalar(kSmooth);
	mPitchBendRange = getparameter_f(kPitchBendRange);
	mMidiMode = getparameter_i(kMidiMode);
	mUserTempoBPM = getparameter_f(kTempo);
	mUseHostTempo = getparameter_b(kTempoAuto);
	mDecayDepth = getparameter_scalar(kDecayDepth);
	mDecayMode = getparameter_i(kDecayMode);
	mDecayShape = getparameter_i(kDecayShape);

	if (getparameterchanged(kDivisor))
	{
		// tell MIDI trigger mode to respect this change
		mDivisorWasChangedByHand = true;
	}
	if (getparameterchanged(kBufferSize_Sync))
	{
		// make sure the cycles match up if the tempo rate has changed
		mNeedResync = true;
	}
	if (getparameterchanged(kBufferTempoSync) && mBufferTempoSync)
	{
		// flag if tempo sync mode has just been switched on
		mNeedResync = true;
	}
	if (auto const value = getparameterifchanged_scalar(kDryWetMix))
	{
		// square root for equal power mix
		auto const dryWetMix = static_cast<float>(*value);
		mInputGain = std::sqrt(1.0f - dryWetMix);
		mOutputGain = std::sqrt(dryWetMix);
	}
	if (getparameterchanged(kMidiMode))
	{
		// reset all notes to off if we're switching into MIDI trigger mode
		if (mMidiMode == kMidiMode_Trigger)
		{
			getmidistate().removeAllNotes();
			mDivisorWasChangedByHand = false;
		}
	}
	if (getparameterchanged(kTempoAuto) && mUseHostTempo)
	{
		// flag if host sync has just been switched on
		mNeedResync = true;
	}
}

//-------------------------------------------------------------------------
void BufferOverride::parameterChanged(dfx::ParameterID inParameterID)
{
	switch (inParameterID)
	{
		case kDivisor:
		case kBufferSize_MS:
		case kBufferSize_Sync:
		case kBufferTempoSync:
		case kTempoAuto:
		case kTempo:
			updateViewDataCache();
			break;
	}
}


#pragma mark _________properties_________

//-------------------------------------------------------------------------
void BufferOverride::updateViewDataCache()
{
	BufferOverrideViewData viewData;
	auto const hostTempoBPS = mHostTempoBPS_viewCache.load(std::memory_order_relaxed);
	auto const tempoBPS = (getparameter_b(kTempoAuto) && (hostTempoBPS > 0.)) ? hostTempoBPS : (getparameter_f(kTempo) / 60.);

	if (getparameter_b(kBufferTempoSync) && (tempoBPS > 0.))
	{
		viewData.mPreLFO.mForcedBufferSeconds = 1. / (tempoBPS * mTempoRateTable.getScalar(getparameter_index(kBufferSize_Sync)));
	}
	else
	{
		viewData.mPreLFO.mForcedBufferSeconds = getparameter_f(kBufferSize_MS) * 0.001;
	}
	viewData.mPostLFO.mForcedBufferSeconds = viewData.mPreLFO.mForcedBufferSeconds * mBufferLFOValue_viewCache.load(std::memory_order_relaxed);

	// just the size of the first repetition with no complexity about the boundary case at the end
	if (auto divisor = static_cast<float>(getparameter_f(kDivisor)); divisor >= kActiveDivisorMinimum)
	{
		viewData.mPreLFO.mMinibufferSeconds = viewData.mPreLFO.mForcedBufferSeconds / divisor;
		divisor = std::max(divisor * mDivisorLFOValue_viewCache.load(std::memory_order_relaxed), kActiveDivisorMinimum);
		viewData.mPostLFO.mMinibufferSeconds = viewData.mPostLFO.mForcedBufferSeconds / divisor;
	}
	else
	{
		viewData.mPreLFO.mMinibufferSeconds = viewData.mPreLFO.mForcedBufferSeconds;
		viewData.mPostLFO.mMinibufferSeconds = viewData.mPostLFO.mForcedBufferSeconds;
	}
	assert(viewData.mPreLFO.mMinibufferSeconds <= viewData.mPreLFO.mForcedBufferSeconds);
	assert(viewData.mPostLFO.mMinibufferSeconds <= viewData.mPostLFO.mForcedBufferSeconds);

	mViewDataCache.store(viewData, std::memory_order_relaxed);
	mViewDataCacheTimestamp.fetch_add(1, std::memory_order_relaxed);
}

//-------------------------------------------------------------------------
dfx::StatusCode BufferOverride::dfx_GetPropertyInfo(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned int inItemIndex,
													size_t& outDataSize, dfx::PropertyFlags& outFlags)
{
	switch (inPropertyID)
	{
		case kBOProperty_LastViewDataTimestamp:
			outDataSize = sizeof(uint64_t);
			outFlags = dfx::kPropertyFlag_Readable;
			return dfx::kStatus_NoError;
		case kBOProperty_ViewData:
			outDataSize = sizeof(BufferOverrideViewData);
			outFlags = dfx::kPropertyFlag_Readable;
			return dfx::kStatus_NoError;
		default:
			return DfxPlugin::dfx_GetPropertyInfo(inPropertyID, inScope, inItemIndex, outDataSize, outFlags);
	}
}

//-------------------------------------------------------------------------
dfx::StatusCode BufferOverride::dfx_GetProperty(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned int inItemIndex,
												void* outData)
{
	switch (inPropertyID)
	{
		case kBOProperty_LastViewDataTimestamp:
			*static_cast<uint64_t*>(outData) = mViewDataCacheTimestamp.load(std::memory_order_relaxed);
			return dfx::kStatus_NoError;
		case kBOProperty_ViewData:
			*static_cast<BufferOverrideViewData*>(outData) = mViewDataCache.load(std::memory_order_relaxed);
			return dfx::kStatus_NoError;
		default:
			return DfxPlugin::dfx_GetProperty(inPropertyID, inScope, inItemIndex, outData);
	}
}
