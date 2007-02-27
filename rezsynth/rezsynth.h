/*-------------- by Sophia Poirier  ][  January - March 2001 -------------*/

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
	virtual void processaudio(const float **in, float **out, unsigned long inNumFrames, bool replacing=true);


private:
	double processAmpEvener(int numBands, int currentNote);
	void processCoefficients(int *numbands, int currentNote);
	void processFilterOuts(const float *in, float *out, long sampleFrames, double ampEvener, 
						int currentNote, int numBands, double *prevOut, double *prevprevOut);
	void processUnaffected(const float *in, float *out, long sampleFrames);
	void checkForNewNote(long currentEvent, unsigned long numChannels);

	// parameters
	double bandwidth, sepAmount_octaval, sepAmount_linear, pitchbendRange;
	float attack, release, velCurve, velInfluence, gain, betweenGain, wetGain, dryWetMix;
	int numBands, sepMode, scaleMode, dryWetMixMode;
	bool legato, fades, foldover, wiseAmp;

	double *inputAmp;	// gains for the current sample input, for each band
	double *delay1amp;	// gains for the 1 sample delayed input, for each band
	double *delay2amp;	// gains for the 2 second delayed input, for each band
	double ***prevOutValue, ***prevprevOutValue;	// arrays of previous resonator output values
	unsigned long numBuffers;

	double twoPiDivSR, nyquist;	// values that are needed when calculating coefficients

	int unaffectedState, unaffectedFadeSamples;
};

#endif
