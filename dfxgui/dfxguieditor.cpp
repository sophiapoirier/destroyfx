#include "dfxgui.h"

#include "dfxplugin.h"
#include "dfx-au-utilities.h"



#if MAC
static pascal OSStatus DGControlEventHandler(EventHandlerCallRef, EventRef, void * inUserData);
static pascal OSStatus DGWindowEventHandler(EventHandlerCallRef, EventRef, void * inUserData);

static pascal void DGIdleTimerProc(EventLoopTimerRef inTimer, void * inUserData);
#endif

// for now this is a good idea, until it's totally obsoleted
#define AU_DO_OLD_STYLE_PARAMETER_CHANGE_GESTURES


//-----------------------------------------------------------------------------
DfxGuiEditor::DfxGuiEditor(DGEditorListenerInstance inInstance)
:	AUCarbonViewBase(inInstance, 0.015f)	// 15 ms parameter notification update interval
{
/*
CFStringRef text = CFSTR("yo dude let's go");
CFRange foundRange = CFStringFind(text, CFSTR(" "), 0);
if (foundRange.length != 0)
{
CFMutableStringRef mut = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, text);
if (mut != NULL)
{
fprintf(stderr, "\n\tbefore:\n");
CFShow(mut);
CFStringFindAndReplace(mut, CFSTR(" "), CFSTR(""), CFRangeMake(0, CFStringGetLength(mut)), 0);
fprintf(stderr, "\treplaced:\n");
CFShow(mut);
CFRelease(mut);
}
}
*/



	controlsList = NULL;
	imagesList = NULL;
  
	idleTimer = NULL;
	idleTimerUPP = NULL;

	backgroundImage = NULL;
	backgroundColor(randFloat(), randFloat(), randFloat());

	dgControlSpec.defType = kControlDefObjectClass;
	dgControlSpec.u.classRef = NULL;
	controlHandlerUPP = NULL;
	windowEventHandlerUPP = NULL;
	windowEventHandlerRef = NULL;

	fontsATSContainer = NULL;
	fontsWereActivated = false;	// guilty until proven innocent

	currentControl_clicked = NULL;
	currentControl_mouseover = NULL;

	dfxplugin = NULL;
}

//-----------------------------------------------------------------------------
DfxGuiEditor::~DfxGuiEditor()
{
	if (idleTimer != NULL)
		RemoveEventLoopTimer(idleTimer);
	idleTimer = NULL;
	if (idleTimerUPP != NULL)
		DisposeEventLoopTimerUPP(idleTimerUPP);
	idleTimerUPP = NULL;

	// deleting a list link also deletes the list's item
	while (controlsList != NULL)
	{
		DGControlsList * tempcl = controlsList->next;
		delete controlsList;
		controlsList = tempcl;
	}
	while (imagesList != NULL)
	{
		DGImagesList * tempcl = imagesList->next;
		delete imagesList;
		imagesList = tempcl;
	}

	if (windowEventHandlerRef != NULL)
		RemoveEventHandler(windowEventHandlerRef);
	windowEventHandlerRef = NULL;
	if (windowEventHandlerUPP != NULL)
		DisposeEventHandlerUPP(windowEventHandlerUPP);
	windowEventHandlerUPP = NULL;

	// only unregister in Mac OS X version 10.2.3 or higher, otherwise this will cause a crash
	if ( GetMacOSVersion() >= 0x1023 )
	{
//fprintf(stderr, "using Mac OS X version 10.2.3 or higher, so our control toolbox class will be unregistered\n");
		if (dgControlSpec.u.classRef != NULL)
		{
			OSStatus unregResult = UnregisterToolboxObjectClass( (ToolboxObjectClassRef)(dgControlSpec.u.classRef) );
			if (unregResult == noErr)
			{
				dgControlSpec.u.classRef = NULL;
				if (controlHandlerUPP != NULL)
					DisposeEventHandlerUPP(controlHandlerUPP);
				controlHandlerUPP = NULL;
			}
//else fprintf(stderr, "unregistering our control toolbox class FAILED, error %ld\n", unregResult);
		}
	}
//else fprintf(stderr, "using a version of Mac OS X lower than 10.2.3, so our control toolbox class will NOT be unregistered\n");

	// This will actually automatically be deactivated when the host app is quit, 
	// so there's no need to deactivate fonts ourselves.  
	// In fact, since there may be multiple plugin GUI instances, it's safer 
	// to just let the fonts stay activated.
//	if ( fontsWereActivated && (fontsATSContainer != NULL) )
//		ATSFontDeactivate(fontsATSContainer, NULL, kATSOptionFlagsDefault);
}


#ifdef TARGET_API_AUDIOUNIT
//-----------------------------------------------------------------------------
OSStatus DfxGuiEditor::CreateUI(Float32 inXOffset, Float32 inYOffset)
{
	#if TARGET_PLUGIN_USES_MIDI
		setmidilearning(false);
	#endif


// create our HIToolbox object class for common event handling amongst our custom Carbon Controls
	EventTypeSpec toolboxClassEvents[] = {
		{ kEventClassControl, kEventControlDraw },
//		{ kEventClassControl, kEventControlInitialize },
		{ kEventClassControl, kEventControlHitTest },
		{ kEventClassControl, kEventControlTrack },
//		{ kEventClassControl, kEventControlHit }, 
//		{ kEventClassControl, kEventControlClick }, 
		{ kEventClassControl, kEventControlContextualMenuClick }, 
		{ kEventClassControl, kEventControlValueFieldChanged }
	};
	ToolboxObjectClassRef newControlClass = NULL;
	controlHandlerUPP = NewEventHandlerUPP(DGControlEventHandler);
	unsigned long instanceAddress = (unsigned long) this;
	bool noSuccessYet = true;
	while (noSuccessYet)
	{
		CFStringRef toolboxClassIDcfstring = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%s.DfxGuiControlClass%lu"), 
																		PLUGIN_BUNDLE_IDENTIFIER, instanceAddress);
		if (toolboxClassIDcfstring != NULL)
		{
			if ( RegisterToolboxObjectClass(toolboxClassIDcfstring, NULL, GetEventTypeCount(toolboxClassEvents), toolboxClassEvents, 
											controlHandlerUPP, this, &newControlClass) == noErr )
				noSuccessYet = false;
			CFRelease(toolboxClassIDcfstring);
		}
		instanceAddress++;
	}
	dgControlSpec.u.classRef = newControlClass;


// create the window event handler that supplements the control event handler by tracking mouse dragging, mouseover controls, etc.
	currentControl_clicked = NULL;	// make sure that it ain't nuthin
	setCurrentControl_mouseover(NULL);
	EventTypeSpec windowEvents[] = {
									{ kEventClassMouse, kEventMouseDragged }, 
									{ kEventClassMouse, kEventMouseUp }, 
//									{ kEventClassMouse, kEventMouseMoved }, 
									{ kEventClassKeyboard, kEventRawKeyDown }, 
									{ kEventClassKeyboard, kEventRawKeyRepeat }, 
									{ kEventClassCommand, kEventCommandProcess }, 
								};
	windowEventHandlerUPP = NewEventHandlerUPP(DGWindowEventHandler);
	InstallEventHandler(GetWindowEventTarget(GetCarbonWindow()), windowEventHandlerUPP, 
						GetEventTypeCount(windowEvents), windowEvents, this, &windowEventHandlerRef);


// register for HitTest events on the background embedding pane so that we know when the mouse hovers over it
	EventTypeSpec paneEvents[] = {
								{ kEventClassControl, kEventControlDraw }, 
								{ kEventClassControl, kEventControlApplyBackground },
								{ kEventClassControl, kEventControlHitTest }
								};
	WantEventTypes(GetControlEventTarget(mCarbonPane), GetEventTypeCount(paneEvents), paneEvents);


// load any fonts from our bundle resources to be accessible locally within our component instance
	CFBundleRef pluginBundle = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));
	if (pluginBundle != NULL)
	{
		CFURLRef bundleResourcesDirCFURL = CFBundleCopyResourcesDirectoryURL(pluginBundle);
		if (bundleResourcesDirCFURL != NULL)
		{
			FSRef bundleResourcesDirFSRef;
			if ( CFURLGetFSRef(bundleResourcesDirCFURL, &bundleResourcesDirFSRef) )
			{
				FSSpec bundleResourcesDirFSSpec;
				OSStatus status = FSGetCatalogInfo(&bundleResourcesDirFSRef, kFSCatInfoNone, NULL, NULL, &bundleResourcesDirFSSpec, NULL);
				// activate all of our fonts in the local (host application) context only
				// if the fonts already exist on the user's system, our versions will take precedence
				if (status == noErr)
					status = ATSFontActivateFromFileSpecification(&bundleResourcesDirFSSpec, kATSFontContextLocal, 
									kATSFontFormatUnspecified, NULL, kATSOptionFlagsProcessSubdirectories, &fontsATSContainer);
				if (status == noErr)
					fontsWereActivated = true;
			}
		}
	}


	// XXX this is not a good thing to do, hmmm, maybe I should not do it...
	UInt32 dataSize = sizeof(dfxplugin);
	if (AudioUnitGetProperty(GetEditAudioUnit(), kDfxPluginProperty_PluginPtr, 
							kAudioUnitScope_Global, (AudioUnitElement)0, &dfxplugin, &dataSize) 
							!= noErr)
		dfxplugin = NULL;


	// let the child class do its thing
	long openErr = open();
	if (openErr == noErr)
	{
		// set the size of the embedding pane
		if (backgroundImage != NULL)
			SizeControl(mCarbonPane, (SInt16) (backgroundImage->getWidth()), (SInt16) (backgroundImage->getHeight()));

		idleTimerUPP = NewEventLoopTimerUPP(DGIdleTimerProc);
		InstallEventLoopTimer(GetCurrentEventLoop(), 0.0, kEventDurationMillisecond * 50.0, idleTimerUPP, this, &idleTimer);

		// embed/activate every control
		DGControlsList * tempcl = controlsList;
		while (tempcl != NULL)
		{
			tempcl->control->embed();
			tempcl = tempcl->next;
		}
	}

	return openErr;
}
#endif
// TARGET_API_AUDIOUNIT

#ifdef TARGET_API_AUDIOUNIT
//-----------------------------------------------------------------------------
ComponentResult DfxGuiEditor::Version()
{
	return PLUGIN_VERSION;
}
#endif
// TARGET_API_AUDIOUNIT

#if MAC
//-----------------------------------------------------------------------------
// inEvent is expected to be an event of the class kEventClassControl.  
// The return value is true if a CGContextRef was available from the event parameters 
// (which happens with compositing windows) or false if the CGContext had to be created 
// for the control's window port QuickDraw-style.  
bool InitControlDrawingContext(EventRef inEvent, CGContextRef & outContext, CGrafPtr & outPort, long & outPortHeight)
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
	outPortHeight = portBounds.bottom - portBounds.top;

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
#ifdef FLIP_CG_COORDINATES
	// this lets me position things with non-upside-down coordinates, 
	// but unfortunately draws all images and text upside down...
	CGContextTranslateCTM(outContext, 0.0f, (float)outPortHeight);
	CGContextScaleCTM(outContext, 1.0f, -1.0f);
#endif

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
#ifndef FLIP_CG_COORDINATES
				clipRect.origin.y = (float)outPortHeight - (clipRect.origin.y + clipRect.size.height);
#endif
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
#endif
// MAC

#ifdef TARGET_API_AUDIOUNIT
//-----------------------------------------------------------------------------
bool DfxGuiEditor::HandleEvent(EventRef inEvent)
{
	if (GetEventClass(inEvent) == kEventClassControl)
	{
		ControlRef carbonControl = NULL;
		OSStatus error = GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, NULL, sizeof(ControlRef), NULL, &carbonControl);
		if (error != noErr)
			carbonControl = NULL;

		UInt32 inEventKind = GetEventKind(inEvent);

		if (inEventKind == kEventControlDraw)
		{
			if (carbonControl == mCarbonPane)
			{
				CGContextRef context = NULL;
				CGrafPtr windowPort = NULL;
				long portHeight;
				bool gotAutoContext = InitControlDrawingContext(inEvent, context, windowPort, portHeight);
				if ( (context == NULL) || (windowPort == NULL) )
					return false;

				CGContextSetShouldAntialias(context, false);	// XXX disable anti-aliased drawing for image rendering
				DrawBackground(context, portHeight);

				CleanupControlDrawingContext(context, gotAutoContext, windowPort);
				return true;
			}
		}

		// we want to catch when the mouse hovers over onto the background area
		else if ( (inEventKind == kEventControlHitTest) || (inEventKind == kEventControlClick) )
		{
			if (carbonControl == mCarbonPane)
				setCurrentControl_mouseover(NULL);	// we don't count the background
		}

		else if (inEventKind == kEventControlApplyBackground)
		{
//fprintf(stderr, "mCarbonPane HandleEvent(kEventControlApplyBackground)\n");
			return false;
		}
	}
	
	// let the parent implementation do its thing
	return AUCarbonViewBase::HandleEvent(inEvent);
}
#endif
// TARGET_API_AUDIOUNIT


//-----------------------------------------------------------------------------
void DfxGuiEditor::do_idle()
{
	// call any child class implementation of the virtual idle method
	idle();

	// call every controls' implementation of its idle method
	DGControlsList * tempcl = controlsList;
	while (tempcl != NULL)
	{
		tempcl->control->idle();
		tempcl = tempcl->next;
	}
}

#if MAC
//-----------------------------------------------------------------------------
static pascal void DGIdleTimerProc(EventLoopTimerRef inTimer, void * inUserData)
{
	if (inUserData != NULL)
		((DfxGuiEditor*)inUserData)->do_idle();
}
#endif

//-----------------------------------------------------------------------------
void DfxGuiEditor::addImage(DGImage * inImage)
{
	if (inImage == NULL)
		return;

	imagesList = new DGImagesList(inImage, imagesList);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::addControl(DGControl * inControl)
{
	if (inControl == NULL)
		return;

	controlsList = new DGControlsList(inControl, controlsList);
}

#if MAC
//-----------------------------------------------------------------------------
DGControl * DfxGuiEditor::getDGControlByCarbonControlRef(ControlRef inControl)
{
	DGControlsList * tempcl = controlsList;
	while (tempcl != NULL)
	{
		if ( tempcl->control->isControlRef(inControl) )
			return tempcl->control;
		tempcl = tempcl->next;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::IsWindowCompositing()
{
	if (GetCarbonWindow() != NULL)
	{
		WindowAttributes attributes = 0;
		OSStatus error = GetWindowAttributes(GetCarbonWindow(), &attributes);
		if (error == noErr)
			return (attributes & kWindowCompositingAttribute) ? true : false;
	}
	return false;
}
#endif

//-----------------------------------------------------------------------------
void DfxGuiEditor::DrawBackground(CGContextRef inContext, long inPortHeight)
{
	if (backgroundImage != NULL)
	{
		DGRect drawRect( (long)GetXOffset(), (long)GetYOffset(), backgroundImage->getWidth(), backgroundImage->getHeight() );
		backgroundImage->draw(&drawRect, inContext, inPortHeight);
	}
	else
	{
		// XXX draw a rectangle with the background color
	}
}

//-----------------------------------------------------------------------------
// XXX this function should really go somewhere else, like in that promised DFX utilities file or something like that
long launch_documentation()
{

#if MAC
	// no assumptions can be made about how long the reference is valid, 
	// and the caller should not attempt to release the CFBundleRef object
	CFBundleRef pluginBundleRef = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));
	if (pluginBundleRef != NULL)
	{
		CFStringRef fileCFName = CFSTR( PLUGIN_NAME_STRING" manual.html" );
		CFURLRef fileURL = CFBundleCopyResourceURL(pluginBundleRef, fileCFName, NULL, NULL);
		if (fileURL != NULL)
		{
// open the manual with the default application for the file type
#if 0
			OSStatus status = LSOpenCFURLRef(fileURL, NULL);
			CFRelease(fileURL);
// open the manual with Apple's system Help Viewer
#else
			OSStatus status = coreFoundationUnknownErr;
			CFStringRef fileUrlString = CFURLGetString(fileURL);
			if (fileUrlString != NULL)
			{
				status = AHGotoPage(NULL, fileUrlString, NULL);
				CFRelease(fileUrlString);
			}
			CFRelease(fileURL);
#endif
			return status;
		}
	}

	return fnfErr;	// file not found error
#endif

}


#ifdef TARGET_API_AUDIOUNIT
//-----------------------------------------------------------------------------
OSStatus DfxGuiEditor::SendAUParameterEvent(AudioUnitParameterID inParameterID, AudioUnitEventType inEventType)
{
	// we're not actually prepared to do anything at this point if we don't yet know which AU we are controlling
	if (GetEditAudioUnit() == NULL)
		return kAudioUnitErr_Uninitialized;

	OSStatus result = noErr;

	// do the new-fangled way, if it's available on the user's system
	if (AUEventListenerNotify != NULL)
	{
		AudioUnitEvent paramEvent;
		memset(&paramEvent, 0, sizeof(paramEvent));
		paramEvent.mEventType = inEventType;
		paramEvent.mArgument.mParameter.mParameterID = inParameterID;
		paramEvent.mArgument.mParameter.mAudioUnit = GetEditAudioUnit();
		paramEvent.mArgument.mParameter.mScope = kAudioUnitScope_Global;
		paramEvent.mArgument.mParameter.mElement = 0;
		result = AUEventListenerNotify(NULL, NULL, &paramEvent);
	}

#ifdef AU_DO_OLD_STYLE_PARAMETER_CHANGE_GESTURES
	// as a back-up, also still do the old way, until it's enough obsolete
	CAAUParameter auvp(GetEditAudioUnit(), (AudioUnitParameterID)inParameterID, kAudioUnitScope_Global, (AudioUnitElement)0);
	if (inEventType == kAudioUnitEvent_BeginParameterChangeGesture)
		TellListener(auvp, kAudioUnitCarbonViewEvent_MouseDownInControl, NULL);
	else if (inEventType == kAudioUnitEvent_EndParameterChangeGesture)
		TellListener(auvp, kAudioUnitCarbonViewEvent_MouseUpInControl, NULL);
#endif

	return result;
}
#endif

//-----------------------------------------------------------------------------
void DfxGuiEditor::automationgesture_begin(long inParameterID)
{
#ifdef TARGET_API_AUDIOUNIT
	SendAUParameterEvent((AudioUnitParameterID)inParameterID, kAudioUnitEvent_BeginParameterChangeGesture);
#endif
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::automationgesture_end(long inParameterID)
{
#ifdef TARGET_API_AUDIOUNIT
	SendAUParameterEvent((AudioUnitParameterID)inParameterID, kAudioUnitEvent_EndParameterChangeGesture);
#endif
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::randomizeparameters(bool writeAutomation)
{
	AudioUnitSetProperty(GetEditAudioUnit(), kDfxPluginProperty_RandomizeParameters, 
				kAudioUnitScope_Global, (AudioUnitElement)0, &writeAutomation, sizeof(bool));
}

//-----------------------------------------------------------------------------
// set the control that is currently idly under the mouse pointer, if any (NULL if none)
void DfxGuiEditor::setCurrentControl_mouseover(DGControl * inNewMousedOverControl)
{
	DGControl * oldcontrol = currentControl_mouseover;
	currentControl_mouseover = inNewMousedOverControl;
	// post notification if the mouseovered control has changed
	if (oldcontrol != inNewMousedOverControl)
		mouseovercontrolchanged(inNewMousedOverControl);
}

//-----------------------------------------------------------------------------
double DfxGuiEditor::getparameter_f(long inParameterID)
{
	DfxParameterValueRequest request;
	UInt32 dataSize = sizeof(request);
	request.parameterID = inParameterID;
	request.valueItem = kDfxParameterValueItem_current;
	request.valueType = kDfxParamValueType_float;

	if (AudioUnitGetProperty(GetEditAudioUnit(), kDfxPluginProperty_ParameterValue, 
							kAudioUnitScope_Global, (AudioUnitElement)0, &request, &dataSize) 
							== noErr)
		return request.value.f;
	else
		return 0.0;
}

//-----------------------------------------------------------------------------
long DfxGuiEditor::getparameter_i(long inParameterID)
{
	DfxParameterValueRequest request;
	UInt32 dataSize = sizeof(request);
	request.parameterID = inParameterID;
	request.valueItem = kDfxParameterValueItem_current;
	request.valueType = kDfxParamValueType_int;

	if (AudioUnitGetProperty(GetEditAudioUnit(), kDfxPluginProperty_ParameterValue, 
							kAudioUnitScope_Global, (AudioUnitElement)0, &request, &dataSize) 
							== noErr)
		return request.value.i;
	else
		return 0;
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::getparameter_b(long inParameterID)
{
	DfxParameterValueRequest request;
	UInt32 dataSize = sizeof(request);
	request.parameterID = inParameterID;
	request.valueItem = kDfxParameterValueItem_current;
	request.valueType = kDfxParamValueType_boolean;

	if (AudioUnitGetProperty(GetEditAudioUnit(), kDfxPluginProperty_ParameterValue, 
							kAudioUnitScope_Global, (AudioUnitElement)0, &request, &dataSize) 
							== noErr)
		return request.value.b;
	else
		return false;
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::setparameter_f(long inParameterID, double inValue)
{
	automationgesture_begin(inParameterID);

	DfxParameterValueRequest request;
	request.parameterID = inParameterID;
	request.valueItem = kDfxParameterValueItem_current;
	request.valueType = kDfxParamValueType_float;
	request.value.f = inValue;

	AudioUnitSetProperty(GetEditAudioUnit(), kDfxPluginProperty_ParameterValue, 
				kAudioUnitScope_Global, (AudioUnitElement)0, &request, sizeof(request));

	automationgesture_end(inParameterID);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::setparameter_i(long inParameterID, long inValue)
{
	automationgesture_begin(inParameterID);

	DfxParameterValueRequest request;
	request.parameterID = inParameterID;
	request.valueItem = kDfxParameterValueItem_current;
	request.valueType = kDfxParamValueType_int;
	request.value.i = inValue;

	AudioUnitSetProperty(GetEditAudioUnit(), kDfxPluginProperty_ParameterValue, 
				kAudioUnitScope_Global, (AudioUnitElement)0, &request, sizeof(request));

	automationgesture_end(inParameterID);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::setparameter_b(long inParameterID, bool inValue)
{
	automationgesture_begin(inParameterID);

	DfxParameterValueRequest request;
	request.parameterID = inParameterID;
	request.valueItem = kDfxParameterValueItem_current;
	request.valueType = kDfxParamValueType_boolean;
	request.value.b = inValue;

	AudioUnitSetProperty(GetEditAudioUnit(), kDfxPluginProperty_ParameterValue, 
				kAudioUnitScope_Global, (AudioUnitElement)0, &request, sizeof(request));

	automationgesture_end(inParameterID);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::getparametervaluestring(long inParameterID, char * outText)
{
	DfxParameterValueStringRequest request;
	UInt32 dataSize = sizeof(request);
	request.parameterID = inParameterID;
	request.stringIndex = getparameter_i(inParameterID);

	if (AudioUnitGetProperty(GetEditAudioUnit(), kDfxPluginProperty_ParameterValueString, 
							kAudioUnitScope_Global, (AudioUnitElement)0, &request, &dataSize) 
							== noErr)
		strcpy(outText, request.valueString);
}

#if TARGET_PLUGIN_USES_MIDI
//-----------------------------------------------------------------------------
void DfxGuiEditor::setmidilearning(bool newLearnMode)
{
	AudioUnitSetProperty(GetEditAudioUnit(), kDfxPluginProperty_MidiLearn, 
				kAudioUnitScope_Global, (AudioUnitElement)0, &newLearnMode, sizeof(bool));
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::getmidilearning()
{
	bool learnMode;
	UInt32 dataSize = sizeof(learnMode);
	if (AudioUnitGetProperty(GetEditAudioUnit(), kDfxPluginProperty_MidiLearn, 
							kAudioUnitScope_Global, (AudioUnitElement)0, &learnMode, &dataSize) 
							== noErr)
		return learnMode;
	else
		return false;
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::resetmidilearn()
{
	bool nud;	// irrelavant
	AudioUnitSetProperty(GetEditAudioUnit(), kDfxPluginProperty_ResetMidiLearn, 
				kAudioUnitScope_Global, (AudioUnitElement)0, &nud, sizeof(bool));
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::setmidilearner(long parameterIndex)
{
	AudioUnitSetProperty(GetEditAudioUnit(), kDfxPluginProperty_MidiLearner, 
				kAudioUnitScope_Global, (AudioUnitElement)0, &parameterIndex, sizeof(long));
}

//-----------------------------------------------------------------------------
long DfxGuiEditor::getmidilearner()
{
	long learner;
	UInt32 dataSize = sizeof(learner);
	if (AudioUnitGetProperty(GetEditAudioUnit(), kDfxPluginProperty_MidiLearner, 
							kAudioUnitScope_Global, (AudioUnitElement)0, &learner, &dataSize) 
							== noErr)
		return learner;
	else
		return DFX_PARAM_INVALID_ID;
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::ismidilearner(long parameterIndex)
{
	return (getmidilearner() == parameterIndex);
}

#endif
// TARGET_PLUGIN_USES_MIDI



#if MAC
//-----------------------------------------------------------------------------
static pascal OSStatus DGWindowEventHandler(EventHandlerCallRef myHandler, EventRef inEvent, void * inUserData)
{
	bool eventWasHandled = false;
	DfxGuiEditor * ourOwnerEditor = (DfxGuiEditor*) inUserData;

	switch ( GetEventClass(inEvent) )
	{
		case kEventClassMouse:
			eventWasHandled = ourOwnerEditor->HandleMouseEvent(inEvent);
			break;
		case kEventClassKeyboard:
			eventWasHandled = ourOwnerEditor->HandleKeyboardEvent(inEvent);
			break;
		case kEventClassCommand:
			eventWasHandled = ourOwnerEditor->HandleCommandEvent(inEvent);
			break;
		default:
			return eventClassIncorrectErr;
	}

	if (eventWasHandled)
		return noErr;
	else
		return eventNotHandledErr;
}
#endif

#if MAC
//-----------------------------------------------------------------------------
bool DfxGuiEditor::HandleMouseEvent(EventRef inEvent)
{
	UInt32 inEventKind = GetEventKind(inEvent);
	OSStatus error;

	HIPoint mouseLocation;
	error = GetEventParameter(inEvent, kEventParamMouseLocation, typeHIPoint, NULL, sizeof(HIPoint), NULL, &mouseLocation);
	if (error != noErr)
		return false;


// follow the mouse around, see if it falls over any of our hot spots
	if (inEventKind == kEventMouseMoved)
return false;
/*
	{
		// remember current port
		CGrafPtr oldport;
		GetPort(&oldport);
		// switch to our window's port
		WindowRef window = NULL;
		error = GetEventParameter(inEvent, kEventParamWindowRef, typeWindowRef, NULL, sizeof(WindowRef), NULL, &window);
		if ( (error != noErr) || (window == NULL) )
			return false;
		SetPortWindowPort(window);

		Point mouseLocation_i;
		mouseLocation_i.h = (short) mouseLocation.x;
		mouseLocation_i.v = (short) mouseLocation.y;
		// figure out which control is currently under the mouse, if any
		GlobalToLocal(&mouseLocation_i);
		ControlRef underCarbonControl = FindControlUnderMouse(mouseLocation_i, window, NULL);
		DGControl * underDGControl = NULL;
		if (underCarbonControl != NULL)
			underDGControl = getDGControlByCarbonControlRef(underCarbonControl);
		setCurrentControl_mouseover(underDGControl);

		// restore the original port
		SetPort(oldport);

		return false;
	}
*/


// follow the mouse when dragging (adjusting) a GUI control
	DGControl * ourControl = currentControl_clicked;
	if (ourControl == NULL)
		return false;

	UInt32 modifiers = 0;
	error = GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32, NULL, sizeof(UInt32), NULL, &modifiers);
	if (error != noErr)
		modifiers = GetCurrentEventKeyModifiers();
	unsigned long keyModifiers = 0;
	if (modifiers & cmdKey)
		keyModifiers |= kDGKeyModifier_accel;
	if ( (modifiers & optionKey) || (modifiers & rightOptionKey) )
		keyModifiers |= kDGKeyModifier_alt;
	if ( (modifiers & shiftKey) || (modifiers & rightShiftKey) )
		keyModifiers |= kDGKeyModifier_shift;
	if ( (modifiers & controlKey) || (modifiers & rightControlKey) )
		keyModifiers |= kDGKeyModifier_extra;

// orient the mouse coordinates as though the control were at 0, 0 (for convenience)
	WindowRef window = NULL;
	error = GetEventParameter(inEvent, kEventParamWindowRef, typeWindowRef, NULL, sizeof(WindowRef), NULL, &window);
	// XXX should we bail if this fails?
	if ( (error == noErr) && (window != NULL) )
	{
		// the content area of the window (i.e. not the title bar or any borders)
		Rect windowBounds;
		GetWindowBounds(window, kWindowGlobalPortRgn, &windowBounds);
		if ( IsWindowCompositing() )
		{
			Rect paneBounds;
			GetControlBounds(mCarbonPane, &paneBounds);
			OffsetRect(&windowBounds, paneBounds.left, paneBounds.top);
		}
		// the position of the control relative to the top left corner of the window content area
		Rect controlBounds;
		GetControlBounds(ourControl->getCarbonControl(), &controlBounds);
		mouseLocation.x -= (float) (windowBounds.left + controlBounds.left);
		mouseLocation.y -= (float) (windowBounds.top + controlBounds.top);
	}


	if (inEventKind == kEventMouseDragged)
	{
		UInt32 mouseButtons = 1;	// bit 0 is mouse button 1, bit 1 is button 2, etc.
		error = GetEventParameter(inEvent, kEventParamMouseChord, typeUInt32, NULL, sizeof(UInt32), NULL, &mouseButtons);
		if (error != noErr)
			mouseButtons = GetCurrentEventButtonState();

		ourControl->mouseTrack(mouseLocation.x, mouseLocation.y, mouseButtons, keyModifiers);

		return false;	// let it fall through in case the host needs the event
	}

	if (inEventKind == kEventMouseUp)
	{
		ourControl->mouseUp(mouseLocation.x, mouseLocation.y, keyModifiers);

		currentControl_clicked = NULL;

		// do this to make Logic's touch automation work
		if ( ourControl->isParameterAttached() )
		{
			automationgesture_end( ourControl->getParameterID() );
//			fprintf(stderr, "DGControlMouseHandler -> TellListener(MouseUp, %ld)\n", ourControl->getParameterID());
		}

		return false;	// let it fall through in case the host needs the event
	}

	return false;
}
#endif
// MAC

#if MAC
//-----------------------------------------------------------------------------
bool DfxGuiEditor::HandleKeyboardEvent(EventRef inEvent)
{
	UInt32 inEventKind = GetEventKind(inEvent);

	if ( (inEventKind == kEventRawKeyDown) || (inEventKind == kEventRawKeyRepeat) )
	{
		UInt32 keyCode = 0;
		GetEventParameter(inEvent, kEventParamKeyCode, typeUInt32, NULL, sizeof(UInt32), NULL, &keyCode);
		unsigned char charCode = 0;
		GetEventParameter(inEvent, kEventParamKeyMacCharCodes, typeChar, NULL, sizeof(char), NULL, &charCode);
		UInt32 modifiers = 0;
		OSStatus error = GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32, NULL, sizeof(UInt32), NULL, &modifiers);
		if (error != noErr)
			modifiers = GetCurrentEventKeyModifiers();
//fprintf(stderr, "keyCode = %lu,  charCode = ", keyCode);
//if ( (charCode > 0x7F) || iscntrl(charCode) ) fprintf(stderr, "0x%.2X\n", charCode);
//else fprintf(stderr, "%c\n", charCode);

		if ( ((keyCode == 44) && (modifiers & cmdKey)) || 
				(charCode == kHelpCharCode) )
		{
			if (launch_documentation() == noErr)
				return true;
		}

		return false;
	}

	return false;
}
#endif
// MAC

#if MAC
//-----------------------------------------------------------------------------
bool DfxGuiEditor::HandleCommandEvent(EventRef inEvent)
{
	UInt32 inEventKind = GetEventKind(inEvent);

	if (inEventKind == kEventCommandProcess)
	{
		HICommand hiCommand;
		OSStatus error = GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &hiCommand);
		if (error != noErr)
			error = GetEventParameter(inEvent, kEventParamHICommand, typeHICommand, NULL, sizeof(HICommand), NULL, &hiCommand);
		if (error != noErr)
			return false;

		if (hiCommand.commandID == kHICommandAppHelp)
		{
//fprintf(stderr, "command ID = kHICommandAppHelp\n");
			if (launch_documentation() == noErr)
				return true;
		}
//else fprintf(stderr, "command ID = %.4s\n", (char*) &(hiCommand.commandID));

		return false;
	}

	return false;
}
#endif
// MAC


#if MAC
//-----------------------------------------------------------------------------
static pascal OSStatus DGControlEventHandler(EventHandlerCallRef myHandler, EventRef inEvent, void * inUserData)
{
	if (GetEventClass(inEvent) != kEventClassControl)
		return eventClassIncorrectErr;

	bool eventWasHandled = ((DfxGuiEditor*)inUserData)->HandleControlEvent(inEvent);
	if (eventWasHandled)
		return noErr;
	else
		return eventNotHandledErr;
}
#endif

#if MAC
//-----------------------------------------------------------------------------
bool DfxGuiEditor::HandleControlEvent(EventRef inEvent)
{
	UInt32 inEventKind = GetEventKind(inEvent);

	ControlRef ourCarbonControl = NULL;
	GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, NULL, sizeof(ControlRef), NULL, &ourCarbonControl);
	DGControl * ourDGControl = NULL;
	ourDGControl = getDGControlByCarbonControlRef(ourCarbonControl);

/*
	// the Carbon control reference has not been added yet, so our DGControl pointer is NULL, because we can't look it up by ControlRef yet
	if (inEventKind == kEventControlInitialize)
	{
		UInt32 dfxControlFeatures = kControlHandlesTracking | kControlSupportsDataAccess | kControlSupportsGetRegion;
		SetEventParameter(inEvent, kEventParamControlFeatures, typeUInt32, sizeof(dfxControlFeatures), &dfxControlFeatures);
		return noErr;
	}
*/

	if (ourDGControl != NULL)
	{
		switch (inEventKind)
		{
			case kEventControlDraw:
//fprintf(stderr, "kEventControlDraw\n");
				{
					CGContextRef context = NULL;
					CGrafPtr windowPort = NULL;
					long portHeight;
					bool gotAutoContext = InitControlDrawingContext(inEvent, context, windowPort, portHeight);
					if ( (context == NULL) || (windowPort == NULL) )
						return false;

					// XXX disable anti-aliased drawing for image rendering
					CGContextSetShouldAntialias(context, false);
					ourDGControl->do_draw(context, portHeight);

					CleanupControlDrawingContext(context, gotAutoContext, windowPort);
				}
				return true;

			case kEventControlHitTest:
				{
//fprintf(stderr, "kEventControlHitTest\n");
					// get mouse location
					Point mouseLocation;
					GetEventParameter(inEvent, kEventParamMouseLocation, typeQDPoint, NULL, sizeof(Point), NULL, &mouseLocation);
					// get control bounds rect
					Rect controlBounds;
					GetControlBounds(ourCarbonControl, &controlBounds);
					ControlPartCode hitPart = kControlNoPart;
					// see if the mouse is inside the control bounds (it probably is, wouldn't you say?)
//					if ( PtInRgn(mouseLocation, controlBounds) )
					{
						hitPart = kControlIndicatorPart;	// scroll handle
						// also there is kControlButtonPart, kControlCheckBoxPart, kControlPicturePart
						setCurrentControl_mouseover(ourDGControl);
					}
					SetEventParameter(inEvent, kEventParamControlPart, typeControlPartCode, sizeof(hitPart), &hitPart);
				}
//				return false;	// let other event listeners have this if they want it
				return true;

/*
			case kEventControlHit:
				{
fprintf(stderr, "kEventControlHit\n");
				}
				return true;
*/

			case kEventControlTrack:
				{
//fprintf(stderr, "kEventControlTrack\n");
//					ControlPartCode whatPart = kControlIndicatorPart;
					ControlPartCode whatPart = kControlNoPart;	// cuz otherwise we get a Hit event which triggers AUCVControl automation end
					SetEventParameter(inEvent, kEventParamControlPart, typeControlPartCode, sizeof(whatPart), &whatPart);
				}
//			case kEventControlClick:
//if (inEventKind == kEventControlClick) fprintf(stderr, "kEventControlClick\n");
			case kEventControlContextualMenuClick:
//if (inEventKind == kEventControlContextualMenuClick) fprintf(stderr, "kEventControlContextualMenuClick\n");
				{
					setCurrentControl_mouseover(ourDGControl);

					UInt32 mouseButtons = GetCurrentEventButtonState();	// bit 0 is mouse button 1, bit 1 is button 2, etc.
//					UInt32 mouseButtons = 1;	// bit 0 is mouse button 1, bit 1 is button 2, etc.
					// XXX kEventParamMouseChord does not exist for control class events, only mouse class
					// XXX hey, that's not what the headers say, they say that kEventControlClick should have that parameter
//					GetEventParameter(inEvent, kEventParamMouseChord, typeUInt32, NULL, sizeof(UInt32), NULL, &mouseButtons);

					HIPoint mouseLocation;
					GetEventParameter(inEvent, kEventParamMouseLocation, typeHIPoint, NULL, sizeof(HIPoint), NULL, &mouseLocation);
//fprintf(stderr, "mousef.x = %.0f, mousef.y = %.0f\n", mouseLocation.x, mouseLocation.y);
					// XXX only kEventControlClick gives global mouse coordinates for kEventParamMouseLocation?
					if ( (inEventKind == kEventControlContextualMenuClick) || (inEventKind == kEventControlTrack) )
					{
						Point mouseLocation_i;
						GetGlobalMouse(&mouseLocation_i);
						mouseLocation.x = (float) mouseLocation_i.h;
						mouseLocation.y = (float) mouseLocation_i.v;
//fprintf(stderr, "mouse.x = %d, mouse.y = %d\n\n", mouseLocation_i.h, mouseLocation_i.v);
					}

					// orient the mouse coordinates as though the control were at 0, 0 (for convenience)
					// the content area of the window (i.e. not the title bar or any borders)
					Rect windowBounds;
					GetWindowBounds(GetControlOwner(ourCarbonControl), kWindowGlobalPortRgn, &windowBounds);
					if ( IsWindowCompositing() )
					{
						Rect paneBounds;
						GetControlBounds(mCarbonPane, &paneBounds);
						OffsetRect(&windowBounds, paneBounds.left, paneBounds.top);
					}
					// the position of the control relative to the top left corner of the window content area
					Rect controlBounds;
					GetControlBounds(ourCarbonControl, &controlBounds);
					mouseLocation.x -= (float) (windowBounds.left + controlBounds.left);
					mouseLocation.y -= (float) (windowBounds.top + controlBounds.top);

					UInt32 modifiers = 0;
					OSStatus error = GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32, NULL, sizeof(UInt32), NULL, &modifiers);
					if (error != noErr)
						modifiers = GetCurrentEventKeyModifiers();
					unsigned long keyModifiers = 0;
					if (modifiers & cmdKey)
						keyModifiers |= kDGKeyModifier_accel;
					if ( (modifiers & optionKey) || (modifiers & rightOptionKey) )
						keyModifiers |= kDGKeyModifier_alt;
					if ( (modifiers & shiftKey) || (modifiers & rightShiftKey) )
						keyModifiers |= kDGKeyModifier_shift;
					if ( (modifiers & controlKey) || (modifiers & rightControlKey) )
						keyModifiers |= kDGKeyModifier_extra;

					// do this to make Logic's touch automation work
					// AUCarbonViewControl::HandleEvent will catch ControlClick but not ControlContextualMenuClick
					if ( ourDGControl->isParameterAttached() && (inEventKind == kEventControlContextualMenuClick) )
					{
						automationgesture_begin( ourDGControl->getParameterID() );
//						fprintf(stderr, "DGControlEventHandler -> TellListener(MouseDown, %ld)\n", ourDGControl->getParameterID());
					}

					ourDGControl->mouseDown(mouseLocation.x, mouseLocation.y, mouseButtons, keyModifiers);
					currentControl_clicked = ourDGControl;
				}
				return true;

			case kEventControlValueFieldChanged:
				// XXX it seems that I need to manually invalidate the control to get it to 
				// redraw in response to a value change in compositing mode ?
				if ( IsWindowCompositing() )
					ourDGControl->redraw();
				return false;

			default:
				return false;
		}
	}

	return false;
}
#endif
// MAC
