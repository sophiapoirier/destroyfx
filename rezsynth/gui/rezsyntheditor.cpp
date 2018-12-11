/*------------------------------------------------------------------------
Copyright (C) 2001-2018  Sophia Poirier

This file is part of Rez Synth.

Rez Synth is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Rez Synth is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Rez Synth.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "rezsyntheditor.h"

#include <array>
#include <cmath>

#include "dfxmath.h"
#include "rezsynth.h"


//-----------------------------------------------------------------------------
enum
{
	// positions
	kSliderX = 22,
	kSliderY = 59,
	kSliderInc = 27,//35,

	kDisplayWidth = 123,
	kDisplayHeight = 11,
	kDisplayX = kSliderX,
	kDisplayY = kSliderY - kDisplayHeight - 2,

	kTallSliderX = 383,
	kTallSliderY = 60,
	kTallSliderInc = 51,
	//
	kTallDisplayWidener = 8,
	kTallDisplayX = kTallSliderX - 10 - (kTallDisplayWidener / 2),
	kTallDisplayY = kTallSliderY + 189,
	kTallDisplayWidth = 36 + kTallDisplayWidener,
	kTallDisplayHeight = 12,
	//
	kTallLabelWidth = kTallSliderInc,
	kTallLabelHeight = kDisplayHeight,
	kTallLabelX = kTallDisplayX - ((kTallLabelWidth - kTallDisplayWidth) / 2),
	kTallLabelY = kTallDisplayY + 17,
	kTallLabelInc = 12,

	kResonAlgButtonX = 69,
	kResonAlgButtonY = 26,
	kBandwidthModeButtonX = 274,
	kBandwidthModeButtonY = 53,
	kSepModeButtonX = 274,
	kSepModeButtonY = 121,
	kScaleButtonX = 400,
	kScaleButtonY = 33,
	kLegatoButtonX = 435 - 15,
	kLegatoButtonY = 296,
	kFadeTypeButtonX = 274,
	kFadeTypeButtonY = 176,
	kFoldoverButtonX = 274,
	kFoldoverButtonY = 87,
	kWiseAmpButtonX = 368 - 1,
	kWiseAmpButtonY = 296,
	kDryWetMixModeButtonX = 471,
	kDryWetMixModeButtonY = kWiseAmpButtonY,

	kMidiLearnButtonX = 189,
	kMidiLearnButtonY = 331,
	kMidiResetButtonX = kMidiLearnButtonX + 72,
	kMidiResetButtonY = 331,

	kGoButtonX = 22,
	kGoButtonY = 331,

	kDestroyFXLinkX = 453,
	kDestroyFXLinkY = 327,
	kSmartElectronixLinkX = 358,
	kSmartElectronixLinkY = 345
};


static char const* const kValueTextFont = "Arial";
constexpr float kValueTextFontSize = 10.0f;
//constexpr DGColor kBackgroundColor(43, 93, 63);
//constexpr DGColor kRSGrayColor(111, 111, 111);
constexpr DGColor kRSLightGrayColor(230, 230, 228);
constexpr DGColor kRSVeryLightGrayColor(233, 242, 237);



//-----------------------------------------------------------------------------
// value text display procedures

bool bandwidthAmountDisplayProc(float value, char* outText, void* inEditor)
{
	const auto success = snprintf(outText, DGTextDisplay::kTextMaxLength, "%.3f", value) > 0;
	if (static_cast<DfxGuiEditor*>(inEditor)->getparameter_i(kBandwidthMode) == kBandwidthAmount_Hz)
	{
		strlcat(outText, " Hz", DGTextDisplay::kTextMaxLength);
	}
	else
	{
		strlcat(outText, " Q", DGTextDisplay::kTextMaxLength);
	}
	return success;
}

bool numBandsDisplayProc(float value, char* outText, void*)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.0f", value) > 0;
}

bool sepAmountDisplayProc(float value, char* outText, void* inEditor)
{
	if (static_cast<DfxGuiEditor*>(inEditor)->getparameter_i(kSepMode) == kSeparationMode_Octaval)
	{
		return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.3f semitones", value) > 0;
	}
	else  // linear values
	{
		return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.3f x", value) > 0;
	}
}

bool attackDecayReleaseDisplayProc(float value, char* outText, void*)
{
	long const thousands = static_cast<long>(value) / 1000;
	float const remainder = std::fmod(value, 1000.0f);

	bool success = false;
	if (thousands > 0)
	{
		success = snprintf(outText, DGTextDisplay::kTextMaxLength, "%ld,%05.1f", thousands, remainder) > 0;
	}
	else
	{
		success = snprintf(outText, DGTextDisplay::kTextMaxLength, "%.1f", value) > 0;
	}
	strlcat(outText, " ms", DGTextDisplay::kTextMaxLength);

	return success;
}

bool percentDisplayProc(float value, char* outText, void*)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.0f%%", value) > 0;
}

bool velocityCurveDisplayProc(float value, char* outText, void*)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.3f", value) > 0;
}

bool pitchBendDisplayProc(float value, char* outText, void*)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "\xB1 %.2f semitones", value) > 0;
}

bool gainDisplayProc(float value, char* outText, void*)
{
	bool success = false;
	if (value <= 0.0f)
	{
		success = snprintf(outText, DGTextDisplay::kTextMaxLength, "-%s", dfx::kInfinityUTF8) > 0;
	}
	else if (value > 1.0001f)
	{
		success = snprintf(outText, DGTextDisplay::kTextMaxLength, "+%.1f", dfx::math::Linear2dB(value)) > 0;
	}
	else
	{
		success = snprintf(outText, DGTextDisplay::kTextMaxLength, "%.1f", dfx::math::Linear2dB(value)) > 0;
	}
	strlcat(outText, " dB", DGTextDisplay::kTextMaxLength);
	return success;
}



//-----------------------------------------------------------------------------
DFX_EDITOR_ENTRY(RezSynthEditor)

//-----------------------------------------------------------------------------
RezSynthEditor::RezSynthEditor(DGEditorListenerInstance inInstance)
:	DfxGuiEditor(inInstance)
{
}

//-----------------------------------------------------------------------------
long RezSynthEditor::OpenEditor()
{
	// create images

	// sliders
	auto const horizontalSliderBackgroundImage = VSTGUI::makeOwned<DGImage>("horizontal-slider-background.png");
	auto const horizontalSliderHandleImage = VSTGUI::makeOwned<DGImage>("horizontal-slider-handle.png");
	auto const horizontalSliderHandleImage_glowing = VSTGUI::makeOwned<DGImage>("horizontal-slider-handle-glowing.png");
	auto const verticalSliderBackgroundImage = VSTGUI::makeOwned<DGImage>("vertical-slider-background.png");
	auto const verticalSliderHandleImage = VSTGUI::makeOwned<DGImage>("vertical-slider-handle.png");
	auto const verticalSliderHandleImage_glowing = VSTGUI::makeOwned<DGImage>("vertical-slider-handle-glowing.png");

	// buttons
	auto const resonAlgButtonImage = VSTGUI::makeOwned<DGImage>("reson-algorithm-button.png");
	auto const bandwidthModeButtonImage = VSTGUI::makeOwned<DGImage>("bandwidth-mode-button.png");
	auto const sepModeButtonImage = VSTGUI::makeOwned<DGImage>("separation-mode-button.png");
	auto const scaleModeButtonImage = VSTGUI::makeOwned<DGImage>("scale-mode-button.png");
	auto const fadesButtonImage = VSTGUI::makeOwned<DGImage>("fades-button.png");
	auto const legatoButtonImage = VSTGUI::makeOwned<DGImage>("legato-button.png");
	auto const foldoverButtonImage = VSTGUI::makeOwned<DGImage>("foldover-button.png");
	auto const wiseAmpButtonImage = VSTGUI::makeOwned<DGImage>("wise-amp-button.png");
	auto const dryWetMixModeButtonImage = VSTGUI::makeOwned<DGImage>("dry-wet-mix-mode-button.png");
	auto const midiLearnButtonImage = VSTGUI::makeOwned<DGImage>("midi-learn-button.png");
	auto const midiResetButtonImage = VSTGUI::makeOwned<DGImage>("midi-reset-button.png");
	auto const goButtonImage = VSTGUI::makeOwned<DGImage>("go-button.png");
	auto const destroyFXLinkImage = VSTGUI::makeOwned<DGImage>("destroy-fx-link.png");
	auto const smartElectronixLinkImage = VSTGUI::makeOwned<DGImage>("smart-electronix-link.png");

	auto const verticalValueDisplayBackgroundImage = VSTGUI::makeOwned<DGImage>("vertical-value-display-background.png");



	// create controls

	DGRect pos, labelDisplayPos, valueDisplayPos;

	//--create the horizontal sliders and labels and displays---------------------------------
	pos.set(kSliderX, kSliderY, horizontalSliderBackgroundImage->getWidth(), horizontalSliderBackgroundImage->getHeight());
	labelDisplayPos.set(kDisplayX, kDisplayY, kDisplayWidth, kDisplayHeight);
	valueDisplayPos.set(kDisplayX + kDisplayWidth, kDisplayY, horizontalSliderBackgroundImage->getWidth() - kDisplayWidth, kDisplayHeight);

	auto const addSliderComponents = [&](long const inParamID, auto const& inDisplayProc)
	{
		// slider control
		auto const slider = emplaceControl<DGSlider>(this, inParamID, pos, dfx::kAxis_Horizontal, horizontalSliderHandleImage, horizontalSliderBackgroundImage);
		slider->setAlternateHandle(horizontalSliderHandleImage_glowing);

		// parameter name label
		auto const label = emplaceControl<DGStaticTextDisplay>(labelDisplayPos, nullptr, dfx::TextAlignment::Left, kValueTextFontSize, kRSVeryLightGrayColor, kValueTextFont);
		std::array<char, dfx::kParameterNameMaxLength> paramName;
		strlcpy(paramName.data(), getparametername(inParamID).c_str(), paramName.size());
		// check if it's a separation amount parameter and, if it is, truncate the "(blah)" qualifying part
		auto const breakpoint = strrchr(paramName.data(), '(');
		if (breakpoint && (breakpoint != paramName.data()))
		{
			breakpoint[-1] = 0;
		}
		label->setText(paramName.data());

		// value display
		auto const display = emplaceControl<DGTextDisplay>(this, inParamID, valueDisplayPos, inDisplayProc, this, nullptr, dfx::TextAlignment::Right, kValueTextFontSize, kRSVeryLightGrayColor, kValueTextFont);

		if ((inParamID == kBandwidthAmount_Hz) || (inParamID == kBandwidthAmount_Q))
		{
			mBandwidthAmountSlider = slider;
			mBandwidthAmountDisplay = display;
		}
		else if ((inParamID == kSepAmount_Octaval) || (inParamID == kSepAmount_Linear))
		{
			mSepAmountSlider = slider;
			mSepAmountDisplay = display;
		}

		pos.offset(0, kSliderInc);
		labelDisplayPos.offset(0, kSliderInc);
		valueDisplayPos.offset(0, kSliderInc);
	};

	addSliderComponents((getparameter_i(kBandwidthMode) == kBandwidthMode_Hz) ? kBandwidthAmount_Hz : kBandwidthAmount_Q, bandwidthAmountDisplayProc);
	addSliderComponents(kNumBands, numBandsDisplayProc);
	addSliderComponents((getparameter_i(kSepMode) == kSeparationMode_Octaval) ? kSepAmount_Octaval : kSepAmount_Linear, sepAmountDisplayProc);
	addSliderComponents(kEnvAttack, attackDecayReleaseDisplayProc);
	addSliderComponents(kEnvDecay, attackDecayReleaseDisplayProc);
	addSliderComponents(kEnvSustain, percentDisplayProc);
	addSliderComponents(kEnvRelease, attackDecayReleaseDisplayProc);
	addSliderComponents(kVelocityInfluence, percentDisplayProc);
	addSliderComponents(kVelocityCurve, velocityCurveDisplayProc);
	addSliderComponents(kPitchBendRange, pitchBendDisplayProc);

	//--initialize the vertical gain sliders---------------------------------
	pos.set(kTallSliderX, kTallSliderY, verticalSliderBackgroundImage->getWidth(), verticalSliderBackgroundImage->getHeight());
	valueDisplayPos.set(kTallDisplayX, kTallDisplayY, kTallDisplayWidth, kTallDisplayHeight);
	labelDisplayPos.set(kTallLabelX, kTallLabelY, kTallLabelWidth, kTallLabelHeight);
	for (long paramID = kFilterOutputGain; paramID <= kDryWetMix; paramID++)
	{
		auto displayProc = gainDisplayProc;
		char const* label1 = "filtered";
		char const* label2 = "gain";
		if (paramID == kBetweenGain)
		{
			label1 = "between";
		}
		else if (paramID == kDryWetMix)
		{
			label1 = "dry/wet";
			label2 = "mix";
			displayProc = percentDisplayProc;
		}
		// slider control
		emplaceControl<DGSlider>(this, paramID, pos, dfx::kAxis_Vertical, verticalSliderHandleImage, verticalSliderBackgroundImage)->setAlternateHandle(verticalSliderHandleImage_glowing);

		// value display
		emplaceControl<DGTextDisplay>(this, paramID, valueDisplayPos, displayProc, nullptr, verticalValueDisplayBackgroundImage, dfx::TextAlignment::Center, kValueTextFontSize, kRSLightGrayColor, kValueTextFont);
//		display->setBackgroundColor(kRSGrayColor);
//		display->setFrameColor(kBlackColor);

		// parameter name label
		auto label = emplaceControl<DGStaticTextDisplay>(labelDisplayPos, nullptr, dfx::TextAlignment::Center, kValueTextFontSize, kRSVeryLightGrayColor, kValueTextFont);
		label->setText(label1);
		labelDisplayPos.offset(0, kTallLabelInc);
		label = emplaceControl<DGStaticTextDisplay>(labelDisplayPos, nullptr, dfx::TextAlignment::Center, kValueTextFontSize, kRSVeryLightGrayColor, kValueTextFont);
		label->setText(label2);
		labelDisplayPos.offset(0, -kTallLabelInc);

		pos.offset(kTallSliderInc, 0);
		valueDisplayPos.offset(kTallSliderInc, 0);
		labelDisplayPos.offset(kTallSliderInc, 0);
	}



	//--initialize the buttons----------------------------------------------

	// filter response scaling
	pos.set(kScaleButtonX, kScaleButtonY, scaleModeButtonImage->getWidth(), scaleModeButtonImage->getHeight() / kNumScaleModes);
	emplaceControl<DGButton>(this, kScaleMode, pos, scaleModeButtonImage, DGButton::Mode::Radio);
	// parameter name label
	pos.set(kScaleButtonX, kScaleButtonY - kDisplayHeight - 6, scaleModeButtonImage->getWidth(), kDisplayHeight);
	auto label = emplaceControl<DGStaticTextDisplay>(pos, nullptr, dfx::TextAlignment::Center, kValueTextFontSize, kRSVeryLightGrayColor, kValueTextFont);
	label->setText("filter scaling");

	// bandwidth mode
	pos.set(kBandwidthModeButtonX, kBandwidthModeButtonY, bandwidthModeButtonImage->getWidth(), bandwidthModeButtonImage->getHeight() / kNumBandwidthModes);
	emplaceControl<DGButton>(this, kBandwidthMode, pos, bandwidthModeButtonImage, DGButton::Mode::Increment);

	// separation mode (logarithmic or linear)
	pos.set(kSepModeButtonX, kSepModeButtonY, sepModeButtonImage->getWidth(), sepModeButtonImage->getHeight() / kNumSeparationModes);
	emplaceControl<DGButton>(this, kSepMode, pos, sepModeButtonImage, DGButton::Mode::Increment);

	// legato
	pos.set(kLegatoButtonX, kLegatoButtonY, legatoButtonImage->getWidth(), legatoButtonImage->getHeight() / 2);
	emplaceControl<DGButton>(this, kLegato, pos, legatoButtonImage, DGButton::Mode::Increment);

	// attack and release fade mode
	pos.set(kFadeTypeButtonX, kFadeTypeButtonY, fadesButtonImage->getWidth(), fadesButtonImage->getHeight() /DfxEnvelope::kCurveType_NumTypes);
	emplaceControl<DGButton>(this, kFadeType, pos, fadesButtonImage, DGButton::Mode::Increment);

	// resonance algorithm
	pos.set(kResonAlgButtonX, kResonAlgButtonY, resonAlgButtonImage->getWidth(), resonAlgButtonImage->getHeight() / kNumResonAlgs);
	emplaceControl<DGButton>(this, kResonAlgorithm, pos, resonAlgButtonImage, DGButton::Mode::Radio);
	// parameter name label
	pos.set(kResonAlgButtonX, kResonAlgButtonY - kDisplayHeight - 6, resonAlgButtonImage->getWidth(), kDisplayHeight);
	label = emplaceControl<DGStaticTextDisplay>(pos, nullptr, dfx::TextAlignment::Center, kValueTextFontSize, kRSVeryLightGrayColor, kValueTextFont);
	label->setText("resonance algorithm");

	// allow Nyquist foldover or no
	pos.set(kFoldoverButtonX, kFoldoverButtonY, foldoverButtonImage->getWidth(), foldoverButtonImage->getHeight() / 2);
	emplaceControl<DGButton>(this, kFoldover, pos, foldoverButtonImage, DGButton::Mode::Increment);

	// wisely lower the output gain to accomodate for resonance or no
	pos.set(kWiseAmpButtonX, kWiseAmpButtonY, wiseAmpButtonImage->getWidth(), wiseAmpButtonImage->getHeight() / 2);
	emplaceControl<DGButton>(this, kWiseAmp, pos, wiseAmpButtonImage, DGButton::Mode::Increment);

	// dry/wet mix mode (linear or equal power)
	pos.set(kDryWetMixModeButtonX, kDryWetMixModeButtonY, dryWetMixModeButtonImage->getWidth(), dryWetMixModeButtonImage->getHeight() / kNumDryWetMixModes);
	emplaceControl<DGButton>(this, kDryWetMixMode, pos, dryWetMixModeButtonImage, DGButton::Mode::Increment);

	// turn on/off MIDI learn mode for CC parameter automation
	CreateMidiLearnButton(kMidiLearnButtonX, kMidiLearnButtonY, midiLearnButtonImage);

	// clear all MIDI CC assignments
	CreateMidiResetButton(kMidiResetButtonX, kMidiResetButtonY, midiResetButtonImage);

	// go!
	pos.set(kGoButtonX, kGoButtonY, goButtonImage->getWidth(), goButtonImage->getHeight() / 2);
	emplaceControl<DGWebLink>(this, pos, goButtonImage, "http://www.streetharassmentproject.org/");

	// Destroy FX web page link
	pos.set(kDestroyFXLinkX, kDestroyFXLinkY, destroyFXLinkImage->getWidth(), destroyFXLinkImage->getHeight() / 2);
	emplaceControl<DGWebLink>(this, pos, destroyFXLinkImage, DESTROYFX_URL);

	// Smart Electronix web page link
	pos.set(kSmartElectronixLinkX, kSmartElectronixLinkY, smartElectronixLinkImage->getWidth(), smartElectronixLinkImage->getHeight() / 2);
	emplaceControl<DGWebLink>(this, pos, smartElectronixLinkImage, SMARTELECTRONIX_URL);



	return dfx::kStatus_NoError;
}

//-----------------------------------------------------------------------------
void RezSynthEditor::parameterChanged(long inParameterID)
{
	auto const value_i = getparameter_i(inParameterID);
	auto newParamID = dfx::kParameterID_Invalid;
	DGSlider* slider = nullptr;
	DGTextDisplay* display = nullptr;

	switch (inParameterID)
	{
		case kSepMode:
			newParamID = (value_i == kSeparationMode_Octaval) ? kSepAmount_Octaval : kSepAmount_Linear;
			slider = mSepAmountSlider;
			display = mSepAmountDisplay;
			break;
		case kBandwidthMode:
			newParamID = (value_i == kBandwidthMode_Hz) ? kBandwidthAmount_Hz : kBandwidthAmount_Q;
			slider = mBandwidthAmountSlider;
			display = mBandwidthAmountDisplay;
			break;
		default:
			return;
	}

	if (newParamID != dfx::kParameterID_Invalid)
	{
		if (slider)
		{
			slider->setParameterID(newParamID);
		}
		if (display)
		{
			display->setParameterID(newParamID);
		}
	}
}
