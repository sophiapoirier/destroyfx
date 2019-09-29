/*------------------------------------------------------------------------
Copyright (C) 2002-2019  Sophia Poirier

This file is part of Scrubby.

Scrubby is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Scrubby is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Scrubby.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "scrubby.h"

#include <algorithm>
#include <array>
#include <cmath>


#pragma mark init

// this macro does boring entry point stuff for us
DFX_EFFECT_ENTRY(Scrubby)

//-----------------------------------------------------------------------------
// initializations and such
Scrubby::Scrubby(TARGET_API_BASE_INSTANCE_TYPE inInstance)
:	DfxPlugin(inInstance, kNumParameters, kNumPresets)
{
	mActiveNotesTable.fill(0);
	mPitchSteps.fill(false);

	// initialize the parameters
	auto const numTempoRates = mTempoRateTable.getNumRates();
	auto const unitTempoRateIndex = mTempoRateTable.getNearestTempoRateIndex(1.0f);
	initparameter_f(kSeekRange, "seek range", 333.0, 333.0, 0.3, 6000.0, DfxParam::Unit::MS, DfxParam::Curve::Squared);
	initparameter_b(kFreeze, "freeze", false, false);
	initparameter_f(kSeekRate_Hz, "seek rate (free)", 9.0, 3.0, 0.3, 810.0, DfxParam::Unit::Hz, DfxParam::Curve::Log);//DfxParam::Curve::Cubed
	initparameter_list(kSeekRate_Sync, "seek rate (sync)", unitTempoRateIndex, unitTempoRateIndex, numTempoRates, DfxParam::Unit::Beats);
//	initparameter_f(kSeekRateRandMin_Hz, "seek rate rand min (free)", 9.0, 3.0, 0.3, 810.0, DfxParam::Unit::Hz, DfxParam::Curve::Log);//DfxParam::Curve::Cubed
//	initparameter_list(kSeekRateRandMin_Sync, "seek rate rand min (sync)", unitTempoRateIndex, unitTempoRateIndex, numTempoRates, DfxParam::Unit::Beats);
// XXX temporary while implementing range sliders in DFX GUI
initparameter_f(kSeekRateRandMin_Hz, "seek rate rand min (free)", 810.0, 3.0, 0.3, 810.0, DfxParam::Unit::Hz, DfxParam::Curve::Log);
initparameter_list(kSeekRateRandMin_Sync, "seek rate rand min (sync)", numTempoRates - 1, unitTempoRateIndex, numTempoRates, DfxParam::Unit::Beats);
	initparameter_b(kTempoSync, "tempo sync", false, false);
	initparameter_f(kSeekDur, "seek duration", 100.0, 100.0, 3.0, 100.0, DfxParam::Unit::Percent);  // percent of range
	initparameter_f(kSeekDurRandMin, "seek dur rand min", 100.0, 100.0, 3.0, 100.0, DfxParam::Unit::Percent);  // percent of range
	initparameter_list(kSpeedMode, "speeds", kSpeedMode_Robot, kSpeedMode_Robot, kNumSpeedModes);
	initparameter_b(kSplitChannels, "channels split", false, false);
	initparameter_b(kPitchConstraint, "pitch constraint", false, false);
	// default all notes to off (looks better on the GUI)
	// no, I changed my mind, at least leave 1 note on so that the user isn't 
	// confused the first time turning on pitch constraint and getting silence
	initparameter_b(kPitchStep0, "semi0 (unity/octave)", true, false);
	initparameter_b(kPitchStep1, "semi1 (minor 2nd)", false, false);
	initparameter_b(kPitchStep2, "semi2 (major 2nd)", false, false);
	initparameter_b(kPitchStep3, "semi3 (minor 3rd)", false, false);
	initparameter_b(kPitchStep4, "semi4 (major 3rd)", false, false);
	initparameter_b(kPitchStep5, "semi5 (perfect 4th)", false, false);
	initparameter_b(kPitchStep6, "semi6 (augmented 4th)", false, false);
	initparameter_b(kPitchStep7, "semi7 (perfect 5th)", false, false);
	initparameter_b(kPitchStep8, "semi8 (minor 6th)", false, false);
	initparameter_b(kPitchStep9, "semi9 (major 6th)", false, false);
	initparameter_b(kPitchStep10, "semi10 (minor 7th)", false, false);
	initparameter_b(kPitchStep11, "semi11 (major 7th)", false, false);
	initparameter_i(kOctaveMin, "octave minimum", kOctave_MinValue, kOctave_MinValue, kOctave_MinValue, 0, DfxParam::Unit::Octaves);
	setparameterusevaluestrings(kOctaveMin, true);
	initparameter_i(kOctaveMax, "octave maximum", kOctave_MaxValue, kOctave_MaxValue, 0, kOctave_MaxValue, DfxParam::Unit::Octaves);
	setparameterusevaluestrings(kOctaveMax, true);
	initparameter_f(kTempo, "tempo", 120.0, 120.0, 39.0, 480.0, DfxParam::Unit::BPM);
	initparameter_b(kTempoAuto, "sync to host tempo", true, true);
	initparameter_f(kPredelay, "predelay", 0.0, 50.0, 0.0, 100.0, DfxParam::Unit::Percent);  // percent of range

	// set the value strings for the sync rate parameters
	for (long i = 0; i < numTempoRates; i++)
	{
		auto const& tempoRateName = mTempoRateTable.getDisplay(i);
		setparametervaluestring(kSeekRate_Sync, i, tempoRateName.c_str());
		setparametervaluestring(kSeekRateRandMin_Sync, i, tempoRateName.c_str());
	}
	// set the value strings for the speed modes
	setparametervaluestring(kSpeedMode, kSpeedMode_Robot, "robot");
	setparametervaluestring(kSpeedMode, kSpeedMode_DJ, "DJ");
	// set the value strings for the octave range parameters
	for (long i = getparametermin_i(kOctaveMin) + 1; i <= getparametermax_i(kOctaveMin); i++)
	{
		setparametervaluestring(kOctaveMin, i, std::to_string(i).c_str());
	}
	setparametervaluestring(kOctaveMin, getparametermin_i(kOctaveMin), "no min");
	for (long i = getparametermin_i(kOctaveMax); i < getparametermax_i(kOctaveMax); i++)
	{
		auto octaveName = std::to_string(i);
		if (i > 0)
		{
			octaveName.insert(octaveName.begin(), '+');
		}
		setparametervaluestring(kOctaveMax, i, octaveName.c_str());
	}
	setparametervaluestring(kOctaveMax, getparametermax_i(kOctaveMax), "no max");

	addparameterattributes(kFreeze, DfxParam::kAttribute_OmitFromRandomizeAll);


	settailsize_seconds(getparametermax_f(kSeekRange) * 0.001 / getparametermin_f(kSeekRate_Hz));

	setpresetname(0, "scub");  // default preset name
	initPresets();

	// give mCurrentTempoBPS a value in case that's useful for a freshly opened GUI
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
void Scrubby::reset()
{
	// reset these position trackers thingies and whatnot
	mWritePos = 0;

	// delete any stored active notes
	for (size_t i = 0; i < mActiveNotesTable.size(); i++)
	{
		if (std::exchange(mActiveNotesTable[i], 0) > 0)
		{
			setparameter_b(i + kPitchStep0, false);
			postupdate_parameter(i + kPitchStep0);
		}
	}

mSineCount = 0;
}

//-----------------------------------------------------------------------------
void Scrubby::createbuffers()
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
}

//-----------------------------------------------------------------------------
void Scrubby::releasebuffers()
{
	mAudioBuffers.clear();
	mReadPos.clear();
	mReadStep.clear();
	mPortamentoStep.clear();
	mMoveCount.clear();
	mSeekCount.clear();
	mNeedResync.clear();
}

//-----------------------------------------------------------------------------
void Scrubby::clearbuffers()
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
}


#pragma mark presets

/*
//----------------------------------------------------------------------------- 
ScrubbyChunk::ScrubbyChunk(long numParameters, long numPrograms, long magic, AudioEffectX* effect)
:	VstChunk(numParameters, numPrograms, magic, effect)
{
	// start off with split CC automation of both range slider points
	seekRateDoubleAutomate = seekDurDoubleAutomate = false;
}

//----------------------------------------------------------------------------- 
// this gets called when Scrubby automates a parameter from CC messages.
// this is where we can link parameter automation for range slider points.
void ScrubbyChunk::doLearningAssignStuff(long tag, long eventType, long eventChannel, 
										 long eventNum, long delta, long eventNum2, 
										 long eventBehaviourFlags, 
										 long data1, long data2, float fdata1, float fdata2)
{
	if (getSteal())
	{
		return;
	}

	switch (tag)
	{
		case kSeekRate:
			if (seekRateDoubleAutomate)
			{
				assignParam(kSeekRateRandMin, eventType, eventChannel, eventNum, eventNum2, 
							eventBehaviourFlags, data1, data2, fdata1, fdata2);
			}
			break;
		case kSeekRateRandMin:
			if (seekRateDoubleAutomate)
			{
				assignParam(kSeekRate, eventType, eventChannel, eventNum, eventNum2, 
							eventBehaviourFlags, data1, data2, fdata1, fdata2);
			}
			break;
		case kSeekDur:
			if (seekDurDoubleAutomate)
			{
				assignParam(kSeekDurRandMin, eventType, eventChannel, eventNum, eventNum2, 
							eventBehaviourFlags, data1, data2, fdata1, fdata2);
			}
			break;
		case kSeekDurRandMin:
			if (seekDurDoubleAutomate)
			{
				assignParam(kSeekDur, eventType, eventChannel, eventNum, eventNum2, 
							eventBehaviourFlags, data1, data2, fdata1, fdata2);
			}
			break;
		default:
			break;
	}
}

//----------------------------------------------------------------------------- 
void ScrubbyChunk::unassignParam(long tag)
{
	VstChunk::unassignParam(tag);

	switch (tag)
	{
		case kSeekRate:
			if (seekRateDoubleAutomate)
			{
				VstChunk::unassignParam(kSeekRateRandMin);
			}
			break;
		case kSeekRateRandMin:
			if (seekRateDoubleAutomate)
			{
				VstChunk::unassignParam(kSeekRate);
			}
			break;
		case kSeekDur:
			if (seekDurDoubleAutomate)
			{
				VstChunk::unassignParam(kSeekDurRandMin);
			}
			break;
		case kSeekDurRandMin:
			if (seekDurDoubleAutomate)
			{
				VstChunk::unassignParam(kSeekDur);
			}
			break;
		default:
			break;
	}
}
*/


//-------------------------------------------------------------------------
void Scrubby::initPresets()
{
	long i = 1;

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
	i++;

	setpresetname(i, "blib");
	setpresetparameter_f(i, kSeekRange, 270.0);
	setpresetparameter_b(i, kFreeze, false);
	setpresetparameter_f(i, kSeekRate_Hz, 420.0);
	setpresetparameter_f(i, kSeekRateRandMin_Hz, 7.2);
	setpresetparameter_b(i, kTempoSync, false);
	setpresetparameter_f(i, kSeekDur, 57.0);//5700.0);
	setpresetparameter_f(i, kSeekDurRandMin, 30.0);//3000.0);
	setpresetparameter_i(i, kSpeedMode, kSpeedMode_Robot);
	setpresetparameter_b(i, kSplitChannels, false);
	setpresetparameter_b(i, kPitchConstraint, false);
	i++;

	setpresetname(i, "DJ staccato");
	setpresetparameter_f(i, kSeekRange, 1800.0);
	setpresetparameter_b(i, kFreeze, false);
	setpresetparameter_i(i, kSeekRate_Sync, mTempoRateTable.getNearestTempoRateIndex(2.0f));
	setpresetparameter_i(i, kSeekRateRandMin_Sync, mTempoRateTable.getNearestTempoRateIndex(2.0f));
	setpresetparameter_b(i, kTempoSync, true);
	setpresetparameter_f(i, kSeekDur, 22.2);
	setpresetparameter_f(i, kSeekDurRandMin, 22.2);
	setpresetparameter_i(i, kSpeedMode, kSpeedMode_DJ);
	setpresetparameter_b(i, kSplitChannels, false);
	setpresetparameter_b(i, kPitchConstraint, false);
	setpresetparameter_b(i, kTempoAuto, true);
	i++;

/*
	setpresetname(i, "");
	setpresetparameter_f(i, kSeekRange, );
	setpresetparameter_b(i, kFreeze, );
	setpresetparameter_f(i, kSeekRate_Hz, );
	setpresetparameter_i(i, kSeekRate_Sync, mTempoRateTable.getNearestTempoRateIndex(f));
	setpresetparameter_f(i, kSeekRateRandMin_Hz, );
	setpresetparameter_i(i, kSeekRateRandMin_Sync, mTempoRateTable.getNearestTempoRateIndex(f));
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
	i++;
*/
}


#pragma mark parameters

//-------------------------------------------------------------------------
void Scrubby::processparameters()
{
	mSeekRangeSeconds = getparameter_f(kSeekRange) * 0.001;
	mFreeze = getparameter_b(kFreeze);
	mSeekRateHz = getparameter_f(kSeekRate_Hz);
	mSeekRateIndex = getparameter_i(kSeekRate_Sync);
	mSeekRateSync = mTempoRateTable.getScalar(mSeekRateIndex);
	auto const seekRateRandMinHz = getparameter_f(kSeekRateRandMin_Hz);
	mSeekRateRandMinIndex = getparameter_i(kSeekRateRandMin_Sync);
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
	for (size_t i = 0; i < mPitchSteps.size(); i++)
	{
		mPitchSteps[i] = getparameter_b(i + kPitchStep0);
	}

	// get the "generic" values of these parameters for randomization
	mSeekRateHz_gen = getparameter_gen(kSeekRate_Hz);
	mSeekRateRandMinHz_gen = getparameter_gen(kSeekRateRandMin_Hz);

	bool tempNeedResync = false;
	if (getparameterchanged(kSeekRate_Sync))
	{
		// make sure the cycles match up if the tempo rate has changed
		tempNeedResync = true;
	}
	if (getparameterchanged(kTempoSync) && mTempoSync)
	{
		// set needResync true if tempo sync mode has just been switched on
		tempNeedResync = true;
	}
	if (getparameterchanged(kPredelay))
	{
		// tell the host what the length of delay compensation should be
		setlatency_seconds(mSeekRangeSeconds * getparameter_scalar(kPredelay));
	#ifdef TARGET_API_VST
		// this tells the host to call a suspend()-resume() pair, 
		// which updates initialDelay value
		setlatencychanged(true);
	#endif
	}
	for (size_t i = 0; i < mActiveNotesTable.size(); i++)
	{
		if (getparameterchanged(i + kPitchStep0))
		{
			// reset the associated note in the notes table; manual changes override MIDI
			mActiveNotesTable[i] = 0;
		}
	}
	std::fill(mNeedResync.begin(), mNeedResync.end(), tempNeedResync);

	mUseSeekRateRandMin = mTempoSync ? (seekRateRandMinSync < mSeekRateSync) : (seekRateRandMinHz < mSeekRateHz);
	mUseSeekDurRandMin = (mSeekDurRandMin < mSeekDur);
}
