/*--------------- by Marc Poirier  ][  June 2001 + February 2003 --------------*/

#include "rmsbuddyeditor.h"
#include "rmsbuddy.h"


static pascal OSStatus ControlEventHandler(EventHandlerCallRef, EventRef, void *inUserData);
static pascal OSStatus WindowEventHandler(EventHandlerCallRef, EventRef, void *inUserData);


//-----------------------------------------------------------------------------
RMSControl::RMSControl(RMSbuddyEditor *inOwnerEditor, long inXpos, long inYpos, long inWidth, long inHeight)
:	ownerEditor(inOwnerEditor), xpos(inXpos), ypos(inYpos), width(inWidth), height(inHeight), 
	carbonControl(NULL)
{
	boundsRect.left = xpos + (short)ownerEditor->GetXOffset();
	boundsRect.top = ypos + (short)ownerEditor->GetYOffset();
	boundsRect.right = boundsRect.left + width;
	boundsRect.bottom = boundsRect.top + height;
//printf("rect = %d, %d, %d, %d\n", boundsRect.left, boundsRect.top, boundsRect.right, boundsRect.bottom);

	if (CreateCustomControl(ownerEditor->GetCarbonWindow(), &boundsRect, ownerEditor->getControlClassSpec(), NULL, &carbonControl) == noErr)
	{
		SetControl32BitMinimum(carbonControl, 0);
		SetControl32BitMaximum(carbonControl, 1);
		SetControl32BitValue(carbonControl, 0);
#if 0
		ownerEditor->EmbedControl(carbonControl);
#else
		WindowAttributes attributes;
		verify_noerr( GetWindowAttributes(ownerEditor->GetCarbonWindow(), &attributes) );
		if (attributes & kWindowCompositingAttribute)
			::HIViewAddSubview(ownerEditor->GetCarbonPane(), carbonControl);
		else
			::EmbedControl(carbonControl, ownerEditor->GetCarbonPane());
#endif
	}

	needsToBeClipped = false;
}

//-----------------------------------------------------------------------------
RMSControl::~RMSControl()
{
	if (carbonControl != NULL)
		DisposeControl(carbonControl);
	carbonControl = NULL;
}



#define MINUS_INFINITY_STRING	"-oo"

//-----------------------------------------------------------------------------
RMSTextDisplay::RMSTextDisplay(RMSbuddyEditor *inOwnerEditor, long inXpos, long inYpos, long inWidth, long inHeight, 
					RMSColor inTextColor, RMSColor inBackColor, RMSColor inFrameColor, 
					const char *inFontName, float inFontSize, long inTextAlignment)
:	RMSControl(inOwnerEditor, inXpos, inYpos, inWidth, inHeight), 
	textColor(inTextColor), backColor(inBackColor), frameColor(inFrameColor), 
	fontSize(inFontSize), textAlignment(inTextAlignment), 
	fontName(NULL), text(NULL)
{
	fontName = (char*) malloc(256);
	fontName[0] = 0;
	if (inFontName != NULL)
		strcpy(fontName, inFontName);

	text = (char*) malloc(256);
	text[0] = 0;
}

//-----------------------------------------------------------------------------
RMSTextDisplay::~RMSTextDisplay()
{
	if (fontName != NULL)
		free(fontName);
	fontName = NULL;

	if (text != NULL)
		free(text);
	text = NULL;
}

//-----------------------------------------------------------------------------
void RMSTextDisplay::draw(CGContextRef inContext, UInt32 inPortHeight)
{
	CGContextSetShouldAntialias(inContext, false);	// XXX maybe this is more efficient?

	// fill the background color
	CGRect bounds = CGRectMake(getBoundsRect()->left, inPortHeight - getBoundsRect()->bottom, width, height);
	CGContextSetRGBFillColor(inContext, (float)backColor.r/255.0f, (float)backColor.g/255.0f, (float)backColor.b/255.0f, 1.0f);
	CGContextFillRect(inContext, bounds);

	// draw the box frame
	CGContextSetRGBStrokeColor(inContext, (float)frameColor.r/255.0f, (float)frameColor.g/255.0f, (float)frameColor.b/255.0f, 1.0f);
	// Quartz draws lines on top of the pixel, so you need to move the coordinates to the middle of the pixel, 
	// and then also shrink the size accordingly
	CGRect box = CGRectMake(bounds.origin.x + 0.5f, bounds.origin.y + 0.5f, bounds.size.width - 1.0f, bounds.size.height - 1.0f);
	CGContextStrokeRectWithWidth(inContext, box, 1.0f);

	// draw the text
	CGContextSetShouldAntialias(inContext, true);
	CGContextSelectFont(inContext, fontName, fontSize, kCGEncodingMacRoman);
	CGContextSetRGBFillColor(inContext, (float)textColor.r/255.0f, (float)textColor.g/255.0f, (float)textColor.b/255.0f, 1.0f);
	if (textAlignment != kTextAlign_left)
	{
		CGContextSetTextDrawingMode(inContext, kCGTextInvisible);
		CGContextShowTextAtPoint(inContext, 0.0f, 0.0f, text, strlen(text));
		CGPoint pt = CGContextGetTextPosition(inContext);
		if (textAlignment == kTextAlign_center)
			bounds.origin.x += (bounds.size.width - pt.x) / 2.0f;
		else if (textAlignment == kTextAlign_right)
			bounds.origin.x += bounds.size.width - pt.x;
	}
	CGContextSetTextDrawingMode(inContext, kCGTextFill);
	// try to simulate an infinity symbol by squashing two lowercase "o"s together
	long minusInfLength = strlen(MINUS_INFINITY_STRING);
	if (strncmp(text, MINUS_INFINITY_STRING, minusInfLength) == 0)
	{
		CGContextShowTextAtPoint(inContext, bounds.origin.x, bounds.origin.y+4.0f, text, minusInfLength-1);
		bounds.origin.x = CGContextGetTextPosition(inContext).x - 1.5f;
		char *text2 = &(text[minusInfLength-1]);
		CGContextShowTextAtPoint(inContext, bounds.origin.x, bounds.origin.y+4.0f, text2, strlen(text2));
	}
	else
		CGContextShowTextAtPoint(inContext, bounds.origin.x, bounds.origin.y+4.0f, text, strlen(text));
}

//-----------------------------------------------------------------------------
void RMSTextDisplay::setText(const char *inText)
{
	if (inText != NULL)
	{
		strcpy(text, inText);
		Draw1Control(carbonControl);
	}
}

//-----------------------------------------------------------------------------
void RMSTextDisplay::setText_dB(float inLinearValue)
{
	if (inLinearValue <= 0.0f)
		sprintf(text, MINUS_INFINITY_STRING);
	else
	{
		float dBvalue = 20.0f * (float)log10(inLinearValue);	// convert linear value to dB
		if (dBvalue >= 0.01f)
			sprintf(text, "+%.2f", dBvalue);
		else if (fabsf(dBvalue) >= 100.0f)
			sprintf(text, "%.1f", dBvalue);
		else
			sprintf(text, "%.2f", dBvalue);
	}
	strcat(text, " dB");

	Draw1Control(carbonControl);
}



//-----------------------------------------------------------------------------
RMSButton::RMSButton(RMSbuddyEditor *inOwnerEditor, long inXpos, long inYpos, CGImageRef inImage)
:	RMSControl(inOwnerEditor, inXpos, inYpos, CGImageGetWidth(inImage), CGImageGetHeight(inImage)/2), 
	buttonImage(inImage)
{
	needsToBeClipped = true;
}

//-----------------------------------------------------------------------------
RMSButton::~RMSButton()
{
}

//-----------------------------------------------------------------------------
void RMSButton::draw(CGContextRef inContext, UInt32 inPortHeight)
{
	CGContextSetShouldAntialias(inContext, false);	// XXX maybe this is more efficient?
	CGRect bounds = CGRectMake(getBoundsRect()->left, inPortHeight - getBoundsRect()->bottom, width, height);
//	bounds.size.width = CGImageGetWidth(theButton);
//	bounds.size.height = CGImageGetHeight(theButton);
	bounds.size.height *= 2;
	if ( GetControl32BitValue(getCarbonControl()) == 0 )
		bounds.origin.y -= (float)height;
	CGContextDrawImage(inContext, bounds, buttonImage);
}

//-----------------------------------------------------------------------------
void RMSButton::mouseDown(long inXpos, long inYpos)
{
	SetControl32BitValue(getCarbonControl(), 1);
}

//-----------------------------------------------------------------------------
void RMSButton::mouseTrack(long inXpos, long inYpos)
{
	SInt32 currentValue = GetControl32BitValue(getCarbonControl());

	// mouse is inside the button zone, so the value should be 1
	if ( (inXpos >= 0) && (inXpos <= width) && (inYpos >= 0) && (inYpos <= height) )
	{
		if (currentValue == 0)
			SetControl32BitValue(getCarbonControl(), 1);
	}

	// mouse is outside of the button zone, so the value should be 0
	else
	{
		if (currentValue != 0)
			SetControl32BitValue(getCarbonControl(), 0);
	}
}

//-----------------------------------------------------------------------------
void RMSButton::mouseUp(long inXpos, long inYpos)
{
	SetControl32BitValue(getCarbonControl(), 0);
}



//-----------------------------------------------------------------------------
// constants

const RMSColor kMyBrownColor = { 97, 73, 46 };
const RMSColor kMyLightBrownColor = { 146, 116, 98 };
const RMSColor kMyDarkBlueColor = { 54, 69, 115 };
const RMSColor kMyLightOrangeColor = { 219, 145, 85 };
const RMSColor kWhiteColor = { 255, 255, 255 };

#define VALUE_DISPLAY_FONT	"Monaco"
#define VALUE_DISPLAY_FONT_SIZE	12.0f
#define LABEL_DISPLAY_FONT	"Lucida Grande"
#define LABEL_DISPLAY_FONT_SIZE	12.0f

#define kBackgroundColor	kMyLightBrownColor
#define kLabelTextColor		kWhiteColor
#define kReadoutFrameColor	kMyBrownColor
#define kReadoutBoxColor	kMyLightOrangeColor
#define kReadoutTextColor	kMyDarkBlueColor


//-----------------------------------------------------------------------------
enum {
	// positions
	kBackgroundWidth = 330,
	kBackgroundHeight = 156,

	kRMSlabelX = 6,
	kRMSlabelY = 27,
	kRMSlabelWidth = 87,
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
};



//-----------------------------------------------------------------------------
static void TimeToUpdateListenerProc(void *inRefCon, void *inObject, const AudioUnitParameter *inParameter, Float32 inValue)
{
	if (inObject != NULL)
		((RMSbuddyEditor*)inObject)->updateDisplays();
}




// macro for boring entry point stuff
COMPONENT_ENTRY(RMSbuddyEditor);

//-----------------------------------------------------------------------------
RMSbuddyEditor::RMSbuddyEditor(AudioUnitCarbonView inInstance)
:	AUCarbonViewBase(inInstance), 
	parameterListener(NULL)
{
	// initialize the graphics pointers
	gResetButton = NULL;

	// initialize the controls pointers
	resetRMSbutton = NULL;
	resetPeakButton = NULL;

	// initialize the value display box pointers
	leftAverageRMSDisplay = NULL;
	rightAverageRMSDisplay = NULL;
	leftContinualRMSDisplay = NULL;
	rightContinualRMSDisplay = NULL;
	leftAbsolutePeakDisplay = NULL;
	rightAbsolutePeakDisplay = NULL;
	leftContinualPeakDisplay = NULL;
	rightContinualPeakDisplay = NULL;
	averageRMSDisplay = NULL;
	continualRMSDisplay = NULL;
	absolutePeakDisplay = NULL;
	continualPeakDisplay = NULL;
	leftDisplay = NULL;
	rightDisplay = NULL;

	controlClassSpec.defType = kControlDefObjectClass;
	controlClassSpec.u.classRef = NULL;
	controlHandlerUPP = NULL;
	windowEventHandlerUPP = NULL;
	windowEventEventHandlerRef = NULL;
	currentControl = NULL;
}

//-----------------------------------------------------------------------------
RMSbuddyEditor::~RMSbuddyEditor()
{
	// free the graphics resource
	if (gResetButton != NULL)
		CGImageRelease(gResetButton);
	gResetButton = NULL;

	// free the controls
#define SAFE_DELETE_CONTROL(ctrl)	if (ctrl != NULL)   delete ctrl;   ctrl = NULL;
	SAFE_DELETE_CONTROL(resetRMSbutton)
	SAFE_DELETE_CONTROL(resetPeakButton)
	SAFE_DELETE_CONTROL(leftAverageRMSDisplay)
	SAFE_DELETE_CONTROL(rightAverageRMSDisplay)
	SAFE_DELETE_CONTROL(leftContinualRMSDisplay)
	SAFE_DELETE_CONTROL(rightContinualRMSDisplay)
	SAFE_DELETE_CONTROL(leftAbsolutePeakDisplay)
	SAFE_DELETE_CONTROL(rightAbsolutePeakDisplay)
	SAFE_DELETE_CONTROL(leftContinualPeakDisplay)
	SAFE_DELETE_CONTROL(rightContinualPeakDisplay)
	SAFE_DELETE_CONTROL(averageRMSDisplay)
	SAFE_DELETE_CONTROL(continualRMSDisplay)
	SAFE_DELETE_CONTROL(absolutePeakDisplay)
	SAFE_DELETE_CONTROL(continualPeakDisplay)
	SAFE_DELETE_CONTROL(leftDisplay)
	SAFE_DELETE_CONTROL(rightDisplay)
#undef SAFE_DELETE_CONTROL

	if (parameterListener != NULL)
	{
		AUListenerRemoveParameter(parameterListener, this, &timeToUpdateAUP);
		AUListenerDispose(parameterListener);
	}
	parameterListener = NULL;

	if (windowEventEventHandlerRef != NULL)
		RemoveEventHandler(windowEventEventHandlerRef);
	windowEventEventHandlerRef = NULL;
	if (windowEventHandlerUPP != NULL)
		DisposeEventHandlerUPP(windowEventHandlerUPP);
	windowEventHandlerUPP = NULL;

	// only unregister in Mac OS X version 10.2.3 or higher, otherwise this will cause a crash
	long systemVersion = 0;
	if ( (Gestalt(gestaltSystemVersion, &systemVersion) == noErr) && ((systemVersion & 0xFFFF) >= 0x1023) )
	{
		if (controlClassSpec.u.classRef != NULL)
		{
			OSStatus unregResult = UnregisterToolboxObjectClass( (ToolboxObjectClassRef)(controlClassSpec.u.classRef) );
			if (unregResult == noErr)
			{
				controlClassSpec.u.classRef = NULL;
				if (controlHandlerUPP != NULL)
					DisposeEventHandlerUPP(controlHandlerUPP);
				controlHandlerUPP = NULL;
			}
		}
	}
}

//-----------------------------------------------------------------------------
OSStatus RMSbuddyEditor::CreateUI(Float32 inXOffset, Float32 inYOffset)
{
	// register for draw events for our pane
	EventTypeSpec paneEvents[] = {
		{ kEventClassControl, kEventControlDraw }
	};
	WantEventTypes(GetControlEventTarget(mCarbonPane), GetEventTypeCount(paneEvents), paneEvents);


// create our HIToolbox object class for common event handling amongst our custom Carbon Controls

	EventTypeSpec toolboxClassEvents[] = {
		{ kEventClassControl, kEventControlDraw }, 
		{ kEventClassControl, kEventControlClick }, 
		{ kEventClassControl, kEventControlValueFieldChanged }, 
	};

	ToolboxObjectClassRef newControlClass = NULL;
	controlHandlerUPP = NewEventHandlerUPP(ControlEventHandler);
	unsigned long instanceAddress = (unsigned long) this;
	bool noSuccessYet = true;
	while (noSuccessYet)
	{
		CFStringRef toolboxClassIDcfstring = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%s.ControlClass%lu"), 
																		RMS_BUDDY_BUNDLE_ID, instanceAddress);
		if ( RegisterToolboxObjectClass(toolboxClassIDcfstring, NULL, GetEventTypeCount(toolboxClassEvents), toolboxClassEvents, 
										controlHandlerUPP, this, &newControlClass) == noErr )
			noSuccessYet = false;
		CFRelease(toolboxClassIDcfstring);
		instanceAddress++;
	}

	controlClassSpec.u.classRef = newControlClass;


// create the window event handler that supplements the control event handler by tracking mouse dragging, mouseover controls, etc.

	setCurrentControl(NULL);	// make sure that it ain't nuthin
	EventTypeSpec controlMouseEvents[] = {
								  { kEventClassMouse, kEventMouseDragged }, 
								  { kEventClassMouse, kEventMouseUp }, 
								};
	windowEventHandlerUPP = NewEventHandlerUPP(WindowEventHandler);
	InstallEventHandler(GetWindowEventTarget(GetCarbonWindow()), windowEventHandlerUPP, 
						GetEventTypeCount(controlMouseEvents), controlMouseEvents, this, &windowEventEventHandlerRef);



	// load our graphics resource
	CFBundleRef pluginBundleRef = CFBundleGetBundleWithIdentifier(CFSTR(RMS_BUDDY_BUNDLE_ID));
	if (pluginBundleRef != NULL)
	{
		CFURLRef resourceURL = CFBundleCopyResourceURL(pluginBundleRef, CFSTR("reset-button.png"), NULL, NULL);
		if (resourceURL != NULL)
		{
			CGDataProviderRef provider = CGDataProviderCreateWithURL(resourceURL);
			if (provider != NULL)
			{
				gResetButton = CGImageCreateWithPNGDataProvider(provider, NULL, false, kCGRenderingIntentDefault);
				CGDataProviderRelease(provider);
			}
			CFRelease(resourceURL);
		}
	}


	//--initialize the displays---------------------------------------------

	// start the counting/accumulating cycle anew
//	((RMSbuddy*)effect)->resetGUIcounters();

	long xpos = kValueDisplayX;
	long ypos = kValueDisplayY;

#define VALUE_TEXT_DISPLAY	RMSTextDisplay(this, xpos, ypos, kValueDisplayWidth, kValueDisplayHeight, kReadoutTextColor, kReadoutBoxColor, kReadoutFrameColor, VALUE_DISPLAY_FONT, VALUE_DISPLAY_FONT_SIZE, kTextAlign_center)

	// left channel average RMS read-out
	leftAverageRMSDisplay = new VALUE_TEXT_DISPLAY;
//	leftAverageRMSDisplay = new RMSTextDisplay(this, xpos, ypos, 75, 17, kReadoutTextColor, kReadoutBoxColor, kReadoutFrameColor, VALUE_DISPLAY_FONT, VALUE_DISPLAY_FONT_SIZE, kTextAlign_center);

	// right channel average RMS read-out
	xpos += kXinc;
	rightAverageRMSDisplay = new VALUE_TEXT_DISPLAY;

	// left channel continual RMS read-out
	xpos -= kXinc;
	ypos += kYinc;
	leftContinualRMSDisplay = new VALUE_TEXT_DISPLAY;

	// right channel continual RMS read-out
	xpos += kXinc;
	rightContinualRMSDisplay = new VALUE_TEXT_DISPLAY;

	// left channel absolute peak read-out
	xpos -= kXinc;
	ypos += kYinc;
	leftAbsolutePeakDisplay = new VALUE_TEXT_DISPLAY;

	// right channel absolute peak read-out
	xpos += kXinc;
	rightAbsolutePeakDisplay = new VALUE_TEXT_DISPLAY;

	// left channel continual peak read-out
	xpos -= kXinc;
	ypos += kYinc;
	leftContinualPeakDisplay = new VALUE_TEXT_DISPLAY;

	// right channel continual peak read-out
	xpos += kXinc;
	rightContinualPeakDisplay = new VALUE_TEXT_DISPLAY;

#undef VALUE_TEXT_DISPLAY

	// display the proper current dynamics value read-outs
	updateDisplays();


	xpos = kRMSlabelX;
	ypos = kRMSlabelY;

#define LABEL_TEXT_DISPLAY	RMSTextDisplay(this, xpos, ypos, kRMSlabelWidth, kRMSlabelHeight, kLabelTextColor, kBackgroundColor, kBackgroundColor, LABEL_DISPLAY_FONT, LABEL_DISPLAY_FONT_SIZE, kTextAlign_right)

	// the words "average RMS"
	averageRMSDisplay = new LABEL_TEXT_DISPLAY;
	averageRMSDisplay->setText("average RMS");

	// the words "continual RMS"
	ypos += kYinc;
	continualRMSDisplay = new LABEL_TEXT_DISPLAY;
	continualRMSDisplay->setText("continual RMS");

	// the words "absolute peak"
	ypos += kYinc;
	absolutePeakDisplay = new LABEL_TEXT_DISPLAY;
	absolutePeakDisplay->setText("absolute peak");

	// the words "continual peak"
	ypos += kYinc;
	continualPeakDisplay = new LABEL_TEXT_DISPLAY;
	continualPeakDisplay->setText("continual peak");

#undef LABEL_TEXT_DISPLAY

	xpos = kRLlabelX;
	ypos = kRLlabelY;

#define LABEL_TEXT_DISPLAY	RMSTextDisplay(this, xpos, ypos, kRLlabelWidth, kRLlabelHeight, kLabelTextColor, kBackgroundColor, kBackgroundColor, LABEL_DISPLAY_FONT, LABEL_DISPLAY_FONT_SIZE, kTextAlign_center)

	// the word "left"
	leftDisplay = new LABEL_TEXT_DISPLAY;
	leftDisplay->setText("left");

	// the word "right"
	xpos += kXinc;
	rightDisplay = new LABEL_TEXT_DISPLAY;
	rightDisplay->setText("right");

#undef LABEL_TEXT_DISPLAY


	//--initialize the buttons-----------------------------------------------

	// reset continual RMS button
	resetRMSbutton = new RMSButton(this, kButtonX, kButtonY, gResetButton);

	// reset peak button
	resetPeakButton = new RMSButton(this, kButtonX, kButtonY + (kYinc*2), gResetButton);


	AUListenerCreate(TimeToUpdateListenerProc, this,
		CFRunLoopGetCurrent(), kCFRunLoopDefaultMode, 0.010f, // 10 ms
		&parameterListener);

	timeToUpdateAUP.mAudioUnit = GetEditAudioUnit();
	timeToUpdateAUP.mScope = kAudioUnitScope_Global;
	timeToUpdateAUP.mElement = 0;
	timeToUpdateAUP.mParameterID = kTimeToUpdate;
	AUListenerAddParameter(parameterListener, this, &timeToUpdateAUP);


	// set size of the background pane
	SizeControl(mCarbonPane, kBackgroundWidth, kBackgroundHeight);


	return noErr;
}

//-----------------------------------------------------------------------------
bool RMSbuddyEditor::HandleEvent(EventRef inEvent)
{
	// catch any clicks on the embedding pane in order to clear focus
	if ( (GetEventClass(inEvent) == kEventClassControl) && (GetEventKind(inEvent) == kEventControlDraw) )
	{
		ControlRef carbonControl;
		GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, NULL, sizeof(ControlRef), NULL, &carbonControl);
		if (carbonControl == mCarbonPane)
		{
			CGrafPtr oldPort = NULL;
			CGrafPtr windowPort;
			// if we received a graphics port parameter, use that...
			if ( GetEventParameter(inEvent, kEventParamGrafPort, typeGrafPtr, NULL, sizeof(CGrafPtr), NULL, &windowPort) == noErr )
			{
				GetPort(&oldPort);	// remember original port
				SetPort(windowPort);	// use new port
			}
			// ... otherwise use the current graphics port
			else
				GetPort(&windowPort);
			Rect portBounds;
			GetPortBounds(windowPort, &portBounds);

			// drawing
			CGContextRef context;
			QDBeginCGContext(windowPort, &context);
			SyncCGContextOriginWithPort(context, windowPort);
			CGContextSaveGState(context);
			CGRect bounds = CGRectMake(GetXOffset(), portBounds.bottom - kBackgroundHeight - GetYOffset(), kBackgroundWidth, kBackgroundHeight);
			CGContextSetRGBFillColor(context, (float)kBackgroundColor.r/255.0f, (float)kBackgroundColor.g/255.0f, 
										(float)kBackgroundColor.b/255.0f, 1.0f);
			CGContextFillRect(context, bounds);
			CGContextRestoreGState(context);
			CGContextSynchronize(context);
			QDEndCGContext(windowPort, &context);

			// restore original port, if we set a different port
			if (oldPort != NULL)
				SetPort(oldPort);
			return true;
		}
	}

	return AUCarbonViewBase::HandleEvent(inEvent);
}

//-----------------------------------------------------------------------------
static pascal OSStatus WindowEventHandler(EventHandlerCallRef myHandler, EventRef inEvent, void *inUserData)
{
	if (GetEventClass(inEvent) != kEventClassMouse)
		return eventClassIncorrectErr;

	RMSbuddyEditor *ourOwnerEditor = (RMSbuddyEditor*) inUserData;

	HIPoint mouseLocation_f;
	GetEventParameter(inEvent, kEventParamMouseLocation, typeHIPoint, NULL, sizeof(HIPoint), NULL, &mouseLocation_f);
	long mouseX = (long) mouseLocation_f.x;
	long mouseY = (long) mouseLocation_f.y;


// follow the mouse when dragging (adjusting) a GUI control
	RMSControl *ourRMSControl = ourOwnerEditor->getCurrentControl();
	if (ourRMSControl == NULL)
		return eventNotHandledErr;

	Rect controlBounds;
	GetControlBounds(ourRMSControl->getCarbonControl(), &controlBounds);
	Rect globalBounds;	// Window Content Region
	WindowRef window;
	GetEventParameter(inEvent, kEventParamWindowRef, typeWindowRef, NULL, sizeof(WindowRef), NULL, &window);
	GetWindowBounds(window, kWindowGlobalPortRgn, &globalBounds);

	// orient the mouse coordinates as though the control were at 0, 0 (for convenience)
	mouseX -= controlBounds.left + globalBounds.left;
	mouseY -= controlBounds.top + globalBounds.top;


	UInt32 inEventKind = GetEventKind(inEvent);

	if (inEventKind == kEventMouseDragged)
	{
		ourRMSControl->mouseTrack(mouseX, mouseY);
		return noErr;
	}

	if (inEventKind == kEventMouseUp)
	{
		ourRMSControl->mouseUp(mouseX, mouseY);
		ourOwnerEditor->setCurrentControl(NULL);
		return noErr;
	}

	return eventKindIncorrectErr;
}

//-----------------------------------------------------------------------------
static pascal OSStatus ControlEventHandler(EventHandlerCallRef myHandler, EventRef inEvent, void *inUserData)
{
	if (GetEventClass(inEvent) != kEventClassControl)
		return eventClassIncorrectErr;

	ControlRef ourCarbonControl = NULL;
	GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, NULL, sizeof(ControlRef), NULL, &ourCarbonControl);
	RMSbuddyEditor *ourOwnerEditor = (RMSbuddyEditor*) inUserData;
	RMSControl *ourRMSControl = ourOwnerEditor->getRMSControl(ourCarbonControl);
	if (ourRMSControl == NULL)
		return eventNotHandledErr;

	switch (GetEventKind(inEvent))
	{
		case kEventControlDraw:
			{
				CGrafPtr oldPort = NULL;
				CGrafPtr windowPort;
				// if we received a graphics port parameter, use that...
				if ( GetEventParameter(inEvent, kEventParamGrafPort, typeGrafPtr, NULL, sizeof(CGrafPtr), NULL, &windowPort) == noErr )
				{
					GetPort(&oldPort);	// remember original port
					SetPort(windowPort);	// use new port
				}
				// ... otherwise use the current graphics port
				else
					GetPort(&windowPort);
				Rect portBounds;
				GetPortBounds(windowPort, &portBounds);

				// clip
				RgnHandle clipRgn;
				if ( ourRMSControl->needsClipping() )
				{
					clipRgn = NewRgn();
					OpenRgn();
					FrameRect(ourRMSControl->getBoundsRect());
					CloseRgn(clipRgn);
					SetClip(clipRgn);
					clipRgn = GetPortClipRegion(windowPort, clipRgn);
				}

				// draw
				CGContextRef context;
				QDBeginCGContext(windowPort, &context);
				if ( ourRMSControl->needsClipping() )
				{
					ClipCGContextToRegion(context, &portBounds, clipRgn);
					DisposeRgn(clipRgn);
				}
				SyncCGContextOriginWithPort(context, windowPort);
				CGContextSaveGState(context);
				ourRMSControl->draw(context, portBounds.bottom);
				CGContextRestoreGState(context);
				CGContextSynchronize(context);
				QDEndCGContext(windowPort, &context);
			
				// restore original port, if we set a different port
				if (oldPort != NULL)
					SetPort(oldPort);
			}
			return noErr;

		case kEventControlClick:
			{
				Rect controlBounds;
				GetControlBounds(ourCarbonControl, &controlBounds);
				Rect globalBounds;	// Window Content Region
				GetWindowBounds( GetControlOwner(ourCarbonControl), kWindowGlobalPortRgn, &globalBounds );

				HIPoint mouseLocation_f;
				GetEventParameter(inEvent, kEventParamMouseLocation, typeHIPoint, NULL, sizeof(HIPoint), NULL, &mouseLocation_f);
				long mouseX = (long) mouseLocation_f.x;
				long mouseY = (long) mouseLocation_f.y;

				// orient the mouse coordinates as though the control were at 0, 0 (for convenience)
				mouseX -= controlBounds.left + globalBounds.left;
				mouseY -= controlBounds.top + globalBounds.top;

				ourRMSControl->mouseDown(mouseX, mouseY);
				ourOwnerEditor->setCurrentControl(ourRMSControl);
			}
			return noErr;

		case kEventControlValueFieldChanged:
			ourOwnerEditor->handleControlValueChange(ourRMSControl, GetControl32BitValue(ourCarbonControl));
			return noErr;

		default:
			return eventKindIncorrectErr;
	}
}

//-----------------------------------------------------------------------------
void RMSbuddyEditor::updateDisplays()
{
	DynamicsData request;
	UInt32 dataSize = sizeof(request);

	request.channel = 0;
	if (AudioUnitGetProperty(GetEditAudioUnit(), kDynamicsDataProperty, kAudioUnitScope_Global, 0, &request, &dataSize) == noErr)
	{
		if (leftAverageRMSDisplay != NULL)
			leftAverageRMSDisplay->setText_dB(request.averageRMS);
		if (leftContinualRMSDisplay != NULL)
			leftContinualRMSDisplay->setText_dB(request.continualRMS);
		if (leftAbsolutePeakDisplay != NULL)
			leftAbsolutePeakDisplay->setText_dB(request.absolutePeak);
		if (leftContinualPeakDisplay != NULL)
			leftContinualPeakDisplay->setText_dB(request.continualPeak);
	}

	request.channel = 1;
	if (AudioUnitGetProperty(GetEditAudioUnit(), kDynamicsDataProperty, kAudioUnitScope_Global, 0, &request, &dataSize) == noErr)
	{
		if (rightAverageRMSDisplay != NULL)
			rightAverageRMSDisplay->setText_dB(request.averageRMS);
		if (rightContinualRMSDisplay != NULL)
			rightContinualRMSDisplay->setText_dB(request.continualRMS);
		if (rightAbsolutePeakDisplay != NULL)
			rightAbsolutePeakDisplay->setText_dB(request.absolutePeak);
		if (rightContinualPeakDisplay != NULL)
			rightContinualPeakDisplay->setText_dB(request.continualPeak);
	}
}

//-----------------------------------------------------------------------------
void RMSbuddyEditor::resetRMS()
{
	char nud;	// irrelavant
	AudioUnitSetProperty(GetEditAudioUnit(), kResetRMSProperty, kAudioUnitScope_Global, (AudioUnitElement)0, &nud, sizeof(char));
}

//-----------------------------------------------------------------------------
void RMSbuddyEditor::resetPeak()
{
	char nud;	// irrelavant
	AudioUnitSetProperty(GetEditAudioUnit(), kResetPeakProperty, kAudioUnitScope_Global, (AudioUnitElement)0, &nud, sizeof(char));
}

//-----------------------------------------------------------------------------
void RMSbuddyEditor::handleControlValueChange(RMSControl *inControl, SInt32 inValue)
{
	if (inControl == NULL)
		return;

	// if the button was pressed, the value will be 1 (it's 0 when the button is released)
	if (inValue > 0)
	{
		if (inControl == resetRMSbutton)
		{
			resetRMS();
			updateDisplays();
		}

		else if (inControl == resetPeakButton)
		{
			resetPeak();
			updateDisplays();
		}
	}
}

//-----------------------------------------------------------------------------
// get the RMSControl object for a given CarbonControl reference
RMSControl * RMSbuddyEditor::getRMSControl(ControlRef inCarbonControl)
{
#define CHECK_RMS_CONTROL(ctrl)	\
	if (ctrl != NULL)	\
	{	\
		if (ctrl->getCarbonControl() == inCarbonControl)	\
			return ctrl;	\
	}

	CHECK_RMS_CONTROL(resetRMSbutton)
	CHECK_RMS_CONTROL(resetPeakButton)
	CHECK_RMS_CONTROL(leftAverageRMSDisplay)
	CHECK_RMS_CONTROL(rightAverageRMSDisplay)
	CHECK_RMS_CONTROL(leftContinualRMSDisplay)
	CHECK_RMS_CONTROL(rightContinualRMSDisplay)
	CHECK_RMS_CONTROL(leftAbsolutePeakDisplay)
	CHECK_RMS_CONTROL(rightAbsolutePeakDisplay)
	CHECK_RMS_CONTROL(leftContinualPeakDisplay)
	CHECK_RMS_CONTROL(rightContinualPeakDisplay)
	CHECK_RMS_CONTROL(averageRMSDisplay)
	CHECK_RMS_CONTROL(continualRMSDisplay)
	CHECK_RMS_CONTROL(absolutePeakDisplay)
	CHECK_RMS_CONTROL(continualPeakDisplay)
	CHECK_RMS_CONTROL(leftDisplay)
	CHECK_RMS_CONTROL(rightDisplay)

#undef CHECK_RMS_CONTROL

	return NULL;
}
