/*------------------------------------------------------------------------
Copyright (C) 2000-2021  Sophia Poirier

This file is part of Skidder.

Skidder is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Skidder is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Skidder.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "skidder.h"

#include <cmath>

#include "dfxmisc.h"


// this macro does boring entry point stuff for us
DFX_EFFECT_ENTRY(Skidder)

//-----------------------------------------------------------------------------
// initializations and such
Skidder::Skidder(TARGET_API_BASE_INSTANCE_TYPE inInstance)
:	DfxPlugin(inInstance, kNumParameters, kNumPresets)
{
	// initialize the parameters
	auto const unitTempoRateIndex = mTempoRateTable.getNearestTempoRateIndex(1.0f);
	auto const numTempoRates = mTempoRateTable.getNumRates();
	initparameter_f(kRate_Hz, {"rate (free)", "RateFre", "RateFr", "RtFr"}, 3.0, 3.0, 0.3, 21.0, DfxParam::Unit::Hz, DfxParam::Curve::Log);
	initparameter_list(kRate_Sync, {"rate (sync)", "RateSnc", "RatSnc", "RtSc"}, unitTempoRateIndex, unitTempoRateIndex, numTempoRates, DfxParam::Unit::Beats);
	initparameter_f(kRateRandMin_Hz, {"rate random min (free)", "RatMinF", "RatMnF", "RtMF"}, 3.0, 3.0, 0.3, 21.0, DfxParam::Unit::Hz, DfxParam::Curve::Log);
	initparameter_list(kRateRandMin_Sync, {"rate random min (sync)", "RatMinS", "RatMnS", "RtMS"}, unitTempoRateIndex, unitTempoRateIndex, numTempoRates, DfxParam::Unit::Beats);
	initparameter_b(kTempoSync, dfx::MakeParameterNames(dfx::kParameterNames_TempoSync), false, false);
	initparameter_f(kPulsewidth, {"pulsewidth", "PulsWid", "PulsWd", "PlsW"}, 0.5, 0.5, 0.001, 0.999, DfxParam::Unit::Scalar);
	initparameter_f(kPulsewidthRandMin, {"pulsewidth random min", "PlsWdMn", "PlsWdM", "PlWM"}, 0.5, 0.5, 0.001, 0.999, DfxParam::Unit::Scalar);
	initparameter_f(kSlope, {"slope", "Slop"}, 3.0, 3.0, 0.0, 300.0, DfxParam::Unit::MS, DfxParam::Curve::Squared);
	initparameter_f(kPan, {"stereo spread", "Stereo", "Ster"}, 0.0, 0.6, 0.0, 1.0, DfxParam::Unit::Scalar);
	initparameter_f(kFloor, dfx::MakeParameterNames(dfx::kParameterNames_Floor), 0.0, 0.0, 0.0, 1.0, DfxParam::Unit::LinearGain, DfxParam::Curve::Cubed);
	initparameter_f(kFloorRandMin, {"floor random min", "FlorMin", "FlorMn", "FlrM"}, 0.0, 0.0, 0.0, 1.0, DfxParam::Unit::LinearGain, DfxParam::Curve::Cubed);
	initparameter_f(kNoise, {"rupture", "Ruptur", "Rptr"}, 0.0, std::pow(0.5, 2.0), 0.0, 1.0, DfxParam::Unit::LinearGain, DfxParam::Curve::Squared);
	initparameter_f(kCrossoverFrequency, {"crossover frequency", "RateFre", "RateFr", "RtFr"}, 3'000., 3'000., 20., 20'000., DfxParam::Unit::Hz, DfxParam::Curve::Log);
	initparameter_list(kCrossoverMode, {"crossover mode", "XovrMod", "XovrMd", "XvrM"}, kCrossoverMode_All, kCrossoverMode_All, kNumCrossoverModes);
	initparameter_list(kMidiMode, dfx::MakeParameterNames(dfx::kParameterNames_MidiMode), kMidiMode_None, kMidiMode_None, kNumMidiModes);
	initparameter_b(kVelocity, {"velocity", "Velocty", "Veloct", "Velo"}, false, false);
	initparameter_f(kTempo, dfx::MakeParameterNames(dfx::kParameterNames_Tempo), 120.0, 120.0, 39.0, 480.0, DfxParam::Unit::BPM);
	initparameter_b(kTempoAuto, dfx::MakeParameterNames(dfx::kParameterNames_TempoAuto), true, true);

	// set the value strings for the sync rate parameters
	for (long i = 0; i < mTempoRateTable.getNumRates(); i++)
	{
		auto const& tempoRateName = mTempoRateTable.getDisplay(i);
		setparametervaluestring(kRate_Sync, i, tempoRateName);
		setparametervaluestring(kRateRandMin_Sync, i, tempoRateName);
	}
	// value strings for the crossover modes
	setparametervaluestring(kCrossoverMode, kCrossoverMode_All, "all");
	setparametervaluestring(kCrossoverMode, kCrossoverMode_Low, "low");
	setparametervaluestring(kCrossoverMode, kCrossoverMode_High, "high");
	// value strings for the MIDI modes
	setparametervaluestring(kMidiMode, kMidiMode_None, "none");
	setparametervaluestring(kMidiMode, kMidiMode_Trigger, "trigger");
	setparametervaluestring(kMidiMode, kMidiMode_Apply, "apply");


	setpresetname(0, "thip thip thip");  // default preset name

	mNoteTable.fill(0);

	addchannelconfig(kChannelConfig_AnyMatchedIO);  // N-in/N-out
	addchannelconfig(1, 2);  // 1-in/2-out

	// give mCurrentTempoBPS a value in case that's useful for a freshly opened GUI
	mCurrentTempoBPS = getparameter_f(kTempo) / 60.0;

	// start off with split CC automation of both range slider points
	mRateDoubleAutomate = mPulsewidthDoubleAutomate = mFloorDoubleAutomate = false;

	registerSmoothedAudioValue(&mNoise);
	registerSmoothedAudioValue(&mCrossoverFrequency_gen);
}

//-----------------------------------------------------------------------------------------
void Skidder::dfx_PostConstructor()
{
	// since we don't use pitchbend for anything special, 
	// allow it be assigned to control parameters
	getsettings().setAllowPitchbendEvents(true);
}

//-----------------------------------------------------------------------------------------
void Skidder::createbuffers()
{
	mInputAudio.assign(getnumoutputs(), nullptr);  // allocating output channel count is intentional, for mono fan-out
	mOutputAudio.assign(getnumoutputs(), nullptr);
	mEffectualInputAudioBuffers.assign(getnuminputs(), std::vector<float>(getmaxframes()));
	if (asymmetricalchannels())
	{
		mAsymmetricalInputAudioBuffer.assign(getmaxframes(), 0.0f);
	}

	mCrossover = std::make_unique<dfx::Crossover>(getnuminputs(), getsamplerate(), getparameter_f(kCrossoverFrequency));
}

//-----------------------------------------------------------------------------------------
void Skidder::releasebuffers()
{
	mInputAudio.clear();
	mOutputAudio.clear();
	mEffectualInputAudioBuffers.clear();
	mAsymmetricalInputAudioBuffer.clear();

	mCrossover.reset();
}

//-----------------------------------------------------------------------------------------
void Skidder::reset()
{
	if (mCrossover)
	{
		mCrossover->reset();
	}

	mState = SkidState::Valley;
	mValleySamples = 0;
	mPanGainL = mPanGainR = 1.0f;
	mRMS = 0.0;
	mRMSCount = 0;
	mRandomFloor = 0.0f;
	mRandomGainRange = 1.0f;

	resetMidi();

	mNeedResync = true;  // some hosts may call resume when restarting playback
	mWaitSamples = 0;
}

//-----------------------------------------------------------------------------------------
void Skidder::processparameters()
{
	mRate_Hz = getparameter_f(kRate_Hz);
	mRateIndex = getparameter_i(kRate_Sync);
	mRate_Sync = mTempoRateTable.getScalar(mRateIndex);
	auto const rateRandMin_Hz = getparameter_f(kRateRandMin_Hz);
	mRateRandMinIndex = getparameter_i(kRateRandMin_Sync);
	auto const rateRandMin_Sync = mTempoRateTable.getScalar(mRateRandMinIndex);
	mTempoSync = getparameter_b(kTempoSync);
	mPulsewidth = getparameter_f(kPulsewidth);
	mPulsewidthRandMin = getparameter_f(kPulsewidthRandMin);
	mSlopeSeconds = getparameter_f(kSlope) * 0.001;
	mPanWidth = getparameter_f(kPan);
	if (auto const value = getparameterifchanged_scalar(kNoise))
	{
		mNoise = *value;
	}
	mMidiMode = getparameter_i(kMidiMode);
	mUseVelocity = getparameter_b(kVelocity);
	mFloor = getparameter_f(kFloor);
	auto const floorRandMin = static_cast<float>(getparameter_f(kFloorRandMin));
	if (auto const value = getparameterifchanged_gen(kCrossoverFrequency))
	{
		mCrossoverFrequency_gen = *value;
	}
	if (auto const value = getparameterifchanged_i(kCrossoverMode))
	{
		auto const entryCrossoverMode = std::exchange(mCrossoverMode, *value);
		if (mCrossoverMode == kCrossoverMode_All)
		{
			mCrossover->reset();
		}
		// only if the value definitely differs (e.g. it could have changed once and then back since last audio render)
		else if ((entryCrossoverMode == kCrossoverMode_All) && (mCrossoverMode != entryCrossoverMode))
		{
			mCrossoverFrequency_gen.snap();
		}
	}
	mUserTempo = getparameter_f(kTempo);
	mUseHostTempo = getparameter_b(kTempoAuto);

	// get the "generic" values of these parameters for randomization
	mRateHz_gen = getparameter_gen(kRate_Hz);
	mRateRandMinHz_gen = getparameter_gen(kRateRandMin_Hz);
	mFloor_gen = getparameter_gen(kFloor);
	mFloorRandMin_gen = getparameter_gen(kFloorRandMin);

	// set mNeedResync true if tempo sync mode has just been switched on
	if (getparameterchanged(kTempoSync) && mTempoSync)
	{
		mNeedResync = true;
	}
	if (getparameterchanged(kRate_Sync))
	{
		mNeedResync = true;
	}
	if (getparameterchanged(kMidiMode))
	{
		// if we've just entered a MIDI mode, zero out all notes and reset mWaitSamples
// XXX fetch old value
//		if ((oldMidiMode == kMidiMode_None) && (mMidiMode != kMidiMode_None))
		{
			mNoteTable.fill(0);
			noteOff();
		}
	}

	mGainRange = 1.0f - mFloor;  // the range of the skidding on/off gain
	mUseRandomRate = mTempoSync ? (rateRandMin_Sync < mRate_Sync) : (rateRandMin_Hz < mRate_Hz);
	mUseRandomFloor = (floorRandMin < mFloor);
	mUseRandomPulsewidth = (mPulsewidthRandMin < mPulsewidth);
}

double Skidder::getCrossoverFrequency() const
{
	return expandparametervalue(kCrossoverFrequency, mCrossoverFrequency_gen.getValue());
}
