#include "dfxgui.h"

#include "dfxplugin.h"



#if MAC
static pascal OSStatus DGControlEventHandler(EventHandlerCallRef, EventRef, void * inUserData);
static pascal OSStatus DGWindowEventHandler(EventHandlerCallRef, EventRef, void * inUserData);

static pascal void DGIdleTimerProc(EventLoopTimerRef inTimer, void * inUserData);
#endif


//-----------------------------------------------------------------------------
DfxGuiEditor::DfxGuiEditor(AudioUnitCarbonView inInstance)
:	AUCarbonViewBase(inInstance)
{
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
	long systemVersion = 0;
	if ( (Gestalt(gestaltSystemVersion, &systemVersion) == noErr) && ((systemVersion & 0xFFFF) >= 0x1023) )
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
		{ kEventClassControl, kEventControlContextualMenuClick } 
	};
	ToolboxObjectClassRef newControlClass = NULL;
	controlHandlerUPP = NewEventHandlerUPP(DGControlEventHandler);
	unsigned long instanceAddress = (unsigned long) this;
	bool noSuccessYet = true;
	while (noSuccessYet)
	{
		CFStringRef toolboxClassIDcfstring = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%s.DfxGuiControlClass%lu"), 
																		PLUGIN_BUNDLE_IDENTIFIER, instanceAddress);
		if ( RegisterToolboxObjectClass(toolboxClassIDcfstring, NULL, GetEventTypeCount(toolboxClassEvents), toolboxClassEvents, 
										controlHandlerUPP, this, &newControlClass) == noErr )
			noSuccessYet = false;
		CFRelease(toolboxClassIDcfstring);
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


// register for HitTest events on the background embedding pane so that we now when the mouse hovers over it
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
bool DfxGuiEditor::HandleEvent(EventRef inEvent)
{
	if (GetEventClass(inEvent) == kEventClassControl)
	{
		UInt32 inEventKind = GetEventKind(inEvent);
		if (inEventKind == kEventControlDraw)
		{
			ControlRef carbonControl;
			GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, NULL, sizeof(ControlRef), NULL, &carbonControl);
			if (carbonControl == mCarbonPane)
			{
				CGrafPtr windowPort = NULL;
				// if we received a graphics port parameter, use that...
				OSStatus error = GetEventParameter(inEvent, kEventParamGrafPort, typeGrafPtr, NULL, sizeof(CGrafPtr), NULL, &windowPort);
				// ... otherwise use the current graphics port
				if ( (error != noErr) || (windowPort == NULL) )
					GetPort(&windowPort);
				if (windowPort == NULL)
					return false;
				Rect portBounds;
				GetPortBounds(windowPort, &portBounds);

				// drawing
				CGContextRef context = NULL;
				error = QDBeginCGContext(windowPort, &context);
				if ( (error != noErr) || (windowPort == NULL) )
					return false;
				SyncCGContextOriginWithPort(context, windowPort);
				CGContextSaveGState(context);
#ifdef FLIP_CG_COORDINATES
				// this lets me position things with non-upside-down coordinates, 
				// but unfortunately draws all images and text upside down...
				CGContextTranslateCTM(context, 0.0f, (float)(portBounds.bottom - portBounds.top));
				CGContextScaleCTM(context, 1.0f, -1.0f);
#endif
				// XXX need a better way of determining the background size
				// XXX moreover, what's the point of clipping to the image size?  it's not like that image will draw outside its own bounds
				if (backgroundImage != NULL)
				{
					// define the clipping region
					CGRect clipRect = CGRectMake( GetXOffset(), GetYOffset(), 
											(float)(backgroundImage->getWidth()), (float)(backgroundImage->getHeight()) );
					CGContextClipToRect(context, clipRect);
				}
				CGContextSetShouldAntialias(context, false);	// XXX disable anti-aliased drawing for image rendering
				DrawBackground(context, portBounds.bottom);
				CGContextRestoreGState(context);
				CGContextSynchronize(context);
				QDEndCGContext(windowPort, &context);

				return true;
			}
		}

		// we want to catch when the mouse hovers over onto the background area
		else if ( (inEventKind == kEventControlHitTest) || (inEventKind == kEventControlClick) )
		{
			ControlRef control;
			GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, NULL, sizeof(ControlRef), NULL, &control);
			if (control == mCarbonPane)
				setCurrentControl_mouseover(NULL);	// we don't count the background
		}

		else if (inEventKind == kEventControlApplyBackground)
		{
//			fprintf(stderr, "mCarbonPane HandleEvent(kEventControlApplyBackground)\n");
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
	WindowAttributes attributes = 0;
	OSStatus error = GetWindowAttributes(GetCarbonWindow(), &attributes);
	if (error == noErr)
		return (attributes & kWindowCompositingAttribute) ? true : false;
	else
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
			OSStatus status = LSOpenCFURLRef(fileURL, NULL);
			CFRelease(fileURL);
			return status;
		}
	}

	return fnfErr;	// file not found error
#endif

}


//-----------------------------------------------------------------------------
void DfxGuiEditor::automationgesture_begin(long inParameterID)
{
#ifdef TARGET_API_AUDIOUNIT
	AUVParameter auvp(GetEditAudioUnit(), (AudioUnitParameterID)inParameterID, kAudioUnitScope_Global, (AudioUnitElement)0);
	TellListener(auvp, kAudioUnitCarbonViewEvent_MouseDownInControl, NULL);
#endif
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::automationgesture_end(long inParameterID)
{
#ifdef TARGET_API_AUDIOUNIT
	AUVParameter auvp(GetEditAudioUnit(), (AudioUnitParameterID)inParameterID, kAudioUnitScope_Global, (AudioUnitElement)0);
	TellListener(auvp, kAudioUnitCarbonViewEvent_MouseUpInControl, NULL);
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

	HIPoint mouseLocation;
	GetEventParameter(inEvent, kEventParamMouseLocation, typeHIPoint, NULL, sizeof(HIPoint), NULL, &mouseLocation);
	Point mouseLocation_i;
	mouseLocation_i.h = (short) mouseLocation.x;
	mouseLocation_i.v = (short) mouseLocation.y;


// follow the mouse around, see if it falls over any of our hot spots
	if (inEventKind == kEventMouseMoved)
return false;
/*
	{
		// remember current port
		CGrafPtr oldport;
		GetPort(&oldport);
		// switch to our window's port
		WindowRef window;
		GetEventParameter(inEvent, kEventParamWindowRef, typeWindowRef, NULL, sizeof(WindowRef), NULL, &window);
		SetPort( GetWindowPort(window) );

		// figure out which control is currently under the mouse, if any
		GlobalToLocal(&mouseLocation_i);
		ControlRef underCarbonControl = FindControlUnderMouse(mouseLocation_i, window, NULL);
		DGControl * underDGControl = NULL;
		if (underCarbonControl != NULL)
			underDGControl = getDGControlByCarbonControlRef(underCarbonControl);
		setCurrentControl_mouseover(underDGControl);

		// restore the original port
		SetPort(oldport);

		return true;
	}
*/


// follow the mouse when dragging (adjusting) a GUI control
	DGControl * ourControl = currentControl_clicked;
	if (ourControl == NULL)
		return false;

	UInt32 modifiers;
	GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32, NULL, sizeof(UInt32), NULL, &modifiers);
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
	WindowRef window;
	GetEventParameter(inEvent, kEventParamWindowRef, typeWindowRef, NULL, sizeof(WindowRef), NULL, &window);
	// the content area of the window (i.e. not the title bar or any borders)
	Rect windowBounds;
	GetWindowBounds(window, kWindowGlobalPortRgn, &windowBounds);
	// the position of the control relative to the top left corner of the window content area
	Rect controlBounds;
	GetControlBounds(ourControl->getCarbonControl(), &controlBounds);
	mouseLocation.x -= (float) (windowBounds.left + controlBounds.left);
	mouseLocation.y -= (float) (windowBounds.top + controlBounds.top);


	if (inEventKind == kEventMouseDragged)
	{
		UInt32 mouseButtons = 1;	// bit 0 is mouse button 1, bit 1 is button 2, etc.
		OSStatus paramstatus = GetEventParameter(inEvent, kEventParamMouseChord, typeUInt32, NULL, sizeof(UInt32), NULL, &mouseButtons);
		if (paramstatus != noErr)
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
			TellListener(ourControl->getAUVP(), kAudioUnitCarbonViewEvent_MouseUpInControl, NULL);
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
		UInt32 keyCode;
		GetEventParameter(inEvent, kEventParamKeyCode, typeUInt32, NULL, sizeof(UInt32), NULL, &keyCode);
		unsigned char charCode;
		GetEventParameter(inEvent, kEventParamKeyMacCharCodes, typeChar, NULL, sizeof(char), NULL, &charCode);
		UInt32 modifiers;
		GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32, NULL, sizeof(UInt32), NULL, &modifiers);
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
		OSStatus status = GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &hiCommand);
		if (status != noErr)
			status = GetEventParameter(inEvent, kEventParamHICommand, typeHICommand, NULL, sizeof(HICommand), NULL, &hiCommand);
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
//CGContextRef econtext = NULL;
//OSStatus cgstat = GetEventParameter(inEvent, kEventParamCGContextRef, typeCGContextRef, NULL, sizeof(CGContextRef), NULL, &econtext);
//fprintf(stderr, "GetEventParameter(kEventParamCGContextRef) = %ld\n", cgstat);
					CGrafPtr windowPort = NULL;
					// if we received a graphics port parameter, use that...
					OSStatus error = GetEventParameter(inEvent, kEventParamGrafPort, typeGrafPtr, NULL, sizeof(CGrafPtr), NULL, &windowPort);
					// ... otherwise use the current graphics port
					if ( (error != noErr) || (windowPort == NULL) )
						GetPort(&windowPort);
					if (windowPort == NULL)
						return false;
					Rect portBounds;
					GetPortBounds(windowPort, &portBounds);

					// set up the CG context
					CGContextRef context = NULL;
					error = QDBeginCGContext(windowPort, &context);
					if ( (error != noErr) || (context == NULL) )
						return false;
					SyncCGContextOriginWithPort(context, windowPort);
					CGContextSaveGState(context);
#ifdef FLIP_CG_COORDINATES
					// this lets me position things with non-upside-down coordinates, 
					// but unfortunately draws all images and text upside down...
					CGContextTranslateCTM(context, 0.0f, (float)(portBounds.bottom - portBounds.top));
					CGContextScaleCTM(context, 1.0f, -1.0f);
#endif
					// define the clipping region
					CGRect clipRect = ourDGControl->getBounds()->convertToCGRect(portBounds.bottom);
					CGContextClipToRect(context, clipRect);
					// XXX disable anti-aliased drawing for image rendering
					CGContextSetShouldAntialias(context, false);
					ourDGControl->do_draw(context, portBounds.bottom);
					CGContextRestoreGState(context);
					CGContextSynchronize(context);
					QDEndCGContext(windowPort, &context);
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
					Point mouseLocation_i;
					mouseLocation_i.h = (short) mouseLocation.x;
					mouseLocation_i.v = (short) mouseLocation.y;
					// XXX only kEventControlClick gives global mouse coordinates for kEventParamMouseLocation?
					if ( (inEventKind == kEventControlContextualMenuClick) || (inEventKind == kEventControlTrack) )
					{
						GetGlobalMouse(&mouseLocation_i);
						mouseLocation.x = (float) mouseLocation_i.h;
						mouseLocation.y = (float) mouseLocation_i.v;
					}
//fprintf(stderr, "mouse.x = %d, mouse.y = %d\n\n", mouseLocation_i.h, mouseLocation_i.v);

					// orient the mouse coordinates as though the control were at 0, 0 (for convenience)
					// the content area of the window (i.e. not the title bar or any borders)
					Rect windowBounds;
					GetWindowBounds(GetControlOwner(ourCarbonControl), kWindowGlobalPortRgn, &windowBounds);
					// the position of the control relative to the top left corner of the window content area
					Rect controlBounds;
					GetControlBounds(ourCarbonControl, &controlBounds);
					mouseLocation.x -= (float) (windowBounds.left + controlBounds.left);
					mouseLocation.y -= (float) (windowBounds.top + controlBounds.top);

					UInt32 modifiers;
					GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32, NULL, sizeof(UInt32), NULL, &modifiers);
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
						TellListener(ourDGControl->getAUVP(), kAudioUnitCarbonViewEvent_MouseDownInControl, NULL);
//						fprintf(stderr, "DGControlEventHandler -> TellListener(MouseDown, %ld)\n", ourDGControl->getParameterID());
					}

					ourDGControl->mouseDown(mouseLocation.x, mouseLocation.y, mouseButtons, keyModifiers);
					currentControl_clicked = ourDGControl;
				}
				return true;

			default:
				return false;
		}
	}

	return false;
}
#endif
// MAC
