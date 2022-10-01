/*------------------------------------------------------------------------
Copyright (C) 2002-2022  Sophia Poirier

This file is part of Scrubby.

Scrubby is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Scrubby is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Scrubby.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#include "scrubbyeditor.h"

#include <algorithm>
#include <cassert>

#include "dfxmisc.h"
#include "scrubby.h"


//-----------------------------------------------------------------------------
constexpr auto kValueDisplayFont = dfx::kFontName_Snooty10px;
constexpr auto kValueDisplayFontSize = dfx::kFontSize_Snooty10px;
constexpr DGColor kValueDisplayFontColor(187, 173, 131);
constexpr float kUnusedControlAlpha = 0.234f;
constexpr int kRangeSliderOvershoot = 3;

constexpr int kOctavesSliderWidth = 118 - 2;
// TODO C++23: constexpr
static int const kOctaveMaxSliderWidth = static_cast<int>((static_cast<float>(kOctave_MaxValue) / static_cast<float>((std::abs(kOctave_MinValue) + kOctave_MaxValue))) * static_cast<float>(kOctavesSliderWidth));
static int const kOctaveMinSliderWidth = kOctavesSliderWidth - kOctaveMaxSliderWidth;
constexpr int kOctaveMinSliderX = 251 + 1;
static int const kOctaveMaxSliderX = kOctaveMinSliderX + kOctaveMinSliderWidth;

//-----------------------------------------------------------------------------
enum
{
	// positions
	kSliderHeight = 28,

	kSeekRateSliderX = 33 + 1,
	kSeekRateSliderY = 165,
	kSeekRateSliderWidth = 290 - 2,

	kSeekRangeSliderX = 33 + 1,
	kSeekRangeSliderY = 208,
	kSeekRangeSliderWidth = 336 - 2,

	kSeekDurSliderX = 33 + 1,
	kSeekDurSliderY = 251,
	kSeekDurSliderWidth = 336 - 2,

	kOctaveMinSliderY = 384,
	kOctaveMaxSliderY = kOctaveMinSliderY,

	kDryLevelSliderX = 33 + 1,
	kDryLevelSliderY = kOctaveMinSliderY,
	kWetLevelSliderX = kDryLevelSliderX,
	kWetLevelSliderY = kDryLevelSliderY + 43,
	kMixLevelSliderWidth = 150 - 2,

	kTempoSliderX = 33 + 1,
	kTempoSliderY = 470,
	kTempoSliderWidth = 194 - 2,

	kPredelaySliderX = kOctaveMinSliderX,
	kPredelaySliderY = kOctaveMinSliderY + 43,
	kPredelaySliderWidth = kOctavesSliderWidth,

	kDisplayWidth = 90,
	kDisplayWidth_big = 117,  // for seek rate
	kDisplayHeight = dfx::kFontContainerHeight_Snooty10px,
	kDisplayInsetX = 0,
	kDisplayInsetX_leftAlign = 2,
	kDisplayInsetY = 0,

	kSpeedModeButtonX = 31,
	kSpeedModeButtonY = 60,
	kFreezeButtonX = 127,
	kFreezeButtonY = 60,
	kSplitChannelsButtonX = 223,
	kSplitChannelsButtonY = 60,
	kTempoSyncButtonX = 319,
	kTempoSyncButtonY = 60,
	kPitchConstraintButtonX = 415,
	kPitchConstraintButtonY = 60,

	kLittleTempoSyncButtonX = 327,
	kLittleTempoSyncButtonY = 170,

	kTempoAutoButtonX = 231,
	kTempoAutoButtonY = 475,

	kKeyboardX = 33,
	kKeyboardY = 292,

	kTransposeDownButtonX = 263,
	kTransposeDownButtonY = 292,
	kTransposeUpButtonX = kTransposeDownButtonX,
	kTransposeUpButtonY = kTransposeDownButtonY + 46,

	kMajorChordButtonX = 299,
	kMajorChordButtonY = 292,
	kMinorChordButtonX = kMajorChordButtonX,
	kMinorChordButtonY = kMajorChordButtonY + 19,
	kAllNotesButtonX = kMajorChordButtonX,
	kAllNotesButtonY = kMajorChordButtonY + 42,
	kNoneNotesButtonX = kMajorChordButtonX,
	kNoneNotesButtonY = kMajorChordButtonY + 61,

	kMidiLearnButtonX = 433,
	kMidiLearnButtonY = 463,
	kMidiResetButtonX = kMidiLearnButtonX,
	kMidiResetButtonY = kMidiLearnButtonY + 19,

	kHelpX = 4,
	kHelpY = 508,

	kDestroyFXLinkX = 247,
	kDestroyFXLinkY = 47,

	kTitleAreaX = 125,
	kTitleAreaY = 8,
	kTitleAreaWidth = 260,
	kTitleAreaHeight = 37
};

enum : size_t
{
	kNotes_Down = 0,
	kNotes_Up,
	kNotes_Major,
	kNotes_Minor,
	kNotes_All,
	kNotes_None,
	kNumNotesButtons
};



//-----------------------------------------------------------------------------
// parameter value string display conversion functions

static bool seekRangeDisplayProc(float inValue, char* outText, void*)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.1f ms", inValue) > 0;
}

static bool seekRateGenDisplayProc(float inValue, dfx::ParameterID inParameterID, char* outText, DfxGuiEditor* inEditor)
{
	if (inEditor->getparameter_b(kTempoSync))
	{
		if (auto const valueString = inEditor->getparametervaluestring(inParameterID)) //&& (valueString->length() <= 3)
		{
			return snprintf(outText, DGTextDisplay::kTextMaxLength, "%s cycles/beat", valueString->c_str()) > 0;
		}
	}
	else
	{
		return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.3f Hz", inValue) > 0;
	}
	return false;
}

static bool seekRateDisplayProc(float inValue, char* outText, void* inEditor)
{
	return seekRateGenDisplayProc(inValue, kSeekRate_Sync, outText, static_cast<DfxGuiEditor*>(inEditor));
}

static bool seekRateRandMinDisplayProc(float inValue, char* outText, void* inEditor)
{
	return seekRateGenDisplayProc(inValue, kSeekRateRandMin_Sync, outText, static_cast<DfxGuiEditor*>(inEditor));
}

static bool octaveMinDisplayProc(float inValue, char* outText, void*)
{
	auto const octaves = static_cast<long>(inValue);
	if (octaves <= kOctave_MinValue)
	{
		return dfx::StrLCpy(outText, "no min", DGTextDisplay::kTextMaxLength) > 0;
	}
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%ld", octaves) > 0;
}

static bool octaveMaxDisplayProc(float inValue, char* outText, void*)
{
	auto const octaves = static_cast<long>(inValue);
	if (octaves >= kOctave_MaxValue)
	{
		return dfx::StrLCpy(outText, "no max", DGTextDisplay::kTextMaxLength) > 0;
	}
	if (octaves == 0)
	{
		return snprintf(outText, DGTextDisplay::kTextMaxLength, "0") > 0;
	}
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%+ld", octaves) > 0;
}

static bool tempoDisplayProc(float inValue, char* outText, void*)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.3f bpm", inValue) > 0;
}

static bool predelayDisplayProc(float inValue, char* outText, void*)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.0f%%", inValue) > 0;
}



#pragma mark -

//-----------------------------------------------------------------------------
DFX_EDITOR_ENTRY(ScrubbyEditor)

//-----------------------------------------------------------------------------
ScrubbyEditor::ScrubbyEditor(DGEditorListenerInstance inInstance)
:	DfxGuiEditor(inInstance)
{
}

//-----------------------------------------------------------------------------
long ScrubbyEditor::OpenEditor()
{
	// create images

	// sliders
	auto const sliderHandleImage = LoadImage("slider-handle.png");
	auto const sliderHandleImage_glowing = LoadImage("slider-handle-glowing.png");
	auto const rangeSliderHandleLeftImage = LoadImage("range-slider-handle-left.png");
	auto const rangeSliderHandleLeftImage_glowing = LoadImage("range-slider-handle-left-glowing.png");
	auto const rangeSliderHandleRightImage = LoadImage("range-slider-handle-right.png");
	auto const rangeSliderHandleRightImage_glowing = LoadImage("range-slider-handle-right-glowing.png");

	// mode buttons
	auto const speedModeButtonImage = LoadImage("speed-mode-button.png");
	auto const freezeButtonImage = LoadImage("freeze-button.png");
	auto const tempoSyncButtonImage = LoadImage("tempo-sync-button.png");
	auto const splitChannelsButtonImage = LoadImage("stereo-button.png");
	auto const pitchConstraintButtonImage = LoadImage("pitch-constraint-button.png");
	auto const tempoSyncButtonImage_little = LoadImage("tempo-sync-button-little.png");
	auto const hostTempoButtonImage = LoadImage("host-tempo-button.png");

	// pitch constraint control buttons
//	auto const keyboardOffImage = LoadImage("keyboard-off.png");
//	auto const keyboardOnImage = LoadImage("keyboard-on.png");
	//
	VSTGUI::SharedPointer<DGImage> keyboardTopKeyImages[kNumPitchSteps];
	keyboardTopKeyImages[1] = keyboardTopKeyImages[3] = keyboardTopKeyImages[6] = keyboardTopKeyImages[8] = keyboardTopKeyImages[10] = LoadImage("keyboard-black-key.png");
	keyboardTopKeyImages[0] = LoadImage("keyboard-white-key-top-1.png");
	keyboardTopKeyImages[2] = LoadImage("keyboard-white-key-top-2.png");
	keyboardTopKeyImages[4] = LoadImage("keyboard-white-key-top-3.png");
	keyboardTopKeyImages[5] = LoadImage("keyboard-white-key-top-4.png");
	keyboardTopKeyImages[7] = keyboardTopKeyImages[9] = LoadImage("keyboard-white-key-top-5-6.png");
	keyboardTopKeyImages[11] = LoadImage("keyboard-white-key-top-7.png");
	//
	VSTGUI::SharedPointer<DGImage> keyboardBottomKeyImages[kNumPitchSteps];
	keyboardBottomKeyImages[0] = LoadImage("keyboard-white-key-bottom-left.png");
	keyboardBottomKeyImages[2] = keyboardBottomKeyImages[4] = keyboardBottomKeyImages[5] = keyboardBottomKeyImages[7] = keyboardBottomKeyImages[9] = LoadImage("keyboard-white-key-bottom.png");
	keyboardBottomKeyImages[kNumPitchSteps - 1] = LoadImage("keyboard-white-key-bottom-right.png");
	//
	auto const transposeDownButtonImage = LoadImage("transpose-down-button.png");
	auto const transposeUpButtonImage = LoadImage("transpose-up-button.png");
	auto const majorChordButtonImage = LoadImage("major-chord-button.png");
	auto const minorChordButtonImage = LoadImage("minor-chord-button.png");
	auto const allNotesButtonImage = LoadImage("all-notes-button.png");
	auto const noneNotesButtonImage = LoadImage("none-notes-button.png");

	// help box
	auto const helpBackgroundImage = LoadImage("help-background.png");

	// other buttons
	auto const midiLearnButtonImage = LoadImage("midi-learn-button.png");
	auto const midiResetButtonImage = LoadImage("midi-reset-button.png");
	auto const destroyFXLinkImage = LoadImage("destroy-fx-link.png");



	DGRect pos;
	auto const seekRateParameterID = getparameter_b(kTempoSync) ? kSeekRate_Sync : kSeekRate_Hz;
	auto const seekRateRandMinParameterID = getparameter_b(kTempoSync) ? kSeekRateRandMin_Sync : kSeekRateRandMin_Hz;

	//--create the sliders-----------------------------------------------

	// seek rate
	pos.set(kSeekRateSliderX, kSeekRateSliderY, kSeekRateSliderWidth, kSliderHeight);
	mSeekRateSlider = emplaceControl<DGRangeSlider>(this, seekRateRandMinParameterID, seekRateParameterID, pos,
													rangeSliderHandleLeftImage, rangeSliderHandleRightImage, nullptr,
													DGRangeSlider::PushStyle::Upper, kRangeSliderOvershoot);
	mSeekRateSlider->setAlternateHandles(rangeSliderHandleLeftImage_glowing, rangeSliderHandleRightImage_glowing);

	// seek range
	pos.set(kSeekRangeSliderX, kSeekRangeSliderY, kSeekRangeSliderWidth, kSliderHeight);
	emplaceControl<DGSlider>(this, kSeekRange, pos, dfx::kAxis_Horizontal, sliderHandleImage)->setAlternateHandle(sliderHandleImage_glowing);

	// seek duration
	pos.set(kSeekDurSliderX, kSeekDurSliderY, kSeekDurSliderWidth, kSliderHeight);
	emplaceControl<DGRangeSlider>(this, kSeekDurRandMin, kSeekDur, pos,
								  rangeSliderHandleLeftImage, rangeSliderHandleRightImage, nullptr,
								  DGRangeSlider::PushStyle::Upper,
								  kRangeSliderOvershoot)->setAlternateHandles(rangeSliderHandleLeftImage_glowing,
																			  rangeSliderHandleRightImage_glowing);

	// octave minimum
	pos.set(kOctaveMinSliderX, kOctaveMinSliderY, kOctaveMinSliderWidth, kSliderHeight);
	emplaceControl<DGSlider>(this, kOctaveMin, pos, dfx::kAxis_Horizontal, rangeSliderHandleLeftImage)->setAlternateHandle(rangeSliderHandleLeftImage_glowing);

	// octave maximum
	pos.set(kOctaveMaxSliderX, kOctaveMaxSliderY, kOctaveMaxSliderWidth, kSliderHeight);
	emplaceControl<DGSlider>(this, kOctaveMax, pos, dfx::kAxis_Horizontal, rangeSliderHandleRightImage)->setAlternateHandle(rangeSliderHandleRightImage_glowing);

	// tempo
	pos.set(kTempoSliderX, kTempoSliderY, kTempoSliderWidth, kSliderHeight);
	emplaceControl<DGSlider>(this, kTempo, pos, dfx::kAxis_Horizontal, sliderHandleImage)->setAlternateHandle(sliderHandleImage_glowing);

	// predelay
	pos.set(kPredelaySliderX, kPredelaySliderY, kPredelaySliderWidth, kSliderHeight);
	emplaceControl<DGSlider>(this, kPredelay, pos, dfx::kAxis_Horizontal, sliderHandleImage)->setAlternateHandle(sliderHandleImage_glowing);

	// dry level
	pos.set(kDryLevelSliderX, kDryLevelSliderY, kMixLevelSliderWidth, kSliderHeight);
	emplaceControl<DGSlider>(this, kDryLevel, pos, dfx::kAxis_Horizontal, sliderHandleImage)->setAlternateHandle(sliderHandleImage_glowing);

	// wet level
	pos.set(kWetLevelSliderX, kWetLevelSliderY, kMixLevelSliderWidth, kSliderHeight);
	emplaceControl<DGSlider>(this, kWetLevel, pos, dfx::kAxis_Horizontal, sliderHandleImage)->setAlternateHandle(sliderHandleImage_glowing);



	//--create the displays---------------------------------------------

	// seek rate random minimum
	pos.set(kSeekRateSliderX + kDisplayInsetX_leftAlign, kSeekRateSliderY - kDisplayHeight + kDisplayInsetY, kDisplayWidth_big, kDisplayHeight);
	mSeekRateRandMinDisplay = emplaceControl<DGTextDisplay>(this, seekRateRandMinParameterID, pos, seekRateRandMinDisplayProc, this, nullptr, dfx::TextAlignment::Left, kValueDisplayFontSize, kValueDisplayFontColor, kValueDisplayFont);

	// seek rate
	pos.set(kSeekRateSliderX + kSeekRateSliderWidth - kDisplayWidth_big - kDisplayInsetX, kSeekRateSliderY - kDisplayHeight + kDisplayInsetY, kDisplayWidth_big, kDisplayHeight);
	mSeekRateDisplay = emplaceControl<DGTextDisplay>(this, seekRateParameterID, pos, seekRateDisplayProc, this, nullptr, dfx::TextAlignment::Right, kValueDisplayFontSize, kValueDisplayFontColor, kValueDisplayFont);

	// seek range
	pos.set(kSeekRangeSliderX + kSeekRangeSliderWidth - kDisplayWidth - kDisplayInsetX, kSeekRangeSliderY - kDisplayHeight + kDisplayInsetY, kDisplayWidth, kDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kSeekRange, pos, seekRangeDisplayProc, nullptr, nullptr, dfx::TextAlignment::Right, kValueDisplayFontSize, kValueDisplayFontColor, kValueDisplayFont);

	// seek duration random minimum
	pos.set(kSeekDurSliderX + kDisplayInsetX_leftAlign, kSeekDurSliderY - kDisplayHeight + kDisplayInsetY, kDisplayWidth, kDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kSeekDurRandMin, pos, DGTextDisplay::valueToTextProc_Percent, nullptr, nullptr, dfx::TextAlignment::Left, kValueDisplayFontSize, kValueDisplayFontColor, kValueDisplayFont);

	// seek duration
	pos.set(kSeekDurSliderX + kSeekDurSliderWidth - kDisplayWidth - kDisplayInsetX, kSeekDurSliderY - kDisplayHeight + kDisplayInsetY, kDisplayWidth, kDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kSeekDur, pos, DGTextDisplay::valueToTextProc_Percent, nullptr, nullptr, dfx::TextAlignment::Right, kValueDisplayFontSize, kValueDisplayFontColor, kValueDisplayFont);

	// octave mininum
	pos.set(kOctaveMinSliderX + kDisplayInsetX_leftAlign, kOctaveMinSliderY - kDisplayHeight + kDisplayInsetY, kDisplayWidth, kDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kOctaveMin, pos, octaveMinDisplayProc, nullptr, nullptr, dfx::TextAlignment::Left, kValueDisplayFontSize, kValueDisplayFontColor, kValueDisplayFont);

	// octave maximum
	pos.set(kOctaveMaxSliderX + kOctaveMaxSliderWidth - kDisplayWidth - kDisplayInsetX, kOctaveMaxSliderY - kDisplayHeight + kDisplayInsetY, kDisplayWidth, kDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kOctaveMax, pos, octaveMaxDisplayProc, nullptr, nullptr, dfx::TextAlignment::Right, kValueDisplayFontSize, kValueDisplayFontColor, kValueDisplayFont);

	// tempo
	pos.set(kTempoSliderX + kDisplayInsetX_leftAlign, kTempoSliderY - kDisplayHeight + kDisplayInsetY, kDisplayWidth, kDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kTempo, pos, tempoDisplayProc, nullptr, nullptr, dfx::TextAlignment::Left, kValueDisplayFontSize, kValueDisplayFontColor, kValueDisplayFont);

	// predelay
	pos.set(kPredelaySliderX + kPredelaySliderWidth - kDisplayWidth - kDisplayInsetX, kPredelaySliderY - kDisplayHeight + kDisplayInsetY, kDisplayWidth, kDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kPredelay, pos, predelayDisplayProc, nullptr, nullptr, dfx::TextAlignment::Right, kValueDisplayFontSize, kValueDisplayFontColor, kValueDisplayFont);

	// dry level
	pos.set(kDryLevelSliderX + kMixLevelSliderWidth - kDisplayWidth - kDisplayInsetX, kDryLevelSliderY - kDisplayHeight + kDisplayInsetY, kDisplayWidth, kDisplayHeight);
	auto textDisplay = emplaceControl<DGTextDisplay>(this, kDryLevel, pos, DGTextDisplay::valueToTextProc_LinearToDb, nullptr, nullptr, dfx::TextAlignment::Right, kValueDisplayFontSize, kValueDisplayFontColor, kValueDisplayFont);
	textDisplay->setTextToValueProc(DGTextDisplay::textToValueProc_DbToLinear);

	// wet level
	pos.set(kWetLevelSliderX + kMixLevelSliderWidth - kDisplayWidth - kDisplayInsetX, kWetLevelSliderY - kDisplayHeight + kDisplayInsetY, kDisplayWidth, kDisplayHeight);
	textDisplay = emplaceControl<DGTextDisplay>(this, kWetLevel, pos, DGTextDisplay::valueToTextProc_LinearToDb, nullptr, nullptr, dfx::TextAlignment::Right, kValueDisplayFontSize, kValueDisplayFontColor, kValueDisplayFont);
	textDisplay->setTextToValueProc(DGTextDisplay::textToValueProc_DbToLinear);



	//--create the keyboard---------------------------------------------
//	pos.set(kKeyboardX, kKeyboardY, keyboardOffImage->getWidth(), keyboardOffImage->getHeight());
//	keyboard = emplaceControl<ScrubbyKeyboard>(this, kPitchStep0, pos, keyboardOffImage, keyboardOnImage, 24, 48, 18, 56, 114, 149, 184);
	pos.set(kKeyboardX, kKeyboardY, keyboardTopKeyImages[0]->getWidth(), keyboardTopKeyImages[0]->getHeight() / 2);
	DGRect keyboardBottomKeyPos(kKeyboardX, kKeyboardY + pos.getHeight(), keyboardBottomKeyImages[0]->getWidth(), keyboardBottomKeyImages[0]->getHeight() / 2);
	for (size_t i = 0; i < kNumPitchSteps; i++)
	{
		dfx::ParameterID const parameterID = kPitchStep0 + i;

		// this visually syncs the top and bottom button images upon mouse clicks
		auto const keyboardButtonProc = [](DGButton* button, long value)
		{
			auto const editor = button->getOwnerEditor();
			editor->getFrame()->forEachChild([originalButton = button->asCControl()](VSTGUI::CView* child)
			{
				auto const parameterIndex = originalButton->getTag();
				auto const childControl = dynamic_cast<VSTGUI::CControl*>(child);
				if (childControl && (childControl->getTag() == parameterIndex) && (childControl != originalButton))
				{
					childControl->setValue(originalButton->getValue());
					if (childControl->isDirty())
					{
						childControl->invalid();
					}
				}
			});
		};

		pos.setWidth(keyboardTopKeyImages[i]->getWidth());
		auto button = emplaceControl<DGButton>(this, parameterID, pos, keyboardTopKeyImages[i], DGButton::Mode::Increment);
		pos.offset(pos.getWidth(), 0);

		if (keyboardBottomKeyImages[i] != nullptr)
		{
			button->setUserProcedure(std::bind_front(keyboardButtonProc, button));

			keyboardBottomKeyPos.setWidth(keyboardBottomKeyImages[i]->getWidth());
			button = emplaceControl<DGButton>(this, parameterID, keyboardBottomKeyPos, keyboardBottomKeyImages[i], DGButton::Mode::Increment);
			button->setUserProcedure(std::bind_front(keyboardButtonProc, button));
			keyboardBottomKeyPos.offset(keyboardBottomKeyPos.getWidth(), 0);
		}
	}



	//--create the buttons----------------------------------------------

	// ...................MODES...........................

	// choose the speed mode (robot or DJ)
	pos.set(kSpeedModeButtonX, kSpeedModeButtonY, speedModeButtonImage->getWidth() / 2, speedModeButtonImage->getHeight() / kNumSpeedModes);
	emplaceControl<DGButton>(this, kSpeedMode, pos, speedModeButtonImage, DGButton::Mode::Increment, true);

	// freeze the input stream
	emplaceControl<DGToggleImageButton>(this, kFreeze, kFreezeButtonX, kFreezeButtonY, freezeButtonImage, true);

	// choose the channels mode (linked or split)
	emplaceControl<DGToggleImageButton>(this, kSplitChannels, kSplitChannelsButtonX, kSplitChannelsButtonY, splitChannelsButtonImage, true);

	// choose the seek rate type ("free" or synced)
	emplaceControl<DGToggleImageButton>(this, kTempoSync, kTempoSyncButtonX, kTempoSyncButtonY, tempoSyncButtonImage, true);
	emplaceControl<DGToggleImageButton>(this, kTempoSync, kLittleTempoSyncButtonX, kLittleTempoSyncButtonY, tempoSyncButtonImage_little, false);

	// toggle pitch constraint
	emplaceControl<DGToggleImageButton>(this, kPitchConstraint, kPitchConstraintButtonX, kPitchConstraintButtonY, pitchConstraintButtonImage, true);

	// enable sync to host tempo
	emplaceControl<DGToggleImageButton>(this, kTempoAuto, kTempoAutoButtonX, kTempoAutoButtonY, hostTempoButtonImage, false);


	// ...............PITCH CONSTRAINT....................

	auto const keyboardMacroProc = [this](size_t notesButtonType, long controlValue)
	{
		if (controlValue != 0)
		{
			HandleNotesButton(notesButtonType);
		}
	};
	mNotesButtons.assign(kNumNotesButtons, nullptr);

	// transpose all of the pitch constraint notes down one semitone
	pos.set(kTransposeDownButtonX, kTransposeDownButtonY, transposeDownButtonImage->getWidth(), transposeDownButtonImage->getHeight() / 2);
	mNotesButtons[kNotes_Down] = emplaceControl<DGButton>(this, pos, transposeDownButtonImage, 2, DGButton::Mode::Momentary);
	mNotesButtons[kNotes_Down]->setUserProcedure(std::bind_front(keyboardMacroProc, kNotes_Down));

	// transpose all of the pitch constraint notes up one semitone
	pos.set(kTransposeUpButtonX, kTransposeUpButtonY, transposeUpButtonImage->getWidth(), transposeUpButtonImage->getHeight() / 2);
	mNotesButtons[kNotes_Up] = emplaceControl<DGButton>(this, pos, transposeUpButtonImage, 2, DGButton::Mode::Momentary);
	mNotesButtons[kNotes_Up]->setUserProcedure(std::bind_front(keyboardMacroProc, kNotes_Up));

	// turn on the pitch constraint notes that form a major chord
	pos.set(kMajorChordButtonX, kMajorChordButtonY, majorChordButtonImage->getWidth(), majorChordButtonImage->getHeight() / 2);
	mNotesButtons[kNotes_Major] = emplaceControl<DGButton>(this, pos, majorChordButtonImage, 2, DGButton::Mode::Momentary);
	mNotesButtons[kNotes_Major]->setUserProcedure(std::bind_front(keyboardMacroProc, kNotes_Major));

	// turn on the pitch constraint notes that form a minor chord
	pos.set(kMinorChordButtonX, kMinorChordButtonY, minorChordButtonImage->getWidth(), minorChordButtonImage->getHeight() / 2);
	mNotesButtons[kNotes_Minor] = emplaceControl<DGButton>(this, pos, minorChordButtonImage, 2, DGButton::Mode::Momentary);
	mNotesButtons[kNotes_Minor]->setUserProcedure(std::bind_front(keyboardMacroProc, kNotes_Minor));

	// turn on all pitch constraint notes
	pos.set(kAllNotesButtonX, kAllNotesButtonY, allNotesButtonImage->getWidth(), allNotesButtonImage->getHeight() / 2);
	mNotesButtons[kNotes_All] = emplaceControl<DGButton>(this, pos, allNotesButtonImage, 2, DGButton::Mode::Momentary);
	mNotesButtons[kNotes_All]->setUserProcedure(std::bind_front(keyboardMacroProc, kNotes_All));

	// turn off all pitch constraint notes
	pos.set(kNoneNotesButtonX, kNoneNotesButtonY, noneNotesButtonImage->getWidth(), noneNotesButtonImage->getHeight() / 2);
	mNotesButtons[kNotes_None] = emplaceControl<DGButton>(this, pos, noneNotesButtonImage, 2, DGButton::Mode::Momentary);
	mNotesButtons[kNotes_None]->setUserProcedure(std::bind_front(keyboardMacroProc, kNotes_None));


	// .....................MISC..........................

	// turn on/off MIDI learn mode for CC parameter automation
	CreateMidiLearnButton(kMidiLearnButtonX, kMidiLearnButtonY, midiLearnButtonImage);

	// clear all MIDI CC assignments
	CreateMidiResetButton(kMidiResetButtonX, kMidiResetButtonY, midiResetButtonImage);

	// Destroy FX web page link
	pos.set(kDestroyFXLinkX, kDestroyFXLinkY, destroyFXLinkImage->getWidth(), destroyFXLinkImage->getHeight() / 2);
	emplaceControl<DGWebLink>(this, pos, destroyFXLinkImage, DESTROYFX_URL);

	pos.set(kTitleAreaX, kTitleAreaY, kTitleAreaWidth, kTitleAreaHeight);
	mTitleArea = emplaceControl<DGNullControl>(this, pos);



	//--create the help display-----------------------------------------
	pos.set(kHelpX, kHelpY, helpBackgroundImage->getWidth(), helpBackgroundImage->getHeight());
	mHelpBox = emplaceControl<DGHelpBox>(this, pos, std::bind_front(&ScrubbyEditor::GetHelpForControl, this), helpBackgroundImage, DGColor::kWhite);
	mHelpBox->setHeaderFontColor(DGColor::kBlack);
	mHelpBox->setTextMargin({12, 2});



	// this will initialize the translucency state of dependent controls
	HandlePitchConstraintChange();
	HandleTempoSyncChange();
	HandleTempoAutoChange();
	// and this will do the same for the channels mode control
	outputChannelsChanged(getNumOutputChannels());



	return dfx::kStatus_NoError;
}

//-----------------------------------------------------------------------------
void ScrubbyEditor::CloseEditor()
{
	mSeekRateSlider = nullptr;
	mSeekRateDisplay = nullptr;
	mSeekRateRandMinDisplay = nullptr;
	mHelpBox = nullptr;
	mTitleArea = nullptr;
	mNotesButtons.clear();
}


//-----------------------------------------------------------------------------
void ScrubbyEditor::HandleNotesButton(size_t inNotesButtonType)
{
	switch (inNotesButtonType)
	{
		case kNotes_Down:
		{
			auto const bottomValue = getparameter_b(kPitchStep0);
			for (dfx::ParameterID i = kPitchStep0; i < kPitchStep11; i++)
			{
				setparameter_b(i, getparameter_b(i + 1));
			}
			setparameter_b(kPitchStep11, bottomValue);
			break;
		}
		case kNotes_Up:
		{
			auto const topValue = getparameter_b(kPitchStep11);
			for (dfx::ParameterID i = kPitchStep11; i > kPitchStep0; i--)
			{
				setparameter_b(i, getparameter_b(i - 1));
			}
			setparameter_b(kPitchStep0, topValue);
			break;
		}
		case kNotes_Major:
			setparameter_b(kPitchStep0, true);
			setparameter_b(kPitchStep1, false);
			setparameter_b(kPitchStep2, true);
			setparameter_b(kPitchStep3, false);
			setparameter_b(kPitchStep4, true);
			setparameter_b(kPitchStep5, true);
			setparameter_b(kPitchStep6, false);
			setparameter_b(kPitchStep7, true);
			setparameter_b(kPitchStep8, false);
			setparameter_b(kPitchStep9, true);
			setparameter_b(kPitchStep10, false);
			setparameter_b(kPitchStep11, true);
			break;
		case kNotes_Minor:
			setparameter_b(kPitchStep0, true);
			setparameter_b(kPitchStep1, false);
			setparameter_b(kPitchStep2, true);
			setparameter_b(kPitchStep3, true);
			setparameter_b(kPitchStep4, false);
			setparameter_b(kPitchStep5, true);
			setparameter_b(kPitchStep6, false);
			setparameter_b(kPitchStep7, true);
			setparameter_b(kPitchStep8, true);
			setparameter_b(kPitchStep9, false);
			setparameter_b(kPitchStep10, true);
			setparameter_b(kPitchStep11, false);
			break;
		case kNotes_All:
			for (dfx::ParameterID i = kPitchStep0; i <= kPitchStep11; i++)
			{
				setparameter_b(i, true);
			}
			break;
		case kNotes_None:
			for (dfx::ParameterID i = kPitchStep0; i <= kPitchStep11; i++)
			{
				setparameter_b(i, false);
			}
			break;
		default:
			assert(false);
			break;
	}
}

//-----------------------------------------------------------------------------
void ScrubbyEditor::HandlePitchConstraintChange()
{
	auto const speedMode = getparameter_i(kSpeedMode);
	auto const pitchConstraint = getparameter_b(kPitchConstraint);
	float const alpha = ((speedMode == kSpeedMode_Robot) && pitchConstraint) ? 1.0f : kUnusedControlAlpha;
	float const pitchConstraintAlpha = (speedMode == kSpeedMode_Robot) ? 1.0f : kUnusedControlAlpha;

	for (auto& control : mControlsList)
	{
		auto const parameterID = control->getParameterID();
		if ((parameterID >= kPitchStep0) && (parameterID <= kPitchStep11))
		{
			control->setDrawAlpha(alpha);
		}
		else if (parameterID == kPitchConstraint)
		{
			control->setDrawAlpha(pitchConstraintAlpha);
		}
	}

	for (auto& notesButton : mNotesButtons)
	{
		if (notesButton)
		{
			notesButton->setDrawAlpha(alpha);
		}
	}
}

//-----------------------------------------------------------------------------
void ScrubbyEditor::HandleTempoSyncChange()
{
	auto const allowTextEdit = !getparameter_b(kTempoSync);
	mSeekRateDisplay->setTextEditEnabled(allowTextEdit);
	mSeekRateRandMinDisplay->setTextEditEnabled(allowTextEdit);
}

//-----------------------------------------------------------------------------
void ScrubbyEditor::HandleTempoAutoChange()
{
	float const alpha = getparameter_b(kTempoAuto) ? kUnusedControlAlpha : 1.f;
	SetParameterAlpha(kTempo, alpha);
}

//-----------------------------------------------------------------------------
void ScrubbyEditor::parameterChanged(dfx::ParameterID inParameterID)
{
	switch (inParameterID)
	{
		case kTempoSync:
		{
			HandleTempoSyncChange();
			auto const updateParameterID = [useSync = getparameter_b(inParameterID)](IDGControl* control)
			{
				assert(control);
				auto const entryParameterID = control->getParameterID();
				auto newParameterID = dfx::kParameterID_Invalid;
				if ((entryParameterID == kSeekRateRandMin_Hz) || (entryParameterID == kSeekRateRandMin_Sync))
				{
					newParameterID = useSync ? kSeekRateRandMin_Sync : kSeekRateRandMin_Hz;
				}
				else
				{
					newParameterID = useSync ? kSeekRate_Sync : kSeekRate_Hz;
				}
				control->setParameterID(newParameterID);
			};
			updateParameterID(mSeekRateSlider);
			updateParameterID(mSeekRateSlider->getChildren().front());
			updateParameterID(mSeekRateDisplay);
			updateParameterID(mSeekRateRandMinDisplay);
			break;
		}
		case kSpeedMode:
		case kPitchConstraint:
			HandlePitchConstraintChange();
			break;
		case kTempoAuto:
			HandleTempoAutoChange();
			break;
	}
}

//-----------------------------------------------------------------------------
void ScrubbyEditor::mouseovercontrolchanged(IDGControl* /*currentControlUnderMouse*/)
{
	if (mHelpBox)
	{
		mHelpBox->redraw();
	}
}

//-----------------------------------------------------------------------------
void ScrubbyEditor::outputChannelsChanged(size_t inChannelCount)
{
	float const alpha = (inChannelCount > 1) ? 1.f : kUnusedControlAlpha;
	SetParameterAlpha(kSplitChannels, alpha);
}

//-----------------------------------------------------------------------------
std::string ScrubbyEditor::GetHelpForControl(IDGControl* inControl) const
{
	if (!inControl)
	{
		return {};
	}

	if (inControl == mTitleArea)
	{
		return R"DELIM(Scrubby randomly zips around through an audio delay buffer.
Scrubby will, at a given seek rate, find random target destinations within a
certain time range and then travel to those destinations.)DELIM";
	}
	if (inControl == GetMidiLearnButton())
	{
		return R"DELIM(MIDI learn:  toggle "MIDI learn" mode for CC control of parameters
When this is enabled, you can click on a parameter control and then the next
MIDI CC received will be assigned to control that parameter.  (not in all hosts))DELIM";
	}
	if (inControl == GetMidiResetButton())
	{
		return R"DELIM(MIDI reset:  erase CC assignments
Push this button to erase all of your MIDI CC -> parameter assignments.
Then CCs will not affect any parameters and you can start over if you want.)DELIM";
	}

	constexpr auto notesHelpText = R"DELIM(notes:  - only for robot mode with pitch constraint turned on -
You can choose which semitone steps within an octave are allowable when
pitch constraint mode is on.  There are preset and transposition buttons, too.)DELIM";
	if (std::find(mNotesButtons.cbegin(), mNotesButtons.cend(), inControl) != mNotesButtons.cend())
	{
		return notesHelpText;
	}

	switch (inControl->getParameterID())
	{
		case kSeekRange:
			return R"DELIM(seek range:  define the time range in which Scrubby can zip around
This specifies how far back in the delay buffer Scrubby can look for new
random target destinations.  This tends to affect playback speeds.)DELIM";
		case kFreeze:
			return R"DELIM(freeze:  freeze the delay buffer
This causes Scrubby to stop reading from your incoming audio stream and
to stick with the current contents of the delay buffer.)DELIM";
		case kSeekRate_Hz:
		case kSeekRate_Sync:
		case kSeekRateRandMin_Hz:
		case kSeekRateRandMin_Sync:
#if TARGET_OS_MAC
	#define SCRUBBY_ALT_KEY_NAME "option"
#else
	#define SCRUBBY_ALT_KEY_NAME "alt"
#endif
			return "seek rate:  the rate at which Scrubby finds new target destinations\n"
			"You can define a randomized range with min & max rate limits for each seek.\n"
			"(control+click to move both together, " SCRUBBY_ALT_KEY_NAME "+click to move both relative)";
#undef SCRUBBY_ALT_KEY_NAME
		case kTempoSync:
			return R"DELIM(tempo sync:  lock the seek rate to the tempo
Turning this on will let you define seek rates in terms of your tempo.
If your host doesn't send tempo info to plugins, you'll need to define a tempo.)DELIM";
		case kSeekDur:
		case kSeekDurRandMin:
			return R"DELIM(seek duration:  amount of a seek cycle spent moving to the target
Scrubby finds a new target to move towards at each seek cycle.  You can
make it reach the target early by lowering this value.  This produces gaps.)DELIM";
		case kSpeedMode:
			return R"DELIM(speed mode:  are you a robot or a DJ?
In Robot mode, Scrubby jumps to the next speed after each target seek.
In DJ mode, Scrubby gradually accelerates or decelerates to the next speed.)DELIM";
		case kSplitChannels:
			return R"DELIM(channels mode:  toggle between linked or split seeks for each channel
When linked, all audio channels will seek the same target destinations.
When split, each audio channel will find different destinations to seek.)DELIM";
		case kPitchConstraint:
			return R"DELIM(pitch constraint:  - only for robot mode -
With this set to "notes," the playback speeds for each seek will always be
semitone increments from the original pitch.  (see also the keyboard help))DELIM";
		case kPitchStep0:
		case kPitchStep1:
		case kPitchStep2:
		case kPitchStep3:
		case kPitchStep4:
		case kPitchStep5:
		case kPitchStep6:
		case kPitchStep7:
		case kPitchStep8:
		case kPitchStep9:
		case kPitchStep10:
		case kPitchStep11:
			return notesHelpText;
		case kOctaveMin:
		case kOctaveMax:
			return R"DELIM(octave limits:  limit Scrubby's speeds within a range of octaves
You can limit how low or how high Scrubby's playback speeds can go in terms
of octaves, or move these to their outer points if you want no limits.)DELIM";
		case kTempo:
			return R"DELIM(tempo:  sets the tempo that Scrubby uses when tempo sync is on
If your host app doesn't send tempo info to plugins, you'll need to adjust this
parameter in order to specify a tempo for Scrubby to use.)DELIM";
		case kTempoAuto:
			return R"DELIM(sync to host tempo:  follow the host's current tempo
If your host app sends tempo info to plugins, you can enable this parameter
to lock the tempo that Scrubby uses to that of the host.)DELIM";
		case kPredelay:
			return R"DELIM(predelay:  compensate for Scrubby's (possible) output delay
Scrubby zips around a delay buffer, which can create some perceived latency.
This asks your host to predelay by a % of the seek range.  (not in all hosts))DELIM";
		case kDryLevel:
			return R"DELIM(dry level:  the mix level of the unprocessed input audio
Mix in as much of the original, unprocessed audio as you wish.)DELIM";
		case kWetLevel:
			return R"DELIM(wet level:  the mix level of the processed audio
Adjust the amount of Scrubby's zoomies that is mixed into the audio output.)DELIM";
		default:
			return {};
	}
}
