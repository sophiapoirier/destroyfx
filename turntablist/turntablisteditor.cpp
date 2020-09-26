#include "turntablist.h"

#include "dfxgui.h"
#include "dfxguislider.h"
#include "dfxguibutton.h"
#include "dfxguidisplay.h"

#include <c.h>  // for sizeofA



const float kTurntablistFontSize = 10.0f;

//-----------------------------------------------------------------------------
enum {
	// positions
	kPitchShiftFaderX = 31,
	kScratchAmountFaderX = 166,
	kFaderY = 249,
	kFaderWidth = 22,
	kFaderHeight = 197,

	kFileNameX = 31,
	kFileNameY = 23 + 3,
	kFileNameWidth = 158,
	kFileNameHeight = 15,

	kDisplayX = 31,
	kDisplayY = 459,
	kDisplayWidth = 122,
	kDisplayHeight = 14,

	kColumn1 = 25,
	kColumn2 = 70,
	kColumn2k = 71,
	kColumn3 = 115,
	kColumn4 = 160,

	kPlayButtonX = kColumn2,
	kPlayButtonY = 241,

	kLoadButtonX = kColumn4,
	kLoadButtonY = 64,

	kMidiLearnX = kColumn2,
	kMidiLearnY = 418,

	kHelpX = 119,
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
class ScratchaEditor : public DfxGuiEditor
{
public:
	ScratchaEditor(AudioUnitCarbonView inInstance);
	virtual ~ScratchaEditor();

	virtual long open();

	OSStatus HandleLoadButton();
	ComponentResult LoadAudioFile(const FSRef & inAudioFileRef);
	ComponentResult HandleAudioFileChange();
	void FileOpenDialogFinished();
	ComponentResult HandlePlayButton(bool inPlay);
	ComponentResult HandlePlayChange();
	void HandleMidiLearnButton(bool inLearn);
	void HandleParameterChange(long inParameterID, float inValue);

private:
	void SetFileNameDisplay(CFStringRef inDisplayText);

	DGStaticTextDisplay * audioFileNameDisplay;
	DGStaticTextDisplay * allParamsTextDisplay;
	DGButton * playButton;
	DGButton * midiLearnButton;
	DGAnimation * scratchSpeedKnob;

	AUEventListenerRef propertyEventListener;
	AudioUnitEvent audioFilePropertyAUEvent;
	AudioUnitEvent playPropertyAUEvent;

	AUParameterListenerRef parameterListener;
	AudioUnitParameter allParamsAUP;

	NavDialogRef audioFileOpenDialog;
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

	virtual void mouseUp(float inXpos, float inYpos, unsigned long inKeyModifiers)
	{
		DGSlider::mouseUp(inXpos, inYpos, inKeyModifiers);

		SInt32 min = GetControl32BitMinimum(carbonControl);
		SInt32 max = GetControl32BitMaximum(carbonControl);
		SetControl32BitValue(carbonControl, ((max - min) / 2) + min);

		((ScratchaEditor*)getDfxGuiEditor())->HandleParameterChange(getParameterID(), 0.0f);
	}
};



#pragma mark -
#pragma mark callbacks

//-----------------------------------------------------------------------------
void LoadButtonProc(long inValue, void * inTurntablistEditor)
{
	if (inTurntablistEditor != NULL)
		((ScratchaEditor*)inTurntablistEditor)->HandleLoadButton();
}

//-----------------------------------------------------------------------------
void PlayButtonProc(long inValue, void * inTurntablistEditor)
{
	if (inTurntablistEditor != NULL)
		((ScratchaEditor*)inTurntablistEditor)->HandlePlayButton( (inValue == 0) ? false : true );
}

//-----------------------------------------------------------------------------
void MidiLearnButtonProc(long inValue, void * inTurntablistEditor)
{
	if (inTurntablistEditor != NULL)
		((ScratchaEditor*)inTurntablistEditor)->HandleMidiLearnButton( (inValue == 0) ? false : true );
}

//-----------------------------------------------------------------------------
void HelpButtonProc(long inValue, void * inTurntablistEditor)
{
//	if (inValue > 0)
		launch_documentation();
}

//-----------------------------------------------------------------------------
void AboutButtonProc(long inValue, void * inTurntablistEditor)
{
	if (inValue > 0)
	{
		launch_url(DESTROYFX_URL);
/*
		CFBundleRef pluginBundleRef = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));
		if ( (pluginBundleRef != NULL) && (HIAboutBox != NULL) )
		{
			CFMutableDictionaryRef aboutDict = CFDictionaryCreateMutable(kCFAllocatorDefault, 3, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
			if (aboutDict != NULL)
			{
				CFStringRef valueString = (CFStringRef) CFBundleGetValueForInfoDictionaryKey(pluginBundleRef, CFSTR("CFBundleName"));
				if (valueString != NULL)
					CFDictionaryAddValue(aboutDict, (const void*)kHIAboutBoxNameKey, (const void*)valueString);

				valueString = (CFStringRef) CFBundleGetValueForInfoDictionaryKey(pluginBundleRef, CFSTR("CFBundleVersion"));
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
	ScratchaEditor * editor = (ScratchaEditor*) inCallbackRefCon;
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
static void TurntablistOldStylePropertyListenerProc(void * inRefCon, AudioUnit inComponentInstance, AudioUnitPropertyID inPropertyID, AudioUnitScope inScope, AudioUnitElement inElement)
{
	ScratchaEditor * editor = (ScratchaEditor*) inRefCon;
	if (editor != NULL)
	{
		if (inPropertyID == kTurntablistProperty_AudioFile)
			editor->HandleAudioFileChange();
		else if (inPropertyID == kTurntablistProperty_Play)
			editor->HandlePlayChange();
	}
}

//-----------------------------------------------------------------------------
static void TurntablistParameterListenerProc(void * inRefCon, void * inObject, const AudioUnitParameter * inParameter, Float32 inValue)
{
	if ( (inObject == NULL) || (inParameter == NULL) )
		return;

	((ScratchaEditor*)inObject)->HandleParameterChange((long)(inParameter->mParameterID), inValue);
}




#pragma mark -
#pragma mark initialization

//-----------------------------------------------------------------------------
COMPONENT_ENTRY(ScratchaEditor);

//-----------------------------------------------------------------------------
// ScratchaEditor class implementation
//-----------------------------------------------------------------------------
ScratchaEditor::ScratchaEditor(AudioUnitCarbonView inInstance)
:	DfxGuiEditor(inInstance)
{
	audioFileNameDisplay = NULL;
	allParamsTextDisplay = NULL;
	playButton = NULL;
	scratchSpeedKnob = NULL;
	midiLearnButton = NULL;

	propertyEventListener = NULL;
	parameterListener = NULL;

	audioFileOpenDialog = NULL;
}

//-----------------------------------------------------------------------------
ScratchaEditor::~ScratchaEditor()
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
	// otherwise, if the UI was opened, then we did an old style property listener
	else if (GetEditAudioUnit() != NULL)
	{
		AudioUnitRemovePropertyListener(GetEditAudioUnit(), kTurntablistProperty_AudioFile, TurntablistOldStylePropertyListenerProc);
		AudioUnitRemovePropertyListener(GetEditAudioUnit(), kTurntablistProperty_Play, TurntablistOldStylePropertyListenerProc);
	}
	propertyEventListener = NULL;

	if (parameterListener != NULL)
	{
		for (AudioUnitParameterID i=0; i < kNumParams; i++)
		{
			allParamsAUP.mParameterID = i;
			AUListenerRemoveParameter(parameterListener, this, &allParamsAUP);
		}
		AUListenerDispose(parameterListener);
	}
	parameterListener = NULL;

	if (audioFileOpenDialog != NULL)
		NavDialogDispose(audioFileOpenDialog);
	audioFileOpenDialog = NULL;
}

//-----------------------------------------------------------------------------
long ScratchaEditor::open()
{
	// background image
	DGImage * gBackground = new DGImage("background.png", this);
	SetBackgroundImage(gBackground);

	DGImage * gSliderHandle = new DGImage("slider-handle.png", this);
	DGImage * gKnob = new DGImage("knob.png", this);
	DGImage * gOnOffButton = new DGImage("on-off-button.png", this);
	DGImage * gOnOffButton_green = new DGImage("on-off-button-green.png", this);
	DGImage * gDirectionButton = new DGImage("direction-button.png", this);
	DGImage * gLoopButton = new DGImage("loop-button.png", this);
	DGImage * gNoteModeButton = new DGImage("note-mode-button.png", this);
	DGImage * gScratchModeButton = new DGImage("scratch-mode-button.png", this);
	DGImage * gHelpButton = new DGImage("help-button.png", this);


	// create controls
	DGRect pos;
	CFStringRef helpText;
	CFBundleRef pluginBundleRef = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));


	// buttons
	DGButton * button;

	pos.set(kColumn1, 123, gOnOffButton->getWidth(), gOnOffButton->getHeight()/2);
	button = new DGButton(this, kPower, &pos, gOnOffButton, 2, kDGButtonType_incbutton);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("this switches the turntable power on or off"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Power parameter"));
	button->setHelpText(helpText);
	CFRelease(helpText);

	pos.set(kColumn3, 182, gOnOffButton->getWidth(), gOnOffButton->getHeight()/2);
	button = new DGButton(this, kMute, &pos, gOnOffButton, 2, kDGButtonType_incbutton);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("this mutes the audio output"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Mute parameter"));
	button->setHelpText(helpText);
	CFRelease(helpText);

	pos.set(kColumn2, 300, gDirectionButton->getWidth(), gDirectionButton->getHeight()/2);
	button = new DGButton(this, kDirection, &pos, gDirectionButton, 2, kDGButtonType_incbutton);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("this changes playback direction of the audio sample, regular or reverse"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Direction parameter"));
	button->setHelpText(helpText);
	CFRelease(helpText);

	pos.set(kColumn3, 241, gNoteModeButton->getWidth(), gNoteModeButton->getHeight()/2);
	button = new DGButton(this, kNoteMode, &pos, gNoteModeButton, 2, kDGButtonType_incbutton);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("This toggles between \"reset mode\" (notes restart playback from the beginning of the audio sample) and \"resume mode\" (notes trigger playback from where the audio sample last stopped)"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Note Mode parameter"));
	button->setHelpText(helpText);
	CFRelease(helpText);

	pos.set(kColumn3, 300, gLoopButton->getWidth(), gLoopButton->getHeight()/2);
	button = new DGButton(this, kLoop, &pos, gLoopButton, 2, kDGButtonType_incbutton);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("if you enable this, the audio sample playback will continuously loop"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Loop parameter"));
	button->setHelpText(helpText);
	CFRelease(helpText);

	pos.set(kColumn4, 123, gOnOffButton->getWidth(), gOnOffButton->getHeight()/2);
	button = new DGButton(this, kNotePowerTrack, &pos, gOnOffButton, 2, kDGButtonType_incbutton);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("enabling this will cause note-on and note-off messages to be mapped to turntable power on and off for an interesting effect"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Note-Power Track parameter"));
	button->setHelpText(helpText);
	CFRelease(helpText);

	pos.set(kColumn2, 360, gOnOffButton->getWidth(), gOnOffButton->getHeight()/2);
	button = new DGButton(this, kKeyTracking, &pos, gOnOffButton, 2, kDGButtonType_incbutton);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("this switches key tracking on or off (key tracking means that the pitch and speed of the audio sample playback are transposed in relation to the pitch of the MIDI note)"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Key Tracking parameter"));
	button->setHelpText(helpText);
	CFRelease(helpText);

	pos.set(kColumn3, 360, gScratchModeButton->getWidth(), gScratchModeButton->getHeight()/2);
	button = new DGButton(this, kScratchMode, &pos, gScratchModeButton, 2, kDGButtonType_incbutton);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("this toggles between scrub mode and spin mode, which affects the behavior of the Scratch Amount parameter"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Scratch Mode parameter"));
	button->setHelpText(helpText);
	CFRelease(helpText);

	pos.set(kLoadButtonX, kLoadButtonY, gOnOffButton->getWidth(), gOnOffButton->getHeight()/2);
	button = new DGButton(this, &pos, gOnOffButton, 2, kDGButtonType_pushbutton);
	button->setUserReleaseProcedure(LoadButtonProc, this);
	button->setUseReleaseProcedureOnlyAtEndWithNoCancel(true);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("find an audio file to load up onto the \"turntable\""), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Open Audio File button"));
	button->setHelpText(helpText);
	CFRelease(helpText);

	pos.set(kPlayButtonX, kPlayButtonY, gOnOffButton_green->getWidth(), gOnOffButton_green->getHeight()/2);
	playButton = new DGButton(this, &pos, gOnOffButton_green, 2, kDGButtonType_incbutton);
	playButton->setUserProcedure(PlayButtonProc, this);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("use this to start or stop the audio sample playback"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Play button"));
	playButton->setHelpText(helpText);
	CFRelease(helpText);

	pos.set(kMidiLearnX, kMidiLearnY, gOnOffButton->getWidth(), gOnOffButton->getHeight()/2);
	midiLearnButton = new DGButton(this, &pos, gOnOffButton, 2, kDGButtonType_incbutton);
	midiLearnButton->setUserProcedure(MidiLearnButtonProc, this);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("This switches MIDI learn mode on or off.  When MIDI learn is on, you can click on a parameter control to enable that parameter as the \"learner\" for incoming MIDI CC messages."), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the MIDI Learn button"));
	midiLearnButton->setHelpText(helpText);
	CFRelease(helpText);

	pos.set(kAboutSplashX, kAboutSplashY, kAboutSplashWidth, kAboutSplashHeight);
	button = new DGButton(this, &pos, NULL, 2, kDGButtonType_incbutton);
	button->setUserProcedure(AboutButtonProc, this);
//	button->setHelpText(CFSTR("click here to go to the "DESTROYFX_NAME_STRING" web site"));
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("click here to go to the Destroy FX web site"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the About hot-spot"));
	button->setHelpText(helpText);
	CFRelease(helpText);

	pos.set(kHelpX, kHelpY, gHelpButton->getWidth(), gHelpButton->getHeight()/2);
	button = new DGButton(this, &pos, gHelpButton, 2, kDGButtonType_pushbutton);
	button->setUserReleaseProcedure(HelpButtonProc, this);
	button->setUseReleaseProcedureOnlyAtEndWithNoCancel(true);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("view the full manual"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the help button"));
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

	pos.set(26, 183, gKnob->getWidth(), gKnob->getHeight()/kKnobFrames);
	knob = new DGAnimation(this, kPitchRange, &pos, gKnob, kKnobFrames);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("this controls the range of pitch adjustment values that the Pitch Shift parameter offers"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Pitch Range parameter"));
	knob->setHelpText(helpText);
	CFRelease(helpText);

	long scratchSpeedParam = (getparameter_i(kScratchMode) == kScratchMode_scrub) ? kScratchSpeed_scrub : kScratchSpeed_spin;
	pos.set(kColumn4, 183, gKnob->getWidth(), gKnob->getHeight()/kKnobFrames);
	scratchSpeedKnob = new DGAnimation(this, scratchSpeedParam, &pos, gKnob, kKnobFrames);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("this sets the speed of the scratching effect that the Scratch Amount parameter produces"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Scratch Speed parameter"));
	scratchSpeedKnob->setHelpText(helpText);
	CFRelease(helpText);

	pos.set(kColumn2k, 124, gKnob->getWidth(), gKnob->getHeight()/kKnobFrames);
	knob = new DGAnimation(this, kSpinUpSpeed, &pos, gKnob, kKnobFrames);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("this controls how quickly the audio playback \"spins up\" when the turntable power turns on"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Spin Up Speed parameter"));
	knob->setHelpText(helpText);
	CFRelease(helpText);

	pos.set(116, 124, gKnob->getWidth(), gKnob->getHeight()/kKnobFrames);
	knob = new DGAnimation(this, kSpinDownSpeed, &pos, gKnob, kKnobFrames);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("this controls how quickly the audio playback \"spins down\" when the turntable power turns off"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Spin Down Speed parameter"));
	knob->setHelpText(helpText);
	CFRelease(helpText);

	pos.set(kColumn2k, 183, gKnob->getWidth(), gKnob->getHeight()/kKnobFrames);
	knob = new DGAnimation(this, kVolume, &pos, gKnob, kKnobFrames);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("this controls the overall volume of the audio output"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Volume parameter"));
	knob->setHelpText(helpText);
	CFRelease(helpText);


	// sliders
	DGSlider * slider;

	// pitch shift
	pos.set(31, kFaderY, kFaderWidth, kFaderHeight);
	slider = new DGSlider(this, kPitchShift, &pos, kDGSliderAxis_vertical, gSliderHandle, NULL);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("changes the audio playback pitch between +/- the Pitch Range value"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Pitch Shift parameter"));
	slider->setHelpText(helpText);
	CFRelease(helpText);

	// scratch amount
	pos.set(kScratchAmountFaderX, kFaderY, kFaderWidth, kFaderHeight);
	slider = new TurntablistScratchSlider(this, kScratchAmount, &pos, kDGSliderAxis_vertical, gSliderHandle);
	helpText = CFCopyLocalizedStringFromTableInBundle(CFSTR("This slider is what does the actual scratching.  In scrub mode, the slider represents time.  In spin mode, the slider represents forward and backward speed."), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Scratch Amount parameter"));
	slider->setHelpText(helpText);
	CFRelease(helpText);


	// text displays

	// universal parameter value display
	pos.set(kDisplayX, kDisplayY, kFileNameWidth, kFileNameHeight);
	allParamsTextDisplay = new DGStaticTextDisplay(this, &pos, NULL, kDGTextAlign_center, kTurntablistFontSize, kWhiteDGColor, NULL);

	// audio file name display
	pos.set(kFileNameX, kFileNameY, kFileNameWidth, kFileNameHeight);
	audioFileNameDisplay = new DGStaticTextDisplay(this, &pos, NULL, kDGTextAlign_center, kTurntablistFontSize, kWhiteDGColor, NULL);
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
	else
	{
		AudioUnitAddPropertyListener(GetEditAudioUnit(), kTurntablistProperty_AudioFile, TurntablistOldStylePropertyListenerProc, this);
		AudioUnitAddPropertyListener(GetEditAudioUnit(), kTurntablistProperty_Play, TurntablistOldStylePropertyListenerProc, this);
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
		for (AudioUnitParameterID i=0; i < kNumParams; i++)
		{
			allParamsAUP.mParameterID = i;
			AUListenerAddParameter(parameterListener, this, &allParamsAUP);
		}
	}



	return noErr;
}



#pragma mark -
#pragma mark audio file chooser

//-----------------------------------------------------------------------------
CFStringRef CopyFileNameString(const FSRef & inFileRef)
{
	CFStringRef fileName = NULL;
	OSStatus status = LSCopyDisplayNameForRef(&inFileRef, &fileName);
	if (status == noErr)
		return fileName;
	else
		return NULL;
}

#include "sndfile.h"	// for the libsndfile error code constants
//-----------------------------------------------------------------------------
OSStatus NotifyAudioFileLoadError(OSStatus inErrorCode, const FSRef & inAudioFileRef)
{
	CFBundleRef pluginBundleRef = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));
	if (pluginBundleRef == NULL)
		return coreFoundationUnknownErr;

	AlertStdCFStringAlertParamRec alertParams;
	GetStandardAlertDefaultParams(&alertParams, kStdCFStringAlertVersionOne);
	alertParams.movable = true;

	CFStringRef titleString = CFCopyLocalizedStringFromTableInBundle(CFSTR("The file \"%@\" could not be loaded."), 
								CFSTR("Localizable"), pluginBundleRef, 
								CFSTR("title for the dialog telling you that the audio file could not be loaded"));
	CFStringRef audioFileName = CopyFileNameString(inAudioFileRef);
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
	CFStringRef errorDescriptionString;
	switch (inErrorCode)
	{
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
	}

	DialogRef alertDialog = NULL;
	OSStatus status = CreateStandardAlert(kAlertNoteAlert, titleString, messageString, &alertParams, &alertDialog);
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
pascal void OpenAudioFileNavEventHandler(NavEventCallbackMessage inCallbackSelector, NavCBRecPtr inCallbackParams, NavCallBackUserData inUserData)
{
	ScratchaEditor * editor = (ScratchaEditor*) inUserData;
	NavDialogRef dialog = inCallbackParams->context;

	switch (inCallbackSelector)
	{
		case kNavCBStart:
			break;

		case kNavCBTerminate:
			if (editor != NULL)
				editor->FileOpenDialogFinished();
			// XXX why does this crash Rax and SynthTest?
//			if (dialog != NULL)
//				NavDialogDispose(dialog);
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
								NotifyAudioFileLoadError(status, audioFileRef);
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
#include <AudioToolbox/AudioFile.h>
static OSType * gSupportedAudioFileTypeCodes = NULL;
static UInt32 gNumSupportedAudioFileTypeCodes = 0;
static CFMutableArrayRef gSupportedAudioFileExtensions = NULL;
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

//-----------------------------------------------------------------------------
// This is the filter callback for the custom open audio file Nav Services dialog.  
// It handles filtering out non-audio files from the file list display.  
// They will still appear in the list, but can not be selected.
// A response of true means "allow the file" and false means "disallow the file."
pascal Boolean OpenAudioFileNavFilterProc(AEDesc * inItem, void * inInfo, void * inUserData, NavFilterModes inFilterMode)
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
					LSRequestedInfo lsInfoFlags = kLSRequestExtension | kLSRequestTypeCreator;
					LSItemInfoRecord lsItemInfo;
					status = LSCopyItemInfoForRef(&fileRef, lsInfoFlags, &lsItemInfo);
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
				}
			}
			AEDisposeDesc(&fsrefDesc);
		}
	}

	return result;
}

// unique identifier of our audio file open dialog for Navigation Services
const UInt32 kTurntablistAudioFileOpenNavDialogKey = PLUGIN_ID;

//-----------------------------------------------------------------------------
void InitializeSupportedAudioFileTypesArrays()
{
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
//fprintf(stderr, "\n\t%.4s\n", (char*)(gSupportedAudioFileTypeCodes+i));
//for (CFIndex j=0 ; j < CFArrayGetCount(extensionsArray); j++) CFShow(CFArrayGetValueAtIndex(extensionsArray, j));
			}
		}
	}
}

//-----------------------------------------------------------------------------
OSStatus ScratchaEditor::HandleLoadButton()
{
	// we already have a file open dialog running
	if (audioFileOpenDialog != NULL)
		return kNavWrongDialogStateErr;

	InitializeSupportedAudioFileTypesArrays();

	NavDialogCreationOptions dialogOptions;
	OSStatus status = NavGetDefaultDialogCreationOptions(&dialogOptions);
	if (status != noErr)
		return status;
	dialogOptions.optionFlags &= ~kNavAllowMultipleFiles;	// disallow multiple file selection
	// this gives this dialog a unique identifier so that it has independent remembered settings for the calling app
	dialogOptions.preferenceKey = kTurntablistAudioFileOpenNavDialogKey;
	dialogOptions.modality = kWindowModalityNone;
	dialogOptions.clientName = CFSTR(PLUGIN_NAME_STRING);
	dialogOptions.windowTitle = CFSTR("Open: "PLUGIN_NAME_STRING" Audio File");

	// create and run a GetFile dialog to allow the user to find and choose an audio file to load
	NavEventUPP eventProc = NewNavEventUPP(OpenAudioFileNavEventHandler);
	NavObjectFilterUPP filterProc = NewNavObjectFilterUPP(OpenAudioFileNavFilterProc);
	audioFileOpenDialog = NULL;
	status = NavCreateGetFileDialog(&dialogOptions, NULL, eventProc, NULL, filterProc, (void*)this, &audioFileOpenDialog);
	if (status == noErr)
	{
		status = NavDialogRun(audioFileOpenDialog);
	}
	if (eventProc != NULL)
		DisposeRoutineDescriptor(eventProc);
	if (filterProc != NULL)
		DisposeRoutineDescriptor(filterProc);

	return status;
}

//-----------------------------------------------------------------------------
ComponentResult ScratchaEditor::LoadAudioFile(const FSRef & inAudioFileRef)
{
	ComponentResult result = AudioUnitSetProperty(GetEditAudioUnit(), kTurntablistProperty_AudioFile, 
												kAudioUnitScope_Global, (AudioUnitElement)0, 
												&inAudioFileRef, sizeof(inAudioFileRef));

	return result;
}

//-----------------------------------------------------------------------------
void ScratchaEditor::FileOpenDialogFinished()
{
	audioFileOpenDialog = NULL;
}



#pragma mark -
#pragma mark features

//-----------------------------------------------------------------------------
CFStringRef CopyNameAndVersionString()
{
	CFBundleRef pluginBundleRef = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));
	if (pluginBundleRef == NULL)
		return NULL;

	CFStringRef nameString = (CFStringRef) CFBundleGetValueForInfoDictionaryKey(pluginBundleRef, CFSTR("CFBundleName"));
	CFStringRef versionString = (CFStringRef) CFBundleGetValueForInfoDictionaryKey(pluginBundleRef, CFSTR("CFBundleVersion"));
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
ComponentResult ScratchaEditor::HandleAudioFileChange()
{
	FSRef audioFileRef;
	CFStringRef displayString = NULL;
	UInt32 dataSize = sizeof(audioFileRef);
	ComponentResult result = AudioUnitGetProperty(GetEditAudioUnit(), kTurntablistProperty_AudioFile, 
												kAudioUnitScope_Global, (AudioUnitElement)0, 
												&audioFileRef, &dataSize);
	if (result == noErr)
		displayString = CopyFileNameString(audioFileRef);

	if (displayString == NULL)
		displayString = CopyNameAndVersionString();

	if (displayString != NULL)
	{
		SetFileNameDisplay(displayString);
		CFRelease(displayString);
	}

	return result;
}

//-----------------------------------------------------------------------------
ComponentResult ScratchaEditor::HandlePlayButton(bool inPlay)
{
	if (allParamsTextDisplay != NULL)
	{
		CFStringRef displayString = (inPlay) ? CFSTR("playback:  Start") : CFSTR("playback:  Stop");
		allParamsTextDisplay->setCFText(displayString);
	}

	return AudioUnitSetProperty(GetEditAudioUnit(), kTurntablistProperty_Play, 
				kAudioUnitScope_Global, (AudioUnitElement)0, &inPlay, sizeof(inPlay));
}

//-----------------------------------------------------------------------------
ComponentResult ScratchaEditor::HandlePlayChange()
{
	if (playButton == NULL)
		return kAudioUnitErr_Uninitialized;

	bool play;
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
void ScratchaEditor::SetFileNameDisplay(CFStringRef inDisplayText)
{
	if (inDisplayText == NULL)
		return;

	if (audioFileNameDisplay != NULL)
		audioFileNameDisplay->setCFText(inDisplayText);
}

//-----------------------------------------------------------------------------
void ScratchaEditor::HandleMidiLearnButton(bool inLearn)
{
	if (allParamsTextDisplay != NULL)
	{
		CFStringRef displayString = (inLearn) ? CFSTR("MIDI learn:  On") : CFSTR("MIDI learn:  Off");
		allParamsTextDisplay->setCFText(displayString);
	}

	setmidilearning(inLearn);
}



#pragma mark -
#pragma mark parameter -> string conversions

//-----------------------------------------------------------------------------
CFStringRef CopyAUParameterName(AudioUnit inAUInstance, long inParameterID)
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
			if (flags & kAudioUnitParameterFlag_CFNameRelease)
				return parameterName;
			else
				return CFStringCreateCopy(kCFAllocatorDefault, parameterName);
		}
		else
		{
			return CFStringCreateWithCString(kCFAllocatorDefault, parameterInfo.name, kCFStringEncodingUTF8);
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
CFArrayRef CopyAUParameterValueStrings(AudioUnit inAUInstance, long inParameterID)
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
void ScratchaEditor::HandleParameterChange(long inParameterID, float inValue)
{
	long value_i = (long) (inValue + 0.001f);

	if ( (inParameterID == kScratchMode) && (scratchSpeedKnob != NULL) )
	{
		long newParamID = (value_i == kScratchMode_scrub) ? kScratchSpeed_scrub : kScratchSpeed_spin;
		scratchSpeedKnob->setParameterID(newParamID);
	}

	if (allParamsTextDisplay == NULL)
		return;
	DGControl * control = getCurrentControl_clicked();
//	if ( (control == NULL) && (inParameterID == kScratchAmount) )
//		goto processDisplayText;
	if (control == NULL)
		return;
	if ( !(control->isParameterAttached()) )
		return;
	if (control->getParameterID() != inParameterID)
		return;

//processDisplayText:
	CFStringRef universalDisplayText = NULL;
	CFAllocatorRef cfAllocator = kCFAllocatorDefault;
	switch (inParameterID)
	{
		case kScratchAmount:
			{
				// XXX float2string(m_fPlaySampleRate, text);
				CFStringRef format = (inValue > 0.0f) ? CFSTR("%+.3f") : CFSTR("%.3f");
				universalDisplayText = CFStringCreateWithFormat(cfAllocator, NULL, format, inValue);
			}
			break;
		case kScratchSpeed_scrub:
			universalDisplayText = CFStringCreateWithFormat(cfAllocator, NULL, CFSTR("%.3f  seconds"), inValue);
			break;
		case kScratchSpeed_spin:
			universalDisplayText = CFStringCreateWithFormat(cfAllocator, NULL, CFSTR("%.3f x"), inValue);
			break;
		case kSpinUpSpeed:
		case kSpinDownSpeed:
			universalDisplayText = CFStringCreateWithFormat(cfAllocator, NULL, CFSTR("%.4f"), inValue);
			break;
		case kPitchShift:
			inValue = inValue*0.01f * getparameter_f(kPitchRange);
			universalDisplayText = CFStringCreateWithFormat(cfAllocator, NULL, CFSTR("%+.2f  semitones"), inValue);
			break;
		case kPitchRange:
			universalDisplayText = CFStringCreateWithFormat(cfAllocator, NULL, CFSTR("%.2f  semitones"), inValue);
			break;

		case kVolume:
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

		case kKeyTracking:
		case kLoop:
		case kPower:
		case kNotePowerTrack:
		case kMute:
			if (value_i)
				universalDisplayText = CFStringCreateCopy(cfAllocator, CFSTR("On"));
			else
				universalDisplayText = CFStringCreateCopy(cfAllocator, CFSTR("Off"));
			break;

		case kDirection:
		case kScratchMode:
		case kNoteMode:
			{
				CFArrayRef valueStrings = CopyAUParameterValueStrings(GetEditAudioUnit(), inParameterID);
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
		CFStringRef parameterNameString = CopyAUParameterName(GetEditAudioUnit(), inParameterID);
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
