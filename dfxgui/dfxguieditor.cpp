/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2012  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Destroy FX Library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Destroy FX Library.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "dfxguieditor.h"
#include "dfxguibutton.h"

#ifdef TARGET_API_RTAS
	#include "dfxplugin-base.h"
#else
	#include "dfxplugin.h"
#endif

#ifdef TARGET_API_AUDIOUNIT
	#include "dfx-au-utilities.h"
#endif

#ifdef TARGET_API_RTAS
	#include "ConvertUtils.h"
#endif



#if TARGET_OS_MAC && !__LP64__
static pascal OSStatus DFXGUI_TextEntryEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void * inUserData);
#endif

#ifdef TARGET_API_AUDIOUNIT
static void DFXGUI_AudioUnitEventListenerProc(void * inCallbackRefCon, void * inObject, const AudioUnitEvent * inEvent, UInt64 inEventHostTime, Float32 inParameterValue);
#endif

// XXX for now this is a good idea, until it's totally obsoleted
#define AU_DO_OLD_STYLE_PARAMETER_CHANGE_GESTURES	1

#if TARGET_OS_MAC && !__LP64__
static EventHandlerUPP gTextEntryEventHandlerUPP = NULL;

static const CFStringRef kDfxGui_NibName = CFSTR("dfxgui");
const OSType kDfxGui_ControlSignature = DESTROYFX_CREATOR_ID;
const SInt32 kDfxGui_TextEntryControlID = 1;
#endif

#ifdef TARGET_API_AUDIOUNIT
//	static const CFStringRef kDfxGui_AUPresetFileUTI = CFSTR("org.destroyfx.aupreset");
	static const CFStringRef kDfxGui_AUPresetFileUTI = CFSTR("com.apple.audio-unit-preset");	// XXX implemented in Mac OS X 10.4.11 or maybe a little earlier, but no public constant published yet
#endif

const long kDfxGui_BackgroundImageResourceID = 128;


//-----------------------------------------------------------------------------
DfxGuiEditor::DfxGuiEditor(DGEditorListenerInstance inInstance)
:	TARGET_API_EDITOR_BASE_CLASS(inInstance)
{
/*
CFStringRef text = CFSTR("oh hey wetty");
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

	#if TARGET_PLUGIN_USES_MIDI
		midiLearnButton = NULL;
		midiResetButton = NULL;
	#endif
  
#if TARGET_OS_MAC
	fontsATSContainer = kATSFontContainerRefUnspecified;
	fontsWereActivated = false;	// guilty until proven innocent

	clipboardRef = NULL;

#if !__LP64__
	textEntryWindow = NULL;
	textEntryControl = NULL;
	textEntryResultString = NULL;
#endif
#endif

#ifdef TARGET_API_AUDIOUNIT
#if !__LP64__
	mOwnerAUCarbonView = NULL;
#endif
	auParameterListSize = 0;
	auParameterList = NULL;
	auMaxParameterID = 0;
	auEventListener = NULL;
#endif

#ifdef TARGET_API_RTAS
	m_Process = inInstance;

	parameterHighlightColors = (long*) malloc(GetNumParameters() * sizeof(*parameterHighlightColors));
	for (long i=0; i < GetNumParameters(); i++)
		parameterHighlightColors[i] = eHighlight_None;
#endif

	currentControl_clicked = NULL;
	currentControl_mouseover = NULL;
	mousedOverControlsList = NULL;
	mTooltipSupport = NULL;

	numAudioChannels = 0;
	mJustOpened = false;
	mEditorOpenErr = kDfxErr_NoError;

	rect.top = rect.left = rect.bottom = rect.right = 0;
	// load the background image
	// we don't need to load all bitmaps, this could be done when open is called
	// XXX hack
	backgroundImage = new DGImage(PLUGIN_BACKGROUND_IMAGE_FILENAME, kDfxGui_BackgroundImageResourceID);
	if (backgroundImage != NULL)
	{
		// init the size of the plugin
		rect.right = rect.left + backgroundImage->getWidth();
		rect.bottom = rect.top + backgroundImage->getHeight();
	}

	setKnobMode(kLinearMode);
}

//-----------------------------------------------------------------------------
DfxGuiEditor::~DfxGuiEditor()
{
	if ( IsOpen() )
		CloseEditor();

	#if TARGET_PLUGIN_USES_MIDI
		setmidilearning(false);
		midiLearnButton = NULL;
		midiResetButton = NULL;
	#endif

#ifdef TARGET_API_AUDIOUNIT
#if !__LP64__
	mOwnerAUCarbonView = NULL;
#endif

	// remove and dispose the event listener, if we created it
	if (auEventListener != NULL)
	{
		if (auParameterList != NULL)
		{
			for (UInt32 i=0; i < auParameterListSize; i++)
			{
				AudioUnitParameter auParam = dfxgui_MakeAudioUnitParameter(auParameterList[i]);
				AUListenerRemoveParameter(auEventListener, this, &auParam);
			}
		}
		if (AUEventListenerRemoveEventType != NULL)
		{
			AUEventListenerRemoveEventType(auEventListener, this, &streamFormatPropertyAUEvent);
			AUEventListenerRemoveEventType(auEventListener, this, &parameterListPropertyAUEvent);
			#if TARGET_PLUGIN_USES_MIDI
				AUEventListenerRemoveEventType(auEventListener, this, &midiLearnPropertyAUEvent);
			#endif
		}
		AUListenerDispose(auEventListener);
	}
	auEventListener = NULL;

	if (auParameterList != NULL)
		free(auParameterList);
	auParameterList = NULL;
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
	// This will actually automatically be deactivated when the host app is quit, 
	// so there's no need to deactivate fonts ourselves.  
	// In fact, since there may be multiple plugin GUI instances, it's safer 
	// to just let the fonts stay activated.
//	if ( fontsWereActivated && (fontsATSContainer != NULL) )
//		ATSFontDeactivate(fontsATSContainer, NULL, kATSOptionFlagsDefault);

	if (clipboardRef != NULL)
		CFRelease(clipboardRef);
	clipboardRef = NULL;
#endif

	if (backgroundImage != NULL)
		backgroundImage->forget();
	backgroundImage = NULL;

#ifdef TARGET_API_RTAS
	m_Process = NULL;

	if (parameterHighlightColors != NULL)
		free(parameterHighlightColors);
	parameterHighlightColors = NULL;
#endif
}


//-----------------------------------------------------------------------------
bool DfxGuiEditor::open(void * inWindow)
{
	// !!! always call this !!!
	TARGET_API_EDITOR_BASE_CLASS::open(inWindow);

	controlsList = NULL;
	imagesList = NULL;

	#if TARGET_PLUGIN_USES_MIDI
		setmidilearning(false);
	#endif

	CRect frameRect(rect.left, rect.top, rect.right, rect.bottom);
	frame = new CFrame(frameRect, inWindow, this);
	frame->setBackground( GetBackgroundImage() );


	currentControl_clicked = NULL;	// make sure that it isn't set to anything
	mousedOverControlsList = NULL;
	setCurrentControl_mouseover(NULL);


#if TARGET_OS_MAC && !__LP64__
	Duration tooltipDelayDur = 0;
	OSStatus hmStatus = HMGetTagDelay(&tooltipDelayDur);
	if (hmStatus == noErr)
	{
		if (tooltipDelayDur < 0)	// this signifies microseconds rather than milliseconds
			tooltipDelayDur = abs(tooltipDelayDur) / 1000;
		mTooltipSupport = new CTooltipSupport(frame, tooltipDelayDur);
	}
	else
#else
		mTooltipSupport = new CTooltipSupport(frame);
#endif


// determine the number of audio channels currently configured for the AU
	numAudioChannels = getNumAudioChannels();

#ifdef TARGET_API_AUDIOUNIT
// install an event listener for the parameters and necessary properties
	auParameterList = CreateParameterList(kAudioUnitScope_Global, &auParameterListSize);
	if ( (AUEventListenerCreate != NULL) && (AUEventListenerAddEventType != NULL) )
	{
		// XXX should I use kCFRunLoopCommonModes instead, like AUCarbonViewBase does?
		OSStatus status = AUEventListenerCreate(DFXGUI_AudioUnitEventListenerProc, this, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode, 
								kDfxGui_NotificationInterval, kDfxGui_NotificationInterval, &auEventListener);
		if (status == noErr)
		{
			if (auParameterList != NULL)
			{
				for (UInt32 i=0; i < auParameterListSize; i++)
				{
					AudioUnitParameter auParam = dfxgui_MakeAudioUnitParameter(auParameterList[i]);
					AUListenerAddParameter(auEventListener, this, &auParam);
				}
			}

			memset(&streamFormatPropertyAUEvent, 0, sizeof(streamFormatPropertyAUEvent));
			streamFormatPropertyAUEvent.mEventType = kAudioUnitEvent_PropertyChange;
			streamFormatPropertyAUEvent.mArgument.mProperty.mAudioUnit = dfxgui_GetEffectInstance();
			streamFormatPropertyAUEvent.mArgument.mProperty.mPropertyID = kAudioUnitProperty_StreamFormat;
			streamFormatPropertyAUEvent.mArgument.mProperty.mScope = kAudioUnitScope_Output;
			streamFormatPropertyAUEvent.mArgument.mProperty.mElement = 0;
			AUEventListenerAddEventType(auEventListener, this, &streamFormatPropertyAUEvent);

			parameterListPropertyAUEvent = streamFormatPropertyAUEvent;
			parameterListPropertyAUEvent.mArgument.mProperty.mPropertyID = kAudioUnitProperty_ParameterList;
			parameterListPropertyAUEvent.mArgument.mProperty.mScope = kAudioUnitScope_Global;
			AUEventListenerAddEventType(auEventListener, this, &parameterListPropertyAUEvent);

			#if TARGET_PLUGIN_USES_MIDI
				midiLearnPropertyAUEvent = streamFormatPropertyAUEvent;
				midiLearnPropertyAUEvent.mArgument.mProperty.mPropertyID = kDfxPluginProperty_MidiLearn;
				midiLearnPropertyAUEvent.mArgument.mProperty.mScope = kAudioUnitScope_Global;
				AUEventListenerAddEventType(auEventListener, this, &midiLearnPropertyAUEvent);
			#endif
		}
	}
#endif


#if TARGET_OS_MAC
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
#if __LP64__ || (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5)
				OSStatus status = ATSFontActivateFromFileReference(&bundleResourcesDirFSRef, kATSFontContextLocal, 
									kATSFontFormatUnspecified, NULL, kATSOptionFlagsProcessSubdirectories, &fontsATSContainer);
#else
				FSSpec bundleResourcesDirFSSpec;
				OSStatus status = FSGetCatalogInfo(&bundleResourcesDirFSRef, kFSCatInfoNone, NULL, NULL, &bundleResourcesDirFSSpec, NULL);
				// activate all of our fonts in the local (host application) context only
				// if the fonts already exist on the user's system, our versions will take precedence
				if (status == noErr)
					status = ATSFontActivateFromFileSpecification(&bundleResourcesDirFSSpec, kATSFontContextLocal, 
									kATSFontFormatUnspecified, NULL, kATSOptionFlagsProcessSubdirectories, &fontsATSContainer);
#endif
				if (status == noErr)
					fontsWereActivated = true;
			}
			CFRelease(bundleResourcesDirCFURL);
		}
	}
#endif


	mEditorOpenErr = OpenEditor();
	if (mEditorOpenErr == kDfxErr_NoError)
	{
		mJustOpened = true;

		// embed/activate every control
		// XXX do this?
		DGControlsList * tempcl = controlsList;
		while (tempcl != NULL)
		{
//			tempcl->control->embed();	// XXX I removed this method, so any need to do this iteration?
			tempcl = tempcl->next;
		}

		// allow for anything that might need to happen after the above post-opening stuff is finished
		post_open();

		return true;
	}
	else
	{
		return false;
	}
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::close()
{
	mJustOpened = false;

	CloseEditor();

	// zero the member frame before we delete it so that other asynchronous calls don't crash
	CFrame * frame_temp = frame;
	frame = NULL;

	if (mTooltipSupport != NULL)
		mTooltipSupport->forget();
	mTooltipSupport = NULL;

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

	if (frame_temp != NULL)
		frame_temp->forget();
	frame_temp = NULL;

	TARGET_API_EDITOR_BASE_CLASS::close();
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::IsOpen()
{
	if (frame == NULL)
		return false;
	return ( isOpen() 
#if (VSTGUI_VERSION_MAJOR < 4)
			&& frame->isOpen() 
#endif
			);	// XXX both?
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::setParameter(TARGET_API_EDITOR_INDEX_TYPE inParameterIndex, float inValue)
{
	if (! IsOpen() )
		return;

	DGControlsList * tempcl = controlsList;
	while (tempcl != NULL)
	{
		if (tempcl->control->getTag() == inParameterIndex)
		{
			tempcl->control->setValue(inValue);
			tempcl->control->invalid();	// XXX this seems to be necessary for 64-bit AU to update from outside parameter value changes?
		}
		tempcl = tempcl->next;
	}

	parameterChanged(inParameterIndex, inValue);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::valueChanged(CControl * inControl)
{
	long paramIndex = inControl->getTag();
	float paramValue_norm = inControl->getValue();

	if ( dfxgui_IsValidParamID(paramIndex) )
	{
#ifdef TARGET_API_AUDIOUNIT
		AudioUnitParameterValue paramValue_literal = dfxgui_ExpandParameterValue(paramIndex, paramValue_norm);
		// XXX or should I call setparameter_f()?
		AudioUnitParameter auParam = dfxgui_MakeAudioUnitParameter(paramIndex);
		AUParameterSet(NULL, inControl, &auParam, paramValue_literal, 0);
#endif
#ifdef TARGET_API_VST
		getEffect()->setParameterAutomated(paramIndex, paramValue_norm);
#endif
#ifdef TARGET_API_RTAS
		if (m_Process != NULL)
		{
			// XXX though the model of calling SetControlValue might make more seem like 
			// better design than calling setparameter_gen on the effect, in practice, 
			// our DfxParam objects won't get their values updated to reflect the change until 
			// the next call to UpdateControlInAlgorithm which be deferred until the start of 
			// the next audio render call, which means that in the meantime getparameter_* 
			// methods will return the previous rather than current value
//			m_Process->SetControlValue( DFX_ParameterID_ToRTAS(paramIndex), ConvertToDigiValue(paramValue_norm) );
			m_Process->setparameter_gen(paramIndex, paramValue_norm);
		}
#endif
	}
}

#ifndef TARGET_API_VST
//-----------------------------------------------------------------------------
void DfxGuiEditor::beginEdit(long inParameterIndex)
{
	TARGET_API_EDITOR_BASE_CLASS::beginEdit(inParameterIndex);
	if (inParameterIndex >= 0)
		automationgesture_begin(inParameterIndex);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::endEdit(long inParameterIndex)
{
	TARGET_API_EDITOR_BASE_CLASS::endEdit(inParameterIndex);
	if (inParameterIndex >= 0)
		automationgesture_end(inParameterIndex);
}
#endif	// !TARGET_API_VST

//-----------------------------------------------------------------------------
void DfxGuiEditor::idle()
{
	// call this so that idle() actually happens
	TARGET_API_EDITOR_BASE_CLASS::idle();

	if (! IsOpen() )
		return;

	if (mJustOpened)
	{
		mJustOpened = false;
		dfxgui_EditorShown();
#if WINDOWS_VERSION // && defined(TARGET_API_RTAS)
		// XXX this seems to be necessary to correct background re-drawing failure 
		// when switching between different plugins in an open plugin editor window
		getFrame()->invalid();
#endif
	}

	do_idle();
}


//-----------------------------------------------------------------------------
void DfxGuiEditor::do_idle()
{
	if (! IsOpen() )
		return;

	// call any child class implementation of the virtual idle method
	dfxgui_Idle();

	// call every controls' implementation of its idle method
	DGControlsList * tempcl = controlsList;
	while (tempcl != NULL)
	{
//		tempcl->control->idle();	// XXX missing from CControl, reimplement?
		tempcl = tempcl->next;
	}
	// XXX call background control idle as well?
}

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

	long parameterIndex = inControl->getTag();
	// XXX only add it to our controls list if it is attached to a parameter (?)
	if ( dfxgui_IsValidParamID(parameterIndex) )
	{
		controlsList = new DGControlsList(inControl, controlsList);

#ifdef TARGET_API_RTAS
		if (m_Process != NULL)
		{
			long parameterIndex_rtas = DFX_ParameterID_ToRTAS(parameterIndex);
			long value_rtas = 0;
			m_Process->GetControlValue(parameterIndex_rtas, &value_rtas);
			inControl->setValue( ConvertToVSTValue(value_rtas) );
			long defaultValue_rtas = 0;
			m_Process->GetControlDefaultValue(parameterIndex_rtas, &defaultValue_rtas);	// VSTGUI: necessary for Alt+Click behavior
			inControl->setDefaultValue( ConvertToVSTValue(defaultValue_rtas) );	// VSTGUI: necessary for Alt+Click behavior
		}
		else
#endif
		{
		inControl->setValue( getparameter_gen(parameterIndex) );
		float defaultValue = GetParameter_defaultValue(parameterIndex);
		defaultValue = dfxgui_ContractParameterValue(parameterIndex, defaultValue);
		inControl->setDefaultValue(defaultValue);
		}
	}
	else
		inControl->setValue(0.0f);

	getFrame()->addView(inControl);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::removeControl(DGControl * inControl)
{
	if (inControl == NULL)
		return;

	DGControlsList * previousControl = NULL;
	DGControlsList * tempcl = controlsList;
	while (tempcl != NULL)
	{
		if (tempcl->control == inControl)
		{
			if (previousControl == NULL)
				controlsList = tempcl->next;
			else
				previousControl->next = tempcl->next;
			delete tempcl;
			return;
		}
		previousControl = tempcl;
		tempcl = tempcl->next;
	}
}

//-----------------------------------------------------------------------------
DGControl * DfxGuiEditor::getNextControlFromParameterID(long inParameterID, DGControl * inPreviousControl)
{
	bool previousFound = (inPreviousControl == NULL) ? true : false;
	DGControlsList * tempcl = controlsList;
	while (tempcl != NULL)
	{
		DGControl * foundControl = NULL;
		if (tempcl->control->getTag() == inParameterID)
			foundControl = tempcl->control;

		if (previousFound)
			return foundControl;
		else if (foundControl == inPreviousControl)
			previousFound = true;

		tempcl = tempcl->next;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::SetBackgroundImage(DGImage * inBackgroundImage)
{
// XXX implement for VST?
/*
	if (backgroundImage != NULL)
		backgroundImage->forget();

	backgroundImage = inBackgroundImage;
	if (backgroundImage != NULL)
	{
		backgroundImage->remember();
		// init the size of the plugin
		rect.right = rect.left + (short)(backgroundImage->getWidth());
		rect.bottom = rect.top + (short)(backgroundImage->getHeight());
	}

	if (frame != NULL)
		frame->setBackground(backgroundImage);
*/
}


#ifdef TARGET_API_AUDIOUNIT
//-----------------------------------------------------------------------------
OSStatus DfxGuiEditor::SendAUParameterEvent(AudioUnitParameterID inParameterID, AudioUnitEventType inEventType)
{
	// we're not actually prepared to do anything at this point if we don't yet know which AU we are controlling
	if (dfxgui_GetEffectInstance() == NULL)
		return kAudioUnitErr_Uninitialized;

	OSStatus result = noErr;
	AudioUnitParameter auParam = dfxgui_MakeAudioUnitParameter(inParameterID);

#if defined(AU_DO_OLD_STYLE_PARAMETER_CHANGE_GESTURES) && !__LP64__
	// still do the old way (which additionally calls the new way), until it's enough obsolete
	if (GetOwnerAUCarbonView() != NULL)
	{
		AudioUnitCarbonViewEventID carbonViewEventType = -1;
		if (inEventType == kAudioUnitEvent_BeginParameterChangeGesture)
			carbonViewEventType = kAudioUnitCarbonViewEvent_MouseDownInControl;
		else if (inEventType == kAudioUnitEvent_EndParameterChangeGesture)
			carbonViewEventType = kAudioUnitCarbonViewEvent_MouseUpInControl;
		if (carbonViewEventType >= 0)
		{
			CAAUParameter caauParam(auParam);
			GetOwnerAUCarbonView()->TellListener(caauParam, carbonViewEventType, NULL);
			return noErr;
		}
	}
#endif

	// do the current way, if it's available on the user's system
	AudioUnitEvent paramEvent = {0};
	paramEvent.mEventType = inEventType;
	paramEvent.mArgument.mParameter = auParam;
	if (AUEventListenerNotify != NULL)
		result = AUEventListenerNotify(getAUEventListener(), NULL, &paramEvent);

	return result;
}
#endif

//-----------------------------------------------------------------------------
void DfxGuiEditor::automationgesture_begin(long inParameterID)
{
#ifdef TARGET_API_AUDIOUNIT
	SendAUParameterEvent((AudioUnitParameterID)inParameterID, kAudioUnitEvent_BeginParameterChangeGesture);
#endif

#ifdef TARGET_API_VST
	beginEdit(inParameterID);
#endif

#ifdef TARGET_API_RTAS
	// called by GUI when mouse down event has occured; NO_UI: Call process' TouchControl()
	// This and endEdit are necessary for Touch Automation to work properly
	if (m_Process != NULL)
		m_Process->ProcessTouchControl( DFX_ParameterID_ToRTAS(inParameterID) );
#endif
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::automationgesture_end(long inParameterID)
{
#ifdef TARGET_API_AUDIOUNIT
	SendAUParameterEvent((AudioUnitParameterID)inParameterID, kAudioUnitEvent_EndParameterChangeGesture);
#endif

#ifdef TARGET_API_VST
	endEdit(inParameterID);
#endif

#ifdef TARGET_API_RTAS
	// called by GUI when mouse up event has occured; NO_UI: Call process' ReleaseControl()
	if (m_Process != NULL)
		m_Process->ProcessReleaseControl( DFX_ParameterID_ToRTAS(inParameterID) );
#endif
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::randomizeparameter(long inParameterID, bool inWriteAutomation)
{
#ifdef TARGET_API_AUDIOUNIT
	if (inWriteAutomation)
		automationgesture_begin(inParameterID);

	Boolean writeAutomation_fixedSize = inWriteAutomation;
	dfxgui_SetProperty(kDfxPluginProperty_RandomizeParameter, kDfxScope_Global, inParameterID, 
						&writeAutomation_fixedSize, sizeof(writeAutomation_fixedSize));

	if (inWriteAutomation)
		automationgesture_end(inParameterID);
#else
#endif
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::randomizeparameters(bool inWriteAutomation)
{
#ifdef TARGET_API_AUDIOUNIT
	if (inWriteAutomation)
	{
		if (auParameterList != NULL)
		{
			for (UInt32 i=0; i < auParameterListSize; i++)
				automationgesture_begin(auParameterList[i]);
		}
	}

	Boolean writeAutomation_fixedSize = inWriteAutomation;
	dfxgui_SetProperty(kDfxPluginProperty_RandomizeParameter, kDfxScope_Global, kAUParameterListener_AnyParameter, 
						&writeAutomation_fixedSize, sizeof(writeAutomation_fixedSize));

	if (inWriteAutomation)
	{
		if (auParameterList != NULL)
		{
			for (UInt32 i=0; i < auParameterListSize; i++)
				automationgesture_end(auParameterList[i]);
		}
	}
#else
#endif
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::dfxgui_IsValidParamID(long inParameterID)
{
	if ( (inParameterID == DFX_PARAM_INVALID_ID) || (inParameterID < 0) )
		return false;
#ifdef TARGET_API_AUDIOUNIT
	if (inParameterID > auMaxParameterID)
		return false;
#else
	if ( inParameterID >= GetNumParameters() )
		return false;
#endif

	return true;
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
#ifdef TARGET_API_AUDIOUNIT
	DfxParameterValueRequest request;
	size_t dataSize = sizeof(request);
	request.inValueItem = kDfxParameterValueItem_current;
	request.inValueType = kDfxParamValueType_float;

	if (dfxgui_GetProperty(kDfxPluginProperty_ParameterValue, kDfxScope_Global, 
							inParameterID, &request, dataSize) 
							== noErr)
		return request.value.f;
	else
		return 0.0;
#else
	return dfxgui_GetEffectInstance()->getparameter_f(inParameterID);
#endif
}

//-----------------------------------------------------------------------------
long DfxGuiEditor::getparameter_i(long inParameterID)
{
#ifdef TARGET_API_AUDIOUNIT
	DfxParameterValueRequest request;
	size_t dataSize = sizeof(request);
	request.inValueItem = kDfxParameterValueItem_current;
	request.inValueType = kDfxParamValueType_int;

	if (dfxgui_GetProperty(kDfxPluginProperty_ParameterValue, kDfxScope_Global, 
							inParameterID, &request, dataSize) 
							== noErr)
		return request.value.i;
	else
		return 0;
#else
	return dfxgui_GetEffectInstance()->getparameter_i(inParameterID);
#endif
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::getparameter_b(long inParameterID)
{
#ifdef TARGET_API_AUDIOUNIT
	DfxParameterValueRequest request;
	size_t dataSize = sizeof(request);
	request.inValueItem = kDfxParameterValueItem_current;
	request.inValueType = kDfxParamValueType_boolean;

	if (dfxgui_GetProperty(kDfxPluginProperty_ParameterValue, kDfxScope_Global, 
							inParameterID, &request, dataSize) 
							== noErr)
		return request.value.b;
	else
		return false;
#else
	return dfxgui_GetEffectInstance()->getparameter_b(inParameterID);
#endif
}

//-----------------------------------------------------------------------------
double DfxGuiEditor::getparameter_gen(long inParameterIndex)
{
#ifdef TARGET_API_VST
	return getEffect()->getParameter(inParameterIndex);
#else
	double currentValue = getparameter_f(inParameterIndex);
	return dfxgui_ContractParameterValue(inParameterIndex, currentValue);
#endif
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::setparameter_f(long inParameterID, double inValue, bool inWrapWithAutomationGesture)
{
	if (inWrapWithAutomationGesture)
		automationgesture_begin(inParameterID);

#ifdef TARGET_API_AUDIOUNIT
	DfxParameterValueRequest request;
	request.inValueItem = kDfxParameterValueItem_current;
	request.inValueType = kDfxParamValueType_float;
	request.value.f = inValue;

	dfxgui_SetProperty(kDfxPluginProperty_ParameterValue, kDfxScope_Global, inParameterID, 
						&request, sizeof(request));
#else
#endif

	if (inWrapWithAutomationGesture)
		automationgesture_end(inParameterID);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::setparameter_i(long inParameterID, long inValue, bool inWrapWithAutomationGesture)
{
	if (inWrapWithAutomationGesture)
		automationgesture_begin(inParameterID);

#ifdef TARGET_API_AUDIOUNIT
	DfxParameterValueRequest request;
	request.inValueItem = kDfxParameterValueItem_current;
	request.inValueType = kDfxParamValueType_int;
	request.value.i = inValue;

	dfxgui_SetProperty(kDfxPluginProperty_ParameterValue, kDfxScope_Global, 
						inParameterID, &request, sizeof(request));
#else
#endif

	if (inWrapWithAutomationGesture)
		automationgesture_end(inParameterID);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::setparameter_b(long inParameterID, bool inValue, bool inWrapWithAutomationGesture)
{
	if (inWrapWithAutomationGesture)
		automationgesture_begin(inParameterID);

#ifdef TARGET_API_AUDIOUNIT
	DfxParameterValueRequest request;
	request.inValueItem = kDfxParameterValueItem_current;
	request.inValueType = kDfxParamValueType_boolean;
	request.value.b = inValue;

	dfxgui_SetProperty(kDfxPluginProperty_ParameterValue, kDfxScope_Global, 
						inParameterID, &request, sizeof(request));
#else
#endif

	if (inWrapWithAutomationGesture)
		automationgesture_end(inParameterID);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::setparameter_default(long inParameterID, bool inWrapWithAutomationGesture)
{
#ifdef TARGET_API_AUDIOUNIT
	AudioUnitParameterInfo paramInfo = {0};
	long result = dfxgui_GetParameterInfo(inParameterID, paramInfo);
	if (result == noErr)
	{
		if (inWrapWithAutomationGesture)
			automationgesture_begin(inParameterID);

		AudioUnitParameter auParam = dfxgui_MakeAudioUnitParameter(inParameterID);
		result = AUParameterSet(NULL, NULL, &auParam, paramInfo.defaultValue, 0);

		if (inWrapWithAutomationGesture)
			automationgesture_end(inParameterID);
	}
#else
#endif
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::setparameters_default(bool inWrapWithAutomationGesture)
{
#ifdef TARGET_API_AUDIOUNIT
	if (auParameterList != NULL)
	{
		for (UInt32 i=0; i < auParameterListSize; i++)
			setparameter_default(auParameterList[i], inWrapWithAutomationGesture);
	}
#else
#endif
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::getparametervaluestring(long inParameterID, char * outText)
{
	if (outText == NULL)
		return false;

	const int64_t stringIndex = getparameter_i(inParameterID);

#ifdef TARGET_API_AUDIOUNIT
	DfxParameterValueStringRequest request;
	size_t dataSize = sizeof(request);
	request.inStringIndex = stringIndex;
	if (dfxgui_GetProperty(kDfxPluginProperty_ParameterValueString, kDfxScope_Global, 
							inParameterID, &request, dataSize) 
							== noErr)
	{
		strcpy(outText, request.valueString);
		return true;
	}

#else
	return dfxgui_GetEffectInstance()->getparametervaluestring(inParameterID, stringIndex, outText);
#endif

	return false;
}

//-----------------------------------------------------------------------------
char * DfxGuiEditor::getparameterunitstring(long inParameterIndex)
{
	char * outputString = NULL;

#ifdef TARGET_API_AUDIOUNIT
	char unitLabel[DFX_PARAM_MAX_UNIT_STRING_LENGTH];
	unitLabel[0] = 0;
	size_t dataSize = sizeof(unitLabel);
	long result = dfxgui_GetProperty(kDfxPluginProperty_ParameterUnitLabel, kDfxScope_Global, 
										inParameterIndex, unitLabel, dataSize);
	if ( (result == noErr) && (strlen(unitLabel) > 0) )
	{
		outputString = (char*) malloc(DFX_PARAM_MAX_UNIT_STRING_LENGTH);
		strcpy(outputString, unitLabel);
	}

#else
	outputString = (char*) calloc(DFX_PARAM_MAX_UNIT_STRING_LENGTH, 1);
	if (outputString != NULL)
		dfxgui_GetEffectInstance()->getparameterunitstring(inParameterIndex, outputString);
#endif

	return outputString;
}

//-----------------------------------------------------------------------------
char * DfxGuiEditor::getparametername(long inParameterID)
{
	char * outputString = NULL;

#ifdef TARGET_API_AUDIOUNIT
	AudioUnitParameterInfo auParamInfo = {0};
	long result = dfxgui_GetParameterInfo(inParameterID, auParamInfo);
	if (result == noErr)
	{
		if ( (auParamInfo.flags & kAudioUnitParameterFlag_HasCFNameString) && 
				(auParamInfo.cfNameString != NULL) )
		{
			outputString = DFX_CreateCStringFromCFString(auParamInfo.cfNameString);
			if (auParamInfo.flags & kAudioUnitParameterFlag_CFNameRelease)
				CFRelease(auParamInfo.cfNameString);
		}
		else
		{
			outputString = (char*) malloc(strlen(auParamInfo.name) + 1);
			if (outputString != NULL)
				strcpy(outputString, auParamInfo.name);
		}
	}

#else
	outputString = (char*) calloc(DFX_PARAM_MAX_NAME_LENGTH, 1);
	if (outputString != NULL)
		dfxgui_GetEffectInstance()->getparametername(inParameterID, outputString);
#endif

	return outputString;
}

//-----------------------------------------------------------------------------
float DfxGuiEditor::dfxgui_ExpandParameterValue(long inParameterIndex, float inValue)
{
#ifdef TARGET_API_AUDIOUNIT
	DfxParameterValueConversionRequest request = {0};
	request.inConversionType = kDfxParameterValueConversion_expand;
	request.inValue = inValue;
	size_t dataSize = sizeof(request);
	if (dfxgui_GetProperty(kDfxPluginProperty_ParameterValueConversion, kDfxScope_Global, 
							inParameterIndex, &request, dataSize) 
							== noErr)
	{
		return request.outValue;
	}
	else
	{
		AudioUnitParameter auParam = dfxgui_MakeAudioUnitParameter(inParameterIndex);
		return AUParameterValueFromLinear(inValue, &auParam);
	}
#else
	return dfxgui_GetEffectInstance()->expandparametervalue(inParameterIndex, inValue);
#endif
}

//-----------------------------------------------------------------------------
float DfxGuiEditor::dfxgui_ContractParameterValue(long inParameterIndex, float inValue)
{
#ifdef TARGET_API_AUDIOUNIT
	DfxParameterValueConversionRequest request = {0};
	request.inConversionType = kDfxParameterValueConversion_contract;
	request.inValue = inValue;
	size_t dataSize = sizeof(request);
	if (dfxgui_GetProperty(kDfxPluginProperty_ParameterValueConversion, kDfxScope_Global, 
							inParameterIndex, &request, dataSize) 
							== noErr)
	{
		return request.outValue;
	}
	else
	{
		AudioUnitParameter auParam = dfxgui_MakeAudioUnitParameter(inParameterIndex);
		return AUParameterValueToLinear(inValue, &auParam);
	}
#else
	return dfxgui_GetEffectInstance()->contractparametervalue(inParameterIndex, inValue);
#endif
}

//-----------------------------------------------------------------------------
float DfxGuiEditor::GetParameter_minValue(long inParameterIndex)
{
#ifdef TARGET_API_AUDIOUNIT
	AudioUnitParameterInfo paramInfo = {0};
	long result = dfxgui_GetParameterInfo(inParameterIndex, paramInfo);
	if (result == noErr)
		return paramInfo.minValue;
#else
	return dfxgui_GetEffectInstance()->getparametermin_f(inParameterIndex);
#endif
	return 0.0f;
}

//-----------------------------------------------------------------------------
float DfxGuiEditor::GetParameter_maxValue(long inParameterIndex)
{
#ifdef TARGET_API_AUDIOUNIT
	AudioUnitParameterInfo paramInfo = {0};
	long result = dfxgui_GetParameterInfo(inParameterIndex, paramInfo);
	if (result == noErr)
		return paramInfo.maxValue;
#else
	return dfxgui_GetEffectInstance()->getparametermax_f(inParameterIndex);
#endif
	return 0.0f;
}

//-----------------------------------------------------------------------------
float DfxGuiEditor::GetParameter_defaultValue(long inParameterIndex)
{
#ifdef TARGET_API_AUDIOUNIT
	AudioUnitParameterInfo paramInfo = {0};
	long result = dfxgui_GetParameterInfo(inParameterIndex, paramInfo);
	if (result == noErr)
		return paramInfo.defaultValue;
#else
	return dfxgui_GetEffectInstance()->getparameterdefault_f(inParameterIndex);
#endif
	return 0.0f;
}

//-----------------------------------------------------------------------------
DfxParamValueType DfxGuiEditor::GetParameterValueType(long inParameterIndex)
{
#ifdef TARGET_API_AUDIOUNIT
	DfxParamValueType valueType = 0;
	size_t dataSize = sizeof(valueType);
	long result = dfxgui_GetProperty(kDfxPluginProperty_ParameterValueType, kDfxScope_Global, 
										inParameterIndex, &valueType, dataSize);
	if (result == noErr)
		return valueType;
#else
	return dfxgui_GetEffectInstance()->getparametervaluetype(inParameterIndex);
#endif
	return kDfxParamValueType_float;
}

//-----------------------------------------------------------------------------
DfxParamUnit DfxGuiEditor::GetParameterUnit(long inParameterIndex)
{
#ifdef TARGET_API_AUDIOUNIT
	DfxParamUnit unitType = 0;
	size_t dataSize = sizeof(unitType);
	long result = dfxgui_GetProperty(kDfxPluginProperty_ParameterUnit, kDfxScope_Global, 
										inParameterIndex, &unitType, dataSize);
	if (result == noErr)
		return unitType;
#else
	return dfxgui_GetEffectInstance()->getparameterunit(inParameterIndex);
#endif
	return kDfxParamUnit_generic;
}

#ifdef TARGET_API_AUDIOUNIT
//-----------------------------------------------------------------------------
long DfxGuiEditor::dfxgui_GetParameterInfo(AudioUnitParameterID inParameterID, AudioUnitParameterInfo & outParameterInfo)
{
	size_t dataSize = sizeof(outParameterInfo);

	long result = dfxgui_GetProperty(kAudioUnitProperty_ParameterInfo, kAudioUnitScope_Global, 
										inParameterID, &outParameterInfo, dataSize);
	return result;
}
#endif

//-----------------------------------------------------------------------------
long DfxGuiEditor::GetNumParameters()
{
#ifdef TARGET_API_VST
	return getEffect()->getAeffect()->numParams;
#endif
#ifdef TARGET_API_RTAS
	return m_Process->getnumparameters();
#endif
#ifdef TARGET_API_AUDIOUNIT
	// XXX questionable implementation; return max ID value +1 instead?
	size_t dataSize = 0;
	DfxPropertyFlags propFlags = 0;
	long result = dfxgui_GetPropertyInfo(kAudioUnitProperty_ParameterList, kDfxScope_Global, 0, dataSize, propFlags);
	if (result == noErr)
		return (dataSize / sizeof(AudioUnitParameterID));
#endif
	return 0;
}

#ifdef TARGET_API_AUDIOUNIT
//-----------------------------------------------------------------------------
AudioUnitParameter DfxGuiEditor::dfxgui_MakeAudioUnitParameter(AudioUnitParameterID inParameterID, AudioUnitScope inScope, AudioUnitElement inElement)
{
	AudioUnitParameter outParam = {0};
	outParam.mAudioUnit = dfxgui_GetEffectInstance();
	outParam.mParameterID = inParameterID;
	outParam.mScope = inScope;
	outParam.mElement = inElement;
	return outParam;
}

//-----------------------------------------------------------------------------
AudioUnitParameterID * DfxGuiEditor::CreateParameterList(AudioUnitScope inScope, UInt32 * outNumParameters)
{
	UInt32 numParameters = 0;
	size_t dataSize = 0;
	DfxPropertyFlags propFlags = 0;
	long result = dfxgui_GetPropertyInfo(kAudioUnitProperty_ParameterList, inScope, 0, dataSize, propFlags);
	if (result == noErr)
		numParameters = dataSize / sizeof(AudioUnitParameterID);

	if (numParameters == 0)
		return NULL;

	AudioUnitParameterID * parameterList = (AudioUnitParameterID*) malloc(dataSize);
	if (parameterList == NULL)
		return NULL;

	result = dfxgui_GetProperty(kAudioUnitProperty_ParameterList, inScope, 0, parameterList, dataSize);
	if (result == noErr)
	{
		if (outNumParameters != NULL)
			*outNumParameters = numParameters;
		auMaxParameterID = 0;
		for (UInt32 i=0; i < numParameters; i++)
		{
			if (parameterList[i] > auMaxParameterID)
				auMaxParameterID = parameterList[i];
		}
		return parameterList;
	}
	else
	{
		free(parameterList);
		return NULL;
	}
}
#endif

//-----------------------------------------------------------------------------
long DfxGuiEditor::dfxgui_GetPropertyInfo(DfxPropertyID inPropertyID, DfxScope inScope, unsigned long inItemIndex, 
											size_t & outDataSize, DfxPropertyFlags & outFlags)
{
#ifdef TARGET_API_AUDIOUNIT
	if (dfxgui_GetEffectInstance() == NULL)
		return kAudioUnitErr_Uninitialized;

	UInt32 auDataSize = 0;
	Boolean writable = false;
	OSStatus status = AudioUnitGetPropertyInfo(dfxgui_GetEffectInstance(), inPropertyID, inScope, inItemIndex, &auDataSize, &writable);
	if (status == noErr)
	{
		outDataSize = auDataSize;
		outFlags = kDfxPropertyFlag_Readable;	// XXX okay to just assume here?
		if (writable)
			outFlags |= kDfxPropertyFlag_Writable;
	}
	return status;
#else
	return dfxgui_GetEffectInstance()->dfx_GetPropertyInfo(inPropertyID, inScope, inItemIndex, outDataSize, outFlags);
#endif
}

//-----------------------------------------------------------------------------
long DfxGuiEditor::dfxgui_GetProperty(DfxPropertyID inPropertyID, DfxScope inScope, unsigned long inItemIndex, 
										void * outData, size_t & ioDataSize)
{
#ifdef TARGET_API_AUDIOUNIT
	if (dfxgui_GetEffectInstance() == NULL)
		return kAudioUnitErr_Uninitialized;

	UInt32 auDataSize = ioDataSize;
	OSStatus status = AudioUnitGetProperty(dfxgui_GetEffectInstance(), inPropertyID, inScope, inItemIndex, outData, &auDataSize);
	if (status == noErr)
		ioDataSize = auDataSize;
	return status;
#else
	return dfxgui_GetEffectInstance()->dfx_GetProperty(inPropertyID, inScope, inItemIndex, outData);
#endif
}

//-----------------------------------------------------------------------------
long DfxGuiEditor::dfxgui_SetProperty(DfxPropertyID inPropertyID, DfxScope inScope, unsigned long inItemIndex, 
										const void * inData, size_t inDataSize)
{
#ifdef TARGET_API_AUDIOUNIT
	if (dfxgui_GetEffectInstance() == NULL)
		return kAudioUnitErr_Uninitialized;

	return AudioUnitSetProperty(dfxgui_GetEffectInstance(), inPropertyID, inScope, inItemIndex, inData, inDataSize);
#else
	return dfxgui_GetEffectInstance()->dfx_SetProperty(inPropertyID, inScope, inItemIndex, inData, inDataSize);
#endif
}

//-----------------------------------------------------------------------------
DGEditorListenerInstance DfxGuiEditor::dfxgui_GetEffectInstance()
{
#ifdef TARGET_API_VST
	return static_cast<DfxPlugin*>( getEffect() );
#endif
#ifdef TARGET_API_RTAS
	return m_Process;
#endif
#ifdef TARGET_API_AUDIOUNIT
	return static_cast<AudioUnit>( getEffect() );
#endif
}

#if TARGET_PLUGIN_USES_MIDI
//-----------------------------------------------------------------------------
void DfxGuiEditor::setmidilearning(bool inNewLearnMode)
{
	Boolean newLearnMode_fixedSize = inNewLearnMode;
	dfxgui_SetProperty(kDfxPluginProperty_MidiLearn, kDfxScope_Global, 0, 
						&newLearnMode_fixedSize, sizeof(newLearnMode_fixedSize));
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::getmidilearning()
{
	Boolean learnMode;
	size_t dataSize = sizeof(learnMode);
	if (dfxgui_GetProperty(kDfxPluginProperty_MidiLearn, kDfxScope_Global, 0, &learnMode, dataSize) == noErr)
		return learnMode;
	else
		return false;
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::resetmidilearn()
{
	Boolean nud;	// irrelevant
	dfxgui_SetProperty(kDfxPluginProperty_ResetMidiLearn, kDfxScope_Global, 0, &nud, sizeof(nud));
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::setmidilearner(long inParameterIndex)
{
	int32_t parameterIndex_fixedSize = inParameterIndex;
	dfxgui_SetProperty(kDfxPluginProperty_MidiLearner, kDfxScope_Global, 0, 
						&parameterIndex_fixedSize, sizeof(parameterIndex_fixedSize));
}

//-----------------------------------------------------------------------------
long DfxGuiEditor::getmidilearner()
{
	int32_t learner;
	size_t dataSize = sizeof(learner);
	if (dfxgui_GetProperty(kDfxPluginProperty_MidiLearner, kDfxScope_Global, 0, &learner, dataSize) == noErr)
		return learner;
	else
		return DFX_PARAM_INVALID_ID;
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::ismidilearner(long inParameterIndex)
{
	return (getmidilearner() == inParameterIndex);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::setparametermidiassignment(long inParameterIndex, DfxParameterAssignment inAssignment)
{
	dfxgui_SetProperty(kDfxPluginProperty_ParameterMidiAssignment, kDfxScope_Global, 
						inParameterIndex, &inAssignment, sizeof(inAssignment));
}

//-----------------------------------------------------------------------------
DfxParameterAssignment DfxGuiEditor::getparametermidiassignment(long inParameterIndex)
{
	DfxParameterAssignment paramAssignment = {0};
	size_t dataSize = sizeof(paramAssignment);
	if (dfxgui_GetProperty(kDfxPluginProperty_ParameterMidiAssignment, kDfxScope_Global, 
							inParameterIndex, &paramAssignment, dataSize) 
							== noErr)
		return paramAssignment;
	else
	{
		paramAssignment.eventType = kParamEventNone;
		return paramAssignment;
	}
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::parametermidiunassign(long inParameterIndex)
{
	DfxParameterAssignment paramAssignment = {0};
	paramAssignment.eventType = kParamEventNone;
	setparametermidiassignment(inParameterIndex, paramAssignment);
}

#endif
// TARGET_PLUGIN_USES_MIDI

//-----------------------------------------------------------------------------
unsigned long DfxGuiEditor::getNumAudioChannels()
{
#ifdef TARGET_API_AUDIOUNIT
	CAStreamBasicDescription streamDesc;
	size_t dataSize = sizeof(streamDesc);
	long result = dfxgui_GetProperty(kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, 
										&streamDesc, dataSize);
	if (result == noErr)
		return streamDesc.NumberChannels();
	else
		return 0;
#endif
#ifdef TARGET_API_VST
	return (unsigned long) (getEffect()->getAeffect()->numOutputs);
#endif
#ifdef TARGET_API_RTAS
	return (unsigned long) (m_Process->getnumoutputs());
#endif
}

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

	return kDfxErr_NoError;	// XXX implement
}

//-----------------------------------------------------------------------------
long DfxGuiEditor::copySettings()
{
	long status = initClipboard();
	if (status != kDfxErr_NoError)
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
	size_t dataSize = sizeof(auSettingsPropertyList);
	status = dfxgui_GetProperty(kAudioUnitProperty_ClassInfo, kAudioUnitScope_Global, 0, 
								&auSettingsPropertyList, dataSize);
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
#endif

	return status;
}

//-----------------------------------------------------------------------------
long DfxGuiEditor::pasteSettings(bool * inQueryPastabilityOnly)
{
	if (inQueryPastabilityOnly != NULL)
		*inQueryPastabilityOnly = false;

	long status = initClipboard();
	if (status != kDfxErr_NoError)
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
		if ((unsigned long)itemID != PLUGIN_ID)	// XXX hacky?
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
#ifdef TARGET_API_AUDIOUNIT
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
							status = dfxgui_SetProperty(kAudioUnitProperty_ClassInfo, kAudioUnitScope_Global, 0, 
														&auSettingsPropertyList, sizeof(auSettingsPropertyList));
							CFRelease(auSettingsPropertyList);
							if (status == noErr)
							{
								pastableItemFound = true;
								AUParameterChange_TellListeners(dfxgui_GetEffectInstance(), kAUParameterListener_AnyParameter);
							}
						}
						CFRelease(flavorData);
					}
				}
			}
#endif	// TARGET_API_AUDIOUNIT
			if (pastableItemFound)
				break;
		}
		CFRelease(flavorTypesArray);
		if (pastableItemFound)
			break;
	}
#endif

	return status;
}



#if TARGET_OS_MAC && !__LP64__
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

#if TARGET_OS_MAC && TARGET_API_AUDIOUNIT && 0
//-----------------------------------------------------------------------------
bool DfxGuiEditor::HandleMouseEvent(EventRef inEvent)
{
	UInt32 eventKind = GetEventKind(inEvent);
	OSStatus status;

	DGKeyModifiers keyModifiers = DFX_GetDGKeyModifiersForEvent(inEvent);



	if (eventKind == kEventMouseWheelMoved)
	{
		SInt32 mouseWheelDelta = 0;
		status = GetEventParameter(inEvent, kEventParamMouseWheelDelta, typeSInt32, NULL, sizeof(mouseWheelDelta), NULL, &mouseWheelDelta);
		if (status != noErr)
			return false;

		EventMouseWheelAxis macMouseWheelAxis = 0;
		DGAxis dgMouseWheelAxis = kDGAxis_vertical;
		status = GetEventParameter(inEvent, kEventParamMouseWheelAxis, typeMouseWheelAxis, NULL, sizeof(macMouseWheelAxis), NULL, &macMouseWheelAxis);
		if (status == noErr)
		{
			switch (macMouseWheelAxis)
			{
				case kEventMouseWheelAxisX:
					dgMouseWheelAxis = kDGAxis_horizontal;
					break;
				case kEventMouseWheelAxisY:
				default:
					dgMouseWheelAxis = kDGAxis_vertical;
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


	if ( (eventKind == kEventMouseEntered) || (eventKind == kEventMouseExited) )
	{
		MouseTrackingRef trackingRegion = NULL;
		status = GetEventParameter(inEvent, kEventParamMouseTrackingRef, typeMouseTrackingRef, NULL, sizeof(trackingRegion), NULL, &trackingRegion);
		if ( (status == noErr) && (trackingRegion != NULL) )
		{
			MouseTrackingRegionID trackingRegionID;
			status = GetMouseTrackingRegionID(trackingRegion, &trackingRegionID);	// XXX deprecated in Mac OS X 10.4
			if ( (status == noErr) && (trackingRegionID.signature == PLUGIN_CREATOR_ID) )
			{
				DGControl * ourMousedOverControl = NULL;
				status = GetMouseTrackingRegionRefCon(trackingRegion, (void**)(&ourMousedOverControl));	// XXX deprecated in Mac OS X 10.4
				if ( (status == noErr) && (ourMousedOverControl != NULL) )
				{
					if (eventKind == kEventMouseEntered)
						addMousedOverControl(ourMousedOverControl);
					else if (eventKind == kEventMouseExited)
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


	if (eventKind == kEventMouseDragged)
	{
		UInt32 mouseButtons = 1;	// bit 0 is mouse button 1, bit 1 is button 2, etc.
		status = GetEventParameter(inEvent, kEventParamMouseChord, typeUInt32, NULL, sizeof(mouseButtons), NULL, &mouseButtons);
		if (status != noErr)
			mouseButtons = GetCurrentEventButtonState();

		ourControl->do_mouseTrack(mouseLocation.x, mouseLocation.y, mouseButtons, keyModifiers);

		return false;	// let it fall through in case the host needs the event
	}

	if (eventKind == kEventMouseUp)
	{
		ourControl->do_mouseUp(mouseLocation.x, mouseLocation.y, keyModifiers);

		currentControl_clicked = NULL;

		return false;	// let it fall through in case the host needs the event
	}

	return false;
}
#endif
// TARGET_OS_MAC

#if TARGET_OS_MAC && TARGET_API_AUDIOUNIT && 0
//-----------------------------------------------------------------------------
bool DfxGuiEditor::HandleKeyboardEvent(EventRef inEvent)
{
	UInt32 eventKind = GetEventKind(inEvent);
	OSStatus status;

	if ( (eventKind == kEventRawKeyDown) || (eventKind == kEventRawKeyRepeat) )
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

#if TARGET_OS_MAC && TARGET_API_AUDIOUNIT && 0
//-----------------------------------------------------------------------------
bool DfxGuiEditor::HandleCommandEvent(EventRef inEvent)
{
	UInt32 eventKind = GetEventKind(inEvent);

	if (eventKind == kEventCommandProcess)
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


#if TARGET_OS_MAC && TARGET_API_AUDIOUNIT && 0
//-----------------------------------------------------------------------------
bool DfxGuiEditor::HandleControlEvent(EventRef inEvent)
{
	UInt32 eventKind = GetEventKind(inEvent);
	OSStatus status;

	ControlRef ourCarbonControl = NULL;
	status = GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, NULL, sizeof(ourCarbonControl), NULL, &ourCarbonControl);
	DGControl * ourDGControl = NULL;
	if ( (status == noErr) && (ourCarbonControl != NULL) )
		ourDGControl = getDGControlByCarbonControlRef(ourCarbonControl);

/*
	// the Carbon control reference has not been added yet, so our DGControl pointer is NULL, because we can't look it up by ControlRef yet
	if (eventKind == kEventControlInitialize)
	{
		UInt32 controlFeatures = kControlHandlesTracking | kControlSupportsDataAccess | kControlSupportsGetRegion;
		if (ourCarbonControl != NULL)
			status = GetControlFeatures(ourCarbonControl, &controlFeatures);
		status = SetEventParameter(inEvent, kEventParamControlFeatures, typeUInt32, sizeof(controlFeatures), &controlFeatures);
		return true;
	}
*/

	if ( (eventKind == kEventControlTrackingAreaEntered) || (eventKind == kEventControlTrackingAreaExited) )
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
				if (eventKind == kEventControlTrackingAreaEntered)
					addMousedOverControl(ourMousedOverControl);
				else if (eventKind == kEventControlTrackingAreaExited)
					removeMousedOverControl(ourMousedOverControl);
				return true;
			}
		}
		return false;
	}


	if (ourDGControl != NULL)
	{
		switch (eventKind)
		{
			case kEventControlDraw:
//fprintf(stderr, "kEventControlDraw\n");
				{
					CGrafPtr windowPort = NULL;
					DGGraphicsContext * context = DGInitControlDrawingContext(inEvent, windowPort);
					if (context == NULL)
						return false;

					ourDGControl->draw(context);

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
//if (eventKind == kEventControlClick) fprintf(stderr, "kEventControlClick\n");
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
					if ( (eventKind == kEventControlContextualMenuClick) || (eventKind == kEventControlTrack) )
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
					if (eventKind == kEventControlClick)
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

#ifdef TARGET_API_AUDIOUNIT
//-----------------------------------------------------------------------------
static void DFXGUI_AudioUnitEventListenerProc(void * inCallbackRefCon, void * inObject, const AudioUnitEvent * inEvent, UInt64 inEventHostTime, Float32 inParameterValue)
{
	DfxGuiEditor * ourOwnerEditor = (DfxGuiEditor*) inCallbackRefCon;
	if ( (ourOwnerEditor != NULL) && (inEvent != NULL) )
	{
		if (inEvent->mEventType == kAudioUnitEvent_ParameterValueChange)
		{
			AudioUnitParameterID paramID = inEvent->mArgument.mParameter.mParameterID;
			float paramCurrentValue_norm = ourOwnerEditor->getparameter_gen(paramID);
			ourOwnerEditor->setParameter(paramID, paramCurrentValue_norm);
		}

		if (inEvent->mEventType == kAudioUnitEvent_PropertyChange)
		{
			switch (inEvent->mArgument.mProperty.mPropertyID)
			{
				case kAudioUnitProperty_StreamFormat:
					ourOwnerEditor->HandleStreamFormatChange();
					break;
				case kAudioUnitProperty_ParameterList:
					ourOwnerEditor->HandleParameterListChange();
					break;
			#if TARGET_PLUGIN_USES_MIDI
				case kDfxPluginProperty_MidiLearn:
					ourOwnerEditor->HandleMidiLearnChange();
					break;
			#endif
				default:
					ourOwnerEditor->HandleAUPropertyChange(inObject, inEvent->mArgument.mProperty, inEventHostTime);
					break;
			}
		}
	}
}
#endif

#ifdef TARGET_API_AUDIOUNIT
//-----------------------------------------------------------------------------
void DfxGuiEditor::HandleStreamFormatChange()
{
	unsigned long oldNumAudioChannels = numAudioChannels;
	numAudioChannels = getNumAudioChannels();
	if (numAudioChannels != oldNumAudioChannels)
		numAudioChannelsChanged(numAudioChannels);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::HandleParameterListChange()
{
	if (auParameterList != NULL)
	{
		for (UInt32 i=0; i < auParameterListSize; i++)
		{
			AudioUnitParameter auParam = dfxgui_MakeAudioUnitParameter(auParameterList[i]);
			AUListenerRemoveParameter(auEventListener, this, &auParam);
		}
		free(auParameterList);
	}

	auParameterList = CreateParameterList(kAudioUnitScope_Global, &auParameterListSize);
	if ( (auParameterList != NULL) && (auEventListener != NULL) )
	{
		for (UInt32 i=0; i < auParameterListSize; i++)
		{
			AudioUnitParameter auParam = dfxgui_MakeAudioUnitParameter(auParameterList[i]);
			AUListenerAddParameter(auEventListener, this, &auParam);
		}
	}
}
#endif


#if TARGET_PLUGIN_USES_MIDI

#ifdef TARGET_API_AUDIOUNIT
//-----------------------------------------------------------------------------
void DfxGuiEditor::HandleMidiLearnChange()
{
	if (midiLearnButton != NULL)
	{
		long newControlValue = getmidilearning() ? 1 : 0;
		midiLearnButton->setValue_i(newControlValue);
	}
}
#endif

//-----------------------------------------------------------------------------
static void DFXGUI_MidiLearnButtonUserProcedure(long inValue, void * inUserData)
{
	if (inUserData != NULL)
	{
		if (inValue == 0)
			((DfxGuiEditor*)inUserData)->setmidilearning(false);
		else
			((DfxGuiEditor*)inUserData)->setmidilearning(true);
	}
}

//-----------------------------------------------------------------------------
static void DFXGUI_MidiResetButtonUserProcedure(long inValue, void * inUserData)
{
	if ( (inUserData != NULL) && (inValue != 0) )
		((DfxGuiEditor*)inUserData)->resetmidilearn();
}

//-----------------------------------------------------------------------------
DGButton * DfxGuiEditor::CreateMidiLearnButton(long inXpos, long inYpos, DGImage * inImage, bool inDrawMomentaryState)
{
	const long numButtonStates = 2;
	long controlWidth = inImage->getWidth();
	if (inDrawMomentaryState)
		controlWidth /= 2;
	const long controlHeight = inImage->getHeight() / numButtonStates;

	DGRect pos(inXpos, inYpos, controlWidth, controlHeight);
	midiLearnButton = new DGButton(this, &pos, inImage, numButtonStates, kDGButtonType_incbutton, inDrawMomentaryState);
	midiLearnButton->setUserProcedure(DFXGUI_MidiLearnButtonUserProcedure, this);
	return midiLearnButton;
}

//-----------------------------------------------------------------------------
DGButton * DfxGuiEditor::CreateMidiResetButton(long inXpos, long inYpos, DGImage * inImage)
{
	DGRect pos(inXpos, inYpos, inImage->getWidth(), inImage->getHeight()/2);
	midiResetButton = new DGButton(this, &pos, inImage, 2, kDGButtonType_pushbutton);
	midiResetButton->setUserProcedure(DFXGUI_MidiResetButtonUserProcedure, this);
	return midiResetButton;
}

#endif
// TARGET_PLUGIN_USES_MIDI


#if TARGET_OS_MAC && !__LP64__
//-----------------------------------------------------------------------------
OSStatus DFX_OpenWindowFromNib(CFStringRef inWindowName, WindowRef * outWindow)
{
	if ( (inWindowName == NULL) || (outWindow == NULL) )
		return paramErr;

	// open the window from our nib
	CFBundleRef pluginBundle = CFBundleGetBundleWithIdentifier( CFSTR(PLUGIN_BUNDLE_IDENTIFIER) );
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

#if TARGET_OS_MAC && !__LP64__
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
	OSStatus status = GetControlByID(inWindow, &controlID, &resultControl);
	if (status != noErr)
		return NULL;
	return resultControl;
}
#endif

#if TARGET_OS_MAC && !__LP64__
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

	// initialize the control according to the current input string
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






#pragma mark -

#ifdef TARGET_API_RTAS

//-----------------------------------------------------------------------------
// fills inRect with dimensions of background frame; NO_UI: called (indirectly) by Pro Tools
void DfxGuiEditor::GetBackgroundRect(sRect * inRect)
{
	inRect->top = rect.top;
	inRect->left = rect.left;
	inRect->bottom = rect.bottom;
	inRect->right = rect.right;
}

//-----------------------------------------------------------------------------
// sets dimensions of background frame; NO_UI: called by the plug-in process class when resizing the window
void DfxGuiEditor::SetBackgroundRect(sRect * inRect)
{
	rect.top = inRect->top;
	rect.left = inRect->left;
	rect.bottom = inRect->bottom;
	rect.right = inRect->right;
}

//-----------------------------------------------------------------------------
// Called by GUI when idle time available; NO_UI: Call process' DoIdle() method.  
// Keeps other processes from starving during certain events (like mouse down).
void DfxGuiEditor::doIdleStuff()
{
	TARGET_API_EDITOR_BASE_CLASS::doIdleStuff();	// XXX do this?

	if (m_Process != NULL)
		m_Process->ProcessDoIdle();
}

//-----------------------------------------------------------------------------
// Called by process to get the index of the control that contains the point
void DfxGuiEditor::GetControlIndexFromPoint(long inXpos, long inYpos, long * outControlIndex)
{
	if (outControlIndex == NULL)
		return;

	*outControlIndex = 0;
	CPoint point(inXpos, inYpos);
	DGControlsList * tempcl = controlsList;
	while (tempcl != NULL)
	{
		CRect controlBounds;
		tempcl->control->getViewSize(controlBounds);
		if ( point.isInside(controlBounds) )
		{
			*outControlIndex = DFX_ParameterID_ToRTAS( tempcl->control->getTag() );
			break;
		}
		tempcl = tempcl->next;
	}
}

//-----------------------------------------------------------------------------
// Called by process when a control's highlight has changed
void DfxGuiEditor::SetControlHighlight(long inControlIndex, short inIsHighlighted, short inColor)
{
	inControlIndex = DFX_ParameterID_FromRTAS(inControlIndex);
	if (!inIsHighlighted)
		inColor = eHighlight_None;

	if ( dfxgui_IsValidParamID(inControlIndex) )
	{
		if (parameterHighlightColors != NULL)
			parameterHighlightColors[inControlIndex] = inColor;
	}

	if ( IsOpen() )
		getFrame()->setDirty(true);
}

//-----------------------------------------------------------------------------
// Called by process when a control's highlight has changed
void DfxGuiEditor::drawControlHighlight(CDrawContext * inContext, CControl * inControl)
{
	if ( (inContext == NULL) || (inControl == NULL) )
		return;
	long parameterIndex = inControl->getTag();
	if ( !dfxgui_IsValidParamID(parameterIndex) )
		return;
	if (parameterHighlightColors == NULL)
		return;

	CColor highlightColor;
	switch (parameterHighlightColors[parameterIndex])
	{
		case eHighlight_Red:
			highlightColor = MakeCColor(255, 0, 0);
			break;
		case eHighlight_Blue:
			highlightColor = MakeCColor(0, 0, 170);
			break;
		case eHighlight_Green:
			highlightColor = MakeCColor(0, 221, 0);
			break;
		case eHighlight_Yellow:
			highlightColor = MakeCColor(255, 204, 0);
			break;
		default:
			return;
	}

	const CCoord highlightWidth = 1;
	CRect highlightRect;
	inControl->getViewSize(highlightRect);
	inContext->setFillColor(highlightColor);
	inContext->setFrameColor(highlightColor);
	inContext->setLineWidth(highlightWidth);
	inContext->drawRect(highlightRect);

	highlightColor.alpha = 90;
	inContext->setFillColor(highlightColor);
	inContext->setFrameColor(highlightColor);
	highlightRect.inset(highlightWidth, highlightWidth);
	inContext->drawRect(highlightRect);
}

#endif
// TARGET_API_RTAS






#ifdef TARGET_API_AUDIOUNIT

#if __LP64__
extern "C" void DGVstGuiAUViewEntry() {}	// XXX workaround to quiet missing exported symbal error
#else

#pragma mark -

//-----------------------------------------------------------------------------
class DGVstGuiAUView : public AUCarbonViewBase 
{
public:
	DGVstGuiAUView(AudioUnitCarbonView inInstance);
	virtual ~DGVstGuiAUView();
	
	virtual OSStatus CreateUI(Float32 inXOffset, Float32 inYOffset);
	virtual void RespondToEventTimer(EventLoopTimerRef inTimer);
	virtual OSStatus Version();

private:
	DfxGuiEditor * mDfxGuiEditor;
};

COMPONENT_ENTRY(DGVstGuiAUView)

//-----------------------------------------------------------------------------
DGVstGuiAUView::DGVstGuiAUView(AudioUnitCarbonView inInstance)
:	AUCarbonViewBase(inInstance, kDfxGui_NotificationInterval)
{
	mDfxGuiEditor = NULL;
}

//-----------------------------------------------------------------------------
DGVstGuiAUView::~DGVstGuiAUView()
{
	if (mDfxGuiEditor != NULL)
	{
		// the idle timer gets disposed in the parent class destructor, 
		// so make sure that the mDfxGuiEditor pointer is zeroed so that it 
		// can't be accessed asynchronously by the idle timer while being deleted
		DfxGuiEditor * editor_temp = mDfxGuiEditor;
		mDfxGuiEditor = NULL;

		editor_temp->close();
		delete editor_temp;
	}
}

//-----------------------------------------------------------------------------
void DGVstGuiAUView::RespondToEventTimer(EventLoopTimerRef inTimer)
{
	if (mDfxGuiEditor != NULL)
		mDfxGuiEditor->idle();
}

extern DfxGuiEditor * DFXGUI_NewEditorInstance(AudioUnit inEffectInstance);

//-----------------------------------------------------------------------------
OSStatus DGVstGuiAUView::CreateUI(Float32 inXOffset, Float32 inYOffset)
{
	if (GetEditAudioUnit() == NULL)
		return kAudioUnitErr_Uninitialized;

	mDfxGuiEditor = DFXGUI_NewEditorInstance( GetEditAudioUnit() );
	if (mDfxGuiEditor == NULL)
		return memFullErr;
	mDfxGuiEditor->SetOwnerAUCarbonView(this);
	bool success = mDfxGuiEditor->open( GetCarbonWindow() );
	if (!success)
		return mDfxGuiEditor->dfxgui_GetEditorOpenErrorCode();

#if (VSTGUI_VERSION_MAJOR < 4)
	HIViewRef editorHIView = (HIViewRef) (mDfxGuiEditor->getFrame()->getPlatformControl());
#else
	HIViewRef editorHIView = (HIViewRef) (mDfxGuiEditor->getFrame()->getPlatformFrame()->getPlatformRepresentation());
#endif
	HIViewMoveBy(editorHIView, inXOffset, inYOffset);
	EmbedControl(editorHIView);

	// set the size of the embedding pane
	CRect frameSize = mDfxGuiEditor->getFrame()->getViewSize(frameSize);
	SizeControl( GetCarbonPane(), frameSize.width(), frameSize.height() );

	CreateEventLoopTimer(kEventDurationMillisecond * 5.0, kDfxGui_IdleTimerInterval);

	HIViewSetVisible(editorHIView, true);
	HIViewSetNeedsDisplay(editorHIView, true);

	return noErr;
}

//-----------------------------------------------------------------------------
OSStatus DGVstGuiAUView::Version()
{
	return DFX_CompositePluginVersionNumberValue();
}

#endif	// !__LP64__

#endif	// TARGET_API_AUDIOUNIT
