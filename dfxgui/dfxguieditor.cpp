#include "dfxgui.h"

#include "dfxplugin.h"



static pascal OSStatus DGControlHandler(EventHandlerCallRef, EventRef, void *inUserData);

static pascal void DGTimerProc(EventLoopTimerRef inTimer, void *inUserData);


static UInt32 numConstructions = 0;
//-----------------------------------------------------------------------------
DfxGuiEditor::DfxGuiEditor(AudioUnitCarbonView inInstance)
:	AUCarbonViewBase(inInstance)
{
	X = Y = 0;
	Images = NULL;
	Controls = NULL;
	itemCount = 0;

	relaxed	= false;

	dgControlClass			= NULL;
	dgControlSpec.defType		= kControlDefObjectClass;
	dgControlSpec.u.classRef	= NULL;

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
			CFURLGetFSRef(myResourcesURL, &myResourceDirRef);
			OSStatus status = FSGetCatalogInfo(&myResourceDirRef, kFSCatInfoNone, NULL, NULL, &bundleResourceDirFSSpec, NULL);
			if (status == noErr)
				status = FMActivateFonts(&bundleResourceDirFSSpec, NULL, NULL, kFMLocalActivationContext);
			if (status == noErr)
				fontsWereActivated = true;
		}
	}

	numConstructions++;
//	printf("DfxGuiEditor constructor no. %ld\n", numConstructions);
}

//-----------------------------------------------------------------------------
DfxGuiEditor::~DfxGuiEditor()
{
	RemoveEventLoopTimer(timer);
	
	if (Controls != NULL)
		delete Controls;
	Controls = NULL;

	if (Images != NULL)
		delete Images;
	Images = NULL;

	// only unregister in Mac OS X version 10.2.3 or higher, otherwise this will cause a crash
	long systemVersion = 0;
	if ( (Gestalt(gestaltSystemVersion, &systemVersion) == noErr) && ((systemVersion & 0xFFFF) >= 0x1023) )
	{
//printf("using Mac OS X version 10.2.3 or higher, so our control class will be unregistered\n");
		if (dgControlClass != NULL)
			UnregisterToolboxObjectClass(dgControlClass);
		dgControlClass = NULL;
	}
//else printf("using a version of Mac OS X lower than 10.2.3, so our control class will NOT be unregistered\n");

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

	EventTypeSpec toolboxClassEventList[] = { {kEventClassControl, kEventControlDraw}, 
											  {kEventClassControl, kEventControlHit}, 
											  {kEventClassControl, kEventControlHitTest}, 
											  {kEventClassControl, kEventControlInitialize}, 
											  {kEventClassControl, kEventControlTrack}, 
											  {kEventClassControl, kEventControlClick}, 
											  {kEventClassControl, kEventControlContextualMenuClick} };
//	EventTypeSpec toolboxClassEventList[] = { {kEventClassMouse, kEventMouseDragged} };
//kEventMouseDragged

	unsigned long instanceAddress = (unsigned long) this;
	bool toolboxClassIDavailable = false;
	char *toolboxClassIDstring = (char*) malloc(256);
	while (!toolboxClassIDavailable)
	{
//printf("GUI instance address = %ld\n", (unsigned long)instanceAddress);
		sprintf(toolboxClassIDstring, "%s.DfxGuiControlClass%ld", PLUGIN_BUNDLE_IDENTIFIER, instanceAddress);
		CFStringRef toolboxClassIDcfstring = CFStringCreateWithCString(kCFAllocatorDefault, toolboxClassIDstring, CFStringGetSystemEncoding());
		if ( RegisterToolboxObjectClass(toolboxClassIDcfstring,
										NULL, 
										GetEventTypeCount(toolboxClassEventList), 
										toolboxClassEventList, 
										NewEventHandlerUPP(DGControlHandler), 
										this,
										&dgControlClass) == noErr )
			toolboxClassIDavailable = true;
		CFRelease(toolboxClassIDcfstring);
		instanceAddress++;
	}
	free(toolboxClassIDstring);

	dgControlSpec.u.classRef = dgControlClass;

	InstallEventLoopTimer(
        GetCurrentEventLoop(),
        0,
        kEventDurationMillisecond * 50.0,
        NewEventLoopTimerUPP(DGTimerProc),
        this,
        &timer);

	UInt32 dataSize = sizeof(dfxplugin);
	if (AudioUnitGetProperty(GetEditAudioUnit(), kDfxPluginProperty_PluginPtr, 
							kAudioUnitScope_Global, (AudioUnitElement)0, &dfxplugin, &dataSize) 
							!= noErr)
		dfxplugin = NULL;

	// let the child class do its thing
	return open(inXOffset, inYOffset);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::idle()
{
// do something idle

	if (Controls != NULL)
		Controls->idle();
}

//-----------------------------------------------------------------------------
static pascal void DGTimerProc(EventLoopTimerRef inTimer, void *inUserData)
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
void DfxGuiEditor::addControl(DGControl *inCtrl)
{
	inCtrl->setID( requestItemID() );

	if (inCtrl->getDaddy() == NULL)
	{
		if (Controls == NULL)
			Controls = inCtrl;
		else
			Controls->append(inCtrl);
		inCtrl->setOffset(X, Y);
	}

	if (inCtrl->providesForeignControls())
		inCtrl->initForeignControls(&dgControlSpec);
	else
	{
		ControlRef newControl;
		Rect r;
		inCtrl->getBounds()->copyToRect(&r);
		verify_noerr(CreateCustomControl(GetCarbonWindow(), &r, &dgControlSpec, NULL, &newControl));
		SetControl32BitMinimum(newControl, 0);
		if (inCtrl->isContinuousControl())
		{
			SInt32 controlrange = 0x7FFF;//1 << 14;
			SetControl32BitMaximum(newControl, controlrange);
/*
			if (inCtrl->isAUVPattached())
			{
				float valnorm = 0.0f;
				DfxParameterValueConversionRequest request;
				UInt32 dataSize = sizeof(request);
				request.parameterID = inCtrl->getAUVP().mParameterID;
				request.conversionType = kDfxParameterValueConversion_contract;
				request.inValue = inCtrl->getAUVP().GetValue();
				if (AudioUnitGetProperty(GetEditAudioUnit(), kDfxPluginProperty_ParameterValueConversion, 
										kAudioUnitScope_Global, (AudioUnitElement)0, &request, &dataSize) 
										== noErr)
					valnorm = request.outValue;
//				valnorm = (inCtrl->getAUVP().GetValue() - inCtrl->getAUVP().ParamInfo().minValue) / 
//							(inCtrl->getAUVP().ParamInfo().maxValue - inCtrl->getAUVP().ParamInfo().minValue);
				SetControl32BitValue( newControl, (SInt32) (valnorm * (float)controlrange) );
			}
*/
		}
		else
		{
			SetControl32BitMaximum(newControl, (SInt32) inCtrl->getRange());
/*
			if (inCtrl->isAUVPattached())
				SetControl32BitValue(newControl, (SInt32) (inCtrl->getAUVP().GetValue() - inCtrl->getAUVP().ParamInfo().minValue));
*/
		}

		inCtrl->setCarbonControl(newControl);
		if (inCtrl->isAUVPattached())
		{
//			AddCarbonControl(AUCarbonViewControl::kTypeContinuous, inCtrl->getAUVP(), newControl);
			EmbedControl(newControl);
			inCtrl->createAUVcontrol();
			inCtrl->getAUVcontrol()->Bind();
//			AddControl(inCtrl->getAUVcontrol());
		}
		else
		{
			SetControl32BitValue(newControl, 0);
			EmbedControl(newControl);
/*
UInt32 feat = 0;
GetControlFeatures(newControl, &feat);
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
DGControl * DfxGuiEditor::getControlByControlRef(ControlRef inControl)
{
	DGControl* current = Controls;
	
	while (current != NULL)
	{
		if (current->isControlRef(inControl))
		{
			current = current->getChild(inControl);
			break;
		}
		current = (DGControl*)current->getNext();
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
	AudioUnitSetProperty(GetEditAudioUnit(), kDfxPluginProperty_ResetMidiLearn, 
				kAudioUnitScope_Global, (AudioUnitElement)0, NULL, sizeof(bool));
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



static pascal OSStatus DGControlHandler(EventHandlerCallRef myHandler, EventRef inEvent, void *inUserData)
{
	OSStatus result = eventNotHandledErr;

	GrafPtr thePort;
	CGrafPtr windowPort;
	WindowRef theWindow;
	ControlRef theControl;
	
	Rect controlBounds;
	Rect portBounds;
	Rect globalBounds;	// Window Content Region...
	
	CGContextRef context;

	UInt32 inEventKind = GetEventKind(inEvent);
	UInt32 inEventClass = GetEventClass(inEvent);

	GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, NULL, sizeof(ControlRef), NULL, &theControl);

	GetControlBounds(theControl, &controlBounds);
	theWindow = GetControlOwner(theControl);
	GetWindowBounds(theWindow, kWindowGlobalPortRgn, &globalBounds);

	DfxGuiEditor *theOwnerEditor = (DfxGuiEditor*)inUserData;
	DGControl *theCtrl = NULL;
	if (theOwnerEditor != NULL)
		theCtrl = theOwnerEditor->getControlByControlRef(theControl);	

	// the control is not created yet so the control pointer is NULL
	if ( (inEventClass == kEventClassControl) && (inEventKind == kEventControlInitialize) )
	{
printf("\tinitialize control!\n");
		UInt32 dfxControlFeatures = kControlHandlesTracking | kControlSupportsDataAccess | kControlSupportsGetRegion;
		SetEventParameter(inEvent, kEventParamControlFeatures, typeUInt32, sizeof(UInt32), &dfxControlFeatures);
		return noErr;
	}

	if (theCtrl != NULL)
	{
		switch (inEventKind)
		{
			case kEventControlApplyBackground:
				break; 

			case kEventControlDraw:
//printf("DGControlHandler -> draw(%ld)   ---   mustUpdate = %s\n", (long)theCtrl, theCtrl->mustUpdate() ? "true" : "false");
				if (theCtrl->mustUpdate() == false)
				{
					result = noErr;
					break;
				}
		
//				printf("drawing\n");
				GetPort (&thePort);
				windowPort = GetWindowPort(theWindow);
				SetPort(windowPort);
				GetPortBounds(windowPort, &portBounds);
				
				// C L I P P I N G
				RgnHandle clipRgn;
				clipRgn = NewRgn();
				OpenRgn();
			
				//FrameRect(&portBounds);
				theCtrl->clipRegion(true);
				CloseRgn(clipRgn);
				SetClip(clipRgn);
				clipRgn = GetPortClipRegion(windowPort, clipRgn);
				
				// D R A W I N G
				QDBeginCGContext(windowPort, &context);            
				ClipCGContextToRegion(context, &portBounds, clipRgn);
				SyncCGContextOriginWithPort(context, windowPort);
				CGContextSaveGState(context);
				theCtrl->draw(context, portBounds.bottom);
				CGContextRestoreGState(context);
				CGContextSynchronize(context);
				QDEndCGContext(windowPort, &context);
				DisposeRgn(clipRgn);
				SetPort(thePort);

				result = noErr;
				break;

			case kEventControlHit:
printf("kEventControlHit\n");
				break;

			case kEventControlHitTest:
				{
printf("\thit (test) control!\n");
					ControlPartCode hitPart = kControlIndicatorPart;	// scroll handle
					// also there is kControlButtonPart, kControlCheckBoxPart, kControlPicturePart
					SetEventParameter(inEvent, kEventParamControlPart, typeControlPartCode, sizeof(ControlPartCode), &hitPart);
					result = noErr;
				}
				break;

case kEventMouseDragged:
printf("draggin' da mouse...\n");
			case kEventControlTrack:
printf("trackin'\n");
			case kEventControlClick:
//printf("kEventControlClick in DfxGui\n");
			case kEventControlContextualMenuClick:
				// prevent unnecessary draws
				theOwnerEditor->setRelaxed(true);
				UInt32 modifiers = GetCurrentEventKeyModifiers();
				bool with_option = ( (modifiers & optionKey) || (modifiers & rightOptionKey) )? true : false;
				bool with_shift = ( (modifiers & shiftKey) || (modifiers & rightShiftKey) ) ? true : false;
				bool with_control = ( (modifiers & controlKey) || (modifiers & rightControlKey) ) ? true : false;
				bool with_command = (modifiers & cmdKey) ? true : false;

				Point P;
				//GetMouse(&P);
				GetGlobalMouse(&P);
				UInt32 buttons = GetCurrentEventButtonState();	// bit 0 is mouse button 1, bit 1 is button 2, etc.
				P.h -= controlBounds.left + globalBounds.left;
				P.v -= controlBounds.top + globalBounds.top;
				
				theCtrl->mouseDown(&P, with_option, with_shift);
				MouseTrackingResult outResult;
            			
//				while (true)
				{
					TrackMouseLocation(NULL, &P, &outResult);
					GetGlobalMouse(&P);
					modifiers = GetCurrentEventKeyModifiers();
					with_option = ( (modifiers & optionKey) || (modifiers & rightOptionKey) ) ? true : false;
					with_shift = ( (modifiers & shiftKey) || (modifiers & rightShiftKey) ) ? true : false;
					with_control = ( (modifiers & controlKey) || (modifiers & rightControlKey) ) ? true : false;
					with_command = (modifiers & cmdKey) ? true : false;
					buttons = GetCurrentEventButtonState();	// bit 0 is mouse button 1, bit 1 is button 2, etc.
					P.h -= controlBounds.left + globalBounds.left;
					P.v -= controlBounds.top + globalBounds.top;

					if (outResult == kMouseTrackingMouseUp)
					{
						theCtrl->mouseUp(&P, with_option, with_shift);
//						break;
					}
					else
						theCtrl->mouseTrack(&P, with_option, with_shift);
				}

				theOwnerEditor->setRelaxed(false);
// XXX do this to make Logic's touch automation work
if ( theCtrl->isAUVPattached() && theCtrl->isContinuousControl() )
{
//printf("DGControlHandler -> TellListener(%ld, kMouseUpInControl)\n", theCtrl->getAUVP().mParameterID);
	theOwnerEditor->TellListener(theCtrl->getAUVP(), kAudioUnitCarbonViewEvent_MouseUpInControl, NULL);
}
				result = noErr;
				break;
		}
	}

	if ( (result != noErr) && (theCtrl != NULL) )
		printf("DGControl ID = %ld,   event type = %ld\n", theCtrl->getID(), inEventKind);

	return result;
}
