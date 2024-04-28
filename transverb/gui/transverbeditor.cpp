/*------------------------------------------------------------------------
Copyright (C) 2001-2024  Tom Murphy 7 and Sophia Poirier

This file is part of Transverb.

Transverb is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Transverb is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Transverb.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#include "transverbeditor.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <optional>

#include "dfxmath.h"
#include "dfxmisc.h"

using namespace dfx::TV;


//-----------------------------------------------------------------------------
// positions
enum
{
	kWideFaderX = 20,
	kWideFaderY = 35,
	kWideFaderInc = 24,
	kWideFaderMoreInc = 42,
	kWideFaderEvenMoreInc = 50,
	kTallFaderX = kWideFaderX,
	kTallFaderY = 265,
	kTallFaderInc = 28,

	kDisplayX = 318 + 1,
	kDisplayWidth = 180,
	kDisplayHeight = dfx::kFontContainerHeight_Snooty10px,
	kDisplayY = kWideFaderY - kDisplayHeight - 1,

	kQualityButtonX = 425,
	kTomsoundButtonX = kQualityButtonX,
	kFreezeButtonX = kWideFaderX,
	kRandomButtonX = 185,
	kButtonY = 236,
	kButtonIncY = 18,

	kFineDownButtonX = 503,
	kFineUpButtonX = 512,
	kFineButtonY = kWideFaderY,
	kSpeedModeButtonX = 503,
	kSpeedModeButtonY = 22,

	kMidiLearnButtonX = 237,
	kMidiLearnButtonY = kButtonY + kButtonIncY,
	kMidiResetButtonX = 288,
	kMidiResetButtonY = kMidiLearnButtonY,

	kDFXLinkX = 107,
	kDFXLinkY = 281,
	kDestroyFXLinkX = 351,
	kDestroyFXLinkY = 339
};


constexpr auto kDisplayFont = dfx::kFontName_Snooty10px;
constexpr DGColor kDisplayTextColor(103, 161, 215);
constexpr auto kDisplayTextSize = dfx::kFontSize_Snooty10px;
constexpr float kUnusedControlAlpha = 0.39f;
constexpr float kFineTuneInc = 0.0001f;
constexpr float kSemitonesPerOctave = 12.0f;

//-----------------------------------------------------------------------------
// value text display procedures

template <std::floating_point T>
constexpr T modfMagnitude(T inValue)
{
	T integral_ignored {};
	return std::fabs(std::modf(inValue, &integral_ignored));
}

static std::string speedDisplayProcedure(float inValue, DGTextDisplay&)
{
	std::array<char, 16> semitonesString {};
	auto speed = inValue;
	auto const remainder = modfMagnitude(speed);
	float semitones = remainder * kSemitonesPerOctave;
	// make sure that these float crap doesn't result in wacky stuff
	// like displays that say "-1 octave & 12.00 semitones"
	[[maybe_unused]] auto printCount = std::snprintf(semitonesString.data(), semitonesString.size(), "%.3f", semitones);
	assert(printCount > 0);
	std::string const semitonesStdString(semitonesString.data());
	if ((semitonesStdString == "12.000") || (semitonesStdString == "-12.000"))
	{
		semitones = 0.0f;
		if (speed < 0.0f)
		{
			speed -= 0.003f;
		}
		else
		{
			speed += 0.003f;
		}
	}
	auto const octaves = static_cast<int>(speed);

	std::array<char, DGTextDisplay::kTextMaxLength> text {};
	if (speed > 0.0f)
	{
		if (octaves == 0)
		{
			printCount = std::snprintf(text.data(), text.size(), "%s%.2f semitones", (semitones < 0.000003f) ? "" : "+", semitones);
		}
		else
		{
			auto const octavesSuffix = (octaves == 1) ? "" : "s";
			printCount = std::snprintf(text.data(), text.size(), "+%d octave%s & %.2f semitones", octaves, octavesSuffix, semitones);
		}
	}
	else if (octaves == 0)
	{
		printCount = std::snprintf(text.data(), text.size(), "-%.2f semitones", semitones);
	}
	else
	{
		auto const octavesSuffix = (octaves == -1) ? "" : "s";
		printCount = std::snprintf(text.data(), text.size(), "%d octave%s & %.2f semitones", octaves, octavesSuffix, semitones);
	}
	assert(printCount > 0);
	return text.data();
}

static std::optional<float> speedTextConvertProcedure(std::string const& inText, DGTextDisplay&)
{
	auto filteredText = inText;
	// TODO: does not support locale for number format, and ignores minus and periods that are not part of fractional numbers
	std::erase_if(filteredText, [](auto character)
	{
		return !(std::isdigit(character) || std::isspace(character) || (character == '-') || (character == '.'));
	});

	float octaves = 0.0f, semitones = 0.0f;
	auto const scanCount = std::sscanf(filteredText.c_str(), "%f%f", &octaves, &semitones);
	if ((scanCount > 0) && (scanCount != EOF))
	{
		// the user only entered one number, which is for octaves,
		// so convert any fractional part of the octaves value into semitones
		if (scanCount == 1)
		{
			// unless we find the one number labeled as semitones, in which case treat as those
			std::vector<char> word(inText.size() + 1, '\0');
			auto const wordScanCount = std::sscanf(inText.c_str(), "%*f%s", word.data());
			if ((wordScanCount > 0) && (wordScanCount != EOF) && dfx::ToLower(word.data()).starts_with("semi"))
			{
				return octaves / kSemitonesPerOctave;
			}
			return octaves;
		}

		// ignore the sign for the semitones unless the octaves value was in the zero range
		auto const negative = std::signbit(octaves) || ((std::fabs(octaves) < 1.0f) && std::signbit(semitones));
		return (std::floor(std::fabs(octaves)) + (std::fabs(semitones) / kSemitonesPerOctave)) * (negative ? -1.0f : 1.0f);
	}
	return {};
}

static std::string distDisplayProcedure(float inValue, DGTextDisplay& inTextDisplay)
{
	float const distance = inValue * inTextDisplay.getOwnerEditor()->getparameter_f(kBsize);
	return DGTextDisplay::valueToTextProc_Generic(distance, inTextDisplay);
}

static float distValueFromTextConvertProcedure(float inValue, DGTextDisplay& inTextDisplay)
{
	auto const bsize = static_cast<float>(inTextDisplay.getOwnerEditor()->getparameter_f(kBsize));
	return !dfx::math::IsZero(bsize) ? (inValue / bsize) : inValue;
}



//-----------------------------------------------------------------------------

static double nearestIntegerBelow(double number)
{
	bool const sign = (number >= 0.0);
	auto const fraction = modfMagnitude(number);

	if (fraction <= 0.0001)
	{
		return number;
	}

	if (sign)
	{
		return static_cast<double>(static_cast<long>(std::fabs(number)));
	}
	else
	{
		return -static_cast<double>(static_cast<long>(std::fabs(number)) + 1);
	}
}

static double nearestIntegerAbove(double number)
{
	bool const sign = (number >= 0.0);
	double const fraction = modfMagnitude(number);

	if (fraction <= 0.0001)
	{
		return number;
	}

	if (sign)
	{
		return static_cast<double>(static_cast<long>(std::fabs(number)) + 1);
	}
	else
	{
		return -static_cast<double>(static_cast<long>(std::fabs(number)));
	}
}

//-----------------------------------------------------------------------------
void TransverbSpeedTuneButton::onMouseDownEvent(VSTGUI::MouseDownEvent& ioEvent)
{
	if ((mTuneMode == kSpeedMode_Fine) || !ioEvent.buttonState.isLeft())
	{
		return DGFineTuneButton::onMouseDownEvent(ioEvent);
	}

	beginEdit();

	mEntryValue = getValue();

	auto const oldSpeedValue = getOwnerEditor()->getparameter_f(getParameterID());
	bool const isInc = (mValueChangeAmount >= 0.0f);
	double const snapAmount = isInc ? 1.001 : -1.001;
	double const snapScalar = (mTuneMode == kSpeedMode_Semitone) ? 12.0 : 1.0;

	double newSpeedValue = (oldSpeedValue * snapScalar) + snapAmount;
	newSpeedValue = isInc ? nearestIntegerBelow(newSpeedValue) : nearestIntegerAbove(newSpeedValue);
	newSpeedValue /= snapScalar;
	if (isParameterAttached())
	{
		newSpeedValue = getOwnerEditor()->dfxgui_ContractParameterValue(getParameterID(), newSpeedValue);
	}
	mNewValue = std::clamp(static_cast<float>(newSpeedValue), getMin(), getMax());

	mMouseIsDown = true;
	if (mNewValue != mEntryValue)
	{
		setValue(mNewValue);
		valueChanged();
		invalid();
	}
	else
	{
		//redraw();  // at least make sure that redrawing occurs for mMouseIsDown change
		// XXX or do I prefer it not to do the mouse-down state when nothing is happening anyway?
	}

	ioEvent.consumed = true;
}



#pragma mark -

//-----------------------------------------------------------------------------
DFX_EDITOR_ENTRY(TransverbEditor)

//-----------------------------------------------------------------------------
TransverbEditor::TransverbEditor(DGEditorListenerInstance inInstance)
:	DfxGuiEditor(inInstance)
{
	for (size_t i = 0; i < kNumDelays; i++)
	{
		RegisterPropertyChange(speedModeIndexToPropertyID(i));
	}
}

//-----------------------------------------------------------------------------
void TransverbEditor::OpenEditor()
{
	// slider handles
	auto const horizontalSliderHandleImage = LoadImage("purple-wide-fader-handle.png");
	auto const grayHorizontalSliderHandleImage = LoadImage("grey-wide-fader-handle.png");
	auto const horizontalSliderHandleImage_glowing = LoadImage("wide-fader-handle-glowing.png");
	auto const verticalSliderHandleImage = LoadImage("tall-fader-handle.png");
	auto const verticalSliderHandleImage_glowing = LoadImage("tall-fader-handle-glowing.png");
	// slider backgrounds
	auto const horizontalSliderBackgroundImage = LoadImage("purple-wide-fader-slide.png");
	auto const grayHorizontalSliderBackgroundImage = LoadImage("grey-wide-fader-slide.png");
	auto const verticalSliderBackgroundImage = LoadImage("tall-fader-slide.png");
	// buttons
	auto const qualityButtonImage = LoadImage("quality-button.png");
	auto const tomsoundButtonImage = LoadImage("tomsound-button.png");
	auto const freezeButtonImage = LoadImage("freeze-button.png");
	auto const randomizeButtonImage = LoadImage("randomize-button.png");
	auto const fineDownButtonImage = LoadImage("fine-down-button.png");
	auto const fineUpButtonImage = LoadImage("fine-up-button.png");
	auto const speedModeButtonImage = LoadImage("speed-mode-button.png");
	auto const midiLearnButtonImage = LoadImage("midi-learn-button.png");
	auto const midiResetButtonImage = LoadImage("midi-reset-button.png");
	auto const dfxLinkButtonImage = LoadImage("dfx-link.png");
	auto const destroyFXLinkButtonImage = LoadImage("super-destroy-fx-link.png");



	DGRect pos, textDisplayPos, tuneDownButtonPos, tuneUpButtonPos;
	constexpr int sliderRangeMargin = 1;

	// Make horizontal sliders and add them to the pane
	pos.set(kWideFaderX, kWideFaderY, horizontalSliderBackgroundImage->getWidth(), horizontalSliderBackgroundImage->getHeight());
	textDisplayPos.set(kDisplayX, kDisplayY, kDisplayWidth, kDisplayHeight);
	tuneDownButtonPos.set(kFineDownButtonX, kFineButtonY, fineDownButtonImage->getWidth(), fineDownButtonImage->getHeight() / 2);
	tuneUpButtonPos.set(kFineUpButtonX, kFineButtonY, fineUpButtonImage->getWidth(), fineUpButtonImage->getHeight() / 2);
	for (dfx::ParameterID parameterID = kSpeed1; parameterID <= kDistParameters.back(); parameterID++)
	{
		DGTextDisplay::ValueToTextProc displayProc;
		std::optional<uint8_t> displayPrecision;
		// TODO C++23: std::ranges::contains
		if (std::ranges::find(kSpeedParameters, parameterID) != kSpeedParameters.cend())
		{
			displayProc = speedDisplayProcedure;
		}
		// TODO C++23: std::ranges::contains
		else if (std::ranges::find(kFeedParameters, parameterID) != kFeedParameters.cend())
		{
			displayProc = DGTextDisplay::valueToTextProc_Percent;
			displayPrecision = 0;
		}
		else
		{
			displayProc = distDisplayProcedure;
			displayPrecision = 2;
		}
		assert(displayProc);
		emplaceControl<DGSlider>(this, parameterID, pos, dfx::kAxis_Horizontal, horizontalSliderHandleImage, horizontalSliderBackgroundImage, sliderRangeMargin)->setAlternateHandle(horizontalSliderHandleImage_glowing);

		auto const textDisplay = emplaceControl<DGTextDisplay>(this, parameterID, textDisplayPos, displayProc, nullptr,
															   dfx::TextAlignment::Right, kDisplayTextSize, kDisplayTextColor, kDisplayFont);
		if (displayPrecision)
		{
			textDisplay->setPrecision(*displayPrecision);
		}

		if (auto const speedParameterID = std::ranges::find(kSpeedParameters, parameterID); speedParameterID != kSpeedParameters.cend())
		{
			auto const head = static_cast<size_t>(std::ranges::distance(kSpeedParameters.cbegin(), speedParameterID));
			mSpeedDownButtons[head] = emplaceControl<TransverbSpeedTuneButton>(this, parameterID, tuneDownButtonPos, fineDownButtonImage, -kFineTuneInc);
			mSpeedUpButtons[head] = emplaceControl<TransverbSpeedTuneButton>(this, parameterID, tuneUpButtonPos, fineUpButtonImage, kFineTuneInc);
			textDisplay->setTextToValueProc(speedTextConvertProcedure);
		}
		else
		{
			emplaceControl<DGFineTuneButton>(this, parameterID, tuneDownButtonPos, fineDownButtonImage, -kFineTuneInc);
			emplaceControl<DGFineTuneButton>(this, parameterID, tuneUpButtonPos, fineUpButtonImage, kFineTuneInc);
		}


		auto yoff = kWideFaderInc;
		if (auto const distParameterID = std::ranges::find(kDistParameters, parameterID); distParameterID != kDistParameters.cend())
		{
			auto const head = static_cast<size_t>(std::ranges::distance(kDistParameters.cbegin(), distParameterID));
			bool const lastHead = (*distParameterID == kDistParameters.back());
			yoff = lastHead ? kWideFaderEvenMoreInc : kWideFaderMoreInc;
			mDistanceTextDisplays[head] = textDisplay;
			textDisplay->setValueToTextSuffix(" ms");
			textDisplay->setValueFromTextConvertProc(distValueFromTextConvertProcedure);
		}
		pos.offset(0, yoff);
		textDisplayPos.offset(0, yoff);
		tuneDownButtonPos.offset(0, yoff);
		tuneUpButtonPos.offset(0, yoff);
	}

	emplaceControl<DGSlider>(this, kBsize, pos, dfx::kAxis_Horizontal, grayHorizontalSliderHandleImage, grayHorizontalSliderBackgroundImage, sliderRangeMargin)->setAlternateHandle(horizontalSliderHandleImage_glowing);

	auto textDisplay = emplaceControl<DGTextDisplay>(this, kBsize, textDisplayPos, DGTextDisplay::valueToTextProc_Generic, nullptr,
													 dfx::TextAlignment::Right, kDisplayTextSize, kDisplayTextColor, kDisplayFont);
	textDisplay->setValueToTextSuffix(" ms");
	textDisplay->setPrecision(1);

	emplaceControl<DGFineTuneButton>(this, kBsize, tuneDownButtonPos, fineDownButtonImage, -kFineTuneInc);
	emplaceControl<DGFineTuneButton>(this, kBsize, tuneUpButtonPos, fineUpButtonImage, kFineTuneInc);


	// make horizontal sliders and add them to the view
	pos.set(kTallFaderX, kTallFaderY, verticalSliderBackgroundImage->getWidth(), verticalSliderBackgroundImage->getHeight());
	for (dfx::ParameterID parameterID = kDrymix; parameterID <= kMixParameters.back(); parameterID++)
	{
		emplaceControl<DGSlider>(this, parameterID, pos, dfx::kAxis_Vertical, verticalSliderHandleImage, verticalSliderBackgroundImage, sliderRangeMargin)->setAlternateHandle(verticalSliderHandleImage_glowing);
		pos.offset(kTallFaderInc, 0);
	}


	// quality mode button
	pos.set(kQualityButtonX, kButtonY, qualityButtonImage->getWidth() / 2, qualityButtonImage->getHeight() / kQualityMode_NumModes);
	emplaceControl<DGButton>(this, kQuality, pos, qualityButtonImage, DGButton::Mode::Increment, true);

	// TOMSOUND button
	emplaceControl<DGToggleImageButton>(this, kTomsound, kTomsoundButtonX, kButtonY + kButtonIncY, tomsoundButtonImage, true);

	// freeze button
	emplaceControl<DGToggleImageButton>(this, kFreeze, kFreezeButtonX, kButtonY, freezeButtonImage, true);

	// randomize button
	pos.set(kRandomButtonX, kButtonY, randomizeButtonImage->getWidth(), randomizeButtonImage->getHeight() / 2);
	auto const button = emplaceControl<DGButton>(this, pos, randomizeButtonImage, 2, DGButton::Mode::Momentary);
	button->setUserProcedure(std::bind(&TransverbEditor::randomizeparameters, this, true));

	// speed mode buttons
	for (size_t speedModeIndex = 0; speedModeIndex < mSpeedModeButtons.size(); speedModeIndex++)
	{
		pos.set(kSpeedModeButtonX, kSpeedModeButtonY + (((kWideFaderInc * 2) + kWideFaderMoreInc) * speedModeIndex),
				speedModeButtonImage->getWidth() / 2, speedModeButtonImage->getHeight() / kSpeedMode_NumModes);
		mSpeedModeButtons[speedModeIndex] = emplaceControl<DGButton>(this, pos, speedModeButtonImage, kSpeedMode_NumModes, DGButton::Mode::Increment, true);
		mSpeedModeButtons[speedModeIndex]->setUserProcedure(std::bind_front(&TransverbEditor::HandleSpeedModeButton, this, speedModeIndex));
	}

	// MIDI learn button
	CreateMidiLearnButton(kMidiLearnButtonX, kMidiLearnButtonY, midiLearnButtonImage, true);

	// MIDI reset button
	CreateMidiResetButton(kMidiResetButtonX, kMidiResetButtonY, midiResetButtonImage);

	// DFX web link
	pos.set(kDFXLinkX, kDFXLinkY, dfxLinkButtonImage->getWidth(), dfxLinkButtonImage->getHeight() / 2);
	emplaceControl<DGWebLink>(this, pos, dfxLinkButtonImage, DESTROYFX_URL);

	// Super Destroy FX web link
	pos.set(kDestroyFXLinkX, kDestroyFXLinkY, destroyFXLinkButtonImage->getWidth(), destroyFXLinkButtonImage->getHeight() / 2);
	emplaceControl<DGWebLink>(this, pos, destroyFXLinkButtonImage, DESTROYFX_URL);


	SetParameterHelpText(kBsize, "the size of the buffer that both delays use");
	constexpr char const* const speedHelpText = "how quickly or slowly the delay playback moves through the delay buffer";
	for (auto const parameterID : kSpeedParameters)
	{
		SetParameterHelpText(parameterID, speedHelpText);
	}
	std::ranges::for_each(mSpeedModeButtons, [](auto* control){ control->setHelpText(speedHelpText); });
	for (auto const parameterID : kFeedParameters)
	{
		SetParameterHelpText(parameterID, "how much of the delay sound gets mixed back into the delay buffer");
	}
	for (auto const parameterID : kDistParameters)
	{
		SetParameterHelpText(parameterID, "how far behind the delay is from the input signal (only really makes a difference when the speed is at zero)");
	}
	SetParameterHelpText(kDrymix, "input audio mix level");
	for (size_t i = 0; i < kMixParameters.size(); i++)
	{
		SetParameterHelpText(kMixParameters[i], ("delay head #" + std::to_string(i + 1) + " mix level").c_str());
	}
	SetParameterHelpText(kQuality, "level of transposition quality of the delays' speed");
	SetParameterHelpText(kTomsound, "megaharsh sound");
	SetParameterHelpText(kFreeze, "pause recording new audio into the delay buffer");
}

//-----------------------------------------------------------------------------
void TransverbEditor::PostOpenEditor()
{
	for (size_t i = 0; i < kNumDelays; i++)
	{
		HandleSpeedModeChange(i);
	}
	HandleFreezeChange();
}

//-----------------------------------------------------------------------------
void TransverbEditor::CloseEditor()
{
	mSpeedModeButtons.fill(nullptr);
	mSpeedDownButtons.fill(nullptr);
	mSpeedUpButtons.fill(nullptr);
	mDistanceTextDisplays.fill(nullptr);
}

//-----------------------------------------------------------------------------
void TransverbEditor::parameterChanged(dfx::ParameterID inParameterID)
{
	switch (inParameterID)
	{
		case kBsize:
			// trigger re-conversion of numerical value to text
			std::ranges::for_each(mDistanceTextDisplays, std::bind_front(&DGTextDisplay::refreshText));
			break;
		case kFreeze:
			HandleFreezeChange();
			break;
	}
}

//-----------------------------------------------------------------------------
void TransverbEditor::HandlePropertyChange(dfx::PropertyID inPropertyID, dfx::Scope /*inScope*/, unsigned int /*inItemIndex*/)
{
	if (isSpeedModePropertyID(inPropertyID))
	{
		HandleSpeedModeChange(speedModePropertyIDToIndex(inPropertyID));
	}
}

//-----------------------------------------------------------------------------
void TransverbEditor::HandleSpeedModeButton(size_t inIndex, long inValue)
{
	auto const value_fixedSize = static_cast<uint8_t>(inValue);
	[[maybe_unused]] bool const ok = dfxgui_SetProperty(speedModeIndexToPropertyID(inIndex), value_fixedSize);
	assert(ok);
}

//-----------------------------------------------------------------------------
void TransverbEditor::HandleSpeedModeChange(size_t inIndex)
{
	auto const tuneMode = dfxgui_GetProperty<uint8_t>(speedModeIndexToPropertyID(inIndex));
	assert(tuneMode.has_value());
	if (tuneMode)
	{
		auto&& speedModeButton = mSpeedModeButtons.at(inIndex);
		assert(speedModeButton);
		speedModeButton->setValue_i(*tuneMode);
		if (speedModeButton->isDirty())
		{
			speedModeButton->invalid();
		}

		mSpeedDownButtons.at(inIndex)->setTuneMode(*tuneMode);
		mSpeedUpButtons.at(inIndex)->setTuneMode(*tuneMode);
	}
}

//-----------------------------------------------------------------------------
void TransverbEditor::HandleFreezeChange()
{
	float const alpha = getparameter_b(kFreeze) ? kUnusedControlAlpha : 1.f;
	std::ranges::for_each(kFeedParameters, [this, alpha](auto parameterID){ SetParameterAlpha(parameterID, alpha); });
}
