#include "dfxgui.h"

#include "dfxplugin.h"



static pascal OSStatus DGControlEventHandler(EventHandlerCallRef, EventRef, void *inUserData);
static pascal OSStatus DGWindowEventHandler(EventHandlerCallRef, EventRef, void *inUserData);

static pascal void DGIdleTimerProc(EventLoopTimerRef inTimer, void *inUserData);


//-----------------------------------------------------------------------------
DfxGuiEditor::DfxGuiEditor(AudioUnitCarbonView inInstance)
:	AUCarbonViewBase(inInstance)
{
  cleanme = 0;

  
	idleTimer = NULL;
	idleTimerUPP = NULL;

	backgroundImage = NULL;
	backgroundColor(rand() % 0xFF, rand() % 0xFF, rand() % 0xFF);

	dgControlSpec.defType = kControlDefObjectClass;
	dgControlSpec.u.classRef = NULL;
	controlHandlerUPP = NULL;
	windowEventHandlerUPP = NULL;
	windowEventEventHandlerRef = NULL;
	currentControl_clicked = NULL;
	currentControl_mouseover = NULL;

	dfxplugin = NULL;

	// load any fonts from our bundle resources to be accessible locally within our component instance
	fontsWereActivated = false;	// guilty until proven innocent
	CFBundleRef myBundle = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));
	if (myBundle != NULL)
	{
		CFURLRef myResourcesURL = CFBundleCopyResourcesDirectoryURL(myBundle);
		if (myResourcesURL != NULL)
		{
			FSRef myResourceDirRef;
			if ( CFURLGetFSRef(myResourcesURL, &myResourceDirRef) )
			{
				OSStatus status = FSGetCatalogInfo(&myResourceDirRef, kFSCatInfoNone, NULL, NULL, &bundleResourceDirFSSpec, NULL);
				if (status == noErr)
					status = FMActivateFonts(&bundleResourceDirFSSpec, NULL, NULL, kFMLocalActivationContext);
				if (status == noErr)
					fontsWereActivated = true;
			}
		}
	}
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

	/* deleting a list item also calls the destroy
	   method for its Destructible data. */
	while (cleanme) {
	  CleanupList * tmp = cleanme->next;
	  delete cleanme;
	  cleanme = tmp;
	}

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
//printf("using Mac OS X version 10.2.3 or higher, so our control toolbox class will be unregistered\n");
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
//else printf("unregistering our control toolbox class FAILED, error %ld\n", unregResult);
		}
	}
//else printf("using a version of Mac OS X lower than 10.2.3, so our control toolbox class will NOT be unregistered\n");

	// XXX should probably catch errors from activation and not attempt to deactivate if activation failed
	if (fontsWereActivated)
		FMDeactivateFonts(&bundleResourceDirFSSpec, NULL, NULL, kFMDefaultOptions);
}


//-----------------------------------------------------------------------------
OSStatus DfxGuiEditor::CreateUI(Float32 inXOffset, Float32 inYOffset)
{
//	X = (UInt32) inXOffset;
//	Y = (UInt32) inYOffset;

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

	setCurrentControl_clicked(NULL);	// make sure that it ain't nuthin
	setCurrentControl_mouseover(NULL);
	EventTypeSpec controlMouseEvents[] = {
									{ kEventClassMouse, kEventMouseDragged }, 
									{ kEventClassMouse, kEventMouseUp }, 
//									{ kEventClassMouse, kEventMouseMoved }, 
									{ kEventClassKeyboard, kEventRawKeyDown }, 
									{ kEventClassKeyboard, kEventRawKeyRepeat }, 
									{ kEventClassCommand, kEventCommandProcess }, 
								};
	windowEventHandlerUPP = NewEventHandlerUPP(DGWindowEventHandler);
	InstallEventHandler(GetWindowEventTarget(GetCarbonWindow()), windowEventHandlerUPP, 
						GetEventTypeCount(controlMouseEvents), controlMouseEvents, this, &windowEventEventHandlerRef);


// register for HitTest events on the background embedding pane so that we now when the mouse hovers over it
	EventTypeSpec paneEvents[] = {
								{ kEventClassControl, kEventControlDraw }, 
								{ kEventClassControl, kEventControlApplyBackground },
								{ kEventClassControl, kEventControlHitTest }
								};
	WantEventTypes(GetControlEventTarget(mCarbonPane), GetEventTypeCount(paneEvents), paneEvents);


	idleTimerUPP = NewEventLoopTimerUPP(DGIdleTimerProc);
	InstallEventLoopTimer(GetCurrentEventLoop(), 0.0, kEventDurationMillisecond * 50.0, 
							idleTimerUPP, this, &idleTimer);


	// XXX this is not a good thing to do, hmmm, maybe I should not do it...
	UInt32 dataSize = sizeof(dfxplugin);
	if (AudioUnitGetProperty(GetEditAudioUnit(), kDfxPluginProperty_PluginPtr, 
							kAudioUnitScope_Global, (AudioUnitElement)0, &dfxplugin, &dataSize) 
							!= noErr)
		dfxplugin = NULL;


	// let the child class do its thing
	OSStatus openErr = open(inXOffset, inYOffset);
	if (openErr == noErr)
	{
		// set the size of the embedding pane
		if (backgroundImage() != NULL)
			SizeControl(mCarbonPane, (SInt16) (backgroundImage()->getWidth()), (SInt16) (backgroundImage()->getHeight()));
	}

	return openErr;
}

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
				CGContextSetShouldAntialias(context, false);	// XXX disable anti-aliased drawing for image rendering
				DrawBackground(context, portBounds.bottom);
				CGContextRestoreGState(context);
				CGContextSynchronize(context);
				QDEndCGContext(windowPort, &context);

				// restore original port, if we set a different port
				if (oldPort != NULL)
					SetPort(oldPort);
				return true;
			}
		}

		// we want to catch when the mouse hovers over onto the background area
		else if (inEventKind == kEventControlHitTest)
		{
			ControlRef control;
			GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, NULL, sizeof(ControlRef), NULL, &control);
			if (control == mCarbonPane)
				setCurrentControl_mouseover(NULL);	// we don't count the background
		}

		else if (inEventKind == kEventControlApplyBackground)
		{
//			printf("mCarbonPane HandleEvent(kEventControlApplyBackground)\n");
			return false;
		}
	}
	
	// let the parent implementation do its thing
	return AUCarbonViewBase::HandleEvent(inEvent);
}


//-----------------------------------------------------------------------------
void DfxGuiEditor::idle()
{
	if (Controls != NULL)
		Controls->idle();
}

//-----------------------------------------------------------------------------
static pascal void DGIdleTimerProc(EventLoopTimerRef inTimer, void *inUserData)
{
	if (inUserData != NULL)
		((DfxGuiEditor*)inUserData)->idle();
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::addImage(DGGraphic *inImage) {
	cleanme = new CleanupList(inImage, cleanme);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::addControl(DGControl *inControl)
{
	if (inControl == NULL)
		return;

	if (inControl->getDaddy() == NULL)
	{
	        cleanme = new CleanupList(inControl, cleanme);
		inControl->setOffset((long)GetXOffset(), (long)GetYOffset());
	}

	ControlRef newCarbonControl;
	Rect r;
	inControl->getBounds()->copyToRect(&r);
	verify_noerr( CreateCustomControl(GetCarbonWindow(), &r, &dgControlSpec, NULL, &newCarbonControl) );
	SetControl32BitMinimum(newCarbonControl, 0);
	if (inControl->isContinuousControl())
	{
		SInt32 controlrange = 0x3FFFFFFF;
		SetControl32BitMaximum(newCarbonControl, controlrange);
	}
	else
		SetControl32BitMaximum(newCarbonControl, (SInt32) (inControl->getRange()+0.01f));

	inControl->setCarbonControl(newCarbonControl);
	if (inControl->isAUVPattached())
	{
//		AddCarbonControl(AUCarbonViewControl::kTypeContinuous, inControl->getAUVP(), newCarbonControl);
		EmbedControl(newCarbonControl);
		inControl->createAUVcontrol();
//		AddControl(inControl->getAUVcontrol());
	}
	else
	{
		EmbedControl(newCarbonControl);
		SetControl32BitValue(newCarbonControl, 0);
/*
UInt32 feat = 0;
GetControlFeatures(newCarbonControl, &feat);
for (int i=0; i < 32; i++)
{
if (feat & (1 << i)) printf("control feature bit %d is active\n", i);
}
*/
	}
}

//-----------------------------------------------------------------------------
DGControl * DfxGuiEditor::getDGControlByPlatformControlRef(PlatformControlRef inControl)
{
	DGControl * current = Controls;

	while (current != NULL)
	{
		if (current->isControlRef(inControl))
		{
			current = current->getChild(inControl);
			break;
		}
		current = (DGControl*) current->getNext();
	}
	
	return current;
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::DrawBackground(CGContextRef inContext, UInt32 inPortHeight)
{
	CGImageRef backgroundCGImage = NULL;
	if (backgroundImage != NULL)
		backgroundCGImage = backgroundImage->getCGImage();
	if (backgroundCGImage != NULL)
	{
		float backWidth = (float) CGImageGetWidth(backgroundCGImage);
		float backHeight = (float) CGImageGetHeight(backgroundCGImage);
		CGRect whole = CGRectMake(GetXOffset(), inPortHeight - (GetYOffset() + backHeight), backWidth, backHeight);
		CGContextDrawImage(inContext, whole, backgroundCGImage);
	}
	else
	{
		// XXX draw a rectangle with the background color
	}
}

//-----------------------------------------------------------------------------
// XXX this function should really go somewhere else, like in that promised DFX utilities file or something like that
OSStatus launch_documentation()
{
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
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::randomizeparameters(bool writeAutomation)
{
	AudioUnitSetProperty(GetEditAudioUnit(), kDfxPluginProperty_RandomizeParameters, 
				kAudioUnitScope_Global, (AudioUnitElement)0, &writeAutomation, sizeof(bool));
}

//-----------------------------------------------------------------------------
// set the control that is currently idly under the mouse pointer, if any (NULL if none)
void DfxGuiEditor::setCurrentControl_mouseover(DGControl *inNewMousedOverControl)
{
	DGControl *oldcontrol = currentControl_mouseover;
	currentControl_mouseover = inNewMousedOverControl;
	// post notification if the mouseovered control has changed
	if (oldcontrol != inNewMousedOverControl)
		mouseovercontrolchanged();
}

//-----------------------------------------------------------------------------
float DfxGuiEditor::getparameter_f(long parameterID)
{
	Float32 value;
	if (AudioUnitGetParameter(GetEditAudioUnit(), (AudioUnitParameterID)parameterID, 
					kAudioUnitScope_Global, (AudioUnitElement)0, &value) == noErr)
		return value;
	else
		return 0.0f;
}

//-----------------------------------------------------------------------------
double DfxGuiEditor::getparameter_d(long parameterID)
{
	DfxParameterValueRequest request;
	UInt32 dataSize = sizeof(request);
	request.parameterID = parameterID;
	request.valueItem = kDfxParameterValueItem_current;
	request.valueType = kDfxParamValueType_double;

	if (AudioUnitGetProperty(GetEditAudioUnit(), kDfxPluginProperty_ParameterValue, 
							kAudioUnitScope_Global, (AudioUnitElement)0, &request, &dataSize) 
							== noErr)
		return request.value.d;
	else
		return 0.0;
}

//-----------------------------------------------------------------------------
long DfxGuiEditor::getparameter_i(long parameterID)
{
	DfxParameterValueRequest request;
	UInt32 dataSize = sizeof(request);
	request.parameterID = parameterID;
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
bool DfxGuiEditor::getparameter_b(long parameterID)
{
	DfxParameterValueRequest request;
	UInt32 dataSize = sizeof(request);
	request.parameterID = parameterID;
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
void DfxGuiEditor::getparametervaluestring(long parameterID, char *outText)
{
	DfxParameterValueStringRequest request;
	UInt32 dataSize = sizeof(request);
	request.parameterID = parameterID;
	request.stringIndex = getparameter_i(parameterID);

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



//-----------------------------------------------------------------------------
static pascal OSStatus DGWindowEventHandler(EventHandlerCallRef myHandler, EventRef inEvent, void *inUserData)
{
	UInt32 inEventClass = GetEventClass(inEvent);
	UInt32 inEventKind = GetEventKind(inEvent);
	DfxGuiEditor *ourOwnerEditor = (DfxGuiEditor*) inUserData;



	if (inEventClass == kEventClassCommand)
	{
		if (inEventKind == kEventCommandProcess)
		{
			HICommand hiCommand;
			OSStatus status = GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &hiCommand);
			if (status != noErr)
				status = GetEventParameter(inEvent, kEventParamHICommand, typeHICommand, NULL, sizeof(HICommand), NULL, &hiCommand);
			if (status != noErr)
				return eventNotHandledErr;

			if (hiCommand.commandID == kHICommandAppHelp)
			{
//printf("command ID = kHICommandAppHelp\n");
				if (launch_documentation() == noErr)
					return noErr;
			}
//else printf("command ID = %.4s\n", (char*) &(hiCommand.commandID));

			return eventNotHandledErr;
		}

		return eventKindIncorrectErr;
	}



	if (inEventClass == kEventClassKeyboard)
	{
		if ( (inEventKind == kEventRawKeyDown) || (inEventKind == kEventRawKeyRepeat) )
		{
			UInt32 keyCode;
			GetEventParameter(inEvent, kEventParamKeyCode, typeUInt32, NULL, sizeof(UInt32), NULL, &keyCode);
			unsigned char charCode;
			GetEventParameter(inEvent, kEventParamKeyMacCharCodes, typeChar, NULL, sizeof(char), NULL, &charCode);
			UInt32 modifiers;
			GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32, NULL, sizeof(UInt32), NULL, &modifiers);
//printf("keyCode = %lu,  charCode = ", keyCode);
//if ( (charCode > 0x7F) || iscntrl(charCode) ) printf("0x%.2X\n", charCode);
//else printf("%c\n", charCode);

			if ( ((keyCode == 44) && (modifiers & cmdKey)) || 
					(charCode == kHelpCharCode) )
			{
				if (launch_documentation() == noErr)
					return noErr;
//return eventNotHandledErr;
			}

			return eventNotHandledErr;
		}

		return eventKindIncorrectErr;
	}



	if (inEventClass != kEventClassMouse)
		return eventClassIncorrectErr;

//	Point mouseLocation;
//	GetEventParameter(inEvent, kEventParamMouseLocation, typeQDPoint, NULL, sizeof(Point), NULL, &mouseLocation);	// XXX being obsoleted
	HIPoint mouseLocation_f;
	GetEventParameter(inEvent, kEventParamMouseLocation, typeHIPoint, NULL, sizeof(HIPoint), NULL, &mouseLocation_f);
	Point mouseLocation;
	mouseLocation.h = (short) mouseLocation_f.x;
	mouseLocation.v = (short) mouseLocation_f.y;


// follow the mouse around, see if it falls over any of our hot spots
	if (inEventKind == kEventMouseMoved)
return eventKindIncorrectErr;
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
		GlobalToLocal(&mouseLocation);
		ControlRef underCarbonControl = FindControlUnderMouse(mouseLocation, window, NULL);
		DGControl * underDGControl = NULL;
		if (underCarbonControl != NULL)
			underDGControl = ourOwnerEditor->getDGControlByPlatformControlRef(underCarbonControl);
		ourOwnerEditor->setCurrentControl_mouseover(underDGControl);

		// restore the original port
		SetPort(oldport);

		return noErr;
	}
*/


// follow the mouse when dragging (adjusting) a GUI control
	DGControl *ourControl = ourOwnerEditor->getCurrentControl_clicked();
	if (ourControl == NULL)
		return eventNotHandledErr;

	UInt32 modifiers;
	GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32, NULL, sizeof(UInt32), NULL, &modifiers);
//	bool with_command = (modifiers & cmdKey) ? true : false;
	bool with_shift = ( (modifiers & shiftKey) || (modifiers & rightShiftKey) ) ? true : false;
	bool with_option = ( (modifiers & optionKey) || (modifiers & rightOptionKey) ) ? true : false;
//	bool with_control = ( (modifiers & controlKey) || (modifiers & rightControlKey) ) ? true : false;

// orient the mouse coordinates as though the control were at 0, 0 (for convenience)
	WindowRef window;
	GetEventParameter(inEvent, kEventParamWindowRef, typeWindowRef, NULL, sizeof(WindowRef), NULL, &window);
	// the content area of the window (i.e. not the title bar or any borders)
	Rect windowBounds;
	GetWindowBounds(window, kWindowGlobalPortRgn, &windowBounds);
	// the position of the control relative to the top left corner of the window content area
	Rect controlBounds;
	GetControlBounds(ourControl->getCarbonControl(), &controlBounds);
	mouseLocation.h -= windowBounds.left + controlBounds.left;
	mouseLocation.v -= windowBounds.top + controlBounds.top;


	if (inEventKind == kEventMouseDragged)
	{
		UInt32 buttons;	// bit 0 is mouse button 1, bit 1 is button 2, etc.
		GetEventParameter(inEvent, kEventParamMouseChord, typeUInt32, NULL, sizeof(UInt32), NULL, &buttons);
//		EventMouseButton button;	// kEventMouseButtonPrimary, kEventMouseButtonSecondary, or kEventMouseButtonTertiary
//		GetEventParameter(inEvent, kEventParamMouseButton, typeMouseButton, NULL, sizeof(EventMouseButton), NULL, &button);

		ourControl->mouseTrack(mouseLocation, with_option, with_shift);

		return noErr;
	}

	if (inEventKind == kEventMouseUp)
	{
		ourControl->mouseUp(mouseLocation, with_option, with_shift);

		ourOwnerEditor->setCurrentControl_clicked(NULL);

		// XXX do this to make Logic's touch automation work
		if ( ourControl->isAUVPattached() )//&& ourControl->isContinuousControl() )
		{
			ourOwnerEditor->TellListener(ourControl->getAUVP(), kAudioUnitCarbonViewEvent_MouseUpInControl, NULL);
//			printf("DGControlMouseHandler -> TellListener(MouseUp, %lu)\n", ourControl->getAUVP().mParameterID);
		}

		return noErr;
	}

	return eventKindIncorrectErr;
}


//-----------------------------------------------------------------------------
static pascal OSStatus DGControlEventHandler(EventHandlerCallRef myHandler, EventRef inEvent, void *inUserData)
{
	if (GetEventClass(inEvent) != kEventClassControl)
		return eventClassIncorrectErr;

	OSStatus result = eventNotHandledErr;

	UInt32 inEventKind = GetEventKind(inEvent);

	ControlRef ourCarbonControl = NULL;
	GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, NULL, sizeof(ControlRef), NULL, &ourCarbonControl);
	DfxGuiEditor *ourOwnerEditor = (DfxGuiEditor*) inUserData;
	DGControl *ourDGControl = NULL;
	if (ourOwnerEditor != NULL)
		ourDGControl = ourOwnerEditor->getDGControlByPlatformControlRef(ourCarbonControl);

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
				{
					if (ourDGControl->mustUpdate() == false)
					{
						result = noErr;
						break;
					}

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

//#define USE_QUICKDRAW_CLIPPING
					// clipping
#ifdef USE_QUICKDRAW_CLIPPING
					RgnHandle clipRgn = NewRgn();
					OpenRgn();
					ourDGControl->clipRegion(true);
					CloseRgn(clipRgn);
					SetClip(clipRgn);
					clipRgn = GetPortClipRegion(windowPort, clipRgn);
#endif

					// set up the CG context
					CGContextRef context;
					QDBeginCGContext(windowPort, &context);
#ifdef USE_QUICKDRAW_CLIPPING
					ClipCGContextToRegion(context, &portBounds, clipRgn);
					DisposeRgn(clipRgn);
#endif
					SyncCGContextOriginWithPort(context, windowPort);
					CGContextSaveGState(context);
#ifndef USE_QUICKDRAW_CLIPPING
					// define the clipping region
					CGRect clipRect;
					ourDGControl->getBounds()->copyToCGRect(&clipRect, portBounds.bottom);
if (ourDGControl->getType() == kDfxGuiType_slider)
{
clipRect.origin.x -= 13.0f;
clipRect.size.width += 27.0f;
}
					CGContextClipToRect(context, clipRect);
#endif
					// XXX disable anti-aliased drawing for image rendering
					CGContextSetShouldAntialias(context, false);
					ourDGControl->draw(context, portBounds.bottom);
					CGContextRestoreGState(context);
					CGContextSynchronize(context);
					QDEndCGContext(windowPort, &context);

					// restore original port, if we set a different port
					if (oldPort != NULL)
						SetPort(oldPort);
					result = noErr;
				}
				break;

			case kEventControlHitTest:
				{
//printf("kEventControlHitTest\n");
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
						ourOwnerEditor->setCurrentControl_mouseover(ourDGControl);
					}
					SetEventParameter(inEvent, kEventParamControlPart, typeControlPartCode, sizeof(hitPart), &hitPart);
//					result = eventNotHandledErr;	// let other event listeners have this if they want it
					result = noErr;
				}
				break;

/*
			case kEventControlHit:
				{
printf("kEventControlHit\n");
					result = noErr;
				}
				break;
*/

			case kEventControlTrack:
				{
//printf("kEventControlTrack\n");
//					ControlPartCode whatPart = kControlIndicatorPart;
					ControlPartCode whatPart = kControlNoPart;	// cuz otherwise we get a Hit event which triggers AUCVControl automation end
					SetEventParameter(inEvent, kEventParamControlPart, typeControlPartCode, sizeof(whatPart), &whatPart);
				}
//			case kEventControlClick:
//if (inEventKind == kEventControlClick) printf("kEventControlClick\n");
			case kEventControlContextualMenuClick:
//if (inEventKind == kEventControlContextualMenuClick) printf("kEventControlContextualMenuClick\n");
				{
					ourOwnerEditor->setCurrentControl_mouseover(ourDGControl);

//					UInt32 buttons = GetCurrentEventButtonState();	// bit 0 is mouse button 1, bit 1 is button 2, etc.
//					UInt32 buttons;	// bit 0 is mouse button 1, bit 1 is button 2, etc.
//					GetEventParameter(inEvent, kEventParamMouseChord, typeUInt32, NULL, sizeof(UInt32), NULL, &buttons);

					HIPoint mouseLocation_f;
//OSStatus fug = 
					GetEventParameter(inEvent, kEventParamMouseLocation, typeHIPoint, NULL, sizeof(HIPoint), NULL, &mouseLocation_f);
//printf("typeHIPoint result = %ld\n", fug);
//printf("mousef.x = %.0f, mousef.y = %.0f\n", mouseLocation_f.x, mouseLocation_f.y);
					Point mouseLocation;
					mouseLocation.h = (short) mouseLocation_f.x;
					mouseLocation.v = (short) mouseLocation_f.y;
					// XXX only kEventControlClick gives global mouse coordinates for kEventParamMouseLocation?
					if ( (inEventKind == kEventControlContextualMenuClick) || (inEventKind == kEventControlTrack) )
						GetGlobalMouse(&mouseLocation);
//printf("mouse.x = %d, mouse.y = %d\n", mouseLocation.h, mouseLocation.v);
//printf("\n");

					// orient the mouse coordinates as though the control were at 0, 0 (for convenience)
					// the content area of the window (i.e. not the title bar or any borders)
					Rect windowBounds;
					GetWindowBounds(GetControlOwner(ourCarbonControl), kWindowGlobalPortRgn, &windowBounds);
					// the position of the control relative to the top left corner of the window content area
					Rect controlBounds;
					GetControlBounds(ourCarbonControl, &controlBounds);
					mouseLocation.h -= windowBounds.left + controlBounds.left;
					mouseLocation.v -= windowBounds.top + controlBounds.top;

					UInt32 modifiers;
					GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32, NULL, sizeof(UInt32), NULL, &modifiers);
//					bool with_command = (modifiers & cmdKey) ? true : false;
					bool with_shift = ( (modifiers & shiftKey) || (modifiers & rightShiftKey) ) ? true : false;
					bool with_option = ( (modifiers & optionKey) || (modifiers & rightOptionKey) ) ? true : false;
//					bool with_control = ( (modifiers & controlKey) || (modifiers & rightControlKey) ) ? true : false;

					ourDGControl->mouseDown(mouseLocation, with_option, with_shift);
					ourOwnerEditor->setCurrentControl_clicked(ourDGControl);

					result = noErr;
				}
				break;

			default:
				result = eventNotHandledErr;
				break;
		}
	}

//	if ( (result != noErr) && (ourDGControl != NULL) )
//		printf("DGControl = 0x%08X,   event type = %ld\n", (unsigned long)ourDGControl, inEventKind);

	return result;
}
