#include "polarizereditor.hpp"
#include "polarizer.hpp"

#include "dfxguislider.h"
#include "dfxguidisplay.h"
#include "dfxguibutton.h"


//-----------------------------------------------------------------------------
enum {
	// positions
	kSliderFrameThickness = 4,
	kSliderX = 67 - kSliderFrameThickness,
	kSliderY = 70 - kSliderFrameThickness,
	kSliderInc = 118,

	kDisplayX = kSliderX - 32 + kSliderFrameThickness,
	kDisplayY = kSliderY + 221,
	kDisplayWidth = 110,
	kDisplayHeight = 17,

	kImplodeButtonX = kSliderX + (kSliderInc*2),
	kImplodeButtonY = kSliderY,

	kDestroyFXlinkX = 40,
	kDestroyFXlinkY = 322
};


const char * kValueTextFont = "Boring Boron";
//const float kValueTextSize = 20.0f;
const float kValueTextSize = 16.8f;



//-----------------------------------------------------------------------------
// parameter value string display conversion functions

void leapDisplayProc(float value, char *outText, void*);
void leapDisplayProc(float value, char *outText, void*)
{
	sprintf(outText, "%ld sample", (long)value);
	if (abs((long)value) > 1)
		strcat(outText, "s");
}

void amountDisplayProc(float value, char *outText, void*);
void amountDisplayProc(float value, char *outText, void*)
{
//	sprintf(outText, "%.3f", value);
	sprintf(outText, "%ld%%", (long)(value*10.0f));
}


//-----------------------------------------------------------------------------
class PolarizerSlider : public DGSlider
{
public:
	PolarizerSlider(DfxGuiEditor *inOwnerEditor, AudioUnitParameterID inParamID, DGRect *inRegion, 
					DfxGuiSliderStyle inOrientation, DGImage *inForeGround, DGImage *inBackground)
	:	DGSlider(inOwnerEditor, inParamID, inRegion, inOrientation, inForeGround, inBackground)
	{
		DGRect *fb = getBounds();
		setForeBounds(fb->x, fb->y + kSliderFrameThickness, fb->w, fb->h - (kSliderFrameThickness*2));
		setMouseOffset(0);
	}
	virtual void draw(CGContextRef inContext, UInt32 inPortHeight)
	{
		ControlRef carbonControl = getCarbonControl();
		SInt32 max = GetControl32BitMaximum(carbonControl);
		SInt32 min = GetControl32BitMinimum(carbonControl);
		SInt32 val = GetControl32BitValue(carbonControl);

		CGRect bounds;
		getBounds()->copyToCGRect(&bounds, inPortHeight);
		CGImageRef theBack = NULL;
		if (BackGround != NULL)
			theBack = BackGround->getCGImage();
		if (theBack != NULL)
			CGContextDrawImage(inContext, bounds, theBack);
		else
			getDfxGuiEditor()->DrawBackground(inContext, inPortHeight);

		CGImageRef theFore = NULL;
		if (ForeGround != NULL)
			theFore = ForeGround->getCGImage();
		if (theFore != NULL)
		{
			float valNorm = ((max-min) == 0) ? 0.0f : (float)(val-min) / (float)(max-min);
			bounds.size.height = (float) ForeGround->getHeight();
			bounds.size.width = (float) ForeGround->getWidth();
			bounds.origin.y += round(bounds.size.height * valNorm) - bounds.size.height + (float)kSliderFrameThickness;
			bounds.origin.x += (float)kSliderFrameThickness;
			CGContextDrawImage(inContext, bounds, theFore);

			getBounds()->copyToCGRect(&bounds, inPortHeight);
			bounds.size.height = (float)kSliderFrameThickness;
			bounds.size.width = (float) ForeGround->getWidth();
			bounds.origin.x += (float)kSliderFrameThickness;
			CGContextSetRGBFillColor(inContext, 0.0f, 0.0f, 0.0f, 1.0f);
			CGContextFillRect(inContext, bounds);
		}
	}
};


//-----------------------------------------------------------------------------
COMPONENT_ENTRY(PolarizerEditor);

//-----------------------------------------------------------------------------
PolarizerEditor::PolarizerEditor(AudioUnitCarbonView inInstance)
:	DfxGuiEditor(inInstance)
{
}

//-----------------------------------------------------------------------------
OSStatus PolarizerEditor::open(float inXOffset, float inYOffset)
{
	// load some graphics

	// background image
	DGImage *gBackground = new DGImage("polarizer-background.png");
	addImage(gBackground);
	SetBackgroundImage(gBackground);
	//
	DGImage *gSliderHandle = new DGImage("slider-handle.png");
	addImage(gSliderHandle);
	DGImage *gSliderBackground = new DGImage("slider-background.png");
	addImage(gSliderBackground);
	//
	DGImage *gImplodeButton = new DGImage("implode-button.png");
	addImage(gImplodeButton);
	DGImage *gDestroyFXlinkButton = new DGImage("destroy-fx-link.png");
	addImage(gDestroyFXlinkButton);


	DGRect pos;

	//--create the sliders---------------------------------
	PolarizerSlider *slider;

	// leap size
	pos.set(kSliderX, kSliderY, gSliderBackground->getWidth(), gSliderBackground->getHeight());
	slider = new PolarizerSlider(this, kSkip, &pos, kDGSliderStyle_vertical, gSliderHandle, gSliderBackground);
	addControl(slider);

	// polarization amount
	pos.offset(kSliderInc, 0);
	slider = new PolarizerSlider(this, kAmount, &pos, kDGSliderStyle_vertical, gSliderHandle, gSliderBackground);
	addControl(slider);


	//--create the displays---------------------------------------------
	DGTextDisplay *display;

	// leap size read-out
	pos.set(kDisplayX, kDisplayY, kDisplayWidth, kDisplayHeight);
	display = new DGTextDisplay(this, kSkip, &pos, leapDisplayProc, NULL, NULL, kValueTextFont);
	display->setFontSize(kValueTextSize);
	display->setTextAlignmentStyle(kDGTextAlign_center);
	display->setFontColor(kBlackDGColor);
	addControl(display);

	// polarization amount read-out
	pos.offset(kSliderInc, 0);
	display = new DGTextDisplay(this, kAmount, &pos, amountDisplayProc, NULL, NULL, kValueTextFont);
	display->setFontSize(kValueTextSize);
	display->setTextAlignmentStyle(kDGTextAlign_center);
	display->setFontColor(kBlackDGColor);
	addControl(display);


	//--create the buttons----------------------------------------------

	// IMPLODE
	pos.set(kImplodeButtonX, kImplodeButtonY, gImplodeButton->getWidth() / 2, gImplodeButton->getHeight() / 2);
	DGButton *implodeButton = new DGButton(this, kImplode, &pos, gImplodeButton, 2, kDGButtonType_incbutton, true);
	addControl(implodeButton);

	// Destroy FX web page link
	pos.set(kDestroyFXlinkX, kDestroyFXlinkY, gDestroyFXlinkButton->getWidth(), gDestroyFXlinkButton->getHeight()/2);
	DGWebLink *dfxLinkButton = new DGWebLink(this, &pos, gDestroyFXlinkButton, DESTROYFX_URL);
	addControl(dfxLinkButton);


	return noErr;
}
