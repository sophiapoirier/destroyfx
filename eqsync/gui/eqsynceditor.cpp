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

void tempoRateDisplayProc(float value, char * outText, void * editor);
void tempoRateDisplayProc(float value, char * outText, void * editor)
{
	((DfxGuiEditor*)editor)->getparametervaluestring(kRate_sync, outText);
}

void smoothDisplayProc(float value, char * outText, void *);
void smoothDisplayProc(float value, char * outText, void *)
{
	sprintf(outText, "%.1f%%", value);
}

void tempoDisplayProc(float value, char * outText, void *);
void tempoDisplayProc(float value, char * outText, void *)
{
	sprintf(outText, "%.3f", value);
}



//-----------------------------------------------------------------------------
class EQSyncSlider : public DGSlider
{
public:
	EQSyncSlider(DfxGuiEditor * inOwnerEditor, AudioUnitParameterID inParamID, DGRect * inRegion, 
					DfxGuiSliderAxis inOrientation, DGImage * inHandle, DGImage * inHandleClicked, DGImage * inBackground)
	:	DGSlider(inOwnerEditor, inParamID, inRegion, inOrientation, inHandle, inBackground), 
		regularHandle(inHandle), clickedHandle(inHandleClicked)
	{
	}
	virtual void draw(CGContextRef inContext, long inPortHeight)
	{
		getDfxGuiEditor()->DrawBackground(inContext, inPortHeight);
		DGSlider::draw(inContext, inPortHeight);
	}
	virtual void mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, unsigned long inKeyModifiers)
	{
		handleImage = clickedHandle;	// switch to the click-styled handle
		DGSlider::mouseDown(inXpos, inYpos, inMouseButtons, inKeyModifiers);
		lastPX = inXpos;
		lastPY = inYpos;
		lastXchange = lastYchange = 0;
	}
#if 0
	virtual void mouseTrack(float inXpos, float inYpos, unsigned long inMouseButtons, unsigned long inKeyModifiers)
	{
		if (orientation == kDGSliderAxis_vertical)
		{
			long xchange = (long)(inXpos - lastPX) + lastXchange;
			long ychange = (long)(inYpos - lastPY) + lastYchange;
			Rect cbounds;
			GetControlBounds(getCarbonControl(), &cbounds);
			MoveControl(getCarbonControl(), cbounds.left + xchange, cbounds.top + ychange);
			getBounds()->offset(xchange, ychange);
			getForeBounds()->offset(xchange, ychange);
			redraw();
			lastPX = inXpos;
			lastPY = inYpos;
			lastXchange = xchange;
			lastYchange = ychange;
		}
		else
			DGSlider::mouseTrack(inXpos, inYpos, inMouseButtons, inKeyModifiers);
	}
#endif
	virtual void mouseUp(float inXpos, float inYpos, unsigned long inKeyModifiers)
	{
		handleImage = regularHandle;	// switch back to the non-click-styled handle
		DGSlider::mouseUp(inXpos, inYpos, inKeyModifiers);
		redraw();	// make sure that the change in slider handle is reflected
	}
private:
	DGImage * regularHandle;
	DGImage * clickedHandle;
	float lastPX, lastPY;
	long lastXchange, lastYchange;
};


//-----------------------------------------------------------------------------
class EQSyncWebLink : public DGControl
{
public:
	EQSyncWebLink(DfxGuiEditor * inOwnerEditor, DGRect * inRegion, DGImage * inImage)
	:	DGControl(inOwnerEditor, inRegion, 1.0f), 
		buttonImage(inImage)
	{
		setControlContinuous(false);
	}

	virtual void draw(CGContextRef inContext, long inPortHeight)
	{
		if (buttonImage != NULL)
		{
			long yoff = (GetControl32BitValue(getCarbonControl()) == 0) ? 0 : (buttonImage->getHeight() / 2);
			buttonImage->draw(getBounds(), inContext, inPortHeight, 0, yoff);
		}
	}
	virtual void mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, unsigned long inKeyModifiers)
	{
		if ( (long)inXpos > ((getBounds()->w / 2) - 6) )
			SetControl32BitValue(getCarbonControl(), 1);
		lastX = inXpos;
		lastY = inYpos;
		lastXchange = lastYchange = 0;
	}
#if 0
	virtual void mouseTrack(float inXpos, float inYpos, unsigned long inMouseButtons, unsigned long inKeyModifiers)
	{
		long xchange = (long)(inXpos - lastX) + lastXchange;
		long ychange = (long)(inYpos - lastY) + lastYchange;
		Rect cbounds;
		GetControlBounds(getCarbonControl(), &cbounds);
		MoveControl(getCarbonControl(), cbounds.left + xchange, cbounds.top + ychange);
		getBounds()->offset(xchange, ychange);
		redraw();
		lastX = inXpos;
		lastY = inYpos;
		lastXchange = xchange;
		lastYchange = ychange;
	}
#endif
	virtual void mouseUp(float inXpos, float inYpos, unsigned long inKeyModifiers)
	{
		if (GetControl32BitValue(getCarbonControl()) != 0)
		{
			launch_url(DESTROYFX_URL);
			SetControl32BitValue(getCarbonControl(), 0);
		}
	}

private:
	DGImage * buttonImage;
	float lastX, lastY;
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
long EQSyncEditor::open(float inXOffset, float inYOffset)
{
	// load some images

	// background image
	DGImage * gBackground = new DGImage("eq-sync-background.png", this);
	SetBackgroundImage(gBackground);

	DGImage * gHorizontalSliderBackground = new DGImage("horizontal-slider-background.png", this);
	DGImage * gVerticalSliderBackground = new DGImage("vertical-slider-background.png", this);
	DGImage * gSliderHandle = new DGImage("slider-handle.png", this);
	DGImage * gSliderHandleClicked = new DGImage("slider-handle-clicked.png", this);

//	DGImage * gHostSyncButton = new DGImage("host-sync-button.png", this);
	DGImage * gDestroyFXlinkTab = new DGImage("destroy-fx-link-tab.png", this);


	DGRect pos;

	for (long i=kRate_sync; i <= kTempo; i++)
	{
		// create the horizontal sliders
		pos.set(kWideFaderX, kWideFaderY + (kWideFaderInc * i), gHorizontalSliderBackground->getWidth(), gHorizontalSliderBackground->getHeight());
		EQSyncSlider * slider = new EQSyncSlider(this, i, &pos, kDGSliderAxis_horizontal, gSliderHandle, gSliderHandleClicked, gHorizontalSliderBackground);

		// create the displays
		displayTextProcedure textproc = NULL;
		if (i == kRate_sync)
			textproc = tempoRateDisplayProc;
		else if (i == kSmooth)
			textproc = smoothDisplayProc;
		else if (i == kTempo)
			textproc = tempoDisplayProc;
		pos.set(kDisplayX, kDisplayY + (kWideFaderInc * i), kDisplayWidth, kDisplayHeight);
		DGTextDisplay * display = new DGTextDisplay(this, i, &pos, textproc, this, NULL, kValueTextSize, kDGTextAlign_left, kBlackDGColor, kValueTextFont);
	}

	// create the vertical sliders
	for (long i=ka0; i <= kb2; i++)
	{
		pos.set(kTallFaderX + (kTallFaderInc * (i-ka0)), kTallFaderY, gVerticalSliderBackground->getWidth(), gVerticalSliderBackground->getHeight());
		EQSyncSlider * slider = new EQSyncSlider(this, i, &pos, kDGSliderAxis_vertical, gSliderHandle, gSliderHandleClicked, gVerticalSliderBackground);
	}


	// create the Destroy FX web page link tab
	pos.set(kDestroyFXlinkX, kDestroyFXlinkY, gDestroyFXlinkTab->getWidth(), gDestroyFXlinkTab->getHeight()/2);
	EQSyncWebLink * dfxLinkButton = new EQSyncWebLink(this, &pos, gDestroyFXlinkTab);


	return noErr;
}
