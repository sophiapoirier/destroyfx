#include "eqsynceditor.hpp"
#include "eqsync.hpp"

#include "dfxguislider.h"
#include "dfxguidisplay.h"
#include "dfxguibutton.h"


//-----------------------------------------------------------------------------
enum {
	// positions
	kWideFaderX = 138 - 2,
	kWideFaderY = 68 - 7,
	kWideFaderInc = 40,

	kTallFaderX = 138 - 7,
	kTallFaderY = 196 - 2,
	kTallFaderInc = 48,

	kDisplayX = 348,
	kDisplayY = kWideFaderY + 2,
	kDisplayWidth = 81,
	kDisplayHeight = 12,
	
	kDestroyFXlinkX = 158,
	kDestroyFXlinkY = 12
};


const char * kValueTextFont = "Lucida Grande";
const float kValueTextSize = 13.0f;



//-----------------------------------------------------------------------------
// parameter value string display conversion functions

void tempoRateDisplayProc(float value, char *outText, void *editor);
void tempoRateDisplayProc(float value, char *outText, void *editor)
{
	((DfxGuiEditor*)editor)->getparametervaluestring(kRate_sync, outText);
}

void smoothDisplayProc(float value, char *outText, void*);
void smoothDisplayProc(float value, char *outText, void*)
{
	sprintf(outText, "%.1f%%", value);
}

void tempoDisplayProc(float value, char *outText, void*);
void tempoDisplayProc(float value, char *outText, void*)
{
	sprintf(outText, "%.3f", value);
}



//-----------------------------------------------------------------------------
class EQSyncSlider : public DGSlider
{
public:
	EQSyncSlider(DfxGuiEditor *inOwnerEditor, AudioUnitParameterID inParamID, DGRect *inRegion, 
					DfxGuiSliderStyle inOrientation, DGGraphic *inHandle, DGGraphic *inHandleClicked, DGGraphic *inBackground)
	:	DGSlider(inOwnerEditor, inParamID, inRegion, inOrientation, inHandle, inBackground), 
		regularHandle(inHandle), clickedHandle(inHandleClicked)
	{
	}
	virtual void draw(CGContextRef inContext, UInt32 inPortHeight)
	{
//		getDfxGuiEditor()->DrawBackground(inContext, inPortHeight);
		DGSlider::draw(inContext, inPortHeight);
	}
	virtual void mouseDown(Point inPos, bool with_option, bool with_shift)
	{
		ForeGround = clickedHandle;	// switch to the click-styled handle
		DGSlider::mouseDown(inPos, with_option, with_shift);
		lastX = inPos.h;
		lastY = inPos.v;
		lastXchange = lastYchange = 0;
	}
#if 0
	virtual void mouseTrack(Point inPos, bool a, bool b)
	{
		if (orientation == kDGSliderStyle_vertical)
		{
			long xchange = inPos.h - lastX + lastXchange;
			long ychange = inPos.v - lastY + lastYchange;
			Rect cbounds;
			GetControlBounds(getCarbonControl(), &cbounds);
			MoveControl(getCarbonControl(), cbounds.left + xchange, cbounds.top + ychange);
			getBounds()->offset(xchange, ychange);
			getForeBounds()->offset(xchange, ychange);
			redraw();
			lastX = inPos.h;
			lastY = inPos.v;
			lastXchange = xchange;
			lastYchange = ychange;
		}
		else
			DGSlider::mouseTrack(inPos, a, b);
	}
#endif
	virtual void mouseUp(Point inPos, bool with_option, bool with_shift)
	{
		ForeGround = regularHandle;	// switch back to the non-click-styled handle
		DGSlider::mouseUp(inPos, with_option, with_shift);
		redraw();	// make sure that the change in slider handle is reflected
	}
private:
	DGGraphic * regularHandle;
	DGGraphic * clickedHandle;
	long lastX, lastY;
	long lastXchange, lastYchange;
};


//-----------------------------------------------------------------------------
class EQSyncWebLink : public DGControl
{
public:
	EQSyncWebLink(DfxGuiEditor *inOwnerEditor, DGRect *inRegion, DGGraphic *inImage)
	:	DGControl(inOwnerEditor, inRegion, 1.0f), 
		buttonImage(inImage)
	{
		setType(kDfxGuiType_button);
		setContinuousControl(false);
	}

	virtual void draw(CGContextRef inContext, UInt32 inPortHeight)
	{
		CGImageRef theButton = (buttonImage == NULL) ? NULL : buttonImage->getCGImage();
		if (theButton != NULL)
		{
			CGRect bounds = getBounds()->convertToCGRect(inPortHeight);
			bounds.size.height = buttonImage->getHeight();
			if (GetControl32BitValue(getCarbonControl()) == 0)
				bounds.origin.y -= (float) (buttonImage->getHeight() / 2);
			CGContextDrawImage(inContext, bounds, theButton);
		}
	}
	virtual void mouseDown(Point inPos, bool, bool)
	{
		if ( inPos.h > ((getBounds()->w / 2) - 6) )
			SetControl32BitValue(getCarbonControl(), 1);
		lastX = inPos.h;
		lastY = inPos.v;
		lastXchange = lastYchange = 0;
	}
#if 0
	virtual void mouseTrack(Point inPos, bool, bool)
	{
		long xchange = inPos.h - lastX + lastXchange;
		long ychange = inPos.v - lastY + lastYchange;
		Rect cbounds;
		GetControlBounds(getCarbonControl(), &cbounds);
		MoveControl(getCarbonControl(), cbounds.left + xchange, cbounds.top + ychange);
		getBounds()->offset(xchange, ychange);
		redraw();
		lastX = inPos.h;
		lastY = inPos.v;
		lastXchange = xchange;
		lastYchange = ychange;
	}
#endif
	virtual void mouseUp(Point inPos, bool, bool)
	{
		if (GetControl32BitValue(getCarbonControl()) != 0)
		{
			launch_url(DESTROYFX_URL);
			SetControl32BitValue(getCarbonControl(), 0);
		}
	}

private:
	DGGraphic * buttonImage;
	long lastX, lastY;
	long lastXchange, lastYchange;
};



//-----------------------------------------------------------------------------
COMPONENT_ENTRY(EQSyncEditor);

//-----------------------------------------------------------------------------
EQSyncEditor::EQSyncEditor(AudioUnitCarbonView inInstance)
:	DfxGuiEditor(inInstance)
{
}

//-----------------------------------------------------------------------------
OSStatus EQSyncEditor::open(float inXOffset, float inYOffset)
{
	// load some graphics

	// background image
	DGGraphic *gBackground = new DGGraphic("eq-sync-background.png");
	addImage(gBackground);
	SetBackgroundImage(gBackground);
	//
	DGGraphic *gHorizontalSliderBackground = new DGGraphic("horizontal-slider-background.png");
	addImage(gHorizontalSliderBackground);
	DGGraphic *gVerticalSliderBackground = new DGGraphic("vertical-slider-background.png");
	addImage(gVerticalSliderBackground);
	DGGraphic *gSliderHandle = new DGGraphic("slider-handle.png");
	addImage(gSliderHandle);
	DGGraphic *gSliderHandleClicked = new DGGraphic("slider-handle-clicked.png");
	addImage(gSliderHandleClicked);
	//
//	DGGraphic *gHostSyncButton = new DGGraphic("host-sync-button.png");
//	addImage(gHostSyncButton);
	DGGraphic *gDestroyFXlinkTab = new DGGraphic("destroy-fx-link-tab.png");
	addImage(gDestroyFXlinkTab);


	DGRect pos;

	for (long i=kRate_sync; i <= kTempo; i++)
	{
		// create the horizontal sliders
		pos.set(kWideFaderX, kWideFaderY + (kWideFaderInc * i), gHorizontalSliderBackground->getWidth(), gHorizontalSliderBackground->getHeight());
		EQSyncSlider *slider = new EQSyncSlider(this, i, &pos, kDGSliderStyle_horizontal, gSliderHandle, gSliderHandleClicked, gHorizontalSliderBackground);
		addControl(slider);

		// create the displays
		displayTextProcedure textproc = NULL;
		if (i == kRate_sync)
			textproc = tempoRateDisplayProc;
		else if (i == kSmooth)
			textproc = smoothDisplayProc;
		else if (i == kTempo)
			textproc = tempoDisplayProc;
		pos.set(kDisplayX, kDisplayY + (kWideFaderInc * i), kDisplayWidth, kDisplayHeight);
		DGTextDisplay *display = new DGTextDisplay(this, i, &pos, textproc, this, NULL, kValueTextFont);
		display->setFontSize(kValueTextSize);
		display->setTextAlignmentStyle(kDGTextAlign_left);
		display->setFontColor(kBlackDGColor);
		addControl(display);
	}

	// create the vertical sliders
	for (long i=ka0; i <= kb2; i++)
	{
		pos.set(kTallFaderX + (kTallFaderInc * (i-ka0)), kTallFaderY, gVerticalSliderBackground->getWidth(), gVerticalSliderBackground->getHeight());
		EQSyncSlider *slider = new EQSyncSlider(this, i, &pos, kDGSliderStyle_vertical, gSliderHandle, gSliderHandleClicked, gVerticalSliderBackground);
		addControl(slider);
	}


	// create the Destroy FX web page link tab
	pos.set(kDestroyFXlinkX, kDestroyFXlinkY, gDestroyFXlinkTab->getWidth(), gDestroyFXlinkTab->getHeight()/2);
	EQSyncWebLink *dfxLinkButton = new EQSyncWebLink(this, &pos, gDestroyFXlinkTab);
	addControl(dfxLinkButton);


	return noErr;
}
