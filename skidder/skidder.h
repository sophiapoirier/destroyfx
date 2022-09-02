/*------------------------------------------------------------------------
Copyright (C) 2000-2022  Sophia Poirier

This file is part of Skidder.

Skidder is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Skidder is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Skidder.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#pragma once


#include <array>
#include <memory>
#include <vector>

#include "dfxmath.h"
#include "dfxplugin.h"
#include "dfxsmoothedvalue.h"
#include "iirfilter.h"
#include "temporatetable.h"


//-----------------------------------------------------------------------------
// these are the plugin parameters:
enum
{
	kRate_Hz,
	kRate_Sync,
	kRateRandMin_Hz,
	kRateRandMin_Sync,
	kTempoSync,
	kPulsewidth,
	kPulsewidthRandMin,
	kSlope,
	kPan,
	kNoise,
	kMidiMode,
	kVelocity,
	kFloor,
	kFloorRandMin,
	kCrossoverFrequency,
	kCrossoverMode,
	kTempo,
	kTempoAuto,

	kNumParameters
};

//-----------------------------------------------------------------------------
// these are the MIDI note control modes:
enum
{
	kMidiMode_None,
	kMidiMode_Trigger,
	kMidiMode_Apply,
	kNumMidiModes
};

//-----------------------------------------------------------------------------
enum
{
	kCrossoverMode_All,
	kCrossoverMode_Low,
	kCrossoverMode_High,
	kNumCrossoverModes
};



//-----------------------------------------------------------------------------
class Skidder final : public DfxPlugin
{
public:
	explicit Skidder(TARGET_API_BASE_INSTANCE_TYPE inInstance);

	void dfx_PostConstructor() override;

	long initialize() override;
	void cleanup() override;
	void reset() override;

	void processparameters() override;
	void processaudio(float const* const* inAudio, float* const* outAudio, size_t inNumFrames) override;

	// stuff for extending DfxSettings
	void settings_doLearningAssignStuff(long tag, dfx::MidiEventType eventType, int eventChannel,
										int eventNum, size_t offsetFrames, int eventNum2 = 0,
										dfx::MidiEventBehaviorFlags eventBehaviourFlags = dfx::kMidiEventBehaviorFlag_None,
										int data1 = 0, int data2 = 0, float fdata1 = 0.0f, float fdata2 = 0.0f) override;
//	void settings_unassignParam(long tag) override;

	// true for unified single-point automation of both parameter range values
	bool mRateDoubleAutomate = false, mPulsewidthDoubleAutomate = false, mFloorDoubleAutomate = false;  // TODO: not public, or DELETE?


private:
	static constexpr size_t kNumPresets = 16;

	// these are the states of the process:
	enum class SkidState
	{
		SlopeIn,
		Plateau,
		SlopeOut,
		Valley
	};

	void processSlopeIn();
	void processPlateau();
	void processSlopeOut();
	void processValley();
	float processOutput(float in1, float in2, float panGain);
	void processMidiNotes();

	void noteOn(size_t offsetFrames);
	void noteOff();
	bool isAnyNoteOn() const;
	void resetMidi();

	double getCrossoverFrequency() const;

	// the parameters
	double mRate_Hz = 1., mRate_Sync = 1.;
	float mPulsewidth = 0.f, mPulsewidthRandMin = 0.f;
	long mRateIndex = 0, mRateRandMinIndex = 0;
	float mPanWidth = 0.0f, mFloor = 0.0f;
	dfx::SmoothedValue<float> mNoise;
	double mSlopeSeconds = 0., mUserTempo = 1.;
	dfx::SmoothedValue<double> mCrossoverFrequency_gen {0.060};
	long mCrossoverMode {}, mMidiMode {};
	bool mTempoSync = false, mUseHostTempo = false, mUseVelocity = false;

	float mGainRange = 0.0f;  // a scaled version of mFloor and the difference between that and 1.0
	float mRandomFloor = 0.0f, mRandomGainRange = 0.0f;
	// generic versions of these parameters for curved randomization
	double mRateHz_gen = 0., mRateRandMinHz_gen = 0.;
	float mFloor_gen = 0.f, mFloorRandMin_gen = 0.f;
	bool mUseRandomRate = false, mUseRandomPulsewidth = false, mUseRandomFloor = false;
	float mSampleAmp = 0.0f;  // output sample scalar
	long mCycleSamples = 1, mPulseSamples = 1, mSlopeSamples = 1, mSlopeDur = 1, mPlateauSamples = 1, mValleySamples = 0;  // sample counters
	float mSlopeStep = 0.0f;  // the scalar for each step of the fade during a slope in or out
	float mPanGainL = 0.0f, mPanGainR = 0.0f;  // the actual pan gain values for each stereo channel during each cycle
	SkidState mState {};  // the state of the process
	double mRMS = 0.0;
	long mRMSCount = 0;
	dfx::math::RandomEngine mRandomEngine {dfx::math::RandomSeed::Entropic};

	double mCurrentTempoBPS = 1.0;  // tempo in beats per second
	double mOldTempoBPS = 1.0;  // holds the previous value of currentTempoBPS for comparison (TODO: unused?)
	bool mNeedResync = false;  // true when playback has just started up again
	dfx::TempoRateTable const mTempoRateTable;

	std::unique_ptr<dfx::Crossover> mCrossover;

	int mMostRecentVelocity = 0;  // the velocity of the most recently played note
	std::array<int, DfxMidi::kNumNotes> mNoteTable {};
	long mWaitSamples = 0;
	bool mMidiIn = false, mMidiOut = false;  // set when notes start or stop so that the floor goes to 0.0

	std::vector<std::vector<float>> mEffectualInputAudioBuffers;
	std::vector<float const*> mInputAudio;
	std::vector<float*> mOutputAudio;
	std::vector<float> mAsymmetricalInputAudioBuffer;
};
