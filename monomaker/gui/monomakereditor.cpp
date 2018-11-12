/*------------------------------------------------------------------------
Copyright (C) 2001-2018  Sophia Poirier

This file is part of Monomaker.

Monomaker is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Monomaker is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Monomaker.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
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

	kButtonX = 21,
	kButtonY = 184,
	kButtonInc = 110,

	kDestroyFXlinkX = 270,
	kDestroyFXlinkY = 3
};

//constexpr DGColor kBackgroundColor(64, 54, 40);
//constexpr DGColor kBackgroundColor(42, 34, 22);
static char const* const kValueTextFont = "Arial";
constexpr float kValueTextSize = 11.0f;



//-----------------------------------------------------------------------------
// parameter value display text conversion functions

bool monomergeDisplayProc(float inValue, char* outText, void*);
bool monomergeDisplayProc(float inValue, char* outText, void*)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, " %.1f %%", inValue) > 0;
}

bool panDisplayProc(float inValue, char* outText, void*);
bool panDisplayProc(float inValue, char* outText, void*)
{
	char const* const prefix = (inValue >= 0.0005f) ? " +" : " ";
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%s%.3f", prefix, inValue) > 0;
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

	auto const sliderHandleImage = makeOwned<DGImage>("slider-handle.png");
	auto const monomergeAnimationImage = makeOwned<DGImage>("monomerge-blobs.png");
	auto const panAnimationImage = makeOwned<DGImage>("pan-blobs.png");

	auto const inputSelectionButtonImage = makeOwned<DGImage>("input-selection-button.png");
	auto const monomergeModeButtonImage = makeOwned<DGImage>("monomerge-mode-button.png");
	auto const panModeButtonImage = makeOwned<DGImage>("pan-mode-button.png");
	auto const destroyFXLinkImage = makeOwned<DGImage>("destroy-fx-link.png");



	//--create the controls-------------------------------------
	DGRect pos;


	// --- sliders ---
	constexpr long numAnimationFrames = 19;

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
	pos.offset(0, kSliderInc - 1);
	emplaceControl<DGTextDisplay>(this, kPan, pos, panDisplayProc, nullptr, nullptr, dfx::TextAlignment::Center, 
								  kValueTextSize, DGColor::kBlack, kValueTextFont);


	// --- buttons ---

	// input selection button
	pos.set(kButtonX, kButtonY, inputSelectionButtonImage->getWidth(), inputSelectionButtonImage->getHeight() / kNumInputSelections);
	emplaceControl<DGButton>(this, kInputSelection, pos, inputSelectionButtonImage, DGButton::Mode::Increment);

	// monomerge mode button
	pos.offset(kButtonInc, 0);
	pos.setSize(monomergeModeButtonImage->getWidth(), monomergeModeButtonImage->getHeight() / kNumMonomergeModes);
	emplaceControl<DGButton>(this, kMonomergeMode, pos, monomergeModeButtonImage, DGButton::Mode::Increment);

	// pan mode button
	pos.offset(kButtonInc, 0);
	pos.setSize(panModeButtonImage->getWidth(), panModeButtonImage->getHeight() / kNumPanModes);
	emplaceControl<DGButton>(this, kPanMode, pos, panModeButtonImage, DGButton::Mode::Increment);

	// Destroy FX web page link
	pos.set(kDestroyFXlinkX, kDestroyFXlinkY, destroyFXLinkImage->getWidth(), destroyFXLinkImage->getHeight() / 2);
	emplaceControl<DGWebLink>(this, pos, destroyFXLinkImage, DESTROYFX_URL);



	return dfx::kStatus_NoError;
}
