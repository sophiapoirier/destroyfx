/*--------------- by Marc Poirier  ][  June 2001 + February 2003 + November 2003 --------------*/

#include "rmsbuddyeditor.h"
#include "rmsbuddy.h"


// I want the analysis window size slider to have a squared-scaled value distribution curve, 
// so it seems that AUCarbonViewControl won't be able to do what I want
#define USE_AUCVCONTROL	0


//-----------------------------------------------------------------------------
// CoreGraphics is oriented bottom-left whereas HIToolbox and Control Manager 
// are oriented top-left.  The function will translate a rectangle from 
// control orientation to graphics orientation.
inline CGRect TransformControlBoundsToDrawingBounds(CGRect inRect, float inPortHeight)
{
	inRect.origin.y = inPortHeight - (inRect.origin.y + inRect.size.height);
	return inRect;
}


#pragma mark _________RMSControl_________
//-----------------------------------------------------------------------------
//                           RMSControl
//-----------------------------------------------------------------------------
RMSControl::RMSControl(RMSBuddyEditor * inOwnerEditor, long inXpos, long inYpos, long inWidth, long inHeight, 
						long inControlRange, long inParamID)
:	ownerEditor(inOwnerEditor), width(inWidth), height(inHeight), 
	carbonControl(NULL), hasParameter(false)
{
	// create the position rectangle, taking into account the window offset of the AU view
	Rect boundsMacRect;
	boundsMacRect.left = inXpos + (short)(ownerEditor->GetXOffset());
	boundsMacRect.top = inYpos + (short)(ownerEditor->GetYOffset());
	boundsMacRect.right = boundsMacRect.left + inWidth;
	boundsMacRect.bottom = boundsMacRect.top + inHeight;

	// attempt to create a custom control using RMS Buddy Editor's custom control toolbox class
	if (CreateCustomControl(ownerEditor->GetCarbonWindow(), &boundsMacRect, ownerEditor->getControlClassSpec(), NULL, &carbonControl) == noErr)
	{
		SetControl32BitMinimum(carbonControl, 0);
		SetControl32BitMaximum(carbonControl, inControlRange);
		SetControl32BitValue(carbonControl, 0);
		ownerEditor->EmbedControl(carbonControl);

		// if a valid parameter ID was input, then we want to attach this control to a parameter
		if (inParamID >= 0)
		{
			hasParameter = true;
			// create an AUCarbonViewControl convenience object for this
			auvParam = CAAUParameter(ownerEditor->GetEditAudioUnit(), inParamID, kAudioUnitScope_Global, (AudioUnitElement)0);
		#if USE_AUCVCONTROL
			AUCarbonViewControl * auvc = new AUCarbonViewControl(ownerEditor, ownerEditor->GetParameterListener(), 
															AUCarbonViewControl::kTypeContinuous, auvParam, carbonControl);
			auvc->Bind();
			ownerEditor->AddAUCVControl(auvc);
		#else
			AUListenerAddParameter(ownerEditor->GetParameterListener(), ownerEditor, &auvParam);
		#endif
		}

		// this is done in the AUCarbonViewControl constructor, and we override it here
		// XXX this won't work in a 64-bit address space
		SetControlReference(carbonControl, (SInt32)this);
	}
}

//-----------------------------------------------------------------------------
RMSControl::~RMSControl()
{
	// release the Carbon ControlRef thingy
	if (carbonControl != NULL)
		DisposeControl(carbonControl);
	carbonControl = NULL;
}

//-----------------------------------------------------------------------------
// get the rectangular position of the control (0-oriented in compositing windows and 
// relative to the view's owner window in non-compositing windows)
CGRect RMSControl::getBoundsRect()
{
	if (carbonControl != NULL)
	{
		CGRect controlRect;
		OSStatus error = HIViewGetBounds(carbonControl, &controlRect);
		if (error == noErr)
			return controlRect;
	}
	return CGRectMake(0.0f, 0.0f, 0.0f, 0.0f);
}

//-----------------------------------------------------------------------------
// force a redraw of the control to occur
void RMSControl::redraw()
{
	if (carbonControl != NULL)
	{
		if ( ownerEditor->IsCompositWindow() )
			HIViewSetNeedsDisplay(carbonControl, true);
		else
			Draw1Control(carbonControl);
	}
}



#pragma mark _________RMSTextDisplay_________
#define MINUS_INFINITY_STRING	"-oo"
//-----------------------------------------------------------------------------
//                           RMSTextDisplay
//-----------------------------------------------------------------------------
RMSTextDisplay::RMSTextDisplay(RMSBuddyEditor * inOwnerEditor, long inXpos, long inYpos, long inWidth, long inHeight, 
					RMSColor inTextColor, RMSColor inBackColor, RMSColor inFrameColor, 
					const char * inFontName, float inFontSize, long inTextAlignment, long inParamID)
:	RMSControl(inOwnerEditor, inXpos, inYpos, inWidth, inHeight, 0x3FFF, inParamID), 
	textColor(inTextColor), backColor(inBackColor), frameColor(inFrameColor), 
	fontSize(inFontSize), textAlignment(inTextAlignment), 
	fontName(NULL), text(NULL)
{
	// allocate the font string and copy the input font name
	fontName = (char*) malloc(256);
	fontName[0] = 0;
	if (inFontName != NULL)
		strcpy(fontName, inFontName);

	// allocate the text display string and initialize it to be an empty string
	text = (char*) malloc(256);
	text[0] = 0;
}

//-----------------------------------------------------------------------------
RMSTextDisplay::~RMSTextDisplay()
{
	// release our strings
	if (fontName != NULL)
		free(fontName);
	fontName = NULL;

	if (text != NULL)
		free(text);
	text = NULL;
}

//-----------------------------------------------------------------------------
void RMSTextDisplay::draw(CGContextRef inContext, float inPortHeight)
{
	CGContextSetShouldAntialias(inContext, false);	// XXX maybe this is more efficient for the box drawing?

	// fill in the background color
	CGRect bounds = getBoundsRect();
	bounds = TransformControlBoundsToDrawingBounds(bounds, inPortHeight);
	CGContextSetRGBFillColor(inContext, (float)backColor.r/255.0f, (float)backColor.g/255.0f, (float)backColor.b/255.0f, 1.0f);
	CGContextFillRect(inContext, bounds);

	// draw the frame around the box
	CGContextSetRGBStrokeColor(inContext, (float)frameColor.r/255.0f, (float)frameColor.g/255.0f, (float)frameColor.b/255.0f, 1.0f);
	// Quartz draws lines on top of the pixel, so you need to move the coordinates to the middle of the pixel, 
	// and then also shrink the size accordingly
	CGRect box = CGRectMake(bounds.origin.x + 0.5f, bounds.origin.y + 0.5f, bounds.size.width - 1.0f, bounds.size.height - 1.0f);
	CGContextStrokeRectWithWidth(inContext, box, 1.0f);

	if ( isParameterAttached() )
	{
		float value = getAUVP()->GetValue();
		sprintf(text, "%.0f ms", value);
	}

	// draw the text
	CGContextSetShouldAntialias(inContext, true);	// now we want anti-aliasing
	CGContextSetShouldSmoothFonts(inContext, true);
	CGContextSelectFont(inContext, fontName, fontSize, kCGEncodingMacRoman);
	CGContextSetRGBFillColor(inContext, (float)textColor.r/255.0f, (float)textColor.g/255.0f, (float)textColor.b/255.0f, 1.0f);
	// do tricks to align the text if it's not left-aligned
	if (textAlignment != kTextAlign_left)
	{
		// draw an invisible string and then see where the drawing ends to determine the rendered text width
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
		// first draw the "-o" part
		CGContextShowTextAtPoint(inContext, bounds.origin.x, bounds.origin.y+4.0f, text, minusInfLength-1);
		// then draw the next "o" and the rest of the string, pushed back a little bit
		bounds.origin.x = CGContextGetTextPosition(inContext).x - 1.5f;
		char * text2 = &(text[minusInfLength-1]);
		CGContextShowTextAtPoint(inContext, bounds.origin.x, bounds.origin.y+4.0f, text2, strlen(text2));
	}
	else
		// draw the text regular-style
		CGContextShowTextAtPoint(inContext, bounds.origin.x, bounds.origin.y+4.0f, text, strlen(text));
}

//-----------------------------------------------------------------------------
// set the display text directly with a string
void RMSTextDisplay::setText(const char * inText)
{
	if (inText != NULL)
	{
		strcpy(text, inText);
		redraw();
	}
}

//-----------------------------------------------------------------------------
// given a linear amplitude value, set the display text with the dB-converted value
void RMSTextDisplay::setText_dB(float inLinearValue)
{
	// -infinity dB
	if (inLinearValue <= 0.0f)
		sprintf(text, MINUS_INFINITY_STRING);
	else
	{
		float dBvalue = 20.0f * (float)log10(inLinearValue);	// convert linear value to dB
		// add a plus sign to positive values
		if (dBvalue >= 0.01f)
			sprintf(text, "+%.2f", dBvalue);
		// 1 decimal precision for -100 or lower
		else if (fabsf(dBvalue) >= 100.0f)
			sprintf(text, "%.1f", dBvalue);
		// regular 2 decimal precision display
		else
			sprintf(text, "%.2f", dBvalue);
	}
	// append the units to the string
	strcat(text, " dB");

	redraw();
}

//-----------------------------------------------------------------------------
// set the display text with an integer value
void RMSTextDisplay::setText_int(long inValue)
{
	sprintf(text, "%ld", inValue);
	redraw();
}



#pragma mark _________RMSButton_________
//-----------------------------------------------------------------------------
//                           RMSButton
//-----------------------------------------------------------------------------
RMSButton::RMSButton(RMSBuddyEditor * inOwnerEditor, long inXpos, long inYpos, CGImageRef inImage)
:	RMSControl(inOwnerEditor, inXpos, inYpos, CGImageGetWidth(inImage), CGImageGetHeight(inImage)/2, 1), 
	buttonImage(inImage)
{
}

//-----------------------------------------------------------------------------
void RMSButton::draw(CGContextRef inContext, float inPortHeight)
{
	CGContextSetShouldAntialias(inContext, false);	// we more or less just want straight blitting of the image
	CGRect bounds = getBoundsRect();
	bounds = TransformControlBoundsToDrawingBounds(bounds, inPortHeight);
	// do this because the image is twice the size of the control, 
	// and if we don't use the actual image size, Quartz will stretch the image
	bounds.size.height *= 2;
	// offset to the top image frame if the button is in its non-pressed state
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



#pragma mark _________RMSSlider_________
//-----------------------------------------------------------------------------
//                           RMSSlider
//-----------------------------------------------------------------------------
RMSSlider::RMSSlider(RMSBuddyEditor * inOwnerEditor, long inParamID, long inXpos, long inYpos, long inWidth, long inHeight, 
					RMSColor inBackColor, RMSColor inFillColor)
:	RMSControl(inOwnerEditor, inXpos, inYpos, inWidth, inHeight, 0x3FFF, inParamID), 
	backColor(inBackColor), fillColor(inFillColor)
{
	borderWidth = 1;
}

//-----------------------------------------------------------------------------
RMSSlider::~RMSSlider()
{
}

//-----------------------------------------------------------------------------
void RMSSlider::draw(CGContextRef inContext, float inPortHeight)
{
	CGContextSetShouldAntialias(inContext, false);	// XXX maybe this is more efficient for the box drawing?

	// fill in the background color
	CGRect bounds = getBoundsRect();
	bounds = TransformControlBoundsToDrawingBounds(bounds, inPortHeight);
	CGContextSetRGBFillColor(inContext, (float)backColor.r/255.0f, (float)backColor.g/255.0f, (float)backColor.b/255.0f, 1.0f);
	CGContextFillRect(inContext, bounds);

	// draw the frame around the box
//	CGContextSetRGBStrokeColor(inContext, (float)frameColor.r/255.0f, (float)frameColor.g/255.0f, (float)frameColor.b/255.0f, 1.0f);
	// Quartz draws lines on top of the pixel, so you need to move the coordinates to the middle of the pixel, 
	// and then also shrink the size accordingly
//	CGRect box = CGRectInset(bounds, 0.5f, 0.5f);
//	CGContextStrokeRectWithWidth(inContext, box, 1.0f);

	// fill in the active area
	CGRect fillbox = CGRectInset(bounds, (float)borderWidth, (float)borderWidth);
	fillbox.size.width *= (float)GetControl32BitValue(getCarbonControl()) / (float)GetControl32BitMaximum(getCarbonControl());
	// naw, let's draw a faded frame on it first
	CGRect fillframe = CGRectInset(fillbox, 0.5f, 0.5f);
	CGContextSetRGBStrokeColor(inContext, (float)fillColor.r/255.0f, (float)fillColor.g/255.0f, (float)fillColor.b/255.0f, 1.0f);
	CGContextSetAlpha(inContext, 0.45f);
	CGContextStrokeRectWithWidth(inContext, fillframe, 1.0f);
	CGContextSetAlpha(inContext, 1.0f);
	fillbox = CGRectInset(fillbox, 1.0f, 1.0f);
	// really fill it
	CGContextSetRGBFillColor(inContext, (float)fillColor.r/255.0f, (float)fillColor.g/255.0f, (float)fillColor.b/255.0f, 1.0f);
	CGContextFillRect(inContext, fillbox);
}

//-----------------------------------------------------------------------------
void RMSSlider::mouseDown(long inXpos, long inYpos)
{
	mouseTrack(inXpos, inYpos);
}

//-----------------------------------------------------------------------------
void RMSSlider::mouseTrack(long inXpos, long inYpos)
{
	if (inXpos < borderWidth)
		inXpos = borderWidth;
	if ( inXpos > (width-borderWidth) )
		inXpos = width - borderWidth;
	float valueNorm = (float)(inXpos-borderWidth) / (float)(width-(borderWidth*2));

	SetControl32BitValue( getCarbonControl(), (SInt32)(valueNorm * (float)GetControl32BitMaximum(getCarbonControl())) );
}

//-----------------------------------------------------------------------------
void RMSSlider::mouseUp(long inXpos, long inYpos)
{
}






#pragma mark _________RMSBuddyEditor_________

//-----------------------------------------------------------------------------
// constants

const RMSColor kMyBrownColor = { 97, 73, 46 };
const RMSColor kMyLightBrownColor = { 146, 116, 98 };
const RMSColor kMyDarkBlueColor = { 54, 69, 115 };
const RMSColor kMyLightOrangeColor = { 219, 145, 85 };
const RMSColor kWhiteColor = { 255, 255, 255 };
const RMSColor kMyYellowColor = { 249, 249, 120 };

#define VALUE_DISPLAY_FONT	"Monaco"
#define VALUE_DISPLAY_FONT_SIZE	12.0f
#define LABEL_DISPLAY_FONT	"Lucida Grande"
#define LABEL_DISPLAY_FONT_SIZE	12.0f
#define SLIDER_LABEL_DISPLAY_FONT	"Lucida Grande"
#define SLIDER_LABEL_DISPLAY_FONT_SIZE	11.0f

#define kBackgroundColor		kMyLightBrownColor
#define kBackgroundFrameColor	kMyBrownColor
#define kLabelTextColor			kWhiteColor
#define kReadoutFrameColor		kMyBrownColor
#define kReadoutBoxColor		kMyLightOrangeColor
#define kReadoutTextColor		kMyDarkBlueColor
#define kSliderBackgroundColor	kMyDarkBlueColor
#define kSliderActiveColor		kMyYellowColor
#define kSliderLabelTextColor	kMyDarkBlueColor


//-----------------------------------------------------------------------------
// control positions and sizes
enum {
	kValueLabelX = 6,
	kValueLabelY = 27,
	kValueLabelWidth = 87,
	kValueLabelHeight = 17,

	kChannelLabelX = kValueLabelX + kValueLabelWidth + 9,
	kChannelLabelY = kValueLabelY - 22,
	kChannelLabelWidth = 75,
	kChannelLabelHeight = 17,

	kValueDisplayX = kChannelLabelX,
	kValueDisplayY = kValueLabelY,
	kValueDisplayWidth = kChannelLabelWidth,
	kValueDisplayHeight = 17,

	kXinc = kChannelLabelWidth + 15,
	kYinc = 33,

	kButtonX = kValueDisplayX - 1,
	kButtonY = kValueDisplayY + 1,

	kSliderX = 15,
	kSliderY = 159,
	kSliderHeight = 11,
	kSliderLabelOffsetX = 3,
	kSliderLabelX = kSliderX + kSliderLabelOffsetX,
	kSliderLabelY = kSliderY + kSliderHeight + 6,
	kSliderLabelHeight = 13,

	kBackgroundWidth = 151,
	kBackgroundHeight = kSliderLabelY + 9 + kSliderLabelHeight,//156,
};


//-----------------------------------------------------------------------------
// static function prototypes
static pascal OSStatus RmsControlEventHandler(EventHandlerCallRef, EventRef, void * inUserData);
static pascal OSStatus RmsWindowEventHandler(EventHandlerCallRef, EventRef, void * inUserData);
static void RmsParameterListenerProc(void * inUserData, void * inObject, const AudioUnitParameter *, Float32);
static void RmsPropertyListenerProc(void * inUserData, AudioUnit inComponentInstance, AudioUnitPropertyID inPropertyID, 
									AudioUnitScope inScope, AudioUnitElement inElement);


//-----------------------------------------------------------------------------
// macro for boring Component entry point stuff
COMPONENT_ENTRY(RMSBuddyEditor);


//-----------------------------------------------------------------------------
//                           RMSBuddyEditor
//-----------------------------------------------------------------------------
RMSBuddyEditor::RMSBuddyEditor(AudioUnitCarbonView inInstance)
:	AUCarbonViewBase(inInstance), 
	parameterListener(NULL)
{
	// we don't have a Component Instance of the DSP component until CreateUI, 
	// so we can't get the number of channels being analyzed and allocate control arrays yet
	numChannels = 0;

	// initialize the graphics pointers
	gResetButton = NULL;

	// initialize the controls pointers
	resetRMSbutton = NULL;
	resetPeakButton = NULL;
	windowSizeSlider = NULL;
	windowSizeLabel = NULL;
	windowSizeDisplay = NULL;
	// initialize the value display box pointers
	averageRMSDisplays = NULL;
	continualRMSDisplays = NULL;
	absolutePeakDisplays = NULL;
	continualPeakDisplays = NULL;
	averageRMSLabel = NULL;
	continualRMSLabel = NULL;
	absolutePeakLabel = NULL;
	continualPeakLabel = NULL;
	channelLabels = NULL;

	// initialize our control class and event handling related stuff
	controlClassSpec.defType = kControlDefObjectClass;
	controlClassSpec.u.classRef = NULL;
	controlHandlerUPP = NULL;
	windowEventHandlerUPP = NULL;
	windowEventEventHandlerRef = NULL;
	currentControl = NULL;
}

//-----------------------------------------------------------------------------
// this is where we actually construct the GUI
OSStatus RMSBuddyEditor::CreateUI(Float32 inXOffset, Float32 inYOffset)
{
	// register for draw events for our embedding pane so that we can draw the background
	EventTypeSpec paneEvents[] = {
		{ kEventClassControl, kEventControlDraw }
	};
	WantEventTypes(GetControlEventTarget(GetCarbonPane()), GetEventTypeCount(paneEvents), paneEvents);


// create our HIToolbox object class for common event handling amongst our custom Carbon Controls

	EventTypeSpec toolboxClassEvents[] = {
		{ kEventClassControl, kEventControlDraw }, 
		{ kEventClassControl, kEventControlClick }, 
		{ kEventClassControl, kEventControlValueFieldChanged }, 
	};

	ToolboxObjectClassRef newControlClass = NULL;
	controlHandlerUPP = NewEventHandlerUPP(RmsControlEventHandler);
	// this is sort of a hack to come up with a unique class ID name, so that we can instanciate multiple plugin instances
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

	// success
	controlClassSpec.u.classRef = newControlClass;


// create the window event handler that supplements the control event handler by tracking mouse dragging, mouseover controls, etc.
	setCurrentControl(NULL);	// make sure that it ain't nuthin
	EventTypeSpec controlMouseEvents[] = {
								  { kEventClassMouse, kEventMouseDragged }, 
								  { kEventClassMouse, kEventMouseUp }, 
								};
	windowEventHandlerUPP = NewEventHandlerUPP(RmsWindowEventHandler);
	InstallEventHandler(GetWindowEventTarget(GetCarbonWindow()), windowEventHandlerUPP, 
						GetEventTypeCount(controlMouseEvents), controlMouseEvents, this, &windowEventEventHandlerRef);



	// load our graphics resource from a PNG file in our plugin bundle's Resources sub-directory
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


	// create the parameter listener
	AUListenerCreate(RmsParameterListenerProc, this,
		CFRunLoopGetCurrent(), kCFRunLoopDefaultMode, 0.010f, // 10 ms
		&parameterListener);

	// install a parameter listener on the fake refresh-notification parameter
	timeToUpdateAUP.mAudioUnit = GetEditAudioUnit();
	timeToUpdateAUP.mScope = kAudioUnitScope_Global;
	timeToUpdateAUP.mElement = 0;
	timeToUpdateAUP.mParameterID = kTimeToUpdate;
	AUListenerAddParameter(parameterListener, this, &timeToUpdateAUP);


	// install AU property listeners for all of the special properties that we need to know about when they change
	AudioUnitAddPropertyListener(GetEditAudioUnit(), kNumChannelsProperty, RmsPropertyListenerProc, this);


	// setup creates and adds all of the UI content
	return setup();
}

//-----------------------------------------------------------------------------
RMSBuddyEditor::~RMSBuddyEditor()
{
	// free the graphics resource
	if (gResetButton != NULL)
		CGImageRelease(gResetButton);
	gResetButton = NULL;

	// if we created and installed the parameter listener, remove and dispose it now
	if (parameterListener != NULL)
	{
		AUListenerRemoveParameter(parameterListener, this, &timeToUpdateAUP);
		AUListenerDispose(parameterListener);
	}
	parameterListener = NULL;

	// if we created and installed the property listener, remove and dispose it now
	if (GetEditAudioUnit() != NULL)
		AudioUnitRemovePropertyListener(GetEditAudioUnit(), kNumChannelsProperty, RmsPropertyListenerProc);

	// remove our event handlers if we created them
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
// this function creates all of the controls for the UI and embeds them into the root pane control
OSStatus RMSBuddyEditor::setup()
{
	// first figure out how many channels of analysis data we will be displaying
	UInt32 dataSize = sizeof(numChannels);
	if (AudioUnitGetProperty(GetEditAudioUnit(), kNumChannelsProperty, kAudioUnitScope_Global, (AudioUnitElement)0, &numChannels, &dataSize) 
			!= noErr)
		numChannels = 0;
	// there's not really anything for us to do in this situation (which is crazy and shouldn't happen anyway)
	if (numChannels == 0)
		return kAudioUnitErr_FailedInitialization;

	// allocate arrays of the per-channel control object pointers
	averageRMSDisplays = (RMSTextDisplay**) malloc(numChannels * sizeof(RMSTextDisplay*));
	continualRMSDisplays = (RMSTextDisplay**) malloc(numChannels * sizeof(RMSTextDisplay*));
	absolutePeakDisplays = (RMSTextDisplay**) malloc(numChannels * sizeof(RMSTextDisplay*));
	continualPeakDisplays = (RMSTextDisplay**) malloc(numChannels * sizeof(RMSTextDisplay*));
	channelLabels = (RMSTextDisplay**) malloc(numChannels * sizeof(RMSTextDisplay*));
	// and initialized the control object pointers in the arrays
	for (unsigned long ch=0; ch < numChannels; ch++)
	{
		averageRMSDisplays[ch] = NULL;
		continualRMSDisplays[ch] = NULL;
		absolutePeakDisplays[ch] = NULL;
		continualPeakDisplays[ch] = NULL;
		channelLabels[ch] = NULL;
	}


//--initialize the text displays---------------------------------------------

	long xpos = kValueDisplayX;
	long ypos = kValueDisplayY;

#define VALUE_TEXT_DISPLAY	RMSTextDisplay(this, xpos, ypos, kValueDisplayWidth, kValueDisplayHeight, kReadoutTextColor, kReadoutBoxColor, kReadoutFrameColor, VALUE_DISPLAY_FONT, VALUE_DISPLAY_FONT_SIZE, kTextAlign_center)

	for (unsigned long ch=0; ch < numChannels; ch++)
	{
		// position back to the top of the channel column
		ypos = kValueDisplayY;

		// average RMS read-out
		averageRMSDisplays[ch] = new VALUE_TEXT_DISPLAY;

		// continual RMS read-out
		ypos += kYinc;
		continualRMSDisplays[ch] = new VALUE_TEXT_DISPLAY;

		// absolute peak read-out
		ypos += kYinc;
		absolutePeakDisplays[ch] = new VALUE_TEXT_DISPLAY;

		// continual peak read-out
		ypos += kYinc;
		continualPeakDisplays[ch] = new VALUE_TEXT_DISPLAY;

		// move position right to the the next channel column
		xpos += kXinc;
	}

#undef VALUE_TEXT_DISPLAY

	// display the proper current dynamics data value read-outs
	updateDisplays();


	xpos = kValueLabelX;
	ypos = kValueLabelY;

#define LABEL_TEXT_DISPLAY	RMSTextDisplay(this, xpos, ypos, kValueLabelWidth, kValueLabelHeight, kLabelTextColor, kBackgroundColor, kBackgroundColor, LABEL_DISPLAY_FONT, LABEL_DISPLAY_FONT_SIZE, kTextAlign_right)

	// the words "average RMS"
	averageRMSLabel = new LABEL_TEXT_DISPLAY;
	averageRMSLabel->setText("average RMS");

	// the words "continual RMS"
	ypos += kYinc;
	continualRMSLabel = new LABEL_TEXT_DISPLAY;
	continualRMSLabel->setText("continual RMS");

	// the words "absolute peak"
	ypos += kYinc;
	absolutePeakLabel = new LABEL_TEXT_DISPLAY;
	absolutePeakLabel->setText("absolute peak");

	// the words "continual peak"
	ypos += kYinc;
	continualPeakLabel = new LABEL_TEXT_DISPLAY;
	continualPeakLabel->setText("continual peak");

#undef LABEL_TEXT_DISPLAY

	xpos = kChannelLabelX;
	ypos = kChannelLabelY;

	// the label(s) for the the name(s) of the channel column(s)
	for (unsigned long ch=0; ch < numChannels; ch++)
	{
		channelLabels[ch] = new RMSTextDisplay(this, xpos, ypos, kChannelLabelWidth, kChannelLabelHeight, kLabelTextColor, 
							kBackgroundColor, kBackgroundColor, LABEL_DISPLAY_FONT, LABEL_DISPLAY_FONT_SIZE, kTextAlign_center);

		if (numChannels <= 1)
			channelLabels[ch]->setText(" ");
		else if (numChannels == 2)
		{
			if (ch == 0)
				channelLabels[ch]->setText("left");
			else
				channelLabels[ch]->setText("right");
		}
		else
			channelLabels[ch]->setText_int(ch+1);

		xpos += kXinc;
	}


//--initialize the buttons-----------------------------------------------

	// reset continual RMS button
	resetRMSbutton = new RMSButton(this, kButtonX + (kXinc*numChannels), kButtonY, gResetButton);

	// reset peak button
	resetPeakButton = new RMSButton(this, kButtonX + (kXinc*numChannels), kButtonY + (kYinc*2), gResetButton);


//--initialize the slider-----------------------------------------------

	long sliderWidth = kBackgroundWidth + (kXinc*numChannels) - (kSliderX*2);
	windowSizeSlider = new RMSSlider(this, kAnalysisFrameSize, kSliderX, kSliderY, sliderWidth, kSliderHeight, 
										kSliderBackgroundColor, kSliderActiveColor);
	updateWindowSize(windowSizeSlider->getAUVP()->GetValue(), windowSizeSlider);

	sliderWidth -= kSliderLabelOffsetX * 2;
	// the words "analysis window size"
	long sliderLabelWidth = (2 * sliderWidth) / 3;
	windowSizeLabel = new RMSTextDisplay(this, kSliderLabelX, kSliderLabelY, sliderLabelWidth, kSliderLabelHeight, 
										kSliderLabelTextColor, kBackgroundColor, kBackgroundColor, 
										SLIDER_LABEL_DISPLAY_FONT, SLIDER_LABEL_DISPLAY_FONT_SIZE, kTextAlign_left);
	windowSizeLabel->setText("analysis window size");

	// the analysis window size value read-out
	long sliderDisplayWidth = sliderWidth - sliderLabelWidth;
	windowSizeDisplay = new RMSTextDisplay(this, kSliderLabelX + sliderLabelWidth, kSliderLabelY, sliderDisplayWidth, kSliderLabelHeight, 
										kSliderLabelTextColor, kBackgroundColor, kBackgroundColor, 
										SLIDER_LABEL_DISPLAY_FONT, SLIDER_LABEL_DISPLAY_FONT_SIZE, kTextAlign_right, kAnalysisFrameSize);


	// set size of the background embedding pane control to fit the entire UI display
	SizeControl(GetCarbonPane(), kBackgroundWidth + (kXinc*numChannels), kBackgroundHeight);


	return noErr;
}

//-----------------------------------------------------------------------------
// this cleans up what setup() creates, which is basically just all of the UI controls
void RMSBuddyEditor::cleanup()
{
	// free the controls
#define SAFE_DELETE_CONTROL(ctrl)	{ if (ctrl != NULL)   delete ctrl;   ctrl = NULL; }
#define SAFE_FREE_ARRAY(array)	{ if (array != NULL)   free(array);   array = NULL; }
	SAFE_DELETE_CONTROL(resetRMSbutton)
	SAFE_DELETE_CONTROL(resetPeakButton)
	for (unsigned long ch=0; ch < numChannels; ch++)
	{
		if (averageRMSDisplays != NULL)
			SAFE_DELETE_CONTROL(averageRMSDisplays[ch])
		if (continualRMSDisplays != NULL)
			SAFE_DELETE_CONTROL(continualRMSDisplays[ch])
		if (absolutePeakDisplays != NULL)
			SAFE_DELETE_CONTROL(absolutePeakDisplays[ch])
		if (continualPeakDisplays != NULL)
			SAFE_DELETE_CONTROL(continualPeakDisplays[ch])
		if (channelLabels != NULL)
			SAFE_DELETE_CONTROL(channelLabels[ch])
	}
	SAFE_FREE_ARRAY(averageRMSDisplays)
	SAFE_FREE_ARRAY(continualRMSDisplays)
	SAFE_FREE_ARRAY(absolutePeakDisplays)
	SAFE_FREE_ARRAY(continualPeakDisplays)
	SAFE_FREE_ARRAY(channelLabels)
	SAFE_DELETE_CONTROL(averageRMSLabel)
	SAFE_DELETE_CONTROL(continualRMSLabel)
	SAFE_DELETE_CONTROL(absolutePeakLabel)
	SAFE_DELETE_CONTROL(continualPeakLabel)
	SAFE_DELETE_CONTROL(windowSizeSlider)
	SAFE_DELETE_CONTROL(windowSizeLabel)
	SAFE_DELETE_CONTROL(windowSizeDisplay)
#undef SAFE_DELETE_CONTROL
}


//-----------------------------------------------------------------------------
// inEvent is expected to be an event of the class kEventClassControl.  
// The return value is true if a CGContextRef was available from the event parameters 
// (which happens with compositing windows) or false if the CGContext had to be created 
// for the control's window port QuickDraw-style.  
bool InitControlDrawingContext(EventRef inEvent, CGContextRef & outContext, CGrafPtr & outPort, float & outPortHeight)
{
	outContext = NULL;
	outPort = NULL;
	if (inEvent == NULL)
		return false;

	bool gotAutoContext = false;
	OSStatus error;

	// if we received a graphics port parameter, use that...
	error = GetEventParameter(inEvent, kEventParamGrafPort, typeGrafPtr, NULL, sizeof(CGrafPtr), NULL, &outPort);
	// ... otherwise use the current graphics port
	if ( (error != noErr) || (outPort == NULL) )
		GetPort(&outPort);
	if (outPort == NULL)
		return false;
	Rect portBounds;
	GetPortBounds(outPort, &portBounds);
	outPortHeight = (float) (portBounds.bottom - portBounds.top);

	// set up the CG context
	// if we are in compositing mode, then we can get a CG context already set up and clipped and whatnot
	error = GetEventParameter(inEvent, kEventParamCGContextRef, typeCGContextRef, NULL, sizeof(CGContextRef), NULL, &outContext);
	if ( (error == noErr) && (outContext != NULL) )
		gotAutoContext = true;
	else
	{
		error = QDBeginCGContext(outPort, &outContext);
		if ( (error != noErr) || (outContext == NULL) )
		{
			outContext = NULL;	// probably crazy, but in case there's an error, and yet a non-null result for the context
			return false;
		}
	}
	CGContextSaveGState(outContext);
	SyncCGContextOriginWithPort(outContext, outPort);

	// define the clipping region if we are not compositing and had to create our own context
	if (!gotAutoContext)
	{
		ControlRef carbonControl = NULL;
		error = GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, NULL, sizeof(ControlRef), NULL, &carbonControl);
		if ( (error == noErr) && (carbonControl != NULL) )
		{
			CGRect clipRect;
			error = HIViewGetBounds(carbonControl, &clipRect);
			if (error == noErr)
			{
				clipRect = TransformControlBoundsToDrawingBounds(clipRect, outPortHeight);
				CGContextClipToRect(outContext, clipRect);
			}
		}
	}

	return gotAutoContext;
}

//-----------------------------------------------------------------------------
// inGotAutoContext should be the return value that you got from InitControlDrawingContext().  
// It is true if inContext came with the control event (compositing window behavior) or 
// false if inContext was created via QDBeginCGContext(), and hence needs to be disposed.  
void CleanupControlDrawingContext(CGContextRef & inContext, bool inGotAutoContext, CGrafPtr inPort)
{
	if (inContext == NULL)
		return;

	CGContextRestoreGState(inContext);
	CGContextSynchronize(inContext);

	if ( !inGotAutoContext && (inPort != NULL) )
		QDEndCGContext(inPort, &inContext);
}

//-----------------------------------------------------------------------------
bool RMSBuddyEditor::HandleEvent(EventRef inEvent)
{
	// we redraw the background when we catch a draw event for the root pane control
	if ( (GetEventClass(inEvent) == kEventClassControl) && (GetEventKind(inEvent) == kEventControlDraw) )
	{
		ControlRef carbonControl = NULL;
		OSStatus error = GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, NULL, sizeof(ControlRef), NULL, &carbonControl);
		if ( (carbonControl == GetCarbonPane()) && (carbonControl != NULL) && (error == noErr) )
		{
			CGRect controlBounds;
			error = HIViewGetBounds(carbonControl, &controlBounds);
			if (error != noErr)
				return false;
			CGContextRef context = NULL;
			CGrafPtr windowPort = NULL;
			float portHeight;
			bool gotAutoContext = InitControlDrawingContext(inEvent, context, windowPort, portHeight);
			if ( (context == NULL) || (windowPort == NULL) )
				return false;

			controlBounds = TransformControlBoundsToDrawingBounds(controlBounds, portHeight);
			// fill in the background color
			CGContextSetRGBFillColor(context, (float)kBackgroundColor.r/255.0f, (float)kBackgroundColor.g/255.0f, 
										(float)kBackgroundColor.b/255.0f, 1.0f);
			CGContextFillRect(context, controlBounds);

			// draw the frame around the box
			CGContextSetRGBStrokeColor(context, (float)kBackgroundFrameColor.r/255.0f, (float)kBackgroundFrameColor.g/255.0f, 
										(float)kBackgroundFrameColor.b/255.0f, 1.0f);
			// Quartz draws lines on top of the pixel, so you need to move the coordinates to the middle of the pixel, 
			// and then also shrink the size accordingly
			CGRect box = CGRectInset(controlBounds, 0.5f,  0.5f);
			CGContextSetLineWidth(context, 1.0f);
			CGContextStrokeRect(context, box);
			// draw a couple more lighter lines to fade the border
			box = CGRectInset(box, 1.0f, 1.0f);
			CGContextSetAlpha(context, 0.27f);
			CGContextStrokeRect(context, box);
			box = CGRectInset(box, 1.0f, 1.0f);
			CGContextSetAlpha(context, 0.081f);
			CGContextStrokeRect(context, box);

			CleanupControlDrawingContext(context, gotAutoContext, windowPort);
			return true;
		}
	}

	// let the parent class handle any other events
	return AUCarbonViewBase::HandleEvent(inEvent);
}

//-----------------------------------------------------------------------------
static pascal OSStatus RmsWindowEventHandler(EventHandlerCallRef myHandler, EventRef inEvent, void * inUserData)
{
	// make sure that it's the correct event class
	if (GetEventClass(inEvent) != kEventClassMouse)
		return eventClassIncorrectErr;

	RMSBuddyEditor * ourOwnerEditor = (RMSBuddyEditor*) inUserData;

	// get the mouse location event parameter
	HIPoint mouseLocation_f;
	GetEventParameter(inEvent, kEventParamMouseLocation, typeHIPoint, NULL, sizeof(HIPoint), NULL, &mouseLocation_f);
	long mouseX = (long) mouseLocation_f.x;
	long mouseY = (long) mouseLocation_f.y;


	// see if a control was clicked on (we only use this event handling to track control mousing)
	RMSControl * ourRMSControl = ourOwnerEditor->getCurrentControl();
	if (ourRMSControl == NULL)
		return eventNotHandledErr;

	// orient the mouse coordinates as though the control were at 0, 0 (for convenience)
	Rect controlBounds;
	GetControlBounds(ourRMSControl->getCarbonControl(), &controlBounds);
	Rect windowBounds;	// window content region
	WindowRef window;
	GetEventParameter(inEvent, kEventParamWindowRef, typeWindowRef, NULL, sizeof(WindowRef), NULL, &window);
	GetWindowBounds(window, kWindowGlobalPortRgn, &windowBounds);
	if ( ourOwnerEditor->IsCompositWindow() )
	{
		Rect paneBounds;
		GetControlBounds(ourOwnerEditor->GetCarbonPane(), &paneBounds);
		OffsetRect(&windowBounds, paneBounds.left, paneBounds.top);
	}
	mouseX -= controlBounds.left + windowBounds.left;
	mouseY -= controlBounds.top + windowBounds.top;


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

		// do this to make Logic's touch automation work
		if ( ourRMSControl->isParameterAttached() )
		{
			CAAUParameter * ourAUVP = ourRMSControl->getAUVP();
			// do the new-fangled way, if it's available on the user's system
			if (AUEventListenerNotify != NULL)
			{
				AudioUnitEvent paramEvent;
				memset(&paramEvent, 0, sizeof(paramEvent));
				paramEvent.mEventType = kAudioUnitEvent_EndParameterChangeGesture;
				paramEvent.mArgument.mParameter = *ourAUVP;
				AUEventListenerNotify(NULL, NULL, &paramEvent);
			}
			// as a back-up, also still do the old way, until it's enough obsolete
			ourOwnerEditor->TellListener(*ourAUVP, kAudioUnitCarbonViewEvent_MouseUpInControl, NULL);
		}

		return noErr;
	}

	return eventKindIncorrectErr;
}

//-----------------------------------------------------------------------------
static pascal OSStatus RmsControlEventHandler(EventHandlerCallRef myHandler, EventRef inEvent, void * inUserData)
{
	// make sure that it's the correct event class
	if (GetEventClass(inEvent) != kEventClassControl)
		return eventClassIncorrectErr;

	// get the Carbon Control reference event parameter
	ControlRef ourCarbonControl = NULL;
	if (GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, NULL, sizeof(ControlRef), NULL, &ourCarbonControl) != noErr)
		return eventNotHandledErr;
	if (ourCarbonControl == NULL)
		return eventNotHandledErr;
	// now try to get a pointer to one of our control objects from that ControlRef
	RMSControl * ourRMSControl = (RMSControl*) GetControlReference(ourCarbonControl);
	if (ourRMSControl == NULL)
		return eventNotHandledErr;
	RMSBuddyEditor * ourOwnerEditor = (RMSBuddyEditor*) inUserData;

	switch (GetEventKind(inEvent))
	{
		case kEventControlDraw:
			{
				CGContextRef context = NULL;
				CGrafPtr windowPort = NULL;
				float portHeight;
				bool gotAutoContext = InitControlDrawingContext(inEvent, context, windowPort, portHeight);
				if ( (context == NULL) || (windowPort == NULL) )
					return eventNotHandledErr;

				// draw
				ourRMSControl->draw(context, portHeight);

				CleanupControlDrawingContext(context, gotAutoContext, windowPort);
			}
			return noErr;

		case kEventControlClick:
			{
				// get the mouse location event parameter
				HIPoint mouseLocation_f;
				GetEventParameter(inEvent, kEventParamMouseLocation, typeHIPoint, NULL, sizeof(HIPoint), NULL, &mouseLocation_f);
				long mouseX = (long) mouseLocation_f.x;
				long mouseY = (long) mouseLocation_f.y;

				// orient the mouse coordinates as though the control were at 0, 0 (for convenience)
				Rect controlBounds;
				GetControlBounds(ourCarbonControl, &controlBounds);
				Rect windowBounds;	// window content region
				GetWindowBounds( GetControlOwner(ourCarbonControl), kWindowGlobalPortRgn, &windowBounds );
				if ( ourOwnerEditor->IsCompositWindow() )
				{
					Rect paneBounds;
					GetControlBounds(ourOwnerEditor->GetCarbonPane(), &paneBounds);
					OffsetRect(&windowBounds, paneBounds.left, paneBounds.top);
				}
				mouseX -= controlBounds.left + windowBounds.left;
				mouseY -= controlBounds.top + windowBounds.top;

				ourRMSControl->mouseDown(mouseX, mouseY);

#if !USE_AUCVCONTROL
				// do this to make Logic's touch automation work
				if ( ourRMSControl->isParameterAttached() )
				{
					CAAUParameter * ourAUVP = ourRMSControl->getAUVP();
					// do the new-fangled way, if it's available on the user's system
					if (AUEventListenerNotify != NULL)
					{
						AudioUnitEvent paramEvent;
						memset(&paramEvent, 0, sizeof(paramEvent));
						paramEvent.mEventType = kAudioUnitEvent_BeginParameterChangeGesture;
						paramEvent.mArgument.mParameter = *ourAUVP;
						AUEventListenerNotify(NULL, NULL, &paramEvent);
					}
					// as a back-up, also still do the old way, until it's enough obsolete
					ourOwnerEditor->TellListener(*ourAUVP, kAudioUnitCarbonViewEvent_MouseDownInControl, NULL);
				}
#endif

				// indicate that this control is being moused (for our mouse tracking handler)
				ourOwnerEditor->setCurrentControl(ourRMSControl);
			}
			return noErr;

		// the ControlValue, ControlMin, or ControlMax has changed
		case kEventControlValueFieldChanged:
			ourOwnerEditor->handleControlValueChange(ourRMSControl, GetControl32BitValue(ourCarbonControl));
			if ( ourOwnerEditor->IsCompositWindow() )
				ourRMSControl->redraw();
			return noErr;

		default:
			return eventKindIncorrectErr;
	}
}

//-----------------------------------------------------------------------------
// this gets called when the DSP component sends notification via the kTimeToUpdate parameter
static void RmsParameterListenerProc(void * inUserData, void * inObject, const AudioUnitParameter * inParameter, Float32 inValue)
{
	RMSBuddyEditor * bud = (RMSBuddyEditor*) inUserData;
	if ( (bud != NULL) && (inParameter != NULL) )
	{
		switch (inParameter->mParameterID)
		{
			case kAnalysisFrameSize:
				bud->updateWindowSize(inValue, (RMSControl*)inObject);
				break;
			case kTimeToUpdate:
				bud->updateDisplays();	// refresh the value displays
				break;
			default:
				break;
		}
	}
}

//-----------------------------------------------------------------------------
// this gets called when the DSP component sends a property-changed notification of the kNumChannelsProperty property
static void RmsPropertyListenerProc(void * inUserData, AudioUnit inComponentInstance, AudioUnitPropertyID inPropertyID, 
									AudioUnitScope inScope, AudioUnitElement inElement)
{
	RMSBuddyEditor * bud = (RMSBuddyEditor*) inUserData;
	// when the number of channels changes, we tear down the existing GUI layout and 
	// then recreate it according to the new number of channels
	if ( (bud != NULL) && (inPropertyID == kNumChannelsProperty) )
	{
		bud->cleanup();	// tear it down
		bud->setup();	// rebuild
	}
}

//-----------------------------------------------------------------------------
// refresh the value displays
void RMSBuddyEditor::updateDisplays()
{
	RmsBuddyDynamicsData request;
	UInt32 dataSize = sizeof(request);

	// get the dynamics data values for each channel being analyzed
	for (unsigned long ch=0; ch < numChannels; ch++)
	{
		request.channel = ch;
		// if getting the property fails, initialize to all zeroes so it's usable anyway
		if (AudioUnitGetProperty(GetEditAudioUnit(), kDynamicsDataProperty, kAudioUnitScope_Global, 0, &request, &dataSize) != noErr)
		{
			request.averageRMS = request.continualRMS = 0.0;
			request.absolutePeak = request.continualPeak = 0.0f;
		}
#define SAFE_SET_TEXT(ctrl, val)	\
	if (ctrl != NULL)	\
	{	\
		if (ctrl[ch] != NULL)	\
			ctrl[ch]->setText_dB(val);	\
	}
		// update the values being displayed
		SAFE_SET_TEXT(averageRMSDisplays, request.averageRMS)
		SAFE_SET_TEXT(continualRMSDisplays, request.continualRMS)
		SAFE_SET_TEXT(absolutePeakDisplays, request.absolutePeak)
		SAFE_SET_TEXT(continualPeakDisplays, request.continualPeak)
#undef SAFE_SET_TEXT

	}
}

//-----------------------------------------------------------------------------
// update analysis window size parameter controls
void RMSBuddyEditor::updateWindowSize(Float32 inParamValue, RMSControl * inRMSControl)
{
/*
if (inRMSControl == windowSizeSlider) fprintf(stderr, "object = slider\n");
else if (inRMSControl == windowSizeDisplay) fprintf(stderr, "object = display\n");
else if (inRMSControl == NULL) fprintf(stderr, "object = NULL\n");
else fprintf(stderr, "object = %lu\n", (unsigned long)inRMSControl);
*/
//	if ( (windowSizeSlider != NULL) && (inRMSControl == windowSizeSlider) )
	if (windowSizeSlider != NULL)
	{
		float fmin = windowSizeSlider->getAUVP()->ParamInfo().minValue;
		float fmax = windowSizeSlider->getAUVP()->ParamInfo().maxValue;
		float valueNorm = (inParamValue - fmin) / (fmax - fmin);
		valueNorm = (float) sqrt(valueNorm);

		ControlRef carbonControl = windowSizeSlider->getCarbonControl();
		SInt32 cmin = GetControl32BitMinimum(carbonControl);
		SInt32 cmax = GetControl32BitMaximum(carbonControl);
		SInt32 cval = (SInt32) (valueNorm * (float)((cmax - cmin) + cmin) + 0.5f);
		SetControl32BitValue(carbonControl, cval);
	}

//	if ( (windowSizeDisplay != NULL) && (inRMSControl == windowSizeDisplay) )
	if (windowSizeDisplay != NULL)
		windowSizeDisplay->redraw();
}

//-----------------------------------------------------------------------------
void RMSBuddyEditor::handleControlValueChange(RMSControl * inControl, SInt32 inControlValue)
{
	if (inControl == NULL)
		return;

	// if the button was pressed, the value will be 1 (it's 0 when the button is released)
	if (inControl == resetRMSbutton)
	{
		if (inControlValue > 0)
		{
			resetRMS();
			updateDisplays();
		}
	}

	else if (inControl == resetPeakButton)
	{
		if (inControlValue > 0)
		{
			resetPeak();
			updateDisplays();
		}
	}

	else if (inControl == windowSizeSlider)
	{
		ControlRef carbonControl = windowSizeSlider->getCarbonControl();
		SInt32 cmin = GetControl32BitMinimum(carbonControl);
		SInt32 cmax = GetControl32BitMaximum(carbonControl);
		Float32 controlValue = (Float32)(inControlValue - cmin) / (Float32)(cmax - cmin);

		Float32 fmin = windowSizeSlider->getAUVP()->ParamInfo().minValue;
		Float32 fmax = windowSizeSlider->getAUVP()->ParamInfo().maxValue;
		Float32 paramValue = ( controlValue*controlValue * (fmax - fmin) ) + fmin;
		windowSizeSlider->getAUVP()->SetValue(parameterListener, windowSizeSlider, paramValue);
	}
}

//-----------------------------------------------------------------------------
// send a message to the DSP component to reset average RMS
void RMSBuddyEditor::resetRMS()
{
	char nud;	// irrelavant, no input data is actually needed, but SetProperty will fail without something
	AudioUnitSetProperty(GetEditAudioUnit(), kResetRMSProperty, kAudioUnitScope_Global, (AudioUnitElement)0, &nud, sizeof(char));
}

//-----------------------------------------------------------------------------
// send a message to the DSP component to reset absolute peak
void RMSBuddyEditor::resetPeak()
{
	char nud;	// irrelavant, no input data is actually needed, but SetProperty will fail without something
	AudioUnitSetProperty(GetEditAudioUnit(), kResetPeakProperty, kAudioUnitScope_Global, (AudioUnitElement)0, &nud, sizeof(char));
}
