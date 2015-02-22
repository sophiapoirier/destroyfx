/*------------------------------------------------------------------------
Copyright (C) 2001-2015  Sophia Poirier

This file is part of Buffer Override.

Buffer Override is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Buffer Override is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Buffer Override.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "bufferoverrideeditor.h"
#include "bufferoverride.h"



const char * kValueDisplayFont = "Helvetica";
const float kValueDisplayRegularFontSize = 10.8f;
const float kValueDisplayTinyFontSize = 10.2f;
const char * kHelpDisplayFont = "Helvetica";
const float kHelpDisplayFontSize = 9.6f;
const DGColor kHelpDisplayTextColor(201, 201, 201);


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

	kHostTempoButtonX = 386,
	kHostTempoButtonY = 6,

	kMidiLearnButtonX = 5,
	kMidiLearnButtonY = 63,
	kMidiResetButtonX = 4,
	kMidiResetButtonY = 86
};



//-----------------------------------------------------------------------------
// callbacks for button-triggered action

void linkKickButtonsDownProc(long, void* otherButton)
{
	DGButton* otherDGButton = reinterpret_cast<DGButton*>(otherButton);
	if (otherDGButton != NULL)
	{
		otherDGButton->setMouseIsDown(true);
	}
}

void linkKickButtonsUpProc(long, void* otherButton)
{
	DGButton* otherDGButton = reinterpret_cast<DGButton*>(otherButton);
	if (otherDGButton != NULL)
	{
		otherDGButton->setMouseIsDown(false);
	}
}


//-----------------------------------------------------------------------------
// value text display procedures

bool divisorDisplayProc(float value, char* outText, void*)
{
	if (value < 2.0f)
		sprintf(outText, "%.2f", 1.0f);
	else
		sprintf(outText, "%.2f", value);
	return true;
}

bool bufferSizeDisplayProc(float value, char* outText, void* editor)
{
	if ( ((DfxGuiEditor*)editor)->getparameter_b(kBufferTempoSync) )
		((DfxGuiEditor*)editor)->getparametervaluestring(kBufferSize_sync, outText);
	else
		sprintf(outText, "%.1f", value);
	return true;
}

bool divisorLFOrateDisplayProc(float value, char* outText, void* editor)
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
	return true;
}

bool bufferLFOrateDisplayProc(float value, char* outText, void* editor)
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
	return true;
}

bool LFOdepthDisplayProc(float value, char* outText, void*)
{
	sprintf(outText, "%ld%%", (long)value);
	return true;
}

bool smoothDisplayProc(float value, char* outText, void*)
{
	sprintf(outText, "%.1f%%", value);
	return true;
}

bool dryWetMixDisplayProc(float value, char* outText, void*)
{
	sprintf(outText, "%ld%%", (long)value);
	return true;
}

bool pitchbendDisplayProc(float value, char* outText, void*)
{
	sprintf(outText, "\xC2\xB1 %.2f", value);
	return true;
}

bool tempoDisplayProc(float value, char* outText, void*)
{
	sprintf(outText, "%.2f", value);
	return true;
}



//-----------------------------------------------------------------------------
DFX_EDITOR_ENTRY(BufferOverrideEditor)

//-----------------------------------------------------------------------------
BufferOverrideEditor::BufferOverrideEditor(DGEditorListenerInstance inInstance)
:	DfxGuiEditor(inInstance), 
	bufferSizeSlider(NULL), 
	divisorLFOrateSlider(NULL), 
	bufferLFOrateSlider(NULL), 
	bufferSizeDisplay(NULL), 
	divisorLFOrateDisplay(NULL), 
	bufferLFOrateDisplay(NULL), 
	helpDisplay(NULL)
{
}

//-----------------------------------------------------------------------------
long BufferOverrideEditor::OpenEditor()
{
	// create images

	// background image
	DGImage * gBackground = new DGImage("buffer-override-background.png", 0, this);
	SetBackgroundImage(gBackground);

	// slider handles
	DGImage * gSliderHandle = new DGImage("slider-handle.png", 0, this);
	DGImage * gXYboxHandle = new DGImage("xy-box-handle.png", 0, this);

	// buttons
	DGImage * gBufferTempoSyncButton = new DGImage("buffer-tempo-sync-button.png", 0, this);
	DGImage * gBufferTempoSyncButtonCorner = new DGImage("buffer-tempo-sync-button-corner.png", 0, this);
	DGImage * gBufferSizeLabel = new DGImage("buffer-size-label.png", 0, this);

	DGImage * gBufferInterruptButton = new DGImage("buffer-interrupt-button.png", 0, this);
	DGImage * gBufferInterruptButtonCorner = new DGImage("buffer-interrupt-button-corner.png", 0, this);

	DGImage * gDivisorLFOtempoSyncButton = new DGImage("divisor-lfo-tempo-sync-button.png", 0, this);
	DGImage * gDivisorLFOrateLabel = new DGImage("divisor-lfo-rate-label.png", 0, this);

	DGImage * gBufferLFOtempoSyncButton = new DGImage("buffer-lfo-tempo-sync-button.png", 0, this);
	DGImage * gBufferLFOrateLabel = new DGImage("buffer-lfo-rate-label.png", 0, this);

	DGImage * gDivisorLFOshapeSwitch = new DGImage("divisor-lfo-shape-switch.png", 0, this);

	DGImage * gBufferLFOshapeSwitch = new DGImage("buffer-lfo-shape-switch.png", 0, this);

	DGImage * gMidiModeButton = new DGImage("midi-mode-button.png", 0, this);
	DGImage * gMidiLearnButton = new DGImage("midi-learn-button.png", 0, this);
	DGImage * gMidiResetButton = new DGImage("midi-reset-button.png", 0, this);

	DGImage * gHostTempoButton = new DGImage("host-tempo-button.png", 0, this);


	// create controls
	
	DGRect pos;

	const long divisorLFOrateTag = getparameter_b(kDivisorLFOtempoSync) ? kDivisorLFOrate_sync : kDivisorLFOrate_abs;
	pos.set(kDivisorLFOrateSliderX, kDivisorLFOrateSliderY, kSliderWidth, kLFOsliderHeight);
	divisorLFOrateSlider = new DGSlider(this, divisorLFOrateTag, &pos, kDGAxis_horizontal, gSliderHandle);

	pos.set(kDivisorLFOdepthSliderX, kDivisorLFOdepthSliderY, kSliderWidth, kLFOsliderHeight);
	new DGSlider(this, kDivisorLFOdepth, &pos, kDGAxis_horizontal, gSliderHandle);

	const long bufferLFOrateTag = getparameter_b(kBufferLFOtempoSync) ? kBufferLFOrate_sync : kBufferLFOrate_abs;
	pos.set(kBufferLFOrateSliderX, kBufferLFOrateSliderY, kSliderWidth, kLFOsliderHeight);
	bufferLFOrateSlider = new DGSlider(this, bufferLFOrateTag, &pos, kDGAxis_horizontal, gSliderHandle);

	pos.set(kBufferLFOdepthSliderX, kBufferLFOdepthSliderY, kSliderWidth, kLFOsliderHeight);
	new DGSlider(this, kBufferLFOdepth, &pos, kDGAxis_horizontal, gSliderHandle);

	pos.set(kSmoothSliderX, kSmoothSliderY, kSliderWidth, kSliderHeight);
	new DGSlider(this, kSmooth, &pos, kDGAxis_horizontal, gSliderHandle);

	pos.set(kDryWetMixSliderX, kDryWetMixSliderY, kSliderWidth, kSliderHeight);
	new DGSlider(this, kDryWetMix, &pos, kDGAxis_horizontal, gSliderHandle);

	pos.set(kPitchbendSliderX, kPitchbendSliderY, kSliderWidth, kSliderHeight);
	new DGSlider(this, kPitchbend, &pos, kDGAxis_horizontal, gSliderHandle);

	pos.set(kTempoSliderX, kTempoSliderY, kTempoSliderWidth, kSliderHeight);
	new DGSlider(this, kTempo, &pos, kDGAxis_horizontal, gSliderHandle);

	pos.set(kDivisorBufferBoxX + 3, kDivisorBufferBoxY + 27, kDivisorBufferBoxWidth - 6, kSliderHeight);
	new DGSlider(this, kDivisor, &pos, kDGAxis_horizontal, gXYboxHandle);

	const long bufferSizeTag = getparameter_b(kBufferTempoSync) ? kBufferSize_sync : kBufferSize_abs;
	pos.offset(0, 57);
	bufferSizeSlider = new DGSlider(this, bufferSizeTag, &pos, kDGAxis_horizontal, gXYboxHandle);



	pos.set(kDivisorDisplayX, kDivisorDisplayY, kDisplayWidth, kDisplayHeight);
	new DGTextDisplay(this, kDivisor, &pos, divisorDisplayProc, NULL, NULL, kDGTextAlign_right, kValueDisplayRegularFontSize, kDGColor_white, kValueDisplayFont);

	pos.set(kBufferDisplayX, kBufferDisplayY, kDisplayWidth, kDisplayHeight);
	bufferSizeDisplay = new DGTextDisplay(this, bufferSizeTag, &pos, bufferSizeDisplayProc, this, NULL, kDGTextAlign_center, kValueDisplayRegularFontSize, kDGColor_white, kValueDisplayFont);

	pos.set(kDivisorLFOrateDisplayX, kDivisorLFOrateDisplayY, kLFOrateDisplayWidth, kDisplayHeight);
	divisorLFOrateDisplay = new DGTextDisplay(this, divisorLFOrateTag, &pos, divisorLFOrateDisplayProc, this, NULL, kDGTextAlign_right, kValueDisplayTinyFontSize, kDGColor_white, kValueDisplayFont);

	pos.set(kDivisorLFOdepthDisplayX, kDivisorLFOdepthDisplayY, kDisplayWidth, kDisplayHeight);
	new DGTextDisplay(this, kDivisorLFOdepth, &pos, LFOdepthDisplayProc, NULL, NULL, kDGTextAlign_right, kValueDisplayTinyFontSize, kDGColor_white, kValueDisplayFont);

	pos.set(kBufferLFOrateDisplayX, kBufferLFOrateDisplayY, kLFOrateDisplayWidth, kDisplayHeight);
	bufferLFOrateDisplay = new DGTextDisplay(this, bufferLFOrateTag, &pos, bufferLFOrateDisplayProc, this, NULL, kDGTextAlign_right, kValueDisplayTinyFontSize, kDGColor_white, kValueDisplayFont);

	pos.set(kBufferLFOdepthDisplayX, kBufferLFOdepthDisplayY, kDisplayWidth, kDisplayHeight);
	new DGTextDisplay(this, kBufferLFOdepth, &pos, LFOdepthDisplayProc, NULL, NULL, kDGTextAlign_right, kValueDisplayTinyFontSize, kDGColor_white, kValueDisplayFont);

	pos.set(kSmoothDisplayX, kSmoothDisplayY, kDisplayWidth, kDisplayHeight);
	new DGTextDisplay(this, kSmooth, &pos, smoothDisplayProc, NULL, NULL, kDGTextAlign_right, kValueDisplayRegularFontSize, kDGColor_white, kValueDisplayFont);

	pos.set(kDryWetMixDisplayX, kDryWetMixDisplayY, kDisplayWidth, kDisplayHeight);
	new DGTextDisplay(this, kDryWetMix, &pos, dryWetMixDisplayProc, NULL, NULL, kDGTextAlign_right, kValueDisplayRegularFontSize, kDGColor_white, kValueDisplayFont);

	pos.set(kPitchbendDisplayX, kPitchbendDisplayY, kDisplayWidth, kDisplayHeight);
	new DGTextDisplay(this, kPitchbend, &pos, pitchbendDisplayProc, NULL, NULL, kDGTextAlign_right, kValueDisplayRegularFontSize, kDGColor_white, kValueDisplayFont);

	pos.set(kTempoDisplayX, kTempoDisplayY, kDisplayWidth, kDisplayHeight);
	new DGTextDisplay(this, kTempo, &pos, tempoDisplayProc, NULL, NULL, kDGTextAlign_left, kValueDisplayTinyFontSize, kDGColor_white, kValueDisplayFont);


	// forced buffer size tempo sync button
	pos.set(kBufferTempoSyncButtonX, kBufferTempoSyncButtonY, gBufferTempoSyncButton->getWidth()/2, gBufferTempoSyncButton->getHeight()/2);
	DGButton * bufferTempoSyncButton = new DGButton(this, kBufferTempoSync, &pos, gBufferTempoSyncButton, 2, kDGButtonType_incbutton, true);
	//
	pos.set(kBufferTempoSyncButtonCornerX, kBufferTempoSyncButtonCornerY, gBufferTempoSyncButtonCorner->getWidth()/2, gBufferTempoSyncButtonCorner->getHeight()/2);
	DGButton * bufferTempoSyncButtonCorner = new DGButton(this, kBufferTempoSync, &pos, gBufferTempoSyncButtonCorner, 2, kDGButtonType_incbutton, true);
	//
	bufferTempoSyncButton->setUserProcedure(linkKickButtonsDownProc, bufferTempoSyncButtonCorner);
	bufferTempoSyncButtonCorner->setUserProcedure(linkKickButtonsDownProc, bufferTempoSyncButton);
	bufferTempoSyncButton->setUserReleaseProcedure(linkKickButtonsUpProc, bufferTempoSyncButtonCorner);
	bufferTempoSyncButtonCorner->setUserReleaseProcedure(linkKickButtonsUpProc, bufferTempoSyncButton);

	// buffer interrupt button
	pos.set(kBufferInterruptButtonX, kBufferInterruptButtonY, gBufferInterruptButton->getWidth()/2, gBufferInterruptButton->getHeight()/2);
	DGButton * bufferInterruptButton = new DGButton(this, kBufferInterrupt, &pos, gBufferInterruptButton, 2, kDGButtonType_incbutton, true);
	//
	pos.set(kBufferInterruptButtonCornerX, kBufferInterruptButtonCornerY, gBufferInterruptButtonCorner->getWidth()/2, gBufferInterruptButtonCorner->getHeight()/2);
	DGButton * bufferInterruptButtonCorner = new DGButton(this, kBufferInterrupt, &pos, gBufferInterruptButtonCorner, 2, kDGButtonType_incbutton, true);
	//
	bufferInterruptButtonCorner->setUserProcedure(linkKickButtonsDownProc, bufferInterruptButton);
	bufferInterruptButton->setUserProcedure(linkKickButtonsDownProc, bufferInterruptButtonCorner);
	bufferInterruptButtonCorner->setUserReleaseProcedure(linkKickButtonsUpProc, bufferInterruptButton);
	bufferInterruptButton->setUserReleaseProcedure(linkKickButtonsUpProc, bufferInterruptButtonCorner);

	// forced buffer size LFO tempo sync button
	pos.set(kBufferLFOtempoSyncButtonX, kBufferLFOtempoSyncButtonY, gBufferLFOtempoSyncButton->getWidth()/2, gBufferLFOtempoSyncButton->getHeight()/2);
	new DGButton(this, kBufferLFOtempoSync, &pos, gBufferLFOtempoSyncButton, 2, kDGButtonType_incbutton, true);

	// divisor LFO tempo sync button
	pos.set(kDivisorLFOtempoSyncButtonX, kDivisorLFOtempoSyncButtonY, gDivisorLFOtempoSyncButton->getWidth()/2, gDivisorLFOtempoSyncButton->getHeight()/2);
	new DGButton(this, kDivisorLFOtempoSync, &pos, gDivisorLFOtempoSyncButton, 2, kDGButtonType_incbutton, true);

	// MIDI mode button
	pos.set(kMidiModeButtonX, kMidiModeButtonY, gMidiModeButton->getWidth()/2, gMidiModeButton->getHeight()/kNumMidiModes);
	new DGButton(this, kMidiMode, &pos, gMidiModeButton, kNumMidiModes, kDGButtonType_incbutton, true);

	// sync to host tempo button
	pos.set(kHostTempoButtonX, kHostTempoButtonY, gHostTempoButton->getWidth(), gHostTempoButton->getHeight()/2);
	new DGButton(this, kTempoAuto, &pos, gHostTempoButton, 2, kDGButtonType_incbutton);

	// MIDI learn button
	CreateMidiLearnButton(kMidiLearnButtonX, kMidiLearnButtonY, gMidiLearnButton);

	// MIDI reset button
	CreateMidiResetButton(kMidiResetButtonX, kMidiResetButtonY, gMidiResetButton);


	// forced buffer size LFO shape switch
	pos.set(kBufferLFOshapeSwitchX, kBufferLFOshapeSwitchY, gBufferLFOshapeSwitch->getWidth(), gBufferLFOshapeSwitch->getHeight()/numLFOshapes);
	new DGButton(this, kBufferLFOshape, &pos, gBufferLFOshapeSwitch, numLFOshapes, kDGButtonType_radiobutton);

	// divisor LFO shape switch
	pos.set(kDivisorLFOshapeSwitchX, kDivisorLFOshapeSwitchY, gDivisorLFOshapeSwitch->getWidth(), gDivisorLFOshapeSwitch->getHeight()/numLFOshapes);
	new DGButton(this, kDivisorLFOshape, &pos, gDivisorLFOshapeSwitch, numLFOshapes, kDGButtonType_radiobutton);


	// forced buffer size label
	pos.set(kBufferSizeLabelX, kBufferSizeLabelY, gBufferSizeLabel->getWidth(), gBufferSizeLabel->getHeight()/2);
	new DGButton(this, kBufferTempoSync, &pos, gBufferSizeLabel, 2, kDGButtonType_picturereel);

	// forced buffer size LFO rate label
	pos.set(kBufferLFOrateLabelX, kBufferLFOrateLabelY, gBufferLFOrateLabel->getWidth(), gBufferLFOrateLabel->getHeight()/2);
	new DGButton(this, kBufferLFOtempoSync, &pos, gBufferLFOrateLabel, 2, kDGButtonType_picturereel);

	// divisor LFO rate label
	pos.set(kDivisorLFOrateLabelX, kDivisorLFOrateLabelY, gDivisorLFOrateLabel->getHeight(), gDivisorLFOrateLabel->getHeight()/2);
	new DGButton(this, kDivisorLFOtempoSync, &pos, gDivisorLFOrateLabel, 2, kDGButtonType_picturereel);


	// the help mouseover hint thingy
	pos.set(kHelpDisplayX, kHelpDisplayY, gBackground->getWidth(), kDisplayHeight);
	helpDisplay = new DGStaticTextDisplay(this, &pos, NULL, kDGTextAlign_center, kHelpDisplayFontSize, kHelpDisplayTextColor, kHelpDisplayFont);


	return noErr;
}


//-----------------------------------------------------------------------------
void BufferOverrideEditor::parameterChanged(long inParameterID)
{
	DGSlider * slider = NULL;
	DGTextDisplay * textDisplay = NULL;
	const bool useSyncParam = getparameter_b(inParameterID);

	long newParameterID;
	switch (inParameterID)
	{
		case kBufferTempoSync:
			newParameterID = useSyncParam ? kBufferSize_sync : kBufferSize_abs;
			slider = bufferSizeSlider;
			textDisplay = bufferSizeDisplay;
			break;
		case kDivisorLFOtempoSync:
			newParameterID = useSyncParam ? kDivisorLFOrate_sync : kDivisorLFOrate_abs;
			slider = divisorLFOrateSlider;
			textDisplay = divisorLFOrateDisplay;
			break;
		case kBufferLFOtempoSync:
			newParameterID = useSyncParam ? kBufferLFOrate_sync : kBufferLFOrate_abs;
			slider = bufferLFOrateSlider;
			textDisplay = bufferLFOrateDisplay;
			break;
		default:
			return;
	}

	if (slider)
		slider->setParameterID(newParameterID);
	if (textDisplay)
		textDisplay->setParameterID(newParameterID);
}


//-----------------------------------------------------------------------------
void BufferOverrideEditor::mouseovercontrolchanged(DGControl * currentControlUnderMouse)
{
	// there's no point in continuing
	if (helpDisplay == NULL)
		return;

	long currentcontrolparam = DFX_PARAM_INVALID_ID;
	if (currentControlUnderMouse != NULL)
	{
		if ( currentControlUnderMouse->isParameterAttached() )
			currentcontrolparam = currentControlUnderMouse->getParameterID();
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
#if TARGET_OS_MAC
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
		case kTempoAuto:
			strcpy(helpstring, "enable this to get the tempo from your sequencer");
			break;

		default:
			if (currentControlUnderMouse == NULL)
				strcpy(helpstring, " ");
			else if (currentControlUnderMouse == GetMidiLearnButton())
				strcpy(helpstring, "activate or deactivate learn mode for assigning MIDI CCs to control Buffer Override's parameters");
			else if (currentControlUnderMouse == GetMidiResetButton())
				strcpy(helpstring, "press this button to erase all of your CC assignments");
			else
				strcpy(helpstring, " ");
			break;
	}

	helpDisplay->setText(helpstring);
}
