/*-------------- by Marc Poirier  ][  January - March 2001 -------------*/

#include "rezsynth.hpp"

#if defined(TARGET_API_VST) && TARGET_PLUGIN_HAS_GUI
	#include "rezsyntheditor.h"
#endif


// this macro does boring entry point stuff for us
DFX_ENTRY(RezSynth);

//-----------------------------------------------------------------------------------------
// initializations
RezSynth::RezSynth(TARGET_API_BASE_INSTANCE_TYPE inInstance)
	: DfxPlugin(inInstance, NUM_PARAMETERS, NUM_PRESETS)	// 19 parameters, 16 presets
{
	inputAmp = NULL;
	delay1amp = NULL;
	delay2amp = NULL;
	prevOutValue = NULL;
	prevprevOutValue = NULL;
	numBuffers = 0;


	initparameter_f(kBandwidth, "bandwidth", 3.0, 3.0, 0.1, 300.0, kDfxParamUnit_hz, kDfxParamCurve_squared);
	initparameter_i(kNumBands, "bands per note", 1, 1, 1, MAX_BANDS, kDfxParamUnit_quantity, kDfxParamCurve_stepped);
	initparameter_f(kSepAmount_octaval, "band separation (octaval)", 12.0, 12.0, 0.0, 36.0, kDfxParamUnit_semitones);
	initparameter_f(kSepAmount_linear, "band separation (linear)", 1.0, 1.0, 0.0, 3.0, kDfxParamUnit_scalar);	// % of center frequency
	initparameter_indexed(kSepMode, "separation mode", kSepMode_octaval, kSepMode_octaval, kNumSepModes);
	initparameter_b(kFoldover, "mistakes", true, false);
	initparameter_f(kAttack, "attack", 3.0, 3.0, 0.0, 3000.0, kDfxParamUnit_ms, kDfxParamCurve_squared);
	initparameter_f(kRelease, "release", 300.0, 300.0, 0.0, 3000.0, kDfxParamUnit_ms, kDfxParamCurve_squared);
	initparameter_b(kFades, "nicer fades", false, false);
	initparameter_b(kLegato, "legato", false, false);
//	initparameter_f(kVelInfluence, "velocity influence", 0.6, 1.0, 0.0, 1.0, kDfxParamUnit_portion);
	initparameter_f(kVelInfluence, "velocity influence", 60.0, 100.0, 0.0, 100.0, kDfxParamUnit_percent);
	initparameter_f(kVelCurve, "velocity curve", 2.0, 1.0, 0.3, 3.0, kDfxParamUnit_exponent);
	initparameter_f(kPitchbendRange, "pitchbend range", 3.0, 3.0, 0.0, PITCHBEND_MAX, kDfxParamUnit_semitones);
	initparameter_indexed(kScaleMode, "input gain scaling mode", kScaleMode_rms, kScaleMode_none, kNumScaleModes);
	initparameter_f(kGain, "output gain", 1.0, 1.0, 0.0, 3.981, kDfxParamUnit_lineargain, kDfxParamCurve_cubed);
	initparameter_f(kBetweenGain, "between gain", 0.0, 1.0, 0.0, 3.981, kDfxParamUnit_lineargain, kDfxParamCurve_cubed);
	initparameter_f(kDryWetMix, "dry/wet mix", 100.0, 50.0, 0.0, 100.0, kDfxParamUnit_drywetmix);
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


	settailsize_seconds(getparametermax_f(kRelease) * 0.001);
	midistuff->setLazyAttack();	// this enables the lazy note attack mode

	setpresetname(0, "feminist synth");	// default preset name


	#ifdef TARGET_API_VST
		canProcessReplacing(false);	// only support accumulating output
		#if TARGET_PLUGIN_HAS_GUI
			editor = new RezSynthEditor(this);
		#endif
	#endif
}

//-----------------------------------------------------------------------------------------
RezSynth::~RezSynth()
{
}

//-----------------------------------------------------------------------------------------
long RezSynth::initialize()
{
	bool result1 = createbuffer_d(&inputAmp, MAX_BANDS, MAX_BANDS);
	bool result2 = createbuffer_d(&delay1amp, MAX_BANDS, MAX_BANDS);
	bool result3 = createbuffer_d(&delay2amp, MAX_BANDS, MAX_BANDS);

	if ( result1 && result2 && result3 )
		return kDfxErr_NoError;
	return kDfxErr_InitializationFailed;
}

//-----------------------------------------------------------------------------------------
void RezSynth::cleanup()
{
	releasebuffer_d(&inputAmp);
	releasebuffer_d(&delay1amp);
	releasebuffer_d(&delay2amp);
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
	unsigned long oldNumBuffers = numBuffers;
	numBuffers = getnumoutputs();

	bool result1 = createbufferarrayarray_d(&prevOutValue, oldNumBuffers, NUM_NOTES, MAX_BANDS, numBuffers, NUM_NOTES, MAX_BANDS);
	bool result2 = createbufferarrayarray_d(&prevprevOutValue, oldNumBuffers, NUM_NOTES, MAX_BANDS, numBuffers, NUM_NOTES, MAX_BANDS);

	if (result1 && result2)
		return true;
	return false;
}

//-----------------------------------------------------------------------------------------
void RezSynth::releasebuffers()
{
	releasebufferarrayarray_d(&prevOutValue, numBuffers, NUM_NOTES);
	releasebufferarrayarray_d(&prevprevOutValue, numBuffers, NUM_NOTES);
}

//-----------------------------------------------------------------------------------------
void RezSynth::clearbuffers()
{
	clearbufferarrayarray_d(prevOutValue, numBuffers, NUM_NOTES, MAX_BANDS);
	clearbufferarrayarray_d(prevprevOutValue, numBuffers, NUM_NOTES, MAX_BANDS);
}

//-----------------------------------------------------------------------------------------
void RezSynth::processparameters()
{
int oldNumBands = numBands;

	bandwidth = getparameter_f(kBandwidth);
	numBands = getparameter_i(kNumBands);
	sepAmount_octaval = getparameter_f(kSepAmount_octaval) / 12.0;
	sepAmount_linear = getparameter_f(kSepAmount_linear);
	sepMode = getparameter_i(kSepMode);	// true for octaval, false for linear
	foldover = getparameter_b(kFoldover);	// true for allow, false for resist
	attack = getparameter_f(kAttack) * 0.001;
	release = getparameter_f(kRelease) * 0.001;
	fades = getparameter_b(kFades);	// true for nicer, false for cheap
	legato = getparameter_b(kLegato);
	velInfluence = getparameter_scalar(kVelInfluence);
	velCurve = getparameter_f(kVelCurve);
	pitchbendRange = getparameter_f(kPitchbendRange);
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
			for (unsigned long ch=0; ch < numBuffers; ch++)
			{
				for (int notecount=0; notecount < NUM_NOTES; notecount++)
				{
					for (int bandcount = numBands; bandcount < oldNumBands; bandcount++)
					{
						prevOutValue[ch][notecount][bandcount] = 0.0;
						prevprevOutValue[ch][notecount][bandcount] = 0.0;
					}
				}
			}
		}
	}

	// feedback buffers need to be cleared
	if (getparameterchanged(kScaleMode))
	{
		for (unsigned long ch=0; ch < numBuffers; ch++)
		{
			for (int notecount=0; notecount < NUM_NOTES; notecount++)
			{
				for (int bandcount=0; bandcount < MAX_BANDS; bandcount++)
				{
					prevOutValue[ch][notecount][bandcount] = 0.0;
					prevprevOutValue[ch][notecount][bandcount] = 0.0;
				}
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
