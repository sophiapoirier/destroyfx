/*--------------- by Marc Poirier  ][  June 2001 + February 2003 --------------*/

#ifndef __RMSBUDDYEDITOR_H
#define __RMSBUDDYEDITOR_H


#include "AUCarbonViewBase.h"


class RMSbuddyEditor;

//-----------------------------------------------------------------------------
class RMSControl
{
public:
	RMSControl(RMSbuddyEditor *inOwnerEditor, long inXpos, long inYpos, long inWidth, long inHeight);
	virtual ~RMSControl();

	virtual void draw(CGContextRef inContext, UInt32 inPortHeight)
		{ }
	virtual void mouseDown(long inXpos, long inYpos)
		{ }
	virtual void mouseTrack(long inXpos, long inYpos)
		{ }
	virtual void mouseUp(long inXpos, long inYpos)
		{ }

	void setCarbonControl(ControlRef inCarbonControl)
		{	carbonControl = inCarbonControl;	}
	ControlRef getCarbonControl()
		{	return carbonControl;	}
	Rect * getBoundsRect()
		{	return &boundsRect;	}
	bool needsClipping()
		{	return needsToBeClipped;	}

protected:
	RMSbuddyEditor *ownerEditor;
	long xpos, ypos, width, height;
	ControlRef carbonControl;
	Rect boundsRect;
	bool needsToBeClipped;
};



struct RMSColor {
	int r;
	int g;
	int b;
};

enum {
	kTextAlign_left = 0,
	kTextAlign_center,
	kTextAlign_right
};

//-----------------------------------------------------------------------------
class RMSTextDisplay : public RMSControl
{
public:
	RMSTextDisplay(RMSbuddyEditor *inOwnerEditor, long inXpos, long inYpos, long inWidth, long inHeight, RMSColor inTextColor, 
					RMSColor inBackColor, RMSColor inFrameColor, const char *inFontName, float inFontSize, long inTextAlignment);
	virtual ~RMSTextDisplay();

	virtual void draw(CGContextRef inContext, UInt32 inPortHeight);
	void setText(const char *inText);
	void setText_dB(float inLinearValue);

private:
	RMSColor textColor, backColor, frameColor;
	float fontSize;
	long textAlignment;
	char *fontName;
	char *text;
};


//-----------------------------------------------------------------------------
class RMSButton : public RMSControl
{
public:
	RMSButton(RMSbuddyEditor *inOwnerEditor, long inXpos, long inYpos, CGImageRef inImage);
	virtual ~RMSButton();

	virtual void draw(CGContextRef inContext, UInt32 inPortHeight);
	virtual void mouseDown(long inXpos, long inYpos);
	virtual void mouseTrack(long inXpos, long inYpos);
	virtual void mouseUp(long inXpos, long inYpos);

private:
	CGImageRef buttonImage;
};


//-----------------------------------------------------------------------------
class RMSbuddyEditor : public AUCarbonViewBase
{
public:
	RMSbuddyEditor(AudioUnitCarbonView inInstance);
	virtual ~RMSbuddyEditor();

	virtual OSStatus CreateUI(Float32 inXOffset, Float32 inYOffset);
	virtual bool HandleEvent(EventRef inEvent);

	void updateDisplays();
	void resetRMS();
	void resetPeak();

	ControlDefSpec * getControlClassSpec()
		{	return &controlClassSpec;	}
	// get the RMSControl object for a given CarbonControl reference
	RMSControl * getRMSControl(ControlRef inCarbonControl);
	// get/set the control that is currently being moused (actively tweaked), if any (returns NULL if none)
	RMSControl * getCurrentControl()
		{	return currentControl;	}
	void setCurrentControl(RMSControl *inNewClickedControl)
		{	currentControl = inNewClickedControl;	}
	ControlRef GetCarbonPane()
		{	return mCarbonPane;	}
	void handleControlValueChange(RMSControl *inControl, SInt32 inValue);

private:
	// controls
	RMSButton *resetRMSbutton;
	RMSButton *resetPeakButton;

	// parameter value display boxes
	RMSTextDisplay *leftAverageRMSDisplay;
	RMSTextDisplay *rightAverageRMSDisplay;
	RMSTextDisplay *leftContinualRMSDisplay;
	RMSTextDisplay *rightContinualRMSDisplay;
	RMSTextDisplay *leftAbsolutePeakDisplay;
	RMSTextDisplay *rightAbsolutePeakDisplay;
	RMSTextDisplay *leftContinualPeakDisplay;
	RMSTextDisplay *rightContinualPeakDisplay;
	RMSTextDisplay *averageRMSDisplay;
	RMSTextDisplay *continualRMSDisplay;
	RMSTextDisplay *absolutePeakDisplay;
	RMSTextDisplay *continualPeakDisplay;
	RMSTextDisplay *leftDisplay;
	RMSTextDisplay *rightDisplay;

	// graphics
	CGImageRef gResetButton;

	EventHandlerUPP controlHandlerUPP;
	ControlDefSpec controlClassSpec;
	EventHandlerUPP windowEventHandlerUPP;
	EventHandlerRef windowEventEventHandlerRef;
	RMSControl *currentControl;


	AUParameterListenerRef parameterListener;
	AudioUnitParameter timeToUpdateAUP;
};


#endif