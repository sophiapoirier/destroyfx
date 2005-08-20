#include "dfxgui.h"

#include "dfxplugin.h"
#include "dfx-au-utilities.h"



#if TARGET_OS_MAC
static pascal OSStatus DGControlEventHandler(EventHandlerCallRef, EventRef, void * inUserData);
static pascal OSStatus DGWindowEventHandler(EventHandlerCallRef, EventRef, void * inUserData);

static pascal void DGIdleTimerProc(EventLoopTimerRef inTimer, void * inUserData);
#endif

// for now this is a good idea, until it's totally obsoleted
#define AU_DO_OLD_STYLE_PARAMETER_CHANGE_GESTURES

static EventHandlerUPP gControlHandlerUPP = NULL;
static EventHandlerUPP gWindowEventHandlerUPP = NULL;
static EventLoopTimerUPP gIdleTimerUPP = NULL;


//-----------------------------------------------------------------------------
DfxGuiEditor::DfxGuiEditor(DGEditorListenerInstance inInstance)
:	AUCarbonViewBase(inInstance, 0.030f)	// 30 ms parameter notification update interval
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

	backgroundImage = NULL;
	backgroundColor(randFloat(), randFloat(), randFloat());

	dgControlSpec.defType = kControlDefObjectClass;
	dgControlSpec.u.classRef = NULL;
	windowEventHandlerRef = NULL;

	fontsATSContainer = kATSFontContainerRefUnspecified;
	fontsWereActivated = false;	// guilty until proven innocent

	currentControl_clicked = NULL;
	currentControl_mouseover = NULL;
	mousedOverControlsList = NULL;

	dfxplugin = NULL;
}

//-----------------------------------------------------------------------------
DfxGuiEditor::~DfxGuiEditor()
{
	#if TARGET_PLUGIN_USES_MIDI
		if (GetEditAudioUnit() != NULL)
			setmidilearning(false);
	#endif

	if (idleTimer != NULL)
		RemoveEventLoopTimer(idleTimer);
	idleTimer = NULL;

	if (windowEventHandlerRef != NULL)
		RemoveEventHandler(windowEventHandlerRef);
	windowEventHandlerRef = NULL;

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
		{ kEventClassControl, kEventControlValueFieldChanged }, 
		{ kEventClassControl, kEventControlTrackingAreaEntered }, 
		{ kEventClassControl, kEventControlTrackingAreaExited }, 
		{ kEventClassMouse, kEventMouseEntered }, 
		{ kEventClassMouse, kEventMouseExited }, 
	};
	ToolboxObjectClassRef newControlClass = NULL;
	if (gControlHandlerUPP == NULL)
		gControlHandlerUPP = NewEventHandlerUPP(DGControlEventHandler);
	unsigned long instanceAddress = (unsigned long) this;
	bool noSuccessYet = true;
	while (noSuccessYet)
	{
		CFStringRef toolboxClassIDcfstring = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%s.DfxGuiControlClass%lu"), 
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
//									{ kEventClassMouse, kEventMouseMoved }, 
									{ kEventClassMouse, kEventMouseWheelMoved }, 
									{ kEventClassKeyboard, kEventRawKeyDown }, 
									{ kEventClassKeyboard, kEventRawKeyRepeat }, 
									{ kEventClassCommand, kEventCommandProcess }, 
								};
	if (gWindowEventHandlerUPP == NULL)
		gWindowEventHandlerUPP = NewEventHandlerUPP(DGWindowEventHandler);
	InstallEventHandler(GetWindowEventTarget(GetCarbonWindow()), gWindowEventHandlerUPP, 
						GetEventTypeCount(windowEvents), windowEvents, this, &windowEventHandlerRef);


// register for HitTest events on the background embedding pane so that we know when the mouse hovers over it
	EventTypeSpec paneEvents[] = {
								{ kEventClassControl, kEventControlDraw }, 
								{ kEventClassControl, kEventControlApplyBackground },
//								{ kEventClassControl, kEventControlHitTest }, 
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
			SizeControl(GetCarbonPane(), (SInt16) (backgroundImage->getWidth()), (SInt16) (backgroundImage->getHeight()));

		if (gIdleTimerUPP == NULL)
			gIdleTimerUPP = NewEventLoopTimerUPP(DGIdleTimerProc);
		InstallEventLoopTimer(GetCurrentEventLoop(), 0.0, kEventDurationMillisecond * 50.0, gIdleTimerUPP, this, &idleTimer);

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

#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
// inEvent is expected to be an event of the class kEventClassControl.  
// outPort will be null if a CGContextRef was available from the event parameters 
// (which happens with compositing windows) or non-null if the CGContext had to be created 
// for the control's window port QuickDraw-style.  
DGGraphicsContext * InitControlDrawingContext(EventRef inEvent, CGrafPtr & outPort)
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
		Rect portBounds;
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
// inPort should be what you got from InitControlDrawingContext().  
// It is null if inContext came with the control event (compositing window behavior) or 
// non-null if inContext was created via QDBeginCGContext(), and hence needs to be terminated.
void CleanupControlDrawingContext(DGGraphicsContext * inContext, CGrafPtr inPort)
{
	if (inContext == NULL)
		return;
	if (inContext->getPlatformGraphicsContext() == NULL)
		return;

	CGContextRestoreGState( inContext->getPlatformGraphicsContext() );
	CGContextSynchronize( inContext->getPlatformGraphicsContext() );

	if (inPort != NULL)
		inContext->endQDContext(inPort);
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
				DGGraphicsContext * context = InitControlDrawingContext(inEvent, windowPort);
				if (context == NULL)
					return false;

				context->setAntialias(false);	// XXX disable anti-aliased drawing for image rendering
				DrawBackground(context);

				CleanupControlDrawingContext(context, windowPort);
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
	if (backgroundImage != NULL)
	{
		DGRect drawRect( (long)GetXOffset(), (long)GetYOffset(), backgroundImage->getWidth(), backgroundImage->getHeight() );
		backgroundImage->draw(&drawRect, inContext);
	}
	else
	{
		// XXX draw a rectangle with the background color
	}
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
	CFBundleRef pluginBundleRef = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));
	if (pluginBundleRef != NULL)
	{
		CFStringRef docsFileName = CFSTR( PLUGIN_NAME_STRING" manual.html" );
		CFURLRef docsFileURL = CFBundleCopyResourceURL(pluginBundleRef, docsFileName, NULL, NULL);
		// if the documentation file is not found in the bundle, then search in appropriate system locations
		if (docsFileURL ==  NULL)
			docsFileURL = DFX_FindDocumentationFileInDomain(docsFileName, kUserDomain);
		if (docsFileURL ==  NULL)
			docsFileURL = DFX_FindDocumentationFileInDomain(docsFileName, kLocalDomain);
		if (docsFileURL ==  NULL)
			docsFileURL = DFX_FindDocumentationFileInDomain(docsFileName, kNetworkDomain);
		if (docsFileURL != NULL)
		{
// open the manual with the default application for the file type
#if 0
			OSStatus status = LSOpenCFURLRef(docsFileURL, NULL);
			CFRelease(docsFileURL);
// open the manual with Apple's system Help Viewer
#else
			OSStatus status = coreFoundationUnknownErr;
			CFStringRef docsFileUrlString = CFURLGetString(docsFileURL);
			if (docsFileUrlString != NULL)
			{
				status = AHGotoPage(NULL, docsFileUrlString, NULL);
			}
			CFRelease(docsFileURL);
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
void DfxGuiEditor::setparameter_default(long inParameterID)
{
	AudioUnitParameterInfo paramInfo;
	memset(&paramInfo, 0, sizeof(paramInfo));
	UInt32 dataSize = sizeof(paramInfo);

	ComponentResult result = AudioUnitGetProperty(GetEditAudioUnit(), kAudioUnitProperty_ParameterInfo, 
							kAudioUnitScope_Global, inParameterID, &paramInfo, &dataSize);
	if (result == noErr)
	{
		AudioUnitParameter auParam;
		auParam.mAudioUnit = GetEditAudioUnit();
		auParam.mParameterID = inParameterID;
		auParam.mScope = kAudioUnitScope_Global;
		auParam.mElement = (AudioUnitElement)0;
		result = AUParameterSet(NULL, NULL, &auParam, paramInfo.defaultValue, 0);
	}
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



#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
DGKeyModifiers GetDGKeyModifiersForEvent(EventRef inEvent)
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
	if ( (macKeyModifiers & optionKey) || (macKeyModifiers & rightOptionKey) )
		dgKeyModifiers |= kDGKeyModifier_alt;
	if ( (macKeyModifiers & shiftKey) || (macKeyModifiers & rightShiftKey) )
		dgKeyModifiers |= kDGKeyModifier_shift;
	if ( (macKeyModifiers & controlKey) || (macKeyModifiers & rightControlKey) )
		dgKeyModifiers |= kDGKeyModifier_extra;

	return dgKeyModifiers;
}
#endif

#if TARGET_OS_MAC
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

#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
bool DfxGuiEditor::HandleMouseEvent(EventRef inEvent)
{
	UInt32 inEventKind = GetEventKind(inEvent);
	OSStatus status;

	DGKeyModifiers keyModifiers = GetDGKeyModifiersForEvent(inEvent);



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
		status = GetEventParameter(inEvent, kEventParamWindowRef, typeWindowRef, NULL, sizeof(window), NULL, &window);
		if ( (status != noErr) || (window == NULL) )
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

// orient the mouse coordinates as though the control were at 0, 0 (for convenience)
	WindowRef window = NULL;
	status = GetEventParameter(inEvent, kEventParamWindowRef, typeWindowRef, NULL, sizeof(window), NULL, &window);
	if ( (status != noErr) || (window == NULL) )
		window = GetControlOwner( ourControl->getCarbonControl() );
	// XXX should we bail if this fails?
	if (window != NULL)
	{
		// the position of the control relative to the top left corner of the window content area
		Rect controlBounds;
		GetControlBounds(ourControl->getCarbonControl(), &controlBounds);
		// the content area of the window (i.e. not the title bar or any borders)
		Rect windowBounds;
		GetWindowBounds(window, kWindowGlobalPortRgn, &windowBounds);
		if ( IsCompositWindow() )
		{
			Rect paneBounds;
			GetControlBounds(GetCarbonPane(), &paneBounds);
			controlBounds.left += paneBounds.left;
			controlBounds.top += paneBounds.top;
		}
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
//else fprintf(stderr, "command ID = %.4s\n", (char*) &(hiCommand.commandID));

		return false;
	}

	return false;
}
#endif
// TARGET_OS_MAC


#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
static pascal OSStatus DGControlEventHandler(EventHandlerCallRef myHandler, EventRef inEvent, void * inUserData)
{
	DfxGuiEditor * ourOwnerEditor = (DfxGuiEditor*)inUserData;
	bool eventWasHandled = false;

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
		UInt32 dfxControlFeatures = kControlHandlesTracking | kControlSupportsDataAccess | kControlSupportsGetRegion;
		status = SetEventParameter(inEvent, kEventParamControlFeatures, typeUInt32, sizeof(dfxControlFeatures), &dfxControlFeatures);
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
					DGGraphicsContext * context = InitControlDrawingContext(inEvent, windowPort);
					if (context == NULL)
						return false;

					context->setAntialias(false);	// XXX disable anti-aliased drawing for image rendering
					ourDGControl->do_draw(context);

					CleanupControlDrawingContext(context, windowPort);
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
			case kEventControlContextualMenuClick:
//if (inEventKind == kEventControlContextualMenuClick) fprintf(stderr, "kEventControlContextualMenuClick\n");
				{
//					setCurrentControl_mouseover(ourDGControl);

					UInt32 mouseButtons = GetCurrentEventButtonState();	// bit 0 is mouse button 1, bit 1 is button 2, etc.
//					UInt32 mouseButtons = 1;	// bit 0 is mouse button 1, bit 1 is button 2, etc.
					// XXX kEventParamMouseChord does not exist for control class events, only mouse class
					// XXX hey, that's not what the headers say, they say that kEventControlClick should have that parameter
//					status = GetEventParameter(inEvent, kEventParamMouseChord, typeUInt32, NULL, sizeof(mouseButtons), NULL, &mouseButtons);

					HIPoint mouseLocation;
					status = GetEventParameter(inEvent, kEventParamMouseLocation, typeHIPoint, NULL, sizeof(mouseLocation), NULL, &mouseLocation);
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
					// the position of the control relative to the top left corner of the window content area
					Rect controlBounds;
					GetControlBounds(ourCarbonControl, &controlBounds);
					// the content area of the window (i.e. not the title bar or any borders)
					Rect windowBounds;
					memset(&windowBounds, 0, sizeof(windowBounds));
					WindowRef window = GetControlOwner(ourCarbonControl);
					if (window != NULL)
						GetWindowBounds(window, kWindowGlobalPortRgn, &windowBounds);
					if ( IsCompositWindow() )
					{
						Rect paneBounds;
						GetControlBounds(GetCarbonPane(), &paneBounds);
						controlBounds.left += paneBounds.left;
						controlBounds.top += paneBounds.top;
					}
					mouseLocation.x -= (float) (controlBounds.left + windowBounds.left);
					mouseLocation.y -= (float) (controlBounds.top + windowBounds.top);

					// check if this is a double-click
					bool isDoubleClick = false;
					// only ControlClick gets the ClickCount event parameter
					if (inEventKind == kEventControlClick)
					{
						UInt32 clickCount = 1;
						status = GetEventParameter(inEvent, kEventParamClickCount, typeUInt32, NULL, sizeof(clickCount), NULL, &clickCount);
						if ( (status == noErr) && (clickCount == 2) )
							isDoubleClick = true;
					}

					DGKeyModifiers keyModifiers = GetDGKeyModifiersForEvent(inEvent);

					// do this to make Logic's touch automation work
					// AUCarbonViewControl::HandleEvent will catch ControlClick but not ControlContextualMenuClick
					if ( ourDGControl->isParameterAttached() && (inEventKind == kEventControlContextualMenuClick) )
					{
						automationgesture_begin( ourDGControl->getParameterID() );
//						fprintf(stderr, "DGControlEventHandler -> TellListener(MouseDown, %ld)\n", ourDGControl->getParameterID());
					}

					ourDGControl->do_mouseDown(mouseLocation.x, mouseLocation.y, mouseButtons, keyModifiers, isDoubleClick);
					currentControl_clicked = ourDGControl;
				}
				return true;

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
