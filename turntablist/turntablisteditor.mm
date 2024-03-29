/*------------------------------------------------------------------------
Copyright (C) 2004-2024  Sophia Poirier

This file is part of Turntablist.

Turntablist is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 2 of the License, or 
(at your option) any later version.

Turntablist is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Turntablist.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#import <AppKit/NSAlert.h>
#import <AppKit/NSView.h>
#include <AudioToolbox/AudioFile.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <mutex>
#include <vector>

#include "dfx-au-utilities.h"
#include "dfxgui.h"
#include "dfxmath.h"
#include "dfxmisc.h"
#include "dfxparameter.h"
#include "lib/platform/iplatformframe.h"
#include "turntablist.h"

#define DRAG_N_DROP !__LP64__  // TODO: reimplement



constexpr float kTurntablistFontSize = 10.0f;
constexpr size_t kKnobFrames = 61;

//-----------------------------------------------------------------------------
enum
{
	// positions
	kScratchAmountSliderX = 31,
	kPitchShiftSliderX = 166,
	kSliderY = 249,
	kSliderWidth = 22,
	kSliderHeight = 197,

	kFileNameX = 31,
	kFileNameY = 23 + 1,
	kFileNameWidth = 158,
	kFileNameHeight = 15,

	kDisplayX = 31,
	kDisplayY = 458,
	kDisplayWidth = 122,
	kDisplayHeight = 14,

	kKnobOffsetX = 1,
	kKnobOffsetY = 1,

	kColumn1 = 25,
	kColumn2 = 70,
	kColumn3 = 115,
	kColumn4 = 160,

	kRow1 = 64,
	kRow2 = 123,
	kRow3 = 182,
	kRow4 = 241,
	kRow5 = 300,
	kRow6 = 359,
	kRow7 = 418,

	kPowerButtonX = kColumn1,
	kPowerButtonY = kRow2,
	kNotePowerTrackButtonX = kColumn4,
	kNotePowerTrackButtonY = kRow2,
	kLoopButtonX = kColumn3,
	kLoopButtonY = kRow3,
	kDirectionButtonX = kColumn2,
	kDirectionButtonY = kRow4,
	kNoteModeButtonX = kColumn3,
	kNoteModeButtonY = kRow4,
	kKeyTrackButtonX = kColumn2,
	kKeyTrackButtonY = kRow5,
	kScratchModeButtonX = kColumn2,
	kScratchModeButtonY = kRow7,

	kLoadButtonX = kColumn4,
	kLoadButtonY = kRow1,
	kPlayButtonX = kColumn2,
	kPlayButtonY = kRow3,

	kMidiLearnX = kColumn2,
	kMidiLearnY = kRow6,
	kMidiResetX = kColumn3,
	kMidiResetY = kMidiLearnY,

	kSpinUpKnobX = kColumn2 + kKnobOffsetX,
	kSpinUpKnobY = kRow2 + kKnobOffsetY,
	kSpinDownKnobX = kColumn3 + kKnobOffsetX,
	kSpinDownKnobY = kRow2 + kKnobOffsetY,
	kPitchRangeKnobX = kColumn4 + kKnobOffsetX,
	kPitchRangeKnobY = kRow3 + kKnobOffsetY,
	kScratchSpeedKnobX = kColumn1 + kKnobOffsetX,
	kScratchSpeedKnobY = kRow3 + kKnobOffsetY,
	kRootKeyKnobX = kColumn3 + kKnobOffsetX,
	kRootKeyKnobY = kRow5 + kKnobOffsetY,

	kHelpX = kColumn3 + 4,
	kHelpY = 421,
	kHelpWidth = 25,
	kHelpHeight = 25,

	kAboutSplashX = 13,
	kAboutSplashY = 64,
	kAboutSplashWidth = 137,
	kAboutSplashHeight = 28
};



#pragma mark -
#pragma mark class definitions
#pragma mark -

//-----------------------------------------------------------------------------
class TurntablistEditor final : public DfxGuiEditor
{
public:
	explicit TurntablistEditor(DGEditorListenerInstance inInstance);

	void OpenEditor() override;
	void PostOpenEditor() override;
	void CloseEditor() override;

	void parameterChanged(dfx::ParameterID inParameterID) override;
	void HandlePropertyChange(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned int inItemIndex) override;

	void dfxgui_Idle() override;
#if DRAG_N_DROP
	bool HandleEvent(EventHandlerCallRef inHandlerRef, EventRef inEvent) override;
#endif

	OSStatus LoadAudioFile(FSRef const& inAudioFileRef);

private:
	void InitializeSupportedAudioFileTypes();
#if DRAG_N_DROP
	bool IsSupportedAudioFileType(FSRef const& inFileRef) const;
#endif
	void HandleLoadButton();
	void HandleAudioFileChange();
	void HandlePlayButton(bool inPlay);
	void HandlePlayChange();
	void HandleMidiLearnButton(bool inLearn);
	void HandleMidiResetButton();

	void SetFileNameDisplay(CFStringRef inText);
	OSStatus NotifyAudioFileLoadError(OSStatus inErrorCode, FSRef const& inAudioFileRef) const;

	DGStaticTextDisplay* mAudioFileNameDisplay = nullptr;
	DGStaticTextDisplay* mAllParametersTextDisplay = nullptr;
	DGButton* mPlayButton = nullptr;
	DGAnimation* mScratchSpeedKnob = nullptr;

	std::vector<CFStringRef> mSupportedAudioFileExtensions;
	std::vector<OSType> mSupportedAudioFileTypeCodes;

#if DRAG_N_DROP
	OSStatus mDragAudioFileError = noErr;
	FSRef mDragAudioFileRef {};
#endif
};



//-----------------------------------------------------------------------------
class TurntablistScratchSlider final : public DGSlider
{
public:
	using DGSlider::DGSlider;

	VSTGUI::CMouseEventResult onMouseUp(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons) override
	{
		setValueNormalized(0.5f);
		return DGSlider::onMouseUp(inPos, inButtons);
	}

	CLASS_METHODS(TurntablistScratchSlider, DGSlider)
};



#pragma mark -
#pragma mark callbacks

//-----------------------------------------------------------------------------
static void TurntablistAboutButtonProc(long inValue)
{
	if (inValue > 0)
	{
#if __LP64__
		dfx::LaunchURL(PLUGIN_HOMEPAGE_URL);
#else
		if (auto const pluginBundleRef = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER)))
		{
			auto const aboutDict = dfx::MakeUniqueCFType(CFDictionaryCreateMutable(kCFAllocatorDefault, 3, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
			if (aboutDict)
			{
				auto valueString = reinterpret_cast<CFStringRef>(CFBundleGetValueForInfoDictionaryKey(pluginBundleRef, kCFBundleNameKey));
				if (valueString)
				{
					CFDictionaryAddValue(aboutDict.get(), kHIAboutBoxNameKey, valueString);
				}

				valueString = reinterpret_cast<CFStringRef>(CFBundleGetValueForInfoDictionaryKey(pluginBundleRef, CFSTR("CFBundleShortVersionString")));
				if (!valueString)
				{
					valueString = reinterpret_cast<CFStringRef>(CFBundleGetValueForInfoDictionaryKey(pluginBundleRef, kCFBundleVersionKey));
				}
				if (valueString)
				{
					CFDictionaryAddValue(aboutDict.get(), kHIAboutBoxVersionKey, valueString);
				}

				valueString = reinterpret_cast<CFStringRef>(CFBundleGetValueForInfoDictionaryKey(pluginBundleRef, CFSTR("NSHumanReadableCopyright")));
				if (valueString)
				{
					CFDictionaryAddValue(aboutDict.get(), kHIAboutBoxCopyrightKey, valueString);
				}

				HIAboutBox(aboutDict.get());
			}
		}
#endif
	}
}




#pragma mark -
#pragma mark initialization

//-----------------------------------------------------------------------------
DFX_EDITOR_ENTRY(TurntablistEditor)

//-----------------------------------------------------------------------------
// TurntablistEditor implementation
//-----------------------------------------------------------------------------
TurntablistEditor::TurntablistEditor(DGEditorListenerInstance inInstance)
:	DfxGuiEditor(inInstance)
{
	RegisterPropertyChange(kTurntablistProperty_AudioFile);
	RegisterPropertyChange(kTurntablistProperty_Play);

	InitializeSupportedAudioFileTypes();
}

//-----------------------------------------------------------------------------
/*
static CFStringRef const kSupportedAudioFileExtensions[] =
{
	CFSTR("aif"),
	CFSTR("aiff"),
	CFSTR("aifc"),
	CFSTR("wav"),
	CFSTR("sd2"),
	CFSTR("au"),
	CFSTR("snd"),
	CFSTR("mp3"),
	CFSTR("mpeg"),
	CFSTR("ac3"),
	CFSTR("aac"),
	CFSTR("adts"),
	CFSTR("m4a")
};
constexpr OSType kSupportedAudioFileTypeCodes[] =
{
	kAudioFileAIFFType,
	kAudioFileAIFCType,
	kAudioFileWAVEType,
	kAudioFileSoundDesigner2Type,
	kAudioFileNextType,
	kAudioFileMP3Type,
	kAudioFileAC3Type,
	kAudioFileAAC_ADTSType
};
*/

//-----------------------------------------------------------------------------
void TurntablistEditor::InitializeSupportedAudioFileTypes()
{
	UInt32 propertySize = 0;
	auto status = AudioFileGetGlobalInfoSize(kAudioFileGlobalInfo_ReadableTypes, 0, nullptr, &propertySize);
	assert(status == noErr);
	assert(propertySize > 0);
	if ((status == noErr) && (propertySize > 0))
	{
		auto const numAudioFileTypeCodes = propertySize / sizeof(decltype(mSupportedAudioFileTypeCodes)::value_type);
		mSupportedAudioFileTypeCodes.assign(numAudioFileTypeCodes, 0);
		status = AudioFileGetGlobalInfo(kAudioFileGlobalInfo_ReadableTypes, 0, nullptr, &propertySize, mSupportedAudioFileTypeCodes.data());
		assert(status == noErr);
	}

	for (auto audioFileTypeCode : mSupportedAudioFileTypeCodes)
	{
		CFArrayRef extensionsArray = nullptr;
		propertySize = sizeof(extensionsArray);
		status = AudioFileGetGlobalInfo(kAudioFileGlobalInfo_ExtensionsForType, sizeof(audioFileTypeCode), &audioFileTypeCode, &propertySize, &extensionsArray);
		assert(status == noErr);
		assert(extensionsArray);
		if ((status == noErr) && extensionsArray)
		{
			auto const cfReleaser = dfx::MakeUniqueCFType(extensionsArray);
//auto const audioFileType_bigEndian = CFSwapInt32HostToBig(audioFileTypeCode);
//std::fprintf(stderr, "\n\t%.*s\n", static_cast<int>(sizeof(audioFileType_bigEndian)), reinterpret_cast<char const*>(&audioFileType_bigEndian));
			auto const numExtensions = CFArrayGetCount(extensionsArray);
			for (CFIndex i = 0; i < numExtensions; i++)
			{
				auto const extension = static_cast<CFStringRef>(CFArrayGetValueAtIndex(extensionsArray, i));
				assert(extension);
				mSupportedAudioFileExtensions.push_back(extension);
				CFRetain(extension);
//CFShow(extension);
			}
		}
	}
}

#if DRAG_N_DROP
//-----------------------------------------------------------------------------
bool TurntablistEditor::IsSupportedAudioFileType(FSRef const& inFileRef) const
{
	bool result = false;

	constexpr LSRequestedInfo lsInfoFlags = kLSRequestExtension | kLSRequestTypeCreator;
	LSItemInfoRecord lsItemInfo {};
	auto const status = LSCopyItemInfoForRef(&inFileRef, lsInfoFlags, &lsItemInfo);
	if (status == noErr)
	{
		auto const cfReleaser = dfx::MakeUniqueCFType(lsItemInfo.extension);
		if (lsItemInfo.flags & kLSItemInfoIsPackage)
		{
			result = false;
		}
		else
		{
			result = false;
			if (lsItemInfo.extension)
			{
				for (auto const& supportedAudioFileExtension : mSupportedAudioFileExtensions)
				{
					if (CFStringCompare(lsItemInfo.extension, supportedAudioFileExtension, kCFCompareCaseInsensitive) == kCFCompareEqualTo)
					{
						result = true;
						break;
					}
				}
			}
			if (!result)
			{
				for (auto const audioFileTypeCode : mSupportedAudioFileTypeCodes)
				{
					if (lsItemInfo.filetype == audioFileTypeCode)
					{
						result = true;
						break;
					}
				}
			}
		}
	}

	return result;
}
#endif

//-----------------------------------------------------------------------------
void TurntablistEditor::OpenEditor()
{
	auto const sliderHandleImage = LoadImage("slider-handle.png");
	auto const knobImage = LoadImage("knob.png", kKnobFrames);
	auto const onOffButtonImage = LoadImage("on-off-button.png");
	auto const playButtonImage = LoadImage("play-button.png");
	auto const directionButtonImage = LoadImage("direction-button.png");
	auto const loopButtonImage = LoadImage("loop-button.png");
	auto const noteModeButtonImage = LoadImage("note-mode-button.png");
	auto const scratchModeButtonImage = LoadImage("scratch-mode-button.png");
	auto const helpButtonImage = LoadImage("help-button.png");


	// create controls
	DGRect pos;
	dfx::UniqueCFType<CFStringRef> helpText;
	auto const pluginBundleRef = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));


	// buttons

	DGButton* button = emplaceControl<DGToggleImageButton>(this, kParam_Power, kPowerButtonX, kPowerButtonY, onOffButtonImage);
	helpText.reset(CFCopyLocalizedStringFromTableInBundle(CFSTR("this switches the turntable power on or off"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Power parameter")));
	button->setHelpText(helpText.get());

#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
	button = emplaceControl<DGToggleImageButton>(this, kParam_Mute, kColumn3, kRow3, onOffButtonImage);
	helpText.reset(CFCopyLocalizedStringFromTableInBundle(CFSTR("this mutes the audio output"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Mute parameter")));
	button->setHelpText(helpText.get());
#endif

	button = emplaceControl<DGToggleImageButton>(this, kParam_Loop, kLoopButtonX, kLoopButtonY, loopButtonImage);
	helpText.reset(CFCopyLocalizedStringFromTableInBundle(CFSTR("if you enable this, the audio sample playback will continuously loop"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Loop parameter")));
	button->setHelpText(helpText.get());

	pos.set(kDirectionButtonX, kDirectionButtonY, directionButtonImage->getWidth(), directionButtonImage->getHeight() / kNumScratchDirections);
	button = emplaceControl<DGButton>(this, kParam_Direction, pos, directionButtonImage, DGButton::Mode::Increment);
	helpText.reset(CFCopyLocalizedStringFromTableInBundle(CFSTR("this changes playback direction of the audio sample, regular or reverse"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Direction parameter")));
	button->setHelpText(helpText.get());

	pos.set(kNoteModeButtonX, kNoteModeButtonY, noteModeButtonImage->getWidth(), noteModeButtonImage->getHeight() / kNumNoteModes);
	button = emplaceControl<DGButton>(this, kParam_NoteMode, pos, noteModeButtonImage, DGButton::Mode::Increment);
	helpText.reset(CFCopyLocalizedStringFromTableInBundle(CFSTR("This toggles between \"reset mode\" (notes restart playback from the beginning of the audio sample) and \"resume mode\" (notes trigger playback from where the audio sample last stopped)"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Note Mode parameter")));
	button->setHelpText(helpText.get());

	button = emplaceControl<DGToggleImageButton>(this, kParam_NotePowerTrack, kNotePowerTrackButtonX, kNotePowerTrackButtonY, onOffButtonImage);
	helpText.reset(CFCopyLocalizedStringFromTableInBundle(CFSTR("enabling this will cause note-on and note-off messages to be mapped to turntable power on and off for an interesting effect"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Note-Power Track parameter")));
	button->setHelpText(helpText.get());

	button = emplaceControl<DGToggleImageButton>(this, kParam_KeyTracking, kKeyTrackButtonX, kKeyTrackButtonY, onOffButtonImage);
	helpText.reset(CFCopyLocalizedStringFromTableInBundle(CFSTR("this switches key tracking on or off (key tracking means that the pitch and speed of the audio sample playback are transposed in relation to the pitch of the MIDI note)"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Key Tracking parameter")));
	button->setHelpText(helpText.get());

	pos.set(kScratchModeButtonX, kScratchModeButtonY, scratchModeButtonImage->getWidth(), scratchModeButtonImage->getHeight() / kNumScratchModes);
	button = emplaceControl<DGButton>(this, kParam_ScratchMode, pos, scratchModeButtonImage, DGButton::Mode::Increment);
	helpText.reset(CFCopyLocalizedStringFromTableInBundle(CFSTR("this toggles between scrub mode and spin mode, which affects the behavior of the Scratch Amount parameter"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Scratch Mode parameter")));
	button->setHelpText(helpText.get());

	class TurntablistButton final : public DGButton
	{
	public:
		using DGButton::DGButton;
		/*void onDragEnter(IDataPackage* inDrag, VSTGUI::CPoint const& inPos) override
		{
			getFrame()->setCursor(kCursorCopy);
		}
		bool onDrop(IDataPackage* inDrag, VSTGUI::CPoint const& inPos) override
		{
			return true;
		}*/
		CLASS_METHODS(TurntablistButton, DGButton)
	};
	pos.set(kLoadButtonX, kLoadButtonY, onOffButtonImage->getWidth(), onOffButtonImage->getHeight() / 2);
	button = emplaceControl<TurntablistButton>(this, pos, onOffButtonImage, 2, DGButton::Mode::Momentary);
	button->setUserReleaseProcedure(std::bind(&TurntablistEditor::HandleLoadButton, this), true);
	helpText.reset(CFCopyLocalizedStringFromTableInBundle(CFSTR("choose an audio file to load up onto the \"turntable\""), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Open Audio File button")));
	button->setHelpText(helpText.get());

	mPlayButton = emplaceControl<DGToggleImageButton>(this, kPlayButtonX, kPlayButtonY, playButtonImage);
	mPlayButton->setUserProcedure(std::bind_front(&TurntablistEditor::HandlePlayButton, this));
	helpText.reset(CFCopyLocalizedStringFromTableInBundle(CFSTR("use this to start or stop the audio sample playback"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Play button")));
	mPlayButton->setHelpText(helpText.get());

	button = emplaceControl<DGToggleImageButton>(this, kMidiLearnX, kMidiLearnY, onOffButtonImage);
	button->setUserProcedure(std::bind_front(&TurntablistEditor::HandleMidiLearnButton, this));
	helpText.reset(CFCopyLocalizedStringFromTableInBundle(CFSTR("This switches MIDI learn mode on or off.  When MIDI learn is on, you can click on a parameter control to enable that parameter as the \"learner\" for incoming MIDI CC messages."), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the MIDI Learn button")));
	button->setHelpText(helpText.get());

	pos.set(kMidiResetX, kMidiResetY, onOffButtonImage->getWidth(), onOffButtonImage->getHeight() / 2);
	button = emplaceControl<DGButton>(this, pos, onOffButtonImage, 2, DGButton::Mode::Momentary);
	button->setUserProcedure([this](long inValue)
	{
		if (inValue != 0)
		{
			HandleMidiResetButton();
		}
	});
	helpText.reset(CFCopyLocalizedStringFromTableInBundle(CFSTR("this removes all of your MIDI CC -> parameter assignments"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the MIDI Reset button")));
	button->setHelpText(helpText.get());

	pos.set(kAboutSplashX, kAboutSplashY, kAboutSplashWidth, kAboutSplashHeight);
	button = emplaceControl<DGButton>(this, pos, nullptr, 2, DGButton::Mode::Increment);
	button->setUserProcedure(std::bind_front(&TurntablistAboutButtonProc));
	//button->setHelpText(CFSTR("click here to go to the "PLUGIN_CREATOR_NAME_STRING" web site"));
	helpText.reset(CFCopyLocalizedStringFromTableInBundle(CFSTR("click here to go to the Destroy FX web site"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the About hot-spot")));
	button->setHelpText(helpText.get());

	pos.set(kHelpX, kHelpY, helpButtonImage->getWidth(), helpButtonImage->getHeight() / 2);
	button = emplaceControl<DGButton>(this, pos, helpButtonImage, 2, DGButton::Mode::Momentary);
	button->setUserReleaseProcedure(std::bind(&dfx::LaunchDocumentation), true);
	helpText.reset(CFCopyLocalizedStringFromTableInBundle(CFSTR("view the full manual"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Help button")));
	button->setHelpText(helpText.get());
/*
Rect buttonRect = pos.convertToRect();
ControlButtonContentInfo buttonContentInfo;
buttonContentInfo.contentType = kControlContentIconRef;
OSStatus buttonStat = GetIconRef(kOnSystemDisk, kSystemIconsCreator, kHelpIcon, &(buttonContentInfo.u.iconRef));
ControlRef buttonControl = nullptr;
buttonStat = CreateRoundButtonControl(GetCarbonWindow(), &buttonRect, kControlSizeNormal, &buttonContentInfo, &buttonControl);
*/


	// knobs
	auto const knobWidth = knobImage->getFrameSize().x;
	auto const knobHeight = knobImage->getFrameSize().y;

	pos.set(kPitchRangeKnobX, kPitchRangeKnobY, knobWidth, knobHeight);
	auto knob = emplaceControl<DGAnimation>(this, kParam_PitchRange, pos, knobImage);
	helpText.reset(CFCopyLocalizedStringFromTableInBundle(CFSTR("this controls the range of pitch adjustment values that the Pitch Shift parameter offers"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Pitch Range parameter")));
	knob->setHelpText(helpText.get());

	auto const scratchSpeedParam = (getparameter_i(kParam_ScratchMode) == kScratchMode_Scrub) ? kParam_ScratchSpeed_scrub : kParam_ScratchSpeed_spin;
	pos.set(kScratchSpeedKnobX, kScratchSpeedKnobY, knobWidth, knobHeight);
	mScratchSpeedKnob = emplaceControl<DGAnimation>(this, scratchSpeedParam, pos, knobImage);
	helpText.reset(CFCopyLocalizedStringFromTableInBundle(CFSTR("this sets the speed of the scratching effect that the Scratch Amount parameter produces"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Scratch Speed parameter")));
	mScratchSpeedKnob->setHelpText(helpText.get());

	pos.set(kSpinUpKnobX, kSpinUpKnobY, knobWidth, knobHeight);
	knob = emplaceControl<DGAnimation>(this, kParam_SpinUpSpeed, pos, knobImage);
	helpText.reset(CFCopyLocalizedStringFromTableInBundle(CFSTR("this controls how quickly the audio playback \"spins up\" when the turntable power turns on"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Spin Up Speed parameter")));
	knob->setHelpText(helpText.get());

	pos.set(kSpinDownKnobX, kSpinDownKnobY, knobWidth, knobHeight);
	knob = emplaceControl<DGAnimation>(this, kParam_SpinDownSpeed, pos, knobImage);
	helpText.reset(CFCopyLocalizedStringFromTableInBundle(CFSTR("this controls how quickly the audio playback \"spins down\" when the turntable power turns off"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Spin Down Speed parameter")));
	knob->setHelpText(helpText.get());

	pos.set(kRootKeyKnobX, kRootKeyKnobY, knobWidth, knobHeight);
	knob = emplaceControl<DGAnimation>(this, kParam_RootKey, pos, knobImage);
	helpText.reset(CFCopyLocalizedStringFromTableInBundle(CFSTR("when Key Track is enabled, this controls the MIDI key around which the transposition is centered"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Root Key parameter")));
	knob->setHelpText(helpText.get());

#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
	pos.set(kColumn2, kRow3 + kKnobOffsetY, knobWidth, knobHeight);
	knob = emplaceControl<DGAnimation>(this, kParam_Volume, pos, knobImage);
	helpText.reset(CFCopyLocalizedStringFromTableInBundle(CFSTR("this controls the overall volume of the audio output"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Volume parameter")));
	knob->setHelpText(helpText.get());
#endif


	// sliders

	// scratch amount
	pos.set(kScratchAmountSliderX, kSliderY, kSliderWidth, kSliderHeight);
	DGSlider* slider = emplaceControl<TurntablistScratchSlider>(this, kParam_ScratchAmount, pos, dfx::kAxis_Vertical, sliderHandleImage);
	helpText.reset(CFCopyLocalizedStringFromTableInBundle(CFSTR("This slider is what does the actual scratching.  In scrub mode, the slider represents time.  In spin mode, the slider represents forward and backward speed."), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Scratch Amount parameter")));
	slider->setHelpText(helpText.get());

	// pitch shift
	pos.set(kPitchShiftSliderX, kSliderY, kSliderWidth, kSliderHeight);
	slider = emplaceControl<DGSlider>(this, kParam_PitchShift, pos, dfx::kAxis_Vertical, sliderHandleImage);
	helpText.reset(CFCopyLocalizedStringFromTableInBundle(CFSTR("changes the audio playback pitch between +/- the Pitch Range value"), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("pop-up help text for the Pitch Shift parameter")));
	slider->setHelpText(helpText.get());


	// text displays

	// universal parameter value display
	pos.set(kDisplayX, kDisplayY, kFileNameWidth, kFileNameHeight);
	mAllParametersTextDisplay = emplaceControl<DGStaticTextDisplay>(this, pos, nullptr, dfx::TextAlignment::Center, kTurntablistFontSize, DGColor::kWhite);

	// audio file name display
	pos.set(kFileNameX, kFileNameY, kFileNameWidth, kFileNameHeight);
	mAudioFileNameDisplay = emplaceControl<DGStaticTextDisplay>(this, pos, nullptr, dfx::TextAlignment::Center, kTurntablistFontSize, DGColor::kWhite);
	HandleAudioFileChange();



#if DRAG_N_DROP
	constexpr EventTypeSpec dragEvents[] = {
//											{ kEventClassControl, kEventControlInitialize }, 
											{ kEventClassControl, kEventControlDragEnter }, 
											{ kEventClassControl, kEventControlDragWithin },
											{ kEventClassControl, kEventControlDragLeave }, 
											{ kEventClassControl, kEventControlDragReceive }
										   };
	WantEventTypes(GetControlEventTarget(GetCarbonPane()), GetEventTypeCount(dragEvents), dragEvents);
	SetControlDragTrackingEnabled(GetCarbonPane(), true);
	SetAutomaticControlDragTrackingEnabledForWindow(GetCarbonWindow(), true);
#endif
}

//-----------------------------------------------------------------------------
void TurntablistEditor::PostOpenEditor()
{
	// initialize the button's current value once the button has been fully created
	HandlePlayChange();
}

//-----------------------------------------------------------------------------
void TurntablistEditor::CloseEditor()
{
	mAudioFileNameDisplay = nullptr;
	mAllParametersTextDisplay = nullptr;
	mPlayButton = nullptr;
	mScratchSpeedKnob = nullptr;
}

//-----------------------------------------------------------------------------
void TurntablistEditor::HandlePropertyChange(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned int inItemIndex)
{
	switch (inPropertyID)
	{
		case kTurntablistProperty_AudioFile:
			return HandleAudioFileChange();
		case kTurntablistProperty_Play:
			return HandlePlayChange();
		default:
			break;
	}
}



#pragma mark -
#pragma mark audio file chooser

//-----------------------------------------------------------------------------
static auto DFX_CopyFileNameString(FSRef const& inFileRef)
{
	CFStringRef fileName = nullptr;
	auto const status = LSCopyDisplayNameForRef(&inFileRef, &fileName);
	return dfx::MakeUniqueCFType<CFStringRef>((status == noErr) ? fileName : nullptr);
}

#ifdef USE_LIBSNDFILE
	#include "sndfile.h"	// for the libsndfile error code constants
#else
	#include <AudioToolbox/AudioToolbox.h>	// for ExtAudioFile error code constants
#endif
//-----------------------------------------------------------------------------
OSStatus TurntablistEditor::NotifyAudioFileLoadError(OSStatus inErrorCode, FSRef const& inAudioFileRef) const
{
	auto const pluginBundleRef = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));
	if (!pluginBundleRef)
	{
		return coreFoundationUnknownErr;
	}

	auto titleString = dfx::MakeUniqueCFType(CFCopyLocalizedStringFromTableInBundle(CFSTR("The file \\U201C%@\\U201D could not be loaded."), 
																					CFSTR("Localizable"), pluginBundleRef, 
																					CFSTR("title for the dialog telling you that the audio file could not be loaded")));
	auto const audioFileName = DFX_CopyFileNameString(inAudioFileRef);
	if (audioFileName)
	{
		auto const titleStringWithFileName = CFStringCreateWithFormat(kCFAllocatorDefault, nullptr, titleString.get(), audioFileName.get());
		if (titleStringWithFileName)
		{
			titleString.reset(titleStringWithFileName);
		}
	}

	auto messageString = dfx::MakeUniqueCFType(CFCopyLocalizedStringFromTableInBundle(CFSTR("An error was encountered while trying to load audio data from the file.  %@"), 
																					  CFSTR("Localizable"), pluginBundleRef, 
																					  CFSTR("explanation for the dialog telling you that the audio file could not be loaded")));
	dfx::UniqueCFType<CFStringRef> errorDescriptionString;
	switch (inErrorCode)
	{
#ifdef USE_LIBSNDFILE
		case SF_ERR_UNRECOGNISED_FORMAT:
			errorDescriptionString.reset(CFCopyLocalizedStringFromTableInBundle(CFSTR("The audio data is in a format that could not be recognized."), 
																				CFSTR("Localizable"), pluginBundleRef, 
																				CFSTR("libsndfile SF_ERR_UNRECOGNISED_FORMAT description")));
			break;
		case SF_ERR_SYSTEM:
			errorDescriptionString.reset(CFCopyLocalizedStringFromTableInBundle(CFSTR("A system error occurred while trying to read the file."), 
																				CFSTR("Localizable"), pluginBundleRef, 
																				CFSTR("libsndfile SF_ERR_SYSTEM description")));
			break;
		case SF_ERR_MALFORMED_FILE:
			errorDescriptionString.reset(CFCopyLocalizedStringFromTableInBundle(CFSTR("The audio data is not readable due to being malformed."), 
																				CFSTR("Localizable"), pluginBundleRef, 
																				CFSTR("libsndfile SF_ERR_MALFORMED_FILE description")));
			break;
		case SF_ERR_UNSUPPORTED_ENCODING:
			errorDescriptionString.reset(CFCopyLocalizedStringFromTableInBundle(CFSTR("The audio data is encoded in a format that is not supported."), 
																				CFSTR("Localizable"), pluginBundleRef, 
																				CFSTR("libsndfile SF_ERR_UNSUPPORTED_ENCODING description")));
			break;
#else
		case unsupportedOSErr:
			errorDescriptionString.reset(CFCopyLocalizedStringFromTableInBundle(CFSTR(PLUGIN_NAME_STRING " requires Mac OS X version 10.4 (Tiger) or higher."), 
																				CFSTR("Localizable"), pluginBundleRef, 
																				CFSTR("unsupportedOSErr description")));
			break;
		// property
		case kExtAudioFileError_InvalidProperty:
		case kExtAudioFileError_InvalidPropertySize:
		case kAudioFileUnsupportedPropertyError:
		case kAudioFileBadPropertySizeError:
		case kAudioConverterErr_PropertyNotSupported:
		// format
		case kExtAudioFileError_NonPCMClientFormat:
		case kExtAudioFileError_InvalidChannelMap:
		case kExtAudioFileError_InvalidDataFormat:
		case kExtAudioFileError_MaxPacketSizeUnknown:
		case kAudioFileUnsupportedDataFormatError:
		case kAudioConverterErr_InvalidInputSize:
		case kAudioConverterErr_InvalidOutputSize:
		case kAudioConverterErr_RequiresPacketDescriptionsError:
		case kAudioConverterErr_InputSampleRateOutOfRange:
		case kAudioConverterErr_OutputSampleRateOutOfRange:
		// operation
		case kExtAudioFileError_InvalidOperationOrder:
		case kExtAudioFileError_InvalidSeek:
		case kAudioFileOperationNotSupportedError:
		// file failure
		case kAudioFileInvalidChunkError:
		case kAudioFileDoesNotAllow64BitDataSizeError:
		case kAudioFileInvalidPacketOffsetError:
		case kAudioFileInvalidFileError:
		// misc
		case kAudioFileUnsupportedFileTypeError:
		case kAudioFilePermissionsError:
		case kAudioFileNotOptimizedError:
		// unknown
		case kAudioFileUnspecifiedError:
		case kAudioConverterErr_UnspecifiedError:
#endif
		default:
			errorDescriptionString.reset(CFStringCreateWithFormat(kCFAllocatorDefault, nullptr, CFSTR("error code %d"), static_cast<int>(inErrorCode)));
			break;
	}
	if (errorDescriptionString)
	{
		auto const messageStringWithError = CFStringCreateWithFormat(kCFAllocatorDefault, nullptr, messageString.get(), errorDescriptionString.get());
		if (messageStringWithError)
		{
			messageString.reset(messageStringWithError);
		}
	}

	auto const alert = [[NSAlert alloc] init];
	alert.messageText = (__bridge NSString*)titleString.get();
	alert.informativeText = (__bridge NSString*)messageString.get();
	alert.alertStyle = NSAlertStyleCritical;
	auto const window = [this]() -> NSWindow*
	{
		if (auto const frame = getFrame())
		{
			if (auto const platformFrame = frame->getPlatformFrame(); platformFrame && (platformFrame->getPlatformType() == VSTGUI::PlatformType::kNSView))
			{
				if (auto const nsView = (__bridge NSView*)(platformFrame->getPlatformRepresentation()))
				{
					return nsView.window;
				}
			}
		}
		return nil;
	}();
	if (window)
	{
		[alert beginSheetModalForWindow:window completionHandler:nil];
	}
	else
	{
		[alert runModal];
	}
	return noErr;
#if 0
	auto const titleCString = dfx::CreateCStringFromCFString(titleString.get());
	auto const dialog = VSTGUI::makeOwned<DGDialog>(DGRect(0, 0, 200, 100), titleCString.get());
	auto const success = dialog->runModal(getFrame());
	return success ? noErr : -1;
#endif
}

//-----------------------------------------------------------------------------
void TurntablistEditor::HandleLoadButton()
{
	VSTGUI::SharedPointer<VSTGUI::CNewFileSelector> fileSelector(VSTGUI::CNewFileSelector::create(getFrame(), VSTGUI::CNewFileSelector::kSelectFile), false);
	if (fileSelector)
	{
		for (auto const& supportedAudioFileExtension : mSupportedAudioFileExtensions)
		{
			auto const supportedAudioFileExtensionC = dfx::CreateCStringFromCFString(supportedAudioFileExtension);
			if (supportedAudioFileExtensionC)
			{
				fileSelector->addFileExtension(VSTGUI::CFileExtension("audio file", supportedAudioFileExtensionC.get()));
			}
		}

		fileSelector->run([this](VSTGUI::CNewFileSelector* inFileSelector)
						  {
							  if (auto const filePath = inFileSelector->getSelectedFile(0))
							  {
								  if (auto const fileURL = PathToCFURL(filePath))
								  {
									  FSRef fileRef {};
									  if (CFURLGetFSRef(fileURL.get(), &fileRef))
									  {
										  auto const status = LoadAudioFile(fileRef);
										  if (status != noErr)
										  {
											  NotifyAudioFileLoadError(status, fileRef);
										  }
									  }
								  }
							  }
						  });
	}
}

//-----------------------------------------------------------------------------
OSStatus TurntablistEditor::LoadAudioFile(FSRef const& inAudioFileRef)
{
	AliasHandle aliasHandle = nullptr;
	auto const error = FSNewAlias(nullptr, &inAudioFileRef, &aliasHandle);
	if (error != noErr)
	{
		return error;
	}
	if (!aliasHandle)
	{
		return nilHandleErr;
	}

	UniqueAliasHandle const ownedAlias(aliasHandle);
	auto const dataSize = GetAliasSize(aliasHandle);
	return dfxgui_SetProperty(kTurntablistProperty_AudioFile, kAudioUnitScope_Global, 0, *aliasHandle, dataSize);
}



#pragma mark -
#pragma mark features

//-----------------------------------------------------------------------------
static dfx::UniqueCFType<CFStringRef> DFX_CopyNameAndVersionString()
{
	auto const pluginBundleRef = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));
	if (!pluginBundleRef)
	{
		return {};
	}

	auto const nameString = reinterpret_cast<CFStringRef>(CFBundleGetValueForInfoDictionaryKey(pluginBundleRef, kCFBundleNameKey));
	auto versionString = reinterpret_cast<CFStringRef>(CFBundleGetValueForInfoDictionaryKey(pluginBundleRef, CFSTR("CFBundleShortVersionString")));
	if (!versionString)
	{
		versionString = reinterpret_cast<CFStringRef>(CFBundleGetValueForInfoDictionaryKey(pluginBundleRef, kCFBundleVersionKey));
	}
	dfx::UniqueCFType<CFStringRef> nameAndVersionString;
	if (nameString && versionString)
	{
		nameAndVersionString.reset(CFStringCreateWithFormat(kCFAllocatorDefault, nullptr, CFSTR("%@  %@"), nameString, versionString));
	}
	if (!nameAndVersionString)
	{
		if (nameString)
		{
			nameAndVersionString.reset(nameString);
			CFRetain(nameString);
		}
		else if (versionString)
		{
			nameAndVersionString.reset(versionString);
			CFRetain(versionString);
		}
	}

	return nameAndVersionString;
}

//-----------------------------------------------------------------------------
void TurntablistEditor::HandleAudioFileChange()
{
	dfx::UniqueCFType<CFStringRef> displayString;
	size_t dataSize = 0;
	dfx::PropertyFlags propertyFlags {};
	auto status = dfxgui_GetPropertyInfo(kTurntablistProperty_AudioFile, kAudioUnitScope_Global, 0, 
										 dataSize, propertyFlags);
	if ((status == dfx::kStatus_NoError) && (dataSize > 0))
	{
		if (auto const aliasHandle = reinterpret_cast<AliasHandle>(NewHandle(dataSize)))
		{
			UniqueAliasHandle const ownedAlias(aliasHandle);
			status = dfxgui_GetProperty(kTurntablistProperty_AudioFile, kAudioUnitScope_Global, 0, 
										*aliasHandle, dataSize);
			assert(status == dfx::kStatus_NoError);
			if (status == dfx::kStatus_NoError)
			{
				FSRef audioFileRef {};
				Boolean wasChanged {};
				status = FSResolveAlias(nullptr, aliasHandle, &audioFileRef, &wasChanged);
				assert(status == noErr);
				if (status == noErr)
				{
					displayString = DFX_CopyFileNameString(audioFileRef);
				}
			}
		}
	}

	if (!displayString)
	{
		displayString = DFX_CopyNameAndVersionString();
	}

	if (displayString)
	{
		SetFileNameDisplay(displayString.get());
	}
}

//-----------------------------------------------------------------------------
void TurntablistEditor::HandlePlayButton(bool inPlay)
{
	if (mAllParametersTextDisplay)
	{
		auto const displayString = inPlay ? CFSTR("playback:  start") : CFSTR("playback:  stop");
		mAllParametersTextDisplay->setCFText(displayString);
	}

	Boolean const play_fixedSize = inPlay;
	[[maybe_unused]] auto const status = dfxgui_SetProperty(kTurntablistProperty_Play, 
															kAudioUnitScope_Global, 0, 
															&play_fixedSize, sizeof(play_fixedSize));
	assert(status == dfx::kStatus_NoError);
}

//-----------------------------------------------------------------------------
void TurntablistEditor::HandlePlayChange()
{
	if (!mPlayButton)
	{
		return;
	}

	auto const play = dfxgui_GetProperty<Boolean>(kTurntablistProperty_Play);
	assert(play.has_value());
	if (play)
	{
		mPlayButton->setValue_i(*play ? 1 : 0);
		if (mPlayButton->isDirty())
		{
			mPlayButton->invalid();
		}
	}
}

//-----------------------------------------------------------------------------
void TurntablistEditor::SetFileNameDisplay(CFStringRef inText)
{
	assert(inText);
	if (mAudioFileNameDisplay)
	{
		mAudioFileNameDisplay->setCFText(inText);
	}
}

//-----------------------------------------------------------------------------
void TurntablistEditor::HandleMidiLearnButton(bool inLearn)
{
	if (mAllParametersTextDisplay)
	{
		auto const displayString = inLearn ? CFSTR("MIDI learn:  on") : CFSTR("MIDI learn:  off");
		mAllParametersTextDisplay->setCFText(displayString);
	}

	setmidilearning(inLearn);
}

//-----------------------------------------------------------------------------
void TurntablistEditor::HandleMidiResetButton()
{
	if (mAllParametersTextDisplay)
	{
		mAllParametersTextDisplay->setCFText(CFSTR("MIDI reset"));
	}

	resetmidilearn();
}

#if DRAG_N_DROP
//-----------------------------------------------------------------------------
bool TurntablistEditor::HandleEvent(EventHandlerCallRef inHandlerRef, EventRef inEvent)
{
	if (GetEventClass(inEvent) == kEventClassControl)
	{
		UInt32 inEventKind = GetEventKind(inEvent);

		ControlRef carbonControl = nullptr;
		OSStatus status = GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, nullptr, sizeof(carbonControl), nullptr, &carbonControl);
		if (status != noErr)
		{
			carbonControl = nullptr;
		}

/*
		if (inEventKind == kEventControlInitialize)
		{
			if (carbonControl)
			{
				status = HIViewChangeFeatures(carbonControl, kControlSupportsDragAndDrop, 0);
			}
			return true;
		}
*/

		if (carbonControl && (carbonControl == GetCarbonPane()))
		{
			DragRef drag = nullptr;
			switch (inEventKind)
			{
				case kEventControlDragEnter:
				case kEventControlDragWithin:
				case kEventControlDragLeave:
				case kEventControlDragReceive:
					status = GetEventParameter(inEvent, kEventParamDragRef, typeDragRef, nullptr, sizeof(drag), nullptr, &drag);
					if ((status != noErr) || !drag)
					{
						return false;
					}
					break;
				default:
					break;
			}

			switch (inEventKind)
			{
				case kEventControlDragEnter:
					{
//std::fprintf(stderr, "kEventControlDragEnter\n");
						getBackgroundControl()->setDragActive(true);
						mDragAudioFileError = noErr;
						constexpr Boolean acceptDrop = true;
						status = SetEventParameter(inEvent, kEventParamControlWouldAcceptDrop, typeBoolean, sizeof(acceptDrop), &acceptDrop);
/*
						dfx::UniqueOpaqueType<RgnHandle, DisposeRgn> const dragRegion(NewRgn());
						if (dragRegion)
						{
							Rect dragRegionBounds {};
							GetControlBounds(carbonControl, &dragRegionBounds);
							RectRgn(dragRegion.get(), &dragRegionBounds);

							CGrafPtr oldPort = nullptr;
							GetPort(&oldPort);
							SetPortWindowPort(GetCarbonWindow());

							status = ShowDragHilite(drag, dragRegion.get(), true);

							if (oldPort)
							{
								SetPort(oldPort);
							}
						}
*/
//						HiliteControl(carbonControl, 1);
//						HIViewSetHilite(carbonControl, 1);//kHIViewIndicatorPart);
//						HIViewSetNeedsDisplay(carbonControl, true);
						return true;
					}

				case kEventControlDragWithin:
//std::fprintf(stderr, "kEventControlDragWithin\n");
					return true;

				case kEventControlDragLeave:
//std::fprintf(stderr, "kEventControlDragLeave\n");
//					status = HideDragHilite(drag);
					getBackgroundControl()->setDragActive(false);
//					HiliteControl(carbonControl, kControlNoPart);
//					HIViewSetHilite(carbonControl, kHIViewNoPart);
//					HIViewSetNeedsDisplay(carbonControl, true);
					return true;

				case kEventControlDragReceive:
					{
//std::fprintf(stderr, "kEventControlDragReceive\n");
//						status = HideDragHilite(drag);
						getBackgroundControl()->setDragActive(false);

						bool foundFile = false;
						memset(&mDragAudioFileRef, 0, sizeof(mDragAudioFileRef));

						UInt16 numDragItems = 0;
						status = CountDragItems(drag, &numDragItems);
						if ((status == noErr) && (numDragItems > 0))
						{
							for (UInt16 dragItemIndex = 1; dragItemIndex <= numDragItems; dragItemIndex++)
							{
								DragItemRef dragItem = 0;
								status = GetDragItemReferenceNumber(drag, dragItemIndex, &dragItem);
								if (status == noErr)
								{
#if 0
									UInt16 numDragFlavors = 0;
									status = CountDragItemFlavors(drag, dragItem, &numDragFlavors);
									if ((status == noErr) && (numDragFlavors > 0))
									{
										for (UInt16 flavorIndex = 1; flavorIndex <= numDragFlavors; flavorIndex++)
										{
											FlavorType dragFlavorType = 0;
											status = GetFlavorType(drag, dragItem, flavorIndex, &dragFlavorType);
											if (status == noErr)
											{
												Size dragFlavorDataSize = 0;
												status = GetFlavorDataSize(drag, dragItem, dragFlavorType, &dragFlavorDataSize);
FlavorType const dragFlavorType_bigEndian = CFSwapInt32HostToBig(dragFlavorType);
std::fprintf(stderr, "flavor = '%.4s', size = %ld\n", reinterpret_cast<char const*>(&dragFlavorType_bigEndian), dragFlavorDataSize);
											}
										}
									}
#endif
									FlavorType dragFlavorType = typeFileURL;
									Size dragFlavorDataSize = 0;
									dfx::UniqueCFType<CFURLRef> fileURL;
									status = GetFlavorDataSize(drag, dragItem, dragFlavorType, &dragFlavorDataSize);
									if ((status == noErr) && (dragFlavorDataSize > 0))
									{
										dfx::UniqueMemoryBlock<UInt8> const fileUrlFlavorData(dragFlavorDataSize);
										if (fileUrlFlavorData)
										{
											status = GetFlavorData(drag, dragItem, dragFlavorType, fileUrlFlavorData.get(), &dragFlavorDataSize, 0);
											if (status == noErr)
											{
//std::fprintf(stderr, "file URL = %.*s\n", dragFlavorDataSize, fileUrlFlavorData.get());
												fileURL.reset(CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, fileUrlFlavorData.get(), dragFlavorDataSize, false));
												if (fileURL)
												{
													auto const success = CFURLGetFSRef(fileURL.get(), &mDragAudioFileRef);
													if (success)
													{
														foundFile = true;
													}
												}
											}
										}
									}
									if (!foundFile)
									{
										dragFlavorType = kDragFlavorTypeHFS;
										dragFlavorDataSize = 0;
										status = GetFlavorDataSize(drag, dragItem, dragFlavorType, &dragFlavorDataSize);
										if ((status == noErr) && (static_cast<size_t>(dragFlavorDataSize) >= sizeof(HFSFlavor)))
										{
											dfx::UniqueMemoryBlock<HFSFlavor> const hfsFlavorData(dragFlavorDataSize);
											if (hfsFlavorData)
											{
												status = GetFlavorData(drag, dragItem, dragFlavorType, hfsFlavorData.get(), &dragFlavorDataSize, 0);
												if (status == noErr)
												{
													status = FSpMakeFSRef(&(hfsFlavorData->fileSpec), &mDragAudioFileRef);
													if (status == noErr)
													{
														foundFile = true;
													}
												}
											}
										}
									}
									bool result = false;
									if (foundFile)
									{
										if (IsSupportedAudioFileType(mDragAudioFileRef))
										{
											status = LoadAudioFile(mDragAudioFileRef);
											if (status == noErr)
											{
												result = true;
											}
											else
											{
//												NotifyAudioFileLoadError(status, mDragAudioFileRef);
												mDragAudioFileError = status;
											}
										}
										else
										{
											if (!fileURL)
											{
												fileURL.reset(CFURLCreateFromFSRef(kCFAllocatorDefault, &mDragAudioFileRef));
											}
											if (fileURL)
											{
												if (CFURLIsAUPreset(fileURL.get()))
												{
													status = RestoreAUStateFromPresetFile(dfxgui_GetEffectInstance(), fileURL.get());
													if (status == noErr)
													{
														result = true;
													}
												}
											}
										}
									}
									return result;
								}
							}
						}

						return false;
					}

				default:
					break;
			}
		}
	}

	// let the parent implementation do its thing
	return DfxGuiEditor::HandleEvent(inHandlerRef, inEvent);
}
#endif



#pragma mark -
#pragma mark parameter -> string conversions

//-----------------------------------------------------------------------------
static dfx::UniqueCFType<CFStringRef> DFX_CopyAUParameterName(AudioUnit inAUInstance, dfx::ParameterID inParameterID)
{
	assert(inAUInstance);

	AudioUnitParameterInfo parameterInfo {};
	UInt32 dataSize = sizeof(parameterInfo);
	auto const status = AudioUnitGetProperty(inAUInstance, kAudioUnitProperty_ParameterInfo, kAudioUnitScope_Global, inParameterID, &parameterInfo, &dataSize);
	if (status == noErr)
	{
		auto const parameterName = parameterInfo.cfNameString;
		auto const flags = parameterInfo.flags;
		if (parameterName && (flags & kAudioUnitParameterFlag_HasCFNameString))
		{
			if (!(flags & kAudioUnitParameterFlag_CFNameRelease))
			{
				CFRetain(parameterName);
			}
			return dfx::MakeUniqueCFType(parameterName);
		}
		return dfx::MakeUniqueCFType(CFStringCreateWithCString(kCFAllocatorDefault, parameterInfo.name, kCFStringEncodingUTF8));
	}

	return {};
}

//-----------------------------------------------------------------------------
static dfx::UniqueCFType<CFArrayRef> DFX_CopyAUParameterValueStrings(AudioUnit inAUInstance, dfx::ParameterID inParameterID)
{
	assert(inAUInstance);
	CFArrayRef strings = nullptr;
	UInt32 dataSize = sizeof(strings);
	auto const status = AudioUnitGetProperty(inAUInstance, kAudioUnitProperty_ParameterValueStrings, kAudioUnitScope_Global, inParameterID, &strings, &dataSize);
	return (status == noErr) ? dfx::MakeUniqueCFType(strings) : nullptr;
}

//-----------------------------------------------------------------------------
void TurntablistEditor::parameterChanged(dfx::ParameterID inParameterID)
{
	auto const value = getparameter_f(inParameterID);
	auto const value_i = (value >= 0.f) ? static_cast<int>(value + DfxParam::kIntegerPadding) : static_cast<int>(value - DfxParam::kIntegerPadding);

	if ((inParameterID == kParam_ScratchMode) && mScratchSpeedKnob)
	{
		auto const newParameterID = (value_i == kScratchMode_Scrub) ? kParam_ScratchSpeed_scrub : kParam_ScratchSpeed_spin;
		mScratchSpeedKnob->setParameterID(newParameterID);
	}

	if (!mAllParametersTextDisplay)
	{
		return;
	}
	auto const getMouseDownView = [this]() -> VSTGUI::CView*  // reimplementation of protected CViewContainer method
	{
		if (VSTGUI::CView* view = nullptr; getFrame()->getAttribute(FOURCC('v', 'c', 'm', 'd')/*kCViewContainerMouseDownViewAttribute*/, view))
		{
			return view;
		}
		return nullptr;
	};
	if (auto const control = dynamic_cast<IDGControl*>(getMouseDownView()))
	{
		if (!control->isParameterAttached())
		{
			return;
		}
		if (control->getParameterID() != inParameterID)
		{
			return;
		}
	}

	dfx::UniqueCFType<CFStringRef> universalDisplayText;
	auto const cfAllocator = kCFAllocatorDefault;
	switch (inParameterID)
	{
		case kParam_ScratchAmount:
			{
				// XXX float2string(m_fPlaySampleRate, text);
				auto const format = (value > 0.f) ? CFSTR("%+.3f") : CFSTR("%.3f");
				universalDisplayText.reset(CFStringCreateWithFormat(cfAllocator, nullptr, format, value));
			}
			break;
		case kParam_ScratchSpeed_scrub:
			universalDisplayText.reset(CFStringCreateWithFormat(cfAllocator, nullptr, CFSTR("%.3f  second%s"), value, (std::fabs(value) > 1.f) ? "s" : ""));
			break;
		case kParam_ScratchSpeed_spin:
			universalDisplayText.reset(CFStringCreateWithFormat(cfAllocator, nullptr, CFSTR("%.3f x"), value));
			break;
		case kParam_SpinUpSpeed:
		case kParam_SpinDownSpeed:
			universalDisplayText.reset(CFStringCreateWithFormat(cfAllocator, nullptr, CFSTR("%.4f"), value));
			break;
		case kParam_PitchShift:
		{
			auto const value_scalar = value * 0.01f * getparameter_f(kParam_PitchRange);
			universalDisplayText.reset(CFStringCreateWithFormat(cfAllocator, nullptr, CFSTR("%+.2f  semitone%s"), value_scalar, (std::fabs(value_scalar) > 1.f) ? "s" : ""));
			break;
		}
		case kParam_PitchRange:
			universalDisplayText.reset(CFStringCreateWithFormat(cfAllocator, nullptr, CFSTR("%.2f  semitone%s"), value, (std::fabs(value) > 1.f) ? "s" : ""));
			break;
		case kParam_RootKey:
			universalDisplayText.reset(CFStringCreateWithCString(cfAllocator, dfx::GetNameForMIDINote(value_i).c_str(), DfxParam::kDefaultCStringEncoding));
			break;

#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
		case kParam_Volume:
			if (value <= 0.f)
			{
				constexpr UniChar minusInfinity[] = { '-', 0x221E, ' ', ' ', 'd', 'B' };
				universalDisplayText.reset(CFStringCreateWithCharacters(cfAllocator, minusInfinity, std::ssize(minusInfinity)));
			}
			else
			{
				#define DB_FORMAT_STRING	"%.2f  dB"
				CFStringRef format = CFSTR(DB_FORMAT_STRING);
				if (value > 1.f)
				{
					format = CFSTR("+" DB_FORMAT_STRING);
				}
				#undef DB_FORMAT_STRING
				universalDisplayText.reset(CFStringCreateWithFormat(cfAllocator, nullptr, format, dfx::math::Linear2dB(value)));
			}
			break;
#endif

		case kParam_KeyTracking:
		case kParam_Loop:
		case kParam_Power:
		case kParam_NotePowerTrack:
#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
		case kParam_Mute:
#endif
			universalDisplayText.reset(value_i ? CFSTR("on") : CFSTR("off"));
			CFRetain(universalDisplayText.get());
			break;

		case kParam_Direction:
		case kParam_ScratchMode:
		case kParam_NoteMode:
			{
				auto const valueStrings = DFX_CopyAUParameterValueStrings(dfxgui_GetEffectInstance(), inParameterID);
				if (valueStrings)
				{
					universalDisplayText.reset(static_cast<CFStringRef>(CFArrayGetValueAtIndex(valueStrings.get(), value_i)));
					if (universalDisplayText)
					{
						CFRetain(universalDisplayText.get());
					}
				}
			}
			break;

		default:
			break;
	}

	if (universalDisplayText)
	{
		auto parameterNameString = DFX_CopyAUParameterName(dfxgui_GetEffectInstance(), inParameterID);
		if (parameterNameString)
		{
			// split the string after the first " (", if it has that, to eliminate the parameter names with sub-specifications 
			// (e.g. the 2 scratch speed parameters)
			auto const nameArray = dfx::MakeUniqueCFType(CFStringCreateArrayBySeparatingStrings(cfAllocator, parameterNameString.get(), CFSTR(" (")));
			if (nameArray)
			{
				if (CFArrayGetCount(nameArray.get()) > 1)
				{
					auto const truncatedName = static_cast<CFStringRef>(CFArrayGetValueAtIndex(nameArray.get(), 0));
					if (truncatedName)
					{
						parameterNameString.reset(truncatedName);
						CFRetain(truncatedName);
					}
				}
			}

			auto const fullDisplayText = CFStringCreateWithFormat(cfAllocator, nullptr, CFSTR("%@:  %@"), parameterNameString.get(), universalDisplayText.get());
			if (fullDisplayText)
			{
				universalDisplayText.reset(fullDisplayText);
			}
		}

		mAllParametersTextDisplay->setCFText(universalDisplayText.get());
	}
}

//-----------------------------------------------------------------------------
void TurntablistEditor::dfxgui_Idle()
{
#if DRAG_N_DROP
	// It's better to do this outside of the drag event handler, so I use the idle loop to delay its occurrence.  
	// Otherwise the alert dialog's run loop will block (and potentially timeout) the drag event callback.
	if (mDragAudioFileError != noErr)
	{
		NotifyAudioFileLoadError(mDragAudioFileError, mDragAudioFileRef);
	}
	mDragAudioFileError = noErr;
#endif
}
