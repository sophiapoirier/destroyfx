#include "scrubbyeditor.hpp"
#include "scrubby.hpp"


//-----------------------------------------------------------------------------
const char * kDisplayFont = "snoot.org pixel10";
const float kDisplayTextSize = 14.0f;
const DGColor kBrownTextColor(187.0f/255.0f, 173.0f/255.0f, 131.0f/255.0f);
const float kUnusedControlAlpha = 0.234f;

const long kOctavesSliderWidth = 226 - 2;
const long kOctaveMaxSliderWidth = (long) (((float)kOctave_MaxValue / (float)(abs(kOctave_MinValue)+kOctave_MaxValue)) * (float)kOctavesSliderWidth);
const long kOctaveMinSliderWidth = kOctavesSliderWidth - kOctaveMaxSliderWidth;
const long kOctaveMinSliderX = 33 + 1;
const long kOctaveMaxSliderX = kOctaveMinSliderX + kOctaveMinSliderWidth;

//-----------------------------------------------------------------------------
enum {
	// positions
	kSliderHeight = 28,

#if 0
	kMainSlidersOffsetY = 0,
	kNotesStuffOffsetY = 0,
#else
	kMainSlidersOffsetY = -133,
	kNotesStuffOffsetY = 128,
#endif

	kSeekRateSliderX = 33 + 1,
	kSeekRateSliderY = 298 + kMainSlidersOffsetY,
	kSeekRateSliderWidth = 290 - 2,

	kSeekRangeSliderX = 33 + 1,
	kSeekRangeSliderY = 341 + kMainSlidersOffsetY,
	kSeekRangeSliderWidth = 336 - 2,

	kSeekDurSliderX = 33 + 1,
	kSeekDurSliderY = 384 + kMainSlidersOffsetY,
	kSeekDurSliderWidth = 336 - 2,

	kOctaveMinSliderY = 255 + kNotesStuffOffsetY,
	kOctaveMaxSliderY = kOctaveMinSliderY,

	kTempoSliderX = 33 + 1,
	kTempoSliderY = 427,
	kTempoSliderWidth = 119 - 2,

	kPredelaySliderX = 250 + 1,
	kPredelaySliderY = 427,
	kPredelaySliderWidth = 119 - 2,

	kDisplayWidth =90,
	kDisplayWidth_big = 117,	// for seek rate
	kDisplayHeight = 10,
	kDisplayInset = 2,

	kSpeedModeButtonX = 31,
	kSpeedModeButtonY = 60,
	kFreezeButtonX = 127,
	kFreezeButtonY = 60,
	kSplitChannelsButtonX = 223,
	kSplitChannelsButtonY = 60,
	kTempoSyncButtonX = 319,
	kTempoSyncButtonY = 60,
	kPitchConstraintButtonX = 415,
	kPitchConstraintButtonY = 60,

	kLittleTempoSyncButtonX = 327,
	kLittleTempoSyncButtonY = 303 + kMainSlidersOffsetY,

	kKeyboardX = 33,
	kKeyboardY = 164 + kNotesStuffOffsetY,

	kTransposeDownButtonX = 263,
	kTransposeDownButtonY = 164 + kNotesStuffOffsetY,
	kTransposeUpButtonX = kTransposeDownButtonX,
	kTransposeUpButtonY = kTransposeDownButtonY + 46,

	kMajorChordButtonX = 299,
	kMajorChordButtonY = 164 + kNotesStuffOffsetY,
	kMinorChordButtonX = kMajorChordButtonX,
	kMinorChordButtonY = kMajorChordButtonY + 19,
	kAllNotesButtonX = kMajorChordButtonX,
	kAllNotesButtonY = kMajorChordButtonY + 42,
	kNoneNotesButtonX = kMajorChordButtonX,
	kNoneNotesButtonY = kMajorChordButtonY + 61,

	kMidiLearnButtonX = 433,
	kMidiLearnButtonY = 248 + kNotesStuffOffsetY,
	kMidiResetButtonX = kMidiLearnButtonX,
	kMidiResetButtonY = kMidiLearnButtonY + 19,

	kHelpX = 4,
	kHelpY = 465,

	kDestroyFXlinkX = 122,
	kDestroyFXlinkY = 47,
	kSmartElectronixLinkX = 260,
	kSmartElectronixLinkY = 47,

	kTitleAreaX = 125,
	kTitleAreaY = 8,
	kTitleAreaWidth = 260,
	kTitleAreaHeight = 37
};

enum {
	kNotes_down = 0,
	kNotes_up,
	kNotes_major,
	kNotes_minor,
	kNotes_all,
	kNotes_none,
	kNumNotesButtons
};

enum {
	kHelp_none = 0,
	kHelp_general,
	kHelp_seekRate,
	kHelp_seekRange,
	kHelp_seekDur,
	kHelp_speedMode,
	kHelp_freeze,
	kHelp_splitChannels,
	kHelp_tempoSync,
	kHelp_pitchConstraint,
	kHelp_notes,
	kHelp_octaves,
	kHelp_tempo,
	kHelp_predelay,
	kHelp_midiLearn,
	kHelp_midiReset,
	kNumHelps,

	kNumHelpTextLines = 3
};

const char * helpstrings[kNumHelps][kNumHelpTextLines] = 
{
	{
		" ", 
		" ", 
		" ", 
	}, 
	// general
	{
		"Scrubby randomly zips around through an audio delay buffer.", 
		"Scrubby will, at a given seek rate, find random target destinations within a ", 
		"certain time range and then travel to those destinations.", 
	}, 
#if TARGET_OS_MAC
#define SCRUBBY_ALT_KEY_NAME "option"
#else
#define SCRUBBY_ALT_KEY_NAME "alt"
#endif
	// seek rate
	{
		"seek rate:  the rate at which Scrubby finds new target destinations", 
		"You can define a randomized range with min & max rate limits for each seek.", 
		"(control+click to move both together, "SCRUBBY_ALT_KEY_NAME"+click to move both relative)", 
	}, 
#undef SCRUBBY_ALT_KEY_NAME
	// seek range
	{
		"seek range:  define the time range in which Scrubby can zip around", 
		"This specifies how far back in the delay buffer Scrubby can look for new ", 
		"random target destinations.  This tends to affect playback speeds.", 
	}, 
	// seek duration
	{
		"seek duration:  amount of a seek cycle spent moving to the target", 
		"Scrubby finds a new target to move towards at each seek cycle.  You can ", 
		"make it reach the target early by lowering this value.  This produces gaps.", 
	}, 
	// speed mode
	{
		"speed mode:  are you a robot or a DJ?", 
		"Robot mode causes Scrubby to jump to the next speed after each target seek.  ", 
		"In DJ mode, Scrubby gradually accelerates or decelerates to next speed.", 
	}, 
	// freeze
	{
		"freeze:  freeze the delay buffer", 
		"This causes Scrubby to stop reading from your incoming audio stream and ", 
		"to stick with the current contents of the delay buffer.", 
	}, 
	// channels mode
	{
		"channels mode:  toggle between linked or split seeks for each channel", 
		"When linked, all audio channels will seek the same target destinations.  ", 
		"When split, each audio channel will find different destinations to seek.", 
	}, 
	// tempo sync
	{
		"tempo sync:  lock the seek rate to the tempo", 
		"Turning this on will let you define seek rates in terms of your tempo.  ", 
		"If your host doesn't give tempo info to plugins, you'll need to define a tempo.", 
	}, 
	// pitch constraint
	{
		"pitch constraint:  - only for robot mode -", 
		"With this set to \"notes,\" the playback speeds for each seek will always be ", 
		"semitone increments from the original pitch.  (see also the keyboard help)", 
	}, 
	// notes
	{
		"notes:  - only for robot mode with pitch constraint turned on -", 
		"You can choose which semitone steps within an octave are allowable when ", 
		"pitch constraint mode is on.  There are preset and transposition buttons, too.", 
	}, 
	// octaves
	{
		"octave limits:  limit Scrubby's speeds within a range of octaves", 
		"You can limit how low or how high Scrubby's playback speeds can go in terms ", 
		"of octaves, or move these to their outer points if you want no limits.", 
	}, 
	// tempo
	{
		"tempo:  sets the tempo that Scrubby uses when tempo sync is on", 
		"If your host app doesn't send tempo info to plugins, you'll need to adjust this ", 
		"parameter in order to specify a tempo for Scrubby to use.", 
	}, 
	// predelay
	{
		"predelay:  compensate for Scrubby's (possible) output delay", 
		"Scrubby zips around in a delay buffer and therefore can create some latency.  ", 
		"This tells your host to predelay by a % of the seek range.  (not in all hosts)", 
	}, 
	// MIDI learn
	{
		"MIDI learn:  toggle \"MIDI learn\" mode for CC control of parameters", 
		"When this is turned on, you can click on a parameter control & then the next ", 
		"MIDI CC received will be assigned to control that parameter.  (not in all hosts)", 
	}, 
	// MIDI reset
	{
		"MIDI reset:  erase CC assignments", 
		"Push this button to erase all of your MIDI CC -> parameter assignments.  ", 
		"Then CCs won't affect any parameters and you can start over if you want.", 
	}, 
};



//-----------------------------------------------------------------------------
// callbacks for button-triggered action

void transposeDownNotesButtonProc(SInt32 value, void * editor)
{	if ( (editor != NULL) && (value != 0) ) ((ScrubbyEditor*)editor)->HandleNotesButton(kNotes_down);	}
void transposeUpNotesButtonProc(SInt32 value, void * editor)
{	if ( (editor != NULL) && (value != 0) ) ((ScrubbyEditor*)editor)->HandleNotesButton(kNotes_up);	}
void majorNotesButtonProc(SInt32 value, void * editor)
{	if ( (editor != NULL) && (value != 0) ) ((ScrubbyEditor*)editor)->HandleNotesButton(kNotes_major);	}
void minorNotesButtonProc(SInt32 value, void * editor)
{	if ( (editor != NULL) && (value != 0) ) ((ScrubbyEditor*)editor)->HandleNotesButton(kNotes_minor);	}
void allNotesButtonProc(SInt32 value, void * editor)
{	if ( (editor != NULL) && (value != 0) ) ((ScrubbyEditor*)editor)->HandleNotesButton(kNotes_all);	}
void noneNotesButtonProc(SInt32 value, void * editor)
{	if ( (editor != NULL) && (value != 0) ) ((ScrubbyEditor*)editor)->HandleNotesButton(kNotes_none);	}

void midilearnScrubby(SInt32 value, void * editor)
{
	if (editor != NULL)
	{
		if (value == 0)
			((DfxGuiEditor*)editor)->setmidilearning(false);
		else
			((DfxGuiEditor*)editor)->setmidilearning(true);
	}
}

void midiresetScrubby(SInt32 value, void * editor)
{
	if ( (editor != NULL) && (value != 0) )
		((DfxGuiEditor*)editor)->resetmidilearn();
}


//-----------------------------------------------------------------------------
// parameter listener procedure
static void ScrubbyParameterListenerProc(void * inRefCon, void * inObject, const AudioUnitParameter * inParameter, Float32 inValue)
{
	if ( (inObject == NULL) || (inParameter == NULL) )
		return;

	AudioUnitParameterID paramID = inParameter->mParameterID;

	if (paramID == kTempoSync)
	{
		DGControl * control = (DGControl*) inObject;
		long oldParamID = control->getParameterID();
		long newParamID;
		bool useSyncParam = FBOOL(inValue);
		if ( (oldParamID == kSeekRateRandMin_abs) || (oldParamID == kSeekRateRandMin_sync) )
			newParamID = useSyncParam ? kSeekRateRandMin_sync : kSeekRateRandMin_abs;
		else
			newParamID = useSyncParam ? kSeekRate_sync : kSeekRate_abs;
		control->setParameterID(newParamID);
//		control->setControlContinuous(!useSyncParam);
	}

	else if ( (paramID == kSpeedMode) || (paramID == kPitchConstraint) )
		((ScrubbyEditor*)inObject)->HandlePitchConstraintChange();
}


//-----------------------------------------------------------------------------
// parameter value string display conversion functions

void seekRangeDisplayProc(float value, char * outText, void *)
{
	sprintf(outText, "%.1f ms", value);
}

void seekRateGenDisplayProc(float inValue, long inParameterID, char * outText, DfxGuiEditor * editor)
{
	if ( editor->getparameter_b(kTempoSync) )
	{
		editor->getparametervaluestring(inParameterID, outText);
//		if (strlen(outText) <= 3)
			strcat(outText, " cycles/beat");
	}
	else
		sprintf(outText, "%.3f Hz", inValue);
}

void seekRateDisplayProc(float value, char * outText, void * editor)
{
	seekRateGenDisplayProc(value, kSeekRate_sync, outText, (DfxGuiEditor*)editor);
}

void seekRateRandMinDisplayProc(float value, char * outText, void * editor)
{
	seekRateGenDisplayProc(value, kSeekRateRandMin_sync, outText, (DfxGuiEditor*)editor);
}

void seekDurDisplayProc(float value, char * outText, void *)
{
	sprintf(outText, "%.1f%%", value);
}

void octaveMinDisplayProc(float value, char * outText, void *)
{
	long octaves = (long) value;
	if (octaves <= kOctave_MinValue)
		strcpy(outText, "no min");
	else
		sprintf(outText, "%ld", octaves);
}

void octaveMaxDisplayProc(float value, char * outText, void *)
{
	long octaves = (long) value;
	if (octaves >= kOctave_MaxValue)
		strcpy(outText, "no max");
	else if (octaves == 0)
		sprintf(outText, "0");
	else
		sprintf(outText, "%+ld", octaves);
}

void tempoDisplayProc(float value, char * outText, void *)
{
	sprintf(outText, "%.3f bpm", value);
}

void predelayDisplayProc(float value, char * outText, void *)
{
	sprintf(outText, "%.0f%%", value);
}



#pragma mark -

//--------------------------------------------------------------------------
// this is a display for Scrubby's built-in help
ScrubbyHelpBox::ScrubbyHelpBox(DfxGuiEditor * inOwnerEditor, DGRect * inRegion, DGImage * inBackground)
:	DGTextDisplay(inOwnerEditor, DFX_PARAM_INVALID_ID, inRegion, NULL, NULL, inBackground, 
					kDGTextAlign_left, kDisplayTextSize, kDGColor_black, kDisplayFont), 
	itemNum(kHelp_none)
{
	setRespondToMouse(false);
}

//--------------------------------------------------------------------------
void ScrubbyHelpBox::draw(DGGraphicsContext * inContext)
{
/*
Rect cbounds;
GetControlBounds(carbonControl, &cbounds);
SInt32 focusMetric = 0;
OSStatus focusStatus = GetThemeMetric(kThemeMetricFocusRectOutset, &focusMetric);
if (focusStatus == noErr) printf("focus ring metric = %ld\n", focusMetric);
else printf("GetThemeMetric(kThemeMetricFocusRectOutset) error = %ld\n", focusStatus);
focusMetric = 4;	// XXX
InsetRect(&cbounds, focusMetric, focusMetric);
*/
	if (itemNum == kHelp_none)
{
//DrawThemeFocusRect(&cbounds, false);
/*
HIRect hiBounds;
OSStatus hiStatus = HIViewGetBounds(carbonControl, &hiBounds);
hiBounds = CGRectInset(hiBounds, focusMetric, focusMetric);
hiStatus = HIThemeDrawFocusRect(&hiBounds, true, inContext->getPlatformGraphicsContext(), kHIThemeOrientationNormal);
*/
		return;
}

	if (backgroundImage != NULL)
		backgroundImage->draw(getBounds(), inContext);

	DGRect textpos(getBounds());
	textpos.h = 10;
	textpos.w -= 13;
	textpos.offset(12, 4);

	fontColor = kDGColor_black;
	drawText(&textpos, helpstrings[itemNum][0], inContext);
	textpos.offset(1, 0);
	drawText(&textpos, helpstrings[itemNum][0], inContext);
	textpos.offset(-1, 13);
	fontColor = kDGColor_white;

	for (int i=1; i < kNumHelpTextLines; i++)
	{
		drawText(&textpos, helpstrings[itemNum][i], inContext);
		textpos.offset(0, 12);
	}
//DrawThemeFocusRect(&cbounds, true);
}

//--------------------------------------------------------------------------
void ScrubbyHelpBox::setDisplayItem(long inItemNum)
{
	if ( (inItemNum < 0) || (inItemNum >= kNumHelps) )
		return;

	bool changed = false;
	if (itemNum != inItemNum)
		changed = true;

	itemNum = inItemNum;

	if (changed)
		redraw();
}





#pragma mark -

//-----------------------------------------------------------------------------
// this is a very simple button class that has 2 states (on and off) and 
// doesn't do any mouse tracking
// this is for the keys on the keyboard control
class ScrubbyKeyboardButton : public DGControl
{
public:
	ScrubbyKeyboardButton(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion, DGImage * inImage)
	:	DGControl(inOwnerEditor, inParamID, inRegion), 
		buttonImage(inImage)
	{
		setControlContinuous(false);
		setWraparoundValues(true);
	}

	virtual void draw(DGGraphicsContext * inContext)
	{
		if (buttonImage != NULL)
		{
			long yoff = (GetControl32BitValue(carbonControl) == 0) ? 0 : (buttonImage->getHeight() / 2);
			buttonImage->draw(getBounds(), inContext, 0, yoff);
		}
	}
	virtual void mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers, bool inIsDoubleClick)
	{
		SetControl32BitValue( carbonControl, (GetControl32BitValue(carbonControl) == 0) ? 1 : 0 );
	}

private:
	DGImage * buttonImage;
};






#pragma mark -

//-----------------------------------------------------------------------------
COMPONENT_ENTRY(ScrubbyEditor)

//-----------------------------------------------------------------------------
ScrubbyEditor::ScrubbyEditor(AudioUnitCarbonView inInstance)
:	DfxGuiEditor(inInstance)
{
	seekRateSlider = NULL;
	seekRateDisplay = NULL;
	seekRateRandMinSlider = NULL;
	seekRateRandMinDisplay = NULL;

	helpbox = NULL;
	midiLearnButton = NULL;
	midiResetButton = NULL;
	titleArea = NULL;
	notesButtons = (DGButton**) malloc(kNumNotesButtons * sizeof(DGButton*));
	for (int i=0; i < kNumNotesButtons; i++)
		notesButtons[i] = NULL;

	parameterListener = NULL;
	OSStatus status = AUListenerCreate(ScrubbyParameterListenerProc, this,
		CFRunLoopGetCurrent(), kCFRunLoopDefaultMode, 0.030f, // 30 ms
		&parameterListener);
	if (status != noErr)
		parameterListener = NULL;
}

//-----------------------------------------------------------------------------
ScrubbyEditor::~ScrubbyEditor()
{
	if (parameterListener != NULL)
	{
		if (seekRateSlider != NULL)
			AUListenerRemoveParameter(parameterListener, seekRateSlider, &tempoSyncAUP);
		if (seekRateDisplay != NULL)
			AUListenerRemoveParameter(parameterListener, seekRateDisplay, &tempoSyncAUP);
		if (seekRateRandMinSlider != NULL)
			AUListenerRemoveParameter(parameterListener, seekRateRandMinSlider, &tempoSyncAUP);
		if (seekRateRandMinDisplay != NULL)
			AUListenerRemoveParameter(parameterListener, seekRateRandMinDisplay, &tempoSyncAUP);

		AUListenerRemoveParameter(parameterListener, this, &speedModeAUP);
		AUListenerRemoveParameter(parameterListener, this, &pitchConstraintAUP);

		AUListenerDispose(parameterListener);
	}

	if (notesButtons != NULL)
		free(notesButtons);
}

//-----------------------------------------------------------------------------
long ScrubbyEditor::open()
{
	// create images

	// background image
	DGImage * gBackground = new DGImage("scrubby-background.png", this);
	SetBackgroundImage(gBackground);

	// sliders
	DGImage * gSliderHandle = new DGImage("slider-handle.png", this);
	DGImage * gRangeSliderHandleLeft = new DGImage("range-slider-handle-left.png", this);
	DGImage * gRangeSliderHandleRight = new DGImage("range-slider-handle-right.png", this);

	// mode buttons
	DGImage * gSpeedModeButton = new DGImage("speed-mode-button.png", this);
	DGImage * gFreezeButton = new DGImage("freeze-button.png", this);
	DGImage * gTempoSyncButton = new DGImage("tempo-sync-button.png", this);
	DGImage * gSplitChannelsButton = new DGImage("stereo-button.png", this);
	DGImage * gPitchConstraintButton = new DGImage("pitch-constraint-button.png", this);
	DGImage * gTempoSyncButton_little = new DGImage("tempo-sync-button-little.png", this);

	// pitch constraint control buttons
//	DGImage * gKeyboardOff = new DGImage("keyboard-off.png", this);
//	DGImage * gKeyboardOn = new DGImage("keyboard-on.png", this);
	//
	DGImage * gKeyboardTopKeys[kNumPitchSteps];
	gKeyboardTopKeys[1] = gKeyboardTopKeys[3] = gKeyboardTopKeys[6] = gKeyboardTopKeys[8] = gKeyboardTopKeys[10] = 
							new DGImage("keyboard-black-key.png", this);
	gKeyboardTopKeys[0] = new DGImage("keyboard-white-key-top-1.png", this);
	gKeyboardTopKeys[2] = new DGImage("keyboard-white-key-top-2.png", this);
	gKeyboardTopKeys[4] = new DGImage("keyboard-white-key-top-3.png", this);
	gKeyboardTopKeys[5] = new DGImage("keyboard-white-key-top-4.png", this);
	gKeyboardTopKeys[7] = gKeyboardTopKeys[9] = new DGImage("keyboard-white-key-top-5-6.png", this);
	gKeyboardTopKeys[11] = new DGImage("keyboard-white-key-top-7.png", this);
	//
	DGImage * gKeyboardBottomKeys[kNumPitchSteps];
	for (int i=0; i < kNumPitchSteps; i++)
		gKeyboardBottomKeys[i] = NULL;
	gKeyboardBottomKeys[0] = new DGImage("keyboard-white-key-bottom-left.png", this);
	gKeyboardBottomKeys[2] = gKeyboardBottomKeys[4] = gKeyboardBottomKeys[5] = gKeyboardBottomKeys[7] = gKeyboardBottomKeys[9] = 
							new DGImage("keyboard-white-key-bottom.png", this);
	gKeyboardBottomKeys[kNumPitchSteps-1] = new DGImage("keyboard-white-key-bottom-right.png", this);
	//
	DGImage * gTransposeDownButton = new DGImage("transpose-down-button.png", this);
	DGImage * gTransposeUpButton = new DGImage("transpose-up-button.png", this);
	DGImage * gMajorChordButton = new DGImage("major-chord-button.png", this);
	DGImage * gMinorChordButton = new DGImage("minor-chord-button.png", this);
	DGImage * gAllNotesButton = new DGImage("all-notes-button.png", this);
	DGImage * gNoneNotesButton = new DGImage("none-notes-button.png", this);

	// help box
	DGImage * gHelpBackground = new DGImage("help-background.png", this);

	// other buttons
	DGImage * gMidiLearnButton = new DGImage("midi-learn-button.png", this);
	DGImage * gMidiResetButton = new DGImage("midi-reset-button.png", this);
	DGImage * gDestroyFXlink = new DGImage("destroy-fx-link.png", this);
	DGImage * gSmartElectronixLink = new DGImage("smart-electronix-link.png", this);



	DGRect pos;
	long seekRateParamID = getparameter_b(kTempoSync) ? kSeekRate_sync : kSeekRate_abs;
	long seekRateRandMinParamID = getparameter_b(kTempoSync) ? kSeekRateRandMin_sync : kSeekRateRandMin_abs;

	//--create the sliders-----------------------------------------------
	DGSlider * slider;

	// seek rate
	pos.set(kSeekRateSliderX, kSeekRateSliderY, kSeekRateSliderWidth, kSliderHeight);
	seekRateSlider = new DGSlider(this, seekRateParamID, &pos, kDGSliderAxis_horizontal, gSliderHandle, NULL);
//	seekRateSlider->setOvershoot(3);

	// seek range
	pos.set(kSeekRangeSliderX, kSeekRangeSliderY, kSeekRangeSliderWidth, kSliderHeight);
	slider = new DGSlider(this, kSeekRange, &pos, kDGSliderAxis_horizontal, gSliderHandle, NULL);

	// seek duration
	pos.set(kSeekDurSliderX, kSeekDurSliderY, kSeekDurSliderWidth, kSliderHeight);
	slider = new DGSlider(this, kSeekDur, &pos, kDGSliderAxis_horizontal, gSliderHandle, NULL);
//	slider->setOvershoot(3);

	// octave minimum
	pos.set(kOctaveMinSliderX, kOctaveMinSliderY, kOctaveMinSliderWidth, kSliderHeight);
	slider = new DGSlider(this, kOctaveMin, &pos, kDGSliderAxis_horizontal, gRangeSliderHandleLeft, NULL);
	slider->setControlContinuous(false);

	// octave maximum
	pos.set(kOctaveMaxSliderX, kOctaveMaxSliderY, kOctaveMaxSliderWidth, kSliderHeight);
	slider = new DGSlider(this, kOctaveMax, &pos, kDGSliderAxis_horizontal, gRangeSliderHandleRight, NULL);
	slider->setControlContinuous(false);

	// tempo
	pos.set(kTempoSliderX, kTempoSliderY, kTempoSliderWidth, kSliderHeight);
	slider = new DGSlider(this, kTempo, &pos, kDGSliderAxis_horizontal, gSliderHandle, NULL);

	// predelay
	pos.set(kPredelaySliderX, kPredelaySliderY, kPredelaySliderWidth, kSliderHeight);
	slider = new DGSlider(this, kPredelay, &pos, kDGSliderAxis_horizontal, gSliderHandle, NULL);



	//--create the displays---------------------------------------------
	DGTextDisplay * display;

	// seek rate random minimum
	pos.set(kSeekRateSliderX + kDisplayInset, kSeekRateSliderY - kDisplayHeight, kDisplayWidth_big, kDisplayHeight);
	seekRateRandMinDisplay = new DGTextDisplay(this, seekRateRandMinParamID, &pos, seekRateRandMinDisplayProc, this, NULL, kDGTextAlign_left, kDisplayTextSize, kBrownTextColor, kDisplayFont);

	// seek rate
	pos.set(kSeekRateSliderX + kSeekRateSliderWidth - kDisplayWidth_big - kDisplayInset, kSeekRateSliderY - kDisplayHeight, kDisplayWidth_big, kDisplayHeight);
	seekRateDisplay = new DGTextDisplay(this, seekRateParamID, &pos, seekRateDisplayProc, this, NULL, kDGTextAlign_right, kDisplayTextSize, kBrownTextColor, kDisplayFont);

	// seek range
	pos.set(kSeekRangeSliderX + kSeekRangeSliderWidth - kDisplayWidth - kDisplayInset, kSeekRangeSliderY - kDisplayHeight, kDisplayWidth, kDisplayHeight);
	display = new DGTextDisplay(this, kSeekRange, &pos, seekRangeDisplayProc, NULL, NULL, kDGTextAlign_right, kDisplayTextSize, kBrownTextColor, kDisplayFont);

	// seek duration random minimum
	pos.set(kSeekDurSliderX + kDisplayInset, kSeekDurSliderY - kDisplayHeight, kDisplayWidth, kDisplayHeight);
	display = new DGTextDisplay(this, kSeekDurRandMin, &pos, seekDurDisplayProc, NULL, NULL, kDGTextAlign_left, kDisplayTextSize, kBrownTextColor, kDisplayFont);

	// seek duration
	pos.set(kSeekDurSliderX + kSeekDurSliderWidth - kDisplayWidth - kDisplayInset, kSeekDurSliderY - kDisplayHeight, kDisplayWidth, kDisplayHeight);
	display = new DGTextDisplay(this, kSeekDur, &pos, seekDurDisplayProc, NULL, NULL, kDGTextAlign_right, kDisplayTextSize, kBrownTextColor, kDisplayFont);

	// octave mininum
	pos.set(kOctaveMinSliderX + kDisplayInset, kOctaveMinSliderY - kDisplayHeight, kDisplayWidth, kDisplayHeight);
	display = new DGTextDisplay(this, kOctaveMin, &pos, octaveMinDisplayProc, NULL, NULL, kDGTextAlign_left, kDisplayTextSize, kBrownTextColor, kDisplayFont);

	// octave maximum
	pos.set(kOctaveMaxSliderX + kOctaveMaxSliderWidth - kDisplayWidth - kDisplayInset, kOctaveMaxSliderY - kDisplayHeight, kDisplayWidth, kDisplayHeight);
	display = new DGTextDisplay(this, kOctaveMax, &pos, octaveMaxDisplayProc, NULL, NULL, kDGTextAlign_right, kDisplayTextSize, kBrownTextColor, kDisplayFont);

	// tempo
	pos.set(kTempoSliderX + kDisplayInset, kTempoSliderY - kDisplayHeight, kDisplayWidth, kDisplayHeight);
	display = new DGTextDisplay(this, kTempo, &pos, tempoDisplayProc, NULL, NULL, kDGTextAlign_left, kDisplayTextSize, kBrownTextColor, kDisplayFont);

	// predelay
	pos.set(kPredelaySliderX + kPredelaySliderWidth - kDisplayWidth - kDisplayInset, kPredelaySliderY - kDisplayHeight, kDisplayWidth, kDisplayHeight);
	display = new DGTextDisplay(this, kPredelay, &pos, predelayDisplayProc, NULL, NULL, kDGTextAlign_right, kDisplayTextSize, kBrownTextColor, kDisplayFont);



	//--create the keyboard---------------------------------------------
//	pos.set(kKeyboardX, kKeyboardY, gKeyboardOff->getWidth(), gKeyboardOff->getHeight());
//	keyboard = new ScrubbyKeyboard(this, kPitchStep0, &pos, gKeyboardOff, gKeyboardOn, 24, 48, 18, 56, 114, 149, 184);
	pos.set(kKeyboardX, kKeyboardY, gKeyboardTopKeys[0]->getWidth(), gKeyboardTopKeys[0]->getHeight()/2);
	DGRect pos2(kKeyboardX, kKeyboardY + pos.h, gKeyboardBottomKeys[0]->getWidth(), gKeyboardBottomKeys[0]->getHeight()/2);
	for (long i=0; i < kNumPitchSteps; i++)
	{
		pos.w = gKeyboardTopKeys[i]->getWidth();
		ScrubbyKeyboardButton * key = new ScrubbyKeyboardButton(this, kPitchStep0 + i, &pos, gKeyboardTopKeys[i]);
		pos.offset(pos.w, 0);

		if (gKeyboardBottomKeys[i] != NULL)
		{
			pos2.w = gKeyboardBottomKeys[i]->getWidth();
			key = new ScrubbyKeyboardButton(this, kPitchStep0 + i, &pos2, gKeyboardBottomKeys[i]);
			pos2.offset(pos2.w, 0);
		}
	}



	//--create the buttons----------------------------------------------
	DGButton * button;

	// ...................MODES...........................

	// choose the speed mode (robot or DJ)
	pos.set(kSpeedModeButtonX, kSpeedModeButtonY, gSpeedModeButton->getWidth()/2, gSpeedModeButton->getHeight()/kNumSpeedModes);
	button = new DGButton(this, kSpeedMode, &pos, gSpeedModeButton, kNumSpeedModes, kDGButtonType_incbutton, true);

	// freeze the input stream
	pos.set(kFreezeButtonX, kFreezeButtonY, gFreezeButton->getWidth()/2, gFreezeButton->getHeight()/2);
	button = new DGButton(this, kFreeze, &pos, gFreezeButton, 2, kDGButtonType_incbutton, true);

	// choose the channels mode (linked or split)
	pos.set(kSplitChannelsButtonX, kSplitChannelsButtonY, gSplitChannelsButton->getWidth()/2, gSplitChannelsButton->getHeight()/2);
	button = new DGButton(this, kSplitChannels, &pos, gSplitChannelsButton, 2, kDGButtonType_incbutton, true);

	// choose the seek rate type ("free" or synced)
	pos.set(kTempoSyncButtonX, kTempoSyncButtonY, gTempoSyncButton->getWidth()/2, gTempoSyncButton->getHeight()/2);
	button = new DGButton(this, kTempoSync, &pos, gTempoSyncButton, 2, kDGButtonType_incbutton, true);

	// toggle pitch constraint
	pos.set(kPitchConstraintButtonX, kPitchConstraintButtonY, gPitchConstraintButton->getWidth()/2, gPitchConstraintButton->getHeight()/2);
	button = new DGButton(this, kPitchConstraint, &pos, gPitchConstraintButton, 2, kDGButtonType_incbutton, true);

	// choose the seek rate type ("free" or synced)
	pos.set(kLittleTempoSyncButtonX, kLittleTempoSyncButtonY, gTempoSyncButton_little->getWidth(), gTempoSyncButton_little->getHeight()/2);
	button = new DGButton(this, kTempoSync, &pos, gTempoSyncButton_little, 2, kDGButtonType_incbutton, false);


	// ...............PITCH CONSTRAINT....................

	// transpose all of the pitch constraint notes down one semitone
	pos.set(kTransposeDownButtonX, kTransposeDownButtonY, gTransposeDownButton->getWidth(), gTransposeDownButton->getHeight()/2);
	notesButtons[kNotes_down] = new DGButton(this, &pos, gTransposeDownButton, 2, kDGButtonType_pushbutton);
	notesButtons[kNotes_down]->setUserProcedure(transposeDownNotesButtonProc, this);

	// transpose all of the pitch constraint notes up one semitone
	pos.set(kTransposeUpButtonX, kTransposeUpButtonY, gTransposeUpButton->getWidth(), gTransposeUpButton->getHeight()/2);
	notesButtons[kNotes_up] = new DGButton(this, &pos, gTransposeUpButton, 2, kDGButtonType_pushbutton);
	notesButtons[kNotes_up]->setUserProcedure(transposeUpNotesButtonProc, this);

	// turn on the pitch constraint notes that form a major chord
	pos.set(kMajorChordButtonX, kMajorChordButtonY, gMajorChordButton->getWidth(), gMajorChordButton->getHeight()/2);
	notesButtons[kNotes_major] = new DGButton(this, &pos, gMajorChordButton, 2, kDGButtonType_pushbutton);
	notesButtons[kNotes_major]->setUserProcedure(majorNotesButtonProc, this);

	// turn on the pitch constraint notes that form a minor chord
	pos.set(kMinorChordButtonX, kMinorChordButtonY, gMinorChordButton->getWidth(), gMinorChordButton->getHeight()/2);
	notesButtons[kNotes_minor] = new DGButton(this, &pos, gMinorChordButton, 2, kDGButtonType_pushbutton);
	notesButtons[kNotes_minor]->setUserProcedure(minorNotesButtonProc, this);

	// turn on all pitch constraint notes
	pos.set(kAllNotesButtonX, kAllNotesButtonY, gAllNotesButton->getWidth(), gAllNotesButton->getHeight()/2);
	notesButtons[kNotes_all] = new DGButton(this, &pos, gAllNotesButton, 2, kDGButtonType_pushbutton);
	notesButtons[kNotes_all]->setUserProcedure(allNotesButtonProc, this);

	// turn off all pitch constraint notes
	pos.set(kNoneNotesButtonX, kNoneNotesButtonY, gNoneNotesButton->getWidth(), gNoneNotesButton->getHeight()/2);
	notesButtons[kNotes_none] = new DGButton(this, &pos, gNoneNotesButton, 2, kDGButtonType_pushbutton);
	notesButtons[kNotes_none]->setUserProcedure(noneNotesButtonProc, this);


	// .....................MISC..........................

	// turn on/off MIDI learn mode for CC parameter automation
	pos.set(kMidiLearnButtonX, kMidiLearnButtonY, gMidiLearnButton->getWidth(), gMidiLearnButton->getHeight()/2);
	midiLearnButton = new DGButton(this, &pos, gMidiLearnButton, 2, kDGButtonType_incbutton);
	midiLearnButton->setUserProcedure(midilearnScrubby, this);

	// clear all MIDI CC assignments
	pos.set(kMidiResetButtonX, kMidiResetButtonY, gMidiResetButton->getWidth(), gMidiResetButton->getHeight()/2);
	midiResetButton = new DGButton(this, &pos, gMidiResetButton, 2, kDGButtonType_pushbutton);
	midiResetButton->setUserProcedure(midiresetScrubby, this);

	DGWebLink * weblink;

	// Destroy FX web page link
	pos.set(kDestroyFXlinkX, kDestroyFXlinkY, gDestroyFXlink->getWidth(), gDestroyFXlink->getHeight()/2);
	weblink = new DGWebLink(this, &pos, gDestroyFXlink, DESTROYFX_URL);

	// Smart Electronix web page link
	pos.set(kSmartElectronixLinkX, kSmartElectronixLinkY, gSmartElectronixLink->getWidth(), gSmartElectronixLink->getHeight()/2);
	weblink = new DGWebLink(this, &pos, gSmartElectronixLink, SMARTELECTRONIX_URL);

	pos.set(125, 8, 260, 37);
	pos.set(kTitleAreaX, kTitleAreaY, kTitleAreaWidth, kTitleAreaHeight);
	titleArea = new DGControl(this, &pos, 0.0f);



	//--create the help display-----------------------------------------
	pos.set(kHelpX, kHelpY, gHelpBackground->getWidth(), gHelpBackground->getHeight());
	helpbox = new ScrubbyHelpBox(this, &pos, gHelpBackground);



	// this will initialize the pitch constraint controls' initial translucency settings 
	HandlePitchConstraintChange();
	// and this will do the same for the channels mode control
	numAudioChannelsChanged( getNumAudioChannels() );



	if (parameterListener != NULL)
	{
		// set up the parameter listeners
		tempoSyncAUP.mAudioUnit = speedModeAUP.mAudioUnit = pitchConstraintAUP.mAudioUnit = GetEditAudioUnit();
		tempoSyncAUP.mScope = speedModeAUP.mScope = pitchConstraintAUP.mScope = kAudioUnitScope_Global;
		tempoSyncAUP.mElement = speedModeAUP.mElement = pitchConstraintAUP.mElement = (AudioUnitElement)0;
		tempoSyncAUP.mParameterID = kTempoSync;

		AUListenerAddParameter(parameterListener, seekRateSlider, &tempoSyncAUP);
		AUListenerAddParameter(parameterListener, seekRateDisplay, &tempoSyncAUP);
//		AUListenerAddParameter(parameterListener, seekRateRandMinSlider, &tempoSyncAUP);
		AUListenerAddParameter(parameterListener, seekRateRandMinDisplay, &tempoSyncAUP);

		speedModeAUP.mParameterID = kSpeedMode;
		pitchConstraintAUP.mParameterID = kPitchConstraint;
		AUListenerAddParameter(parameterListener, this, &speedModeAUP);
		AUListenerAddParameter(parameterListener, this, &pitchConstraintAUP);
	}



	return noErr;
}


//-----------------------------------------------------------------------------
void ScrubbyEditor::HandleNotesButton(long inNotesButtonType)
{
	long i;
	bool tempValue;

	switch (inNotesButtonType)
	{
		case kNotes_down:
			tempValue = getparameter_b(kPitchStep0);
			for (i=kPitchStep0; i < kPitchStep11; i++)
				setparameter_b(i, getparameter_b(i+1));
			setparameter_b(kPitchStep11, tempValue);
			break;
		case kNotes_up:
			tempValue = getparameter_b(kPitchStep11);
			for (i=kPitchStep11; i > kPitchStep0; i--)
				setparameter_b(i, getparameter_b(i-1));
			setparameter_b(kPitchStep0, tempValue);
			break;
		case kNotes_major:
			setparameter_b(kPitchStep0, true);
			setparameter_b(kPitchStep1, false);
			setparameter_b(kPitchStep2, true);
			setparameter_b(kPitchStep3, false);
			setparameter_b(kPitchStep4, true);
			setparameter_b(kPitchStep5, true);
			setparameter_b(kPitchStep6, false);
			setparameter_b(kPitchStep7, true);
			setparameter_b(kPitchStep8, false);
			setparameter_b(kPitchStep9, true);
			setparameter_b(kPitchStep10, false);
			setparameter_b(kPitchStep11, true);
			break;
		case kNotes_minor:
			setparameter_b(kPitchStep0, true);
			setparameter_b(kPitchStep1, false);
			setparameter_b(kPitchStep2, true);
			setparameter_b(kPitchStep3, true);
			setparameter_b(kPitchStep4, false);
			setparameter_b(kPitchStep5, true);
			setparameter_b(kPitchStep6, false);
			setparameter_b(kPitchStep7, true);
			setparameter_b(kPitchStep8, false);
			setparameter_b(kPitchStep9, true);
			setparameter_b(kPitchStep10, true);
			setparameter_b(kPitchStep11, false);
			break;
		case kNotes_all:
			for (i=kPitchStep0; i <= kPitchStep11; i++)
				setparameter_b(i, true);
			break;
		case kNotes_none:
			for (i=kPitchStep0; i <= kPitchStep11; i++)
				setparameter_b(i, false);
			break;
		default:
			break;
	}
}

//-----------------------------------------------------------------------------
void ScrubbyEditor::HandlePitchConstraintChange()
{
//SetWindowAlpha(GetCarbonWindow(), DFX_Rand_f());	// heh heh heh...
	long speedMode = getparameter_i(kSpeedMode);
	bool pitchConstraint = getparameter_b(kPitchConstraint);
	float alpha = ( (speedMode == kSpeedMode_robot) && pitchConstraint ) ? 1.0f : kUnusedControlAlpha;
	float pcalpha = (speedMode == kSpeedMode_robot) ? 1.0f : kUnusedControlAlpha;

	DGControlsList * tempcl = controlsList;
	while (tempcl != NULL)
	{
		long paramid = tempcl->control->getParameterID();
		if ( (paramid >= kPitchStep0) && (paramid <= kPitchStep11) )
			tempcl->control->setDrawAlpha(alpha);
		else if (paramid == kPitchConstraint)
			tempcl->control->setDrawAlpha(pcalpha);
		tempcl = tempcl->next;
	}

	for (long i=0; i < kNumNotesButtons; i++)
	{
		if (notesButtons[i] != NULL)
			notesButtons[i]->setDrawAlpha(alpha);
	}
}

//-----------------------------------------------------------------------------
void ScrubbyEditor::mouseovercontrolchanged(DGControl * currentControlUnderMouse)
{
	long newHelpItem = kHelp_none;
	if (currentControlUnderMouse != NULL)
	{
		if (currentControlUnderMouse == titleArea)
		{
			newHelpItem = kHelp_general;
			goto updateHelp;
		}
		else if (currentControlUnderMouse == midiLearnButton)
		{
			newHelpItem = kHelp_midiLearn;
			goto updateHelp;
		}
		else if (currentControlUnderMouse == midiResetButton)
		{
			newHelpItem = kHelp_midiReset;
			goto updateHelp;
		}

		for (int i=0; i < kNumNotesButtons; i++)
		{
			if (currentControlUnderMouse == notesButtons[i])
			{
				newHelpItem = kHelp_notes;
				goto updateHelp;
			}
		}

		switch (currentControlUnderMouse->getParameterID())
		{
			case kSeekRange:
				newHelpItem = kHelp_seekRange;
				break;
			case kFreeze:
				newHelpItem = kHelp_freeze;
				break;
			case kSeekRate_abs:
			case kSeekRate_sync:
			case kSeekRateRandMin_abs:
			case kSeekRateRandMin_sync:
				newHelpItem = kHelp_seekRate;
				break;
			case kTempoSync:
				newHelpItem = kHelp_tempoSync;
				break;
			case kSeekDur:
			case kSeekDurRandMin:
				newHelpItem = kHelp_seekDur;
				break;
			case kSpeedMode:
				newHelpItem = kHelp_speedMode;
				break;
			case kSplitChannels:
				newHelpItem = kHelp_splitChannels;
				break;
			case kPitchConstraint:
				newHelpItem = kHelp_pitchConstraint;
				break;
			case kPitchStep0:
			case kPitchStep1:
			case kPitchStep2:
			case kPitchStep3:
			case kPitchStep4:
			case kPitchStep5:
			case kPitchStep6:
			case kPitchStep7:
			case kPitchStep8:
			case kPitchStep9:
			case kPitchStep10:
			case kPitchStep11:
				newHelpItem = kHelp_notes;
				break;
			case kOctaveMin:
			case kOctaveMax:
				newHelpItem = kHelp_octaves;
				break;
			case kTempo:
				newHelpItem = kHelp_tempo;
				break;
			case kPredelay:
				newHelpItem = kHelp_predelay;
				break;
			default:
				newHelpItem = kHelp_none;
				break;
		}
	}

updateHelp:
	if (helpbox != NULL)
		helpbox->setDisplayItem(newHelpItem);
}

//-----------------------------------------------------------------------------
void ScrubbyEditor::numAudioChannelsChanged(unsigned long inNewNumChannels)
{
	float alpha = (inNewNumChannels > 1) ? 1.0f : kUnusedControlAlpha;

	DGControlsList * tempcl = controlsList;
	while (tempcl != NULL)
	{
		if (tempcl->control->getParameterID() == kSplitChannels)
			tempcl->control->setDrawAlpha(alpha);
		tempcl = tempcl->next;
	}
}
