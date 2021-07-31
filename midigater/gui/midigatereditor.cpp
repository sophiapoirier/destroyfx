/*------------------------------------------------------------------------
Copyright (C) 2001-2021  Sophia Poirier

This file is part of MIDI Gater.

MIDI Gater is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

MIDI Gater is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with MIDI Gater.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "midigatereditor.h"

#include <cmath>

#include "dfxmath.h"
#include "dfxmisc.h"
#include "midigater.h"


//-----------------------------------------------------------------------------
enum
{
	// positions
	kSliderX = 13,
	kAttackSliderY = 38,
	kReleaseSliderY = 80,
	kVelocityInfluenceSliderY = 122,
	kFloorSliderY = 164,
	kSliderWidth = 289 - kSliderX,

	kDisplayX = 334,
	kAttackDisplayY = kAttackSliderY + 1,
	kReleaseDisplayY = kReleaseSliderY + 1,
	kVelocityInfluenceDisplayY = kVelocityInfluenceSliderY + 1,
	kFloorDisplayY = kFloorSliderY + 1,
	kDisplayWidth = 134,
	kDisplayWidthHalf = kDisplayWidth / 2,
	kDisplayHeight = 12,
	kVelocityInfluenceLabelWidth = kDisplayWidthHalf + 18,

	kButtonY = 195,
	kGateModeButtonX = 324,
	kMidiLearnButtonX = kSliderX - 1,
	kMidiResetButtonX = kMidiLearnButtonX + 125,

	kDestroyFXLinkX = 5,
	kDestroyFXLinkY = 7
};


constexpr DGColor kValueTextColor(152, 221, 251);
//constexpr char const* const kValueTextFont = "Arial";
constexpr char const* const kValueTextFont = "Trebuchet MS";
constexpr float kValueTextSize = 11.0f;


//-----------------------------------------------------------------------------
// parameter value display text conversion functions

struct bool envelopeDisplayProc(float inValue, char* outText, void*)
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



//____________________________________________________________________________
DFX_EDITOR_ENTRY(MIDIGaterEditor)

//-----------------------------------------------------------------------------
MIDIGaterEditor::MIDIGaterEditor(DGEditorListenerInstance inInstance)
:	DfxGuiEditor(inInstance)
{
}

//-----------------------------------------------------------------------------
long MIDIGaterEditor::OpenEditor()
{
	//--load the images-------------------------------------

	auto const envelopeSliderHandleImage = LoadImage("slider-handle-slope.png");
	auto const envelopeSliderHandleImage_glowing = LoadImage("slider-handle-slope-glowing.png");
	auto const floorSliderHandleImage = LoadImage("slider-handle-floor.png");
	auto const floorSliderHandleImage_glowing = LoadImage("slider-handle-floor-glowing.png");
	auto const velocityInfluenceSliderHandleImage = LoadImage("slider-handle-velocity-influence.png");
	auto const velocityInfluenceSliderHandleImage_glowing = LoadImage("slider-handle-velocity-influence-glowing.png");
	auto const gateModeButtonImage = LoadImage("gate-mode-button.png");
	auto const midiLearnButtonImage = LoadImage("midi-learn-button.png");
	auto const midiResetButtonImage = LoadImage("midi-reset-button.png");
	auto const destroyFXLinkButtonImage = LoadImage("destroy-fx-link-button.png");


	//--create the controls-------------------------------------
	DGRect pos;

	// --- sliders ---

	// attack duration
	pos.set(kSliderX, kAttackSliderY, kSliderWidth, envelopeSliderHandleImage->getHeight());
	emplaceControl<DGSlider>(this, kAttack, pos, dfx::kAxis_Horizontal, envelopeSliderHandleImage)->setAlternateHandle(envelopeSliderHandleImage_glowing);

	// release duration
	pos.set(kSliderX, kReleaseSliderY, kSliderWidth, envelopeSliderHandleImage->getHeight());
	emplaceControl<DGSlider>(this, kRelease, pos, dfx::kAxis_Horizontal, envelopeSliderHandleImage)->setAlternateHandle(envelopeSliderHandleImage_glowing);

	// velocity influence
	pos.set(kSliderX, kVelocityInfluenceSliderY, kSliderWidth, velocityInfluenceSliderHandleImage->getHeight());
	emplaceControl<DGSlider>(this, kVelocityInfluence, pos, dfx::kAxis_Horizontal, velocityInfluenceSliderHandleImage)->setAlternateHandle(velocityInfluenceSliderHandleImage_glowing);

	// floor
	pos.set(kSliderX, kFloorSliderY, kSliderWidth, floorSliderHandleImage->getHeight());
	emplaceControl<DGSlider>(this, kFloor, pos, dfx::kAxis_Horizontal, floorSliderHandleImage)->setAlternateHandle(floorSliderHandleImage_glowing);


	// --- text displays ---

	// attack duration
	pos.set(kDisplayX, kAttackDisplayY, kDisplayWidthHalf, kDisplayHeight);
	auto label = emplaceControl<DGStaticTextDisplay>(this, pos, nullptr, dfx::TextAlignment::Left, kValueTextSize, kValueTextColor, kValueTextFont);
	label->setText(getparametername(kAttack));
	//
	pos.offset(kDisplayWidthHalf, 0);
	emplaceControl<DGTextDisplay>(this, kAttack, pos, envelopeDisplayProc, nullptr, nullptr, dfx::TextAlignment::Right,
								  kValueTextSize, kValueTextColor, kValueTextFont);

	// release duration
	pos.set(kDisplayX, kReleaseDisplayY, kDisplayWidthHalf, kDisplayHeight);
	label = emplaceControl<DGStaticTextDisplay>(this, pos, nullptr, dfx::TextAlignment::Left, kValueTextSize, kValueTextColor, kValueTextFont);
	label->setText(getparametername(kRelease));
	//
	pos.offset(kDisplayWidthHalf, 0);
	emplaceControl<DGTextDisplay>(this, kRelease, pos, envelopeDisplayProc, nullptr, nullptr, dfx::TextAlignment::Right,
								  kValueTextSize, kValueTextColor, kValueTextFont);

	// velocity influence
	pos.set(kDisplayX - 1, kVelocityInfluenceDisplayY, kVelocityInfluenceLabelWidth + 1, kDisplayHeight);
	label = emplaceControl<DGStaticTextDisplay>(this, pos, nullptr, dfx::TextAlignment::Left, kValueTextSize - 0.48f, kValueTextColor, kValueTextFont);
	label->setText(getparametername(kVelocityInfluence));
	//
	pos.set(kDisplayX + kVelocityInfluenceLabelWidth, kVelocityInfluenceDisplayY, kDisplayWidth - kVelocityInfluenceLabelWidth, kDisplayHeight);
	auto textDisplay = emplaceControl<DGTextDisplay>(this, kVelocityInfluence, pos,
													 DGTextDisplay::valueToTextProc_LinearToPercent, nullptr,
													 nullptr, dfx::TextAlignment::Right,
													 kValueTextSize, kValueTextColor, kValueTextFont);
	textDisplay->setValueFromTextConvertProc(DGTextDisplay::valueFromTextConvertProc_PercentToLinear);

	// floor
	pos.set(kDisplayX, kFloorDisplayY, kDisplayWidthHalf, kDisplayHeight);
	label = emplaceControl<DGStaticTextDisplay>(this, pos, nullptr, dfx::TextAlignment::Left, kValueTextSize, kValueTextColor, kValueTextFont);
	label->setText(getparametername(kFloor));
	//
	pos.offset(kDisplayWidthHalf, 0);
	textDisplay = emplaceControl<DGTextDisplay>(this, kFloor, pos, DGTextDisplay::valueToTextProc_LinearToDb, nullptr, nullptr,
												dfx::TextAlignment::Right, kValueTextSize, kValueTextColor, kValueTextFont);
	textDisplay->setTextToValueProc(DGTextDisplay::textToValueProc_DbToLinear);


	// --- buttons ---

	pos.set(kGateModeButtonX, kButtonY, gateModeButtonImage->getWidth(), gateModeButtonImage->getHeight() / MIDIGater::kNumGateModes);
	emplaceControl<DGButton>(this, kGateMode, pos, gateModeButtonImage, DGButton::Mode::Increment);

	CreateMidiLearnButton(kMidiLearnButtonX, kButtonY, midiLearnButtonImage);

	CreateMidiResetButton(kMidiResetButtonX, kButtonY, midiResetButtonImage);

	// Destroy FX web page link
	pos.set(kDestroyFXLinkX, kDestroyFXLinkY, destroyFXLinkButtonImage->getWidth(), destroyFXLinkButtonImage->getHeight() / 2);
	emplaceControl<DGWebLink>(this, pos, destroyFXLinkButtonImage, DESTROYFX_URL);



	return dfx::kStatus_NoError;
}
