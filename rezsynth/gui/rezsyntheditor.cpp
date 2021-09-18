/*------------------------------------------------------------------------
Copyright (C) 2001-2021  Sophia Poirier

This file is part of Rez Synth.

Rez Synth is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
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

#include <cassert>
#include <cmath>
#include <map>

#include "dfxmath.h"
#include "dfxmisc.h"
#include "rezsynth.h"


//-----------------------------------------------------------------------------
enum
{
	// positions
	kHorizontalSliderX = 25,
	kHorizontalSliderY = 101,
	kHorizontalSliderInc = 48,
	kHorizontalSliderWidth = 256,
	kHorizontalSliderHeight = 23,

	kHorizontalDisplayWidth = kHorizontalSliderWidth / 2,
	kHorizontalDisplayHeight = dfx::kFontContainerHeight_Snooty10px,
	kHorizontalDisplayX = kHorizontalSliderX + kHorizontalSliderWidth - kHorizontalDisplayWidth - 3,
	kHorizontalDisplayY = kHorizontalSliderY - kHorizontalDisplayHeight,

	kVerticalSliderX = 325,
	kVerticalSliderY = 281,
	kVerticalSliderInc = 64,
	kVerticalSliderWidth = 23,
	kVerticalSliderHeight = 240,
	kVerticalSliderNameWidth = 13,

	kVerticalDisplayX = kVerticalSliderX - 21,
	kVerticalDisplayY = kVerticalSliderY + kVerticalSliderHeight + 9,
	kVerticalDisplayWidth = 48,
	kVerticalDisplayHeight = kHorizontalDisplayHeight,

	kButtonColumn1X = 296,
	kButtonColumn2X = 400,
	kButtonNameHeight = 16,
	kResonAlgButtonX = kButtonColumn2X,
	kResonAlgButtonY = 104,
	kBandwidthModeButtonX = kButtonColumn1X,
	kBandwidthModeButtonY = 104,
	kFoldoverButtonX = kButtonColumn1X,
	kFoldoverButtonY = kBandwidthModeButtonY + kHorizontalSliderInc,
	kSepModeButtonX = kButtonColumn1X,
	kSepModeButtonY = kFoldoverButtonY + kHorizontalSliderInc,
	kScaleButtonX = kButtonColumn1X,
	kScaleButtonY = 568,
	kLegatoButtonX = kButtonColumn2X,
	kLegatoButtonY = 200,
	kFadeTypeButtonX = kButtonColumn1X,
	kFadeTypeButtonY = kSepModeButtonY + kHorizontalSliderInc,
	kWiseAmpButtonX = kButtonColumn2X,
	kWiseAmpButtonY = 248,
	kDryWetMixModeButtonX = kButtonColumn2X,
	kDryWetMixModeButtonY = kScaleButtonY,

	kMidiLearnButtonX = 24,
	kMidiLearnButtonY = 568,
	kMidiResetButtonX = 128,
	kMidiResetButtonY = kMidiLearnButtonY,

	kHelpX = 24,
	kHelpY = 600,
	kHelpWidth = 472,
	kHelpHeight = 72,

	kDestroyFXLinkX = 400,
	kDestroyFXLinkY = 672,

	kTitleAreaX = 131,
	kTitleAreaY = 16,
	kTitleAreaWidth = 364,
	kTitleAreaHeight = 58
};


constexpr auto kValueTextFont = dfx::kFontName_Snooty10px;
constexpr auto kValueTextFontSize = dfx::kFontSize_Snooty10px;
constexpr auto kValueTextFontColor = DGColor::kWhite;
constexpr float kUnusedControlAlpha = 0.39f;



//-----------------------------------------------------------------------------
// value text display procedures

static bool bandwidthAmountDisplayProc(float inValue, char* outText, void* inEditor)
{
	const auto success = snprintf(outText, DGTextDisplay::kTextMaxLength, "%.3f", inValue) > 0;
	if (static_cast<DfxGuiEditor*>(inEditor)->getparameter_i(kBandwidthMode) == kBandwidthAmount_Hz)
	{
		dfx::StrlCat(outText, " Hz", DGTextDisplay::kTextMaxLength);
	}
	else
	{
		dfx::StrlCat(outText, " Q", DGTextDisplay::kTextMaxLength);
	}
	return success;
}

static bool numBandsDisplayProc(float inValue, char* outText, void*)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.0f", inValue) > 0;
}

static bool sepAmountDisplayProc(float inValue, char* outText, void* inEditor)
{
	if (static_cast<DfxGuiEditor*>(inEditor)->getparameter_i(kSepMode) == kSeparationMode_Octaval)
	{
		return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.3f semitones", inValue) > 0;
	}
	else  // linear values
	{
		return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.3f x", inValue) > 0;
	}
}

static bool attackDecayReleaseDisplayProc(float inValue, char* outText, void*)
{
	long const thousands = static_cast<long>(inValue) / 1000;
	float const remainder = std::fmod(inValue, 1000.0f);

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

static bool percentGenDisplayProc(float inValue, char* outText, int inPrecision)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.*f%%", inPrecision, inValue) > 0;
}

static bool percentDisplayProc(float inValue, char* outText, void*)
{
	return percentGenDisplayProc(inValue, outText, 0);
}

static bool sustainDisplayProc(float inValue, char* outText, void*)
{
	// the parameter is scaled and has a lot more resolution at low values, so add display precision
	int const precision = (inValue <= 0.9f) ? 1 : 0;
	return percentGenDisplayProc(inValue, outText, precision);
}

static bool velocityCurveDisplayProc(float inValue, char* outText, void*)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%.3f", inValue) > 0;
}

static bool pitchBendDisplayProc(float inValue, char* outText, void*)
{
	return snprintf(outText, DGTextDisplay::kTextMaxLength, "%s%.2f semitones", dfx::kPlusMinusUTF8, inValue) > 0;
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
	auto const horizontalSliderHandleImage_bands = LoadImage("slider-handle-yellow.png");
	auto const horizontalSliderHandleImage_envelope = LoadImage("slider-handle-red.png");
	auto const horizontalSliderHandleImage_midiNotes = LoadImage("slider-handle-white.png");
	auto const horizontalSliderHandleImage_midiBends = LoadImage("slider-handle-magenta.png");
	auto const horizontalSliderHandleImage_learning = LoadImage("slider-handle-learn.png");
	auto const verticalSliderHandleImage = LoadImage("vslider-handle-green.png");
	auto const verticalSliderHandleImage_learning = LoadImage("vslider-handle-learn.png");

	// buttons
	auto const resonAlgButtonImage = LoadImage("button-reson-algorithm.png");
	auto const bandwidthModeButtonImage = LoadImage("button-bandwidth-mode.png");
	auto const sepModeButtonImage = LoadImage("button-separation-mode.png");
	auto const scaleModeButtonImage = LoadImage("button-scaling-mode.png");
	auto const fadesButtonImage = LoadImage("button-envelopes.png");
	auto const legatoButtonImage = LoadImage("button-legato.png");
	auto const foldoverButtonImage = LoadImage("button-foldover.png");
	auto const wiseAmpButtonImage = LoadImage("button-wise-amp.png");
	auto const dryWetMixModeButtonImage = LoadImage("button-mix-mode.png");
	auto const midiLearnButtonImage = LoadImage("button-midi-learn.png");
	auto const midiResetButtonImage = LoadImage("button-midi-reset.png");
	auto const destroyFXLinkImage = LoadImage("button-destroyfx.png");



	// create controls

	DGRect pos, valueDisplayPos;

	//--create the horizontal sliders and displays---------------------------------
	pos.set(kHorizontalSliderX, kHorizontalSliderY, kHorizontalSliderWidth, kHorizontalSliderHeight);
	valueDisplayPos.set(kHorizontalDisplayX, kHorizontalDisplayY, kHorizontalDisplayWidth, kHorizontalDisplayHeight);

	auto const addSliderComponents = [&](long const inParamID, auto const& inDisplayProc)
	{
		auto const horizontalSliderHandleImage = std::map<Section, DGImage*>{
			{Section::Bands, horizontalSliderHandleImage_bands},
			{Section::Envelope, horizontalSliderHandleImage_envelope},
			{Section::MidiNotes, horizontalSliderHandleImage_midiNotes},
			{Section::MidiBends, horizontalSliderHandleImage_midiBends}
		}.at(ParameterToSection(inParamID));

		// slider control
		auto const slider = emplaceControl<DGSlider>(this, inParamID, pos, dfx::kAxis_Horizontal, horizontalSliderHandleImage);
		slider->setAlternateHandle(horizontalSliderHandleImage_learning);

		// value display
		auto const display = emplaceControl<DGTextDisplay>(this, inParamID, valueDisplayPos, inDisplayProc, this, nullptr, dfx::TextAlignment::Right, kValueTextFontSize, kValueTextFontColor, kValueTextFont);

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

		// help mouseover region where the parameter name is displayed
		auto parameterNamePos = valueDisplayPos;
		parameterNamePos.left = pos.left;
		parameterNamePos.right = valueDisplayPos.left;
		emplaceControl<DGNullControl>(this, parameterNamePos)->setParameterID(inParamID);

		pos.offset(0, kHorizontalSliderInc);
		valueDisplayPos.offset(0, kHorizontalSliderInc);
	};

	addSliderComponents((getparameter_i(kBandwidthMode) == kBandwidthMode_Hz) ? kBandwidthAmount_Hz : kBandwidthAmount_Q, bandwidthAmountDisplayProc);
	addSliderComponents(kNumBands, numBandsDisplayProc);
	addSliderComponents((getparameter_i(kSepMode) == kSeparationMode_Octaval) ? kSepAmount_Octaval : kSepAmount_Linear, sepAmountDisplayProc);
	addSliderComponents(kEnvAttack, attackDecayReleaseDisplayProc);
	addSliderComponents(kEnvDecay, attackDecayReleaseDisplayProc);
	addSliderComponents(kEnvSustain, sustainDisplayProc);
	addSliderComponents(kEnvRelease, attackDecayReleaseDisplayProc);
	addSliderComponents(kVelocityInfluence, percentDisplayProc);
	addSliderComponents(kVelocityCurve, velocityCurveDisplayProc);
	addSliderComponents(kPitchBendRange, pitchBendDisplayProc);

	//--create the vertical gain sliders---------------------------------
	pos.set(kVerticalSliderX, kVerticalSliderY, kVerticalSliderWidth, kVerticalSliderHeight);
	valueDisplayPos.set(kVerticalDisplayX, kVerticalDisplayY, kVerticalDisplayWidth, kVerticalDisplayHeight);
	for (long paramID = kFilterOutputGain; paramID <= kDryWetMix; paramID++)
	{
		auto const displayProc = (paramID == kDryWetMix) ? percentDisplayProc : DGTextDisplay::valueToTextProc_LinearToDb;
		constexpr intptr_t decibelPrecisionOffset = -1;
		auto const displayProcUserData = (paramID == kDryWetMix) ? nullptr : reinterpret_cast<void*>(decibelPrecisionOffset);
		auto const textToValueProc = (paramID == kDryWetMix) ? nullptr : DGTextDisplay::textToValueProc_DbToLinear;
		// slider control
		emplaceControl<DGSlider>(this, paramID, pos, dfx::kAxis_Vertical, verticalSliderHandleImage)->setAlternateHandle(verticalSliderHandleImage_learning);

		// value display
		auto const textDisplay = emplaceControl<DGTextDisplay>(this, paramID, valueDisplayPos, displayProc, displayProcUserData, nullptr, dfx::TextAlignment::Center, kValueTextFontSize, kValueTextFontColor, kValueTextFont);
		if (textToValueProc)
		{
			textDisplay->setTextToValueProc(textToValueProc);
		}

		// help mouseover region where the parameter name is displayed
		auto parameterNamePos = pos;
		parameterNamePos.left = pos.left - kVerticalSliderNameWidth;
		parameterNamePos.right = pos.left;
		emplaceControl<DGNullControl>(this, parameterNamePos)->setParameterID(paramID);

		pos.offset(kVerticalSliderInc, 0);
		valueDisplayPos.offset(kVerticalSliderInc, 0);
	}



	//--create the buttons----------------------------------------------

	auto const addButtonWithNameRegion = [this](long paramID, DGRect const& pos, DGImage* image)
	{
		auto parameterNamePos = pos;
		parameterNamePos.top = pos.top - kButtonNameHeight;
		parameterNamePos.bottom = pos.top;
		emplaceControl<DGNullControl>(this, parameterNamePos)->setParameterID(paramID);

		return emplaceControl<DGButton>(this, paramID, pos, image, DGButton::Mode::Radio);
	};

	// filter response scaling
	pos.set(kScaleButtonX, kScaleButtonY, scaleModeButtonImage->getWidth(), scaleModeButtonImage->getHeight() / kNumScaleModes);
	addButtonWithNameRegion(kScaleMode, pos, scaleModeButtonImage)->setRadioThresholds({24, 55});

	// bandwidth mode
	pos.set(kBandwidthModeButtonX, kBandwidthModeButtonY, bandwidthModeButtonImage->getWidth(), bandwidthModeButtonImage->getHeight() / kNumBandwidthModes);
	addButtonWithNameRegion(kBandwidthMode, pos, bandwidthModeButtonImage)->setRadioThresholds({52});

	// separation mode (logarithmic or linear)
	pos.set(kSepModeButtonX, kSepModeButtonY, sepModeButtonImage->getWidth(), sepModeButtonImage->getHeight() / kNumSeparationModes);
	addButtonWithNameRegion(kSepMode, pos, sepModeButtonImage)->setRadioThresholds({52});

	// legato
	pos.set(kLegatoButtonX, kLegatoButtonY, legatoButtonImage->getWidth(), legatoButtonImage->getHeight() / 2);
	addButtonWithNameRegion(kLegato, pos, legatoButtonImage)->setRadioThresholds({38});

	// attack and release fade mode
	pos.set(kFadeTypeButtonX, kFadeTypeButtonY, fadesButtonImage->getWidth(), fadesButtonImage->getHeight() / RezSynth::kCurveType_NumTypes);
	addButtonWithNameRegion(kFadeType, pos, fadesButtonImage)->setRadioThresholds({24, 55});

	// resonance algorithm
	pos.set(kResonAlgButtonX, kResonAlgButtonY, resonAlgButtonImage->getWidth(), resonAlgButtonImage->getHeight() / kNumResonAlgs);
	addButtonWithNameRegion(kResonAlgorithm, pos, resonAlgButtonImage)->setOrientation(dfx::kAxis_Vertical);

	// allow Nyquist foldover or no
	pos.set(kFoldoverButtonX, kFoldoverButtonY, foldoverButtonImage->getWidth(), foldoverButtonImage->getHeight() / 2);
	addButtonWithNameRegion(kFoldover, pos, foldoverButtonImage)->setRadioThresholds({52});

	// wisely lower the output gain to accommodate for resonance or no
	pos.set(kWiseAmpButtonX, kWiseAmpButtonY, wiseAmpButtonImage->getWidth(), wiseAmpButtonImage->getHeight() / 2);
	addButtonWithNameRegion(kWiseAmp, pos, wiseAmpButtonImage)->setRadioThresholds({38});

	// dry/wet mix mode (linear or equal power)
	pos.set(kDryWetMixModeButtonX, kDryWetMixModeButtonY, dryWetMixModeButtonImage->getWidth(), dryWetMixModeButtonImage->getHeight() / kNumDryWetMixModes);
	addButtonWithNameRegion(kDryWetMixMode, pos, dryWetMixModeButtonImage)->setRadioThresholds({45});

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
	mHelpBox = emplaceControl<DGHelpBox>(this, pos, std::bind(&RezSynthEditor::GetHelpForControl, this, std::placeholders::_1), nullptr, DGColor::kWhite);
	mHelpBox->setTextMargin({11, 6});



	// this will initialize the translucency state of dependent controls
	HandleLegatoChange();



	return dfx::kStatus_NoError;
}

//-----------------------------------------------------------------------------
void RezSynthEditor::CloseEditor()
{
	mSepAmountSlider = nullptr;
	mBandwidthAmountSlider = nullptr;
	mSepAmountDisplay = nullptr;
	mBandwidthAmountDisplay = nullptr;
	mHelpBox = nullptr;
	mTitleArea = nullptr;
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
		case kLegato:
			HandleLegatoChange();
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

//-----------------------------------------------------------------------------
void RezSynthEditor::mouseovercontrolchanged(IDGControl* currentControlUnderMouse)
{
	if (!mHelpBox)
	{
		return;
	}

	if (currentControlUnderMouse)
	{
		auto const headerColor = [this, currentControlUnderMouse]() -> DGColor
		{
			constexpr DGColor midiColor(246, 122, 251);
			if (currentControlUnderMouse == mTitleArea)
			{
				return {0, 135, 126};
			}
			if ((currentControlUnderMouse == GetMidiLearnButton()) || (currentControlUnderMouse == GetMidiResetButton()))
			{
				return midiColor;
			}
			if (!currentControlUnderMouse->isParameterAttached())
			{
				return VSTGUI::kTransparentCColor;
			}
			auto const parameterID = currentControlUnderMouse->getParameterID();
			if (parameterID == kWiseAmp)
			{
				return {150, 158, 251};
			}
			return std::map<Section, DGColor>{
				{Section::Bands, {255, 250, 51}},
				{Section::Envelope, {255, 72, 78}},
				{Section::MidiNotes, DGColor::kWhite},
				{Section::MidiBends, midiColor},
				{Section::Mix, {132, 254, 141}}
			}.at(ParameterToSection(parameterID));
		}();
		mHelpBox->setHeaderFontColor(headerColor);
	}

	mHelpBox->redraw();
}

//-----------------------------------------------------------------------------
void RezSynthEditor::HandleLegatoChange()
{
	float const alpha = getparameter_b(kLegato) ? kUnusedControlAlpha : 1.f;
	SetParameterAlpha(kBetweenGain, alpha);
}

//-----------------------------------------------------------------------------
std::string RezSynthEditor::GetHelpForControl(IDGControl* inControl) const
{
	if (!inControl)
	{
		return {};
	}

	if (inControl == mTitleArea)
	{
		return R"DELIM(Rez Synth allows you to "play" resonant band-pass filter banks.
The center frequency of a bank's base filter is controlled by MIDI notes.
Additional filters in each bank (if any) have ascending center frequencies.)DELIM";
	}
	if (inControl == GetMidiLearnButton())
	{
		return R"DELIM(MIDI learn:  toggle "MIDI learn" mode for CC control of parameters
When enabled, you can click on a parameter control and then the next
MIDI CC received will be assigned to control that parameter.)DELIM";
	}
	if (inControl == GetMidiResetButton())
	{
		return R"DELIM(MIDI reset:  erase CC assignments
Push this button to erase all of your MIDI CC -> parameter assignments.
Then CCs will not affect any parameters and you can start over.)DELIM";
	}

	switch (inControl->getParameterID())
	{
		case kBandwidthAmount_Hz:
		case kBandwidthAmount_Q:
			return R"DELIM(bandwidth:  the frequency width of each resonant filter
This is expressed in Hertz or Q, depending on the "bandwidth mode"
parameter.)DELIM";
		case kBandwidthMode:
			return R"DELIM(bandwidth mode:  defines how the "bandwidth" parameter operates
Hz is absolute bandwidth and sounds wider at lower center frequencies
and narrow at higher frequencies.  Q is relative bandwidth and sounds
uniform width across all center frequencies.)DELIM";
		case kResonAlgorithm:
			return R"DELIM(resonance algorithm:  choose a resonant filter algorithm
The options are second-order with no zeroes, second-order with zeroes
at +/- the square root of the pole radius, and second-order with zeroes
at +/-1.  They each sound slightly different and one might sound better
on certain audio material than another.  It's worth trying each.)DELIM";
		case kNumBands:
			return R"DELIM(bands per note:  how many bands will be in each filter bank
Each MIDI note that you play creates a new filter bank on top of any
other banks that are already playing.)DELIM";
		case kSepAmount_Octaval:
		case kSepAmount_Linear:
			return R"DELIM(band separation:  spacing between each band in a filter bank
The way that it operates depends on the band separation mode in use.
If you are working in "octaval" separation mode, then band separation
controls the separation width in semitones.  If you are in "linear" mode,
then it controls the separation as a multiplier of the base frequency.)DELIM";
		case kSepMode:
			return R"DELIM(separation mode:  how the "band separation" parameter operates
Filter spacing can be "linear" (multiple) or "octaval" (logarithmic) mode.
"Linear" makes the other bands spread like harmonics (or inharmonics,
in the case of fractional values) and "octaval" makes the other bands
spread like notes in a chord.)DELIM";
		case kFoldover:
			return R"DELIM(mistakes:  your tolerance for aliased filter frequencies
Banks of resonant filters that ascend in center frequencies allows for
filters in your bank that have center frequencies above what the digital
audio sampling resolution can represent.  Choose whether to "allow" or
"resist" these aliased fold-over bands.)DELIM";
		case kEnvAttack:
			return R"DELIM(attack:  ADSR note envelope generation
Attack sets the duration for the initial volume fade-in period.)DELIM";
		case kEnvDecay:
			return R"DELIM(decay:  ADSR note envelope generation
Decay sets the duration of the partial fade-down period that
immediately follows the attack.)DELIM";
		case kEnvSustain:
			return R"DELIM(sustain:  ADSR note envelope generation
Sustain sets the portion of full note volume that is used for the note
after the decay period.  Therefore a sustain value of 100% effectively
means no decay, and a sustain value of 0% effectively stops a note
after the decay, regardless of how long you hold the note.)DELIM";
		case kEnvRelease:
			return R"DELIM(release:  ADSR note envelope generation
Release sets the duration of the final fade-out period (beginning after
the note play is released).)DELIM";
		case kFadeType:
			return R"DELIM(envelope fades:  the shape of attack, decay, and release
This selects whether the attack, decay, and release segments of note
envelopes are linear fades, exponential curves, or are achieved through
a low-pass filter.  Exponential usually sounds smoother than linear,
and low-pass can be the most natural sounding.)DELIM";
		case kLegato:
			return R"DELIM(legato:  tied monophony vs. polyphony
Legato mode means no polyphony and no gap between notes.  The first
note played after enabling it will remain in the sustain phase of the
note's envelope, even if you release the note, and subsequent notes
will alter the pitch but still only sound in the sustain phase.)DELIM";
		case kVelocityInfluence:
			return R"DELIM(velocity influence:  key velocity affect on volume
This controls how much of an influence key velocity will have on the
volume of each note.  0% means that key velocity is completely ignored
and 100% means that key velocity tracks the full volume range.)DELIM";
		case kVelocityCurve:
			return R"DELIM(velocity curve:
This lets you adjust the key velocity response curve of your MIDI notes.)DELIM";
		case kPitchBendRange:
			return R"DELIM(pitch bend range:
This lets you adjust your MIDI pitch bend control's range in semitones.)DELIM";
		case kScaleMode:
			return R"DELIM(filter response scaling mode:
This allows you to scale the response factor of the resonant filters.
It can be scaled to bring the RMS of the resonant filter response to
unity volume level, or to bring the peak of the resonant filter response
to unity, or you can disable scaling.)DELIM";
		case kWiseAmp:
			return R"DELIM(careful: filter output limiting
The volume of the output of the resonant band-pass filters is pretty
erratic (sometimes even with scaling).  Careful mode attempts to keep
your output within a narrow range.  Whether you use this mode or not,
you may still want to follow with a dynamic limiter or compressor.)DELIM";
		case kFilterOutputGain:
			return R"DELIM(filtered output gain: note volume
This allows you to control the level of the filtered output audio after
all of its processing has taken place.)DELIM";
		case kBetweenGain:
			return R"DELIM(between gain: dry volume between notes
This controls the volume of the unprocessed input audio when no notes
are sounding.)DELIM";
		case kDryWetMix:
			return R"DELIM(dry/wet mix:  pre/post-filter ratio
This lets you adjust the balance of the (unprocessed) input audio and
the (processed) output audio.  100% is all processed.)DELIM";
		case kDryWetMixMode:
			return R"DELIM(dry/wet mix mode:
This allows you to adjust the way that the dry/wet mix combines the
dry and wet audio. You can choose linear mixing or equal power mixing.)DELIM";
		default:
			return {};
	}
}

RezSynthEditor::Section RezSynthEditor::ParameterToSection(long inParameterID)
{
	switch (inParameterID)
	{
		case kBandwidthAmount_Hz:
		case kBandwidthAmount_Q:
		case kBandwidthMode:
		case kResonAlgorithm:
		case kNumBands:
		case kSepAmount_Octaval:
		case kSepAmount_Linear:
		case kSepMode:
		case kFoldover:
			return Section::Bands;
		case kEnvAttack:
		case kEnvDecay:
		case kEnvSustain:
		case kEnvRelease:
		case kFadeType:
			return Section::Envelope;
		case kVelocityInfluence:
		case kVelocityCurve:
			return Section::MidiNotes;
		case kPitchBendRange:
		case kLegato:
			return Section::MidiBends;
		case kScaleMode:
		case kWiseAmp:
		case kFilterOutputGain:
		case kBetweenGain:
		case kDryWetMix:
		case kDryWetMixMode:
			return Section::Mix;
		default:
			assert(false);
			return {};
	}
}
