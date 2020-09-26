#include "turntablist.h"

#include "dfxgui.h"
#include "dfxguislider.h"
#include "dfxguibutton.h"
#include "dfxguidisplay.h"



const float kTurntablistFontSize = 10.0f;

//-----------------------------------------------------------------------------
enum {
	// positions
	kFaderWidth = 22,
	kFaderHeight = 199,

	kFaderX = 31, //20, //18,
	kFaderY = 248, //174, //10,
	kFaderInc = 18,

	kFileNameX = 31,
	kFileNameY = 23,
	kFileNameWidth = 158,  //30,
	kFileNameHeight = 15,

	kDisplayX = 31,
	kDisplayY = 459,
	kDisplayWidth = 122,  //30,
	kDisplayHeight = 14,

	kPlayButtonX = 70,
	kPlayButtonY = 241,

	kLoadButtonX = 160, //115,
	kLoadButtonY = 64,

#ifdef _MIDI_LEARN_
	kMidiLearnX = 71,
	kMidiLearnY = 419,
#endif

	kAboutSplashX = 13,
	kAboutSplashY = 64,
	kAboutSplashWidth = 137,
	kAboutSplashHeight = 28,

	kKnobFrames = 61
};



#pragma mark -
#pragma mark class definitions

//-----------------------------------------------------------------------------
class ScratchaEditor : public DfxGuiEditor
{
public:
	ScratchaEditor(AudioUnitCarbonView inInstance);
	virtual ~ScratchaEditor();

	virtual long open();

	OSStatus HandleLoadButton();
	ComponentResult LoadAudioFile(FSRef * inAudioFileRef);
	ComponentResult HandleAudioFileChange();
	ComponentResult HandlePlayButton(bool inPlay);

private:
	void SetFileNameDisplay(CFStringRef inDisplayText);
	void SetUniversalDisplayType(long index);

	DGStaticTextDisplay * audioFileNameDisplay;

	AUEventListenerRef propertyEventListener;
	AudioUnitEvent audioFilePropertyAUEvent;
};



//-----------------------------------------------------------------------------
class TurntablistPitchSlider : public DGSlider
{
public:
	TurntablistPitchSlider(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion, 
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
	}
};

//-----------------------------------------------------------------------------
class DGAnimation : public DGTextDisplay
{
public:
	DGAnimation(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion, 
				DGImage * inAnimationImage, long inNumAnimationFrames, DGImage * inBackground = NULL)
	:   DGTextDisplay(inOwnerEditor, inParamID, inRegion, NULL, NULL, inBackground), 
		animationImage(inAnimationImage), numAnimationFrames(inNumAnimationFrames)
	{
		if (numAnimationFrames < 1)
			numAnimationFrames = 1;
		setMouseDragRange(120.0f);  // number of pixels
	}

	virtual void draw(CGContextRef inContext, long inPortHeight)
	{
		if (backgroundImage != NULL)
			backgroundImage->draw(getBounds(), inContext, inPortHeight);
		if (animationImage != NULL)
		{
			SInt32 min = GetControl32BitMinimum(carbonControl);
			SInt32 max = GetControl32BitMaximum(carbonControl);
			SInt32 val = GetControl32BitValue(carbonControl);
			float valNorm = ((max-min) == 0) ? 0.0f : (float)(val-min) / (float)(max-min);
			long frameIndex = (long) ( valNorm * (float)(numAnimationFrames-1) );
			long yoff = frameIndex * (animationImage->getHeight() / numAnimationFrames);
			animationImage->draw(getBounds(), inContext, inPortHeight, 0, yoff);
		}
	}

protected:
	DGImage * animationImage;
	long numAnimationFrames;
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
void AboutButtonProc(long inValue, void * inTurntablistEditor)
{
	if ( (inTurntablistEditor != NULL) && (inValue > 0) )
		launch_documentation();
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
		}
	}
}

//-----------------------------------------------------------------------------
static void TurntablistPropertyListenerProc(void * inRefCon, AudioUnit inComponentInstance, AudioUnitPropertyID inPropertyID, AudioUnitScope inScope, AudioUnitElement inElement)
{
	ScratchaEditor * editor = (ScratchaEditor*) inRefCon;
	if (editor != NULL)
	{
		if (inPropertyID == kTurntablistProperty_AudioFile)
			editor->HandleAudioFileChange();
	}
}



#pragma mark -
#pragma mark string conversion callbacks

//-----------------------------------------------------------------------------
enum
{
	kSTfloat = 0,
	kSTdb,
	kSTlong,
	kSTonoff,
	kSTdir,
	kSTnotemode,
	kSTpamount,
	kSTprange
};

int giStringType;
int giIndex;

void SetUniversalType(int type)
{
	giStringType = type;
}

bool gbBlah = false;

float counter = 0.0f;

void SetDisplayIndex(long index)
{
	giIndex = index;
}

void float2StringConvert(float value, char *text)
{
	sprintf(text, "%.6f", value);
}

void db2StringConvert(float value, char* string)
{
	if(value <= 0)
//		strcpy(string, "   -°   ");
		strcpy(string, "  -oo   ");
	else
		float2StringConvert((float)(20. * log10(value)), string);
}

void long2StringConvert(long value, char *text)
{
	char string[32];
	
	if(value >= 100000000)
	{
		strcpy(text, " Huge!  ");
		return;
	}
	sprintf(string, "%7ld", value);
	string[8] = 0;
	strcpy(text, (char *)string);
}

void OffOnStringConvert(float value, char* string)
{
	if (value < .5)
		strcpy (string, "OFF");
	else
		strcpy (string, "ON");
}

void DirectionStringConvert(float value, char* string)
{
	if (value < .5)
		strcpy (string, "Forward");
	else
		strcpy (string, "Reverse");
}

void NoteModeStringConvert(float value, char* string)
{
	if (value < .5)
		strcpy (string, "Reset");
	else
		strcpy (string, "Resume");
}

void PitchRangeStringConvert(float value, char* string)
{
	float2StringConvert ((value*MAX_PITCH_RANGE*100), string);
}

void PitchAmountStringConvert(float value, char* string)
{
	float2StringConvert (((value-0.5f)*2.0f), string);
}

void UniversalStringConvert(float value, char * text)
{	
	switch(giStringType)
	{
		case(kSTfloat):		float2StringConvert(value, text); break;
		case(kSTdb):		db2StringConvert(value, text); break;
		case(kSTlong):		long2StringConvert((long)(value*127.0f), text); break;
		case(kSTonoff):		OffOnStringConvert(value, text); break;
		case(kSTdir):		DirectionStringConvert(value, text); break;
		case(kSTnotemode):	NoteModeStringConvert(value, text); break;
		case(kSTpamount):	PitchAmountStringConvert(value, text); break;
		case(kSTprange):	PitchRangeStringConvert(value, text); break;
		default:
			float2StringConvert(value, text);
	}

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
	propertyEventListener = NULL;
}

//-----------------------------------------------------------------------------
ScratchaEditor::~ScratchaEditor()
{
	// remove and dispose the new-style property listener, if we created it
	if (propertyEventListener != NULL)
	{
		if (AUEventListenerRemoveEventType != NULL)
			AUEventListenerRemoveEventType(propertyEventListener, this, &audioFilePropertyAUEvent);
		AUListenerDispose(propertyEventListener);
	}
	// otherwise, if the UI was opened, then we did an old style property listener
	else if (GetEditAudioUnit() != NULL)
		AudioUnitRemovePropertyListener(GetEditAudioUnit(), kTurntablistProperty_AudioFile, TurntablistPropertyListenerProc);
	propertyEventListener = NULL;
}

//-----------------------------------------------------------------------------
long ScratchaEditor::open()
{
	// install a property listener for the audio file property
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
		}
	}
	else
	{
		AudioUnitAddPropertyListener(GetEditAudioUnit(), kTurntablistProperty_AudioFile, TurntablistPropertyListenerProc, this);
	}


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


	// create controls
	DGRect pos;

	// buttons
	DGButton * button;

	pos.set(25, 123, gOnOffButton->getWidth(), gOnOffButton->getHeight()/2);
	button = new DGButton(this, kPower, &pos, gOnOffButton, 2, kDGButtonType_incbutton);

	pos.set(115, 182, gOnOffButton->getWidth(), gOnOffButton->getHeight()/2);
	button = new DGButton(this, kMute, &pos, gOnOffButton, 2, kDGButtonType_incbutton);

	pos.set(70, 300, gDirectionButton->getWidth(), gDirectionButton->getHeight()/2);
	button = new DGButton(this, kDirection, &pos, gDirectionButton, 2, kDGButtonType_incbutton);

	pos.set(115, 241, gNoteModeButton->getWidth(), gNoteModeButton->getHeight()/2);
	button = new DGButton(this, kNoteMode, &pos, gNoteModeButton, 2, kDGButtonType_incbutton);

	pos.set(115, 300, gLoopButton->getWidth(), gLoopButton->getHeight()/2);
	button = new DGButton(this, kLoopMode, &pos, gLoopButton, 2, kDGButtonType_incbutton);

	pos.set(160, 123, gOnOffButton->getWidth(), gOnOffButton->getHeight()/2);
	button = new DGButton(this, kNPTrack, &pos, gOnOffButton, 2, kDGButtonType_incbutton);

	pos.set(115, 418, gOnOffButton->getWidth(), gOnOffButton->getHeight()/2);
	button = new DGButton(this, kKeyTracking, &pos, gOnOffButton, 2, kDGButtonType_incbutton);

	pos.set(71, 360, gOnOffButton->getWidth(), gOnOffButton->getHeight()/2);
	button = new DGButton(this, kScratchMode, &pos, gOnOffButton, 2, kDGButtonType_incbutton);

	pos.set(kLoadButtonX, kLoadButtonY, gOnOffButton->getWidth (), gOnOffButton->getHeight()/2);
	button = new DGButton(this, &pos, gOnOffButton, 2, kDGButtonType_pushbutton);
	button->setUserReleaseProcedure(LoadButtonProc, this);
	button->setUseReleaseProcedureOnlyAtEndWithNoCancel(true);

	pos.set(kPlayButtonX, kPlayButtonY, gOnOffButton_green->getWidth (), gOnOffButton_green->getHeight()/2);
	button = new DGButton(this, &pos, gOnOffButton_green, 2, kDGButtonType_incbutton);
	button->setUserProcedure(PlayButtonProc, this);

	pos.set(kAboutSplashX, kAboutSplashY, kAboutSplashWidth, kAboutSplashHeight);
	button = new DGButton(this, &pos, NULL, 2, kDGButtonType_incbutton);
	button->setUserProcedure(AboutButtonProc, this);

	// knobs
	DGAnimation * knob;

	pos.set(26, 183, gKnob->getWidth(), gKnob->getHeight()/kKnobFrames);
	knob = new DGAnimation(this, kPitchRange, &pos, gKnob, kKnobFrames);

	pos.set(160, 183, gKnob->getWidth(), gKnob->getHeight()/kKnobFrames);
	knob = new DGAnimation(this, kScratchSpeed, &pos, gKnob, kKnobFrames);

	pos.set(116, 124, gKnob->getWidth(), gKnob->getHeight()/kKnobFrames);
	knob = new DGAnimation(this, kSpinUpSpeed, &pos, gKnob, kKnobFrames);

	pos.set(71, 124, gKnob->getWidth(), gKnob->getHeight()/kKnobFrames);
	knob = new DGAnimation(this, kSpinDownSpeed, &pos, gKnob, kKnobFrames);

	pos.set(71, 183, gKnob->getWidth(), gKnob->getHeight()/kKnobFrames);
	knob = new DGAnimation(this, kVolume, &pos, gKnob, kKnobFrames);

	// sliders
/*
	int minPos = kFaderY + 1;
	int maxPos = kFaderY + kFaderHeight - gSliderHandle->getHeight();
	CPoint offset(1, 0);
*/
	DGSlider * slider;

	// pitch amount
	pos.set(31, 248, kFaderWidth, kFaderHeight);
	slider = new DGSlider(this, kPitchAmount, &pos, kDGSliderAxis_vertical, gSliderHandle, NULL);
//	vsPitchAmount->setOffsetHandle(offset);
//	vsPitchAmount->setDefaultValue(0.75f);

	// scratch amount
	pos.set(166, 248, kFaderWidth, kFaderHeight);
	slider = new TurntablistPitchSlider(this, kScratchAmount, &pos, kDGSliderAxis_vertical, gSliderHandle);
//	vsScratchAmount->setOffsetHandle(offset);

/*
	// teDisplay
	size (kDisplayX, kDisplayY, kDisplayX + kFileNameWidth, kDisplayY + kFileNameHeight);
	teDisplay = new CTextEdit(size, this, kCenterText, 0, bmpDisplayBackground, 0); // kDoubleClickStyle); 
	teDisplay->setFont (kNormalFontSmall);
	teDisplay->setFontColor (kWhiteCColor);
	teDisplay->setBackColor (kBlackCColor);
	teDisplay->setFrameColor (kBlueCColor);
	teDisplay->setValue (0.0f);
	teDisplay->setTextTransparency(true); 
*/

	pos.set(kFileNameX, kFileNameY, kFileNameWidth, kFileNameHeight);
	audioFileNameDisplay = new DGStaticTextDisplay(this, &pos, NULL, kDGTextAlign_center, kTurntablistFontSize, kWhiteDGColor, NULL);
	HandleAudioFileChange();


	return noErr;
}



#pragma mark -
#pragma mark audio file chooser

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
			if (dialog != NULL)
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
//				gLoadAudioFileDialogResult = userCanceledErr;
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
							status = editor->LoadAudioFile(&audioFileRef);
						else
							status = paramErr;
					}
				}
				NavDisposeReply(&reply);
			}
//			gLoadAudioFileDialogResult = status;
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
							bool processedExtension = false;
							if (lsItemInfo.extension != NULL)
							{
								long numExtensions = sizeof(gSupportedAudioFileExtensions) / sizeof(*gSupportedAudioFileExtensions);
								for (long i=0; i < numExtensions; i++)
								{
									if ( CFStringCompare(lsItemInfo.extension, gSupportedAudioFileExtensions[i], kCFCompareCaseInsensitive) == kCFCompareEqualTo )
									{
										result = true;
										processedExtension = true;
										break;
									}
								}
								CFRelease(lsItemInfo.extension);
							}
							if (!processedExtension)
							{
								long numTypeCodes = sizeof(gSupportedAudioFileTypeCodes) / sizeof(*gSupportedAudioFileTypeCodes);
								for (long i=0; i < numTypeCodes; i++)
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
OSStatus ScratchaEditor::HandleLoadButton()
{
	NavDialogCreationOptions dialogOptions;
	ComponentResult result = NavGetDefaultDialogCreationOptions(&dialogOptions);
	if (result != noErr)
		return result;
	dialogOptions.optionFlags &= ~kNavAllowMultipleFiles;	// disallow multiple file selection
	// this gives this dialog a unique identifier so that it has independent remembered settings for the calling app
	dialogOptions.preferenceKey = kTurntablistAudioFileOpenNavDialogKey;
	dialogOptions.modality = kWindowModalityNone;
	dialogOptions.clientName = CFSTR(PLUGIN_NAME_STRING);
	dialogOptions.windowTitle = CFSTR("Open: "PLUGIN_NAME_STRING" Audio File");

	// create and run a GetFile dialog to allow the user to find and choose an audio file to load
	NavEventUPP eventProc = NewNavEventUPP(OpenAudioFileNavEventHandler);
	NavObjectFilterUPP filterProc = NewNavObjectFilterUPP(OpenAudioFileNavFilterProc);
	NavDialogRef dialog = NULL;
	result = NavCreateGetFileDialog(&dialogOptions, NULL, eventProc, NULL, filterProc, (void*)this, &dialog);
	if (result == noErr)
	{
		result = NavDialogRun(dialog);
	}
	if (eventProc != NULL)
		DisposeRoutineDescriptor(eventProc);
	if (filterProc != NULL)
		DisposeRoutineDescriptor(filterProc);

	return result;
}

//-----------------------------------------------------------------------------
ComponentResult ScratchaEditor::LoadAudioFile(FSRef * inAudioFileRef)
{
	if (inAudioFileRef == NULL)
		return paramErr;

	ComponentResult result = AudioUnitSetProperty(GetEditAudioUnit(), kTurntablistProperty_AudioFile, 
												kAudioUnitScope_Global, (AudioUnitElement)0, 
												inAudioFileRef, sizeof(*inAudioFileRef));

	return result;
}



#pragma mark -
#pragma mark features

//-----------------------------------------------------------------------------
CFStringRef CopyNameAndVersionString()
{
	CFBundleRef pluginBundleRef = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));
	if (pluginBundleRef == NULL)
		return NULL;

	// find the version string in the plugin's Info.plist and show that in the window in the appropriate place
	CFDictionaryRef infoPlistDictionary = CFBundleGetInfoDictionary(pluginBundleRef);
	CFDictionaryRef infoPlistLocalizedDictionary = CFBundleGetLocalInfoDictionary(pluginBundleRef);

	CFStringRef nameString = NULL, versionString = NULL;
	if (infoPlistLocalizedDictionary != NULL)
		nameString = (CFStringRef) CFDictionaryGetValue(infoPlistLocalizedDictionary, CFSTR("CFBundleName"));
	if (infoPlistDictionary != NULL)
	{
		if (nameString == NULL)
			nameString = (CFStringRef) CFDictionaryGetValue(infoPlistDictionary, CFSTR("CFBundleName"));
		versionString = (CFStringRef) CFDictionaryGetValue(infoPlistDictionary, CFSTR("CFBundleVersion"));
	}

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
CFStringRef CopyFileNameString(const FSRef & inFileRef)
{
	CFURLRef fileUrl = CFURLCreateFromFSRef(kCFAllocatorDefault, &inFileRef);
	if (fileUrl == NULL)
		return NULL;

	CFStringRef fileName = CFURLCopyLastPathComponent(fileUrl);
	CFRelease(fileUrl);
	return fileName;
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
	return AudioUnitSetProperty(GetEditAudioUnit(), kTurntablistProperty_Play, 
				kAudioUnitScope_Global, (AudioUnitElement)0, &inPlay, sizeof(inPlay));
}

//-----------------------------------------------------------------------------
void ScratchaEditor::SetFileNameDisplay(CFStringRef inDisplayText)
{
	if (inDisplayText == NULL)
		return;

	if (audioFileNameDisplay != NULL)
		audioFileNameDisplay->setCFText(inDisplayText);

/*
	// we need to the current port to the window's, otherwise DrawThemeTextBox() won't draw into our window
	GrafPtr originalPort = NULL;
	GetPort(&originalPort);
	SetPortWindowPort(GetCarbonWindow());

	SetThemeTextColor(kThemeTextColorWhite, 32, true);	// XXX eh, is there a real way to get the graphics device bit-depth value?
	SInt16 justification = teCenter;
	Rect textRect;
	textRect.left = kFileNameX + (short) GetXOffset();
	textRect.top = kFileNameY + (short) GetYOffset();
	textRect.right = textRect.left + kFileNameWidth;
	textRect.bottom = textRect.top + kFileNameHeight;
	DrawThemeTextBox(inDisplayText, kThemeSystemFont, kThemeStateActive, false, &textRect, justification, NULL);
//kThemeSystemFontDetail, kThemeMiniSystemFont, kThemeLabelFont

	// revert to the previously active port
	if (originalPort != NULL)
		SetPort(originalPort);
*/
}

//-----------------------------------------------------------------------------
void ScratchaEditor::SetUniversalDisplayType(long index)
{
	SetDisplayIndex(index);

	switch(index)
	{
		case kPitchAmount:
			SetUniversalType(kSTpamount);
			break;
		case kPitchRange:
			SetUniversalType(kSTprange);
			break;
		case kDirection:
			SetUniversalType(kSTdir);
			break;
		case kNoteMode:
			SetUniversalType(kSTnotemode);
			break;
		case kScratchAmount:
		case kScratchSpeed:
		case kSpinUpSpeed:
		case kSpinDownSpeed:
			SetUniversalType(kSTfloat);
			break;
		case kVolume:
			SetUniversalType(kSTdb);
			break;
		case kPower:
		case kMute:
		case kNPTrack:
		case kLoopMode:
		case kKeyTracking:
		case kPlay:
			SetUniversalType(kSTonoff);
			break;
		default:
			SetUniversalType(kSTfloat);
	}
}
