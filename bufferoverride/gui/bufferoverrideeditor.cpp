#include "bufferoverrideeditor.h"
#include "bufferoverride.hpp"


#define VALUE_DISPLAY_FONT	"Helvetica"
const float VALUE_DISPLAY_REGULAR_FONT_SIZE = 10.8f;
const float VALUE_DISPLAY_TINY_FONT_SIZE = 10.2f;
#define HELP_DISPLAY_FONT	"Helvetica"
const float HELP_DISPLAY_FONT_SIZE = 9.6f;
const DGColor HELP_DISPLAY_TEXT_COLOR(201.0f/255.0f, 201.0f/255.0f, 201.0f/255.0f);



//-----------------------------------------------------------------------------
enum {
	// sliders
	kSliderWidth = 122,
	kSliderHeight = 14,
	kLFOsliderHeight = 18,
	//
	kDivisorLFOrateSliderX = 7,
	kDivisorLFOrateSliderY = 175,
	kDivisorLFOdepthSliderX = kDivisorLFOrateSliderX,
	kDivisorLFOdepthSliderY = 199,
	kBufferLFOrateSliderX = 351,
	kBufferLFOrateSliderY = 175,
	kBufferLFOdepthSliderX = kBufferLFOrateSliderX,
	kBufferLFOdepthSliderY = 199,
	//
	kPitchbendSliderX = 6,
	kPitchbendSliderY = 28,
	kSmoothSliderX = 354,
	kSmoothSliderY = 28,
	kDryWetMixSliderX = kSmoothSliderX,
	kDryWetMixSliderY = 65,
	//
	kTempoSliderWidth = 172,
	kTempoSliderX = 121,
	kTempoSliderY = 3,

	// displays
	kDisplayWidth = 36,
	kDisplayHeight = 12,
	//
	kDivisorDisplayX = 127 - kDisplayWidth,
	kDivisorDisplayY = 78,
	kBufferDisplayX = 241 - (kDisplayWidth/2),
	kBufferDisplayY = 195,
	//
	kLFOrateDisplayWidth = 21,
	kDivisorLFOrateDisplayX = 174 - kLFOrateDisplayWidth,//155,
	kDivisorLFOrateDisplayY = 188,
	kDivisorLFOdepthDisplayX = 171 - kDisplayWidth + 2,
	kDivisorLFOdepthDisplayY = 212,
	kBufferLFOrateDisplayX = 328 - kLFOrateDisplayWidth,
	kBufferLFOrateDisplayY = 188,
	kBufferLFOdepthDisplayX = 331 - kDisplayWidth,
	kBufferLFOdepthDisplayY = 212,
	//
	kPitchbendDisplayX = 101 - kDisplayWidth,
	kPitchbendDisplayY = 42,
	kSmoothDisplayX = 413 - kDisplayWidth,
	kSmoothDisplayY = 42,
	kDryWetMixDisplayX = 413 - kDisplayWidth,
	kDryWetMixDisplayY = 79,
	kTempoDisplayX = 312 - 1,
	kTempoDisplayY = 4,
	//
	kHelpDisplayX = 0,
	kHelpDisplayY = 231,

	// XY box
	kDivisorBufferBoxX = 156,
	kDivisorBufferBoxY = 40,
	kDivisorBufferBoxWidth = 169,
	kDivisorBufferBoxHeight = 138,

	// buttons
	kBufferInterruptButtonX = 227,
	kBufferInterruptButtonY = 20,
	kBufferInterruptButtonCornerX = 325,
	kBufferInterruptButtonCornerY = 20,

	kBufferTempoSyncButtonX = 156,
	kBufferTempoSyncButtonY = 20,
	kBufferTempoSyncButtonCornerX = 133,
	kBufferTempoSyncButtonCornerY = 20,
	kBufferSizeLabelX = 222,
	kBufferSizeLabelY = 212,

	kMidiModeButtonX = 5,
	kMidiModeButtonY = 6,

	kDivisorLFOshapeSwitchX = 8,
	kDivisorLFOshapeSwitchY = 145,
	kDivisorLFOtempoSyncButtonX = 52,
	kDivisorLFOtempoSyncButtonY = 119,
	kDivisorLFOrateLabelX = 178,
	kDivisorLFOrateLabelY = 188,

	kBufferLFOshapeSwitchX = 352,
	kBufferLFOshapeSwitchY = 145,
	kBufferLFOtempoSyncButtonX = 351,
	kBufferLFOtempoSyncButtonY = 120,
	kBufferLFOrateLabelX = 277,
	kBufferLFOrateLabelY = 188,

	kMidiLearnButtonX = 5,
	kMidiLearnButtonY = 63,
	kMidiResetButtonX = 4,
	kMidiResetButtonY = 86,

	kGoButtonX = 386,
	kGoButtonY = 6,
};



//-----------------------------------------------------------------------------
// callbacks for button-triggered action

void midilearnBufferOverride(long value, void * editor)
{
	if (editor != NULL)
	{
		if (value == 0)
			((DfxGuiEditor*)editor)->setmidilearning(false);
		else
			((DfxGuiEditor*)editor)->setmidilearning(true);
	}
}

void midiresetBufferOverride(long value, void * editor)
{
	if ( (editor != NULL) && (value != 0) )
		((DfxGuiEditor*)editor)->resetmidilearn();
}

void linkKickButtonsDownProc(long, void * otherbutton)
{
	if (otherbutton != NULL)
	{
		((DGButton*)otherbutton)->setMouseIsDown(true);
		((DGButton*)otherbutton)->redraw();
	}
}

void linkKickButtonsUpProc(long, void * otherbutton)
{
	if (otherbutton != NULL)
	{
		((DGButton*)otherbutton)->setMouseIsDown(false);
		((DGButton*)otherbutton)->redraw();
	}
}


//-----------------------------------------------------------------------------
// parameter listener procedure
static void TempoSyncListenerProc(void * inRefCon, void * inObject, const AudioUnitParameter * inParameter, Float32 inValue)
{
	if ( (inObject == NULL) || (inParameter == NULL) )
		return;

	DGControl * control = (DGControl*) inObject;
	bool useSyncParam = FBOOL(inValue);
	AudioUnitParameterID newParameterID;
	switch (inParameter->mParameterID)
	{
		case kBufferTempoSync:
			newParameterID = useSyncParam ? kBufferSize_sync : kBufferSize_abs;
			break;
		case kDivisorLFOtempoSync:
			newParameterID = useSyncParam ? kDivisorLFOrate_sync : kDivisorLFOrate_abs;
			break;
		case kBufferLFOtempoSync:
			newParameterID = useSyncParam ? kBufferLFOrate_sync : kBufferLFOrate_abs;
			break;
		default:
			return;
	}
	control->setParameterID(newParameterID);
}


//-----------------------------------------------------------------------------
// value text display procedures

void divisorDisplayProc(float value, char * outText, void *)
{
	if (value < 2.0f)
		sprintf(outText, "%.2f", 1.0f);
	else
		sprintf(outText, "%.2f", value);
}

void bufferSizeDisplayProc(float value, char * outText, void * editor)
{
	if ( ((DfxGuiEditor*)editor)->getparameter_b(kBufferTempoSync) )
		((DfxGuiEditor*)editor)->getparametervaluestring(kBufferSize_sync, outText);
	else
		sprintf(outText, "%.1f", value);
}

void divisorLFOrateDisplayProc(float value, char * outText, void * editor)
{
	if ( ((DfxGuiEditor*)editor)->getparameter_b(kDivisorLFOtempoSync) )
		((DfxGuiEditor*)editor)->getparametervaluestring(kDivisorLFOrate_sync, outText);
	else
	{
		if (value < 10.0f)
			sprintf(outText, "%.2f", value);
		else
			sprintf(outText, "%.1f", value);
	}
}

void bufferLFOrateDisplayProc(float value, char * outText, void * editor)
{
	if ( ((DfxGuiEditor*)editor)->getparameter_b(kBufferLFOtempoSync) )
		((DfxGuiEditor*)editor)->getparametervaluestring(kBufferLFOrate_sync, outText);
	else
	{
		if (value < 10.0f)
			sprintf(outText, "%.2f", value);
		else
			sprintf(outText, "%.1f", value);
	}
}

void LFOdepthDisplayProc(float value, char * outText, void *)
{
	sprintf(outText, "%ld%%", (long)value);
}

void smoothDisplayProc(float value, char * outText, void *)
{
	sprintf(outText, "%.1f%%", value);
}

void dryWetMixDisplayProc(float value, char * outText, void *)
{
	sprintf(outText, "%ld%%", (long)value);
}

void pitchbendDisplayProc(float value, char * outText, void *)
{
	sprintf(outText, "\xB1 %.2f", value);
}

void tempoDisplayProc(float value, char * outText, void *)
{
	sprintf(outText, "%.2f", value);
}



//-----------------------------------------------------------------------------
COMPONENT_ENTRY(BufferOverrideEditor);

//-----------------------------------------------------------------------------
BufferOverrideEditor::BufferOverrideEditor(AudioUnitCarbonView inInstance)
:	DfxGuiEditor(inInstance)
{
	bufferSizeSlider = NULL;
	divisorLFOrateSlider = NULL;
	bufferLFOrateSlider = NULL;
	bufferSizeDisplay = NULL;
	divisorLFOrateDisplay = NULL;
	bufferLFOrateDisplay = NULL;

	midilearnButton = NULL;
	midiresetButton = NULL;
	helpDisplay = NULL;

	AUListenerCreate(TempoSyncListenerProc, this,
		CFRunLoopGetCurrent(), kCFRunLoopDefaultMode, 0.030f, // 30 ms
		&parameterListener);
}

//-----------------------------------------------------------------------------
BufferOverrideEditor::~BufferOverrideEditor()
{
	if (bufferSizeSlider != NULL)
		AUListenerRemoveParameter(parameterListener, bufferSizeSlider, &bufferSizeTempoSyncAUP);
	if (bufferSizeDisplay != NULL)
		AUListenerRemoveParameter(parameterListener, bufferSizeDisplay, &bufferSizeTempoSyncAUP);
	if (divisorLFOrateSlider != NULL)
		AUListenerRemoveParameter(parameterListener, divisorLFOrateSlider, &divisorLFOtempoSyncAUP);
	if (divisorLFOrateDisplay != NULL)
		AUListenerRemoveParameter(parameterListener, divisorLFOrateDisplay, &divisorLFOtempoSyncAUP);
	if (bufferLFOrateSlider != NULL)
		AUListenerRemoveParameter(parameterListener, bufferLFOrateSlider, &bufferLFOtempoSyncAUP);
	if (bufferLFOrateDisplay != NULL)
		AUListenerRemoveParameter(parameterListener, bufferLFOrateDisplay, &bufferLFOtempoSyncAUP);

	AUListenerDispose(parameterListener);
}

//-----------------------------------------------------------------------------
long BufferOverrideEditor::open()
{
	// create images

	// background image
	DGImage * gBackground = new DGImage("buffer-override-background.png", this);
	SetBackgroundImage(gBackground);

	// slider handles
	DGImage * gSliderHandle = new DGImage("slider-handle.png", this);
	DGImage * gXYboxHandle = new DGImage("xy-box-handle.png", this);

	// buttons
	DGImage * gBufferTempoSyncButton = new DGImage("buffer-tempo-sync-button.png", this);
	DGImage * gBufferTempoSyncButtonCorner = new DGImage("buffer-tempo-sync-button-corner.png", this);
	DGImage * gBufferSizeLabel = new DGImage("buffer-size-label.png", this);

	DGImage * gBufferInterruptButton = new DGImage("buffer-interrupt-button.png", this);
	DGImage * gBufferInterruptButtonCorner = new DGImage("buffer-interrupt-button-corner.png", this);

	DGImage * gDivisorLFOtempoSyncButton = new DGImage("divisor-lfo-tempo-sync-button.png", this);
	DGImage * gDivisorLFOrateLabel = new DGImage("divisor-lfo-rate-label.png", this);

	DGImage * gBufferLFOtempoSyncButton = new DGImage("buffer-lfo-tempo-sync-button.png", this);
	DGImage * gBufferLFOrateLabel = new DGImage("buffer-lfo-rate-label.png", this);

	DGImage * gDivisorLFOshapeSwitch = new DGImage("divisor-lfo-shape-switch.png", this);

	DGImage * gBufferLFOshapeSwitch = new DGImage("buffer-lfo-shape-switch.png", this);

	DGImage * gMidiModeButton = new DGImage("midi-mode-button.png", this);
	DGImage * gMidiLearnButton = new DGImage("midi-learn-button.png", this);
	DGImage * gMidiResetButton = new DGImage("midi-reset-button.png", this);

	DGImage * gGoButton = new DGImage("go-button.png", this);


	// create controls
	
	DGRect pos;
	DGSlider * slider;

	long divisorLFOrateTag = getparameter_b(kDivisorLFOtempoSync) ? kDivisorLFOrate_sync : kDivisorLFOrate_abs;
	pos.set (kDivisorLFOrateSliderX, kDivisorLFOrateSliderY, kSliderWidth, kLFOsliderHeight);
	divisorLFOrateSlider = new DGSlider(this, divisorLFOrateTag, &pos, kDGSliderAxis_horizontal, gSliderHandle, NULL);

	pos.set (kDivisorLFOdepthSliderX, kDivisorLFOdepthSliderY, kSliderWidth, kLFOsliderHeight);
	slider = new DGSlider(this, kDivisorLFOdepth, &pos, kDGSliderAxis_horizontal, gSliderHandle, NULL);

	long bufferLFOrateTag = getparameter_b(kBufferLFOtempoSync) ? kBufferLFOrate_sync : kBufferLFOrate_abs;
	pos.set (kBufferLFOrateSliderX, kBufferLFOrateSliderY, kSliderWidth, kLFOsliderHeight);
	bufferLFOrateSlider = new DGSlider(this, bufferLFOrateTag, &pos, kDGSliderAxis_horizontal, gSliderHandle, NULL);

	pos.set (kBufferLFOdepthSliderX, kBufferLFOdepthSliderY, kSliderWidth, kLFOsliderHeight);
	slider = new DGSlider(this, kBufferLFOdepth, &pos, kDGSliderAxis_horizontal, gSliderHandle, NULL);

	pos.set (kSmoothSliderX, kSmoothSliderY, kSliderWidth, kSliderHeight);
	slider = new DGSlider(this, kSmooth, &pos, kDGSliderAxis_horizontal, gSliderHandle, NULL);

	pos.set (kDryWetMixSliderX, kDryWetMixSliderY, kSliderWidth, kSliderHeight);
	slider = new DGSlider(this, kDryWetMix, &pos, kDGSliderAxis_horizontal, gSliderHandle, NULL);

	pos.set (kPitchbendSliderX, kPitchbendSliderY, kSliderWidth, kSliderHeight);
	slider = new DGSlider(this, kPitchbend, &pos, kDGSliderAxis_horizontal, gSliderHandle, NULL);

	pos.set (kTempoSliderX, kTempoSliderY, kTempoSliderWidth, kSliderHeight);
	slider = new DGSlider(this, kTempo, &pos, kDGSliderAxis_horizontal, gSliderHandle, NULL);

	pos.set (kDivisorBufferBoxX + 3, kDivisorBufferBoxY + 27, kDivisorBufferBoxWidth - 6, kSliderHeight);
	slider = new DGSlider(this, kDivisor, &pos, kDGSliderAxis_horizontal, gXYboxHandle, NULL);

	long bufferSizeTag = getparameter_b(kBufferTempoSync) ? kBufferSize_sync : kBufferSize_abs;
	pos.offset(0, 57);
	bufferSizeSlider = new DGSlider(this, bufferSizeTag, &pos, kDGSliderAxis_horizontal, gXYboxHandle, NULL);



	DGTextDisplay * display;

	pos.set (kDivisorDisplayX, kDivisorDisplayY, kDisplayWidth, kDisplayHeight);
	display = new DGTextDisplay(this, kDivisor, &pos, divisorDisplayProc, NULL, NULL, kDGTextAlign_right, VALUE_DISPLAY_REGULAR_FONT_SIZE, kWhiteDGColor, VALUE_DISPLAY_FONT);

	pos.set (kBufferDisplayX, kBufferDisplayY, kDisplayWidth, kDisplayHeight);
	bufferSizeDisplay = new DGTextDisplay(this, bufferSizeTag, &pos, bufferSizeDisplayProc, this, NULL, kDGTextAlign_center, VALUE_DISPLAY_REGULAR_FONT_SIZE, kWhiteDGColor, VALUE_DISPLAY_FONT);

	pos.set (kDivisorLFOrateDisplayX, kDivisorLFOrateDisplayY, kLFOrateDisplayWidth, kDisplayHeight);
	divisorLFOrateDisplay = new DGTextDisplay(this, divisorLFOrateTag, &pos, divisorLFOrateDisplayProc, this, NULL, kDGTextAlign_right, VALUE_DISPLAY_TINY_FONT_SIZE, kWhiteDGColor, VALUE_DISPLAY_FONT);

	pos.set (kDivisorLFOdepthDisplayX, kDivisorLFOdepthDisplayY, kDisplayWidth, kDisplayHeight);
	display = new DGTextDisplay(this, kDivisorLFOdepth, &pos, LFOdepthDisplayProc, NULL, NULL, kDGTextAlign_right, VALUE_DISPLAY_TINY_FONT_SIZE, kWhiteDGColor, VALUE_DISPLAY_FONT);

	pos.set (kBufferLFOrateDisplayX, kBufferLFOrateDisplayY, kLFOrateDisplayWidth, kDisplayHeight);
	bufferLFOrateDisplay = new DGTextDisplay(this, bufferLFOrateTag, &pos, bufferLFOrateDisplayProc, this, NULL, kDGTextAlign_right, VALUE_DISPLAY_TINY_FONT_SIZE, kWhiteDGColor, VALUE_DISPLAY_FONT);

	pos.set (kBufferLFOdepthDisplayX, kBufferLFOdepthDisplayY, kDisplayWidth, kDisplayHeight);
	display = new DGTextDisplay(this, kBufferLFOdepth, &pos, LFOdepthDisplayProc, NULL, NULL, kDGTextAlign_right, VALUE_DISPLAY_TINY_FONT_SIZE, kWhiteDGColor, VALUE_DISPLAY_FONT);

	pos.set (kSmoothDisplayX, kSmoothDisplayY, kDisplayWidth, kDisplayHeight);
	display = new DGTextDisplay(this, kSmooth, &pos, smoothDisplayProc, NULL, NULL, kDGTextAlign_right, VALUE_DISPLAY_REGULAR_FONT_SIZE, kWhiteDGColor, VALUE_DISPLAY_FONT);

	pos.set (kDryWetMixDisplayX, kDryWetMixDisplayY, kDisplayWidth, kDisplayHeight);
	display = new DGTextDisplay(this, kDryWetMix, &pos, dryWetMixDisplayProc, NULL, NULL, kDGTextAlign_right, VALUE_DISPLAY_REGULAR_FONT_SIZE, kWhiteDGColor, VALUE_DISPLAY_FONT);

	pos.set (kPitchbendDisplayX, kPitchbendDisplayY, kDisplayWidth, kDisplayHeight);
	display = new DGTextDisplay(this, kPitchbend, &pos, pitchbendDisplayProc, NULL, NULL, kDGTextAlign_right, VALUE_DISPLAY_REGULAR_FONT_SIZE, kWhiteDGColor, VALUE_DISPLAY_FONT);

	pos.set (kTempoDisplayX, kTempoDisplayY, kDisplayWidth, kDisplayHeight);
	display = new DGTextDisplay(this, kTempo, &pos, tempoDisplayProc, NULL, NULL, kDGTextAlign_left, VALUE_DISPLAY_TINY_FONT_SIZE, kWhiteDGColor, VALUE_DISPLAY_FONT);


	DGButton *button;

	// forced buffer size tempo sync button
	pos.set (kBufferTempoSyncButtonX, kBufferTempoSyncButtonY, gBufferTempoSyncButton->getWidth()/2, gBufferTempoSyncButton->getHeight()/2);
	DGButton * bufferTempoSyncButton = new DGButton(this, kBufferTempoSync, &pos, gBufferTempoSyncButton, 2, kDGButtonType_incbutton, true);
	//
	pos.set (kBufferTempoSyncButtonCornerX, kBufferTempoSyncButtonCornerY, gBufferTempoSyncButtonCorner->getWidth()/2, gBufferTempoSyncButtonCorner->getHeight()/2);
	DGButton * bufferTempoSyncButtonCorner = new DGButton(this, kBufferTempoSync, &pos, gBufferTempoSyncButtonCorner, 2, kDGButtonType_incbutton, true);
	//
	bufferTempoSyncButton->setUserProcedure(linkKickButtonsDownProc, bufferTempoSyncButtonCorner);
	bufferTempoSyncButtonCorner->setUserProcedure(linkKickButtonsDownProc, bufferTempoSyncButton);
	bufferTempoSyncButton->setUserReleaseProcedure(linkKickButtonsUpProc, bufferTempoSyncButtonCorner);
	bufferTempoSyncButtonCorner->setUserReleaseProcedure(linkKickButtonsUpProc, bufferTempoSyncButton);

	// buffer interrupt button
	pos.set (kBufferInterruptButtonX, kBufferInterruptButtonY, gBufferInterruptButton->getWidth()/2, gBufferInterruptButton->getHeight()/2);
	DGButton * bufferInterruptButton = new DGButton(this, kBufferInterrupt, &pos, gBufferInterruptButton, 2, kDGButtonType_incbutton, true);
	//
	pos.set (kBufferInterruptButtonCornerX, kBufferInterruptButtonCornerY, gBufferInterruptButtonCorner->getWidth()/2, gBufferInterruptButtonCorner->getHeight()/2);
	DGButton * bufferInterruptButtonCorner = new DGButton(this, kBufferInterrupt, &pos, gBufferInterruptButtonCorner, 2, kDGButtonType_incbutton, true);
	//
	bufferInterruptButtonCorner->setUserProcedure(linkKickButtonsDownProc, bufferInterruptButton);
	bufferInterruptButton->setUserProcedure(linkKickButtonsDownProc, bufferInterruptButtonCorner);
	bufferInterruptButtonCorner->setUserReleaseProcedure(linkKickButtonsUpProc, bufferInterruptButton);
	bufferInterruptButton->setUserReleaseProcedure(linkKickButtonsUpProc, bufferInterruptButtonCorner);

	// forced buffer size LFO tempo sync button
	pos.set (kBufferLFOtempoSyncButtonX, kBufferLFOtempoSyncButtonY, gBufferLFOtempoSyncButton->getWidth()/2, gBufferLFOtempoSyncButton->getHeight()/2);
	button = new DGButton(this, kBufferLFOtempoSync, &pos, gBufferLFOtempoSyncButton, 2, kDGButtonType_incbutton, true);

	// divisor LFO tempo sync button
	pos.set (kDivisorLFOtempoSyncButtonX, kDivisorLFOtempoSyncButtonY, gDivisorLFOtempoSyncButton->getWidth()/2, gDivisorLFOtempoSyncButton->getHeight()/2);
	button = new DGButton(this, kDivisorLFOtempoSync, &pos, gDivisorLFOtempoSyncButton, 2, kDGButtonType_incbutton, true);

	// MIDI mode button
	pos.set (kMidiModeButtonX, kMidiModeButtonY, gMidiModeButton->getWidth()/2, gMidiModeButton->getHeight()/kNumMidiModes);
	button = new DGButton(this, kMidiMode, &pos, gMidiModeButton, kNumMidiModes, kDGButtonType_incbutton, true);

	// MIDI learn button
	pos.set (kMidiLearnButtonX, kMidiLearnButtonY, gMidiLearnButton->getWidth(), gMidiLearnButton->getHeight()/2);
	midilearnButton = new DGButton(this, &pos, gMidiLearnButton, 2, kDGButtonType_incbutton);
	midilearnButton->setUserProcedure(midilearnBufferOverride, this);

	// MIDI reset button
	pos.set (kMidiResetButtonX, kMidiResetButtonY, gMidiResetButton->getWidth(), gMidiResetButton->getHeight()/2);
	midiresetButton = new DGButton(this, &pos, gMidiResetButton, 2, kDGButtonType_pushbutton);
	midiresetButton->setUserProcedure(midiresetBufferOverride, this);

	// go button
	pos.set (kGoButtonX, kGoButtonY, gGoButton->getWidth(), gGoButton->getHeight()/2);
	DGWebLink * goButton = new DGWebLink(this, &pos, gGoButton, "http://www.factoryfarming.com/");


	// forced buffer size LFO shape switch
	pos.set (kBufferLFOshapeSwitchX, kBufferLFOshapeSwitchY, gBufferLFOshapeSwitch->getWidth(), gBufferLFOshapeSwitch->getHeight()/numLFOshapes);
	button = new DGButton(this, kBufferLFOshape, &pos, gBufferLFOshapeSwitch, numLFOshapes, kDGButtonType_radiobutton);

	// divisor LFO shape switch
	pos.set (kDivisorLFOshapeSwitchX, kDivisorLFOshapeSwitchY, gDivisorLFOshapeSwitch->getWidth(), gDivisorLFOshapeSwitch->getHeight()/numLFOshapes);
	button = new DGButton(this, kDivisorLFOshape, &pos, gDivisorLFOshapeSwitch, numLFOshapes, kDGButtonType_radiobutton);


	// forced buffer size label
	pos.set (kBufferSizeLabelX, kBufferSizeLabelY, gBufferSizeLabel->getWidth(), gBufferSizeLabel->getHeight()/2);
	button = new DGButton(this, kBufferTempoSync, &pos, gBufferSizeLabel, 2, kDGButtonType_picturereel);

	// forced buffer size LFO rate label
	pos.set (kBufferLFOrateLabelX, kBufferLFOrateLabelY, gBufferLFOrateLabel->getWidth(), gBufferLFOrateLabel->getHeight()/2);
	button = new DGButton(this, kBufferLFOtempoSync, &pos, gBufferLFOrateLabel, 2, kDGButtonType_picturereel);

	// divisor LFO rate label
	pos.set (kDivisorLFOrateLabelX, kDivisorLFOrateLabelY, gDivisorLFOrateLabel->getHeight(), gDivisorLFOrateLabel->getHeight()/2);
	button = new DGButton(this, kDivisorLFOtempoSync, &pos, gDivisorLFOrateLabel, 2, kDGButtonType_picturereel);


/*
HMHelpContentRec helptag;
helptag.version = kMacHelpVersion;
SetRect(&(helptag.absHotRect), 0, 0, 0, 0);
helptag.tagSide = kHMInsideTopCenterAligned;
helptag.content[0].contentType = kHMCFStringContent;
helptag.content[0].u.tagCFString = CFSTR("ooo yeah, MIDI mode shitz");
helptag.content[1].contentType = kHMCFStringContent;
helptag.content[1].u.tagCFString = CFSTR("who deh fuck yoo lookin' at?");
HMSetControlHelpContent(midiModeButton->getCarbonControl(), &helptag);
HMSetHelpTagsDisplayed(true);
*/


	// the help mouseover hint thingy
	pos.set (kHelpDisplayX, kHelpDisplayY, gBackground->getWidth(), kDisplayHeight);
	helpDisplay = new DGStaticTextDisplay(this, &pos, NULL, kDGTextAlign_center, HELP_DISPLAY_FONT_SIZE, HELP_DISPLAY_TEXT_COLOR, HELP_DISPLAY_FONT);


	bufferSizeTempoSyncAUP.mAudioUnit = divisorLFOtempoSyncAUP.mAudioUnit = bufferLFOtempoSyncAUP.mAudioUnit = GetEditAudioUnit();
	bufferSizeTempoSyncAUP.mScope = divisorLFOtempoSyncAUP.mScope = bufferLFOtempoSyncAUP.mScope = kAudioUnitScope_Global;
	bufferSizeTempoSyncAUP.mElement = divisorLFOtempoSyncAUP.mElement = bufferLFOtempoSyncAUP.mElement = (AudioUnitElement)0;
	bufferSizeTempoSyncAUP.mParameterID = kBufferTempoSync;
	divisorLFOtempoSyncAUP.mParameterID = kDivisorLFOtempoSync;
	bufferLFOtempoSyncAUP.mParameterID = kBufferLFOtempoSync;

	AUListenerAddParameter(parameterListener, bufferSizeSlider, &bufferSizeTempoSyncAUP);
	AUListenerAddParameter(parameterListener, bufferSizeDisplay, &bufferSizeTempoSyncAUP);
	AUListenerAddParameter(parameterListener, divisorLFOrateSlider, &divisorLFOtempoSyncAUP);
	AUListenerAddParameter(parameterListener, divisorLFOrateDisplay, &divisorLFOtempoSyncAUP);
	AUListenerAddParameter(parameterListener, bufferLFOrateSlider, &bufferLFOtempoSyncAUP);
	AUListenerAddParameter(parameterListener, bufferLFOrateDisplay, &bufferLFOtempoSyncAUP);


HMSetTagDelay(9);	// make the hints appear quickly <-- XXX this is sort of a hack

	return noErr;
}


//-----------------------------------------------------------------------------
void BufferOverrideEditor::mouseovercontrolchanged()
{
	// there's no point in continuing
	if (helpDisplay == NULL)
		return;

	DGControl * currentcontrol = getCurrentControl_mouseover();
	long currentcontrolparam = DFX_PARAM_INVALID_ID;
	if (currentcontrol != NULL)
	{
		if ( currentcontrol->isParameterAttached() )
			currentcontrolparam = currentcontrol->getParameterID();
	}

	char helpstring[256];
	switch (currentcontrolparam)
	{
		case kDivisor:
			sprintf(helpstring, "buffer divisor is the number of times each forced buffer skips & starts over");
			break;
		case kBufferSize_abs:
		case kBufferSize_sync:
			sprintf(helpstring, "forced buffer size is the length of the sound chunks that Buffer Override works with");
			break;
//		case kBufferDivisorHelpTag:
#if MAC
//			sprintf(helpstring, "left/right is buffer divisor (the number of skips in a forced buffer, hold ctrl).   up/down is forced buffer size (hold option)");
#else
			// shorten display text for Windows (beware larger fonts)
//			sprintf(helpstring, "left/right is buffer divisor (number of skips in a buffer, hold ctrl).  up/down is forced buffer size (hold alt)");
#endif
//			break;
		case kBufferTempoSync:
			strcpy(helpstring, "turn tempo sync on if you want the size of the forced buffers to sync to your tempo");
			break;
		case kBufferInterrupt:
			strcpy(helpstring, "turn this off for the old version 1 style of stubborn \"stuck\" buffers (if you really want that)");
			break;
		case kDivisorLFOrate_abs:
		case kDivisorLFOrate_sync:
			strcpy(helpstring, "this is the speed of the LFO that modulates the buffer divisor");
			break;
		case kDivisorLFOdepth:
			strcpy(helpstring, "the depth (or intensity) of the LFO that modulates the buffer divisor (0% does nothing)");
			break;
		case kDivisorLFOshape:
			strcpy(helpstring, "choose the waveform shape of the LFO that modulates the buffer divisor");
			break;
		case kDivisorLFOtempoSync:
			strcpy(helpstring, "turn tempo sync on if you want the rate of the buffer divisor LFO to sync to your tempo");
			break;
		case kBufferLFOrate_abs:
		case kBufferLFOrate_sync:
			strcpy(helpstring, "this is the speed of the LFO that modulates the forced buffer size");
			break;
		case kBufferLFOdepth:
			strcpy(helpstring, "the depth (or intensity) of the LFO that modulates the forced buffer size (0% does nothing)");
			break;
		case kBufferLFOshape:
			strcpy(helpstring, "choose the waveform shape of the LFO that modulates the forced buffer size");
			break;
		case kBufferLFOtempoSync:
			strcpy(helpstring, "turn tempo sync on if you want the rate of the forced buffer size LFO to sync to your tempo");
			break;
		case kSmooth:
			strcpy(helpstring, "the portion of each minibuffer spent smoothly crossfading the previous one into the new one (prevents glitches)");
			break;
		case kDryWetMix:
			strcpy(helpstring, "the relative mix of the processed sound & the clean/original sound (100% is all processed)");
			break;
		case kPitchbend:
			strcpy(helpstring, "the range, in semitones, of the MIDI pitchbend wheel's effect on the buffer divisor");
			break;
		case kMidiMode:
			strcpy(helpstring, "nudge: MIDI notes adjust the buffer divisor.   trigger: notes also reset the divisor to 1 when they are released");
			break;
		case kTempo:
			strcpy(helpstring, "you can adjust the tempo that Buffer Override uses, or set this to \"auto\" to get the tempo from your sequencer");
			break;
//		case kTempoTextEdit:
//			strcpy(helpstring, "you can type in the tempo that Buffer Override uses, or set this to \"auto\" to get the tempo from your sequencer");
//			break;

		default:
			if (currentcontrol == NULL)
				strcpy(helpstring, " ");
			else if (currentcontrol == midilearnButton)
				strcpy(helpstring, "activate or deactivate learn mode for assigning MIDI CCs to control Buffer Override's parameters");
			else if (currentcontrol == midiresetButton)
				strcpy(helpstring, "press this button to erase all of your CC assignments");
			else
				strcpy(helpstring, " ");
			break;
	}

	helpDisplay->setText(helpstring);
}
