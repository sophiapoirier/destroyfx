#ifndef __MIDIGATEREDITOR_H
#include "midigatereditor.hpp"
#endif

#ifndef __MIDIGATER_H
#include "midigater.hpp"
#endif

#include <stdio.h>


//-----------------------------------------------------------------------------
enum {
	// resource IDs
	kBackgroundID = 128,
	kSlopeFaderHandleID,
	kVelInfluenceFaderHandleID,
	kFloorFaderHandleID,
	kDestroyFXlinkID,

	// positions
	kFaderX = 14,
	kFaderY = 66,
	kFaderInc = 52,
	kFaderWidth = 280,
	kFaderHeight = 19,

	kDisplayX = kFaderX + 300 + 1,
	kDisplayY = kFaderY + 3,
	kDisplayWidth = 56 - 1,
	kDisplayHeight = 10,

	kDestroyFXlinkX = 269,
	kDestroyFXlinkY = 202
};



//-----------------------------------------------------------------------------
// parameter value string display conversion functions

void slopeDisplayConvert(float value, char *string);
void slopeDisplayConvert(float value, char *string)
{
  float slope, remainder;
  long thousands;

	slope = value * value * SLOPE_MAX;
	thousands = (long)slope / 1000;
	remainder = fmodf(slope, 1000.0f);

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
		sprintf(string, "%.1f  ms", slope);
}

void velInfluenceDisplayConvert(float value, char *string);
void velInfluenceDisplayConvert(float value, char *string)
{
	sprintf(string, "%.1f %%", value*100.0f);
}

void floorDisplayConvert(float value, char *string);
void floorDisplayConvert(float value, char *string)
{
	if (value <= 0.0f)
		sprintf(string, "-oo  dB");
	else
		sprintf(string, "%.1f  dB", dBconvert(value*value*value));
}



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
MidiGaterEditor::MidiGaterEditor(AudioEffect *effect)
 : AEffGUIEditor(effect) 
{
	frame = 0;

	// initialize the graphics pointers
	gBackground = 0;
	gSlopeFaderHandle = 0;
	gVelInfluenceFaderHandle = 0;
	gFloorFaderHandle = 0;
	gDestroyFXlink = 0;

	// initialize the controls pointers
	slopeFader = 0;
	velInfluenceFader = 0;
	floorFader = 0;
	DestroyFXlink = 0;

	// initialize the value display box pointers
	slopeDisplay = 0;
	velInfluenceDisplay = 0;
	floorDisplay = 0;

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
MidiGaterEditor::~MidiGaterEditor()
{
	// free background bitmap
	if (gBackground)
		gBackground->forget();
	gBackground = 0;
}

//-----------------------------------------------------------------------------
long MidiGaterEditor::getRect(ERect **erect)
{
	*erect = &rect;
	return true;
}

//-----------------------------------------------------------------------------
long MidiGaterEditor::open(void *ptr)
{
  CPoint displayOffset;	// for positioning the background graphic behind display boxes


	// !!! always call this !!!
	AEffGUIEditor::open(ptr);

	// load some graphics
	if (!gSlopeFaderHandle)
		gSlopeFaderHandle = new CBitmap (kSlopeFaderHandleID);
	if (!gVelInfluenceFaderHandle)
		gVelInfluenceFaderHandle = new CBitmap (kVelInfluenceFaderHandleID);
	if (!gFloorFaderHandle)
		gFloorFaderHandle = new CBitmap (kFloorFaderHandleID);

	if (!gDestroyFXlink)
		gDestroyFXlink = new CBitmap (kDestroyFXlinkID);


	//--initialize the background frame--------------------------------------
	CRect size (0, 0, gBackground->getWidth(), gBackground->getHeight());
	frame = new CFrame (size, ptr, this);
	frame->setBackground(gBackground);


	//--initialize the horizontal faders-------------------------------------
	int minPos = kFaderX;
	int maxPos = kFaderX + kFaderWidth - gSlopeFaderHandle->getWidth();
	CPoint point (0, 0);

	// slope
	// size stores left, top, right, & bottom positions
	size (kFaderX, kFaderY, kFaderX + kFaderWidth, kFaderY + kFaderHeight);
	displayOffset (size.left, size.top);
	slopeFader = new CHorizontalSlider (size, this, kSlope, minPos, maxPos, gSlopeFaderHandle, gBackground, displayOffset, kLeft);
	slopeFader->setDrawTransparentHandle(false);
	slopeFader->setValue(effect->getParameter(kSlope));
	slopeFader->setDefaultValue(sqrtf(3.0f/SLOPE_MAX));
	frame->addView(slopeFader);

	// velocity influence
	size.offset (0, kFaderInc);
	displayOffset (size.left, size.top);
	velInfluenceFader = new CHorizontalSlider (size, this, kVelInfluence, minPos, maxPos, gVelInfluenceFaderHandle, gBackground, displayOffset, kLeft);
	velInfluenceFader->setValue(effect->getParameter(kVelInfluence));
	velInfluenceFader->setDefaultValue(1.0f);
	frame->addView(velInfluenceFader);

	// floor
	size.offset (0, kFaderInc);
	displayOffset (size.left, size.top);
	floorFader = new CHorizontalSlider (size, this, kFloor, minPos, maxPos, gFloorFaderHandle, gBackground, displayOffset, kLeft);
	floorFader->setValue(effect->getParameter(kFloor));
	floorFader->setDefaultValue(0.0f);
	frame->addView(floorFader);

	//--initialize the buttons----------------------------------------------

	// Destroy FX web page link
	size (kDestroyFXlinkX, kDestroyFXlinkY, kDestroyFXlinkX + gDestroyFXlink->getWidth(), kDestroyFXlinkY + (gDestroyFXlink->getHeight())/2);
	DestroyFXlink = new CWebLink (size, this, kDestroyFXlinkID, DESTROYFX_URL, gDestroyFXlink);
	frame->addView(DestroyFXlink);


	//--initialize the displays---------------------------------------------

	// slope
	size (kDisplayX, kDisplayY, kDisplayX + kDisplayWidth, kDisplayY + kDisplayHeight);
	displayOffset (size.left, size.top);
	slopeDisplay = new CParamDisplay (size, gBackground);
	slopeDisplay->setBackOffset(displayOffset);
	slopeDisplay->setHoriAlign(kCenterText);
	slopeDisplay->setFont(kNormalFontVerySmall);
	slopeDisplay->setFontColor(kMyFontCColor);
	slopeDisplay->setValue(effect->getParameter(kSlope));
	slopeDisplay->setStringConvert(slopeDisplayConvert);
	frame->addView(slopeDisplay);

	// velocity influence
	size.offset (0, kFaderInc);
	displayOffset (size.left, size.top);
	velInfluenceDisplay = new CParamDisplay (size, gBackground);
	velInfluenceDisplay->setBackOffset(displayOffset);
	velInfluenceDisplay->setHoriAlign(kCenterText);
	velInfluenceDisplay->setFont(kNormalFontVerySmall);
	velInfluenceDisplay->setFontColor(kMyFontCColor);
	velInfluenceDisplay->setValue(effect->getParameter(kVelInfluence));
	velInfluenceDisplay->setStringConvert(velInfluenceDisplayConvert);
	frame->addView(velInfluenceDisplay);

	// floor
	size.offset (0, kFaderInc);
	displayOffset (size.left, size.top);
	floorDisplay = new CParamDisplay (size, gBackground);
	floorDisplay->setBackOffset(displayOffset);
	floorDisplay->setHoriAlign(kCenterText);
	floorDisplay->setFont(kNormalFontVerySmall);
	floorDisplay->setFontColor(kMyFontCColor);
	floorDisplay->setValue(effect->getParameter(kFloor));
	floorDisplay->setStringConvert(floorDisplayConvert);
	frame->addView(floorDisplay);


	return true;
}

//-----------------------------------------------------------------------------
void MidiGaterEditor::close()
{
	if (frame)
		delete frame;
	frame = 0;

	// free some bitmaps
	if (gSlopeFaderHandle)
		gSlopeFaderHandle->forget();
	gSlopeFaderHandle = 0;
	if (gVelInfluenceFaderHandle)
		gVelInfluenceFaderHandle->forget();
	gVelInfluenceFaderHandle = 0;
	if (gFloorFaderHandle)
		gFloorFaderHandle->forget();
	gFloorFaderHandle = 0;

	if (gDestroyFXlink)
		gDestroyFXlink->forget();
	gDestroyFXlink = 0;
}

//-----------------------------------------------------------------------------
void MidiGaterEditor::setParameter(long index, float value)
{
	if (!frame)
		return;

	// called from MidiGaterEdit
	switch (index)
	{
		case kSlope:
			if (slopeFader)
				slopeFader->setValue(effect->getParameter(index));
			if (slopeDisplay)
				slopeDisplay->setValue(effect->getParameter(index));
			break;

		case kVelInfluence:
			if (velInfluenceFader)
				velInfluenceFader->setValue(effect->getParameter(index));
			if (velInfluenceDisplay)
				velInfluenceDisplay->setValue(effect->getParameter(index));
			break;

		case kFloor:
			if (floorFader)
				floorFader->setValue(effect->getParameter(index));
			if (floorDisplay)
				floorDisplay->setValue(effect->getParameter(index));
			break;

		default:
			return;
	}	

	postUpdate();
}

//-----------------------------------------------------------------------------
void MidiGaterEditor::valueChanged(CDrawContext* context, CControl* control)
{
  long tag = control->getTag();


	switch (tag)
	{
		case kSlope:
		case kVelInfluence:
		case kFloor:
			effect->setParameterAutomated(tag, control->getValue());
		case kDestroyFXlinkID:
			control->update(context);
			break;

		default:
			break;
	}
}
