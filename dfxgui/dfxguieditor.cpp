#include "dfxgui.h"

#include "dfxplugin.h"
#include "dfx-au-utilities.h"



#if TARGET_OS_MAC
static pascal OSStatus DFXGUI_ControlEventHandler(EventHandlerCallRef, EventRef, void * inUserData);
static pascal OSStatus DFXGUI_WindowEventHandler(EventHandlerCallRef, EventRef, void * inUserData);
static pascal void DFXGUI_IdleTimerProc(EventLoopTimerRef inTimer, void * inUserData);
static pascal OSStatus DFXGUI_TextEntryEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void * inUserData);
static pascal OSStatus DFXGUI_WindowTransparencyEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void * inUserData);
#endif

// for now this is a good idea, until it's totally obsoleted
#define AU_DO_OLD_STYLE_PARAMETER_CHANGE_GESTURES

#if TARGET_OS_MAC
static EventHandlerUPP gControlHandlerUPP = NULL;
static EventHandlerUPP gWindowEventHandlerUPP = NULL;
static EventLoopTimerUPP gIdleTimerUPP = NULL;
static EventHandlerUPP gTextEntryEventHandlerUPP = NULL;
static EventHandlerUPP gWindowTransparencyEventHandlerUPP = NULL;
static ControlActionUPP gTransparencyControlActionUPP = NULL;

static const CFStringRef kDfxGui_NibName = CFSTR("dfxgui");
const OSType kDfxGui_ControlSignature = DESTROYFX_ID;
const SInt32 kDfxGui_TextEntryControlID = 1;
const SInt32 kDfxGui_TransparencySliderControlID = 0;
#endif

#ifdef TARGET_API_AUDIOUNIT
	const Float32 kDfxGui_ParameterNotificationInterval = 30.0 * kEventDurationMillisecond;	// 30 ms parameter notification update interval
//	static const CFStringRef kDfxGui_AUPresetFileUTI = (kUTTypeXML != NULL) ? kUTTypeXML : CFSTR("public.xml");	// XXX only available in Mac OS X 10.4 or higher
	static const CFStringRef kDfxGui_AUPresetFileUTI = CFSTR("org.destroyfx.aupreset");
#endif


//-----------------------------------------------------------------------------
DfxGuiEditor::DfxGuiEditor(DGEditorListenerInstance inInstance)
:	AUCarbonViewBase(inInstance, kDfxGui_ParameterNotificationInterval)
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

	backgroundControl = NULL;
  
#if TARGET_OS_MAC
	idleTimer = NULL;

	dgControlSpec.defType = kControlDefObjectClass;
	dgControlSpec.u.classRef = NULL;
	windowEventHandlerRef = NULL;

	fontsATSContainer = kATSFontContainerRefUnspecified;
	fontsWereActivated = false;	// guilty until proven innocent

	clipboardRef = NULL;

	textEntryWindow = NULL;
	textEntryControl = NULL;
	textEntryResultString = NULL;
	windowTransparencyWindow = NULL;
#endif

	currentControl_clicked = NULL;
	currentControl_mouseover = NULL;
	mousedOverControlsList = NULL;
}

//-----------------------------------------------------------------------------
DfxGuiEditor::~DfxGuiEditor()
{
	#if TARGET_PLUGIN_USES_MIDI
		#ifdef TARGET_API_AUDIOUNIT
			if (GetEditAudioUnit() != NULL)
		#endif
			setmidilearning(false);
	#endif

#if TARGET_OS_MAC
	if (idleTimer != NULL)
		RemoveEventLoopTimer(idleTimer);
	idleTimer = NULL;

	if (windowEventHandlerRef != NULL)
		RemoveEventHandler(windowEventHandlerRef);
	windowEventHandlerRef = NULL;
#endif

	// deleting a list link also deletes the list's item
	while (controlsList != NULL)
	{
		DGControlsList * tempcl = controlsList->next;
		delete controlsList;
		controlsList = tempcl;
	}
	while (imagesList != NULL)
	{
		DGImagesList * tempil = imagesList->next;
		delete imagesList;
		imagesList = tempil;
	}
	while (mousedOverControlsList != NULL)
	{
		DGMousedOverControlsList * tempmocl = mousedOverControlsList->next;
		delete mousedOverControlsList;
		mousedOverControlsList = tempmocl;
	}

#if TARGET_OS_MAC
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

	if (windowTransparencyWindow != NULL)
	{
		HideWindow(windowTransparencyWindow);
		DisposeWindow(windowTransparencyWindow);
	}
	windowTransparencyWindow = NULL;

	if (clipboardRef != NULL)
		CFRelease(clipboardRef);
	clipboardRef = NULL;
#endif

	if (backgroundControl != NULL)
		delete backgroundControl;
	backgroundControl = NULL;
}


#ifdef TARGET_API_AUDIOUNIT
//-----------------------------------------------------------------------------
OSStatus DfxGuiEditor::CreateUI(Float32 inXOffset, Float32 inYOffset)
{
	#if TARGET_PLUGIN_USES_MIDI
		setmidilearning(false);
	#endif


// create the background control object using the embedding pane control
	backgroundControl = new DGBackgroundControl(this, GetCarbonPane());


// create our HIToolbox object class for common event handling amongst our custom Carbon Controls
	EventTypeSpec toolboxClassEvents[] = {
		{ kEventClassControl, kEventControlDraw },
//		{ kEventClassControl, kEventControlInitialize },
		{ kEventClassControl, kEventControlHitTest },
		{ kEventClassControl, kEventControlTrack },
//		{ kEventClassControl, kEventControlHit }, 
//		{ kEventClassControl, kEventControlClick }, 
		{ kEventClassControl, kEventControlContextualMenuClick }, 
		{ kEventClassControl, kEventControlValueFieldChanged }, 
		{ kEventClassControl, kEventControlTrackingAreaEntered }, 
		{ kEventClassControl, kEventControlTrackingAreaExited }, 
		{ kEventClassMouse, kEventMouseEntered }, 
		{ kEventClassMouse, kEventMouseExited }, 
	};
	ToolboxObjectClassRef newControlClass = NULL;
	if (gControlHandlerUPP == NULL)
		gControlHandlerUPP = NewEventHandlerUPP(DFXGUI_ControlEventHandler);
	UInt64 instanceAddress = (UInt64) this;
	bool noSuccessYet = true;
	while (noSuccessYet)
	{
		CFStringRef toolboxClassIDcfstring = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%s.DfxGuiControlClass.%llu"), 
																		PLUGIN_BUNDLE_IDENTIFIER, instanceAddress);
		if (toolboxClassIDcfstring != NULL)
		{
			if ( RegisterToolboxObjectClass(toolboxClassIDcfstring, NULL, GetEventTypeCount(toolboxClassEvents), toolboxClassEvents, 
											gControlHandlerUPP, this, &newControlClass) == noErr )
				noSuccessYet = false;
			CFRelease(toolboxClassIDcfstring);
		}
		instanceAddress++;
	}
	dgControlSpec.u.classRef = newControlClass;


// create the window event handler that supplements the control event handler by tracking mouse dragging, mouseover controls, etc.
	currentControl_clicked = NULL;	// make sure that it ain't nuthin
	mousedOverControlsList = NULL;
	setCurrentControl_mouseover(NULL);
	EventTypeSpec windowEvents[] = {
									{ kEventClassMouse, kEventMouseDragged }, 
									{ kEventClassMouse, kEventMouseUp }, 
									{ kEventClassMouse, kEventMouseWheelMoved }, 
									{ kEventClassKeyboard, kEventRawKeyDown }, 
									{ kEventClassKeyboard, kEventRawKeyRepeat }, 
									{ kEventClassCommand, kEventCommandProcess }, 
								};
	if (gWindowEventHandlerUPP == NULL)
		gWindowEventHandlerUPP = NewEventHandlerUPP(DFXGUI_WindowEventHandler);
	InstallEventHandler(GetWindowEventTarget(GetCarbonWindow()), gWindowEventHandlerUPP, 
						GetEventTypeCount(windowEvents), windowEvents, this, &windowEventHandlerRef);


// register for HitTest events on the background embedding pane so that we know when the mouse hovers over it
	EventTypeSpec paneEvents[] = {
								{ kEventClassControl, kEventControlDraw }, 
								{ kEventClassControl, kEventControlApplyBackground }, 
//								{ kEventClassControl, kEventControlHitTest }, 
								{ kEventClassControl, kEventControlContextualMenuClick }, 
								};
	WantEventTypes(GetControlEventTarget(GetCarbonPane()), GetEventTypeCount(paneEvents), paneEvents);


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
			CFRelease(bundleResourcesDirCFURL);
		}
	}


	// let the child class do its thing
	long openErr = open();
	if (openErr == noErr)
	{
		// set the size of the embedding pane
		if (backgroundControl != NULL)
			SizeControl(GetCarbonPane(), (SInt16) (backgroundControl->getWidth()), (SInt16) (backgroundControl->getHeight()));

		if (gIdleTimerUPP == NULL)
			gIdleTimerUPP = NewEventLoopTimerUPP(DFXGUI_IdleTimerProc);
		InstallEventLoopTimer(GetCurrentEventLoop(), 0.0, kEventDurationMillisecond * 50.0, gIdleTimerUPP, this, &idleTimer);

		// embed/activate every control
		DGControlsList * tempcl = controlsList;
		while (tempcl != NULL)
		{
			tempcl->control->embed();
			tempcl = tempcl->next;
		}

		// allow for anything that might need to happen after the above post-opening stuff is finished
		post_open();
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

#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
// inEvent is expected to be an event of the class kEventClassControl.  
// outPort will be null if a CGContextRef was available from the event parameters 
// (which happens with compositing windows) or non-null if the CGContext had to be created 
// for the control's window port QuickDraw-style.  
DGGraphicsContext * DGInitControlDrawingContext(EventRef inEvent, CGrafPtr & outPort)
{
	outPort = NULL;
	if (inEvent == NULL)
		return NULL;

	OSStatus status;
	CGContextRef cgContext = NULL;
	long portHeight = 0;

	// if we are in compositing mode, then we can get a CG context already set up and clipped and whatnot
	status = GetEventParameter(inEvent, kEventParamCGContextRef, typeCGContextRef, NULL, sizeof(cgContext), NULL, &cgContext);
	if ( (status == noErr) && (cgContext != NULL) )
	{
//		CGRect contextBounds = CGContextGetClipBoundingBox(cgContext);
//		portHeight = (long) (contextBounds.size.height);
		portHeight = 0;
	}
	// otherwise we need to get it from the QD port
	else
	{
		// if we received a graphics port parameter, use that...
		status = GetEventParameter(inEvent, kEventParamGrafPort, typeGrafPtr, NULL, sizeof(outPort), NULL, &outPort);
		// ... otherwise use the current graphics port
		if ( (status != noErr) || (outPort == NULL) )
			GetPort(&outPort);	// XXX deprecated in Mac OS X 10.4
		if (outPort == NULL)
			return NULL;
		Rect portBounds = {0};
		GetPortBounds(outPort, &portBounds);
		portHeight = portBounds.bottom - portBounds.top;

		status = QDBeginCGContext(outPort, &cgContext);
		if ( (status != noErr) || (cgContext == NULL) )
			return NULL;
	}

	// create our result graphics context now with the platform-specific graphics context
	DGGraphicsContext * outContext = new DGGraphicsContext(cgContext);
	outContext->setPortHeight(portHeight);

	CGContextSaveGState(cgContext);
	if (outPort != NULL)
		SyncCGContextOriginWithPort(cgContext, outPort);	// XXX deprecated in Mac OS X 10.4
#ifdef FLIP_CG_COORDINATES
	if (outPort != NULL)
	{
		// this lets me position things with non-upside-down coordinates, 
		// but unfortunately draws all images and text upside down...
		CGContextTranslateCTM(cgContext, 0.0f, (float)portHeight);
		CGContextScaleCTM(cgContext, 1.0f, -1.0f);
	}
#endif
#ifndef FLIP_CG_COORDINATES
	if (outPort == NULL)
	{
#endif
	// but this flips text drawing back right-side-up
	CGAffineTransform textCTM = CGAffineTransformMake(1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	CGContextSetTextMatrix(cgContext, textCTM);
#ifndef FLIP_CG_COORDINATES
	}
#endif

	// define the clipping region if we are not compositing and had to create our own context
	if (outPort != NULL)
	{
		ControlRef carbonControl = NULL;
		status = GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, NULL, sizeof(ControlRef), NULL, &carbonControl);
		if ( (status == noErr) && (carbonControl != NULL) )
		{
			CGRect clipRect;
			status = HIViewGetBounds(carbonControl, &clipRect);
			if (status == noErr)
			{
#ifndef FLIP_CG_COORDINATES
				clipRect.origin.y = (float)portHeight - (clipRect.origin.y + clipRect.size.height);
#endif
				CGContextClipToRect(cgContext, clipRect);
			}
		}
	}

	return outContext;
}

//-----------------------------------------------------------------------------
// inPort should be what you got from DGInitControlDrawingContext().  
// It is null if inContext came with the control event (compositing window behavior) or 
// non-null if inContext was created via QDBeginCGContext(), and hence needs to be terminated.
void DGCleanupControlDrawingContext(DGGraphicsContext * inContext, CGrafPtr inPort)
{
	if (inContext == NULL)
		return;
	if (inContext->getPlatformGraphicsContext() == NULL)
		return;

	CGContextRestoreGState( inContext->getPlatformGraphicsContext() );
	CGContextSynchronize( inContext->getPlatformGraphicsContext() );

	if (inPort != NULL)
		inContext->endQDContext(inPort);

	delete inContext;
}
#endif
// TARGET_OS_MAC

#ifdef TARGET_API_AUDIOUNIT
//-----------------------------------------------------------------------------
bool DfxGuiEditor::HandleEvent(EventRef inEvent)
{
	if (GetEventClass(inEvent) == kEventClassControl)
	{
		ControlRef carbonControl = NULL;
		OSStatus status = GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, NULL, sizeof(ControlRef), NULL, &carbonControl);
		if (status != noErr)
			carbonControl = NULL;

		UInt32 inEventKind = GetEventKind(inEvent);

		if (inEventKind == kEventControlDraw)
		{
			if ( (carbonControl != NULL) && (carbonControl == GetCarbonPane()) )
			{
				CGrafPtr windowPort = NULL;
				DGGraphicsContext * context = DGInitControlDrawingContext(inEvent, windowPort);
				if (context == NULL)
					return false;

//				context->setAntialias(false);	// XXX disable anti-aliased drawing for image rendering
				DrawBackground(context);

				DGCleanupControlDrawingContext(context, windowPort);
				return true;
			}
		}

/*
		// we want to catch when the mouse hovers over onto the background area
		else if ( (inEventKind == kEventControlHitTest) || (inEventKind == kEventControlClick) )
		{
			if (carbonControl == GetCarbonPane())
				setCurrentControl_mouseover(NULL);	// we don't count the background
		}
*/

		else if (inEventKind == kEventControlApplyBackground)
		{
//fprintf(stderr, "mCarbonPane HandleEvent(kEventControlApplyBackground)\n");
			return false;
		}

		else if (inEventKind == kEventControlContextualMenuClick)
		{
			if ( (carbonControl != NULL) && (carbonControl == GetCarbonPane()) )
			{
				currentControl_clicked = NULL;
				if (getBackgroundControl() != NULL)
					return getBackgroundControl()->do_contextualMenuClick();
			}
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

#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
static pascal void DFXGUI_IdleTimerProc(EventLoopTimerRef inTimer, void * inUserData)
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

#if TARGET_OS_MAC
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
#endif

//-----------------------------------------------------------------------------
void DfxGuiEditor::DrawBackground(DGGraphicsContext * inContext)
{
	if (backgroundControl != NULL)
		backgroundControl->draw(inContext);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::SetBackgroundImage(DGImage * inBackgroundImage)
{
	if (backgroundControl != NULL)
		backgroundControl->setImage(inBackgroundImage);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::SetBackgroundColor(DGColor inBackgroundColor)
{
	if (backgroundControl != NULL)
		backgroundControl->setColor(inBackgroundColor);
}

//-----------------------------------------------------------------------------
float DfxGuiEditor::getWindowTransparency()
{
#ifdef TARGET_API_AUDIOUNIT
	float resultAlpha = 1.0f;
	OSStatus status = GetWindowAlpha(GetCarbonWindow(), &resultAlpha);
	if (status == noErr)
		return resultAlpha;
	else
		return 1.0f;
#endif
}
//-----------------------------------------------------------------------------
void DfxGuiEditor::setWindowTransparency(float inTransparencyLevel)
{
#ifdef TARGET_API_AUDIOUNIT
	SetWindowAlpha(GetCarbonWindow(), inTransparencyLevel);
#endif
}

//-----------------------------------------------------------------------------
// this function looks for the plugin's documentation file in the appropriate system location, 
// within a given file system domain, and returns a CFURLRef for the file if it is found, 
// and NULL otherwise (or if some error is encountered along the way)
CFURLRef DFX_FindDocumentationFileInDomain(CFStringRef inDocsFileName, short inDomain)
{
	if (inDocsFileName == NULL)
		return NULL;

	// first find the base directory for the system documentation directory
	FSRef docsDirRef;
	OSErr error = FSFindFolder(inDomain, kDocumentationFolderType, kDontCreateFolder, &docsDirRef);
	if (error == noErr)
	{
		// convert the FSRef of the documentation directory to a CFURLRef (for use in the next steps)
		CFURLRef docsDirURL = CFURLCreateFromFSRef(kCFAllocatorDefault, &docsDirRef);
		if (docsDirURL != NULL)
		{
			// create a CFURL for the "manufacturer name" directory within the documentation directory
			CFURLRef dfxDocsDirURL = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, docsDirURL, CFSTR(DESTROYFX_NAME_STRING), true);
			CFRelease(docsDirURL);
			if (dfxDocsDirURL != NULL)
			{
				// create a CFURL for the documentation file within the "manufacturer name" directory
				CFURLRef docsFileURL = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, dfxDocsDirURL, inDocsFileName, false);
				CFRelease(dfxDocsDirURL);
				if (docsFileURL != NULL)
				{
					// check to see if the hypothetical documentation file actually exists 
					// (CFURLs can reference files that don't exist)
					SInt32 urlErrorCode = 0;
					CFBooleanRef docsFileExists = (CFBooleanRef) CFURLCreatePropertyFromResource(kCFAllocatorDefault, docsFileURL, kCFURLFileExists, &urlErrorCode);
					if (docsFileExists != NULL)
					{
						// only return the file's CFURL if the file exists
						if (docsFileExists == kCFBooleanTrue)
							return docsFileURL;
						CFRelease(docsFileExists);
					}
					CFRelease(docsFileURL);
				}
			}
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// XXX this function should really go somewhere else, like in that promised DFX utilities file or something like that
long launch_documentation()
{

#if TARGET_OS_MAC
	// no assumptions can be made about how long the reference is valid, 
	// and the caller should not attempt to release the CFBundleRef object
	CFBundleRef pluginBundleRef = CFBundleGetBundleWithIdentifier( CFSTR(PLUGIN_BUNDLE_IDENTIFIER) );
	if (pluginBundleRef != NULL)
	{
		CFStringRef docsFileName = CFSTR( PLUGIN_NAME_STRING" manual.html" );
		CFURLRef docsFileURL = CFBundleCopyResourceURL(pluginBundleRef, docsFileName, NULL, NULL);
		// if the documentation file is not found in the bundle, then search in appropriate system locations
		if (docsFileURL == NULL)
			docsFileURL = DFX_FindDocumentationFileInDomain(docsFileName, kUserDomain);
		if (docsFileURL == NULL)
			docsFileURL = DFX_FindDocumentationFileInDomain(docsFileName, kLocalDomain);
		if (docsFileURL == NULL)
			docsFileURL = DFX_FindDocumentationFileInDomain(docsFileName, kNetworkDomain);
		if (docsFileURL != NULL)
		{
// open the manual with the default application for the file type
#if 0
			OSStatus status = LSOpenCFURLRef(docsFileURL, NULL);
// open the manual with Apple's system Help Viewer
#else
			OSStatus status = coreFoundationUnknownErr;
			CFStringRef docsFileUrlString = CFURLGetString(docsFileURL);
			if (docsFileUrlString != NULL)
			{
				status = AHGotoPage(NULL, docsFileUrlString, NULL);
			}
#endif
			CFRelease(docsFileURL);
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
	AudioUnitEvent paramEvent = {0};
	paramEvent.mEventType = inEventType;
	paramEvent.mArgument.mParameter.mParameterID = inParameterID;
	paramEvent.mArgument.mParameter.mAudioUnit = GetEditAudioUnit();
	paramEvent.mArgument.mParameter.mScope = kAudioUnitScope_Global;
	paramEvent.mArgument.mParameter.mElement = 0;
	if (AUEventListenerNotify != NULL)
		result = AUEventListenerNotify(NULL, NULL, &paramEvent);

#ifdef AU_DO_OLD_STYLE_PARAMETER_CHANGE_GESTURES
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
void DfxGuiEditor::randomizeparameter(long inParameterID, bool inWriteAutomation)
{
	Boolean writeAutomation_fixedSize = inWriteAutomation;
	AudioUnitSetProperty(GetEditAudioUnit(), kDfxPluginProperty_RandomizeParameter, 
				kAudioUnitScope_Global, (AudioUnitElement)inParameterID, &writeAutomation_fixedSize, sizeof(writeAutomation_fixedSize));
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::randomizeparameters(bool inWriteAutomation)
{
	Boolean writeAutomation_fixedSize = inWriteAutomation;
	AudioUnitSetProperty(GetEditAudioUnit(), kDfxPluginProperty_RandomizeParameter, 
				kAudioUnitScope_Global, kAUParameterListener_AnyParameter, &writeAutomation_fixedSize, sizeof(writeAutomation_fixedSize));
}

//-----------------------------------------------------------------------------
// set the control that is currently idly under the mouse pointer, if any (NULL if none)
void DfxGuiEditor::setCurrentControl_mouseover(DGControl * inNewMousedOverControl)
{
	DGControl * oldcontrol = currentControl_mouseover;
	currentControl_mouseover = inNewMousedOverControl;
	// post notification if the mouse-overed control has changed
	if (oldcontrol != inNewMousedOverControl)
		mouseovercontrolchanged(inNewMousedOverControl);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::addMousedOverControl(DGControl * inMousedOverControl)
{
	if (inMousedOverControl == NULL)
		return;
	mousedOverControlsList = new DGMousedOverControlsList(inMousedOverControl, mousedOverControlsList);
	setCurrentControl_mouseover(inMousedOverControl);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::removeMousedOverControl(DGControl * inMousedOverControl)
{
	DGMousedOverControlsList * currentItem = mousedOverControlsList;
	DGMousedOverControlsList * prevItem = NULL;
	while (currentItem != NULL)
	{
		if (currentItem->control == inMousedOverControl)
		{
			if (prevItem != NULL)
				prevItem->next = currentItem->next;
			// this means that the item to delete is the first in the list, so move the list start to the next item
			else
				mousedOverControlsList = currentItem->next;
			delete currentItem;
			break;
		}
		prevItem = currentItem;
		currentItem = currentItem->next;
	}

	if (mousedOverControlsList == NULL)
		setCurrentControl_mouseover(NULL);
}

//-----------------------------------------------------------------------------
double DfxGuiEditor::getparameter_f(long inParameterID)
{
	DfxParameterValueRequest request;
	UInt32 dataSize = sizeof(request);
	request.inValueItem = kDfxParameterValueItem_current;
	request.inValueType = kDfxParamValueType_float;

	if (AudioUnitGetProperty(GetEditAudioUnit(), kDfxPluginProperty_ParameterValue, 
							kAudioUnitScope_Global, inParameterID, &request, &dataSize) 
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
	request.inValueItem = kDfxParameterValueItem_current;
	request.inValueType = kDfxParamValueType_int;

	if (AudioUnitGetProperty(GetEditAudioUnit(), kDfxPluginProperty_ParameterValue, 
							kAudioUnitScope_Global, inParameterID, &request, &dataSize) 
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
	request.inValueItem = kDfxParameterValueItem_current;
	request.inValueType = kDfxParamValueType_boolean;

	if (AudioUnitGetProperty(GetEditAudioUnit(), kDfxPluginProperty_ParameterValue, 
							kAudioUnitScope_Global, inParameterID, &request, &dataSize) 
							== noErr)
		return request.value.b;
	else
		return false;
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::setparameter_f(long inParameterID, double inValue, bool inWrapWithAutomationGesture)
{
	if (inWrapWithAutomationGesture)
		automationgesture_begin(inParameterID);

	DfxParameterValueRequest request;
	request.inValueItem = kDfxParameterValueItem_current;
	request.inValueType = kDfxParamValueType_float;
	request.value.f = inValue;

	AudioUnitSetProperty(GetEditAudioUnit(), kDfxPluginProperty_ParameterValue, 
				kAudioUnitScope_Global, inParameterID, &request, sizeof(request));

	if (inWrapWithAutomationGesture)
		automationgesture_end(inParameterID);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::setparameter_i(long inParameterID, long inValue, bool inWrapWithAutomationGesture)
{
	if (inWrapWithAutomationGesture)
		automationgesture_begin(inParameterID);

	DfxParameterValueRequest request;
	request.inValueItem = kDfxParameterValueItem_current;
	request.inValueType = kDfxParamValueType_int;
	request.value.i = inValue;

	AudioUnitSetProperty(GetEditAudioUnit(), kDfxPluginProperty_ParameterValue, 
				kAudioUnitScope_Global, inParameterID, &request, sizeof(request));

	if (inWrapWithAutomationGesture)
		automationgesture_end(inParameterID);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::setparameter_b(long inParameterID, bool inValue, bool inWrapWithAutomationGesture)
{
	if (inWrapWithAutomationGesture)
		automationgesture_begin(inParameterID);

	DfxParameterValueRequest request;
	request.inValueItem = kDfxParameterValueItem_current;
	request.inValueType = kDfxParamValueType_boolean;
	request.value.b = inValue;

	AudioUnitSetProperty(GetEditAudioUnit(), kDfxPluginProperty_ParameterValue, 
				kAudioUnitScope_Global, inParameterID, &request, sizeof(request));

	if (inWrapWithAutomationGesture)
		automationgesture_end(inParameterID);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::setparameter_default(long inParameterID, bool inWrapWithAutomationGesture)
{
	AudioUnitParameterInfo paramInfo = {0};
	UInt32 dataSize = sizeof(paramInfo);

	ComponentResult result = AudioUnitGetProperty(GetEditAudioUnit(), kAudioUnitProperty_ParameterInfo, 
							kAudioUnitScope_Global, inParameterID, &paramInfo, &dataSize);
	if (result == noErr)
	{
		if (inWrapWithAutomationGesture)
			automationgesture_begin(inParameterID);

		CAAUParameter auParam(GetEditAudioUnit(), inParameterID, kAudioUnitScope_Global, 0);
		result = AUParameterSet(NULL, NULL, &auParam, paramInfo.defaultValue, 0);

		if (inWrapWithAutomationGesture)
			automationgesture_end(inParameterID);
	}
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::getparametervaluestring(long inParameterID, char * outText)
{
	if (outText == NULL)
		return;

	DfxParameterValueStringRequest request;
	UInt32 dataSize = sizeof(request);
	request.inStringIndex = getparameter_i(inParameterID);

	if (AudioUnitGetProperty(GetEditAudioUnit(), kDfxPluginProperty_ParameterValueString, 
							kAudioUnitScope_Global, inParameterID, &request, &dataSize) 
							== noErr)
		strcpy(outText, request.valueString);
}

#if TARGET_PLUGIN_USES_MIDI
//-----------------------------------------------------------------------------
void DfxGuiEditor::setmidilearning(bool inNewLearnMode)
{
	Boolean newLearnMode_fixedSize = inNewLearnMode;
	AudioUnitSetProperty(GetEditAudioUnit(), kDfxPluginProperty_MidiLearn, 
				kAudioUnitScope_Global, (AudioUnitElement)0, &newLearnMode_fixedSize, sizeof(newLearnMode_fixedSize));
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::getmidilearning()
{
	Boolean learnMode;
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
	Boolean nud;	// irrelevant
	AudioUnitSetProperty(GetEditAudioUnit(), kDfxPluginProperty_ResetMidiLearn, 
				kAudioUnitScope_Global, (AudioUnitElement)0, &nud, sizeof(nud));
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::setmidilearner(long inParameterIndex)
{
	int32_t parameterIndex_fixedSize = inParameterIndex;
	AudioUnitSetProperty(GetEditAudioUnit(), kDfxPluginProperty_MidiLearner, 
				kAudioUnitScope_Global, (AudioUnitElement)0, &parameterIndex_fixedSize, sizeof(parameterIndex_fixedSize));
}

//-----------------------------------------------------------------------------
long DfxGuiEditor::getmidilearner()
{
	int32_t learner;
	UInt32 dataSize = sizeof(learner);
	if (AudioUnitGetProperty(GetEditAudioUnit(), kDfxPluginProperty_MidiLearner, 
							kAudioUnitScope_Global, (AudioUnitElement)0, &learner, &dataSize) 
							== noErr)
		return learner;
	else
		return DFX_PARAM_INVALID_ID;
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::ismidilearner(long inParameterIndex)
{
	return (getmidilearner() == inParameterIndex);
}

#endif
// TARGET_PLUGIN_USES_MIDI

//-----------------------------------------------------------------------------
long DfxGuiEditor::initClipboard()
{
#if TARGET_OS_MAC
	// already initialized (allow for lazy initialization)
	if (clipboardRef != NULL)
		return noErr;
	// XXX only available in Mac OS X 10.3 or higher
	if (PasteboardCreate == NULL)
		return unsupportedOSErr;
	OSStatus status = PasteboardCreate(kPasteboardClipboard, &clipboardRef);
	if (status != noErr)
		clipboardRef = NULL;
	if ( (clipboardRef == NULL) && (status == noErr) )
		status = coreFoundationUnknownErr;
	return status;
#endif
}

//-----------------------------------------------------------------------------
long DfxGuiEditor::copySettings()
{
	long status = initClipboard();
	if (status != noErr)
		return status;

#if TARGET_OS_MAC
	status = PasteboardClear(clipboardRef);
	if (status != noErr)
		return status;	// XXX or just keep going anyway?

	PasteboardSyncFlags syncFlags = PasteboardSynchronize(clipboardRef);
	if (syncFlags & kPasteboardModified)
		return badPasteboardSyncErr;	// XXX this is a good idea?
	if ( !(syncFlags & kPasteboardClientIsOwner) )
		return notPasteboardOwnerErr;

#ifdef TARGET_API_AUDIOUNIT
	CFPropertyListRef auSettingsPropertyList = NULL;
	UInt32 dataSize = sizeof(auSettingsPropertyList);
	status = AudioUnitGetProperty(GetEditAudioUnit(), kAudioUnitProperty_ClassInfo, 
						kAudioUnitScope_Global, (AudioUnitElement)0, &auSettingsPropertyList, &dataSize);
	if (status != noErr)
		return status;
	if (auSettingsPropertyList == NULL)
		return coreFoundationUnknownErr;

	CFDataRef auSettingsCFData = CFPropertyListCreateXMLData(kCFAllocatorDefault, auSettingsPropertyList);
	if (auSettingsCFData == NULL)
		return coreFoundationUnknownErr;
	status = PasteboardPutItemFlavor(clipboardRef, (PasteboardItemID)PLUGIN_ID, kDfxGui_AUPresetFileUTI, auSettingsCFData, kPasteboardFlavorNoFlags);
	CFRelease(auSettingsCFData);
#endif

	return status;
#endif
}

//-----------------------------------------------------------------------------
long DfxGuiEditor::pasteSettings(bool * inQueryPastabilityOnly)
{
	if (inQueryPastabilityOnly != NULL)
		*inQueryPastabilityOnly = false;

	long status = initClipboard();
	if (status != noErr)
		return status;

#if TARGET_OS_MAC
	PasteboardSynchronize(clipboardRef);

	bool pastableItemFound = false;
	ItemCount itemCount = 0;
	status = PasteboardGetItemCount(clipboardRef, &itemCount);
	if (status != noErr)
		return status;
	for (UInt32 itemIndex=1; itemIndex <= itemCount; itemIndex++)
	{
		PasteboardItemID itemID = NULL;
		status = PasteboardGetItemIdentifier(clipboardRef, itemIndex, &itemID);
		if (status != noErr)
			continue;
		if ((OSType)itemID != PLUGIN_ID)
			continue;
		CFArrayRef flavorTypesArray = NULL;
		status = PasteboardCopyItemFlavors(clipboardRef, itemID, &flavorTypesArray);
		if ( (status != noErr) || (flavorTypesArray == NULL) )
			continue;
		CFIndex flavorCount = CFArrayGetCount(flavorTypesArray);
		for (CFIndex flavorIndex=0; flavorIndex < flavorCount; flavorIndex++)
		{
			CFStringRef flavorType = (CFStringRef) CFArrayGetValueAtIndex(flavorTypesArray, flavorIndex);
			if (flavorType == NULL)
				continue;
			if ( UTTypeConformsTo(flavorType, kDfxGui_AUPresetFileUTI) )
			{
				if (inQueryPastabilityOnly != NULL)
				{
					*inQueryPastabilityOnly = pastableItemFound = true;
				}
				else
				{
					CFDataRef flavorData = NULL;
					status = PasteboardCopyItemFlavorData(clipboardRef, itemID, flavorType, &flavorData);
					if ( (status == noErr) && (flavorData != NULL) )
					{
						CFPropertyListRef auSettingsPropertyList = CFPropertyListCreateFromXMLData(kCFAllocatorDefault, flavorData, kCFPropertyListImmutable, NULL);
						if (auSettingsPropertyList != NULL)
						{
							status = AudioUnitSetProperty(GetEditAudioUnit(), kAudioUnitProperty_ClassInfo, 
										kAudioUnitScope_Global, (AudioUnitElement)0, &auSettingsPropertyList, sizeof(auSettingsPropertyList));
							CFRelease(auSettingsPropertyList);
							if (status == noErr)
							{
								pastableItemFound = true;
								AUParameterChange_TellListeners(GetEditAudioUnit(), kAUParameterListener_AnyParameter);
							}
						}
						CFRelease(flavorData);
					}
				}
			}
			if (pastableItemFound)
				break;
		}
		CFRelease(flavorTypesArray);
		if (pastableItemFound)
			break;
	}

	return status;
#endif
}



#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
DGKeyModifiers DFX_GetDGKeyModifiersForEvent(EventRef inEvent)
{
	if (inEvent == NULL)
		return 0;

	UInt32 macKeyModifiers = 0;
	OSStatus status = GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32, NULL, sizeof(macKeyModifiers), NULL, &macKeyModifiers);
	if (status != noErr)
		macKeyModifiers = GetCurrentEventKeyModifiers();

	DGKeyModifiers dgKeyModifiers = 0;
	if (macKeyModifiers & cmdKey)
		dgKeyModifiers |= kDGKeyModifier_accel;
	if (macKeyModifiers & optionKey)
		dgKeyModifiers |= kDGKeyModifier_alt;
	if (macKeyModifiers & shiftKey)
		dgKeyModifiers |= kDGKeyModifier_shift;
	if (macKeyModifiers & controlKey)
		dgKeyModifiers |= kDGKeyModifier_extra;

	return dgKeyModifiers;
}
#endif

#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
static pascal OSStatus DFXGUI_WindowEventHandler(EventHandlerCallRef myHandler, EventRef inEvent, void * inUserData)
{
	bool eventWasHandled = false;
	DfxGuiEditor * ourOwnerEditor = (DfxGuiEditor*) inUserData;
	if (ourOwnerEditor == NULL)
		return eventNotHandledErr;

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

#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
bool DfxGuiEditor::HandleMouseEvent(EventRef inEvent)
{
	UInt32 inEventKind = GetEventKind(inEvent);
	OSStatus status;

	DGKeyModifiers keyModifiers = DFX_GetDGKeyModifiersForEvent(inEvent);



	if (inEventKind == kEventMouseWheelMoved)
	{
		SInt32 mouseWheelDelta = 0;
		status = GetEventParameter(inEvent, kEventParamMouseWheelDelta, typeSInt32, NULL, sizeof(mouseWheelDelta), NULL, &mouseWheelDelta);
		if (status != noErr)
			return false;

		EventMouseWheelAxis macMouseWheelAxis = 0;
		DGMouseWheelAxis dgMouseWheelAxis = kDGMouseWheelAxis_vertical;
		status = GetEventParameter(inEvent, kEventParamMouseWheelAxis, typeMouseWheelAxis, NULL, sizeof(macMouseWheelAxis), NULL, &macMouseWheelAxis);
		if (status == noErr)
		{
			switch (macMouseWheelAxis)
			{
				case kEventMouseWheelAxisX:
					dgMouseWheelAxis = kDGMouseWheelAxis_horizontal;
					break;
				case kEventMouseWheelAxisY:
				default:
					dgMouseWheelAxis = kDGMouseWheelAxis_vertical;
					break;
			}
		}

		bool mouseWheelWasHandled = false;
		DGMousedOverControlsList * currentItem = mousedOverControlsList;
		while (currentItem != NULL)
		{
			bool tempHandled = currentItem->control->do_mouseWheel(mouseWheelDelta, dgMouseWheelAxis, keyModifiers);
			if (tempHandled)
			{
				mouseWheelWasHandled = true;
				break;	// XXX do I want to only allow the first control to use it, or should I keep iterating?
			}
			currentItem = currentItem->next;
		}
		return mouseWheelWasHandled;
	}


	if ( (inEventKind == kEventMouseEntered) || (inEventKind == kEventMouseExited) )
	{
		MouseTrackingRef trackingRegion = NULL;
		status = GetEventParameter(inEvent, kEventParamMouseTrackingRef, typeMouseTrackingRef, NULL, sizeof(trackingRegion), NULL, &trackingRegion);
		if ( (status == noErr) && (trackingRegion != NULL) )
		{
			MouseTrackingRegionID trackingRegionID;
			status = GetMouseTrackingRegionID(trackingRegion, &trackingRegionID);	// XXX deprecated in Mac OS X 10.4
			if ( (status == noErr) && (trackingRegionID.signature == DESTROYFX_ID) )
			{
				DGControl * ourMousedOverControl = NULL;
				status = GetMouseTrackingRegionRefCon(trackingRegion, (void**)(&ourMousedOverControl));	// XXX deprecated in Mac OS X 10.4
				if ( (status == noErr) && (ourMousedOverControl != NULL) )
				{
					if (inEventKind == kEventMouseEntered)
						addMousedOverControl(ourMousedOverControl);
					else if (inEventKind == kEventMouseExited)
						removeMousedOverControl(ourMousedOverControl);
					return true;
				}
			}
		}
		return false;
	}



	HIPoint mouseLocation;
	status = GetEventParameter(inEvent, kEventParamMouseLocation, typeHIPoint, NULL, sizeof(HIPoint), NULL, &mouseLocation);
	if (status != noErr)
		return false;


// follow the mouse when dragging (adjusting) a GUI control
	DGControl * ourControl = currentControl_clicked;
	if (ourControl == NULL)
		return false;

// orient the mouse coordinates as though the control were at 0, 0 (for convenience)
	WindowRef window = NULL;
	status = GetEventParameter(inEvent, kEventParamWindowRef, typeWindowRef, NULL, sizeof(window), NULL, &window);
	if ( (status != noErr) || (window == NULL) )
		window = GetControlOwner( ourControl->getCarbonControl() );
	// XXX should we bail if this fails?
	if (window != NULL)
	{
		// the position of the control relative to the top left corner of the window content area
		Rect controlBounds = ourControl->getMacRect();
		// the content area of the window (i.e. not the title bar or any borders)
		Rect windowBounds;
		GetWindowBounds(window, kWindowGlobalPortRgn, &windowBounds);
		mouseLocation.x -= (float) (controlBounds.left + windowBounds.left);
		mouseLocation.y -= (float) (controlBounds.top + windowBounds.top);
	}


	if (inEventKind == kEventMouseDragged)
	{
		UInt32 mouseButtons = 1;	// bit 0 is mouse button 1, bit 1 is button 2, etc.
		status = GetEventParameter(inEvent, kEventParamMouseChord, typeUInt32, NULL, sizeof(mouseButtons), NULL, &mouseButtons);
		if (status != noErr)
			mouseButtons = GetCurrentEventButtonState();

		ourControl->do_mouseTrack(mouseLocation.x, mouseLocation.y, mouseButtons, keyModifiers);

		return false;	// let it fall through in case the host needs the event
	}

	if (inEventKind == kEventMouseUp)
	{
		ourControl->do_mouseUp(mouseLocation.x, mouseLocation.y, keyModifiers);

		currentControl_clicked = NULL;

		return false;	// let it fall through in case the host needs the event
	}

	return false;
}
#endif
// TARGET_OS_MAC

#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
bool DfxGuiEditor::HandleKeyboardEvent(EventRef inEvent)
{
	UInt32 inEventKind = GetEventKind(inEvent);
	OSStatus status;

	if ( (inEventKind == kEventRawKeyDown) || (inEventKind == kEventRawKeyRepeat) )
	{
		UInt32 keyCode = 0;
		status = GetEventParameter(inEvent, kEventParamKeyCode, typeUInt32, NULL, sizeof(keyCode), NULL, &keyCode);
		unsigned char charCode = 0;
		status = GetEventParameter(inEvent, kEventParamKeyMacCharCodes, typeChar, NULL, sizeof(charCode), NULL, &charCode);
		UInt32 modifiers = 0;
		status = GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32, NULL, sizeof(modifiers), NULL, &modifiers);
		if (status != noErr)
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
// TARGET_OS_MAC

#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
bool DfxGuiEditor::HandleCommandEvent(EventRef inEvent)
{
	UInt32 inEventKind = GetEventKind(inEvent);

	if (inEventKind == kEventCommandProcess)
	{
		HICommand hiCommand;
		OSStatus status = GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(hiCommand), NULL, &hiCommand);
		if (status != noErr)
			status = GetEventParameter(inEvent, kEventParamHICommand, typeHICommand, NULL, sizeof(hiCommand), NULL, &hiCommand);
		if (status != noErr)
			return false;

		if (hiCommand.commandID == kHICommandAppHelp)
		{
//fprintf(stderr, "command ID = kHICommandAppHelp\n");
			if (launch_documentation() == noErr)
				return true;
		}

		return false;
	}

	return false;
}
#endif
// TARGET_OS_MAC


#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
static pascal OSStatus DFXGUI_ControlEventHandler(EventHandlerCallRef myHandler, EventRef inEvent, void * inUserData)
{
	bool eventWasHandled = false;
	DfxGuiEditor * ourOwnerEditor = (DfxGuiEditor*)inUserData;
	if (ourOwnerEditor == NULL)
		return eventNotHandledErr;

	switch ( GetEventClass(inEvent) )
	{
		case kEventClassControl:
			eventWasHandled = ourOwnerEditor->HandleControlEvent(inEvent);
			break;
		case kEventClassMouse:
			eventWasHandled = ourOwnerEditor->HandleMouseEvent(inEvent);
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

#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
bool DfxGuiEditor::HandleControlEvent(EventRef inEvent)
{
	UInt32 inEventKind = GetEventKind(inEvent);
	OSStatus status;

	ControlRef ourCarbonControl = NULL;
	status = GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, NULL, sizeof(ourCarbonControl), NULL, &ourCarbonControl);
	DGControl * ourDGControl = NULL;
	if ( (status == noErr) && (ourCarbonControl != NULL) )
		ourDGControl = getDGControlByCarbonControlRef(ourCarbonControl);

/*
	// the Carbon control reference has not been added yet, so our DGControl pointer is NULL, because we can't look it up by ControlRef yet
	if (inEventKind == kEventControlInitialize)
	{
		UInt32 controlFeatures = kControlHandlesTracking | kControlSupportsDataAccess | kControlSupportsGetRegion;
		if (ourCarbonControl != NULL)
			status = GetControlFeatures(ourCarbonControl, &controlFeatures);
		status = SetEventParameter(inEvent, kEventParamControlFeatures, typeUInt32, sizeof(controlFeatures), &controlFeatures);
		return true;
	}
*/

	if ( (inEventKind == kEventControlTrackingAreaEntered) || (inEventKind == kEventControlTrackingAreaExited) )
	{
		HIViewTrackingAreaRef trackingArea = NULL;
		status = GetEventParameter(inEvent, kEventParamHIViewTrackingArea, typeHIViewTrackingAreaRef, NULL, sizeof(trackingArea), NULL, &trackingArea);
		if ( (status == noErr) && (trackingArea != NULL) && (HIViewGetTrackingAreaID != NULL) )
		{
			HIViewTrackingAreaID trackingAreaID = 0;
			status = HIViewGetTrackingAreaID(trackingArea, &trackingAreaID);
			if ( (status == noErr) && (trackingAreaID != 0) )
			{
				DGControl * ourMousedOverControl = (DGControl*)trackingAreaID;
				if (inEventKind == kEventControlTrackingAreaEntered)
					addMousedOverControl(ourMousedOverControl);
				else if (inEventKind == kEventControlTrackingAreaExited)
					removeMousedOverControl(ourMousedOverControl);
				return true;
			}
		}
		return false;
	}


	if (ourDGControl != NULL)
	{
		switch (inEventKind)
		{
			case kEventControlDraw:
//fprintf(stderr, "kEventControlDraw\n");
				{
					CGrafPtr windowPort = NULL;
					DGGraphicsContext * context = DGInitControlDrawingContext(inEvent, windowPort);
					if (context == NULL)
						return false;

//					context->setAntialias(false);	// XXX disable anti-aliased drawing for image rendering
					ourDGControl->do_draw(context);

					DGCleanupControlDrawingContext(context, windowPort);
				}
				return true;

			case kEventControlHitTest:
				{
//fprintf(stderr, "kEventControlHitTest\n");
					// get mouse location
					Point mouseLocation;
					status = GetEventParameter(inEvent, kEventParamMouseLocation, typeQDPoint, NULL, sizeof(mouseLocation), NULL, &mouseLocation);
					// get control bounds rect
					Rect controlBounds;
					GetControlBounds(ourCarbonControl, &controlBounds);
					ControlPartCode hitPart = kControlNoPart;
					// see if the mouse is inside the control bounds (it probably is, wouldn't you say?)
//					if ( PtInRgn(mouseLocation, controlBounds) )
					{
						hitPart = kControlIndicatorPart;	// scroll handle
						// also there is kControlButtonPart, kControlCheckBoxPart, kControlPicturePart
//						setCurrentControl_mouseover(ourDGControl);
					}
					status = SetEventParameter(inEvent, kEventParamControlPart, typeControlPartCode, sizeof(hitPart), &hitPart);
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
					status = SetEventParameter(inEvent, kEventParamControlPart, typeControlPartCode, sizeof(whatPart), &whatPart);
				}
//			case kEventControlClick:
//if (inEventKind == kEventControlClick) fprintf(stderr, "kEventControlClick\n");
				{
//					setCurrentControl_mouseover(ourDGControl);

					UInt32 mouseButtons = GetCurrentEventButtonState();	// bit 0 is mouse button 1, bit 1 is button 2, etc.
//					UInt32 mouseButtons = 1;	// bit 0 is mouse button 1, bit 1 is button 2, etc.
					// XXX kEventParamMouseChord does not exist for control class events, only mouse class
					// XXX hey, that's not what the headers say, they say that kEventControlClick should have that parameter
					// XXX and so should ContextualMenuClick in Mac OS X 10.3 and above
//					status = GetEventParameter(inEvent, kEventParamMouseChord, typeUInt32, NULL, sizeof(mouseButtons), NULL, &mouseButtons);

					HIPoint mouseLocation;
					status = GetEventParameter(inEvent, kEventParamMouseLocation, typeHIPoint, NULL, sizeof(mouseLocation), NULL, &mouseLocation);
//fprintf(stderr, "mousef.x = %.0f, mousef.y = %.0f\n", mouseLocation.x, mouseLocation.y);
					// XXX only kEventControlClick gives global mouse coordinates for kEventParamMouseLocation?
					// XXX says Eric Schlegel, "You should probably always get the direct object from the event, 
					// get the mouse location, and use HIPointConvert or HIViewConvertPoint to convert 
					// the mouse location from the direct object view to your view (or to global coordinates)."
					// e.g. HIPointConvert(&hiPt, kHICoordSpaceView, view, kHICoordSpace72DPIGlobal, NULL);
					if ( (inEventKind == kEventControlContextualMenuClick) || (inEventKind == kEventControlTrack) )
					{
						Point mouseLocation_i;
						GetGlobalMouse(&mouseLocation_i);
						mouseLocation.x = (float) mouseLocation_i.h;
						mouseLocation.y = (float) mouseLocation_i.v;
//fprintf(stderr, "mouse.x = %d, mouse.y = %d\n\n", mouseLocation_i.h, mouseLocation_i.v);
					}

					// orient the mouse coordinates as though the control were at 0, 0 (for convenience)
					// the position of the control relative to the top left corner of the window content area
					Rect controlBounds = ourDGControl->getMacRect();
					// the content area of the window (i.e. not the title bar or any borders)
					Rect windowBounds = {0};
					WindowRef window = GetControlOwner(ourCarbonControl);
					if (window != NULL)
						GetWindowBounds(window, kWindowGlobalPortRgn, &windowBounds);
					mouseLocation.x -= (float) (controlBounds.left + windowBounds.left);
					mouseLocation.y -= (float) (controlBounds.top + windowBounds.top);

					// check if this is a double-click
					bool isDoubleClick = false;
					// only ControlClick gets the ClickCount event parameter
					// XXX no, ContextualMenuClick does, too, but only in Mac OS X 10.3 or higher
					if (inEventKind == kEventControlClick)
					{
						UInt32 clickCount = 1;
						status = GetEventParameter(inEvent, kEventParamClickCount, typeUInt32, NULL, sizeof(clickCount), NULL, &clickCount);
						if ( (status == noErr) && (clickCount == 2) )
							isDoubleClick = true;
					}

					DGKeyModifiers keyModifiers = DFX_GetDGKeyModifiersForEvent(inEvent);

					ourDGControl->do_mouseDown(mouseLocation.x, mouseLocation.y, mouseButtons, keyModifiers, isDoubleClick);
					currentControl_clicked = ourDGControl;
				}
				return true;

			case kEventControlContextualMenuClick:
//fprintf(stderr, "kEventControlContextualMenuClick\n");
				{
					currentControl_clicked = ourDGControl;
					bool contextualMenuResult = ourDGControl->do_contextualMenuClick();
					// XXX now it's done (?)
					currentControl_clicked = NULL;
					return contextualMenuResult;
				}

			case kEventControlValueFieldChanged:
				// XXX it seems that I need to manually invalidate the control to get it to 
				// redraw in response to a value change in compositing mode ?
				if ( IsCompositWindow() )
					ourDGControl->redraw();
				return false;

			default:
				return false;
		}
	}

	return false;
}
#endif
// TARGET_OS_MAC

#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
OSStatus DFX_OpenWindowFromNib(CFStringRef inWindowName, WindowRef * outWindow)
{
	if ( (inWindowName == NULL) || (outWindow == NULL) )
		return paramErr;

	// open the window from our nib
	CFBundleRef pluginBundle = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));
	if (pluginBundle == NULL)
		return coreFoundationUnknownErr;

	IBNibRef nibRef = NULL;
	OSStatus status = CreateNibReferenceWithCFBundle(pluginBundle, kDfxGui_NibName, &nibRef);
	if ( (nibRef == NULL) && (status == noErr) )	// unlikely, but just in case
		status = kIBCarbonRuntimeCantFindNibFile;
	if (status != noErr)
		return status;

	*outWindow = NULL;
	status = CreateWindowFromNib(nibRef, inWindowName, outWindow);
	DisposeNibReference(nibRef);
	if ( (*outWindow == NULL) && (status == noErr) )	// unlikely, but just in case
		status = kIBCarbonRuntimeCantFindObject;
	if (status != noErr)
		return status;

	return noErr;
}
#endif

#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
// this gets an embedded ControlRef from a window given a control's ID value
ControlRef DFX_GetControlWithID(WindowRef inWindow, SInt32 inID)
{
	if (inWindow == NULL)
		return NULL;

	ControlID controlID = {0};
	controlID.signature = kDfxGui_ControlSignature;
	controlID.id = inID;
	ControlRef resultControl = NULL;
	GetControlByID(inWindow, &controlID, &resultControl);
	return resultControl;
}
#endif

#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
static pascal OSStatus DFXGUI_TextEntryEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void * inUserData)
{
	bool eventWasHandled = false;
	DfxGuiEditor * ourOwnerEditor = (DfxGuiEditor*)inUserData;
	if (ourOwnerEditor == NULL)
		return eventNotHandledErr;

	UInt32 eventClass = GetEventClass(inEvent);
	UInt32 eventKind = GetEventKind(inEvent);

	if ( (eventClass == kEventClassCommand) && (eventKind == kEventCommandProcess) )
	{
		HICommand command;
		OSStatus status = GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(command), NULL, &command);
		if (status == noErr)
			eventWasHandled = ourOwnerEditor->handleTextEntryCommand(command.commandID);
	}

	if (eventWasHandled)
		return noErr;
	else
		return eventNotHandledErr;
}

//-----------------------------------------------------------------------------
CFStringRef DfxGuiEditor::openTextEntryWindow(CFStringRef inInitialText)
{
	textEntryWindow = NULL;
	textEntryControl = NULL;
	textEntryResultString = NULL;

	OSStatus status = DFX_OpenWindowFromNib(CFSTR("text entry"), &textEntryWindow);
	if (status != noErr)
		return NULL;

	// set the title of the window according to the particular plugin's name
	status = SetWindowTitleWithCFString( textEntryWindow, CFSTR(PLUGIN_NAME_STRING" Text Entry") );

	// initialize the controls according to the current transparency value
	textEntryControl = DFX_GetControlWithID(textEntryWindow, kDfxGui_TextEntryControlID);
	if (textEntryControl != NULL)
	{
		if (inInitialText != NULL)
			SetControlData(textEntryControl, kControlNoPart, kControlEditTextCFStringTag, sizeof(inInitialText), &inInitialText);
		SetKeyboardFocus(textEntryWindow, textEntryControl, kControlFocusNextPart);
	}

	// set up the text entry window event handler
	if (gTextEntryEventHandlerUPP == NULL)
		gTextEntryEventHandlerUPP = NewEventHandlerUPP(DFXGUI_TextEntryEventHandler);
	EventTypeSpec windowEvents[] = { { kEventClassCommand, kEventCommandProcess } };
	EventHandlerRef windowEventHandler = NULL;
	status = InstallWindowEventHandler(textEntryWindow, gTextEntryEventHandlerUPP, GetEventTypeCount(windowEvents), windowEvents, this, &windowEventHandler);
	if (status != noErr)
	{
		DisposeWindow(textEntryWindow);
		textEntryWindow = NULL;
		return NULL;
	}

	SetWindowActivationScope(textEntryWindow, kWindowActivationScopeAll);	// this allows keyboard focus for floating windows
	ShowWindow(textEntryWindow);

	// run the dialog window modally
	// this will return when the dialog's event handler breaks the modal run loop
	RunAppModalLoopForWindow(textEntryWindow);

	// clean up all of the dialog stuff now that's it's finished
	RemoveEventHandler(windowEventHandler);
	HideWindow(textEntryWindow);
	DisposeWindow(textEntryWindow);
	textEntryWindow = NULL;
	textEntryControl = NULL;

	return textEntryResultString;
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::handleTextEntryCommand(UInt32 inCommandID)
{
	bool eventWasHandled = false;

	switch (inCommandID)
	{
		case kHICommandOK:
			if (textEntryControl != NULL)
			{
				OSErr error = GetControlData(textEntryControl, kControlNoPart, kControlEditTextCFStringTag, sizeof(textEntryResultString), &textEntryResultString, NULL);
				if (error != noErr)
					textEntryResultString = NULL;
			}
		case kHICommandCancel:
			QuitAppModalLoopForWindow(textEntryWindow);
			eventWasHandled = true;
			break;

		default:
			break;
	}

	return eventWasHandled;
}
#endif
// TARGET_OS_MAC

#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
static pascal OSStatus DFXGUI_WindowTransparencyEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void * inUserData)
{
	bool eventWasHandled = false;
	DfxGuiEditor * ourOwnerEditor = (DfxGuiEditor*)inUserData;
	if (ourOwnerEditor == NULL)
		return eventNotHandledErr;

	UInt32 eventClass = GetEventClass(inEvent);
	UInt32 eventKind = GetEventKind(inEvent);

	if ( (eventClass == kEventClassWindow) && (eventKind == kEventWindowClose) )
	{
		ourOwnerEditor->heedWindowTransparencyWindowClose();
		eventWasHandled = false;	// let the standard window event handler dispose of the window and such
	}

	else if ( (eventClass == kEventClassControl) && (eventKind == kEventControlValueFieldChanged) )
	{
		ControlRef control = NULL;
		OSStatus status = GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, NULL, sizeof(control), NULL, &control);
		if ( (status == noErr) && (control != NULL) )
		{
			ControlID controlID;
			status = GetControlID(control, &controlID);
			if (status == noErr)
			{
				if ( (controlID.signature == kDfxGui_ControlSignature) && (controlID.id == kDfxGui_TransparencySliderControlID) )
				{
					SInt32 min = GetControl32BitMinimum(control);
					SInt32 max = GetControl32BitMaximum(control);
					SInt32 controlValue = GetControl32BitValue(control);
					float transparencyValue = (float)(controlValue - min) / (float)(max - min);
					transparencyValue = 1.0f - transparencyValue;	// invert it to reflect transparency, not opacity
					transparencyValue = powf(transparencyValue, 0.69f);	// XXX root-scale the value distribution
					ourOwnerEditor->setWindowTransparency(transparencyValue);
					eventWasHandled = true;
				}
			}
		}
	}

	if (eventWasHandled)
		return noErr;
	else
		return eventNotHandledErr;
}

//-----------------------------------------------------------------------------
// this function doesn't actually have to do anything to get "live update" slider behavior, 
// it just needs to exist
static pascal void DFXGUI_TransparencyControlActionProc(ControlRef inControl, ControlPartCode inPartCode)
{
}

//-----------------------------------------------------------------------------
OSStatus DfxGuiEditor::openWindowTransparencyWindow()
{
	// if the window is already open, then don't open a new one, just bring the existing one to the front
	if (windowTransparencyWindow != NULL)
	{
		SelectWindow(windowTransparencyWindow);
		return noErr;
	}

	OSStatus status = DFX_OpenWindowFromNib(CFSTR("set transparency"), &windowTransparencyWindow);
	if (status != noErr)
		return status;

	// set the title of the window according to the particular plugin's name
	status = SetWindowTitleWithCFString( windowTransparencyWindow, CFSTR(PLUGIN_NAME_STRING" Window Transparency") );

	// initialize the controls according to the current transparency value
	ControlRef transparencyControl = DFX_GetControlWithID(windowTransparencyWindow, kDfxGui_TransparencySliderControlID);
	if (transparencyControl != NULL)
	{
		SInt32 min = GetControl32BitMinimum(transparencyControl);
		SInt32 max = GetControl32BitMaximum(transparencyControl);
		SInt32 initialValue = (SInt32)((1.0f - getWindowTransparency()) * (float)(max - min)) + min;
		SetControl32BitValue(transparencyControl, initialValue);

		// this is necessary for the slider to actually do the "live update" dragging behavior
		if (gTransparencyControlActionUPP == NULL)
			gTransparencyControlActionUPP = NewControlActionUPP(DFXGUI_TransparencyControlActionProc);
		SetControlAction(transparencyControl, gTransparencyControlActionUPP);
	}

	// set up the window transparency window event handler
	if (gWindowTransparencyEventHandlerUPP == NULL)
		gWindowTransparencyEventHandlerUPP = NewEventHandlerUPP(DFXGUI_WindowTransparencyEventHandler);
	EventTypeSpec windowEvents[] = { { kEventClassWindow, kEventWindowClose } };
	EventHandlerRef windowEventHandler = NULL;
	status = InstallWindowEventHandler(windowTransparencyWindow, gWindowTransparencyEventHandlerUPP, GetEventTypeCount(windowEvents), windowEvents, this, &windowEventHandler);
	if (status != noErr)
	{
		DisposeWindow(windowTransparencyWindow);
		windowTransparencyWindow = NULL;
		return status;
	}

	// set up the window transparency control event handler
	EventTypeSpec controlEvents[] = { { kEventClassControl, kEventControlValueFieldChanged } };
	EventHandlerRef controlEventHandler = NULL;
	if (transparencyControl != NULL)
	{
		status = InstallControlEventHandler(transparencyControl, gWindowTransparencyEventHandlerUPP, GetEventTypeCount(controlEvents), controlEvents, this, &controlEventHandler);
	}

	SetWindowActivationScope(windowTransparencyWindow, kWindowActivationScopeAll);	// this allows keyboard focus for floating windows
	ShowWindow(windowTransparencyWindow);
	return noErr;
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::heedWindowTransparencyWindowClose()
{
	windowTransparencyWindow = NULL;
}
#endif
// TARGET_OS_MAC
