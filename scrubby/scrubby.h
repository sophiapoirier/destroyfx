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

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#pragma once


#include <array>
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
	kSeekRange,
	kFreeze,

	kSeekRate_Hz,
	kSeekRate_Sync,
	kSeekRateRandMin_Hz,
	kSeekRateRandMin_Sync,
	kTempoSync,
	kSeekDur,
	kSeekDurRandMin,

	kSpeedMode,
	kSplitChannels,

	kPitchConstraint,
	kPitchStep0,
	kPitchStep1,
	kPitchStep2,
	kPitchStep3,
	kPitchStep4,
	kPitchStep5,
	kPitchStep6,
	kPitchStep7,
	kPitchStep8,
	kPitchStep9,
	kPitchStep10,
	kPitchStep11,
	kOctaveMin,
	kOctaveMax,

	kTempo,
	kTempoAuto,
	kPredelay,

	kDryLevel,
	kWetLevel,

	kNumParameters
};


//-----------------------------------------------------------------------------
// constants

constexpr long kOctave_MinValue = -5;
constexpr long kOctave_MaxValue = 7;

// the number of semitones in an octave
constexpr size_t kNumPitchSteps = 12;

#define USE_LINEAR_ACCELERATION 0

enum
{
	kSpeedMode_Robot,
	kSpeedMode_DJ,
	kNumSpeedModes
};


//-----------------------------------------------------------------------------
class Scrubby final : public DfxPlugin
{
public:
	explicit Scrubby(TARGET_API_BASE_INSTANCE_TYPE inInstance);

	void dfx_PostConstructor() override;

	long initialize() override;
	void cleanup() override;
	void reset() override;

	void randomizeparameters() override;

	void processparameters() override;
	void processaudio(float const* const* inAudio, float* const* outAudio, size_t inNumFrames) override;


private:
	static constexpr size_t kNumPresets = 16;
	static constexpr double kHighpassFilterCutoff = 39.;


	void initPresets();

	void generateNewTarget(size_t channel);
	double processPitchConstraint(double readStep) const;
	void checkTempoSyncStuff();
	void processMidiNotes();

	// the parameters
	double mSeekRangeSeconds = 0.0, mSeekDur = 0.0, mSeekDurRandMin = 0.0;
	double mSeekRateHz = 0.0, mSeekRateSync = 0.0;
	long mSeekRateIndex = 0, mSeekRateRandMinIndex = 0;
	double mUserTempo = 0.0;
	long mSpeedMode = kSpeedMode_Robot, mOctaveMin = 0, mOctaveMax = 0;
	bool mFreeze = false, mSplitChannels = false, mPitchConstraint = false, mTempoSync = false, mUseHostTempo = false;
	std::array<bool, kNumPitchSteps> mPitchSteps {};
	dfx::SmoothedValue<float> mInputGain, mOutputGain;

	// generic versions of these parameters for curved randomization
	double mSeekRateHz_gen = 0.0, mSeekRateRandMinHz_gen = 0.0;

	bool mUseSeekRateRandMin = false, mUseSeekDurRandMin = false;

	// buffers and associated position values/counters/etc.
	std::vector<std::vector<float>> mAudioBuffers;
	long mWritePos = 0;
	std::vector<double> mReadPos, mReadStep, mPortamentoStep;
	std::vector<long> mMoveCount, mSeekCount;

	long mMaxAudioBufferSize = 0;  // the maximum size (in samples) of the audio buffer
	double mMaxAudioBufferSize_f = 0.0;  // for avoiding casting

	std::vector<dfx::IIRFilter> mHighpassFilters;

	dfx::math::RandomEngine mRandomEngine {dfx::math::RandomSeed::Entropic};

	// tempo sync stuff
	double mCurrentTempoBPS = 0.0;  // tempo in beats per second
	std::vector<bool> mNeedResync;   // true when playback has just started up again
	dfx::TempoRateTable const mTempoRateTable;

	// MIDI note control stuff
	std::array<long, kNumPitchSteps> mActiveNotesTable {};  // how many voices of each note in the octave are being played
	bool mNotesWereAlreadyActive = false;  // says whether any notes were active in the previous block

long mSineCount = 0;
};
