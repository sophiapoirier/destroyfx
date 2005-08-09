/*--------------- by Marc Poirier  ][  June 2001 + February 2003 + November 2003 --------------*/

#ifndef __RMSBUDDYEDITOR_H
#define __RMSBUDDYEDITOR_H


#include "AUCarbonViewBase.h"


class RMSBuddyEditor;

//-----------------------------------------------------------------------------
// a generic custom control class, which our useful controls are derived from
class RMSControl
{
public:
	RMSControl(RMSBuddyEditor * inOwnerEditor, long inXpos, long inYpos, long inWidth, long inHeight, 
				long inControlRange, long inParamID = -1);
	virtual ~RMSControl();

	// draw the control
	virtual void draw(CGContextRef inContext, float inPortHeight)
		{ }
	// handle a mouse click in the control
	virtual void mouseDown(long inXpos, long inYpos)
		{ }
	// handle a mouse drag on the control (mouse moved while clicked)
	virtual void mouseTrack(long inXpos, long inYpos)
		{ }
	// handle a mouse button release on the control
	virtual void mouseUp(long inXpos, long inYpos)
		{ }

	// force a redraw of the control to occur
	void redraw();

	// set/get the Carbon Control Manager control reference
	void setCarbonControl(ControlRef inCarbonControl)
		{	carbonControl = inCarbonControl;	}
	ControlRef getCarbonControl()
		{	return carbonControl;	}

	// get the rectangular position of the control
	CGRect getBoundsRect();
	// says whether or not this control needs to be clipped to its bounds when drawn
	// says whether or not this control is connected to an AU parameter
	bool isParameterAttached()
		{	return hasParameter;	}
	CAAUParameter * getAUVP()
		{	return &auvParam;	}

protected:
	RMSBuddyEditor * ownerEditor;
	long width, height;
	ControlRef carbonControl;
	CAAUParameter auvParam;
	bool hasParameter;
};



// 24-bit RGB color description, 0 - 255 values for each field
struct RMSColor {
	int r;
	int g;
	int b;
};

// used for specifying text alignment in RMSTextDisplay objects
enum {
	kTextAlign_left = 0,
	kTextAlign_center,
	kTextAlign_right
};

//-----------------------------------------------------------------------------
// a text display control object
class RMSTextDisplay : public RMSControl
{
public:
	RMSTextDisplay(RMSBuddyEditor * inOwnerEditor, long inXpos, long inYpos, long inWidth, long inHeight, 
					RMSColor inTextColor, RMSColor inBackColor, RMSColor inFrameColor, 
					const char * inFontName, float inFontSize, long inTextAlignment, long inParamID = -1);
	virtual ~RMSTextDisplay();

	virtual void draw(CGContextRef inContext, float inPortHeight);
	// set the display text directly with a string
	void setText(const char * inText);
	// given a linear amplitude value, set the display text with the dB-converted value
	void setText_dB(float inLinearValue);
	// set the display text with an integer value
	void setText_int(long inValue);

private:
	RMSColor textColor, backColor, frameColor;
	float fontSize;
	long textAlignment;
	char * fontName;
	char * text;
};


//-----------------------------------------------------------------------------
// a push-button control object
class RMSButton : public RMSControl
{
public:
	RMSButton(RMSBuddyEditor * inOwnerEditor, long inXpos, long inYpos, CGImageRef inImage);

	virtual void draw(CGContextRef inContext, float inPortHeight);
	virtual void mouseDown(long inXpos, long inYpos);
	virtual void mouseTrack(long inXpos, long inYpos);
	virtual void mouseUp(long inXpos, long inYpos);

private:
	CGImageRef buttonImage;
};


//-----------------------------------------------------------------------------
// a slider control object
class RMSSlider : public RMSControl
{
public:
	RMSSlider(RMSBuddyEditor * inOwnerEditor, long inParamID, long inXpos, long inYpos, long inWidth, long inHeight, 
				RMSColor inBackColor, RMSColor inFillColor);
	virtual ~RMSSlider();

	virtual void draw(CGContextRef inContext, float inPortHeight);
	virtual void mouseDown(long inXpos, long inYpos);
	virtual void mouseTrack(long inXpos, long inYpos);
	virtual void mouseUp(long inXpos, long inYpos);

private:
	RMSColor backColor;
	RMSColor fillColor;
	long borderWidth;
};



//-----------------------------------------------------------------------------
// our GUI
class RMSBuddyEditor : public AUCarbonViewBase
{
public:
	RMSBuddyEditor(AudioUnitCarbonView inInstance);
	virtual ~RMSBuddyEditor();

	virtual OSStatus CreateUI(Float32 inXOffset, Float32 inYOffset);
	virtual bool HandleEvent(EventRef inEvent);
	virtual ComponentResult Version()
		{	return RMS_BUDDY_VERSION;	}

	OSStatus setup();
	void cleanup();

	void updateDisplays();	// refresh the value displays
	void updateWindowSize(Float32 inParamValue, RMSControl * inRMSControl);	// update analysis window size parameter controls
	void resetRMS();	// send a message to the DSP component to reset average RMS
	void resetPeak();	// send a message to the DSP component to reset absolute peak

	// these are accessors for data that the custom controls need in order to create themselves
	ControlDefSpec * getControlClassSpec()
		{	return &controlClassSpec;	}
	AUParameterListenerRef GetParameterListener()
//		{	return mParameterListener;	}
		{	return parameterListener;	}
	void AddAUCVControl(AUCarbonViewControl * inControl)
		{	AddControl(inControl);	}

	// get/set the control that is currently being moused (actively tweaked), if any (returns NULL if none)
	RMSControl * getCurrentControl()
		{	return currentControl;	}
	void setCurrentControl(RMSControl * inNewClickedControl)
		{	currentControl = inNewClickedControl;	}

	void handleControlValueChange(RMSControl * inControl, SInt32 inControlValue);

private:
	unsigned long numChannels;	// the number of channels being analyzed

	// buttons
	RMSButton * resetRMSbutton;
	RMSButton * resetPeakButton;

	// text display boxes
	RMSTextDisplay ** averageRMSDisplays;
	RMSTextDisplay ** continualRMSDisplays;
	RMSTextDisplay ** absolutePeakDisplays;
	RMSTextDisplay ** continualPeakDisplays;
	RMSTextDisplay * averageRMSLabel;
	RMSTextDisplay * continualRMSLabel;
	RMSTextDisplay * absolutePeakLabel;
	RMSTextDisplay * continualPeakLabel;
	RMSTextDisplay ** channelLabels;

	// slider
	RMSSlider * windowSizeSlider;
	RMSTextDisplay * windowSizeLabel;
	RMSTextDisplay * windowSizeDisplay;

	// bitmap graphics resource
	CGImageRef gResetButton;

	// event handling related stuff
	EventHandlerUPP controlHandlerUPP;
	ControlDefSpec controlClassSpec;
	EventHandlerUPP windowEventHandlerUPP;
	EventHandlerRef windowEventEventHandlerRef;
	RMSControl * currentControl;


	// parameter listener related stuff
	AUParameterListenerRef parameterListener;
	AudioUnitParameter timeToUpdateAUP;
};


#endif