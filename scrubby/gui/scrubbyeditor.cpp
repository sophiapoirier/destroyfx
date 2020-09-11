/*------------------------------------------------------------------------
Copyright (C) 2002-2020  Sophia Poirier

This file is part of Scrubby.

Scrubby is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Scrubby is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Scrubby.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "scrubbyeditor.h"

#include <array>
#include <cassert>
#include <cstdlib>
#include <sstream>

#include "dfxmisc.h"
#include "scrubby.h"


//-----------------------------------------------------------------------------
static auto const kDisplayFont = dfx::kFontName_SnootPixel10;
constexpr auto kDisplayTextSize = dfx::kFontSize_SnootPixel10;
constexpr DGColor kBrownTextColor(187, 173, 131);
constexpr float kUnusedControlAlpha = 0.234f;
constexpr long kRangeSliderOvershoot = 3;

constexpr long kOctavesSliderWidth = 118 - 2;
static long const kOctaveMaxSliderWidth = static_cast<long>((static_cast<float>(kOctave_MaxValue) / static_cast<float>((std::abs(kOctave_MinValue) + kOctave_MaxValue))) * static_cast<float>(kOctavesSliderWidth));
static long const kOctaveMinSliderWidth = kOctavesSliderWidth - kOctaveMaxSliderWidth;
constexpr long kOctaveMinSliderX = 33 + 1;
static long const kOctaveMaxSliderX = kOctaveMinSliderX + kOctaveMinSliderWidth;

//-----------------------------------------------------------------------------
enum
{
	// positions
	kSliderHeight = 28,

#if 0
	kMainSlidersOffsetY = 0,
	kNotesStuffOffsetY = 0,
#else
	kMainSlidersOffsetY = -133,
	kNotesStuffOffsetY = 128,
#endif

	kSeekRateSliderX = 33 + 1,
	kSeekRateSliderY = 298 + kMainSlidersOffsetY,
	kSeekRateSliderWidth = 290 - 2,

	kSeekRangeSliderX = 33 + 1,
	kSeekRangeSliderY = 341 + kMainSlidersOffsetY,
	kSeekRangeSliderWidth = 336 - 2,

	kSeekDurSliderX = 33 + 1,
	kSeekDurSliderY = 384 + kMainSlidersOffsetY,
	kSeekDurSliderWidth = 336 - 2,

	kOctaveMinSliderY = 255 + kNotesStuffOffsetY,
	kOctaveMaxSliderY = kOctaveMinSliderY,

	kTempoSliderX = 33 + 1,
	kTempoSliderY = 427,
	kTempoSliderWidth = 194 - 2,

	kPredelaySliderX = 251 + 1,
	kPredelaySliderY = 383,
	kPredelaySliderWidth = 118 - 2,

	kDisplayWidth = 90,
	kDisplayWidth_big = 117,  // for seek rate
	kDisplayHeight = 10,
	kDisplayInsetX = 2,
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
	kLittleTempoSyncButtonY = 303 + kMainSlidersOffsetY,

	kTempoAutoButtonX = 231,
	kTempoAutoButtonY = 432,

	kKeyboardX = 33,
	kKeyboardY = 164 + kNotesStuffOffsetY,

	kTransposeDownButtonX = 263,
	kTransposeDownButtonY = 164 + kNotesStuffOffsetY,
	kTransposeUpButtonX = kTransposeDownButtonX,
	kTransposeUpButtonY = kTransposeDownButtonY + 46,

	kMajorChordButtonX = 299,
	kMajorChordButtonY = 164 + kNotesStuffOffsetY,
	kMinorChordButtonX = kMajorChordButtonX,
	kMinorChordButtonY = kMajorChordButtonY + 19,
	kAllNotesButtonX = kMajorChordButtonX,
	kAllNotesButtonY = kMajorChordButtonY + 42,
	kNoneNotesButtonX = kMajorChordButtonX,
	kNoneNotesButtonY = kMajorChordButtonY + 61,

	kMidiLearnButtonX = 433,
	kMidiLearnButtonY = 292 + kNotesStuffOffsetY,
	kMidiResetButtonX = kMidiLearnButtonX,
	kMidiResetButtonY = kMidiLearnButtonY + 19,

	kHelpX = 4,
	kHelpY = 465,

	kDestroyFXLinkX = 247,
	kDestroyFXLinkY = 47,

	kTitleAreaX = 125,
	kTitleAreaY = 8,
	kTitleAreaWidth = 260,
	kTitleAreaHeight = 37
};

enum
{
	kNotes_Down = 0,
	kNotes_Up,
	kNotes_Major,
	kNotes_Minor,
	kNotes_All,
	kNotes_None,
	kNumNotesButtons
};

enum
{
	kHelp_None = 0,
	kHelp_General,
	kHelp_SeekRate,
	kHelp_SeekRange,
	kHelp_SeekDur,
	kHelp_SpeedMode,
	kHelp_Freeze,
	kHelp_SplitChannels,
	kHelp_TempoSync,
	kHelp_PitchConstraint,
	kHelp_Notes,
	kHelp_Octaves,
	kHelp_Tempo,
	kHelp_TempoAuto,
	kHelp_PreDelay,
	kHelp_MidiLearn,
	kHelp_MidiReset,
	kNumHelps
};

constexpr std::array<const char* const, kNumHelps> const kHelpStrings
{{
	"",
	// general
	R"DELIM(Scrubby randomly zips around through an audio delay buffer.
Scrubby will, at a given seek rate, find random target destinations within a 
certain time range and then travel to those destinations.)DELIM", 
#if TARGET_OS_MAC
	#define SCRUBBY_ALT_KEY_NAME "option"
#else
	#define SCRUBBY_ALT_KEY_NAME "alt"
#endif
	// seek rate
	"seek rate:  the rate at which Scrubby finds new target destinations\n"
	"You can define a randomized range with min and max rate limits for each seek.\n"
	"(control+click to move both together, " SCRUBBY_ALT_KEY_NAME "+click to move both relative)", 
#undef SCRUBBY_ALT_KEY_NAME
	// seek range
	R"DELIM(seek range:  define the time range in which Scrubby can zip around
This specifies how far back in the delay buffer Scrubby can look for new 
random target destinations.  This tends to affect playback speeds.)DELIM", 
	// seek duration
	R"DELIM(seek duration:  amount of a seek cycle spent moving to the target
Scrubby finds a new target to move towards at each seek cycle.  You can 
make it reach the target early by lowering this value.  This produces gaps.)DELIM", 
	// speed mode
	R"DELIM(speed mode:  are you a robot or a DJ?
Robot mode causes Scrubby to jump to the next speed after each target seek.  
In DJ mode, Scrubby gradually accelerates or decelerates to next speed.)DELIM", 
	// freeze
	R"DELIM(freeze:  freeze the delay buffer
This causes Scrubby to stop reading from your incoming audio stream and 
to stick with the current contents of the delay buffer.)DELIM", 
	// channels mode
	R"DELIM(channels mode:  toggle between linked or split seeks for each channel
When linked, all audio channels will seek the same target destinations.  
When split, each audio channel will find different destinations to seek.)DELIM", 
	// tempo sync
	R"DELIM(tempo sync:  lock the seek rate to the tempo
Turning this on will let you define seek rates in terms of your tempo.  
If your host doesn't send tempo info to plugins, you'll need to define a tempo.)DELIM", 
	// pitch constraint
	R"DELIM(pitch constraint:  - only for robot mode -
With this set to "notes," the playback speeds for each seek will always be 
semitone increments from the original pitch.  (see also the keyboard help))DELIM", 
	// notes
	R"DELIM(notes:  - only for robot mode with pitch constraint turned on -
You can choose which semitone steps within an octave are allowable when 
pitch constraint mode is on.  There are preset and transposition buttons, too.)DELIM", 
	// octaves
	R"DELIM(octave limits:  limit Scrubby's speeds within a range of octaves
You can limit how low or how high Scrubby's playback speeds can go in terms 
of octaves, or move these to their outer points if you want no limits.)DELIM", 
	// tempo
	R"DELIM(tempo:  sets the tempo that Scrubby uses when tempo sync is on
If your host app doesn't send tempo info to plugins, you'll need to adjust this 
parameter in order to specify a tempo for Scrubby to use.)DELIM", 
	// tempo auto
	R"DELIM(sync to host tempo:  follow the host's current tempo
If your host app sends tempo info to plugins, you can enable this parameter 
to lock the tempo that Scrubby uses to that of the host.)DELIM", 
	// predelay
	R"DELIM(predelay:  compensate for Scrubby's (possible) output delay
Scrubby zips around a delay buffer, which can create some perceived latency.  
This asks your host to predelay by a % of the seek range.  (not in all hosts))DELIM", 
	// MIDI learn
	R"DELIM(MIDI learn:  toggle "MIDI learn" mode for CC control of parameters
When this is enabled, you can click on a parameter control and then the next 
MIDI CC received will be assigned to control that parameter.  (not in all hosts))DELIM", 
	// MIDI reset
	R"DELIM(MIDI reset:  erase CC assignments
Push this button to erase all of your MIDI CC -> parameter assignments.  
Then CCs won't affect any parameters and you can start over if you want.)DELIM"
}};



//-----------------------------------------------------------------------------
// parameter value string display conversion functions

bool seekRangeDisplayProc(float inValue, char* outText, void*)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.1f ms", inValue) > 0;
}

bool seekRateGenDisplayProc(float inValue, long inParameterID, char* outText, DfxGuiEditor* inEditor)
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

bool seekRateDisplayProc(float inValue, char* outText, void* inEditor)
{
	return seekRateGenDisplayProc(inValue, kSeekRate_Sync, outText, static_cast<DfxGuiEditor*>(inEditor));
}

bool seekRateRandMinDisplayProc(float inValue, char* outText, void* inEditor)
{
	return seekRateGenDisplayProc(inValue, kSeekRateRandMin_Sync, outText, static_cast<DfxGuiEditor*>(inEditor));
}

bool seekDurDisplayProc(float inValue, char* outText, void*)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.1f%%", inValue) > 0;
}

bool octaveMinDisplayProc(float inValue, char* outText, void*)
{
	auto const octaves = static_cast<long>(inValue);
	if (octaves <= kOctave_MinValue)
	{
		return dfx::StrLCpy(outText, "no min", DGTextDisplay::kTextMaxLength) > 0;
	}
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%ld", octaves) > 0;
}

bool octaveMaxDisplayProc(float inValue, char* outText, void*)
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

bool tempoDisplayProc(float inValue, char* outText, void*)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.3f bpm", inValue) > 0;
}

bool predelayDisplayProc(float inValue, char* outText, void*)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.0f%%", inValue) > 0;
}



#pragma mark -

//--------------------------------------------------------------------------
// this is a display for Scrubby's built-in help
ScrubbyHelpBox::ScrubbyHelpBox(DfxGuiEditor* inOwnerEditor, DGRect const& inRegion, DGImage* inBackground)
:	DGStaticTextDisplay(inOwnerEditor, inRegion, inBackground, dfx::TextAlignment::Left, 
						kDisplayTextSize, DGColor::kBlack, kDisplayFont), 
	mItemNum(kHelp_None)
{
}

//--------------------------------------------------------------------------
void ScrubbyHelpBox::draw(VSTGUI::CDrawContext* inContext)
{
	if (mItemNum == kHelp_None)
	{
		return;
	}

	if (auto const image = getDrawBackground())
	{
		image->draw(inContext, getViewSize());
	}

	DGRect textpos(getViewSize());
	textpos.setSize(textpos.getWidth() - 13, kDisplayHeight);
	textpos.offset(12, 4);

	std::istringstream stream(kHelpStrings.at(mItemNum));
	std::string line;
	bool headerDrawn = false;
	while (std::getline(stream, line))
	{
		if (!std::exchange(headerDrawn, true))
		{
			setFontColor(DGColor::kBlack);
			drawPlatformText(inContext, VSTGUI::UTF8String(line).getPlatformString(), textpos);
			textpos.offset(1, 0);
			drawPlatformText(inContext, VSTGUI::UTF8String(line).getPlatformString(), textpos);
			textpos.offset(-1, 13);
			setFontColor(DGColor::kWhite);
		}
		else
		{
			drawPlatformText(inContext, VSTGUI::UTF8String(line).getPlatformString(), textpos);
			textpos.offset(0, 11);
		}
	}

	setDirty(false);
}

//--------------------------------------------------------------------------
void ScrubbyHelpBox::setDisplayItem(long inItemNum)
{
	if ((inItemNum < 0) || (inItemNum >= kNumHelps))
	{
		return;
	}

	bool const changed = (mItemNum != inItemNum);

	mItemNum = inItemNum;

	if (changed)
	{
		redraw();
	}
}





#pragma mark -

//-----------------------------------------------------------------------------
DFX_EDITOR_ENTRY(ScrubbyEditor)

//-----------------------------------------------------------------------------
ScrubbyEditor::ScrubbyEditor(DGEditorListenerInstance inInstance)
:	DfxGuiEditor(inInstance),
	mNotesButtons(kNumNotesButtons, nullptr)
{
}

//-----------------------------------------------------------------------------
long ScrubbyEditor::OpenEditor()
{
	// create images

	// sliders
	auto const sliderHandleImage = VSTGUI::makeOwned<DGImage>("slider-handle.png");
	auto const sliderHandleImage_glowing = VSTGUI::makeOwned<DGImage>("slider-handle-glowing.png");
	auto const rangeSliderHandleLeftImage = VSTGUI::makeOwned<DGImage>("range-slider-handle-left.png");
	auto const rangeSliderHandleLeftImage_glowing = VSTGUI::makeOwned<DGImage>("range-slider-handle-left-glowing.png");
	auto const rangeSliderHandleRightImage = VSTGUI::makeOwned<DGImage>("range-slider-handle-right.png");
	auto const rangeSliderHandleRightImage_glowing = VSTGUI::makeOwned<DGImage>("range-slider-handle-right-glowing.png");

	// mode buttons
	auto const speedModeButtonImage = VSTGUI::makeOwned<DGImage>("speed-mode-button.png");
	auto const freezeButtonImage = VSTGUI::makeOwned<DGImage>("freeze-button.png");
	auto const tempoSyncButtonImage = VSTGUI::makeOwned<DGImage>("tempo-sync-button.png");
	auto const splitChannelsButtonImage = VSTGUI::makeOwned<DGImage>("stereo-button.png");
	auto const pitchConstraintButtonImage = VSTGUI::makeOwned<DGImage>("pitch-constraint-button.png");
	auto const tempoSyncButtonImage_little = VSTGUI::makeOwned<DGImage>("tempo-sync-button-little.png");
	auto const hostTempoButtonImage = VSTGUI::makeOwned<DGImage>("host-tempo-button.png");

	// pitch constraint control buttons
//	auto const keyboardOffImage = VSTGUI::makeOwned<DGImage>("keyboard-off.png");
//	auto const keyboardOnImage = VSTGUI::makeOwned<DGImage>("keyboard-on.png");
	//
	VSTGUI::SharedPointer<DGImage> keyboardTopKeyImages[kNumPitchSteps];
	keyboardTopKeyImages[1] = keyboardTopKeyImages[3] = keyboardTopKeyImages[6] = keyboardTopKeyImages[8] = keyboardTopKeyImages[10] = VSTGUI::makeOwned<DGImage>("keyboard-black-key.png");
	keyboardTopKeyImages[0] = VSTGUI::makeOwned<DGImage>("keyboard-white-key-top-1.png");
	keyboardTopKeyImages[2] = VSTGUI::makeOwned<DGImage>("keyboard-white-key-top-2.png");
	keyboardTopKeyImages[4] = VSTGUI::makeOwned<DGImage>("keyboard-white-key-top-3.png");
	keyboardTopKeyImages[5] = VSTGUI::makeOwned<DGImage>("keyboard-white-key-top-4.png");
	keyboardTopKeyImages[7] = keyboardTopKeyImages[9] = VSTGUI::makeOwned<DGImage>("keyboard-white-key-top-5-6.png");
	keyboardTopKeyImages[11] = VSTGUI::makeOwned<DGImage>("keyboard-white-key-top-7.png");
	//
	VSTGUI::SharedPointer<DGImage> keyboardBottomKeyImages[kNumPitchSteps];
	keyboardBottomKeyImages[0] = VSTGUI::makeOwned<DGImage>("keyboard-white-key-bottom-left.png");
	keyboardBottomKeyImages[2] = keyboardBottomKeyImages[4] = keyboardBottomKeyImages[5] = keyboardBottomKeyImages[7] = keyboardBottomKeyImages[9] = VSTGUI::makeOwned<DGImage>("keyboard-white-key-bottom.png");
	keyboardBottomKeyImages[kNumPitchSteps - 1] = VSTGUI::makeOwned<DGImage>("keyboard-white-key-bottom-right.png");
	//
	auto const transposeDownButtonImage = VSTGUI::makeOwned<DGImage>("transpose-down-button.png");
	auto const transposeUpButtonImage = VSTGUI::makeOwned<DGImage>("transpose-up-button.png");
	auto const majorChordButtonImage = VSTGUI::makeOwned<DGImage>("major-chord-button.png");
	auto const minorChordButtonImage = VSTGUI::makeOwned<DGImage>("minor-chord-button.png");
	auto const allNotesButtonImage = VSTGUI::makeOwned<DGImage>("all-notes-button.png");
	auto const noneNotesButtonImage = VSTGUI::makeOwned<DGImage>("none-notes-button.png");

	// help box
	auto const helpBackgroundImage = VSTGUI::makeOwned<DGImage>("help-background.png");

	// other buttons
	auto const midiLearnButtonImage = VSTGUI::makeOwned<DGImage>("midi-learn-button.png");
	auto const midiResetButtonImage = VSTGUI::makeOwned<DGImage>("midi-reset-button.png");
	auto const destroyFXLinkImage = VSTGUI::makeOwned<DGImage>("destroy-fx-link.png");



	DGRect pos;
	long const seekRateParamID = getparameter_b(kTempoSync) ? kSeekRate_Sync : kSeekRate_Hz;
	long const seekRateRandMinParamID = getparameter_b(kTempoSync) ? kSeekRateRandMin_Sync : kSeekRateRandMin_Hz;

	//--create the sliders-----------------------------------------------

	// seek rate
	pos.set(kSeekRateSliderX, kSeekRateSliderY, kSeekRateSliderWidth, kSliderHeight);
	mSeekRateSlider = emplaceControl<DGRangeSlider>(this, seekRateRandMinParamID, seekRateParamID, pos, 
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



	//--create the displays---------------------------------------------

	// seek rate random minimum
	pos.set(kSeekRateSliderX + kDisplayInsetX, kSeekRateSliderY - kDisplayHeight + kDisplayInsetY, kDisplayWidth_big, kDisplayHeight);
	mSeekRateRandMinDisplay = emplaceControl<DGTextDisplay>(this, seekRateRandMinParamID, pos, seekRateRandMinDisplayProc, this, nullptr, dfx::TextAlignment::Left, kDisplayTextSize, kBrownTextColor, kDisplayFont);

	// seek rate
	pos.set(kSeekRateSliderX + kSeekRateSliderWidth - kDisplayWidth_big - kDisplayInsetX, kSeekRateSliderY - kDisplayHeight + kDisplayInsetY, kDisplayWidth_big, kDisplayHeight);
	mSeekRateDisplay = emplaceControl<DGTextDisplay>(this, seekRateParamID, pos, seekRateDisplayProc, this, nullptr, dfx::TextAlignment::Right, kDisplayTextSize, kBrownTextColor, kDisplayFont);

	// seek range
	pos.set(kSeekRangeSliderX + kSeekRangeSliderWidth - kDisplayWidth - kDisplayInsetX, kSeekRangeSliderY - kDisplayHeight + kDisplayInsetY, kDisplayWidth, kDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kSeekRange, pos, seekRangeDisplayProc, nullptr, nullptr, dfx::TextAlignment::Right, kDisplayTextSize, kBrownTextColor, kDisplayFont);

	// seek duration random minimum
	pos.set(kSeekDurSliderX + kDisplayInsetX, kSeekDurSliderY - kDisplayHeight + kDisplayInsetY, kDisplayWidth, kDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kSeekDurRandMin, pos, seekDurDisplayProc, nullptr, nullptr, dfx::TextAlignment::Left, kDisplayTextSize, kBrownTextColor, kDisplayFont);

	// seek duration
	pos.set(kSeekDurSliderX + kSeekDurSliderWidth - kDisplayWidth - kDisplayInsetX, kSeekDurSliderY - kDisplayHeight + kDisplayInsetY, kDisplayWidth, kDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kSeekDur, pos, seekDurDisplayProc, nullptr, nullptr, dfx::TextAlignment::Right, kDisplayTextSize, kBrownTextColor, kDisplayFont);

	// octave mininum
	pos.set(kOctaveMinSliderX + kDisplayInsetX, kOctaveMinSliderY - kDisplayHeight + kDisplayInsetY, kDisplayWidth, kDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kOctaveMin, pos, octaveMinDisplayProc, nullptr, nullptr, dfx::TextAlignment::Left, kDisplayTextSize, kBrownTextColor, kDisplayFont);

	// octave maximum
	pos.set(kOctaveMaxSliderX + kOctaveMaxSliderWidth - kDisplayWidth - kDisplayInsetX, kOctaveMaxSliderY - kDisplayHeight + kDisplayInsetY, kDisplayWidth, kDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kOctaveMax, pos, octaveMaxDisplayProc, nullptr, nullptr, dfx::TextAlignment::Right, kDisplayTextSize, kBrownTextColor, kDisplayFont);

	// tempo
	pos.set(kTempoSliderX + kDisplayInsetX, kTempoSliderY - kDisplayHeight + kDisplayInsetY, kDisplayWidth, kDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kTempo, pos, tempoDisplayProc, nullptr, nullptr, dfx::TextAlignment::Left, kDisplayTextSize, kBrownTextColor, kDisplayFont);

	// predelay
	pos.set(kPredelaySliderX + kPredelaySliderWidth - kDisplayWidth - kDisplayInsetX, kPredelaySliderY - kDisplayHeight + kDisplayInsetY, kDisplayWidth, kDisplayHeight);
	emplaceControl<DGTextDisplay>(this, kPredelay, pos, predelayDisplayProc, nullptr, nullptr, dfx::TextAlignment::Right, kDisplayTextSize, kBrownTextColor, kDisplayFont);



	//--create the keyboard---------------------------------------------
//	pos.set(kKeyboardX, kKeyboardY, keyboardOffImage->getWidth(), keyboardOffImage->getHeight());
//	keyboard = emplaceControl<ScrubbyKeyboard>(this, kPitchStep0, pos, keyboardOffImage, keyboardOnImage, 24, 48, 18, 56, 114, 149, 184);
	pos.set(kKeyboardX, kKeyboardY, keyboardTopKeyImages[0]->getWidth(), keyboardTopKeyImages[0]->getHeight() / 2);
	DGRect keyboardBottomKeyPos(kKeyboardX, kKeyboardY + pos.getHeight(), keyboardBottomKeyImages[0]->getWidth(), keyboardBottomKeyImages[0]->getHeight() / 2);
	for (size_t i = 0; i < kNumPitchSteps; i++)
	{
		long const paramID = kPitchStep0 + i;

		// this visually syncs the top and bottom button images upon mouse clicks
		auto const keyboardButtonProc = [](long value, void* button)
		{
			auto const keyboardButton = static_cast<DGButton*>(button);
			auto const editor = keyboardButton->getOwnerEditor();
			editor->getFrame()->forEachChild([originalButton = keyboardButton->asCControl()](VSTGUI::CView* child)
			{
				auto const paramID = originalButton->getTag();
				auto const control = dynamic_cast<VSTGUI::CControl*>(child);
				if (control && (control->getTag() == paramID) && (control != originalButton))
				{
					control->setValue(originalButton->getValue());
					if (control->isDirty())
					{
						control->invalid();
					}
				}
			});
		};

		pos.setWidth(keyboardTopKeyImages[i]->getWidth());
		auto button = emplaceControl<DGButton>(this, paramID, pos, keyboardTopKeyImages[i], DGButton::Mode::Increment);
		pos.offset(pos.getWidth(), 0);

		if (keyboardBottomKeyImages[i] != nullptr)
		{
			button->setUserProcedure(keyboardButtonProc, button);

			keyboardBottomKeyPos.setWidth(keyboardBottomKeyImages[i]->getWidth());
			button = emplaceControl<DGButton>(this, paramID, keyboardBottomKeyPos, keyboardBottomKeyImages[i], DGButton::Mode::Increment);
			button->setUserProcedure(keyboardButtonProc, button);
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

	// toggle pitch constraint
	emplaceControl<DGToggleImageButton>(this, kPitchConstraint, kPitchConstraintButtonX, kPitchConstraintButtonY, pitchConstraintButtonImage, true);

	// choose the seek rate type ("free" or synced)
	emplaceControl<DGToggleImageButton>(this, kTempoSync, kLittleTempoSyncButtonX, kLittleTempoSyncButtonY, tempoSyncButtonImage_little, false);

	// enable sync to host tempo
	emplaceControl<DGToggleImageButton>(this, kTempoAuto, kTempoAutoButtonX, kTempoAutoButtonY, hostTempoButtonImage, false);


	// ...............PITCH CONSTRAINT....................

	// transpose all of the pitch constraint notes down one semitone
	pos.set(kTransposeDownButtonX, kTransposeDownButtonY, transposeDownButtonImage->getWidth(), transposeDownButtonImage->getHeight() / 2);
	mNotesButtons[kNotes_Down] = emplaceControl<DGButton>(this, pos, transposeDownButtonImage, 2, DGButton::Mode::Momentary);
	mNotesButtons[kNotes_Down]->setUserProcedure([](long value, void* editor)
	{
		if (value != 0)
		{
			static_cast<ScrubbyEditor*>(editor)->HandleNotesButton(kNotes_Down);
		}
	}, this);

	// transpose all of the pitch constraint notes up one semitone
	pos.set(kTransposeUpButtonX, kTransposeUpButtonY, transposeUpButtonImage->getWidth(), transposeUpButtonImage->getHeight() / 2);
	mNotesButtons[kNotes_Up] = emplaceControl<DGButton>(this, pos, transposeUpButtonImage, 2, DGButton::Mode::Momentary);
	mNotesButtons[kNotes_Up]->setUserProcedure([](long value, void* editor)
	{
		if (value != 0)
		{
			static_cast<ScrubbyEditor*>(editor)->HandleNotesButton(kNotes_Up);
		}
	}, this);

	// turn on the pitch constraint notes that form a major chord
	pos.set(kMajorChordButtonX, kMajorChordButtonY, majorChordButtonImage->getWidth(), majorChordButtonImage->getHeight() / 2);
	mNotesButtons[kNotes_Major] = emplaceControl<DGButton>(this, pos, majorChordButtonImage, 2, DGButton::Mode::Momentary);
	mNotesButtons[kNotes_Major]->setUserProcedure([](long value, void* editor)
	{
		if (value != 0)
		{
			static_cast<ScrubbyEditor*>(editor)->HandleNotesButton(kNotes_Major);
		}
	}, this);

	// turn on the pitch constraint notes that form a minor chord
	pos.set(kMinorChordButtonX, kMinorChordButtonY, minorChordButtonImage->getWidth(), minorChordButtonImage->getHeight() / 2);
	mNotesButtons[kNotes_Minor] = emplaceControl<DGButton>(this, pos, minorChordButtonImage, 2, DGButton::Mode::Momentary);
	mNotesButtons[kNotes_Minor]->setUserProcedure([](long value, void* editor)
	{
		if (value != 0)
		{
			static_cast<ScrubbyEditor*>(editor)->HandleNotesButton(kNotes_Minor);
		}
	}, this);

	// turn on all pitch constraint notes
	pos.set(kAllNotesButtonX, kAllNotesButtonY, allNotesButtonImage->getWidth(), allNotesButtonImage->getHeight() / 2);
	mNotesButtons[kNotes_All] = emplaceControl<DGButton>(this, pos, allNotesButtonImage, 2, DGButton::Mode::Momentary);
	mNotesButtons[kNotes_All]->setUserProcedure([](long value, void* editor)
	{
		if (value != 0)
		{
			static_cast<ScrubbyEditor*>(editor)->HandleNotesButton(kNotes_All);
		}
	}, this);

	// turn off all pitch constraint notes
	pos.set(kNoneNotesButtonX, kNoneNotesButtonY, noneNotesButtonImage->getWidth(), noneNotesButtonImage->getHeight() / 2);
	mNotesButtons[kNotes_None] = emplaceControl<DGButton>(this, pos, noneNotesButtonImage, 2, DGButton::Mode::Momentary);
	mNotesButtons[kNotes_None]->setUserProcedure([](long value, void* editor)
	{
		if (value != 0)
		{
			static_cast<ScrubbyEditor*>(editor)->HandleNotesButton(kNotes_None);
		}
	}, this);


	// .....................MISC..........................

	// turn on/off MIDI learn mode for CC parameter automation
	mMidiLearnButton = CreateMidiLearnButton(kMidiLearnButtonX, kMidiLearnButtonY, midiLearnButtonImage);

	// clear all MIDI CC assignments
	mMidiResetButton = CreateMidiResetButton(kMidiResetButtonX, kMidiResetButtonY, midiResetButtonImage);

	// Destroy FX web page link
	pos.set(kDestroyFXLinkX, kDestroyFXLinkY, destroyFXLinkImage->getWidth(), destroyFXLinkImage->getHeight() / 2);
	emplaceControl<DGWebLink>(this, pos, destroyFXLinkImage, DESTROYFX_URL);

	pos.set(kTitleAreaX, kTitleAreaY, kTitleAreaWidth, kTitleAreaHeight);
	mTitleArea = emplaceControl<DGNullControl>(this, pos);



	//--create the help display-----------------------------------------
	pos.set(kHelpX, kHelpY, helpBackgroundImage->getWidth(), helpBackgroundImage->getHeight());
	mHelpBox = emplaceControl<ScrubbyHelpBox>(this, pos, helpBackgroundImage);



	// this will initialize the translucency state of dependent controls 
	HandlePitchConstraintChange();
	HandleTempoSyncChange();
	HandleTempoAutoChange();
	// and this will do the same for the channels mode control
	numAudioChannelsChanged(getNumAudioChannels());



	return dfx::kStatus_NoError;
}


//-----------------------------------------------------------------------------
void ScrubbyEditor::HandleNotesButton(long inNotesButtonType)
{
	switch (inNotesButtonType)
	{
		case kNotes_Down:
		{
			auto const tempValue = getparameter_b(kPitchStep0);
			for (long i = kPitchStep0; i < kPitchStep11; i++)
			{
				setparameter_b(i, getparameter_b(i + 1));
			}
			setparameter_b(kPitchStep11, tempValue);
			break;
		}
		case kNotes_Up:
		{
			auto const tempValue = getparameter_b(kPitchStep11);
			for (long i = kPitchStep11; i > kPitchStep0; i--)
			{
				setparameter_b(i, getparameter_b(i - 1));
			}
			setparameter_b(kPitchStep0, tempValue);
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
			for (long i = kPitchStep0; i <= kPitchStep11; i++)
			{
				setparameter_b(i, true);
			}
			break;
		case kNotes_None:
			for (long i = kPitchStep0; i <= kPitchStep11; i++)
			{
				setparameter_b(i, false);
			}
			break;
		default:
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
		auto const paramID = control->getParameterID();
		if ((paramID >= kPitchStep0) && (paramID <= kPitchStep11))
		{
			control->setDrawAlpha(alpha);
		}
		else if (paramID == kPitchConstraint)
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
	mSeekRateDisplay->setMouseEnabled(allowTextEdit);
	mSeekRateRandMinDisplay->setMouseEnabled(allowTextEdit);
}

//-----------------------------------------------------------------------------
void ScrubbyEditor::HandleTempoAutoChange()
{
	float const alpha = getparameter_b(kTempoAuto) ? kUnusedControlAlpha : 1.0f;
	for (auto& control : mControlsList)
	{
		if (control->getParameterID() == kTempo)
		{
			control->setDrawAlpha(alpha);
		}
	}
}

//-----------------------------------------------------------------------------
void ScrubbyEditor::parameterChanged(long inParameterID)
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
		default:
			break;
	}
}

//-----------------------------------------------------------------------------
void ScrubbyEditor::mouseovercontrolchanged(IDGControl* currentControlUnderMouse)
{
	auto const newHelpItem = [this, currentControlUnderMouse]() -> long
	{

		if (currentControlUnderMouse)
		{
			if (currentControlUnderMouse == mTitleArea)
			{
				return kHelp_General;
			}
			else if (currentControlUnderMouse == mMidiLearnButton)
			{
				return kHelp_MidiLearn;
			}
			else if (currentControlUnderMouse == mMidiResetButton)
			{
				return kHelp_MidiReset;
			}

			for (auto const& notesButton : mNotesButtons)
			{
				if (currentControlUnderMouse == notesButton)
				{
					return kHelp_Notes;
				}
			}

			switch (currentControlUnderMouse->getParameterID())
			{
				case kSeekRange:
					return kHelp_SeekRange;
				case kFreeze:
					return kHelp_Freeze;
				case kSeekRate_Hz:
				case kSeekRate_Sync:
				case kSeekRateRandMin_Hz:
				case kSeekRateRandMin_Sync:
					return kHelp_SeekRate;
				case kTempoSync:
					return kHelp_TempoSync;
				case kSeekDur:
				case kSeekDurRandMin:
					return kHelp_SeekDur;
				case kSpeedMode:
					return kHelp_SpeedMode;
				case kSplitChannels:
					return kHelp_SplitChannels;
				case kPitchConstraint:
					return kHelp_PitchConstraint;
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
					return kHelp_Notes;
				case kOctaveMin:
				case kOctaveMax:
					return kHelp_Octaves;
				case kTempo:
					return kHelp_Tempo;
				case kTempoAuto:
					return kHelp_TempoAuto;
				case kPredelay:
					return kHelp_PreDelay;
				default:
					break;
			}
		}

		return kHelp_None;
	}();

	if (mHelpBox)
	{
		mHelpBox->setDisplayItem(newHelpItem);
	}
}

//-----------------------------------------------------------------------------
void ScrubbyEditor::numAudioChannelsChanged(unsigned long inNumChannels)
{
	float const alpha = (inNumChannels > 1) ? 1.0f : kUnusedControlAlpha;

	for (auto& control : mControlsList)
	{
		if (control->getParameterID() == kSplitChannels)
		{
			control->setDrawAlpha(alpha);
		}
	}
}
