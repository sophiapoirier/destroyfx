/*----------------------- by Marc Poirier  ][  June 2001 ----------------------*/

#include "rmsbuddyeditor-vst.h"
#include "rmsbuddy-vst.h"

#include <stdio.h>
#include <math.h>


//-----------------------------------------------------------------------------
// constants

#define WAIT_THIS_MANY_SAMPLES_TO_REFRESH	3000

const CColor kMyBrownCColor = {97, 73, 46, 0};
//const CColor kMyLightishBlueCColor = {114, 114, 255, 0};
//const CColor kMyDeeperBlueCColor = {86, 86, 255, 0};
const CColor kMyLightBrownCColor = {146, 116, 98, 0};
const CColor kMyDarkBlueCColor = {54, 69, 115, 0};
const CColor kMyLightOrangeCColor = {219, 145, 85, 0};

#define kValueDisplayFont	kNormalFont
#define kLabelDisplayFont	kNormalFontSmall
#define kBackgroundColor	kMyLightBrownCColor
#define kLabelColor			kWhiteCColor
#define kReadoutFrameColor	kMyBrownCColor
#define kReadoutBoxColor	kMyLightOrangeCColor
#define kReadoutTextColor	kMyDarkBlueCColor


//-----------------------------------------------------------------------------
enum {
	// resource IDs
	kResetButtonID = 128,

	// positions
	kBackgroundWidth = 321,
	kBackgroundHeight = 123 + 33,

	kRMSlabelX = 6,
	kRMSlabelY = 27,
	kRMSlabelWidth = 78,
	kRMSlabelHeight = 17,

	kRLlabelX = kRMSlabelX + kRMSlabelWidth + 9,
	kRLlabelY = kRMSlabelY - 22,
	kRLlabelWidth = 75,
	kRLlabelHeight = 17,

	kValueDisplayX = kRLlabelX,
	kValueDisplayY = kRMSlabelY,
	kValueDisplayWidth = kRLlabelWidth,
	kValueDisplayHeight = 17,

	kXinc = kRLlabelWidth + 15,
	kYinc = 33,

	kButtonX = kValueDisplayX + (kXinc*2),
	kButtonY = kValueDisplayY + 1,

	kResetRmsTag = 0,
	kResetPeakTag
};



//-----------------------------------------------------------------------------
// parameter value string display conversion functions

void dBdisplayConvert(float value, char *string, void *theLinearValue);
void dBdisplayConvert(float value, char *string, void *theLinearValue)
{
	float linearValue = *(float*)theLinearValue;

	if (linearValue <= 0.0f)
		sprintf(string, "-oo  dB");
	else
	{
		float dBvalue = 20.0f * log10f(linearValue);	// convert linear value to dB
		if (dBvalue >= 0.01f)
			sprintf(string, " +%.2f  dB", dBvalue);
		else
			sprintf(string, " %.2f  dB", dBvalue);
	}
}

void staticTextDisplayConvert(float value, char *string, void *staticText);
void staticTextDisplayConvert(float value, char *string, void *staticText)
{
	strcpy(string, (const char*)staticText);
}



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
RMSbuddyEditor::RMSbuddyEditor(AudioEffect *effect)
 : AEffGUIEditor(effect) 
{
	frame = 0;

	// initialize the graphics pointers
	gResetButton = 0;

	// initialize the controls pointers
	resetRMSbutton = 0;
	resetPeakButton = 0;

	// initialize the value display box pointers
	backgroundDisplay = 0;
	leftAverageRMSDisplay = 0;
	rightAverageRMSDisplay = 0;
	leftContinualRMSDisplay = 0;
	rightContinualRMSDisplay = 0;
	leftAbsolutePeakDisplay = 0;
	rightAbsolutePeakDisplay = 0;
	leftContinualPeakDisplay = 0;
	rightContinualPeakDisplay = 0;
	averageRMSDisplay = 0;
	continualRMSDisplay = 0;
	absolutePeakDisplay = 0;
	continualPeakDisplay = 0;
	leftDisplay = 0;
	rightDisplay = 0;

	// load the background bitmap
	// we don't need to load all bitmaps, this could be done when open is called
//	gBackground = new CBitmap (kBackgroundID);

	// init the size of the plugin
	rect.left = 0;
	rect.top = 0;
	rect.right  = kBackgroundWidth;
	rect.bottom = kBackgroundHeight;
//	rect.right = (short)gBackground->getWidth();
//	rect.bottom = (short)gBackground->getHeight();
}

//-----------------------------------------------------------------------------
RMSbuddyEditor::~RMSbuddyEditor()
{
	// nud
}

//-----------------------------------------------------------------------------
long RMSbuddyEditor::getRect(ERect **erect)
{
	*erect = &rect;
	return 1;
}

//-----------------------------------------------------------------------------
long RMSbuddyEditor::open(void *ptr)
{
  CPoint point (0, 0);
//  CPoint displayOffset;	// for positioning the background graphic behind display boxes

	// !!! always call this !!!
	AEffGUIEditor::open(ptr);

	// load some bitmaps
	if (!gResetButton)
		gResetButton = new CBitmap (kResetButtonID);


	//--initialize the background frame--------------------------------------
	CRect size (0, 0, kBackgroundWidth, kBackgroundHeight);
	frame = new CFrame (size, ptr, this);


	//--initialize the displays---------------------------------------------

	// reset these display values
	theLeftContinualRMS = theRightContinualRMS = 0.0f;
	theLeftContinualPeak = theRightContinualPeak = 0.0f;
	// start the counting/accumulating cycle anew
	((RMSbuddy*)effect)->resetGUIcounters();

	// the background box
	size (0, 0, kBackgroundWidth, kBackgroundHeight);
	backgroundDisplay = new CParamDisplay (size);
	backgroundDisplay->setBackColor(kBackgroundColor);
	backgroundDisplay->setFrameColor(kBackgroundColor);
	backgroundDisplay->setValue(0.0f);
	backgroundDisplay->setStringConvert(staticTextDisplayConvert, (void*)" ");
	frame->addView(backgroundDisplay);

	// left channel average RMS read-out
	size (kValueDisplayX, kValueDisplayY, kValueDisplayX + kValueDisplayWidth, kValueDisplayY + kValueDisplayHeight);
	leftAverageRMSDisplay = new CParamDisplay (size);
	leftAverageRMSDisplay->setHoriAlign(kCenterText);
	leftAverageRMSDisplay->setFont(kValueDisplayFont);
	leftAverageRMSDisplay->setFontColor(kReadoutTextColor);
	leftAverageRMSDisplay->setBackColor(kReadoutBoxColor);
	leftAverageRMSDisplay->setFrameColor(kReadoutFrameColor);
	leftAverageRMSDisplay->setValue(0.0f);
	leftAverageRMSDisplay->setStringConvert( dBdisplayConvert, &((RMSbuddy*)effect)->leftAverageRMS );
	frame->addView(leftAverageRMSDisplay);

	// right channel average RMS read-out
	size.offset (kXinc, 0);
	rightAverageRMSDisplay = new CParamDisplay (size);
	rightAverageRMSDisplay->setHoriAlign(kCenterText);
	rightAverageRMSDisplay->setFont(kValueDisplayFont);
	rightAverageRMSDisplay->setFontColor(kReadoutTextColor);
	rightAverageRMSDisplay->setBackColor(kReadoutBoxColor);
	rightAverageRMSDisplay->setFrameColor(kReadoutFrameColor);
	rightAverageRMSDisplay->setValue(0.0f);
	rightAverageRMSDisplay->setStringConvert( dBdisplayConvert, &((RMSbuddy*)effect)->rightAverageRMS );
	frame->addView(rightAverageRMSDisplay);

	// left channel continual RMS read-out
	size.offset (-kXinc, kYinc);
	leftContinualRMSDisplay = new CParamDisplay (size);
	leftContinualRMSDisplay->setHoriAlign(kCenterText);
	leftContinualRMSDisplay->setFont(kValueDisplayFont);
	leftContinualRMSDisplay->setFontColor(kReadoutTextColor);
	leftContinualRMSDisplay->setBackColor(kReadoutBoxColor);
	leftContinualRMSDisplay->setFrameColor(kReadoutFrameColor);
	leftContinualRMSDisplay->setValue(0.0f);
	leftContinualRMSDisplay->setStringConvert( dBdisplayConvert, &theLeftContinualRMS );
	frame->addView(leftContinualRMSDisplay);

	// right channel continual RMS read-out
	size.offset (kXinc, 0);
	rightContinualRMSDisplay = new CParamDisplay (size);
	rightContinualRMSDisplay->setHoriAlign(kCenterText);
	rightContinualRMSDisplay->setFont(kValueDisplayFont);
	rightContinualRMSDisplay->setFontColor(kReadoutTextColor);
	rightContinualRMSDisplay->setBackColor(kReadoutBoxColor);
	rightContinualRMSDisplay->setFrameColor(kReadoutFrameColor);
	rightContinualRMSDisplay->setValue(0.0f);
	rightContinualRMSDisplay->setStringConvert( dBdisplayConvert, &theRightContinualRMS );
	frame->addView(rightContinualRMSDisplay);

	// left channel absolute peak read-out
	size.offset (-kXinc, kYinc);
	leftAbsolutePeakDisplay = new CParamDisplay (size);
	leftAbsolutePeakDisplay->setHoriAlign(kCenterText);
	leftAbsolutePeakDisplay->setFont(kValueDisplayFont);
	leftAbsolutePeakDisplay->setFontColor(kReadoutTextColor);
	leftAbsolutePeakDisplay->setBackColor(kReadoutBoxColor);
	leftAbsolutePeakDisplay->setFrameColor(kReadoutFrameColor);
	leftAbsolutePeakDisplay->setValue(0.0f);
	leftAbsolutePeakDisplay->setStringConvert( dBdisplayConvert, &((RMSbuddy*)effect)->leftAbsolutePeak );
	frame->addView(leftAbsolutePeakDisplay);

	// right channel absolute peak read-out
	size.offset (kXinc, 0);
	rightAbsolutePeakDisplay = new CParamDisplay (size);
	rightAbsolutePeakDisplay->setHoriAlign(kCenterText);
	rightAbsolutePeakDisplay->setFont(kValueDisplayFont);
	rightAbsolutePeakDisplay->setFontColor(kReadoutTextColor);
	rightAbsolutePeakDisplay->setBackColor(kReadoutBoxColor);
	rightAbsolutePeakDisplay->setFrameColor(kReadoutFrameColor);
	rightAbsolutePeakDisplay->setValue(0.0f);
	rightAbsolutePeakDisplay->setStringConvert( dBdisplayConvert, &((RMSbuddy*)effect)->rightAbsolutePeak );
	frame->addView(rightAbsolutePeakDisplay);

	// left channel continual peak read-out
	size.offset (-kXinc, kYinc);
	leftContinualPeakDisplay = new CParamDisplay (size);
	leftContinualPeakDisplay->setHoriAlign(kCenterText);
	leftContinualPeakDisplay->setFont(kValueDisplayFont);
	leftContinualPeakDisplay->setFontColor(kReadoutTextColor);
	leftContinualPeakDisplay->setBackColor(kReadoutBoxColor);
	leftContinualPeakDisplay->setFrameColor(kReadoutFrameColor);
	leftContinualPeakDisplay->setValue(0.0f);
	leftContinualPeakDisplay->setStringConvert( dBdisplayConvert, &theLeftContinualPeak );
	frame->addView(leftContinualPeakDisplay);

	// right channel continual peak read-out
	size.offset (kXinc, 0);
	rightContinualPeakDisplay = new CParamDisplay (size);
	rightContinualPeakDisplay->setHoriAlign(kCenterText);
	rightContinualPeakDisplay->setFont(kValueDisplayFont);
	rightContinualPeakDisplay->setFontColor(kReadoutTextColor);
	rightContinualPeakDisplay->setBackColor(kReadoutBoxColor);
	rightContinualPeakDisplay->setFrameColor(kReadoutFrameColor);
	rightContinualPeakDisplay->setValue(0.0f);
	rightContinualPeakDisplay->setStringConvert( dBdisplayConvert, &theRightContinualPeak );
	frame->addView(rightContinualPeakDisplay);

	// the words "average RMS"
	size (kRMSlabelX, kRMSlabelY, kRMSlabelX + kRMSlabelWidth, kRMSlabelY + kRMSlabelHeight);
	averageRMSDisplay = new CParamDisplay (size);
	averageRMSDisplay->setHoriAlign(kRightText);
	averageRMSDisplay->setFont(kLabelDisplayFont);
	averageRMSDisplay->setFontColor(kLabelColor);
	averageRMSDisplay->setBackColor(kTransparentCColor);
	averageRMSDisplay->setFrameColor(kTransparentCColor);
	averageRMSDisplay->setTransparency(true);
	averageRMSDisplay->setValue(0.0f);
	averageRMSDisplay->setStringConvert(staticTextDisplayConvert, (void*)"average  RMS:");
	frame->addView(averageRMSDisplay);

	// the words "continual RMS"
	size.offset (0, kYinc);
	continualRMSDisplay = new CParamDisplay (size);
	continualRMSDisplay->setHoriAlign(kRightText);
	continualRMSDisplay->setFont(kLabelDisplayFont);
	continualRMSDisplay->setFontColor(kLabelColor);
	continualRMSDisplay->setBackColor(kTransparentCColor);
	continualRMSDisplay->setFrameColor(kTransparentCColor);
	continualRMSDisplay->setTransparency(true);
	continualRMSDisplay->setValue(0.0f);
	continualRMSDisplay->setStringConvert(staticTextDisplayConvert, (void*)"continual  RMS:");
	frame->addView(continualRMSDisplay);

	// the words "absolute peak"
	size.offset (0, kYinc);
	absolutePeakDisplay = new CParamDisplay (size);
	absolutePeakDisplay->setHoriAlign(kRightText);
	absolutePeakDisplay->setFont(kLabelDisplayFont);
	absolutePeakDisplay->setFontColor(kLabelColor);
	absolutePeakDisplay->setBackColor(kTransparentCColor);
	absolutePeakDisplay->setFrameColor(kTransparentCColor);
	absolutePeakDisplay->setTransparency(true);
	absolutePeakDisplay->setValue(0.0f);
	absolutePeakDisplay->setStringConvert(staticTextDisplayConvert, (void*)"absolute  peak:");
	frame->addView(absolutePeakDisplay);

	// the words "continual peak"
	size.offset (0, kYinc);
	continualPeakDisplay = new CParamDisplay (size);
	continualPeakDisplay->setHoriAlign(kRightText);
	continualPeakDisplay->setFont(kLabelDisplayFont);
	continualPeakDisplay->setFontColor(kLabelColor);
	continualPeakDisplay->setBackColor(kTransparentCColor);
	continualPeakDisplay->setFrameColor(kTransparentCColor);
	continualPeakDisplay->setTransparency(true);
	continualPeakDisplay->setValue(0.0f);
	continualPeakDisplay->setStringConvert(staticTextDisplayConvert, (void*)"continual  peak:");
	frame->addView(continualPeakDisplay);

	// the word "left"
	size (kRLlabelX, kRLlabelY, kRLlabelX + kRLlabelWidth, kRLlabelY + kRLlabelHeight);
	leftDisplay = new CParamDisplay (size);
	leftDisplay->setHoriAlign(kCenterText);
	leftDisplay->setFont(kLabelDisplayFont);
	leftDisplay->setFontColor(kLabelColor);
	leftDisplay->setBackColor(kTransparentCColor);
	leftDisplay->setFrameColor(kTransparentCColor);
	leftDisplay->setTransparency(true);
	leftDisplay->setValue(0.0f);
	leftDisplay->setStringConvert(staticTextDisplayConvert, (void*)" left");
	frame->addView(leftDisplay);

	// the word "right"
	size.offset (kXinc, 0);
	rightDisplay = new CParamDisplay (size);
	rightDisplay->setHoriAlign(kCenterText);
	rightDisplay->setFont(kLabelDisplayFont);
	rightDisplay->setFontColor(kLabelColor);
	rightDisplay->setBackColor(kTransparentCColor);
	rightDisplay->setFrameColor(kTransparentCColor);
	rightDisplay->setTransparency(true);
	rightDisplay->setValue(0.0f);
	rightDisplay->setStringConvert(staticTextDisplayConvert, (void*)" right");
	frame->addView(rightDisplay);


	//--initialize the button-----------------------------------------------

	// reset continual RMS button
	// size stores left, top, right, & bottom positions
	size (kButtonX, kButtonY, kButtonX + gResetButton->getWidth(), kButtonY + (gResetButton->getHeight())/2);
	resetRMSbutton = new CKickButton (size, this, kResetRmsTag, (gResetButton->getHeight())/2, gResetButton, point);
	resetRMSbutton->setValue(0.0f);
	frame->addView(resetRMSbutton);

	// reset peak button
	size.offset (0, kYinc*2);
	resetPeakButton = new CKickButton (size, this, kResetPeakTag, (gResetButton->getHeight())/2, gResetButton, point);
	resetPeakButton->setValue(0.0f);
	frame->addView(resetPeakButton);


	return 1;
}

//-----------------------------------------------------------------------------
void RMSbuddyEditor::close()
{
	if (frame)
		delete frame;
	frame = 0;

	// free some bitmaps
	if (gResetButton)
		gResetButton->forget();
	gResetButton = 0;
}

//-----------------------------------------------------------------------------
void RMSbuddyEditor::valueChanged(CDrawContext* context, CControl* control)
{
  long tag = control->getTag();

	switch (tag)
	{
		case kResetRmsTag:
			if (control->getValue() >= 0.5f)
			{
				((RMSbuddy*)effect)->totalSamples = 0;
				((RMSbuddy*)effect)->totalSquaredCollection1 = 0.0f;
				((RMSbuddy*)effect)->totalSquaredCollection2 = 0.0f;
				((RMSbuddy*)effect)->leftAverageRMS = 0.0f;
				((RMSbuddy*)effect)->rightAverageRMS = 0.0f;
				if (leftAverageRMSDisplay)
					leftAverageRMSDisplay->setDirty();
				if (rightAverageRMSDisplay)
					rightAverageRMSDisplay->setDirty();
			}
			break;

		case kResetPeakTag:
			if (control->getValue() >= 0.5f)
			{
				((RMSbuddy*)effect)->leftAbsolutePeak = 0.0f;
				((RMSbuddy*)effect)->rightAbsolutePeak = 0.0f;
				if (leftAbsolutePeakDisplay)
					leftAbsolutePeakDisplay->setDirty();
				if (rightAbsolutePeakDisplay)
					rightAbsolutePeakDisplay->setDirty();
			}
			break;

		default:
			break;
	}

	control->update(context);
}

//-----------------------------------------------------------------------------
// the idle routine here is for updating all of the RMS value displays

void RMSbuddyEditor::idle()
{
	if (frame != NULL)
	{
		if ( frame->isOpen() )
		{
			RMSbuddy *buddy = (RMSbuddy*)effect;
			// refresh the displays if it's been long enough
			if ( buddy->GUIsamplesCounter > WAIT_THIS_MANY_SAMPLES_TO_REFRESH )
			{
				// update all of the display values for proper value display
				theLeftContinualRMS = sqrtf( buddy->GUIleftContinualRMS / (float)(buddy->GUIsamplesCounter) );
				theRightContinualRMS = sqrtf( buddy->GUIrightContinualRMS / (float)(buddy->GUIsamplesCounter) );
				theLeftContinualPeak = buddy->GUIleftContinualPeak;
				theRightContinualPeak = buddy->GUIrightContinualPeak;

				// start the counting/accumulating cycle anew
				buddy->resetGUIcounters();

				// force all of the value displays to refresh & display the new values
				if (leftAverageRMSDisplay)
					leftAverageRMSDisplay->setDirty();
				if (rightAverageRMSDisplay)
					rightAverageRMSDisplay->setDirty();
				if (leftContinualRMSDisplay)
					leftContinualRMSDisplay->setDirty();
				if (rightContinualRMSDisplay)
					rightContinualRMSDisplay->setDirty();
				if (leftAbsolutePeakDisplay)
					leftAbsolutePeakDisplay->setDirty();
				if (rightAbsolutePeakDisplay)
					rightAbsolutePeakDisplay->setDirty();
				if (leftContinualPeakDisplay)
					leftContinualPeakDisplay->setDirty();
				if (rightContinualPeakDisplay)
					rightContinualPeakDisplay->setDirty();

				// indicate that changed controls need to be redrawn
				postUpdate();
			}
		}
	}

	// this is called so that idle() actually happens
	AEffGUIEditor::idle();
}
