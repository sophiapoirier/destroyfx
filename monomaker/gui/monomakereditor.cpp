/*------------------------------------------------------------------------
Copyright (C) 2001-2022  Sophia Poirier

This file is part of Monomaker.

Monomaker is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Monomaker is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Monomaker.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#include "monomakereditor.h"

#include "monomaker.h"


//-----------------------------------------------------------------------------
enum
{
	kSliderX = 15,
	kSliderY = 81,
	kSliderInc = 61,
	kSliderWidth = 227,

	kDisplayX = 252,
	kDisplayY = 77,
	kDisplayWidth = 83,
	kDisplayHeight = 12,

	kMonomergeAnimationX = 14,
	kMonomergeAnimationY = 28,
	kPanAnimationX = 15,
	kPanAnimationY = 116,

	kPhaseInvertButtonX = 249 + 24,
	kPhaseInvertButtonY = 30,
	kPhaseInvertButtonIncX = 29,

	kModeButtonX = 21,
	kModeButtonY = 184,
	kModeButtonIncX = 110,

	kDestroyFXLinkX = 270,
	kDestroyFXLinkY = 3
};

constexpr char const* const kValueTextFont = "Arial";
constexpr float kValueTextSize = 12.0f;
constexpr float kUnusedControlAlpha = 0.36f;



//-----------------------------------------------------------------------------
// parameter value display text conversion functions

static bool monomergeDisplayProc(float inValue, char* outText, void*)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, " %.1f%%", inValue) > 0;
}

static bool panDisplayProc(float inValue, char* outText, void*)
{
	char const* const prefix = (inValue >= 0.0005f) ? "+" : "";
	return snprintf(outText, DGTextDisplay::kTextMaxLength, " %s%.1f%%", prefix, inValue * 100.0f) > 0;
}



//____________________________________________________________________________
DFX_EDITOR_ENTRY(MonomakerEditor)

//-----------------------------------------------------------------------------
MonomakerEditor::MonomakerEditor(DGEditorListenerInstance inInstance)
:	DfxGuiEditor(inInstance)
{
}

//-----------------------------------------------------------------------------
long MonomakerEditor::OpenEditor()
{
	//--load the images-------------------------------------

	auto const sliderHandleImage = LoadImage("slider-handle.png");
	auto const monomergeAnimationImage = LoadImage("monomerge-blobs.png");
	auto const panAnimationImage = LoadImage("pan-blobs.png");

	auto const phaseInvertButtonImage_left = LoadImage("phase-invert-left-button.png");
	auto const phaseInvertButtonImage_right = LoadImage("phase-invert-right-button.png");
	auto const inputSelectionButtonImage = LoadImage("input-selection-button.png");
	auto const monomergeModeButtonImage = LoadImage("monomerge-mode-button.png");
	auto const panModeButtonImage = LoadImage("pan-mode-button.png");
	auto const destroyFXLinkImage = LoadImage("destroy-fx-link.png");



	//--create the controls-------------------------------------
	DGRect pos;


	// --- sliders ---
	constexpr size_t numAnimationFrames = 19;

	// monomerge slider
	pos.set(kSliderX, kSliderY, kSliderWidth, sliderHandleImage->getHeight());
	emplaceControl<DGSlider>(this, kMonomerge, pos, dfx::kAxis_Horizontal, sliderHandleImage);

	// pan slider
	pos.offset(0, kSliderInc);
	emplaceControl<DGSlider>(this, kPan, pos, dfx::kAxis_Horizontal, sliderHandleImage);

	// monomerge animation
	pos.set(kMonomergeAnimationX, kMonomergeAnimationY, monomergeAnimationImage->getWidth(), monomergeAnimationImage->getHeight() / numAnimationFrames);
	auto blobs = emplaceControl<DGAnimation>(this, kMonomerge, pos, monomergeAnimationImage, numAnimationFrames);
	blobs->setMouseAxis(dfx::kAxis_Horizontal);

	// pan animation
	pos.set(kPanAnimationX, kPanAnimationY, panAnimationImage->getWidth(), panAnimationImage->getHeight() / numAnimationFrames);
	blobs = emplaceControl<DGAnimation>(this, kPan, pos, panAnimationImage, numAnimationFrames);
	blobs->setMouseAxis(dfx::kAxis_Horizontal);


	// --- text displays ---

	// mono merge
	pos.set(kDisplayX, kDisplayY, kDisplayWidth, kDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kMonomerge, pos, monomergeDisplayProc, nullptr, nullptr, dfx::TextAlignment::Center,
								  kValueTextSize, DGColor::kBlack, kValueTextFont);

	// pan
	pos.offset(0, kSliderInc);
	auto const panDisplay = emplaceControl<DGTextDisplay>(this, kPan, pos, panDisplayProc, nullptr, nullptr,
														  dfx::TextAlignment::Center, kValueTextSize,
														  DGColor::kBlack, kValueTextFont);
	panDisplay->setValueFromTextConvertProc(DGTextDisplay::valueFromTextConvertProc_PercentToLinear);


	// --- buttons ---

	// phase invert button (left)
	emplaceControl<DGToggleImageButton>(this, kPhaseInvert_LeftChannel, kPhaseInvertButtonX, kPhaseInvertButtonY, phaseInvertButtonImage_left)->setHelpText("invert left channel phase");

	// phase invert button (right)
	emplaceControl<DGToggleImageButton>(this, kPhaseInvert_RightChannel, kPhaseInvertButtonX + kPhaseInvertButtonIncX, kPhaseInvertButtonY, phaseInvertButtonImage_right)->setHelpText("invert right channel phase");

	// input selection button
	pos.set(kModeButtonX, kModeButtonY, inputSelectionButtonImage->getWidth(), inputSelectionButtonImage->getHeight() / kNumInputSelections);
	emplaceControl<DGButton>(this, kInputSelection, pos, inputSelectionButtonImage, DGButton::Mode::Increment);

	// monomerge mode button
	pos.offset(kModeButtonIncX, 0);
	pos.setSize(monomergeModeButtonImage->getWidth(), monomergeModeButtonImage->getHeight() / kNumMonomergeModes);
	emplaceControl<DGButton>(this, kMonomergeMode, pos, monomergeModeButtonImage, DGButton::Mode::Increment);

	// pan mode button
	pos.offset(kModeButtonIncX, 0);
	pos.setSize(panModeButtonImage->getWidth(), panModeButtonImage->getHeight() / kNumPanModes);
	emplaceControl<DGButton>(this, kPanMode, pos, panModeButtonImage, DGButton::Mode::Increment);

	// Destroy FX web page link
	pos.set(kDestroyFXLinkX, kDestroyFXLinkY, destroyFXLinkImage->getWidth(), destroyFXLinkImage->getHeight() / 2);
	emplaceControl<DGWebLink>(this, pos, destroyFXLinkImage, DESTROYFX_URL);


	inputChannelsChanged(getNumInputChannels());



	return dfx::kStatus_NoError;
}

//-----------------------------------------------------------------------------
void MonomakerEditor::inputChannelsChanged(size_t inChannelCount)
{
	float const alpha = (inChannelCount > 1) ? 1.f : kUnusedControlAlpha;
	SetParameterAlpha(kInputSelection, alpha);
	SetParameterAlpha(kMonomerge, alpha);
	SetParameterAlpha(kMonomergeMode, alpha);
	SetParameterAlpha(kPanMode, alpha);
}
