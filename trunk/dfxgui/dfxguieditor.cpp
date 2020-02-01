/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2020  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Destroy FX Library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Destroy FX Library.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "dfxguieditor.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <functional>
#include <map>

#include "dfxguibutton.h"
#include "dfxguidialog.h"
#include "dfxmisc.h"
#include "idfxguicontrol.h"

#if TARGET_PLUGIN_USES_MIDI
	#include "dfxsettings.h"
#endif

#ifdef TARGET_API_AUDIOUNIT
	#include "dfx-au-utilities.h"
	#if !__LP64__
		#include "AUCarbonViewBase.h"
		#include "lib/platform/iplatformframe.h"
	#endif
#endif

#ifdef TARGET_API_VST
	#include "dfxplugin.h"
#endif

#ifdef TARGET_API_RTAS
	#include "ConvertUtils.h"
#endif



#ifdef TARGET_API_AUDIOUNIT
static void DFXGUI_AudioUnitEventListenerProc(void* inCallbackRefCon, void* inObject, AudioUnitEvent const* inEvent, UInt64 inEventHostTime, Float32 inParameterValue);
#endif

#ifdef TARGET_API_AUDIOUNIT
	#define kDfxGui_AUPresetFileUTI "com.apple.audio-unit-preset"  // XXX implemented in Mac OS X 10.4.11 or maybe a little earlier, but no public constant published yet
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wunknown-attributes"
	__attribute__((no_destroy)) static auto const kDfxGui_AUPresetFileExtension = new VSTGUI::CFileExtension("Audio Unit preset", "aupreset", "", 0, kDfxGui_AUPresetFileUTI);
	#pragma clang diagnostic pop
#endif


//-----------------------------------------------------------------------------
DfxGuiEditor::DfxGuiEditor(DGEditorListenerInstance inInstance)
:	TARGET_API_EDITOR_BASE_CLASS(inInstance)
{
#ifdef TARGET_API_RTAS
	m_Process = inInstance;

	mParameterHighlightColors.assign(GetNumParameters(), eHighlight_None);
#endif

	rect.top = rect.left = rect.bottom = rect.right = 0;
	// load the background image
	// we don't need to load all bitmaps, this could be done when open is called
	// XXX hack
	mBackgroundImage = VSTGUI::makeOwned<DGImage>(PLUGIN_BACKGROUND_IMAGE_FILENAME);
	if (mBackgroundImage)
	{
		// init the size of the plugin
		rect.right = rect.left + std::lround(mBackgroundImage->getWidth());
		rect.bottom = rect.top + std::lround(mBackgroundImage->getHeight());
	}

	VSTGUI::CView::kDirtyCallAlwaysOnMainThread = true;
	setKnobMode(VSTGUI::kLinearMode);

#ifdef TARGET_API_RTAS
	// XXX do these?
//	VSTGUI::CControl::kZoomModifier = kControl;
//	VSTGUI::CControl::kDefaultValueModifier = kShift;
#endif
}

//-----------------------------------------------------------------------------
DfxGuiEditor::~DfxGuiEditor()
{
	if (IsOpen())
	{
		close();
	}

#if TARGET_PLUGIN_USES_MIDI
	setmidilearning(false);
#endif

#ifdef TARGET_API_AUDIOUNIT
	// remove and dispose the event listener, if we created it
	if (mAUEventListener)
	{
		for (auto const& parameterID : mAUParameterList)
		{
			auto const auParam = dfxgui_MakeAudioUnitParameter(parameterID);
			AUListenerRemoveParameter(mAUEventListener.get(), this, &auParam);
		}
		AUEventListenerRemoveEventType(mAUEventListener.get(), this, &mStreamFormatPropertyAUEvent);
		AUEventListenerRemoveEventType(mAUEventListener.get(), this, &mParameterListPropertyAUEvent);
	#if TARGET_PLUGIN_USES_MIDI
		AUEventListenerRemoveEventType(mAUEventListener.get(), this, &mMidiLearnPropertyAUEvent);
		AUEventListenerRemoveEventType(mAUEventListener.get(), this, &mMidiLearnerPropertyAUEvent);
	#endif
		std::for_each(mCustomPropertyAUEvents.cbegin(), mCustomPropertyAUEvents.cend(), [this](auto const& propertyAUEvent)
					  {
						  AUEventListenerRemoveEventType(mAUEventListener.get(), this, &propertyAUEvent);
					  });
	}
#endif
}


//-----------------------------------------------------------------------------
bool DfxGuiEditor::open(void* inWindow)
{
	// !!! always call this !!!
	auto const baseSuccess = TARGET_API_EDITOR_BASE_CLASS::open(inWindow);
	if (!baseSuccess)
	{
		return baseSuccess;
	}

	mControlsList.clear();

	frame = new VSTGUI::CFrame(VSTGUI::CRect(rect.left, rect.top, rect.right, rect.bottom), this);
	if (!frame)
	{
		return false;
	}
	frame->open(inWindow);
	frame->setBackground(GetBackgroundImage());
	frame->enableTooltips(true);


	mCurrentControl_clicked = nullptr;  // make sure that it isn't set to anything
	mMousedOverControlsList.clear();
	setCurrentControl_mouseover(nullptr);


// determine the number of audio channels currently configured for the AU
	mNumAudioChannels = getNumAudioChannels();


#ifdef TARGET_API_AUDIOUNIT
// install an event listener for the parameters and necessary properties
	{
		std::lock_guard const guard(mAUParameterListLock);
		mAUParameterList = CreateParameterList();
	}
	// XXX should I use kCFRunLoopCommonModes instead, like AUCarbonViewBase does?
	AUEventListenerRef auEventListener_temp = nullptr; 
	auto const status = AUEventListenerCreate(DFXGUI_AudioUnitEventListenerProc, this, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode,
											  kNotificationInterval, kNotificationInterval, &auEventListener_temp);
	if ((status == noErr) && auEventListener_temp)
	{
		mAUEventListener.reset(auEventListener_temp);

		{
			std::lock_guard const guard(mAUParameterListLock);
			for (auto const& parameterID : mAUParameterList)
			{
				auto const auParam = dfxgui_MakeAudioUnitParameter(parameterID);
				AUListenerAddParameter(mAUEventListener.get(), this, &auParam);
			}
		}

		memset(&mStreamFormatPropertyAUEvent, 0, sizeof(mStreamFormatPropertyAUEvent));
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

		std::for_each(mCustomPropertyAUEvents.cbegin(), mCustomPropertyAUEvents.cend(), [this](auto const& propertyAUEvent)
					  {
						  AUEventListenerAddEventType(mAUEventListener.get(), this, &propertyAUEvent);
					  });
	}
#endif


#if TARGET_OS_MAC
// load any fonts from our bundle resources to be accessible locally within our component instance
	auto const pluginBundle = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));
	if (pluginBundle)
	{
		dfx::UniqueCFType const bundleResourcesDirURL = CFBundleCopyResourcesDirectoryURL(pluginBundle);
		if (bundleResourcesDirURL)
		{
			constexpr CFURLEnumeratorOptions options = kCFURLEnumeratorSkipInvisibles | kCFURLEnumeratorSkipPackageContents | kCFURLEnumeratorDescendRecursively;
			dfx::UniqueCFType const dirEnumerator = CFURLEnumeratorCreateForDirectoryURL(kCFAllocatorDefault, bundleResourcesDirURL.get(), options, nullptr);
			if (dirEnumerator)
			{
				CFURLRef fileURL = nullptr;
				while (CFURLEnumeratorGetNextURL(dirEnumerator.get(), &fileURL, nullptr) == kCFURLEnumeratorSuccess)
				{
					if (fileURL)
					{
						if (CFURLHasDirectoryPath(fileURL))
						{
							continue;
						}
						// XXX TODO: optimize to skip non-font files?
						[[maybe_unused]] auto const success = CTFontManagerRegisterFontsForURL(fileURL, kCTFontManagerScopeProcess, nullptr);
					}
				}
			}
		}
	}
#endif


	// HACK: must do this after creating the tooltip support because 
	// it will steal the mouse observer role (we can still forward to it)
	// (though that no longer seems to apply with VSTGUI 4)
	frame->registerMouseObserver(this);


	mEditorOpenErr = OpenEditor();
	if (mEditorOpenErr != dfx::kStatus_NoError)
	{
		return false;
	}

	mJustOpened = true;

	// allow for anything that might need to happen after the above post-opening stuff is finished
	post_open();

#if TARGET_PLUGIN_USES_MIDI
	HandleMidiLearnChange();
	HandleMidiLearnerChange();
#endif

	return true;
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::close()
{
	mJustOpened = false;

	CloseEditor();

	frame->unregisterMouseObserver(this);
	// zero the member frame before we delete it so that other asynchronous calls don't crash
	auto const frame_temp = std::exchange(frame, nullptr);

	for (auto& control : mControlsList)
	{
		control->asCControl()->unregisterViewMouseListener(this);
	}
	mControlsList.clear();

	if (frame_temp)
	{
		frame_temp->forget();
	}

	TARGET_API_EDITOR_BASE_CLASS::close();
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::IsOpen() const noexcept
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

	for (auto& control : mControlsList)
	{
		if (control->getParameterID() == inParameterIndex)
		{
			control->setValue_gen(inValue);
			control->asCControl()->setDirty();  // XXX this seems to be necessary for 64-bit AU to update from outside parameter value changes?
		}
	}

	parameterChanged(inParameterIndex);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::valueChanged(VSTGUI::CControl* inControl)
{
	auto const paramIndex = inControl->getTag();
	auto const paramValue_norm = inControl->getValue();

	if (dfxgui_IsValidParamID(paramIndex))
	{
#ifdef TARGET_API_AUDIOUNIT
		auto const paramValue_literal = dfxgui_ExpandParameterValue(paramIndex, paramValue_norm);
		// XXX or should I call setparameter_f()?
		auto const auParam = dfxgui_MakeAudioUnitParameter(paramIndex);
		AUParameterSet(nullptr, inControl, &auParam, paramValue_literal, 0);
#endif
#ifdef TARGET_API_VST
		getEffect()->setParameterAutomated(paramIndex, paramValue_norm);
#endif
#ifdef TARGET_API_RTAS
		if (m_Process)
		{
			// XXX though the model of calling SetControlValue might make more seem like 
			// better design than calling setparameter_gen on the effect, in practice, 
			// our DfxParam objects won't get their values updated to reflect the change until 
			// the next call to UpdateControlInAlgorithm which be deferred until the start of 
			// the next audio render call, which means that in the meantime getparameter_* 
			// methods will return the previous rather than current value
//			m_Process->SetControlValue(dfx::ParameterID_ToRTAS(paramIndex), ConvertToDigiValue(paramValue_norm));
			m_Process->setparameter_gen(paramIndex, paramValue_norm);
		}
#endif
	}
}

//-----------------------------------------------------------------------------
int32_t DfxGuiEditor::controlModifierClicked(VSTGUI::CControl* inControl, VSTGUI::CButtonState inButtons)
{
	static constexpr int32_t kNotHandled = 0;
	static constexpr int32_t kHandled = 1;

	auto const handled = handleContextualMenuClick(inControl, inButtons);
	return handled ? kHandled : kNotHandled;
}

#ifndef TARGET_API_VST
//-----------------------------------------------------------------------------
void DfxGuiEditor::beginEdit(int32_t inParameterIndex)
{
	if (dfxgui_IsValidParamID(inParameterIndex))
	{
		automationgesture_begin(inParameterIndex);
	}
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::endEdit(int32_t inParameterIndex)
{
	if (dfxgui_IsValidParamID(inParameterIndex))
	{
		automationgesture_end(inParameterIndex);
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
#if WINDOWS_VERSION // && defined(TARGET_API_RTAS)
		// XXX this seems to be necessary to correct background re-drawing failure 
		// when switching between different plugins in an open plugin editor window
		getFrame()->invalid();
#endif
	}

	// call any child class implementation
	dfxgui_Idle();
}


//-----------------------------------------------------------------------------
void DfxGuiEditor::RegisterPropertyChange(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned long inItemIndex)
{
	assert(!IsOpen());  // you need to register these all before opening a view

#ifdef TARGET_API_AUDIOUNIT
	AudioUnitEvent auEvent{};
	auEvent.mEventType = kAudioUnitEvent_PropertyChange;
	auEvent.mArgument.mProperty.mAudioUnit = dfxgui_GetEffectInstance();
	auEvent.mArgument.mProperty.mPropertyID = inPropertyID;
	auEvent.mArgument.mProperty.mScope = inScope;
	auEvent.mArgument.mProperty.mElement = inItemIndex;
	mCustomPropertyAUEvents.push_back(auEvent);
#else
	assert(false);  // TODO: implement
#endif
}


//-----------------------------------------------------------------------------
void DfxGuiEditor::addControl(IDGControl* inControl)
{
	assert(inControl);

	auto const parameterIndex = inControl->getParameterID();
	// XXX only add it to our controls list if it is attached to a parameter (?)
	if (dfxgui_IsValidParamID(parameterIndex))
	{
		assert(std::find(mControlsList.cbegin(), mControlsList.cend(), inControl) == mControlsList.cend());
		mControlsList.push_back(inControl);
		inControl->asCControl()->registerViewMouseListener(this);

#ifdef TARGET_API_RTAS
		if (m_Process)
		{
			auto const parameterIndex_rtas = dfx::ParameterID_ToRTAS(parameterIndex);
			long value_rtas {};
			m_Process->GetControlValue(parameterIndex_rtas, &value_rtas);
			inControl->setValue_gen(ConvertToVSTValue(value_rtas));
			long defaultValue_rtas {};
			m_Process->GetControlDefaultValue(parameterIndex_rtas, &defaultValue_rtas);  // VSTGUI: necessary for Alt+Click behavior
			inControl->setDefaultValue_gen(ConvertToVSTValue(defaultValue_rtas));  // VSTGUI: necessary for Alt+Click behavior
		}
		else
#endif
		{
			inControl->setValue_gen(getparameter_gen(parameterIndex));
			auto defaultValue = GetParameter_defaultValue(parameterIndex);
			defaultValue = dfxgui_ContractParameterValue(parameterIndex, defaultValue);
			inControl->setDefaultValue_gen(defaultValue);
		}
	}
	else
	{
		inControl->setValue_gen(0.0f);
	}

	inControl->asCControl()->setOldValue(inControl->asCControl()->getValue());

	[[maybe_unused]] auto const success = getFrame()->addView(inControl->asCControl());
	assert(success);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::removeControl(IDGControl* inControl)
{
	assert(false);  // TODO: test or remove this method? (it currently is not used anywhere)

	auto const foundControl = std::find(mControlsList.cbegin(), mControlsList.cend(), inControl);
	assert(foundControl != mControlsList.cend());
	if (foundControl != mControlsList.cend())
	{
		mControlsList.erase(foundControl);
	}

	[[maybe_unused]] auto const success = getFrame()->removeView(inControl->asCControl());
	assert(success);
}

//-----------------------------------------------------------------------------
long DfxGuiEditor::GetWidth()
{
	VSTGUI::ERect* editorRect = nullptr;
	if (getRect(&editorRect) && editorRect)
	{
		return editorRect->right - editorRect->left;
	}
	return 0;
}

//-----------------------------------------------------------------------------
long DfxGuiEditor::GetHeight()
{
	VSTGUI::ERect* editorRect = nullptr;
	if (getRect(&editorRect) && editorRect)
	{
		return editorRect->bottom - editorRect->top;
	}
	return 0;
}


#ifdef TARGET_API_AUDIOUNIT
//-----------------------------------------------------------------------------
OSStatus DfxGuiEditor::SendAUParameterEvent(AudioUnitParameterID inParameterID, AudioUnitEventType inEventType)
{
	// we're not actually prepared to do anything at this point if we don't yet know which AU we are controlling
	if (!dfxgui_GetEffectInstance())
	{
		return kAudioUnitErr_Uninitialized;
	}

	AudioUnitEvent paramEvent = {};
	paramEvent.mEventType = inEventType;
	paramEvent.mArgument.mParameter = dfxgui_MakeAudioUnitParameter(inParameterID);
	return AUEventListenerNotify(getAUEventListener(), nullptr, &paramEvent);
}
#endif

//-----------------------------------------------------------------------------
void DfxGuiEditor::automationgesture_begin(long inParameterID)
{
#ifdef TARGET_API_AUDIOUNIT
	SendAUParameterEvent(static_cast<AudioUnitParameterID>(inParameterID), kAudioUnitEvent_BeginParameterChangeGesture);
#endif

#ifdef TARGET_API_VST
	TARGET_API_EDITOR_BASE_CLASS::beginEdit(inParameterID);
#endif

#ifdef TARGET_API_RTAS
	// called by GUI when mouse down event has occured; NO_UI: Call process' TouchControl()
	// This and endEdit are necessary for Touch Automation to work properly
	if (m_Process)
	{
		m_Process->ProcessTouchControl(dfx::ParameterID_ToRTAS(inParameterID));
	}
#endif
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::automationgesture_end(long inParameterID)
{
#ifdef TARGET_API_AUDIOUNIT
	SendAUParameterEvent(static_cast<AudioUnitParameterID>(inParameterID), kAudioUnitEvent_EndParameterChangeGesture);
#endif

#ifdef TARGET_API_VST
	TARGET_API_EDITOR_BASE_CLASS::endEdit(inParameterID);
#endif

#ifdef TARGET_API_RTAS
	// called by GUI when mouse up event has occured; NO_UI: Call process' ReleaseControl()
	if (m_Process)
	{
		m_Process->ProcessReleaseControl(dfx::ParameterID_ToRTAS(inParameterID));
	}
#endif
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::randomizeparameter(long inParameterID, bool inWriteAutomation)
{
#ifdef TARGET_API_AUDIOUNIT
	if (inWriteAutomation)
	{
		automationgesture_begin(inParameterID);
	}

	Boolean const writeAutomation_fixedSize = inWriteAutomation;
	dfxgui_SetProperty(dfx::kPluginProperty_RandomizeParameter, dfx::kScope_Global, inParameterID, 
					   &writeAutomation_fixedSize, sizeof(writeAutomation_fixedSize));

	if (inWriteAutomation)
	{
		automationgesture_end(inParameterID);
	}
#else
	assert(false);  // TODO: implement
#endif
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::randomizeparameters(bool inWriteAutomation)
{
#ifdef TARGET_API_AUDIOUNIT
	if (inWriteAutomation)
	{
		std::lock_guard const guard(mAUParameterListLock);
		for (auto const& parameterID : mAUParameterList)
		{
			automationgesture_begin(parameterID);
		}
	}

	Boolean const writeAutomation_fixedSize = inWriteAutomation;
	dfxgui_SetProperty(dfx::kPluginProperty_RandomizeParameter, dfx::kScope_Global, kAUParameterListener_AnyParameter, 
					   &writeAutomation_fixedSize, sizeof(writeAutomation_fixedSize));

	if (inWriteAutomation)
	{
		std::lock_guard const guard(mAUParameterListLock);
		for (auto const& parameterID : mAUParameterList)
		{
			automationgesture_end(parameterID);
		}
	}
#else
	assert(false);  // TODO: implement
#endif
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::GenerateParameterAutomationSnapshot(long inParameterID)
{
	setparameter_f(inParameterID, getparameter_f(inParameterID), true);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::GenerateParametersAutomationSnapshot()
{
	std::lock_guard const guard(mAUParameterListLock);
	for (auto const& parameterID : mAUParameterList)
	{
		GenerateParameterAutomationSnapshot(parameterID);
	}
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::dfxgui_GetParameterValueFromString_f(long inParameterID, std::string const& inText, double& outValue)
{
	outValue = 0.0;
	bool success = false;

	if (GetParameterValueType(inParameterID) == DfxParam::ValueType::Float)
	{
		auto const readCount = sscanf(dfx::SanitizeNumericalInput(inText).c_str(), "%lf", &outValue);
		success = (readCount >= 1) && (readCount != EOF);
	}
	else
	{
		long newValue_i {};
		success = dfxgui_GetParameterValueFromString_i(inParameterID, inText, newValue_i);
		if (success)
		{
			outValue = static_cast<double>(newValue_i);
		}
	}

	return success;
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::dfxgui_GetParameterValueFromString_i(long inParameterID, std::string const& inText, long& outValue)
{
	outValue = 0;
	bool success = false;

	if (GetParameterValueType(inParameterID) == DfxParam::ValueType::Float)
	{
		double newValue_f = 0.0;
		success = dfxgui_GetParameterValueFromString_f(inParameterID, inText, newValue_f);
		if (success)
		{
			DfxParam param;
			param.init_f("", 0.0, 0.0, -1.0, 1.0);
			DfxParam::Value paramValue;
			paramValue.f = newValue_f;
			outValue = param.derive_i(paramValue);
		}
	}
	else
	{
		auto const readCount = sscanf(dfx::SanitizeNumericalInput(inText).c_str(), "%ld", &outValue);
		success = (readCount >= 1) && (readCount != EOF);
	}

	return success;
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::dfxgui_SetParameterValueWithString(long inParameterID, std::string const& inText)
{
	bool success = false;

	if (dfxgui_IsValidParamID(inParameterID))
	{
		if (GetParameterValueType(inParameterID) == DfxParam::ValueType::Float)
		{
			double newValue = 0.0;
			success = dfxgui_GetParameterValueFromString_f(inParameterID, inText, newValue);
			if (success)
			{
				setparameter_f(inParameterID, newValue, true);
			}
		}
		else
		{
			long newValue {};
			success = dfxgui_GetParameterValueFromString_i(inParameterID, inText, newValue);
			if (success)
			{
				setparameter_i(inParameterID, newValue, true);
			}
		}
	}

	return success;
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::dfxgui_IsValidParamID(long inParameterID) const
{
	if ((inParameterID == dfx::kParameterID_Invalid) || (inParameterID < 0))
	{
		return false;
	}
#ifdef TARGET_API_AUDIOUNIT
	// TODO: actually search parameter ID list to ensure that this ID is present?
	if (static_cast<AudioUnitParameterID>(inParameterID) > mAUMaxParameterID)
	{
		return false;
	}
#else
	if (inParameterID >= GetNumParameters())
	{
		return false;
	}
#endif

	return true;
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::TextEntryForParameterValue(long inParameterID)
{
	if (!getFrame())
	{
		return;
	}

	mTextEntryDialog = VSTGUI::makeOwned<DGTextEntryDialog>(inParameterID, getparametername(inParameterID), "enter value:");
	if (mTextEntryDialog)
	{
		std::array<char, dfx::kParameterValueStringMaxLength> textValue;
		textValue.fill(0);
		if (GetParameterValueType(inParameterID) == DfxParam::ValueType::Float)
		{
			snprintf(textValue.data(), textValue.size(), "%.6lf", getparameter_f(inParameterID));
		}
		else
		{
			snprintf(textValue.data(), textValue.size(), "%ld", getparameter_i(inParameterID));
		}
		mTextEntryDialog->setText(textValue.data());

		auto const textEntryCallback = [this](DGDialog* inDialog, DGDialog::Selection inSelection)
		{
			auto const textEntryDialog = dynamic_cast<DGTextEntryDialog*>(inDialog);
			if (textEntryDialog && (inSelection == DGDialog::kSelection_OK))
			{
				return dfxgui_SetParameterValueWithString(textEntryDialog->getParameterID(), textEntryDialog->getText());
			}
			return true;
		};
		[[maybe_unused]] auto const success = mTextEntryDialog->runModal(getFrame(), textEntryCallback);
		assert(success);
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
	if (!inMousedOverControl)
	{
		return;
	}
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
VSTGUI::CMouseEventResult DfxGuiEditor::onMouseDown(VSTGUI::CFrame* inFrame, VSTGUI::CPoint const& inPos, VSTGUI::CButtonState const& inButtons)
{
	if (!inFrame->getViewAt(inPos, VSTGUI::GetViewOptions().deep()))
	{
		auto const handled = handleContextualMenuClick(nullptr, inButtons);
		if (handled)
		{
			return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
		}
	}

	return VSTGUI::kMouseEventNotHandled;
}

//-----------------------------------------------------------------------------
// entered and exited is not enough because they stop notifying when mouse buttons are pressed
VSTGUI::CMouseEventResult DfxGuiEditor::onMouseMoved(VSTGUI::CFrame* inFrame, VSTGUI::CPoint const& inPos, VSTGUI::CButtonState const& /*inButtons*/)
{
	IDGControl* currentControl = nullptr;
	if (auto const currentView = inFrame->getViewAt(inPos, VSTGUI::GetViewOptions().deep()))
	{
		if (auto const dgControl = dynamic_cast<IDGControl*>(currentView))
		{
			currentControl = dgControl;
		}
	}
	setCurrentControl_mouseover(currentControl);

	return VSTGUI::kMouseEventNotHandled;
}

//-----------------------------------------------------------------------------
VSTGUI::CMouseEventResult DfxGuiEditor::viewOnMouseDown(VSTGUI::CView* inView, VSTGUI::CPoint inPos, VSTGUI::CButtonState inButtons)
{
#if TARGET_PLUGIN_USES_MIDI
	auto const dgControl = dynamic_cast<IDGControl*>(inView);
	if (dgControl && dgControl->isParameterAttached() && getmidilearning() && inButtons.isLeftButton())
	{
		setmidilearner(dgControl->getParameterID());
	}
#endif
	return VSTGUI::ViewMouseListenerAdapter::viewOnMouseDown(inView, inPos, inButtons);
}

//-----------------------------------------------------------------------------
double DfxGuiEditor::getparameter_f(long inParameterID)
{
#ifdef TARGET_API_AUDIOUNIT
	dfx::ParameterValueRequest request;
	size_t dataSize = sizeof(request);
	request.inValueItem = dfx::ParameterValueItem::Current;
	request.inValueType = DfxParam::ValueType::Float;

	auto const status = dfxgui_GetProperty(dfx::kPluginProperty_ParameterValue, dfx::kScope_Global, 
										   inParameterID, &request, dataSize); 
	if (status == noErr)
	{
		return request.value.f;
	}
	return 0.0;
#else
	return dfxgui_GetEffectInstance()->getparameter_f(inParameterID);
#endif
}

//-----------------------------------------------------------------------------
long DfxGuiEditor::getparameter_i(long inParameterID)
{
#ifdef TARGET_API_AUDIOUNIT
	dfx::ParameterValueRequest request;
	size_t dataSize = sizeof(request);
	request.inValueItem = dfx::ParameterValueItem::Current;
	request.inValueType = DfxParam::ValueType::Int;

	auto const status = dfxgui_GetProperty(dfx::kPluginProperty_ParameterValue, dfx::kScope_Global, 
										   inParameterID, &request, dataSize);
	if (status == noErr)
	{
		return request.value.i;
	}
	return 0;
#else
	return dfxgui_GetEffectInstance()->getparameter_i(inParameterID);
#endif
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::getparameter_b(long inParameterID)
{
#ifdef TARGET_API_AUDIOUNIT
	dfx::ParameterValueRequest request;
	size_t dataSize = sizeof(request);
	request.inValueItem = dfx::ParameterValueItem::Current;
	request.inValueType = DfxParam::ValueType::Boolean;

	auto const status = dfxgui_GetProperty(dfx::kPluginProperty_ParameterValue, dfx::kScope_Global, 
										   inParameterID, &request, dataSize); 
	if (status == noErr)
	{
		return request.value.b;
	}
	return false;
#else
	return dfxgui_GetEffectInstance()->getparameter_b(inParameterID);
#endif
}

//-----------------------------------------------------------------------------
double DfxGuiEditor::getparameter_gen(long inParameterIndex)
{
#ifdef TARGET_API_VST
	return getEffect()->getParameter(inParameterIndex);
#else
	double currentValue = getparameter_f(inParameterIndex);
	return dfxgui_ContractParameterValue(inParameterIndex, currentValue);
#endif
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::setparameter_f(long inParameterID, double inValue, bool inWrapWithAutomationGesture)
{
	if (inWrapWithAutomationGesture)
	{
		automationgesture_begin(inParameterID);
	}

#ifdef TARGET_API_AUDIOUNIT
	dfx::ParameterValueRequest request;
	request.inValueItem = dfx::ParameterValueItem::Current;
	request.inValueType = DfxParam::ValueType::Float;
	request.value.f = inValue;

	dfxgui_SetProperty(dfx::kPluginProperty_ParameterValue, dfx::kScope_Global, inParameterID, 
					   &request, sizeof(request));
#else
	assert(false);  // TODO: implement
#endif

	if (inWrapWithAutomationGesture)
	{
		automationgesture_end(inParameterID);
	}
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::setparameter_i(long inParameterID, long inValue, bool inWrapWithAutomationGesture)
{
	if (inWrapWithAutomationGesture)
	{
		automationgesture_begin(inParameterID);
	}

#ifdef TARGET_API_AUDIOUNIT
	dfx::ParameterValueRequest request;
	request.inValueItem = dfx::ParameterValueItem::Current;
	request.inValueType = DfxParam::ValueType::Int;
	request.value.i = inValue;

	dfxgui_SetProperty(dfx::kPluginProperty_ParameterValue, dfx::kScope_Global, 
					   inParameterID, &request, sizeof(request));
#else
	assert(false);  // TODO: implement
#endif

	if (inWrapWithAutomationGesture)
	{
		automationgesture_end(inParameterID);
	}
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::setparameter_b(long inParameterID, bool inValue, bool inWrapWithAutomationGesture)
{
	if (inWrapWithAutomationGesture)
	{
		automationgesture_begin(inParameterID);
	}

#ifdef TARGET_API_AUDIOUNIT
	dfx::ParameterValueRequest request;
	request.inValueItem = dfx::ParameterValueItem::Current;
	request.inValueType = DfxParam::ValueType::Boolean;
	request.value.b = inValue;

	dfxgui_SetProperty(dfx::kPluginProperty_ParameterValue, dfx::kScope_Global, 
					   inParameterID, &request, sizeof(request));
#else
	assert(false);  // TODO: implement
#endif

	if (inWrapWithAutomationGesture)
	{
		automationgesture_end(inParameterID);
	}
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::setparameter_default(long inParameterID, bool inWrapWithAutomationGesture)
{
#ifdef TARGET_API_AUDIOUNIT
	AudioUnitParameterInfo paramInfo;
	auto status = dfxgui_GetParameterInfo(inParameterID, paramInfo);
	if (status == noErr)
	{
		if (inWrapWithAutomationGesture)
		{
			automationgesture_begin(inParameterID);
		}

		auto const auParam = dfxgui_MakeAudioUnitParameter(inParameterID);
		status = AUParameterSet(nullptr, nullptr, &auParam, paramInfo.defaultValue, 0);
		assert(status == noErr);

		if (inWrapWithAutomationGesture)
		{
			automationgesture_end(inParameterID);
		}
	}
#else
	assert(false);  // TODO: implement
#endif
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::setparameters_default(bool inWrapWithAutomationGesture)
{
#ifdef TARGET_API_AUDIOUNIT
	std::lock_guard const guard(mAUParameterListLock);
	for (auto const& parameterID : mAUParameterList)
	{
		setparameter_default(parameterID, inWrapWithAutomationGesture);
	}
#else
	assert(false);  // TODO: implement
#endif
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::getparametervaluestring(long inParameterID, char* outText)
{
	assert(outText);

	auto const stringIndex = getparameter_i(inParameterID);

#ifdef TARGET_API_AUDIOUNIT
	dfx::ParameterValueStringRequest request;
	size_t dataSize = sizeof(request);
	request.inStringIndex = stringIndex;
	auto const status = dfxgui_GetProperty(dfx::kPluginProperty_ParameterValueString, dfx::kScope_Global, 
										   inParameterID, &request, dataSize); 
	if (status == noErr)
	{
		strcpy(outText, request.valueString);
		return true;
	}

#else
	return dfxgui_GetEffectInstance()->getparametervaluestring(inParameterID, stringIndex, outText);
#endif

	return false;
}

//-----------------------------------------------------------------------------
std::string DfxGuiEditor::getparameterunitstring(long inParameterIndex)
{
	char unitLabel[dfx::kParameterUnitStringMaxLength];
	unitLabel[0] = 0;

#ifdef TARGET_API_AUDIOUNIT
	size_t dataSize = sizeof(unitLabel);
	auto const status = dfxgui_GetProperty(dfx::kPluginProperty_ParameterUnitLabel, dfx::kScope_Global, 
										   inParameterIndex, unitLabel, dataSize);
	if (status != noErr)
	{
		unitLabel[0] = 0;
	}

#else
	dfxgui_GetEffectInstance()->getparameterunitstring(inParameterIndex, unitLabel);
#endif

	return std::string(unitLabel);
}

//-----------------------------------------------------------------------------
std::string DfxGuiEditor::getparametername(long inParameterID)
{
	std::string resultString;

#ifdef TARGET_API_AUDIOUNIT
	AudioUnitParameterInfo auParamInfo;
	auto const status = dfxgui_GetParameterInfo(inParameterID, auParamInfo);
	if (status == noErr)
	{
		if ((auParamInfo.flags & kAudioUnitParameterFlag_HasCFNameString) && auParamInfo.cfNameString)
		{
			auto const tempString = dfx::CreateCStringFromCFString(auParamInfo.cfNameString);
			if (tempString)
			{
				resultString.assign(tempString.get());
			}
			if (auParamInfo.flags & kAudioUnitParameterFlag_CFNameRelease)
			{
				CFRelease(auParamInfo.cfNameString);
			}
		}
		if (resultString.empty())
		{
			resultString.assign(auParamInfo.name);
		}
	}

#else
	char parameterCName[dfx::kParameterNameMaxLength];
	parameterCName[0] = 0;
	dfxgui_GetEffectInstance()->getparametername(inParameterID, parameterCName);
	resultString.assign(parameterCName);
#endif

	return resultString;
}

//-----------------------------------------------------------------------------
float DfxGuiEditor::dfxgui_ExpandParameterValue(long inParameterIndex, float inValue)
{
#ifdef TARGET_API_AUDIOUNIT
	dfx::ParameterValueConversionRequest request;
	request.inConversionType = dfx::ParameterValueConversionType::Expand;
	request.inValue = inValue;
	size_t dataSize = sizeof(request);
	auto const status = dfxgui_GetProperty(dfx::kPluginProperty_ParameterValueConversion, dfx::kScope_Global, 
										   inParameterIndex, &request, dataSize); 
	if (status == noErr)
	{
		return request.outValue;
	}
	else
	{
		auto const auParam = dfxgui_MakeAudioUnitParameter(inParameterIndex);
		return AUParameterValueFromLinear(inValue, &auParam);
	}
#else
	return dfxgui_GetEffectInstance()->expandparametervalue(inParameterIndex, inValue);
#endif
}

//-----------------------------------------------------------------------------
float DfxGuiEditor::dfxgui_ContractParameterValue(long inParameterIndex, float inValue)
{
#ifdef TARGET_API_AUDIOUNIT
	dfx::ParameterValueConversionRequest request;
	request.inConversionType = dfx::ParameterValueConversionType::Contract;
	request.inValue = inValue;
	size_t dataSize = sizeof(request);
	auto const status = dfxgui_GetProperty(dfx::kPluginProperty_ParameterValueConversion, dfx::kScope_Global, 
										   inParameterIndex, &request, dataSize); 
	if (status == noErr)
	{
		return request.outValue;
	}
	else
	{
		assert(false);  // really the above should not be failing
		auto const auParam = dfxgui_MakeAudioUnitParameter(inParameterIndex);
		return AUParameterValueToLinear(inValue, &auParam);
	}
#else
	return dfxgui_GetEffectInstance()->contractparametervalue(inParameterIndex, inValue);
#endif
}

//-----------------------------------------------------------------------------
float DfxGuiEditor::GetParameter_minValue(long inParameterIndex)
{
#ifdef TARGET_API_AUDIOUNIT
	AudioUnitParameterInfo paramInfo;
	auto const status = dfxgui_GetParameterInfo(inParameterIndex, paramInfo);
	if (status == noErr)
	{
		return paramInfo.minValue;
	}
#else
	return dfxgui_GetEffectInstance()->getparametermin_f(inParameterIndex);
#endif
	return 0.0f;
}

//-----------------------------------------------------------------------------
float DfxGuiEditor::GetParameter_maxValue(long inParameterIndex)
{
#ifdef TARGET_API_AUDIOUNIT
	AudioUnitParameterInfo paramInfo;
	auto const status = dfxgui_GetParameterInfo(inParameterIndex, paramInfo);
	if (status == noErr)
	{
		return paramInfo.maxValue;
	}
#else
	return dfxgui_GetEffectInstance()->getparametermax_f(inParameterIndex);
#endif
	return 0.0f;
}

//-----------------------------------------------------------------------------
float DfxGuiEditor::GetParameter_defaultValue(long inParameterIndex)
{
#ifdef TARGET_API_AUDIOUNIT
	AudioUnitParameterInfo paramInfo;
	auto const status = dfxgui_GetParameterInfo(inParameterIndex, paramInfo);
	if (status == noErr)
	{
		return paramInfo.defaultValue;
	}
#else
	return dfxgui_GetEffectInstance()->getparameterdefault_f(inParameterIndex);
#endif
	return 0.0f;
}

//-----------------------------------------------------------------------------
DfxParam::ValueType DfxGuiEditor::GetParameterValueType(long inParameterIndex)
{
#ifdef TARGET_API_AUDIOUNIT
	auto const valueType = dfxgui_GetProperty<DfxParam::ValueType>(dfx::kPluginProperty_ParameterValueType, 
																   dfx::kScope_Global, inParameterIndex);
	if (valueType)
	{
		return *valueType;
	}
#else
	return dfxgui_GetEffectInstance()->getparametervaluetype(inParameterIndex);
#endif
	return DfxParam::ValueType::Float;
}

//-----------------------------------------------------------------------------
DfxParam::Unit DfxGuiEditor::GetParameterUnit(long inParameterIndex)
{
#ifdef TARGET_API_AUDIOUNIT
	auto const unitType = dfxgui_GetProperty<DfxParam::Unit>(dfx::kPluginProperty_ParameterUnit, 
															 dfx::kScope_Global, inParameterIndex);
	if (unitType)
	{
		return *unitType;
	}
#else
	return dfxgui_GetEffectInstance()->getparameterunit(inParameterIndex);
#endif
	return DfxParam::Unit::Generic;
}

#ifdef TARGET_API_AUDIOUNIT
//-----------------------------------------------------------------------------
long DfxGuiEditor::dfxgui_GetParameterInfo(AudioUnitParameterID inParameterID, AudioUnitParameterInfo& outParameterInfo)
{
	memset(&outParameterInfo, 0, sizeof(outParameterInfo));
	size_t dataSize = sizeof(outParameterInfo);

	return dfxgui_GetProperty(kAudioUnitProperty_ParameterInfo, kAudioUnitScope_Global, inParameterID, 
							  &outParameterInfo, dataSize);
}
#endif

//-----------------------------------------------------------------------------
long DfxGuiEditor::GetNumParameters()
{
#ifdef TARGET_API_AUDIOUNIT
	// XXX questionable implementation; return max ID value +1 instead?
	size_t dataSize {};
	dfx::PropertyFlags propFlags {};
	auto const status = dfxgui_GetPropertyInfo(kAudioUnitProperty_ParameterList, dfx::kScope_Global, 0, dataSize, propFlags);
	if (status == noErr)
	{
		return (dataSize / sizeof(AudioUnitParameterID));
	}
#endif
#ifdef TARGET_API_VST
	return getEffect()->getAeffect()->numParams;
#endif
#ifdef TARGET_API_RTAS
	return m_Process->getnumparameters();
#endif
	return 0;
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
std::vector<AudioUnitParameterID> DfxGuiEditor::CreateParameterList(AudioUnitScope inScope)
{
	size_t dataSize {};
	dfx::PropertyFlags propFlags {};
	auto status = dfxgui_GetPropertyInfo(kAudioUnitProperty_ParameterList, inScope, 0, dataSize, propFlags);

	size_t const numParameters = (status == noErr) ? (dataSize / sizeof(AudioUnitParameterID)) : 0;
	if (numParameters == 0)
	{
		return {};
	}

	dfx::UniqueMemoryBlock<AudioUnitParameterID> const parameterListMemoryBlock(dataSize);
	if (!parameterListMemoryBlock)
	{
		return {};
	}

	status = dfxgui_GetProperty(kAudioUnitProperty_ParameterList, inScope, 0, parameterListMemoryBlock.get(), dataSize);
	if (status != noErr)
	{
		return {};
	}

	std::vector<AudioUnitParameterID> parameterList(parameterListMemoryBlock.get(), parameterListMemoryBlock.get() + numParameters);
	mAUMaxParameterID = *std::max_element(parameterList.cbegin(), parameterList.cend());
	return parameterList;
}
#endif

//-----------------------------------------------------------------------------
long DfxGuiEditor::dfxgui_GetPropertyInfo(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned long inItemIndex, 
										  size_t& outDataSize, dfx::PropertyFlags& outFlags)
{
#ifdef TARGET_API_AUDIOUNIT
	if (!dfxgui_GetEffectInstance())
	{
		return kAudioUnitErr_Uninitialized;
	}

	UInt32 auDataSize {};
	Boolean writable {};
	auto const status = AudioUnitGetPropertyInfo(dfxgui_GetEffectInstance(), inPropertyID, inScope, inItemIndex, &auDataSize, &writable);
	if (status == noErr)
	{
		outDataSize = auDataSize;
		outFlags = dfx::kPropertyFlag_Readable;  // XXX okay to just assume here?
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
long DfxGuiEditor::dfxgui_GetProperty(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned long inItemIndex, 
									  void* outData, size_t& ioDataSize)
{
#ifdef TARGET_API_AUDIOUNIT
	if (!dfxgui_GetEffectInstance())
	{
		return kAudioUnitErr_Uninitialized;
	}

	UInt32 auDataSize = ioDataSize;
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
long DfxGuiEditor::dfxgui_SetProperty(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned long inItemIndex, 
									  void const* inData, size_t inDataSize)
{
#ifdef TARGET_API_AUDIOUNIT
	if (!dfxgui_GetEffectInstance())
	{
		return kAudioUnitErr_Uninitialized;
	}

	return AudioUnitSetProperty(dfxgui_GetEffectInstance(), inPropertyID, inScope, inItemIndex, inData, inDataSize);
#else
	return dfxgui_GetEffectInstance()->dfx_SetProperty(inPropertyID, inScope, inItemIndex, inData, inDataSize);
#endif
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::LoadPresetFile()
{
	VSTGUI::SharedPointer<VSTGUI::CNewFileSelector> fileSelector(VSTGUI::CNewFileSelector::create(getFrame(), VSTGUI::CNewFileSelector::kSelectFile), false);
	if (fileSelector)
	{
		fileSelector->setTitle("Open");
#ifdef TARGET_API_AUDIOUNIT
		fileSelector->addFileExtension(*kDfxGui_AUPresetFileExtension);
		FSRef presetFileDirRef;
		auto const status = FindPresetsDirForAU(reinterpret_cast<Component>(dfxgui_GetEffectInstance()), kUserDomain, kDontCreateFolder, &presetFileDirRef);
		if (status == noErr)
		{
			dfx::UniqueCFType const presetFileDirURL = CFURLCreateFromFSRef(kCFAllocatorDefault, &presetFileDirRef);
			if (presetFileDirURL)
			{
				dfx::UniqueCFType const presetFileDirPathCF = CFURLCopyFileSystemPath(presetFileDirURL.get(), kCFURLPOSIXPathStyle);
				if (presetFileDirPathCF)
				{
					auto const presetFileDirPathC = dfx::CreateCStringFromCFString(presetFileDirPathCF.get());
					if (presetFileDirPathC)
					{
						fileSelector->setInitialDirectory(reinterpret_cast<VSTGUI::UTF8StringPtr>(presetFileDirPathC.get()));
					}
				}
			}
		}
#endif
#ifdef TARGET_API_VST
		fileSelector->addFileExtension(VSTGUI::CFileExtension("VST preset", "fxp"));
		fileSelector->addFileExtension(VSTGUI::CFileExtension("VST bank", "fxb"));
#endif
		fileSelector->run([effect = dfxgui_GetEffectInstance()](VSTGUI::CNewFileSelector* inFileSelector)
						  {
							  if (auto const filePath = inFileSelector->getSelectedFile(0))
							  {
#ifdef TARGET_API_AUDIOUNIT
								  dfx::UniqueCFType const fileUrl = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, reinterpret_cast<UInt8 const*>(filePath), static_cast<CFIndex>(strlen(filePath)), false);
								  if (fileUrl)
								  {
									  RestoreAUStateFromPresetFile(effect, fileUrl.get());
								  }
#endif
#ifdef TARGET_API_VST
								  assert(false);  // TODO: implement
#endif
							  }
						  });
	}
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::SavePresetFile()
{
	if (getFrame())
	{
#ifdef TARGET_API_AUDIOUNIT
		mTextEntryDialog = VSTGUI::makeOwned<DGTextEntryDialog>("Save preset file", "save as:", 
																DGDialog::kButtons_OKCancelOther, 
																"Save", nullptr, "Choose custom location...");
		if (mTextEntryDialog)
		{
			if (auto const button = mTextEntryDialog->getButton(DGDialog::Selection::kSelection_Other))
			{
				char const* const helpText = "choose a specific location to save in rather than the standard location (note:  this means that your presets will not be easily accessible in other host applications)";
				button->setTooltipText(helpText);
			}
			auto const textEntryCallback = [this](DGDialog* inDialog, DGDialog::Selection inSelection)
			{
				if (auto const textEntryDialog = dynamic_cast<DGTextEntryDialog*>(inDialog))
				{
					switch (inSelection)
					{
						case DGDialog::kSelection_OK:
							if (!textEntryDialog->getText().empty())
							{
								dfx::UniqueCFType const cfText = CFStringCreateWithCString(kCFAllocatorDefault, textEntryDialog->getText().c_str(), kCFStringEncodingUTF8);
								if (cfText)
								{
									auto const pluginBundle = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));
									assert(pluginBundle);
									auto const saveFileStatus = SaveAUStateToPresetFile_Bundle(dfxgui_GetEffectInstance(), cfText.get(), nullptr, true, pluginBundle);
									if (saveFileStatus == userCanceledErr)
									{
										return false;
									}
								}
							}
							return true;
						case DGDialog::kSelection_Other:
						{
							VSTGUI::SharedPointer<VSTGUI::CNewFileSelector> fileSelector(VSTGUI::CNewFileSelector::create(getFrame(), VSTGUI::CNewFileSelector::kSelectSaveFile), false);
							if (fileSelector)
							{
								fileSelector->setTitle("Save");
								fileSelector->setDefaultExtension(*kDfxGui_AUPresetFileExtension);
								if (!textEntryDialog->getText().empty())
								{
									fileSelector->setDefaultSaveName(textEntryDialog->getText());
								}
								fileSelector->run([effect = dfxgui_GetEffectInstance()](VSTGUI::CNewFileSelector* inFileSelector)
												  {
													  if (auto const filePath = inFileSelector->getSelectedFile(0))
													  {
														  dfx::UniqueCFType const fileUrl = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, reinterpret_cast<UInt8 const*>(filePath), static_cast<CFIndex>(strlen(filePath)), false);
														  if (fileUrl)
														  {
															  auto const pluginBundle = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));
															  assert(pluginBundle);
															  CustomSaveAUPresetFile_Bundle(effect, fileUrl.get(), false, pluginBundle);
														  }
													  }
												  });
							}
							return true;
						}
						default:
							break;
					}
				}
				return true;
			};
			[[maybe_unused]] auto const success = mTextEntryDialog->runModal(getFrame(), textEntryCallback);
			assert(success);
		}
#endif
#ifdef TARGET_API_VST
		assert(false);  // TODO: implement with a file selector, no text entry dialog
#endif
	}
}

//-----------------------------------------------------------------------------
DGEditorListenerInstance DfxGuiEditor::dfxgui_GetEffectInstance()
{
#ifdef TARGET_API_AUDIOUNIT
	return static_cast<AudioUnit>(getEffect());
#endif
#ifdef TARGET_API_VST
	return static_cast<DfxPlugin*>(getEffect());
#endif
#ifdef TARGET_API_RTAS
	return m_Process;
#endif
}

#if DEBUG
//-----------------------------------------------------------------------------
DfxPlugin* DfxGuiEditor::dfxgui_GetDfxPluginInstance()
{
	if (auto const pluginInstance = dfxgui_GetProperty<DfxPlugin*>(dfx::kPluginProperty_DfxPluginInstance))
	{
		return *pluginInstance;
	}
	return nullptr;
}
#endif

#if TARGET_PLUGIN_USES_MIDI
//-----------------------------------------------------------------------------
void DfxGuiEditor::setmidilearning(bool inLearnMode)
{
	Boolean newLearnMode_fixedSize = inLearnMode;
	dfxgui_SetProperty(dfx::kPluginProperty_MidiLearn, dfx::kScope_Global, 0, 
					   &newLearnMode_fixedSize, sizeof(newLearnMode_fixedSize));
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::getmidilearning()
{
	if (auto const learnMode = dfxgui_GetProperty<Boolean>(dfx::kPluginProperty_MidiLearn))
	{
		return *learnMode;
	}
	return false;
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::resetmidilearn()
{
	Boolean nud;  // irrelevant
	dfxgui_SetProperty(dfx::kPluginProperty_ResetMidiLearn, dfx::kScope_Global, 0, &nud, sizeof(nud));
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::setmidilearner(long inParameterIndex)
{
	int32_t parameterIndex_fixedSize = inParameterIndex;
	dfxgui_SetProperty(dfx::kPluginProperty_MidiLearner, dfx::kScope_Global, 0, 
					   &parameterIndex_fixedSize, sizeof(parameterIndex_fixedSize));
}

//-----------------------------------------------------------------------------
long DfxGuiEditor::getmidilearner()
{
	if (auto const learner = dfxgui_GetProperty<int32_t>(dfx::kPluginProperty_MidiLearner))
	{
		return *learner;
	}
	return dfx::kParameterID_Invalid;
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::ismidilearner(long inParameterIndex)
{
	return (getmidilearner() == inParameterIndex);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::setparametermidiassignment(long inParameterIndex, dfx::ParameterAssignment const& inAssignment)
{
	dfxgui_SetProperty(dfx::kPluginProperty_ParameterMidiAssignment, dfx::kScope_Global, 
					   inParameterIndex, &inAssignment, sizeof(inAssignment));
}

//-----------------------------------------------------------------------------
dfx::ParameterAssignment DfxGuiEditor::getparametermidiassignment(long inParameterIndex)
{
	dfx::ParameterAssignment parameterAssignment;
	size_t dataSize = sizeof(parameterAssignment);
	auto const status = dfxgui_GetProperty(dfx::kPluginProperty_ParameterMidiAssignment, dfx::kScope_Global, 
										   inParameterIndex, &parameterAssignment, dataSize); 
	if (status != noErr)
	{
		parameterAssignment.mEventType = dfx::MidiEventType::None;
	}
	return parameterAssignment;
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::parametermidiunassign(long inParameterIndex)
{
	dfx::ParameterAssignment parameterAssignment;
	parameterAssignment.mEventType = dfx::MidiEventType::None;
	setparametermidiassignment(inParameterIndex, parameterAssignment);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::TextEntryForParameterMidiCC(long inParameterID)
{
	if (!getFrame())
	{
		return;
	}

	mTextEntryDialog = VSTGUI::makeOwned<DGTextEntryDialog>(inParameterID, getparametername(inParameterID), "enter value:");
	if (mTextEntryDialog)
	{
		// initialize the text with the current CC assignment, if there is one
		std::array<char, dfx::kParameterValueStringMaxLength> initialText;
		initialText.fill(0);
		auto const currentParameterAssignment = getparametermidiassignment(inParameterID);
		if (currentParameterAssignment.mEventType == dfx::MidiEventType::CC)
		{
			snprintf(initialText.data(), initialText.size(), "%d", currentParameterAssignment.mEventNum);
		}
		mTextEntryDialog->setText(initialText.data());

		auto const textEntryCallback = [this](DGDialog* inDialog, DGDialog::Selection inSelection)
		{
			auto const textEntryDialog = dynamic_cast<DGTextEntryDialog*>(inDialog);
			if (textEntryDialog && (inSelection == DGDialog::kSelection_OK))
			{
				int newValue {};
				auto const scanCount = sscanf(textEntryDialog->getText().c_str(), "%d", &newValue);
				if ((scanCount > 0) && (scanCount != EOF))
				{
					dfx::ParameterAssignment newParameterAssignment;
					newParameterAssignment.mEventType = dfx::MidiEventType::CC;
					newParameterAssignment.mEventChannel = 0;  // XXX not currently implemented
					newParameterAssignment.mEventNum = newValue;
					setparametermidiassignment(textEntryDialog->getParameterID(), newParameterAssignment);
					return true;
				}
				return false;
			}
			return true;
		};
		[[maybe_unused]] auto const success = mTextEntryDialog->runModal(getFrame(), textEntryCallback);
		assert(success);
	}
}

#endif
// TARGET_PLUGIN_USES_MIDI

//-----------------------------------------------------------------------------
unsigned long DfxGuiEditor::getNumAudioChannels()
{
#ifdef TARGET_API_AUDIOUNIT
	auto const streamDesc = dfxgui_GetProperty<CAStreamBasicDescription>(kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output);
	return streamDesc ? streamDesc->NumberChannels() : 0;
#endif
#ifdef TARGET_API_VST
	return static_cast<unsigned long>(getEffect()->getAeffect()->numOutputs);
#endif
#ifdef TARGET_API_RTAS
	return m_Process->getnumoutputs();
#endif
}


//-----------------------------------------------------------------------------
enum
{
	kDfxContextualMenuItem_Global_SetDefaultParameterValues = 0,
//	kDfxContextualMenuItem_Global_Undo,
	kDfxContextualMenuItem_Global_RandomizeParameterValues,
	kDfxContextualMenuItem_Global_GenerateAutomationSnapshot,
	kDfxContextualMenuItem_Global_CopyState,
	kDfxContextualMenuItem_Global_PasteState,
	kDfxContextualMenuItem_Global_SavePresetFile,
	kDfxContextualMenuItem_Global_LoadPresetFile,
//	kDfxContextualMenuItem_Global_UserPresets,	// preset files sub-menu(s)?
//	kDfxContextualMenuItem_Global_FactoryPresets,	// factory presets sub-menu?
#if !TARGET_PLUGIN_IS_INSTRUMENT
//	kDfxContextualMenuItem_Global_Bypass,	// effect bypass?
#endif
#if TARGET_PLUGIN_USES_MIDI
	kDfxContextualMenuItem_Global_MidiLearn,
	kDfxContextualMenuItem_Global_MidiReset,
#endif
	kDfxContextualMenuItem_Global_OpenDocumentation,
//	kDfxContextualMenuItem_Global_ResizeGUI,
	kDfxContextualMenuItem_Global_OpenWebSite,
	kDfxContextualMenuItem_Global_NumItems
};

enum
{
	kDfxContextualMenuItem_Parameter_SetDefaultValue = 0,
	kDfxContextualMenuItem_Parameter_TextEntryForValue,	// type in value
//	kDfxContextualMenuItem_Parameter_ValueStrings,	// a sub-menu of value selections, for parameters that have them
//	kDfxContextualMenuItem_Parameter_Undo,
	kDfxContextualMenuItem_Parameter_RandomizeParameterValue,
	kDfxContextualMenuItem_Parameter_GenerateAutomationSnapshot,
//	kDfxContextualMenuItem_Parameter_SnapMode,	// toggle snap mode
#if TARGET_PLUGIN_USES_MIDI
	kDfxContextualMenuItem_Parameter_MidiLearner,
	kDfxContextualMenuItem_Parameter_MidiUnassign,
	kDfxContextualMenuItem_Parameter_TextEntryForMidiCC,	// assign MIDI CC by typing in directly
#endif
	kDfxContextualMenuItem_Parameter_NumItems
};

static constexpr int32_t kDfxGui_ContextualMenuStyle = VSTGUI::COptionMenu::kMultipleCheckStyle;  // this is necessary for menu entry checkmarks to show up

namespace
{

//-----------------------------------------------------------------------------
VSTGUI::CMenuItem* DFX_AppendCommandItemToMenu(VSTGUI::COptionMenu& inMenu, VSTGUI::UTF8StringPtr inMenuItemText, int32_t inCommandID, bool inEnabled, bool inChecked)
{
	if (auto const menuItem = new VSTGUI::CMenuItem(inMenuItemText, inCommandID))
	{
		menuItem->setEnabled(inEnabled);
		menuItem->setChecked(inChecked);
		return inMenu.addEntry(menuItem);
	}
	return nullptr;
}

}  // namespace

//-----------------------------------------------------------------------------
bool DfxGuiEditor::handleContextualMenuClick(VSTGUI::CControl* inControl, VSTGUI::CButtonState const& inButtons)
{
	auto isContextualMenuClick = inButtons.isRightButton();
#if TARGET_OS_MAC
	if (inButtons.isLeftButton() && inButtons.isAppleSet())
	{
		isContextualMenuClick = true;
	}
#endif
	if (!isContextualMenuClick)
	{
		return false;
	}

	if (!getFrame())
	{
		return false;
	}

	auto const dgControl = dynamic_cast<IDGControl*>(inControl);
	auto popupMenu = createContextualMenu(dgControl);

	// --------- show the contextual menu ---------
	VSTGUI::CPoint mousePos;
	[[maybe_unused]] auto const mousePosSuccess = getFrame()->getCurrentMouseLocation(mousePos);
	assert(mousePosSuccess);
	auto const handled = popupMenu.popup(getFrame(), mousePos);
	if (!handled)
	{
		return handled;
	}

	int32_t resultIndex = -1;
	auto const resultMenu = popupMenu.getLastItemMenu(resultIndex);
	if (!resultMenu)
	{
		return false;
	}
	auto const resultMenuItem = resultMenu->getEntry(resultIndex);
	if (!resultMenuItem)
	{
		return false;
	}
	auto const menuSelectionCommandID = resultMenuItem->getTag();

// --------- handle the contextual menu command (global) ---------
	std::map<int, std::function<void()>> const globalActionMap
	{
		{ kDfxContextualMenuItem_Global_SetDefaultParameterValues, [this](){ setparameters_default(true); } },
#if 0
		{ kDfxContextualMenuItem_Global_Undo, },  //XXX TODO: implement
#endif
		{ kDfxContextualMenuItem_Global_RandomizeParameterValues, [this](){ randomizeparameters(true); } },  // XXX "yes" to writing automation data?
		{ kDfxContextualMenuItem_Global_GenerateAutomationSnapshot, std::bind(&DfxGuiEditor::GenerateParametersAutomationSnapshot, this) },
		{ kDfxContextualMenuItem_Global_CopyState, [this](){ copySettings(); } },
		{ kDfxContextualMenuItem_Global_PasteState, [this](){ pasteSettings(); } },
		{ kDfxContextualMenuItem_Global_SavePresetFile, std::bind(&DfxGuiEditor::SavePresetFile, this) },
		{ kDfxContextualMenuItem_Global_LoadPresetFile, std::bind(&DfxGuiEditor::LoadPresetFile, this) },
#if TARGET_PLUGIN_USES_MIDI
		{ kDfxContextualMenuItem_Global_MidiLearn, std::bind(&DfxGuiEditor::setmidilearning, this, !getmidilearning()) },
		{ kDfxContextualMenuItem_Global_MidiReset, std::bind(&DfxGuiEditor::resetmidilearn, this) },
#endif
		{ kDfxContextualMenuItem_Global_OpenDocumentation, [](){ dfx::LaunchDocumentation(); } },
		{ kDfxContextualMenuItem_Global_OpenWebSite, [](){ dfx::LaunchURL(PLUGIN_HOMEPAGE_URL); } },
	};
	if (auto const action = globalActionMap.find(menuSelectionCommandID); action != globalActionMap.end())
	{
		action->second();
	}
// --------- handle the contextual menu command (parameter-specific) ---------
	else if (dgControl && dgControl->isParameterAttached())
	{
		using std::placeholders::_1;
		std::map<int, std::function<void(long)>> const parameterActionMap
		{
			{ kDfxContextualMenuItem_Parameter_SetDefaultValue, std::bind(&DfxGuiEditor::setparameter_default, this, _1, true) },
			{ kDfxContextualMenuItem_Parameter_TextEntryForValue, std::bind(&DfxGuiEditor::TextEntryForParameterValue, this, _1) },
#if 0
			{ kDfxContextualMenuItem_Parameter_Undo, },  //XXX TODO: implement
#endif
			{ kDfxContextualMenuItem_Parameter_RandomizeParameterValue, std::bind(&DfxGuiEditor::randomizeparameter, this, _1, true) },  // XXX "yes" to writing automation data?
			{ kDfxContextualMenuItem_Parameter_GenerateAutomationSnapshot, std::bind(&DfxGuiEditor::GenerateParameterAutomationSnapshot, this, _1) },
#if TARGET_PLUGIN_USES_MIDI
			{ kDfxContextualMenuItem_Parameter_MidiLearner, [this](long inParameterID){ setmidilearner((getmidilearner() == inParameterID) ? DfxSettings::kNoLearner : inParameterID); } },
			{ kDfxContextualMenuItem_Parameter_MidiUnassign, std::bind(&DfxGuiEditor::parametermidiunassign, this, _1) },
			{ kDfxContextualMenuItem_Parameter_TextEntryForMidiCC, std::bind(&DfxGuiEditor::TextEntryForParameterMidiCC, this, _1) },
#endif
		};
		if (auto const parameterAction = parameterActionMap.find(menuSelectionCommandID - kDfxContextualMenuItem_Global_NumItems); parameterAction != parameterActionMap.end())
		{
			parameterAction->second(dgControl->getParameterID());
		}
		else
		{
			assert(false);
		}
	}
	else
	{
		assert(false);
	}

	return handled;
}

//-----------------------------------------------------------------------------
VSTGUI::COptionMenu DfxGuiEditor::createContextualMenu(IDGControl* inControl)
{
	VSTGUI::COptionMenu resultMenu;
	resultMenu.setStyle(kDfxGui_ContextualMenuStyle);

	// populate the parameter-specific section of the menu
	if (inControl && inControl->isParameterAttached())
	{
		auto const paramID = inControl->getParameterID();
		if (auto const parameterSubMenu = createParameterContextualMenu(paramID))
		{
			resultMenu.addEntry(parameterSubMenu, getparametername(paramID));
			resultMenu.addSeparator();  // preface the global commands section with a divider
		}
	}

	// populate the global section of the menu
	for (UInt32 i = 0; i < kDfxContextualMenuItem_Global_NumItems; i++)
	{
		bool showCheckmark = false;
		bool disableItem = false;
		bool isFirstItemOfSubgroup = false;
		VSTGUI::UTF8StringPtr menuItemText = nullptr;
		switch (i)
		{
			case kDfxContextualMenuItem_Global_SetDefaultParameterValues:
				menuItemText = "Reset all parameter values to default";
				isFirstItemOfSubgroup = true;
				break;
#if 0
			case kDfxContextualMenuItem_Global_Undo:
				if (true)  // XXX needs appropriate check for which text/function to use
				{
					menuItemText = "Undo";
				}
				else
				{
					menuItemText = "Redo";
				}
				break;
#endif
			case kDfxContextualMenuItem_Global_RandomizeParameterValues:
				menuItemText = "Randomize all parameter values";
				break;
			case kDfxContextualMenuItem_Global_GenerateAutomationSnapshot:
				menuItemText = "Generate parameter automation snapshot";
				break;
			case kDfxContextualMenuItem_Global_CopyState:
				menuItemText = "Copy settings";
				isFirstItemOfSubgroup = true;
				break;
			case kDfxContextualMenuItem_Global_PasteState:
			{
				menuItemText = "Paste settings";
				bool currentClipboardIsPastable {};
				pasteSettings(&currentClipboardIsPastable);
				if (!currentClipboardIsPastable)
				{
					disableItem = true;
				}
				break;
			}
#ifndef TARGET_API_RTAS  // RTAS has no API for preset files, Pro Tools handles them
			case kDfxContextualMenuItem_Global_SavePresetFile:
				menuItemText = "Save preset file...";
				break;
			case kDfxContextualMenuItem_Global_LoadPresetFile:
				menuItemText = "Load preset file...";
				break;
#endif
#if TARGET_PLUGIN_USES_MIDI
			case kDfxContextualMenuItem_Global_MidiLearn:
				menuItemText = "MIDI learn";
				showCheckmark = getmidilearning();
				isFirstItemOfSubgroup = true;
				break;
			case kDfxContextualMenuItem_Global_MidiReset:
				menuItemText = "MIDI assignments reset";
				break;
#endif
			case kDfxContextualMenuItem_Global_OpenDocumentation:
				menuItemText = PLUGIN_NAME_STRING " manual";
				isFirstItemOfSubgroup = true;
				break;
			case kDfxContextualMenuItem_Global_OpenWebSite:
				menuItemText = "Open " PLUGIN_CREATOR_NAME_STRING " web site";
				break;
			default:
				assert(false);
				break;
		}
		if (isFirstItemOfSubgroup)
		{
			resultMenu.addSeparator();
		}
		if (menuItemText)
		{
			int32_t const menuItemCommandID = i;
			DFX_AppendCommandItemToMenu(resultMenu, menuItemText, menuItemCommandID, !disableItem, showCheckmark);
		}
	}
	resultMenu.cleanupSeparators(true);

	return resultMenu;
}

//-----------------------------------------------------------------------------
VSTGUI::SharedPointer<VSTGUI::COptionMenu> DfxGuiEditor::createParameterContextualMenu(long inParameterID)
{
	auto resultMenu = VSTGUI::makeOwned<VSTGUI::COptionMenu>();
	resultMenu->setStyle(kDfxGui_ContextualMenuStyle);

	for (UInt32 i = 0; i < kDfxContextualMenuItem_Parameter_NumItems; i++)
	{
		bool showCheckmark = false;
		bool disableItem = false;
		bool isFirstItemOfSubgroup = false;
		VSTGUI::UTF8StringPtr menuItemText = nullptr;
		std::array<char, 256> menuItemText_temp;
		menuItemText_temp.fill(0);
		switch (i)
		{
			case kDfxContextualMenuItem_Parameter_SetDefaultValue:
				menuItemText = "Set to default value";
				isFirstItemOfSubgroup = true;
				break;
			case kDfxContextualMenuItem_Parameter_TextEntryForValue:
				menuItemText = "Type in a value...";
				break;
#if 0
			case kDfxContextualMenuItem_Parameter_Undo:
				if (true)  // XXX needs appropriate check for which text/function to use
				{
					menuItemText = "Undo";
				}
				else
				{
					menuItemText = "Redo";
				}
				break;
#endif
			case kDfxContextualMenuItem_Parameter_RandomizeParameterValue:
				menuItemText = "Randomize value";
				break;
			case kDfxContextualMenuItem_Parameter_GenerateAutomationSnapshot:
				menuItemText = "Generate parameter automation snapshot";
				break;
#if TARGET_PLUGIN_USES_MIDI
			case kDfxContextualMenuItem_Parameter_MidiLearner:
				menuItemText = "MIDI learner";
				if (getmidilearning())
				{
					showCheckmark = ismidilearner(inParameterID);
				}
				else
				{
					disableItem = true;
				}
				isFirstItemOfSubgroup = true;
				break;
			case kDfxContextualMenuItem_Parameter_MidiUnassign:
			{
				menuItemText = "Unassign MIDI control";
				auto const currentParameterAssignment = getparametermidiassignment(inParameterID);
				// disable if not assigned
				if (currentParameterAssignment.mEventType == dfx::MidiEventType::None)
				{
					disableItem = true;
				}
				// append the current MIDI assignment, if there is one, to the menu item text
				else
				{
					constexpr auto maxTextLength = menuItemText_temp.size();
					switch (currentParameterAssignment.mEventType)
					{
						case dfx::MidiEventType::CC:
							snprintf(menuItemText_temp.data(), maxTextLength, "%s (CC %d)", menuItemText, currentParameterAssignment.mEventNum);
							break;
						case dfx::MidiEventType::ChannelAftertouch:
							snprintf(menuItemText_temp.data(), maxTextLength, "%s (channel aftertouch)", menuItemText);
							break;
						case dfx::MidiEventType::PitchBend:
							snprintf(menuItemText_temp.data(), maxTextLength, "%s (pitchbend)", menuItemText);
							break;
						case dfx::MidiEventType::Note:
							snprintf(menuItemText_temp.data(), maxTextLength, "%s (notes %s - %s)", menuItemText, dfx::GetNameForMIDINote(currentParameterAssignment.mEventNum).c_str(), dfx::GetNameForMIDINote(currentParameterAssignment.mEventNum2).c_str());
							break;
						default:
							break;
					}
					if (strlen(menuItemText_temp.data()) > 0)
					{
						menuItemText = menuItemText_temp.data();
					}
				}
				break;
			}
			case kDfxContextualMenuItem_Parameter_TextEntryForMidiCC:
				menuItemText = "Type in a MIDI CC assignment...";
				break;
#endif
			default:
				assert(false);
				break;
		}
		if (isFirstItemOfSubgroup)
		{
			resultMenu->addSeparator();
		}
		if (menuItemText)
		{
			int32_t const menuItemCommandID = i + kDfxContextualMenuItem_Global_NumItems;
			DFX_AppendCommandItemToMenu(*resultMenu, menuItemText, menuItemCommandID, !disableItem, showCheckmark);
		}
	}

	return resultMenu;
}

//-----------------------------------------------------------------------------
long DfxGuiEditor::initClipboard()
{
#if TARGET_OS_MAC
	// already initialized (allow for lazy initialization)
	if (mClipboardRef)
	{
		return noErr;
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
	#warning "implementation missing"
#endif

	return dfx::kStatus_NoError;  // XXX TODO: implement
}

//-----------------------------------------------------------------------------
long DfxGuiEditor::copySettings()
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

#ifdef TARGET_API_AUDIOUNIT
	CFPropertyListRef auSettingsPropertyList = nullptr;
	size_t dataSize = sizeof(auSettingsPropertyList);
	status = dfxgui_GetProperty(kAudioUnitProperty_ClassInfo, kAudioUnitScope_Global, 0, 
								&auSettingsPropertyList, dataSize);
	if (status != noErr)
	{
		return status;
	}
	if (!auSettingsPropertyList)
	{
		return coreFoundationUnknownErr;
	}

	dfx::UniqueCFType const auSettingsCFData = CFPropertyListCreateXMLData(kCFAllocatorDefault, auSettingsPropertyList);
	if (!auSettingsCFData)
	{
		return coreFoundationUnknownErr;
	}
	status = PasteboardPutItemFlavor(mClipboardRef.get(), (PasteboardItemID)PLUGIN_ID, CFSTR(kDfxGui_AUPresetFileUTI), auSettingsCFData.get(), kPasteboardFlavorNoFlags);
#endif
#else
	#warning "implementation missing"
#endif  // TARGET_OS_MAC

	return status;
}

//-----------------------------------------------------------------------------
long DfxGuiEditor::pasteSettings(bool* inQueryPastabilityOnly)
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

#if TARGET_OS_MAC
	PasteboardSynchronize(mClipboardRef.get());

	bool pastableItemFound = false;
	ItemCount itemCount {};
	status = PasteboardGetItemCount(mClipboardRef.get(), &itemCount);
	if (status != noErr)
	{
		return status;
	}
	for (UInt32 itemIndex = 1; itemIndex <= itemCount; itemIndex++)
	{
		PasteboardItemID itemID = nullptr;
		status = PasteboardGetItemIdentifier(mClipboardRef.get(), itemIndex, &itemID);
		if (status != noErr)
		{
			continue;
		}
		if (reinterpret_cast<unsigned long>(itemID) != PLUGIN_ID)  // XXX hacky?
		{
			continue;
		}
		CFArrayRef flavorTypesArray_temp = nullptr;
		status = PasteboardCopyItemFlavors(mClipboardRef.get(), itemID, &flavorTypesArray_temp);
		if ((status != noErr) || !flavorTypesArray_temp)
		{
			continue;
		}
		dfx::UniqueCFType const flavorTypesArray = flavorTypesArray_temp;
		auto const flavorCount = CFArrayGetCount(flavorTypesArray.get());
		for (CFIndex flavorIndex = 0; flavorIndex < flavorCount; flavorIndex++)
		{
			auto const flavorType = static_cast<CFStringRef>(CFArrayGetValueAtIndex(flavorTypesArray.get(), flavorIndex));
			if (!flavorType)
			{
				continue;
			}
#ifdef TARGET_API_AUDIOUNIT
			if (UTTypeConformsTo(flavorType, CFSTR(kDfxGui_AUPresetFileUTI)))
			{
				if (inQueryPastabilityOnly)
				{
					*inQueryPastabilityOnly = pastableItemFound = true;
				}
				else
				{
					CFDataRef flavorData_temp = nullptr;
					status = PasteboardCopyItemFlavorData(mClipboardRef.get(), itemID, flavorType, &flavorData_temp);
					if ((status == noErr) && flavorData_temp)
					{
						dfx::UniqueCFType const flavorData = flavorData_temp;
						dfx::UniqueCFType const auSettingsPropertyList = CFPropertyListCreateFromXMLData(kCFAllocatorDefault, flavorData.get(), kCFPropertyListImmutable, nullptr);
						if (auSettingsPropertyList)
						{
							auto const auSettingsPropertyList_temp = auSettingsPropertyList.get();
							status = dfxgui_SetProperty(kAudioUnitProperty_ClassInfo, kAudioUnitScope_Global, 0, 
														&auSettingsPropertyList_temp, sizeof(auSettingsPropertyList_temp));
							if (status == noErr)
							{
								pastableItemFound = true;
								AUParameterChange_TellListeners(dfxgui_GetEffectInstance(), kAUParameterListener_AnyParameter);
							}
						}
					}
				}
			}
#endif	// TARGET_API_AUDIOUNIT
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
#else  // TARGET_OS_MAC
	#warning "implementation missing"
#endif

	return status;
}



#ifdef TARGET_API_AUDIOUNIT
//-----------------------------------------------------------------------------
static void DFXGUI_AudioUnitEventListenerProc(void* inCallbackRefCon, void* /*inObject*/, AudioUnitEvent const* inEvent, UInt64 /*inEventHostTime*/, Float32 /*inParameterValue*/)
{
	auto const ourOwnerEditor = static_cast<DfxGuiEditor*>(inCallbackRefCon);
	if (ourOwnerEditor && inEvent)
	{
		if (inEvent->mEventType == kAudioUnitEvent_ParameterValueChange)
		{
			auto const paramID = inEvent->mArgument.mParameter.mParameterID;
			auto const paramCurrentValue_norm = ourOwnerEditor->getparameter_gen(paramID);
			ourOwnerEditor->setParameter(paramID, paramCurrentValue_norm);
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
#endif

#ifdef TARGET_API_AUDIOUNIT
//-----------------------------------------------------------------------------
void DfxGuiEditor::HandleStreamFormatChange()
{
	auto const oldNumAudioChannels = mNumAudioChannels;
	mNumAudioChannels = getNumAudioChannels();
	if (mNumAudioChannels != oldNumAudioChannels)
	{
		numAudioChannelsChanged(mNumAudioChannels);
	}
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::HandleParameterListChange()
{
	std::lock_guard const guard(mAUParameterListLock);

	if (mAUEventListener)
	{
		for (auto const& parameterID : mAUParameterList)
		{
			auto const auParam = dfxgui_MakeAudioUnitParameter(parameterID);
			AUListenerRemoveParameter(mAUEventListener.get(), this, &auParam);
		}
	}

	mAUParameterList = CreateParameterList();

	if (mAUEventListener)
	{
		for (auto const& parameterID : mAUParameterList)
		{
			auto const auParam = dfxgui_MakeAudioUnitParameter(parameterID);
			AUListenerAddParameter(mAUEventListener.get(), this, &auParam);
		}
	}
}
#endif


#if TARGET_PLUGIN_USES_MIDI

//-----------------------------------------------------------------------------
void DfxGuiEditor::HandleMidiLearnChange()
{
	auto const midiLearning = getmidilearning();

	if (mMidiLearnButton)
	{
		long const controlValue = midiLearning ? 1 : 0;
		mMidiLearnButton->setValue_i(controlValue);
		if (mMidiLearnButton->isDirty())
		{
			mMidiLearnButton->valueChanged();
			mMidiLearnButton->invalid();
		}
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
DGButton* DfxGuiEditor::CreateMidiLearnButton(long inXpos, long inYpos, DGImage* inImage, bool inDrawMomentaryState)
{
	constexpr long numButtonStates = 2;
	auto controlWidth = inImage->getWidth();
	if (inDrawMomentaryState)
	{
		controlWidth /= 2;
	}
	auto const controlHeight = inImage->getHeight() / numButtonStates;

	DGRect const pos(inXpos, inYpos, controlWidth, controlHeight);
	mMidiLearnButton = emplaceControl<DGButton>(this, pos, inImage, numButtonStates, DGButton::Mode::Increment, inDrawMomentaryState);
	mMidiLearnButton->setUserProcedure([](long inValue, void* inUserData)
	{
		assert(inUserData);
		static_cast<DfxGuiEditor*>(inUserData)->setmidilearning(inValue != 0);
	}, this);
	return mMidiLearnButton;
}

//-----------------------------------------------------------------------------
DGButton* DfxGuiEditor::CreateMidiResetButton(long inXpos, long inYpos, DGImage* inImage)
{
	DGRect const pos(inXpos, inYpos, inImage->getWidth(), inImage->getHeight() / 2);
	mMidiResetButton = emplaceControl<DGButton>(this, pos, inImage, 2, DGButton::Mode::Momentary);
	mMidiResetButton->setUserProcedure([](long inValue, void* inUserData)
	{
		assert(inUserData);
		if (inValue != 0)
		{
			static_cast<DfxGuiEditor*>(inUserData)->resetmidilearn();
		}
	}, this);
	return mMidiResetButton;
}

#endif
// TARGET_PLUGIN_USES_MIDI






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
// Called by GUI when idle time available; NO_UI: Call process' DoIdle() method.  
// Keeps other processes from starving during certain events (like mouse down).
// XXX TODO: is this actually ever called by anything? seemingly not for AU
void DfxGuiEditor::doIdleStuff()
{
	TARGET_API_EDITOR_BASE_CLASS::doIdleStuff();  // XXX do this?

	if (m_Process)
	{
		m_Process->ProcessDoIdle();
	}
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
		if (point.isInside(controlBounds))
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
	inControlIndex = dfx::ParameterID_FromRTAS(inControlIndex);
	if (!inIsHighlighted)
	{
		inColor = eHighlight_None;
	}

	if (dfxgui_IsValidParamID(inControlIndex))
	{
		mParameterHighlightColors.at(inControlIndex) = inColor;
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

	auto const parameterIndex = inControl->getTag();
	if (!dfxgui_IsValidParamID(parameterIndex))
	{
		return;
	}

	VSTGUI::CColor highlightColor;
	switch (mParameterHighlightColors.at(parameterIndex))
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






#ifdef TARGET_API_AUDIOUNIT

#if __LP64__
extern "C" void DGVstGuiAUViewEntry() {}	// XXX workaround to quiet missing exported symbol error
#else

#pragma mark -

//-----------------------------------------------------------------------------
class DGVstGuiAUView : public AUCarbonViewBase 
{
public:
	explicit DGVstGuiAUView(AudioUnitCarbonView inInstance);
	virtual ~DGVstGuiAUView();

	OSStatus CreateUI(Float32 inXOffset, Float32 inYOffset) override;
	void RespondToEventTimer(EventLoopTimerRef inTimer) override;
	OSStatus Version() override;

private:
	std::unique_ptr<DfxGuiEditor> mDfxGuiEditor;
};

VIEW_COMPONENT_ENTRY(DGVstGuiAUView)

//-----------------------------------------------------------------------------
DGVstGuiAUView::DGVstGuiAUView(AudioUnitCarbonView inInstance)
:	AUCarbonViewBase(inInstance, DfxGuiEditor::kNotificationInterval)
{
}

//-----------------------------------------------------------------------------
DGVstGuiAUView::~DGVstGuiAUView()
{
	if (mDfxGuiEditor)
	{
		mDfxGuiEditor->close();
	}
}

//-----------------------------------------------------------------------------
void DGVstGuiAUView::RespondToEventTimer(EventLoopTimerRef inTimer)
{
	if (mDfxGuiEditor)
	{
		mDfxGuiEditor->idle();
	}
}

extern DfxGuiEditor* DFXGUI_NewEditorInstance(AudioUnit inEffectInstance);

//-----------------------------------------------------------------------------
OSStatus DGVstGuiAUView::CreateUI(Float32 inXOffset, Float32 inYOffset)
{
	if (!GetEditAudioUnit())
	{
		return kAudioUnitErr_Uninitialized;
	}

	mDfxGuiEditor.reset(DFXGUI_NewEditorInstance(GetEditAudioUnit()));
	if (!mDfxGuiEditor)
	{
		return memFullErr;
	}
	auto const success = mDfxGuiEditor->open(GetCarbonWindow());
	if (!success)
	{
		return mDfxGuiEditor->dfxgui_GetEditorOpenErrorCode();
	}

	assert(mDfxGuiEditor->getFrame()->getPlatformFrame()->getPlatformType() == kWindowRef);
	auto const editorHIView = static_cast<HIViewRef>(mDfxGuiEditor->getFrame()->getPlatformFrame()->getPlatformRepresentation());
	HIViewMoveBy(editorHIView, inXOffset, inYOffset);
	EmbedControl(editorHIView);

	// set the size of the embedding pane
	auto const& frameSize = mDfxGuiEditor->getFrame()->getViewSize();
	SizeControl(GetCarbonPane(), frameSize.getWidth(), frameSize.getHeight());

	CreateEventLoopTimer(kEventDurationMillisecond * 5.0, DfxGuiEditor::kIdleTimerInterval);

	HIViewSetVisible(editorHIView, true);
	HIViewSetNeedsDisplay(editorHIView, true);

	return noErr;
}

//-----------------------------------------------------------------------------
OSStatus DGVstGuiAUView::Version()
{
	return dfx::CompositePluginVersionNumberValue();
}

#endif	// !__LP64__

#endif	// TARGET_API_AUDIOUNIT
