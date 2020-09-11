/*------------------------------------------------------------------------
Copyright (C) 2000-2020  Sophia Poirier

This file is part of Skidder.

Skidder is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Skidder is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Skidder.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "skiddereditor.h"

#include <algorithm>
#include <cmath>
#include <cctype>

#include "dfxmath.h"
#include "dfxmisc.h"
#include "skidder.h"
#include "temporatetable.h"


//-----------------------------------------------------------------------------
constexpr char const* const kValueDisplayFont = "Arial";
constexpr auto kValueDisplayFontColor = DGColor::kWhite;
constexpr float kValueDisplayFontSize = 10.0f;
constexpr float kValueDisplaySmallerFontSize = 9.0f;
constexpr float kUnusedControlAlpha = 0.3f;

//-----------------------------------------------------------------------------
// positions
enum
{
	kSliderX = 36,
	kSliderY = 59,
	kSliderInc = 40,

	kDisplayX = 288,
	kDisplayY = kSliderY + 3,
	kDisplayWidth = 360 - kDisplayX,
	kDisplayHeight = 12,

	kRandMinDisplayX = 0,
	kRandMinDisplayY = kDisplayY,
	kRandMinDisplayWidth = kSliderX - kRandMinDisplayX - 3,
	kRandMinDisplayHeight = kDisplayHeight,

	kParameterNameDisplayX = 14,
	kParameterNameDisplayY = kDisplayY - 17,

	kTempoSyncButtonX = 276,
	kTempoSyncButtonY = kDisplayY + kDisplayHeight + 4,

	kTempoAutoButtonX = kTempoSyncButtonX,
	kTempoAutoButtonY = kTempoSyncButtonY + (kSliderInc * 6),

	kMidiModeButtonX = 105,
	kMidiModeButtonY = 338,

	kVelocityButtonX = kMidiModeButtonX + 11,
	kVelocityButtonY = kMidiModeButtonY + 23,

	kMidiLearnButtonX = 24,
	kMidiLearnButtonY = 338,
	kMidiResetButtonX = kMidiLearnButtonX,
	kMidiResetButtonY = kMidiLearnButtonY + 20,

	kDestroyFXLinkX = 276,
	kDestroyFXLinkY = 359
};



//-----------------------------------------------------------------------------
// parameter value string display conversion functions

bool rateGenDisplayProc(float inValue, long inSyncParameterID, char* outText, DfxGuiEditor* inEditor, bool inShowUnits)
{
	if (inEditor->getparameter_b(kTempoSync))
	{
		if (auto const valueString = inEditor->getparametervaluestring(inSyncParameterID))
		{
			bool success = dfx::StrLCpy(outText, *valueString, DGTextDisplay::kTextMaxLength) > 0;
			dfx::TempoRateTable const tempoRateTable;
			if (*valueString == tempoRateTable.getDisplay(tempoRateTable.getNumRates() - 1))
			{
				success = dfx::StrLCpy(outText, VSTGUI::kInfiniteSymbol, DGTextDisplay::kTextMaxLength) > 0;
			}
			if (inShowUnits)
			{
				dfx::StrlCat(outText, " cycles/beat", DGTextDisplay::kTextMaxLength);
			}
			return success;
		}
	}
	else
	{
		return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.3f%s", inValue, inShowUnits ? " Hz" : "") > 0;
	}
	return false;
}

bool rateDisplayProc(float inValue, char* outText, void* inEditor)
{
	return rateGenDisplayProc(inValue, kRate_Sync, outText, static_cast<DfxGuiEditor*>(inEditor), true);
}

bool rateRandMinDisplayProc(float inValue, char* outText, void* inEditor)
{
	return rateGenDisplayProc(inValue, kRateRandMin_Sync, outText, static_cast<DfxGuiEditor*>(inEditor), false);
}

bool pulsewidthDisplayProc(float inValue, char* outText, void*)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.1f%%", inValue * 100.0f) > 0;
}

bool slopeDisplayProc(float inValue, char* outText, void*)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.3f ms", inValue) > 0;
}

bool gainRandMinDisplayProc(float inValue, char* outText, void* inUserData)
{
	auto const success = DGTextDisplay::valueToTextProc_LinearToDb(inValue, outText, inUserData);
	if (success)
	{
		// truncate units or any suffix beyond the numerical value
		std::transform(outText, outText + strlen(outText), outText, [](auto character)
		{
			return std::isspace(character) ? '\0' : character;
		});
	}
	return success;
}

bool panDisplayProc(float inValue, char* outText, void*)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.1f%%", inValue * 100.0f) > 0;
}

bool tempoDisplayProc(float inValue, char* outText, void*)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.3f bpm", inValue) > 0;
}



//-----------------------------------------------------------------------------
DFX_EDITOR_ENTRY(SkidderEditor)

//-----------------------------------------------------------------------------
SkidderEditor::SkidderEditor(DGEditorListenerInstance inInstance)
:	DfxGuiEditor(inInstance)
{
}

//-----------------------------------------------------------------------------
long SkidderEditor::OpenEditor()
{
	// create images

	// sliders
	auto const sliderBackgroundImage = VSTGUI::makeOwned<DGImage>("slider-background.png");
	auto const sliderHandleImage = VSTGUI::makeOwned<DGImage>("slider-handle.png");
	auto const sliderHandleImage_glowing = VSTGUI::makeOwned<DGImage>("slider-handle-glowing.png");
	auto const rangeSliderHandleLeftImage = VSTGUI::makeOwned<DGImage>("range-slider-handle-left.png");
	auto const rangeSliderHandleLeftImage_glowing = VSTGUI::makeOwned<DGImage>("range-slider-handle-left-glowing.png");
	auto const rangeSliderHandleRightImage = VSTGUI::makeOwned<DGImage>("range-slider-handle-right.png");
	auto const rangeSliderHandleRightImage_glowing = VSTGUI::makeOwned<DGImage>("range-slider-handle-right-glowing.png");

	// mode buttons
	auto const tempoSyncButtonImage = VSTGUI::makeOwned<DGImage>("tempo-sync-button.png");
	auto const tempoAutoButtonImage = VSTGUI::makeOwned<DGImage>("host-tempo-button.png");
	auto const midiModeButtonImage = VSTGUI::makeOwned<DGImage>("midi-mode-button.png");
	auto const velocityButtonImage = VSTGUI::makeOwned<DGImage>("velocity-button.png");

	// other buttons
	auto const midiLearnButtonImage = VSTGUI::makeOwned<DGImage>("midi-learn-button.png");
	auto const midiResetButtonImage = VSTGUI::makeOwned<DGImage>("midi-reset-button.png");
	auto const destroyFXLinkImage = VSTGUI::makeOwned<DGImage>("destroy-fx-link.png");


	auto const rateParameterIDs = GetActiveRateParameterIDs();


	//--initialize the sliders-----------------------------------------------
	DGRect pos;
	constexpr auto rangeSliderPushStyle = DGRangeSlider::PushStyle::Upper;

	// rate   (single slider for "free" Hz rate and tempo synced rate)
	pos.set(kSliderX, kSliderY, sliderBackgroundImage->getWidth(), sliderBackgroundImage->getHeight());
	mRateSlider = emplaceControl<DGRangeSlider>(this, rateParameterIDs.first, rateParameterIDs.second, pos, rangeSliderHandleLeftImage, rangeSliderHandleRightImage, sliderBackgroundImage, rangeSliderPushStyle);
	mRateSlider->setAlternateHandles(rangeSliderHandleLeftImage_glowing, rangeSliderHandleRightImage_glowing);

	// pulsewidth
	pos.offset(0, kSliderInc);
	auto rangeSlider = emplaceControl<DGRangeSlider>(this, kPulsewidthRandMin, kPulsewidth, pos, rangeSliderHandleLeftImage, rangeSliderHandleRightImage, sliderBackgroundImage, rangeSliderPushStyle);
	rangeSlider->setAlternateHandles(rangeSliderHandleLeftImage_glowing, rangeSliderHandleRightImage_glowing);

	// slope
	pos.offset(0, kSliderInc);
	emplaceControl<DGSlider>(this, kSlope, pos, dfx::kAxis_Horizontal, sliderHandleImage, sliderBackgroundImage)->setAlternateHandle(sliderHandleImage_glowing);

	// floor
	pos.offset(0, kSliderInc);
	rangeSlider = emplaceControl<DGRangeSlider>(this, kFloorRandMin, kFloor, pos, rangeSliderHandleLeftImage, rangeSliderHandleRightImage, sliderBackgroundImage, rangeSliderPushStyle);
	rangeSlider->setAlternateHandles(rangeSliderHandleLeftImage_glowing, rangeSliderHandleRightImage_glowing);

	// pan
	pos.offset(0, kSliderInc);
	emplaceControl<DGSlider>(this, kPan, pos, dfx::kAxis_Horizontal, sliderHandleImage, sliderBackgroundImage)->setAlternateHandle(sliderHandleImage_glowing);

	// noise
	pos.offset(0, kSliderInc);
	emplaceControl<DGSlider>(this, kNoise, pos, dfx::kAxis_Horizontal, sliderHandleImage, sliderBackgroundImage)->setAlternateHandle(sliderHandleImage_glowing);

	// tempo (in bpm)
	pos.offset(0, kSliderInc);
	emplaceControl<DGSlider>(this, kTempo, pos, dfx::kAxis_Horizontal, sliderHandleImage, sliderBackgroundImage)->setAlternateHandle(sliderHandleImage_glowing);


	//--initialize the buttons----------------------------------------------

	// choose the rate type ("free" or synced)
	emplaceControl<DGToggleImageButton>(this, kTempoSync, kTempoSyncButtonX, kTempoSyncButtonY, tempoSyncButtonImage);

	// use host tempo
	emplaceControl<DGToggleImageButton>(this, kTempoAuto, kTempoAutoButtonX, kTempoAutoButtonY, tempoAutoButtonImage);

	// MIDI note control mode button
	pos.set(kMidiModeButtonX, kMidiModeButtonY, midiModeButtonImage->getWidth(), midiModeButtonImage->getHeight() / kNumMidiModes);
	emplaceControl<DGButton>(this, kMidiMode, pos, midiModeButtonImage, DGButton::Mode::Increment);

	// use-note-velocity button
	emplaceControl<DGToggleImageButton>(this, kVelocity, kVelocityButtonX, kVelocityButtonY, velocityButtonImage);

	// turn on/off MIDI learn mode for CC parameter automation
	CreateMidiLearnButton(kMidiLearnButtonX, kMidiLearnButtonY, midiLearnButtonImage);

	// clear all MIDI CC assignments
	CreateMidiResetButton(kMidiResetButtonX, kMidiResetButtonY, midiResetButtonImage);
	
	// Destroy FX web page link
	pos.set(kDestroyFXLinkX, kDestroyFXLinkY, destroyFXLinkImage->getWidth(), destroyFXLinkImage->getHeight() / 2);
	emplaceControl<DGWebLink>(this, pos, destroyFXLinkImage, DESTROYFX_URL);


	//--initialize the displays---------------------------------------------

	// rate   (unified display for "free" Hz rate and tempo synced rate)
	pos.set(kDisplayX, kDisplayY, kDisplayWidth, kDisplayHeight);
	mRateDisplay = emplaceControl<DGTextDisplay>(this, rateParameterIDs.second, pos, rateDisplayProc, this, nullptr, dfx::TextAlignment::Left, kValueDisplayFontSize, kValueDisplayFontColor, kValueDisplayFont);

	// rate random minimum
	pos.set(kRandMinDisplayX, kRandMinDisplayY, kRandMinDisplayWidth, kRandMinDisplayHeight);
	mRateRandMinDisplay = emplaceControl<DGTextDisplay>(this, rateParameterIDs.first, pos, rateRandMinDisplayProc, this, nullptr, dfx::TextAlignment::Right, kValueDisplaySmallerFontSize, kValueDisplayFontColor, kValueDisplayFont);

	// pulsewidth
	pos.set(kDisplayX, kDisplayY + (kSliderInc * 1), kDisplayWidth, kDisplayHeight);
	auto textDisplay = emplaceControl<DGTextDisplay>(this, kPulsewidth, pos, pulsewidthDisplayProc, nullptr, nullptr, dfx::TextAlignment::Left, kValueDisplayFontSize, kValueDisplayFontColor, kValueDisplayFont);
	textDisplay->setValueFromTextConvertProc(DGTextDisplay::valueFromTextConvertProc_PercentToLinear);

	// pulsewidth random minimum
	pos.set(kRandMinDisplayX, kRandMinDisplayY + (kSliderInc * 1), kRandMinDisplayWidth, kRandMinDisplayHeight);
	mPulsewidthRandMinDisplay = emplaceControl<DGTextDisplay>(this, kPulsewidthRandMin, pos, pulsewidthDisplayProc, nullptr, nullptr, dfx::TextAlignment::Right, kValueDisplaySmallerFontSize, kValueDisplayFontColor, kValueDisplayFont);
	mPulsewidthRandMinDisplay->setValueFromTextConvertProc(DGTextDisplay::valueFromTextConvertProc_PercentToLinear);

	// slope
	pos.set(kDisplayX, kDisplayY + (kSliderInc * 2), kDisplayWidth, kDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kSlope, pos, slopeDisplayProc, nullptr, nullptr, dfx::TextAlignment::Left, kValueDisplayFontSize, kValueDisplayFontColor, kValueDisplayFont);

	// floor
	pos.offset(0, kSliderInc);
	textDisplay = emplaceControl<DGTextDisplay>(this, kFloor, pos, DGTextDisplay::valueToTextProc_LinearToDb, nullptr, nullptr, dfx::TextAlignment::Left, kValueDisplayFontSize, kValueDisplayFontColor, kValueDisplayFont);
	textDisplay->setTextToValueProc(DGTextDisplay::textToValueProc_DbToLinear);

	// floor random minimum
	pos.set(kRandMinDisplayX, kRandMinDisplayY + (kSliderInc * 3), kRandMinDisplayWidth, kRandMinDisplayHeight);
	mFloorRandMinDisplay = emplaceControl<DGTextDisplay>(this, kFloorRandMin, pos, gainRandMinDisplayProc, nullptr, nullptr, dfx::TextAlignment::Right, kValueDisplaySmallerFontSize, kValueDisplayFontColor, kValueDisplayFont);
	mFloorRandMinDisplay->setTextToValueProc(DGTextDisplay::textToValueProc_DbToLinear);

	// pan
	pos.set(kDisplayX, kDisplayY + (kSliderInc * 4), kDisplayWidth, kDisplayHeight);
	textDisplay = emplaceControl<DGTextDisplay>(this, kPan, pos, panDisplayProc, nullptr, nullptr, dfx::TextAlignment::Left, kValueDisplayFontSize, kValueDisplayFontColor, kValueDisplayFont);
	textDisplay->setValueFromTextConvertProc(DGTextDisplay::valueFromTextConvertProc_PercentToLinear);

	// noise
	pos.offset(0, kSliderInc);
	textDisplay = emplaceControl<DGTextDisplay>(this, kNoise, pos, DGTextDisplay::valueToTextProc_LinearToDb, nullptr, nullptr, dfx::TextAlignment::Left, kValueDisplayFontSize, kValueDisplayFontColor, kValueDisplayFont);
	textDisplay->setTextToValueProc(DGTextDisplay::textToValueProc_DbToLinear);

	// tempo (in bpm)
	pos.offset(0, kSliderInc);
	emplaceControl<DGTextDisplay>(this, kTempo, pos, tempoDisplayProc, nullptr, nullptr, dfx::TextAlignment::Left, kValueDisplayFontSize, kValueDisplayFontColor, kValueDisplayFont);

	// parameter name labels
	auto const addParameterName = [this](int sliderIndex, long inParameterID)
	{
		DGRect const pos(kParameterNameDisplayX, kParameterNameDisplayY + (kSliderInc * sliderIndex), kDisplayWidth, kDisplayHeight);
		auto const label = emplaceControl<DGStaticTextDisplay>(this, pos, nullptr, dfx::TextAlignment::Left, kValueDisplayFontSize, kValueDisplayFontColor, kValueDisplayFont);
		std::array<char, dfx::kParameterNameMaxLength> parameterName;
		dfx::StrLCpy(parameterName.data(), getparametername(inParameterID), parameterName.size());
		// check if it's a separation amount parameter and, if it is, truncate the "(blah)" qualifying part
		auto const breakpoint = strrchr(parameterName.data(), '(');
		if (breakpoint && (breakpoint != parameterName.data()))
		{
			breakpoint[-1] = '\0';
		}
		label->setText(parameterName.data());
	};
	addParameterName(0, rateParameterIDs.second);
	addParameterName(1, kPulsewidth);
	addParameterName(2, kSlope);
	addParameterName(3, kFloor);
	addParameterName(4, kPan);
	addParameterName(5, kNoise);
	addParameterName(6, kTempo);


	UpdateRandomMinimumDisplays();
	HandleTempoSyncChange();
	HandleTempoAutoChange();
	HandleMidiModeChange();
	numAudioChannelsChanged(getNumAudioChannels());


	return dfx::kStatus_NoError;
}

//-----------------------------------------------------------------------------
void SkidderEditor::parameterChanged(long inParameterID)
{
	switch (inParameterID)
	{
		case kTempoSync:
		{
			auto const rateParameterIDs = GetActiveRateParameterIDs();
			mRateSlider->setParameterID(rateParameterIDs.second);
			mRateSlider->getChildren().front()->setParameterID(rateParameterIDs.first);
			mRateDisplay->setParameterID(rateParameterIDs.second);
			mRateRandMinDisplay->setParameterID(rateParameterIDs.first);
			HandleTempoSyncChange();
			UpdateRandomMinimumDisplays();
			break;
		}

		case kRate_Hz:
		case kRate_Sync:
		case kRateRandMin_Hz:
		case kRateRandMin_Sync:
		case kPulsewidth:
		case kPulsewidthRandMin:
		case kFloor:
		case kFloorRandMin:
			UpdateRandomMinimumDisplays();
			break;

		case kTempoAuto:
			HandleTempoAutoChange();
			break;

		case kMidiMode:
		case kVelocity:
			HandleMidiModeChange();
			break;

		default:
			break;
	}
}

//-----------------------------------------------------------------------------
void SkidderEditor::numAudioChannelsChanged(unsigned long inNumChannels)
{
	float const alpha = (inNumChannels == 2) ? 1.0f : kUnusedControlAlpha;
	for (auto& control : mControlsList)
	{
		if (control->getParameterID() == kPan)
		{
			control->setDrawAlpha(alpha);
		}
	}
}

//-----------------------------------------------------------------------------
std::pair<long, long> SkidderEditor::GetActiveRateParameterIDs()
{
	if (getparameter_b(kTempoSync))
	{
		return {kRateRandMin_Sync, kRate_Sync};
	}
	return {kRateRandMin_Hz, kRate_Hz};
}

//-----------------------------------------------------------------------------
void SkidderEditor::UpdateRandomMinimumDisplays()
{
	auto const updateRandomMinimumVisibility = [this](long mainParameterID, long randMinParameterID, VSTGUI::CControl* control)
	{
		bool const visible = getparameter_f(mainParameterID) > getparameter_f(randMinParameterID);
		control->setVisible(visible);
	};

	auto const rateParameterIDs = GetActiveRateParameterIDs();
	updateRandomMinimumVisibility(rateParameterIDs.second, rateParameterIDs.first, mRateRandMinDisplay);
	updateRandomMinimumVisibility(kPulsewidth, kPulsewidthRandMin, mPulsewidthRandMinDisplay);
	updateRandomMinimumVisibility(kFloor, kFloorRandMin, mFloorRandMinDisplay);
}

//-----------------------------------------------------------------------------
void SkidderEditor::HandleTempoSyncChange()
{
	auto const allowTextEdit = !getparameter_b(kTempoSync);
	mRateDisplay->setMouseEnabled(allowTextEdit);
	mRateRandMinDisplay->setMouseEnabled(allowTextEdit);
}

//-----------------------------------------------------------------------------
void SkidderEditor::HandleTempoAutoChange()
{
	float const alpha = getparameter_b(kTempoAuto) ? kUnusedControlAlpha : 1.0f;
	SetParameterAlpha(kTempo, alpha);
}

//-----------------------------------------------------------------------------
void SkidderEditor::HandleMidiModeChange()
{
	bool const isMidiModeEnabled = (getparameter_i(kMidiMode) != kMidiMode_None);

	float const velocityAlpha = isMidiModeEnabled ? 1.0f : kUnusedControlAlpha;
	SetParameterAlpha(kVelocity, velocityAlpha);

	float const floorAlpha = (isMidiModeEnabled && getparameter_b(kVelocity)) ? kUnusedControlAlpha : 1.0f;
	SetParameterAlpha(kFloor, floorAlpha);
	SetParameterAlpha(kFloorRandMin, floorAlpha);
}

//-----------------------------------------------------------------------------
void SkidderEditor::SetParameterAlpha(long inParameterID, float inAlpha)
{
	for (auto& control : mControlsList)
	{
		if (control->getParameterID() == inParameterID)
		{
			control->setDrawAlpha(inAlpha);
		}
	}
}
