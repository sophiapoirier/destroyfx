/*------------------------------------------------------------------------
Copyright (C) 2000-2023  Sophia Poirier

This file is part of Skidder.

Skidder is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Skidder is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Skidder.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#include "skiddereditor.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "dfxmath.h"
#include "dfxmisc.h"
#include "skidder.h"
#include "temporatetable.h"


//-----------------------------------------------------------------------------
constexpr auto kValueDisplayFont = dfx::kFontName_Snooty10px;
constexpr auto kValueDisplayFontColor = DGColor::kWhite;
constexpr auto kValueDisplayFontSize = dfx::kFontSize_Snooty10px;
constexpr float kUnusedControlAlpha = 0.33333333f;

//-----------------------------------------------------------------------------
// positions
enum
{
	kSliderX = 60,
	kSliderY = 97,
	kSliderInc = 64,

	kDisplayX = 334,
	kDisplayY = kSliderY + 10,
	kDisplayWidth = 74,
	kDisplayHeight = 12,

	kRandMinDisplayX = 0,
	kRandMinDisplayY = kDisplayY,
	kRandMinDisplayWidth = kSliderX - kRandMinDisplayX - 7,
	kRandMinDisplayHeight = kDisplayHeight,

	kParameterNameDisplayX = 14,
	kParameterNameDisplayY = kDisplayY - 17,
	kParameterNameDisplayWidth = kDisplayWidth + 30,

	kParameterButtonX = 408,

	kTempoSyncButtonX = kParameterButtonX,
	kTempoSyncButtonY = 102,

	kTempoAutoButtonX = kParameterButtonX,
	kTempoAutoButtonY = 550,

	kCrossoverModeButtonX = kParameterButtonX,
	kCrossoverModeButtonY = 358,

	kMidiModeButtonX = 60,
	kMidiModeButtonY = 620,

	kVelocityButtonX = 101,
	kVelocityButtonY = 646,

	kMidiLearnButtonX = 221,
	kMidiLearnButtonY = kMidiModeButtonY,
	kMidiResetButtonX = 295,
	kMidiResetButtonY = kMidiLearnButtonY,

	kHelpX = 56,
	kHelpY = 671,
	kHelpWidth = 447,
	kHelpHeight = 72,

	kDestroyFXLinkX = 421,
	kDestroyFXLinkY = 745,

	kTitleAreaX = 55,
	kTitleAreaY = 13,
	kTitleAreaWidth = 317,
	kTitleAreaHeight = 48
};



//-----------------------------------------------------------------------------
// parameter value string display conversion functions

static bool rateGenDisplayProc(float inValue, dfx::ParameterID inSyncParameterID, char* outText, DfxGuiEditor* inEditor, bool inShowUnits)
{
	if (inEditor->getparameter_b(kTempoSync))
	{
		if (auto const valueString = inEditor->getparametervaluestring(inSyncParameterID))
		{
			bool success = dfx::StrLCpy(outText, *valueString, DGTextDisplay::kTextMaxLength) > 0;
			dfx::TempoRateTable const tempoRateTable;
			if (*valueString == tempoRateTable.getDisplay(tempoRateTable.getNumRates() - 1))
			{
				success = dfx::StrLCpy(outText, VSTGUI::kInfiniteSymbol, DGTextDisplay::kTextMaxLength) > 0;
			}
			if (inShowUnits)
			{
				dfx::StrlCat(outText, " cyc/beat", DGTextDisplay::kTextMaxLength);
			}
			return success;
		}
	}
	else
	{
		return std::snprintf(outText, DGTextDisplay::kTextMaxLength, "%.3f%s", inValue, inShowUnits ? " Hz" : "") > 0;
	}
	return false;
}

static bool rateDisplayProc(float inValue, char* outText, void* inEditor)
{
	return rateGenDisplayProc(inValue, kRate_Sync, outText, static_cast<DfxGuiEditor*>(inEditor), true);
}

static bool rateRandMinDisplayProc(float inValue, char* outText, void* inEditor)
{
	return rateGenDisplayProc(inValue, kRateRandMin_Sync, outText, static_cast<DfxGuiEditor*>(inEditor), false);
}

static bool slopeDisplayProc(float inValue, char* outText, void*)
{
	return std::snprintf(outText, DGTextDisplay::kTextMaxLength, "%.3f ms", inValue) > 0;
}

static bool crossoverDisplayProc(float inValue, char* outText, void*)
{
	if (inValue >= 1'000.f)
	{
		return std::snprintf(outText, DGTextDisplay::kTextMaxLength, "%.2f kHz", inValue / 1'000.f) > 0;
	}
	return std::snprintf(outText, DGTextDisplay::kTextMaxLength, "%.0f Hz", inValue) > 0;
}

static std::optional<float> crossoverTextToValueProc(std::string const& inText, DGTextDisplay* inTextDisplay)
{
	auto const value = DGTextDisplay::textToValueProc_Generic(inText, inTextDisplay);
	if (value && (dfx::ToLower(inText).find("khz") != std::string::npos))
	{
		return *value * 1'000.f;
	}
	return value;
}

static bool gainRandMinDisplayProc(float inValue, char* outText, void* inUserData)
{
	auto const success = DGTextDisplay::valueToTextProc_LinearToDb(inValue, outText, inUserData);
	if (success)
	{
		// truncate units or any suffix beyond the numerical value
		std::transform(outText, outText + std::strlen(outText), outText, [](auto character)
		{
			return std::isspace(character) ? '\0' : character;
		});
	}
	return success;
}

static bool tempoDisplayProc(float inValue, char* outText, void*)
{
	return std::snprintf(outText, DGTextDisplay::kTextMaxLength, "%.2f bpm", inValue) > 0;
}



//-----------------------------------------------------------------------------
DFX_EDITOR_ENTRY(SkidderEditor)

//-----------------------------------------------------------------------------
void SkidderEditor::OpenEditor()
{
	// create images

	// sliders
	auto const sliderBackgroundImage = LoadImage("slider-background.png");
	auto const sliderHandleImage = LoadImage("slider-handle.png");
	auto const sliderHandleImage_learning = LoadImage("slider-handle-learn.png");
	auto const rangeSliderHandleLeftImage = LoadImage("range-slider-handle-left.png");
	auto const rangeSliderHandleLeftImage_learning = LoadImage("range-slider-handle-left-learn.png");
	auto const rangeSliderHandleRightImage = LoadImage("range-slider-handle-right.png");
	auto const rangeSliderHandleRightImage_learning = LoadImage("range-slider-handle-right-learn.png");

	// mode buttons
	auto const tempoSyncButtonImage = LoadImage("tempo-sync-button.png");
	auto const tempoAutoButtonImage = LoadImage("host-tempo-button.png");
	auto const crossoverModeButtonImage = LoadImage("crossover-mode-button.png");
	auto const midiModeButtonImage = LoadImage("midi-mode-button.png");
	auto const velocityButtonImage = LoadImage("velocity-button.png");

	// other buttons
	auto const midiLearnButtonImage = LoadImage("midi-learn-button.png");
	auto const midiResetButtonImage = LoadImage("midi-reset-button.png");
	auto const destroyFXLinkImage = LoadImage("destroy-fx-link.png");


	auto const [rateRandMinParameterID, rateParameterID] = GetActiveRateParameterIDs();


	//--initialize the sliders-----------------------------------------------
	DGRect pos;
	constexpr auto rangeSliderPushStyle = DGRangeSlider::PushStyle::Upper;

	// rate   (single slider for "free" Hz rate and tempo synced rate)
	pos.set(kSliderX, kSliderY, sliderBackgroundImage->getWidth(), sliderBackgroundImage->getHeight());
	mRateSlider = emplaceControl<DGRangeSlider>(this, rateRandMinParameterID, rateParameterID, pos, rangeSliderHandleLeftImage, rangeSliderHandleRightImage, sliderBackgroundImage, rangeSliderPushStyle);
	mRateSlider->setAlternateHandles(rangeSliderHandleLeftImage_learning, rangeSliderHandleRightImage_learning);

	// pulsewidth
	pos.offset(0, kSliderInc);
	auto rangeSlider = emplaceControl<DGRangeSlider>(this, kPulsewidthRandMin, kPulsewidth, pos, rangeSliderHandleLeftImage, rangeSliderHandleRightImage, sliderBackgroundImage, rangeSliderPushStyle);
	rangeSlider->setAlternateHandles(rangeSliderHandleLeftImage_learning, rangeSliderHandleRightImage_learning);

	// slope
	pos.offset(0, kSliderInc);
	emplaceControl<DGSlider>(this, kSlope, pos, dfx::kAxis_Horizontal, sliderHandleImage, sliderBackgroundImage)->setAlternateHandle(sliderHandleImage_learning);

	// floor
	pos.offset(0, kSliderInc);
	rangeSlider = emplaceControl<DGRangeSlider>(this, kFloorRandMin, kFloor, pos, rangeSliderHandleLeftImage, rangeSliderHandleRightImage, sliderBackgroundImage, rangeSliderPushStyle);
	rangeSlider->setAlternateHandles(rangeSliderHandleLeftImage_learning, rangeSliderHandleRightImage_learning);

	// crossover
	pos.offset(0, kSliderInc);
	emplaceControl<DGSlider>(this, kCrossoverFrequency, pos, dfx::kAxis_Horizontal, sliderHandleImage, sliderBackgroundImage)->setAlternateHandle(sliderHandleImage_learning);

	// pan
	pos.offset(0, kSliderInc);
	emplaceControl<DGSlider>(this, kPan, pos, dfx::kAxis_Horizontal, sliderHandleImage, sliderBackgroundImage)->setAlternateHandle(sliderHandleImage_learning);

	// noise
	pos.offset(0, kSliderInc);
	emplaceControl<DGSlider>(this, kNoise, pos, dfx::kAxis_Horizontal, sliderHandleImage, sliderBackgroundImage)->setAlternateHandle(sliderHandleImage_learning);

	// tempo (in bpm)
	pos.offset(0, kSliderInc);
	emplaceControl<DGSlider>(this, kTempo, pos, dfx::kAxis_Horizontal, sliderHandleImage, sliderBackgroundImage)->setAlternateHandle(sliderHandleImage_learning);


	//--initialize the buttons----------------------------------------------

	// choose the rate type ("free" or synced)
	pos.set(kTempoSyncButtonX, kTempoSyncButtonY, tempoSyncButtonImage->getWidth(), tempoSyncButtonImage->getHeight() / 2);
	emplaceControl<DGButton>(this, kTempoSync, pos, tempoSyncButtonImage, DGButton::Mode::Radio)->setRadioThresholds({28});

	// use host tempo
	pos.set(kTempoAutoButtonX, kTempoAutoButtonY, tempoAutoButtonImage->getWidth(), tempoAutoButtonImage->getHeight() / 2);
	emplaceControl<DGButton>(this, kTempoAuto, pos, tempoAutoButtonImage, DGButton::Mode::Radio)->setRadioThresholds({36});

	// crossover mode
	pos.set(kCrossoverModeButtonX, kCrossoverModeButtonY, crossoverModeButtonImage->getWidth(), crossoverModeButtonImage->getHeight() / kNumCrossoverModes);
	emplaceControl<DGButton>(this, kCrossoverMode, pos, crossoverModeButtonImage, DGButton::Mode::Radio)->setRadioThresholds({26, 59});

	// MIDI note control mode button
	pos.set(kMidiModeButtonX, kMidiModeButtonY, midiModeButtonImage->getWidth(), midiModeButtonImage->getHeight() / kNumMidiModes);
	emplaceControl<DGButton>(this, kMidiMode, pos, midiModeButtonImage, DGButton::Mode::Radio)->setRadioThresholds({41, 95});

	// use-note-velocity button
	emplaceControl<DGToggleImageButton>(this, kVelocity, kVelocityButtonX, kVelocityButtonY, velocityButtonImage);

	// turn on/off MIDI learn mode for CC parameter automation
	CreateMidiLearnButton(kMidiLearnButtonX, kMidiLearnButtonY, midiLearnButtonImage);

	// clear all MIDI CC assignments
	CreateMidiResetButton(kMidiResetButtonX, kMidiResetButtonY, midiResetButtonImage);

	// Destroy FX web page link
	pos.set(kDestroyFXLinkX, kDestroyFXLinkY, destroyFXLinkImage->getWidth(), destroyFXLinkImage->getHeight() / 2);
	emplaceControl<DGWebLink>(this, pos, destroyFXLinkImage, DESTROYFX_URL);

	pos.set(kTitleAreaX, kTitleAreaY, kTitleAreaWidth, kTitleAreaHeight);
	mTitleArea = emplaceControl<DGNullControl>(this, pos);

	// help display
	pos.set(kHelpX, kHelpY, kHelpWidth, kHelpHeight);
	mHelpBox = emplaceControl<DGHelpBox>(this, pos, std::bind_front(&SkidderEditor::GetHelpForControl, this), nullptr, DGColor::kWhite);
	mHelpBox->setTextMargin({8, 10});
	mHelpBox->setLineSpacing(3);


	//--initialize the displays---------------------------------------------

	// rate   (unified display for "free" Hz rate and tempo synced rate)
	pos.set(kDisplayX, kDisplayY, kDisplayWidth, kDisplayHeight);
	mRateDisplay = emplaceControl<DGTextDisplay>(this, rateParameterID, pos, rateDisplayProc, this, nullptr, dfx::TextAlignment::Left, kValueDisplayFontSize, kValueDisplayFontColor, kValueDisplayFont);

	// rate random minimum
	pos.set(kRandMinDisplayX, kRandMinDisplayY, kRandMinDisplayWidth, kRandMinDisplayHeight);
	mRateRandMinDisplay = emplaceControl<DGTextDisplay>(this, rateRandMinParameterID, pos, rateRandMinDisplayProc, this, nullptr, dfx::TextAlignment::Right, kValueDisplayFontSize, kValueDisplayFontColor, kValueDisplayFont);

	// pulsewidth
	pos.set(kDisplayX, kDisplayY + (kSliderInc * 1), kDisplayWidth, kDisplayHeight);
	auto textDisplay = emplaceControl<DGTextDisplay>(this, kPulsewidth, pos, DGTextDisplay::valueToTextProc_LinearToPercent, nullptr, nullptr, dfx::TextAlignment::Left, kValueDisplayFontSize, kValueDisplayFontColor, kValueDisplayFont);
	textDisplay->setValueFromTextConvertProc(DGTextDisplay::valueFromTextConvertProc_PercentToLinear);

	// pulsewidth random minimum
	pos.set(kRandMinDisplayX, kRandMinDisplayY + (kSliderInc * 1), kRandMinDisplayWidth, kRandMinDisplayHeight);
	mPulsewidthRandMinDisplay = emplaceControl<DGTextDisplay>(this, kPulsewidthRandMin, pos, DGTextDisplay::valueToTextProc_LinearToPercent, nullptr, nullptr, dfx::TextAlignment::Right, kValueDisplayFontSize, kValueDisplayFontColor, kValueDisplayFont);
	mPulsewidthRandMinDisplay->setValueFromTextConvertProc(DGTextDisplay::valueFromTextConvertProc_PercentToLinear);

	// slope
	pos.set(kDisplayX, kDisplayY + (kSliderInc * 2), kDisplayWidth, kDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kSlope, pos, slopeDisplayProc, nullptr, nullptr, dfx::TextAlignment::Left, kValueDisplayFontSize, kValueDisplayFontColor, kValueDisplayFont);

	// floor
	pos.offset(0, kSliderInc);
	textDisplay = emplaceControl<DGTextDisplay>(this, kFloor, pos, DGTextDisplay::valueToTextProc_LinearToDb, nullptr, nullptr, dfx::TextAlignment::Left, kValueDisplayFontSize, kValueDisplayFontColor, kValueDisplayFont);
	textDisplay->setTextToValueProc(DGTextDisplay::textToValueProc_DbToLinear);

	// floor random minimum
	pos.set(kRandMinDisplayX, kRandMinDisplayY + (kSliderInc * 3), kRandMinDisplayWidth, kRandMinDisplayHeight);
	mFloorRandMinDisplay = emplaceControl<DGTextDisplay>(this, kFloorRandMin, pos, gainRandMinDisplayProc, nullptr, nullptr, dfx::TextAlignment::Right, kValueDisplayFontSize, kValueDisplayFontColor, kValueDisplayFont);
	mFloorRandMinDisplay->setTextToValueProc(DGTextDisplay::textToValueProc_DbToLinear);

	// crossover
	pos.set(kDisplayX, kDisplayY + (kSliderInc * 4), kDisplayWidth, kDisplayHeight);
	textDisplay = emplaceControl<DGTextDisplay>(this, kCrossoverFrequency, pos, crossoverDisplayProc, nullptr, nullptr, dfx::TextAlignment::Left, kValueDisplayFontSize, kValueDisplayFontColor, kValueDisplayFont);
	textDisplay->setTextToValueProc(crossoverTextToValueProc);

	// pan
	pos.offset(0, kSliderInc);
	textDisplay = emplaceControl<DGTextDisplay>(this, kPan, pos, DGTextDisplay::valueToTextProc_LinearToPercent, nullptr, nullptr, dfx::TextAlignment::Left, kValueDisplayFontSize, kValueDisplayFontColor, kValueDisplayFont);
	textDisplay->setValueFromTextConvertProc(DGTextDisplay::valueFromTextConvertProc_PercentToLinear);

	// noise
	pos.offset(0, kSliderInc);
	textDisplay = emplaceControl<DGTextDisplay>(this, kNoise, pos, DGTextDisplay::valueToTextProc_LinearToDb, nullptr, nullptr, dfx::TextAlignment::Left, kValueDisplayFontSize, kValueDisplayFontColor, kValueDisplayFont);
	textDisplay->setTextToValueProc(DGTextDisplay::textToValueProc_DbToLinear);

	// tempo (in bpm)
	pos.offset(0, kSliderInc);
	emplaceControl<DGTextDisplay>(this, kTempo, pos, tempoDisplayProc, nullptr, nullptr, dfx::TextAlignment::Left, kValueDisplayFontSize, kValueDisplayFontColor, kValueDisplayFont);


	UpdateRandomMinimumDisplays();
	HandleTempoSyncChange();
	HandleTempoAutoChange();
	HandleCrossoverModeChange();
	HandleMidiModeChange();
	outputChannelsChanged(getNumOutputChannels());
}

//-----------------------------------------------------------------------------
void SkidderEditor::CloseEditor()
{
	mRateSlider = nullptr;
	mRateDisplay = nullptr;
	mRateRandMinDisplay = nullptr;
	mPulsewidthRandMinDisplay = nullptr;
	mFloorRandMinDisplay = nullptr;
	mHelpBox = nullptr;
	mTitleArea = nullptr;
}

//-----------------------------------------------------------------------------
void SkidderEditor::parameterChanged(dfx::ParameterID inParameterID)
{
	switch (inParameterID)
	{
		case kTempoSync:
		{
			auto const [rateRandMinParameterID, rateParameterID] = GetActiveRateParameterIDs();
			mRateSlider->setParameterID(rateParameterID);
			mRateSlider->getChildren().front()->setParameterID(rateRandMinParameterID);
			mRateDisplay->setParameterID(rateParameterID);
			mRateRandMinDisplay->setParameterID(rateRandMinParameterID);
			HandleTempoSyncChange();
			UpdateRandomMinimumDisplays();
			break;
		}

		case kRate_Hz:
		case kRate_Sync:
		case kRateRandMin_Hz:
		case kRateRandMin_Sync:
		case kPulsewidth:
		case kPulsewidthRandMin:
		case kFloor:
		case kFloorRandMin:
			UpdateRandomMinimumDisplays();
			break;

		case kCrossoverMode:
			HandleCrossoverModeChange();
			break;

		case kTempoAuto:
			HandleTempoAutoChange();
			break;

		case kMidiMode:
		case kVelocity:
			HandleMidiModeChange();
			break;

		default:
			break;
	}
}

//-----------------------------------------------------------------------------
void SkidderEditor::outputChannelsChanged(size_t inChannelCount)
{
	float const alpha = (inChannelCount == 2) ? 1.f : kUnusedControlAlpha;
	SetParameterAlpha(kPan, alpha);
}

//-----------------------------------------------------------------------------
void SkidderEditor::mouseovercontrolchanged(IDGControl* currentControlUnderMouse)
{
	if (mHelpBox)
	{
		mHelpBox->redraw();
	}
}

//-----------------------------------------------------------------------------
std::pair<dfx::ParameterID, dfx::ParameterID> SkidderEditor::GetActiveRateParameterIDs()
{
	if (getparameter_b(kTempoSync))
	{
		return {kRateRandMin_Sync, kRate_Sync};
	}
	return {kRateRandMin_Hz, kRate_Hz};
}

//-----------------------------------------------------------------------------
void SkidderEditor::UpdateRandomMinimumDisplays()
{
	auto const updateRandomMinimumVisibility = [this](dfx::ParameterID mainParameterID, dfx::ParameterID randMinParameterID, VSTGUI::CControl* control)
	{
		bool const visible = getparameter_f(mainParameterID) > getparameter_f(randMinParameterID);
		control->setVisible(visible);
	};

	auto const [rateRandMinParameterID, rateParameterID] = GetActiveRateParameterIDs();
	updateRandomMinimumVisibility(rateParameterID, rateRandMinParameterID, mRateRandMinDisplay);
	updateRandomMinimumVisibility(kPulsewidth, kPulsewidthRandMin, mPulsewidthRandMinDisplay);
	updateRandomMinimumVisibility(kFloor, kFloorRandMin, mFloorRandMinDisplay);
}

//-----------------------------------------------------------------------------
void SkidderEditor::HandleTempoSyncChange()
{
	auto const allowTextEdit = !getparameter_b(kTempoSync);
	mRateDisplay->setTextEditEnabled(allowTextEdit);
	mRateRandMinDisplay->setTextEditEnabled(allowTextEdit);
}

//-----------------------------------------------------------------------------
void SkidderEditor::HandleTempoAutoChange()
{
	float const alpha = getparameter_b(kTempoAuto) ? kUnusedControlAlpha : 1.0f;
	SetParameterAlpha(kTempo, alpha);
}

//-----------------------------------------------------------------------------
void SkidderEditor::HandleCrossoverModeChange()
{
	float const alpha = (getparameter_i(kCrossoverMode) == kCrossoverMode_All) ? kUnusedControlAlpha : 1.0f;
	SetParameterAlpha(kCrossoverFrequency, alpha);
}

//-----------------------------------------------------------------------------
void SkidderEditor::HandleMidiModeChange()
{
	bool const isMidiModeEnabled = (getparameter_i(kMidiMode) != kMidiMode_None);

	float const velocityAlpha = isMidiModeEnabled ? 1.0f : kUnusedControlAlpha;
	SetParameterAlpha(kVelocity, velocityAlpha);

	float const floorAlpha = (isMidiModeEnabled && getparameter_b(kVelocity)) ? kUnusedControlAlpha : 1.0f;
	SetParameterAlpha(kFloor, floorAlpha);
	SetParameterAlpha(kFloorRandMin, floorAlpha);
}

//-----------------------------------------------------------------------------
std::string SkidderEditor::GetHelpForControl(IDGControl* inControl) const
{
	if (!inControl)
	{
		return {};
	}

	if (inControl == mTitleArea)
	{
		return R"DELIM(Skidder turns the sound on and off.
It also can randomly pan your choppy chunks of sound all about,
which is why it's a skidder, like how a rock skids across a frozen
lake when you throw it, bouncing here and there lopsided-style.)DELIM";
	}
	if (inControl == GetMidiLearnButton())
	{
		return R"DELIM(MIDI learn:  toggle "MIDI learn" mode for CC control of parameters
When enabled, you can click on a parameter control and then the
next MIDI CC received will be assigned to control that parameter.)DELIM";
	}
	if (inControl == GetMidiResetButton())
	{
		return R"DELIM(MIDI reset:  erase CC assignments
Push this button to erase all your MIDI CC -> parameter assignments.
Then CCs will not affect any parameters and you can start over.)DELIM";
	}

#if TARGET_OS_MAC
	#define SKIDDER_ALT_KEY_NAME "option"
#else
	#define SKIDDER_ALT_KEY_NAME "alt"
#endif
	std::string const randomRangeText = "You can define a randomized range with min/max limits for each cycle.\n"
	"(control+click to move both together, " SKIDDER_ALT_KEY_NAME "+click to move relative)";
#undef SKIDDER_ALT_KEY_NAME

	switch (inControl->getParameterID())
	{
		case kRate_Hz:
		case kRate_Sync:
		case kRateRandMin_Hz:
		case kRateRandMin_Sync:
			return R"DELIM(rate:  the frequency at which the sound goes on and off
The value is cycles per second, or per beat with "beat sync" enabled.
)DELIM" + randomRangeText;
		case kTempoSync:
			return R"DELIM(tempo sync:  defines how the "rate" parameter operates
Hz mode means the "rate" parameter operates in cycles per second,
and beat sync mode means that "rate" operates in cycles per beat,
which itself is dependent upon the "tempo" parameter or host tempo.)DELIM";
		case kPulsewidth:
		case kPulsewidthRandMin:
			return R"DELIM(pulse width:  the ratio of sound on vs off
This controls how much of each cycle is "on" (full volume).
)DELIM" + randomRangeText;
		case kSlope:
			return R"DELIM(slope:  gain smoothing duration
This sets the length of the volume-smoothing slopes that are at
the beginning (fade in) and end (fade out) of each pulse.)DELIM";
		case kPan:
			return R"DELIM(stereo spread:  width of random panning
This controls the amount of random panning to which your outputted
sound chunks will be subjected. Lowering the stereo spread value will
limit how far from center the random panning is allowed to stray.)DELIM";
		case kNoise:
			return "rupture\nblurps of noise betwixt your skids";
		case kFloor:
		case kFloorRandMin:
			return R"DELIM(floor:  level of the reduced volume period
This controls sound level during the "off" period of each cycle.
)DELIM" + randomRangeText;
		case kCrossoverFrequency:
			return R"DELIM(crossover frequency:  selective skids cutoff point
This adjusts the crossover cutoff frequency used when operating in
"low" or "high" crossover mode.)DELIM";
		case kCrossoverMode:
			return R"DELIM(crossover mode:  which side skids
You can limit the skidding to only part of the frequency spectrum.
With "all", Skidder effects the full spectrum. With "low" or "high", the
effect is only applied to audio below or above crossover frequency.)DELIM";
		case kTempo:
			return R"DELIM(tempo:  sets the tempo used when tempo sync is on
If your host app doesn't send tempo info to plugins, you'll need to
adjust this parameter in order to specify a tempo for Skidder to use.)DELIM";
		case kTempoAuto:
			return R"DELIM(sync to host tempo:  follow the host's current tempo
If your host app sends tempo info to plugins, you can enable this
parameter to lock the tempo and beat position that Skidder uses to
follow the host.)DELIM";
		case kMidiMode:
			return R"DELIM(MIDI mode:  note control
In MIDI trigger mode, the effect is active while you play a note and
otherwise sound is muted. MIDI apply mode is similar except that the
sound stays on, unprocessed, between notes. "None" ignores notes.)DELIM";
		case kVelocity:
			return R"DELIM(velocity:  effect depth control
If you enable this and you are using a MIDI note control mode, then
the floor level will be controlled by note velocity. The floor is lower
when the note velocity is higher, and vice versa.)DELIM";
		default:
			return {};
	}
}
