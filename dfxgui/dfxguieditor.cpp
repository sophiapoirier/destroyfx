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
	timer = NULL;

	relaxed	= false;

	dgControlSpec.defType = kControlDefObjectClass;
	dgControlSpec.u.classRef = NULL;

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
	if (timer != NULL)
		RemoveEventLoopTimer(timer);
	timer = NULL;
	
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
		if (dgControlSpec.u.classRef != NULL)
			UnregisterToolboxObjectClass( (ToolboxObjectClassRef)(dgControlSpec.u.classRef) );
		dgControlSpec.u.classRef = NULL;
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


	EventTypeSpec toolboxClassEvents[] = {
		{ kEventClassControl, kEventControlInitialize },
		{ kEventClassControl, kEventControlTrack },
		{ kEventClassControl, kEventControlDraw },
		{ kEventClassControl, kEventControlHitTest },
		{ kEventClassControl, kEventControlHit }, 
		{ kEventClassControl, kEventControlClick }, 
		{ kEventClassControl, kEventControlContextualMenuClick } 
	};

//	EventTypeSpec toolboxClassEvents[] = { {kEventClassMouse, kEventMouseDragged} };

	ToolboxObjectClassRef dgControlClass = NULL;
	unsigned long instanceAddress = (unsigned long) this;
	char toolboxClassIDstring[256];
	bool noSuccessYet = true;
	while (noSuccessYet)
	{
		sprintf(toolboxClassIDstring, "%s.DfxGuiControlClass%ld", PLUGIN_BUNDLE_IDENTIFIER, instanceAddress);
		CFStringRef toolboxClassIDcfstring = CFStringCreateWithCString(kCFAllocatorDefault, toolboxClassIDstring, CFStringGetSystemEncoding());
		if ( RegisterToolboxObjectClass(toolboxClassIDcfstring, NULL, GetEventTypeCount(toolboxClassEvents), toolboxClassEvents, 
										NewEventHandlerUPP(DGControlHandler), this, &dgControlClass) == noErr )
			noSuccessYet = false;
		CFRelease(toolboxClassIDcfstring);
		instanceAddress++;
	}

	dgControlSpec.u.classRef = dgControlClass;


	InstallEventLoopTimer(GetCurrentEventLoop(), 0.0, kEventDurationMillisecond * 50.0, 
							NewEventLoopTimerUPP(DGTimerProc), this, &timer);


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
DGControl * DfxGuiEditor::getControlByControlRef(ControlRef inControl)
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



//-----------------------------------------------------------------------------
static pascal OSStatus DGControlHandler(EventHandlerCallRef myHandler, EventRef inEvent, void *inUserData)
{
	OSStatus result = eventNotHandledErr;

	UInt32 inEventKind = GetEventKind(inEvent);
//	UInt32 inEventClass = GetEventClass(inEvent);

	ControlRef ourCarbonControl = NULL;
	GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, NULL, sizeof(ControlRef), NULL, &ourCarbonControl);
	DfxGuiEditor *ourOwnerEditor = (DfxGuiEditor*) inUserData;
	DGControl *ourDGControl = NULL;
	if (ourOwnerEditor != NULL)
		ourDGControl = ourOwnerEditor->getControlByControlRef(ourCarbonControl);	

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
//printf("DGControlHandler -> draw() --- mustUpdate = %s\n", ourDGControl->mustUpdate() ? "true" : "false");
					if (ourDGControl->mustUpdate() == false)
					{
						result = noErr;
						break;
					}
		
					GrafPtr oldPort;
					GetPort(&oldPort);
					CGrafPtr windowPort = GetWindowPort( GetControlOwner(ourCarbonControl) );
					SetPort(windowPort);
					Rect portBounds;
					GetPortBounds(windowPort, &portBounds);

					// C L I P P I N G
					RgnHandle clipRgn;
					clipRgn = NewRgn();
					OpenRgn();
					ourDGControl->clipRegion(true);
					CloseRgn(clipRgn);
					SetClip(clipRgn);
					clipRgn = GetPortClipRegion(windowPort, clipRgn);

					// D R A W I N G
					CGContextRef context;
					QDBeginCGContext(windowPort, &context);            
					ClipCGContextToRegion(context, &portBounds, clipRgn);
					SyncCGContextOriginWithPort(context, windowPort);
					CGContextSaveGState(context);
					ourDGControl->draw(context, portBounds.bottom);
					CGContextRestoreGState(context);
					CGContextSynchronize(context);
					QDEndCGContext(windowPort, &context);
					DisposeRgn(clipRgn);

					SetPort(oldPort);
					result = noErr;
				}
				break;

			case kEventControlHitTest:
				{
printf("kEventControlHitTest\n");
					ControlPartCode hitPart = kControlIndicatorPart;	// scroll handle
					// also there is kControlButtonPart, kControlCheckBoxPart, kControlPicturePart
					SetEventParameter(inEvent, kEventParamControlPart, typeControlPartCode, sizeof(ControlPartCode), &hitPart);
					result = noErr;
				}
				break;

			case kEventControlHit:
				{
printf("kEventControlHit\n");
					result = noErr;
				}
				break;

			case kEventControlTrack:
				{
printf("kEventControlTrack\n");
					ControlPartCode whatPart = kControlIndicatorPart;
					SetEventParameter(inEvent, kEventParamControlPart, typeControlPartCode, sizeof(ControlPartCode), &whatPart);
					result = noErr;
				}
				break;

			case kEventControlClick:
printf("kEventControlClick\n");
			case kEventControlContextualMenuClick:
if (inEventKind == kEventControlContextualMenuClick) printf("kEventControlContextualMenuClick\n");
				{
					UInt32 buttons = GetCurrentEventButtonState();	// bit 0 is mouse button 1, bit 1 is button 2, etc.

					// prevent unnecessary draws
					ourOwnerEditor->setRelaxed(true);

					Rect controlBounds;
					GetControlBounds(ourCarbonControl, &controlBounds);
					Rect globalBounds;	// Window Content Region
					GetWindowBounds( GetControlOwner(ourCarbonControl), kWindowGlobalPortRgn, &globalBounds );

					Point mouseLocation;
//					GetMouse(&mouseLocation);
//					GetGlobalMouse(&mouseLocation);
					GetEventParameter(inEvent, kEventParamMouseLocation, typeQDPoint, NULL, sizeof(Point), NULL, &mouseLocation);
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
							ourDGControl->mouseDown(&mouseLocation, with_option, with_shift);
//							printf("MouseDown\n");
						}
						else if (mouseResult == kMouseTrackingMouseDragged)
						{
							ourDGControl->mouseTrack(&mouseLocation, with_option, with_shift);
//							printf("MouseDragged\n");
						}
						else if (mouseResult == kMouseTrackingMouseUp)
						{
							ourDGControl->mouseUp(&mouseLocation, with_option, with_shift);
//							printf("MouseUp\n");
							break;
						}
						else printf("TrackMouseLocation = %d\n", mouseResult);

						TrackMouseLocation((GrafPtr)(-1), &mouseLocation, &mouseResult);
						buttons = GetCurrentEventButtonState();	// bit 0 is mouse button 1, bit 1 is button 2, etc.
					}

					ourOwnerEditor->setRelaxed(false);

					// XXX do this to make Logic's touch automation work
					if ( ourDGControl->isAUVPattached() )//&& ourDGControl->isContinuousControl() )
					{
//printf("DGControlHandler -> TellListener(%ld, kMouseUpInControl)\n", ourDGControl->getAUVP().mParameterID);
//						ourOwnerEditor->TellListener(ourDGControl->getAUVP(), kAudioUnitCarbonViewEvent_MouseUpInControl, NULL);
					}

					result = eventNotHandledErr;	// cuz otherwise we don't get HitTest, Hit, or Track events (???)
//					result = noErr;
				}
				break;

			default:
				result = eventNotHandledErr;
				break;
		}
	}

	if ( (result != noErr) && (ourDGControl != NULL) )
		printf("DGControl ID = %ld,   event type = %ld\n", ourDGControl->getID(), inEventKind);

	return result;
}
