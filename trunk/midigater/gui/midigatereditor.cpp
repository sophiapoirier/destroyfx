/*------------------------------------------------------------------------
Copyright (C) 2001-2018  Sophia Poirier

This file is part of MIDI Gater.

MIDI Gater is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
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
#include "midigater.h"


//-----------------------------------------------------------------------------
enum
{
	// positions
	kSliderX = 13,
	kAttackSlopeSliderY = 37,
	kReleaseSlopeSliderY = 77,
	kVelocityInfluenceSliderY = 117,
	kFloorSliderY = 156,
	kSliderWidth = 289 - kSliderX,

	kDisplayX = 333+1,
	kAttackSlopeDisplayY = kAttackSlopeSliderY + 1,
	kReleaseSlopeDisplayY = kReleaseSlopeSliderY + 1,
	kVelocityInfluenceDisplayY = kVelocityInfluenceSliderY + 1,
	kFloorDisplayY = kFloorSliderY + 2,
	kDisplayWidth = 114,
	kDisplayWidthHalf = kDisplayWidth / 2,
	kDisplayHeight = 12,
	kVelocityInfluenceLabelWidth = kDisplayWidth - 33,

	kDestroyFXlinkX = 5,
	kDestroyFXlinkY = 7
};


static DGColor const kValueTextColor(152, 221, 251);
//static char const* const kValueTextFont = "Arial";
static char const* const kValueTextFont = "Trebuchet MS";
constexpr float kValueTextSize = 10.5f;


//-----------------------------------------------------------------------------
// parameter value display text conversion functions

bool slopeDisplayProc(float inValue, char* outText, void*);
bool slopeDisplayProc(float inValue, char* outText, void*)
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
	strlcat(outText, " ms", DGTextDisplay::kTextMaxLength);

	return success;
}

bool velocityInfluenceDisplayProc(float inValue, char* outText, void*);
bool velocityInfluenceDisplayProc(float inValue, char* outText, void*)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.1f%%", inValue * 100.0f) > 0;
}

bool floorDisplayProc(float inValue, char* outText, void*);
bool floorDisplayProc(float inValue, char* outText, void*)
{
	if (inValue <= 0.0f)
	{
		return snprintf(outText, DGTextDisplay::kTextMaxLength, u8"-%s dB", dfx::kInfinityUTF8.c_str()) > 0;
	}
	else
	{
		return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.1f dB", dfx::math::Linear2dB(inValue)) > 0;
	}
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

	auto const slopeSliderHandleImage = makeOwned<DGImage>("slider-handle-slope.png");
	auto const floorSliderHandleImage = makeOwned<DGImage>("slider-handle-floor.png");
	auto const velocityInfluenceSliderHandleImage = makeOwned<DGImage>("slider-handle-velocity-influence.png");
	auto const destroyFXLinkButtonImage = makeOwned<DGImage>("destroy-fx-link-button.png");


	//--create the controls-------------------------------------
	DGRect pos;

	// --- sliders ---

	// attack slope
	pos.set(kSliderX, kAttackSlopeSliderY, kSliderWidth, slopeSliderHandleImage->getHeight());
	emplaceControl<DGSlider>(this, kAttackSlope, pos, dfx::kAxis_Horizontal, slopeSliderHandleImage);

	// release slope
	pos.set(kSliderX, kReleaseSlopeSliderY, kSliderWidth, slopeSliderHandleImage->getHeight());
	emplaceControl<DGSlider>(this, kReleaseSlope, pos, dfx::kAxis_Horizontal, slopeSliderHandleImage);

	// velocity influence
	pos.set(kSliderX, kVelocityInfluenceSliderY, kSliderWidth, velocityInfluenceSliderHandleImage->getHeight());
	emplaceControl<DGSlider>(this, kVelocityInfluence, pos, dfx::kAxis_Horizontal, velocityInfluenceSliderHandleImage);

	// floor
	pos.set(kSliderX, kFloorSliderY, kSliderWidth, floorSliderHandleImage->getHeight());
	emplaceControl<DGSlider>(this, kFloor, pos, dfx::kAxis_Horizontal, floorSliderHandleImage);


	// --- text displays ---

	// attack slope
	pos.set(kDisplayX, kAttackSlopeDisplayY, kDisplayWidthHalf, kDisplayHeight);
	auto label = emplaceControl<DGStaticTextDisplay>(pos, nullptr, dfx::TextAlignment::Left, kValueTextSize, kValueTextColor, kValueTextFont);
	label->setText(getparametername(kAttackSlope));
	//
	pos.offset(kDisplayWidthHalf, 0);
	emplaceControl<DGTextDisplay>(this, kAttackSlope, pos, slopeDisplayProc, nullptr, nullptr, dfx::TextAlignment::Right, 
								  kValueTextSize, kValueTextColor, kValueTextFont);

	// release slope
	pos.set(kDisplayX, kReleaseSlopeDisplayY, kDisplayWidthHalf, kDisplayHeight);
	label = emplaceControl<DGStaticTextDisplay>(pos, nullptr, dfx::TextAlignment::Left, kValueTextSize, kValueTextColor, kValueTextFont);
	label->setText(getparametername(kReleaseSlope));
	//
	pos.offset(kDisplayWidthHalf, 0);
	emplaceControl<DGTextDisplay>(this, kReleaseSlope, pos, slopeDisplayProc, nullptr, nullptr, dfx::TextAlignment::Right, 
								  kValueTextSize, kValueTextColor, kValueTextFont);

	// velocity influence
	pos.set(kDisplayX - 1, kVelocityInfluenceDisplayY, kVelocityInfluenceLabelWidth + 1, kDisplayHeight);
	label = emplaceControl<DGStaticTextDisplay>(pos, nullptr, dfx::TextAlignment::Left, kValueTextSize - 0.48f, kValueTextColor, kValueTextFont);
	label->setText(getparametername(kVelocityInfluence));
	//
	pos.set(kDisplayX + kVelocityInfluenceLabelWidth, kVelocityInfluenceDisplayY, kDisplayWidth - kVelocityInfluenceLabelWidth, kDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kVelocityInfluence, pos, velocityInfluenceDisplayProc, nullptr, nullptr, 
								  dfx::TextAlignment::Right, kValueTextSize, kValueTextColor, kValueTextFont);

	// floor
	pos.set(kDisplayX, kFloorDisplayY, kDisplayWidthHalf, kDisplayHeight);
	label = emplaceControl<DGStaticTextDisplay>(pos, nullptr, dfx::TextAlignment::Left, kValueTextSize, kValueTextColor, kValueTextFont);
	label->setText(getparametername(kFloor));
	//
	pos.offset(kDisplayWidthHalf, 0);
	emplaceControl<DGTextDisplay>(this, kFloor, pos, floorDisplayProc, nullptr, nullptr, dfx::TextAlignment::Right, 
								  kValueTextSize, kValueTextColor, kValueTextFont);


	// --- buttons ---

	// Destroy FX web page link
	pos.set(kDestroyFXlinkX, kDestroyFXlinkY, destroyFXLinkButtonImage->getWidth(), destroyFXLinkButtonImage->getHeight() / 2);
	emplaceControl<DGWebLink>(this, pos, destroyFXLinkButtonImage, DESTROYFX_URL);



	return dfx::kStatus_NoError;
}
