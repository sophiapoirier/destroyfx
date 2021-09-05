/*------------------------------------------------------------------------
Copyright (C) 2001-2021  Sophia Poirier

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

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "bufferoverride-base.h"
#include "bufferoverride.h"

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
	auto const unitTempoRateIndex = mTempoRateTable.getNearestTempoRateIndex(1.0f);
	initparameter_f(kDivisor, {"buffer divisor", "BufDiv", "BfDv"}, 1.92, 1.92, 1.92, 222.0, DfxParam::Unit::Divisor, DfxParam::Curve::Squared);
	initparameter_f(kBufferSize_MS, {"forced buffer size (free)", "BufSizF", "BufSzF", "BfSF"}, 90.0, 33.3, 1.0, 999.0, DfxParam::Unit::MS, DfxParam::Curve::Squared);
	initparameter_list(kBufferSize_Sync, {"forced buffer size (sync)", "BufSizS", "BufSzS", "BfSS"}, unitTempoRateIndex, unitTempoRateIndex, numTempoRates, DfxParam::Unit::Beats);
	initparameter_b(kBufferTempoSync, {"forced buffer tempo sync", "BufSync", "BufSnc", "BfSc"}, false, false);
	initparameter_b(kBufferInterrupt, {"buffer interrupt", "BufRupt", "BufInt", "BfIn"}, true, true);
	initparameter_f(kDivisorLFORate_Hz, {"divisor LFO rate (free)", "DvLFORF", "DLFORF", "DLRF"}, 0.3, 3.0, 0.03, 21.0, DfxParam::Unit::Hz, DfxParam::Curve::Squared);
	initparameter_list(kDivisorLFORate_Sync, {"divisor LFO rate (sync)", "DvLFORS", "DLFORS", "DLRS"}, unitTempoRateIndex, unitTempoRateIndex, numTempoRates, DfxParam::Unit::Beats);
	initparameter_f(kDivisorLFODepth, {"divisor LFO depth", "DvLFODp", "DvLFOD", "DvLD"}, 0.0, 0.0, 0.0, 100.0, DfxParam::Unit::Percent);
	initparameter_list(kDivisorLFOShape, {"divisor LFO shape", "DvLFOSh", "DLFOSh", "DLSh"}, 0, 0, dfx::LFO::kNumShapes);
	initparameter_b(kDivisorLFOTempoSync, {"divisor LFO tempo sync", "DvLFOSc", "DLFOSc", "DLSc"}, false, false);
	initparameter_f(kBufferLFORate_Hz, {"buffer LFO rate (free)", "BfLFORF", "BLFORF", "BLRF"}, 3.0, 3.0, 0.03, 21.0, DfxParam::Unit::Hz, DfxParam::Curve::Exp);//DfxParam::Curve::Squared);
	initparameter_list(kBufferLFORate_Sync, {"buffer LFO rate (sync)", "BfLFORS", "BLFORS", "BLRS"}, unitTempoRateIndex, unitTempoRateIndex, numTempoRates, DfxParam::Unit::Beats);
	initparameter_f(kBufferLFODepth, {"buffer LFO depth", "BfLFODp", "BfLFOD", "BfLD"}, 0.0, 0.0, 0.0, 100.0, DfxParam::Unit::Percent);
	initparameter_list(kBufferLFOShape, {"buffer LFO shape", "BfLFOSh", "BLFOSh", "BLSh"}, 0, 0, dfx::LFO::kNumShapes);
	initparameter_b(kBufferLFOTempoSync, {"buffer LFO tempo sync", "BfLFOSc", "BLFOSc", "BLSc"}, false, false);
	initparameter_f(kSmooth, {"smooth", "Smth"}, 9.0, 3.0, 0.0, 100.0, DfxParam::Unit::Percent);
	initparameter_f(kDryWetMix, dfx::MakeParameterNames(dfx::kParameterNames_DryWetMix), 100.0, 50.0, 0.0, 100.0, DfxParam::Unit::DryWetMix);
	initparameter_f(kPitchBendRange, dfx::MakeParameterNames(dfx::kParameterNames_PitchBendRange), 6.0, 3.0, 0.0, DfxMidi::kPitchBendSemitonesMax, DfxParam::Unit::Semitones);
	initparameter_list(kMidiMode, dfx::MakeParameterNames(dfx::kParameterNames_MidiMode), kMidiMode_Nudge, kMidiMode_Nudge, kNumMidiModes);
	initparameter_f(kTempo, dfx::MakeParameterNames(dfx::kParameterNames_Tempo), 120.0, 120.0, 57.0, 480.0, DfxParam::Unit::BPM);
	initparameter_b(kTempoAuto, dfx::MakeParameterNames(dfx::kParameterNames_TempoAuto), true, true);

	// set the value strings for the LFO shape parameters
	for (dfx::LFO::Shape i = 0; i < dfx::LFO::kNumShapes; i++)
	{
		auto const shapeName = dfx::LFO::getShapeName(i);
		setparametervaluestring(kDivisorLFOShape, i, shapeName);
		setparametervaluestring(kBufferLFOShape, i, shapeName);
	}
	// set the value strings for the sync rate parameters
	for (int i = 0; i < mTempoRateTable.getNumRates(); i++)
	{
		auto const& tempoRateName = mTempoRateTable.getDisplay(i);
		setparametervaluestring(kBufferSize_Sync, i, tempoRateName);
		setparametervaluestring(kDivisorLFORate_Sync, i, tempoRateName);
		setparametervaluestring(kBufferLFORate_Sync, i, tempoRateName);
	}
	// set the value strings for the MIDI mode parameter
	setparametervaluestring(kMidiMode, kMidiMode_Nudge, "nudge");
	setparametervaluestring(kMidiMode, kMidiMode_Trigger, "trigger");

	addparametergroup("buffer divisor LFO", {kDivisorLFORate_Hz, kDivisorLFORate_Sync, kDivisorLFODepth, kDivisorLFOShape, kDivisorLFOTempoSync});
	addparametergroup("forced buffer size LFO", {kBufferLFORate_Hz, kBufferLFORate_Sync, kBufferLFODepth, kBufferLFOShape, kBufferLFOTempoSync});

	settailsize_seconds(1.0 / (mTempoRateTable.getScalar(0) * kMinAllowableBPS));


	// give a value in case that's useful for a freshly opened GUI
	mCurrentTempoBPS = getparameter_f(kTempo) / 60.0;

	setpresetname(0, "self-determined");  // default preset name
	initPresets();

	registerSmoothedAudioValue(&mInputGain);
	registerSmoothedAudioValue(&mOutputGain);

	mViewDataCache_reader = &mViewDataCaches.front();
	mViewDataCache_writer = &mViewDataCaches.back();
}

//-------------------------------------------------------------------------
void BufferOverride::reset()
{
	// setting the values like this will restart the forced buffer in the next process()
	mCurrentForcedBufferSize = 1;
	mWritePos = mReadPos = 1;
	mMinibufferSize = 1;
    mMinibufferSizeForView = 1;
	mPrevMinibufferSize = 0;
	mSmoothCount = mSmoothDur = 0;
//	mSqrtFadeIn = mSqrtFadeOut = 1.0f;

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

	clearViewDataCache();
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
	mBuffers = {};
	mAudioOutputValues = {};
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
	mPitchBendRange = getparameter_f(kPitchBendRange);
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


#pragma mark _________properties_________

//-------------------------------------------------------------------------
void BufferOverride::clearViewDataCache()
{
	mViewDataCache_writer->clear();
	{
		std::unique_lock const guard(mViewDataCachesLock, std::try_to_lock);
		if (guard.owns_lock())
		{
			std::swap(mViewDataCache_reader, mViewDataCache_writer);
		}
	}
	mLastViewDataCacheTimestamp.fetch_add(1, std::memory_order_relaxed);
}

//-------------------------------------------------------------------------
void BufferOverride::updateViewDataCache()
{
	// XXX should be copying something computed from update
	mViewDataCache_writer->forced_buffer_samples = mCurrentForcedBufferSize;
	mViewDataCache_writer->mini_buffer_samples = mMinibufferSizeForView;

	bool const updated = [this]()
	{
		// willing to drop window cache updates to ensure realtime-safety
		// by not blocking here
		std::unique_lock const guard(mViewDataCachesLock, std::try_to_lock);
		if (guard.owns_lock())
		{
			std::swap(mViewDataCache_reader, mViewDataCache_writer);
			return true;
		}
		return false;
	}();

	if (updated)
	{
		mLastViewDataCacheTimestamp.fetch_add(1, std::memory_order_relaxed);
	}
}

//-------------------------------------------------------------------------
long BufferOverride::dfx_GetPropertyInfo(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned long inItemIndex,
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
long BufferOverride::dfx_GetProperty(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned long inItemIndex,
									 void* outData)
{
	switch (inPropertyID)
	{
		case kBOProperty_LastViewDataTimestamp:
			*static_cast<uint64_t*>(outData) = mLastViewDataCacheTimestamp.load(std::memory_order_relaxed);
			return dfx::kStatus_NoError;
		case kBOProperty_ViewData:
		{
			std::lock_guard const guard(mViewDataCachesLock);
			*static_cast<BufferOverrideViewData*>(outData) = *mViewDataCache_reader;
			return dfx::kStatus_NoError;
		}
		default:
			return DfxPlugin::dfx_GetProperty(inPropertyID, inScope, inItemIndex, outData);
	}
}
