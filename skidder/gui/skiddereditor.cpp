#ifndef __SKIDDEREDITOR_H
#include "skiddereditor.hpp"
#endif

#include <stdio.h>


//-----------------------------------------------------------------------------
enum {
	// resource IDs
	kBackgroundID = 128,
	kFaderSlideID,
	kFaderHandleID,
	kGlowingFaderHandleID,
	kFaderHandleLeftID,
	kGlowingFaderHandleLeftID,
	kFaderHandleRightID,
	kGlowingFaderHandleRightID,
	kTempoSyncButtonID,
	kGoButtonID,
	kDestroyFXlinkID,
	kSmartElectronixLinkID,
	kMidiModeButtonID,
	kVelocityButtonID,
	kMidiLearnButtonID,
	kMidiResetButtonID,
	kConnectButtonID,

	// positions
	kFaderX = 36,
	kFaderY = 59,
	kFaderInc = 40,

	kDisplayX = 288,
	kDisplayY = kFaderY + 1,
	kDisplayWidth = 360 - kDisplayX,
	kDisplayHeight = 12,

	kTempoSyncButtonX = 276,
	kTempoSyncButtonY = 79,

	kGoButtonX = 18,
	kGoButtonY = 338,

	kMidiModeButtonX = 198,
	kMidiModeButtonY = 338,

	kVelocityButtonX = kMidiModeButtonX + 11,
	kVelocityButtonY = kMidiModeButtonY + 19,

	kMidiLearnButtonX = 120,
	kMidiLearnButtonY = 338,
	kMidiResetButtonX = kMidiLearnButtonX,
	kMidiResetButtonY = kMidiLearnButtonY + 18,

	kRandomMinimumWidth = 87,
	kRandomMinimumX = kDisplayX - kRandomMinimumWidth - 9,
	kRandomMinimumIncY = 19,

	kDestroyFXlinkX = 286,
	kDestroyFXlinkY = 373 - 19,
	kSmartElectronixLinkX = 191,
	kSmartElectronixLinkY = 390 - 19,

	kTempoTextEdit = 333,
	kNoGoDisplay = 33333
};



// goofy stuff that we have to do because this variable is static, 
// & it's static because tempoRateDisplayConvert() is static,  & that's static 
// so that it can be a class member & still be acceptable to setStringConvert()
float SkidderEditor::theTempoSync;


//-----------------------------------------------------------------------------
// parameter value string display conversion functions

void SkidderEditor::rateDisplayConvert(float value, char *string, void *temporatestring)
{
	if ( onOffTest(theTempoSync) )
	{
		strcpy(string, (char*)temporatestring);
		if (strlen(string) <= 3)
			strcat(string, "  cycles/beat");
	}
	else
		sprintf(string, "%.3f  Hz", rateScaled(value));
}

void pulsewidthDisplayConvert(float value, char *string);
void pulsewidthDisplayConvert(float value, char *string)
{
	sprintf(string, "%.3f", pulsewidthScaled(value));
}

void pulsewidthRandMinDisplayConvert(float value, char *string, void *fpulsewidth);
void pulsewidthRandMinDisplayConvert(float value, char *string, void *fpulsewidth)
{
	if ( *(float*)fpulsewidth > value )
		sprintf(string, "%.3f", pulsewidthScaled(value));
	else
		strcpy(string, " ");
}

void randomMinimumDisplayConvert(float value, char *string, void *fpulsewidth);
void randomMinimumDisplayConvert(float value, char *string, void *fpulsewidth)
{
	if ( *(float*)fpulsewidth > value )
		strcpy(string, "random  minimum :");
	else
		strcpy(string, " ");
}

void slopeDisplayConvert(float value, char *string);
void slopeDisplayConvert(float value, char *string)
{
	sprintf(string, "%.3f  ms", (value*SLOPEMAX));
}

void floorDisplayConvert(float value, char *string);
void floorDisplayConvert(float value, char *string)
{
	if (value <= 0.0f)
		strcpy(string, "-oo  dB");
	else
		sprintf(string, "%.2f  dB", linear2dB(gainScaled(value)));
}

void floorRandMinDisplayConvert(float value, char *string, void *ffloor);
void floorRandMinDisplayConvert(float value, char *string, void *ffloor)
{
	if ( *(float*)ffloor > value )
		floorDisplayConvert(value, string);
	else
		strcpy(string, " ");
}

void panDisplayConvert(float value, char *string);
void panDisplayConvert(float value, char *string)
{
	sprintf(string, "%.3f", value);
}

void noiseDisplayConvert(float value, char *string);
void noiseDisplayConvert(float value, char *string)
{
	sprintf(string, "%ld", (long)(value*value*18732));
}

void goDisplayConvert(float value, char *string, void *goErr);
void goDisplayConvert(float value, char *string, void *goErr)
{
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
SkidderEditor::SkidderEditor(AudioEffect *effect)
 : AEffGUIEditor(effect) 
{
	frame = 0;
	isOpen = false;

	// initialize the graphics pointers
	gBackground = 0;
	gFaderSlide = 0;
	gFaderHandle = 0;
	gGlowingFaderHandle = 0;
	gFaderHandleLeft = 0;
	gGlowingFaderHandleLeft = 0;
	gFaderHandleRight = 0;
	gGlowingFaderHandleRight = 0;
	gTempoSyncButton = 0;
	gMidiLearnButton = 0;
	gMidiResetButton = 0;
	gGoButton = 0;
	gDestroyFXlink = 0;
	gSmartElectronixLink = 0;

	// initialize the controls pointers
	rateFader = 0;
	tempoSyncButton = 0;
	tempoFader = 0;
	pulsewidthFader = 0;
	slopeFader = 0;
	floorFader = 0;
	panFader = 0;
	noiseFader = 0;
	midiLearnButton = 0;
	midiResetButton = 0;
	goButton = 0;
	DestroyFXlink = 0;
	SmartElectronixLink = 0;

	// initialize the value display box pointers
	rateDisplay = 0;
	tempoTextEdit = 0;
	pulsewidthDisplay = 0;
	pulsewidthRandMinDisplay = 0;
	randomMinimumDisplay = 0;
	slopeDisplay = 0;
	floorDisplay = 0;
	floorRandMinDisplay = 0;
	randomMinimum2Display = 0;
	panDisplay = 0;
	noiseDisplay = 0;
	goDisplay = 0;

#ifdef MSKIDDER
	gMidiModeButton = 0;
	gVelocityButton = 0;
	midiModeButton = 0;
	velocityButton = 0;
#endif

	// load the background bitmap
	// we don't need to load all bitmaps, this could be done when open is called
	gBackground = new CBitmap (kBackgroundID);

	// init the size of the plugin
	rect.left   = 0;
	rect.top    = 0;
	rect.right  = (short)gBackground->getWidth();
	rect.bottom = (short)gBackground->getHeight();

	tempoString = new char[256];
	tempoRateString = new char[16];
	chunk = ((DfxPlugin*)effect)->getsettings_ptr();	// this just simplifies pointing

	faders = 0;
	faders = (CHorizontalSlider**)malloc(sizeof(CHorizontalSlider*)*NUM_PARAMETERS);
	for (long i=0; i < NUM_PARAMETERS; i++)
		faders[i] = NULL;
}

//-----------------------------------------------------------------------------
SkidderEditor::~SkidderEditor()
{
	// free background bitmap
	if (gBackground)
		gBackground->forget();
	gBackground = 0;

	free(faders);
	faders = 0;

	if (tempoString)
		delete[] tempoString;
	if (tempoRateString)
		delete[] tempoRateString;
}

//-----------------------------------------------------------------------------
long SkidderEditor::getRect(ERect **erect)
{
	*erect = &rect;
	return true;
}

//-----------------------------------------------------------------------------
long SkidderEditor::open(void *ptr)
{
  CPoint displayOffset;	// for positioning the background graphic behind display boxes


	// !!! always call this !!!
	AEffGUIEditor::open(ptr);

	// load some bitmaps
	if (!gFaderSlide)
		gFaderSlide = new CBitmap (kFaderSlideID);

	if (!gFaderHandle)
		gFaderHandle = new CBitmap (kFaderHandleID);
	if (!gGlowingFaderHandle)
		gGlowingFaderHandle = new CBitmap (kGlowingFaderHandleID);
	if (!gFaderHandleLeft)
		gFaderHandleLeft = new CBitmap (kFaderHandleLeftID);
	if (!gGlowingFaderHandleLeft)
		gGlowingFaderHandleLeft = new CBitmap (kGlowingFaderHandleLeftID);
	if (!gFaderHandleRight)
		gFaderHandleRight = new CBitmap (kFaderHandleRightID);
	if (!gGlowingFaderHandleRight)
		gGlowingFaderHandleRight = new CBitmap (kGlowingFaderHandleRightID);

	if (!gTempoSyncButton)
		gTempoSyncButton = new CBitmap (kTempoSyncButtonID);

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

#ifdef MSKIDDER
	if (!gMidiModeButton)
		gMidiModeButton = new CBitmap (kMidiModeButtonID);
	if (!gVelocityButton)
		gVelocityButton = new CBitmap (kVelocityButtonID);
#endif


	chunk->resetLearning();
	goError = kNoGoDisplay;
	long rateTag = ((DfxPlugin*)effect)->getparameter_b(kTempoSync) ? kRate_sync : kRate_abs;


	//--initialize the background frame--------------------------------------
	CRect size (0, 0, gBackground->getWidth(), gBackground->getHeight());
	frame = new CFrame (size, ptr, this);
	frame->setBackground(gBackground);


	//--initialize the faders-----------------------------------------------
	int minPos = kFaderX;
	int maxPos = kFaderX + gFaderSlide->getWidth() - gFaderHandle->getWidth() - 1;
	CPoint point (0, 0);
	CPoint offset (0, 1);

	// rate   (single slider for "free" Hz rate & tempo synced rate)
	// size stores left, top, right, & bottom positions
	size (kFaderX, kFaderY, kFaderX + gFaderSlide->getWidth(), kFaderY + gFaderSlide->getHeight());
	rateFader = new CHorizontalSlider (size, this, rateTag, minPos, maxPos, gFaderHandle, gFaderSlide, point, kLeft);
	rateFader->setOffsetHandle(offset);
	rateFader->setValue(effect->getParameter(rateTag));
	rateFader->setDefaultValue(rateUnscaled(3.0f));
	frame->addView(rateFader);

	// tempo (in bpm)
	size.offset (0, kFaderInc);
	tempoFader = new CHorizontalSlider (size, this, kTempo, minPos, maxPos, gFaderHandle, gFaderSlide, point, kLeft);
	tempoFader->setOffsetHandle(offset);
	tempoFader->setValue(effect->getParameter(kTempo));
	tempoFader->setDefaultValue(0.0f);
	frame->addView(tempoFader);

	int minRPos = kFaderX;
	int maxRPos = kFaderX + gFaderSlide->getWidth();
	// pulsewidth
	size.offset (0, kFaderInc);
	pulsewidthFader = new CHorizontalRangeSlider (size, this, kPulsewidthRandMin, kPulsewidth, minRPos, maxRPos, gFaderHandleLeft, gFaderSlide, point, kLeft, kMaxCanPush, gFaderHandleRight);
	pulsewidthFader->setOffsetHandle(point);
	pulsewidthFader->setValueTagged(kPulsewidth, effect->getParameter(kPulsewidth));
	pulsewidthFader->setValueTagged(kPulsewidthRandMin, effect->getParameter(kPulsewidthRandMin));
	pulsewidthFader->setDefaultValueTagged(kPulsewidth, 0.5f);
	pulsewidthFader->setDefaultValueTagged(kPulsewidthRandMin, 0.5f);
	frame->addView(pulsewidthFader);

	// slope
	size.offset (0, kFaderInc);
	slopeFader = new CHorizontalSlider (size, this, kSlope, minPos, maxPos, gFaderHandle, gFaderSlide, point, kLeft);
	slopeFader->setOffsetHandle(offset);
	slopeFader->setValue(effect->getParameter(kSlope));
	slopeFader->setDefaultValue(3.0f/SLOPEMAX);
	frame->addView(slopeFader);

	// floor
	size.offset (0, kFaderInc);
	floorFader = new CHorizontalRangeSlider (size, this, kFloorRandMin, kFloor, minRPos, maxRPos, gFaderHandleLeft, gFaderSlide, point, kLeft, kMaxCanPush, gFaderHandleRight);
	floorFader->setOffsetHandle(point);
	floorFader->setValueTagged(kFloor, effect->getParameter(kFloor));
	floorFader->setValueTagged(kFloorRandMin, effect->getParameter(kFloorRandMin));
	floorFader->setDefaultValueTagged(kFloor, 0.0f);
	floorFader->setDefaultValueTagged(kFloorRandMin, 0.0f);
	frame->addView(floorFader);

	// pan
	size.offset (0, kFaderInc);
	panFader = new CHorizontalSlider (size, this, kPan, minPos, maxPos, gFaderHandle, gFaderSlide, point, kLeft);
	panFader->setOffsetHandle(offset);
	panFader->setValue(effect->getParameter(kPan));
	panFader->setDefaultValue(0.6f);
	frame->addView(panFader);

	// noise
	size.offset (0, kFaderInc);
	noiseFader = new CHorizontalSlider (size, this, kNoise, minPos, maxPos, gFaderHandle, gFaderSlide, point, kLeft);
	noiseFader->setOffsetHandle(offset);
	noiseFader->setValue(effect->getParameter(kNoise));
	noiseFader->setDefaultValue(1.0f);
	frame->addView(noiseFader);


	//--initialize the buttons----------------------------------------------

	// choose the rate type ("free" or synced)
	size (kTempoSyncButtonX, kTempoSyncButtonY, kTempoSyncButtonX + gTempoSyncButton->getWidth(), kTempoSyncButtonY + (gTempoSyncButton->getHeight())/2);
	tempoSyncButton = new COnOffButton (size, this, kTempoSync, gTempoSyncButton);
	tempoSyncButton->setValue(effect->getParameter(kTempoSync));
	frame->addView(tempoSyncButton);

	// go!
	// size stores left, top, right, & bottom positions
	size (kGoButtonX, kGoButtonY, kGoButtonX + gGoButton->getWidth(), kGoButtonY + (gGoButton->getHeight())/2);
	goButton = new CWebLink (size, this, kGoButtonID, "http://www.whirledbank.org/", gGoButton);
	frame->addView(goButton);

	// Destroy FX web page link
	size (kDestroyFXlinkX, kDestroyFXlinkY, kDestroyFXlinkX + gDestroyFXlink->getWidth(), kDestroyFXlinkY + (gDestroyFXlink->getHeight())/2);
	DestroyFXlink = new CWebLink (size, this, kDestroyFXlinkID, DESTROYFX_URL, gDestroyFXlink);
	frame->addView(DestroyFXlink);

	// Smart Electronix web page link
	size (kSmartElectronixLinkX, kSmartElectronixLinkY, kSmartElectronixLinkX + gSmartElectronixLink->getWidth(), kSmartElectronixLinkY + (gSmartElectronixLink->getHeight())/2);
	SmartElectronixLink = new CWebLink (size, this, kSmartElectronixLinkID, SMARTELECTRONIX_URL, gSmartElectronixLink);
	frame->addView(SmartElectronixLink);

	// MIDI note control mode button
	size (kMidiModeButtonX, kMidiModeButtonY, kMidiModeButtonX + gMidiModeButton->getWidth(), kMidiModeButtonY + (gMidiModeButton->getHeight())/kNumMidiModes);
	midiModeButton = new CMultiToggle (size, this, kMidiMode, kNumMidiModes, gMidiModeButton->getHeight()/kNumMidiModes, gMidiModeButton, point);
	midiModeButton->setValue(effect->getParameter(kMidiMode));
	frame->addView(midiModeButton);

	// use-note-velocity button
	size (kVelocityButtonX, kVelocityButtonY, kVelocityButtonX + gVelocityButton->getWidth(), kVelocityButtonY + (gVelocityButton->getHeight())/2);
	velocityButton = new COnOffButton (size, this, kVelocity, gVelocityButton);
	velocityButton->setValue(effect->getParameter(kVelocity));
	frame->addView(velocityButton);

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


	//--initialize the displays---------------------------------------------

	// first store the proper values for all of the globals so that displays are correct
	strcpy( tempoRateString, ((Skidder*)effect)->tempoRateTable->getDisplay_gen(effect->getParameter(kRate_sync)) );
	theTempoSync = effect->getParameter(kTempoSync);

	// rate   (unified display for "free" Hz rate & tempo synced rate)
	size (kDisplayX, kDisplayY, kDisplayX + kDisplayWidth, kDisplayY + kDisplayHeight);
	rateDisplay = new CParamDisplay (size, gBackground);
	displayOffset (size.left, size.top);
	rateDisplay->setBackOffset(displayOffset);
	rateDisplay->setHoriAlign(kLeftText);
	rateDisplay->setFont(kNormalFontSmall);
	rateDisplay->setFontColor(kWhiteCColor);
	rateDisplay->setValue(effect->getParameter(rateTag));
	rateDisplay->setStringConvert(rateDisplayConvert, tempoRateString);
	rateDisplay->setTag(rateTag);
	frame->addView(rateDisplay);

	// tempo (in bpm)   (editable)
	size.offset (0, kFaderInc);
	tempoTextEdit = new CTextEdit (size, this, kTempoTextEdit, 0, gBackground);
	displayOffset (size.left, size.top);
	tempoTextEdit->setBackOffset(displayOffset);
	tempoTextEdit->setFont (kNormalFontSmall);
	tempoTextEdit->setFontColor (kWhiteCColor);
	tempoTextEdit->setHoriAlign (kLeftText);
	frame->addView (tempoTextEdit);
	// this makes it display the current value
	setParameter(kTempo, effect->getParameter(kTempo));

	// pulsewidth
	size.offset (0, kFaderInc - 1);
	pulsewidthDisplay = new CNumberBox (size, this, kPulsewidth, gBackground, 0, kVertical);
	displayOffset (size.left, size.top);
	pulsewidthDisplay->setBackOffset(displayOffset);
	pulsewidthDisplay->setHoriAlign(kLeftText);
	pulsewidthDisplay->setFont(kNormalFontSmall);
	pulsewidthDisplay->setFontColor(kWhiteCColor);
	pulsewidthDisplay->setValue(effect->getParameter(kPulsewidth));
	pulsewidthDisplay->setStringConvert(pulsewidthDisplayConvert);
	pulsewidthDisplay->setTag(kPulsewidth);
	frame->addView(pulsewidthDisplay);

	// pulsewidth random minimum
	size.offset (1, kRandomMinimumIncY + 1);
	pulsewidthRandMinDisplay = new CParamDisplay (size, gBackground);
	displayOffset (size.left, size.top);
	pulsewidthRandMinDisplay->setBackOffset(displayOffset);
	pulsewidthRandMinDisplay->setHoriAlign(kLeftText);
	pulsewidthRandMinDisplay->setFont(kNormalFontVerySmall);
	pulsewidthRandMinDisplay->setFontColor(kWhiteCColor);
	pulsewidthRandMinDisplay->setValue(effect->getParameter(kPulsewidthRandMin));
	pulsewidthRandMinDisplay->setStringConvert( pulsewidthRandMinDisplayConvert, &(((Skidder*)effect)->fPulsewidth) );
	pulsewidthRandMinDisplay->setTag(kPulsewidthRandMin);
	frame->addView(pulsewidthRandMinDisplay);

	// the words "random minimum"
	size (kRandomMinimumX, kDisplayY + (kFaderInc*2) + kRandomMinimumIncY, kRandomMinimumX + kRandomMinimumWidth, kDisplayY + (kFaderInc*2) + kRandomMinimumIncY + kDisplayHeight);
	randomMinimumDisplay = new CParamDisplay (size, gBackground);
	displayOffset (size.left, size.top);
	randomMinimumDisplay->setBackOffset(displayOffset);
	randomMinimumDisplay->setHoriAlign(kRightText);
	randomMinimumDisplay->setFont(kNormalFontVerySmall);
	randomMinimumDisplay->setFontColor(kWhiteCColor);
	randomMinimumDisplay->setValue(effect->getParameter(kPulsewidthRandMin));
	randomMinimumDisplay->setStringConvert( randomMinimumDisplayConvert, &(((Skidder*)effect)->fPulsewidth) );
	randomMinimumDisplay->setTag(kPulsewidthRandMin);
	frame->addView(randomMinimumDisplay);

	// slope
	size (kDisplayX, kDisplayY + (kFaderInc*3), kDisplayX + kDisplayWidth, kDisplayY + (kFaderInc*3) + kDisplayHeight);
	slopeDisplay = new CParamDisplay (size, gBackground);
	displayOffset (size.left, size.top);
	slopeDisplay->setBackOffset(displayOffset);
	slopeDisplay->setHoriAlign(kLeftText);
	slopeDisplay->setFont(kNormalFontSmall);
	slopeDisplay->setFontColor(kWhiteCColor);
	slopeDisplay->setValue(effect->getParameter(kSlope));
	slopeDisplay->setStringConvert(slopeDisplayConvert);
	frame->addView(slopeDisplay);

	// floor
	size.offset (0, kFaderInc - 1);
	floorDisplay = new CParamDisplay (size, gBackground);
	displayOffset (size.left, size.top);
	floorDisplay->setBackOffset(displayOffset);
	floorDisplay->setHoriAlign(kLeftText);
	floorDisplay->setFont(kNormalFontSmall);
	floorDisplay->setFontColor(kWhiteCColor);
	floorDisplay->setValue(effect->getParameter(kFloor));
	floorDisplay->setStringConvert(floorDisplayConvert);
	frame->addView(floorDisplay);

	// floor random minimum
	size.offset (1, kRandomMinimumIncY + 1);
	floorRandMinDisplay = new CParamDisplay (size, gBackground);
	displayOffset (size.left, size.top);
	floorRandMinDisplay->setBackOffset(displayOffset);
	floorRandMinDisplay->setHoriAlign(kLeftText);
	floorRandMinDisplay->setFont(kNormalFontVerySmall);
	floorRandMinDisplay->setFontColor(kWhiteCColor);
	floorRandMinDisplay->setValue(effect->getParameter(kFloorRandMin));
	floorRandMinDisplay->setStringConvert( floorRandMinDisplayConvert, &(((Skidder*)effect)->fFloor) );
	floorRandMinDisplay->setTag(kFloorRandMin);
	frame->addView(floorRandMinDisplay);

	// the words "random minimum" again
	size (kRandomMinimumX, kDisplayY + (kFaderInc*4) + kRandomMinimumIncY, kRandomMinimumX + kRandomMinimumWidth, kDisplayY + (kFaderInc*4) + kRandomMinimumIncY + kDisplayHeight);
	randomMinimum2Display = new CParamDisplay (size, gBackground);
	displayOffset (size.left, size.top);
	randomMinimum2Display->setBackOffset(displayOffset);
	randomMinimum2Display->setHoriAlign(kRightText);
	randomMinimum2Display->setFont(kNormalFontVerySmall);
	randomMinimum2Display->setFontColor(kWhiteCColor);
	randomMinimum2Display->setValue(effect->getParameter(kFloorRandMin));
	randomMinimum2Display->setStringConvert( randomMinimumDisplayConvert, &(((Skidder*)effect)->fFloor) );
	randomMinimum2Display->setTag(kFloorRandMin);
	frame->addView(randomMinimum2Display);

	// pan
	size (kDisplayX, kDisplayY + (kFaderInc*5), kDisplayX + kDisplayWidth, kDisplayY + (kFaderInc*5) + kDisplayHeight);
	panDisplay = new CParamDisplay (size, gBackground);
	displayOffset (size.left, size.top);
	panDisplay->setBackOffset(displayOffset);
	panDisplay->setHoriAlign(kLeftText);
	panDisplay->setFont(kNormalFontSmall);
	panDisplay->setFontColor(kWhiteCColor);
	panDisplay->setValue(effect->getParameter(kPan));
	panDisplay->setStringConvert(panDisplayConvert);
	frame->addView(panDisplay);

	// noise
	size.offset (0, kFaderInc);
	noiseDisplay = new CParamDisplay (size, gBackground);
	displayOffset (size.left, size.top);
	noiseDisplay->setBackOffset(displayOffset);
	noiseDisplay->setHoriAlign(kLeftText);
	noiseDisplay->setFont(kNormalFontSmall);
	noiseDisplay->setFontColor(kWhiteCColor);
	noiseDisplay->setValue(effect->getParameter(kNoise));
	noiseDisplay->setStringConvert(noiseDisplayConvert);
	frame->addView(noiseDisplay);

	// go! error/success display
	size (kGoButtonX, kGoButtonY + 17, kGoButtonX + gGoButton->getWidth(), kGoButtonY + kDisplayHeight + 17);
	goDisplay = new CParamDisplay (size, gBackground);
	displayOffset (size.left, size.top);
	goDisplay->setBackOffset(displayOffset);
	goDisplay->setHoriAlign(kCenterText);
	goDisplay->setFont(kNormalFontSmall);
	goDisplay->setFontColor(kWhiteCColor);
	goDisplay->setValue(0.0f);
	goDisplay->setStringConvert(goDisplayConvert, &goError);
	frame->addView(goDisplay);


	for (long i=0; i < NUM_PARAMETERS; i++)
		faders[i] = NULL;
	faders[kTempo] = tempoFader;
	faders[kSlope] = slopeFader;
	faders[kPan] = panFader;
	faders[kNoise] = noiseFader;


	isOpen = true;

	return true;
}

//-----------------------------------------------------------------------------
void SkidderEditor::close()
{
	isOpen = false;
	if (frame)
		delete frame;
	frame = 0;

	for (long i=0; i < NUM_PARAMETERS; i++)
		faders[i] = NULL;

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

	if (gFaderHandleLeft)
		gFaderHandleLeft->forget();
	gFaderHandleLeft = 0;
	if (gGlowingFaderHandleLeft)
		gGlowingFaderHandleLeft->forget();
	gGlowingFaderHandleLeft = 0;

	if (gFaderHandleRight)
		gFaderHandleRight->forget();
	gFaderHandleRight = 0;
	if (gGlowingFaderHandleRight)
		gGlowingFaderHandleRight->forget();
	gGlowingFaderHandleRight = 0;

	if (gTempoSyncButton)
		gTempoSyncButton->forget();
	gTempoSyncButton = 0;

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

#ifdef MSKIDDER
	if (gMidiModeButton)
		gMidiModeButton->forget();
	gMidiModeButton = 0;
	if (gVelocityButton)
		gVelocityButton->forget();
	gVelocityButton = 0;
#endif
}

//-----------------------------------------------------------------------------
// called from SkidderEdit
void SkidderEditor::setParameter(long index, float value)
{
	if (!frame)
		return;

	long rateTag = ((DfxPlugin*)effect)->getparameter_b(kTempoSync) ? kRate_sync : kRate_abs;

	switch (index)
	{
		case kRate_abs:
		case kRate_sync:
			// store these into the static global variables so that the string convert function can see them
			strcpy( tempoRateString, ((Skidder*)effect)->tempoRateTable->getDisplay_gen(effect->getParameter(kRate_sync)) );
			theTempoSync = effect->getParameter(kTempoSync);
			if (rateTag == index)
			{
				if (rateFader)
					rateFader->setValue(value);
				if (rateDisplay)
					rateDisplay->setValue(value);
			}
			break;

		case kTempoSync:
			strcpy( tempoRateString, ((Skidder*)effect)->tempoRateTable->getDisplay_gen(effect->getParameter(kRate_sync)) );
			theTempoSync = effect->getParameter(kTempoSync);
			if (tempoSyncButton)
				tempoSyncButton->setValue(value);
			// see if we need to swap the parameter assignment for the rate controls
			if (rateFader)
			{
				if (rateTag != rateFader->getTag())
				{
					rateFader->setTag(rateTag);
					rateFader->setValue(effect->getParameter(rateTag));
				}
			}
			if (rateDisplay)
			{
				if (rateTag != rateDisplay->getTag())
				{
					rateDisplay->setTag(rateTag);
					rateDisplay->setValue(effect->getParameter(rateTag));
					rateDisplay->setDirty();
				}
			}
			break;

		case kTempo:
			if (tempoFader)
				tempoFader->setValue(value);
			if (tempoTextEdit)
			{
				if ( (value > 0.0f) || (((Skidder*)effect)->hostCanDoTempo != 1) )
					sprintf(tempoString, "%.3f  bpm", tempoScaled(value));
				else
					strcpy(tempoString, "auto");
				tempoTextEdit->setText(tempoString);
			}
			break;

		case kPulsewidth:
			if (pulsewidthFader)
				pulsewidthFader->setValueTagged(index, value);
			if (pulsewidthDisplay)
				pulsewidthDisplay->setValue(value);
			if (randomMinimumDisplay)
				randomMinimumDisplay->setValue(effect->getParameter(kPulsewidthRandMin));
			if (pulsewidthRandMinDisplay)
				pulsewidthRandMinDisplay->setValue(effect->getParameter(kPulsewidthRandMin));
			// because the regular pulsewidth can affect the random's read-out
			if (pulsewidthRandMinDisplay)
				pulsewidthRandMinDisplay->setDirty();
			if (randomMinimumDisplay)
				randomMinimumDisplay->setDirty();
			break;

		case kPulsewidthRandMin:
			if (pulsewidthFader)
				pulsewidthFader->setValueTagged(index, value);
			if (pulsewidthRandMinDisplay)
				pulsewidthRandMinDisplay->setValue(value);
			if (randomMinimumDisplay)
				randomMinimumDisplay->setValue(value);
			if (pulsewidthDisplay)
				pulsewidthDisplay->setValue(effect->getParameter(kPulsewidth));
			break;

		case kSlope:
			if (slopeFader)
				slopeFader->setValue(value);
			if (slopeDisplay)
				slopeDisplay->setValue(value);
			break;

		case kFloor:
			if (floorFader)
				floorFader->setValueTagged(index, value);
			if (floorDisplay)
				floorDisplay->setValue(value);
			// because the regular floor can affect the random's read-out
			if (floorRandMinDisplay)
				floorRandMinDisplay->setDirty();
			if (randomMinimum2Display)
				randomMinimum2Display->setDirty();
			break;

		case kFloorRandMin:
			if (floorFader)
				floorFader->setValueTagged(index, value);
			if (floorRandMinDisplay)
				floorRandMinDisplay->setValue(value);
			if (randomMinimum2Display)
				randomMinimum2Display->setValue(value);
			break;

		case kPan:
			if (panFader)
				panFader->setValue(value);
			if (panDisplay)
				panDisplay->setValue(value);
			break;

		case kNoise:
			if (noiseFader)
				noiseFader->setValue(value);
			if (noiseDisplay)
				noiseDisplay->setValue(value);
			break;

	#ifdef MSKIDDER
		case kMidiMode:
			if (midiModeButton)
				midiModeButton->setValue(value);
			break;
		case kVelocity:
			if (velocityButton)
				velocityButton->setValue(value);
			break;
	#endif

		default:
			return;
	}

	postUpdate();
}

//-----------------------------------------------------------------------------
void SkidderEditor::valueChanged(CDrawContext* context, CControl* control)
{
  long tag = control->getTag();
  float tempTempo;
  long sscanfReturn;


	switch (tag)
	{
		// process the tempo text input
		case kTempoTextEdit:
			if (tempoTextEdit)
			{
				tempoTextEdit->getText(tempoString);
				sscanfReturn = sscanf(tempoString, "%f", &tempTempo);
				if (strcmp(tempoString, "auto") == 0)
					effect->setParameterAutomated(kTempo, 0.0f);
				else if ( (sscanfReturn != EOF) && (sscanfReturn > 0) )
				{
					// check if the user typed in something that's not a number
					if (tempTempo == 0.0f)
						tempoTextEdit->setText("very bad");
					// the user typed in a number
					else
					{
						// no negative tempos
						if (tempTempo < 0.0f)
							tempTempo = -tempTempo;
						// if the tempo entered is 0, then leave it at 0.0 as the parameter value
						if (tempTempo > 0.0f)
							// scale the value to a 0.0 to 1.0 parameter value
							tempTempo = tempoUnscaled(tempTempo);
						// this updates the display with "bpm" appended & the fader position
						effect->setParameterAutomated(kTempo, tempTempo);
					}
				}
				// there was a sscanf() error
				else
					tempoTextEdit->setText("bad");
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
			chunk->setParameterMidiLearn(control->getValue() > 0.5f);
			break;
		case kMidiResetButtonID:
			chunk->setParameterMidiReset(control->getValue() > 0.5f);
			break;


		case kPulsewidth:
		case kPulsewidthRandMin:
			if (pulsewidthFader)
			{
				effect->setParameterAutomated(kPulsewidth, pulsewidthFader->getValueTagged(kPulsewidth));
				effect->setParameterAutomated(kPulsewidthRandMin, pulsewidthFader->getValueTagged(kPulsewidthRandMin));
				chunk->setLearner(pulsewidthFader->getTag());
				// set the automation link mode for the pulsewidth range slider
				if (pulsewidthFader->getClickBetween())
					((Skidder*)effect)->pulsewidthDoubleAutomate = true;
				else
					((Skidder*)effect)->pulsewidthDoubleAutomate = false;
				if (chunk->isLearning())
				{
					if ( ((pulsewidthFader->getTag() == kPulsewidthRandMin) || 
								((Skidder*)effect)->pulsewidthDoubleAutomate) && 
							(pulsewidthFader->getHandle() == gFaderHandleLeft) )
					{
						pulsewidthFader->setHandle(gGlowingFaderHandleLeft);
						pulsewidthFader->setDirty();
					}
					if ( ((pulsewidthFader->getTag() == kPulsewidth) || 
								((Skidder*)effect)->pulsewidthDoubleAutomate) && 
							(pulsewidthFader->getHandle2() == gFaderHandleRight) )
					{
						pulsewidthFader->setHandle2(gGlowingFaderHandleRight);
						pulsewidthFader->setDirty();
					}
				}
			}
			break;

		case kFloor:
		case kFloorRandMin:
			if (floorFader)
			{
				effect->setParameterAutomated(kFloor, floorFader->getValueTagged(kFloor));
				effect->setParameterAutomated(kFloorRandMin, floorFader->getValueTagged(kFloorRandMin));
				chunk->setLearner(floorFader->getTag());
				// set the automation link mode for the floor range slider
				if (floorFader->getClickBetween())
					((Skidder*)effect)->floorDoubleAutomate = true;
				else
					((Skidder*)effect)->floorDoubleAutomate = false;
				if (chunk->isLearning())
				{
					if ( ((floorFader->getTag() == kFloorRandMin) || 
								((Skidder*)effect)->floorDoubleAutomate) && 
							(floorFader->getHandle() == gFaderHandleLeft) )
					{
						floorFader->setHandle(gGlowingFaderHandleLeft);
						floorFader->setDirty();
					}
					if ( ((floorFader->getTag() == kFloor) || 
								((Skidder*)effect)->floorDoubleAutomate) && 
							(floorFader->getHandle2() == gFaderHandleRight) )
					{
						floorFader->setHandle2(gGlowingFaderHandleRight);
						floorFader->setDirty();
					}
				}
			}
			break;

		case kRate_abs:
		case kRate_sync:
			effect->setParameterAutomated(tag, control->getValue());
			chunk->setLearner(tag);
			if (chunk->isLearning())
			{
				if (rateFader != NULL)
				{
					if ( (rateFader->getTag() == tag) && 
							(rateFader->getHandle() == gFaderHandle) )
					{
						rateFader->setHandle(gGlowingFaderHandle);
						rateFader->setDirty();
					}
				}
			}
			break;

		case kTempoSync:
	#ifdef MSKIDDER
		case kVelocity:
	#endif
			effect->setParameterAutomated(tag, control->getValue());
			// these are on/off buttons, so use toggle MIDI control
			chunk->setLearner(tag, kEventBehaviourToggle);
			break;

	#ifdef MSKIDDER
		case kMidiMode:
			effect->setParameterAutomated(tag, control->getValue());
			// this is a multi-state switch, so use multi-state toggle MIDI control
			chunk->setLearner(tag, kEventBehaviourToggle, kNumMidiModes);
			break;
	#endif

		case kTempo:
		case kSlope:
		case kPan:
		case kNoise:
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
			}

			break;

		default:
			break;
	}

	control->update(context);
}

//-----------------------------------------------------------------------------
void SkidderEditor::idle()
{
	bool somethingChanged = false;


	if (isOpen)
	{
		// turn off any glowing controls that are no longer learning
		for (long i=0; i < NUM_PARAMETERS; i++)
		{
			if ( (faders[i] != NULL) && chunk->isLearner(i) )
			{
				if (faders[i]->getHandle() == gGlowingFaderHandle)
				{
					faders[i]->setHandle(gFaderHandle);
					faders[i]->setDirty();
					somethingChanged = true;
				}
			}
		}
		//
		if (rateFader)
		{
			if ( !chunk->isLearner(rateFader->getTag()) && 
					(rateFader->getHandle() == gGlowingFaderHandle) )
			{
				rateFader->setHandle(gFaderHandle);
				rateFader->setDirty();
				somethingChanged = true;
			}
			else if ( chunk->isLearner(rateFader->getTag()) && 
					(rateFader->getHandle() == gFaderHandle) )
			{
				rateFader->setHandle(gGlowingFaderHandle);
				rateFader->setDirty();
				somethingChanged = true;
			}
		}
		//
		if (pulsewidthFader)
		{
			if ( ( !chunk->isLearner(kPulsewidthRandMin) && 
					(pulsewidthFader->getHandle() == gGlowingFaderHandleLeft) ) && 
					!(chunk->isLearner(kPulsewidth) && ((Skidder*)effect)->pulsewidthDoubleAutomate) )
			{
				pulsewidthFader->setHandle(gFaderHandleLeft);
				pulsewidthFader->setDirty();
				somethingChanged = true;
			}
			if ( (!chunk->isLearner(kPulsewidth) && 
					(pulsewidthFader->getHandle2() == gGlowingFaderHandleRight)) && 
					!(chunk->isLearner(kPulsewidthRandMin) && ((Skidder*)effect)->pulsewidthDoubleAutomate) )
			{
				pulsewidthFader->setHandle2(gFaderHandleRight);
				pulsewidthFader->setDirty();
				somethingChanged = true;
			}
		}
		//
		if (floorFader)
		{
			if ( (!chunk->isLearner(kFloorRandMin) && 
					(floorFader->getHandle() == gGlowingFaderHandleLeft)) && 
					!(chunk->isLearner(kFloor) && ((Skidder*)effect)->floorDoubleAutomate) )
			{
				floorFader->setHandle(gFaderHandleLeft);
				floorFader->setDirty();
				somethingChanged = true;
			}
			if ( (!chunk->isLearner(kFloor) && 
					(floorFader->getHandle2() == gGlowingFaderHandleRight)) && 
					!(chunk->isLearner(kFloorRandMin) && ((Skidder*)effect)->floorDoubleAutomate) )
			{
				floorFader->setHandle2(gFaderHandleRight);
				floorFader->setDirty();
				somethingChanged = true;
			}
		}

		// indicate that changed controls need to be redrawn
		if (somethingChanged)
			postUpdate();
	}

	// this is called so that idle() actually happens
	AEffGUIEditor::idle();
}
