/*--------- by Tom Murphy 7 & Marc Poirier  ][  September 2001 ---------*/

#ifndef __transverbeditor
#include "transverbeditor.hpp"
#endif

#ifndef __TRANSVERB_H
#include "transverb.hpp"
#endif

#include <stdio.h>



//-----------------------------------------------------------------------------
enum {
	// resource IDs
	kBackgroundID = 128,
	kPurpleWideFaderSlideID,
	kGreyWideFaderSlideID,
	kPurpleTallFaderSlideID,
	kPurpleWideFaderHandleID,
	kGlowingPurpleWideFaderHandleID,
	kGreyWideFaderHandleID,
	kGlowingGreyWideFaderHandleID,
	kTallFaderHandleID,
	kGlowingTallFaderHandleID,
	kFineDownButtonID,
	kFineUpButtonID,
	kSpeedModeButtonID,
	kQualityButtonID,
	kOnOffButtonID,
	kRandomButtonID,
	kMidiLearnButtonID,
	kMidiResetButtonID,
	kDFXlinkID,
	kSuperDFXlinkID,
	kSmartElectronixLinkID,

	// positions
	kWideFaderX = 20,
	kWideFaderY = 35,
	kWideFaderInc = 24,
	kWideFaderMoreInc = 42,
	kWideFaderEvenMoreInc = 50,
	kTallFaderX = kWideFaderX,
	kTallFaderY = 265,
	kTallFaderInc = 28,

	kButtonX = kWideFaderX,
	kRandomButtonX = 185,
	kTomsoundButtonX = 425,
	kQualityButtonY = 236,

	kFineDownButtonX = 503,
	kFineUpButtonX = 512,
	kFineButtonY = kWideFaderY,
	kSpeedModeButtonX = 503,
	kSpeedModeButtonY = 22,

	kMidiLearnButtonX = 237,
	kMidiLearnButtonY = 254,
	kMidiResetButtonX = 288,
	kMidiResetButtonY = 254,

	kDFXlinkX = 107,
	kDFXlinkY = 281,
	kSuperDFXlinkX = 159,
	kSuperDFXlinkY = 339,
	kSmartElectronixLinkX = 306,
	kSmartElectronixLinkY = kSuperDFXlinkY,

	kDisplayX = 318,
	kDisplayY = 23,
	kDisplayWidth = 180,
	kDisplayHeight = 11,

	kFineDownOffset = 3333,
	kFineUpOffset = 9999,
	kSpeed1FineDown = kSpeed1 + kFineDownOffset,
	kSpeed1FineUp = kSpeed1 + kFineUpOffset,
	kSpeed2FineDown = kSpeed2 + kFineDownOffset,
	kSpeed2FineUp = kSpeed2 + kFineUpOffset,

	kRandomButton = 333,

	kSpeedTextEditOffset = 300,
	kSpeed1TextEdit = kSpeed1 + kSpeedTextEditOffset,
	kSpeed2TextEdit = kSpeed2 + kSpeedTextEditOffset
};



//-----------------------------------------------------------------------------
// parameter value string display conversion functions

void bsizeDisplayConvert(float value, char *string);
void bsizeDisplayConvert(float value, char *string)
{
  float buffersize, remainder;
  long thousands;

	buffersize = bufferMsScaled(value);
	thousands = (long)buffersize / 1000;
	remainder = fmodf(buffersize, 1000.0f);

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
		sprintf(string, "%.1f  ms", buffersize);
}

void speedDisplayConvert(float value, char *string);
void speedDisplayConvert(float value, char *string)
{
	char *semitonesString = (char*) malloc(16);
	float speed = speedScaled(value);
	float remainder = fmodf(fabsf(speed), 1.0f);
	float semitones = remainder * 12.0f;
	// make sure that these float crap doesn't result in wacky stuff 
	// like displays that say "-1 octave & 12.00 semitones"
	sprintf(semitonesString, "%.3f", semitones);
	if ( (strcmp(semitonesString, "12.000") == 0) || (strcmp(semitonesString, "-12.000") == 0) )
	{
		semitones = 0.0f;
		if (speed < 0.0f)
			speed -= 0.003f;
		else
			speed += 0.003f;
	}
	int octaves = (int) speed;

	if ( speed > 0.0f )
	{
		if (octaves == 0)
			sprintf(string, "+%.2f  semitones", semitones);
		else if (octaves == 1)
			sprintf(string, "+%d octave  &  %.2f  semitones", octaves, semitones);
		else
			sprintf(string, "+%d octaves  &  %.2f  semitones", octaves, semitones);
	}
	else if (octaves == 0)
		sprintf(string, "-%.2f  semitones", semitones);
	else
	{
		if (octaves == -1)
			sprintf(string, "%d octave  &  %.2f  semitones", octaves, semitones);
		else
			sprintf(string, "%d octaves &  %.2f  semitones", octaves, semitones);
	}

	if (semitonesString != NULL)
		free(semitonesString);
}

void feedDisplayConvert(float value, char *string);
void feedDisplayConvert(float value, char *string)
{
	sprintf(string, "%ld %%", (long)(value*100.0f));
}

void distDisplayConvert(float value, char *string, void *fbsize);
void distDisplayConvert(float value, char *string, void *fbsize)
{
  float distance, remainder;
  long thousands;

	distance = value * bufferMsScaled(*(float*)fbsize);
	thousands = (long)distance / 1000;
	remainder = fmodf(distance, 1000.0f);

	if (thousands > 0)
	{
		if (remainder < 10.0f)
			sprintf(string, "%ld,00%.2f  ms", thousands, remainder);
		else if (remainder < 100.0f)
			sprintf(string, "%ld,0%.2f  ms", thousands, remainder);
		else
			sprintf(string, "%ld,%.2f  ms", thousands, remainder);
	}
	else
		sprintf(string, "%.2f  ms", distance);
}



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
TransverbEditor::TransverbEditor(AudioEffect *effect)
 : AEffGUIEditor(effect) 
{
	frame = 0;

	// initialize the graphics pointers
	gBackground = 0;
	gPurpleWideFaderSlide = 0;
	gGreyWideFaderSlide = 0;
	gPurpleTallFaderSlide = 0;
	gPurpleWideFaderHandle = 0;
	gGreyWideFaderHandle = 0;
	gTallFaderHandle = 0;
	gGlowingPurpleWideFaderHandle = 0;
	gGlowingGreyWideFaderHandle = 0;
	gGlowingTallFaderHandle = 0;
	gFineDownButton = 0;
	gFineUpButton = 0;
	gSpeedModeButton = 0;
	gQualityButton = 0;
	gOnOffButton = 0;
	gRandomButton = 0;
	gMidiLearnButton = 0;
	gMidiResetButton = 0;
	gDFXlink = 0;
	gSuperDFXlink = 0;
	gSmartElectronixLink = 0;

	// initialize the controls pointers
	speed1Fader = 0;
	feed1Fader = 0;
	dist1Fader = 0;
	speed2Fader = 0;
	feed2Fader = 0;
	dist2Fader = 0;
	bsizeFader = 0;
	//
	qualityButton = 0;
	tomsoundButton = 0;
	randomButton = 0;
	//
	drymixFader = 0;
	mix1Fader = 0;
	mix2Fader = 0;
	//
	speed1FineDownButton = 0;
	speed1FineUpButton = 0;
	feed1FineDownButton = 0;
	feed1FineUpButton = 0;
	dist1FineDownButton = 0;
	dist1FineUpButton = 0;
	speed2FineDownButton = 0;
	speed2FineUpButton = 0;
	feed2FineDownButton = 0;
	feed2FineUpButton = 0;
	dist2FineDownButton = 0;
	dist2FineUpButton = 0;
	bsizeFineDownButton = 0;
	bsizeFineUpButton = 0;
	//
	speed1modeButton = 0;
	speed2modeButton = 0;
	//
	midiLearnButton = 0;
	midiResetButton = 0;
	//
	DFXlink = 0;
	SuperDFXlink = 0;
	SmartElectronixLink = 0;

	// initialize the value display box pointers
	speed1TextEdit = 0;
	feed1Display = 0;
	dist1Display = 0;
	speed2TextEdit = 0;
	feed2Display = 0;
	dist2Display = 0;
	bsizeDisplay = 0;

	// load the background bitmap
	// we don't need to load all bitmaps, this could be done when open is called
	gBackground = new CBitmap (kBackgroundID);

	// init the size of the plugin
	rect.left   = 0;
	rect.top    = 0;
	rect.right  = (short)gBackground->getWidth();
	rect.bottom = (short)gBackground->getHeight();

	speedString = new char[256];

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
TransverbEditor::~TransverbEditor()
{
	// free background bitmap
	if (gBackground)
		gBackground->forget();
	gBackground = 0;

	free(faders);
	faders = 0;
	free(tallFaders);
	tallFaders = 0;

	if (speedString)
		delete[] speedString;
}

//-----------------------------------------------------------------------------
long TransverbEditor::getRect(ERect **erect)
{
	*erect = &rect;
	return true;
}

//-----------------------------------------------------------------------------
long TransverbEditor::open(void *ptr)
{
  CPoint displayOffset;	// for positioning the background graphic behind display boxes


	// !!! always call this !!!
	AEffGUIEditor::open(ptr);

	// load some bitmaps
	if (!gPurpleWideFaderSlide)
		gPurpleWideFaderSlide = new CBitmap (kPurpleWideFaderSlideID);
	if (!gGreyWideFaderSlide)
		gGreyWideFaderSlide = new CBitmap (kGreyWideFaderSlideID);
	if (!gPurpleTallFaderSlide)
		gPurpleTallFaderSlide = new CBitmap (kPurpleTallFaderSlideID);

	if (!gPurpleWideFaderHandle)
		gPurpleWideFaderHandle = new CBitmap (kPurpleWideFaderHandleID);
	if (!gGreyWideFaderHandle)
		gGreyWideFaderHandle = new CBitmap (kGreyWideFaderHandleID);
	if (!gTallFaderHandle)
		gTallFaderHandle = new CBitmap (kTallFaderHandleID);

	if (!gGlowingPurpleWideFaderHandle)
		gGlowingPurpleWideFaderHandle = new CBitmap (kGlowingPurpleWideFaderHandleID);
	if (!gGlowingGreyWideFaderHandle)
		gGlowingGreyWideFaderHandle = new CBitmap (kGlowingGreyWideFaderHandleID);
	if (!gGlowingTallFaderHandle)
		gGlowingTallFaderHandle = new CBitmap (kGlowingTallFaderHandleID);

	if (!gFineDownButton)
		gFineDownButton = new CBitmap (kFineDownButtonID);
	if (!gFineUpButton)
		gFineUpButton = new CBitmap (kFineUpButtonID);
	if (!gSpeedModeButton)
		gSpeedModeButton = new CBitmap (kSpeedModeButtonID);

	if (!gQualityButton)
		gQualityButton = new CBitmap (kQualityButtonID);
	if (!gOnOffButton)
		gOnOffButton = new CBitmap (kOnOffButtonID);
	if (!gRandomButton)
		gRandomButton = new CBitmap (kRandomButtonID);

	if (!gMidiLearnButton)
		gMidiLearnButton = new CBitmap (kMidiLearnButtonID);
	if (!gMidiResetButton)
		gMidiResetButton = new CBitmap (kMidiResetButtonID);

	if (!gDFXlink)
		gDFXlink = new CBitmap (kDFXlinkID);
	if (!gSuperDFXlink)
		gSuperDFXlink = new CBitmap (kSuperDFXlinkID);
	if (!gSmartElectronixLink)
		gSmartElectronixLink = new CBitmap (kSmartElectronixLinkID);


	chunk->resetLearning();
	fBsize = ((DfxPlugin*)effect)->getparameter_gen(kBsize);

	//--initialize the background frame--------------------------------------
	CRect size (0, 0, gBackground->getWidth(), gBackground->getHeight());
	frame = new CFrame (size, ptr, this);
	frame->setBackground(gBackground);


	//--initialize the faders-----------------------------------------------
	int minWidePos = kWideFaderX + 1;
	int maxWidePos = kWideFaderX + gPurpleWideFaderSlide->getWidth() - gPurpleWideFaderHandle->getWidth();
	CPoint point (0, 0);
	CPoint offset (0, 1);

	// head 1 playback speed
	// size stores left, top, right, & bottom positions
	size (kWideFaderX, kWideFaderY, kWideFaderX + gPurpleWideFaderSlide->getWidth(), kWideFaderY + gPurpleWideFaderSlide->getHeight());
	speed1Fader = new CHorizontalSlider (size, this, kSpeed1, minWidePos, maxWidePos, gPurpleWideFaderHandle, gPurpleWideFaderSlide, point, kLeft);
	speed1Fader->setOffsetHandle(offset);
	speed1Fader->setValue(effect->getParameter(kSpeed1));
	speed1Fader->setDefaultValue((0.0f-SPEED_MIN)/(SPEED_MAX-SPEED_MIN));
	frame->addView(speed1Fader);

	// head 1 feedback
	size.offset (0, kWideFaderInc);
	feed1Fader = new CHorizontalSlider (size, this, kFeed1, minWidePos, maxWidePos, gPurpleWideFaderHandle, gPurpleWideFaderSlide, point, kLeft);
	feed1Fader->setOffsetHandle(offset);
	feed1Fader->setValue(effect->getParameter(kFeed1));
	feed1Fader->setDefaultValue(0.333f);
	frame->addView(feed1Fader);

	// head 1 starting distance
	size.offset (0, kWideFaderInc);
	dist1Fader = new CHorizontalSlider (size, this, kDist1, minWidePos, maxWidePos, gPurpleWideFaderHandle, gPurpleWideFaderSlide, point, kLeft);
	dist1Fader->setOffsetHandle(offset);
	dist1Fader->setValue(effect->getParameter(kDist1));
	dist1Fader->setDefaultValue(0.5f);
	frame->addView(dist1Fader);

	// head 2 playback speed
	size.offset (0, kWideFaderMoreInc);
	speed2Fader = new CHorizontalSlider (size, this, kSpeed2, minWidePos, maxWidePos, gPurpleWideFaderHandle, gPurpleWideFaderSlide, point, kLeft);
	speed2Fader->setOffsetHandle(offset);
	speed2Fader->setValue(effect->getParameter(kSpeed2));
	speed2Fader->setDefaultValue((0.0f-SPEED_MIN)/(SPEED_MAX-SPEED_MIN));
	frame->addView(speed2Fader);

	// head 2 feedback
	size.offset (0, kWideFaderInc);
	feed2Fader = new CHorizontalSlider (size, this, kFeed2, minWidePos, maxWidePos, gPurpleWideFaderHandle, gPurpleWideFaderSlide, point, kLeft);
	feed2Fader->setOffsetHandle(offset);
	feed2Fader->setValue(effect->getParameter(kFeed2));
	feed2Fader->setDefaultValue(0.333f);
	frame->addView(feed2Fader);

	// head 2 starting distance
	size.offset (0, kWideFaderInc);
	dist2Fader = new CHorizontalSlider (size, this, kDist2, minWidePos, maxWidePos, gPurpleWideFaderHandle, gPurpleWideFaderSlide, point, kLeft);
	dist2Fader->setOffsetHandle(offset);
	dist2Fader->setValue(effect->getParameter(kDist2));
	dist2Fader->setDefaultValue(0.5f);
	frame->addView(dist2Fader);

	minWidePos = kWideFaderX + 1;
	maxWidePos = kWideFaderX + gGreyWideFaderSlide->getWidth() - gGreyWideFaderHandle->getWidth();
	// buffer size
	size.offset (0, kWideFaderEvenMoreInc);
	bsizeFader = new CHorizontalSlider (size, this, kBsize, minWidePos, maxWidePos, gGreyWideFaderHandle, gGreyWideFaderSlide, point, kLeft);
	bsizeFader->setOffsetHandle(offset);
	bsizeFader->setValue(effect->getParameter(kBsize));
	bsizeFader->setDefaultValue((333.0f-BUFFER_MIN)/(BUFFER_MAX-BUFFER_MIN));
	frame->addView(bsizeFader);

	//--initialize the vertical gain faders---------------------------------
	int minTallPos = kTallFaderY + 1;
	int maxTallPos = kTallFaderY + gPurpleTallFaderSlide->getHeight() - gTallFaderHandle->getHeight();
	offset (1, 0);

	// dry gain
	size (kTallFaderX, kTallFaderY, kTallFaderX + gPurpleTallFaderSlide->getWidth(), kTallFaderY + gPurpleTallFaderSlide->getHeight());
	drymixFader = new CVerticalSlider (size, this, kDrymix, minTallPos, maxTallPos, gTallFaderHandle, gPurpleTallFaderSlide, point, kBottom);
	drymixFader->setOffsetHandle(offset);
	drymixFader->setValue(effect->getParameter(kDrymix));
	drymixFader->setDefaultValue(1.0f);
	frame->addView(drymixFader);

	// head 1 gain
	size.offset (kTallFaderInc, 0);
	mix1Fader = new CVerticalSlider (size, this, kMix1, minTallPos, maxTallPos, gTallFaderHandle, gPurpleTallFaderSlide, point, kBottom);
	mix1Fader->setOffsetHandle(offset);
	mix1Fader->setValue(effect->getParameter(kMix1));
	mix1Fader->setDefaultValue(1.0f);
	frame->addView(mix1Fader);

	// head 2 gain
	size.offset (kTallFaderInc, 0);
	mix2Fader = new CVerticalSlider (size, this, kMix2, minTallPos, maxTallPos, gTallFaderHandle, gPurpleTallFaderSlide, point, kBottom);
	mix2Fader->setOffsetHandle(offset);
	mix2Fader->setValue(effect->getParameter(kMix2));
	mix2Fader->setDefaultValue(1.0f);
	frame->addView(mix2Fader);

	//--initialize the buttons----------------------------------------------

	// quality button
	size (kButtonX, kQualityButtonY, kButtonX + gQualityButton->getWidth(), kQualityButtonY + (gQualityButton->getHeight())/(numQualities*2));
	qualityButton = new MultiKick (size, this, kQuality, numQualities, gQualityButton->getHeight()/(numQualities*2), gQualityButton, point);
	qualityButton->setValue(effect->getParameter(kQuality));
	frame->addView(qualityButton);

	// random button
	size (kRandomButtonX, kQualityButtonY, kRandomButtonX + gRandomButton->getWidth(), kQualityButtonY + (gRandomButton->getHeight())/2);
	randomButton = new CKickButton (size, this, kRandomButton, (gRandomButton->getHeight())/2, gRandomButton, point);
	randomButton->setValue(0.0f);
	frame->addView(randomButton);

	// TOMSOUND button
	size (kTomsoundButtonX, kQualityButtonY, kTomsoundButtonX + gOnOffButton->getWidth(), kQualityButtonY + (gOnOffButton->getHeight())/4);
	tomsoundButton = new MultiKick (size, this, kTomsound, 2, gOnOffButton->getHeight()/4, gOnOffButton, point);
	tomsoundButton->setValue(effect->getParameter(kTomsound));
	frame->addView(tomsoundButton);

	// head 1 speed mode button
	size (kSpeedModeButtonX, kSpeedModeButtonY, kSpeedModeButtonX + gSpeedModeButton->getWidth(), kSpeedModeButtonY + (gSpeedModeButton->getHeight())/6);
	speed1modeButton = new MultiKick (size, this, kSpeed1mode, 3, gSpeedModeButton->getHeight()/6, gSpeedModeButton, point);
	speed1modeButton->setValue(effect->getParameter(kSpeed1mode));
	frame->addView(speed1modeButton);

	// head 2 speed mode button
	size.offset (0, (kWideFaderInc*2)+kWideFaderMoreInc);
	speed2modeButton = new MultiKick (size, this, kSpeed2mode, 3, gSpeedModeButton->getHeight()/6, gSpeedModeButton, point);
	speed2modeButton->setValue(effect->getParameter(kSpeed2mode));
	frame->addView(speed2modeButton);

	// DFX web page link
	size (kDFXlinkX, kDFXlinkY, kDFXlinkX + gDFXlink->getWidth(), kDFXlinkY + (gDFXlink->getHeight())/2);
	DFXlink = new CWebLink (size, this, kDFXlinkID, DESTROYFX_URL, gDFXlink);
	frame->addView(DFXlink);

	// Super Destroy FX web page link
	size (kSuperDFXlinkX, kSuperDFXlinkY, kSuperDFXlinkX + gSuperDFXlink->getWidth(), kSuperDFXlinkY + (gSuperDFXlink->getHeight())/2);
	SuperDFXlink = new CWebLink (size, this, kSuperDFXlinkID, DESTROYFX_URL, gSuperDFXlink);
	frame->addView(SuperDFXlink);

	// Smart Electronix web page link
	size (kSmartElectronixLinkX, kSmartElectronixLinkY, kSmartElectronixLinkX + gSmartElectronixLink->getWidth(), kSmartElectronixLinkY + (gSmartElectronixLink->getHeight())/2);
	SmartElectronixLink = new CWebLink (size, this, kSmartElectronixLinkID, SMARTELECTRONIX_URL, gSmartElectronixLink);
	frame->addView(SmartElectronixLink);

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


	//--initialize the fine control buttons---------------------------------

	// head 1 speed fine down button
	size (kFineDownButtonX, kFineButtonY, kFineDownButtonX + gFineDownButton->getWidth(), kFineButtonY + (gFineDownButton->getHeight())/2);
	speed1FineDownButton = new CKickButton (size, this, kSpeed1FineDown, (gFineDownButton->getHeight())/2, gFineDownButton, point);
	speed1FineDownButton->setValue(0.0f);
	frame->addView(speed1FineDownButton);

	// head 1 feedback fine down button
	size.offset (0, kWideFaderInc);
	feed1FineDownButton = new CFineTuneButton (size, this, kFeed1, (gFineDownButton->getHeight())/2, gFineDownButton, point, kFineDown);
	feed1FineDownButton->setValue(effect->getParameter(kFeed1));
	frame->addView(feed1FineDownButton);

	// head 1 starting distance fine down button
	size.offset (0, kWideFaderInc);
	dist1FineDownButton = new CFineTuneButton (size, this, kDist1, (gFineDownButton->getHeight())/2, gFineDownButton, point, kFineDown);
	dist1FineDownButton->setValue(effect->getParameter(kDist1));
	frame->addView(dist1FineDownButton);

	// head 2 speed fine down button
	size.offset (0, kWideFaderMoreInc);
	speed2FineDownButton = new CKickButton (size, this, kSpeed2FineDown, (gFineDownButton->getHeight())/2, gFineDownButton, point);
	speed2FineDownButton->setValue(0.0f);
	frame->addView(speed2FineDownButton);

	// head 2 feedback fine down button
	size.offset (0, kWideFaderInc);
	feed2FineDownButton = new CFineTuneButton (size, this, kFeed2, (gFineDownButton->getHeight())/2, gFineDownButton, point, kFineDown);
	feed2FineDownButton->setValue(effect->getParameter(kFeed2));
	frame->addView(feed2FineDownButton);

	// head 2 starting distance fine down button
	size.offset (0, kWideFaderInc);
	dist2FineDownButton = new CFineTuneButton (size, this, kDist2, (gFineDownButton->getHeight())/2, gFineDownButton, point, kFineDown);
	dist2FineDownButton->setValue(effect->getParameter(kDist2));
	frame->addView(dist2FineDownButton);

	// buffer size fine down button
	size.offset (0, kWideFaderEvenMoreInc);
	bsizeFineDownButton = new CFineTuneButton (size, this, kBsize, (gFineDownButton->getHeight())/2, gFineDownButton, point, kFineDown);
	bsizeFineDownButton->setValue(effect->getParameter(kBsize));
	frame->addView(bsizeFineDownButton);

	// head 1 speed fine up button
	size (kFineUpButtonX, kFineButtonY, kFineUpButtonX + gFineUpButton->getWidth(), kFineButtonY + (gFineUpButton->getHeight())/2);
	speed1FineUpButton = new CKickButton (size, this, kSpeed1FineUp, (gFineUpButton->getHeight())/2, gFineUpButton, point);
	speed1FineUpButton->setValue(0.0f);
	frame->addView(speed1FineUpButton);

	// head 1 feedback fine up button
	size.offset (0, kWideFaderInc);
	feed1FineUpButton = new CFineTuneButton (size, this, kFeed1, (gFineUpButton->getHeight())/2, gFineUpButton, point, kFineUp);
	feed1FineUpButton->setValue(effect->getParameter(kFeed1));
	frame->addView(feed1FineUpButton);

	// head 1 starting distance fine up button
	size.offset (0, kWideFaderInc);
	dist1FineUpButton = new CFineTuneButton (size, this, kDist1, (gFineUpButton->getHeight())/2, gFineUpButton, point, kFineUp);
	dist1FineUpButton->setValue(effect->getParameter(kDist1));
	frame->addView(dist1FineUpButton);

	// head 2 speed fine up button
	size.offset (0, kWideFaderMoreInc);
	speed2FineUpButton = new CKickButton (size, this, kSpeed2FineUp, (gFineUpButton->getHeight())/2, gFineUpButton, point);
	speed2FineUpButton->setValue(0.0f);
	frame->addView(speed2FineUpButton);

	// head 2 feedback fine up button
	size.offset (0, kWideFaderInc);
	feed2FineUpButton = new CFineTuneButton (size, this, kFeed2, (gFineUpButton->getHeight())/2, gFineUpButton, point, kFineUp);
	feed2FineUpButton->setValue(effect->getParameter(kFeed2));
	frame->addView(feed2FineUpButton);

	// head 2 starting distance fine up button
	size.offset (0, kWideFaderInc);
	dist2FineUpButton = new CFineTuneButton (size, this, kDist2, (gFineUpButton->getHeight())/2, gFineUpButton, point, kFineUp);
	dist2FineUpButton->setValue(effect->getParameter(kDist2));
	frame->addView(dist2FineUpButton);

	// buffer size fine up button
	size.offset (0, kWideFaderEvenMoreInc);
	bsizeFineUpButton = new CFineTuneButton (size, this, kBsize, (gFineUpButton->getHeight())/2, gFineUpButton, point, kFineUp);
	bsizeFineUpButton->setValue(effect->getParameter(kBsize));
	frame->addView(bsizeFineUpButton);


	//--initialize the displays---------------------------------------------

/*
#if MAC
#if CALL_NOT_IN_CARBON
  SInt16 fontNum;

	GetFNum("\psnoot.org pixel10", &fontNum);
	if (fontNum != kUnresolvedCFragSymbolAddress)
	{
		TextFont(fontNum);
		TextSize(10);
	}

#else
  FMFontFamily fontFamily;
	fontFamily = FMGetFontFamilyFromName("snoot.org pixel10");
	if (fontFamily != kInvalidFontFamily)
	{
		TextFont(fontFamily);
		TextSize(10);
	}
#endif
#endif
*/

	// head 1 speed display   (typable)
	size (kDisplayX, kDisplayY, kDisplayX + kDisplayWidth, kDisplayY + kDisplayHeight);
	speed1TextEdit = new CTextEdit (size, this, kSpeed1TextEdit, 0, gBackground);
	displayOffset (kDisplayX, kDisplayY);
	speed1TextEdit->setBackOffset(displayOffset);
	speed1TextEdit->setHoriAlign(kRightText);
	speed1TextEdit->setFont(kNormalFontSmall);
	speed1TextEdit->setFontColor(kBlueTextCColor);
	frame->addView(speed1TextEdit);
	// this makes it display the current value
	setParameter(kSpeed1, effect->getParameter(kSpeed1));

	// head 1 feedback display
	size.offset (0, kWideFaderInc);
	feed1Display = new CParamDisplay (size, gBackground);
	displayOffset.offset (0, kWideFaderInc);
	feed1Display->setBackOffset(displayOffset);
	feed1Display->setHoriAlign(kRightText);
	feed1Display->setFont(kNormalFontSmall);
	feed1Display->setFontColor(kBlueTextCColor);
	feed1Display->setValue(effect->getParameter(kFeed1));
	feed1Display->setStringConvert(feedDisplayConvert);
	frame->addView(feed1Display);

	// head 1 starting distance display
	size.offset (0, kWideFaderInc);
	dist1Display = new CParamDisplay (size, gBackground);
	displayOffset.offset (0, kWideFaderInc);
	dist1Display->setBackOffset(displayOffset);
	dist1Display->setHoriAlign(kRightText);
	dist1Display->setFont(kNormalFontSmall);
	dist1Display->setFontColor(kBlueTextCColor);
	dist1Display->setValue(effect->getParameter(kDist1));
	dist1Display->setStringConvert(distDisplayConvert, &fBsize);
	dist1Display->setTag(kDist1);
	frame->addView(dist1Display);

	// head 2 speed display   (typable)
	size.offset (0, kWideFaderMoreInc);
	speed2TextEdit = new CTextEdit (size, this, kSpeed2TextEdit, 0, gBackground);
	displayOffset.offset (0, kWideFaderMoreInc);
	speed2TextEdit->setBackOffset(displayOffset);
	speed2TextEdit->setHoriAlign(kRightText);
	speed2TextEdit->setFont(kNormalFontSmall);
	speed2TextEdit->setFontColor(kBlueTextCColor);
	frame->addView(speed2TextEdit);
	// this makes it display the current value
	setParameter(kSpeed2, effect->getParameter(kSpeed2));

	// head 2 feedback display
	size.offset (0, kWideFaderInc);
	feed2Display = new CParamDisplay (size, gBackground);
	displayOffset.offset (0, kWideFaderInc);
	feed2Display->setBackOffset(displayOffset);
	feed2Display->setHoriAlign(kRightText);
	feed2Display->setFont(kNormalFontSmall);
	feed2Display->setFontColor(kBlueTextCColor);
	feed2Display->setValue(effect->getParameter(kFeed2));
	feed2Display->setStringConvert(feedDisplayConvert);
	frame->addView(feed2Display);

	// head 2 starting distance display
	size.offset (0, kWideFaderInc);
	dist2Display = new CParamDisplay (size, gBackground);
	displayOffset.offset (0, kWideFaderInc);
	dist2Display->setBackOffset(displayOffset);
	dist2Display->setHoriAlign(kRightText);
	dist2Display->setFont(kNormalFontSmall);
	dist2Display->setFontColor(kBlueTextCColor);
	dist2Display->setValue(effect->getParameter(kDist2));
	dist2Display->setStringConvert(distDisplayConvert, &fBsize);
	dist2Display->setTag(kDist2);
	frame->addView(dist2Display);

	// buffer size display
	size.offset (0, kWideFaderEvenMoreInc);
	bsizeDisplay = new CParamDisplay (size, gBackground);
	displayOffset.offset (0, kWideFaderEvenMoreInc);
	bsizeDisplay->setBackOffset(displayOffset);
	bsizeDisplay->setHoriAlign(kRightText);
	bsizeDisplay->setFont(kNormalFontSmall);
	bsizeDisplay->setFontColor(kBlueTextCColor);
	bsizeDisplay->setValue(effect->getParameter(kBsize));
	bsizeDisplay->setStringConvert(bsizeDisplayConvert);
	frame->addView(bsizeDisplay);


	for (long i=0; i < NUM_PARAMETERS; i++)
		faders[i] = NULL;
	faders[kSpeed1] = speed1Fader;
	faders[kFeed1] = feed1Fader;
	faders[kDist1] = dist1Fader;
	faders[kSpeed2] = speed2Fader;
	faders[kFeed2] = feed2Fader;
	faders[kDist2] = dist2Fader;
	faders[kBsize] = bsizeFader;

	for (long j=0; j < NUM_PARAMETERS; j++)
		tallFaders[j] = NULL;
	tallFaders[kDrymix] = drymixFader;
	tallFaders[kMix1] = mix1Fader;
	tallFaders[kMix2] = mix2Fader;


	return true;
}

//-----------------------------------------------------------------------------
void TransverbEditor::close()
{
	if (frame)
		delete frame;
	frame = 0;

	for (long i=0; i < NUM_PARAMETERS; i++)
		faders[i] = NULL;
	for (long j=0; j < NUM_PARAMETERS; j++)
		tallFaders[j] = NULL;

	chunk->resetLearning();

	// free some graphics
	if (gPurpleWideFaderSlide)
		gPurpleWideFaderSlide->forget();
	gPurpleWideFaderSlide = 0;
	if (gGreyWideFaderSlide)
		gGreyWideFaderSlide->forget();
	gGreyWideFaderSlide = 0;
	if (gPurpleTallFaderSlide)
		gPurpleTallFaderSlide->forget();
	gPurpleTallFaderSlide = 0;

	if (gPurpleWideFaderHandle)
		gPurpleWideFaderHandle->forget();
	gPurpleWideFaderHandle = 0;
	if (gGreyWideFaderHandle)
		gGreyWideFaderHandle->forget();
	gGreyWideFaderHandle = 0;
	if (gTallFaderHandle)
		gTallFaderHandle->forget();
	gTallFaderHandle = 0;

	if (gGlowingPurpleWideFaderHandle)
		gGlowingPurpleWideFaderHandle->forget();
	gGlowingPurpleWideFaderHandle = 0;
	if (gGlowingGreyWideFaderHandle)
		gGlowingGreyWideFaderHandle->forget();
	gGlowingGreyWideFaderHandle = 0;
	if (gGlowingTallFaderHandle)
		gGlowingTallFaderHandle->forget();
	gGlowingTallFaderHandle = 0;

	if (gFineDownButton)
		gFineDownButton->forget();
	gFineDownButton = 0;
	if (gFineUpButton)
		gFineUpButton->forget();
	gFineUpButton = 0;
	if (gSpeedModeButton)
		gSpeedModeButton->forget();
	gSpeedModeButton = 0;

	if (gQualityButton)
		gQualityButton->forget();
	gQualityButton = 0;
	if (gOnOffButton)
		gOnOffButton->forget();
	gOnOffButton = 0;
	if (gRandomButton)
		gRandomButton->forget();
	gRandomButton = 0;

	if (gMidiLearnButton)
		gMidiLearnButton->forget();
	gMidiLearnButton = 0;
	if (gMidiResetButton)
		gMidiResetButton->forget();
	gMidiResetButton = 0;

	if (gDFXlink)
		gDFXlink->forget();
	gDFXlink = 0;
	if (gSuperDFXlink)
		gSuperDFXlink->forget();
	gSuperDFXlink = 0;
	if (gSmartElectronixLink)
		gSmartElectronixLink->forget();
	gSmartElectronixLink = 0;
}

//-----------------------------------------------------------------------------
void TransverbEditor::setParameter(long index, float value)
{
	if (!frame)
		return;

	// called from TransverbEdit
	switch (index)
	{
		case kBsize:
			if (bsizeFader)
				bsizeFader->setValue(value);
			if (bsizeDisplay)
				bsizeDisplay->setValue(value);
			if (bsizeFineDownButton)
				bsizeFineDownButton->setValue(value);
			if (bsizeFineUpButton)
				bsizeFineUpButton->setValue(value);
			// update the distance displays if the buffer size is changed
			fBsize = ((DfxPlugin*)effect)->getparameter_gen(kBsize);
			if (dist1Display)
				dist1Display->setDirty();
			if (dist2Display)
				dist2Display->setDirty();
			break;

		case kSpeed1:
			if (speed1Fader)
				speed1Fader->setValue(value);
			if (speed1TextEdit)
			{
				speedDisplayConvert(value, speedString);
				speed1TextEdit->setText(speedString);
			}
			break;

		case kFeed1:
			if (feed1Fader)
				feed1Fader->setValue(value);
			if (feed1Display)
				feed1Display->setValue(value);
			if (feed1FineDownButton)
				feed1FineDownButton->setValue(value);
			if (feed1FineUpButton)
				feed1FineUpButton->setValue(value);
			break;

		case kDist1:
			fBsize = ((DfxPlugin*)effect)->getparameter_gen(kBsize);
			if (dist1Fader)
				dist1Fader->setValue(value);
			if (dist1Display)
				dist1Display->setValue(value);
			if (dist1FineDownButton)
				dist1FineDownButton->setValue(value);
			if (dist1FineUpButton)
				dist1FineUpButton->setValue(value);
			break;

		case kSpeed2:
			if (speed2Fader)
				speed2Fader->setValue(value);
			if (speed2TextEdit)
			{
				speedDisplayConvert(value, speedString);
				speed2TextEdit->setText(speedString);
			}
			break;

		case kFeed2:
			if (feed2Fader)
				feed2Fader->setValue(value);
			if (feed2Display)
				feed2Display->setValue(value);
			if (feed2FineDownButton)
				feed2FineDownButton->setValue(value);
			if (feed2FineUpButton)
				feed2FineUpButton->setValue(value);
			break;

		case kDist2:
			fBsize = ((DfxPlugin*)effect)->getparameter_gen(kBsize);
			if (dist2Fader)
				dist2Fader->setValue(value);
			if (dist2Display)
				dist2Display->setValue(value);
			if (dist2FineDownButton)
				dist2FineDownButton->setValue(value);
			if (dist2FineUpButton)
				dist2FineUpButton->setValue(value);
			break;

		case kDrymix:
			if (drymixFader)
				drymixFader->setValue(value);
			break;

		case kMix1:
			if (mix1Fader)
				mix1Fader->setValue(value);
			break;

		case kMix2:
			if (mix2Fader)
				mix2Fader->setValue(value);
			break;

		case kQuality:
			if (qualityButton)
				qualityButton->setValue(value);
			break;

		case kTomsound:
			if (tomsoundButton)
				tomsoundButton->setValue(value);
			break;

		case kSpeed1mode:
			if (speed1modeButton)
				speed1modeButton->setValue(value);
			break;

		case kSpeed2mode:
			if (speed2modeButton)
				speed2modeButton->setValue(value);
			break;

		default:
			return;
	}	

	postUpdate();
}

//-----------------------------------------------------------------------------
void TransverbEditor::valueChanged(CDrawContext* context, CControl* control)
{
  long tag = control->getTag();
  float octaves, semitones, tempSpeed;
  long sscanfReturn;


	switch (tag)
	{
		// process the speed text input
		case kSpeed1TextEdit:
		case kSpeed2TextEdit:
			if ( (speed1TextEdit) && (speed2TextEdit) )
			{
				if (tag == kSpeed1TextEdit)
					speed1TextEdit->getText(speedString);
				else
					speed2TextEdit->getText(speedString);
				sscanfReturn = sscanf(speedString, "%f%f", &octaves, &semitones);
				if ( (sscanfReturn != EOF) && (sscanfReturn > 0) )
				{
					// the user only entered one number, which is for octaves, 
					// so convert any fractional part of the octaves value into semitones
					if (sscanfReturn == 1)
						semitones = fmodf(fabsf(octaves), 1.0f) * 12.0f;
					// no negative values for the semitones ...
					else if ( (octaves >= 1.0f) || (octaves <= -1.0f) )
						semitones = fabsf(semitones);
					// ... unless the octaves value was negative or in the zero range
					if (octaves < 0.0f)
						semitones = 0.0f - fabsf(semitones);
					// scale the new speed to a 0 to 1 parameter value
					tempSpeed = floorf(octaves) + (semitones/12.0f);
					tempSpeed = (tempSpeed-SPEED_MIN) / (SPEED_MAX-SPEED_MIN);
					// this updates the display with units appended & the fader position
//					setParameter(tag-kTextEditOffset, tempSpeed);
					effect->setParameterAutomated(tag-kSpeedTextEditOffset, tempSpeed);
				}
				// there was a sscanf() error
				else if (tag == kSpeed1TextEdit)
					speed1TextEdit->setText("bad");
				else
					speed2TextEdit->setText("bad");
			}
			break;

		// parameter randomizer button
		case kRandomButton:
			// only if the button has just been pressed down, not released
			if (control->getValue() >= 0.5f)
				((DfxPlugin*)effect)->randomizeparameters(true);
			break;

		// clicking on these parts of the GUI takes you to Destroy FX or SE web pages
		case kDFXlinkID:
		case kSuperDFXlinkID:
		case kSmartElectronixLinkID:
			break;

		case kMidiLearnButtonID:
			chunk->setParameterMidiLearn(control->getValue() > 0.5f);
			break;
		case kMidiResetButtonID:
			chunk->setParameterMidiReset(control->getValue() > 0.5f);
			break;

		case kSpeed1FineDown:
		case kSpeed1FineUp:
			if (control->getValue() >= 0.5f)
			{
				if (tag == kSpeed1FineDown)
					effect->setParameterAutomated(kSpeed1, speedDown(kSpeed1, kSpeed1mode));
				else
					effect->setParameterAutomated(kSpeed1, speedUp(kSpeed1, kSpeed1mode));
			}
			chunk->setLearner(kSpeed1);
			if (chunk->isLearning())
			{
				if (speed1Fader != NULL)
				{
					if (speed1Fader->getHandle() == gPurpleWideFaderHandle)
					{
						speed1Fader->setHandle(gGlowingPurpleWideFaderHandle);
						speed1Fader->setDirty();
					}
				}
			}
			break;

		case kSpeed2FineDown:
		case kSpeed2FineUp:
			if (control->getValue() >= 0.5f)
			{
				if (tag == kSpeed2FineDown)
					effect->setParameterAutomated(kSpeed2, speedDown(kSpeed2, kSpeed2mode));
				else
					effect->setParameterAutomated(kSpeed2, speedUp(kSpeed2, kSpeed2mode));
			}
			chunk->setLearner(kSpeed2);
			if (chunk->isLearning())
			{
				if (speed2Fader != NULL)
				{
					if (speed2Fader->getHandle() == gPurpleWideFaderHandle)
					{
						speed2Fader->setHandle(gGlowingPurpleWideFaderHandle);
						speed2Fader->setDirty();
					}
				}
			}
			break;

		case kTomsound:
			effect->setParameterAutomated(tag, control->getValue());
			// this is an on/off button, so use toggle MIDI control
			chunk->setLearner(tag, kEventBehaviourToggle);
			break;

		// these are multi-state switches, so use multi-state toggle MIDI control
		case kQuality:
			effect->setParameterAutomated(tag, control->getValue());
			chunk->setLearner(tag, kEventBehaviourToggle, numQualities);
			break;
		case kSpeed1mode:
		case kSpeed2mode:
			effect->setParameterAutomated(tag, control->getValue());
			chunk->setLearner(tag, kEventBehaviourToggle, numSpeedModes);
			break;

		case kBsize:
		case kDrymix:
		case kMix1:
		case kDist1:
		case kSpeed1:
		case kFeed1:
		case kMix2:
		case kDist2:
		case kSpeed2:
		case kFeed2:
			effect->setParameterAutomated(tag, control->getValue());

			chunk->setLearner(tag);
			if (chunk->isLearning())
			{
				if (faders[tag] != NULL)
				{
					if (faders[tag]->getHandle() == gPurpleWideFaderHandle)
					{
						faders[tag]->setHandle(gGlowingPurpleWideFaderHandle);
						faders[tag]->setDirty();
					}
					else if (faders[tag]->getHandle() == gGreyWideFaderHandle)
					{
						faders[tag]->setHandle(gGlowingGreyWideFaderHandle);
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
float TransverbEditor::speedDown(long speedTag, long speedModeTag)
{
  float oldSpeedValue = effect->getParameter(speedTag);
  float newSpeedValue = 0.333333333f;


	switch (speedModeScaled(effect->getParameter(speedModeTag)))
	{
		case kFineMode:
			if ( oldSpeedValue < FINE_INCR )
				newSpeedValue = 0.0f;
			else
				newSpeedValue = oldSpeedValue - FINE_INCR;
			break;
		case kSemitoneMode:
			if ( speedScaled(oldSpeedValue) < (SPEED_MIN+(1.0f/12.0f)) )
				newSpeedValue = 0.0f;
			else
			{
				newSpeedValue = (speedScaled(oldSpeedValue) * 12.0f) - 1.001f;
				newSpeedValue = nearestIntegerAbove(newSpeedValue);
				newSpeedValue = speedUnscaled(newSpeedValue/12.0f);
			}
			break;
		case kOctaveMode:
			if ( speedScaled(oldSpeedValue) < (SPEED_MIN+1.0f) )
				newSpeedValue = 0.0f;
			else
			{
				newSpeedValue = speedScaled(oldSpeedValue) - 1.001f;
				newSpeedValue = nearestIntegerAbove(newSpeedValue);
				newSpeedValue = speedUnscaled(newSpeedValue);
			}
			break;
		default:
			break;
	}

	return newSpeedValue;
}

//-----------------------------------------------------------------------------
float TransverbEditor::speedUp(long speedTag, long speedModeTag)
{
  float newSpeedValue, oldSpeedValue = effect->getParameter(speedTag);


	switch (speedModeScaled(effect->getParameter(speedModeTag)))
	{
		case kFineMode:
			if ( oldSpeedValue > (1.0f-FINE_INCR) )
				newSpeedValue = 1.0f;
			else
				newSpeedValue = oldSpeedValue + FINE_INCR;
			break;
		case kSemitoneMode:
			if ( speedScaled(oldSpeedValue) > (SPEED_MAX-(1.0f/12.0f)) )
				newSpeedValue = 1.0f;
			else
			{
				newSpeedValue = (speedScaled(oldSpeedValue) * 12.0f) + 1.001f;
				newSpeedValue = nearestIntegerBelow(newSpeedValue);
				newSpeedValue = speedUnscaled(newSpeedValue/12.0f);
			}
			break;
		case kOctaveMode:
			if ( speedScaled(oldSpeedValue) > (SPEED_MAX-1.0f) )
				newSpeedValue = 1.0f;
			else
			{
				newSpeedValue = speedScaled(oldSpeedValue) + 1.001f;
				newSpeedValue = nearestIntegerBelow(newSpeedValue);
				newSpeedValue = speedUnscaled(newSpeedValue);
			}
			break;
		default:
			newSpeedValue = speedUnscaled(0.0f);
			break;
	}

	return newSpeedValue;
}

//-----------------------------------------------------------------------------
float TransverbEditor::nearestIntegerBelow(float number)
{
  bool sign = (number >= 0.0f);
  float fraction = fmodf(fabsf(number), 1.0f);

	if ( (fraction >= -0.0001f) && (fraction <= 0.0001f) )
		return number;

	if (sign)
		return (float) ((long)fabsf(number));
	else
		return 0.0f - (float) ((long)fabsf(number) + 1);
}

//-----------------------------------------------------------------------------
float TransverbEditor::nearestIntegerAbove(float number)
{
  bool sign = (number >= 0.0f);
  float fraction = fmodf(fabsf(number), 1.0f);

	if ( (fraction >= -0.0001f) && (fraction <= 0.0001f) )
		return number;

	if (sign)
		return (float) ((long)fabsf(number) + 1);
	else
		return 0.0f - (float) ((long)fabsf(number));
}


//-----------------------------------------------------------------------------
void TransverbEditor::idle()
{
  bool somethingChanged = false;


	for (long i=0; i < NUM_PARAMETERS; i++)
	{
		if (i != chunk->getLearner())
		{
			if (faders[i] != NULL)
			{
				if (faders[i]->getHandle() == gGlowingPurpleWideFaderHandle)
				{
					faders[i]->setHandle(gPurpleWideFaderHandle);
					faders[i]->setDirty();
					somethingChanged = true;
				}
				else if (faders[i]->getHandle() == gGlowingGreyWideFaderHandle)
				{
					faders[i]->setHandle(gGreyWideFaderHandle);
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

	// indicate that changed controls need to be redrawn
	if (somethingChanged)
		postUpdate();

	// this is called so that idle() actually happens
	AEffGUIEditor::idle();
}
