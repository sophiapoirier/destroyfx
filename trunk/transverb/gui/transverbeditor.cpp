/*------------------------------------------------------------------------
Copyright (C) 2001-2018  Tom Murphy 7 and Sophia Poirier

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
#include <cmath>
#include <stdio.h>

#include "transverb.h"


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

	kQualityButtonX = kWideFaderX,
	kRandomButtonX = 185,
	kTomsoundButtonX = 425,
	kButtonY = 236,

	kFineDownButtonX = 503,
	kFineUpButtonX = 512,
	kFineButtonY = kWideFaderY,
	kSpeedModeButtonX = 503,
	kSpeedModeButtonY = 22,

	kMidiLearnButtonX = 237,
	kMidiLearnButtonY = 254,
	kMidiResetButtonX = 288,
	kMidiResetButtonY = 254,

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


static char const* const kDisplayFont = kDGFontName_SnootPixel10;
static DGColor const kDisplayTextColor(103, 161, 215);
constexpr float kDisplayTextSize = kDGFontSize_SnootPixel10;
constexpr float kFineTuneInc = 0.0001f;
constexpr float kSemitonesPerOctave = 12.0f;



//-----------------------------------------------------------------------------
// callbacks for button-triggered action

void randomizeTransverb(long /*value*/, void* editor)
{
	static_cast<DfxGuiEditor*>(editor)->randomizeparameters(true);
}



//-----------------------------------------------------------------------------
// value text display procedures

bool bsizeDisplayProcedure(float value, char* outText, void*)
{
	long const thousands = static_cast<long>(value) / 1000;
	auto const remainder = std::fmod(value, 1000.0f);

	if (thousands > 0)
	{
		snprintf(outText, DGTextDisplay::kTextMaxLength, "%ld,%05.1f", thousands, remainder);
	}
	else
	{
		snprintf(outText, DGTextDisplay::kTextMaxLength, "%.1f", value);
	}
	strcat(outText, " ms");

	return true;
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

	if (speed > 0.0f)
	{
		if (octaves == 0)
		{
			snprintf(outText, DGTextDisplay::kTextMaxLength, "%s%.2f semitones", (semitones < 0.000003f) ? "" : "+", semitones);
		}
		else if (octaves == 1)
		{
			snprintf(outText, DGTextDisplay::kTextMaxLength, "+%d octave & %.2f semitones", octaves, semitones);
		}
		else
		{
			snprintf(outText, DGTextDisplay::kTextMaxLength, "+%d octaves & %.2f semitones", octaves, semitones);
		}
	}
	else if (octaves == 0)
	{
		snprintf(outText, DGTextDisplay::kTextMaxLength, "-%.2f semitones", semitones);
	}
	else
	{
		if (octaves == -1)
		{
			snprintf(outText, DGTextDisplay::kTextMaxLength, "%d octave & %.2f semitones", octaves, semitones);
		}
		else
		{
			snprintf(outText, DGTextDisplay::kTextMaxLength, "%d octaves & %.2f semitones", octaves, semitones);
		}
	}

	return true;
}

bool speedTextConvertProcedure(std::string const& inText, float& outValue, DGTextDisplay* /*textDisplay*/)
{
	std::string filteredText(inText.size(), '\0');
	// TODO: does not support locale for number format, and ignores minus and periods that are not part of fractional numbers
	filteredText.erase(std::remove_copy_if(inText.cbegin(), inText.cend(), filteredText.begin(), [](auto character)
										   {
											   return !(isnumber(character) || isspace(character) || (character == '-') || (character == '.'));
										   }), filteredText.end());

	float octaves = 0.0f, semitones = 0.0f;
	auto const scanCount = sscanf(filteredText.c_str(), "%f%f", &octaves, &semitones);
	if ((scanCount > 0) && (scanCount != EOF))
	{
		// the user only entered one number, which is for octaves, 
		// so convert any fractional part of the octaves value into semitones
		if (scanCount == 1)
		{
			// unless we find the one number labeled as semitones, in which case treat as those
			std::vector<char> word(inText.size() + 1, 0);
			auto const wordScanCount = sscanf(inText.c_str(), "%*f%s", word.data());
			static std::string const wordCompare("semi");
			if ((wordScanCount > 0) && (wordScanCount != EOF) && !strncasecmp(word.data(), wordCompare.c_str(), wordCompare.size()))
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
		return true;
	}
	return false;
}

bool feedbackDisplayProcedure(float value, char* outText, void*)
{
	snprintf(outText, DGTextDisplay::kTextMaxLength, "%ld%%", static_cast<long>(value));
	return true;
}

bool distDisplayProcedure(float value, char* outText, void* editor)
{
	float const distance = value * static_cast<DfxGuiEditor*>(editor)->getparameter_f(kBsize);
	long const thousands = static_cast<long>(distance) / 1000;
	auto const remainder = std::fmod(distance, 1000.0f);

	if (thousands > 0)
	{
		snprintf(outText, DGTextDisplay::kTextMaxLength, "%ld,%06.2f", thousands, remainder);
	}
	else
	{
		snprintf(outText, DGTextDisplay::kTextMaxLength, "%.2f", distance);
	}
	strcat(outText, " ms");

	return true;
}

bool distTextConvertProcedure(std::string const& inText, float& outValue, DGTextDisplay* textDisplay)
{
	auto const scanCount = sscanf(inText.c_str(), "%f", &outValue);
	if ((scanCount > 0) && (scanCount != EOF))
	{
		auto const bsize = static_cast<float>(textDisplay->getOwnerEditor()->getparameter_f(kBsize));
		if (bsize != 0.0f)
		{
			outValue /= bsize;
		}
		return true;
	}
	return false;
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
CMouseEventResult TransverbSpeedTuneButton::onMouseDown(CPoint& inPos, CButtonState const& inButtons)
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
	}

	return kMouseEventHandled;
}



#pragma mark -

//-----------------------------------------------------------------------------
DFX_EDITOR_ENTRY(TransverbEditor)

//-----------------------------------------------------------------------------
TransverbEditor::TransverbEditor(DGEditorListenerInstance inInstance)
:	DfxGuiEditor(inInstance)
{
}

//-----------------------------------------------------------------------------
long TransverbEditor::OpenEditor()
{
	// slider handles
	auto const horizontalSliderHandleImage = makeOwned<DGImage>("purple-wide-fader-handle.png");
	auto const greyHorizontalSliderHandleImage = makeOwned<DGImage>("grey-wide-fader-handle.png");
	auto const verticalSliderHandleImage = makeOwned<DGImage>("tall-fader-handle.png");
	// slider backgrounds
	auto const horizontalSliderBackgroundImage = makeOwned<DGImage>("purple-wide-fader-slide.png");
	auto const greyHorizontalSliderBackgroundImage = makeOwned<DGImage>("grey-wide-fader-slide.png");
	auto const verticalSliderBackgroundImage = makeOwned<DGImage>("tall-fader-slide.png");
	// buttons
	auto const qualityButtonImage = makeOwned<DGImage>("quality-button.png");
	auto const tomsoundButtonImage = makeOwned<DGImage>("tomsound-button.png");
	auto const randomizeButtonImage = makeOwned<DGImage>("randomize-button.png");
	auto const fineDownButtonImage = makeOwned<DGImage>("fine-down-button.png");
	auto const fineUpButtonImage = makeOwned<DGImage>("fine-up-button.png");
	auto const speedModeButtonImage = makeOwned<DGImage>("speed-mode-button.png");
	auto const midiLearnButtonImage = makeOwned<DGImage>("midi-learn-button.png");
	auto const midiResetButtonImage = makeOwned<DGImage>("midi-reset-button.png");
	auto const dfxLinkButtonImage = makeOwned<DGImage>("dfx-link.png");
	auto const superDestroyFXLinkButtonImage = makeOwned<DGImage>("super-destroy-fx-link.png");
	auto const smartElectronixLinkButtonImage = makeOwned<DGImage>("smart-electronix-link.png");



	DGRect pos, textDisplayPos, tuneDownButtonPos, tuneUpButtonPos;
	constexpr long sliderRangeMargin = 1;

	// Make horizontal sliders and add them to the pane
	pos.set(kWideFaderX, kWideFaderY, horizontalSliderBackgroundImage->getWidth(), horizontalSliderBackgroundImage->getHeight());
	textDisplayPos.set(kDisplayX, kDisplayY, kDisplayWidth, kDisplayHeight);
	tuneDownButtonPos.set(kFineDownButtonX, kFineButtonY, fineDownButtonImage->getWidth(), fineDownButtonImage->getHeight() / 2);
	tuneUpButtonPos.set(kFineUpButtonX, kFineButtonY, fineUpButtonImage->getWidth(), fineUpButtonImage->getHeight() / 2);
	for (long tag = kSpeed1; tag <= kDist2; tag++)
	{
		CParamDisplayValueToStringProc displayProc = nullptr;
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
		emplaceControl<DGSlider>(this, tag, pos, kDGAxis_Horizontal, horizontalSliderHandleImage, horizontalSliderBackgroundImage, sliderRangeMargin);

		auto const textDisplay = emplaceControl<DGTextDisplay>(this, tag, textDisplayPos, displayProc, userData, nullptr, 
															   DGTextAlignment::Right, kDisplayTextSize, kDisplayTextColor, kDisplayFont);

		if (tag == kSpeed1)
		{
			auto const tuneMode = getparameter_i(kSpeed1mode);
			mSpeed1DownButton = emplaceControl<TransverbSpeedTuneButton>(this, tag, tuneDownButtonPos, fineDownButtonImage, -kFineTuneInc, tuneMode);
			mSpeed1UpButton = emplaceControl<TransverbSpeedTuneButton>(this, tag, tuneUpButtonPos, fineUpButtonImage, kFineTuneInc, tuneMode);
			textDisplay->setTextToValueProc(speedTextConvertProcedure);
		}
		else if (tag == kSpeed2)
		{
			auto const tuneMode = getparameter_i(kSpeed2mode);
			mSpeed2DownButton = emplaceControl<TransverbSpeedTuneButton>(this, tag, tuneDownButtonPos, fineDownButtonImage, -kFineTuneInc, tuneMode);
			mSpeed2UpButton = emplaceControl<TransverbSpeedTuneButton>(this, tag, tuneUpButtonPos, fineUpButtonImage, kFineTuneInc, tuneMode);
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
			mDistance1TextDisplay = textDisplay;
			textDisplay->setTextToValueProc(distTextConvertProcedure);
		}
		else if (tag == kDist2)
		{
			yoff =  kWideFaderEvenMoreInc;
			mDistance2TextDisplay = textDisplay;
			textDisplay->setTextToValueProc(distTextConvertProcedure);
		}
		pos.offset(0, yoff);
		textDisplayPos.offset(0, yoff);
		tuneDownButtonPos.offset(0, yoff);
		tuneUpButtonPos.offset(0, yoff);
	}

	emplaceControl<DGSlider>(this, kBsize, pos, kDGAxis_Horizontal, greyHorizontalSliderHandleImage, greyHorizontalSliderBackgroundImage, sliderRangeMargin);

	emplaceControl<DGTextDisplay>(this, kBsize, textDisplayPos, bsizeDisplayProcedure, nullptr, nullptr, 
								  DGTextAlignment::Right, kDisplayTextSize, kDisplayTextColor, kDisplayFont);

	emplaceControl<DGFineTuneButton>(this, kBsize, tuneDownButtonPos, fineDownButtonImage, -kFineTuneInc);
	emplaceControl<DGFineTuneButton>(this, kBsize, tuneUpButtonPos, fineUpButtonImage, kFineTuneInc);


	// make horizontal sliders and add them to the view
	pos.set(kTallFaderX, kTallFaderY, verticalSliderBackgroundImage->getWidth(), verticalSliderBackgroundImage->getHeight());
	for (long tag = kDrymix; tag <= kMix2; tag++)
	{
		emplaceControl<DGSlider>(this, tag, pos, kDGAxis_Vertical, verticalSliderHandleImage, verticalSliderBackgroundImage, sliderRangeMargin);
		pos.offset(kTallFaderInc, 0);
	}


	// quality mode button
	pos.set(kQualityButtonX, kButtonY, qualityButtonImage->getWidth() / 2, qualityButtonImage->getHeight()/kQualityMode_NumModes);
	emplaceControl<DGButton>(this, kQuality, pos, qualityButtonImage, kQualityMode_NumModes, DGButton::Mode::Increment, true);

	// TOMSOUND button
	pos.set(kTomsoundButtonX, kButtonY, tomsoundButtonImage->getWidth() / 2, tomsoundButtonImage->getHeight() / 2);
	emplaceControl<DGButton>(this, kTomsound, pos, tomsoundButtonImage, 2, DGButton::Mode::Increment, true);

	// randomize button
	pos.set(kRandomButtonX, kButtonY, randomizeButtonImage->getWidth(), randomizeButtonImage->getHeight() / 2);
	auto const button = emplaceControl<DGButton>(this, pos, randomizeButtonImage, 2, DGButton::Mode::Momentary);
	button->setUserProcedure(randomizeTransverb, this);

	// speed 1 mode button
	pos.set(kSpeedModeButtonX, kSpeedModeButtonY, speedModeButtonImage->getWidth() / 2, speedModeButtonImage->getHeight()/kSpeedMode_NumModes);
	emplaceControl<DGButton>(this, kSpeed1mode, pos, speedModeButtonImage, kSpeedMode_NumModes, DGButton::Mode::Increment, true);
	//
	// speed 2 mode button
	pos.offset(0, (kWideFaderInc * 2) + kWideFaderMoreInc);
	emplaceControl<DGButton>(this, kSpeed2mode, pos, speedModeButtonImage, kSpeedMode_NumModes, DGButton::Mode::Increment, true);

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


	return kDfxErr_NoError;
}

//-----------------------------------------------------------------------------
void TransverbEditor::parameterChanged(long inParameterID)
{
	auto const tuneMode = getparameter_i(inParameterID);

	switch (inParameterID)
	{
		case kBsize:
			// trigger re-conversion of numerical value to text
			mDistance1TextDisplay->refreshText();
			mDistance2TextDisplay->refreshText();
			break;
		case kSpeed1mode:
			mSpeed1DownButton->setTuneMode(tuneMode);
			mSpeed1UpButton->setTuneMode(tuneMode);
			break;
		case kSpeed2mode:
			mSpeed2DownButton->setTuneMode(tuneMode);
			mSpeed2UpButton->setTuneMode(tuneMode);
			break;
		default:
			return;
	}
}
