#include "eqsynceditor.hpp"
#include "eqsync.hpp"

#include "dfxguislider.h"
#include "dfxguidisplay.h"
#include "dfxguibutton.h"

#include "dfx-au-utilities.h"


//-----------------------------------------------------------------------------
enum {
	// positions
	kWideFaderX = 138 - 2,
	kWideFaderY = 68 - 7,
	kWideFaderX_panther = 141 - 2,
	kWideFaderY_panther = 64 - 7,
	kWideFaderInc = 40,

	kTallFaderX = 138 - 7,
	kTallFaderY = 196 - 2,
	kTallFaderX_panther = 141 - 7,
	kTallFaderY_panther = 192 - 2,
	kTallFaderInc = 48,

	kDisplayOffsetX = 212,
	kDisplayOffsetY = 2,
	kDisplayWidth = 81,
	kDisplayHeight = 12,

	kHostSyncButtonX = 53,
	kHostSyncButtonY = 164,
	kHostSyncButtonX_panther = 56,
	kHostSyncButtonY_panther = 160,
	
	kDestroyFXlinkX = 158,
	kDestroyFXlinkY = 12,
	kDestroyFXlinkX_panther = 159,
	kDestroyFXlinkY_panther = 11,

	kHelpButtonX = 419,
	kHelpButtonY = 293,
	kHelpButtonX_panther = 422,
	kHelpButtonY_panther = 289
};


const char * kValueTextFont = "Lucida Grande";
const float kValueTextSize = 13.0f;



//-----------------------------------------------------------------------------
// parameter value string display conversion functions

void tempoRateDisplayProc(float inValue, char * outText, void * inEditor);
void tempoRateDisplayProc(float inValue, char * outText, void * inEditor)
{
	((DfxGuiEditor*)inEditor)->getparametervaluestring(kRate_sync, outText);
}

void smoothDisplayProc(float inValue, char * outText, void *);
void smoothDisplayProc(float inValue, char * outText, void *)
{
	sprintf(outText, "%.1f%%", inValue);
}

void tempoDisplayProc(float inValue, char * outText, void *);
void tempoDisplayProc(float inValue, char * outText, void *)
{
	sprintf(outText, "%.3f", inValue);
}

//-----------------------------------------------------------------------------
void helpButtonProc(long, void *)
{
	launch_documentation();
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
	virtual void mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers, bool inIsDoubleClick)
	{
		handleImage = clickedHandle;	// switch to the click-styled handle
		DGSlider::mouseDown(inXpos, inYpos, inMouseButtons, inKeyModifiers, inIsDoubleClick);
		lastPX = inXpos;
		lastPY = inYpos;
		lastXchange = lastYchange = 0;
	}
#if 0
	virtual void mouseTrack(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers)
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
	virtual void mouseUp(float inXpos, float inYpos, DGKeyModifiers inKeyModifiers)
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

	virtual void draw(DGGraphicsContext * inContext)
	{
		if (buttonImage != NULL)
		{
			long yoff = (GetControl32BitValue(getCarbonControl()) == 0) ? 0 : (buttonImage->getHeight() / 2);
			buttonImage->draw(getBounds(), inContext, 0, yoff);
		}
	}
	virtual void mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers, bool inIsDoubleClick)
	{
		if ( (long)inXpos > ((getBounds()->w / 2) - 6) )
			SetControl32BitValue(getCarbonControl(), 1);
		lastX = inXpos;
		lastY = inYpos;
		lastXchange = lastYchange = 0;
	}
#if 0
	virtual void mouseTrack(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers)
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
	virtual void mouseUp(float inXpos, float inYpos, DGKeyModifiers inKeyModifiers)
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
COMPONENT_ENTRY(EQSyncEditor)

//-----------------------------------------------------------------------------
EQSyncEditor::EQSyncEditor(AudioUnitCarbonView inInstance)
:	DfxGuiEditor(inInstance)
{
}

//-----------------------------------------------------------------------------
long EQSyncEditor::open()
{
	long wideFaderX = kWideFaderX_panther;
	long wideFaderY = kWideFaderY_panther;
	long tallFaderX = kTallFaderX_panther;
	long tallFaderY = kTallFaderY_panther;
	long hostSyncButtonX = kHostSyncButtonX_panther;
	long hostSyncButtonY = kHostSyncButtonY_panther;
	long destroyFXlinkX = kDestroyFXlinkX_panther;
	long destroyFXlinkY = kDestroyFXlinkY_panther;
	long helpButtonX = kHelpButtonX_panther;
	long helpButtonY = kHelpButtonY_panther;

	DGImage * gBackground = NULL;
	DGImage * gHorizontalSliderBackground = NULL;
	DGImage * gVerticalSliderBackground = NULL;
	DGImage * gSliderHandle = NULL;
	DGImage * gSliderHandleClicked = NULL;
	DGImage * gHostSyncButton = NULL;
	DGImage * gDestroyFXlinkTab = NULL;

	long macOS = GetMacOSVersion() & 0xFFF0;
	switch (macOS)
	{
		// Jaguar (Mac OS X 10.2)
		case 0x1020:
			wideFaderX = kWideFaderX;
			wideFaderY = kWideFaderY;
			tallFaderX = kTallFaderX;
			tallFaderY = kTallFaderY;
			hostSyncButtonX = kHostSyncButtonX;
			hostSyncButtonY = kHostSyncButtonY;
			destroyFXlinkX = kDestroyFXlinkX;
			destroyFXlinkY = kDestroyFXlinkY;
			helpButtonX = kHelpButtonX;
			helpButtonY = kHelpButtonY;
			gBackground = new DGImage("eq-sync-background.png", this);
			gHorizontalSliderBackground = new DGImage("horizontal-slider-background.png", this);
			gVerticalSliderBackground = new DGImage("vertical-slider-background.png", this);
			gSliderHandle = new DGImage("slider-handle.png", this);
			gSliderHandleClicked = new DGImage("slider-handle-clicked.png", this);
			gHostSyncButton = new DGImage("host-sync-button-panther.png", this);	// it's the same widget image in Panther and Jaguar
			gDestroyFXlinkTab = new DGImage("destroy-fx-link-tab.png", this);
			break;
		// Panther (Mac OS X 10.3)
		case 0x1030:
		default:
			gBackground = new DGImage("eq-sync-background-panther.png", this);
			gHorizontalSliderBackground = new DGImage("horizontal-slider-background-panther.png", this);
			gVerticalSliderBackground = new DGImage("vertical-slider-background-panther.png", this);
			gSliderHandle = new DGImage("slider-handle-panther.png", this);
			gSliderHandleClicked = new DGImage("slider-handle-clicked-panther.png", this);
			gHostSyncButton = new DGImage("host-sync-button-panther.png", this);
			gDestroyFXlinkTab = new DGImage("destroy-fx-link-tab-panther.png", this);
			break;
	}

	DGImage * gHelpButton = new DGImage("help-button.png", this);

	SetBackgroundImage(gBackground);


	DGRect pos;

	for (long i=kRate_sync; i <= kTempo; i++)
	{
		// create the horizontal sliders
		pos.set(wideFaderX, wideFaderY + (kWideFaderInc * i), gHorizontalSliderBackground->getWidth(), gHorizontalSliderBackground->getHeight());
		EQSyncSlider * slider = new EQSyncSlider(this, i, &pos, kDGSliderAxis_horizontal, gSliderHandle, gSliderHandleClicked, gHorizontalSliderBackground);

		// create the displays
		displayTextProcedure textproc = NULL;
		if (i == kRate_sync)
			textproc = tempoRateDisplayProc;
		else if (i == kSmooth)
			textproc = smoothDisplayProc;
		else if (i == kTempo)
			textproc = tempoDisplayProc;
		pos.set(wideFaderX + kDisplayOffsetX, wideFaderY + kDisplayOffsetY + (kWideFaderInc * i), kDisplayWidth, kDisplayHeight);
		DGTextDisplay * display = new DGTextDisplay(this, i, &pos, textproc, this, NULL, kDGTextAlign_left, kValueTextSize, kDGColor_black, kValueTextFont);
	}

	// create the vertical sliders
	for (long i=ka0; i <= kb2; i++)
	{
		pos.set(tallFaderX + (kTallFaderInc * (i-ka0)), tallFaderY, gVerticalSliderBackground->getWidth(), gVerticalSliderBackground->getHeight());
		EQSyncSlider * slider = new EQSyncSlider(this, i, &pos, kDGSliderAxis_vertical, gSliderHandle, gSliderHandleClicked, gVerticalSliderBackground);
	}


	// create the host sync button
	pos.set(hostSyncButtonX, hostSyncButtonY, gHostSyncButton->getWidth()/2, gHostSyncButton->getHeight()/2);
	DGButton * hostSyncButton = new DGButton(this, kTempoAuto, &pos, gHostSyncButton, 2, kDGButtonType_incbutton, true);

	// create the Destroy FX web page link tab
	pos.set(destroyFXlinkX, destroyFXlinkY, gDestroyFXlinkTab->getWidth(), gDestroyFXlinkTab->getHeight()/2);
	EQSyncWebLink * dfxLinkButton = new EQSyncWebLink(this, &pos, gDestroyFXlinkTab);

	// create the help button
	pos.set(helpButtonX, helpButtonY, gHelpButton->getWidth(), gHelpButton->getHeight()/2);
	DGButton * helpButton = new DGButton(this, &pos, gHelpButton, 2, kDGButtonType_pushbutton);
	helpButton->setUserReleaseProcedure(helpButtonProc, this);
	helpButton->setUseReleaseProcedureOnlyAtEndWithNoCancel(true);


	return noErr;
}
