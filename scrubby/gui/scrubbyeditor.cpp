#ifndef __scrubbyeditor
#include "scrubbyeditor.hpp"
#endif

#include <stdio.h>
#include <math.h>


//-----------------------------------------------------------------------------
const long kOctavesFaderWidth = 226 - 2;
const long kOctaveMaxFaderWidth = (long) (((float)OCTAVE_MAX / (float)(abs(OCTAVE_MIN)+OCTAVE_MAX)) * (float)kOctavesFaderWidth);
const long kOctaveMinFaderWidth = kOctavesFaderWidth - kOctaveMaxFaderWidth;
const long kOctaveMinFaderX = 33 + 1;
const long kOctaveMaxFaderX = kOctaveMinFaderX + kOctaveMinFaderWidth;
//-----------------------------------------------------------------------------
enum {
	// resource IDs
	kBackgroundID = 128,
	kFaderHandleID,
	kGlowingFaderHandleID,
	kFaderHandleLeftID,
	kGlowingFaderHandleLeftID,
	kFaderHandleRightID,
	kGlowingFaderHandleRightID,
	kSpeedModeButtonID,
	kFreezeButtonID,
	kTempoSyncButtonID,
	kStereoButtonID,
	kPitchConstraintButtonID,
	kLittleTempoSyncButtonID,
	kKeyboardOffID,
	kKeyboardOnID,
	kTransposeDownButtonID,
	kTransposeUpButtonID,
	kMajorChordButtonID,
	kMinorChordButtonID,
	kAllNotesButtonID,
	kNoneNotesButtonID,
	kMidiLearnButtonID,
	kMidiResetButtonID,
	kHelpID,
	kDestroyFXlinkID,
	kSmartElectronixLinkID,

	// positions
	kDisplayWidth = 90,
	kDisplayHeight = 12,
	kDisplayInset = 2,

	kFaderHeight = 28,

	kOctaveMinFaderY = 255,
	kOctaveMaxFaderY = kOctaveMinFaderY,

	kSeekRateFaderX = 33 + 1,
	kSeekRateFaderY = 298,
	kSeekRateFaderWidth = 290 - 2,

	kSeekDurFaderX = 33 + 1,
	kSeekDurFaderY = 341,
	kSeekDurFaderWidth = 336 - 2,

	kSeekRangeFaderX = 33 + 1,
	kSeekRangeFaderY = 384,
	kSeekRangeFaderWidth = 336 - 2,

	kTempoFaderX = 33 + 1,
	kTempoFaderY = 427,
	kTempoFaderWidth = 119 - 2,

	kPredelayFaderX = 250 + 1,
	kPredelayFaderY = 427,
	kPredelayFaderWidth = 119 - 2,

	kSpeedModeButtonX = 32,
	kSpeedModeButtonY = 60,
	kFreezeButtonX = 127,
	kFreezeButtonY = 60,
	kTempoSyncButtonX = 223,
	kTempoSyncButtonY = 60,
	kStereoButtonX = 319,
	kStereoButtonY = 60,
	kPitchConstraintButtonX = 415,
	kPitchConstraintButtonY = 60,

	kLittleTempoSyncButtonX = 327,
	kLittleTempoSyncButtonY = 303,

	kTransposeDownButtonX = 263,
	kTransposeDownButtonY = 164,
	kTransposeUpButtonX = kTransposeDownButtonX,
	kTransposeUpButtonY = kTransposeDownButtonY + 46,

	kKeyboardX = 33,
	kKeyboardY = 164,

	kMajorChordButtonX = 299,
	kMajorChordButtonY = 164,
	kMinorChordButtonX = kMajorChordButtonX,
	kMinorChordButtonY = kMajorChordButtonY + 19,
	kAllNotesButtonX = kMajorChordButtonX,
	kAllNotesButtonY = kMajorChordButtonY + 42,
	kNoneNotesButtonX = kMajorChordButtonX,
	kNoneNotesButtonY = kMajorChordButtonY + 61,

	kMidiLearnButtonX = 433,
	kMidiLearnButtonY = 248,
	kMidiResetButtonX = kMidiLearnButtonX,
	kMidiResetButtonY = kMidiLearnButtonY + 19,

	kHelpX = 4,
	kHelpY = 465,

	kDestroyFXlinkX = 122,
	kDestroyFXlinkY = 47,
	kSmartElectronixLinkX = 260,
	kSmartElectronixLinkY = 47,

	kGeneralHelp = 0,
	kSpeedModeHelp,
	kFreezeHelp,
	kTempoSyncHelp,
	kStereoHelp,
	kPitchConstraintHelp,
	kNotesHelp,
	kOctavesHelp,
	kSeekRateHelp,
	kSeekDurHelp,
	kSeekRangeHelp,
	kTempoHelp,
	kPredelayHelp,
	kMidiLearnHelp,
	kMidiResetHelp,
	kNoHelp,
	kNumHelps,

	kTempoTextEdit = 333
};



// goofy stuff that we have to do because this variable is static, 
// & it's static because tempoRateDisplayConvert() is static,  & that's static 
// so that it can be a class member & still be acceptable to setStringConvert()
float ScrubbyEditor::theTempoSync;


//-----------------------------------------------------------------------------
// parameter value string display conversion functions

void seekRangeDisplayConvert(float value, char *string);
void seekRangeDisplayConvert(float value, char *string)
{
	sprintf(string, "%.1f  ms", seekRangeScaled(value));
}

void ScrubbyEditor::seekRateDisplayConvert(float value, char *string, void *temporatestring)
{
	if ( onOffTest(theTempoSync) )
	{
		strcpy(string, (char*)temporatestring);
		if (strlen(string) <= 3)
			strcat(string, "  cycles/beat");
	}
	else
		sprintf(string, "%.3f  Hz", seekRateScaled(value));
//sprintf(string, "%.3f", *(float*)temporatestring);
}

void seekDurDisplayConvert(float value, char *string);
void seekDurDisplayConvert(float value, char *string)
{
	sprintf(string, "%.1f %%", seekDurScaled(value) * 100.0f);
}

void octaveMinDisplayConvert(float value, char *string);
void octaveMinDisplayConvert(float value, char *string)
{
	if (octaveMinScaled(value) <= OCTAVE_MIN)
		strcpy(string, "no  min");
	else
		sprintf(string, "%ld", octaveMinScaled(value));
}

void octaveMaxDisplayConvert(float value, char *string);
void octaveMaxDisplayConvert(float value, char *string)
{
	int octaves = octaveMaxScaled(value);
	if (octaves >= OCTAVE_MAX)
		strcpy(string, "no  max");
	else if (octaves == 0)
		sprintf(string, "0");
	else
		sprintf(string, "%+d", octaves);
}

void predelayDisplayConvert(float value, char *string);
void predelayDisplayConvert(float value, char *string)
{
	sprintf(string, "%d %%", (int)(value*100.0f));
}



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
ScrubbyEditor::ScrubbyEditor(AudioEffect *effect)
 : AEffGUIEditor(effect) 
{
	frame = 0;
	isOpen = false;

	// initialize the graphics pointers
	gBackground = 0;
	gFaderHandle = 0;
	gGlowingFaderHandle = 0;
	gFaderHandleLeft = 0;
	gGlowingFaderHandleLeft = 0;
	gFaderHandleRight = 0;
	gGlowingFaderHandleRight = 0;
	gSpeedModeButton = 0;
	gFreezeButton = 0;
	gTempoSyncButton = 0;
	gStereoButton = 0;
	gPitchConstraintButton = 0;
	gLittleTempoSyncButton = 0;
	gKeyboardOff = 0;
	gKeyboardOn = 0;
	gTransposeDownButton = 0;
	gTransposeUpButton = 0;
	gMajorChordButton = 0;
	gMinorChordButton = 0;
	gAllNotesButton = 0;
	gNoneNotesButton = 0;
	gMidiLearnButton = 0;
	gMidiResetButton = 0;
	gHelp = 0;
	gDestroyFXlink = 0;
	gSmartElectronixLink = 0;

	// initialize the controls pointers
	octaveMinFader = 0;
	octaveMaxFader = 0;
	seekRateFader = 0;
	seekDurFader = 0;
	seekRangeFader = 0;
	tempoFader = 0;
	predelayFader = 0;
	speedModeButton = 0;
	freezeButton = 0;
	tempoSyncButton = 0;
	stereoButton = 0;
	pitchConstraintButton = 0;
	littleTempoSyncButton = 0;
	keyboard = 0;
	transposeDownButton = 0;
	transposeUpButton = 0;
	majorChordButton = 0;
	minorChordButton = 0;
	allNotesButton = 0;
	noneNotesButton = 0;
	COnOffButton *midiLearnButton = 0;
	midiResetButton = 0;
	help = 0;
	DestroyFXlink = 0;
	SmartElectronixLink = 0;

	// initialize the value display box pointers
	octaveMinDisplay = 0;
	octaveMaxDisplay = 0;
	seekRateDisplay = 0;
	seekRateRandMinDisplay = 0;
	seekDurDisplay = 0;
	seekDurRandMinDisplay = 0;
	seekRangeDisplay = 0;
	predelayDisplay = 0;
	tempoTextEdit = 0;

	// load the background bitmap
	// we don't need to load all bitmaps, this could be done when open is called
	gBackground = new CBitmap (kBackgroundID);

	// init the size of the plugin
	rect.left   = 0;
	rect.top    = 0;
	rect.right  = 100; // (short)gBackground->getWidth();
	rect.bottom = 100; // (short)gBackground->getHeight();

#if 1

	FILE * ff = fopen("c:\\temp\\scrubby_log.txt", "a");
	fprintf(ff, "gbackground(%p): %d x %d\n", gBackground, 
		gBackground?gBackground->getWidth():0,
		gBackground?gBackground->getHeight():0);

	fclose(ff);

#endif

	tempoString = new char[256];
	tempoRateString = new char[16];
	tempoRateRandMinString = new char[16];
	chunk = ((Scrubby*)effect)->chunk;	// this just simplifies pointing

	faders = 0;
	faders = (CHorizontalSlider**)malloc(sizeof(CHorizontalSlider*)*NUM_PARAMETERS);
	for (long i=0; i < NUM_PARAMETERS; i++)
		faders[i] = NULL;
}

//-----------------------------------------------------------------------------
ScrubbyEditor::~ScrubbyEditor()
{
	// free background bitmap
	if (gBackground)
		gBackground->forget();
	gBackground = 0;

	free(faders);
	faders = 0;

	if (tempoString)
		delete[] tempoString;
	if (tempoRateString)
		delete[] tempoRateString;
	if (tempoRateRandMinString)
		delete[] tempoRateRandMinString;
}

//-----------------------------------------------------------------------------
long ScrubbyEditor::getRect(ERect **erect)
{
	*erect = &rect;
	return true;
}

//-----------------------------------------------------------------------------
long ScrubbyEditor::open(void *ptr)
{
  CPoint displayOffset;	// for positioning the background graphic behind display boxes
  CPoint point (0, 0);	// for no offset


	// !!! always call this !!!
	AEffGUIEditor::open(ptr);

	// load some bitmaps
	if (!gFaderHandle)
		gFaderHandle = new CBitmap (kFaderHandleID);
	if (!gGlowingFaderHandle)
		gGlowingFaderHandle = new CBitmap (kGlowingFaderHandleID);
	if (!gFaderHandleLeft)
		gFaderHandleLeft = new CBitmap (kFaderHandleLeftID);
	if (!gGlowingFaderHandleLeft)
		gGlowingFaderHandleLeft = new CBitmap (kGlowingFaderHandleLeftID);
	if (!gFaderHandleRight)
		gFaderHandleRight = new CBitmap (kFaderHandleRightID);
	if (!gGlowingFaderHandleRight)
		gGlowingFaderHandleRight = new CBitmap (kGlowingFaderHandleRightID);

	if (!gSpeedModeButton)
		gSpeedModeButton = new CBitmap (kSpeedModeButtonID);
	if (!gFreezeButton)
		gFreezeButton = new CBitmap (kFreezeButtonID);
	if (!gTempoSyncButton)
		gTempoSyncButton = new CBitmap (kTempoSyncButtonID);
	if (!gStereoButton)
		gStereoButton = new CBitmap (kStereoButtonID);
	if (!gPitchConstraintButton)
		gPitchConstraintButton = new CBitmap (kPitchConstraintButtonID);

	if (!gLittleTempoSyncButton)
		gLittleTempoSyncButton = new CBitmap (kLittleTempoSyncButtonID);

	if (!gTransposeDownButton)
		gTransposeDownButton = new CBitmap (kTransposeDownButtonID);
	if (!gTransposeUpButton)
		gTransposeUpButton = new CBitmap (kTransposeUpButtonID);
	if (!gMajorChordButton)
		gMajorChordButton = new CBitmap (kMajorChordButtonID);
	if (!gMinorChordButton)
		gMinorChordButton = new CBitmap (kMinorChordButtonID);
	if (!gAllNotesButton)
		gAllNotesButton = new CBitmap (kAllNotesButtonID);
	if (!gNoneNotesButton)
		gNoneNotesButton = new CBitmap (kNoneNotesButtonID);
	if (!gKeyboardOff)
		gKeyboardOff = new CBitmap (kKeyboardOffID);
	if (!gKeyboardOn)
		gKeyboardOn = new CBitmap (kKeyboardOnID);

	if (!gHelp)
		gHelp = new CBitmap (kHelpID);

	if (!gMidiLearnButton)
		gMidiLearnButton = new CBitmap (kMidiLearnButtonID);
	if (!gMidiResetButton)
		gMidiResetButton = new CBitmap (kMidiResetButtonID);

	if (!gDestroyFXlink)
		gDestroyFXlink = new CBitmap (kDestroyFXlinkID);
	if (!gSmartElectronixLink)
		gSmartElectronixLink = new CBitmap (kSmartElectronixLinkID);


#if 1

	FILE * ff = fopen("c:\\temp\\scrubby_log.txt", "a");
	fprintf(ff, "[2]gbackground(%p): %d x %d\n", gBackground, 
		gBackground?gBackground->getWidth():0,
		gBackground?gBackground->getHeight():0);
	fprintf(ff, "[2]DestroyFXlink(%p): %d x %d\n", gDestroyFXlink, 
		gDestroyFXlink?gDestroyFXlink->getWidth():0,
		gDestroyFXlink?gDestroyFXlink->getHeight():0);

	fclose(ff);

#endif


	chunk->resetLearning();


	//--initialize the background frame--------------------------------------
	CRect size = (0, 0, 100, 100); // (short)gBackground->getWidth(), (short)gBackground->getHeight());
	frame = new CFrame (size, ptr, this);
	frame->setBackground(gBackground);



	//--initialize the faders-----------------------------------------------
	long minPos, maxPos;

	// octave minimum
	size (kOctaveMinFaderX, kOctaveMinFaderY, kOctaveMinFaderX + kOctaveMinFaderWidth, kOctaveMinFaderY + kFaderHeight);
	minPos = kOctaveMinFaderX;
	maxPos = kOctaveMinFaderX + kOctaveMinFaderWidth - gFaderHandleLeft->getWidth();
	displayOffset (size.left, size.top);
	octaveMinFader = new CHorizontalSliderChunky (size, this, kOctaveMin, minPos, maxPos, abs(OCTAVE_MIN)+1, gFaderHandleLeft, gBackground, displayOffset, kLeft);
	octaveMinFader->setValue(effect->getParameter(kOctaveMin));
	octaveMinFader->setDefaultValue(0.0f);
	frame->addView(octaveMinFader);

	// octave maximum
	size (kOctaveMaxFaderX, kOctaveMaxFaderY, kOctaveMaxFaderX + kOctaveMaxFaderWidth, kOctaveMaxFaderY + kFaderHeight);
	minPos = kOctaveMaxFaderX;
	maxPos = kOctaveMaxFaderX + kOctaveMaxFaderWidth - gFaderHandleRight->getWidth();
	displayOffset (size.left, size.top);
	octaveMaxFader = new CHorizontalSliderChunky (size, this, kOctaveMax, minPos, maxPos, OCTAVE_MAX+1, gFaderHandleRight, gBackground, displayOffset, kLeft);
	octaveMaxFader->setValue(effect->getParameter(kOctaveMax));
	octaveMaxFader->setDefaultValue(0.0f);
	frame->addView(octaveMaxFader);

	// seek rate
	size (kSeekRateFaderX, kSeekRateFaderY, kSeekRateFaderX + kSeekRateFaderWidth, kSeekRateFaderY + kFaderHeight);
	minPos = kSeekRateFaderX;
	maxPos = kSeekRateFaderX + kSeekRateFaderWidth;
	displayOffset (size.left, size.top);
	seekRateFader = new CHorizontalRangeSlider (size, this, kSeekRateRandMin, kSeekRate, minPos, maxPos, gFaderHandleLeft, gBackground, displayOffset, kLeft, kMaxCanPush, gFaderHandleRight);
	seekRateFader->setValueTagged(kSeekRate, effect->getParameter(kSeekRate));
	seekRateFader->setValueTagged(kSeekRateRandMin, effect->getParameter(kSeekRateRandMin));
	seekRateFader->setDefaultValueTagged(kSeekRate, seekRateUnscaled(3.0f));
	seekRateFader->setDefaultValueTagged(kSeekRateRandMin, seekRateUnscaled(3.0f));
	seekRateFader->setOvershoot(3);
	frame->addView(seekRateFader);

	// seek duration
	size (kSeekDurFaderX, kSeekDurFaderY, kSeekDurFaderX + kSeekDurFaderWidth, kSeekDurFaderY + kFaderHeight);
	minPos = kSeekDurFaderX;
	maxPos = kSeekDurFaderX + kSeekDurFaderWidth;
	displayOffset (size.left, size.top);
	seekDurFader = new CHorizontalRangeSlider (size, this, kSeekDurRandMin, kSeekDur, minPos, maxPos, gFaderHandleLeft, gBackground, displayOffset, kLeft, kMaxCanPush, gFaderHandleRight);
	seekDurFader->setValueTagged(kSeekDur, effect->getParameter(kSeekDur));
	seekDurFader->setValueTagged(kSeekDurRandMin, effect->getParameter(kSeekDurRandMin));
	seekDurFader->setDefaultValueTagged(kSeekDur, 1.0f);
	seekDurFader->setDefaultValueTagged(kSeekDurRandMin, 1.0f);
	seekDurFader->setOvershoot(3);
	frame->addView(seekDurFader);

	// seek range
	size (kSeekRangeFaderX, kSeekRangeFaderY, kSeekRangeFaderX + kSeekRangeFaderWidth, kSeekRangeFaderY + kFaderHeight);
	minPos = kSeekRangeFaderX;
	maxPos = kSeekRangeFaderX + kSeekRangeFaderWidth - gFaderHandle->getWidth();
	displayOffset (size.left, size.top);
	seekRangeFader = new CHorizontalSlider (size, this, kSeekRange, minPos, maxPos, gFaderHandle, gBackground, displayOffset, kLeft);
	seekRangeFader->setValue(effect->getParameter(kSeekRange));
	seekRangeFader->setDefaultValue(seekRangeUnscaled(333.0f));
	frame->addView(seekRangeFader);

	// tempo (in bpm)
	size (kTempoFaderX, kTempoFaderY, kTempoFaderX + kTempoFaderWidth, kTempoFaderY + kFaderHeight);
	minPos = kTempoFaderX;
	maxPos = kTempoFaderX + kTempoFaderWidth - gFaderHandle->getWidth();
	displayOffset (size.left, size.top);
	tempoFader = new CHorizontalSlider (size, this, kTempo, minPos, maxPos, gFaderHandle, gBackground, displayOffset, kLeft);
	tempoFader->setValue(effect->getParameter(kTempo));
	tempoFader->setDefaultValue(0.0f);
	frame->addView(tempoFader);

	// predelay
	size (kPredelayFaderX, kPredelayFaderY, kPredelayFaderX + kPredelayFaderWidth, kPredelayFaderY + kFaderHeight);
	minPos = kPredelayFaderX;
	maxPos = kPredelayFaderX + kPredelayFaderWidth - gFaderHandle->getWidth();
	displayOffset (size.left, size.top);
	predelayFader = new CHorizontalSlider (size, this, kPredelay, minPos, maxPos, gFaderHandle, gBackground, displayOffset, kLeft);
	predelayFader->setValue(effect->getParameter(kPredelay));
	predelayFader->setDefaultValue(0.5f);
	frame->addView(predelayFader);



	//--initialize the keyboard---------------------------------------------

	size (kKeyboardX, kKeyboardY, kKeyboardX + gKeyboardOff->getWidth(), kKeyboardY + gKeyboardOff->getHeight());
	keyboard = new ScrubbyKeyboard(size, this, kPitchStep0, gKeyboardOff, gKeyboardOn, 
									24, 48, 18, 56, 114, 149, 184);
	for (int t=0; t < 12; t++)
		keyboard->setValueIndexed(t, effect->getParameter(kPitchStep0+t));
	frame->addView(keyboard);



	//--initialize the buttons----------------------------------------------

	// ...................MODES...........................

	// choose the speed mode (robot or DJ)
	size (kSpeedModeButtonX, kSpeedModeButtonY, kSpeedModeButtonX + (gSpeedModeButton->getWidth()/2), kSpeedModeButtonY + (gSpeedModeButton->getHeight())/2);
	speedModeButton = new MultiKick (size, this, kPortamento, 2, gSpeedModeButton->getHeight()/2, gSpeedModeButton, point, kKickPairs);
	speedModeButton->setValue(effect->getParameter(kPortamento));
	frame->addView(speedModeButton);

	// freeze the input stream
	size (kFreezeButtonX, kFreezeButtonY, kFreezeButtonX + (gFreezeButton->getWidth()/2), kFreezeButtonY + (gFreezeButton->getHeight())/2);
	freezeButton = new MultiKick (size, this, kFreeze, 2, gFreezeButton->getHeight()/2, gFreezeButton, point, kKickPairs);
	freezeButton->setValue(effect->getParameter(kFreeze));
	frame->addView(freezeButton);

	// choose the seek rate type ("free" or synced)
	size (kTempoSyncButtonX, kTempoSyncButtonY, kTempoSyncButtonX + (gTempoSyncButton->getWidth()/2), kTempoSyncButtonY + (gTempoSyncButton->getHeight())/2);
	tempoSyncButton = new MultiKick (size, this, kTempoSync, 2, gTempoSyncButton->getHeight()/2, gTempoSyncButton, point, kKickPairs);
	tempoSyncButton->setValue(effect->getParameter(kTempoSync));
	frame->addView(tempoSyncButton);

	// choose the stereo mode (linked or split)
	size (kStereoButtonX, kStereoButtonY, kStereoButtonX + (gStereoButton->getWidth()/2), kStereoButtonY + (gStereoButton->getHeight())/2);
	stereoButton = new MultiKick (size, this, kSplitStereo, 2, gStereoButton->getHeight()/2, gStereoButton, point, kKickPairs);
	stereoButton->setValue(effect->getParameter(kSplitStereo));
	frame->addView(stereoButton);

	// toggle pitch constraint
	size (kPitchConstraintButtonX, kPitchConstraintButtonY, kPitchConstraintButtonX + (gPitchConstraintButton->getWidth()/2), kPitchConstraintButtonY + (gPitchConstraintButton->getHeight())/2);
	pitchConstraintButton = new MultiKick (size, this, kPitchConstraint, 2, gPitchConstraintButton->getHeight()/2, gPitchConstraintButton, point, kKickPairs);
	pitchConstraintButton->setValue(effect->getParameter(kPitchConstraint));
	frame->addView(pitchConstraintButton);

	// choose the seek rate type ("free" or synced)
	size (kLittleTempoSyncButtonX, kLittleTempoSyncButtonY, kLittleTempoSyncButtonX + gLittleTempoSyncButton->getWidth(), kLittleTempoSyncButtonY + (gLittleTempoSyncButton->getHeight())/2);
	littleTempoSyncButton = new COnOffButton (size, this, kTempoSync, gLittleTempoSyncButton);
	littleTempoSyncButton->setValue(effect->getParameter(kTempoSync));
	frame->addView(littleTempoSyncButton);


	// ...............PITCH CONSTRAINT....................

	// transpose all of the pitch constraint notes down one semitone
	size (kTransposeDownButtonX, kTransposeDownButtonY, kTransposeDownButtonX + gTransposeDownButton->getWidth(), kTransposeDownButtonY + (gTransposeDownButton->getHeight())/2);
	transposeDownButton = new CKickButton (size, this, kTransposeDownButtonID, (gTransposeDownButton->getHeight())/2, gTransposeDownButton, point);
	transposeDownButton->setValue(0.0f);
	frame->addView(transposeDownButton);

	// transpose all of the pitch constraint notes up one semitone
	size (kTransposeUpButtonX, kTransposeUpButtonY, kTransposeUpButtonX + gTransposeUpButton->getWidth(), kTransposeUpButtonY + (gTransposeUpButton->getHeight())/2);
	transposeUpButton = new CKickButton (size, this, kTransposeUpButtonID, (gTransposeUpButton->getHeight())/2, gTransposeUpButton, point);
	transposeUpButton->setValue(0.0f);
	frame->addView(transposeUpButton);

	// turn on the pitch constraint notes that form a major chord
	size (kMajorChordButtonX, kMajorChordButtonY, kMajorChordButtonX + gMajorChordButton->getWidth(), kMajorChordButtonY + (gMajorChordButton->getHeight())/2);
	majorChordButton = new CKickButton (size, this, kMajorChordButtonID, (gMajorChordButton->getHeight())/2, gMajorChordButton, point);
	majorChordButton->setValue(0.0f);
	frame->addView(majorChordButton);

	// turn on the pitch constraint notes that form a minor chord
	size (kMinorChordButtonX, kMinorChordButtonY, kMinorChordButtonX + gMinorChordButton->getWidth(), kMinorChordButtonY + (gMinorChordButton->getHeight())/2);
	minorChordButton = new CKickButton (size, this, kMinorChordButtonID, (gMinorChordButton->getHeight())/2, gMinorChordButton, point);
	minorChordButton->setValue(0.0f);
	frame->addView(minorChordButton);

	// turn on all pitch constraint notes
	size (kAllNotesButtonX, kAllNotesButtonY, kAllNotesButtonX + gAllNotesButton->getWidth(), kAllNotesButtonY + (gAllNotesButton->getHeight())/2);
	allNotesButton = new CKickButton (size, this, kAllNotesButtonID, (gAllNotesButton->getHeight())/2, gAllNotesButton, point);
	allNotesButton->setValue(0.0f);
	frame->addView(allNotesButton);

	// turn off all pitch constraint notes
	size (kNoneNotesButtonX, kNoneNotesButtonY, kNoneNotesButtonX + gNoneNotesButton->getWidth(), kNoneNotesButtonY + (gNoneNotesButton->getHeight())/2);
	noneNotesButton = new CKickButton (size, this, kNoneNotesButtonID, (gNoneNotesButton->getHeight())/2, gNoneNotesButton, point);
	noneNotesButton->setValue(0.0f);
	frame->addView(noneNotesButton);


	// .....................MISC..........................

	// turn on/off MIDI learn mode for CC parameter automation
	size (kMidiLearnButtonX, kMidiLearnButtonY, kMidiLearnButtonX + gMidiLearnButton->getWidth(), kMidiLearnButtonY + (gMidiLearnButton->getHeight())/2);
	midiLearnButton = new COnOffButton (size, this, kMidiLearnButtonID, gMidiLearnButton);
	midiLearnButton->setValue(0.0f);
	frame->addView(midiLearnButton);

	// clear all MIDI CC assignments
	size (kMidiResetButtonX, kMidiResetButtonY, kMidiResetButtonX + gMidiResetButton->getWidth(), kMidiResetButtonY + (gMidiResetButton->getHeight())/2);
	midiResetButton = new CKickButton (size, this, kMidiResetButtonID, (gMidiResetButton->getHeight())/2, gMidiResetButton, point);
	midiResetButton->setValue(0.0f);
	frame->addView(midiResetButton);

	// Destroy FX web page link
	size (kDestroyFXlinkX, kDestroyFXlinkY, kDestroyFXlinkX + gDestroyFXlink->getWidth(), kDestroyFXlinkY + (gDestroyFXlink->getHeight())/2);
	DestroyFXlink = new CWebLink (size, this, kDestroyFXlinkID, DESTROYFX_URL, gDestroyFXlink);
	frame->addView(DestroyFXlink);

	// Smart Electronix web page link
	size (kSmartElectronixLinkX, kSmartElectronixLinkY, kSmartElectronixLinkX + gSmartElectronixLink->getWidth(), kSmartElectronixLinkY + (gSmartElectronixLink->getHeight())/2);
	SmartElectronixLink = new CWebLink (size, this, kSmartElectronixLinkID, SMARTELECTRONIX_URL, gSmartElectronixLink);
	frame->addView(SmartElectronixLink);



	//--initialize the displays---------------------------------------------

	// first store the proper values for all of the globals so that displays are correct
	strcpy( tempoRateString, ((Scrubby*)effect)->tempoRateTable->getDisplay(effect->getParameter(kSeekRate)) );
	strcpy( tempoRateRandMinString, ((Scrubby*)effect)->tempoRateTable->getDisplay(effect->getParameter(kSeekRateRandMin)) );
	theTempoSync = ((Scrubby*)effect)->fTempoSync;

	// octave mininum (for pitch constraint)
	size (kOctaveMinFaderX + kDisplayInset, kOctaveMinFaderY - kDisplayHeight, kOctaveMinFaderX + kDisplayInset + kDisplayWidth, kOctaveMinFaderY);
	octaveMinDisplay = new CParamDisplay (size, gBackground);
	displayOffset (size.left, size.top);
	octaveMinDisplay->setBackOffset(displayOffset);
	octaveMinDisplay->setHoriAlign(kLeftText);
	octaveMinDisplay->setFont(kNormalFontSmall);
	octaveMinDisplay->setFontColor(kBrownTextCColor);
	octaveMinDisplay->setValue(effect->getParameter(kOctaveMin));
	octaveMinDisplay->setStringConvert(octaveMinDisplayConvert);
	octaveMinDisplay->setTag(kOctaveMin);
	frame->addView(octaveMinDisplay);

	// octave maximum (for pitch constraint)
	size (kOctaveMaxFaderX + kOctaveMaxFaderWidth - kDisplayWidth - kDisplayInset, kOctaveMaxFaderY - kDisplayHeight, kOctaveMaxFaderX + kOctaveMaxFaderWidth - kDisplayInset, kOctaveMaxFaderY);
	octaveMaxDisplay = new CParamDisplay (size, gBackground);
	displayOffset (size.left, size.top);
	octaveMaxDisplay->setBackOffset(displayOffset);
	octaveMaxDisplay->setHoriAlign(kRightText);
	octaveMaxDisplay->setFont(kNormalFontSmall);
	octaveMaxDisplay->setFontColor(kBrownTextCColor);
	octaveMaxDisplay->setValue(effect->getParameter(kOctaveMax));
	octaveMaxDisplay->setStringConvert(octaveMaxDisplayConvert);
	octaveMaxDisplay->setTag(kOctaveMax);
	frame->addView(octaveMaxDisplay);

	// seek rate random minimum
	size (kSeekRateFaderX + kDisplayInset, kSeekRateFaderY - kDisplayHeight, kSeekRateFaderX + kDisplayInset + kDisplayWidth, kSeekRateFaderY);
	seekRateRandMinDisplay = new CParamDisplay (size, gBackground);
	displayOffset (size.left, size.top);
	seekRateRandMinDisplay->setBackOffset(displayOffset);
	seekRateRandMinDisplay->setHoriAlign(kLeftText);
	seekRateRandMinDisplay->setFont(kNormalFontSmall);
	seekRateRandMinDisplay->setFontColor(kBrownTextCColor);
	seekRateRandMinDisplay->setValue(effect->getParameter(kSeekRateRandMin));
	seekRateRandMinDisplay->setStringConvert(seekRateDisplayConvert, tempoRateRandMinString);
//	seekRateRandMinDisplay->setStringConvert(seekRateDisplayConvert, &(((Scrubby*)effect)->showme));
	seekRateRandMinDisplay->setTag(kSeekRateRandMin);
	frame->addView(seekRateRandMinDisplay);

	// seek rate
	size (kSeekRateFaderX + kSeekRateFaderWidth - kDisplayWidth - kDisplayInset, kSeekRateFaderY - kDisplayHeight, kSeekRateFaderX + kSeekRateFaderWidth - kDisplayInset, kSeekRateFaderY);
	seekRateDisplay = new CParamDisplay (size, gBackground);
	displayOffset (size.left, size.top);
	seekRateDisplay->setBackOffset(displayOffset);
	seekRateDisplay->setHoriAlign(kRightText);
	seekRateDisplay->setFont(kNormalFontSmall);
	seekRateDisplay->setFontColor(kBrownTextCColor);
	seekRateDisplay->setValue(effect->getParameter(kSeekRate));
	seekRateDisplay->setStringConvert(seekRateDisplayConvert, tempoRateString);
	seekRateDisplay->setTag(kSeekRate);
	frame->addView(seekRateDisplay);

	// seek duration random minimum
	size (kSeekDurFaderX + kDisplayInset, kSeekDurFaderY - kDisplayHeight, kSeekDurFaderX + kDisplayInset + kDisplayWidth, kSeekDurFaderY);
	seekDurRandMinDisplay = new CParamDisplay (size, gBackground);
	displayOffset (size.left, size.top);
	seekDurRandMinDisplay->setBackOffset(displayOffset);
	seekDurRandMinDisplay->setHoriAlign(kLeftText);
	seekDurRandMinDisplay->setFont(kNormalFontSmall);
	seekDurRandMinDisplay->setFontColor(kBrownTextCColor);
	seekDurRandMinDisplay->setValue(effect->getParameter(kSeekDurRandMin));
	seekDurRandMinDisplay->setStringConvert(seekDurDisplayConvert);
	seekDurRandMinDisplay->setTag(kSeekDurRandMin);
	frame->addView(seekDurRandMinDisplay);

	// seek duration
	size (kSeekDurFaderX + kSeekDurFaderWidth - kDisplayWidth - kDisplayInset, kSeekDurFaderY - kDisplayHeight, kSeekDurFaderX + kSeekDurFaderWidth - kDisplayInset, kSeekDurFaderY);
	seekDurDisplay = new CParamDisplay (size, gBackground);
	displayOffset (size.left, size.top);
	seekDurDisplay->setBackOffset(displayOffset);
	seekDurDisplay->setHoriAlign(kRightText);
	seekDurDisplay->setFont(kNormalFontSmall);
	seekDurDisplay->setFontColor(kBrownTextCColor);
	seekDurDisplay->setValue(effect->getParameter(kSeekDur));
	seekDurDisplay->setStringConvert(seekDurDisplayConvert);
	seekDurDisplay->setTag(kSeekDur);
	frame->addView(seekDurDisplay);

	// seek range
	size (kSeekRangeFaderX + kDisplayInset, kSeekRangeFaderY - kDisplayHeight, kSeekRangeFaderX + kDisplayInset + kDisplayWidth, kSeekRangeFaderY);
	seekRangeDisplay = new CParamDisplay (size, gBackground);
	displayOffset (size.left, size.top);
	seekRangeDisplay->setBackOffset(displayOffset);
	seekRangeDisplay->setHoriAlign(kLeftText);
	seekRangeDisplay->setFont(kNormalFontSmall);
	seekRangeDisplay->setFontColor(kBrownTextCColor);
	seekRangeDisplay->setValue(effect->getParameter(kSeekRange));
	seekRangeDisplay->setStringConvert(seekRangeDisplayConvert);
	seekRangeDisplay->setTag(kSeekRange);
	frame->addView(seekRangeDisplay);

	// tempo (in bpm)   (editable)
	size (kTempoFaderX + kDisplayInset, kTempoFaderY - kDisplayHeight, kTempoFaderX + kDisplayInset + kDisplayWidth, kTempoFaderY);
	tempoTextEdit = new CTextEdit (size, this, kTempoTextEdit, 0, gBackground);
	displayOffset (size.left, size.top);
	tempoTextEdit->setBackOffset(displayOffset);
	tempoTextEdit->setFont(kNormalFontSmall);
	tempoTextEdit->setFontColor(kBrownTextCColor);
	tempoTextEdit->setHoriAlign(kLeftText);
	frame->addView(tempoTextEdit);
	// this makes it display the current value
	setParameter(kTempo, effect->getParameter(kTempo));

	// predelay
	size (kPredelayFaderX + kPredelayFaderWidth - kDisplayWidth - kDisplayInset, kPredelayFaderY - kDisplayHeight, kPredelayFaderX + kPredelayFaderWidth - kDisplayInset, kPredelayFaderY);
	predelayDisplay = new CParamDisplay (size, gBackground);
	displayOffset (size.left, size.top);
	predelayDisplay->setBackOffset(displayOffset);
	predelayDisplay->setHoriAlign(kRightText);
	predelayDisplay->setFont(kNormalFontSmall);
	predelayDisplay->setFontColor(kBrownTextCColor);
	predelayDisplay->setValue(effect->getParameter(kPredelay));
	predelayDisplay->setStringConvert(predelayDisplayConvert);
	predelayDisplay->setTag(kPredelay);
	frame->addView(predelayDisplay);



	//--initialize the help display-----------------------------------------
	size (kHelpX, kHelpY, kHelpX + gHelp->getWidth(), kHelpY + (gHelp->getHeight()/kNumHelps));
	help = new IndexBitmap (size, gHelp->getHeight()/kNumHelps, gHelp, point);
	help->setindex(kNumHelps-1);
	frame->addView(help);



	for (long i=0; i < NUM_PARAMETERS; i++)
		faders[i] = NULL;
	faders[kSeekRange] = seekRangeFader;
	faders[kTempo] = tempoFader;
	faders[kPredelay] = predelayFader;


	isOpen = true;

	return true;
}

//-----------------------------------------------------------------------------
void ScrubbyEditor::close()
{
	isOpen = false;
	if (frame)
		delete frame;
	frame = 0;

	for (long i=0; i < NUM_PARAMETERS; i++)
		faders[i] = NULL;

	chunk->resetLearning();

	// free some bitmaps
	if (gFaderHandle)
		gFaderHandle->forget();
	gFaderHandle = 0;
	if (gGlowingFaderHandle)
		gGlowingFaderHandle->forget();
	gGlowingFaderHandle = 0;
	//
	if (gFaderHandleLeft)
		gFaderHandleLeft->forget();
	gFaderHandleLeft = 0;
	if (gGlowingFaderHandleLeft)
		gGlowingFaderHandleLeft->forget();
	gGlowingFaderHandleLeft = 0;
	//
	if (gFaderHandleRight)
		gFaderHandleRight->forget();
	gFaderHandleRight = 0;
	if (gGlowingFaderHandleRight)
		gGlowingFaderHandleRight->forget();
	gGlowingFaderHandleRight = 0;

	if (gSpeedModeButton)
		gSpeedModeButton->forget();
	gSpeedModeButton = 0;
	if (gFreezeButton)
		gFreezeButton->forget();
	gFreezeButton = 0;
	if (gTempoSyncButton)
		gTempoSyncButton->forget();
	gTempoSyncButton = 0;
	if (gStereoButton)
		gStereoButton->forget();
	gStereoButton = 0;
	if (gPitchConstraintButton)
		gPitchConstraintButton->forget();
	gPitchConstraintButton = 0;

	if (gLittleTempoSyncButton)
		gLittleTempoSyncButton->forget();
	gLittleTempoSyncButton = 0;

	if (gTransposeDownButton)
		gTransposeDownButton->forget();
	gTransposeDownButton = 0;
	if (gTransposeUpButton)
		gTransposeUpButton->forget();
	gTransposeUpButton = 0;
	if (gMajorChordButton)
		gMajorChordButton->forget();
	gMajorChordButton = 0;
	if (gMinorChordButton)
		gMinorChordButton->forget();
	gMinorChordButton = 0;
	if (gAllNotesButton)
		gAllNotesButton->forget();
	gAllNotesButton = 0;
	if (gNoneNotesButton)
		gNoneNotesButton->forget();
	gNoneNotesButton = 0;

	if (gKeyboardOff)
		gKeyboardOff->forget();
	gKeyboardOff = 0;
	if (gKeyboardOn)
		gKeyboardOn->forget();
	gKeyboardOn = 0;

	if (gMidiLearnButton)
		gMidiLearnButton->forget();
	gMidiLearnButton = 0;
	if (gMidiResetButton)
		gMidiResetButton->forget();
	gMidiResetButton = 0;

	if (gHelp)
		gHelp->forget();
	gHelp = 0;

	if (gDestroyFXlink)
		gDestroyFXlink->forget();
	gDestroyFXlink = 0;
	if (gSmartElectronixLink)
		gSmartElectronixLink->forget();
	gSmartElectronixLink = 0;
}

//-----------------------------------------------------------------------------
// called from ScrubbyEdit
void ScrubbyEditor::setParameter(long index, float value)
{
	if (!frame)
		return;

	switch (index)
	{
		case kSeekRange:
			if (seekRangeFader)
				seekRangeFader->setValue(value);
			if (seekRangeDisplay)
				seekRangeDisplay->setValue(value);
			break;

		case kFreeze:
			if (freezeButton)
				freezeButton->setValue(value);
			break;

		case kSeekRate:
			// store these into the static global variables so that the string convert function can see them
			strcpy( tempoRateString, ((Scrubby*)effect)->tempoRateTable->getDisplay(effect->getParameter(kSeekRate)) );
			theTempoSync = ((Scrubby*)effect)->fTempoSync;
			if (seekRateFader)
				seekRateFader->setValueTagged(index, value);
			if (seekRateDisplay)
				seekRateDisplay->setValue(value);
			break;
		case kSeekRateRandMin:
			strcpy( tempoRateRandMinString, ((Scrubby*)effect)->tempoRateTable->getDisplay(effect->getParameter(kSeekRateRandMin)) );
			theTempoSync = ((Scrubby*)effect)->fTempoSync;
			if (seekRateFader)
				seekRateFader->setValueTagged(index, value);
			if (seekRateRandMinDisplay)
				seekRateRandMinDisplay->setValue(value);
			break;

		case kTempoSync:
			strcpy( tempoRateString, ((Scrubby*)effect)->tempoRateTable->getDisplay(effect->getParameter(kSeekRate)) );
			strcpy( tempoRateRandMinString, ((Scrubby*)effect)->tempoRateTable->getDisplay(effect->getParameter(kSeekRateRandMin)) );
			theTempoSync = ((Scrubby*)effect)->fTempoSync;
			if (tempoSyncButton)
				tempoSyncButton->setValue(value);
			if (littleTempoSyncButton)
				littleTempoSyncButton->setValue(value);
			if (seekRateDisplay)
				seekRateDisplay->setDirty();
			if (seekRateRandMinDisplay)
				seekRateRandMinDisplay->setDirty();
			break;

		case kSeekDur:
			if (seekDurFader)
				seekDurFader->setValueTagged(index, value);
			if (seekDurDisplay)
				seekDurDisplay->setValue(value);
			break;
		case kSeekDurRandMin:
			if (seekDurFader)
				seekDurFader->setValueTagged(index, value);
			if (seekDurRandMinDisplay)
				seekDurRandMinDisplay->setValue(value);
			break;

		case kPortamento:
			if (speedModeButton)
				speedModeButton->setValue(value);
			break;

		case kSplitStereo:
			if (stereoButton)
				stereoButton->setValue(value);
			break;

		case kPitchConstraint:
			if (pitchConstraintButton)
				pitchConstraintButton->setValue(value);
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
			if (keyboard)
				keyboard->setValueTagged(index, value);
			break;

		case kOctaveMin:
			if (octaveMinFader)
				octaveMinFader->setValue(value);
			if (octaveMinDisplay)
				octaveMinDisplay->setValue(value);
			break;

		case kOctaveMax:
			if (octaveMaxFader)
				octaveMaxFader->setValue(value);
			if (octaveMaxDisplay)
				octaveMaxDisplay->setValue(value);
			break;

		case kTempo:
			if (tempoFader)
				tempoFader->setValue(value);
			if (tempoTextEdit)
			{
				if ( (value > 0.0f) || (((Scrubby*)effect)->hostCanDoTempo != 1) )
					sprintf(tempoString, "%.3f  bpm", tempoScaled(value));
				else
					strcpy(tempoString, "auto");
				tempoTextEdit->setText(tempoString);
			}
			break;

		case kPredelay:
			if (predelayFader)
				predelayFader->setValue(value);
			if (predelayDisplay)
				predelayDisplay->setValue(value);
			break;

		default:
			return;
	}

	postUpdate();
}

//-----------------------------------------------------------------------------
void ScrubbyEditor::valueChanged(CDrawContext* context, CControl* control)
{
  long tag = control->getTag();
  float tempTempo, tempValue;
  long sscanfReturn, i;


	switch (tag)
	{
		// process the tempo text input
		case kTempoTextEdit:
			if (tempoTextEdit)
			{
				tempoTextEdit->getText(tempoString);
				sscanfReturn = sscanf(tempoString, "%f", &tempTempo);
				if (strcmp(tempoString, "auto") == 0)
					effect->setParameterAutomated(kTempo, 0.0f);
				else if ( (sscanfReturn != EOF) && (sscanfReturn > 0) )
				{
					// check if the user typed in something that's not a number
					if (tempTempo == 0.0f)
						tempoTextEdit->setText("very bad");
					// the user typed in a number
					else
					{
						// no negative tempos
						if (tempTempo < 0.0f)
							tempTempo = -tempTempo;
						// if the tempo entered is 0, then leave it at 0.0 as the parameter value
						if (tempTempo > 0.0f)
							// scale the value to a 0.0 to 1.0 parameter value
							tempTempo = tempoUnscaled(tempTempo);
						// this updates the display with "bpm" appended & the fader position
						effect->setParameterAutomated(kTempo, tempTempo);
					}
				}
				// there was a sscanf() error
				else
					tempoTextEdit->setText("bad");
			}
			break;

		case kMidiLearnButtonID:
			chunk->setParameterMidiLearn(control->getValue());
			break;
		case kMidiResetButtonID:
			chunk->setParameterMidiReset(control->getValue());
			break;

		case kTransposeDownButtonID:
			if (control->getValue() > 0.5f)
			{
				tempValue = effect->getParameter(kPitchStep0);
				for (i=kPitchStep0; i < kPitchStep11; i++)
					effect->setParameterAutomated(i, effect->getParameter(i+1));
				effect->setParameterAutomated(kPitchStep11, tempValue);
			}
			break;
		case kTransposeUpButtonID:
			if (control->getValue() > 0.5f)
			{
				tempValue = effect->getParameter(kPitchStep11);
				for (i=kPitchStep11; i > kPitchStep0; i--)
					effect->setParameterAutomated(i, effect->getParameter(i-1));
				effect->setParameterAutomated(kPitchStep0, tempValue);
			}
			break;
		case kMajorChordButtonID:
			if (control->getValue() > 0.5f)
			{
				effect->setParameterAutomated(kPitchStep0, 1.0f);
				effect->setParameterAutomated(kPitchStep1, 0.0f);
				effect->setParameterAutomated(kPitchStep2, 1.0f);
				effect->setParameterAutomated(kPitchStep3, 0.0f);
				effect->setParameterAutomated(kPitchStep4, 1.0f);
				effect->setParameterAutomated(kPitchStep5, 1.0f);
				effect->setParameterAutomated(kPitchStep6, 0.0f);
				effect->setParameterAutomated(kPitchStep7, 1.0f);
				effect->setParameterAutomated(kPitchStep8, 0.0f);
				effect->setParameterAutomated(kPitchStep9, 1.0f);
				effect->setParameterAutomated(kPitchStep10, 0.0f);
				effect->setParameterAutomated(kPitchStep11, 1.0f);
			}
			break;
		case kMinorChordButtonID:
			if (control->getValue() > 0.5f)
			{
				effect->setParameterAutomated(kPitchStep0, 1.0f);
				effect->setParameterAutomated(kPitchStep1, 0.0f);
				effect->setParameterAutomated(kPitchStep2, 1.0f);
				effect->setParameterAutomated(kPitchStep3, 1.0f);
				effect->setParameterAutomated(kPitchStep4, 0.0f);
				effect->setParameterAutomated(kPitchStep5, 1.0f);
				effect->setParameterAutomated(kPitchStep6, 0.0f);
				effect->setParameterAutomated(kPitchStep7, 1.0f);
				effect->setParameterAutomated(kPitchStep8, 0.0f);
				effect->setParameterAutomated(kPitchStep9, 1.0f);
				effect->setParameterAutomated(kPitchStep10, 1.0f);
				effect->setParameterAutomated(kPitchStep11, 0.0f);
			}
			break;
		case kAllNotesButtonID:
			if (control->getValue() > 0.5f)
			{
				for (i=kPitchStep0; i <= kPitchStep11; i++)
					effect->setParameterAutomated(i, 1.0f);
			}
			break;
		case kNoneNotesButtonID:
			if (control->getValue() > 0.5f)
			{
				for (i=kPitchStep0; i <= kPitchStep11; i++)
					effect->setParameterAutomated(i, 0.0f);
			}
			break;

		case kSeekRate:
		case kSeekRateRandMin:
			if (seekRateFader)
			{
				effect->setParameterAutomated(kSeekRate, seekRateFader->getValueTagged(kSeekRate));
				effect->setParameterAutomated(kSeekRateRandMin, seekRateFader->getValueTagged(kSeekRateRandMin));
				chunk->setLearner(seekRateFader->getTag());
				// set the automation link mode for the seek rate range slider
				chunk->seekRateDoubleAutomate = seekRateFader->getClickBetween();
				if (chunk->isLearning())
				{
					if ( ((seekRateFader->getTag() == kSeekRateRandMin) || 
								(chunk->seekRateDoubleAutomate)) && 
							(seekRateFader->getHandle() == gFaderHandleLeft) )
					{
						seekRateFader->setHandle(gGlowingFaderHandleLeft);
						seekRateFader->setDirty();
					}
					if ( ((seekRateFader->getTag() == kSeekRate) || 
								(chunk->seekRateDoubleAutomate)) && 
							(seekRateFader->getHandle2() == gFaderHandleRight) )
					{
						seekRateFader->setHandle2(gGlowingFaderHandleRight);
						seekRateFader->setDirty();
					}
				}
			}
			break;

		case kSeekDur:
		case kSeekDurRandMin:
			if (seekDurFader)
			{
				effect->setParameterAutomated(kSeekDur, seekDurFader->getValueTagged(kSeekDur));
				effect->setParameterAutomated(kSeekDurRandMin, seekDurFader->getValueTagged(kSeekDurRandMin));
				chunk->setLearner(seekDurFader->getTag());
				// set the automation link mode for the seek dur range slider
				chunk->seekDurDoubleAutomate = seekDurFader->getClickBetween();
				if (chunk->isLearning())
				{
					if ( ((seekDurFader->getTag() == kSeekDurRandMin) || 
								(chunk->seekDurDoubleAutomate)) && 
							(seekDurFader->getHandle() == gFaderHandleLeft) )
					{
						seekDurFader->setHandle(gGlowingFaderHandleLeft);
						seekDurFader->setDirty();
					}
					if ( ((seekDurFader->getTag() == kSeekDur) || 
								(chunk->seekDurDoubleAutomate)) && 
							(seekDurFader->getHandle2() == gFaderHandleRight) )
					{
						seekDurFader->setHandle2(gGlowingFaderHandleRight);
						seekDurFader->setDirty();
					}
				}
			}
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
			if (keyboard)
			{
				tag = keyboard->getTag();
				effect->setParameterAutomated(tag, keyboard->getValueTagged(tag));
				chunk->setLearner(tag, kEventBehaviourToggle);
			}
			break;

		case kFreeze:
		case kTempoSync:
		case kPortamento:
		case kSplitStereo:
		case kPitchConstraint:
			effect->setParameterAutomated(tag, control->getValue());
			// these are all on/off buttons, so set them up for toggle-style automation
			chunk->setLearner(tag, kEventBehaviourToggle);
			break;

		case kSeekRange:
		case kOctaveMin:
		case kOctaveMax:
		case kTempo:
		case kPredelay:
			effect->setParameterAutomated(tag, control->getValue());

			chunk->setLearner(tag);
			if (chunk->isLearning())
			{
				if (faders[tag] != NULL)
				{
					if (faders[tag]->getHandle() == gFaderHandle)
					{
						faders[tag]->setHandle(gGlowingFaderHandle);
						faders[tag]->setDirty();
					}
				}
				else if (tag == kOctaveMin)
				{
					if (octaveMinFader->getHandle() == gFaderHandleLeft)
					{
						octaveMinFader->setHandle(gGlowingFaderHandleLeft);
						octaveMinFader->setDirty();
					}
				}
				else if (tag == kOctaveMax)
				{
					if (octaveMaxFader->getHandle() == gFaderHandleRight)
					{
						octaveMaxFader->setHandle(gGlowingFaderHandleRight);
						octaveMaxFader->setDirty();
					}
				}
			}

			break;

		default:
			break;
	}

	control->update(context);
}

//-----------------------------------------------------------------------------
void ScrubbyEditor::idle()
{
	bool somethingChanged = false;


	if (isOpen)
	{
		// update the keyboard control if it has changed via MIDI notes
		if ( ((Scrubby*)effect)->keyboardWasPlayedByMidi )
		{
			if (keyboard)
			{
				for (int i=0; i < NUM_PITCH_STEPS; i++)
					keyboard->setValueTagged(kPitchStep0+i, effect->getParameter(kPitchStep0+i));
			}
			((Scrubby*)effect)->keyboardWasPlayedByMidi = false;	// reset it
			somethingChanged = true;
		}


		if (help != NULL)
		{
			// first look to see if any of the main control menus are moused over
			CControl *control = (CControl*) (frame->getCurrentView());
			long currentHelpIndex = help->getindex();
			long newHelpIndex = -1;
			if (control == NULL)
			{
				CPoint where;
				CRect titleZone(125, 8, 385, 45);
				frame->getCurrentLocation(where);
				if ( where.isInside(titleZone) )
					newHelpIndex = kGeneralHelp;
				else
					newHelpIndex = kNoHelp;
			}
			else
			{
				switch (control->getTag())
				{
					case kSeekRange:
						newHelpIndex = kSeekRangeHelp;
						break;
					case kFreeze:
						newHelpIndex = kFreezeHelp;
						break;
					case kSeekRate:
					case kSeekRateRandMin:
						newHelpIndex = kSeekRateHelp;
						break;
					case kTempoSync:
						newHelpIndex = kTempoSyncHelp;
						break;
					case kSeekDur:
					case kSeekDurRandMin:
						newHelpIndex = kSeekDurHelp;
						break;
					case kPortamento:
						newHelpIndex = kSpeedModeHelp;
						break;
					case kSplitStereo:
						newHelpIndex = kStereoHelp;
						break;
					case kPitchConstraint:
						newHelpIndex = kPitchConstraintHelp;
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
					case kTransposeDownButtonID:
					case kTransposeUpButtonID:
					case kMajorChordButtonID:
					case kMinorChordButtonID:
					case kAllNotesButtonID:
					case kNoneNotesButtonID:
						newHelpIndex = kNotesHelp;
						break;
					case kOctaveMin:
					case kOctaveMax:
						newHelpIndex = kOctavesHelp;
						break;
					case kTempo:
						newHelpIndex = kTempoHelp;
						break;
					case kPredelay:
						newHelpIndex = kPredelayHelp;
						break;
					case kMidiLearnButtonID:
						newHelpIndex = kMidiLearnHelp;
						break;
					case kMidiResetButtonID:
						newHelpIndex = kMidiResetHelp;
						break;
					default:
						newHelpIndex = kNoHelp;
						break;
				}
			}
			if ( (newHelpIndex >= 0) && (newHelpIndex != currentHelpIndex) )
			{
				help->setindex(newHelpIndex);
				somethingChanged = true;
			}
		}


		// turn off any glowing controls that are no longer learning
		for (long i=0; i < NUM_PARAMETERS; i++)
		{
			if ( (faders[i] != NULL) && (i != chunk->getLearner()) )
			{
				if (faders[i]->getHandle() == gGlowingFaderHandle)
				{
					faders[i]->setHandle(gFaderHandle);
					faders[i]->setDirty();
					somethingChanged = true;
				}
			}
		}
		//
		if (octaveMinFader)
		{
			if ( (chunk->getLearner() != kOctaveMin) && 
					(octaveMinFader->getHandle() == gGlowingFaderHandleLeft) )
			{
				octaveMinFader->setHandle(gFaderHandleLeft);
				octaveMinFader->setDirty();
				somethingChanged = true;
			}
		}
		//
		if (octaveMaxFader)
		{
			if ( (chunk->getLearner() != kOctaveMax) && 
					(octaveMaxFader->getHandle() == gGlowingFaderHandleRight) )
			{
				octaveMaxFader->setHandle(gFaderHandleRight);
				octaveMaxFader->setDirty();
				somethingChanged = true;
			}
		}
		//
		if (seekRateFader)
		{
			if ( ((chunk->getLearner() != kSeekRateRandMin) && 
					(seekRateFader->getHandle() == gGlowingFaderHandleLeft)) && 
					!((chunk->getLearner() == kSeekRate) && (chunk->seekRateDoubleAutomate)) )
			{
				seekRateFader->setHandle(gFaderHandleLeft);
				seekRateFader->setDirty();
				somethingChanged = true;
			}
			if ( ((chunk->getLearner() != kSeekRate) && 
					(seekRateFader->getHandle2() == gGlowingFaderHandleRight)) && 
					!((chunk->getLearner() == kSeekRateRandMin) && (chunk->seekRateDoubleAutomate)) )
			{
				seekRateFader->setHandle2(gFaderHandleRight);
				seekRateFader->setDirty();
				somethingChanged = true;
			}
		}
		//
		if (seekDurFader)
		{
			if ( ((chunk->getLearner() != kSeekDurRandMin) && 
					(seekDurFader->getHandle() == gGlowingFaderHandleLeft)) && 
					!((chunk->getLearner() == kSeekDur) && (chunk->seekDurDoubleAutomate)) )
			{
				seekDurFader->setHandle(gFaderHandleLeft);
				seekDurFader->setDirty();
				somethingChanged = true;
			}
			if ( ((chunk->getLearner() != kSeekDur) && 
					(seekDurFader->getHandle2() == gGlowingFaderHandleRight)) && 
					!((chunk->getLearner() == kSeekDurRandMin) && (chunk->seekDurDoubleAutomate)) )
			{
				seekDurFader->setHandle2(gFaderHandleRight);
				seekDurFader->setDirty();
				somethingChanged = true;
			}
		}


		// indicate that changed controls need to be redrawn
		if (somethingChanged)
			postUpdate();
	}

	// this is called so that idle() actually happens
	AEffGUIEditor::idle();
}
