/*------------------------------------------------------------------------
Copyright (C) 2001-2010  Sophia Poirier

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

#ifndef __REZ_SYNTH_H
#define __REZ_SYNTH_H


#include "dfxplugin.h"


//----------------------------------------------------------------------------- 
// enums

// these are the plugin parameters:
enum
{
	kBandwidth,
	kNumBands,
	kSepAmount_octaval,
	kSepAmount_linear,
	kSepMode,
	kFoldover,
	kAttack,
	kRelease,
	kFades,
	kLegato,
	kVelInfluence,
	kVelCurve,
	kPitchbendRange,
	kScaleMode,
	kGain,
	kBetweenGain,
	kDryWetMix,
	kDryWetMixMode,
	kWiseAmp,
	kResonAlgorithm,

	kNumParameters
};

// these are the input gain scaling modes
enum
{
	kScaleMode_none,
	kScaleMode_rms,
	kScaleMode_peak,

	kNumScaleModes
};

// algorithm options for the resonant filter
enum
{
	kResonAlg_2poleNoZero,
	kResonAlg_2pole2zeroR,
	kResonAlg_2pole2zero1,

	kNumResonAlgs
};

// these are the filter bank band separation modes
enum
{
	kSepMode_octaval,
	kSepMode_linear,

	kNumSepModes
};

// these are the dry/wet mixing modes
enum
{
	kDryWetMixMode_linear,
	kDryWetMixMode_equalpower,

	kNumDryWetMixModes
};

// these are the 3 states of the unaffected audio input between notes
enum
{
	kUnaffectedState_FadeIn,
	kUnaffectedState_Flat,
	kUnaffectedState_FadeOut
};

//----------------------------------------------------------------------------- 
// constants and macros

const int kMaxBands = 30;	// the maximum number of resonant bands

const long kUnaffectedFadeDur = 18;
const float kUnaffectedFadeStep = 1.0f / (float)kUnaffectedFadeDur;

const long kNumPresets = 16;


//----------------------------------------------------------------------------- 
class RezSynth : public DfxPlugin
{
public:
	RezSynth(TARGET_API_BASE_INSTANCE_TYPE inInstance);
	virtual ~RezSynth();

	virtual long initialize();
	virtual void cleanup();
	virtual void reset();

	virtual bool createbuffers();
	virtual void releasebuffers();
	virtual void clearbuffers();

	virtual void processparameters();
	virtual void processaudio(const float ** in, float ** out, unsigned long inNumFrames, bool replacing=true);


private:
	double calculateAmpEvener(int numBands, int currentNote);
	void calculateCoefficients(int * numbands, int currentNote);
	void processFilterOuts(const float * in, float * out, long sampleFrames, double ampEvener, 
						int currentNote, int numBands, double & prevIn, double & prevprevIn, 
						double * prevOut, double * prevprevOut);
	void processUnaffected(const float * in, float * out, long sampleFrames);
	void checkForNewNote(long currentEvent, unsigned long numChannels);

	// parameters
	double bandwidth, sepAmount_octaval, sepAmount_linear, pitchbendRange;
	float attack, release, velCurve, velInfluence, gain, betweenGain, wetGain, dryWetMix;
	int numBands, sepMode, scaleMode, resonAlgorithm, dryWetMixMode;
	bool legato, fades, foldover, wiseAmp;

	double * inputAmp;	// gains for the current sample input, for each band
	double * prevOutCoeff;	// coefficients for the 1-sample delayed ouput, for each band
	double * prevprevOutCoeff;	// coefficients for the 2-sample delayed ouput, for each band
	double * prevprevInCoeff;	// coefficients for the 2-sample delayed input, for each band
	double *** prevOutValue, *** prevprevOutValue;	// arrays of previous resonator output values
	double ** prevInValue, ** prevprevInValue;	// arrays of previous audio input values
	unsigned long numBuffers;

	double piDivSR, twoPiDivSR, nyquist;	// values that are needed when calculating coefficients

	int unaffectedState, unaffectedFadeSamples;
};

#endif
