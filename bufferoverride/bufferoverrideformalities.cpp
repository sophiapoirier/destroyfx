/*------------------------------------------------------------------------
Copyright (C) 2001-2019  Sophia Poirier

This file is part of Buffer Override.

Buffer Override is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Buffer Override is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Buffer Override.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "bufferoverride.h"

#include <cmath>


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
	auto const unitTempoRateIndex = mTempoRateTable.getNearestTempoRateIndex(1.0f);
	initparameter_f(kDivisor, "buffer divisor", 1.92, 1.92, 1.92, 222.0, DfxParam::Unit::Divisor, DfxParam::Curve::Squared);
	initparameter_f(kBufferSize_MS, "forced buffer size (free)", 90.0, 33.3, 1.0, 999.0, DfxParam::Unit::MS, DfxParam::Curve::Squared);
	initparameter_list(kBufferSize_Sync, "forced buffer size (sync)", unitTempoRateIndex, unitTempoRateIndex, numTempoRates, DfxParam::Unit::Beats);
	initparameter_b(kBufferTempoSync, "forced buffer tempo sync", false, false);
	initparameter_b(kBufferInterrupt, "buffer interrupt", true, true);
	initparameter_f(kDivisorLFORate_Hz, "divisor LFO rate (free)", 0.3, 3.0, 0.03, 21.0, DfxParam::Unit::Hz, DfxParam::Curve::Squared);
	initparameter_list(kDivisorLFORate_Sync, "divisor LFO rate (sync)", unitTempoRateIndex, unitTempoRateIndex, numTempoRates, DfxParam::Unit::Beats);
	initparameter_f(kDivisorLFODepth, "divisor LFO depth", 0.0, 0.0, 0.0, 100.0, DfxParam::Unit::Percent);
	initparameter_list(kDivisorLFOShape, "divisor LFO shape", 0, 0, dfx::LFO::kNumShapes);
	initparameter_b(kDivisorLFOTempoSync, "divisor LFO tempo sync", false, false);
	initparameter_f(kBufferLFORate_Hz, "buffer LFO rate (free)", 3.0, 3.0, 0.03, 21.0, DfxParam::Unit::Hz, DfxParam::Curve::Exp);//DfxParam::Curve::Squared);
	initparameter_list(kBufferLFORate_Sync, "buffer LFO rate (sync)", unitTempoRateIndex, unitTempoRateIndex, numTempoRates, DfxParam::Unit::Beats);
	initparameter_f(kBufferLFODepth, "buffer LFO depth", 0.0, 0.0, 0.0, 100.0, DfxParam::Unit::Percent);
	initparameter_list(kBufferLFOShape, "buffer LFO shape", 0, 0, dfx::LFO::kNumShapes);
	initparameter_b(kBufferLFOTempoSync, "buffer LFO tempo sync", false, false);
	initparameter_f(kSmooth, "smooth", 9.0, 3.0, 0.0, 100.0, DfxParam::Unit::Percent);
	initparameter_f(kDryWetMix, "dry/wet mix", 100.0, 50.0, 0.0, 100.0, DfxParam::Unit::DryWetMix);
	initparameter_f(kPitchbend, "pitch bend range", 6.0, 3.0, 0.0, DfxMidi::kPitchBendSemitonesMax, DfxParam::Unit::Semitones);
	initparameter_list(kMidiMode, "MIDI mode", kMidiMode_Nudge, kMidiMode_Nudge, kNumMidiModes);
	initparameter_f(kTempo, "tempo", 120.0, 120.0, 57.0, 480.0, DfxParam::Unit::BPM);
	initparameter_b(kTempoAuto, "sync to host tempo", true, true);

	// set the value strings for the LFO shape parameters
	for (dfx::LFO::Shape i = 0; i < dfx::LFO::kNumShapes; i++)
	{
		auto const shapeName = dfx::LFO::getShapeName(i);
		setparametervaluestring(kDivisorLFOShape, i, shapeName.c_str());
		setparametervaluestring(kBufferLFOShape, i, shapeName.c_str());
	}
	// set the value strings for the sync rate parameters
	for (int i = 0; i < mTempoRateTable.getNumRates(); i++)
	{
		auto const& tempoRateName = mTempoRateTable.getDisplay(i);
		setparametervaluestring(kBufferSize_Sync, i, tempoRateName.c_str());
		setparametervaluestring(kDivisorLFORate_Sync, i, tempoRateName.c_str());
		setparametervaluestring(kBufferLFORate_Sync, i, tempoRateName.c_str());
	}
	// set the value strings for the MIDI mode parameter
	setparametervaluestring(kMidiMode, kMidiMode_Nudge, "nudge");
	setparametervaluestring(kMidiMode, kMidiMode_Trigger, "trigger");

	settailsize_seconds(1.0 / (mTempoRateTable.getScalar(0) * kMinAllowableBPS));


	// give a value in case that's useful for a freshly opened GUI
	mCurrentTempoBPS = getparameter_f(kTempo) / 60.0;

	setpresetname(0, "self-determined");  // default preset name
	initPresets();

	registerSmoothedAudioValue(&mInputGain);
	registerSmoothedAudioValue(&mOutputGain);
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
	mSqrtFadeIn = mSqrtFadeOut = 1.0f;

	mDivisorLFO.reset();
	mBufferLFO.reset();

	mOldNote = false;
	mLastNoteOn = DfxMidi::kInvalidValue;
	mLastPitchbendLSB = mLastPitchbendMSB = DfxMidi::kInvalidValue;
	mPitchBend = 1.0;
	mOldPitchBend = 1.0;
	mDivisorWasChangedByMIDI = mDivisorWasChangedByHand = false;

	mNeedResync = true;  // some hosts may call resume when restarting playback

	// this is a handy value to have during LFO calculations and wasteful to recalculate at every sample
	mOneDivSR = 1.0f / getsamplerate_f();
}

//-------------------------------------------------------------------------
void BufferOverride::createbuffers()
{
	auto const numChannels = getnumoutputs();
	auto const maxAudioBufferSize = static_cast<size_t>(getsamplerate() / (kMinAllowableBPS * mTempoRateTable.getScalar(0)));

	mBuffers.resize(numChannels);
	for (auto& buffer : mBuffers)
	{
		buffer.assign(maxAudioBufferSize, 0.0f);
	}
	mAudioOutputValues.assign(numChannels, 0.0f);
}

//-------------------------------------------------------------------------
void BufferOverride::releasebuffers()
{
	mBuffers.clear();
	mAudioOutputValues.clear();
}


#pragma mark _________presets_________

//-------------------------------------------------------------------------
void BufferOverride::initPresets()
{
	int i = 1;

	setpresetname(i, "drum roll");
	setpresetparameter_f(i, kDivisor, 4.0);
	setpresetparameter_i(i, kBufferSize_Sync, mTempoRateTable.getNearestTempoRateIndex(4.0f));
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
	setpresetparameter_i(i, kBufferSize_Sync, mTempoRateTable.getNearestTempoRateIndex(4.0f));
	setpresetparameter_b(i, kBufferTempoSync, true);
	setpresetparameter_b(i, kBufferInterrupt, true);
	setpresetparameter_i(i, kDivisorLFORate_Sync, mTempoRateTable.getNearestTempoRateIndex(0.5f));
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
	setpresetparameter_i(i, kDivisorLFORate_Sync, mTempoRateTable.getNearestTempoRateIndex(2.0f));
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
	setpresetparameter_i(i, kBufferSize_Sync, mTempoRateTable.getNearestTempoRateIndex(f));
	setpresetparameter_b(i, kBufferTempoSync, );
	setpresetparameter_b(i, kBufferInterrupt, );
	setpresetparameter_f(i, kDivisorLFORate_Hz, );
	setpresetparameter_i(i, kDivisorLFORate_Sync, mTempoRateTable.getNearestTempoRateIndex(f));
	setpresetparameter_f(i, kDivisorLFODepth, );
	setpresetparameter_i(i, kDivisorLFOShape, dfx::LFO::kShape_);
	setpresetparameter_b(i, kDivisorLFOTempoSync, );
	setpresetparameter_f(i, kBufferLFORate_Hz, );
	setpresetparameter_i(i, kBufferLFORate_Sync, mTempoRateTable.getNearestTempoRateIndex(f));
	setpresetparameter_f(i, kBufferLFODepth, );
	setpresetparameter_i(i, kBufferLFOShape, dfx::LFO::kShape_);
	setpresetparameter_b(i, kBufferLFOTempoSync, );
	setpresetparameter_f(i, kSmooth, );
	setpresetparameter_f(i, kDryWetMix, );
	setpresetparameter_d(i, kPitchbend, );
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
	mBufferSizeSync = mTempoRateTable.getScalar(getparameter_i(kBufferSize_Sync));
	mBufferTempoSync = getparameter_b(kBufferTempoSync);
	mBufferInterrupt = getparameter_b(kBufferInterrupt);
	mDivisorLFORateHz = getparameter_f(kDivisorLFORate_Hz);
	mDivisorLFOTempoRate = mTempoRateTable.getScalar(getparameter_i(kDivisorLFORate_Sync));
	mDivisorLFO.setDepth(getparameter_scalar(kDivisorLFODepth));
	mDivisorLFO.setShape(getparameter_i(kDivisorLFOShape));
	mDivisorLFOTempoSync = getparameter_b(kDivisorLFOTempoSync);
	mBufferLFORateHz = getparameter_f(kBufferLFORate_Hz);
	mBufferLFOTempoRate = mTempoRateTable.getScalar(getparameter_i(kBufferLFORate_Sync));
	mBufferLFO.setDepth(getparameter_scalar(kBufferLFODepth));
	mBufferLFO.setShape(getparameter_i(kBufferLFOShape));
	mBufferLFOTempoSync = getparameter_b(kBufferLFOTempoSync);
	mSmoothPortion = getparameter_scalar(kSmooth);
	mPitchbendRange = getparameter_f(kPitchbend);
	mMidiMode = getparameter_i(kMidiMode);
	mUserTempoBPM = getparameter_f(kTempo);
	mUseHostTempo = getparameter_b(kTempoAuto);

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
