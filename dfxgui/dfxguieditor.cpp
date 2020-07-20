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

#include "dfxguibutton.h"
#include "dfxguidialog.h"
#include "dfxmisc.h"
#include "idfxguicontrol.h"

#if TARGET_PLUGIN_USES_MIDI
	#include "dfxsettings.h"
#endif

#ifdef TARGET_API_AUDIOUNIT
	#include "dfx-au-utilities.h"
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
	__attribute__((no_destroy)) static VSTGUI::CFileExtension const kDfxGui_AUPresetFileExtension("Audio Unit preset", "aupreset", "", 0, kDfxGui_AUPresetFileUTI);  // TODO: C++23 [[no_destroy]]
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
	// XXX: This is possibly bogus, because it's accessing the effect
	// instance, but in VST the effect's destructor calls the editor's
	// destructor, so if we got here that way, the effect is already
	// ill-defined. (DfxPlugin now explicitly deletes the editor before its
	// destructor runs, so we don't use the problematic destructor order.)
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
	// In VST, this always returns false (not clear whether that is
	// intended), so don't treat that as an error.
#ifdef TARGET_API_VST
	(void)TARGET_API_EDITOR_BASE_CLASS::open(inWindow);
#else
	if (!TARGET_API_EDITOR_BASE_CLASS::open(inWindow))
	{
		return baseSuccess;
	}
#endif
	
	mControlsList.clear();

	frame = new VSTGUI::CFrame(VSTGUI::CRect(rect.left, rect.top, rect.right, rect.bottom), this);
	if (!frame)
	{
		return false;
	}
	frame->open(inWindow);
	frame->setBackground(GetBackgroundImage());
	frame->enableTooltips(true);


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
#else
	#warning "implementation missing (maybe VSTGUI::IPlatformFont::getAllPlatformFontFamilies can assist?)"
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

	for (auto& control : mControlsList)
	{
		if (control->getParameterID() == inParameterIndex)
		{
			control->setValue_gen(inValue);
			control->redraw();  // TODO: why is this also necessary? redraws are sometimes dropped without it
		}
	}

	parameterChanged(inParameterIndex);
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::valueChanged(VSTGUI::CControl* inControl)
{
	auto const paramIndex = inControl->getTag();
	auto const paramValue_norm = inControl->getValueNormalized();

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
	AudioUnitEvent auEvent {};
	auEvent.mEventType = kAudioUnitEvent_PropertyChange;
	auEvent.mArgument.mProperty.mAudioUnit = dfxgui_GetEffectInstance();
	auEvent.mArgument.mProperty.mPropertyID = inPropertyID;
	auEvent.mArgument.mProperty.mScope = inScope;
	auEvent.mArgument.mProperty.mElement = inItemIndex;
	mCustomPropertyAUEvents.push_back(auEvent);
#else
	#warning "implementation missing"
	assert(false);  // TODO: implement
#endif
}


//-----------------------------------------------------------------------------
IDGControl* DfxGuiEditor::addControl(IDGControl* inControl)
{
	assert(inControl);

	// XXX only add it to our controls list if it is attached to a parameter (?)
	if (dfxgui_IsValidParamID(inControl->getParameterID()))
	{
		assert(std::find(mControlsList.cbegin(), mControlsList.cend(), inControl) == mControlsList.cend());
		mControlsList.push_back(inControl);
		inControl->asCControl()->registerViewMouseListener(this);
	}

	[[maybe_unused]] auto const success = getFrame()->addView(inControl->asCControl());
	assert(success);

	return inControl;
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

	inControl->asCControl()->unregisterViewMouseListener(this);
	[[maybe_unused]] auto const success = getFrame()->removeView(inControl->asCControl());
	assert(success);
}

//-----------------------------------------------------------------------------
long DfxGuiEditor::GetWidth()
{
	ERect* editorRect = nullptr;
	if (getRect(&editorRect) && editorRect)
	{
		return editorRect->right - editorRect->left;
	}
	return 0;
}

//-----------------------------------------------------------------------------
long DfxGuiEditor::GetHeight()
{
	ERect* editorRect = nullptr;
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
	if (inWriteAutomation)
	{
		automationgesture_begin(inParameterID);
	}

#ifdef TARGET_API_AUDIOUNIT
	Boolean const writeAutomation_fixedSize = inWriteAutomation;
	dfxgui_SetProperty(dfx::kPluginProperty_RandomizeParameter, dfx::kScope_Global, inParameterID, 
					   &writeAutomation_fixedSize, sizeof(writeAutomation_fixedSize));
#endif

#ifdef TARGET_API_VST
	#warning "implementation missing"
	assert(false);  // TODO: implement
#endif

#ifdef TARGET_API_RTAS
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
	if (inWriteAutomation)
	{
		for (long parameterIndex = 0; parameterIndex < GetNumParameters(); parameterIndex++)
		{
			automationgesture_begin(parameterIndex);
		}
	}

	for (long parameterIndex = 0; parameterIndex < GetNumParameters(); parameterIndex++)
	{
		randomizeparameter(parameterIndex, false);  // TODO: false? not inWriteAutomation? though that would double begin/end?
	}

	if (inWriteAutomation)
	{
		for (long parameterIndex = 0; parameterIndex < GetNumParameters(); parameterIndex++)
		{
			automationgesture_end(parameterIndex);
		}
	}
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
#ifdef TARGET_API_AUDIOUNIT
	std::lock_guard const guard(mAUParameterListLock);
	for (auto const& parameterID : mAUParameterList)
	{
		GenerateParameterAutomationSnapshot(parameterID);
	}
#else
	// XXX Untested, but this looks like how we normally loop over the parameters. -tom7
	// (Or perhaps we could be keeping a list of parameter ids for all plugin formats?)
	for (long parameterID = 0; parameterID < GetNumParameters(); parameterID++)
		GenerateParameterAutomationSnapshot(parameterID);
#endif
}

//-----------------------------------------------------------------------------
std::optional<double> DfxGuiEditor::dfxgui_GetParameterValueFromString_f(long inParameterID, std::string const& inText)
{
	if (GetParameterValueType(inParameterID) == DfxParam::ValueType::Float)
	{
		double value {};
		auto const readCount = sscanf(dfx::SanitizeNumericalInput(inText).c_str(), "%lf", &value);
		if ((readCount >= 1) && (readCount != EOF))
		{
			return value;
		}
	}
	else
	{
		if (auto const newValue_i = dfxgui_GetParameterValueFromString_i(inParameterID, inText))
		{
			return static_cast<double>(*newValue_i);
		}
	}

	return {};
}

//-----------------------------------------------------------------------------
std::optional<long> DfxGuiEditor::dfxgui_GetParameterValueFromString_i(long inParameterID, std::string const& inText)
{
	if (GetParameterValueType(inParameterID) == DfxParam::ValueType::Float)
	{
		if (auto const newValue_f = dfxgui_GetParameterValueFromString_f(inParameterID, inText))
		{
			DfxParam param;
			param.init_f("", 0.0, 0.0, -1.0, 1.0);
			DfxParam::Value paramValue {};
			paramValue.f = *newValue_f;
			return param.derive_i(paramValue);
		}
	}
	else
	{
		long value {};
		auto const readCount = sscanf(dfx::SanitizeNumericalInput(inText).c_str(), "%ld", &value);
		if ((readCount >= 1) && (readCount != EOF))
		{
			return value;
		}
	}

	return {};
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::dfxgui_SetParameterValueWithString(long inParameterID, std::string const& inText)
{
	bool success = false;

	if (dfxgui_IsValidParamID(inParameterID))
	{
		if (GetParameterValueType(inParameterID) == DfxParam::ValueType::Float)
		{
			if (auto const newValue = dfxgui_GetParameterValueFromString_f(inParameterID, inText))
			{
				setparameter_f(inParameterID, *newValue, true);
			}
		}
		else
		{
			if (auto const newValue = dfxgui_GetParameterValueFromString_i(inParameterID, inText))
			{
				setparameter_i(inParameterID, *newValue, true);
			}
		}
	}

	return success;
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::dfxgui_IsValidParamID(long inParameterID)
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
		std::array<char, dfx::kParameterValueStringMaxLength> textValue {};
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
	auto const dgControl = dynamic_cast<IDGControl*>(inView);
	if (dgControl && inView->getMouseEnabled())
	{
		dgControl->invalidateMouseWheelEditingTimer();
#if TARGET_PLUGIN_USES_MIDI
		auto const isMultiControl = !dgControl->getChildren().empty();  // multi-controls should self-manage learn
		if (dgControl->isParameterAttached() && !isMultiControl && getmidilearning() && inButtons.isLeftButton())
		{
			setmidilearner(dgControl->getParameterID());
		}
#endif
	}
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
#endif

#ifdef TARGET_API_VST
	auto const value_norm = dfxgui_ContractParameterValue(inParameterID, inValue);
	getEffect()->setParameterAutomated(inParameterID, value_norm);
#endif

#ifdef TARGET_API_RTAS
	#warning "implementation missing"
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
#endif

#ifdef TARGET_API_VST
	auto const value_norm = dfxgui_ContractParameterValue(inParameterID, inValue);
	getEffect()->setParameterAutomated(inParameterID, value_norm);
#endif

#ifdef TARGET_API_RTAS
	#warning "implementation missing"
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
#endif

#ifdef TARGET_API_VST
	getEffect()->setParameterAutomated(inParameterID, inValue ? 1.0f : 0.0f);
#endif

#ifdef TARGET_API_RTAS
	#warning "implementation missing"
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
#endif

#ifdef TARGET_API_VST
	auto const defaultValue = GetParameter_defaultValue(inParameterID);
	auto const defaultValue_norm = dfxgui_ContractParameterValue(inParameterID, defaultValue);
	getEffect()->setParameterAutomated(inParameterID, defaultValue_norm);
#endif

#ifdef TARGET_API_RTAS
	#warning "implementation missing"
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
	for (long parameterIndex = 0; parameterIndex < GetNumParameters(); parameterIndex++)
	{
		setparameter_default(parameterIndex, inWrapWithAutomationGesture);
	}
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
	if (auto const parameterInfo = dfxgui_GetParameterInfo(inParameterID))
	{
		if ((parameterInfo->flags & kAudioUnitParameterFlag_HasCFNameString) && parameterInfo->cfNameString)
		{
			auto const tempString = dfx::CreateCStringFromCFString(parameterInfo->cfNameString);
			if (tempString)
			{
				resultString.assign(tempString.get());
			}
		}
		if (resultString.empty())
		{
			resultString.assign(parameterInfo->name);
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
	if (auto const parameterInfo = dfxgui_GetParameterInfo(inParameterIndex))
	{
		return parameterInfo->minValue;
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
	if (auto const parameterInfo = dfxgui_GetParameterInfo(inParameterIndex))
	{
		return parameterInfo->maxValue;
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
	if (auto const parameterInfo = dfxgui_GetParameterInfo(inParameterIndex))
	{
		return parameterInfo->defaultValue;
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
std::optional<DfxGuiEditor::AUParameterInfo> DfxGuiEditor::dfxgui_GetParameterInfo(AudioUnitParameterID inParameterID)
{
	auto const parameterInfo = dfxgui_GetProperty<AudioUnitParameterInfo>(kAudioUnitProperty_ParameterInfo, 
																		  kAudioUnitScope_Global, inParameterID);
	return parameterInfo ? std::make_optional(AUParameterInfo(*parameterInfo)) : std::nullopt;
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
		fileSelector->addFileExtension(kDfxGui_AUPresetFileExtension);
		dfx::UniqueCFType const presetsDirURL = FindPresetsDirForAU(AudioComponentInstanceGetComponent(dfxgui_GetEffectInstance()), kDFXFileSystemDomain_User, false);
		if (presetsDirURL)
		{
			if (dfx::UniqueCFType const presetsDirPathCF = CFURLCopyFileSystemPath(presetsDirURL.get(), kCFURLPOSIXPathStyle))
			{
				if (auto const presetsDirPathC = dfx::CreateCStringFromCFString(presetsDirPathCF.get()))
				{
					fileSelector->setInitialDirectory(presetsDirPathC.get());
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
								  dfx::UniqueCFType const fileURL = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, reinterpret_cast<UInt8 const*>(filePath), static_cast<CFIndex>(strlen(filePath)), false);
								  if (fileURL)
								  {
									  RestoreAUStateFromPresetFile(effect, fileURL.get());
								  }
#endif
#ifdef TARGET_API_VST
	#warning "implementation missing"
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
								fileSelector->setDefaultExtension(kDfxGui_AUPresetFileExtension);
								if (!textEntryDialog->getText().empty())
								{
									fileSelector->setDefaultSaveName(textEntryDialog->getText());
								}
								fileSelector->run([effect = dfxgui_GetEffectInstance()](VSTGUI::CNewFileSelector* inFileSelector)
												  {
													  if (auto const filePath = inFileSelector->getSelectedFile(0))
													  {
														  dfx::UniqueCFType const fileURL = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, reinterpret_cast<UInt8 const*>(filePath), static_cast<CFIndex>(strlen(filePath)), false);
														  if (fileURL)
														  {
															  auto const pluginBundle = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));
															  assert(pluginBundle);
															  CustomSaveAUPresetFile_Bundle(effect, fileURL.get(), false, pluginBundle);
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
	#warning "implementation missing"
		assert(false);  // TODO: implement with a file selector, no text entry dialog
#endif
	}
}

//-----------------------------------------------------------------------------
DGEditorListenerInstance DfxGuiEditor::dfxgui_GetEffectInstance()
{
#ifdef TARGET_API_RTAS
	return m_Process;
#else
	return static_cast<DGEditorListenerInstance>(getEffect());
#endif
}

#if defined(TARGET_API_AUDIOUNIT) && DEBUG
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
#ifdef TARGET_API_AUDIOUNIT
	Boolean newLearnMode_fixedSize = inLearnMode;
	dfxgui_SetProperty(dfx::kPluginProperty_MidiLearn, dfx::kScope_Global, 0, 
					   &newLearnMode_fixedSize, sizeof(newLearnMode_fixedSize));
#else
	dfxgui_GetEffectInstance()->setmidilearning(inLearnMode);
#endif
}

//-----------------------------------------------------------------------------
bool DfxGuiEditor::getmidilearning()
{
#ifdef TARGET_API_AUDIOUNIT
	if (auto const learnMode = dfxgui_GetProperty<Boolean>(dfx::kPluginProperty_MidiLearn))
	{
		return *learnMode;
	}
	return false;
#else
	return dfxgui_GetEffectInstance()->getmidilearning();
#endif
}

//-----------------------------------------------------------------------------
void DfxGuiEditor::resetmidilearn()
{
#ifdef TARGET_API_AUDIOUNIT  
	Boolean nud;  // irrelevant
	dfxgui_SetProperty(dfx::kPluginProperty_ResetMidiLearn, dfx::kScope_Global, 0, &nud, sizeof(nud));
#else
	dfxgui_GetEffectInstance()->resetmidilearn();
#endif
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
	auto const opt = dfxgui_GetProperty<dfx::ParameterAssignment>(dfx::kPluginProperty_ParameterMidiAssignment,
								      dfx::kScope_Global,
								      inParameterIndex);
	if (opt.has_value()) return opt.value();
	
	dfx::ParameterAssignment none;
	none.mEventType = dfx::MidiEventType::None;
	return none;
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
		std::array<char, dfx::kParameterValueStringMaxLength> initialText {};
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
bool DfxGuiEditor::handleContextualMenuClick(VSTGUI::CControl* inControl, VSTGUI::CButtonState const& inButtons)
{
	auto isContextualMenuClick = inButtons.isRightButton();
#if TARGET_OS_MAC
	isContextualMenuClick |= (inButtons.isLeftButton() && inButtons.isAppleSet());
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
	return popupMenu.popup(getFrame(), mousePos);
}

//-----------------------------------------------------------------------------
VSTGUI::COptionMenu DfxGuiEditor::createContextualMenu(IDGControl* inControl)
{
	VSTGUI::COptionMenu resultMenu;
	resultMenu.setStyle(kDfxGui_ContextualMenuStyle);

	// populate the parameter-specific section of the menu
	if (inControl && inControl->isParameterAttached())
	{
		auto const addParameterSubMenu = [this, &resultMenu](IDGControl const* control)
		{
			auto const paramID = control->getParameterID();
			if (auto const parameterSubMenu = createParameterContextualMenu(paramID))
			{
				resultMenu.addEntry(parameterSubMenu, getparametername(paramID));
			}
		};
		addParameterSubMenu(inControl);
		for (auto const& child : inControl->getChildren())
		{
			addParameterSubMenu(child);
		}
		if (resultMenu.getNbEntries())
		{
			resultMenu.addSeparator();  // preface the global commands section with a divider
		}
	}

	// populate the global section of the menu
	resultMenu.addSeparator();
	DFX_AppendCommandItemToMenu(resultMenu, "Reset all parameter values to default", 
								std::bind(&DfxGuiEditor::setparameters_default, this, true));
//	DFX_AppendCommandItemToMenu(resultMenu, true ? "Undo" : "Redo", std::bind());  // TODO: implement
	DFX_AppendCommandItemToMenu(resultMenu, "Randomize all parameter values", 
								std::bind(&DfxGuiEditor::randomizeparameters, this, true));  // XXX yes to writing automation data?
	DFX_AppendCommandItemToMenu(resultMenu, "Generate parameter automation snapshot", 
								std::bind(&DfxGuiEditor::GenerateParametersAutomationSnapshot, this));

	resultMenu.addSeparator();
	DFX_AppendCommandItemToMenu(resultMenu, "Copy settings", std::bind(&DfxGuiEditor::copySettings, this));
	{
		bool currentClipboardIsPastable {};
		pasteSettings(&currentClipboardIsPastable);
		DFX_AppendCommandItemToMenu(resultMenu, "Paste settings", std::bind(&DfxGuiEditor::pasteSettings, this, nullptr), currentClipboardIsPastable);
	}
#ifndef TARGET_API_RTAS  // RTAS has no API for preset files, Pro Tools handles them
	DFX_AppendCommandItemToMenu(resultMenu, "Save preset file...", std::bind(&DfxGuiEditor::SavePresetFile, this));
	DFX_AppendCommandItemToMenu(resultMenu, "Load preset file...", std::bind(&DfxGuiEditor::LoadPresetFile, this));
#endif

#if TARGET_PLUGIN_USES_MIDI
	resultMenu.addSeparator();
	DFX_AppendCommandItemToMenu(resultMenu, "MIDI learn", 
								std::bind(&DfxGuiEditor::setmidilearning, this, !getmidilearning()), 
								true, getmidilearning());
	DFX_AppendCommandItemToMenu(resultMenu, "MIDI assignments reset", std::bind(&DfxGuiEditor::resetmidilearn, this));
#endif

	resultMenu.addSeparator();
	DFX_AppendCommandItemToMenu(resultMenu, PLUGIN_NAME_STRING " manual", std::bind(&dfx::LaunchDocumentation));
	DFX_AppendCommandItemToMenu(resultMenu, "Open " PLUGIN_CREATOR_NAME_STRING " web site", 
								std::bind(&dfx::LaunchURL, PLUGIN_HOMEPAGE_URL));

	resultMenu.cleanupSeparators(true);
	return resultMenu;
}

//-----------------------------------------------------------------------------
VSTGUI::SharedPointer<VSTGUI::COptionMenu> DfxGuiEditor::createParameterContextualMenu(long inParameterID)
{
	assert(dfxgui_IsValidParamID(inParameterID));

	auto resultMenu = VSTGUI::makeOwned<VSTGUI::COptionMenu>();
	resultMenu->setStyle(kDfxGui_ContextualMenuStyle);

	resultMenu->addSeparator();
	DFX_AppendCommandItemToMenu(*resultMenu, "Set to default value", 
								std::bind(&DfxGuiEditor::setparameter_default, this, inParameterID, true));
	DFX_AppendCommandItemToMenu(*resultMenu, "Type in a value...", 
								std::bind(&DfxGuiEditor::TextEntryForParameterValue, this, inParameterID));
//	DFX_AppendCommandItemToMenu(*resultMenu, true ? "Undo" : "Redo", std::bind());  // TODO: implement
	DFX_AppendCommandItemToMenu(*resultMenu, "Randomize value", 
								std::bind(&DfxGuiEditor::randomizeparameter, this, inParameterID, true));  // XXX yes to writing automation data?
	DFX_AppendCommandItemToMenu(*resultMenu, "Generate parameter automation snapshot", 
								std::bind(&DfxGuiEditor::GenerateParameterAutomationSnapshot, this, inParameterID));

#if TARGET_PLUGIN_USES_MIDI
	resultMenu->addSeparator();
	DFX_AppendCommandItemToMenu(*resultMenu, "MIDI learner", 
								std::bind(&DfxGuiEditor::setmidilearner, this, 
										  ismidilearner(inParameterID) ? DfxSettings::kNoLearner : inParameterID), 
								getmidilearning(), ismidilearner(inParameterID));
	{
		VSTGUI::UTF8String menuItemText = "Unassign MIDI control";
		auto const currentParameterAssignment = getparametermidiassignment(inParameterID);
		bool const enableItem = (currentParameterAssignment.mEventType != dfx::MidiEventType::None);
		// append the current MIDI assignment, if there is one, to the menu item text
		if (enableItem)
		{
			menuItemText += [currentParameterAssignment]() -> std::string
			{
				switch (currentParameterAssignment.mEventType)
				{
					case dfx::MidiEventType::CC:
						return " (CC " + std::to_string(currentParameterAssignment.mEventNum) + ")";
					case dfx::MidiEventType::ChannelAftertouch:
						return " (channel aftertouch)";
					case dfx::MidiEventType::PitchBend:
						return " (pitchbend)";
					case dfx::MidiEventType::Note:
						return " (notes " + dfx::GetNameForMIDINote(currentParameterAssignment.mEventNum) + " - " + dfx::GetNameForMIDINote(currentParameterAssignment.mEventNum2) + ")";
					default:
						assert(false);
						return {};
				}
			}();
		}
		DFX_AppendCommandItemToMenu(*resultMenu, menuItemText, 
									std::bind(&DfxGuiEditor::parametermidiunassign, this, inParameterID), 
									enableItem);
	}
	DFX_AppendCommandItemToMenu(*resultMenu, "Type in a MIDI CC assignment...", 
								std::bind(&DfxGuiEditor::TextEntryForParameterMidiCC, this, inParameterID));
#endif

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
	assert(false);
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

	dfx::UniqueCFType const auSettingsCFData = CFPropertyListCreateData(kCFAllocatorDefault, auSettingsPropertyList, kCFPropertyListXMLFormat_v1_0, 0, nullptr);
	if (!auSettingsCFData)
	{
		return coreFoundationUnknownErr;
	}
	status = PasteboardPutItemFlavor(mClipboardRef.get(), PasteboardItemID(PLUGIN_ID), CFSTR(kDfxGui_AUPresetFileUTI), auSettingsCFData.get(), kPasteboardFlavorNoFlags);
#endif
#else
	#warning "implementation missing"
	assert(false);
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
		if (reinterpret_cast<std::intptr_t>(itemID) != PLUGIN_ID)  // XXX hacky?
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
						dfx::UniqueCFType const auSettingsPropertyList = CFPropertyListCreateWithData(kCFAllocatorDefault, flavorData.get(), kCFPropertyListImmutable, nullptr, nullptr);
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
	assert(false);
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
