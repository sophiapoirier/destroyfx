/*------------------------------------------------------------------------
Copyright (C) 2001-2020  Tom Murphy 7 and Sophia Poirier

This file is part of Transverb.

Transverb is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
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
#include <cassert>
#include <cctype>
#include <cmath>
#include <stdio.h>
#include <string_view>

#include "dfxmisc.h"

#include "dfxtrace.h"

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
	kSuperDFXLinkX = 159,
	kSuperDFXLinkY = 339,
	kSmartElectronixLinkX = 306,
	kSmartElectronixLinkY = kSuperDFXLinkY,

	kDisplayX = 318 - 1,
	kDisplayY = 24,
	kDisplayWidth = 180,
	kDisplayHeight = 10
};


static auto const kDisplayFont = dfx::kFontName_SnootPixel10;
constexpr DGColor kDisplayTextColor(103, 161, 215);
constexpr auto kDisplayTextSize = dfx::kFontSize_SnootPixel10;
constexpr float kFineTuneInc = 0.0001f;
constexpr float kSemitonesPerOctave = 12.0f;

//-----------------------------------------------------------------------------
// value text display procedures

bool bsizeDisplayProcedure(float value, char* outText, void*)
{
	long const thousands = static_cast<long>(value) / 1000;
	auto const remainder = std::fmod(value, 1000.0f);

	bool success = false;
	if (thousands > 0)
	{
		success = snprintf(outText, DGTextDisplay::kTextMaxLength, "%ld,%05.1f", thousands, remainder) > 0;
	}
	else
	{
		success = snprintf(outText, DGTextDisplay::kTextMaxLength, "%.1f", value) > 0;
	}
	dfx::StrlCat(outText, " ms", DGTextDisplay::kTextMaxLength);

	return success;
}

bool speedDisplayProcedure(float value, char* outText, void*)
{
	char semitonesString[16];
	auto speed = value;
	auto const remainder = std::fmod(std::fabs(speed), 1.0f);
	float semitones = remainder * kSemitonesPerOctave;
	// make sure that these float crap doesn't result in wacky stuff 
	// like displays that say "-1 octave & 12.00 semitones"
	snprintf(semitonesString, sizeof(semitonesString), "%.3f", semitones);
	if ((strcmp(semitonesString, "12.000") == 0) || (strcmp(semitonesString, "-12.000") == 0))
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

	bool success = false;
	if (speed > 0.0f)
	{
		if (octaves == 0)
		{
			success = snprintf(outText, DGTextDisplay::kTextMaxLength, "%s%.2f semitones", (semitones < 0.000003f) ? "" : "+", semitones) > 0;
		}
		else if (octaves == 1)
		{
			success = snprintf(outText, DGTextDisplay::kTextMaxLength, "+%d octave & %.2f semitones", octaves, semitones) > 0;
		}
		else
		{
			success = snprintf(outText, DGTextDisplay::kTextMaxLength, "+%d octaves & %.2f semitones", octaves, semitones) > 0;
		}
	}
	else if (octaves == 0)
	{
		success = snprintf(outText, DGTextDisplay::kTextMaxLength, "-%.2f semitones", semitones) > 0;
	}
	else
	{
		if (octaves == -1)
		{
			success = snprintf(outText, DGTextDisplay::kTextMaxLength, "%d octave & %.2f semitones", octaves, semitones) > 0;
		}
		else
		{
			success = snprintf(outText, DGTextDisplay::kTextMaxLength, "%d octaves & %.2f semitones", octaves, semitones) > 0;
		}
	}

	return success;
}

bool speedTextConvertProcedure(std::string const& inText, float& outValue, DGTextDisplay* /*textDisplay*/)
{
	std::string filteredText(inText.size(), '\0');
	// TODO: does not support locale for number format, and ignores minus and periods that are not part of fractional numbers
	filteredText.erase(std::remove_copy_if(inText.cbegin(), inText.cend(), filteredText.begin(), [](auto character)
										   {
										     return !(std::isdigit(character) || std::isspace(character) || (character == '-') || (character == '.'));
										   }), filteredText.cend());

	float octaves = 0.0f, semitones = 0.0f;
	auto const scanCount = sscanf(filteredText.c_str(), "%f%f", &octaves, &semitones);
	bool const success = (scanCount > 0) && (scanCount != EOF);
	if (success)
	{
		// the user only entered one number, which is for octaves, 
		// so convert any fractional part of the octaves value into semitones
		if (scanCount == 1)
		{
			// unless we find the one number labeled as semitones, in which case treat as those
			std::vector<char> word(inText.size() + 1, 0);
			auto const wordScanCount = sscanf(inText.c_str(), "%*f%s", word.data());
			constexpr std::string_view wordCompare("semi");
			if ((wordScanCount > 0) && (wordScanCount != EOF) && !strncasecmp(word.data(), wordCompare.data(), wordCompare.size()))
			{
				outValue = octaves / kSemitonesPerOctave;
			}
			else
			{
				outValue = octaves;
			}
		}
		else
		{
			// ignore the sign for the semitones unless the octaves value was in the zero range
			auto const negative = std::signbit(octaves) || ((std::fabs(octaves) < 1.0f) && std::signbit(semitones));
			outValue = (std::floor(std::fabs(octaves)) + (std::fabs(semitones) / kSemitonesPerOctave)) * (negative ? -1.0f : 1.0f);
		}
	}
	return success;
}

bool feedbackDisplayProcedure(float value, char* outText, void*)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%ld%%", static_cast<long>(value)) > 0;
}

bool distDisplayProcedure(float value, char* outText, void* editor)
{
	float const distance = value * static_cast<DfxGuiEditor*>(editor)->getparameter_f(kBsize);
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

bool distTextConvertProcedure(std::string const& inText, float& outValue, DGTextDisplay* textDisplay)
{
	auto const scanCount = sscanf(inText.c_str(), "%f", &outValue);
	bool const success = (scanCount > 0) && (scanCount != EOF);
	if (success)
	{
		auto const bsize = static_cast<float>(textDisplay->getOwnerEditor()->getparameter_f(kBsize));
		if (bsize != 0.0f)
		{
			outValue /= bsize;
		}
	}
	return success;
}



//-----------------------------------------------------------------------------

double nearestIntegerBelow(double number)
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

double nearestIntegerAbove(double number)
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
	mSpeedModeButtons.fill(nullptr);
	mSpeedDownButtons.fill(nullptr);
	mSpeedUpButtons.fill(nullptr);
	mDistanceTextDisplays.fill(nullptr);

	for (size_t i = 0; i < kNumDelays; i++)
	{
		RegisterPropertyChange(speedModeIndexToPropertyID(i));
	}
}

//-----------------------------------------------------------------------------
long TransverbEditor::OpenEditor()
{
	// slider handles
	auto const horizontalSliderHandleImage = VSTGUI::makeOwned<DGImage>("purple-wide-fader-handle.png");
	auto const grayHorizontalSliderHandleImage = VSTGUI::makeOwned<DGImage>("grey-wide-fader-handle.png");
	auto const horizontalSliderHandleImage_glowing = VSTGUI::makeOwned<DGImage>("wide-fader-handle-glowing.png");
	auto const verticalSliderHandleImage = VSTGUI::makeOwned<DGImage>("tall-fader-handle.png");
	auto const verticalSliderHandleImage_glowing = VSTGUI::makeOwned<DGImage>("tall-fader-handle-glowing.png");
	// slider backgrounds
	auto const horizontalSliderBackgroundImage = VSTGUI::makeOwned<DGImage>("purple-wide-fader-slide.png");
	auto const grayHorizontalSliderBackgroundImage = VSTGUI::makeOwned<DGImage>("grey-wide-fader-slide.png");
	auto const verticalSliderBackgroundImage = VSTGUI::makeOwned<DGImage>("tall-fader-slide.png");
	// buttons
	auto const qualityButtonImage = VSTGUI::makeOwned<DGImage>("quality-button.png");
	auto const tomsoundButtonImage = VSTGUI::makeOwned<DGImage>("tomsound-button.png");
	auto const freezeButtonImage = VSTGUI::makeOwned<DGImage>("freeze-button.png");
	auto const randomizeButtonImage = VSTGUI::makeOwned<DGImage>("randomize-button.png");
	auto const fineDownButtonImage = VSTGUI::makeOwned<DGImage>("fine-down-button.png");
	auto const fineUpButtonImage = VSTGUI::makeOwned<DGImage>("fine-up-button.png");
	auto const speedModeButtonImage = VSTGUI::makeOwned<DGImage>("speed-mode-button.png");
	auto const midiLearnButtonImage = VSTGUI::makeOwned<DGImage>("midi-learn-button.png");
	auto const midiResetButtonImage = VSTGUI::makeOwned<DGImage>("midi-reset-button.png");
	auto const dfxLinkButtonImage = VSTGUI::makeOwned<DGImage>("dfx-link.png");
	auto const superDestroyFXLinkButtonImage = VSTGUI::makeOwned<DGImage>("super-destroy-fx-link.png");
	auto const smartElectronixLinkButtonImage = VSTGUI::makeOwned<DGImage>("smart-electronix-link.png");



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
		emplaceControl<DGSlider>(this, tag, pos, dfx::kAxis_Horizontal, horizontalSliderHandleImage, horizontalSliderBackgroundImage, sliderRangeMargin)->setAlternateHandle(horizontalSliderHandleImage_glowing);

		auto const textDisplay = emplaceControl<DGTextDisplay>(this, tag, textDisplayPos, displayProc, userData, nullptr, 
															   dfx::TextAlignment::Right, kDisplayTextSize, kDisplayTextColor, kDisplayFont);

		if (tag == kSpeed1)
		{
			mSpeedDownButtons[0] = emplaceControl<TransverbSpeedTuneButton>(this, tag, tuneDownButtonPos, fineDownButtonImage, -kFineTuneInc);
			mSpeedUpButtons[0] = emplaceControl<TransverbSpeedTuneButton>(this, tag, tuneUpButtonPos, fineUpButtonImage, kFineTuneInc);
			textDisplay->setTextToValueProc(speedTextConvertProcedure);
		}
		else if (tag == kSpeed2)
		{
			mSpeedDownButtons[1] = emplaceControl<TransverbSpeedTuneButton>(this, tag, tuneDownButtonPos, fineDownButtonImage, -kFineTuneInc);
			mSpeedUpButtons[1] = emplaceControl<TransverbSpeedTuneButton>(this, tag, tuneUpButtonPos, fineUpButtonImage, kFineTuneInc);
			textDisplay->setTextToValueProc(speedTextConvertProcedure);
		}
		else
		{
			emplaceControl<DGFineTuneButton>(this, tag, tuneDownButtonPos, fineDownButtonImage, -kFineTuneInc);
			emplaceControl<DGFineTuneButton>(this, tag, tuneUpButtonPos, fineUpButtonImage, kFineTuneInc);
		}

		long yoff = kWideFaderInc;
		if (tag == kDist1)
		{
			yoff = kWideFaderMoreInc;
			mDistanceTextDisplays[0] = textDisplay;
			textDisplay->setTextToValueProc(distTextConvertProcedure);
		}
		else if (tag == kDist2)
		{
			yoff =  kWideFaderEvenMoreInc;
			mDistanceTextDisplays[1] = textDisplay;
			textDisplay->setTextToValueProc(distTextConvertProcedure);
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
	pos.set(kQualityButtonX, kButtonY, qualityButtonImage->getWidth() / 2, qualityButtonImage->getHeight()/kQualityMode_NumModes);
	emplaceControl<DGButton>(this, kQuality, pos, qualityButtonImage, DGButton::Mode::Increment, true);

	// TOMSOUND button
	pos.set(kTomsoundButtonX, kButtonY + kButtonIncY, tomsoundButtonImage->getWidth() / 2, tomsoundButtonImage->getHeight() / 2);
	emplaceControl<DGButton>(this, kTomsound, pos, tomsoundButtonImage, DGButton::Mode::Increment, true);

	// freeze button
	pos.set(kFreezeButtonX, kButtonY, freezeButtonImage->getWidth() / 2, freezeButtonImage->getHeight() / 2);
	emplaceControl<DGButton>(this, kFreeze, pos, freezeButtonImage, DGButton::Mode::Increment, true);

	// randomize button
	pos.set(kRandomButtonX, kButtonY, randomizeButtonImage->getWidth(), randomizeButtonImage->getHeight() / 2);
	auto const button = emplaceControl<DGButton>(this, pos, randomizeButtonImage, 2, DGButton::Mode::Momentary);
	button->setUserProcedure([](long, void* editor)
	{
		static_cast<DfxGuiEditor*>(editor)->randomizeparameters(true);
	}, this);

	// speed 1 mode button
	{
		constexpr size_t speedModeIndex = 0;
		pos.set(kSpeedModeButtonX, kSpeedModeButtonY, speedModeButtonImage->getWidth() / 2, speedModeButtonImage->getHeight()/kSpeedMode_NumModes);
		mSpeedModeButtons[speedModeIndex] = emplaceControl<DGButton>(this, pos, speedModeButtonImage, kQualityMode_NumModes, DGButton::Mode::Increment, true);
		mSpeedModeButtons[speedModeIndex]->setUserProcedure([](long value, void* editor)
		{
			HandleSpeedModeButton(speedModeIndex, value, editor);
		}, this);
	}
	//
	// speed 2 mode button
	{
		constexpr size_t speedModeIndex = 1;
		pos.offset(0, (kWideFaderInc * 2) + kWideFaderMoreInc);
		mSpeedModeButtons[speedModeIndex] = emplaceControl<DGButton>(this, pos, speedModeButtonImage, kQualityMode_NumModes, DGButton::Mode::Increment, true);
		mSpeedModeButtons[speedModeIndex]->setUserProcedure([](long value, void* editor)
		{
			HandleSpeedModeButton(speedModeIndex, value, editor);
		}, this);
	}

	// MIDI learn button
	CreateMidiLearnButton(kMidiLearnButtonX, kMidiLearnButtonY, midiLearnButtonImage);

	// MIDI reset button
	CreateMidiResetButton(kMidiResetButtonX, kMidiResetButtonY, midiResetButtonImage);

	// DFX web link
	pos.set(kDFXLinkX, kDFXLinkY, dfxLinkButtonImage->getWidth(), dfxLinkButtonImage->getHeight() / 2);
	emplaceControl<DGWebLink>(this, pos, dfxLinkButtonImage, DESTROYFX_URL);

	// Super Destroy FX web link
	pos.set(kSuperDFXLinkX, kSuperDFXLinkY, superDestroyFXLinkButtonImage->getWidth(), superDestroyFXLinkButtonImage->getHeight() / 2);
	emplaceControl<DGWebLink>(this, pos, superDestroyFXLinkButtonImage, DESTROYFX_URL);

	// Smart Electronix web link
	pos.set(kSmartElectronixLinkX, kSmartElectronixLinkY, smartElectronixLinkButtonImage->getWidth(), smartElectronixLinkButtonImage->getHeight() / 2);
	emplaceControl<DGWebLink>(this, pos, smartElectronixLinkButtonImage, SMARTELECTRONIX_URL);


	return dfx::kStatus_NoError;
}

//-----------------------------------------------------------------------------
void TransverbEditor::post_open()
{
	for (size_t i = 0; i < kNumDelays; i++)
	{
		HandleSpeedModeChange(i);
	}
}

//-----------------------------------------------------------------------------
void TransverbEditor::parameterChanged(long inParameterID)
{
	if (inParameterID == kBsize)
	{
		// trigger re-conversion of numerical value to text
		std::for_each(mDistanceTextDisplays.begin(), mDistanceTextDisplays.end(), 
					  [](auto& display){ display->refreshText(); });
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
void TransverbEditor::HandleSpeedModeButton(size_t inIndex, long inValue, void* inEditor)
{
	auto const editor = static_cast<TransverbEditor*>(inEditor);
	auto const value_fixedSize = static_cast<uint8_t>(inValue);
	[[maybe_unused]] bool const ok = editor->dfxgui_SetProperty(speedModeIndexToPropertyID(inIndex), value_fixedSize); 
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
