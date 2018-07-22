/*------------------------------------------------------------------------
Copyright (C) 2001-2018  Sophia Poirier

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



static char const* const kValueDisplayFont = "Helvetica";
constexpr float kValueDisplayRegularFontSize = 10.8f;
constexpr float kValueDisplayTinyFontSize = 10.2f;
static char const* const kHelpDisplayFont = "Helvetica";
constexpr float kHelpDisplayFontSize = 9.6f;
static DGColor const kHelpDisplayTextColor(201, 201, 201);


//-----------------------------------------------------------------------------
enum
{
	// sliders
	kSliderWidth = 122,
	kSliderHeight = 14,
	kLFOSliderHeight = 18,
	//
	kDivisorLFORateSliderX = 7,
	kDivisorLFORateSliderY = 175,
	kDivisorLFODepthSliderX = kDivisorLFORateSliderX,
	kDivisorLFODepthSliderY = 199,
	kBufferLFORateSliderX = 351,
	kBufferLFORateSliderY = 175,
	kBufferLFODepthSliderX = kBufferLFORateSliderX,
	kBufferLFODepthSliderY = 199,
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
	kBufferDisplayX = 241 - (kDisplayWidth / 2),
	kBufferDisplayY = 195,
	//
	kLFORateDisplayWidth = 21,
	kDivisorLFORateDisplayX = 174 - kLFORateDisplayWidth,//155,
	kDivisorLFORateDisplayY = 188,
	kDivisorLFODepthDisplayX = 171 - kDisplayWidth + 2,
	kDivisorLFODepthDisplayY = 212,
	kBufferLFORateDisplayX = 328 - kLFORateDisplayWidth,
	kBufferLFORateDisplayY = 188,
	kBufferLFODepthDisplayX = 331 - kDisplayWidth,
	kBufferLFODepthDisplayY = 212,
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

	kDivisorLFOShapeSwitchX = 8,
	kDivisorLFOShapeSwitchY = 145,
	kDivisorLFOTempoSyncButtonX = 52,
	kDivisorLFOTempoSyncButtonY = 119,
	kDivisorLFORateLabelX = 178,
	kDivisorLFORateLabelY = 188,

	kBufferLFOShapeSwitchX = 352,
	kBufferLFOShapeSwitchY = 145,
	kBufferLFOTempoSyncButtonX = 351,
	kBufferLFOTempoSyncButtonY = 120,
	kBufferLFORateLabelX = 277,
	kBufferLFORateLabelY = 188,

	kHostTempoButtonX = 386,
	kHostTempoButtonY = 6,

	kMidiLearnButtonX = 5,
	kMidiLearnButtonY = 63,
	kMidiResetButtonX = 4,
	kMidiResetButtonY = 86
};



//-----------------------------------------------------------------------------
// value text display procedures

bool divisorDisplayProc(float value, char* outText, void*)
{
	if (value < 2.0f)
	{
		return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.2f", 1.0f) > 0;
	}
	else
	{
		return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.2f", value) > 0;
	}
}

bool bufferSizeDisplayProc(float value, char* outText, void* editor)
{
	auto const dfxEditor = static_cast<DfxGuiEditor*>(editor);
	if (dfxEditor->getparameter_b(kBufferTempoSync))
	{
		return dfxEditor->getparametervaluestring(kBufferSize_Sync, outText);
	}
	else
	{
		return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.1f", value) > 0;
	}
}

bool divisorLFORateDisplayProc(float value, char* outText, void* editor)
{
	auto const dfxEditor = static_cast<DfxGuiEditor*>(editor);
	if (dfxEditor->getparameter_b(kDivisorLFOTempoSync))
	{
		return dfxEditor->getparametervaluestring(kDivisorLFORate_Sync, outText);
	}
	else
	{
		if (value < 10.0f)
		{
			return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.2f", value) > 0;
		}
		else
		{
			return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.1f", value) > 0;
		}
	}
}

bool bufferLFORateDisplayProc(float value, char* outText, void* editor)
{
	auto const dfxEditor = static_cast<DfxGuiEditor*>(editor);
	if (dfxEditor->getparameter_b(kBufferLFOTempoSync))
	{
		return dfxEditor->getparametervaluestring(kBufferLFORate_Sync, outText);
	}
	else
	{
		if (value < 10.0f)
		{
			return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.2f", value) > 0;
		}
		else
		{
			return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.1f", value) > 0;
		}
	}
}

bool lfoDepthDisplayProc(float value, char* outText, void*)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.0f%%", value) > 0;
}

bool smoothDisplayProc(float value, char* outText, void*)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.1f%%", value) > 0;
}

bool dryWetMixDisplayProc(float value, char* outText, void*)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.0f%%", value) > 0;
}

bool pitchbendDisplayProc(float value, char* outText, void*)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "\xC2\xB1 %.2f", value) > 0;
}

bool tempoDisplayProc(float value, char* outText, void*)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.2f", value) > 0;
}



//-----------------------------------------------------------------------------
DFX_EDITOR_ENTRY(BufferOverrideEditor)

//-----------------------------------------------------------------------------
BufferOverrideEditor::BufferOverrideEditor(DGEditorListenerInstance inInstance)
:	DfxGuiEditor(inInstance)
{
}

//-----------------------------------------------------------------------------
long BufferOverrideEditor::OpenEditor()
{
	// create images

	// slider handles
	auto const sliderHandleImage = makeOwned<DGImage>("slider-handle.png");
	auto const xyBoxHandleImage = makeOwned<DGImage>("xy-box-handle.png");

	// buttons
	auto const bufferTempoSyncButtonImage = makeOwned<DGImage>("buffer-tempo-sync-button.png");
	auto const bufferTempoSyncButtonCornerImage = makeOwned<DGImage>("buffer-tempo-sync-button-corner.png");
	auto const bufferSizeLabelImage = makeOwned<DGImage>("buffer-size-label.png");

	auto const bufferInterruptButtonImage = makeOwned<DGImage>("buffer-interrupt-button.png");
	auto const bufferInterruptButtonCornerImage = makeOwned<DGImage>("buffer-interrupt-button-corner.png");

	auto const divisorLFOTempoSyncButtonImage = makeOwned<DGImage>("divisor-lfo-tempo-sync-button.png");
	auto const divisorLFORateLabelImage = makeOwned<DGImage>("divisor-lfo-rate-label.png");

	auto const bufferLFOTempoSyncButtonImage = makeOwned<DGImage>("buffer-lfo-tempo-sync-button.png");
	auto const bufferLFORateLabelImage = makeOwned<DGImage>("buffer-lfo-rate-label.png");

	auto const divisorLFOShapeSwitchImage = makeOwned<DGImage>("divisor-lfo-shape-switch.png");

	auto const bufferLFOShapeSwitchImage = makeOwned<DGImage>("buffer-lfo-shape-switch.png");

	auto const midiModeButtonImage = makeOwned<DGImage>("midi-mode-button.png");
	auto const midiLearnButtonImage = makeOwned<DGImage>("midi-learn-button.png");
	auto const midiResetButtonImage = makeOwned<DGImage>("midi-reset-button.png");

	auto const hostTempoButtonImage = makeOwned<DGImage>("host-tempo-button.png");


	// create controls
	
	DGRect pos;

	auto const divisorLFORateTag = getparameter_b(kDivisorLFOTempoSync) ? kDivisorLFORate_Sync : kDivisorLFORate_Hz;
	pos.set(kDivisorLFORateSliderX, kDivisorLFORateSliderY, kSliderWidth, kLFOSliderHeight);
	mDivisorLFORateSlider = emplaceControl<DGSlider>(this, divisorLFORateTag, pos, dfx::kAxis_Horizontal, sliderHandleImage);

	pos.set(kDivisorLFODepthSliderX, kDivisorLFODepthSliderY, kSliderWidth, kLFOSliderHeight);
	emplaceControl<DGSlider>(this, kDivisorLFODepth, pos, dfx::kAxis_Horizontal, sliderHandleImage);

	auto const bufferLFORateTag = getparameter_b(kBufferLFOTempoSync) ? kBufferLFORate_Sync : kBufferLFORate_Hz;
	pos.set(kBufferLFORateSliderX, kBufferLFORateSliderY, kSliderWidth, kLFOSliderHeight);
	mBufferLFORateSlider = emplaceControl<DGSlider>(this, bufferLFORateTag, pos, dfx::kAxis_Horizontal, sliderHandleImage);

	pos.set(kBufferLFODepthSliderX, kBufferLFODepthSliderY, kSliderWidth, kLFOSliderHeight);
	emplaceControl<DGSlider>(this, kBufferLFODepth, pos, dfx::kAxis_Horizontal, sliderHandleImage);

	pos.set(kSmoothSliderX, kSmoothSliderY, kSliderWidth, kSliderHeight);
	emplaceControl<DGSlider>(this, kSmooth, pos, dfx::kAxis_Horizontal, sliderHandleImage);

	pos.set(kDryWetMixSliderX, kDryWetMixSliderY, kSliderWidth, kSliderHeight);
	emplaceControl<DGSlider>(this, kDryWetMix, pos, dfx::kAxis_Horizontal, sliderHandleImage);

	pos.set(kPitchbendSliderX, kPitchbendSliderY, kSliderWidth, kSliderHeight);
	emplaceControl<DGSlider>(this, kPitchbend, pos, dfx::kAxis_Horizontal, sliderHandleImage);

	pos.set(kTempoSliderX, kTempoSliderY, kTempoSliderWidth, kSliderHeight);
	emplaceControl<DGSlider>(this, kTempo, pos, dfx::kAxis_Horizontal, sliderHandleImage);

	pos.set(kDivisorBufferBoxX + 3, kDivisorBufferBoxY + 27, kDivisorBufferBoxWidth - 6, kSliderHeight);
	emplaceControl<DGSlider>(this, kDivisor, pos, dfx::kAxis_Horizontal, xyBoxHandleImage);

	auto const bufferSizeTag = getparameter_b(kBufferTempoSync) ? kBufferSize_Sync : kBufferSize_MS;
	pos.offset(0, 57);
	mBufferSizeSlider = emplaceControl<DGSlider>(this, bufferSizeTag, pos, dfx::kAxis_Horizontal, xyBoxHandleImage);



	pos.set(kDivisorDisplayX, kDivisorDisplayY, kDisplayWidth, kDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kDivisor, pos, divisorDisplayProc, nullptr, nullptr, dfx::TextAlignment::Right, kValueDisplayRegularFontSize, DGColor::kWhite, kValueDisplayFont);

	pos.set(kBufferDisplayX, kBufferDisplayY, kDisplayWidth, kDisplayHeight);
	mBufferSizeDisplay = emplaceControl<DGTextDisplay>(this, bufferSizeTag, pos, bufferSizeDisplayProc, this, nullptr, dfx::TextAlignment::Center, kValueDisplayRegularFontSize, DGColor::kWhite, kValueDisplayFont);

	pos.set(kDivisorLFORateDisplayX, kDivisorLFORateDisplayY, kLFORateDisplayWidth, kDisplayHeight);
	mDivisorLFORateDisplay = emplaceControl<DGTextDisplay>(this, divisorLFORateTag, pos, divisorLFORateDisplayProc, this, nullptr, dfx::TextAlignment::Right, kValueDisplayTinyFontSize, DGColor::kWhite, kValueDisplayFont);

	pos.set(kDivisorLFODepthDisplayX, kDivisorLFODepthDisplayY, kDisplayWidth, kDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kDivisorLFODepth, pos, lfoDepthDisplayProc, nullptr, nullptr, dfx::TextAlignment::Right, kValueDisplayTinyFontSize, DGColor::kWhite, kValueDisplayFont);

	pos.set(kBufferLFORateDisplayX, kBufferLFORateDisplayY, kLFORateDisplayWidth, kDisplayHeight);
	mBufferLFORateDisplay = emplaceControl<DGTextDisplay>(this, bufferLFORateTag, pos, bufferLFORateDisplayProc, this, nullptr, dfx::TextAlignment::Right, kValueDisplayTinyFontSize, DGColor::kWhite, kValueDisplayFont);

	pos.set(kBufferLFODepthDisplayX, kBufferLFODepthDisplayY, kDisplayWidth, kDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kBufferLFODepth, pos, lfoDepthDisplayProc, nullptr, nullptr, dfx::TextAlignment::Right, kValueDisplayTinyFontSize, DGColor::kWhite, kValueDisplayFont);

	pos.set(kSmoothDisplayX, kSmoothDisplayY, kDisplayWidth, kDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kSmooth, pos, smoothDisplayProc, nullptr, nullptr, dfx::TextAlignment::Right, kValueDisplayRegularFontSize, DGColor::kWhite, kValueDisplayFont);

	pos.set(kDryWetMixDisplayX, kDryWetMixDisplayY, kDisplayWidth, kDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kDryWetMix, pos, dryWetMixDisplayProc, nullptr, nullptr, dfx::TextAlignment::Right, kValueDisplayRegularFontSize, DGColor::kWhite, kValueDisplayFont);

	pos.set(kPitchbendDisplayX, kPitchbendDisplayY, kDisplayWidth, kDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kPitchbend, pos, pitchbendDisplayProc, nullptr, nullptr, dfx::TextAlignment::Right, kValueDisplayRegularFontSize, DGColor::kWhite, kValueDisplayFont);

	pos.set(kTempoDisplayX, kTempoDisplayY, kDisplayWidth, kDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kTempo, pos, tempoDisplayProc, nullptr, nullptr, dfx::TextAlignment::Left, kValueDisplayTinyFontSize, DGColor::kWhite, kValueDisplayFont);


	// callbacks for button-triggered action
	auto const linkKickButtonsDownProc = [](long, void* otherButton)
	{
		if (auto const otherDGButton = static_cast<DGButton*>(otherButton))
		{
			otherDGButton->setMouseIsDown(true);
		}
	};
	auto const linkKickButtonsUpProc = [](long, void* otherButton)
	{
		if (auto const otherDGButton = static_cast<DGButton*>(otherButton))
		{
			otherDGButton->setMouseIsDown(false);
		}
	};

	// forced buffer size tempo sync button
	pos.set(kBufferTempoSyncButtonX, kBufferTempoSyncButtonY, bufferTempoSyncButtonImage->getWidth() / 2, bufferTempoSyncButtonImage->getHeight() / 2);
	auto const bufferTempoSyncButton = emplaceControl<DGButton>(this, kBufferTempoSync, pos, bufferTempoSyncButtonImage, 2, DGButton::Mode::Increment, true);
	//
	pos.set(kBufferTempoSyncButtonCornerX, kBufferTempoSyncButtonCornerY, bufferTempoSyncButtonCornerImage->getWidth() / 2, bufferTempoSyncButtonCornerImage->getHeight() / 2);
	auto const bufferTempoSyncButtonCorner = emplaceControl<DGButton>(this, kBufferTempoSync, pos, bufferTempoSyncButtonCornerImage, 2, DGButton::Mode::Increment, true);
	//
	bufferTempoSyncButton->setUserProcedure(linkKickButtonsDownProc, bufferTempoSyncButtonCorner);
	bufferTempoSyncButtonCorner->setUserProcedure(linkKickButtonsDownProc, bufferTempoSyncButton);
	bufferTempoSyncButton->setUserReleaseProcedure(linkKickButtonsUpProc, bufferTempoSyncButtonCorner);
	bufferTempoSyncButtonCorner->setUserReleaseProcedure(linkKickButtonsUpProc, bufferTempoSyncButton);

	// buffer interrupt button
	pos.set(kBufferInterruptButtonX, kBufferInterruptButtonY, bufferInterruptButtonImage->getWidth() / 2, bufferInterruptButtonImage->getHeight() / 2);
	auto const bufferInterruptButton = emplaceControl<DGButton>(this, kBufferInterrupt, pos, bufferInterruptButtonImage, 2, DGButton::Mode::Increment, true);
	//
	pos.set(kBufferInterruptButtonCornerX, kBufferInterruptButtonCornerY, bufferInterruptButtonCornerImage->getWidth() / 2, bufferInterruptButtonCornerImage->getHeight() / 2);
	auto const bufferInterruptButtonCorner = emplaceControl<DGButton>(this, kBufferInterrupt, pos, bufferInterruptButtonCornerImage, 2, DGButton::Mode::Increment, true);
	//
	bufferInterruptButtonCorner->setUserProcedure(linkKickButtonsDownProc, bufferInterruptButton);
	bufferInterruptButton->setUserProcedure(linkKickButtonsDownProc, bufferInterruptButtonCorner);
	bufferInterruptButtonCorner->setUserReleaseProcedure(linkKickButtonsUpProc, bufferInterruptButton);
	bufferInterruptButton->setUserReleaseProcedure(linkKickButtonsUpProc, bufferInterruptButtonCorner);

	// forced buffer size LFO tempo sync button
	pos.set(kBufferLFOTempoSyncButtonX, kBufferLFOTempoSyncButtonY, bufferLFOTempoSyncButtonImage->getWidth() / 2, bufferLFOTempoSyncButtonImage->getHeight() / 2);
	emplaceControl<DGButton>(this, kBufferLFOTempoSync, pos, bufferLFOTempoSyncButtonImage, 2, DGButton::Mode::Increment, true);

	// divisor LFO tempo sync button
	pos.set(kDivisorLFOTempoSyncButtonX, kDivisorLFOTempoSyncButtonY, divisorLFOTempoSyncButtonImage->getWidth() / 2, divisorLFOTempoSyncButtonImage->getHeight() / 2);
	emplaceControl<DGButton>(this, kDivisorLFOTempoSync, pos, divisorLFOTempoSyncButtonImage, 2, DGButton::Mode::Increment, true);

	// MIDI mode button
	pos.set(kMidiModeButtonX, kMidiModeButtonY, midiModeButtonImage->getWidth() / 2, midiModeButtonImage->getHeight() / BufferOverride::kNumMidiModes);
	emplaceControl<DGButton>(this, kMidiMode, pos, midiModeButtonImage, BufferOverride::kNumMidiModes, DGButton::Mode::Increment, true);

	// sync to host tempo button
	pos.set(kHostTempoButtonX, kHostTempoButtonY, hostTempoButtonImage->getWidth(), hostTempoButtonImage->getHeight() / 2);
	emplaceControl<DGButton>(this, kTempoAuto, pos, hostTempoButtonImage, 2, DGButton::Mode::Increment);

	// MIDI learn button
	CreateMidiLearnButton(kMidiLearnButtonX, kMidiLearnButtonY, midiLearnButtonImage);

	// MIDI reset button
	CreateMidiResetButton(kMidiResetButtonX, kMidiResetButtonY, midiResetButtonImage);


	// forced buffer size LFO shape switch
	pos.set(kBufferLFOShapeSwitchX, kBufferLFOShapeSwitchY, bufferLFOShapeSwitchImage->getWidth(), bufferLFOShapeSwitchImage->getHeight() / dfx::LFO::kNumShapes);
	emplaceControl<DGButton>(this, kBufferLFOShape, pos, bufferLFOShapeSwitchImage, dfx::LFO::kNumShapes, DGButton::Mode::Radio);

	// divisor LFO shape switch
	pos.set(kDivisorLFOShapeSwitchX, kDivisorLFOShapeSwitchY, divisorLFOShapeSwitchImage->getWidth(), divisorLFOShapeSwitchImage->getHeight() / dfx::LFO::kNumShapes);
	emplaceControl<DGButton>(this, kDivisorLFOShape, pos, divisorLFOShapeSwitchImage, dfx::LFO::kNumShapes, DGButton::Mode::Radio);


	// forced buffer size label
	pos.set(kBufferSizeLabelX, kBufferSizeLabelY, bufferSizeLabelImage->getWidth(), bufferSizeLabelImage->getHeight() / 2);
	emplaceControl<DGButton>(this, kBufferTempoSync, pos, bufferSizeLabelImage, 2, DGButton::Mode::PictureReel);

	// forced buffer size LFO rate label
	pos.set(kBufferLFORateLabelX, kBufferLFORateLabelY, bufferLFORateLabelImage->getWidth(), bufferLFORateLabelImage->getHeight() / 2);
	emplaceControl<DGButton>(this, kBufferLFOTempoSync, pos, bufferLFORateLabelImage, 2, DGButton::Mode::PictureReel);

	// divisor LFO rate label
	pos.set(kDivisorLFORateLabelX, kDivisorLFORateLabelY, divisorLFORateLabelImage->getHeight(), divisorLFORateLabelImage->getHeight() / 2);
	emplaceControl<DGButton>(this, kDivisorLFOTempoSync, pos, divisorLFORateLabelImage, 2, DGButton::Mode::PictureReel);


	// the help mouseover hint thingy
	pos.set(kHelpDisplayX, kHelpDisplayY, GetBackgroundImage()->getWidth(), kDisplayHeight);
	mHelpDisplay = emplaceControl<DGStaticTextDisplay>(this, pos, nullptr, dfx::TextAlignment::Center, kHelpDisplayFontSize, kHelpDisplayTextColor, kHelpDisplayFont);


	return dfx::kStatus_NoError;
}


//-----------------------------------------------------------------------------
void BufferOverrideEditor::parameterChanged(long inParameterID)
{
	DGSlider* slider = nullptr;
	DGTextDisplay* textDisplay = nullptr;
	auto const useSyncParam = getparameter_b(inParameterID);

	auto newParameterID = dfx::kParameterID_Invalid;
	switch (inParameterID)
	{
		case kBufferTempoSync:
			newParameterID = useSyncParam ? kBufferSize_Sync : kBufferSize_MS;
			slider = mBufferSizeSlider;
			textDisplay = mBufferSizeDisplay;
			break;
		case kDivisorLFOTempoSync:
			newParameterID = useSyncParam ? kDivisorLFORate_Sync : kDivisorLFORate_Hz;
			slider = mDivisorLFORateSlider;
			textDisplay = mDivisorLFORateDisplay;
			break;
		case kBufferLFOTempoSync:
			newParameterID = useSyncParam ? kBufferLFORate_Sync : kBufferLFORate_Hz;
			slider = mBufferLFORateSlider;
			textDisplay = mBufferLFORateDisplay;
			break;
		default:
			return;
	}

	if (slider)
	{
		slider->setParameterID(newParameterID);
	}
	if (textDisplay)
	{
		textDisplay->setParameterID(newParameterID);
	}
}


//-----------------------------------------------------------------------------
void BufferOverrideEditor::mouseovercontrolchanged(DGControl* currentControlUnderMouse)
{
	if (!mHelpDisplay)
	{
		return;
	}

	auto currentcontrolparam = dfx::kParameterID_Invalid;
	if (currentControlUnderMouse && currentControlUnderMouse->isParameterAttached())
	{
		currentcontrolparam = currentControlUnderMouse->getParameterID();
	}

	char const* helpstring = nullptr;
	switch (currentcontrolparam)
	{
		case kDivisor:
			helpstring = "buffer divisor is the number of times each forced buffer skips & starts over";
			break;
		case kBufferSize_MS:
		case kBufferSize_Sync:
			helpstring = "forced buffer size is the length of the sound chunks that Buffer Override works with";
			break;
//		case kBufferDivisorHelpTag:
#if TARGET_OS_MAC
//			helpstring = "left/right is buffer divisor (the number of skips in a forced buffer, hold ctrl).   up/down is forced buffer size (hold option)";
#else
			// shorten display text for Windows (beware larger fonts)
//			helpstring = "left/right is buffer divisor (number of skips in a buffer, hold ctrl).  up/down is forced buffer size (hold alt)";
#endif
//			break;
		case kBufferTempoSync:
			helpstring = "turn tempo sync on if you want the size of the forced buffers to sync to your tempo";
			break;
		case kBufferInterrupt:
			helpstring = "turn this off for the old version 1 style of stubborn \"stuck\" buffers (if you really want that)";
			break;
		case kDivisorLFORate_Hz:
		case kDivisorLFORate_Sync:
			helpstring = "this is the speed of the LFO that modulates the buffer divisor";
			break;
		case kDivisorLFODepth:
			helpstring = "the depth (or intensity) of the LFO that modulates the buffer divisor (0% does nothing)";
			break;
		case kDivisorLFOShape:
			helpstring = "choose the waveform shape of the LFO that modulates the buffer divisor";
			break;
		case kDivisorLFOTempoSync:
			helpstring = "turn tempo sync on if you want the rate of the buffer divisor LFO to sync to your tempo";
			break;
		case kBufferLFORate_Hz:
		case kBufferLFORate_Sync:
			helpstring = "this is the speed of the LFO that modulates the forced buffer size";
			break;
		case kBufferLFODepth:
			helpstring = "the depth (or intensity) of the LFO that modulates the forced buffer size (0% does nothing)";
			break;
		case kBufferLFOShape:
			helpstring = "choose the waveform shape of the LFO that modulates the forced buffer size";
			break;
		case kBufferLFOTempoSync:
			helpstring = "turn tempo sync on if you want the rate of the forced buffer size LFO to sync to your tempo";
			break;
		case kSmooth:
			helpstring = "the portion of each minibuffer spent smoothly crossfading the previous one into the new one (prevents glitches)";
			break;
		case kDryWetMix:
			helpstring = "the relative mix of the processed sound & the clean/original sound (100% is all processed)";
			break;
		case kPitchbend:
			helpstring = "the range, in semitones, of the MIDI pitch bend wheel's effect on the buffer divisor";
			break;
		case kMidiMode:
			helpstring = "nudge: MIDI notes adjust the buffer divisor.   trigger: notes also reset the divisor to 1 when they are released";
			break;
		case kTempo:
			helpstring = "you can adjust the tempo that Buffer Override uses, or set this to \"auto\" to get the tempo from your sequencer";
			break;
		case kTempoAuto:
			helpstring = "enable this to get the tempo from your sequencer";
			break;

		default:
			if (currentControlUnderMouse)
			{
				if (currentControlUnderMouse == GetMidiLearnButton())
				{
					helpstring = "activate or deactivate learn mode for assigning MIDI CCs to control Buffer Override's parameters";
				}
				else if (currentControlUnderMouse == GetMidiResetButton())
				{
					helpstring = "press this button to erase all of your CC assignments";
				}
			}
			break;
	}

	mHelpDisplay->setText(helpstring ? helpstring : "");
}
