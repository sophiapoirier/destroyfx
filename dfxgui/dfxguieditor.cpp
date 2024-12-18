/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2024  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 2 of the License, or 
(at your option) any later version.

Destroy FX Library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Destroy FX Library.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#include "dfxguieditor.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <exception>
#include <functional>
#include <locale>
#include <map>
#include <mutex>
#include <numeric>
#include <ranges>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "dfxguibutton.h"
#include "dfxguidialog.h"
#include "dfxmath.h"
#include "dfxmidi.h"
#include "dfxmisc.h"
#include "idfxguicontrol.h"
#include "lib/vstguiinit.h"

#if TARGET_PLUGIN_USES_MIDI
	#include "dfxsettings.h"
#endif

#ifdef TARGET_API_AUDIOUNIT
	#include <AudioUnitSDK/AUUtility.h>
	#include "dfx-au-utilities.h"
#elifdef TARGET_API_VST
	#include "dfxmisc.h"
	#include "dfxplugin.h"
	#include "dfxsettings.h"
	#include "lib/platform/common/fileresourceinputstream.h"
	#include "lib/platform/platformfactory.h"
	#include "pluginterfaces/vst2.x/vstfxstore.h"
	#if TARGET_OS_WIN32
	#include "uidescription/base64codec.h"
	#endif
#elifdef TARGET_API_RTAS
	#include "ConvertUtils.h"
#endif


using namespace std::string_literals;


#if TARGET_OS_MAC
	#define kDfxGui_AUPresetFileUTI "com.apple.audio-unit-preset"  // XXX implemented in Mac OS X 10.4.11 or maybe a little earlier, but no public constant published yet
	static CFStringRef const kDfxGui_SettingsPasteboardFlavorType_AU = CFSTR(kDfxGui_AUPresetFileUTI);
#endif

#ifdef TARGET_API_AUDIOUNIT
	static auto const kDfxGui_SettingsPasteboardFlavorType = kDfxGui_SettingsPasteboardFlavorType_AU;
#endif

#ifdef TARGET_API_VST
	constexpr bool kDfxGui_CopySettingsIsPreset = true;
	constexpr VstInt32 kDfxGui_VSTSettingsChunkMagic = FOURCC('C', 'c', 'n', 'K');
	constexpr VstInt32 kDfxGui_VSTSettingsRegularMagic = FOURCC('F', 'x', 'C', 'k');
	constexpr VstInt32 kDfxGui_VSTSettingsOpaqueChunkMagic = FOURCC('F', 'P', 'C', 'h');
	constexpr VstInt32 kDfxGui_VSTSettingsVersion = 1;
	#if TARGET_OS_MAC
	static CFStringRef const kDfxGui_SettingsPasteboardFlavorType = CFSTR(PLUGIN_BUNDLE_IDENTIFIER);
	#endif
	#if TARGET_OS_WIN32
	// From vstplugmain in VST SDK, but no include :/
	extern void* hInstance;
	#endif
	using ERect = ::ERect;
#else
	using ERect = VSTGUI::ERect;
#endif

//-----------------------------------------------------------------------------
DfxGuiEditor::DfxGuiEditor(DGEditorListenerInstance inInstance)
:	TARGET_API_EDITOR_BASE_CLASS(inInstance)
#ifdef TARGET_API_AUDIOUNIT
	, kAUPresetFileExtension("Audio Unit preset", "aupreset", "", 0, kDfxGui_AUPresetFileUTI)
#endif
{
	// ugly wart added in VSTGUI 4.10: the library must be statically "initialized" before use
	static std::once_flag once;
	std::call_once(once, []
	{
#if TARGET_OS_MAC
		VSTGUI::init(CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER)));
#elif TARGET_OS_WIN32 && defined(TARGET_API_VST)
		VSTGUI::init(static_cast<HINSTANCE>(hInstance));
#else
	#error "implementation needed"
#endif
		dfx::InitGUI();

		static auto const originalCheckDefaultValueEventFunc = VSTGUI::CControl::CheckDefaultValueEventFunc;
		VSTGUI::CControl::CheckDefaultValueEventFunc = [](VSTGUI::CControl* inControl, VSTGUI::MouseDownEvent& ioEvent)
		{
			auto const result = originalCheckDefaultValueEventFunc(inControl, ioEvent);
			auto const dgControl = dynamic_cast<IDGControl*>(inControl);
			if (result && dgControl)
			{
				for (auto const childControl : dgControl->getChildren())
				{
					auto const childCControl = childControl->asCControl();
					auto const defaultValue = childCControl->getDefaultValue();
					if (defaultValue != childCControl->getValue())
					{
						childCControl->beginEdit();
						childCControl->setValue(defaultValue);
						childCControl->valueChanged();
						childCControl->endEdit();
						childCControl->setDirty();
					}
				}
			}
			return result;
		};
	});

	// This activates embedded font resources. We do this as early as
	// possible (e.g. not in open()) because on Windows it seems to
	// take a few milliseconds for the font to actually become available.
	mFontFactory = dfx::FontFactory::Create();

	rect.top = rect.left = rect.bottom = rect.right = 0;
	// load the background image
	// we don't need to load all bitmaps, this could be done when open is called
	// XXX hack
	mBackgroundImage = LoadImage(PLUGIN_BACKGROUND_IMAGE_FILENAME);
	if (mBackgroundImage)
	{
		// init the size of the plugin
		rect.right = rect.left + dfx::math::IRound(mBackgroundImage->getWidth());
		rect.bottom = rect.top + dfx::math::IRound(mBackgroundImage->getHeight());
	}

	setKnobMode(VSTGUI::kLinearMode);

#if !defined(TARGET_API_AUDIOUNIT) && TARGET_PLUGIN_USES_MIDI
	RegisterPropertyChange(dfx::kPluginProperty_MidiLearn);
	RegisterPropertyChange(dfx::kPluginProperty_MidiLearner);
#endif

#ifdef TARGET_API_RTAS
	mParameterHighlightColors.assign(GetNumParameters(), eHighlight_None);

	// XXX do these?
//	VSTGUI::CControl::kZoomModifier = kControl;
//	VSTGUI::CControl::kDefaultValueModifier = kShift;
#endif
}

//-----------------------------------------------------------------------------
DfxGuiEditor::~DfxGuiEditor()
{
	assert(!IsOpen());
#ifdef TARGET_API_AUDIOUNIT
	RemoveAUEventListeners();
#endif
}


//-----------------------------------------------------------------------------
bool DfxGuiEditor::open(void* inWindow)
try
{
	// !!! always call this !!!
	[[maybe_unused]] auto const baseSuccess = TARGET_API_EDITOR_BASE_CLASS::open(inWindow);
#ifndef TARGET_API_VST
	// In VST, this always returns false (not clear whether that is
	// intended), so don't treat that as an error.
	if (!baseSuccess)
	{
		return baseSuccess;
	}
#endif

	mControlsList.clear();

	frame = new VSTGUI::CFrame(VSTGUI::CRect(rect.left, rect.top, rect.right, rect.bottom), this);
	frame->open(inWindow);
	frame->setBackground(GetBackgroundImage());
	frame->registerMouseObserver(this);
	frame->enableTooltips(true);

	mMousedOverControlsList.clear();
	setCurrentControl_mouseover(nullptr);

	// determine the number of audio channels currently configured for the AU
	mNumInputChannels = getNumInputChannels();
	mNumOutputChannels = getNumOutputChannels();

#ifdef TARGET_API_AUDIOUNIT
	// create a cache of the parameter list before creating controls as the process depends on that information
	{
		std::lock_guard const guard(mParameterListLock);
		mParameterList = CreateParameterList();
	}
#else
	mParameterList.assign(GetNumParameters(), dfx::kParameterID_Invalid);
	// TODO C++23: std::ranges::iota
	std::iota(mParameterList.begin(), mParameterList.end(), 0);
	std::erase_if(mParameterList, [this](auto parameterID)
	{
		return HasParameterAttribute(parameterID, DfxParam::kAttribute_Unused);
	});
#endif

	OpenEditor();
	mJustOpened = true;

	// allow for anything that might need to happen after the above post-opening stuff is finished
	PostOpenEditor();

#if TARGET_PLUGIN_USES_MIDI
	HandleMidiLearnChange();
	HandleMidiLearnerChange();
#endif

#ifdef TARGET_API_AUDIOUNIT
	// once all GUI elements are created, install an event listener for the parameters and necessary properties
	InstallAUEventListeners();
#endif

	return true;
}
catch (...)
{
	return false;
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::close()
{
	mJustOpened = false;
	mCurrentControl_mouseover = nullptr;

#if TARGET_PLUGIN_USES_MIDI
	mMidiLearnButton = mMidiResetButton = nullptr;
	// call this before closing the editor and deleting the frame's controls, 
	// since this can loop back a notification to the editor and MIDI learning control
	setmidilearning(false);
#endif

	CloseEditor();

	frame->unregisterMouseObserver(this);
	// zero the member frame before we release it so that other asynchronous calls don't crash
	auto const frame_temp = std::exchange(frame, nullptr);

	for (auto& control : mControlsList)
	{
		control->asCControl()->unregisterViewEventListener(this);
	}
	mControlsList.clear();

	frame_temp->forget();

	TARGET_API_EDITOR_BASE_CLASS::close();
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::IsOpen()
{
	return (getFrame() && isOpen());
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::setParameter(TARGET_API_EDITOR_INDEX_TYPE inParameterIndex, float inValue)
{
	if (!IsOpen())
	{
		return;
	}

	updateParameterControls(dfx::ParameterID_FromVST(inParameterIndex), inValue);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::updateParameterControls(dfx::ParameterID inParameterID, float inValue, VSTGUI::CControl* inSendingControl)
{
	for (auto& control : mControlsList)
	{
		if ((control->getParameterID() == inParameterID) && (control->asCControl() != inSendingControl))
		{
			control->setValue_gen(inValue);
			control->redraw();  // TODO: why is this also necessary? redraws are sometimes dropped without it
		}
	}

	parameterChanged(inParameterID);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::valueChanged(VSTGUI::CControl* inControl)
{
	auto const parameterID = dfx::ParameterID_FromVST(inControl->getTag());
	auto const parameterValue_norm = inControl->getValueNormalized();

	if (dfxgui_IsValidParameterID(parameterID))
	{
#ifdef TARGET_API_AUDIOUNIT
		auto const parameterValue_literal = dfxgui_ExpandParameterValue(parameterID, parameterValue_norm);
		// XXX or should we call setparameter_f()?
		auto const auParam = dfxgui_MakeAudioUnitParameter(parameterID);
		AUParameterSet(mAUEventListener.get(), inControl, &auParam, parameterValue_literal, 0);
#elifdef TARGET_API_VST
		getEffect()->setParameterAutomated(dfx::ParameterID_ToVST(parameterID), parameterValue_norm);
#elifdef TARGET_API_RTAS
		// XXX though the model of calling SetControlValue might make more seem like 
		// better design than calling setparameter_gen on the effect, in practice, 
		// our DfxParam objects won't get their values updated to reflect the change until 
		// the next call to UpdateControlInAlgorithm which be deferred until the start of 
		// the next audio render call, which means that in the meantime getparameter_* 
		// methods will return the previous rather than current value
//		dfxgui_GetEffectInstance()->SetControlValue(dfx::ParameterID_ToRTAS(parameterID), ConvertToDigiValue(parameterValue_norm));
		dfxgui_GetEffectInstance()->setparameter_gen(parameterID, parameterValue_norm);
#endif
	}
}

#ifndef TARGET_API_VST
//-----------------------------------------------------------------------------
void DfxGuiEditor::beginEdit(int32_t inParameterIndex)
{
	if (auto const parameterID = dfx::ParameterID_FromVST(inParameterIndex); dfxgui_IsValidParameterID(parameterID))
	{
		automationgesture_begin(parameterID);
	}
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::endEdit(int32_t inParameterIndex)
{
	if (auto const parameterID = dfx::ParameterID_FromVST(inParameterIndex); dfxgui_IsValidParameterID(parameterID))
	{
		automationgesture_end(parameterID);
	}
}
#endif	// !TARGET_API_VST

//-----------------------------------------------------------------------------
void DfxGuiEditor::idle()
{
	// call this so that idle() actually happens
	TARGET_API_EDITOR_BASE_CLASS::idle();

	if (!IsOpen())
	{
		return;
	}

	if (std::exchange(mJustOpened, false))
	{
		dfxgui_EditorShown();
#if TARGET_OS_WIN32 // && defined(TARGET_API_RTAS)
		// XXX this seems to be necessary to correct background re-drawing failure 
		// when switching between different plugins in an open plugin editor window
		getFrame()->invalid();
#endif
	}

	if (!mPendingErrorMessage.empty() && !getFrame()->getModalView())
	{
		ShowMessage(std::exchange(mPendingErrorMessage, {}));
	}

#ifndef TARGET_API_AUDIOUNIT
	for (auto& [propertyDescriptor, propertyChangeHasPosted] : mPropertyChangesHavePosted)
	{
		if (!propertyChangeHasPosted.test_and_set())
		{
			switch (propertyDescriptor.mID)
			{
			#if TARGET_PLUGIN_USES_MIDI
				case dfx::kPluginProperty_MidiLearn:
					HandleMidiLearnChange();
					break;
				case dfx::kPluginProperty_MidiLearner:
					HandleMidiLearnerChange();
					break;
			#endif
				default:
					HandlePropertyChange(propertyDescriptor.mID, propertyDescriptor.mScope, propertyDescriptor.mItemIndex);
					break;
			}
		}
	}
#endif

#ifdef TARGET_API_RTAS
	// Called by GUI when idle time available; NO_UI: Call process' DoIdle() method.  
	// Keeps other processes from starving during certain events (like mouse down).
	dfxgui_GetEffectInstance()->ProcessDoIdle();
#endif

	// call any child class implementation
	dfxgui_Idle();
}


//-----------------------------------------------------------------------------
void DfxGuiEditor::RegisterPropertyChange(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned int inItemIndex)
{
	assert(!IsOpen());  // you need to register these all before opening a view
	assert(!IsPropertyRegistered(inPropertyID, inScope, inItemIndex));

	PropertyDescriptor const property{inPropertyID, inScope, inItemIndex};
	mRegisteredProperties.push_back(property);

#ifndef TARGET_API_AUDIOUNIT
	mPropertyChangesHavePosted[property].test_and_set();
#endif
}

#ifndef TARGET_API_AUDIOUNIT
//-----------------------------------------------------------------------------
void DfxGuiEditor::PropertyChanged(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned int inItemIndex)
{
	if (IsPropertyRegistered(inPropertyID, inScope, inItemIndex))
	{
		// defer handling to the main thread
		PropertyDescriptor const property{inPropertyID, inScope, inItemIndex};
		assert(mPropertyChangesHavePosted.contains(property));
		mPropertyChangesHavePosted[property].clear();
	}
}
#endif

//-----------------------------------------------------------------------------
bool DfxGuiEditor::IsPropertyRegistered(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned int inItemIndex) const
{
	PropertyDescriptor const property{inPropertyID, inScope, inItemIndex};
	// TODO C++23: std::ranges::contains
	return std::ranges::find(mRegisteredProperties, property) != mRegisteredProperties.cend();
}

//-----------------------------------------------------------------------------
IDGControl* DfxGuiEditor::addControl(IDGControl* inControl)
{
	assert(inControl);

	// XXX only add it to our controls list if it is attached to a parameter (?)
	if (dfxgui_IsValidParameterID(inControl->getParameterID()))
	{
		// TODO C++23: std::ranges::contains
		assert(std::ranges::find(mControlsList, inControl) == mControlsList.cend());
		mControlsList.push_back(inControl);
		inControl->asCControl()->registerViewEventListener(this);
	}

	[[maybe_unused]] auto const success = getFrame()->addView(inControl->asCControl());
	assert(success);

	return inControl;
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::removeControl(IDGControl* inControl)
{
	assert(false);  // TODO: test or remove this method? (it currently is not used anywhere)

	// TODO C++23: std::ranges::contains
	assert(std::ranges::find(mControlsList, inControl) != mControlsList.cend());
	std::erase(mControlsList, inControl);

	inControl->asCControl()->unregisterViewEventListener(this);
	[[maybe_unused]] auto const success = getFrame()->removeView(inControl->asCControl());
	assert(success);
}

//-----------------------------------------------------------------------------
int DfxGuiEditor::GetWidth()
{
	ERect* editorRect = nullptr;
	if (getRect(&editorRect) && editorRect)
	{
		return editorRect->right - editorRect->left;
	}
	return 0;
}

//-----------------------------------------------------------------------------
int DfxGuiEditor::GetHeight()
{
	ERect* editorRect = nullptr;
	if (getRect(&editorRect) && editorRect)
	{
		return editorRect->bottom - editorRect->top;
	}
	return 0;
}

//-----------------------------------------------------------------------------
VSTGUI::SharedPointer<DGImage> DfxGuiEditor::LoadImage(std::string const& inFileName)
{
	auto const result = VSTGUI::makeOwned<DGImage>(inFileName.c_str());
	HandleLoadImageError(*result, inFileName);
	return result;
}

//-----------------------------------------------------------------------------
VSTGUI::SharedPointer<DGMultiFrameImage> DfxGuiEditor::LoadImage(std::string const& inFileName, size_t inFrameCount)
{
	assert(inFrameCount > 0);

	auto const result = VSTGUI::makeOwned<DGMultiFrameImage>(inFileName.c_str());
	HandleLoadImageError(*result, inFileName);

	// fix up the frame dimensions relative to those of the actual image data
	if (result->isLoaded())
	{
		VSTGUI::CMultiFrameBitmapDescription const framesDescription
		{
			.frameSize = { result->getWidth(), result->getHeight() / inFrameCount },
			.numFrames = static_cast<uint16_t>(inFrameCount),
			.framesPerRow = 1
		};
		result->setMultiFrameDesc(framesDescription);
	}

	return result;
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::HandleLoadImageError(DGImage& inImage, std::string const& inFileName)
{
	if (!inImage.isLoaded())
	{
		// use the pending error message since typically images are loaded in series 
		// upon opening the editor view, prior to the idle timer running, and so 
		// a cumulative error message can be built this way if multiple images fail
		if (mPendingErrorMessage.empty())
		{
			mPendingErrorMessage = "This software is C0RRVPT!\n";
		}
		mPendingErrorMessage += "failed to load image resource: " + inFileName + "\n";
	}
}


#ifdef TARGET_API_AUDIOUNIT
//-----------------------------------------------------------------------------
OSStatus DfxGuiEditor::SendAUParameterEvent(AudioUnitParameterID inParameterID, AudioUnitEventType inEventType)
{
	AUSDK_Require(dfxgui_GetEffectInstance(), kAudioUnitErr_Uninitialized);

	AudioUnitEvent paramEvent = {};
	paramEvent.mEventType = inEventType;
	paramEvent.mArgument.mParameter = dfxgui_MakeAudioUnitParameter(inParameterID);
	return AUEventListenerNotify(getAUEventListener(), nullptr, &paramEvent);
}
#endif

//-----------------------------------------------------------------------------
void DfxGuiEditor::automationgesture_begin(dfx::ParameterID inParameterID)
{
#ifdef TARGET_API_AUDIOUNIT
	SendAUParameterEvent(inParameterID, kAudioUnitEvent_BeginParameterChangeGesture);
#elifdef TARGET_API_VST
	TARGET_API_EDITOR_BASE_CLASS::beginEdit(dfx::ParameterID_ToVST(inParameterID));
#elifdef TARGET_API_RTAS
	// called by GUI when mouse down event has occured; NO_UI: Call process' TouchControl()
	// (this and endEdit are necessary for Touch Automation to work properly)
	dfxgui_GetEffectInstance()->ProcessTouchControl(dfx::ParameterID_ToRTAS(inParameterID));
#endif
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::automationgesture_end(dfx::ParameterID inParameterID)
{
#ifdef TARGET_API_AUDIOUNIT
	SendAUParameterEvent(inParameterID, kAudioUnitEvent_EndParameterChangeGesture);
#elifdef TARGET_API_VST
	TARGET_API_EDITOR_BASE_CLASS::endEdit(dfx::ParameterID_ToVST(inParameterID));
#elifdef TARGET_API_RTAS
	// called by GUI when mouse up event has occured; NO_UI: Call process' ReleaseControl()
	dfxgui_GetEffectInstance()->ProcessReleaseControl(dfx::ParameterID_ToRTAS(inParameterID));
#endif
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::randomizeparameter(dfx::ParameterID inParameterID, bool inWriteAutomation)
{
	if (inWriteAutomation)
	{
		automationgesture_begin(inParameterID);
	}

#ifdef TARGET_API_AUDIOUNIT
	dfxgui_SetProperty(dfx::kPluginProperty_RandomizeParameter, dfx::kScope_Global, inParameterID, 
					   static_cast<Boolean>(inWriteAutomation));

#elifdef TARGET_API_VST
	dfxgui_GetEffectInstance()->randomizeparameter(inParameterID);
	if (inWriteAutomation)
	{
		getEffect()->setParameterAutomated(dfx::ParameterID_ToVST(inParameterID), getparameter_gen(inParameterID));
	}

#elifdef TARGET_API_RTAS
	#warning "implementation missing"
	assert(false);  // TODO: implement
#endif

	if (inWriteAutomation)
	{
		automationgesture_end(inParameterID);
	}
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::randomizeparameters(bool inWriteAutomation)
{
	auto const parameterList = std::ranges::to<std::vector>(std::views::filter(GetParameterList(), [this](auto parameterID)
	{
		return !HasParameterAttribute(parameterID, DfxParam::kAttribute_OmitFromRandomizeAll);
	}));

	if (inWriteAutomation)
	{
		for (auto const parameterID : parameterList)
		{
			automationgesture_begin(parameterID);
		}
	}

#ifdef TARGET_API_AUDIOUNIT
	dfxgui_SetProperty(dfx::kPluginProperty_RandomizeParameter, 
					   dfx::kScope_Global, kAUParameterListener_AnyParameter, 
					   static_cast<Boolean>(inWriteAutomation));

#elifdef TARGET_API_VST
	dfxgui_GetEffectInstance()->randomizeparameters();

#elifdef TARGET_API_RTAS
	#warning "implementation missing"
	assert(false);  // TODO: implement
#endif

	if (inWriteAutomation)
	{
		for (auto const parameterID : parameterList)
		{
#ifdef TARGET_API_VST
			getEffect()->setParameterAutomated(dfx::ParameterID_ToVST(parameterID), getparameter_gen(parameterID));
#endif
			automationgesture_end(parameterID);
		}
	}
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::GenerateParameterAutomationSnapshot(dfx::ParameterID inParameterID)
{
	setparameter_f(inParameterID, getparameter_f(inParameterID), true);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::GenerateParametersAutomationSnapshot()
{
	for (auto const parameterID : GetParameterList())
	{
		GenerateParameterAutomationSnapshot(parameterID);
	}
}

//-----------------------------------------------------------------------------
std::optional<double> DfxGuiEditor::dfxgui_GetParameterValueFromString_f(dfx::ParameterID inParameterID, std::string const& inText)
{
	if (GetParameterValueType(inParameterID) == DfxParam::Value::Type::Float)
	{
		double value {};
		auto const readCount = std::sscanf(dfx::SanitizeNumericalInput(inText).c_str(), "%lf", &value);
		if ((readCount >= 1) && (readCount != EOF))
		{
			return value;
		}
	}
	else
	{
		if (auto const parsedValue_i = dfxgui_GetParameterValueFromString_i(inParameterID, inText))
		{
			return DfxParam::derive_f(*parsedValue_i);
		}
	}

	return {};
}

//-----------------------------------------------------------------------------
std::optional<long> DfxGuiEditor::dfxgui_GetParameterValueFromString_i(dfx::ParameterID inParameterID, std::string const& inText)
{
	if (GetParameterValueType(inParameterID) == DfxParam::Value::Type::Float)
	{
		if (auto const parsedValue_f = dfxgui_GetParameterValueFromString_f(inParameterID, inText))
		{
			return DfxParam::derive_i(*parsedValue_f);
		}
	}
	else
	{
		long value {};
		auto const readCount = std::sscanf(dfx::SanitizeNumericalInput(inText).c_str(), "%ld", &value);
		if ((readCount >= 1) && (readCount != EOF))
		{
			return value;
		}
	}

	return {};
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::dfxgui_SetParameterValueWithString(dfx::ParameterID inParameterID, std::string const& inText)
{
	if (dfxgui_IsValidParameterID(inParameterID))
	{
		constexpr bool automationGesture = true;
		if (GetParameterValueType(inParameterID) == DfxParam::Value::Type::Float)
		{
			if (auto const newValue = dfxgui_GetParameterValueFromString_f(inParameterID, inText))
			{
				setparameter_f(inParameterID, *newValue, automationGesture);
				return true;
			}
		}
		else
		{
			if (auto const newValue = dfxgui_GetParameterValueFromString_i(inParameterID, inText))
			{
				setparameter_i(inParameterID, *newValue, automationGesture);
				return true;
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::dfxgui_IsValidParameterID(dfx::ParameterID inParameterID)
{
	if ((inParameterID == dfx::kParameterID_Invalid) || (inParameterID < 0))
	{
		return false;
	}
#ifdef TARGET_API_AUDIOUNIT
	// TODO: actually search parameter ID list to ensure that this ID is present?
	return (inParameterID <= mAUMaxParameterID);
#else
	return (inParameterID < GetNumParameters());
#endif
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::TextEntryForParameterValue(dfx::ParameterID inParameterID)
{
	if (!getFrame())
	{
		return;
	}

	mTextEntryDialog = VSTGUI::makeOwned<DGTextEntryDialog>(inParameterID, getparametername(inParameterID), "enter value:");
	std::array<char, dfx::kParameterValueStringMaxLength> textValue {};
	if (GetParameterValueType(inParameterID) == DfxParam::Value::Type::Float)
	{
		std::snprintf(textValue.data(), textValue.size(), "%.6lf", getparameter_f(inParameterID));
	}
	else
	{
		std::snprintf(textValue.data(), textValue.size(), "%ld", getparameter_i(inParameterID));
	}
	mTextEntryDialog->setText(textValue.data());

	auto const textEntryCallback = [this](std::string const& inText, dfx::ParameterID inParameterID)
	{
		return dfxgui_SetParameterValueWithString(inParameterID, inText);
	};
	if (!mTextEntryDialog->runModal(getFrame(), textEntryCallback))
	{
		ShowMessage("could not display text entry dialog");
	}
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::SetParameterHelpText(dfx::ParameterID inParameterID, char const* inText)
{
	for (auto& control : mControlsList)
	{
		if (control->getParameterID() == inParameterID)
		{
			control->setHelpText(inText);
		}
	}
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::SetParameterAlpha(dfx::ParameterID inParameterID, float inAlpha)
{
	for (auto& control : mControlsList)
	{
		if (control->getParameterID() == inParameterID)
		{
			control->setDrawAlpha(inAlpha);
		}
	}
}

//-----------------------------------------------------------------------------
// set the control that is currently idly under the mouse pointer, if any (nullptr if none)
void DfxGuiEditor::setCurrentControl_mouseover(IDGControl* inMousedOverControl)
{
	// post notification if the mouse-overed control has changed
	if (std::exchange(mCurrentControl_mouseover, inMousedOverControl) != inMousedOverControl)
	{
		mouseovercontrolchanged(inMousedOverControl);
	}
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::addMousedOverControl(IDGControl* inMousedOverControl)
{
	assert(inMousedOverControl);
	mMousedOverControlsList.push_back(inMousedOverControl);
	setCurrentControl_mouseover(inMousedOverControl);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::removeMousedOverControl(IDGControl* inMousedOverControl)
{
	mMousedOverControlsList.remove(inMousedOverControl);
	if (mMousedOverControlsList.empty())
	{
		setCurrentControl_mouseover(nullptr);
	}
	else if (inMousedOverControl == mCurrentControl_mouseover)
	{
		setCurrentControl_mouseover(mMousedOverControlsList.back());
	}
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::onMouseEntered(VSTGUI::CView* inView, VSTGUI::CFrame* /*inFrame*/)
{
	if (auto const dgControl = dynamic_cast<IDGControl*>(inView))
	{
		addMousedOverControl(dgControl);
	}
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::onMouseExited(VSTGUI::CView* inView, VSTGUI::CFrame* /*inFrame*/)
{
	if (auto const dgControl = dynamic_cast<IDGControl*>(inView))
	{
		removeMousedOverControl(dgControl);
	}
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::onMouseEvent(VSTGUI::MouseEvent& ioEvent, VSTGUI::CFrame* inFrame)
{
	switch (ioEvent.type)
	{
		case VSTGUI::EventType::MouseDown:
			if (!inFrame->getViewAt(ioEvent.mousePosition, VSTGUI::GetViewOptions().mouseEnabled().deep()))
			{
				auto const handled = handleContextualMenuClick(nullptr, ioEvent.buttonState);
				if (handled)
				{
					ioEvent.consumed = true;
					VSTGUI::castMouseDownEvent(ioEvent).ignoreFollowUpMoveAndUpEvents(true);
				}
			}
			break;

		// entered and exited is not enough because they stop notifying when mouse buttons are pressed
		case VSTGUI::EventType::MouseMove:
		{
			IDGControl* currentControl = nullptr;
			if (auto const currentView = inFrame->getViewAt(ioEvent.mousePosition, VSTGUI::GetViewOptions().deep()))
			{
				if (auto const dgControl = dynamic_cast<IDGControl*>(currentView))
				{
					currentControl = dgControl;
				}
			}
			setCurrentControl_mouseover(currentControl);
			break;
		}

		default:
			break;
	}
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::viewOnEvent(VSTGUI::CView* inView, VSTGUI::Event& ioEvent)
{
	if ((ioEvent.type != VSTGUI::EventType::MouseDown) || !inView->getMouseEnabled())
	{
		return;
	}
	if (auto const dgControl = dynamic_cast<IDGControl*>(inView))
	{
		dgControl->invalidateMouseWheelEditingTimer();

		auto const mouseButtonState = VSTGUI::castMouseEvent(ioEvent).buttonState;
		auto const handled = handleContextualMenuClick(dgControl->asCControl(), mouseButtonState);
		if (handled)
		{
			ioEvent.consumed = true;
			VSTGUI::castMouseDownEvent(ioEvent).ignoreFollowUpMoveAndUpEvents(true);
			return;
		}

#if TARGET_PLUGIN_USES_MIDI
		auto const isMultiControl = !dgControl->getChildren().empty();  // multi-controls should self-manage learn
		if (dgControl->isParameterAttached() && !isMultiControl && getmidilearning() && mouseButtonState.isLeft())
		{
			setmidilearner(dgControl->getParameterID());
		}
#endif
	}
}

//-----------------------------------------------------------------------------
double DfxGuiEditor::getparameter_f(dfx::ParameterID inParameterID)
{
#ifdef TARGET_API_AUDIOUNIT
	dfx::ParameterValueRequest request;
	size_t dataSize = sizeof(request);
	request.inValueItem = dfx::ParameterValueItem::Current;
	request.inValueType = DfxParam::Value::Type::Float;

	auto const status = dfxgui_GetProperty(dfx::kPluginProperty_ParameterValue, dfx::kScope_Global, 
										   inParameterID, &request, dataSize);
	if (status == dfx::kStatus_NoError)
	{
		return request.value.get_f();
	}
	return 0.0;
#else
	return dfxgui_GetEffectInstance()->getparameter_f(inParameterID);
#endif
}

//-----------------------------------------------------------------------------
long DfxGuiEditor::getparameter_i(dfx::ParameterID inParameterID)
{
#ifdef TARGET_API_AUDIOUNIT
	dfx::ParameterValueRequest request;
	size_t dataSize = sizeof(request);
	request.inValueItem = dfx::ParameterValueItem::Current;
	request.inValueType = DfxParam::Value::Type::Int;

	auto const status = dfxgui_GetProperty(dfx::kPluginProperty_ParameterValue, dfx::kScope_Global, 
										   inParameterID, &request, dataSize);
	if (status == dfx::kStatus_NoError)
	{
		return request.value.get_i();
	}
	return 0;
#else
	return dfxgui_GetEffectInstance()->getparameter_i(inParameterID);
#endif
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::getparameter_b(dfx::ParameterID inParameterID)
{
#ifdef TARGET_API_AUDIOUNIT
	dfx::ParameterValueRequest request;
	size_t dataSize = sizeof(request);
	request.inValueItem = dfx::ParameterValueItem::Current;
	request.inValueType = DfxParam::Value::Type::Boolean;

	auto const status = dfxgui_GetProperty(dfx::kPluginProperty_ParameterValue, dfx::kScope_Global, 
										   inParameterID, &request, dataSize);
	if (status == dfx::kStatus_NoError)
	{
		return request.value.get_b();
	}
	return false;
#else
	return dfxgui_GetEffectInstance()->getparameter_b(inParameterID);
#endif
}

//-----------------------------------------------------------------------------
double DfxGuiEditor::getparameter_gen(dfx::ParameterID inParameterID)
{
#ifdef TARGET_API_VST
	return getEffect()->getParameter(dfx::ParameterID_ToVST(inParameterID));
#else
	auto const currentValue = getparameter_f(inParameterID);
	return dfxgui_ContractParameterValue(inParameterID, currentValue);
#endif
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::setparameter_f(dfx::ParameterID inParameterID, double inValue, bool inWrapWithAutomationGesture)
{
	if (inWrapWithAutomationGesture)
	{
		automationgesture_begin(inParameterID);
	}

#ifdef TARGET_API_AUDIOUNIT
	dfx::ParameterValueRequest request;
	request.inValueItem = dfx::ParameterValueItem::Current;
	request.inValueType = DfxParam::Value::Type::Float;
	request.value = inValue;

	dfxgui_SetProperty(dfx::kPluginProperty_ParameterValue, dfx::kScope_Global, inParameterID, request);

#elifdef TARGET_API_VST
	auto const value_norm = dfxgui_ContractParameterValue(inParameterID, inValue);
	getEffect()->setParameterAutomated(dfx::ParameterID_ToVST(inParameterID), value_norm);

#elifdef TARGET_API_RTAS
	#warning "implementation missing"
	assert(false);  // TODO: implement
#endif

	if (inWrapWithAutomationGesture)
	{
		automationgesture_end(inParameterID);
	}
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::setparameter_i(dfx::ParameterID inParameterID, long inValue, bool inWrapWithAutomationGesture)
{
	if (inWrapWithAutomationGesture)
	{
		automationgesture_begin(inParameterID);
	}

#ifdef TARGET_API_AUDIOUNIT
	dfx::ParameterValueRequest request;
	request.inValueItem = dfx::ParameterValueItem::Current;
	request.inValueType = DfxParam::Value::Type::Int;
	request.value = inValue;

	dfxgui_SetProperty(dfx::kPluginProperty_ParameterValue, dfx::kScope_Global, inParameterID, request);

#elifdef TARGET_API_VST
	auto const value_norm = dfxgui_ContractParameterValue(inParameterID, inValue);
	getEffect()->setParameterAutomated(dfx::ParameterID_ToVST(inParameterID), value_norm);

#elifdef TARGET_API_RTAS
	#warning "implementation missing"
	assert(false);  // TODO: implement
#endif

	if (inWrapWithAutomationGesture)
	{
		automationgesture_end(inParameterID);
	}
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::setparameter_b(dfx::ParameterID inParameterID, bool inValue, bool inWrapWithAutomationGesture)
{
	if (inWrapWithAutomationGesture)
	{
		automationgesture_begin(inParameterID);
	}

#ifdef TARGET_API_AUDIOUNIT
	dfx::ParameterValueRequest request;
	request.inValueItem = dfx::ParameterValueItem::Current;
	request.inValueType = DfxParam::Value::Type::Boolean;
	request.value = inValue;

	dfxgui_SetProperty(dfx::kPluginProperty_ParameterValue, dfx::kScope_Global, inParameterID, request);

#elifdef TARGET_API_VST
	getEffect()->setParameterAutomated(dfx::ParameterID_ToVST(inParameterID), inValue ? 1.f : 0.f);

#elifdef TARGET_API_RTAS
	#warning "implementation missing"
	assert(false);  // TODO: implement
#endif

	if (inWrapWithAutomationGesture)
	{
		automationgesture_end(inParameterID);
	}
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::setparameter_default(dfx::ParameterID inParameterID, bool inWrapWithAutomationGesture)
{
#ifdef TARGET_API_AUDIOUNIT
	if (auto const parameterInfo = dfxgui_GetParameterInfo(inParameterID))
	{
		if (inWrapWithAutomationGesture)
		{
			automationgesture_begin(inParameterID);
		}

		auto const auParam = dfxgui_MakeAudioUnitParameter(inParameterID);
		[[maybe_unused]] auto const status = AUParameterSet(nullptr, nullptr, &auParam, parameterInfo->defaultValue, 0);
		assert(status == noErr);

		if (inWrapWithAutomationGesture)
		{
			automationgesture_end(inParameterID);
		}
	}

#elifdef TARGET_API_VST
	auto const defaultValue = GetParameter_defaultValue(inParameterID);
	auto const defaultValue_norm = dfxgui_ContractParameterValue(inParameterID, defaultValue);
	getEffect()->setParameterAutomated(dfx::ParameterID_ToVST(inParameterID), defaultValue_norm);

#elifdef TARGET_API_RTAS
	#warning "implementation missing"
	assert(false);  // TODO: implement
#endif
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::setparameters_default(bool inWrapWithAutomationGesture)
{
	for (auto const parameterID : GetParameterList())
	{
		setparameter_default(parameterID, inWrapWithAutomationGesture);
	}
}

//-----------------------------------------------------------------------------
std::optional<std::string> DfxGuiEditor::getparametervaluestring(dfx::ParameterID inParameterID)
{
	return getparametervaluestring(inParameterID, getparameter_i(inParameterID));
}

//-----------------------------------------------------------------------------
std::optional<std::string> DfxGuiEditor::getparametervaluestring(dfx::ParameterID inParameterID, int64_t inStringIndex)
{
#ifdef TARGET_API_AUDIOUNIT
	dfx::ParameterValueStringRequest request;
	size_t dataSize = sizeof(request);
	request.inStringIndex = inStringIndex;
	auto const status = dfxgui_GetProperty(dfx::kPluginProperty_ParameterValueString, dfx::kScope_Global, 
										   inParameterID, &request, dataSize);
	if (status == dfx::kStatus_NoError)
	{
		return request.valueString;
	}
	return {};
#else
	return dfxgui_GetEffectInstance()->getparametervaluestring(inParameterID, inStringIndex);
#endif
}

//-----------------------------------------------------------------------------
std::string DfxGuiEditor::getparameterunitstring(dfx::ParameterID inParameterID)
{
#ifdef TARGET_API_AUDIOUNIT
	return dfxgui_GetPropertyAsString(dfx::kPluginProperty_ParameterUnitLabel, dfx::kScope_Global, inParameterID);
#else
	return dfxgui_GetEffectInstance()->getparameterunitstring(inParameterID);
#endif
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::GetParameterUseValueStrings(dfx::ParameterID inParameterID)
{
#ifdef TARGET_API_AUDIOUNIT
	return dfxgui_GetProperty<Boolean>(dfx::kPluginProperty_ParameterUseValueStrings, dfx::kScope_Global, inParameterID).value_or(false);
#else
	return dfxgui_GetEffectInstance()->getparameterusevaluestrings(inParameterID);
#endif
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::HasParameterAttribute(dfx::ParameterID inParameterID, DfxParam::Attribute inFlag)
{
	assert(inFlag);
#ifdef TARGET_API_AUDIOUNIT
	return dfxgui_GetProperty<DfxParam::Attribute>(dfx::kPluginProperty_ParameterAttributes, dfx::kScope_Global, inParameterID).value_or(0) & inFlag;
#else
	return dfxgui_GetEffectInstance()->hasparameterattribute(inParameterID, inFlag);
#endif
}

//-----------------------------------------------------------------------------
std::optional<size_t> DfxGuiEditor::GetParameterGroup(dfx::ParameterID inParameterID)
{
#ifdef TARGET_API_AUDIOUNIT
	return dfxgui_GetProperty<size_t>(dfx::kPluginProperty_ParameterGroup, dfx::kScope_Global, inParameterID);
#else
	return dfxgui_GetEffectInstance()->getparametergroup(inParameterID);
#endif
}

//-----------------------------------------------------------------------------
std::string DfxGuiEditor::GetParameterGroupName(size_t inGroupIndex)
{
#ifdef TARGET_API_AUDIOUNIT
	return dfxgui_GetPropertyAsString(dfx::kPluginProperty_ParameterGroupName, dfx::kScope_Global,
									  static_cast<unsigned int>(inGroupIndex));
#else
	return dfxgui_GetEffectInstance()->getparametergroupname(inGroupIndex);
#endif
}

//-----------------------------------------------------------------------------
std::string DfxGuiEditor::getparametername(dfx::ParameterID inParameterID)
{
#ifdef TARGET_API_AUDIOUNIT
	if (auto const parameterInfo = dfxgui_GetParameterInfo(inParameterID))
	{
		if ((parameterInfo->flags & kAudioUnitParameterFlag_HasCFNameString) && parameterInfo->cfNameString)
		{
			if (auto const nameC = dfx::CreateCStringFromCFString(parameterInfo->cfNameString))
			{
				return nameC.get();
			}
		}
		return parameterInfo->name;
	}
	return {};
#else
	return dfxgui_GetEffectInstance()->getparametername(inParameterID);
#endif
}

//-----------------------------------------------------------------------------
float DfxGuiEditor::dfxgui_ExpandParameterValue(dfx::ParameterID inParameterID, float inValue)
{
#ifdef TARGET_API_AUDIOUNIT
	dfx::ParameterValueConversionRequest request;
	request.inConversionType = dfx::ParameterValueConversionType::Expand;
	request.inValue = inValue;
	size_t dataSize = sizeof(request);
	auto const status = dfxgui_GetProperty(dfx::kPluginProperty_ParameterValueConversion, dfx::kScope_Global, 
										   inParameterID, &request, dataSize);
	if (status == dfx::kStatus_NoError)
	{
		return request.outValue;
	}
	else
	{
		assert(false);  // really the above should not be failing
		auto const auParam = dfxgui_MakeAudioUnitParameter(inParameterID);
		return AUParameterValueFromLinear(inValue, &auParam);
	}
#else
	return dfxgui_GetEffectInstance()->expandparametervalue(inParameterID, inValue);
#endif
}

//-----------------------------------------------------------------------------
float DfxGuiEditor::dfxgui_ContractParameterValue(dfx::ParameterID inParameterID, float inValue)
{
#ifdef TARGET_API_AUDIOUNIT
	dfx::ParameterValueConversionRequest request;
	request.inConversionType = dfx::ParameterValueConversionType::Contract;
	request.inValue = inValue;
	size_t dataSize = sizeof(request);
	auto const status = dfxgui_GetProperty(dfx::kPluginProperty_ParameterValueConversion, dfx::kScope_Global, 
										   inParameterID, &request, dataSize);
	if (status == dfx::kStatus_NoError)
	{
		return request.outValue;
	}
	else
	{
		assert(false);  // really the above should not be failing
		auto const auParam = dfxgui_MakeAudioUnitParameter(inParameterID);
		return AUParameterValueToLinear(inValue, &auParam);
	}
#else
	return dfxgui_GetEffectInstance()->contractparametervalue(inParameterID, inValue);
#endif
}

//-----------------------------------------------------------------------------
float DfxGuiEditor::GetParameter_minValue(dfx::ParameterID inParameterID)
{
#ifdef TARGET_API_AUDIOUNIT
	if (auto const parameterInfo = dfxgui_GetParameterInfo(inParameterID))
	{
		return parameterInfo->minValue;
	}
	return 0.f;
#else
	return dfxgui_GetEffectInstance()->getparametermin_f(inParameterID);
#endif
}

//-----------------------------------------------------------------------------
float DfxGuiEditor::GetParameter_maxValue(dfx::ParameterID inParameterID)
{
#ifdef TARGET_API_AUDIOUNIT
	if (auto const parameterInfo = dfxgui_GetParameterInfo(inParameterID))
	{
		return parameterInfo->maxValue;
	}
	return 0.f;
#else
	return dfxgui_GetEffectInstance()->getparametermax_f(inParameterID);
#endif
}

//-----------------------------------------------------------------------------
float DfxGuiEditor::GetParameter_defaultValue(dfx::ParameterID inParameterID)
{
#ifdef TARGET_API_AUDIOUNIT
	if (auto const parameterInfo = dfxgui_GetParameterInfo(inParameterID))
	{
		return parameterInfo->defaultValue;
	}
	return 0.f;
#else
	return dfxgui_GetEffectInstance()->getparameterdefault_f(inParameterID);
#endif
}

//-----------------------------------------------------------------------------
DfxParam::Value::Type DfxGuiEditor::GetParameterValueType(dfx::ParameterID inParameterID)
{
#ifdef TARGET_API_AUDIOUNIT
	return dfxgui_GetProperty<DfxParam::Value::Type>(dfx::kPluginProperty_ParameterValueType, 
													 dfx::kScope_Global, 
													 inParameterID).value_or(DfxParam::Value::Type::Float);
#else
	return dfxgui_GetEffectInstance()->getparametervaluetype(inParameterID);
#endif
}

//-----------------------------------------------------------------------------
DfxParam::Unit DfxGuiEditor::GetParameterUnit(dfx::ParameterID inParameterID)
{
#ifdef TARGET_API_AUDIOUNIT
	return dfxgui_GetProperty<DfxParam::Unit>(dfx::kPluginProperty_ParameterUnit, 
											  dfx::kScope_Global, 
											  inParameterID).value_or(DfxParam::Unit::Generic);
#else
	return dfxgui_GetEffectInstance()->getparameterunit(inParameterID);
#endif
}

#ifdef TARGET_API_AUDIOUNIT
//-----------------------------------------------------------------------------
std::optional<DfxGuiEditor::AUParameterInfo> DfxGuiEditor::dfxgui_GetParameterInfo(AudioUnitParameterID inParameterID)
{
	auto const parameterInfo = dfxgui_GetProperty<AudioUnitParameterInfo>(kAudioUnitProperty_ParameterInfo, 
																		  kAudioUnitScope_Global, inParameterID);
	return parameterInfo ? std::make_optional(AUParameterInfo(*parameterInfo)) : std::nullopt;
}
#endif

//-----------------------------------------------------------------------------
size_t DfxGuiEditor::GetNumParameters()
{
#ifdef TARGET_API_AUDIOUNIT
	// XXX questionable implementation; return max ID value +1 instead?
	size_t dataSize {};
	dfx::PropertyFlags propFlags {};
	auto const status = dfxgui_GetPropertyInfo(kAudioUnitProperty_ParameterList, dfx::kScope_Global, 0, dataSize, propFlags);
	if (status == dfx::kStatus_NoError)
	{
		assert((dataSize % sizeof(AudioUnitParameterID)) == 0);
		return dataSize / sizeof(AudioUnitParameterID);
	}
#elifdef TARGET_API_VST
	return dfx::math::ToUnsigned(getEffect()->getAeffect()->numParams);
#elifdef TARGET_API_RTAS
	return dfxgui_GetEffectInstance()->getnumparameters();
#endif
	return 0;
}

//-----------------------------------------------------------------------------
std::vector<dfx::ParameterID> DfxGuiEditor::GetParameterList() const
{
#ifdef TARGET_API_AUDIOUNIT
	std::lock_guard const guard(mParameterListLock);
#endif
	return mParameterList;
}

#ifdef TARGET_API_AUDIOUNIT
//-----------------------------------------------------------------------------
AudioUnitParameter DfxGuiEditor::dfxgui_MakeAudioUnitParameter(AudioUnitParameterID inParameterID, AudioUnitScope inScope, AudioUnitElement inElement)
{
	AudioUnitParameter outParam {};
	outParam.mAudioUnit = dfxgui_GetEffectInstance();
	outParam.mParameterID = inParameterID;
	outParam.mScope = inScope;
	outParam.mElement = inElement;
	return outParam;
}

//-----------------------------------------------------------------------------
std::vector<dfx::ParameterID> DfxGuiEditor::CreateParameterList(AudioUnitScope inScope)
{
	size_t dataSize {};
	dfx::PropertyFlags propFlags {};
	auto status = dfxgui_GetPropertyInfo(kAudioUnitProperty_ParameterList, inScope, 0, dataSize, propFlags);
	if (status != dfx::kStatus_NoError)
	{
		return {};
	}

	constexpr auto getParameterCount = [](size_t byteCount)
	{
		assert((byteCount % sizeof(AudioUnitParameterID)) == 0);
		return byteCount / sizeof(AudioUnitParameterID);
	};
	auto const numParameters = getParameterCount(dataSize);
	if (numParameters == 0)
	{
		return {};
	}

	std::vector<AudioUnitParameterID> parameterList(numParameters);
	status = dfxgui_GetProperty(kAudioUnitProperty_ParameterList, inScope, 0, parameterList.data(), dataSize);
	if (status != dfx::kStatus_NoError)
	{
		return {};
	}
	assert(getParameterCount(dataSize) == parameterList.size());

	mAUMaxParameterID = *std::ranges::max_element(parameterList);

	return std::vector<dfx::ParameterID>(parameterList.cbegin(), parameterList.cend());
}
#endif

//-----------------------------------------------------------------------------
dfx::StatusCode DfxGuiEditor::dfxgui_GetPropertyInfo(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned int inItemIndex, 
													 size_t& outDataSize, dfx::PropertyFlags& outFlags)
{
#ifdef TARGET_API_AUDIOUNIT
	AUSDK_Require(dfxgui_GetEffectInstance(), kAudioUnitErr_Uninitialized);

	UInt32 auDataSize {};
	Boolean writable {};
	auto const status = AudioUnitGetPropertyInfo(dfxgui_GetEffectInstance(), inPropertyID, inScope, inItemIndex, &auDataSize, &writable);
	if (status == noErr)
	{
		outDataSize = auDataSize;
		outFlags = dfx::kPropertyFlag_Readable;
		if (writable)
		{
			outFlags |= dfx::kPropertyFlag_Writable;
		}
	}
	return status;
#else
	return dfxgui_GetEffectInstance()->dfx_GetPropertyInfo(inPropertyID, inScope, inItemIndex, outDataSize, outFlags);
#endif
}

//-----------------------------------------------------------------------------
dfx::StatusCode DfxGuiEditor::dfxgui_GetProperty(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned int inItemIndex, 
												 void* outData, size_t& ioDataSize)
{
#ifdef TARGET_API_AUDIOUNIT
	AUSDK_Require(dfxgui_GetEffectInstance(), kAudioUnitErr_Uninitialized);

	auto auDataSize = static_cast<UInt32>(ioDataSize);
	auto const status = AudioUnitGetProperty(dfxgui_GetEffectInstance(), inPropertyID, inScope, inItemIndex, outData, &auDataSize);
	if (status == noErr)
	{
		ioDataSize = auDataSize;
	}
	return status;
#else
	return dfxgui_GetEffectInstance()->dfx_GetProperty(inPropertyID, inScope, inItemIndex, outData);
#endif
}

//-----------------------------------------------------------------------------
std::string DfxGuiEditor::dfxgui_GetPropertyAsString(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned int inItemIndex)
{
	size_t dataSize {};
	dfx::PropertyFlags flags {};
	if (dfxgui_GetPropertyInfo(inPropertyID, inScope, inItemIndex, dataSize, flags) == dfx::kStatus_NoError)
	{
		std::vector<char> text(dataSize);
		if (dfxgui_GetProperty(inPropertyID, inScope, inItemIndex, text.data(), dataSize) == dfx::kStatus_NoError)
		{
			return text.data();
		}
	}
	return {};
}

//-----------------------------------------------------------------------------
dfx::StatusCode DfxGuiEditor::dfxgui_SetProperty(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned int inItemIndex, 
												 void const* inData, size_t inDataSize)
{
#ifdef TARGET_API_AUDIOUNIT
	AUSDK_Require(dfxgui_GetEffectInstance(), kAudioUnitErr_Uninitialized);

	return AudioUnitSetProperty(dfxgui_GetEffectInstance(), inPropertyID, inScope, inItemIndex, 
								inData, static_cast<UInt32>(inDataSize));
#else
	auto const status = dfxgui_GetEffectInstance()->dfx_SetProperty(inPropertyID, inScope, inItemIndex, inData, inDataSize);
	// AU system framework handles this notification in AU, but VST needs to manage it manually.
	// Only propagate notification if the specific property has been registered.
	if ((status == dfx::kStatus_NoError) && IsPropertyRegistered(inPropertyID, inScope, inItemIndex))
	{
		HandlePropertyChange(inPropertyID, inScope, inItemIndex);
	}
	return status;
#endif
}

#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
dfx::UniqueCFType<CFURLRef> DfxGuiEditor::PathToCFURL(VSTGUI::UTF8StringPtr inFilePath)
{
	return dfx::MakeUniqueCFType(CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault,
																		 reinterpret_cast<UInt8 const*>(inFilePath),
																		 static_cast<CFIndex>(std::strlen(inFilePath)),
																		 false));
}
#endif

//-----------------------------------------------------------------------------
void DfxGuiEditor::LoadPresetFile()
{
	try
	{
		VSTGUI::SharedPointer<VSTGUI::CNewFileSelector> fileSelector(VSTGUI::CNewFileSelector::create(getFrame(), VSTGUI::CNewFileSelector::kSelectFile), false);
		Require(fileSelector, "could not create load file dialog");
		fileSelector->setTitle("Open");
#ifdef TARGET_API_AUDIOUNIT
		fileSelector->addFileExtension(kAUPresetFileExtension);
		auto const presetsDirURL = dfx::MakeUniqueCFType(FindPresetsDirForAU(AudioComponentInstanceGetComponent(dfxgui_GetEffectInstance()), kDFXFileSystemDomain_User, false));
		if (presetsDirURL)
		{
			if (auto const presetsDirPathCF = dfx::MakeUniqueCFType(CFURLCopyFileSystemPath(presetsDirURL.get(), kCFURLPOSIXPathStyle)))
			{
				if (auto const presetsDirPathC = dfx::CreateCStringFromCFString(presetsDirPathCF.get()))
				{
					fileSelector->setInitialDirectory(presetsDirPathC.get());
				}
			}
		}
#elifdef TARGET_API_VST
		fileSelector->addFileExtension(kVSTProgramFileExtension);
//		fileSelector->addFileExtension(kVSTBankFileExtension);  // TODO: could also support banks of programs
#endif
		fileSelector->run([this](VSTGUI::CNewFileSelector* inFileSelector)
		{
			if (auto const filePath = inFileSelector->getSelectedFile(0))
			{
				try
				{
#ifdef TARGET_API_AUDIOUNIT
					auto const fileURL = PathToCFURL(filePath);
					Require(fileURL.get(), "failed to create file URL");
					auto const status = RestoreAUStateFromPresetFile(dfxgui_GetEffectInstance(), fileURL.get());
					ausdk::ThrowQuietIf(status != noErr, status);
#elifdef TARGET_API_VST
					RestoreVSTStateFromProgramFile(filePath);
#else
	#warning "implementation missing"
					assert(false);  // TODO: implement
#endif  // TARGET_API_AUDIOUNIT/TARGET_API_VST
				}
				catch (std::exception const& e)
				{
					auto const message = "failed to load preset file:\n"s + e.what();
					ShowMessage(message);
				}
			}
		});
	}
	catch (std::exception const& e)
	{
		ShowMessage(e.what());
	}
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::SavePresetFile()
{
	constexpr char const* const errorTitle = "failed to save preset file:\n";
	try
	{
#ifdef TARGET_API_AUDIOUNIT
		mTextEntryDialog = VSTGUI::makeOwned<DGTextEntryDialog>("Save preset file", "save as:", 
																DGDialog::kButtons_OKCancelOther, 
																"Save", nullptr, "Choose custom location...");
		if (auto const button = mTextEntryDialog->getButton(DGDialog::Selection::kSelection_Other))
		{
			constexpr char const* const helpText = "choose a specific location in which to save rather than the standard location (note:  this means that your presets will not be easily accessible in other host applications)";
			button->setTooltipText(helpText);
		}
		auto const textEntryCallback = [this](DGDialog::Selection inSelection, std::string const& inText, dfx::ParameterID)
		{
			try
			{
				switch (inSelection)
				{
					case DGDialog::kSelection_OK:
					{
						if (inText.empty())
						{
							return false;
						}
						auto const cfText = dfx::MakeUniqueCFType(CFStringCreateWithCString(kCFAllocatorDefault, inText.c_str(), kCFStringEncodingUTF8));
						Require(cfText.get(), "could not create platform representation of text input");
						auto const pluginBundle = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));
						assert(pluginBundle);
						auto const status = SaveAUStateToPresetFile_Bundle(dfxgui_GetEffectInstance(), cfText.get(), nullptr, true, pluginBundle);
						if (status == userCanceledErr)
						{
							return false;
						}
						Require(status == noErr, "error code " + std::to_string(status));
						return true;
					}
					case DGDialog::kSelection_Other:
					{
						VSTGUI::SharedPointer<VSTGUI::CNewFileSelector> fileSelector(VSTGUI::CNewFileSelector::create(getFrame(), VSTGUI::CNewFileSelector::kSelectSaveFile), false);
						Require(fileSelector, "could not create save file dialog");
						fileSelector->setTitle("Save");
						fileSelector->setDefaultExtension(kAUPresetFileExtension);
						if (!inText.empty())
						{
							fileSelector->setDefaultSaveName(inText.c_str());
						}
						fileSelector->run([this](VSTGUI::CNewFileSelector* inFileSelector)
						{
							if (auto const filePath = inFileSelector->getSelectedFile(0))
							{
								try
								{
									auto const fileURL = PathToCFURL(filePath);
									Require(fileURL.get(), "could not create platform representation of preset file location");
									auto const pluginBundle = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));
									assert(pluginBundle);
									auto const status = CustomSaveAUPresetFile_Bundle(dfxgui_GetEffectInstance(), fileURL.get(), false, pluginBundle);
									Require((status == noErr) || (status == userCanceledErr), 
											"error code " + std::to_string(status));
								}
								catch (std::exception const& e)
								{
									auto const message = std::string(errorTitle) + e.what();
									ShowMessage(message);
								}
							}
						});
						return true;
					}
					default:
						break;
				}
			}
			catch (std::exception const& e)
			{
				mPendingErrorMessage = std::string(errorTitle) + e.what();
			}
			return true;
		};
		auto const success = mTextEntryDialog->runModal(getFrame(), textEntryCallback);
		Require(success, "could not display save preset dialog");
#elifdef TARGET_API_VST
		VSTGUI::SharedPointer<VSTGUI::CNewFileSelector> fileSelector(VSTGUI::CNewFileSelector::create(getFrame(), VSTGUI::CNewFileSelector::kSelectSaveFile), false);
		Require(fileSelector, "could not create save file dialog");
		fileSelector->setTitle("Save");
		fileSelector->setDefaultExtension(kVSTProgramFileExtension);
		fileSelector->run([this](VSTGUI::CNewFileSelector* inFileSelector)
		{
			if (auto const filePath = inFileSelector->getSelectedFile(0))
			{
				try
				{
					SaveVSTStateToProgramFile(filePath);
				}
				catch (std::exception const& e)
				{
					auto const message = std::string(errorTitle) + e.what();
					ShowMessage(message);
				}
			}
		});
#endif  // TARGET_API_AUDIOUNIT/TARGET_API_VST
	}
	catch (std::exception const& e)
	{
		ShowMessage(e.what());
	}
}

//-----------------------------------------------------------------------------
std::tuple<uint8_t, uint8_t, uint8_t> DfxGuiEditor::getPluginVersion() noexcept
{
	auto const compositeVersion = dfx::CompositePluginVersionNumberValue();
	return {(compositeVersion & 0xFFFF0000) >> 16, (compositeVersion & 0x0000FF00) >> 8, compositeVersion & 0x000000FF};
}

//-----------------------------------------------------------------------------
DGEditorListenerInstance DfxGuiEditor::dfxgui_GetEffectInstance()
{
	return static_cast<DGEditorListenerInstance>(getEffect());
}

#if defined(TARGET_API_AUDIOUNIT) && DEBUG
//-----------------------------------------------------------------------------
DfxPlugin* DfxGuiEditor::dfxgui_GetDfxPluginInstance()
{
	return dfxgui_GetProperty<DfxPlugin*>(dfx::kPluginProperty_DfxPluginInstance).value_or(nullptr);
}
#endif

#if TARGET_PLUGIN_USES_MIDI
//-----------------------------------------------------------------------------
void DfxGuiEditor::setmidilearning(bool inLearnMode)
{
#ifdef TARGET_API_AUDIOUNIT
	dfxgui_SetProperty(dfx::kPluginProperty_MidiLearn, static_cast<Boolean>(inLearnMode));
#else
	dfxgui_GetEffectInstance()->setmidilearning(inLearnMode);
#endif
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::getmidilearning()
{
#ifdef TARGET_API_AUDIOUNIT
	return dfxgui_GetProperty<Boolean>(dfx::kPluginProperty_MidiLearn).value_or(false);
#else
	return dfxgui_GetEffectInstance()->getmidilearning();
#endif
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::resetmidilearn()
{
#ifdef TARGET_API_AUDIOUNIT  
	dfxgui_SetProperty(dfx::kPluginProperty_ResetMidiLearn, Boolean{} /* irrelevant */);
#else
	dfxgui_GetEffectInstance()->resetmidilearn();
#endif
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::setmidilearner(dfx::ParameterID inParameterID)
{
	if (dfxgui_IsValidParameterID(inParameterID) && !getmidilearning())
	{
		setmidilearning(true);
	}
#ifdef TARGET_API_AUDIOUNIT
	dfxgui_SetProperty(dfx::kPluginProperty_MidiLearner, static_cast<uint32_t>(inParameterID));
#else
	dfxgui_GetEffectInstance()->setmidilearner(inParameterID);
#endif
}

//-----------------------------------------------------------------------------
dfx::ParameterID DfxGuiEditor::getmidilearner()
{
#ifdef TARGET_API_AUDIOUNIT
	return dfxgui_GetProperty<uint32_t>(dfx::kPluginProperty_MidiLearner).value_or(dfx::kParameterID_Invalid);
#else
	return dfxgui_GetEffectInstance()->getmidilearner();
#endif
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::ismidilearner(dfx::ParameterID inParameterID)
{
	return (getmidilearner() == inParameterID);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::setparametermidiassignment(dfx::ParameterID inParameterID, dfx::ParameterAssignment const& inAssignment)
{
#ifdef TARGET_API_AUDIOUNIT
	dfxgui_SetProperty(dfx::kPluginProperty_ParameterMidiAssignment, dfx::kScope_Global, inParameterID, inAssignment);
#else
	dfxgui_GetEffectInstance()->setparametermidiassignment(inParameterID, inAssignment);
#endif
}

//-----------------------------------------------------------------------------
dfx::ParameterAssignment DfxGuiEditor::getparametermidiassignment(dfx::ParameterID inParameterID)
{
#ifdef TARGET_API_AUDIOUNIT
	dfx::ParameterAssignment none;
	none.mEventType = dfx::MidiEventType::None;
	return dfxgui_GetProperty<dfx::ParameterAssignment>(dfx::kPluginProperty_ParameterMidiAssignment,
														dfx::kScope_Global,
														inParameterID).value_or(none);
#else
	return dfxgui_GetEffectInstance()->getparametermidiassignment(inParameterID);
#endif
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::parametermidiunassign(dfx::ParameterID inParameterID)
{
	dfx::ParameterAssignment parameterAssignment;
	parameterAssignment.mEventType = dfx::MidiEventType::None;
	setparametermidiassignment(inParameterID, parameterAssignment);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::setMidiAssignmentsUseChannel(bool inEnable)
{
#ifdef TARGET_API_AUDIOUNIT
	dfxgui_SetProperty(dfx::kPluginProperty_MidiAssignmentsUseChannel, static_cast<Boolean>(inEnable));
#else
	dfxgui_GetEffectInstance()->setMidiAssignmentsUseChannel(inEnable);
#endif
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::getMidiAssignmentsUseChannel()
{
#ifdef TARGET_API_AUDIOUNIT
	return dfxgui_GetProperty<Boolean>(dfx::kPluginProperty_MidiAssignmentsUseChannel).value_or(false);
#else
	return dfxgui_GetEffectInstance()->getMidiAssignmentsUseChannel();
#endif
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::setMidiAssignmentsSteal(bool inEnable)
{
#ifdef TARGET_API_AUDIOUNIT
	dfxgui_SetProperty(dfx::kPluginProperty_MidiAssignmentsSteal, static_cast<Boolean>(inEnable));
#else
	dfxgui_GetEffectInstance()->setMidiAssignmentsSteal(inEnable);
#endif
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::getMidiAssignmentsSteal()
{
#ifdef TARGET_API_AUDIOUNIT
	return dfxgui_GetProperty<Boolean>(dfx::kPluginProperty_MidiAssignmentsSteal).value_or(false);
#else
	return dfxgui_GetEffectInstance()->getMidiAssignmentsSteal();
#endif
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::TextEntryForParameterMidiCC(dfx::ParameterID inParameterID)
{
	if (!getFrame())
	{
		return;
	}

	mTextEntryDialog = VSTGUI::makeOwned<DGTextEntryDialog>(inParameterID, getparametername(inParameterID), "enter value:");
	// initialize the text with the current CC assignment, if there is one
	auto const currentParameterAssignment = getparametermidiassignment(inParameterID);
	if (currentParameterAssignment.mEventType == dfx::MidiEventType::CC)
	{
		mTextEntryDialog->setText(std::to_string(currentParameterAssignment.mEventNum));
	}

	auto const textEntryCallback = [this](std::string const& inText, dfx::ParameterID inParameterID)
	{
		int value {};
		auto const readCount = std::sscanf(dfx::SanitizeNumericalInput(inText).c_str(), "%d", &value);
		if ((readCount < 1) || (readCount == EOF) || (value < 0) || (value > DfxMidi::kMaxValue))
		{
			return false;
		}
		dfx::ParameterAssignment parameterAssignment;
		parameterAssignment.mEventType = dfx::MidiEventType::CC;
		parameterAssignment.mEventChannel = getparametermidiassignment(inParameterID).mEventChannel;  // persist any existing choice
		parameterAssignment.mEventNum = value;
		setparametermidiassignment(inParameterID, parameterAssignment);
		return true;
	};
	if (!mTextEntryDialog->runModal(getFrame(), textEntryCallback))
	{
		ShowMessage("could not display text entry dialog");
	}
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::TextEntryForParameterMidiChannel(dfx::ParameterID inParameterID)
{
	if (!getFrame())
	{
		return;
	}

	mTextEntryDialog = VSTGUI::makeOwned<DGTextEntryDialog>(inParameterID, getparametername(inParameterID), "enter value:");
	mTextEntryDialog->setText(std::to_string(getparametermidiassignment(inParameterID).mEventChannel + 1));

	auto const textEntryCallback = [this](std::string const& inText, dfx::ParameterID inParameterID)
	{
		int value {};
		auto const readCount = std::sscanf(dfx::SanitizeNumericalInput(inText).c_str(), "%d", &value);
		value -= 1;  // transform from display value to zero-based index as used by MIDI
		if ((readCount < 1) || (readCount == EOF) || (value < 0) || (value > DfxMidi::kMaxChannelValue))
		{
			return false;
		}
		auto parameterAssignment = getparametermidiassignment(inParameterID);
		parameterAssignment.mEventChannel = value;
		setparametermidiassignment(inParameterID, parameterAssignment);
		return true;
	};
	if (!mTextEntryDialog->runModal(getFrame(), textEntryCallback))
	{
		ShowMessage("could not display text entry dialog");
	}
}

#endif
// TARGET_PLUGIN_USES_MIDI

//-----------------------------------------------------------------------------
size_t DfxGuiEditor::getNumInputChannels()
{
#ifdef TARGET_API_AUDIOUNIT
	auto const streamDesc = dfxgui_GetProperty<AudioStreamBasicDescription>(kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input);
	return streamDesc ? streamDesc->mChannelsPerFrame : 0;
#elifdef TARGET_API_VST
	return dfx::math::ToUnsigned(getEffect()->getAeffect()->numInputs);
#elifdef TARGET_API_RTAS
	return dfxgui_GetEffectInstance()->getnuminputs();
#endif
}

//-----------------------------------------------------------------------------
size_t DfxGuiEditor::getNumOutputChannels()
{
#ifdef TARGET_API_AUDIOUNIT
	auto const streamDesc = dfxgui_GetProperty<AudioStreamBasicDescription>(kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output);
	return streamDesc ? streamDesc->mChannelsPerFrame : 0;
#elifdef TARGET_API_VST
	return dfx::math::ToUnsigned(getEffect()->getAeffect()->numOutputs);
#elifdef TARGET_API_RTAS
	return dfxgui_GetEffectInstance()->getnumoutputs();
#endif
}

//-----------------------------------------------------------------------------
std::optional<double> DfxGuiEditor::getSmoothedAudioValueTime()
{
#ifdef TARGET_API_AUDIOUNIT
	return dfxgui_GetProperty<double>(dfx::kPluginProperty_SmoothedAudioValueTime);
#else
	return dfxgui_GetEffectInstance()->getSmoothedAudioValueTime();
#endif
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::setSmoothedAudioValueTime(double inSmoothingTimeInSeconds)
{
#ifdef TARGET_API_AUDIOUNIT
	dfxgui_SetProperty(dfx::kPluginProperty_SmoothedAudioValueTime, inSmoothingTimeInSeconds);
#else
	dfxgui_GetEffectInstance()->setSmoothedAudioValueTime(inSmoothingTimeInSeconds);
#endif
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::TextEntryForSmoothedAudioValueTime()
{
	auto const currentValue = getSmoothedAudioValueTime();
	if (!currentValue || !getFrame())
	{
		return;
	}

	mTextEntryDialog = VSTGUI::makeOwned<DGTextEntryDialog>("parameter value smoothing time", "enter seconds:");
	mTextEntryDialog->setText(std::to_string(*currentValue));

	auto const textEntryCallback = [this](std::string const& inText, dfx::ParameterID)
	{
		double value {};
		auto const readCount = std::sscanf(dfx::SanitizeNumericalInput(inText).c_str(), "%lf", &value);
		if ((readCount < 1) || (readCount == EOF) || (value < 0.))
		{
			return false;
		}
		setSmoothedAudioValueTime(value);
		return true;
	};
	if (!mTextEntryDialog->runModal(getFrame(), textEntryCallback))
	{
		ShowMessage("could not display text entry dialog");
	}
}


namespace
{

static constexpr int32_t kDfxGui_ContextualMenuStyle = VSTGUI::COptionMenu::kMultipleCheckStyle;  // necessary for menu entry checkmarks to show up

//-----------------------------------------------------------------------------
VSTGUI::CMenuItem* DFX_AppendCommandItemToMenu(VSTGUI::COptionMenu& ioMenu, VSTGUI::UTF8String const& inMenuItemText, 
											   VSTGUI::CCommandMenuItem::SelectedCallbackFunction&& inCommandCallback, 
											   bool inEnabled = true, bool inChecked = false)
{
	if (auto const menuItem = new VSTGUI::CCommandMenuItem(inMenuItemText))
	{
		menuItem->setActions(std::move(inCommandCallback));
		menuItem->setEnabled(inEnabled);
		menuItem->setChecked(inChecked);
		return ioMenu.addEntry(menuItem);
	}
	return nullptr;
}

}  // namespace

//-----------------------------------------------------------------------------
bool DfxGuiEditor::handleContextualMenuClick(VSTGUI::CControl* inControl, VSTGUI::MouseEventButtonState inButtonState)
{
	if (!inButtonState.isRight())
	{
		return false;
	}

	if (!getFrame())
	{
		return false;
	}

	auto const dgControl = dynamic_cast<IDGControl*>(inControl);
	auto const popupMenu = createContextualMenu(dgControl);

	// --------- show the contextual menu ---------
	VSTGUI::CPoint mousePos;
	[[maybe_unused]] auto const mousePosSuccess = getFrame()->getCurrentMouseLocation(mousePos);
	assert(mousePosSuccess);
	return popupMenu->popup(getFrame(), mousePos);
}

//-----------------------------------------------------------------------------
VSTGUI::SharedPointer<VSTGUI::COptionMenu> DfxGuiEditor::createContextualMenu(IDGControl* inControl)
{
	auto const resultMenu = VSTGUI::makeOwned<VSTGUI::COptionMenu>();
	resultMenu->setStyle(kDfxGui_ContextualMenuStyle);

	// populate the parameter-specific section of the menu
	bool const parameterAttached = inControl && inControl->isParameterAttached();
	if (parameterAttached)
	{
		auto const addParameterSubMenu = [this, &resultMenu](IDGControl const* control)
		{
			auto const parameterID = control->getParameterID();
			if (auto const parameterSubMenu = createParameterContextualMenu(parameterID))
			{
				resultMenu->addEntry(parameterSubMenu, getparametername(parameterID));
			}
		};
		addParameterSubMenu(inControl);
		for (auto const& child : inControl->getChildren())
		{
			addParameterSubMenu(child);
		}
		if (resultMenu->getNbEntries())
		{
			resultMenu->addSeparator();  // preface the global commands section with a divider
		}
	}

	// populate the global section of the menu
	resultMenu->addSeparator();
	constexpr bool automationGesture = true;
	DFX_AppendCommandItemToMenu(*resultMenu, "Reset all parameter values to default",
								std::bind(&DfxGuiEditor::setparameters_default, this, automationGesture));
	DFX_AppendCommandItemToMenu(*resultMenu, "Randomize all parameter values",
								std::bind(&DfxGuiEditor::randomizeparameters, this, automationGesture));  // XXX yes to writing automation data?
	DFX_AppendCommandItemToMenu(*resultMenu, "Generate parameter automation snapshot",
								std::bind(&DfxGuiEditor::GenerateParametersAutomationSnapshot, this));
	if (getSmoothedAudioValueTime().has_value())
	{
		DFX_AppendCommandItemToMenu(*resultMenu, "Set parameter value smoothing time...",
									std::bind(&DfxGuiEditor::TextEntryForSmoothedAudioValueTime, this));
	}
	if (!parameterAttached)
	{
		if (auto const parametersSubMenu = createParametersContextualMenu())
		{
			resultMenu->addEntry(parametersSubMenu, "Parameters");
		}
	}

	resultMenu->addSeparator();
	DFX_AppendCommandItemToMenu(*resultMenu, "Copy settings", std::bind(&DfxGuiEditor::copySettings, this));
	{
		bool currentClipboardIsPastable {};
		pasteSettings(&currentClipboardIsPastable);
		DFX_AppendCommandItemToMenu(*resultMenu, "Paste settings", std::bind(&DfxGuiEditor::pasteSettings, this, nullptr), currentClipboardIsPastable);
	}
#ifndef TARGET_API_RTAS  // RTAS has no API for preset files, Pro Tools handles them
	#ifdef TARGET_API_VST
	std::string const presetFileLabel("program");
	#else
	std::string const presetFileLabel("preset");
	#endif
	DFX_AppendCommandItemToMenu(*resultMenu, "Save " + presetFileLabel + " file...", std::bind(&DfxGuiEditor::SavePresetFile, this));
	DFX_AppendCommandItemToMenu(*resultMenu, "Load " + presetFileLabel + " file...", std::bind(&DfxGuiEditor::LoadPresetFile, this));
#endif

#if TARGET_PLUGIN_USES_MIDI
	resultMenu->addSeparator();
	DFX_AppendCommandItemToMenu(*resultMenu, "MIDI learn",
								std::bind(&DfxGuiEditor::setmidilearning, this, !getmidilearning()), 
								true, getmidilearning());
	DFX_AppendCommandItemToMenu(*resultMenu, "MIDI assignments reset", std::bind(&DfxGuiEditor::resetmidilearn, this));
	DFX_AppendCommandItemToMenu(*resultMenu, "MIDI assignments use channel",
								std::bind(&DfxGuiEditor::setMidiAssignmentsUseChannel, this, !getMidiAssignmentsUseChannel()), 
								true, getMidiAssignmentsUseChannel());
	DFX_AppendCommandItemToMenu(*resultMenu, "Steal MIDI assignments",
								std::bind(&DfxGuiEditor::setMidiAssignmentsSteal, this, !getMidiAssignmentsSteal()), 
								true, getMidiAssignmentsSteal());
#endif

	resultMenu->addSeparator();
	DFX_AppendCommandItemToMenu(*resultMenu, PLUGIN_NAME_STRING " manual", std::bind(&dfx::LaunchDocumentation));
#if TARGET_PLUGIN_USES_MIDI
	DFX_AppendCommandItemToMenu(*resultMenu, PLUGIN_CREATOR_NAME_STRING " MIDI features manual", std::bind(&dfx::LaunchURL, DESTROYFX_URL "/docs/destroy-fx-midi.html"));
#endif
	DFX_AppendCommandItemToMenu(*resultMenu, "Open " PLUGIN_CREATOR_NAME_STRING " web site",
								std::bind(&dfx::LaunchURL, PLUGIN_HOMEPAGE_URL));
	DFX_AppendCommandItemToMenu(*resultMenu, "Acknowledgements", std::bind(&DfxGuiEditor::ShowAcknowledgements, this));

	resultMenu->addSeparator();
	auto const [versionMajor, versionMinor, versionBugfix] = getPluginVersion();
	auto const versionText = std::to_string(versionMajor) + "." + std::to_string(versionMinor) + "." + std::to_string(versionBugfix);
	DFX_AppendCommandItemToMenu(*resultMenu, "Version " + versionText, {}, false);

	resultMenu->cleanupSeparators(true);
	return resultMenu;
}

//-----------------------------------------------------------------------------
VSTGUI::SharedPointer<VSTGUI::COptionMenu> DfxGuiEditor::createParameterContextualMenu(dfx::ParameterID inParameterID)
{
	assert(dfxgui_IsValidParameterID(inParameterID));

	auto const resultMenu = VSTGUI::makeOwned<VSTGUI::COptionMenu>();
	resultMenu->setStyle(kDfxGui_ContextualMenuStyle);

	resultMenu->addSeparator();
	constexpr bool automationGesture = true;
	DFX_AppendCommandItemToMenu(*resultMenu, "Set to default value", 
								std::bind(&DfxGuiEditor::setparameter_default, this, inParameterID, automationGesture));
	if (GetParameterUseValueStrings(inParameterID))
	{
		auto const valueStringsSubMenu = VSTGUI::makeOwned<VSTGUI::COptionMenu>();
		valueStringsSubMenu->setStyle(kDfxGui_ContextualMenuStyle);
		auto const currentValue = getparameter_i(inParameterID);
		auto const minValue = std::lround(GetParameter_minValue(inParameterID));
		auto const maxValue = std::lround(GetParameter_maxValue(inParameterID));
		for (auto i = minValue; i <= maxValue; i++)
		{
			if (auto const valueString = getparametervaluestring(inParameterID, i))
			{
				constexpr bool enabled = true;
				DFX_AppendCommandItemToMenu(*valueStringsSubMenu, valueString->c_str(), 
											std::bind(&DfxGuiEditor::setparameter_i, this, inParameterID, i, automationGesture), 
											enabled, i == currentValue);
			}
		}
		assert(valueStringsSubMenu->getNbEntries() > 0);
		resultMenu->addEntry(valueStringsSubMenu, "Select value");
	}
	else if (GetParameterValueType(inParameterID) == DfxParam::Value::Type::Boolean)
	{
		auto const parameterIsOn = getparameter_b(inParameterID);
		constexpr bool enabled = true;
		DFX_AppendCommandItemToMenu(*resultMenu, "Enable", 
									std::bind(&DfxGuiEditor::setparameter_b, this, inParameterID, !parameterIsOn, automationGesture), 
									enabled, parameterIsOn);
	}
	else
	{
		DFX_AppendCommandItemToMenu(*resultMenu, "Enter a value...", 
									std::bind(&DfxGuiEditor::TextEntryForParameterValue, this, inParameterID));
	}
	DFX_AppendCommandItemToMenu(*resultMenu, "Randomize value", 
								std::bind(&DfxGuiEditor::randomizeparameter, this, inParameterID, automationGesture));  // XXX yes to writing automation data?
	DFX_AppendCommandItemToMenu(*resultMenu, "Generate parameter automation snapshot", 
								std::bind(&DfxGuiEditor::GenerateParameterAutomationSnapshot, this, inParameterID));

#if TARGET_PLUGIN_USES_MIDI
	resultMenu->addSeparator();
	auto const currentParameterAssignment = getparametermidiassignment(inParameterID);
	DFX_AppendCommandItemToMenu(*resultMenu, "MIDI learner", 
								std::bind(&DfxGuiEditor::setmidilearner, this, 
										  ismidilearner(inParameterID) ? dfx::kParameterID_Invalid : inParameterID), 
								true, ismidilearner(inParameterID));
	{
		VSTGUI::UTF8String menuItemText = "Unassign MIDI control";
		bool const enableItem = (currentParameterAssignment.mEventType != dfx::MidiEventType::None);
		// append the current MIDI assignment, if there is one, to the menu item text
		if (enableItem)
		{
			menuItemText += [currentParameterAssignment]() -> std::string
			{
				switch (currentParameterAssignment.mEventType)
				{
					case dfx::MidiEventType::CC:
						return " (CC " + std::to_string(currentParameterAssignment.mEventNum);
					case dfx::MidiEventType::ChannelAftertouch:
						return " (channel aftertouch";
					case dfx::MidiEventType::PitchBend:
						return " (pitchbend";
					case dfx::MidiEventType::Note:
						return " (notes " + dfx::GetNameForMIDINote(currentParameterAssignment.mEventNum) + " - " + dfx::GetNameForMIDINote(currentParameterAssignment.mEventNum2);
					default:
						assert(false);
						return {};
				}
			}();
			if (getMidiAssignmentsUseChannel())
			{
				menuItemText += ", channel " + std::to_string(currentParameterAssignment.mEventChannel + 1);
			}
			menuItemText += ")";
		}
		DFX_AppendCommandItemToMenu(*resultMenu, menuItemText, 
									std::bind(&DfxGuiEditor::parametermidiunassign, this, inParameterID), 
									enableItem);
	}
	DFX_AppendCommandItemToMenu(*resultMenu, "Enter a MIDI CC assignment...", 
								std::bind(&DfxGuiEditor::TextEntryForParameterMidiCC, this, inParameterID));
	DFX_AppendCommandItemToMenu(*resultMenu, "Enter a MIDI channel assignment...",
								std::bind(&DfxGuiEditor::TextEntryForParameterMidiChannel, this, inParameterID),
								getMidiAssignmentsUseChannel() && (currentParameterAssignment.mEventType != dfx::MidiEventType::None));
#endif

	return resultMenu;
}

//-----------------------------------------------------------------------------
VSTGUI::SharedPointer<VSTGUI::COptionMenu> DfxGuiEditor::createParametersContextualMenu()
{
	auto const parameterList = std::ranges::to<std::vector>(std::views::filter(GetParameterList(), [this](auto parameterID)
	{
		return !HasParameterAttribute(parameterID, DfxParam::kAttribute_Hidden);
	}));
	auto const resultMenu = VSTGUI::makeOwned<VSTGUI::COptionMenu>();
	resultMenu->setStyle(kDfxGui_ContextualMenuStyle);
	std::map<size_t, VSTGUI::COptionMenu&> groupSubMenus;

	for (auto const parameterID : parameterList)
	{
		if (auto const parameterSubMenu = createParameterContextualMenu(parameterID))
		{
			if (auto const groupIndex = GetParameterGroup(parameterID))
			{
				auto groupKeyValue = groupSubMenus.find(*groupIndex);
				if (groupKeyValue == groupSubMenus.cend())
				{
					auto const groupSubMenu = VSTGUI::makeOwned<VSTGUI::COptionMenu>();
					groupSubMenu->setStyle(kDfxGui_ContextualMenuStyle);
					[[maybe_unused]] bool itemAdded {};
					std::tie(groupKeyValue, itemAdded) = groupSubMenus.emplace(*groupIndex, *groupSubMenu);
					assert(itemAdded);
					resultMenu->addEntry(groupSubMenu, "[" + GetParameterGroupName(*groupIndex) + "]");
				}
				groupKeyValue->second.addEntry(parameterSubMenu, getparametername(parameterID));
			}
			else
			{
				resultMenu->addEntry(parameterSubMenu, getparametername(parameterID));
			}
		}
	}

	if (resultMenu->getNbEntries() <= 0)
	{
		return {};
	}
	return resultMenu;
}

//-----------------------------------------------------------------------------
dfx::StatusCode DfxGuiEditor::initClipboard()
{
#if TARGET_OS_MAC
	// already initialized (allow for lazy initialization)
	if (mClipboardRef)
	{
		return dfx::kStatus_NoError;
	}
	PasteboardRef clipboardRef_temp = nullptr;
	auto const status = PasteboardCreate(kPasteboardClipboard, &clipboardRef_temp);
	if (status != noErr)
	{
		return status;
	}
	if (!clipboardRef_temp)
	{
		return coreFoundationUnknownErr;
	}
	mClipboardRef.reset(clipboardRef_temp);
	return status;
#else
	return dfx::kStatus_NoError;  // nothing to do
#endif
}

#ifdef TARGET_API_VST
//-----------------------------------------------------------------------------
class SettingsDataPackage final : public VSTGUI::IDataPackage
{
public:
#if TARGET_OS_WIN32  // https://github.com/steinbergmedia/vstgui/issues/217 workaround
	static constexpr auto kDataType = kText;
#else
	static constexpr auto kDataType = kBinary;
#endif

	SettingsDataPackage(std::byte const* inSettingsData, VstInt32 inSettingsDataSize)
	:	mData(encode(inSettingsData, inSettingsDataSize))
	{
	}

	uint32_t getCount() const override
	{
		return mData.empty() ? 0 : 1;
	}

	uint32_t getDataSize(uint32_t inIndex) const override
	{
		if (inIndex >= getCount())
		{
			return 0;
		}
		return static_cast<uint32_t>(mData.size());
	}

	Type getDataType(uint32_t inIndex) const override
	{
		if (inIndex >= getCount())
		{
			return kError;
		}
		return kDataType;
	}

	uint32_t getData(uint32_t inIndex, void const*& outBuffer, Type& outType) const override
	{
		outBuffer = (inIndex < getCount()) ? mData.data() : nullptr;
		outType = getDataType(inIndex);
		return getDataSize(inIndex);
	}

#if TARGET_OS_WIN32
	static std::vector<std::byte> decode(void const* inSettingsData, VstInt32 inSettingsDataSize)
	{
		std::string_view inString(static_cast<char const*>(inSettingsData), inSettingsDataSize);
		// validate input because VSTGUI does not (and 'text' data in clipboard could be anything)

		// Strip exterior whitespace. Aside from being convenient, this
		// also allows us to ignore our padding that we include as a
		// workaround (kPasteTextPadding).
		while (!inString.empty() && std::isspace(inString.front()))
		{
			inString.remove_prefix(1);
		}
		while (!inString.empty() && std::isspace(inString.back()))
		{
			inString.remove_suffix(1);
		}

		// Must start with [dfx-settings] and end with [/dfx-settings];
		// this also allows us to reject non-pathological buffers in
		// constant time.
		if (inString.size() < (kPasteTextPrefix.size() + kPasteTextSuffix.size()))
		{
			return {};
		}
		if (inString.substr(0, kPasteTextPrefix.size()) !=
			kPasteTextPrefix ||
			inString.substr(inString.size() - kPasteTextSuffix.size(),
							std::string_view::npos) !=
			kPasteTextSuffix)
		{
			return {};
		}

		// Strip prefix and suffix.
		inString.remove_prefix(kPasteTextPrefix.size());
		inString.remove_suffix(kPasteTextSuffix.size());

		auto const matchBase64 = [](char character)
		{
			return std::isalnum(character, std::locale::classic()) || (character == '+') || (character == '/') || (character == '=');
		};

		if (!std::ranges::all_of(inString, matchBase64))
		{
			return {};
		}
		auto const binary = VSTGUI::Base64Codec::decode(inString);
		auto const decodedDataPtr = reinterpret_cast<std::byte const*>(binary.data.get());
		return {decodedDataPtr, std::next(decodedDataPtr, binary.dataSize)};
	}
#endif

private:
#if TARGET_OS_WIN32
	static constexpr std::string_view kPasteTextPrefix = "[dfx-settings]";
	static constexpr std::string_view kPasteTextSuffix = "[/dfx-settings]";
	// XXX: For unknown reasons on windows, the clipboard we prepare drops
	// a character from the end of the string when pasting into some apps
	// (notepad.exe) even though the character appears to be there. The
	// unsatisfying workaround is to add whitespace to the end of the
	// string, which we ignore upon paste; if it gets dropped, no harm is
	// done.
	static constexpr std::string_view kPasteTextPadding = "\n\n";
#endif

	static std::vector<std::byte> encode(std::byte const* inSettingsData, VstInt32 inSettingsDataSize)
	{
#if TARGET_OS_WIN32
		auto const base64 = VSTGUI::Base64Codec::encode(inSettingsData, inSettingsDataSize);
		std::vector<std::byte> result;
		result.reserve(kPasteTextPrefix.size() +
					   base64.dataSize +
					   kPasteTextSuffix.size());
		constexpr auto toByte = []<typename T> requires(sizeof(T) == 1) (T value){ return static_cast<std::byte>(value); };
		std::ranges::transform(kPasteTextPrefix, std::back_inserter(result), toByte);
		std::ranges::transform(std::span(base64.data).subspan(0, base64.dataSize), std::back_inserter(result), toByte);
		std::ranges::transform(kPasteTextSuffix, std::back_inserter(result), toByte);
		std::ranges::transform(kPasteTextPadding, std::back_inserter(result), toByte);
		return result;
#else
		return {inSettingsData, std::next(inSettingsData, inSettingsDataSize)};
#endif
	}

	std::vector<std::byte> const mData;
};
#endif  // TARGET_API_VST

//-----------------------------------------------------------------------------
dfx::StatusCode DfxGuiEditor::copySettings()
{
	auto status = initClipboard();
	if (status != dfx::kStatus_NoError)
	{
		return status;
	}

#if TARGET_OS_MAC
	status = PasteboardClear(mClipboardRef.get());
	if (status != noErr)
	{
		return status;  // XXX or just keep going anyway?
	}

	auto const syncFlags = PasteboardSynchronize(mClipboardRef.get());
	if (syncFlags & kPasteboardModified)
	{
		return badPasteboardSyncErr;  // XXX this is a good idea?
	}
	if (!(syncFlags & kPasteboardClientIsOwner))
	{
		return notPasteboardOwnerErr;
	}
	static PasteboardItemID const pasteboardItemID = PasteboardItemID(&pasteboardItemID);
#endif  // TARGET_OS_MAC

#ifdef TARGET_API_AUDIOUNIT
	CFPropertyListRef auSettingsPropertyList = nullptr;
	size_t dataSize = sizeof(auSettingsPropertyList);
	AUSDK_Require_noerr(dfxgui_GetProperty(kAudioUnitProperty_ClassInfo, kAudioUnitScope_Global, 0, 
										   &auSettingsPropertyList, dataSize));
	if (!auSettingsPropertyList)
	{
		return coreFoundationUnknownErr;
	}
	auto const auSettingsPropertyListOwner = dfx::MakeUniqueCFType(auSettingsPropertyList);

	auto const auSettingsCFData = dfx::MakeUniqueCFType(CFPropertyListCreateData(kCFAllocatorDefault, auSettingsPropertyList, kCFPropertyListXMLFormat_v1_0, 0, nullptr));
	if (!auSettingsCFData)
	{
		return coreFoundationUnknownErr;
	}
	status = PasteboardPutItemFlavor(mClipboardRef.get(), pasteboardItemID, kDfxGui_SettingsPasteboardFlavorType, auSettingsCFData.get(), kPasteboardFlavorNoFlags);

#elifdef TARGET_API_VST
	void* vstSettingsData {};
	VstInt32 vstSettingsDataSize {};
	std::vector<float> parameterValues;
	if (getEffect()->getAeffect()->flags & effFlagsProgramChunks)
	{
		vstSettingsDataSize = getEffect()->getChunk(&vstSettingsData, kDfxGui_CopySettingsIsPreset);
		assert(vstSettingsDataSize > 0);
		if (vstSettingsDataSize <= 0)
		{
			return dfx::kStatus_CannotDoInCurrentContext;
		}
	}
	else
	{
		auto const numParameters = getEffect()->getAeffect()->numParams;
		if (numParameters <= 0)  // there is effectively no state to copy
		{
			return dfx::kStatus_CannotDoInCurrentContext;
		}
		for (VstInt32 parameterID = 0; parameterID < numParameters; parameterID++)
		{
			parameterValues.push_back(getEffect()->getParameter(parameterID));
		}
		vstSettingsData = parameterValues.data();
		vstSettingsDataSize = static_cast<VstInt32>(parameterValues.size() * sizeof(parameterValues.front()));
	}

	#if TARGET_OS_MAC
	auto const vstSettingsCFData = dfx::MakeUniqueCFType(CFDataCreate(kCFAllocatorDefault, static_cast<UInt8*>(vstSettingsData), vstSettingsDataSize));
	if (!vstSettingsCFData)
	{
		return coreFoundationUnknownErr;
	}
	status = PasteboardPutItemFlavor(mClipboardRef.get(), pasteboardItemID, kDfxGui_SettingsPasteboardFlavorType, vstSettingsCFData.get(), kPasteboardFlavorNoFlags);
	#else
	auto const vstSettingsDataPackage = VSTGUI::makeOwned<SettingsDataPackage>(static_cast<std::byte const*>(vstSettingsData), vstSettingsDataSize);
	auto const success = VSTGUI::getPlatformFactory().setClipboard(vstSettingsDataPackage);
	status = success ? dfx::kStatus_NoError : dfx::kStatus_CannotDoInCurrentContext;
	#endif  // TARGET_OS_MAC

#else
	#warning "implementation missing"
	assert(false);
#endif  // TARGET_API_AUDIOUNIT/TARGET_API_VST

	return status;
}

//-----------------------------------------------------------------------------
dfx::StatusCode DfxGuiEditor::pasteSettings(bool* inQueryPastabilityOnly)
{
	if (inQueryPastabilityOnly)
	{
		*inQueryPastabilityOnly = false;
	}

	auto status = initClipboard();
	if (status != dfx::kStatus_NoError)
	{
		return status;
	}

#ifdef TARGET_API_VST
	void const* vstSettingsData {};
	VstInt32 vstSettingsDataSize {};
#endif

#if TARGET_OS_MAC
	PasteboardSynchronize(mClipboardRef.get());

	bool pastableItemFound = false;
	ItemCount itemCount {};
	status = PasteboardGetItemCount(mClipboardRef.get(), &itemCount);
	if (status != noErr)
	{
		return status;
	}

	dfx::UniqueCFType<CFDataRef> flavorData;  // needs lifetime to remain in scope for VST implementation later
	for (UInt32 itemIndex = 1; itemIndex <= itemCount; itemIndex++)
	{
		PasteboardItemID itemID = nullptr;
		status = PasteboardGetItemIdentifier(mClipboardRef.get(), itemIndex, &itemID);
		if (status != noErr)
		{
			continue;
		}
		CFArrayRef flavorTypesArray_temp = nullptr;
		status = PasteboardCopyItemFlavors(mClipboardRef.get(), itemID, &flavorTypesArray_temp);
		if ((status != noErr) || !flavorTypesArray_temp)
		{
			continue;
		}
		auto const flavorTypesArray = dfx::MakeUniqueCFType(flavorTypesArray_temp);
		auto const flavorCount = CFArrayGetCount(flavorTypesArray.get());
		for (CFIndex flavorIndex = 0; flavorIndex < flavorCount; flavorIndex++)
		{
			auto const flavorType = static_cast<CFStringRef>(CFArrayGetValueAtIndex(flavorTypesArray.get(), flavorIndex));
			if (!flavorType)
			{
				continue;
			}
			[[maybe_unused]] auto const isAUSettings = UTTypeConformsTo(flavorType, kDfxGui_SettingsPasteboardFlavorType_AU);
			if (UTTypeConformsTo(flavorType, kDfxGui_SettingsPasteboardFlavorType)
	#ifdef TARGET_API_VST
				|| (isAUSettings && (getEffect()->getAeffect()->flags & effFlagsProgramChunks))
	#endif
				)
			{
				if (inQueryPastabilityOnly)
				{
					*inQueryPastabilityOnly = true;
					return dfx::kStatus_NoError;
				}
				CFDataRef flavorData_temp = nullptr;
				status = PasteboardCopyItemFlavorData(mClipboardRef.get(), itemID, flavorType, &flavorData_temp);
				if ((status == noErr) && flavorData_temp)
				{
					flavorData.reset(flavorData_temp);
	#ifdef TARGET_API_AUDIOUNIT
					auto const auSettingsPropertyList = dfx::MakeUniqueCFType(CFPropertyListCreateWithData(kCFAllocatorDefault, flavorData.get(), kCFPropertyListImmutable, nullptr, nullptr));
					if (auSettingsPropertyList)
					{
						auto const auSettingsPropertyList_temp = auSettingsPropertyList.get();
						status = dfxgui_SetProperty(kAudioUnitProperty_ClassInfo, auSettingsPropertyList_temp);
						if (status == dfx::kStatus_NoError)
						{
							pastableItemFound = true;
							AUParameterChange_TellListeners(dfxgui_GetEffectInstance(), kAUParameterListener_AnyParameter);
						}
					}
	#elifdef TARGET_API_VST
					CFDataRef vstSettingsDataCF = nullptr;
					dfx::UniqueCFType<CFPropertyListRef> auSettingsPropertyList;
					if (isAUSettings)
					{
						auSettingsPropertyList.reset(CFPropertyListCreateWithData(kCFAllocatorDefault, flavorData.get(), kCFPropertyListImmutable, nullptr, nullptr));
						if (auSettingsPropertyList)
						{
							vstSettingsDataCF = static_cast<CFDataRef>(CFDictionaryGetValue(reinterpret_cast<CFDictionaryRef>(auSettingsPropertyList.get()), DfxSettings::kDfxDataAUClassInfoKeyString));
						}
					}
					else
					{
						vstSettingsDataCF = flavorData.get();
					}
					if (vstSettingsDataCF)
					{
						vstSettingsData = CFDataGetBytePtr(vstSettingsDataCF);
						vstSettingsDataSize = static_cast<VstInt32>(CFDataGetLength(vstSettingsDataCF));
						pastableItemFound = (vstSettingsData && (vstSettingsDataSize > 0));
					}
	#else
					#warning "implementation missing"
					assert(false);
	#endif	// TARGET_API_AUDIOUNIT/TARGET_API_VST
				}
			}
			if (pastableItemFound)
			{
				break;
			}
		}
		if (pastableItemFound)
		{
			break;
		}
	}
#else
	auto const clipboardData = VSTGUI::getPlatformFactory().getClipboard();
	std::vector<std::byte> decodedDataStorage;
	if (clipboardData)
	{
		constexpr uint32_t dataIndex = 0;  // TODO: should we iterate? can a client add another clipboard atop ours?
		if (clipboardData->getCount() && clipboardData->getDataSize(dataIndex) && (clipboardData->getDataType(dataIndex) == SettingsDataPackage::kDataType))
		{
			VSTGUI::IDataPackage::Type ignoredType {};
			vstSettingsDataSize = static_cast<VstInt32>(clipboardData->getData(dataIndex, vstSettingsData, ignoredType));
			assert(ignoredType == SettingsDataPackage::kDataType);
			decodedDataStorage = SettingsDataPackage::decode(vstSettingsData, vstSettingsDataSize);
			vstSettingsData = decodedDataStorage.data();
			vstSettingsDataSize = static_cast<VstInt32>(decodedDataStorage.size());
		}
	}
#endif  // TARGET_OS_MAC

#ifdef TARGET_API_VST
	if (!vstSettingsData || (vstSettingsDataSize <= 0))
	{
		return dfx::kStatus_CannotDoInCurrentContext;
	}
	if (getEffect()->getAeffect()->flags & effFlagsProgramChunks)
	{
		if (inQueryPastabilityOnly)
		{
			*inQueryPastabilityOnly = dfxgui_GetEffectInstance()->settingsMinimalValidate(vstSettingsData, vstSettingsDataSize);
			return dfx::kStatus_NoError;
		}
		auto const chunkSuccess = getEffect()->setChunk(const_cast<void*>(vstSettingsData), vstSettingsDataSize, kDfxGui_CopySettingsIsPreset);
		assert(chunkSuccess);
		if (chunkSuccess == 0)
		{
			return dfx::kStatus_CannotDoInCurrentContext;
		}
	}
	else
	{
		auto const parameterValues = static_cast<float const*>(vstSettingsData);
		if ((vstSettingsDataSize % sizeof(*parameterValues)) != 0)
		{
			return dfx::kStatus_CannotDoInCurrentContext;
		}
		auto const numParameters = vstSettingsDataSize / static_cast<VstInt32>(sizeof(*parameterValues));
		if (numParameters != getEffect()->getAeffect()->numParams)
		{
			return dfx::kStatus_CannotDoInCurrentContext;
		}
		constexpr auto validateParameterValue = [](float value)
		{
			constexpr float margin = 0.1f;  // allow some legal parameter range forgiveness
			return (value >= (0.f - margin)) && (value <= (1.f + margin));
		};
		if (!std::all_of(parameterValues, std::next(parameterValues, numParameters), validateParameterValue))
		{
			return dfx::kStatus_CannotDoInCurrentContext;
		}
		if (inQueryPastabilityOnly)
		{
			*inQueryPastabilityOnly = true;
			return dfx::kStatus_NoError;
		}
		for (VstInt32 parameterID = 0; parameterID < numParameters; parameterID++)
		{
			getEffect()->setParameter(parameterID, parameterValues[parameterID]);
		}
	}
#endif

#ifdef TARGET_API_RTAS
	#warning "implementation missing"
	assert(false);
#endif

	return status;
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::ShowMessage(std::string const& inMessage)
{
	bool shown = false;
	if (getFrame())
	{
		mErrorDialog = VSTGUI::makeOwned<DGDialog>(DGRect(0, 0, 300, 120), inMessage);
		shown = mErrorDialog->runModal(getFrame());
		assert(shown);
	}
	if (!shown)
	{
		std::fprintf(stderr, "%s\n", inMessage.c_str());
	}
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::ShowAcknowledgements()
{
	constexpr char const* const message = R"DELIM(Portions of this software may utilize the following copyrighted material, the use of which is hereby acknowledged.

-

IMPORTANT:  This Apple software is supplied to you by Apple Inc. ("Apple") in consideration of your agreement to the following terms, and your use, installation, modification or redistribution of this Apple software constitutes acceptance of these terms.  If you do not agree with these terms, please do not use, install, modify or redistribute this Apple software.

In consideration of your agreement to abide by the following terms, and subject to these terms, Apple grants you a personal, non-exclusive license, under Apple's copyrights in this original Apple software (the "Apple Software"), to use, reproduce, modify and redistribute the Apple Software, with or without modifications, in source and/or binary forms; provided that if you redistribute the Apple Software in its entirety and without modifications, you must retain this notice and the following text and disclaimers in all such redistributions of the Apple Software.  Neither the name, trademarks, service marks or logos of Apple Inc. may be used to endorse or promote products derived from the Apple Software without specific prior written permission from Apple.  Except as expressly stated in this notice, no other rights or licenses, express or implied, are granted by Apple herein, including but not limited to any patent rights that may be infringed by your derivative works or by other works in which the Apple Software may be incorporated.

The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.

IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Copyright (C) 2016 Apple Inc. All Rights Reserved.

-

VST PlugIn Technology by Steinberg

-

VSTGUI LICENSE
(c) 2018, Steinberg Media Technologies, All Rights Reserved

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
  * Neither the name of the Steinberg Media Technologies nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
)DELIM";

	try
	{
		mAcknowledgementsDialog = VSTGUI::makeOwned<DGTextScrollDialog>(DGRect(0., 0., GetWidth(), GetHeight()), message);
		auto const success = mAcknowledgementsDialog->runModal(getFrame());
		Require(success, "could not display acknowledgements dialog");
	}
	catch (std::exception const& e)
	{
		ShowMessage(e.what());
	}
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::Require(bool inCondition, std::string const& inFailureMessage)
{
	if (!inCondition)
	{
		throw std::runtime_error(inFailureMessage);
	}
}



#ifdef TARGET_API_AUDIOUNIT

//-----------------------------------------------------------------------------
void DfxGuiEditor::InstallAUEventListeners()
{
	// TODO: should we use kCFRunLoopCommonModes instead, like AUCarbonViewBase does?
	AUEventListenerRef auEventListener_temp = nullptr;
	auto const status = AUEventListenerCreate(AudioUnitEventListenerProc, this, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode, 
											  kNotificationInterval, kNotificationInterval, &auEventListener_temp);
	if ((status != noErr) || !auEventListener_temp)
	{
		assert(false);	// should never happen
		return;
	}

	mAUEventListener.reset(auEventListener_temp);

	{
		std::lock_guard const guard(mParameterListLock);
		for (auto const& parameterID : mParameterList)
		{
			auto const auParam = dfxgui_MakeAudioUnitParameter(parameterID);
			AUListenerAddParameter(mAUEventListener.get(), this, &auParam);
		}
	}

	mStreamFormatPropertyAUEvent = {};
	mStreamFormatPropertyAUEvent.mEventType = kAudioUnitEvent_PropertyChange;
	mStreamFormatPropertyAUEvent.mArgument.mProperty.mAudioUnit = dfxgui_GetEffectInstance();
	mStreamFormatPropertyAUEvent.mArgument.mProperty.mPropertyID = kAudioUnitProperty_StreamFormat;
	mStreamFormatPropertyAUEvent.mArgument.mProperty.mScope = kAudioUnitScope_Output;
	mStreamFormatPropertyAUEvent.mArgument.mProperty.mElement = 0;
	AUEventListenerAddEventType(mAUEventListener.get(), this, &mStreamFormatPropertyAUEvent);

	mParameterListPropertyAUEvent = mStreamFormatPropertyAUEvent;
	mParameterListPropertyAUEvent.mArgument.mProperty.mPropertyID = kAudioUnitProperty_ParameterList;
	mParameterListPropertyAUEvent.mArgument.mProperty.mScope = kAudioUnitScope_Global;
	AUEventListenerAddEventType(mAUEventListener.get(), this, &mParameterListPropertyAUEvent);

#if TARGET_PLUGIN_USES_MIDI
	mMidiLearnPropertyAUEvent = mStreamFormatPropertyAUEvent;
	mMidiLearnPropertyAUEvent.mArgument.mProperty.mPropertyID = dfx::kPluginProperty_MidiLearn;
	mMidiLearnPropertyAUEvent.mArgument.mProperty.mScope = kAudioUnitScope_Global;
	AUEventListenerAddEventType(mAUEventListener.get(), this, &mMidiLearnPropertyAUEvent);

	mMidiLearnerPropertyAUEvent = mStreamFormatPropertyAUEvent;
	mMidiLearnerPropertyAUEvent.mArgument.mProperty.mPropertyID = dfx::kPluginProperty_MidiLearner;
	mMidiLearnerPropertyAUEvent.mArgument.mProperty.mScope = kAudioUnitScope_Global;
	AUEventListenerAddEventType(mAUEventListener.get(), this, &mMidiLearnerPropertyAUEvent);
#endif

	ForEachRegisteredAudioUnitEvent([this](AudioUnitEvent const& propertyAUEvent)
	{
		AUEventListenerAddEventType(mAUEventListener.get(), this, &propertyAUEvent);
	});
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::RemoveAUEventListeners()
{
	if (!mAUEventListener)
	{
		return;
	}

	{
		std::lock_guard const guard(mParameterListLock);
		for (auto const& parameterID : mParameterList)
		{
			auto const auParam = dfxgui_MakeAudioUnitParameter(parameterID);
			AUListenerRemoveParameter(mAUEventListener.get(), this, &auParam);
		}
	}

	AUEventListenerRemoveEventType(mAUEventListener.get(), this, &mStreamFormatPropertyAUEvent);
	AUEventListenerRemoveEventType(mAUEventListener.get(), this, &mParameterListPropertyAUEvent);
#if TARGET_PLUGIN_USES_MIDI
	AUEventListenerRemoveEventType(mAUEventListener.get(), this, &mMidiLearnPropertyAUEvent);
	AUEventListenerRemoveEventType(mAUEventListener.get(), this, &mMidiLearnerPropertyAUEvent);
#endif


	ForEachRegisteredAudioUnitEvent([this](AudioUnitEvent const& propertyAUEvent)
	{
		AUEventListenerRemoveEventType(mAUEventListener.get(), this, &propertyAUEvent);
	});
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::AudioUnitEventListenerProc(void* inCallbackRefCon, void* inObject, AudioUnitEvent const* inEvent, 
											  UInt64 /*inEventHostTime*/, Float32 inParameterValue)
{
	assert(inCallbackRefCon);
	assert(inEvent);
	// TODO: why in macOS 10.15.5 is inObject equal to inCallbackRefCon and not the sending CControl?
	auto const ourOwnerEditor = static_cast<DfxGuiEditor*>(inCallbackRefCon);
	if (ourOwnerEditor && inEvent)
	{
		if (inEvent->mEventType == kAudioUnitEvent_ParameterValueChange)
		{
			auto const parameterID = inEvent->mArgument.mParameter.mParameterID;
			auto const parameterValue_norm = ourOwnerEditor->dfxgui_ContractParameterValue(parameterID, inParameterValue);
			ourOwnerEditor->updateParameterControls(parameterID, parameterValue_norm, static_cast<VSTGUI::CControl*>(inObject));
		}

		if (inEvent->mEventType == kAudioUnitEvent_PropertyChange)
		{
			switch (inEvent->mArgument.mProperty.mPropertyID)
			{
				case kAudioUnitProperty_StreamFormat:
					ourOwnerEditor->HandleStreamFormatChange();
					break;
				case kAudioUnitProperty_ParameterList:
					ourOwnerEditor->HandleParameterListChange();
					break;
			#if TARGET_PLUGIN_USES_MIDI
				case dfx::kPluginProperty_MidiLearn:
					ourOwnerEditor->HandleMidiLearnChange();
					break;
				case dfx::kPluginProperty_MidiLearner:
					ourOwnerEditor->HandleMidiLearnerChange();
					break;
			#endif
				default:
					ourOwnerEditor->HandlePropertyChange(inEvent->mArgument.mProperty.mPropertyID, inEvent->mArgument.mProperty.mScope, inEvent->mArgument.mProperty.mElement);
					break;
			}
		}
	}
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::ForEachRegisteredAudioUnitEvent(std::function<void(AudioUnitEvent const&)>&& f)
{
	for (auto const property : mRegisteredProperties)
	{
		AudioUnitEvent auEvent {};
		auEvent.mEventType = kAudioUnitEvent_PropertyChange;
		auEvent.mArgument.mProperty.mAudioUnit = dfxgui_GetEffectInstance();
		auEvent.mArgument.mProperty.mPropertyID = property.mID;
		auEvent.mArgument.mProperty.mScope = property.mScope;
		auEvent.mArgument.mProperty.mElement = property.mItemIndex;

		f(auEvent);
	}
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::HandleStreamFormatChange()
{
	if (auto const count = getNumInputChannels(); std::exchange(mNumInputChannels, count) != count)
	{
		inputChannelsChanged(count);
	}
	if (auto const count = getNumOutputChannels(); std::exchange(mNumOutputChannels, count) != count)
	{
		outputChannelsChanged(count);
	}
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::HandleParameterListChange()
{
	std::lock_guard const guard(mParameterListLock);

	if (mAUEventListener)
	{
		for (auto const& parameterID : mParameterList)
		{
			auto const auParam = dfxgui_MakeAudioUnitParameter(parameterID);
			AUListenerRemoveParameter(mAUEventListener.get(), this, &auParam);
		}
	}

	mParameterList = CreateParameterList();

	if (mAUEventListener)
	{
		for (auto const& parameterID : mParameterList)
		{
			auto const auParam = dfxgui_MakeAudioUnitParameter(parameterID);
			AUListenerAddParameter(mAUEventListener.get(), this, &auParam);
		}
	}
}

#endif  // TARGET_API_AUDIOUNIT


#if TARGET_PLUGIN_USES_MIDI

//-----------------------------------------------------------------------------
void DfxGuiEditor::HandleMidiLearnChange()
{
	auto const midiLearning = getmidilearning();

	if (mMidiLearnButton)
	{
		long const controlValue = midiLearning ? 1 : 0;
		mMidiLearnButton->setValue_i(controlValue);
		mMidiLearnButton->notifyIfChanged();
	}

	midiLearningChanged(midiLearning);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::HandleMidiLearnerChange()
{
	auto const midiLearner = getmidilearner();

	for (auto& control : mControlsList)
	{
		control->setMidiLearner(control->getParameterID() == midiLearner);
	}

	midiLearnerChanged(midiLearner);
}

//-----------------------------------------------------------------------------
DGButton* DfxGuiEditor::CreateMidiLearnButton(VSTGUI::CCoord inXpos, VSTGUI::CCoord inYpos, DGImage* inImage, bool inDrawMomentaryState)
{
	mMidiLearnButton = emplaceControl<DGToggleImageButton>(this, inXpos, inYpos, inImage, inDrawMomentaryState);
	mMidiLearnButton->setUserProcedure(std::bind_front(&DfxGuiEditor::setmidilearning, this));
	return mMidiLearnButton;
}

//-----------------------------------------------------------------------------
DGButton* DfxGuiEditor::CreateMidiResetButton(VSTGUI::CCoord inXpos, VSTGUI::CCoord inYpos, DGImage* inImage)
{
	DGRect const pos(inXpos, inYpos, inImage->getWidth(), inImage->getHeight() / 2);
	mMidiResetButton = emplaceControl<DGButton>(this, pos, inImage, 2, DGButton::Mode::Momentary);
	mMidiResetButton->setUserProcedure([this](long inValue)
	{
		if (inValue != 0)
		{
			resetmidilearn();
		}
	});
	return mMidiResetButton;
}

#endif
// TARGET_PLUGIN_USES_MIDI






#pragma mark -

#ifdef TARGET_API_VST

//-----------------------------------------------------------------------------
template <dfx::TriviallySerializable T>
[[nodiscard]] T DFXGUI_CorrectEndian(T inValue)
{
	if constexpr (!DfxSettings::serializationIsNativeEndian())  // VST program and bank files are also big-endian
	{
		dfx::ReverseBytes(inValue);
	}
	return inValue;
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::RestoreVSTStateFromProgramFile(char const* inFilePath)
{
	auto const fileStream = VSTGUI::FileResourceInputStream::create(inFilePath);
	Require(fileStream.get(), "could not open file");
	fileStream->seek(0, VSTGUI::SeekMode::End);
	auto const fileSize = dfx::math::ToIndex<uint32_t>(fileStream->tell());
	fileStream->seek(0, VSTGUI::SeekMode::Set);
	Require(fileStream->tell() == 0, "could not rewind file");

	auto const readWithValidation = [&fileStream](void* buffer, uint32_t size)
	{
		auto const amountRead = fileStream->readRaw(buffer, size);
		Require(amountRead == size, "failed to read enough file data");
	};
	auto const requireReadCompletion = [&fileStream]
	{
		Require(fileStream->tell() == fileStream->seek(0, VSTGUI::SeekMode::End), 
				"file contains unexpected excess data");
	};

	fxProgram programData {};
	constexpr uint32_t headerSize = sizeof(programData) - sizeof(programData.content);
	Require(fileSize > headerSize, "invalid file size");
	readWithValidation(&programData, headerSize);
	Require(DFXGUI_CorrectEndian(programData.chunkMagic) == kDfxGui_VSTSettingsChunkMagic, 
			"invalid file data");
	Require(DFXGUI_CorrectEndian(programData.version) == kDfxGui_VSTSettingsVersion, "invalid data format version");
	Require(DFXGUI_CorrectEndian(programData.fxID) == PLUGIN_ID, "file data is for a different plugin");
	auto const expectedByteSizeValue = static_cast<VstInt32>(fileSize - sizeof(programData.chunkMagic) - sizeof(programData.byteSize));
	Require(DFXGUI_CorrectEndian(programData.byteSize) == expectedByteSizeValue, "byte size value is incorrect");
	if (DFXGUI_CorrectEndian(programData.fxMagic) == kDfxGui_VSTSettingsRegularMagic)
	{
		auto const numParameters = DFXGUI_CorrectEndian(programData.numParams);
		Require(numParameters >= 0, "invalid negative number of parameters");
		using ValueT = std::decay_t<decltype(*(programData.content.params))>;
		std::vector<ValueT> parameterValues(static_cast<size_t>(numParameters), {});
		readWithValidation(parameterValues.data(), static_cast<uint32_t>(parameterValues.size() * sizeof(ValueT)));
		requireReadCompletion();
		for (VstInt32 parameterID = 0; parameterID < numParameters; parameterID++)
		{
			getEffect()->setParameter(parameterID, DFXGUI_CorrectEndian(parameterValues[parameterID]));
		}
	}
	else if (DFXGUI_CorrectEndian(programData.fxMagic) == kDfxGui_VSTSettingsOpaqueChunkMagic)
	{
		auto& chunkDataSize = programData.content.data.size;
		readWithValidation(&chunkDataSize, sizeof(chunkDataSize));
		Require(DFXGUI_CorrectEndian(chunkDataSize) >= 0, "invalid negative number of chunk bytes");
		std::vector<std::byte> chunkData(static_cast<size_t>(DFXGUI_CorrectEndian(chunkDataSize)), {});
		readWithValidation(chunkData.data(), static_cast<uint32_t>(chunkData.size()));
		requireReadCompletion();
		Require(getEffect()->setChunk(chunkData.data(), static_cast<VstInt32>(chunkData.size()), true), 
				"failed to load chunk data");
	}
	else
	{
		throw std::runtime_error("unrecognized data format");
	}
	getEffect()->setProgramName(programData.prgName);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::SaveVSTStateToProgramFile(char const* inFilePath)
{
	std::unique_ptr<FILE, decltype(&fclose)> file(fopen(inFilePath, "wb"), fclose);
	Require(file.get(), "could not open file to write");

	auto const serializedInt = [](auto value)
	{
		using ResultT = VstInt32;
		static_assert(sizeof(value) == sizeof(ResultT));
		return static_cast<ResultT>(DFXGUI_CorrectEndian(value));
	};

	fxProgram programData {};
	programData.chunkMagic = serializedInt(kDfxGui_VSTSettingsChunkMagic);
	programData.version = serializedInt(kDfxGui_VSTSettingsVersion);
	programData.fxID = serializedInt(PLUGIN_ID);
	programData.fxVersion = serializedInt(dfxgui_GetEffectInstance()->getVendorVersion());
	auto const numParameters = getEffect()->getAeffect()->numParams;
	programData.numParams = serializedInt(numParameters);
	getEffect()->getProgramName(programData.prgName);

	auto const writeWithValidation = [&file](void const* data, uint32_t size)
	{
		auto const amountWritten = fwrite(data, 1, size, file.get());
		Require(amountWritten == size, "failed to write enough file data");
	};

	constexpr uint32_t headerSize = sizeof(programData) - sizeof(programData.content);
	constexpr uint32_t byteSizeBase = headerSize - sizeof(programData.chunkMagic) - sizeof(programData.byteSize);

	if (getEffect()->getAeffect()->flags & effFlagsProgramChunks)
	{
		void* chunkData {};
		auto const chunkDataSize = getEffect()->getChunk(&chunkData, true);
		Require(chunkData && (chunkDataSize > 0), "failed to query plugin chunk data");

		programData.fxMagic = serializedInt(kDfxGui_VSTSettingsOpaqueChunkMagic);
		programData.content.data.size = serializedInt(chunkDataSize);

		constexpr uint32_t chunkHeaderSize = sizeof(programData.content.data.size);
		programData.byteSize = serializedInt(byteSizeBase + chunkHeaderSize + chunkDataSize);
		writeWithValidation(&programData, headerSize + chunkHeaderSize);
		writeWithValidation(chunkData, chunkDataSize);
	}
	else
	{
		programData.fxMagic = serializedInt(kDfxGui_VSTSettingsRegularMagic);
		using ValueT = std::decay_t<decltype(*(programData.content.params))>;
		std::vector<ValueT> parameterValues(static_cast<size_t>(numParameters), {});
		for (VstInt32 parameterID = 0; parameterID < numParameters; parameterID++)
		{
			parameterValues[parameterID] = DFXGUI_CorrectEndian(getEffect()->getParameter(parameterID));
		}

		auto const parameterValuesDataSize = static_cast<uint32_t>(parameterValues.size() * sizeof(decltype(parameterValues)::value_type));
		programData.byteSize = serializedInt(byteSizeBase + parameterValuesDataSize);
		writeWithValidation(&programData, headerSize);
		writeWithValidation(parameterValues.data(), parameterValuesDataSize);
	}
}

#endif  // TARGET_API_VST






#pragma mark -

#ifdef TARGET_API_RTAS

//-----------------------------------------------------------------------------
// fills inRect with dimensions of background frame; NO_UI: called (indirectly) by Pro Tools
void DfxGuiEditor::GetBackgroundRect(sRect* inRect)
{
	inRect->top = rect.top;
	inRect->left = rect.left;
	inRect->bottom = rect.bottom;
	inRect->right = rect.right;
}

//-----------------------------------------------------------------------------
// sets dimensions of background frame; NO_UI: called by the plug-in process class when resizing the window
void DfxGuiEditor::SetBackgroundRect(sRect* inRect)
{
	rect.top = inRect->top;
	rect.left = inRect->left;
	rect.bottom = inRect->bottom;
	rect.right = inRect->right;
}

//-----------------------------------------------------------------------------
// Called by process to get the index of the control that contains the point
void DfxGuiEditor::GetControlIndexFromPoint(long inXpos, long inYpos, long* outControlIndex)
{
	if (!outControlIndex)
	{
		return;
	}

	*outControlIndex = 0;
	VSTGUI::CPoint const point(inXpos, inYpos);
	for (auto const& control : mControlsList)
	{
		auto const& controlBounds = control->asCControl()->getViewSize();
		if (point.isInside(controlBounds) && control->asCControl()->isVisible())
		{
			*outControlIndex = dfx::ParameterID_ToRTAS(control->getParameterID());
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Called by process when a control's highlight has changed
void DfxGuiEditor::SetControlHighlight(long inControlIndex, short inIsHighlighted, short inColor)
{
	auto const parameterID = dfx::ParameterID_FromRTAS(inControlIndex);
	if (!inIsHighlighted)
	{
		inColor = eHighlight_None;
	}

	if (dfxgui_IsValidParameterID(parameterID))
	{
		mParameterHighlightColors.at(parameterID) = inColor;
	}

	if (IsOpen())
	{
		getFrame()->setDirty(true);
	}
}

//-----------------------------------------------------------------------------
// Called by process when a control's highlight has changed
void DfxGuiEditor::drawControlHighlight(VSTGUI::CDrawContext* inContext, VSTGUI::CControl* inControl)
{
	assert(inContext);
	assert(inControl);

	auto const parameterID = dfx::ParameterID_FromVST(inControl->getTag());
	if (!dfxgui_IsValidParameterID(parameterID))
	{
		return;
	}

	VSTGUI::CColor highlightColor;
	switch (mParameterHighlightColors.at(parameterID))
	{
		case eHighlight_Red:
			highlightColor = VSTGUI::MakeCColor(255, 0, 0);
			break;
		case eHighlight_Blue:
			highlightColor = VSTGUI::MakeCColor(0, 0, 170);
			break;
		case eHighlight_Green:
			highlightColor = VSTGUI::MakeCColor(0, 221, 0);
			break;
		case eHighlight_Yellow:
			highlightColor = VSTGUI::MakeCColor(255, 204, 0);
			break;
		default:
			assert(false);
			return;
	}

	constexpr VSTGUI::CCoord highlightWidth = 1;
	VSTGUI::CRect highlightRect = inControl->getViewSize();
	inContext->setFillColor(highlightColor);
	inContext->setFrameColor(highlightColor);
	inContext->setLineWidth(highlightWidth);
	inContext->drawRect(highlightRect);

	highlightColor.alpha = 90;
	inContext->setFillColor(highlightColor);
	inContext->setFrameColor(highlightColor);
	highlightRect.inset(highlightWidth, highlightWidth);
	inContext->drawRect(highlightRect);
}

#endif
// TARGET_API_RTAS
