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

void leapDisplayProc(float value, char * outText, void *);
void leapDisplayProc(float value, char * outText, void *)
{
	sprintf(outText, "%ld sample", (long)value);
	if (abs((long)value) > 1)
		strcat(outText, "s");
}

void amountDisplayProc(float value, char * outText, void *);
void amountDisplayProc(float value, char * outText, void *)
{
//	sprintf(outText, "%.3f", value);
	sprintf(outText, "%ld%%", (long)(value*10.0f));
}


//-----------------------------------------------------------------------------
class PolarizerSlider : public DGSlider
{
public:
	PolarizerSlider(DfxGuiEditor * inOwnerEditor, AudioUnitParameterID inParamID, DGRect * inRegion, 
					DfxGuiSliderAxis inOrientation, DGImage * inHandleImage, DGImage * inBackgroundImage)
	:	DGSlider(inOwnerEditor, inParamID, inRegion, inOrientation, inHandleImage, inBackgroundImage)
	{
		DGRect * fb = getBounds();
		setForeBounds(fb->x, fb->y + kSliderFrameThickness, fb->w, fb->h - (kSliderFrameThickness*2));
		setMouseOffset(0);
	}
	virtual void draw(CGContextRef inContext, long inPortHeight)
	{
		if (backgroundImage != NULL)
			backgroundImage->draw(getBounds(), inContext, inPortHeight);
		else
			getDfxGuiEditor()->DrawBackground(inContext, inPortHeight);

		ControlRef carbonControl = getCarbonControl();
		SInt32 max = GetControl32BitMaximum(carbonControl);
		SInt32 min = GetControl32BitMinimum(carbonControl);
		SInt32 val = GetControl32BitValue(carbonControl);
		float valNorm = ((max-min) == 0) ? 0.0f : (float)(val-min) / (float)(max-min);

		CGImageRef handleCGImage = NULL;
		if (handleImage != NULL)
			handleCGImage = handleImage->getCGImage();
		if (handleCGImage != NULL)
		{
			long yoff = (long)round( (float)(handleImage->getHeight()) * (1.0f-valNorm) ) + kSliderFrameThickness;
			handleImage->draw(getBounds(), inContext, inPortHeight, -kSliderFrameThickness, -yoff);

			CGRect bottomBorderRect = getBounds()->convertToCGRect(inPortHeight);
			bottomBorderRect.size.height = (float)kSliderFrameThickness;
#ifdef FLIP_CG_COORDINATES
			bottomBorderRect.origin.y += (float) (getBounds()->h - kSliderFrameThickness);
#endif
			bottomBorderRect.size.width = (float) handleImage->getWidth();
			bottomBorderRect.origin.x += (float)kSliderFrameThickness;
			CGContextSetRGBFillColor(inContext, 0.0f, 0.0f, 0.0f, 1.0f);
			CGContextFillRect(inContext, bottomBorderRect);
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
long PolarizerEditor::open(float inXOffset, float inYOffset)
{
	// load some graphics

	// background image
	DGImage * gBackground = new DGImage("polarizer-background.png", this);
	SetBackgroundImage(gBackground);

	DGImage * gSliderHandle = new DGImage("slider-handle.png", this);
	DGImage * gSliderBackground = new DGImage("slider-background.png", this);

	DGImage * gImplodeButton = new DGImage("implode-button.png", this);
	DGImage * gDestroyFXlinkButton = new DGImage("destroy-fx-link.png", this);


	DGRect pos;

	//--create the sliders---------------------------------
	PolarizerSlider * slider;

	// leap size
	pos.set(kSliderX, kSliderY, gSliderBackground->getWidth(), gSliderBackground->getHeight());
	slider = new PolarizerSlider(this, kSkip, &pos, kDGSliderAxis_vertical, gSliderHandle, gSliderBackground);

	// polarization amount
	pos.offset(kSliderInc, 0);
	slider = new PolarizerSlider(this, kAmount, &pos, kDGSliderAxis_vertical, gSliderHandle, gSliderBackground);


	//--create the displays---------------------------------------------
	DGTextDisplay * display;

	// leap size read-out
	pos.set(kDisplayX, kDisplayY, kDisplayWidth, kDisplayHeight);
	display = new DGTextDisplay(this, kSkip, &pos, leapDisplayProc, NULL, NULL, kValueTextSize, 
								kDGTextAlign_center, kBlackDGColor, kValueTextFont);

	// polarization amount read-out
	pos.offset(kSliderInc, 0);
	display = new DGTextDisplay(this, kAmount, &pos, amountDisplayProc, NULL, NULL, kValueTextSize, 
								kDGTextAlign_center, kBlackDGColor, kValueTextFont);


	//--create the buttons----------------------------------------------

	// IMPLODE
	pos.set(kImplodeButtonX, kImplodeButtonY, gImplodeButton->getWidth() / 2, gImplodeButton->getHeight() / 2);
	DGButton * implodeButton = new DGButton(this, kImplode, &pos, gImplodeButton, 2, kDGButtonType_incbutton, true);

	// Destroy FX web page link
	pos.set(kDestroyFXlinkX, kDestroyFXlinkY, gDestroyFXlinkButton->getWidth(), gDestroyFXlinkButton->getHeight()/2);
	DGWebLink * dfxLinkButton = new DGWebLink(this, &pos, gDestroyFXlinkButton, DESTROYFX_URL);


	return noErr;
}
