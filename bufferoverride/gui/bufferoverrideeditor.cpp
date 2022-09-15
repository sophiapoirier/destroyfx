/*------------------------------------------------------------------------
Copyright (C) 2001-2022  Sophia Poirier

This file is part of Buffer Override.

Buffer Override is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
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

#include <algorithm>
#include <array>
#include <cassert>
#include <functional>

#include "bufferoverride-base.h"
#include "bufferoverrideview.h"
#include "lfo.h"


constexpr char const* const kValueDisplayFont = dfx::kFontName_Wetar16px;
constexpr float kValueDisplayFontSize = dfx::kFontSize_Wetar16px;

constexpr char const* const kHelpDisplayFont = dfx::kFontName_Wetar16px;
constexpr float kHelpDisplayFontSize = dfx::kFontSize_Wetar16px;

constexpr DGColor kHelpDisplayTextColor(201, 201, 201);

// TODO: This color is too dark when in text edit mode on Windows
// (black background); should add the ability to DGTextDisplay to
// use an alternate color (#81ba28) on Windows when editing.
static constexpr DGColor kLCDGreenTextColor(0x0B, 0x2A, 0x00);
static constexpr DGColor kLCDCyanTextColor(0xB5, 0xDF, 0xE4);

constexpr float kUnusedControlAlpha = 0.39f;

static constexpr int kVTextOffset = -3;

//-----------------------------------------------------------------------------
enum
{
	// Sliders are all the same height.
	kSliderHeight = 18,
	// ... but different widths
	kLFOSliderWidth = 227,
	kSmoothSliderWidth = 237,
	kDryWetSliderWidth = 237,
	kPitchBendSliderWidth = 227,
	kTempoSliderWidth = 374,
	//
	kDivisorLFORateSliderX = 12,
	kDivisorLFORateSliderY = 364 - 9,
	kDivisorLFODepthSliderX = kDivisorLFORateSliderX,
	kDivisorLFODepthSliderY = 409 - 9,
	kBufferLFORateSliderX = 721,
	kBufferLFORateSliderY = kDivisorLFORateSliderY,
	kBufferLFODepthSliderX = kBufferLFORateSliderX,
	kBufferLFODepthSliderY = kDivisorLFODepthSliderY,
	//
	kPitchBendRangeSliderX = 326,
	kPitchBendRangeSliderY = 425 - 9,
	kSmoothSliderX = 708,
	kSmoothSliderY = 53 - 9,
	kDryWetMixSliderX = kSmoothSliderX,
	kDryWetMixSliderY = 103 - 9,
	//
	kTempoSliderX = 287,
	kTempoSliderY = 14 - 9,

	// "lcd displays" are the green displays: tempo,
	// smoothing, dry/wet, LFO rate and depth
	kLCDDisplayWidth = 99,
	kLCDDisplayHeight = dfx::kFontContainerHeight_Wetar16px,
	kPitchbendDisplayWidth = 89,
	// "oled displays" are the breakout boards for
	// divisor and buffer
	kOLEDDisplayWidth = 91,
	kOLEDDisplayHeight = 2 * dfx::kFontContainerHeight_Wetar16px,
	kDivisorDisplayX = 249 + 2,
	kDivisorDisplayY = 343 - (16 + 2),
	kBufferDisplayX = 684 + 2,
	kBufferDisplayY = 229 - (16 + 2),
	//
	kDivisorLFORateDisplayX = 141,
	kDivisorLFORateDisplayY = 376 + kVTextOffset,
	kDivisorLFODepthDisplayX = kDivisorLFORateDisplayX,
	kDivisorLFODepthDisplayY = 421 + kVTextOffset,
	kBufferLFORateDisplayX = 850,
	kBufferLFORateDisplayY = kDivisorLFORateDisplayY,
	kBufferLFODepthDisplayX = kBufferLFORateDisplayX,
	kBufferLFODepthDisplayY = kDivisorLFODepthDisplayY,
	//
	kPitchBendRangeDisplayX = 482,
	kPitchBendRangeDisplayY = 437 + kVTextOffset,
	kSmoothDisplayX = 847,
	kSmoothDisplayY = 65 + kVTextOffset,
	kDryWetMixDisplayX = kSmoothDisplayX,
	kDryWetMixDisplayY = 115 + kVTextOffset,
	kTempoDisplayX = 558,
	kTempoDisplayY = 26 + kVTextOffset,

	// XY box
	kDivisorBufferBoxX = 302,
	kDivisorBufferBoxY = 59,
	kDivisorBufferBoxWidth = 354,
	kDivisorBufferBoxHeight = 249,

	// buttons
	kBufferInterruptButtonX = 71,
	kBufferInterruptButtonY = 169,

	kBufferTempoSyncButtonX = 736,
	kBufferTempoSyncButtonY = 143,
	kBufferSizeLabelX = 684,
	kBufferSizeLabelY = 255,

	kMidiModeButtonX = 346,
	kMidiModeButtonY = 358,

	kDivisorLFOShapeSwitchX = 8,
	kDivisorLFOShapeSwitchY = 272,
	kDivisorLFOTempoSyncButtonX = 27,
	kDivisorLFOTempoSyncButtonY = 311,
	kDivisorLFORateLabelX = 178,
	kDivisorLFORateLabelY = 188,

	kBufferLFOShapeSwitchX = 713,
	kBufferLFOShapeSwitchY = kDivisorLFOShapeSwitchY,
	kBufferLFOTempoSyncButtonX = 740,
	kBufferLFOTempoSyncButtonY = kDivisorLFOTempoSyncButtonY,
	kBufferLFORateLabelX = 277,
	kBufferLFORateLabelY = kDivisorLFORateLabelY,

	kHostTempoButtonX = 670,
	kHostTempoButtonY = 7,

	kMidiLearnButtonX = 466,
	kMidiLearnButtonY = 395,
	kMidiResetButtonX = 523,
	kMidiResetButtonY = kMidiLearnButtonY,

	// help display
	kHelpDisplayX = 0,
	kHelpDisplayY = 463,
	kHelpDisplayHeight = dfx::kFontContainerHeight_Wetar16px,
	kHelpDisplayLineSpacing = 19,

	kDestroyFXLinkX = 203,
	kDestroyFXLinkY = 8,

	kTitleAreaX = 18,
	kTitleAreaY = 14,
	kTitleAreaWidth = 172,
	kTitleAreaHeight = 16,

	kVisualizationX = 17,
	kVisualizationY = 59,
	kVisualizationW = BufferOverrideView::WIDTH,
	kVisualizationH = BufferOverrideView::HEIGHT
};



//-----------------------------------------------------------------------------
// value text display procedures

static bool divisorDisplayProc(float inValue, char* outText, void*)
{
	int const precision = (inValue <= 99.99f) ? 2 : 1;
	float const effectiveValue = (inValue < 2.0f) ? 1.0f : inValue;
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.*f", precision, effectiveValue) > 0;
}

static bool bufferSizeDisplayProc(float inValue, char* outText, void* inEditor)
{
	auto const dgEditor = static_cast<DfxGuiEditor*>(inEditor);
	if (dgEditor->getparameter_b(kBufferTempoSync))
	{
		if (auto const valueString = dgEditor->getparametervaluestring(kBufferSize_Sync))
		{
			return dfx::StrLCpy(outText, *valueString, DGTextDisplay::kTextMaxLength) > 0;
		}
		return false;
	}
	else
	{
		return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.1f", inValue) > 0;
	}
}

// Generate the display text for the divisor or buffer LFO rate.
// This depends on the slider value and whether we're synced to tempo.
static bool lfoRateGenDisplayProc(float inValue, char* outText, void* inEditor, long rateSyncParameterID, long tempoSyncParameterID)
{
	auto const dgEditor = static_cast<DfxGuiEditor*>(inEditor);
	if (dgEditor->getparameter_b(tempoSyncParameterID))
	{
		if (std::optional<std::string> const valueString = dgEditor->getparametervaluestring(rateSyncParameterID))
		{
			return snprintf(outText, DGTextDisplay::kTextMaxLength, "%s / beat", valueString->c_str()) > 0;
		}
		return false;
	}

	int const precision = (inValue <= 9.99f) ? 2 : 1;
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.*f Hz", precision, inValue) > 0;
}

static bool divisorLFORateDisplayProc(float inValue, char* outText, void* inEditor)
{
	return lfoRateGenDisplayProc(inValue, outText, inEditor, kDivisorLFORate_Sync, kDivisorLFOTempoSync);
}

static bool bufferLFORateDisplayProc(float inValue, char* outText, void* inEditor)
{
	return lfoRateGenDisplayProc(inValue, outText, inEditor, kBufferLFORate_Sync, kBufferLFOTempoSync);
}

static bool lfoDepthDisplayProc(float inValue, char* outText, void*)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.0f%%", inValue) > 0;
}

static bool dryWetMixDisplayProc(float inValue, char* outText, void*)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.0f%%", inValue) > 0;
}

static bool pitchBendRangeDisplayProc(float inValue, char* outText, void*)
{
	int const precision = (inValue <= 9.99f) ? 2 : 1;
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%s %.*f", dfx::kPlusMinusUTF8, precision, inValue) > 0;
}

static bool tempoDisplayProc(float inValue, char* outText, void*)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.2f", inValue) > 0;
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
	auto const sliderHandleImage = LoadImage("slider-handle.png");
	auto const sliderHandleImage_glowing = LoadImage("slider-handle-glowing.png");
	auto const xyBoxHandleImage = LoadImage("xy-box-handle.png");
	auto const xyBoxHandleImage_divisor_glowing = LoadImage("xy-box-handle-divisor-glow.png");
	auto const xyBoxHandleImage_buffer_glowing = LoadImage("xy-box-handle-buffer-glow.png");

	// buttons
	auto const bufferTempoSyncButtonImage = LoadImage("buffer-tempo-sync-button.png");
	auto const bufferSizeLabelImage = LoadImage("buffer-size-label.png");

	auto const bufferInterruptButtonImage = LoadImage("buffer-interrupt-button.png");

	auto const lfoShapeSwitchImage = LoadImage("lfo-shape-switch.png");
	auto const lfoTempoSyncImage = LoadImage("lfo-tempo-sync-button.png");

	auto const hostTempoButtonImage = LoadImage("host-tempo-button.png");

	auto const midiModeButtonImage = LoadImage("midi-mode-button.png");
	auto const midiLearnButtonImage = LoadImage("midi-learn-button.png");
	auto const midiResetButtonImage = LoadImage("midi-reset-button.png");

	auto const destroyFXLinkImage = LoadImage("destroy-fx-link.png");


	// create controls

	DGRect pos;

	auto const divisorLFORateTag = getparameter_b(kDivisorLFOTempoSync) ? kDivisorLFORate_Sync : kDivisorLFORate_Hz;
	pos.set(kDivisorLFORateSliderX, kDivisorLFORateSliderY, kLFOSliderWidth, kSliderHeight);
	mDivisorLFORateSlider = emplaceControl<DGSlider>(this, divisorLFORateTag, pos, dfx::kAxis_Horizontal, sliderHandleImage);
	mDivisorLFORateSlider->setAlternateHandle(sliderHandleImage_glowing);

	pos.set(kDivisorLFODepthSliderX, kDivisorLFODepthSliderY, kLFOSliderWidth, kSliderHeight);
	emplaceControl<DGSlider>(this, kDivisorLFODepth, pos, dfx::kAxis_Horizontal, sliderHandleImage)->setAlternateHandle(sliderHandleImage_glowing);

	auto const bufferLFORateTag = getparameter_b(kBufferLFOTempoSync) ? kBufferLFORate_Sync : kBufferLFORate_Hz;
	pos.set(kBufferLFORateSliderX, kBufferLFORateSliderY, kLFOSliderWidth, kSliderHeight);
	mBufferLFORateSlider = emplaceControl<DGSlider>(this, bufferLFORateTag, pos, dfx::kAxis_Horizontal, sliderHandleImage);
	mBufferLFORateSlider->setAlternateHandle(sliderHandleImage_glowing);

	pos.set(kBufferLFODepthSliderX, kBufferLFODepthSliderY, kLFOSliderWidth, kSliderHeight);
	emplaceControl<DGSlider>(this, kBufferLFODepth, pos, dfx::kAxis_Horizontal, sliderHandleImage)->setAlternateHandle(sliderHandleImage_glowing);

	pos.set(kSmoothSliderX, kSmoothSliderY, kSmoothSliderWidth, kSliderHeight);
	emplaceControl<DGSlider>(this, kSmooth, pos, dfx::kAxis_Horizontal, sliderHandleImage)->setAlternateHandle(sliderHandleImage_glowing);

	pos.set(kDryWetMixSliderX, kDryWetMixSliderY, kDryWetSliderWidth, kSliderHeight);
	emplaceControl<DGSlider>(this, kDryWetMix, pos, dfx::kAxis_Horizontal, sliderHandleImage)->setAlternateHandle(sliderHandleImage_glowing);

	pos.set(kPitchBendRangeSliderX, kPitchBendRangeSliderY, kPitchBendSliderWidth, kSliderHeight);
	emplaceControl<DGSlider>(this, kPitchBendRange, pos, dfx::kAxis_Horizontal, sliderHandleImage)->setAlternateHandle(sliderHandleImage_glowing);

	pos.set(kTempoSliderX, kTempoSliderY, kTempoSliderWidth, kSliderHeight);
	emplaceControl<DGSlider>(this, kTempo, pos, dfx::kAxis_Horizontal, sliderHandleImage)->setAlternateHandle(sliderHandleImage_glowing);

	auto const bufferSizeTag = getparameter_b(kBufferTempoSync) ? kBufferSize_Sync : kBufferSize_MS;
	pos.set(kDivisorBufferBoxX, kDivisorBufferBoxY, kDivisorBufferBoxWidth, kDivisorBufferBoxHeight);
	mDivisorBufferBox = emplaceControl<DGXYBox>(this, kDivisor, bufferSizeTag, pos, xyBoxHandleImage, nullptr,
												VSTGUI::CSliderBase::kLeft | VSTGUI::CSliderBase::kTop);
	mDivisorBufferBox->setAlternateHandles(xyBoxHandleImage_divisor_glowing, xyBoxHandleImage_buffer_glowing);
	mDivisorBufferBox->setIntegralPosition(true);


	pos.set(kDivisorDisplayX, kDivisorDisplayY, kOLEDDisplayWidth, kOLEDDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kDivisor, pos, divisorDisplayProc, nullptr, nullptr, dfx::TextAlignment::Center, kValueDisplayFontSize * 2.0f, kLCDCyanTextColor, kValueDisplayFont);

	pos.set(kBufferDisplayX, kBufferDisplayY, kOLEDDisplayWidth, kOLEDDisplayHeight);
	mBufferSizeDisplay = emplaceControl<DGTextDisplay>(this, bufferSizeTag, pos, bufferSizeDisplayProc, this, nullptr, dfx::TextAlignment::Center, kValueDisplayFontSize * 2.0f, kLCDCyanTextColor, kValueDisplayFont);

	pos.set(kDivisorLFORateDisplayX, kDivisorLFORateDisplayY, kLCDDisplayWidth, kLCDDisplayHeight);
	mDivisorLFORateDisplay = emplaceControl<DGTextDisplay>(this, divisorLFORateTag, pos, divisorLFORateDisplayProc, this, nullptr, dfx::TextAlignment::Right, kValueDisplayFontSize, kLCDGreenTextColor, kValueDisplayFont);

	pos.set(kDivisorLFODepthDisplayX, kDivisorLFODepthDisplayY, kLCDDisplayWidth, kLCDDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kDivisorLFODepth, pos, lfoDepthDisplayProc, nullptr, nullptr, dfx::TextAlignment::Right, kValueDisplayFontSize, kLCDGreenTextColor, kValueDisplayFont);

	pos.set(kBufferLFORateDisplayX, kBufferLFORateDisplayY, kLCDDisplayWidth, kLCDDisplayHeight);
	mBufferLFORateDisplay = emplaceControl<DGTextDisplay>(this, bufferLFORateTag, pos, bufferLFORateDisplayProc, this, nullptr, dfx::TextAlignment::Right, kValueDisplayFontSize, kLCDGreenTextColor, kValueDisplayFont);

	pos.set(kBufferLFODepthDisplayX, kBufferLFODepthDisplayY, kLCDDisplayWidth, kLCDDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kBufferLFODepth, pos, lfoDepthDisplayProc, nullptr, nullptr, dfx::TextAlignment::Right, kValueDisplayFontSize, kLCDGreenTextColor, kValueDisplayFont);

	pos.set(kSmoothDisplayX, kSmoothDisplayY, kLCDDisplayWidth, kLCDDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kSmooth, pos, DGTextDisplay::valueToTextProc_Percent, nullptr, nullptr, dfx::TextAlignment::Right, kValueDisplayFontSize, kLCDGreenTextColor, kValueDisplayFont);

	pos.set(kDryWetMixDisplayX, kDryWetMixDisplayY, kLCDDisplayWidth, kLCDDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kDryWetMix, pos, dryWetMixDisplayProc, nullptr, nullptr, dfx::TextAlignment::Right, kValueDisplayFontSize, kLCDGreenTextColor, kValueDisplayFont);

	pos.set(kPitchBendRangeDisplayX, kPitchBendRangeDisplayY, kPitchbendDisplayWidth, kLCDDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kPitchBendRange, pos, pitchBendRangeDisplayProc, nullptr, nullptr, dfx::TextAlignment::Right, kValueDisplayFontSize, kLCDCyanTextColor, kValueDisplayFont);

	pos.set(kTempoDisplayX, kTempoDisplayY, kLCDDisplayWidth, kLCDDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kTempo, pos, tempoDisplayProc, nullptr, nullptr, dfx::TextAlignment::Right, kValueDisplayFontSize, kLCDGreenTextColor, kValueDisplayFont);


	// forced buffer size tempo sync button
	emplaceControl<DGToggleImageButton>(this, kBufferTempoSync, kBufferTempoSyncButtonX, kBufferTempoSyncButtonY, bufferTempoSyncButtonImage, true);

	// buffer interrupt button
	emplaceControl<DGToggleImageButton>(this, kBufferInterrupt, kBufferInterruptButtonX, kBufferInterruptButtonY, bufferInterruptButtonImage, true);

	// forced buffer size LFO tempo sync button
	emplaceControl<DGToggleImageButton>(this, kBufferLFOTempoSync, kBufferLFOTempoSyncButtonX, kBufferLFOTempoSyncButtonY, lfoTempoSyncImage, true);

	// divisor LFO tempo sync button
	emplaceControl<DGToggleImageButton>(this, kDivisorLFOTempoSync, kDivisorLFOTempoSyncButtonX, kDivisorLFOTempoSyncButtonY, lfoTempoSyncImage, true);

	// MIDI mode button
	pos.set(kMidiModeButtonX, kMidiModeButtonY, midiModeButtonImage->getWidth() / 2, midiModeButtonImage->getHeight() / kNumMidiModes);
	emplaceControl<DGButton>(this, kMidiMode, pos, midiModeButtonImage, DGButton::Mode::Radio, true)->setRadioThresholds({108});

	// sync to host tempo button
	emplaceControl<DGToggleImageButton>(this, kTempoAuto, kHostTempoButtonX, kHostTempoButtonY, hostTempoButtonImage, true);

	// MIDI learn button
	CreateMidiLearnButton(kMidiLearnButtonX, kMidiLearnButtonY, midiLearnButtonImage);

	// MIDI reset button
	CreateMidiResetButton(kMidiResetButtonX, kMidiResetButtonY, midiResetButtonImage);


	// forced buffer size LFO shape switch
	pos.set(kBufferLFOShapeSwitchX, kBufferLFOShapeSwitchY, lfoShapeSwitchImage->getWidth() / 2, lfoShapeSwitchImage->getHeight() / dfx::LFO::kNumShapes);
	emplaceControl<DGButton>(this, kBufferLFOShape, pos, lfoShapeSwitchImage, DGButton::Mode::Radio, true);

	// divisor LFO shape switch
	pos.set(kDivisorLFOShapeSwitchX, kDivisorLFOShapeSwitchY, lfoShapeSwitchImage->getWidth() / 2, lfoShapeSwitchImage->getHeight() / dfx::LFO::kNumShapes);
	emplaceControl<DGButton>(this, kDivisorLFOShape, pos, lfoShapeSwitchImage, DGButton::Mode::Radio, true);


	// forced buffer size label
	pos.set(kBufferSizeLabelX, kBufferSizeLabelY, bufferSizeLabelImage->getWidth(), bufferSizeLabelImage->getHeight() / 2);
	emplaceControl<DGButton>(this, kBufferTempoSync, pos, bufferSizeLabelImage, DGButton::Mode::PictureReel);


	// Destroy FX web page link
	pos.set(kDestroyFXLinkX, kDestroyFXLinkY, destroyFXLinkImage->getWidth(), destroyFXLinkImage->getHeight() / 2);
	emplaceControl<DGWebLink>(this, pos, destroyFXLinkImage, DESTROYFX_URL);

	pos.set(kTitleAreaX, kTitleAreaY, kTitleAreaWidth, kTitleAreaHeight);
	mTitleArea = emplaceControl<DGNullControl>(this, pos);


	// the help mouseover hint thingy
	pos.set(kHelpDisplayX, kHelpDisplayY, GetBackgroundImage()->getWidth(), kHelpDisplayHeight);
	for (auto& helpDisplay : mHelpDisplays)
	{
		helpDisplay = emplaceControl<DGStaticTextDisplay>(this, pos, nullptr, dfx::TextAlignment::Center, kHelpDisplayFontSize, kHelpDisplayTextColor, kHelpDisplayFont);
		pos.offset(0, kHelpDisplayLineSpacing);
	}


	// Visualization
	pos.set(kVisualizationX, kVisualizationY, kVisualizationW, kVisualizationH);
	getFrame()->addView(new BufferOverrideView(pos));


	HandleTempoSyncChange();
	HandleTempoAutoChange();

	return dfx::kStatus_NoError;
}

//-----------------------------------------------------------------------------
void BufferOverrideEditor::CloseEditor()
{
	mDivisorBufferBox = nullptr;
	mDivisorLFORateSlider = nullptr;
	mBufferLFORateSlider = nullptr;
	mBufferSizeDisplay = nullptr;
	mDivisorLFORateDisplay = nullptr;
	mBufferLFORateDisplay = nullptr;
	mTitleArea = nullptr;
	mHelpDisplays.fill(nullptr);
}


//-----------------------------------------------------------------------------
void BufferOverrideEditor::parameterChanged(long inParameterID)
{
	IDGControl* slider = nullptr;
	DGTextDisplay* textDisplay = nullptr;
	auto const useSyncParam = getparameter_b(inParameterID);

	auto newParameterID = dfx::kParameterID_Invalid;
	switch (inParameterID)
	{
		case kBufferTempoSync:
		{
			HandleTempoSyncChange();
			constexpr std::array<long, 2> parameterIDs = { kBufferSize_MS, kBufferSize_Sync };
			newParameterID = parameterIDs[useSyncParam];
			slider = mDivisorBufferBox->getControlByParameterID(parameterIDs[!useSyncParam]);
			textDisplay = mBufferSizeDisplay;
			break;
		}
		case kDivisorLFOTempoSync:
			HandleTempoSyncChange();
			newParameterID = useSyncParam ? kDivisorLFORate_Sync : kDivisorLFORate_Hz;
			slider = mDivisorLFORateSlider;
			textDisplay = mDivisorLFORateDisplay;
			break;
		case kBufferLFOTempoSync:
			HandleTempoSyncChange();
			newParameterID = useSyncParam ? kBufferLFORate_Sync : kBufferLFORate_Hz;
			slider = mBufferLFORateSlider;
			textDisplay = mBufferLFORateDisplay;
			break;
		case kTempoAuto:
			HandleTempoAutoChange();
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
void BufferOverrideEditor::mouseovercontrolchanged(IDGControl* currentControlUnderMouse)
{
	if (std::any_of(mHelpDisplays.cbegin(), mHelpDisplays.cend(), std::bind_front(std::equal_to<>{}, nullptr)))
	{
		return;
	}

	auto const helpStrings = [currentControlUnderMouse, this]() -> std::array<char const* const, kHelpDisplayLineCount>
	{
		auto currentControlParamID = dfx::kParameterID_Invalid;
		if (currentControlUnderMouse)
		{
			if (currentControlUnderMouse->isParameterAttached())
			{
				currentControlParamID = currentControlUnderMouse->getParameterID();
			}

			if (currentControlUnderMouse == mTitleArea)
			{
				return
				{
					PLUGIN_NAME_STRING " can overcome your host application's audio processing buffer size",
					"and then (unsuccessfully) override that new buffer size to be a smaller buffer size."
				};
			}
			if (currentControlUnderMouse == mDivisorBufferBox)
			{
#if TARGET_OS_MAC
	#define BO_XLOCK_KEY "option"
	#define BO_YLOCK_KEY "control"
#else
	#define BO_XLOCK_KEY "alt"
	#define BO_YLOCK_KEY "ctrl"
#endif
				return
				{
					"Left/right controls buffer divisor (number of repeats in a buffer, hold " BO_YLOCK_KEY " key to constrain).",
					"Up/down controls forced buffer size (hold " BO_XLOCK_KEY " key to constrain)."
				};
#undef BO_XLOCK_KEY
#undef BO_YLOCK_KEY
			}
			if (currentControlUnderMouse == GetMidiLearnButton())
			{
				return
				{
					"When MIDI Learn is enabled, you can click on a parameter control and then",
					"the next MIDI CC received will be assigned to control that parameter."
				};
			}
			if (currentControlUnderMouse == GetMidiResetButton())
			{
				return
				{
					"Push this button to erase all of your MIDI CC -> parameter assignments.",
					"Then CCs will no longer affect any parameters and you can start over."
				};
			}
		}

		switch (currentControlParamID)
		{
			case kDivisor:
				return
				{
					"Buffer divisor is the value divided into the forced buffer size, resulting in the mini-buffer size.",
					"It determines how many times you hear the audio repeat within a forced buffer."
				};
			case kBufferSize_MS:
			case kBufferSize_Sync:
				return
				{
					"Forced buffer size is the length of time within which audio \"mini-buffers\" repeat.",
					"If buffer tempo sync is enabled, then the forced buffer size is adapted to the tempo of your song."
				};
			case kBufferTempoSync:
				return {"Enable tempo sync if you want the size of the forced buffers to sync to your tempo.", nullptr};
			case kBufferInterrupt:
				return
				{
					"Buffer interrupt allows for adjustments of the buffer divisor during a forced buffer.",
					"Turn this off for the old version 1 style of stubborn \"stuck\" buffers (if you really want that)."
				};
			case kDivisorLFORate_Hz:
			case kDivisorLFORate_Sync:
				return
				{
					"This is the speed of the LFO that modulates the buffer divisor.",
					"This can be synced to tempo using the \"tempo sync\" button for the LFO."
				};
			case kDivisorLFODepth:
				return
				{
					"This is the depth (or intensity) of the LFO that modulates the buffer divisor.",
					"(0% does nothing.)"
				};
			case kDivisorLFOShape:
				return {"Choose the waveform shape of the LFO that modulates the buffer divisor.", nullptr};
			case kDivisorLFOTempoSync:
				return {"Enable tempo sync if you want the rate of the buffer divisor LFO to sync to your tempo.", nullptr};
			case kBufferLFORate_Hz:
			case kBufferLFORate_Sync:
				return
				{
					"This is the speed of the LFO that modulates the forced buffer size.",
					"This can be synced to tempo using the \"tempo sync\" button for the LFO."
				};
			case kBufferLFODepth:
				return
				{
					"This is the depth (or intensity) of the LFO that modulates the forced buffer size.",
					"(0% does nothing.)"
				};
			case kBufferLFOShape:
				return {"Choose the waveform shape of the LFO that modulates the forced buffer size.", nullptr};
			case kBufferLFOTempoSync:
				return {"Enable tempo sync if you want the rate of the forced buffer size LFO to sync to your tempo.", nullptr};
			case kSmooth:
				return
				{
					"Smoothing lets you decide how much of each mini-buffer is spent crossfading",
					"from the end of the previous mini-buffer to the beginning of the new one. (prevents glitches)"
				};
			case kDryWetMix:
				return
				{
					"Dry/wet mix lets you adjust the balance of the input audio (unprocessed)",
					"and the output audio (processed).  (100% is all processed.)"
				};
			case kPitchBendRange:
				return {"This is the range, in semitones, of MIDI pitch bend's effect on the buffer divisor.", nullptr};
			case kMidiMode:
				return
				{
					"nudge: MIDI notes adjust the buffer divisor",
					"trigger: notes also reset the divisor to 1 when they are released"
				};
			case kTempo:
				return
				{
					"You can adjust the tempo used for sync,",
					"or enable the \"host tempo\" button to get tempo from your host."
				};
			case kTempoAuto:
				return {"Enable this to get the tempo from your host application (if available).", nullptr};
			default:
				break;
		}

		return {nullptr, nullptr};
	}();
	assert(helpStrings.size() == mHelpDisplays.size());

	for (size_t i = 0; i < mHelpDisplays.size(); i++)
	{
		mHelpDisplays[i]->setText(helpStrings[i] ? helpStrings[i] : "");
	}
}

//-----------------------------------------------------------------------------
void BufferOverrideEditor::HandleTempoSyncChange()
{
	auto const updateTextDisplay = [this](DGTextDisplay* control, long tempoSyncParameterID)
	{
		auto const allowTextEdit = !getparameter_b(tempoSyncParameterID);
		control->setTextEditEnabled(allowTextEdit);
	};
	updateTextDisplay(mBufferSizeDisplay, kBufferTempoSync);
	updateTextDisplay(mDivisorLFORateDisplay, kDivisorLFOTempoSync);
	updateTextDisplay(mBufferLFORateDisplay, kBufferLFOTempoSync);
}

//-----------------------------------------------------------------------------
void BufferOverrideEditor::HandleTempoAutoChange()
{
	float const alpha = getparameter_b(kTempoAuto) ? kUnusedControlAlpha : 1.f;
	SetParameterAlpha(kTempo, alpha);
}
