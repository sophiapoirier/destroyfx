#ifndef __POLARIZEREDITOR_H
#include "polarizerEditor.hpp"
#endif

#ifndef __POLARIZER_H
#include "polarizer.hpp"
#endif

#include <stdio.h>


//-----------------------------------------------------------------------------
enum {
	// resource IDs
	kBackgroundID = 128,
	kFaderSlideID,
	kFaderHandleID,
	kImplodeButtonID,
	kDestroyFXlinkID,

	// positions
	kFaderX = 67,
	kFaderY = 66,
	kFaderInc = 118,

	kDisplayX = kFaderX - 32,
	kDisplayY = kFaderY + 221,
	kDisplayWidth = 110,
	kDisplayHeight = 18,

	kImplodeButtonX = kFaderX + (kFaderInc*2) - 4,
	kImplodeButtonY = kFaderY,

	kDestroyFXlinkX = 40,
	kDestroyFXlinkY = 322
};

#if MOTIF
// resource for MOTIF (format XPM)
#include "bmp00128.xpm"
#include "bmp00129.xpm"
#include "bmp00130.xpm"
#include "bmp00131.xpm"
#include "bmp00132.xpm"

CResTable xpmResources = {
	{kBackgroundID, bmp00128},
	{kFaderSlideID, bmp00129},
	{kFaderHandleID, bmp00130},
	{kImplodeButtonID, bmp00131},
	{kDestroyFXlinkID, bmp00132},
	{0, 0}
};
#endif



//-----------------------------------------------------------------------------
// parameter value string display conversion functions

void leapDisplayConvert(float value, char *string);
void leapDisplayConvert(float value, char *string)
{
  long leap = leapScaled(value);

	if (leap == 1)
		sprintf(string, "1 sample");
	else
		sprintf(string, "%d samples", leap);
}

void amountDisplayConvert(float value, char *string);
void amountDisplayConvert(float value, char *string)
{
//	sprintf(string, "%.3f", value);
	sprintf(string, "%ld %%", (long)(value*1000.0f));
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
PolarizerEditor::PolarizerEditor(AudioEffect *effect)
 : AEffGUIEditor(effect) 
{
	frame = 0;

	// initialize the graphics pointers
	gBackground = 0;
	gFaderSlide = 0;
	gFaderHandle = 0;
	gImplodeButton = 0;
	gDestroyFXlink = 0;

	// initialize the controls pointers
	leapFader = 0;
	amountFader = 0;
	implodeButton = 0;
	DestroyFXlink = 0;

	// initialize the value display box pointers
	leapDisplay = 0;
	amountDisplay = 0;

	// load the background bitmap
	// we don't need to load all bitmaps, this could be done when open is called
	gBackground = new CBitmap (kBackgroundID);

	// init the size of the plugin
	rect.left   = 0;
	rect.top    = 0;
	rect.right  = (short)gBackground->getWidth();
	rect.bottom = (short)gBackground->getHeight();
}

//-----------------------------------------------------------------------------
PolarizerEditor::~PolarizerEditor()
{
	// free background bitmap
	if (gBackground)
		gBackground->forget();
	gBackground = 0;
}

//-----------------------------------------------------------------------------
long PolarizerEditor::getRect(ERect **erect)
{
	*erect = &rect;
	return true;
}

//-----------------------------------------------------------------------------
long PolarizerEditor::open(void *ptr)
{
  CPoint displayOffset;	// for positioning the background graphic behind display boxes


	// !!! always call this !!!
	AEffGUIEditor::open(ptr);

	// load some graphics
	if (!gFaderSlide)
		gFaderSlide = new CBitmap (kFaderSlideID);
	if (!gFaderHandle)
		gFaderHandle = new CBitmap (kFaderHandleID);
	if (!gImplodeButton)
		gImplodeButton = new CBitmap (kImplodeButtonID);
	if (!gDestroyFXlink)
		gDestroyFXlink = new CBitmap (kDestroyFXlinkID);


	//--initialize the background frame--------------------------------------
	CRect size (0, 0, gBackground->getWidth(), gBackground->getHeight());
	frame = new CFrame (size, ptr, this);
	frame->setBackground(gBackground);


	//--initialize the faders---------------------------------
//	int minPos = kFaderY - 93;
//	int maxPos = kFaderY + 96;
//	int minPos = kFaderY - (gFaderSlide->getHeight() / 2) + 3;
//	int maxPos = kFaderY + (gFaderSlide->getHeight() / 2);
	int minPos = kFaderY - 84 - 17 + 15;
	int maxPos = kFaderY + 84 - 0 + 15;
	CPoint point (0, 0);
	CPoint offset (4, 0);

	// leap size
	size (kFaderX, kFaderY, kFaderX + gFaderSlide->getWidth(), kFaderY + gFaderSlide->getHeight());
	leapFader = new CVerticalSlider (size, this, kSkip, minPos, maxPos, gFaderHandle, gFaderSlide, point, kBottom);
	leapFader->setOffsetHandle(point);
	leapFader->setValue(effect->getParameter(kSkip));
	leapFader->setDefaultValue(0.09f);
	frame->addView(leapFader);

	// polarization amount
	size.offset (kFaderInc, 0);
	amountFader = new CVerticalSlider (size, this, kAmount, minPos, maxPos, gFaderHandle, gFaderSlide, point, kBottom);
	amountFader->setOffsetHandle(point);
	amountFader->setValue(effect->getParameter(kAmount));
	amountFader->setDefaultValue(0.5f);
	frame->addView(amountFader);

	//--initialize the buttons----------------------------------------------

	// IMPLODE
	size (kImplodeButtonX, kImplodeButtonY, kImplodeButtonX + gImplodeButton->getWidth(), kImplodeButtonY + (gImplodeButton->getHeight())/4);
	implodeButton = new MultiKick (size, this, kImplode, 2, gImplodeButton->getHeight()/4, gImplodeButton, point);
//	size (kImplodeButtonX, kImplodeButtonY, kImplodeButtonX + gImplodeButton->getWidth(), kImplodeButtonY + (gImplodeButton->getHeight())/2);
//	implodeButton = new COnOffButton (size, this, kImplode, gImplodeButton);
	implodeButton->setValue(effect->getParameter(kImplode));
	frame->addView(implodeButton);

	// Destroy FX web page link
	size (kDestroyFXlinkX, kDestroyFXlinkY, kDestroyFXlinkX + gDestroyFXlink->getWidth(), kDestroyFXlinkY + (gDestroyFXlink->getHeight())/2);
	DestroyFXlink = new CWebLink (size, this, kDestroyFXlinkID, "http://www.smartelectronix.com/~destroyfx/", gDestroyFXlink);
	frame->addView(DestroyFXlink);


	//--initialize the displays---------------------------------------------

	// leap size read-out
	size (kDisplayX, kDisplayY, kDisplayX + kDisplayWidth, kDisplayY + kDisplayHeight);
	leapDisplay = new CParamDisplay (size, gBackground);
	displayOffset (kDisplayX, kDisplayY);
	leapDisplay->setBackOffset(displayOffset);
	leapDisplay->setHoriAlign(kCenterText);
	leapDisplay->setFont(kNormalFontVeryBig);
	leapDisplay->setFontColor(kBlackCColor);
	leapDisplay->setValue(effect->getParameter(kSkip));
	leapDisplay->setStringConvert(leapDisplayConvert);
	frame->addView(leapDisplay);

	// polarization amount read-out
	size.offset (kFaderInc, 0);
	amountDisplay = new CParamDisplay (size, gBackground);
	displayOffset.offset (kFaderInc, 0);
	amountDisplay->setBackOffset(displayOffset);
	amountDisplay->setHoriAlign(kCenterText);
	amountDisplay->setFont(kNormalFontVeryBig);
	amountDisplay->setFontColor(kBlackCColor);
	amountDisplay->setValue(effect->getParameter(kAmount));
	amountDisplay->setStringConvert(amountDisplayConvert);
	frame->addView(amountDisplay);

	return true;
}

//-----------------------------------------------------------------------------
void PolarizerEditor::close()
{
	if (frame)
		delete frame;
	frame = 0;

	// free some bitmaps
	if (gFaderSlide)
		gFaderSlide->forget();
	gFaderSlide = 0;

	if (gFaderHandle)
		gFaderHandle->forget();
	gFaderHandle = 0;

	if (gImplodeButton)
		gImplodeButton->forget();
	gImplodeButton = 0;

	if (gDestroyFXlink)
		gDestroyFXlink->forget();
	gDestroyFXlink = 0;
}

//-----------------------------------------------------------------------------
void PolarizerEditor::setParameter(long index, float value)
{
	if (!frame)
		return;

	// called from PolarizerEdit
	switch (index)
	{
		case kSkip:
			if (leapFader)
				leapFader->setValue(effect->getParameter(index));
			if (leapDisplay)
				leapDisplay->setValue(effect->getParameter(index));
			break;

		case kAmount:
			if (amountFader)
				amountFader->setValue(effect->getParameter(index));
			if (amountDisplay)
				amountDisplay->setValue(effect->getParameter(index));
			break;

		case kImplode:
			if (implodeButton)
				implodeButton->setValue(effect->getParameter(index));
			break;

		default:
			return;
	}

	postUpdate();
}

//-----------------------------------------------------------------------------
void PolarizerEditor::valueChanged(CDrawContext* context, CControl* control)
{
  long tag = control->getTag();


	switch (tag)
	{
		case kSkip:
		case kAmount:
		case kImplode:
			effect->setParameterAutomated(tag, control->getValue());
		case kDestroyFXlinkID:
			control->update(context);
			break;

		default:
			break;
	}
}
