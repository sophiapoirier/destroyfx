#ifndef __REZSYNTHEDITOR_H
#include "rezsyntheditor.hpp"
#endif

#ifndef __REZSYNTH_H
#include "rezsynth.hpp"
#endif

#include <stdio.h>


//-----------------------------------------------------------------------------
enum {
	// resource IDs
	kBackgroundID = 128,
	kFaderSlideID,
	kFaderHandleID,
	kGlowingFaderHandleID,
	kTallFaderSlideID,
	kTallFaderHandleID,
	kGlowingTallFaderHandleID,
	kSepModeButtonID,
	kScaleButtonID,
	kLegatoButtonID,
	kFadesButtonID,
	kFoldoverButtonID,
	kWiseAmpButtonID,
	kMidiLearnButtonID,
	kMidiResetButtonID,
	kGoButtonID,
	kDestroyFXlinkID,
	kSmartElectronixLinkID,
	kConnectButtonID,

	// positions
	kFaderX = 22,
	kFaderY = 59,
	kFaderInc = 35,

	kDisplayX = kFaderX + 252,
	kDisplayY = kFaderY - 8,
	kDisplayWidth = 123,
	kDisplayHeight = 11,

	kTallFaderX = 383,
	kTallFaderY = 60,
	kTallFaderInc = 51,
	kTallDisplayX = kTallFaderX - 10,
	kTallDisplayY = kTallFaderY + 189,
	kTallDisplayWidth = 36,
	kTallDisplayHeight = 11,

	kSepModeButtonX = 274,
	kSepModeButtonY = 121,
	kScaleButtonX = 400,
	kScaleButtonY = 33,
	kLegatoButtonX = 435,
	kLegatoButtonY = 296,
	kFadesButtonX = 274,
	kFadesButtonY = 176,
	kFoldoverButtonX = 274,
	kFoldoverButtonY = 87,
	kWiseAmpButtonX = 368,
	kWiseAmpButtonY = 296,

	kMidiLearnButtonX = 189,
	kMidiLearnButtonY = 331,
	kMidiResetButtonX = kMidiLearnButtonX + 72,
	kMidiResetButtonY = 331,

	kGoButtonX = 22,
	kGoButtonY = 331,

	kDestroyFXlinkX = 453,
	kDestroyFXlinkY = 327,
	kSmartElectronixLinkX = 358,
	kSmartElectronixLinkY = 345,

	kConnectButtonX = 139,
	kConnectButtonY = 331,

	kSepAmountTextEdit = 333,
	kNoGoDisplay = 33333
};



//-----------------------------------------------------------------------------
// parameter value string display conversion functions

void bandwidthDisplayConvert(float value, char *string);
void bandwidthDisplayConvert(float value, char *string)
{
	sprintf(string, "%.3f  Hz", bandwidthScaled(value));
}

void bandwidthParamDisplayConvert(float value, char *string);
void bandwidthParamDisplayConvert(float value, char *string)
{
	strcpy(string, "bandwidth");
}

void numBandsDisplayConvert(float value, char *string);
void numBandsDisplayConvert(float value, char *string)
{
	sprintf(string, " %d ", numBandsScaled(value) );
}

void numBandsParamDisplayConvert(float value, char *string);
void numBandsParamDisplayConvert(float value, char *string)
{
	strcpy(string, "bands  per  note");
}

void sepAmountDisplayConvert(float value, char *string, void *septag);
void sepAmountDisplayConvert(float value, char *string, void *septag)
{
	if ( *(long*)septag == kSepAmount_octaval )
		sprintf(string, "%.3f  semitones", value*(float)MAX_SEP_AMOUNT*12.0f);
	else	// linear values
		sprintf(string, "%.3f  x", value*(float)MAX_SEP_AMOUNT);
}

void sepAmountParamDisplayConvert(float value, char *string);
void sepAmountParamDisplayConvert(float value, char *string)
{
	strcpy(string, "band  separation");
}

void attackDisplayConvert(float value, char *string);
void attackDisplayConvert(float value, char *string)
{
  float attack, remainder;
  long thousands;

	attack = attackScaled(value)*1000.0f;
	thousands = (long)attack / 1000;
	remainder = fmodf(attack, 1000.0f);

	if (thousands > 0)
	{
		if (remainder < 10)
			sprintf(string, "%ld,00%.1f  ms", thousands, remainder);
		else if (remainder < 100)
			sprintf(string, "%ld,0%.1f  ms", thousands, remainder);
		else
			sprintf(string, "%ld,%.1f  ms", thousands, remainder);
	}
	else
		sprintf(string, "%.1f  ms", attack);
}

void attackParamDisplayConvert(float value, char *string);
void attackParamDisplayConvert(float value, char *string)
{
	strcpy(string, "attack");
}

void releaseDisplayConvert(float value, char *string);
void releaseDisplayConvert(float value, char *string)
{
  float release, remainder;
  long thousands;

	release = releaseScaled(value)*1000.0f;
	thousands = (long)release / 1000;
	remainder = fmodf(release, 1000.0f);

	if (thousands > 0)
	{
		if (remainder < 10)
			sprintf(string, "%ld,00%.1f  ms", thousands, remainder);
		else if (remainder < 100)
			sprintf(string, "%ld,0%.1f  ms", thousands, remainder);
		else
			sprintf(string, "%ld,%.1f  ms", thousands, remainder);
	}
	else
		sprintf(string, "%.1f  ms", release);
}

void releaseParamDisplayConvert(float value, char *string);
void releaseParamDisplayConvert(float value, char *string)
{
	strcpy(string, "release");
}

void velCurveDisplayConvert(float value, char *string);
void velCurveDisplayConvert(float value, char *string)
{
	sprintf(string, "%.3f", velCurveScaled(value));
}

void velCurveParamDisplayConvert(float value, char *string);
void velCurveParamDisplayConvert(float value, char *string)
{
	strcpy(string, "velocity  curve");
}

void velInfluenceDisplayConvert(float value, char *string);
void velInfluenceDisplayConvert(float value, char *string)
{
	sprintf(string, "%.3f", value);
}

void velInfluenceParamDisplayConvert(float value, char *string);
void velInfluenceParamDisplayConvert(float value, char *string)
{
	strcpy(string, "velocity  influence");
}

void pitchbendDisplayConvert(float value, char *string);
void pitchbendDisplayConvert(float value, char *string)
{
	sprintf(string, "\xB1 %.2f  semitones", value*PITCHBEND_MAX);
}

void pitchbendParamDisplayConvert(float value, char *string);
void pitchbendParamDisplayConvert(float value, char *string)
{
	strcpy(string, "pitchbend  range");
}

void gainDisplayConvert(float value, char *string);
void gainDisplayConvert(float value, char *string)
{
	if (gainScaled(value) <= 0.0f)
		strcpy(string, "-oo");
	else if (gainScaled(value) > 1.0001f)
		sprintf(string, "+%.1f", dBconvert(gainScaled(value)));
	else
		sprintf(string, "%.1f", dBconvert(gainScaled(value)));
}

void betweenGainDisplayConvert(float value, char *string);
void betweenGainDisplayConvert(float value, char *string)
{
	if (betweenGainScaled(value) <= 0.0f)
			strcpy(string, "-oo");
	else if (betweenGainScaled(value) > 1.0001)
		sprintf(string, "+%.1f", dBconvert(betweenGainScaled(value)));
	else
		sprintf(string, "%.1f", dBconvert(betweenGainScaled(value)));
}

void dryWetMixDisplayConvert(float value, char *string);
void dryWetMixDisplayConvert(float value, char *string)
{
	sprintf(string, "%ld %%", (long) (value * 100.0f) );
}

void goDisplayConvert(float value, char *string, void *goErr);
void goDisplayConvert(float value, char *string, void *goErr)
{
//	if (value < 0.12f)
	if ( *(long*)goErr == kNoGoDisplay )
		strcpy(string, " ");
	else
	{
		if ( *(long*)goErr )
			strcpy(string, "!ERR0R!");
		else
			strcpy(string, "yay!");
	}
}



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
RezSynthEditor::RezSynthEditor(AudioEffect *effect)
 : AEffGUIEditor(effect) 
{
	frame = 0;

	// initialize the graphics pointers
	gBackground = 0;
	gFaderSlide = 0;
	gFaderHandle = 0;
	gGlowingFaderHandle = 0;
	gTallFaderSlide = 0;
	gTallFaderHandle = 0;
	gGlowingTallFaderHandle = 0;
	gSepModeButton = 0;
	gScaleButton = 0;
	gLegatoButton = 0;
	gFadesButton = 0;
	gFoldoverButton = 0;
	gWiseAmpButton = 0;
	gMidiLearnButton = 0;
	gMidiResetButton = 0;
	gGoButton = 0;
	gDestroyFXlink = 0;
	gSmartElectronixLink = 0;

	// initialize the controls pointers
	bandwidthFader = 0;
	numBandsFader = 0;
	sepModeButton = 0;
	sepAmountFader = 0;
	attackFader = 0;
	releaseFader = 0;
	velCurveFader = 0;
	velInfluenceFader = 0;
	pitchbendFader = 0;
	scaleButton = 0;
	gainFader = 0;
	betweenGainFader = 0;
	dryWetMixFader = 0;
	legatoButton = 0;
	fadesButton = 0;
	foldoverButton = 0;
	wiseAmpButton = 0;
	midiLearnButton = 0;
	midiResetButton = 0;
	goButton = 0;
	DestroyFXlink = 0;
	SmartElectronixLink = 0;

	// initialize the value display box pointers
	bandwidthDisplay = 0;
	numBandsDisplay = 0;
	sepAmountTextEdit = 0;
	attackDisplay = 0;
	releaseDisplay = 0;
	velCurveDisplay = 0;
	velInfluenceDisplay = 0;
	pitchbendDisplay = 0;
	bandwidthParamDisplay = 0;
	numBandsParamDisplay = 0;
	sepAmountParamDisplay = 0;
	attackParamDisplay = 0;
	releaseParamDisplay = 0;
	velCurveParamDisplay = 0;
	velInfluenceParamDisplay = 0;
	pitchbendParamDisplay = 0;
	gainDisplay = 0;
	betweenGainDisplay = 0;
	dryWetMixDisplay = 0;
	goDisplay = 0;

	// load the background bitmap
	// we don't need to load all bitmaps, this could be done when open is called
	gBackground = new CBitmap (kBackgroundID);

	// init the size of the plugin
	rect.left   = 0;
	rect.top    = 0;
	rect.right  = (short)gBackground->getWidth();
	rect.bottom = (short)gBackground->getHeight();

	sepAmountString = new char[256];

	chunk = ((DfxPlugin*)effect)->getsettings_ptr();	// this just simplifies pointing

	faders = 0;
	tallFaders = 0;
	faders = (CHorizontalSlider**)malloc(sizeof(CHorizontalSlider*)*NUM_PARAMETERS);
	tallFaders = (CVerticalSlider**)malloc(sizeof(CVerticalSlider*)*NUM_PARAMETERS);
	for (long i=0; i < NUM_PARAMETERS; i++)
		faders[i] = NULL;
	for (long j=0; j < NUM_PARAMETERS; j++)
		tallFaders[j] = NULL;
}

//-----------------------------------------------------------------------------
RezSynthEditor::~RezSynthEditor()
{
	// free background bitmap
	if (gBackground)
		gBackground->forget();
	gBackground = 0;

	free(faders);
	faders = 0;
	free(tallFaders);
	tallFaders = 0;

	if (sepAmountString)
		delete[] sepAmountString;
}

//-----------------------------------------------------------------------------
long RezSynthEditor::getRect(ERect **erect)
{
	*erect = &rect;
	return true;
}

//-----------------------------------------------------------------------------
long RezSynthEditor::open(void *ptr)
{
  CPoint displayOffset;	// for positioning the background graphic behind display boxes


	// !!! always call this !!!
	AEffGUIEditor::open(ptr);

	// load some graphics
	if (!gFaderSlide)
		gFaderSlide = new CBitmap (kFaderSlideID);
	if (!gFaderHandle)
		gFaderHandle = new CBitmap (kFaderHandleID);
	if (!gGlowingFaderHandle)
		gGlowingFaderHandle = new CBitmap (kGlowingFaderHandleID);
	if (!gTallFaderSlide)
		gTallFaderSlide = new CBitmap (kTallFaderSlideID);
	if (!gTallFaderHandle)
		gTallFaderHandle = new CBitmap (kTallFaderHandleID);
	if (!gGlowingTallFaderHandle)
		gGlowingTallFaderHandle = new CBitmap (kGlowingTallFaderHandleID);

	if (!gSepModeButton)
		gSepModeButton = new CBitmap (kSepModeButtonID);
	if (!gScaleButton)
		gScaleButton = new CBitmap (kScaleButtonID);
	if (!gLegatoButton)
		gLegatoButton = new CBitmap (kLegatoButtonID);
	if (!gFadesButton)
		gFadesButton = new CBitmap (kFadesButtonID);
	if (!gFoldoverButton)
		gFoldoverButton = new CBitmap (kFoldoverButtonID);
	if (!gWiseAmpButton)
		gWiseAmpButton = new CBitmap (kWiseAmpButtonID);

	if (!gMidiLearnButton)
		gMidiLearnButton = new CBitmap (kMidiLearnButtonID);
	if (!gMidiResetButton)
		gMidiResetButton = new CBitmap (kMidiResetButtonID);

	if (!gGoButton)
		gGoButton = new CBitmap (kGoButtonID);
	if (!gDestroyFXlink)
		gDestroyFXlink = new CBitmap (kDestroyFXlinkID);
	if (!gSmartElectronixLink)
		gSmartElectronixLink = new CBitmap (kSmartElectronixLinkID);


	chunk->resetLearning();
	goError = kNoGoDisplay;
	long sepTag = (((DfxPlugin*)effect)->getparameter_i(kSepMode) == kSepMode_octaval) ? kSepAmount_octaval : kSepAmount_linear;


	//--initialize the background frame--------------------------------------
	CRect size (0, 0, gBackground->getWidth(), gBackground->getHeight());
	frame = new CFrame (size, ptr, this);
	frame->setBackground(gBackground);


	//--initialize the horizontal faders-------------------------------------
	int minPos = kFaderX;
	int maxPos = kFaderX + gFaderSlide->getWidth() - gFaderHandle->getWidth();
	CPoint point (0, 0);

	// bandwidth
	// size stores left, top, right, & bottom positions
	size (kFaderX, kFaderY, kFaderX + gFaderSlide->getWidth(), kFaderY + gFaderSlide->getHeight());
	bandwidthFader = new CHorizontalSlider (size, this, kBandwidth, minPos, maxPos, gFaderHandle, gFaderSlide, point, kLeft);
	bandwidthFader->setValue(effect->getParameter(kBandwidth));
	bandwidthFader->setDefaultValue(0.0983355987f);//(0.1722688563f);
	frame->addView(bandwidthFader);

	// number of bands
	size.offset (0, kFaderInc);
//	numBandsFader = new CHorizontalSlider (size, this, kNumBands, minPos, maxPos, gFaderHandle, gFaderSlide, point, kLeft);
	numBandsFader = new CHorizontalSliderChunky (size, this, kNumBands, minPos, maxPos, MAX_BANDS, gFaderHandle, gFaderSlide, point, kLeft);
	numBandsFader->setValue(effect->getParameter(kNumBands));
	numBandsFader->setDefaultValue(0.0f);
	frame->addView(numBandsFader);

	// separation amount between bands
	size.offset (0, kFaderInc);
	sepAmountFader = new CHorizontalSlider (size, this, sepTag, minPos, maxPos, gFaderHandle, gFaderSlide, point, kLeft);
	sepAmountFader->setValue(effect->getParameter(sepTag));
	sepAmountFader->setDefaultValue(1.0f/3.0f);
	frame->addView(sepAmountFader);

	// note attack duration
	size.offset (0, kFaderInc);
	attackFader = new CHorizontalSlider (size, this, kAttack, minPos, maxPos, gFaderHandle, gFaderSlide, point, kLeft);
	attackFader->setValue(effect->getParameter(kAttack));
	attackFader->setDefaultValue(0.0316227766f);
	frame->addView(attackFader);

	// note release duration
	size.offset (0, kFaderInc);
	releaseFader = new CHorizontalSlider (size, this, kRelease, minPos, maxPos, gFaderHandle, gFaderSlide, point, kLeft);
	releaseFader->setValue(effect->getParameter(kRelease));
	releaseFader->setDefaultValue(0.316227766f);
	frame->addView(releaseFader);

	// velocity curve factor
	size.offset (0, kFaderInc);
	velCurveFader = new CHorizontalSlider (size, this, kVelCurve, minPos, maxPos, gFaderHandle, gFaderSlide, point, kLeft);
	velCurveFader->setValue(effect->getParameter(kVelCurve));
	velCurveFader->setDefaultValue(velCurveUnscaled(1.0f));
	frame->addView(velCurveFader);

	// velocity influence
	size.offset (0, kFaderInc);
	velInfluenceFader = new CHorizontalSlider (size, this, kVelInfluence, minPos, maxPos, gFaderHandle, gFaderSlide, point, kLeft);
	velInfluenceFader->setValue(effect->getParameter(kVelInfluence));
	velInfluenceFader->setDefaultValue(1.0f);
	frame->addView(velInfluenceFader);

	// pitchbend range
	size.offset (0, kFaderInc);
	pitchbendFader = new CHorizontalSlider (size, this, kPitchbendRange, minPos, maxPos, gFaderHandle, gFaderSlide, point, kLeft);
	pitchbendFader->setValue(effect->getParameter(kPitchbendRange));
	pitchbendFader->setDefaultValue(1.0f/12.0f);
	frame->addView(pitchbendFader);

	//--initialize the vertical gain faders---------------------------------
	int minTallPos = kTallFaderY;
	int maxTallPos = kTallFaderY + gTallFaderSlide->getHeight() - gTallFaderHandle->getHeight() - 1;

	// filtered output gain
	size (kTallFaderX, kTallFaderY, kTallFaderX + gTallFaderSlide->getWidth(), kTallFaderY + gTallFaderSlide->getHeight());
	gainFader = new CVerticalSlider (size, this, kGain, minTallPos, maxTallPos, gTallFaderHandle, gTallFaderSlide, point, kBottom);
	gainFader->setValue(effect->getParameter(kGain));
	gainFader->setDefaultValue(0.630961132713f);
	frame->addView(gainFader);

	// between-filtering gain
	size.offset (kTallFaderInc, 0);
	betweenGainFader = new CVerticalSlider (size, this, kBetweenGain, minTallPos, maxTallPos, gTallFaderHandle, gTallFaderSlide, point, kBottom);
	betweenGainFader->setValue(effect->getParameter(kBetweenGain));
	betweenGainFader->setDefaultValue(0.630961132713f);
	frame->addView(betweenGainFader);

	// dry/wet mix
	size.offset (kTallFaderInc, 0);
	dryWetMixFader = new CVerticalSlider (size, this, kDryWetMix, minTallPos, maxTallPos, gTallFaderHandle, gTallFaderSlide, point, kBottom);
	dryWetMixFader->setValue(effect->getParameter(kDryWetMix));
	dryWetMixFader->setDefaultValue(0.5f);
	frame->addView(dryWetMixFader);

	//--initialize the buttons----------------------------------------------

	// input gain scaling
	size (kScaleButtonX, kScaleButtonY, kScaleButtonX + gScaleButton->getWidth(), kScaleButtonY + (gScaleButton->getHeight())/kNumScaleModes);
	scaleButton = new CHorizontalSwitch (size, this, kScaleMode, kNumScaleModes, (gScaleButton->getHeight())/kNumScaleModes, kNumScaleModes, gScaleButton, point);
	scaleButton->setValue(effect->getParameter(kScaleMode));
	frame->addView(scaleButton);

	// separation mode (linear or logarithmic)
	size (kSepModeButtonX, kSepModeButtonY, kSepModeButtonX + gSepModeButton->getWidth(), kSepModeButtonY + (gSepModeButton->getHeight())/2);
	sepModeButton = new COnOffButton (size, this, kSepMode, gSepModeButton);
	sepModeButton->setValue(effect->getParameter(kSepMode));
	frame->addView(sepModeButton);

	// legato
	size (kLegatoButtonX, kLegatoButtonY, kLegatoButtonX + gLegatoButton->getWidth(), kLegatoButtonY + (gLegatoButton->getHeight())/2);
	legatoButton = new COnOffButton (size, this, kLegato, gLegatoButton);
	legatoButton->setValue(effect->getParameter(kLegato));
	frame->addView(legatoButton);

	// attack & release fade mode
	size (kFadesButtonX, kFadesButtonY, kFadesButtonX + gFadesButton->getWidth(), kFadesButtonY + (gFadesButton->getHeight())/2);
	fadesButton = new COnOffButton (size, this, kFades, gFadesButton);
	fadesButton->setValue(effect->getParameter(kFades));
	frame->addView(fadesButton);

	// allow Nyquist foldover or no
	size (kFoldoverButtonX, kFoldoverButtonY, kFoldoverButtonX + gFoldoverButton->getWidth(), kFoldoverButtonY + (gFoldoverButton->getHeight())/2);
	foldoverButton = new COnOffButton (size, this, kFoldover, gFoldoverButton);
	foldoverButton->setValue(effect->getParameter(kFoldover));
	frame->addView(foldoverButton);

	// wisely lower the output gain to accomodate for resonance or no
	size (kWiseAmpButtonX, kWiseAmpButtonY, kWiseAmpButtonX + gWiseAmpButton->getWidth(), kWiseAmpButtonY + (gWiseAmpButton->getHeight())/2);
	wiseAmpButton = new COnOffButton (size, this, kWiseAmp, gWiseAmpButton);
	wiseAmpButton->setValue(effect->getParameter(kWiseAmp));
	frame->addView(wiseAmpButton);

	// turn on/off MIDI learn mode for CC parameter automation
	size (kMidiLearnButtonX, kMidiLearnButtonY, kMidiLearnButtonX + gMidiLearnButton->getWidth(), kMidiLearnButtonY + (gMidiLearnButton->getHeight())/2);
	midiLearnButton = new COnOffButton (size, this, kMidiLearnButtonID, gMidiLearnButton);
	midiLearnButton->setValue(0.0f);
	frame->addView(midiLearnButton);

	// clear all MIDI CC assignments
	size (kMidiResetButtonX, kMidiResetButtonY, kMidiResetButtonX + gMidiResetButton->getWidth(), kMidiResetButtonY + (gMidiResetButton->getHeight())/2);
	midiResetButton = new CKickButton (size, this, kMidiResetButtonID, (gMidiResetButton->getHeight())/2, gMidiResetButton, point);
	midiResetButton->setValue(0.0f);
	frame->addView(midiResetButton);

	// go!
	size (kGoButtonX, kGoButtonY, kGoButtonX + gGoButton->getWidth(), kGoButtonY + (gGoButton->getHeight())/2);
	goButton = new CWebLink (size, this, kGoButtonID, "http://www.streetharassmentproject.org/", gGoButton);
	frame->addView(goButton);

	// Destroy FX web page link
	size (kDestroyFXlinkX, kDestroyFXlinkY, kDestroyFXlinkX + gDestroyFXlink->getWidth(), kDestroyFXlinkY + (gDestroyFXlink->getHeight())/2);
	DestroyFXlink = new CWebLink (size, this, kDestroyFXlinkID, DESTROYFX_URL, gDestroyFXlink);
	frame->addView(DestroyFXlink);

	// Smart Electronix web page link
	size (kSmartElectronixLinkX, kSmartElectronixLinkY, kSmartElectronixLinkX + gSmartElectronixLink->getWidth(), kSmartElectronixLinkY + (gSmartElectronixLink->getHeight())/2);
	SmartElectronixLink = new CWebLink (size, this, kSmartElectronixLinkID, SMARTELECTRONIX_URL, gSmartElectronixLink);
	frame->addView(SmartElectronixLink);


	//--initialize the displays---------------------------------------------

	// bandwidth parameter label
	size (kFaderX, kFaderY-kDisplayHeight-3, kFaderX + kDisplayWidth, kFaderY);
	bandwidthParamDisplay = new CParamDisplay (size);
	bandwidthParamDisplay->setHoriAlign(kLeftText);
	bandwidthParamDisplay->setFont(kNormalFontVerySmall);
	bandwidthParamDisplay->setFontColor(kMyVeryLightGreyCColor);
	bandwidthParamDisplay->setBackColor(kTransparentCColor);
	bandwidthParamDisplay->setFrameColor(kTransparentCColor);
	bandwidthParamDisplay->setTransparency(true);
	bandwidthParamDisplay->setStringConvert(bandwidthParamDisplayConvert);
	frame->addView(bandwidthParamDisplay);

	// bandwidth
	size.offset (kDisplayWidth, 0);
	bandwidthDisplay = new CParamDisplay (size, gBackground);
	displayOffset (kFaderX+kDisplayWidth, kFaderY-kDisplayHeight-3);
	bandwidthDisplay->setBackOffset(displayOffset);
//	bandwidthDisplay->setBackOffset(bandwidthDisplay->getViewSize());
	bandwidthDisplay->setHoriAlign(kRightText);
	bandwidthDisplay->setFont(kNormalFontVerySmall);
	bandwidthDisplay->setFontColor(kMyVeryLightGreyCColor);
	bandwidthDisplay->setValue(effect->getParameter(kBandwidth));
	bandwidthDisplay->setStringConvert(bandwidthDisplayConvert);
	frame->addView(bandwidthDisplay);

	// number of bands parameter label
	size.offset (-kDisplayWidth, kFaderInc);
	numBandsParamDisplay = new CParamDisplay (size);
	numBandsParamDisplay->setHoriAlign(kLeftText);
	numBandsParamDisplay->setFont(kNormalFontVerySmall);
	numBandsParamDisplay->setFontColor(kMyVeryLightGreyCColor);
	numBandsParamDisplay->setBackColor(kTransparentCColor);
	numBandsParamDisplay->setFrameColor(kTransparentCColor);
	numBandsParamDisplay->setTransparency(true);
	numBandsParamDisplay->setStringConvert(numBandsParamDisplayConvert);
	frame->addView(numBandsParamDisplay);

	// number of bands
	size.offset (kDisplayWidth, 0);
	numBandsDisplay = new CParamDisplay (size, gBackground);
	displayOffset.offset (0, kFaderInc);
	numBandsDisplay->setBackOffset(displayOffset);
	numBandsDisplay->setHoriAlign(kRightText);
	numBandsDisplay->setFont(kNormalFontVerySmall);
	numBandsDisplay->setFontColor(kMyVeryLightGreyCColor);
	numBandsDisplay->setValue(effect->getParameter(kNumBands));
	numBandsDisplay->setStringConvert(numBandsDisplayConvert);
	frame->addView(numBandsDisplay);

	// separation amount parameter label
	size.offset (-kDisplayWidth, kFaderInc);
	sepAmountParamDisplay = new CParamDisplay (size);
	sepAmountParamDisplay->setHoriAlign(kLeftText);
	sepAmountParamDisplay->setFont(kNormalFontVerySmall);
	sepAmountParamDisplay->setFontColor(kMyVeryLightGreyCColor);
	sepAmountParamDisplay->setBackColor(kTransparentCColor);
	sepAmountParamDisplay->setFrameColor(kTransparentCColor);
	sepAmountParamDisplay->setTransparency(true);
	sepAmountParamDisplay->setStringConvert(sepAmountParamDisplayConvert);
	frame->addView(sepAmountParamDisplay);

	// separation amount between bands   (typable)
	size.offset (kDisplayWidth, 0);
	sepAmountTextEdit = new CTextEdit (size, this, kSepAmountTextEdit, 0, gBackground);
	displayOffset.offset (0, kFaderInc);
	sepAmountTextEdit->setBackOffset(displayOffset);
	sepAmountTextEdit->setFont(kNormalFontVerySmall);
	sepAmountTextEdit->setFontColor(kMyVeryLightGreyCColor);
	sepAmountTextEdit->setHoriAlign(kRightText);
	frame->addView(sepAmountTextEdit);
	// this makes it display the current value
	setParameter(sepTag, effect->getParameter(sepTag));

	// note attack parameter label
	size.offset (-kDisplayWidth, kFaderInc);
	attackParamDisplay = new CParamDisplay (size);
	attackParamDisplay->setHoriAlign(kLeftText);
	attackParamDisplay->setFont(kNormalFontVerySmall);
	attackParamDisplay->setFontColor(kMyVeryLightGreyCColor);
	attackParamDisplay->setBackColor(kTransparentCColor);
	attackParamDisplay->setFrameColor(kTransparentCColor);
	attackParamDisplay->setTransparency(true);
	attackParamDisplay->setStringConvert(attackParamDisplayConvert);
	frame->addView(attackParamDisplay);

	// note attack duration
	size.offset (kDisplayWidth, 0);
	attackDisplay = new CParamDisplay (size, gBackground);
	displayOffset.offset (0, kFaderInc);
	attackDisplay->setBackOffset(displayOffset);
	attackDisplay->setHoriAlign(kRightText);
	attackDisplay->setFont(kNormalFontVerySmall);
	attackDisplay->setFontColor(kMyVeryLightGreyCColor);
	attackDisplay->setValue(effect->getParameter(kAttack));
	attackDisplay->setStringConvert(attackDisplayConvert);
	frame->addView(attackDisplay);

	// note release parameter label
	size.offset (-kDisplayWidth, kFaderInc);
	releaseParamDisplay = new CParamDisplay (size);
	releaseParamDisplay->setHoriAlign(kLeftText);
	releaseParamDisplay->setFont(kNormalFontVerySmall);
	releaseParamDisplay->setFontColor(kMyVeryLightGreyCColor);
	releaseParamDisplay->setBackColor(kTransparentCColor);
	releaseParamDisplay->setFrameColor(kTransparentCColor);
	releaseParamDisplay->setTransparency(true);
	releaseParamDisplay->setStringConvert(releaseParamDisplayConvert);
	frame->addView(releaseParamDisplay);

	// note release duration
	size.offset (kDisplayWidth, 0);
	releaseDisplay = new CParamDisplay (size, gBackground);
	displayOffset.offset (0, kFaderInc);
	releaseDisplay->setBackOffset(displayOffset);
	releaseDisplay->setHoriAlign(kRightText);
	releaseDisplay->setFont(kNormalFontVerySmall);
	releaseDisplay->setFontColor(kMyVeryLightGreyCColor);
	releaseDisplay->setValue(effect->getParameter(kRelease));
	releaseDisplay->setStringConvert(releaseDisplayConvert);
	frame->addView(releaseDisplay);

	// velocity curve parameter label
	size.offset (-kDisplayWidth, kFaderInc);
	velCurveParamDisplay = new CParamDisplay (size);
	velCurveParamDisplay->setHoriAlign(kLeftText);
	velCurveParamDisplay->setFont(kNormalFontVerySmall);
	velCurveParamDisplay->setFontColor(kMyVeryLightGreyCColor);
	velCurveParamDisplay->setBackColor(kTransparentCColor);
	velCurveParamDisplay->setFrameColor(kTransparentCColor);
	velCurveParamDisplay->setTransparency(true);
	velCurveParamDisplay->setStringConvert(velCurveParamDisplayConvert);
	frame->addView(velCurveParamDisplay);

	// velocity curve factor
	size.offset (kDisplayWidth, 0);
	velCurveDisplay = new CParamDisplay (size, gBackground);
	displayOffset.offset (0, kFaderInc);
	velCurveDisplay->setBackOffset(displayOffset);
	velCurveDisplay->setHoriAlign(kRightText);
	velCurveDisplay->setFont(kNormalFontVerySmall);
	velCurveDisplay->setFontColor(kMyVeryLightGreyCColor);
	velCurveDisplay->setValue(effect->getParameter(kVelCurve));
	velCurveDisplay->setStringConvert(velCurveDisplayConvert);
	frame->addView(velCurveDisplay);

	// velocity influence parameter label
	size.offset (-kDisplayWidth, kFaderInc);
	velInfluenceParamDisplay = new CParamDisplay (size);
	velInfluenceParamDisplay->setHoriAlign(kLeftText);
	velInfluenceParamDisplay->setFont(kNormalFontVerySmall);
	velInfluenceParamDisplay->setFontColor(kMyVeryLightGreyCColor);
	velInfluenceParamDisplay->setBackColor(kTransparentCColor);
	velInfluenceParamDisplay->setFrameColor(kTransparentCColor);
	velInfluenceParamDisplay->setTransparency(true);
	velInfluenceParamDisplay->setStringConvert(velInfluenceParamDisplayConvert);
	frame->addView(velInfluenceParamDisplay);

	// velocity influence
	size.offset (kDisplayWidth, 0);
	velInfluenceDisplay = new CParamDisplay (size, gBackground);
	displayOffset.offset (0, kFaderInc);
	velInfluenceDisplay->setBackOffset(displayOffset);
	velInfluenceDisplay->setHoriAlign(kRightText);
	velInfluenceDisplay->setFont(kNormalFontVerySmall);
	velInfluenceDisplay->setFontColor(kMyVeryLightGreyCColor);
	velInfluenceDisplay->setValue(effect->getParameter(kVelInfluence));
	velInfluenceDisplay->setStringConvert(velInfluenceDisplayConvert);
	frame->addView(velInfluenceDisplay);

	// pitchbend range parameter label
	size.offset (-kDisplayWidth, kFaderInc);
	pitchbendParamDisplay = new CParamDisplay (size);
	pitchbendParamDisplay->setHoriAlign(kLeftText);
	pitchbendParamDisplay->setFont(kNormalFontVerySmall);
	pitchbendParamDisplay->setFontColor(kMyVeryLightGreyCColor);
	pitchbendParamDisplay->setBackColor(kTransparentCColor);
	pitchbendParamDisplay->setFrameColor(kTransparentCColor);
	pitchbendParamDisplay->setTransparency(true);
	pitchbendParamDisplay->setStringConvert(pitchbendParamDisplayConvert);
	frame->addView(pitchbendParamDisplay);

	// pitchbend range
	size.offset (kDisplayWidth, 0);
	pitchbendDisplay = new CParamDisplay (size, gBackground);
	displayOffset.offset (0, kFaderInc);
	pitchbendDisplay->setBackOffset(displayOffset);
	pitchbendDisplay->setHoriAlign(kRightText);
	pitchbendDisplay->setFont(kNormalFontVerySmall);
	pitchbendDisplay->setFontColor(kMyVeryLightGreyCColor);
	pitchbendDisplay->setValue(effect->getParameter(kPitchbendRange));
	pitchbendDisplay->setStringConvert(pitchbendDisplayConvert);
	frame->addView(pitchbendDisplay);

	// filtered output gain
	size (kTallDisplayX, kTallDisplayY, kTallDisplayX + kTallDisplayWidth, kTallDisplayY + kTallDisplayHeight);
	gainDisplay = new CParamDisplay (size);
	gainDisplay->setHoriAlign(kCenterText);
	gainDisplay->setFont(kNormalFontVerySmall);
	gainDisplay->setFontColor(kMyLightGreyCColor);
	gainDisplay->setBackColor(kMyGreyCColor);
	gainDisplay->setFrameColor(kBlackCColor);
	gainDisplay->setValue(effect->getParameter(kGain));
	gainDisplay->setStringConvert(gainDisplayConvert);
	frame->addView(gainDisplay);

	// between-filtering gain
	size.offset (kTallFaderInc, 0);
	betweenGainDisplay = new CParamDisplay (size);
	betweenGainDisplay->setHoriAlign(kCenterText);
	betweenGainDisplay->setFont(kNormalFontVerySmall);
	betweenGainDisplay->setFontColor(kMyLightGreyCColor);
	betweenGainDisplay->setBackColor(kMyGreyCColor);
	betweenGainDisplay->setFrameColor(kBlackCColor);
	betweenGainDisplay->setValue(effect->getParameter(kBetweenGain));
	betweenGainDisplay->setStringConvert(betweenGainDisplayConvert);
	frame->addView(betweenGainDisplay);

	// dry/wet mix
	size.offset (kTallFaderInc, 0);
	dryWetMixDisplay = new CParamDisplay (size);
	dryWetMixDisplay->setHoriAlign(kCenterText);
	dryWetMixDisplay->setFont(kNormalFontVerySmall);
	dryWetMixDisplay->setFontColor(kMyLightGreyCColor);
	dryWetMixDisplay->setBackColor(kMyGreyCColor);
	dryWetMixDisplay->setFrameColor(kBlackCColor);
	dryWetMixDisplay->setValue(effect->getParameter(kDryWetMix));
	dryWetMixDisplay->setStringConvert(dryWetMixDisplayConvert);
	frame->addView(dryWetMixDisplay);

	// go! error display
	size (kGoButtonX, kGoButtonY + 17, kGoButtonX + gGoButton->getWidth(), kGoButtonY + 17 + kTallDisplayHeight);
	goDisplay = new CParamDisplay (size, gBackground);
	displayOffset (kGoButtonX, kGoButtonY + 17);
	goDisplay->setBackOffset(displayOffset);
	goDisplay->setHoriAlign(kCenterText);
	goDisplay->setFont(kNormalFontSmall);
	goDisplay->setFontColor(kWhiteCColor);
	goDisplay->setValue(0.0f);
	goDisplay->setStringConvert(goDisplayConvert, &goError);
	frame->addView(goDisplay);


	for (long i=0; i < NUM_PARAMETERS; i++)
		faders[i] = NULL;
	faders[kBandwidth] = bandwidthFader;
	faders[kNumBands] = numBandsFader;
	faders[kAttack] = attackFader;
	faders[kRelease] = releaseFader;
	faders[kVelCurve] = velCurveFader;
	faders[kVelInfluence] = velInfluenceFader;
	faders[kPitchbendRange] = pitchbendFader;

	for (long j=0; j < NUM_PARAMETERS; j++)
		tallFaders[j] = NULL;
	tallFaders[kGain] = gainFader;
	tallFaders[kBetweenGain] = betweenGainFader;
	tallFaders[kDryWetMix] = dryWetMixFader;


	return true;
}

//-----------------------------------------------------------------------------
void RezSynthEditor::close()
{
	if (frame)
		delete frame;
	frame = 0;

	for (long i=0; i < NUM_PARAMETERS; i++)
		faders[i] = NULL;
	for (long j=0; j < NUM_PARAMETERS; j++)
		tallFaders[j] = NULL;

	chunk->resetLearning();

	// free some bitmaps
	if (gFaderSlide)
		gFaderSlide->forget();
	gFaderSlide = 0;

	if (gFaderHandle)
		gFaderHandle->forget();
	gFaderHandle = 0;

	if (gGlowingFaderHandle)
		gGlowingFaderHandle->forget();
	gGlowingFaderHandle = 0;

	if (gTallFaderSlide)
		gTallFaderSlide->forget();
	gTallFaderSlide = 0;

	if (gTallFaderHandle)
		gTallFaderHandle->forget();
	gTallFaderHandle = 0;

	if (gGlowingTallFaderHandle)
		gGlowingTallFaderHandle->forget();
	gGlowingTallFaderHandle = 0;

	if (gSepModeButton)
		gSepModeButton->forget();
	gSepModeButton = 0;

	if (gScaleButton)
		gScaleButton->forget();
	gScaleButton = 0;

	if (gLegatoButton)
		gLegatoButton->forget();
	gLegatoButton = 0;

	if (gFadesButton)
		gFadesButton->forget();
	gFadesButton = 0;

	if (gFoldoverButton)
		gFoldoverButton->forget();
	gFoldoverButton = 0;

	if (gWiseAmpButton)
		gWiseAmpButton->forget();
	gWiseAmpButton = 0;

	if (gMidiLearnButton)
		gMidiLearnButton->forget();
	gMidiLearnButton = 0;
	if (gMidiResetButton)
		gMidiResetButton->forget();
	gMidiResetButton = 0;

	if (gGoButton)
		gGoButton->forget();
	gGoButton = 0;

	if (gDestroyFXlink)
		gDestroyFXlink->forget();
	gDestroyFXlink = 0;
	if (gSmartElectronixLink)
		gSmartElectronixLink->forget();
	gSmartElectronixLink = 0;
}

//-----------------------------------------------------------------------------
void RezSynthEditor::setParameter(long index, float value)
{
	if (!frame)
		return;

	long sepTag = (((DfxPlugin*)effect)->getparameter_i(kSepMode) == kSepMode_octaval) ? kSepAmount_octaval : kSepAmount_linear;

	// called from RezSynth
	switch (index)
	{
		case kBandwidth:
			if (bandwidthFader)
				bandwidthFader->setValue(value);
			if (bandwidthDisplay)
				bandwidthDisplay->setValue(value);
			break;

		case kNumBands:
			if (numBandsFader)
				numBandsFader->setValue(value);
			if (numBandsDisplay)
				numBandsDisplay->setValue(value);
			break;

		case kSepAmount_octaval:
		case kSepAmount_linear:
			if (sepTag == index)
			{
				if (sepAmountFader)
					sepAmountFader->setValue(value);
				if (sepAmountTextEdit)
				{
					sepAmountDisplayConvert(value, sepAmountString, &sepTag);
					sepAmountTextEdit->setText(sepAmountString);
				}
			}
			break;

		case kSepMode:
			if (sepModeButton)
				sepModeButton->setValue(value);
			// see if we need to swap the parameter assignment for the sep amount controls
			if (sepAmountFader)
			{
				if (sepTag != sepAmountFader->getTag())
				{
					sepAmountFader->setTag(sepTag);
					sepAmountFader->setValue(effect->getParameter(sepTag));
				}
			}
			if (sepAmountTextEdit)
			{
				sepAmountDisplayConvert(effect->getParameter(sepTag), sepAmountString, &sepTag);
				sepAmountTextEdit->setText(sepAmountString);
			}
			break;

		case kAttack:
			if (attackFader)
				attackFader->setValue(value);
			if (attackDisplay)
				attackDisplay->setValue(value);
			break;

		case kRelease:
			if (releaseFader)
				releaseFader->setValue(value);
			if (releaseDisplay)
				releaseDisplay->setValue(value);
			break;

		case kVelCurve:
			if (velCurveFader)
				velCurveFader->setValue(value);
			if (velCurveDisplay)
				velCurveDisplay->setValue(value);
			break;

		case kVelInfluence:
			if (velInfluenceFader)
				velInfluenceFader->setValue(value);
			if (velInfluenceDisplay)
				velInfluenceDisplay->setValue(value);
			break;

		case kPitchbendRange:
			if (pitchbendFader)
				pitchbendFader->setValue(value);
			if (pitchbendDisplay)
				pitchbendDisplay->setValue(value);
			break;

		case kScaleMode:
			if (scaleButton)
				scaleButton->setValue(value);
			break;

		case kGain:
			if (gainFader)
				gainFader->setValue(value);
			if (gainDisplay)
				gainDisplay->setValue(value);
			break;

		case kBetweenGain:
			if (betweenGainFader)
				betweenGainFader->setValue(value);
			if (betweenGainDisplay)
				betweenGainDisplay->setValue(value);
			break;

		case kDryWetMix:
			if (dryWetMixFader)
				dryWetMixFader->setValue(value);
			if (dryWetMixDisplay)
				dryWetMixDisplay->setValue(value);
			break;

		case kLegato:
			if (legatoButton)
				legatoButton->setValue(value);
			break;

		case kFades:
			if (fadesButton)
				fadesButton->setValue(value);
			break;

		case kFoldover:
			if (foldoverButton)
				foldoverButton->setValue(value);
			break;

		case kWiseAmp:
			if (wiseAmpButton)
				wiseAmpButton->setValue(value);
			break;

		default:
			return;
	}	

	postUpdate();
}

//-----------------------------------------------------------------------------
void RezSynthEditor::valueChanged(CDrawContext* context, CControl* control)
{
	long tag = control->getTag();

	switch (tag)
	{
		// process the band separation amount text input
		case kSepAmountTextEdit:
			if (sepAmountTextEdit)
			{
				sepAmountTextEdit->getText(sepAmountString);
				float tempSepAmount;
				long sscanfReturn = sscanf(sepAmountString, "%f", &tempSepAmount);
				if ( (sscanfReturn != EOF) && (sscanfReturn > 0) )
				{
					// no negative values
					tempSepAmount = fabsf(tempSepAmount);
					long sepTag = (((DfxPlugin*)effect)->getparameter_i(kSepMode) == kSepMode_octaval) ? kSepAmount_octaval : kSepAmount_linear;
					// scale the value to a 0.0 to 1.0 parameter value
					if (sepTag == kSepAmount_octaval)
						tempSepAmount = tempSepAmount / ((float)MAX_SEP_AMOUNT*12.0f);
					else
						tempSepAmount = tempSepAmount / (float)MAX_SEP_AMOUNT;
					effect->setParameterAutomated(sepTag, tempSepAmount);
				}
				// there was a sscanf() error
				else
					sepAmountTextEdit->setText("bad");
			}
			break;

		// clicking on these parts of the GUI takes you to Destroy FX or SE web pages
		case kGoButtonID:
			if (goButton)
			{
				if (goButton->mouseIsDown())
					goError = goButton->getError();
				else
					goError = kNoGoDisplay;
				if (goDisplay)
					goDisplay->setDirty();
			}
		case kDestroyFXlinkID:
		case kSmartElectronixLinkID:
			break;

		case kMidiLearnButtonID:
			chunk->setParameterMidiLearn(control->getValue());
			break;
		case kMidiResetButtonID:
			chunk->setParameterMidiReset(control->getValue());
			break;


		case kScaleMode:
			effect->setParameterAutomated(tag, control->getValue());
			// this is a multi-state switch, so use multi-state toggle MIDI control
			chunk->setLearner(tag, kEventBehaviourToggle, kNumScaleModes);
			break;

		case kSepMode:
		case kLegato:
		case kFades:
		case kFoldover:
		case kWiseAmp:
			effect->setParameterAutomated(tag, control->getValue());
			// these are on/off buttons, so use toggle MIDI control
			chunk->setLearner(tag, kEventBehaviourToggle);
			break;

		case kSepAmount_octaval:
		case kSepAmount_linear:
			effect->setParameterAutomated(tag, control->getValue());
			chunk->setLearner(tag);
			if (chunk->isLearning())
			{
				if (sepAmountFader != NULL)
				{
					if ( (sepAmountFader->getTag() == tag) && 
							(sepAmountFader->getHandle() == gFaderHandle) )
					{
						sepAmountFader->setHandle(gGlowingFaderHandle);
						sepAmountFader->setDirty();
					}
				}
			}
			break;

		case kBandwidth:
		case kNumBands:
		case kAttack:
		case kRelease:
		case kVelCurve:
		case kVelInfluence:
		case kPitchbendRange:
		case kGain:
		case kBetweenGain:
		case kDryWetMix:
			effect->setParameterAutomated(tag, control->getValue());

			chunk->setLearner(tag);
			if (chunk->isLearning())
			{
				if (faders[tag] != NULL)
				{
					if (faders[tag]->getHandle() == gFaderHandle)
					{
						faders[tag]->setHandle(gGlowingFaderHandle);
						faders[tag]->setDirty();
					}
				}
				else if (tallFaders[tag] != NULL)
				{
					if (tallFaders[tag]->getHandle() == gTallFaderHandle)
					{
						tallFaders[tag]->setHandle(gGlowingTallFaderHandle);
						tallFaders[tag]->setDirty();
					}
				}
			}

			break;

		default:
			break;
	}

	control->update(context);
}


//-----------------------------------------------------------------------------
void RezSynthEditor::idle()
{
  bool somethingChanged = false;


	for (long i=0; i < NUM_PARAMETERS; i++)
	{
		if (i != chunk->getLearner())
		{
			if (faders[i] != NULL)
			{
				if (faders[i]->getHandle() == gGlowingFaderHandle)
				{
					faders[i]->setHandle(gFaderHandle);
					faders[i]->setDirty();
					somethingChanged = true;
				}
			}			
			else if (tallFaders[i] != NULL)
			{
				if (tallFaders[i]->getHandle() == gGlowingTallFaderHandle)
				{
					tallFaders[i]->setHandle(gTallFaderHandle);
					tallFaders[i]->setDirty();
					somethingChanged = true;
				}
			}
		}
	}
	//
	if (sepAmountFader != NULL)
	{
		if ( !chunk->isLearner(sepAmountFader->getTag()) && 
				(sepAmountFader->getHandle() == gGlowingFaderHandle) )
		{
			sepAmountFader->setHandle(gFaderHandle);
			sepAmountFader->setDirty();
			somethingChanged = true;
		}
		else if ( chunk->isLearner(sepAmountFader->getTag()) && 
				(sepAmountFader->getHandle() == gFaderHandle) )
		{
			sepAmountFader->setHandle(gGlowingFaderHandle);
			sepAmountFader->setDirty();
			somethingChanged = true;
		}
	}

	// indicate that changed controls need to be redrawn
	if (somethingChanged)
		postUpdate();

	// this is called so that idle() actually happens
	AEffGUIEditor::idle();
}
