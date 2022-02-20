/*------------------------------------------------------------------------
Copyright (C) 2001-2022  Tom Murphy 7 and Sophia Poirier

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

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "transverbeditor.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string_view>

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

static bool bsizeDisplayProcedure(float inValue, char* outText, void*)
{
	long const thousands = static_cast<long>(inValue) / 1000;
	auto const remainder = std::fmod(inValue, 1000.0f);

	bool success = false;
	if (thousands > 0)
	{
		success = snprintf(outText, DGTextDisplay::kTextMaxLength, "%ld,%05.1f", thousands, remainder) > 0;
	}
	else
	{
		success = snprintf(outText, DGTextDisplay::kTextMaxLength, "%.1f", inValue) > 0;
	}
	dfx::StrlCat(outText, " ms", DGTextDisplay::kTextMaxLength);

	return success;
}

static bool speedDisplayProcedure(float inValue, char* outText, void*)
{
	std::array<char, 16> semitonesString {};
	auto speed = inValue;
	auto const remainder = std::fmod(std::fabs(speed), 1.0f);
	float semitones = remainder * kSemitonesPerOctave;
	// make sure that these float crap doesn't result in wacky stuff
	// like displays that say "-1 octave & 12.00 semitones"
	snprintf(semitonesString.data(), semitonesString.size(), "%.3f", semitones);
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

	if (speed > 0.0f)
	{
		if (octaves == 0)
		{
			return snprintf(outText, DGTextDisplay::kTextMaxLength, "%s%.2f semitones", (semitones < 0.000003f) ? "" : "+", semitones) > 0;
		}
		auto const octavesSuffix = (octaves == 1) ? "" : "s";
		return snprintf(outText, DGTextDisplay::kTextMaxLength, "+%d octave%s & %.2f semitones", octaves, octavesSuffix, semitones) > 0;
	}
	else if (octaves == 0)
	{
		return snprintf(outText, DGTextDisplay::kTextMaxLength, "-%.2f semitones", semitones) > 0;
	}
	else
	{
		auto const octavesSuffix = (octaves == -1) ? "" : "s";
		return snprintf(outText, DGTextDisplay::kTextMaxLength, "%d octave%s & %.2f semitones", octaves, octavesSuffix, semitones) > 0;
	}
}

static std::optional<float> speedTextConvertProcedure(std::string const& inText, DGTextDisplay*)
{
	std::string filteredText(inText.size(), '\0');
	// TODO: does not support locale for number format, and ignores minus and periods that are not part of fractional numbers
	filteredText.erase(std::remove_copy_if(inText.cbegin(), inText.cend(), filteredText.begin(), [](auto character)
										   {
										     return !(std::isdigit(character) || std::isspace(character) || (character == '-') || (character == '.'));
										   }), filteredText.cend());

	float octaves = 0.0f, semitones = 0.0f;
	auto const scanCount = sscanf(filteredText.c_str(), "%f%f", &octaves, &semitones);
	if ((scanCount > 0) && (scanCount != EOF))
	{
		// the user only entered one number, which is for octaves,
		// so convert any fractional part of the octaves value into semitones
		if (scanCount == 1)
		{
			// unless we find the one number labeled as semitones, in which case treat as those
			std::vector<char> word(inText.size() + 1, '\0');
			auto const wordScanCount = sscanf(inText.c_str(), "%*f%s", word.data());
			constexpr std::string_view wordCompare("semi");
			if ((wordScanCount > 0) && (wordScanCount != EOF) && !strncasecmp(word.data(), wordCompare.data(), wordCompare.size()))
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

static bool feedbackDisplayProcedure(float inValue, char* outText, void*)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%ld%%", static_cast<long>(inValue)) > 0;
}

static bool distDisplayProcedure(float inValue, char* outText, void* inEditor)
{
	float const distance = inValue * static_cast<DfxGuiEditor*>(inEditor)->getparameter_f(kBsize);
	long const thousands = static_cast<long>(distance) / 1000;
	auto const remainder = std::fmod(distance, 1000.0f);

	bool success = false;
	if (thousands > 0)
	{
		success = snprintf(outText, DGTextDisplay::kTextMaxLength, "%ld,%06.2f", thousands, remainder) > 0;
	}
	else
	{
		success = snprintf(outText, DGTextDisplay::kTextMaxLength, "%.2f", distance) > 0;
	}
	dfx::StrlCat(outText, " ms", DGTextDisplay::kTextMaxLength);

	return success;
}

static float distValueFromTextConvertProcedure(float inValue, DGTextDisplay* inTextDisplay)
{
	auto const bsize = static_cast<float>(inTextDisplay->getOwnerEditor()->getparameter_f(kBsize));
	return (bsize != 0.0f) ? (inValue / bsize) : inValue;
}



//-----------------------------------------------------------------------------

static double nearestIntegerBelow(double number)
{
	bool const sign = (number >= 0.0);
	auto const fraction = std::fmod(std::fabs(number), 1.0);

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
	double const fraction = std::fmod(std::fabs(number), 1.0);

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
VSTGUI::CMouseEventResult TransverbSpeedTuneButton::onMouseDown(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons)
{
	if ((mTuneMode == kSpeedMode_Fine) || !inButtons.isLeftButton())
	{
		return DGFineTuneButton::onMouseDown(inPos, inButtons);
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

	return VSTGUI::kMouseEventHandled;
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
long TransverbEditor::OpenEditor()
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
	constexpr long sliderRangeMargin = 1;

	// Make horizontal sliders and add them to the pane
	pos.set(kWideFaderX, kWideFaderY, horizontalSliderBackgroundImage->getWidth(), horizontalSliderBackgroundImage->getHeight());
	textDisplayPos.set(kDisplayX, kDisplayY, kDisplayWidth, kDisplayHeight);
	tuneDownButtonPos.set(kFineDownButtonX, kFineButtonY, fineDownButtonImage->getWidth(), fineDownButtonImage->getHeight() / 2);
	tuneUpButtonPos.set(kFineUpButtonX, kFineButtonY, fineUpButtonImage->getWidth(), fineUpButtonImage->getHeight() / 2);
	for (long tag = kSpeed1; tag <= kDist2; tag++)
	{
		VSTGUI::CParamDisplayValueToStringProc displayProc = nullptr;
		void* userData = nullptr;
		if ((tag == kSpeed1) || (tag == kSpeed2))
		{
			displayProc = speedDisplayProcedure;
		}
		else if ((tag == kFeed1) || (tag == kFeed2))
		{
			displayProc = feedbackDisplayProcedure;
		}
		else
		{
			displayProc = distDisplayProcedure;
			userData = this;
		}
		assert(displayProc);
		emplaceControl<DGSlider>(this, tag, pos, dfx::kAxis_Horizontal, horizontalSliderHandleImage, horizontalSliderBackgroundImage, sliderRangeMargin)->setAlternateHandle(horizontalSliderHandleImage_glowing);

		auto const textDisplay = emplaceControl<DGTextDisplay>(this, tag, textDisplayPos, displayProc, userData, nullptr,
															   dfx::TextAlignment::Right, kDisplayTextSize, kDisplayTextColor, kDisplayFont);

		if (auto const speedTag = std::find(kSpeedParameters.cbegin(), kSpeedParameters.cend(), tag); speedTag != kSpeedParameters.cend())
		{
			auto const head = static_cast<size_t>(std::distance(kSpeedParameters.cbegin(), speedTag));
			mSpeedDownButtons[head] = emplaceControl<TransverbSpeedTuneButton>(this, tag, tuneDownButtonPos, fineDownButtonImage, -kFineTuneInc);
			mSpeedUpButtons[head] = emplaceControl<TransverbSpeedTuneButton>(this, tag, tuneUpButtonPos, fineUpButtonImage, kFineTuneInc);
			textDisplay->setTextToValueProc(speedTextConvertProcedure);
		}
		else
		{
			emplaceControl<DGFineTuneButton>(this, tag, tuneDownButtonPos, fineDownButtonImage, -kFineTuneInc);
			emplaceControl<DGFineTuneButton>(this, tag, tuneUpButtonPos, fineUpButtonImage, kFineTuneInc);
		}

		long yoff = kWideFaderInc;
		for (size_t head = 0; head < kNumDelays; head++)
		{
			if (tag == kDistParameters[head])
			{
				bool const lastHead = (kDistParameters[head] == kDistParameters.back());
				yoff = lastHead ? kWideFaderEvenMoreInc : kWideFaderMoreInc;
				mDistanceTextDisplays[head] = textDisplay;
				textDisplay->setValueFromTextConvertProc(distValueFromTextConvertProcedure);
			}
		}
		pos.offset(0, yoff);
		textDisplayPos.offset(0, yoff);
		tuneDownButtonPos.offset(0, yoff);
		tuneUpButtonPos.offset(0, yoff);
	}

	emplaceControl<DGSlider>(this, kBsize, pos, dfx::kAxis_Horizontal, grayHorizontalSliderHandleImage, grayHorizontalSliderBackgroundImage, sliderRangeMargin)->setAlternateHandle(horizontalSliderHandleImage_glowing);

	emplaceControl<DGTextDisplay>(this, kBsize, textDisplayPos, bsizeDisplayProcedure, nullptr, nullptr,
								  dfx::TextAlignment::Right, kDisplayTextSize, kDisplayTextColor, kDisplayFont);

	emplaceControl<DGFineTuneButton>(this, kBsize, tuneDownButtonPos, fineDownButtonImage, -kFineTuneInc);
	emplaceControl<DGFineTuneButton>(this, kBsize, tuneUpButtonPos, fineUpButtonImage, kFineTuneInc);


	// make horizontal sliders and add them to the view
	pos.set(kTallFaderX, kTallFaderY, verticalSliderBackgroundImage->getWidth(), verticalSliderBackgroundImage->getHeight());
	for (long tag = kDrymix; tag <= kMix2; tag++)
	{
		emplaceControl<DGSlider>(this, tag, pos, dfx::kAxis_Vertical, verticalSliderHandleImage, verticalSliderBackgroundImage, sliderRangeMargin)->setAlternateHandle(verticalSliderHandleImage_glowing);
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
		// TODO: C++20 bind_front
		mSpeedModeButtons[speedModeIndex]->setUserProcedure(std::bind(&TransverbEditor::HandleSpeedModeButton, this, speedModeIndex, std::placeholders::_1));
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
	SetParameterHelpText(kSpeed1, speedHelpText);
	SetParameterHelpText(kSpeed2, speedHelpText);
	std::for_each(mSpeedModeButtons.begin(), mSpeedModeButtons.end(), [](auto* control){ control->setHelpText(speedHelpText); });
	constexpr char const* const feedbackHelpText = "how much of the delay sound gets mixed back into the delay buffer";
	SetParameterHelpText(kFeed1, feedbackHelpText);
	SetParameterHelpText(kFeed2, feedbackHelpText);
	constexpr char const* const distanceHelpText = "how far behind the delay is from the input signal (only really makes a difference when the speed is at zero)";
	SetParameterHelpText(kDist1, distanceHelpText);
	SetParameterHelpText(kDist2, distanceHelpText);
	SetParameterHelpText(kDrymix, "input audio mix level");
	SetParameterHelpText(kMix1, "delay head #1 mix level");
	SetParameterHelpText(kMix2, "delay head #2 mix level");
	SetParameterHelpText(kQuality, "level of transposition quality of the delays' speed");
	SetParameterHelpText(kTomsound, "megaharsh sound");
	SetParameterHelpText(kFreeze, "pause recording new audio into the delay buffer");


	return dfx::kStatus_NoError;
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
void TransverbEditor::parameterChanged(long inParameterID)
{
	switch (inParameterID)
	{
		case kBsize:
			// trigger re-conversion of numerical value to text
			std::for_each(mDistanceTextDisplays.begin(), mDistanceTextDisplays.end(),
						  [](auto& display){ display->refreshText(); });
			break;
		case kFreeze:
			HandleFreezeChange();
			break;
	}
}

//-----------------------------------------------------------------------------
void TransverbEditor::HandlePropertyChange(dfx::PropertyID inPropertyID, dfx::Scope /*inScope*/, unsigned long /*inItemIndex*/)
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
	std::for_each(kFeedParameters.cbegin(), kFeedParameters.cend(),
				  [this, alpha](auto parameterID){ SetParameterAlpha(parameterID, alpha); });
}
