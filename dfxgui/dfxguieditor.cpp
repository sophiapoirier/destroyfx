#include "dfxgui.h"

#include "dfxplugin.h"



static pascal OSStatus DGControlEventHandler(EventHandlerCallRef, EventRef, void *inUserData);
#define USE_BETTER_MOUSE_TRACKING	1
static pascal OSStatus DGWindowEventHandler(EventHandlerCallRef, EventRef, void *inUserData);

static pascal void DGIdleTimerProc(EventLoopTimerRef inTimer, void *inUserData);


//-----------------------------------------------------------------------------
DfxGuiEditor::DfxGuiEditor(AudioUnitCarbonView inInstance)
:	AUCarbonViewBase(inInstance)
{
	X = Y = 0;
	Images = NULL;
	Controls = NULL;
	itemCount = 0;
	idleTimer = NULL;
	idleTimerUPP = NULL;

	dgControlSpec.defType = kControlDefObjectClass;
	dgControlSpec.u.classRef = NULL;
	controlHandlerUPP = NULL;
	windowEventHandlerUPP = NULL;
	windowEventEventHandlerRef = NULL;
	currentControl_clicked = NULL;
	currentControl_mouseover = NULL;

	relaxed	= false;

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
	
	if (Controls != NULL)
		delete Controls;
	Controls = NULL;

	if (Images != NULL)
		delete Images;
	Images = NULL;

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
	X = (UInt32) inXOffset;
	Y = (UInt32) inYOffset;

	#if TARGET_PLUGIN_USES_MIDI
		setmidilearning(false);
	#endif


// create our HIToolbox object class for common event handling amongst our custom Carbon Controls

	EventTypeSpec toolboxClassEvents[] = {
		{ kEventClassControl, kEventControlDraw },
		{ kEventClassControl, kEventControlInitialize },
		{ kEventClassControl, kEventControlHitTest },
#if !USE_BETTER_MOUSE_TRACKING
		{ kEventClassControl, kEventControlTrack },
		{ kEventClassControl, kEventControlHit }, 
#endif
		{ kEventClassControl, kEventControlClick }, 
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
//								  { kEventClassMouse, kEventMouseMoved } 
								};
	windowEventHandlerUPP = NewEventHandlerUPP(DGWindowEventHandler);
	InstallEventHandler(GetWindowEventTarget(GetCarbonWindow()), windowEventHandlerUPP, 
						GetEventTypeCount(controlMouseEvents), controlMouseEvents, this, &windowEventEventHandlerRef);


// register for HitTest events on the background embedding pane so that we now when the mouse hovers over it
	EventTypeSpec paneEvents[] = {
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
	return open(inXOffset, inYOffset);
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::HandleEvent(EventRef inEvent)
{
	// we just want to catch when the mouse hovers over onto the background area
	if ( (GetEventClass(inEvent) == kEventClassControl) && (GetEventKind(inEvent) == kEventControlHitTest) )
	{
		ControlRef control;
		GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, NULL, sizeof(ControlRef), NULL, &control);
		if (control == mCarbonPane)
			setCurrentControl_mouseover(NULL);	// we don't count the background
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
void DfxGuiEditor::addImage(DGGraphic *inImage)
{
	inImage->setID( requestItemID() );

	if (Images == NULL)
		Images = inImage;
	else
		Images->append(inImage);
}

//-----------------------------------------------------------------------------
DGGraphic * DfxGuiEditor::getImageByID(UInt32 inID)
{
	if (Images == NULL)
		return NULL;
	return (DGGraphic*)Images->getByID(inID);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::addControl(DGControl *inControl)
{
	inControl->setID( requestItemID() );

	if (inControl->getDaddy() == NULL)
	{
		if (Controls == NULL)
			Controls = inControl;
		else
			Controls->append(inControl);
		inControl->setOffset(X, Y);
	}

	if (inControl->providesForeignControls())
		inControl->initForeignControls(&dgControlSpec);
	else
	{
		ControlRef newCarbonControl;
		Rect r;
		inControl->getBounds()->copyToRect(&r);
		verify_noerr( CreateCustomControl(GetCarbonWindow(), &r, &dgControlSpec, NULL, &newCarbonControl) );
		SetControl32BitMinimum(newCarbonControl, 0);
		if (inControl->isContinuousControl())
		{
			SInt32 controlrange = 0x7FFF;//1 << 14;
			SetControl32BitMaximum(newCarbonControl, controlrange);
/*
			if (inControl->isAUVPattached())
			{
				float valnorm = 0.0f;
				DfxParameterValueConversionRequest request;
				UInt32 dataSize = sizeof(request);
				request.parameterID = inControl->getAUVP().mParameterID;
				request.conversionType = kDfxParameterValueConversion_contract;
				request.inValue = inControl->getAUVP().GetValue();
				if (AudioUnitGetProperty(GetEditAudioUnit(), kDfxPluginProperty_ParameterValueConversion, 
										kAudioUnitScope_Global, (AudioUnitElement)0, &request, &dataSize) 
										== noErr)
					valnorm = request.outValue;
//				valnorm = (inControl->getAUVP().GetValue() - inControl->getAUVP().ParamInfo().minValue) / 
//							(inControl->getAUVP().ParamInfo().maxValue - inControl->getAUVP().ParamInfo().minValue);
				SetControl32BitValue( newCarbonControl, (SInt32) (valnorm * (float)controlrange) );
			}
*/
		}
		else
		{
			SetControl32BitMaximum(newCarbonControl, (SInt32) (inControl->getRange()+0.01f));
/*
			if (inControl->isAUVPattached())
				SetControl32BitValue(newCarbonControl, (SInt32) (inControl->getAUVP().GetValue() - inControl->getAUVP().ParamInfo().minValue));
*/
		}

		inControl->setCarbonControl(newCarbonControl);
		if (inControl->isAUVPattached())
		{
//			AddCarbonControl(AUCarbonViewControl::kTypeContinuous, inControl->getAUVP(), newCarbonControl);
			EmbedControl(newCarbonControl);
			inControl->createAUVcontrol();
//			AddControl(inControl->getAUVcontrol());
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
}

//-----------------------------------------------------------------------------
DGControl * DfxGuiEditor::getControlByID(UInt32 inID)
{
	if (Controls == NULL)
		return NULL;
	return (DGControl*) Controls->getByID(inID);
}

//-----------------------------------------------------------------------------
DGControl * DfxGuiEditor::getDGControlByCarbonControlRef(ControlRef inControl)
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
UInt32 DfxGuiEditor::requestItemID()
{
	itemCount++;
	return itemCount;
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
#if !USE_BETTER_MOUSE_TRACKING
	return eventNotHandledErr;
#endif

	if (GetEventClass(inEvent) != kEventClassMouse)
		return eventClassIncorrectErr;

	UInt32 inEventKind = GetEventKind(inEvent);
	DfxGuiEditor *ourOwnerEditor = (DfxGuiEditor*) inUserData;

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
			underDGControl = ourOwnerEditor->getDGControlByCarbonControlRef(underCarbonControl);
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

	Rect controlBounds;
	GetControlBounds(ourControl->getCarbonControl(), &controlBounds);
	Rect globalBounds;	// Window Content Region
	WindowRef window;
	GetEventParameter(inEvent, kEventParamWindowRef, typeWindowRef, NULL, sizeof(WindowRef), NULL, &window);
	GetWindowBounds(window, kWindowGlobalPortRgn, &globalBounds);

	// orient the mouse coordinates as though the control were at 0, 0 (for convenience)
	mouseLocation.h -= controlBounds.left + globalBounds.left;
	mouseLocation.v -= controlBounds.top + globalBounds.top;


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
		ourOwnerEditor->setRelaxed(false);

		// XXX do this to make Logic's touch automation work
		if ( ourControl->isAUVPattached() )//&& ourControl->isContinuousControl() )
		{
			ourOwnerEditor->TellListener(ourControl->getAUVP(), kAudioUnitCarbonViewEvent_MouseUpInControl, NULL);
			printf("DGControlMouseHandler -> TellListener(MouseUp, %lu)\n", ourControl->getAUVP().mParameterID);
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
		ourDGControl = ourOwnerEditor->getDGControlByCarbonControlRef(ourCarbonControl);	

	// the Carbon control reference has not been added yet, so our DGControl pointer is NULL, because we can't look it up by ControlRef yet
//	if ( (inEventClass == kEventClassControl) && (inEventKind == kEventControlInitialize) )
	if (inEventKind == kEventControlInitialize)
	{
		UInt32 dfxControlFeatures = kControlHandlesTracking | kControlSupportsDataAccess | kControlSupportsGetRegion;
		SetEventParameter(inEvent, kEventParamControlFeatures, typeUInt32, sizeof(UInt32), &dfxControlFeatures);
		return noErr;
	}

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

					// clipping
					RgnHandle clipRgn = NewRgn();
					OpenRgn();
					ourDGControl->clipRegion(true);
					CloseRgn(clipRgn);
					SetClip(clipRgn);
					clipRgn = GetPortClipRegion(windowPort, clipRgn);

					// drawing
					CGContextRef context;
					QDBeginCGContext(windowPort, &context);
					ClipCGContextToRegion(context, &portBounds, clipRgn);
					DisposeRgn(clipRgn);
					SyncCGContextOriginWithPort(context, windowPort);
					CGContextSaveGState(context);
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
					ourOwnerEditor->setCurrentControl_mouseover(ourDGControl);	// make sure that it ain't nuthin
					ControlPartCode hitPart = kControlIndicatorPart;	// scroll handle
					// also there is kControlButtonPart, kControlCheckBoxPart, kControlPicturePart
					SetEventParameter(inEvent, kEventParamControlPart, typeControlPartCode, sizeof(ControlPartCode), &hitPart);
#if USE_BETTER_MOUSE_TRACKING
					result = eventNotHandledErr;	// let other event listeners have this if they want it
#else
					result = noErr;
#endif
				}
				break;

			case kEventControlHit:
				{
//printf("kEventControlHit\n");
					result = noErr;
				}
				break;

			case kEventControlTrack:
				{
//printf("kEventControlTrack\n");
					ControlPartCode whatPart = kControlIndicatorPart;
					SetEventParameter(inEvent, kEventParamControlPart, typeControlPartCode, sizeof(ControlPartCode), &whatPart);
#if USE_BETTER_MOUSE_TRACKING
					result = eventNotHandledErr;	// cuz this makes us get a Hit event, which triggers AUCViewControl automation end
#else
					result = noErr;
#endif
				}
				break;

			case kEventControlClick:
//printf("kEventControlClick\n");
			case kEventControlContextualMenuClick:
//if (inEventKind == kEventControlContextualMenuClick) printf("kEventControlContextualMenuClick\n");
				{
					// prevent unnecessary draws
					ourOwnerEditor->setRelaxed(true);

//					UInt32 buttons = GetCurrentEventButtonState();	// bit 0 is mouse button 1, bit 1 is button 2, etc.
//					UInt32 buttons;	// bit 0 is mouse button 1, bit 1 is button 2, etc.
//					GetEventParameter(inEvent, kEventParamMouseChord, typeUInt32, NULL, sizeof(UInt32), NULL, &buttons);

					Rect controlBounds;
					GetControlBounds(ourCarbonControl, &controlBounds);
					Rect globalBounds;	// Window Content Region
					GetWindowBounds( GetControlOwner(ourCarbonControl), kWindowGlobalPortRgn, &globalBounds );

					HIPoint mouseLocation_f;
					GetEventParameter(inEvent, kEventParamMouseLocation, typeHIPoint, NULL, sizeof(HIPoint), NULL, &mouseLocation_f);
					Point mouseLocation;
//					GetGlobalMouse(&mouseLocation);
					mouseLocation.h = (short) mouseLocation_f.x;
					mouseLocation.v = (short) mouseLocation_f.y;

#if USE_BETTER_MOUSE_TRACKING
					// orient the mouse coordinates as though the control were at 0, 0 (for convenience)
					mouseLocation.h -= controlBounds.left + globalBounds.left;
					mouseLocation.v -= controlBounds.top + globalBounds.top;

					UInt32 modifiers;
					GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32, NULL, sizeof(UInt32), NULL, &modifiers);
//					bool with_command = (modifiers & cmdKey) ? true : false;
					bool with_shift = ( (modifiers & shiftKey) || (modifiers & rightShiftKey) ) ? true : false;
					bool with_option = ( (modifiers & optionKey) || (modifiers & rightOptionKey) ) ? true : false;
//					bool with_control = ( (modifiers & controlKey) || (modifiers & rightControlKey) ) ? true : false;

					ourDGControl->mouseDown(mouseLocation, with_option, with_shift);
					ourOwnerEditor->setCurrentControl_clicked(ourDGControl);

					result = noErr;
#else
					MouseTrackingResult mouseResult = kMouseTrackingMouseDown;
					while (true)
					{
						// orient the mouse coordinates as though the control were at 0, 0 (for convenience)
						mouseLocation.h -= controlBounds.left + globalBounds.left;
						mouseLocation.v -= controlBounds.top + globalBounds.top;

						UInt32 modifiers = GetCurrentEventKeyModifiers();
//						bool with_command = (modifiers & cmdKey) ? true : false;
						bool with_shift = ( (modifiers & shiftKey) || (modifiers & rightShiftKey) ) ? true : false;
						bool with_option = ( (modifiers & optionKey) || (modifiers & rightOptionKey) ) ? true : false;
//						bool with_control = ( (modifiers & controlKey) || (modifiers & rightControlKey) ) ? true : false;

						if (mouseResult == kMouseTrackingMouseDown)
						{
							ourDGControl->mouseDown(mouseLocation, with_option, with_shift);
//							printf("MouseDown\n");
						}
						else if (mouseResult == kMouseTrackingMouseDragged)
						{
							ourDGControl->mouseTrack(mouseLocation, with_option, with_shift);
//							printf("MouseDragged\n");
						}
						else if (mouseResult == kMouseTrackingMouseUp)
						{
							ourDGControl->mouseUp(mouseLocation, with_option, with_shift);
//							printf("MouseUp\n");
							break;
						}
						else printf("TrackMouseLocation = %d\n", mouseResult);

						TrackMouseLocation( (GrafPtr)(-1), &mouseLocation, &mouseResult );
						buttons = GetCurrentEventButtonState();	// bit 0 is mouse button 1, bit 1 is button 2, etc.
					}

					ourOwnerEditor->setRelaxed(false);

					// XXX do this to make Logic's touch automation work
					if ( ourDGControl->isAUVPattached() )//&& ourDGControl->isContinuousControl() )
					{
//printf("DGControlEventHandler -> TellListener(MouseUp, %lu)\n", ourDGControl->getAUVP().mParameterID);
//						ourOwnerEditor->TellListener(ourDGControl->getAUVP(), kAudioUnitCarbonViewEvent_MouseUpInControl, NULL);
					}

					result = eventNotHandledErr;	// cuz otherwise we don't get HitTest, Track, or Hit events (???)
#endif
// USE_BETTER_MOUSE_TRACKING
				}
				break;

			default:
				result = eventNotHandledErr;
				break;
		}
	}

//	if ( (result != noErr) && (ourDGControl != NULL) )
//		printf("DGControl ID = %ld,   event type = %ld\n", ourDGControl->getID(), inEventKind);

	return result;
}
