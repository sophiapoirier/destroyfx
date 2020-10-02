/*------------------------------------------------------------------------
Copyright (C) 2001-2020  Sophia Poirier

This file is part of Rez Synth.

Rez Synth is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Rez Synth is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Rez Synth.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#pragma once


#include <array>
#include <vector>

#include "dfxplugin.h"
#include "dfxsmoothedvalue.h"


//----------------------------------------------------------------------------- 
// enums

// these are the plugin parameters:
enum
{
	kBandwidthAmount_Hz,
	kBandwidthAmount_Q,
	kBandwidthMode,
	kResonAlgorithm,
	kNumBands,
	kSepAmount_Octaval,
	kSepAmount_Linear,
	kSepMode,
	kFoldover,
	kEnvAttack,
	kEnvDecay,
	kEnvSustain,
	kEnvRelease,
	kFadeType,
	kLegato,
	kVelocityInfluence,
	kVelocityCurve,
	kPitchBendRange,
	kScaleMode,
	kWiseAmp,
	kFilterOutputGain,
	kBetweenGain,
	kDryWetMix,
	kDryWetMixMode,

	kNumParameters
};

// algorithm options for the resonant filter
enum
{
	kResonAlg_2PoleNoZero,
	kResonAlg_2Pole2ZeroR,
	kResonAlg_2Pole2Zero1,

	kNumResonAlgs
};

// bandwidth modes
enum
{
	kBandwidthMode_Hz,
	kBandwidthMode_Q,

	kNumBandwidthModes
};

// these are the input gain scaling modes
enum
{
	kScaleMode_None,
	kScaleMode_RMS,
	kScaleMode_Peak,

	kNumScaleModes
};

// these are the filter bank band separation modes
enum
{
	kSeparationMode_Octaval,
	kSeparationMode_Linear,

	kNumSeparationModes
};

// these are the dry/wet mixing modes
enum
{
	kDryWetMixMode_Linear,
	kDryWetMixMode_EqualPower,

	kNumDryWetMixModes
};


//----------------------------------------------------------------------------- 
class RezSynth final : public DfxPlugin
{
public:
	RezSynth(TARGET_API_BASE_INSTANCE_TYPE inInstance);

	long initialize() override;
	void reset() override;

	void createbuffers() override;
	void releasebuffers() override;
	void clearbuffers() override;

	void processparameters() override;
	void processaudio(float const* const* inAudio, float* const* outAudio, unsigned long inNumFrames) override;

private:
	static constexpr int64_t kMaxBands = 30;  // the maximum number of resonant bands
	static constexpr long kUnaffectedFadeDur = 18;
	static constexpr float kUnaffectedFadeStep = 1.0f / static_cast<float>(kUnaffectedFadeDur);
	static constexpr long kNumPresets = 16;

	// these are the 3 states of the unaffected audio input between notes
	enum class UnaffectedState
	{
		FadeIn,
		Flat,
		FadeOut
	};

	using ChannelsOfNotesOfBands = std::vector<std::array<std::array<double, kMaxBands>, DfxMidi::kNumNotesWithLegatoVoice>>;
	static void clearChannelsOfNotesOfBands(ChannelsOfNotesOfBands& channelsOfNotesOfBands);
	void clearFilterOutputForBands(int bandIndexBegin);

	double calculateAmpEvener(int currentNote) const;
	[[nodiscard]] int calculateCoefficients(int currentNote);
	void processFilterOuts(float const* inAudio, float* outAudio, unsigned long sampleFrames, 
						   int currentNote, int numBands, double& prevIn, double& prevprevIn, 
						   double* prevOut, double* prevprevOut);
	void processUnaffected(float const* inAudio, float* outAudio, unsigned long sampleFrames);
	double getBandwidthForFreq(double inFreq) const;
	void checkForNewNote(long currentEvent);

	// parameters
	double mBandwidthAmount_Hz = 1.0, mBandwidthAmount_Q = 1.0, mSepAmount_Octaval = 0.0, mSepAmount_Linear = 0.0;
	double mPitchBendRange = 0.0;
	float mAttack_Seconds = 0.0f, mDecay_Seconds = 0.0f, mSustain = 0.0f, mRelease_Seconds = 0.0f;
	float mVelocityCurve = 0.0f, mVelocityInfluence = 0.0f;
	dfx::SmoothedValue<double> mOutputGain;
	dfx::SmoothedValue<float> mBetweenGain;
	dfx::SmoothedValue<float> mDryGain, mWetGain;
	int mBandwidthMode {}, mNumBands = 1, mSepMode {}, mScaleMode {}, mResonAlgorithm {}, mDryWetMixMode {};
	DfxEnvelope::CurveType mFadeType {};
	bool mFoldover = false, mWiseAmp = false;
	std::array<dfx::SmoothedValue<double>, DfxMidi::kNumNotesWithLegatoVoice> mAmpEvener;

	std::array<dfx::SmoothedValue<double>, DfxMidi::kNumNotesWithLegatoVoice> mBaseFreq;
	std::array<std::array<dfx::SmoothedValue<double>, kMaxBands>, DfxMidi::kNumNotesWithLegatoVoice> mBandCenterFreq;
	std::array<std::array<dfx::SmoothedValue<double>, kMaxBands>, DfxMidi::kNumNotesWithLegatoVoice> mBandBandwidth;
	std::array<bool, DfxMidi::kNumNotesWithLegatoVoice> mNoteActiveLastRender {};

	std::array<double, kMaxBands> mInputAmp {};  // gains for the current sample input, for each band
	std::array<double, kMaxBands> mPrevOutCoeff {};  // coefficients for the 1-sample delayed ouput, for each band
	std::array<double, kMaxBands> mPrevPrevOutCoeff {};  // coefficients for the 2-sample delayed ouput, for each band
	std::array<double, kMaxBands> mPrevPrevInCoeff {};  // coefficients for the 2-sample delayed input, for each band
	ChannelsOfNotesOfBands mPrevOutValue, mPrevPrevOutValue;  // arrays of previous resonator output values
	std::vector<std::array<double, DfxMidi::kNumNotesWithLegatoVoice>> mPrevInValue, mPrevPrevInValue;  // arrays of previous audio input values

	double mPiDivSR = 0.0, mTwoPiDivSR = 0.0, mNyquist = 0.0;  // values that are needed when calculating coefficients

	UnaffectedState mUnaffectedState = UnaffectedState::FadeIn;
	int mUnaffectedFadeSamples = 0;
};
