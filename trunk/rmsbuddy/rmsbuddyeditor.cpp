/*--------------- by Marc Poirier  ][  June 2001 + February 2003 + November 2003 --------------*/

#include "rmsbuddyeditor.h"
#include "rmsbuddy.h"

#include <c.h>  // for sizeofA


// I want the analysis window size slider to have a squared-scaled value distribution curve, 
// so it seems that AUCarbonViewControl won't be able to do what I want.
// XXX That's not true anymore, but AUCarbonViewControl still has problems worth avoiding, I think.
//#define USE_AUCVCONTROL


//-----------------------------------------------------------------------------
// CoreGraphics is oriented bottom-left whereas HIToolbox and Control Manager 
// are oriented top-left.  The function will translate a rectangle from 
// control orientation to graphics orientation.
inline CGRect TransformControlBoundsToDrawingBounds(CGRect inRect, long inPortHeight)
{
	if (inPortHeight > 0)	// 0 signifies a compositing window, which is already top-left-oriented
		inRect.origin.y = (float)inPortHeight - (inRect.origin.y + inRect.size.height);
	return inRect;
}


#pragma mark -
#pragma mark RMSControl
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
		#ifdef USE_AUCVCONTROL
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
		OSStatus status = HIViewGetBounds(carbonControl, &controlRect);
		if (status == noErr)
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



#pragma mark -
#pragma mark RMSTextDisplay
//-----------------------------------------------------------------------------
//                           RMSTextDisplay
//-----------------------------------------------------------------------------
RMSTextDisplay::RMSTextDisplay(RMSBuddyEditor * inOwnerEditor, long inXpos, long inYpos, long inWidth, long inHeight, 
					RMSColor inTextColor, RMSColor inBackColor, RMSColor inFrameColor, 
					ThemeFontID inFontID, HIThemeTextHorizontalFlush inTextAlignment, long inParamID)
:	RMSControl(inOwnerEditor, inXpos, inYpos, inWidth, inHeight, 0x3FFF, inParamID), 
	textColor(inTextColor), backColor(inBackColor), frameColor(inFrameColor), 
	fontID(inFontID), textAlignment(inTextAlignment), 
	text(NULL)
{
}

//-----------------------------------------------------------------------------
RMSTextDisplay::~RMSTextDisplay()
{
	// release our string
	if (text != NULL)
		CFRelease(text);
	text = NULL;
}

//-----------------------------------------------------------------------------
void RMSTextDisplay::draw(CGContextRef inContext, long inPortHeight)
{
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
		if (text != NULL)
			CFRelease(text);
		text = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%.0f ms"), value);
	}

	// draw the text
	CGContextSetRGBFillColor(inContext, (float)textColor.r/255.0f, (float)textColor.g/255.0f, (float)textColor.b/255.0f, 1.0f);
	if (text != NULL)
	{
		// this function is only available in Mac OS X 10.3 or higher
		if (HIThemeDrawTextBox != NULL)
		{
			HIThemeTextInfo textInfo;
			memset(&textInfo, 0, sizeof(textInfo));
			textInfo.version = 0;
			textInfo.state = kThemeStateActive;
			textInfo.fontID = fontID;
			textInfo.horizontalFlushness = textAlignment;
			textInfo.verticalFlushness = kHIThemeTextVerticalFlushCenter;
			textInfo.truncationPosition = kHIThemeTextTruncationEnd;
			textInfo.truncationMaxLines = 1;
			textInfo.options = 0;
			HIThemeOrientation contextOrientation = kHIThemeOrientationNormal;
			if (! ownerEditor->IsCompositWindow() )
				contextOrientation = kHIThemeOrientationInverted;

			HIThemeDrawTextBox(text, &bounds, &textInfo, inContext, contextOrientation);
		}
	}
}

//-----------------------------------------------------------------------------
// set the display text directly with a string
void RMSTextDisplay::setText(CFStringRef inText)
{
	if (text != NULL)
		CFRelease(text);
	text = inText;
	if (text != NULL)
		CFRetain(text);

	redraw();
}

//-----------------------------------------------------------------------------
// given a linear amplitude value, set the display text with the dB-converted value
void RMSTextDisplay::setText_dB(float inLinearValue)
{
	if (text != NULL)
		CFRelease(text);

	// -infinity dB
	if (inLinearValue <= 0.0f)
	{
		const UniChar minusInfinity[] = { '-', 0x221E };
		text = CFStringCreateWithCharacters(kCFAllocatorDefault, minusInfinity, sizeofA(minusInfinity));
	}
	else
	{
		float dBvalue = 20.0f * (float)log10(inLinearValue);	// convert linear value to dB
		CFStringRef formatString;
		// add a plus sign to positive values
		if (dBvalue >= 0.01f)
			formatString = CFSTR("+%.2f");
		// 1 decimal precision for -100 or lower
		else if (fabsf(dBvalue) >= 100.0f)
			formatString = CFSTR("%.1f");
		// regular 2 decimal precision display
		else
			formatString = CFSTR("%.2f");
		text = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, formatString, dBvalue);
	}

	if (text != NULL)
	{
		CFMutableStringRef mutableText = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, text);
		if (mutableText != NULL)
		{
			CFStringAppend(mutableText, CFSTR(" dB"));
			CFRelease(text);
			text = mutableText;
		}
	}

	redraw();
}

//-----------------------------------------------------------------------------
// set the display text with an integer value
void RMSTextDisplay::setText_int(long inValue)
{
	if (text != NULL)
		CFRelease(text);
	text = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%ld"), inValue);

	redraw();
}



#pragma mark -
#pragma mark RMSButton
//-----------------------------------------------------------------------------
//                           RMSButton
//-----------------------------------------------------------------------------
RMSButton::RMSButton(RMSBuddyEditor * inOwnerEditor, long inXpos, long inYpos, CGImageRef inImage)
:	RMSControl(inOwnerEditor, inXpos, inYpos, CGImageGetWidth(inImage), CGImageGetHeight(inImage)/2, 1), 
	buttonImage(inImage)
{
}

//-----------------------------------------------------------------------------
void RMSButton::draw(CGContextRef inContext, long inPortHeight)
{
	CGRect bounds = getBoundsRect();
	bounds = TransformControlBoundsToDrawingBounds(bounds, inPortHeight);
	// do this because the image is twice the size of the control, 
	// and if we don't use the actual image size, Quartz will stretch the image
	bounds.size.height *= 2;
	// the indexing positioning is reversed in a composite window, so compensate here
	if ( ownerEditor->IsCompositWindow() )
		bounds.origin.y += (float)height;
	// offset to the top image frame if the button is in its non-pressed state
	if ( GetControl32BitValue(getCarbonControl()) == 0 )
		bounds.origin.y -= (float)height;
	// the image drawing is upside-down in compositing windows unless we do this transformation
	if ( ownerEditor->IsCompositWindow() )
	{
		CGContextTranslateCTM(inContext, 0.0f, bounds.size.height);
		CGContextScaleCTM(inContext, 1.0f, -1.0f);
	}
	CGContextDrawImage(inContext, bounds, buttonImage);
}

//-----------------------------------------------------------------------------
void RMSButton::mouseDown(float inXpos, float inYpos)
{
	SetControl32BitValue(getCarbonControl(), 1);
}

//-----------------------------------------------------------------------------
void RMSButton::mouseTrack(float inXpos, float inYpos)
{
	SInt32 currentValue = GetControl32BitValue(getCarbonControl());

	// mouse is inside the button zone, so the value should be 1
	if ( (inXpos >= 0.0f) && (inXpos < (float)width) && (inYpos >= 0.0f) && (inYpos < (float)height) )
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
void RMSButton::mouseUp(float inXpos, float inYpos)
{
	SetControl32BitValue(getCarbonControl(), 0);
}



#pragma mark -
#pragma mark RMSSlider
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
void RMSSlider::draw(CGContextRef inContext, long inPortHeight)
{
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
void RMSSlider::mouseDown(float inXpos, float inYpos)
{
	mouseTrack(inXpos, inYpos);
}

//-----------------------------------------------------------------------------
void RMSSlider::mouseTrack(float inXpos, float inYpos)
{
	if (inXpos < (float)borderWidth)
		inXpos = (float)borderWidth;
	if ( inXpos > (float)(width-borderWidth) )
		inXpos = (float) (width - borderWidth);
	float valueNorm = (inXpos - (float)borderWidth) / (float)(width - (borderWidth*2));

	SetControl32BitValue( getCarbonControl(), (SInt32)(valueNorm * (float)GetControl32BitMaximum(getCarbonControl())) );
}






#pragma mark -
#pragma mark RMSBuddyEditor
#pragma mark -

//-----------------------------------------------------------------------------
// constants

const RMSColor kBrownColor = { 97, 73, 46 };
const RMSColor kLightBrownColor = { 146, 116, 98 };
const RMSColor kDarkBlueColor = { 54, 69, 115 };
const RMSColor kLightOrangeColor = { 219, 145, 85 };
const RMSColor kWhiteColor = { 255, 255, 255 };
const RMSColor kYellowColor = { 249, 249, 120 };

const ThemeFontID kValueDisplayFontID = kThemeSmallSystemFont;
const ThemeFontID kLabelDisplayFontID = kThemeSmallSystemFont;
const ThemeFontID kSliderLabelDisplayFontID = kThemeSmallSystemFont;
//kThemeSystemFont kThemeSmallSystemFont kThemeMiniSystemFont kThemeLabelFont kThemeApplicationFont

#define kBackgroundColor	kLightBrownColor
#define kBackgroundFrameColor	kBrownColor
#define kLabelTextColor	kWhiteColor
#define kReadoutFrameColor	kBrownColor
#define kReadoutBoxColor	kLightOrangeColor
#define kReadoutTextColor	kDarkBlueColor
#define kSliderBackgroundColor	kDarkBlueColor
#define kSliderActiveColor	kYellowColor
#define kSliderLabelTextColor	kDarkBlueColor


//-----------------------------------------------------------------------------
// control positions and sizes
enum {
	kPadding_general = 20,
	kPadding_between = 8,
	kPadding_help = 12,

	kValueLabelX = kPadding_general,
	kValueLabelWidth = 84,
	kValueLabelHeight = 13,

	kChannelLabelX = kValueLabelX + kValueLabelWidth + kPadding_between,
	kChannelLabelY = kPadding_general,
	kChannelLabelWidth = 75,
	kChannelLabelHeight = 13,

	kValueDisplayX = kChannelLabelX,
	kValueDisplayY = kChannelLabelY + kChannelLabelHeight + kPadding_between,
	kValueDisplayWidth = kChannelLabelWidth,
	kValueDisplayHeight = 17,

	kValueLabelY = kValueDisplayY + ((kValueDisplayHeight - kValueLabelHeight) / 2),

	kXinc = kValueDisplayWidth + kPadding_between,
	kYinc = kValueDisplayHeight + kPadding_between,

	kSliderX = kPadding_general,
	kSliderY = kValueDisplayY + (kYinc * 4) - kPadding_between + kPadding_general,
	kSliderHeight = 11,
	kSliderLabelOffsetX = 3,
	kSliderLabelX = kSliderX + kSliderLabelOffsetX,
	kSliderLabelY = kSliderY + kSliderHeight + kPadding_between,
	kSliderLabelHeight = 13,

	kHelpButtonY = kSliderLabelY + kSliderLabelHeight + kPadding_help,
	kHelpButtonWidth = 25,
	kHelpButtonHeight = 25
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
COMPONENT_ENTRY(RMSBuddyEditor)


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
	resetButtonImage = NULL;
	helpButtonImage = NULL;

	// initialize the controls pointers
	resetRMSbutton = NULL;
	resetPeakButton = NULL;
	helpButton = NULL;
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
	CFBundleRef pluginBundleRef = CFBundleGetBundleWithIdentifier( CFSTR(RMS_BUDDY_BUNDLE_ID) );
	if (pluginBundleRef != NULL)
	{
		CFURLRef resetButtonImageURL = CFBundleCopyResourceURL(pluginBundleRef, CFSTR("reset-button.png"), NULL, NULL);
		if (resetButtonImageURL != NULL)
		{
			CGDataProviderRef provider = CGDataProviderCreateWithURL(resetButtonImageURL);
			if (provider != NULL)
			{
				resetButtonImage = CGImageCreateWithPNGDataProvider(provider, NULL, false, kCGRenderingIntentDefault);
				CGDataProviderRelease(provider);
			}
			CFRelease(resetButtonImageURL);
		}

#ifdef RMSBUDDY_SHOW_HELP_BUTTON
		CFURLRef helpButtonImageURL = CFBundleCopyResourceURL(pluginBundleRef, CFSTR("help-button.png"), NULL, NULL);
		if (helpButtonImageURL != NULL)
		{
			CGDataProviderRef provider = CGDataProviderCreateWithURL(helpButtonImageURL);
			if (provider != NULL)
			{
				helpButtonImage = CGImageCreateWithPNGDataProvider(provider, NULL, false, kCGRenderingIntentDefault);
				CGDataProviderRelease(provider);
			}
			CFRelease(helpButtonImageURL);
		}
#endif
	}


	// create the parameter listener
	AUListenerCreate(RmsParameterListenerProc, this,
		CFRunLoopGetCurrent(), kCFRunLoopDefaultMode, 0.010f, // 10 ms
		&parameterListener);

	// install a parameter listener on the fake refresh-notification parameter
	timeToUpdateAUP.mAudioUnit = GetEditAudioUnit();
	timeToUpdateAUP.mScope = kAudioUnitScope_Global;
	timeToUpdateAUP.mElement = 0;
	timeToUpdateAUP.mParameterID = kRMSBuddyParameter_TimeToUpdate;
	AUListenerAddParameter(parameterListener, this, &timeToUpdateAUP);


	// install AU property listeners for all of the special properties that we need to know about when they change
	AudioUnitAddPropertyListener(GetEditAudioUnit(), kRMSBuddyProperty_NumChannels, RmsPropertyListenerProc, this);


	// setup creates and adds all of the UI content
	return setup();
}

//-----------------------------------------------------------------------------
RMSBuddyEditor::~RMSBuddyEditor()
{
	// free the graphics resource
	if (resetButtonImage != NULL)
		CGImageRelease(resetButtonImage);
	resetButtonImage = NULL;
	if (helpButtonImage != NULL)
		CGImageRelease(helpButtonImage);
	helpButtonImage = NULL;

	// if we created and installed the parameter listener, remove and dispose it now
	if (parameterListener != NULL)
	{
		AUListenerRemoveParameter(parameterListener, this, &timeToUpdateAUP);
		AUListenerDispose(parameterListener);
	}
	parameterListener = NULL;

	// if we created and installed the property listener, remove and dispose it now
	if (GetEditAudioUnit() != NULL)
		AudioUnitRemovePropertyListener(GetEditAudioUnit(), kRMSBuddyProperty_NumChannels, RmsPropertyListenerProc);

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
	if (AudioUnitGetProperty(GetEditAudioUnit(), kRMSBuddyProperty_NumChannels, kAudioUnitScope_Global, (AudioUnitElement)0, &numChannels, &dataSize) 
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

#define VALUE_TEXT_DISPLAY	RMSTextDisplay(this, xpos, ypos, kValueDisplayWidth, kValueDisplayHeight, kReadoutTextColor, kReadoutBoxColor, kReadoutFrameColor, kValueDisplayFontID, kHIThemeTextHorizontalFlushCenter)

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

#define LABEL_TEXT_DISPLAY	RMSTextDisplay(this, xpos, ypos, kValueLabelWidth, kValueLabelHeight, kLabelTextColor, kBackgroundColor, kBackgroundColor, kLabelDisplayFontID, kHIThemeTextHorizontalFlushRight)

	CFBundleRef pluginBundleRef = CFBundleGetBundleWithIdentifier( CFSTR(RMS_BUDDY_BUNDLE_ID) );
	CFStringRef textLabelString = NULL;

	// the words "average RMS"
	averageRMSLabel = new LABEL_TEXT_DISPLAY;
	textLabelString = CFCopyLocalizedStringFromTableInBundle(CFSTR("Average RMS"), CFSTR("Localizable"), pluginBundleRef, CFSTR("average RMS label"));
	averageRMSLabel->setText(textLabelString);
	CFRelease(textLabelString);

	// the words "continual RMS"
	ypos += kYinc;
	continualRMSLabel = new LABEL_TEXT_DISPLAY;
	textLabelString = CFCopyLocalizedStringFromTableInBundle(CFSTR("Continual RMS"), CFSTR("Localizable"), pluginBundleRef, CFSTR("continual RMS label"));
	continualRMSLabel->setText(textLabelString);
	CFRelease(textLabelString);

	// the words "absolute peak"
	ypos += kYinc;
	absolutePeakLabel = new LABEL_TEXT_DISPLAY;
	textLabelString = CFCopyLocalizedStringFromTableInBundle(CFSTR("Absolute Peak"), CFSTR("Localizable"), pluginBundleRef, CFSTR("absolute peak label"));
	absolutePeakLabel->setText(textLabelString);
	CFRelease(textLabelString);

	// the words "continual peak"
	ypos += kYinc;
	continualPeakLabel = new LABEL_TEXT_DISPLAY;
	textLabelString = CFCopyLocalizedStringFromTableInBundle(CFSTR("Continual Peak"), CFSTR("Localizable"), pluginBundleRef, CFSTR("continual peak label"));
	continualPeakLabel->setText(textLabelString);
	CFRelease(textLabelString);

#undef LABEL_TEXT_DISPLAY

	xpos = kChannelLabelX;
	ypos = kChannelLabelY;

	// the label(s) for the the name(s) of the channel column(s)
	for (unsigned long ch=0; ch < numChannels; ch++)
	{
		channelLabels[ch] = new RMSTextDisplay(this, xpos, ypos, kChannelLabelWidth, kChannelLabelHeight, kLabelTextColor, 
							kBackgroundColor, kBackgroundColor, kLabelDisplayFontID, kHIThemeTextHorizontalFlushCenter);

		if (numChannels <= 1)
			channelLabels[ch]->setText(NULL);
		else if (numChannels == 2)
		{
			if (ch == 0)
			{
				textLabelString = CFCopyLocalizedStringFromTableInBundle(CFSTR("Left"), CFSTR("Localizable"), pluginBundleRef, CFSTR("left channel label"));
				channelLabels[ch]->setText(textLabelString);
				CFRelease(textLabelString);
			}
			else
			{
				textLabelString = CFCopyLocalizedStringFromTableInBundle(CFSTR("Right"), CFSTR("Localizable"), pluginBundleRef, CFSTR("right channel label"));
				channelLabels[ch]->setText(textLabelString);
				CFRelease(textLabelString);
			}
		}
		else
			channelLabels[ch]->setText_int(ch+1);

		xpos += kXinc;
	}


//--initialize the buttons-----------------------------------------------

	// reset continual RMS button
	const long resetButtonWidth = CGImageGetWidth(resetButtonImage);
	const long resetButtonHeight = CGImageGetHeight(resetButtonImage) / 2;
	const long resetButtonX = kValueDisplayX + (kXinc * numChannels);
	const long resetButtonY = kValueDisplayY + ((kValueDisplayHeight - resetButtonHeight) / 2);
	resetRMSbutton = new RMSButton(this, resetButtonX, resetButtonY, resetButtonImage);

	// reset peak button
	resetPeakButton = new RMSButton(this, resetButtonX, resetButtonY + (kYinc*2), resetButtonImage);

	const long backgroundWidth = resetButtonX + resetButtonWidth + kPadding_general;

#ifdef RMSBUDDY_SHOW_HELP_BUTTON
	// help button
#if 1
	const long helpButtonWidth = CGImageGetWidth(helpButtonImage);
	const long helpButtonHeight = CGImageGetHeight(helpButtonImage) / 2;
	helpButton = new RMSButton(this, backgroundWidth - helpButtonWidth - 18, kHelpButtonY, helpButtonImage);
#else
// XXX using system icons and buttons doesn't embed in the right place in compositing windows (my own error) 
// and doesn't draw pane background underneath in non-compositing windows
	Rect helpButtonRect;
	helpButtonRect.left = backgroundWidth - kHelpButtonWidth - kPadding_general;
	helpButtonRect.top = kHelpButtonY;
	helpButtonRect.right = kHelpButtonWidth + helpButtonRect.left;
	helpButtonRect.bottom = kHelpButtonHeight + helpButtonRect.top;
	ControlButtonContentInfo buttonContentInfo;
	buttonContentInfo.contentType = kControlContentIconRef;
	OSStatus buttonStatus = GetIconRef(kOnSystemDisk, kSystemIconsCreator, kHelpIcon, &(buttonContentInfo.u.iconRef));
	if (buttonStatus == noErr)
	{
		ControlRef buttonControl = NULL;
		buttonStatus = CreateRoundButtonControl(GetCarbonWindow(), &helpButtonRect, kControlSizeNormal, &buttonContentInfo, &buttonControl);
		if ( (buttonStatus == noErr) && (buttonControl == NULL) )
		{
			if ( IsCompositWindow() )
				buttonStatus = ::HIViewAddSubview(GetCarbonPane(), buttonControl);
			else
				buttonStatus = ::EmbedControl(buttonControl, GetCarbonPane());
		}
	}
#endif
#endif


//--initialize the slider-----------------------------------------------

	long sliderWidth = backgroundWidth - (kSliderX*2);
	windowSizeSlider = new RMSSlider(this, kRMSBuddyParameter_AnalysisWindowSize, kSliderX, kSliderY, sliderWidth, kSliderHeight, 
										kSliderBackgroundColor, kSliderActiveColor);
	updateWindowSize(windowSizeSlider->getAUVP()->GetValue(), windowSizeSlider);

	sliderWidth -= kSliderLabelOffsetX * 2;
	// the words "analysis window size"
	const long sliderLabelWidth = (2 * sliderWidth) / 3;
	windowSizeLabel = new RMSTextDisplay(this, kSliderLabelX, kSliderLabelY, sliderLabelWidth, kSliderLabelHeight, 
										kSliderLabelTextColor, kBackgroundColor, kBackgroundColor, 
										kSliderLabelDisplayFontID, kHIThemeTextHorizontalFlushLeft);
	textLabelString = CFCopyLocalizedStringFromTableInBundle(CFSTR("Analysis Window Size"), CFSTR("Localizable"), pluginBundleRef, CFSTR("analysis window size slider label"));
	windowSizeLabel->setText(textLabelString);
	CFRelease(textLabelString);

	// the analysis window size value read-out
	const long sliderDisplayWidth = sliderWidth - sliderLabelWidth;
	windowSizeDisplay = new RMSTextDisplay(this, kSliderLabelX + sliderLabelWidth, kSliderLabelY, sliderDisplayWidth, kSliderLabelHeight, 
										kSliderLabelTextColor, kBackgroundColor, kBackgroundColor, 
										kSliderLabelDisplayFontID, kHIThemeTextHorizontalFlushRight, kRMSBuddyParameter_AnalysisWindowSize);


	// set size of the background embedding pane control to fit the entire UI display
#ifdef RMSBUDDY_SHOW_HELP_BUTTON
	const long backgroundHeight = kHelpButtonY + helpButtonHeight + kPadding_help;
#else
	const long backgroundHeight = kSliderLabelY + kSliderLabelHeight + kPadding_general;
#endif
	SizeControl(GetCarbonPane(), backgroundWidth, backgroundHeight);


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
	SAFE_DELETE_CONTROL(helpButton)
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
// outPort will be null if a CGContextRef was available from the event parameters 
// (which happens with compositing windows) or non-null if the CGContext had to be created 
// for the control's window port QuickDraw-style.  
OSStatus InitControlDrawingContext(EventRef inEvent, CGContextRef & outContext, CGrafPtr & outPort, long & outPortHeight)
{
	outContext = NULL;
	outPort = NULL;
	if (inEvent == NULL)
		return paramErr;

	OSStatus status;

	// if we are in compositing mode, then we can get a CG context already set up and clipped and whatnot
	status = GetEventParameter(inEvent, kEventParamCGContextRef, typeCGContextRef, NULL, sizeof(outContext), NULL, &outContext);
	if ( (status == noErr) && (outContext != NULL) )
	{
		outPortHeight = 0;
	}
	// otherwise we need to get it from the QD port
	else
	{
		// if we received a graphics port parameter, use that...
		status = GetEventParameter(inEvent, kEventParamGrafPort, typeGrafPtr, NULL, sizeof(outPort), NULL, &outPort);
		// ... otherwise use the current graphics port
		if ( (status != noErr) || (outPort == NULL) )
			GetPort(&outPort);
		if (outPort == NULL)
			return noPortErr;
		Rect portBounds;
		GetPortBounds(outPort, &portBounds);
		outPortHeight = (long) (portBounds.bottom - portBounds.top);

		status = QDBeginCGContext(outPort, &outContext);
		if ( (status != noErr) || (outContext == NULL) )
		{
			if (status == noErr)
				status = statusErr;
			outContext = NULL;	// probably unlikely, but in case there's an error, and yet a non-null result for the context
			return status;
		}
	}

	CGContextSaveGState(outContext);
	if (outPort != NULL)
		SyncCGContextOriginWithPort(outContext, outPort);

	// define the clipping region if we are not compositing and had to create our own context
	if (outPort != NULL)
	{
		ControlRef carbonControl = NULL;
		status = GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, NULL, sizeof(carbonControl), NULL, &carbonControl);
		if ( (status == noErr) && (carbonControl != NULL) )
		{
			CGRect clipRect;
			status = HIViewGetBounds(carbonControl, &clipRect);
			if (status == noErr)
			{
				clipRect = TransformControlBoundsToDrawingBounds(clipRect, outPortHeight);
				CGContextClipToRect(outContext, clipRect);
			}
		}
	}

	return noErr;
}

//-----------------------------------------------------------------------------
// inPort should be what you got from InitControlDrawingContext().  
// It is null if inContext came with the control event (compositing window behavior) or 
// non-null if inContext was created via QDBeginCGContext(), and hence needs to be terminated.
void CleanupControlDrawingContext(CGContextRef & inContext, CGrafPtr inPort)
{
	if (inContext == NULL)
		return;

	CGContextRestoreGState(inContext);
	CGContextSynchronize(inContext);

	if (inPort != NULL)
		QDEndCGContext(inPort, &inContext);
}

//-----------------------------------------------------------------------------
bool RMSBuddyEditor::HandleEvent(EventRef inEvent)
{
	// we redraw the background when we catch a draw event for the root pane control
	if ( (GetEventClass(inEvent) == kEventClassControl) && (GetEventKind(inEvent) == kEventControlDraw) )
	{
		ControlRef carbonControl = NULL;
		OSStatus status = GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, NULL, sizeof(carbonControl), NULL, &carbonControl);
		if ( (carbonControl == GetCarbonPane()) && (carbonControl != NULL) && (status == noErr) )
		{
			CGRect controlBounds;
			status = HIViewGetBounds(carbonControl, &controlBounds);
			if (status != noErr)
				return false;
			CGContextRef context = NULL;
			CGrafPtr windowPort = NULL;
			long portHeight;
			status = InitControlDrawingContext(inEvent, context, windowPort, portHeight);
			if ( (status != noErr) || (context == NULL) )
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

			CleanupControlDrawingContext(context, windowPort);
			return true;
		}
	}

	// let the parent class handle any other events
	return AUCarbonViewBase::HandleEvent(inEvent);
}

//-----------------------------------------------------------------------------
static pascal OSStatus RmsWindowEventHandler(EventHandlerCallRef inHandler, EventRef inEvent, void * inUserData)
{
	// make sure that it's the correct event class
	if (GetEventClass(inEvent) != kEventClassMouse)
		return eventClassIncorrectErr;

	RMSBuddyEditor * ourOwnerEditor = (RMSBuddyEditor*) inUserData;

	// get the mouse location event parameter
	HIPoint mouseLocation;
	GetEventParameter(inEvent, kEventParamMouseLocation, typeHIPoint, NULL, sizeof(mouseLocation), NULL, &mouseLocation);

	// see if a control was clicked on (we only use this event handling to track control mousing)
	RMSControl * ourRMSControl = ourOwnerEditor->getCurrentControl();
	if (ourRMSControl == NULL)
		return eventNotHandledErr;

	// orient the mouse coordinates as though the control were at 0, 0 (for convenience)
	Rect controlBounds;
	GetControlBounds(ourRMSControl->getCarbonControl(), &controlBounds);
	Rect windowBounds;	// window content region
	memset(&windowBounds, 0, sizeof(windowBounds));
	WindowRef window = NULL;
	GetEventParameter(inEvent, kEventParamWindowRef, typeWindowRef, NULL, sizeof(window), NULL, &window);
	if (window == NULL)
		GetControlOwner( ourRMSControl->getCarbonControl() );
	if (window != NULL)
		GetWindowBounds(window, kWindowGlobalPortRgn, &windowBounds);
	if ( ourOwnerEditor->IsCompositWindow() )
	{
		Rect paneBounds;
		GetControlBounds(ourOwnerEditor->GetCarbonPane(), &paneBounds);
		controlBounds.left += paneBounds.left;
		controlBounds.top += paneBounds.top;
	}
	mouseLocation.x -= (float) (controlBounds.left + windowBounds.left);
	mouseLocation.y -= (float) (controlBounds.top + windowBounds.top);


	UInt32 inEventKind = GetEventKind(inEvent);

	if (inEventKind == kEventMouseDragged)
	{
		ourRMSControl->mouseTrack(mouseLocation.x, mouseLocation.y);
		return noErr;
	}

	if (inEventKind == kEventMouseUp)
	{
		ourRMSControl->mouseUp(mouseLocation.x, mouseLocation.y);
		ourOwnerEditor->setCurrentControl(NULL);

		// do this to make Logic's touch automation work
		if ( ourRMSControl->isParameterAttached() )
			ourOwnerEditor->SendAUParameterEvent(ourRMSControl->getAUVP()->mParameterID, kAudioUnitEvent_EndParameterChangeGesture);

		return noErr;
	}

	return eventKindIncorrectErr;
}

//-----------------------------------------------------------------------------
static pascal OSStatus RmsControlEventHandler(EventHandlerCallRef inHandler, EventRef inEvent, void * inUserData)
{
	// make sure that it's the correct event class
	if (GetEventClass(inEvent) != kEventClassControl)
		return eventClassIncorrectErr;

	// get the Carbon Control reference event parameter
	ControlRef ourCarbonControl = NULL;
	OSStatus status = GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, NULL, sizeof(ourCarbonControl), NULL, &ourCarbonControl);
	if (status != noErr)
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
				long portHeight;
				status = InitControlDrawingContext(inEvent, context, windowPort, portHeight);
				if ( (status != noErr) || (context == NULL) )
					return eventNotHandledErr;

				// draw
				ourRMSControl->draw(context, portHeight);

				CleanupControlDrawingContext(context, windowPort);
			}
			return noErr;

		case kEventControlClick:
			{
				// get the mouse location event parameter
				HIPoint mouseLocation;
				GetEventParameter(inEvent, kEventParamMouseLocation, typeHIPoint, NULL, sizeof(mouseLocation), NULL, &mouseLocation);

				// orient the mouse coordinates as though the control were at 0, 0 (for convenience)
				Rect controlBounds;
				GetControlBounds(ourCarbonControl, &controlBounds);
				Rect windowBounds;	// window content region
				memset(&windowBounds, 0, sizeof(windowBounds));
				WindowRef window = GetControlOwner(ourCarbonControl);
				if (window != NULL)
					GetWindowBounds(window, kWindowGlobalPortRgn, &windowBounds );
				if ( ourOwnerEditor->IsCompositWindow() )
				{
					Rect paneBounds;
					GetControlBounds(ourOwnerEditor->GetCarbonPane(), &paneBounds);
					controlBounds.left += paneBounds.left;
					controlBounds.top += paneBounds.top;
				}
				mouseLocation.x -= (float) (controlBounds.left + windowBounds.left);
				mouseLocation.y -= (float) (controlBounds.top + windowBounds.top);

				ourRMSControl->mouseDown(mouseLocation.x, mouseLocation.y);

#ifndef USE_AUCVCONTROL
				// do this to make Logic's touch automation work
				if ( ourRMSControl->isParameterAttached() )
					ourOwnerEditor->SendAUParameterEvent(ourRMSControl->getAUVP()->mParameterID, kAudioUnitEvent_BeginParameterChangeGesture);
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
// this gets called when the DSP component sends notification via the TimeToUpdate parameter
static void RmsParameterListenerProc(void * inUserData, void * inObject, const AudioUnitParameter * inParameter, Float32 inValue)
{
	RMSBuddyEditor * bud = (RMSBuddyEditor*) inUserData;
	if ( (bud != NULL) && (inParameter != NULL) )
	{
		switch (inParameter->mParameterID)
		{
			case kRMSBuddyParameter_AnalysisWindowSize:
				bud->updateWindowSize(inValue, (RMSControl*)inObject);
				break;
			case kRMSBuddyParameter_TimeToUpdate:
				bud->updateDisplays();	// refresh the value displays
				break;
			default:
				break;
		}
	}
}

//-----------------------------------------------------------------------------
// this gets called when the DSP component sends a property-changed notification of the NumChannels property
static void RmsPropertyListenerProc(void * inUserData, AudioUnit inComponentInstance, AudioUnitPropertyID inPropertyID, 
									AudioUnitScope inScope, AudioUnitElement inElement)
{
	RMSBuddyEditor * bud = (RMSBuddyEditor*) inUserData;
	// when the number of channels changes, we tear down the existing GUI layout and 
	// then recreate it according to the new number of channels
	if ( (bud != NULL) && (inPropertyID == kRMSBuddyProperty_NumChannels) )
	{
		bud->cleanup();	// tear it down
		bud->setup();	// rebuild
	}
}

//-----------------------------------------------------------------------------
OSStatus RMSBuddyEditor::SendAUParameterEvent(AudioUnitParameterID inParameterID, AudioUnitEventType inEventType)
{
	// we're not actually prepared to do anything at this point if we don't yet know which AU we are controlling
	if (GetEditAudioUnit() == NULL)
		return kAudioUnitErr_Uninitialized;

	OSStatus result = noErr;

	// do the new-fangled way, if it's available on the user's system
	AudioUnitEvent paramEvent;
	paramEvent.mEventType = inEventType;
	paramEvent.mArgument.mParameter.mParameterID = inParameterID;
	paramEvent.mArgument.mParameter.mAudioUnit = GetEditAudioUnit();
	paramEvent.mArgument.mParameter.mScope = kAudioUnitScope_Global;
	paramEvent.mArgument.mParameter.mElement = 0;
	if (AUEventListenerNotify != NULL)
		result = AUEventListenerNotify(NULL, NULL, &paramEvent);

	// as a back-up, also still do the old way, until it's enough obsolete
	if (mEventListener != NULL)
	{
		AudioUnitCarbonViewEventID carbonViewEventType = -1;
		if (inEventType == kAudioUnitEvent_BeginParameterChangeGesture)
			carbonViewEventType = kAudioUnitCarbonViewEvent_MouseDownInControl;
		else if (inEventType == kAudioUnitEvent_EndParameterChangeGesture)
			carbonViewEventType = kAudioUnitCarbonViewEvent_MouseUpInControl;
		(*mEventListener)(mEventListenerUserData, GetComponentInstance(), &(paramEvent.mArgument.mParameter), carbonViewEventType, NULL);
	}

	return result;
}

//-----------------------------------------------------------------------------
// refresh the value displays
void RMSBuddyEditor::updateDisplays()
{
	RMSBuddyDynamicsData request;
	UInt32 dataSize = sizeof(request);

	// get the dynamics data values for each channel being analyzed
	for (unsigned long ch=0; ch < numChannels; ch++)
	{
		request.inChannel = ch;
		// if getting the property fails, initialize to all zeroes so it's usable anyway
		if (AudioUnitGetProperty(GetEditAudioUnit(), kRMSBuddyProperty_DynamicsData, kAudioUnitScope_Global, 0, &request, &dataSize) != noErr)
		{
			request.outAverageRMS = request.outContinualRMS = 0.0;
			request.outAbsolutePeak = request.outContinualPeak = 0.0f;
		}
#define SAFE_SET_TEXT(ctrl, val)	\
	if (ctrl != NULL)	\
	{	\
		if (ctrl[ch] != NULL)	\
			ctrl[ch]->setText_dB(val);	\
	}
		// update the values being displayed
		SAFE_SET_TEXT(averageRMSDisplays, request.outAverageRMS)
		SAFE_SET_TEXT(continualRMSDisplays, request.outContinualRMS)
		SAFE_SET_TEXT(absolutePeakDisplays, request.outAbsolutePeak)
		SAFE_SET_TEXT(continualPeakDisplays, request.outContinualPeak)
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
		float valueNorm = AUParameterValueToLinear(inParamValue, windowSizeSlider->getAUVP());

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
		Float32 controlValueNorm = (Float32)(inControlValue - cmin) / (Float32)(cmax - cmin);

		Float32 paramValue = AUParameterValueFromLinear(controlValueNorm, windowSizeSlider->getAUVP());
		windowSizeSlider->getAUVP()->SetValue(parameterListener, windowSizeSlider, paramValue);
	}

#ifdef RMSBUDDY_SHOW_HELP_BUTTON
	else if (inControl == helpButton)
	{
		if (inControlValue == 0)
		{
			CFBundleRef pluginBundleRef = CFBundleGetBundleWithIdentifier( CFSTR(RMS_BUDDY_BUNDLE_ID) );
			if (pluginBundleRef != NULL)
			{
				CFStringRef docsFileName = CFSTR("RMS Buddy manual.html");
				CFURLRef docsFileURL = CFBundleCopyResourceURL(pluginBundleRef, docsFileName, NULL, NULL);
				if (docsFileURL != NULL)
				{
					CFStringRef docsFileUrlString = CFURLGetString(docsFileURL);
					if (docsFileUrlString != NULL)
						AHGotoPage(NULL, docsFileUrlString, NULL);
					CFRelease(docsFileURL);
				}
			}
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// send a message to the DSP component to reset average RMS
void RMSBuddyEditor::resetRMS()
{
	char nud;	// irrelavant, no input data is actually needed, but SetProperty will fail without something
	AudioUnitSetProperty(GetEditAudioUnit(), kRMSBuddyProperty_ResetRMS, kAudioUnitScope_Global, (AudioUnitElement)0, &nud, sizeof(char));
}

//-----------------------------------------------------------------------------
// send a message to the DSP component to reset absolute peak
void RMSBuddyEditor::resetPeak()
{
	char nud;	// irrelavant, no input data is actually needed, but SetProperty will fail without something
	AudioUnitSetProperty(GetEditAudioUnit(), kRMSBuddyProperty_ResetPeak, kAudioUnitScope_Global, (AudioUnitElement)0, &nud, sizeof(char));
}
