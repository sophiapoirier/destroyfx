/*---------------------------------------------------------------
   © 2001, Marcberg Soft und Hard GmbH, All Rights Perversed
---------------------------------------------------------------*/

#ifndef __REZSYNTH_H
#include "rezsynth.hpp"
#endif

#if TARGET_API_VST && TARGET_PLUGIN_HAS_GUI
	#ifndef __REZSYNTHEDITOR_H
	#include "rezsyntheditor.hpp"
	#endif
#endif


// this macro does boring entry point stuff for us
DFX_ENTRY(RezSynth);

//-----------------------------------------------------------------------------------------
// initializations
RezSynth::RezSynth(TARGET_API_BASE_INSTANCE_TYPE inInstance)
	: DfxPlugin(inInstance, NUM_PARAMETERS, NUM_PRESETS)	// 19 parameters, 16 presets
{
	initparameter_d(kBandwidth, "bandwidth", 3.0, 3.0, 0.1, 300.0, kDfxParamUnit_hz, kDfxParamCurve_squared);
	initparameter_i(kNumBands, "# of bands", 1, 1, 1, 30, kDfxParamUnit_quantity, kDfxParamCurve_stepped);
	initparameter_d(kSepAmount_octaval, "band separation (octaval)", 12.0, 12.0, 0.0, 36.0, kDfxParamUnit_semitones);
	initparameter_d(kSepAmount_linear, "band separation (linear)", 1.0, 1.0, 0.0, 3.0, kDfxParamUnit_scalar);	// % of center frequency
	initparameter_indexed(kSepMode, "separation mode", kSepMode_octaval, kSepMode_octaval, kNumSepModes);
	initparameter_b(kFoldover, "mistakes", true, false);
	initparameter_f(kAttack, "attack", 3.0f, 3.0f, 0.0f, 3000.0f, kDfxParamUnit_ms, kDfxParamCurve_squared);
	initparameter_f(kRelease, "release", 300.0f, 300.0f, 0.0f, 3000.0f, kDfxParamUnit_ms, kDfxParamCurve_squared);
	initparameter_b(kFades, "nicer fades", false, false);
	initparameter_b(kLegato, "legato", false, false);
//	initparameter_f(kVelInfluence, "velocity influence", 0.6f, 1.0f, 0.0f, 1.0f, kDfxParamUnit_portion);
	initparameter_f(kVelInfluence, "velocity influence", 60.0f, 100.0f, 0.0f, 100.0f, kDfxParamUnit_percent);
	initparameter_f(kVelCurve, "velocity curve", 2.0f, 1.0f, 0.3f, 3.0f, kDfxParamUnit_exponent);
	initparameter_d(kPitchbendRange, "pitchbend range", 3.0, 3.0, 0.0, PITCHBEND_MAX, kDfxParamUnit_semitones);
	initparameter_indexed(kScaleMode, "input gain mode", kScaleMode_rms, kScaleMode_none, kNumScaleModes);
	initparameter_f(kGain, "output gain", 1.0f, 1.0f, 0.0f, 3.981f, kDfxParamUnit_lineargain, kDfxParamCurve_cubed);
	initparameter_f(kBetweenGain, "between gain", 0.0f, 1.0f, 0.0f, 3.981f, kDfxParamUnit_lineargain, kDfxParamCurve_cubed);
	initparameter_f(kDryWetMix, "dry/wet mix", 100.0f, 50.0f, 0.0f, 100.0f, kDfxParamUnit_drywetmix);
	initparameter_indexed(kDryWetMixMode, "dry/wet mix mode", kDryWetMixMode_equalpower, kDryWetMixMode_linear, kNumDryWetMixModes);
	initparameter_b(kWiseAmp, "careful", true, true);

	setparametervaluestring(kSepMode, kSepMode_octaval, "octaval");
	setparametervaluestring(kSepMode, kSepMode_linear, "linear");
	setparametervaluestring(kScaleMode, kScaleMode_none, "no scaling");
	setparametervaluestring(kScaleMode, kScaleMode_rms, "RMS normalize");
	setparametervaluestring(kScaleMode, kScaleMode_peak, "peak normalize");
	setparametervaluestring(kDryWetMixMode, kDryWetMixMode_linear, "linear");
	setparametervaluestring(kDryWetMixMode, kDryWetMixMode_equalpower, "equal power");
//	setparametervaluestring(kFades, 0, "cheap");
//	setparametervaluestring(kFades, 1, "nicer");
//	setparametervaluestring(kFoldover, 0, "resist");
//	setparametervaluestring(kFoldover, 1, "allow");


	// allocate memory for these arrays
	delay1amp = (double*) malloc(MAX_BANDS * sizeof(double));
	delay2amp = (double*) malloc(MAX_BANDS * sizeof(double));
	inputAmp = (double*) malloc(MAX_BANDS * sizeof(double));
	//
	// some 2-dimensional arrays
	prevOutValue = (double**) malloc(NUM_NOTES * sizeof(double*));
	prevprevOutValue = (double**) malloc(NUM_NOTES * sizeof(double*));
	prevOut2Value = (double**) malloc(NUM_NOTES * sizeof(double*));
	prevprevOut2Value = (double**) malloc(NUM_NOTES * sizeof(double*));
	for (int i = 0; i < NUM_NOTES; i++)
	{
		prevOutValue[i] = (double*) malloc(MAX_BANDS * sizeof(double));
		prevprevOutValue[i] = (double*) malloc(MAX_BANDS * sizeof(double));
		prevOut2Value[i] = (double*) malloc(MAX_BANDS * sizeof(double));
		prevprevOut2Value[i] = (double*) malloc(MAX_BANDS * sizeof(double));
	}

	settailsize_seconds(RELEASE_MAX);
	midistuff->setLazyAttack();	// this enables the lazy note attack mode

	setpresetname(0, "feminist synth");	// default preset name


	#if TARGET_API_VST
		canProcessReplacing(false);	// only support accumulating output
		#if TARGET_PLUGIN_HAS_GUI
			editor = new RezSynthEditor(this);
		#endif
	#endif
}

//-----------------------------------------------------------------------------------------
RezSynth::~RezSynth()
{
	// deallocate the memory from these arrays
	if (delay1amp)
		free(delay1amp);
	if (delay2amp)
		free(delay2amp);
	if (inputAmp)
		free(inputAmp);
	//
	// some 2-dimensional arrays
	if (prevOutValue)
	{
		for (int i=0; i < NUM_NOTES; i++)
			free(prevOutValue[i]);
		free(prevOutValue);
	}
	if (prevprevOutValue)
	{
		for (int i=0; i < NUM_NOTES; i++)
			free(prevprevOutValue[i]);
		free(prevprevOutValue);
	}
	if (prevOut2Value)
	{
		for (int i=0; i < NUM_NOTES; i++)
			free(prevOut2Value[i]);
		free(prevOut2Value);
	}
	if (prevprevOut2Value)
	{
		for (int i=0; i < NUM_NOTES; i++)
			free(prevprevOut2Value[i]);
		free(prevprevOut2Value);
	}


#if TARGET_API_VST
	// VST doesn't have initialize and cleanup methods like Audio Unit does, 
	// so we need to call this manually here
	do_cleanup();
#endif
}

//-----------------------------------------------------------------------------------------
void RezSynth::reset()
{
	// reset the unaffected between audio stuff
	unaffectedState = unFadeIn;
	unaffectedFadeSamples = 0;
}

//-----------------------------------------------------------------------------------------
bool RezSynth::createbuffers()
{
	return true;
}

//-----------------------------------------------------------------------------------------
void RezSynth::releasebuffers()
{
}

//-----------------------------------------------------------------------------------------
void RezSynth::clearbuffers()
{
	// zero out all of these feedback buffers
	for (int i = 0; i < NUM_NOTES; i++)
	{
		for (int j = 0; j < MAX_BANDS; j++)
		{
			prevOutValue[i][j] = 0.0;
			prevprevOutValue[i][j] = 0.0;
			prevOut2Value[i][j] = 0.0;
			prevprevOut2Value[i][j] = 0.0;
		}
	}
}

//-----------------------------------------------------------------------------------------
void RezSynth::processparameters()
{
int oldNumBands = numBands;

	bandwidth = getparameter_f(kBandwidth);
	numBands = getparameter_i(kNumBands);
	sepAmount_octaval = getparameter_d(kSepAmount_octaval) / 12.0;
	sepAmount_linear = getparameter_d(kSepAmount_linear);
	sepMode = getparameter_i(kSepMode);	// true for octaval, false for linear
	foldover = getparameter_b(kFoldover);	// true for allow, false for resist
	attack = getparameter_f(kAttack) * 0.001f;
	release = getparameter_f(kRelease) * 0.001f;
	fades = getparameter_b(kFades);	// true for nicer, false for cheap
	legato = getparameter_b(kLegato);
	velInfluence = getparameter_scalar(kVelInfluence);
	velCurve = getparameter_f(kVelCurve);
	pitchbendRange = getparameter_d(kPitchbendRange);
	scaleMode = getparameter_i(kScaleMode);
	gain = getparameter_f(kGain);	// max gain is +12 dB
	betweenGain = getparameter_f(kBetweenGain);	// max betweenGain is +12 dB
	dryWetMix = getparameter_scalar(kDryWetMix);
	dryWetMixMode = getparameter_i(kDryWetMixMode);
	wiseAmp = getparameter_b(kWiseAmp);

	if (getparameterchanged(kNumBands))
	{
		// protect against accessing out of the arrays' bounds
		oldNumBands = (oldNumBands > MAX_BANDS) ? MAX_BANDS : oldNumBands;
		// clear the output buffers of abandoned bands when the number decreases
		if (numBands < oldNumBands)
		{
			for (int notecount=0; notecount < NUM_NOTES; notecount++)
			{
				for (int bandcount = numBands; bandcount < oldNumBands; bandcount++)
				{
					prevOutValue[notecount][bandcount] = 0.0;
					prevprevOutValue[notecount][bandcount] = 0.0;
					prevOut2Value[notecount][bandcount] = 0.0;
					prevprevOut2Value[notecount][bandcount] = 0.0;
				}
			}
		}
	}

	// feedback buffers need to be cleared
	if (getparameterchanged(kScaleMode))
	{
		for (int notecount=0; notecount < NUM_NOTES; notecount++)
		{
			for (int bandcount=0; bandcount < MAX_BANDS; bandcount++)
			{
				prevOutValue[notecount][bandcount] = 0.0;
				prevprevOutValue[notecount][bandcount] = 0.0;
				prevOut2Value[notecount][bandcount] = 0.0;
				prevprevOut2Value[notecount][bandcount] = 0.0;
			}
		}
	}

	// if we have just exited legato mode, we must end any active notes so that 
	// they don't hang in legato mode (remember, legato mode ignores note-offs)
	if ( getparameterchanged(kLegato) && !legato )
	{
		for (int notecount=0; notecount < NUM_NOTES; notecount++)
		{
			if (midistuff->noteTable[notecount].velocity)
			{
				// if the note is currently fading in, pick up where it left off
				if (midistuff->noteTable[notecount].attackDur)
					midistuff->noteTable[notecount].releaseSamples = midistuff->noteTable[notecount].attackSamples;
				// otherwise do the full fade out duration (if the note is not already fading out)
				else if ( (midistuff->noteTable[notecount].releaseSamples) <= 0 )
					midistuff->noteTable[notecount].releaseSamples = LEGATO_FADE_DUR;
				midistuff->noteTable[notecount].releaseDur = LEGATO_FADE_DUR;
				midistuff->noteTable[notecount].attackDur = 0;
				midistuff->noteTable[notecount].attackSamples = 0;
				midistuff->noteTable[notecount].fadeTableStep = (float)NUM_FADE_POINTS / (float)LEGATO_FADE_DUR;
				midistuff->noteTable[notecount].linearFadeStep = 1.0f / (float)LEGATO_FADE_DUR;
			}
		}
	}
}
