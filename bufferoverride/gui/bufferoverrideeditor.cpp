#ifndef __BUFFEROVERRIDEEDITOR_H
#include "bufferoverrideeditor.hpp"
#endif

#ifndef __BUFFEROVERRIDE_H
#include "bufferoverride.hpp"
#endif

#include <stdio.h>


#define kRegularTextSize kNormalFontSmall
#define kTinyTextSize kNormalFontVerySmall
const CColor kHintTextCColor = {201, 201, 201, 0};


#if MAC
	#define XLOCK_KEY "option"
#else
	#define XLOCK_KEY "alt"
#endif

//-----------------------------------------------------------------------------
enum {
	// resource IDs
	kBackgroundID = 128,
	kXYboxHandleID,
	kGlowingDivisorHandleID,
	kGlowingBufferHandleID,
	kFaderHandleID,
	kGlowingFaderHandleID,
	kPitchbendFaderHandleID,
	kGlowingPitchbendFaderHandleID,
	kBufferInterruptButtonID,
	kBufferInterruptButtonCornerID,
	kBufferTempoSyncButtonID,
	kBufferTempoSyncButtonCornerID,
	kBufferSizeLabelID,
	kMidiModeButtonID,
	kDivisorLFOshapeSwitchID,
	kDivisorLFOtempoSyncButtonID,
	kDivisorLFOrateLabelID,
	kBufferLFOshapeSwitchID,
	kBufferLFOtempoSyncButtonID,
	kBufferLFOrateLabelID,
	kMidiLearnButtonID,
	kMidiResetButtonID,
	kGoButtonID,

	// . . . positions . . .

	// faders
	kFaderWidth = 122,
	kFaderHeight = 14,
	kLFOfaderHeight = 18,
	//
	kDivisorLFOrateFaderX = 7,
	kDivisorLFOrateFaderY = 175,
	kDivisorLFOdepthFaderX = kDivisorLFOrateFaderX,
	kDivisorLFOdepthFaderY = 199,
	kBufferLFOrateFaderX = 351,
	kBufferLFOrateFaderY = 175,
	kBufferLFOdepthFaderX = kBufferLFOrateFaderX,
	kBufferLFOdepthFaderY = 199,
	//
	kPitchbendFaderX = 6,
	kPitchbendFaderY = 28,
	kSmoothFaderX = 354,
	kSmoothFaderY = 28,
	kDryWetMixFaderX = kSmoothFaderX,
	kDryWetMixFaderY = 65,
	//
	kTempoFaderWidth = 172,
	kTempoFaderX = 121,
	kTempoFaderY = 3,

	// displays
	kDisplayWidth = 36,
	kDisplayHeight = 12,
	//
	kDivisorDisplayX = 127 - kDisplayWidth,
	kDivisorDisplayY = 78,
	kBufferDisplayX = 241 - (kDisplayWidth/2),
	kBufferDisplayY = 195,
	//
	kLFOrateDisplayWidth = 21,
	kDivisorLFOrateDisplayX = 174 - kLFOrateDisplayWidth,//155,
	kDivisorLFOrateDisplayY = 188,
	kDivisorLFOdepthDisplayX = 171 - kDisplayWidth + 2,
	kDivisorLFOdepthDisplayY = 212,
	kBufferLFOrateDisplayX = 326 - kLFOrateDisplayWidth,
	kBufferLFOrateDisplayY = 188,
	kBufferLFOdepthDisplayX = 331 - kDisplayWidth,
	kBufferLFOdepthDisplayY = 212,
	//
	kPitchbendDisplayX = 99 - kDisplayWidth,
	kPitchbendDisplayY = 42,
	kSmoothDisplayX = 413 - kDisplayWidth,
	kSmoothDisplayY = 42,
	kDryWetMixDisplayX = 413 - kDisplayWidth,
	kDryWetMixDisplayY = 79,
	kTempoDisplayX = 312 - 1,
	kTempoDisplayY = 4,
	//
	kHelpDisplayX = 0,
	kHelpDisplayY = 232,

	// XY box
	kDivisorBufferBoxX = 156,
	kDivisorBufferBoxY = 40,
	kDivisorBufferBoxWidth = 169,
	kDivisorBufferBoxHeight = 138,

	// buttons
	kBufferInterruptButtonX = 227,
	kBufferInterruptButtonY = 20,
	kBufferInterruptButtonCornerX = 325,
	kBufferInterruptButtonCornerY = 20,

	kBufferTempoSyncButtonX = 156,
	kBufferTempoSyncButtonY = 20,
	kBufferTempoSyncButtonCornerX = 133,
	kBufferTempoSyncButtonCornerY = 20,
	kBufferSizeLabelX = 222,
	kBufferSizeLabelY = 212,

	kMidiModeButtonX = 5,
	kMidiModeButtonY = 6,

	kDivisorLFOshapeSwitchX = 8,
	kDivisorLFOshapeSwitchY = 145,
	kDivisorLFOtempoSyncButtonX = 52,
	kDivisorLFOtempoSyncButtonY = 119,
	kDivisorLFOrateLabelX = 178,
	kDivisorLFOrateLabelY = 188,

	kBufferLFOshapeSwitchX = 352,
	kBufferLFOshapeSwitchY = 145,
	kBufferLFOtempoSyncButtonX = 351,
	kBufferLFOtempoSyncButtonY = 120,
	kBufferLFOrateLabelX = 277,
	kBufferLFOrateLabelY = 188,

	kMidiLearnButtonX = 5,
	kMidiLearnButtonY = 63,
	kMidiResetButtonX = 4,
	kMidiResetButtonY = 86,

	kGoButtonX = 386,
	kGoButtonY = 6,

	// misc
	kTempoTextEdit = 333,
	kBufferDivisorHelpTag = 3333,
	kNoGoDisplay = 33333
};



// goofy stuff that we have to do because this variable is static, 
// & it's static because bufferDisplayConvert() is static,  & that's static 
// so that it can be a class member & still be acceptable to setStringConvert()
float BufferOverrideEditor::theBufferTempoSync;
float BufferOverrideEditor::theDivisorLFOtempoSync;
float BufferOverrideEditor::theBufferLFOtempoSync;


//-----------------------------------------------------------------------------
// parameter value string display conversion functions

void divisorDisplayConvert(float value, char *string);
void divisorDisplayConvert(float value, char *string)
{
	if (bufferDivisorScaled(value) < 2.0f)
		sprintf(string, "%.2f", 1.0f);
	else
		sprintf(string, "%.2f", bufferDivisorScaled(value));
}

void BufferOverrideEditor::bufferDisplayConvert(float value, char *string, void *temporatestring)
{
	if (onOffTest(theBufferTempoSync))
		strcpy(string, (char*)temporatestring);
	else
		sprintf(string, "%.1f", forcedBufferSizeScaled(value));
}

void BufferOverrideEditor::divisorLFOrateDisplayConvert(float value, char *string, void *temporatestring)
{
	if (onOffTest(theDivisorLFOtempoSync))
		strcpy(string, (char*)temporatestring);
	else
	{
		float rate = LFOrateScaled(value);
		if (rate < 10.0f)
			sprintf(string, "%.2f", rate);
		else
			sprintf(string, "%.1f", rate);
	}
}

void BufferOverrideEditor::bufferLFOrateDisplayConvert(float value, char *string, void *temporatestring)
{
	if (onOffTest(theBufferLFOtempoSync))
		strcpy(string, (char*)temporatestring);
	else
	{
		float rate = LFOrateScaled(value);
		if (rate < 10.0f)
			sprintf(string, "%.2f", rate);
		else
			sprintf(string, "%.1f", rate);
	}
}

void LFOdepthDisplayConvert(float value, char *string);
void LFOdepthDisplayConvert(float value, char *string)
{
	sprintf(string, "%ld %%", (long)(value*100.0f));
}

void smoothDisplayConvert(float value, char *string);
void smoothDisplayConvert(float value, char *string)
{
	sprintf(string, "%.1f %%", (value*100.0f));
}

void dryWetMixDisplayConvert(float value, char *string);
void dryWetMixDisplayConvert(float value, char *string)
{
	sprintf(string, "%ld %%", (long)(value*100.0f));
}

void pitchbendDisplayConvert(float value, char *string);
void pitchbendDisplayConvert(float value, char *string)
{
	sprintf(string, "\xB1%.2f", value*PITCHBEND_MAX);
}

void goDisplayConvert(float value, char *string, void *goErr);
void goDisplayConvert(float value, char *string, void *goErr)
{
	switch (*(long*)goErr)
	{
		case kNoGoDisplay:
			strcpy(string, " ");
			break;
		case 0:
			strcpy(string, "yay!");
			break;
		default:
			strcpy(string, "!ERR0R!");
			break;
	}
}

void helpDisplayConvert(float value, char *string, void *mouseoverparam);
void helpDisplayConvert(float value, char *string, void *mouseoverparam)
{
	switch (*(long*)mouseoverparam)
	{
		case kDivisor:
			sprintf(string, "buffer divisor is the number of times each forced buffer skips & starts over");
			break;
		case kBufferSize:
			sprintf(string, "forced buffer size is the length of the sound chunks that Buffer Override works with");
			break;
		case kBufferDivisorHelpTag:
#ifdef WIN32
		        /* shorten long text for win32 */
		        sprintf(string, "left/right is buffer divisor (number of skips in a buffer, hold ctrl).  up/down is forced buffer size (hold %s)", XLOCK_KEY);
#else
		        sprintf(string, "left/right is buffer divisor (the number of skips in a forced buffer, hold ctrl).   up/down is forced buffer size (hold %s)", XLOCK_KEY);
#endif
			break;
		case kBufferTempoSync:
			strcpy(string, "turn tempo sync on if you want the size of the forced buffers to sync to your tempo");
			break;
		case kBufferInterrupt:
			strcpy(string, "turn this off for the old version 1 style of stubborn \"stuck\" buffers (if you really want that)");
			break;
		case kDivisorLFOrate:
			strcpy(string, "this is the speed of the LFO that modulates the buffer divisor");
			break;
		case kDivisorLFOdepth:
			strcpy(string, "the depth (or intensity) of the LFO that modulates the buffer divisor (0% does nothing)");
			break;
		case kDivisorLFOshape:
			strcpy(string, "choose the waveform shape of the LFO that modulates the buffer divisor");
			break;
		case kDivisorLFOtempoSync:
			strcpy(string, "turn tempo sync on if you want the rate of the buffer divisor LFO to sync to your tempo");
			break;
		case kBufferLFOrate:
			strcpy(string, "this is the speed of the LFO that modulates the forced buffer size");
			break;
		case kBufferLFOdepth:
			strcpy(string, "the depth (or intensity) of the LFO that modulates the forced buffer size (0% does nothing)");
			break;
		case kBufferLFOshape:
			strcpy(string, "choose the waveform shape of the LFO that modulates the forced buffer size");
			break;
		case kBufferLFOtempoSync:
			strcpy(string, "turn tempo sync on if you want the rate of the forced buffer size LFO to sync to your tempo");
			break;
		case kSmooth:
			strcpy(string, "the portion of each minibuffer spent smoothly crossfading the previous one into the new one (prevents glitches)");
			break;
		case kDryWetMix:
			strcpy(string, "the relative mix of the processed sound & the clean/original sound (100% is all processed)");
			break;
		case kPitchbend:
			strcpy(string, "the range, in semitones, of the MIDI pitchbend wheel's effect on the buffer divisor");
			break;
		case kMidiMode:
			strcpy(string, "nudge: MIDI notes adjust the buffer divisor.   trigger: notes also reset the divisor to 1 when they are released");
			break;
		case kTempo:
			strcpy(string, "you can adjust the tempo that Buffer Override uses, or set this to \"auto\" to get the tempo from your sequencer");
			break;
		case kTempoTextEdit:
			strcpy(string, "you can type in the tempo that Buffer Override uses, or set this to \"auto\" to get the tempo from your sequencer");
			break;

		case kMidiLearnButtonID:
			strcpy(string, "activate or deactivate learn mode for assigning MIDI CCs to control Buffer Override's parameters");
			break;
		case kMidiResetButtonID:
			strcpy(string, "press this button to erase all of your CC assignments");
			break;

		default:
			strcpy(string, " ");
	}
}



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BufferOverrideEditor::BufferOverrideEditor(AudioEffect *effect)
 : AEffGUIEditor(effect) 
{
	frame = 0;

	// initialize the graphics pointers
	gBackground = 0;
	gXYboxHandle = 0;
	gGlowingDivisorHandle = 0;
	gGlowingBufferHandle = 0;
	gFaderHandle = 0;
	gGlowingFaderHandle = 0;
	gPitchbendFaderHandle = 0;
	gGlowingPitchbendFaderHandle = 0;
	gBufferInterruptButton = 0;
	gBufferInterruptButtonCorner = 0;
	gBufferTempoSyncButton = 0;
	gBufferTempoSyncButtonCorner = 0;
	gBufferSizeLabel = 0;
	gMidiModeButton = 0;
	gDivisorLFOshapeSwitch = 0;
	gDivisorLFOtempoSyncButton = 0;
	gDivisorLFOrateLabel = 0;
	gBufferLFOshapeSwitch = 0;
	gBufferLFOtempoSyncButton = 0;
	gBufferLFOrateLabel = 0;
	gMidiLearnButton = 0;
	gMidiResetButton = 0;
	gGoButton = 0;

	// initialize the controls pointers
	divisorBufferBox = 0;
	divisorLFOrateFader = 0;
	divisorLFOdepthFader = 0;
	bufferLFOrateFader = 0;
	bufferLFOdepthFader = 0;
	smoothFader = 0;
	dryWetMixFader = 0;
	pitchbendFader = 0;
	tempoFader = 0;
	bufferTempoSyncButton = 0;
	bufferTempoSyncButtonCorner = 0;
	bufferInterruptButton = 0;
	bufferInterruptButtonCorner = 0;
	divisorLFOtempoSyncButton = 0;
	bufferLFOtempoSyncButton = 0;
	midiModeButton = 0;
	midiLearnButton = 0;
	midiResetButton = 0;
	divisorLFOshapeSwitch = 0;
	bufferLFOshapeSwitch = 0;
	bufferSizeLabel = 0;
	divisorLFOrateLabel = 0;
	bufferLFOrateLabel = 0;
	goButton = 0;

	// initialize the value display box pointers
	divisorDisplay = 0;
	bufferDisplay = 0;
	divisorLFOrateDisplay = 0;
	divisorLFOdepthDisplay = 0;
	bufferLFOrateDisplay = 0;
	bufferLFOdepthDisplay = 0;
	smoothDisplay = 0;
	dryWetMixDisplay = 0;
	pitchbendDisplay = 0;
	tempoTextEdit = 0;
	goDisplay = 0;
	helpDisplay = 0;

	// load the background bitmap
	// we don't need to load all bitmaps, this could be done when open is called
	gBackground = new CBitmap (kBackgroundID);

	// init the size of the plugin
	rect.left   = 0;
	rect.top    = 0;
	rect.right  = (short)gBackground->getWidth();
	rect.bottom = (short)gBackground->getHeight();

	tempoString = new char[256];
	bufferTempoRateString = new char[16];
	divisorLFOtempoRateString = new char[16];
	bufferLFOtempoRateString = new char[16];

	faders = 0;
	faders = (CHorizontalSlider**)malloc(sizeof(CHorizontalSlider*)*NUM_PARAMETERS);
	for (long i=0; i < NUM_PARAMETERS; i++)
		faders[i] = NULL;

	chunk = ((DfxPlugin*)effect)->getsettings_ptr();	// this just simplifies pointing
}

//-----------------------------------------------------------------------------
BufferOverrideEditor::~BufferOverrideEditor()
{
	// free background bitmap
	if (gBackground)
		gBackground->forget();
	gBackground = 0;

	free(faders);
	faders = 0;

	if (tempoString)
		delete[] tempoString;
	if (bufferTempoRateString)
		delete[] bufferTempoRateString;
	if (divisorLFOtempoRateString)
		delete[] divisorLFOtempoRateString;
	if (bufferLFOtempoRateString)
		delete[] bufferLFOtempoRateString;
}

//-----------------------------------------------------------------------------
long BufferOverrideEditor::getRect(ERect **erect)
{
	*erect = &rect;
	return true;
}

//-----------------------------------------------------------------------------
long BufferOverrideEditor::open(void *ptr)
{
  CPoint offset;	// the offset of the background graphic behind certain controls
  CPoint point (0, 0);	// for stuff with no offset


	// !!! always call this !!!
	AEffGUIEditor::open(ptr);

	// load some bitmaps
	if (!gXYboxHandle)
		gXYboxHandle = new CBitmap (kXYboxHandleID);
	if (!gGlowingDivisorHandle)
		gGlowingDivisorHandle = new CBitmap (kGlowingDivisorHandleID);
	if (!gGlowingBufferHandle)
		gGlowingBufferHandle = new CBitmap (kGlowingBufferHandleID);

	if (!gFaderHandle)
		gFaderHandle = new CBitmap (kFaderHandleID);
	if (!gGlowingFaderHandle)
		gGlowingFaderHandle = new CBitmap (kGlowingFaderHandleID);
	if (!gPitchbendFaderHandle)
		gPitchbendFaderHandle = new CBitmap (kPitchbendFaderHandleID);
	if (!gGlowingPitchbendFaderHandle)
		gGlowingPitchbendFaderHandle = new CBitmap (kGlowingPitchbendFaderHandleID);

	if (!gBufferInterruptButton)
		gBufferInterruptButton = new CBitmap (kBufferInterruptButtonID);
	if (!gBufferInterruptButtonCorner)
		gBufferInterruptButtonCorner = new CBitmap (kBufferInterruptButtonCornerID);

	if (!gBufferTempoSyncButton)
		gBufferTempoSyncButton = new CBitmap (kBufferTempoSyncButtonID);
	if (!gBufferTempoSyncButtonCorner)
		gBufferTempoSyncButtonCorner = new CBitmap (kBufferTempoSyncButtonCornerID);
	if (!gBufferSizeLabel)
		gBufferSizeLabel = new CBitmap (kBufferSizeLabelID);

	if (!gMidiModeButton)
		gMidiModeButton = new CBitmap (kMidiModeButtonID);

	if (!gDivisorLFOshapeSwitch)
		gDivisorLFOshapeSwitch = new CBitmap(kDivisorLFOshapeSwitchID);
	if (!gDivisorLFOtempoSyncButton)
		gDivisorLFOtempoSyncButton = new CBitmap(kDivisorLFOtempoSyncButtonID);
	if (!gDivisorLFOrateLabel)
		gDivisorLFOrateLabel = new CBitmap(kDivisorLFOrateLabelID);

	if (!gBufferLFOshapeSwitch)
		gBufferLFOshapeSwitch = new CBitmap(kBufferLFOshapeSwitchID);
	if (!gBufferLFOtempoSyncButton)
		gBufferLFOtempoSyncButton = new CBitmap(kBufferLFOtempoSyncButtonID);
	if (!gBufferLFOrateLabel)
		gBufferLFOrateLabel = new CBitmap(kBufferLFOrateLabelID);

	if (!gMidiLearnButton)
		gMidiLearnButton = new CBitmap (kMidiLearnButtonID);
	if (!gMidiResetButton)
		gMidiResetButton = new CBitmap (kMidiResetButtonID);

	if (!gGoButton)
		gGoButton = new CBitmap (kGoButtonID);


	goError = kNoGoDisplay;
	mouseoverParam = -1;	// not over any parameter control yet
	theBufferTempoSync = effect->getParameter(kBufferTempoSync);
	theDivisorLFOtempoSync = effect->getParameter(kDivisorLFOtempoSync);
	theBufferLFOtempoSync = effect->getParameter(kBufferLFOtempoSync);
	chunk->resetLearning();
	XYmouseDown = false;
	mostRecentControl = 0;

	//--initialize the background frame--------------------------------------
	CRect size (0, 0, gBackground->getWidth(), gBackground->getHeight());
	frame = new CFrame (size, ptr, this);
	frame->setBackground(gBackground);


	//--initialize the faders------------------------------------------------
	long handleWidth = gFaderHandle->getWidth();

	// buffer divisor
	int minXPos = kDivisorBufferBoxX;
	int maxXPos = kDivisorBufferBoxX + kDivisorBufferBoxWidth - gXYboxHandle->getWidth();
	int minYPos = kDivisorBufferBoxY;
	int maxYPos = kDivisorBufferBoxY + kDivisorBufferBoxHeight - gXYboxHandle->getHeight();
	size (kDivisorBufferBoxX, kDivisorBufferBoxY, kDivisorBufferBoxX + kDivisorBufferBoxWidth, kDivisorBufferBoxY + kDivisorBufferBoxHeight);
	offset (size.left, size.top);
	divisorBufferBox = new XYbox (size, this, kDivisor, kBufferSize, minXPos, maxXPos, 
						minYPos, maxYPos, gXYboxHandle, gBackground, offset, kLeft, kBottom);
//	divisorBufferBox = new XYbox (size, this, kBufferSize, kDivisor, minXPos, maxXPos, 
//						minYPos, maxYPos, gXYboxHandle, gXYbox, offset, kRight, kBottom);
	divisorBufferBox->setValueTagged(kDivisor, effect->getParameter(kDivisor));
	divisorBufferBox->setValueTagged(kBufferSize, effect->getParameter(kBufferSize));
	divisorBufferBox->setDefaultValueTagged(kDivisor, 0.0f);
	divisorBufferBox->setDefaultValueTagged(kBufferSize, forcedBufferSizeUnscaled(33.3f));
	frame->addView(divisorBufferBox);

	// tempo (in bpm)
	size (kTempoFaderX, kTempoFaderY, kTempoFaderX + kTempoFaderWidth, kTempoFaderY + kFaderHeight);
	offset (size.left, size.top);
	tempoFader = new CHorizontalSlider (size, this, kTempo, size.left, size.right-handleWidth, gFaderHandle, gBackground, offset, kLeft);
	tempoFader->setValue(effect->getParameter(kTempo));
	tempoFader->setDefaultValue(0.0f);
	frame->addView(tempoFader);

	// divisor LFO rate
	size (kDivisorLFOrateFaderX, kDivisorLFOrateFaderY, kDivisorLFOrateFaderX + kFaderWidth, kDivisorLFOrateFaderY + kLFOfaderHeight);
	offset (size.left, size.top);
	divisorLFOrateFader = new CHorizontalSlider (size, this, kDivisorLFOrate, size.left, size.right-handleWidth, gFaderHandle, gBackground, offset, kLeft);
	divisorLFOrateFader->setValue(effect->getParameter(kDivisorLFOrate));
	divisorLFOrateFader->setDefaultValue(LFOrateUnscaled(3.0f));
	frame->addView(divisorLFOrateFader);

	// divisor LFO depth
	size (kDivisorLFOdepthFaderX, kDivisorLFOdepthFaderY, kDivisorLFOdepthFaderX + kFaderWidth, kDivisorLFOdepthFaderY + kLFOfaderHeight);
	offset (size.left, size.top);
	divisorLFOdepthFader = new CHorizontalSlider (size, this, kDivisorLFOdepth, size.left, size.right-handleWidth, gFaderHandle, gBackground, offset, kLeft);
	divisorLFOdepthFader->setValue(effect->getParameter(kDivisorLFOdepth));
	divisorLFOdepthFader->setDefaultValue(0.0f);
	frame->addView(divisorLFOdepthFader);

	// buffer LFO rate
	size (kBufferLFOrateFaderX, kBufferLFOrateFaderY, kBufferLFOrateFaderX + kFaderWidth, kBufferLFOrateFaderY + kLFOfaderHeight);
	offset (size.left, size.top);
	bufferLFOrateFader = new CHorizontalSlider (size, this, kBufferLFOrate, size.left, size.right-handleWidth, gFaderHandle, gBackground, offset, kLeft);
	bufferLFOrateFader->setValue(effect->getParameter(kBufferLFOrate));
	bufferLFOrateFader->setDefaultValue(LFOrateUnscaled(3.0f));
	frame->addView(bufferLFOrateFader);

	// buffer LFO depth
	size (kBufferLFOdepthFaderX, kBufferLFOdepthFaderY, kBufferLFOdepthFaderX + kFaderWidth, kBufferLFOdepthFaderY + kLFOfaderHeight);
	offset (size.left, size.top);
	bufferLFOdepthFader = new CHorizontalSlider (size, this, kBufferLFOdepth, size.left, size.right-handleWidth, gFaderHandle, gBackground, offset, kLeft);
	bufferLFOdepthFader->setValue(effect->getParameter(kBufferLFOdepth));
	bufferLFOdepthFader->setDefaultValue(0.0f);
	frame->addView(bufferLFOdepthFader);

	// smoothing amount
	size (kSmoothFaderX, kSmoothFaderY, kSmoothFaderX + kFaderWidth, kSmoothFaderY + kFaderHeight);
	offset (size.left, size.top);
	smoothFader = new CHorizontalSlider (size, this, kSmooth, size.left, size.right-handleWidth, gFaderHandle, gBackground, offset, kLeft);
	smoothFader->setValue(effect->getParameter(kSmooth));
	smoothFader->setDefaultValue(0.03f);
	frame->addView(smoothFader);

	// dry/wet mix
	size (kDryWetMixFaderX, kDryWetMixFaderY, kDryWetMixFaderX + kFaderWidth, kDryWetMixFaderY + kFaderHeight);
	offset (size.left, size.top);
	dryWetMixFader = new CHorizontalSlider (size, this, kDryWetMix, size.left, size.right-handleWidth, gFaderHandle, gBackground, offset, kLeft);
	dryWetMixFader->setValue(effect->getParameter(kDryWetMix));
	dryWetMixFader->setDefaultValue(0.5f);
	frame->addView(dryWetMixFader);

	// pitchbend range
	size (kPitchbendFaderX, kPitchbendFaderY, kPitchbendFaderX + kFaderWidth, kPitchbendFaderY + kFaderHeight);
	offset (size.left, size.top);
	handleWidth = gPitchbendFaderHandle->getWidth();
	pitchbendFader = new CHorizontalSlider (size, this, kPitchbend, size.left, size.right-handleWidth, gPitchbendFaderHandle, gBackground, offset, kLeft);
	pitchbendFader->setValue(effect->getParameter(kPitchbend));
	pitchbendFader->setDefaultValue(1.0f/12.0f);
	frame->addView(pitchbendFader);


	//--initialize the switchs----------------------------------------------

	// divisor LFO shape switch
	size (kDivisorLFOshapeSwitchX, kDivisorLFOshapeSwitchY, kDivisorLFOshapeSwitchX + gDivisorLFOshapeSwitch->getWidth(), kDivisorLFOshapeSwitchY + (gDivisorLFOshapeSwitch->getHeight())/numLFOshapes);
	divisorLFOshapeSwitch = new CHorizontalSwitch (size, this, kDivisorLFOshape, numLFOshapes, (gDivisorLFOshapeSwitch->getHeight())/numLFOshapes, numLFOshapes, gDivisorLFOshapeSwitch, point);
	divisorLFOshapeSwitch->setValue(effect->getParameter(kDivisorLFOshape));
	frame->addView(divisorLFOshapeSwitch);

	// buffer LFO shape switch
	size (kBufferLFOshapeSwitchX, kBufferLFOshapeSwitchY, kBufferLFOshapeSwitchX + gBufferLFOshapeSwitch->getWidth(), kBufferLFOshapeSwitchY + (gBufferLFOshapeSwitch->getHeight())/numLFOshapes);
	bufferLFOshapeSwitch = new CHorizontalSwitch (size, this, kBufferLFOshape, numLFOshapes, (gBufferLFOshapeSwitch->getHeight())/numLFOshapes, numLFOshapes, gBufferLFOshapeSwitch, point);
	bufferLFOshapeSwitch->setValue(effect->getParameter(kBufferLFOshape));
	frame->addView(bufferLFOshapeSwitch);


	//--initialize the buttons----------------------------------------------

	// choose the forced buffer behaviour (good behaviour or "stuck" v1 behaviour)
	size (kBufferInterruptButtonX, kBufferInterruptButtonY, kBufferInterruptButtonX + (gBufferInterruptButton->getWidth()/2), kBufferInterruptButtonY + (gBufferInterruptButton->getHeight())/2);
	bufferInterruptButton = new MultiKickLink (size, this, kBufferInterrupt, 2, gBufferInterruptButton->getHeight()/2, gBufferInterruptButton, point, kKickPairs);
	bufferInterruptButton->setValue(effect->getParameter(kBufferInterrupt));
	frame->addView(bufferInterruptButton);
	//
	size (kBufferInterruptButtonCornerX, kBufferInterruptButtonCornerY, kBufferInterruptButtonCornerX + (gBufferInterruptButtonCorner->getWidth()/2), kBufferInterruptButtonCornerY + (gBufferInterruptButtonCorner->getHeight())/2);
	bufferInterruptButtonCorner = new MultiKickLink (size, this, kBufferInterrupt, 2, gBufferInterruptButtonCorner->getHeight()/2, gBufferInterruptButtonCorner, point, kKickPairs);
	bufferInterruptButtonCorner->setValue(effect->getParameter(kBufferInterrupt));
	frame->addView(bufferInterruptButtonCorner);
	//
	bufferInterruptButton->setPartner(bufferInterruptButtonCorner);
	bufferInterruptButtonCorner->setPartner(bufferInterruptButton);

	// choose the forced buffer size method ("free" or synced)
	size (kBufferTempoSyncButtonX, kBufferTempoSyncButtonY, kBufferTempoSyncButtonX + (gBufferTempoSyncButton->getWidth()/2), kBufferTempoSyncButtonY + (gBufferTempoSyncButton->getHeight())/2);
	bufferTempoSyncButton = new MultiKickLink (size, this, kBufferTempoSync, 2, gBufferTempoSyncButton->getHeight()/2, gBufferTempoSyncButton, point, kKickPairs);
	bufferTempoSyncButton->setValue(effect->getParameter(kBufferTempoSync));
	frame->addView(bufferTempoSyncButton);
	//
	size (kBufferTempoSyncButtonCornerX, kBufferTempoSyncButtonCornerY, kBufferTempoSyncButtonCornerX + (gBufferTempoSyncButtonCorner->getWidth()/2), kBufferTempoSyncButtonCornerY + (gBufferTempoSyncButtonCorner->getHeight())/2);
	bufferTempoSyncButtonCorner = new MultiKickLink (size, this, kBufferTempoSync, 2, gBufferTempoSyncButtonCorner->getHeight()/2, gBufferTempoSyncButtonCorner, point, kKickPairs);
	bufferTempoSyncButtonCorner->setValue(effect->getParameter(kBufferTempoSync));
	frame->addView(bufferTempoSyncButtonCorner);
	//
	bufferTempoSyncButton->setPartner(bufferTempoSyncButtonCorner);
	bufferTempoSyncButtonCorner->setPartner(bufferTempoSyncButton);
	//
	// display the right parameter label for the forced buffer size display, 
	// according to whether or not buffer tempo sync is on
	size (kBufferSizeLabelX, kBufferSizeLabelY, kBufferSizeLabelX + gBufferSizeLabel->getWidth(), kBufferSizeLabelY + (gBufferSizeLabel->getHeight())/2);
	bufferSizeLabel = new CMovieBitmap (size, this, kBufferTempoSync, 2, gBufferSizeLabel->getHeight()/2, gBufferSizeLabel, point);
	bufferSizeLabel->setValue(effect->getParameter(kBufferTempoSync));
	frame->addView(bufferSizeLabel);

	// choose the divisor LFO rate type ("free" or synced)
	size (kDivisorLFOtempoSyncButtonX, kDivisorLFOtempoSyncButtonY, kDivisorLFOtempoSyncButtonX + (gDivisorLFOtempoSyncButton->getWidth()/2), kDivisorLFOtempoSyncButtonY + (gDivisorLFOtempoSyncButton->getHeight())/2);
	divisorLFOtempoSyncButton = new MultiKick (size, this, kDivisorLFOtempoSync, 2, gDivisorLFOtempoSyncButton->getHeight()/2, gDivisorLFOtempoSyncButton, point, kKickPairs);
	divisorLFOtempoSyncButton->setValue(effect->getParameter(kDivisorLFOtempoSync));
	frame->addView(divisorLFOtempoSyncButton);
	//
	// display the right parameter label for the divisor LFO rate display, 
	// according to whether or not divisor LFO tempo sync is on
	size (kDivisorLFOrateLabelX, kDivisorLFOrateLabelY, kDivisorLFOrateLabelX + gDivisorLFOrateLabel->getWidth(), kDivisorLFOrateLabelY + (gDivisorLFOrateLabel->getHeight())/2);
	divisorLFOrateLabel = new CMovieBitmap (size, this, kDivisorLFOtempoSync, 2, gDivisorLFOrateLabel->getHeight()/2, gDivisorLFOrateLabel, point);
	divisorLFOrateLabel->setValue(effect->getParameter(kDivisorLFOtempoSync));
	frame->addView(divisorLFOrateLabel);

	// choose the forced buffer size LFO rate type ("free" or synced)
	size (kBufferLFOtempoSyncButtonX, kBufferLFOtempoSyncButtonY, kBufferLFOtempoSyncButtonX + (gBufferLFOtempoSyncButton->getWidth()/2), kBufferLFOtempoSyncButtonY + (gBufferLFOtempoSyncButton->getHeight())/2);
	bufferLFOtempoSyncButton = new MultiKick (size, this, kBufferLFOtempoSync, 2, gBufferLFOtempoSyncButton->getHeight()/2, gBufferLFOtempoSyncButton, point, kKickPairs);
	bufferLFOtempoSyncButton->setValue(effect->getParameter(kBufferLFOtempoSync));
	frame->addView(bufferLFOtempoSyncButton);
	//
	// display the right parameter label for the buffer LFO rate display, 
	// according to whether or not buffer LFO tempo sync is on
	size (kBufferLFOrateLabelX, kBufferLFOrateLabelY, kBufferLFOrateLabelX + gBufferLFOrateLabel->getWidth(), kBufferLFOrateLabelY + (gBufferLFOrateLabel->getHeight())/2);
	bufferLFOrateLabel = new CMovieBitmap (size, this, kBufferLFOtempoSync, 2, gBufferLFOrateLabel->getHeight()/2, gBufferLFOrateLabel, point);
	bufferLFOrateLabel->setValue(effect->getParameter(kBufferLFOtempoSync));
	frame->addView(bufferLFOrateLabel);

	// choose the MIDI note control mode ("nudge" or "trigger")
	size (kMidiModeButtonX, kMidiModeButtonY, kMidiModeButtonX + (gMidiModeButton->getWidth()/2), kMidiModeButtonY + (gMidiModeButton->getHeight())/2);
	midiModeButton = new MultiKick (size, this, kMidiMode, 2, gMidiModeButton->getHeight()/2, gMidiModeButton, point, kKickPairs);
	midiModeButton->setValue(effect->getParameter(kMidiMode));
	frame->addView(midiModeButton);

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
	// size stores left, top, right, & bottom positions
	size (kGoButtonX, kGoButtonY, kGoButtonX + gGoButton->getWidth(), kGoButtonY + (gGoButton->getHeight())/2);
	goButton = new CWebLink (size, this, kGoButtonID, "http://www.factoryfarming.org/", gGoButton);
	frame->addView(goButton);


	//--initialize the displays---------------------------------------------

	strcpy( bufferTempoRateString, ((BufferOverride*)effect)->tempoRateTable->getDisplay(effect->getParameter(kBufferSize)) );
	strcpy( divisorLFOtempoRateString, ((BufferOverride*)effect)->tempoRateTable->getDisplay(effect->getParameter(kDivisorLFOrate)) );
	strcpy( bufferLFOtempoRateString, ((BufferOverride*)effect)->tempoRateTable->getDisplay(effect->getParameter(kBufferLFOrate)) );

	// buffer divisor
	size (kDivisorDisplayX, kDivisorDisplayY, kDivisorDisplayX + kDisplayWidth, kDivisorDisplayY + kDisplayHeight);
//	divisorDisplay = new CParamDisplay (size, gBackground);
	divisorDisplay = new CNumberBox(size, this, kDivisor, gBackground);
	offset (size.left, size.top);
	divisorDisplay->setBackOffset(offset);
	divisorDisplay->setHoriAlign(kRightText);
	divisorDisplay->setFont(kRegularTextSize);
	divisorDisplay->setFontColor(kWhiteCColor);
	divisorDisplay->setValue(effect->getParameter(kDivisor));
	divisorDisplay->setStringConvert(divisorDisplayConvert);
//	divisorDisplay->setTag(kDivisor);
	frame->addView(divisorDisplay);

	// forced buffer size
	size (kBufferDisplayX, kBufferDisplayY, kBufferDisplayX + kDisplayWidth, kBufferDisplayY + kDisplayHeight);
//	bufferDisplay = new CParamDisplay (size, gBackground);
	bufferDisplay = new CNumberBox(size, this, kBufferSize, gBackground);
	offset (size.left, size.top);
	bufferDisplay->setBackOffset(offset);
	bufferDisplay->setHoriAlign(kCenterText);
	bufferDisplay->setFont(kRegularTextSize);
	bufferDisplay->setFontColor(kWhiteCColor);
	bufferDisplay->setValue(effect->getParameter(kBufferSize));
	bufferDisplay->setStringConvert(bufferDisplayConvert, bufferTempoRateString);
//	bufferDisplay->setTag(kBufferSize);
	frame->addView(bufferDisplay);

	// tempo (in bpm)   (editable)
	size (kTempoDisplayX, kTempoDisplayY, kTempoDisplayX + kDisplayWidth, kTempoDisplayY + kDisplayHeight);
	tempoTextEdit = new CTextEdit (size, this, kTempoTextEdit, 0, gBackground);
	offset (size.left, size.top);
	tempoTextEdit->setBackOffset(offset);
	tempoTextEdit->setFont(kTinyTextSize);
	tempoTextEdit->setFontColor(kWhiteCColor);
	tempoTextEdit->setHoriAlign(kLeftText);
	frame->addView(tempoTextEdit);
	// this makes it display the current value
	setParameter(kTempo, effect->getParameter(kTempo));

	// divisor LFO rate
	size (kDivisorLFOrateDisplayX, kDivisorLFOrateDisplayY, kDivisorLFOrateDisplayX + kLFOrateDisplayWidth, kDivisorLFOrateDisplayY + kDisplayHeight);
	divisorLFOrateDisplay = new CParamDisplay (size, gBackground);
	offset (size.left, size.top);
	divisorLFOrateDisplay->setBackOffset(offset);
	divisorLFOrateDisplay->setHoriAlign(kRightText);
	divisorLFOrateDisplay->setFont(kTinyTextSize);
	divisorLFOrateDisplay->setFontColor(kWhiteCColor);
	divisorLFOrateDisplay->setValue(effect->getParameter(kDivisorLFOrate));
	divisorLFOrateDisplay->setStringConvert(divisorLFOrateDisplayConvert, divisorLFOtempoRateString);
	divisorLFOrateDisplay->setTag(kDivisorLFOrate);
	frame->addView(divisorLFOrateDisplay);

	// divisor LFO depth
	size (kDivisorLFOdepthDisplayX, kDivisorLFOdepthDisplayY, kDivisorLFOdepthDisplayX + kDisplayWidth, kDivisorLFOdepthDisplayY + kDisplayHeight);
	divisorLFOdepthDisplay = new CParamDisplay (size, gBackground);
	offset (size.left, size.top);
	divisorLFOdepthDisplay->setBackOffset(offset);
	divisorLFOdepthDisplay->setHoriAlign(kRightText);
	divisorLFOdepthDisplay->setFont(kTinyTextSize);
	divisorLFOdepthDisplay->setFontColor(kWhiteCColor);
	divisorLFOdepthDisplay->setValue(effect->getParameter(kDivisorLFOdepth));
	divisorLFOdepthDisplay->setStringConvert(LFOdepthDisplayConvert);
	divisorLFOdepthDisplay->setTag(kDivisorLFOdepth);
	frame->addView(divisorLFOdepthDisplay);

	// buffer LFO rate
	size (kBufferLFOrateDisplayX, kBufferLFOrateDisplayY, kBufferLFOrateDisplayX + kLFOrateDisplayWidth, kBufferLFOrateDisplayY + kDisplayHeight);
	bufferLFOrateDisplay = new CParamDisplay (size, gBackground);
	offset (size.left, size.top);
	bufferLFOrateDisplay->setBackOffset(offset);
	bufferLFOrateDisplay->setHoriAlign(kRightText);
	bufferLFOrateDisplay->setFont(kTinyTextSize);
	bufferLFOrateDisplay->setFontColor(kWhiteCColor);
	bufferLFOrateDisplay->setValue(effect->getParameter(kBufferLFOrate));
	bufferLFOrateDisplay->setStringConvert(bufferLFOrateDisplayConvert, bufferLFOtempoRateString);
	bufferLFOrateDisplay->setTag(kBufferLFOrate);
	frame->addView(bufferLFOrateDisplay);

	// buffer LFO depth
	size (kBufferLFOdepthDisplayX, kBufferLFOdepthDisplayY, kBufferLFOdepthDisplayX + kDisplayWidth, kBufferLFOdepthDisplayY + kDisplayHeight);
	bufferLFOdepthDisplay = new CParamDisplay (size, gBackground);
	offset (size.left, size.top);
	bufferLFOdepthDisplay->setBackOffset(offset);
	bufferLFOdepthDisplay->setHoriAlign(kRightText);
	bufferLFOdepthDisplay->setFont(kTinyTextSize);
	bufferLFOdepthDisplay->setFontColor(kWhiteCColor);
	bufferLFOdepthDisplay->setValue(effect->getParameter(kBufferLFOdepth));
	bufferLFOdepthDisplay->setStringConvert(LFOdepthDisplayConvert);
	bufferLFOdepthDisplay->setTag(kBufferLFOdepth);
	frame->addView(bufferLFOdepthDisplay);

	// smoothing amount
	size (kSmoothDisplayX, kSmoothDisplayY, kSmoothDisplayX + kDisplayWidth, kSmoothDisplayY + kDisplayHeight);
	smoothDisplay = new CParamDisplay (size, gBackground);
	offset (size.left, size.top);
	smoothDisplay->setBackOffset(offset);
	smoothDisplay->setHoriAlign(kRightText);
	smoothDisplay->setFont(kRegularTextSize);
	smoothDisplay->setFontColor(kWhiteCColor);
	smoothDisplay->setValue(effect->getParameter(kSmooth));
	smoothDisplay->setStringConvert(smoothDisplayConvert);
	smoothDisplay->setTag(kSmooth);
	frame->addView(smoothDisplay);

	// dry/wet mix
	size (kDryWetMixDisplayX, kDryWetMixDisplayY, kDryWetMixDisplayX + kDisplayWidth, kDryWetMixDisplayY + kDisplayHeight);
	dryWetMixDisplay = new CParamDisplay (size, gBackground);
	offset (size.left, size.top);
	dryWetMixDisplay->setBackOffset(offset);
	dryWetMixDisplay->setHoriAlign(kRightText);
	dryWetMixDisplay->setFont(kRegularTextSize);
	dryWetMixDisplay->setFontColor(kWhiteCColor);
	dryWetMixDisplay->setValue(effect->getParameter(kDryWetMix));
	dryWetMixDisplay->setStringConvert(dryWetMixDisplayConvert);
	dryWetMixDisplay->setTag(kDryWetMix);
	frame->addView(dryWetMixDisplay);

	// pitchbend range
	size (kPitchbendDisplayX, kPitchbendDisplayY, kPitchbendDisplayX + kDisplayWidth, kPitchbendDisplayY + kDisplayHeight);
	pitchbendDisplay = new CParamDisplay (size, gBackground);
	offset (size.left, size.top);
	pitchbendDisplay->setBackOffset(offset);
	pitchbendDisplay->setHoriAlign(kRightText);
	pitchbendDisplay->setFont(kRegularTextSize);
	pitchbendDisplay->setFontColor(kWhiteCColor);
	pitchbendDisplay->setValue(effect->getParameter(kPitchbend));
	pitchbendDisplay->setStringConvert(pitchbendDisplayConvert);
	pitchbendDisplay->setTag(kPitchbend);
	frame->addView(pitchbendDisplay);

/*	// go! success/error display
	size (kGoButtonX, kGoButtonY + 17, kGoButtonX + gGoButton->getWidth(), kGoButtonY + kDisplayHeight + 17);
	goDisplay = new CParamDisplay (size, gBackground);
	offset (size.left, size.top);
	goDisplay->setBackOffset(offset);
	goDisplay->setHoriAlign(kCenterText);
	goDisplay->setFont(kTinyTextSize);
	goDisplay->setFontColor(kWhiteCColor);
	goDisplay->setValue(0.0f);
	goDisplay->setStringConvert(goDisplayConvert, &goError);
	goDisplay->setTag(kGoButtonID);
	frame->addView(goDisplay);
*/
	// controls help display
	size (kHelpDisplayX, kHelpDisplayY, kHelpDisplayX + gBackground->getWidth(), kHelpDisplayY + kDisplayHeight);
	helpDisplay = new CParamDisplay (size, gBackground);
	offset (size.left, size.top);
	helpDisplay->setBackOffset(offset);
	helpDisplay->setHoriAlign(kCenterText);
	helpDisplay->setFont(kTinyTextSize);
	helpDisplay->setFontColor(kHintTextCColor);
	helpDisplay->setStringConvert(helpDisplayConvert, &mouseoverParam);
	helpDisplay->setTag(-1);
	frame->addView(helpDisplay);


	for (long i=0; i < NUM_PARAMETERS; i++)
		faders[i] = NULL;
	faders[kDivisorLFOrate] = divisorLFOrateFader;
	faders[kDivisorLFOdepth] = divisorLFOdepthFader;
	faders[kBufferLFOrate] = bufferLFOrateFader;
	faders[kBufferLFOdepth] = bufferLFOdepthFader;
	faders[kSmooth] = smoothFader;
	faders[kDryWetMix] = dryWetMixFader;
	faders[kPitchbend] = pitchbendFader;
	faders[kTempo] = tempoFader;


	return true;
}

//-----------------------------------------------------------------------------
void BufferOverrideEditor::close()
{
	if (frame)
		delete frame;
	frame = 0;

	for (long i=0; i < NUM_PARAMETERS; i++)
		faders[i] = NULL;

	chunk->resetLearning();

	// free some bitmaps
	if (gXYboxHandle)
		gXYboxHandle->forget();
	gXYboxHandle = 0;
	if (gGlowingDivisorHandle)
		gGlowingDivisorHandle->forget();
	gGlowingDivisorHandle = 0;
	if (gGlowingBufferHandle)
		gGlowingBufferHandle->forget();
	gGlowingBufferHandle = 0;

	if (gFaderHandle)
		gFaderHandle->forget();
	gFaderHandle = 0;
	if (gGlowingFaderHandle)
		gGlowingFaderHandle->forget();
	gGlowingFaderHandle = 0;
	if (gPitchbendFaderHandle)
		gPitchbendFaderHandle->forget();
	gPitchbendFaderHandle = 0;
	if (gGlowingPitchbendFaderHandle)
		gGlowingPitchbendFaderHandle->forget();
	gGlowingPitchbendFaderHandle = 0;

	if (gBufferInterruptButton)
		gBufferInterruptButton->forget();
	gBufferInterruptButton = 0;
	if (gBufferInterruptButtonCorner)
		gBufferInterruptButtonCorner->forget();
	gBufferInterruptButtonCorner = 0;

	if (gBufferTempoSyncButton)
		gBufferTempoSyncButton->forget();
	gBufferTempoSyncButton = 0;
	if (gBufferTempoSyncButtonCorner)
		gBufferTempoSyncButtonCorner->forget();
	gBufferTempoSyncButtonCorner = 0;
	if (gBufferSizeLabel)
		gBufferSizeLabel->forget();
	gBufferSizeLabel = 0;

	if (gMidiModeButton)
		gMidiModeButton->forget();
	gMidiModeButton = 0;

	if (gDivisorLFOshapeSwitch)
		gDivisorLFOshapeSwitch->forget();
	gDivisorLFOshapeSwitch = 0;
	if (gDivisorLFOtempoSyncButton)
		gDivisorLFOtempoSyncButton->forget();
	gDivisorLFOtempoSyncButton = 0;
	if (gDivisorLFOrateLabel)
		gDivisorLFOrateLabel->forget();
	gDivisorLFOrateLabel = 0;

	if (gBufferLFOshapeSwitch)
		gBufferLFOshapeSwitch->forget();
	gBufferLFOshapeSwitch = 0;
	if (gBufferLFOtempoSyncButton)
		gBufferLFOtempoSyncButton->forget();
	gBufferLFOtempoSyncButton = 0;
	if (gBufferLFOrateLabel)
		gBufferLFOrateLabel->forget();
	gBufferLFOrateLabel = 0;

	if (gMidiLearnButton)
		gMidiLearnButton->forget();
	gMidiLearnButton = 0;
	if (gMidiResetButton)
		gMidiResetButton->forget();
	gMidiResetButton = 0;

	if (gGoButton)
		gGoButton->forget();
	gGoButton = 0;
}

//-----------------------------------------------------------------------------
void BufferOverrideEditor::setParameter(long index, float value)
{
	if (!frame)
		return;

	// called from BufferOverrideEdit
	switch (index)
	{
		case kDivisor:
			if (divisorBufferBox)
				divisorBufferBox->setValueTagged(kDivisor, effect->getParameter(kDivisor));
			if (divisorDisplay)
				divisorDisplay->setValue(effect->getParameter(kDivisor));
			break;

		case kBufferSize:
			theBufferTempoSync = effect->getParameter(kBufferTempoSync);
			strcpy( bufferTempoRateString, ((BufferOverride*)effect)->tempoRateTable->getDisplay(effect->getParameter(kBufferSize)) );
			if (divisorBufferBox)
				divisorBufferBox->setValueTagged(kBufferSize, effect->getParameter(kBufferSize));
			if (bufferDisplay)
				bufferDisplay->setValue(effect->getParameter(kBufferSize));
			break;

		case kBufferTempoSync:
			theBufferTempoSync = effect->getParameter(kBufferTempoSync);
			strcpy( bufferTempoRateString, ((BufferOverride*)effect)->tempoRateTable->getDisplay(effect->getParameter(kBufferSize)) );
			if (bufferTempoSyncButton)
				bufferTempoSyncButton->setValue(value);
			if (bufferTempoSyncButtonCorner)
				bufferTempoSyncButtonCorner->setValue(value);
			if (bufferSizeLabel)
				bufferSizeLabel->setValue(value);
			// update the forced buffer size display if the tempo sync mode is changed
			if (bufferDisplay)
				bufferDisplay->setDirty();
			break;

		case kBufferInterrupt:
			if (bufferInterruptButton)
				bufferInterruptButton->setValue(value);
			if (bufferInterruptButtonCorner)
				bufferInterruptButtonCorner->setValue(value);
			break;

		case kDivisorLFOrate:
			theDivisorLFOtempoSync = effect->getParameter(kDivisorLFOtempoSync);
			strcpy( divisorLFOtempoRateString, ((BufferOverride*)effect)->tempoRateTable->getDisplay(effect->getParameter(kDivisorLFOrate)) );
			if (divisorLFOrateFader)
				divisorLFOrateFader->setValue(value);
			if (divisorLFOrateDisplay)
				divisorLFOrateDisplay->setValue(value);
			break;

		case kDivisorLFOdepth:
			if (divisorLFOdepthFader)
				divisorLFOdepthFader->setValue(value);
			if (divisorLFOdepthDisplay)
				divisorLFOdepthDisplay->setValue(value);
			break;

		case kDivisorLFOshape:
			if (divisorLFOshapeSwitch)
				divisorLFOshapeSwitch->setValue(value);
			break;

		case kDivisorLFOtempoSync:
			theDivisorLFOtempoSync = effect->getParameter(kDivisorLFOtempoSync);
			strcpy( divisorLFOtempoRateString, ((BufferOverride*)effect)->tempoRateTable->getDisplay(effect->getParameter(kDivisorLFOrate)) );
			if (divisorLFOtempoSyncButton)
				divisorLFOtempoSyncButton->setValue(value);
			if (divisorLFOrateLabel)
				divisorLFOrateLabel->setValue(value);
			// update the divisor LFO rate display if the tempo sync mode is changed
			if (divisorLFOrateDisplay)
				divisorLFOrateDisplay->setDirty();
			break;

		case kBufferLFOrate:
			theBufferLFOtempoSync = effect->getParameter(kBufferLFOtempoSync);
			strcpy( bufferLFOtempoRateString, ((BufferOverride*)effect)->tempoRateTable->getDisplay(effect->getParameter(kBufferLFOrate)) );
			if (bufferLFOrateFader)
				bufferLFOrateFader->setValue(value);
			if (bufferLFOrateDisplay)
				bufferLFOrateDisplay->setValue(value);
			break;

		case kBufferLFOdepth:
			if (bufferLFOdepthFader)
				bufferLFOdepthFader->setValue(value);
			if (bufferLFOdepthDisplay)
				bufferLFOdepthDisplay->setValue(value);
			break;

		case kBufferLFOshape:
			if (bufferLFOshapeSwitch)
				bufferLFOshapeSwitch->setValue(value);
			break;

		case kBufferLFOtempoSync:
			theBufferLFOtempoSync = effect->getParameter(kBufferLFOtempoSync);
			strcpy( bufferLFOtempoRateString, ((BufferOverride*)effect)->tempoRateTable->getDisplay(effect->getParameter(kBufferLFOrate)) );
			if (bufferLFOtempoSyncButton)
				bufferLFOtempoSyncButton->setValue(value);
			if (bufferLFOrateLabel)
				bufferLFOrateLabel->setValue(value);
			// update the forced buffer LFO rate display if the tempo sync mode is changed
			if (bufferLFOrateDisplay)
				bufferLFOrateDisplay->setDirty();
			break;

		case kSmooth:
			if (smoothFader)
				smoothFader->setValue(value);
			if (smoothDisplay)
				smoothDisplay->setValue(value);
			break;

		case kDryWetMix:
			if (dryWetMixFader)
				dryWetMixFader->setValue(value);
			if (dryWetMixDisplay)
				dryWetMixDisplay->setValue(value);
			break;

		case kMidiMode:
			if (midiModeButton)
				midiModeButton->setValue(value);
			break;

		case kPitchbend:
			if (pitchbendFader)
				pitchbendFader->setValue(value);
			if (pitchbendDisplay)
				pitchbendDisplay->setValue(value);
			break;

		case kTempo:
			if (tempoFader)
				tempoFader->setValue(value);
			if (tempoTextEdit)
			{
				if ( (value > 0.0f) || (((BufferOverride*)effect)->hostCanDoTempo < 1) )
					sprintf(tempoString, "%.2f", tempoScaled(value));
				else
					strcpy(tempoString, "auto");
				tempoTextEdit->setText(tempoString);
			}
			break;

		default:
			return;
	}	

	postUpdate();
}

//-----------------------------------------------------------------------------
void BufferOverrideEditor::valueChanged(CDrawContext* context, CControl* control)
{
  long tag = control->getTag();
  float tempTempo;
  long sscanfReturn;


	mostRecentControl = control;

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
						setParameter(kTempo, tempTempo);
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
			break;

		case kMidiLearnButtonID:
			chunk->setParameterMidiLearn(control->getValue());
			break;
		case kMidiResetButtonID:
			chunk->setParameterMidiReset(control->getValue());
			break;


		case kDivisor:
		case kBufferSize:
			if (control == divisorBufferBox)
			{
				effect->setParameterAutomated(kDivisor, divisorBufferBox->getValueTagged(kDivisor));
				effect->setParameterAutomated(kBufferSize, divisorBufferBox->getValueTagged(kBufferSize));
			}
			else
			{
				effect->setParameterAutomated(tag, control->getValue());
				chunk->setLearner(tag);
				if (chunk->isLearning())
				{
					if (divisorBufferBox != NULL)
					{
						if ( (tag == kDivisor) && 
							(divisorBufferBox->getHandle() != gGlowingDivisorHandle) )
						{
							divisorBufferBox->setHandle(gGlowingDivisorHandle);
							divisorBufferBox->setDirty();
						}
						else if ( (tag == kBufferSize) && 
							(divisorBufferBox->getHandle() != gGlowingBufferHandle) )
						{
							divisorBufferBox->setHandle(gGlowingBufferHandle);
							divisorBufferBox->setDirty();
						}

					}
				}
			}
			break;

		case kBufferTempoSync:
		case kBufferInterrupt:
		case kDivisorLFOtempoSync:
		case kBufferLFOtempoSync:
		case kMidiMode:
			effect->setParameterAutomated(tag, control->getValue());
			// these are on/off buttons, so use toggle MIDI control
			chunk->setLearner(tag, kEventBehaviourToggle);
			break;

		case kDivisorLFOshape:
		case kBufferLFOshape:
			effect->setParameterAutomated(tag, control->getValue());
			// these are multi-state switches, so use multi-state toggle MIDI control
			chunk->setLearner(tag, kEventBehaviourToggle, numLFOshapes);
			break;

		case kDivisorLFOrate:
		case kDivisorLFOdepth:
		case kBufferLFOrate:
		case kBufferLFOdepth:
		case kSmooth:
		case kDryWetMix:
		case kPitchbend:
		case kTempo:
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
					else if (faders[tag]->getHandle() == gPitchbendFaderHandle)
					{
						faders[tag]->setHandle(gGlowingPitchbendFaderHandle);
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
// the idle routine in Buffer Override is for updating the divisor display
// whenever MIDI notes or pitchbend messages have changed the divisor's value

void BufferOverrideEditor::idle()
{
  bool somethingChanged = false;


	if ( (frame != NULL) && frame->isOpen() )
	{
		// update the divisor control & display if it has changed via MIDI notes or pitchbend
		if ( ((BufferOverride*)effect)->divisorWasChangedByMIDI )
		{
			if (divisorBufferBox)
				divisorBufferBox->setValueTagged(kDivisor, effect->getParameter(kDivisor));
			if (divisorDisplay)
				divisorDisplay->setValue(effect->getParameter(kDivisor));
			((BufferOverride*)effect)->divisorWasChangedByMIDI = false;	// reset it
			somethingChanged = true;
		}


		if ( (helpDisplay != NULL) && (frame != NULL) )
		{
			// first look to see if any of the main control menus are moused over
			CControl *control = (CControl*) (frame->getCurrentView());
			long oldParameter = mouseoverParam;
			mouseoverParam = -1;
			if ( ((control != NULL) && (control != bufferSizeLabel)) && 
					((control != divisorLFOrateLabel) && (control != bufferLFOrateLabel)) )
				mouseoverParam = control->getTag();
			if ( (control != NULL) && (control == divisorBufferBox) )
				mouseoverParam = kBufferDivisorHelpTag;
			if (mouseoverParam != oldParameter)
			{
				helpDisplay->setDirty();
				somethingChanged = true;
			}
		}


		// handle the bi-state glowing of XY buffer-divisor box
		if (divisorBufferBox)
		{
			if (divisorBufferBox->mouseIsDown())
			{
				if (!XYmouseDown)	// we just had a click...
				{
					if (chunk->isLearning())	// & we're learning, so learn an XY parameter
					{
						if (chunk->getLearner() == kDivisor)
						{
							chunk->setLearner(kBufferSize);
							if (divisorBufferBox->getHandle() != gGlowingBufferHandle)
							{
								divisorBufferBox->setHandle(gGlowingBufferHandle);
								divisorBufferBox->setDirty();
								somethingChanged = true;
							}
						}
						else
						{
							chunk->setLearner(kDivisor);
							if (divisorBufferBox->getHandle() != gGlowingDivisorHandle)
							{
								divisorBufferBox->setHandle(gGlowingDivisorHandle);
								divisorBufferBox->setDirty();
								somethingChanged = true;
							}
						}
					}
				}
				XYmouseDown = true;
			}
			else
				XYmouseDown = false;
		}

		// turn off any glowing controls that are no longer learning
		for (long i=0; i < NUM_PARAMETERS; i++)
		{
			if ( (faders[i] != NULL) && (i != chunk->getLearner()) )
			{
				if (faders[i]->getHandle() == gGlowingFaderHandle)
				{
					faders[i]->setHandle(gFaderHandle);
					faders[i]->setDirty();
					somethingChanged = true;
				}
				else if (faders[i]->getHandle() == gGlowingPitchbendFaderHandle)
				{
					faders[i]->setHandle(gPitchbendFaderHandle);
					faders[i]->setDirty();
					somethingChanged = true;
				}
			}			
		}
		if ( (chunk->getLearner() != kDivisor) && (chunk->getLearner() != kBufferSize) )
		{
			if (divisorBufferBox)
			{
				if (divisorBufferBox->getHandle() != gXYboxHandle)
				{
					divisorBufferBox->setHandle(gXYboxHandle);
					divisorBufferBox->setDirty();
					somethingChanged = true;
				}
			}
		}


		// indicate that changed controls need to be redrawn
		if (somethingChanged)
			postUpdate();
	}

	// this is called so that idle() actually happens
	AEffGUIEditor::idle();
}
