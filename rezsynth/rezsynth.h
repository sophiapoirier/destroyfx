/*-------------- by Marc Poirier  ][  January - March 2001 -------------*/

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

	NUM_PARAMETERS
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
	unFadeIn,
	unFlat,
	unFadeOut
};

//----------------------------------------------------------------------------- 
// constants & macros


const double BANDWIDTH_MIN = 0.1;
const double BANDWIDTH_MAX = 300.0;
#define bandwidthScaled(A)   ( paramRangeSquaredScaled(((double)(A)), BANDWIDTH_MIN, BANDWIDTH_MAX) )
#define bandwidthUnscaled(A)   ( paramRangeSquaredUnscaled((A), BANDWIDTH_MIN, BANDWIDTH_MAX) )

const int MAX_BANDS = 30;	// the maximum number of resonant bands
#define numBandsScaled(A)   ( paramRangeIntScaled((A), 1, MAX_BANDS) )
#define numBandsUnscaled(A)   ( paramRangeIntUnscaled((A), 1, MAX_BANDS) )

const double MAX_SEP_AMOUNT = 3.0;
#define sepModeBool(A) ((A) < 0.12f)

const float ATTACK_MAX = 3.0f;
const float RELEASE_MAX = 3.0f;
#define attackScaled(A)   ((A)*(A)*ATTACK_MAX)
#define releaseScaled(A) ((A)*(A)*RELEASE_MAX)

const float VELCURVE_MIN = 0.3f;
const float VELCURVE_MAX = 3.0f;
#define velCurveScaled(A)   ( paramRangeScaled((A), VELCURVE_MIN, VELCURVE_MAX) )
#define velCurveUnscaled(A)   ( paramRangeUnscaled((A), VELCURVE_MIN, VELCURVE_MAX) )

#define gainScaled(A)        ((A)*(A)*(A)*3.981f)
#define betweenGainScaled(A) ((A)*(A)*(A)*3.981f)

const long UNAFFECTED_FADE_DUR = 18;
const float UNAFFECTED_FADE_STEP = 1.0f / (float)UNAFFECTED_FADE_DUR;

const long NUM_PRESETS = 16;


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
