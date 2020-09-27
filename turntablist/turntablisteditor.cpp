#include "turntablist.h"

#include "dfxgui.h"
#include "dfxguislider.h"
#include "dfxguibutton.h"
#include "dfxguidisplay.h"

#include "dfx-au-utilities.h"
#include <c.h>  // for sizeofA



const float kTurntablistFontSize = 10.0f;

//-----------------------------------------------------------------------------
enum {
	// positions
	kScratchAmountSliderX = 31,
	kPitchShiftSliderX = 166,
	kSliderY = 249,
	kSliderWidth = 22,
	kSliderHeight = 197,

	kFileNameX = 31,
	kFileNameY = 23 + 1,
	kFileNameWidth = 158,
	kFileNameHeight = 15,

	kDisplayX = 31,
	kDisplayY = 458,
	kDisplayWidth = 122,
	kDisplayHeight = 14,

	kKnobOffsetX = 1,
	kKnobOffsetY = 1,

	kColumn1 = 25,
	kColumn2 = 70,
	kColumn3 = 115,
	kColumn4 = 160,

	kRow1 = 64,
	kRow2 = 123,
	kRow3 = 182,
	kRow4 = 241,
	kRow5 = 300,
	kRow6 = 359,
	kRow7 = 418,

	kPowerButtonX = kColumn1,
	kPowerButtonY = kRow2,
	kNotePowerTrackButtonX = kColumn4,
	kNotePowerTrackButtonY = kRow2,
	kLoopButtonX = kColumn3,
	kLoopButtonY = kRow3,
	kDirectionButtonX = kColumn2,
	kDirectionButtonY = kRow4,
	kNoteModeButtonX = kColumn3,
	kNoteModeButtonY = kRow4,
	kKeyTrackButtonX = kColumn2,
	kKeyTrackButtonY = kRow5,
	kScratchModeButtonX = kColumn2,
	kScratchModeButtonY = kRow7,

	kLoadButtonX = kColumn4,
	kLoadButtonY = kRow1,
	kPlayButtonX = kColumn2,
	kPlayButtonY = kRow3,

	kMidiLearnX = kColumn2,
	kMidiLearnY = kRow6,
	kMidiResetX = kColumn3,
	kMidiResetY = kMidiLearnY,

	kSpinUpKnobX = kColumn2 + kKnobOffsetX,
	kSpinUpKnobY = kRow2 + kKnobOffsetY,
	kSpinDownKnobX = kColumn3 + kKnobOffsetX,
	kSpinDownKnobY = kRow2 + kKnobOffsetY,
	kPitchRangeKnobX = kColumn4 + kKnobOffsetX,
	kPitchRangeKnobY = kRow3 + kKnobOffsetY,
	kScratchSpeedKnobX = kColumn1 + kKnobOffsetX,
	kScratchSpeedKnobY = kRow3 + kKnobOffsetY,
	kRootKeyKnobX = kColumn3 + kKnobOffsetX,
	kRootKeyKnobY = kRow5 + kKnobOffsetY,

	kHelpX = kColumn3 + 4,
	kHelpY = 421,
	kHelpWidth = 25,
	kHelpHeight = 25,

	kAboutSplashX = 13,
	kAboutSplashY = 64,
	kAboutSplashWidth = 137,
	kAboutSplashHeight = 28,

	kKnobFrames = 61
};



#pragma mark -
#pragma mark class definitions
#pragma mark -

//-----------------------------------------------------------------------------
class TurntablistEditor : public DfxGuiEditor
{
public:
	TurntablistEditor(AudioUnitCarbonView inInstance);
	virtual ~TurntablistEditor();

	virtual long open();
	virtual void post_open();

	virtual void idle();
	virtual bool HandleEvent(EventHandlerCallRef inHandlerRef, EventRef inEvent);

	OSStatus HandleLoadButton();
	ComponentResult LoadAudioFile(const FSRef & inAudioFileRef);
	ComponentResult HandleAudioFileChange();
	void FileOpenDialogFinished();
	ComponentResult HandlePlayButton(bool inPlay);
	ComponentResult HandlePlayChange();
	void HandleMidiLearnButton(bool inLearn);
	void HandleMidiResetButton();
	void HandleParameterChange(long inParameterID, float inValue);

private:
	void SetFileNameDisplay(CFStringRef inDisplayText);

	DGStaticTextDisplay * audioFileNameDisplay;
	DGStaticTextDisplay * allParamsTextDisplay;
	DGButton * playButton;
	DGAnimation * scratchSpeedKnob;

	AUEventListenerRef propertyEventListener;
	AudioUnitEvent audioFilePropertyAUEvent;
	AudioUnitEvent playPropertyAUEvent;

	AUParameterListenerRef parameterListener;
	AudioUnitParameter allParamsAUP;

	NavDialogRef audioFileOpenDialog;

	OSStatus dragAudioFileError;
	FSRef dragAudioFileRef;
};



//-----------------------------------------------------------------------------
class TurntablistScratchSlider : public DGSlider
{
public:
	TurntablistScratchSlider(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion, 
				DfxGuiSliderAxis inOrientation, DGImage * inHandleImage)
	:   DGSlider(inOwnerEditor, inParamID, inRegion, inOrientation, inHandleImage, NULL)
	{
	}

	virtual void mouseUp(float inXpos, float inYpos, DGKeyModifiers inKeyModifiers)
	{
		DGSlider::mouseUp(inXpos, inYpos, inKeyModifiers);

		SInt32 min = GetControl32BitMinimum(carbonControl);
		SInt32 max = GetControl32BitMaximum(carbonControl);
		SetControl32BitValue(carbonControl, ((max - min) / 2) + min);

		((TurntablistEditor*)getDfxGuiEditor())->HandleParameterChange(getParameterID(), 0.0f);
	}
};



#pragma mark -
#pragma mark callbacks

//-----------------------------------------------------------------------------
void TurntablistLoadButtonProc(long inValue, void * inTurntablistEditor)
{
	if (inTurntablistEditor != NULL)
		((TurntablistEditor*)inTurntablistEditor)->HandleLoadButton();
}

//-----------------------------------------------------------------------------
void TurntablistPlayButtonProc(long inValue, void * inTurntablistEditor)
{
	if (inTurntablistEditor != NULL)
		((TurntablistEditor*)inTurntablistEditor)->HandlePlayButton( (inValue == 0) ? false : true );
}

//-----------------------------------------------------------------------------
void TurntablistMidiLearnButtonProc(long inValue, void * inTurntablistEditor)
{
	if (inTurntablistEditor != NULL)
		((TurntablistEditor*)inTurntablistEditor)->HandleMidiLearnButton( (inValue == 0) ? false : true );
}

//-----------------------------------------------------------------------------
void TurntablistMidiResetButtonProc(long inValue, void * inTurntablistEditor)
{
	if ( (inTurntablistEditor != NULL) && (inValue != 0) )
		((TurntablistEditor*)inTurntablistEditor)->HandleMidiResetButton();
}

//-----------------------------------------------------------------------------
void TurntablistHelpButtonProc(long inValue, void * inTurntablistEditor)
{
//	if (inValue > 0)
		launch_documentation();
}

//-----------------------------------------------------------------------------
void TurntablistAboutButtonProc(long inValue, void * inTurntablistEditor)
{
	if (inValue > 0)
	{
		launch_url(PLUGIN_HOMEPAGE_URL);
/*
		CFBundleRef pluginBundleRef = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));
		if ( (pluginBundleRef != NULL) && (HIAboutBox != NULL) )
		{
			CFMutableDictionaryRef aboutDict = CFDictionaryCreateMutable(kCFAllocatorDefault, 3, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
			if (aboutDict != NULL)
			{
				CFStringRef valueString = (CFStringRef) CFBundleGetValueForInfoDictionaryKey(pluginBundleRef, kCFBundleNameKey);
				if (valueString != NULL)
					CFDictionaryAddValue(aboutDict, (const void*)kHIAboutBoxNameKey, (const void*)valueString);

				valueString = (CFStringRef) CFBundleGetValueForInfoDictionaryKey(pluginBundleRef, CFSTR("CFBundleShortVersionString"));
				if (valueString == NULL)
					valueString = (CFStringRef) CFBundleGetValueForInfoDictionaryKey(pluginBundleRef, kCFBundleVersionKey);
				if (valueString != NULL)
					CFDictionaryAddValue(aboutDict, (const void*)kHIAboutBoxVersionKey, (const void*)valueString);

				valueString = (CFStringRef) CFBundleGetValueForInfoDictionaryKey(pluginBundleRef, CFSTR("CFBundleGetInfoString"));
				if (valueString != NULL)
					CFDictionaryAddValue(aboutDict, (const void*)kHIAboutBoxCopyrightKey, (const void*)valueString);

				HIAboutBox(aboutDict);
				CFRelease(aboutDict);
			}
		}
*/
	}
}

//-----------------------------------------------------------------------------
static void TurntablistPropertyEventListenerProc(void * inCallbackRefCon, void * inObject, const AudioUnitEvent * inEvent, UInt64 inEventHostTime, Float32 inParameterValue)
{
	TurntablistEditor * editor = (TurntablistEditor*) inCallbackRefCon;
	if ( (editor != NULL) && (inEvent != NULL) )
	{
		if (inEvent->mEventType == kAudioUnitEvent_PropertyChange)
		{
			if (inEvent->mArgument.mProperty.mPropertyID == kTurntablistProperty_AudioFile)
				editor->HandleAudioFileChange();
			else if (inEvent->mArgument.mProperty.mPropertyID == kTurntablistProperty_Play)
				editor->HandlePlayChange();
		}
	}
}

//-----------------------------------------------------------------------------
static void TurntablistParameterListenerProc(void * inRefCon, void * inObject, const AudioUnitParameter * inParameter, Float32 inValue)
{
	if ( (inObject == NULL) || (inParameter == NULL) )
		return;

	((TurntablistEditor*)inObject)->HandleParameterChange((long)(inParameter->mParameterID), inValue);
}




#pragma mark -
#pragma mark initialization

//-----------------------------------------------------------------------------
COMPONENT_ENTRY(TurntablistEditor)

//-----------------------------------------------------------------------------
// TurntablistEditor class implementation
//-----------------------------------------------------------------------------
TurntablistEditor::TurntablistEditor(AudioUnitCarbonView inInstance)
:	DfxGuiEditor(inInstance)
{
	audioFileNameDisplay = NULL;
	allParamsTextDisplay = NULL;
	playButton = NULL;
	scratchSpeedKnob = NULL;

	propertyEventListener = NULL;
	parameterListener = NULL;

	audioFileOpenDialog = NULL;

	dragAudioFileError = noErr;
}

//-----------------------------------------------------------------------------
TurntablistEditor::~TurntablistEditor()
{
	// remove and dispose the new-style property listener, if we created it
	if (propertyEventListener != NULL)
	{
		if (AUEventListenerRemoveEventType != NULL)
		{
			AUEventListenerRemoveEventType(propertyEventListener, this, &audioFilePropertyAUEvent);
			AUEventListenerRemoveEventType(propertyEventListener, this, &playPropertyAUEvent);
		}
		AUListenerDispose(propertyEventListener);
	}
	propertyEventListener = NULL;

	if (parameterListener != NULL)
	{
		for (AudioUnitParameterID i=0; i < kNumParameters; i++)
		{
			allParamsAUP.mParameterID = i;
			AUListenerRemoveParameter(parameterListener, this, &allParamsAUP);
		}
		AUListenerDispose(parameterListener);
	}
	parameterListener = NULL;

	FileOpenDialogFinished();
}

//-----------------------------------------------------------------------------
/*
CFStringRef gSupportedAudioFileExtensions[] = 
{
	CFSTR("aif"),
	CFSTR("aiff"),
	CFSTR("aifc"),
	CFSTR("wav"),
	CFSTR("sd2"),
	CFSTR("au"),
	CFSTR("snd"),
	CFSTR("mp3"),
	CFSTR("mpeg"),
	CFSTR("ac3"),
	CFSTR("aac"),
	CFSTR("adts"),
	CFSTR("m4a")
};
OSType gSupportedAudioFileTypeCodes[] = 
{
	kAudioFileAIFFType,
	kAudioFileAIFCType,
	kAudioFileWAVEType,
	kAudioFileSoundDesigner2Type,
	kAudioFileNextType,
	kAudioFileMP3Type,
	kAudioFileAC3Type,
	kAudioFileAAC_ADTSType
};
*/
#include <AudioToolbox/AudioFile.h>
static OSType * gSupportedAudioFileTypeCodes = NULL;
static UInt32 gNumSupportedAudioFileTypeCodes = 0;
static CFMutableArrayRef gSupportedAudioFileExtensions = NULL;

//-----------------------------------------------------------------------------
void DFX_InitializeSupportedAudioFileTypesArrays()
{
	// these functions are only supported in Mac OS X 10.3 or higher
	if ( (AudioFileGetGlobalInfoSize == NULL) || (AudioFileGetGlobalInfo == NULL) )
		return;

	OSStatus status;

	if (gSupportedAudioFileTypeCodes == NULL)
	{
		UInt32 propertySize = 0;
		status = AudioFileGetGlobalInfoSize(kAudioFileGlobalInfo_ReadableTypes, 0, NULL, &propertySize);
		if ( (status == noErr) && (propertySize > 0) )
		{
			gSupportedAudioFileTypeCodes = (OSType*) malloc(propertySize);
			status = AudioFileGetGlobalInfo(kAudioFileGlobalInfo_ReadableTypes, 0, NULL, &propertySize, gSupportedAudioFileTypeCodes);
			if (status == noErr)
				gNumSupportedAudioFileTypeCodes = propertySize / sizeof(*gSupportedAudioFileTypeCodes);
		}
	}

	if ( (gSupportedAudioFileExtensions == NULL) && (gSupportedAudioFileTypeCodes != NULL) )
	{
		for (UInt32 i=0; i < gNumSupportedAudioFileTypeCodes; i++)
		{
			CFArrayRef extensionsArray = NULL;
			UInt32 propertySize = sizeof(extensionsArray);
			status = AudioFileGetGlobalInfo(kAudioFileGlobalInfo_ExtensionsForType, sizeof(*gSupportedAudioFileTypeCodes), gSupportedAudioFileTypeCodes+i, &propertySize, &extensionsArray);
			if ( (status == noErr) && (extensionsArray != NULL) )
			{
				if (gSupportedAudioFileExtensions == NULL)
					gSupportedAudioFileExtensions = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
				if (gSupportedAudioFileExtensions != NULL)
					CFArrayAppendArray( gSupportedAudioFileExtensions, extensionsArray, CFRangeMake(0, CFArrayGetCount(extensionsArray)) );
//OSType audioFileType_bigEndian = CFSwapInt32HostToBig(gSupportedAudioFileTypeCodes[i]);
//fprintf(stderr, "\n\t%.4s\n", (char*)(&audioFileType_bigEndian));
//for (CFIndex j=0 ; j < CFArrayGetCount(extensionsArray); j++) CFShow(CFArrayGetValueAtIndex(extensionsArray, j));
				CFRelease(extensionsArray);	// XXX I think this should be released?  that's not documented, though
			}
		}
	}
}

//-----------------------------------------------------------------------------
bool DFX_IsSupportedAudioFileType(const FSRef * inFileRef)
{
	if (inFileRef == NULL)
		return false;

	bool result = false;

	LSRequestedInfo lsInfoFlags = kLSRequestExtension | kLSRequestTypeCreator;
	LSItemInfoRecord lsItemInfo = {0};
	OSStatus status = LSCopyItemInfoForRef(inFileRef, lsInfoFlags, &lsItemInfo);
	if (status == noErr)
	{
		if (lsItemInfo.flags & kLSItemInfoIsPackage)
			result = false;
		else
		{
			result = false;
			if (lsItemInfo.extension != NULL)
			{
				if (gSupportedAudioFileExtensions != NULL)
				{
					CFIndex numExtensions = CFArrayGetCount(gSupportedAudioFileExtensions);
					for (CFIndex i=0; i < numExtensions; i++)
					{
						CFStringRef supportedAudioFileExtension = (CFStringRef) CFArrayGetValueAtIndex(gSupportedAudioFileExtensions, i);
						if (supportedAudioFileExtension != NULL)
						{
							if ( CFStringCompare(lsItemInfo.extension, supportedAudioFileExtension, kCFCompareCaseInsensitive) == kCFCompareEqualTo )
							{
								result = true;
								break;
							}
						}
					}
				}
				CFRelease(lsItemInfo.extension);
			}
			if ( (!result) && (gSupportedAudioFileTypeCodes != NULL) )
			{
				for (UInt32 i=0; i < gNumSupportedAudioFileTypeCodes; i++)
				{
					if (lsItemInfo.filetype == gSupportedAudioFileTypeCodes[i])
					{
						result = true;
						break;
					}
				}
			}
		}
	}

	return result;
}

//-----------------------------------------------------------------------------
long TurntablistEditor::open()
{
	// background image
	DGImage * backgroundImage = new DGImage("background.png", this);
	SetBackgroundImage(backgroundImage);

	DGImage * sliderHandleImage = new DGImage("slider-handle.png", this);
	DGImage * knobImage = new DGImage("knob.png", this);
	DGImage * onOffButtonImage = new DGImage("on-off-button.png", this);
	DGImage * playButtonImage = new DGImage("play-button.png", this);
	DGImage * directionButtonImage = new DGImage("direction-button.png", this);
	DGImage * loopButtonImage = new DGImage("loop-button.png", this);
	DGImage * noteModeButtonImage = new DGImage("note-mode-button.png", this);
	DGImage * scratchModeButtonImage = new DGImage("scratch-mode-button.png", this);
	DGImage * helpButtonImage = new DGImage("help-button.png", this);


	// create controls
	DGRect pos;
	CFStringRef helpText;
	CFBundleRef pluginBundleRef = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));


	// buttons
	DGButton * button;

	pos.set(kPowerButtonX, kPowerButtonY, onOffButtonImage->getWidth(), onOffButtonImage->getHeight()/2);
	button = new DGButton(this, kParam_Power, &pos, onOffButtonImage, 2, kDGButtonType_incbutton);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("this switches the turntable power on or off"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Power parameter"));
	button->setHelpText(helpText);
	CFRelease(helpText);

#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
	pos.set(kColumn3, kRow3, onOffButtonImage->getWidth(), onOffButtonImage->getHeight()/2);
	button = new DGButton(this, kParam_Mute, &pos, onOffButtonImage, 2, kDGButtonType_incbutton);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("this mutes the audio output"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Mute parameter"));
	button->setHelpText(helpText);
	CFRelease(helpText);
#endif

	pos.set(kLoopButtonX, kLoopButtonY, loopButtonImage->getWidth(), loopButtonImage->getHeight()/2);
	button = new DGButton(this, kParam_Loop, &pos, loopButtonImage, 2, kDGButtonType_incbutton);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("if you enable this, the audio sample playback will continuously loop"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Loop parameter"));
	button->setHelpText(helpText);
	CFRelease(helpText);

	pos.set(kDirectionButtonX, kDirectionButtonY, directionButtonImage->getWidth(), directionButtonImage->getHeight()/2);
	button = new DGButton(this, kParam_Direction, &pos, directionButtonImage, 2, kDGButtonType_incbutton);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("this changes playback direction of the audio sample, regular or reverse"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Direction parameter"));
	button->setHelpText(helpText);
	CFRelease(helpText);

	pos.set(kNoteModeButtonX, kNoteModeButtonY, noteModeButtonImage->getWidth(), noteModeButtonImage->getHeight()/2);
	button = new DGButton(this, kParam_NoteMode, &pos, noteModeButtonImage, 2, kDGButtonType_incbutton);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("This toggles between \"reset mode\" (notes restart playback from the beginning of the audio sample) and \"resume mode\" (notes trigger playback from where the audio sample last stopped)"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Note Mode parameter"));
	button->setHelpText(helpText);
	CFRelease(helpText);

	pos.set(kNotePowerTrackButtonX, kNotePowerTrackButtonY, onOffButtonImage->getWidth(), onOffButtonImage->getHeight()/2);
	button = new DGButton(this, kParam_NotePowerTrack, &pos, onOffButtonImage, 2, kDGButtonType_incbutton);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("enabling this will cause note-on and note-off messages to be mapped to turntable power on and off for an interesting effect"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Note-Power Track parameter"));
	button->setHelpText(helpText);
	CFRelease(helpText);

	pos.set(kKeyTrackButtonX, kKeyTrackButtonY, onOffButtonImage->getWidth(), onOffButtonImage->getHeight()/2);
	button = new DGButton(this, kParam_KeyTracking, &pos, onOffButtonImage, 2, kDGButtonType_incbutton);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("this switches key tracking on or off (key tracking means that the pitch and speed of the audio sample playback are transposed in relation to the pitch of the MIDI note)"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Key Tracking parameter"));
	button->setHelpText(helpText);
	CFRelease(helpText);

	pos.set(kScratchModeButtonX, kScratchModeButtonY, scratchModeButtonImage->getWidth(), scratchModeButtonImage->getHeight()/2);
	button = new DGButton(this, kParam_ScratchMode, &pos, scratchModeButtonImage, 2, kDGButtonType_incbutton);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("this toggles between scrub mode and spin mode, which affects the behavior of the Scratch Amount parameter"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Scratch Mode parameter"));
	button->setHelpText(helpText);
	CFRelease(helpText);

	pos.set(kLoadButtonX, kLoadButtonY, onOffButtonImage->getWidth(), onOffButtonImage->getHeight()/2);
	button = new DGButton(this, &pos, onOffButtonImage, 2, kDGButtonType_pushbutton);
	button->setUserReleaseProcedure(TurntablistLoadButtonProc, this);
	button->setUseReleaseProcedureOnlyAtEndWithNoCancel(true);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("choose an audio file to load up onto the \"turntable\""), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Open Audio File button"));
	button->setHelpText(helpText);
	CFRelease(helpText);

	pos.set(kPlayButtonX, kPlayButtonY, playButtonImage->getWidth(), playButtonImage->getHeight()/2);
	playButton = new DGButton(this, &pos, playButtonImage, 2, kDGButtonType_incbutton);
	playButton->setUserProcedure(TurntablistPlayButtonProc, this);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("use this to start or stop the audio sample playback"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Play button"));
	playButton->setHelpText(helpText);
	CFRelease(helpText);

	pos.set(kMidiLearnX, kMidiLearnY, onOffButtonImage->getWidth(), onOffButtonImage->getHeight()/2);
	button = new DGButton(this, &pos, onOffButtonImage, 2, kDGButtonType_incbutton);
	button->setUserProcedure(TurntablistMidiLearnButtonProc, this);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("This switches MIDI learn mode on or off.  When MIDI learn is on, you can click on a parameter control to enable that parameter as the \"learner\" for incoming MIDI CC messages."), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the MIDI Learn button"));
	button->setHelpText(helpText);
	CFRelease(helpText);

	pos.set(kMidiResetX, kMidiResetY, onOffButtonImage->getWidth(), onOffButtonImage->getHeight()/2);
	button = new DGButton(this, &pos, onOffButtonImage, 2, kDGButtonType_pushbutton);
	button->setUserProcedure(TurntablistMidiResetButtonProc, this);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("this removes all of your MIDI CC -> parameter assignments"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the MIDI Reset button"));
	button->setHelpText(helpText);
	CFRelease(helpText);

	pos.set(kAboutSplashX, kAboutSplashY, kAboutSplashWidth, kAboutSplashHeight);
	button = new DGButton(this, &pos, NULL, 2, kDGButtonType_incbutton);
	button->setUserProcedure(TurntablistAboutButtonProc, this);
//	button->setHelpText(CFSTR("click here to go to the "PLUGIN_CREATOR_NAME_STRING" web site"));
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("click here to go to the Destroy FX web site"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the About hot-spot"));
	button->setHelpText(helpText);
	CFRelease(helpText);

	pos.set(kHelpX, kHelpY, helpButtonImage->getWidth(), helpButtonImage->getHeight()/2);
	button = new DGButton(this, &pos, helpButtonImage, 2, kDGButtonType_pushbutton);
	button->setUserReleaseProcedure(TurntablistHelpButtonProc, this);
	button->setUseReleaseProcedureOnlyAtEndWithNoCancel(true);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("view the full manual"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Help button"));
	button->setHelpText(helpText);
	CFRelease(helpText);
/*
Rect buttonRect = pos.convertToRect();
ControlButtonContentInfo buttonContentInfo;
buttonContentInfo.contentType = kControlContentIconRef;
OSStatus buttonStat = GetIconRef(kOnSystemDisk, kSystemIconsCreator, kHelpIcon, &(buttonContentInfo.u.iconRef));
ControlRef buttonControl = NULL;
buttonStat = CreateRoundButtonControl(GetCarbonWindow(), &buttonRect, kControlSizeNormal, &buttonContentInfo, &buttonControl);
*/


	// knobs
	DGAnimation * knob;
	const long knobWidth = knobImage->getWidth();
	const long knobHeight = knobImage->getHeight() / kKnobFrames;

	pos.set(kPitchRangeKnobX, kPitchRangeKnobY, knobWidth, knobHeight);
	knob = new DGAnimation(this, kParam_PitchRange, &pos, knobImage, kKnobFrames);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("this controls the range of pitch adjustment values that the Pitch Shift parameter offers"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Pitch Range parameter"));
	knob->setHelpText(helpText);
	CFRelease(helpText);

	long scratchSpeedParam = (getparameter_i(kParam_ScratchMode) == kScratchMode_Scrub) ? kParam_ScratchSpeed_scrub : kParam_ScratchSpeed_spin;
	pos.set(kScratchSpeedKnobX, kScratchSpeedKnobY, knobWidth, knobHeight);
	scratchSpeedKnob = new DGAnimation(this, scratchSpeedParam, &pos, knobImage, kKnobFrames);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("this sets the speed of the scratching effect that the Scratch Amount parameter produces"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Scratch Speed parameter"));
	scratchSpeedKnob->setHelpText(helpText);
	CFRelease(helpText);

	pos.set(kSpinUpKnobX, kSpinUpKnobY, knobWidth, knobHeight);
	knob = new DGAnimation(this, kParam_SpinUpSpeed, &pos, knobImage, kKnobFrames);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("this controls how quickly the audio playback \"spins up\" when the turntable power turns on"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Spin Up Speed parameter"));
	knob->setHelpText(helpText);
	CFRelease(helpText);

	pos.set(kSpinDownKnobX, kSpinDownKnobY, knobWidth, knobHeight);
	knob = new DGAnimation(this, kParam_SpinDownSpeed, &pos, knobImage, kKnobFrames);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("this controls how quickly the audio playback \"spins down\" when the turntable power turns off"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Spin Down Speed parameter"));
	knob->setHelpText(helpText);
	CFRelease(helpText);

	pos.set(kRootKeyKnobX, kRootKeyKnobY, knobWidth, knobHeight);
	knob = new DGAnimation(this, kParam_RootKey, &pos, knobImage, kKnobFrames);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("when Key Track is enabled, this controls the MIDI key around which the transposition is centered"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Root Key parameter"));
	knob->setHelpText(helpText);
	CFRelease(helpText);

#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
	pos.set(kColumn2k, kRow3 + kKnobOffsetY, knobWidth, knobHeight);
	knob = new DGAnimation(this, kParam_Volume, &pos, knobImage, kKnobFrames);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("this controls the overall volume of the audio output"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Volume parameter"));
	knob->setHelpText(helpText);
	CFRelease(helpText);
#endif


	// sliders
	DGSlider * slider;

	// scratch amount
	pos.set(kScratchAmountSliderX, kSliderY, kSliderWidth, kSliderHeight);
	slider = new TurntablistScratchSlider(this, kParam_ScratchAmount, &pos, kDGSliderAxis_vertical, sliderHandleImage);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("This slider is what does the actual scratching.  In scrub mode, the slider represents time.  In spin mode, the slider represents forward and backward speed."), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Scratch Amount parameter"));
	slider->setHelpText(helpText);
	CFRelease(helpText);

	// pitch shift
	pos.set(kPitchShiftSliderX, kSliderY, kSliderWidth, kSliderHeight);
	slider = new DGSlider(this, kParam_PitchShift, &pos, kDGSliderAxis_vertical, sliderHandleImage, NULL);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("changes the audio playback pitch between +/- the Pitch Range value"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Pitch Shift parameter"));
	slider->setHelpText(helpText);
	CFRelease(helpText);


	// text displays

	// universal parameter value display
	pos.set(kDisplayX, kDisplayY, kFileNameWidth, kFileNameHeight);
	allParamsTextDisplay = new DGStaticTextDisplay(this, &pos, NULL, kDGTextAlign_center, kTurntablistFontSize, kDGColor_white, NULL);

	// audio file name display
	pos.set(kFileNameX, kFileNameY, kFileNameWidth, kFileNameHeight);
	audioFileNameDisplay = new DGStaticTextDisplay(this, &pos, NULL, kDGTextAlign_center, kTurntablistFontSize, kDGColor_white, NULL);
	HandleAudioFileChange();



	// install property listeners for the "audio file" and "play" properties
	if ( (AUEventListenerCreate != NULL) && (AUEventListenerAddEventType != NULL) )
	{
		OSStatus status = AUEventListenerCreate(TurntablistPropertyEventListenerProc, this, 
												CFRunLoopGetCurrent(), kCFRunLoopDefaultMode, 
												0.030f, 0.030f, &propertyEventListener);

		if (status == noErr)
		{
			memset(&audioFilePropertyAUEvent, 0, sizeof(audioFilePropertyAUEvent));
			audioFilePropertyAUEvent.mEventType = kAudioUnitEvent_PropertyChange;
			audioFilePropertyAUEvent.mArgument.mProperty.mAudioUnit = GetEditAudioUnit();
			audioFilePropertyAUEvent.mArgument.mProperty.mPropertyID = kTurntablistProperty_AudioFile;
			audioFilePropertyAUEvent.mArgument.mProperty.mScope = kAudioUnitScope_Global;
			audioFilePropertyAUEvent.mArgument.mProperty.mElement = 0;
			status = AUEventListenerAddEventType(propertyEventListener, this, &audioFilePropertyAUEvent);

			playPropertyAUEvent = audioFilePropertyAUEvent;
			playPropertyAUEvent.mArgument.mProperty.mPropertyID = kTurntablistProperty_Play;
			status = AUEventListenerAddEventType(propertyEventListener, this, &playPropertyAUEvent);
		}
	}



	// install a parameter listener for every parameter
	OSStatus status = AUListenerCreate(TurntablistParameterListenerProc, this,
						CFRunLoopGetCurrent(), kCFRunLoopDefaultMode, 0.030f, // 30 ms
						&parameterListener);
	if (status != noErr)
		parameterListener = NULL;

	if (parameterListener != NULL)
	{
		allParamsAUP.mAudioUnit = GetEditAudioUnit();
		allParamsAUP.mScope = kAudioUnitScope_Global;
		allParamsAUP.mElement = (AudioUnitElement)0;
		for (AudioUnitParameterID i=0; i < kNumParameters; i++)
		{
			allParamsAUP.mParameterID = i;
			AUListenerAddParameter(parameterListener, this, &allParamsAUP);
		}
	}



	EventTypeSpec dragEvents[] = {
//								{ kEventClassControl, kEventControlInitialize }, 
								{ kEventClassControl, kEventControlDragEnter }, 
								{ kEventClassControl, kEventControlDragWithin },
								{ kEventClassControl, kEventControlDragLeave }, 
								{ kEventClassControl, kEventControlDragReceive }
								};
	WantEventTypes(GetControlEventTarget(GetCarbonPane()), GetEventTypeCount(dragEvents), dragEvents);
	SetControlDragTrackingEnabled(GetCarbonPane(), true);
	SetAutomaticControlDragTrackingEnabledForWindow(GetCarbonWindow(), true);

	DFX_InitializeSupportedAudioFileTypesArrays();



	return noErr;
}

//-----------------------------------------------------------------------------
void TurntablistEditor::post_open()
{
	// initialize the button's current value once the button has been fully created
	HandlePlayChange();
}



#pragma mark -
#pragma mark audio file chooser

//-----------------------------------------------------------------------------
CFStringRef DFX_CopyFileNameString(const FSRef & inFileRef)
{
	CFStringRef fileName = NULL;
	OSStatus status = LSCopyDisplayNameForRef(&inFileRef, &fileName);
	if (status == noErr)
		return fileName;
	else
		return NULL;
}

#ifdef USE_LIBSNDFILE
	#include "sndfile.h"	// for the libsndfile error code constants
#else
	#include <AudioToolbox/AudioToolbox.h>	// for ExtAudioFile error code constants
#endif
//-----------------------------------------------------------------------------
OSStatus DFX_NotifyAudioFileLoadError(OSStatus inErrorCode, const FSRef & inAudioFileRef)
{
	CFBundleRef pluginBundleRef = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));
	if (pluginBundleRef == NULL)
		return coreFoundationUnknownErr;

	CFStringRef titleString = CFCopyLocalizedStringFromTableInBundle(CFSTR("The file \"%@\" could not be loaded."), 
								CFSTR("Localizable"), pluginBundleRef, 
								CFSTR("title for the dialog telling you that the audio file could not be loaded"));
	CFStringRef audioFileName = DFX_CopyFileNameString(inAudioFileRef);
	if (audioFileName != NULL)
	{
		CFStringRef titleStringWithFileName = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, titleString, audioFileName);
		if (titleStringWithFileName != NULL)
		{
			CFRelease(titleString);
			titleString = titleStringWithFileName;
		}
		CFRelease(audioFileName);
	}

	CFStringRef messageString = CFCopyLocalizedStringFromTableInBundle(CFSTR("An error was encountered while trying to load audio data from the file.  %@"), 
								CFSTR("Localizable"), pluginBundleRef, 
								CFSTR("explanation for the dialog telling you that the audio file could not be loaded"));
	CFStringRef errorDescriptionString = NULL;
	switch (inErrorCode)
	{
#ifdef USE_LIBSNDFILE
		case SF_ERR_UNRECOGNISED_FORMAT:
			errorDescriptionString = CFCopyLocalizedStringFromTableInBundle(CFSTR("The audio data is in a format that could not be recognized."), 
										CFSTR("Localizable"), pluginBundleRef, 
										CFSTR("libsndfile SF_ERR_UNRECOGNISED_FORMAT description"));
			break;
		case SF_ERR_SYSTEM:
			errorDescriptionString = CFCopyLocalizedStringFromTableInBundle(CFSTR("A system error occurred while trying to read the file."), 
										CFSTR("Localizable"), pluginBundleRef, 
										CFSTR("libsndfile SF_ERR_SYSTEM description"));
			break;
		case SF_ERR_MALFORMED_FILE:
			errorDescriptionString = CFCopyLocalizedStringFromTableInBundle(CFSTR("The audio data is not readable due to being malformed."), 
										CFSTR("Localizable"), pluginBundleRef, 
										CFSTR("libsndfile SF_ERR_MALFORMED_FILE description"));
			break;
		case SF_ERR_UNSUPPORTED_ENCODING:
			errorDescriptionString = CFCopyLocalizedStringFromTableInBundle(CFSTR("The audio data is encoded in a format that is not supported."), 
										CFSTR("Localizable"), pluginBundleRef, 
										CFSTR("libsndfile SF_ERR_UNSUPPORTED_ENCODING description"));
			break;
#else
		case unsupportedOSErr:
			errorDescriptionString = CFCopyLocalizedStringFromTableInBundle(CFSTR(PLUGIN_NAME_STRING" requires Mac OS X version 10.4 (Tiger) or higher."), 
										CFSTR("Localizable"), pluginBundleRef, 
										CFSTR("unsupportedOSErr description"));
			break;
		// property
		case kExtAudioFileError_InvalidProperty:
		case kExtAudioFileError_InvalidPropertySize:
		case kAudioFileUnsupportedPropertyError:
		case kAudioFileBadPropertySizeError:
		case kAudioConverterErr_PropertyNotSupported:
		// format
		case kExtAudioFileError_NonPCMClientFormat:
		case kExtAudioFileError_InvalidChannelMap:
		case kExtAudioFileError_InvalidDataFormat:
		case kExtAudioFileError_MaxPacketSizeUnknown:
		case kAudioFileUnsupportedDataFormatError:
		case kAudioConverterErr_InvalidInputSize:
		case kAudioConverterErr_InvalidOutputSize:
		case kAudioConverterErr_RequiresPacketDescriptionsError:
		case kAudioConverterErr_InputSampleRateOutOfRange:
		case kAudioConverterErr_OutputSampleRateOutOfRange:
		// operation
		case kExtAudioFileError_InvalidOperationOrder:
		case kExtAudioFileError_InvalidSeek:
		case kAudioFileOperationNotSupportedError:
		// file failure
		case kAudioFileInvalidChunkError:
		case kAudioFileDoesNotAllow64BitDataSizeError:
		case kAudioFileInvalidPacketOffsetError:
		case kAudioFileInvalidFileError:
		// misc
		case kAudioFileUnsupportedFileTypeError:
		case kAudioFilePermissionsError:
		case kAudioFileNotOptimizedError:
		// unknown
		case kAudioFileUnspecifiedError:
		case kAudioConverterErr_UnspecifiedError:
#endif
		default:
			errorDescriptionString = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("error code %ld"), inErrorCode);
			break;
	}
	if (errorDescriptionString != NULL)
	{
		CFStringRef messageStringWithError = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, messageString, errorDescriptionString);
		if (messageStringWithError != NULL)
		{
			CFRelease(messageString);
			messageString = messageStringWithError;
		}
		CFRelease(errorDescriptionString);
	}

	DialogRef alertDialog = NULL;
	OSStatus status = CreateStandardAlert(kAlertNoteAlert, titleString, messageString, NULL, &alertDialog);
	CFRelease(titleString);
	CFRelease(messageString);
	if (status == noErr)
	{
		DialogItemIndex itemHit;
		status = RunStandardAlert(alertDialog, NULL, &itemHit);
	}

	return status;
}

//-----------------------------------------------------------------------------
// this is the event handler callback for the custom open audio file Nav Services dialog
static pascal void DFX_OpenAudioFileNavEventHandlerProc(NavEventCallbackMessage inCallbackSelector, NavCBRecPtr inCallbackParams, NavCallBackUserData inUserData)
{
	TurntablistEditor * editor = (TurntablistEditor*) inUserData;
	NavDialogRef dialog = inCallbackParams->context;

	switch (inCallbackSelector)
	{
		case kNavCBStart:
			break;

		case kNavCBTerminate:
			if (editor != NULL)
				editor->FileOpenDialogFinished();
			else if (dialog != NULL)
				NavDialogDispose(dialog);
			break;

		// the user did something action-packed
		case kNavCBUserAction:
		{
			if (dialog == NULL)
				break;
			// we're only interested in file open actions
			NavUserAction userAction = NavDialogGetUserAction(dialog);
			// and I guess cancel, too
			if (userAction == kNavUserActionCancel)
			{
//				userCanceledErr;
				break;
			}
			if (userAction != kNavUserActionOpen)
				break;

			// get the file-choose response and check that it's valid (not a cancel or something like that)
			NavReplyRecord reply;
			OSStatus status = NavDialogGetReply(dialog, &reply);
			if (status == noErr)
			{
				if (reply.validRecord)
				{
					// grab the first response item (since there can only be 1) as an FSRef
					AEKeyword theKeyword;
					DescType actualType;
					Size actualSize;
					FSRef audioFileRef;
					status = AEGetNthPtr(&(reply.selection), 1, typeFSRef, &theKeyword, &actualType, 
										&audioFileRef, sizeof(audioFileRef), &actualSize);
					if (status == noErr)
					{
						if (editor != NULL)
						{
							status = editor->LoadAudioFile(audioFileRef);
							if (status != noErr)
								DFX_NotifyAudioFileLoadError(status, audioFileRef);
						}
						else
							status = paramErr;
					}
				}
				NavDisposeReply(&reply);
			}
			break;
		}

		case kNavCBEvent:
			switch (inCallbackParams->eventData.eventDataParms.event->what)
			{
				// XXX do something with the update events?
				case updateEvt:
					break;
			}
			break;
	}
}

//-----------------------------------------------------------------------------
// This is the filter callback for the custom open audio file Nav Services dialog.  
// It handles filtering out non-audio files from the file list display.  
// They will still appear in the list, but can not be selected.
// A response of true means "allow the file" and false means "disallow the file."
static pascal Boolean DFX_OpenAudioFileNavFilterProc(AEDesc * inItem, void * inInfo, void * inUserData, NavFilterModes inFilterMode)
{
	// default to allowing the item
	// only if we determine that a file, but not an audio file, do we want to reject it
	Boolean result = true;

	// the only items that we are filtering are files listed in the browser area
	if (inFilterMode == kNavFilteringBrowserList)
	{
		NavFileOrFolderInfo * info = (NavFileOrFolderInfo*) inInfo;
		// only interested in filtering files, all directories are fine
		if ( !(info->isFolder) )
		{
			// try to determine if this item is an audio file
			// if it is not, then reject it; otherwise, allow it
			AEDesc fsrefDesc;
			OSStatus status = AECoerceDesc(inItem, typeFSRef, &fsrefDesc);
			if (status == noErr)
			{
				FSRef fileRef;
				status = AEGetDescData(&fsrefDesc, &fileRef, sizeof(fileRef));
				if (status == noErr)
				{
					result = DFX_IsSupportedAudioFileType(&fileRef);
				}
			}
			AEDisposeDesc(&fsrefDesc);
		}
	}

	return result;
}

// unique identifier of our audio file open dialog for Navigation Services
const UInt32 kTurntablistAudioFileOpenNavDialogKey = PLUGIN_ID;

static NavEventUPP gOpenAudioFileNavEventHandlerUPP = NULL;
static NavObjectFilterUPP gOpenAudioFileNavFilterUPP = NULL;
//-----------------------------------------------------------------------------
OSStatus TurntablistEditor::HandleLoadButton()
{
	// we already have a file open dialog running
	if (audioFileOpenDialog != NULL)
	{
		// if the window is already open, then don't open a new one, just bring the existing one to the front
		WindowRef dialogWindow = NavDialogGetWindow(audioFileOpenDialog);
		if (dialogWindow != NULL)
			SelectWindow(dialogWindow);
		return kNavWrongDialogStateErr;
	}

	NavDialogCreationOptions dialogOptions;
	OSStatus status = NavGetDefaultDialogCreationOptions(&dialogOptions);
	if (status != noErr)
		return status;
	dialogOptions.optionFlags &= ~kNavAllowMultipleFiles;	// disallow multiple file selection
	// this gives this dialog a unique identifier so that it has independent remembered settings for the calling app
	dialogOptions.preferenceKey = kTurntablistAudioFileOpenNavDialogKey;
	dialogOptions.modality = kWindowModalityNone;
	// these 2 things make it a sheet dialog
	dialogOptions.modality = kWindowModalityWindowModal;
	dialogOptions.parentWindow = GetCarbonWindow();
	UInt32 propertyDataSize = sizeof(dialogOptions.clientName);
	status = AudioUnitGetProperty(GetEditAudioUnit(), kAudioUnitProperty_ContextName, kAudioUnitScope_Global, (AudioUnitElement)0, &(dialogOptions.clientName), &propertyDataSize);
	if ( (status != noErr) || (dialogOptions.clientName == NULL) )
		dialogOptions.clientName = CFSTR(PLUGIN_NAME_STRING);
/*
	// XXX do I really want to set this, or better to just allow the standard titling?
	if (dialogOptions.clientName != NULL)
	{
		CFStringRef windowTitle_base = CFCopyLocalizedStringFromTableInBundle(CFSTR("Open: %@ Audio File"), 
											CFSTR("Localizable"), CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER)), 
											CFSTR("window title for the audio file chooser dialog"));
		dialogOptions.windowTitle = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, windowTitle_base, dialogOptions.clientName);
		CFRelease(windowTitle_base);
	}
*/

	// create and run a GetFile dialog to allow the user to find and choose an audio file to load
	if (gOpenAudioFileNavEventHandlerUPP == NULL)
		gOpenAudioFileNavEventHandlerUPP = NewNavEventUPP(DFX_OpenAudioFileNavEventHandlerProc);
	if (gOpenAudioFileNavFilterUPP == NULL)
		gOpenAudioFileNavFilterUPP = NewNavObjectFilterUPP(DFX_OpenAudioFileNavFilterProc);
	audioFileOpenDialog = NULL;
	status = NavCreateGetFileDialog(&dialogOptions, NULL, gOpenAudioFileNavEventHandlerUPP, NULL, gOpenAudioFileNavFilterUPP, (void*)this, &audioFileOpenDialog);
	if (status == noErr)
	{
		status = NavDialogRun(audioFileOpenDialog);
	}
	if (dialogOptions.windowTitle != NULL)
		CFRelease(dialogOptions.windowTitle);

	return status;
}

//-----------------------------------------------------------------------------
ComponentResult TurntablistEditor::LoadAudioFile(const FSRef & inAudioFileRef)
{
	AliasHandle aliasHandle = NULL;
	OSErr error = FSNewAlias(NULL, &inAudioFileRef, &aliasHandle);
	if (error != noErr)
		return error;
	if (aliasHandle == NULL)
		return nilHandleErr;

	UInt32 dataSize;
	if (GetAliasSize != NULL)
		dataSize = GetAliasSize(aliasHandle);
	else
		dataSize = GetHandleSize((Handle)aliasHandle);

	ComponentResult result = AudioUnitSetProperty(GetEditAudioUnit(), kTurntablistProperty_AudioFile, 
												kAudioUnitScope_Global, (AudioUnitElement)0, 
												*aliasHandle, dataSize);

	DisposeHandle((Handle)aliasHandle);

	return result;
}

//-----------------------------------------------------------------------------
void TurntablistEditor::FileOpenDialogFinished()
{
	if (audioFileOpenDialog != NULL)
	{
		// circumvent recursive calls
		NavDialogRef tempDialog = audioFileOpenDialog;
		audioFileOpenDialog = NULL;
		NavDialogDispose(tempDialog);
	}
}



#pragma mark -
#pragma mark features

//-----------------------------------------------------------------------------
CFStringRef DFX_CopyNameAndVersionString()
{
	CFBundleRef pluginBundleRef = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));
	if (pluginBundleRef == NULL)
		return NULL;

	CFStringRef nameString = (CFStringRef) CFBundleGetValueForInfoDictionaryKey(pluginBundleRef, kCFBundleNameKey);
	CFStringRef versionString = (CFStringRef) CFBundleGetValueForInfoDictionaryKey(pluginBundleRef, CFSTR("CFBundleShortVersionString"));
	if (versionString == NULL)
		versionString = (CFStringRef) CFBundleGetValueForInfoDictionaryKey(pluginBundleRef, kCFBundleVersionKey);
	CFStringRef nameAndVersionString = NULL;
	if ( (nameString != NULL) && (versionString != NULL) )
		nameAndVersionString = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%@  %@"), nameString, versionString);
	else if (nameString != NULL)
		nameAndVersionString = (CFStringRef) CFRetain(nameString);
	else if (versionString != NULL)
		nameAndVersionString = (CFStringRef) CFRetain(versionString);

	return nameAndVersionString;
}

//-----------------------------------------------------------------------------
ComponentResult TurntablistEditor::HandleAudioFileChange()
{
	CFStringRef displayString = NULL;
	UInt32 dataSize = 0;
	Boolean propertyWritable;
	ComponentResult result = AudioUnitGetPropertyInfo(GetEditAudioUnit(), kTurntablistProperty_AudioFile, 
														kAudioUnitScope_Global, (AudioUnitElement)0, 
														&dataSize, &propertyWritable);
	if ( (result == noErr) && (dataSize > 0) )
	{
		AliasHandle aliasHandle = (AliasHandle) NewHandle(dataSize);
		if (aliasHandle != NULL)
		{
			result = AudioUnitGetProperty(GetEditAudioUnit(), kTurntablistProperty_AudioFile, 
											kAudioUnitScope_Global, (AudioUnitElement)0, 
											*aliasHandle, &dataSize);
			if (result == noErr)
			{
				FSRef audioFileRef;
				Boolean wasChanged;
				result = FSResolveAlias(NULL, aliasHandle, &audioFileRef, &wasChanged);
				if (result == noErr)
					displayString = DFX_CopyFileNameString(audioFileRef);
			}
			DisposeHandle((Handle)aliasHandle);
		}
	}

	if (displayString == NULL)
		displayString = DFX_CopyNameAndVersionString();

	if (displayString != NULL)
	{
		SetFileNameDisplay(displayString);
		CFRelease(displayString);
	}

	return result;
}

//-----------------------------------------------------------------------------
ComponentResult TurntablistEditor::HandlePlayButton(bool inPlay)
{
	if (allParamsTextDisplay != NULL)
	{
		CFStringRef displayString = (inPlay) ? CFSTR("playback:  start") : CFSTR("playback:  stop");
		allParamsTextDisplay->setCFText(displayString);
	}

	Boolean play_fixedSize = inPlay;
	return AudioUnitSetProperty(GetEditAudioUnit(), kTurntablistProperty_Play, 
				kAudioUnitScope_Global, (AudioUnitElement)0, &play_fixedSize, sizeof(play_fixedSize));
}

//-----------------------------------------------------------------------------
ComponentResult TurntablistEditor::HandlePlayChange()
{
	if (playButton == NULL)
		return kAudioUnitErr_Uninitialized;

	Boolean play;
	UInt32 dataSize = sizeof(play);
	ComponentResult result = AudioUnitGetProperty(GetEditAudioUnit(), kTurntablistProperty_Play, 
												kAudioUnitScope_Global, (AudioUnitElement)0, 
												&play, &dataSize);
	if (result == noErr)
	{
		ControlRef carbonControl = playButton->getCarbonControl();
		if (carbonControl != NULL)
			SetControl32BitValue( carbonControl, (play ? 1 : 0) );
	}

	return result;
}

//-----------------------------------------------------------------------------
void TurntablistEditor::SetFileNameDisplay(CFStringRef inDisplayText)
{
	if (inDisplayText == NULL)
		return;

	if (audioFileNameDisplay != NULL)
		audioFileNameDisplay->setCFText(inDisplayText);
}

//-----------------------------------------------------------------------------
void TurntablistEditor::HandleMidiLearnButton(bool inLearn)
{
	if (allParamsTextDisplay != NULL)
	{
		CFStringRef displayString = (inLearn) ? CFSTR("MIDI learn:  on") : CFSTR("MIDI learn:  off");
		allParamsTextDisplay->setCFText(displayString);
	}

	setmidilearning(inLearn);
}

//-----------------------------------------------------------------------------
void TurntablistEditor::HandleMidiResetButton()
{
	if (allParamsTextDisplay != NULL)
		allParamsTextDisplay->setCFText( CFSTR("MIDI reset") );

	resetmidilearn();
}

//-----------------------------------------------------------------------------
bool TurntablistEditor::HandleEvent(EventHandlerCallRef inHandlerRef, EventRef inEvent)
{
	if (GetEventClass(inEvent) == kEventClassControl)
	{
		UInt32 inEventKind = GetEventKind(inEvent);

		ControlRef carbonControl = NULL;
		OSStatus status = GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, NULL, sizeof(carbonControl), NULL, &carbonControl);
		if (status != noErr)
			carbonControl = NULL;

/*
		if (inEventKind == kEventControlInitialize)
		{
			if (HIViewChangeFeatures != NULL)
			{
				if (carbonControl != NULL)
					status = HIViewChangeFeatures(carbonControl, kControlSupportsDragAndDrop, 0);
			}
			else
			{
				UInt32 controlFeatures = kControlHandlesTracking | kControlSupportsDataAccess | kControlSupportsGetRegion;
				if (carbonControl != NULL)
					status = GetControlFeatures(carbonControl, &controlFeatures);
				controlFeatures |= kControlSupportsDragAndDrop;
				status = SetEventParameter(inEvent, kEventParamControlFeatures, typeUInt32, sizeof(controlFeatures), &controlFeatures);
			}
			return true;
		}
*/

		if ( (carbonControl != NULL) && (carbonControl == GetCarbonPane()) )
		{
			DragRef drag = NULL;
			switch (inEventKind)
			{
				case kEventControlDragEnter:
				case kEventControlDragWithin:
				case kEventControlDragLeave:
				case kEventControlDragReceive:
					{
						status = GetEventParameter(inEvent, kEventParamDragRef, typeDragRef, NULL, sizeof(drag), NULL, &drag);
						if ( (status != noErr) || (drag == NULL) )
							return false;
					}
					break;
				default:
					break;
			}

			switch (inEventKind)
			{
				case kEventControlDragEnter:
					{
//fprintf(stderr, "kEventControlDragEnter\n");
						getBackgroundControl()->setDragActive(true);
						dragAudioFileError = noErr;
						Boolean acceptDrop = true;
						status = SetEventParameter(inEvent, kEventParamControlWouldAcceptDrop, typeBoolean, sizeof(acceptDrop), &acceptDrop);
/*
						RgnHandle dragRegion = NewRgn();
						if (dragRegion != NULL)
						{
							Rect dragRegionBounds;
							GetControlBounds(carbonControl, &dragRegionBounds);
							RectRgn(dragRegion, &dragRegionBounds);

							CGrafPtr oldPort = NULL;
							GetPort(&oldPort);
							SetPortWindowPort( GetCarbonWindow() );

							status = ShowDragHilite(drag, dragRegion, true);

							if (oldPort != NULL)
								SetPort(oldPort);

							DisposeRgn(dragRegion);
						}
*/
//						HiliteControl(carbonControl, 1);
//						HIViewSetHilite(carbonControl, 1);//kHIViewIndicatorPart);
//						HIViewSetNeedsDisplay(carbonControl, true);
						return true;
					}
					break;

				case kEventControlDragWithin:
					{
//fprintf(stderr, "kEventControlDragWithin\n");
						return true;
					}
					break;

				case kEventControlDragLeave:
//fprintf(stderr, "kEventControlDragLeave\n");
					{
//						status = HideDragHilite(drag);
						getBackgroundControl()->setDragActive(false);
//						HiliteControl(carbonControl, kControlNoPart);
//						HIViewSetHilite(carbonControl, kHIViewNoPart);
//						HIViewSetNeedsDisplay(carbonControl, true);
						return true;
					}
					break;

				case kEventControlDragReceive:
					{
//fprintf(stderr, "kEventControlDragReceive\n");
//						status = HideDragHilite(drag);
						getBackgroundControl()->setDragActive(false);

						bool foundFile = false;
						memset(&dragAudioFileRef, 0, sizeof(dragAudioFileRef));

						UInt16 numDragItems = 0;
						status = CountDragItems(drag, &numDragItems);
						if ( (status == noErr) && (numDragItems > 0) )
						{
							for (UInt16 dragItemIndex=1; dragItemIndex <= numDragItems; dragItemIndex++)
							{
								DragItemRef dragItem = 0;
								status = GetDragItemReferenceNumber(drag, dragItemIndex, &dragItem);
								if (status == noErr)
								{
#if 0
									UInt16 numDragFlavors = 0;
									status = CountDragItemFlavors(drag, dragItem, &numDragFlavors);
									if ( (status == noErr) && (numDragFlavors > 0) )
									{
										for (UInt16 flavorIndex=1; flavorIndex <= numDragFlavors; flavorIndex++)
										{
											FlavorType dragFlavorType = 0;
											status = GetFlavorType(drag, dragItem, flavorIndex, &dragFlavorType);
											if (status == noErr)
											{
												Size dragFlavorDataSize = 0;
												status = GetFlavorDataSize(drag, dragItem, dragFlavorType, &dragFlavorDataSize);
FlavorType dragFlavorType_bigEndian = CFSwapInt32HostToBig(dragFlavorType);
fprintf(stderr, "flavor = '%.4s', size = %ld\n", (char*)(&dragFlavorType_bigEndian), dragFlavorDataSize);
											}
										}
									}
#endif
									FlavorType dragFlavorType = typeFileURL;
									Size dragFlavorDataSize = 0;
									CFURLRef fileURL = NULL;
									status = GetFlavorDataSize(drag, dragItem, dragFlavorType, &dragFlavorDataSize);
									if ( (status == noErr) && (dragFlavorDataSize > 0) )
									{
										UInt8 * fileUrlFlavorData = (UInt8*) malloc(dragFlavorDataSize);
										if (fileUrlFlavorData != NULL)
										{
											status = GetFlavorData(drag, dragItem, dragFlavorType, fileUrlFlavorData, &dragFlavorDataSize, 0);
											if (status == noErr)
											{
//fprintf(stderr, "file URL = %.*s\n", dragFlavorDataSize, fileUrlFlavorData);
												fileURL = CFURLCreateWithBytes(kCFAllocatorDefault, fileUrlFlavorData, dragFlavorDataSize, kCFStringEncodingUTF8, NULL);
												if (fileURL != NULL)
												{
													Boolean success = CFURLGetFSRef(fileURL, &dragAudioFileRef);
													if (success)
														foundFile = true;
												}
											}
											free(fileUrlFlavorData);
										}
									}
									if (!foundFile)
									{
										dragFlavorType = kDragFlavorTypeHFS;
										dragFlavorDataSize = 0;
										status = GetFlavorDataSize(drag, dragItem, dragFlavorType, &dragFlavorDataSize);
										if ( (status == noErr) && ((size_t)dragFlavorDataSize >= sizeof(HFSFlavor)) )
										{
											HFSFlavor * hfsFlavorData = (HFSFlavor*) malloc(dragFlavorDataSize);
											if (hfsFlavorData != NULL)
											{
												status = GetFlavorData(drag, dragItem, dragFlavorType, hfsFlavorData, &dragFlavorDataSize, 0);
												if (status == noErr)
												{
													status = FSpMakeFSRef(&(hfsFlavorData->fileSpec), &dragAudioFileRef);
													if (status == noErr)
														foundFile = true;
												}
												free(hfsFlavorData);
											}
										}
									}
									bool result = false;
									if (foundFile)
									{
										if ( DFX_IsSupportedAudioFileType(&dragAudioFileRef) )
										{
											status = LoadAudioFile(dragAudioFileRef);
											if (status == noErr)
												result = true;
											else
											{
//												DFX_NotifyAudioFileLoadError(status, dragAudioFileRef);
												dragAudioFileError = status;
											}
										}
										else
										{
											if (fileURL == NULL)
												fileURL = CFURLCreateFromFSRef(kCFAllocatorDefault, &dragAudioFileRef);
											if (fileURL != NULL)
											{
												if ( CFURLIsAUPreset(fileURL) )
												{
													status = RestoreAUStateFromPresetFile(GetEditAudioUnit(), fileURL);
													if (status == noErr)
														result = true;
												}
											}
										}
									}
									if (fileURL != NULL)
										CFRelease(fileURL);
									return result;
								}
							}
						}

						return false;
					}
					break;

				default:
					break;
			}
		}
	}

	// let the parent implementation do its thing
	return DfxGuiEditor::HandleEvent(inHandlerRef, inEvent);
}



#pragma mark -
#pragma mark parameter -> string conversions

//-----------------------------------------------------------------------------
CFStringRef DFX_CopyAUParameterName(AudioUnit inAUInstance, long inParameterID)
{
	if (inAUInstance == NULL)
		return NULL;

	AudioUnitParameterInfo parameterInfo;
	memset(&parameterInfo, 0, sizeof(parameterInfo));
	UInt32 dataSize = sizeof(parameterInfo);
	ComponentResult result = AudioUnitGetProperty(inAUInstance, kAudioUnitProperty_ParameterInfo, kAudioUnitScope_Global, (AudioUnitElement)inParameterID, &parameterInfo, &dataSize);
	if (result == noErr)
	{
		CFStringRef parameterName = parameterInfo.cfNameString;
		UInt32 flags = parameterInfo.flags;
		if ( (parameterName != NULL) && (flags & kAudioUnitParameterFlag_HasCFNameString) )
		{
			CFStringRef resultString = parameterName;
			if (! (flags & kAudioUnitParameterFlag_CFNameRelease) )
				CFRetain(resultString);
			return resultString;
		}
		else
		{
			return CFStringCreateWithCString(kCFAllocatorDefault, parameterInfo.name, kCFStringEncodingUTF8);
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
CFArrayRef DFX_CopyAUParameterValueStrings(AudioUnit inAUInstance, long inParameterID)
{
	if (inAUInstance == NULL)
		return NULL;
	CFArrayRef strings = NULL;
	UInt32 dataSize = sizeof(strings);
	ComponentResult result = AudioUnitGetProperty(inAUInstance, kAudioUnitProperty_ParameterValueStrings, kAudioUnitScope_Global, (AudioUnitElement)inParameterID, &strings, &dataSize);
	if (result == noErr)
		return strings;
	else
		return NULL;
}

//-----------------------------------------------------------------------------
void TurntablistEditor::HandleParameterChange(long inParameterID, float inValue)
{
	long value_i = (inValue >= 0.0f) ? (long)(inValue + 0.001f) : (long)(inValue - 0.001f);

	if ( (inParameterID == kParam_ScratchMode) && (scratchSpeedKnob != NULL) )
	{
		long newParamID = (value_i == kScratchMode_Scrub) ? kParam_ScratchSpeed_scrub : kParam_ScratchSpeed_spin;
		scratchSpeedKnob->setParameterID(newParamID);
	}

	if (allParamsTextDisplay == NULL)
		return;
	DGControl * control = getCurrentControl_clicked();
//	if ( (control == NULL) && (inParameterID == kParam_ScratchAmount) )
//		goto processDisplayText;
	if (control != NULL)
	{
		if ( !(control->isParameterAttached()) )
			return;
		if (control->getParameterID() != inParameterID)
			return;
	}

//processDisplayText:
	CFStringRef universalDisplayText = NULL;
	CFAllocatorRef cfAllocator = kCFAllocatorDefault;
	switch (inParameterID)
	{
		case kParam_ScratchAmount:
			{
				// XXX float2string(m_fPlaySampleRate, text);
				CFStringRef format = (inValue > 0.0f) ? CFSTR("%+.3f") : CFSTR("%.3f");
				universalDisplayText = CFStringCreateWithFormat(cfAllocator, NULL, format, inValue);
			}
			break;
		case kParam_ScratchSpeed_scrub:
			universalDisplayText = CFStringCreateWithFormat(cfAllocator, NULL, CFSTR("%.3f  second%s"), inValue, (fabsf(inValue) > 1.0f) ? "s" : "");
			break;
		case kParam_ScratchSpeed_spin:
			universalDisplayText = CFStringCreateWithFormat(cfAllocator, NULL, CFSTR("%.3f x"), inValue);
			break;
		case kParam_SpinUpSpeed:
		case kParam_SpinDownSpeed:
			universalDisplayText = CFStringCreateWithFormat(cfAllocator, NULL, CFSTR("%.4f"), inValue);
			break;
		case kParam_PitchShift:
			inValue = inValue*0.01f * getparameter_f(kParam_PitchRange);
			universalDisplayText = CFStringCreateWithFormat(cfAllocator, NULL, CFSTR("%+.2f  semitone%s"), inValue, (fabsf(inValue) > 1.0f) ? "s" : "");
			break;
		case kParam_PitchRange:
			universalDisplayText = CFStringCreateWithFormat(cfAllocator, NULL, CFSTR("%.2f  semitone%s"), inValue, (fabsf(inValue) > 1.0f) ? "s" : "");
			break;
		case kParam_RootKey:
			universalDisplayText = CFStringCreateWithCString(cfAllocator, DFX_GetNameForMIDINote(value_i), kDFX_DefaultCStringEncoding);
			break;

#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
		case kParam_Volume:
			if (inValue <= 0.0f)
			{
				const UniChar minusInfinity[] = { '-', 0x221E, ' ', ' ', 'd', 'B' };
				universalDisplayText = CFStringCreateWithCharacters(cfAllocator, minusInfinity, sizeofA(minusInfinity));
			}
			else
			{
				#define DB_FORMAT_STRING	"%.2f  dB"
				CFStringRef format = CFSTR(DB_FORMAT_STRING);
				if (inValue > 1.0f)
					format = CFSTR("+"DB_FORMAT_STRING);
				#undef DB_FORMAT_STRING
				universalDisplayText = CFStringCreateWithFormat(cfAllocator, NULL, format, linear2dB(inValue));
			}
			break;
#endif

		case kParam_KeyTracking:
		case kParam_Loop:
		case kParam_Power:
		case kParam_NotePowerTrack:
#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
		case kParam_Mute:
#endif
			if (value_i)
				universalDisplayText = CFSTR("on");
			else
				universalDisplayText = CFSTR("off");
			CFRetain(universalDisplayText);
			break;

		case kParam_Direction:
		case kParam_ScratchMode:
		case kParam_NoteMode:
			{
				CFArrayRef valueStrings = DFX_CopyAUParameterValueStrings(GetEditAudioUnit(), inParameterID);
				if (valueStrings != NULL)
				{
					universalDisplayText = (CFStringRef) CFArrayGetValueAtIndex(valueStrings, value_i);
					if (universalDisplayText != NULL)
						CFRetain(universalDisplayText);
					CFRelease(valueStrings);
				}
			}
			break;

		default:
			break;
	}

	if (universalDisplayText != NULL)
	{
		CFStringRef parameterNameString = DFX_CopyAUParameterName(GetEditAudioUnit(), inParameterID);
		if (parameterNameString != NULL)
		{
			// split the string after the first " (", if it has that, to eliminate the parameter names with sub-specifications 
			// (e.g. the 2 scratch speed parameters)
			CFArrayRef nameArray = CFStringCreateArrayBySeparatingStrings(cfAllocator, parameterNameString, CFSTR(" ("));
			if (nameArray != NULL)
			{
				if (CFArrayGetCount(nameArray) > 1)
				{
					CFStringRef truncatedName = (CFStringRef) CFArrayGetValueAtIndex(nameArray, 0);
					if (truncatedName != NULL)
					{
						CFRelease(parameterNameString);
						CFRetain(truncatedName);
						parameterNameString = truncatedName;
					}
				}
				CFRelease(nameArray);
			}
			
			CFStringRef fullDisplayText = CFStringCreateWithFormat(cfAllocator, NULL, CFSTR("%@:  %@"), parameterNameString, universalDisplayText);
			if (fullDisplayText != NULL)
			{
				CFRelease(universalDisplayText);
				universalDisplayText = fullDisplayText;
			}
			CFRelease(parameterNameString);
		}

		allParamsTextDisplay->setCFText(universalDisplayText);
		CFRelease(universalDisplayText);
	}
}

//-----------------------------------------------------------------------------
void TurntablistEditor::idle()
{
	// It's better to do this outside of the drag event handler, so I use the idle loop to delay its occurrence.  
	// Otherwise the alert dialog's run loop will block (and potentially timeout) the drag event callback.
	if (dragAudioFileError != noErr)
		DFX_NotifyAudioFileLoadError(dragAudioFileError, dragAudioFileRef);
	dragAudioFileError = noErr;
}
