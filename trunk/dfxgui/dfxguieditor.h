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

#pragma once


#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include "dfxdefines.h"
#include "dfxgui-base.h"
#include "dfxguimisc.h"
#include "dfxmisc.h"
#include "dfxplugin-base.h"
#include "dfxpluginproperties.h"

#if TARGET_OS_MAC
	#include <ApplicationServices/ApplicationServices.h>
#endif

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wnon-virtual-dtor"

#include "vstgui.h"

#ifdef TARGET_API_AUDIOUNIT
	using DGEditorListenerInstance = AudioComponentInstance;
#endif
#ifdef TARGET_API_VST
	using DGEditorListenerInstance = class DfxPlugin*;
#endif
#ifdef TARGET_API_RTAS
	using DGEditorListenerInstance = ITemplateProcess*;
	#if WINDOWS_VERSION
		using sRect = RECT;
	#elif MAC_VERSION
		using sRect = Rect;
	#endif
#endif

#ifdef TARGET_API_VST
	#include "aeffguieditor.h"
	using TARGET_API_EDITOR_BASE_CLASS = VSTGUI::AEffGUIEditor;
	using TARGET_API_EDITOR_INDEX_TYPE = VstInt32;
	using ERect = ::ERect;
#else
	#include "plugguieditor.h"
	using TARGET_API_EDITOR_BASE_CLASS = VSTGUI::PluginGUIEditor;
	using TARGET_API_EDITOR_INDEX_TYPE = int32_t;
	using ERect = VSTGUI::ERect;
#endif

#pragma clang diagnostic pop



//-----------------------------------------------------------------------------
#if defined(TARGET_API_AUDIOUNIT) || defined(TARGET_API_RTAS)
	#define DFX_EDITOR_ENTRY(PluginEditorClass)												\
		DfxGuiEditor* DFXGUI_NewEditorInstance(DGEditorListenerInstance inProcessInstance)	\
		{																					\
			return new PluginEditorClass(inProcessInstance);								\
		}
#endif

#ifdef TARGET_API_VST
	#define DFX_EDITOR_ENTRY(PluginEditorClass)											\
		AEffEditor* DFXGUI_NewEditorInstance(DGEditorListenerInstance inEffectInstance)	\
		{																				\
			return new PluginEditorClass(inEffectInstance);								\
		}
#endif



//-----------------------------------------------------------------------------
class IDGControl;
class DGButton;
class DGTextEntryDialog;


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"


/***********************************************************************
	DfxGuiEditor
***********************************************************************/

//-----------------------------------------------------------------------------
class DfxGuiEditor :	public TARGET_API_EDITOR_BASE_CLASS, 
						public VSTGUI::IControlListener, 
						public VSTGUI::IMouseObserver,
						public VSTGUI::ViewMouseListenerAdapter
{
public:
#ifdef TARGET_API_AUDIOUNIT
	static constexpr double kIdleTimerInterval = 1.0 / 24.0;  // 24 fps
	static constexpr Float32 kNotificationInterval = kIdleTimerInterval;

	class AUParameterInfo : public AudioUnitParameterInfo
	{
	public:
		explicit AUParameterInfo(AudioUnitParameterInfo const& inParameterInfo)
		:	AudioUnitParameterInfo(inParameterInfo), 
			mNameOwner(((flags & kAudioUnitParameterFlag_HasCFNameString) && (flags & kAudioUnitParameterFlag_CFNameRelease)) ? cfNameString : nullptr), 
			mUnitNameOwner((unit == kAudioUnitParameterUnit_CustomUnit) ? unitName : nullptr)
		{
		}
	private:
		dfx::UniqueCFType<CFStringRef> mNameOwner;
		dfx::UniqueCFType<CFStringRef> mUnitNameOwner;
	};
#endif

	explicit DfxGuiEditor(DGEditorListenerInstance inInstance);
	virtual ~DfxGuiEditor();

	// VSTGUI overrides
	bool open(void* inWindow) override;
	void close() override;
	void setParameter(TARGET_API_EDITOR_INDEX_TYPE inParameterIndex, float inValue) override;
	void valueChanged(VSTGUI::CControl* inControl) override;
	int32_t controlModifierClicked(VSTGUI::CControl* inControl, VSTGUI::CButtonState inButtons) override;
#ifndef TARGET_API_VST
	void beginEdit(int32_t inParameterIndex) override;
	void endEdit(int32_t inParameterIndex) override;
#endif
	void idle() override final;

	// these are for the child class of DfxGuiEditor to override
	virtual long OpenEditor() = 0;
	virtual void CloseEditor() {}
	virtual void post_open() {}
	virtual void dfxgui_EditorShown() {}

#ifdef TARGET_API_AUDIOUNIT
	std::optional<AUParameterInfo> dfxgui_GetParameterInfo(AudioUnitParameterID inParameterID);
	auto getAUEventListener() const noexcept
	{
		return mAUEventListener.get();
	}
#endif

	// TODO: implement plumbing to these for VST etc.
	void RegisterPropertyChange(dfx::PropertyID inPropertyID, dfx::Scope inScope = dfx::kScope_Global, unsigned long inItemIndex = 0);
	virtual void HandlePropertyChange(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned long inItemIndex) {}

	IDGControl* addControl(IDGControl* inControl);
	// in-place constructor variant that instantiates the control in addition to adding it
	template <typename T, typename... Args>
	T* emplaceControl(Args&&... args)
	{
		static_assert(std::is_base_of_v<IDGControl, T>);
		static_assert(std::is_base_of_v<VSTGUI::CControl, T>);
		auto const control = new T(std::forward<Args>(args)...);
		addControl(control);
		return control;
	}
	void removeControl(IDGControl* inControl);
	long GetWidth();
	long GetHeight();
	auto GetBackgroundImage() const noexcept
	{
		return mBackgroundImage.get();
	}

	virtual void dfxgui_Idle() {}

#ifdef TARGET_API_AUDIOUNIT
	void HandleStreamFormatChange();
	void HandleParameterListChange();
#endif
	virtual void numAudioChannelsChanged(unsigned long inNumChannels) {}

	void automationgesture_begin(long inParameterID);
	void automationgesture_end(long inParameterID);
#ifdef TARGET_API_AUDIOUNIT
	OSStatus SendAUParameterEvent(AudioUnitParameterID inParameterID, AudioUnitEventType inEventType);
#endif
	virtual void parameterChanged(long inParameterID) {}

	bool IsOpen();
	DGEditorListenerInstance dfxgui_GetEffectInstance();
#if defined(TARGET_API_AUDIOUNIT) && DEBUG
	class DfxPlugin* dfxgui_GetDfxPluginInstance();
#endif
	auto dfxgui_GetEditorOpenErrorCode() const noexcept
	{
		return mEditorOpenErr;
	}

	// get/set the control that is currently under the mouse pointer, if any (returns nullptr if none)
	auto getCurrentControl_mouseover() const noexcept
	{
		return mCurrentControl_mouseover;
	}
	void setCurrentControl_mouseover(IDGControl* inMousedOverControl);
	// override this if you want your GUI to react when the mouseovered control changes
	virtual void mouseovercontrolchanged(IDGControl* currentControlUnderMouse) {}
	// IMouseObserver overrides
	void onMouseEntered(VSTGUI::CView* inView, VSTGUI::CFrame* inFrame) override;
	void onMouseExited(VSTGUI::CView* inView, VSTGUI::CFrame* inFrame) override;
	VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CFrame* inFrame, VSTGUI::CPoint const& inPos, VSTGUI::CButtonState const& inButtons) override;
	VSTGUI::CMouseEventResult onMouseMoved(VSTGUI::CFrame* inFrame, VSTGUI::CPoint const& inPos, VSTGUI::CButtonState const& inButtons) override;
	// ViewMouseListenerAdapter override
	VSTGUI::CMouseEventResult viewOnMouseDown(VSTGUI::CView* inView, VSTGUI::CPoint inPos, VSTGUI::CButtonState inButtons) override;

#ifdef TARGET_API_RTAS
	void GetBackgroundRect(sRect* outRect);
	void SetBackgroundRect(sRect* inRect);
	void GetControlIndexFromPoint(long inXpos, long inYpos, long* outControlIndex);  // Called by CProcess::ChooseControl
	void SetControlHighlight(long inControlIndex, short inIsHighlighted, short inColor);
	void drawControlHighlight(VSTGUI::CDrawContext* inContext, VSTGUI::CControl* inControl);

	// VSTGUI: needed the following so that the algorithm is updated while the mouse is down
	void doIdleStuff() override;
#endif

#if PLUGGUI
	bool isOpen() const noexcept
	{
		return (systemWindow != nullptr);
	}
#endif

	// VST/RTAS abstraction methods
	long GetNumParameters();
	long GetNumAudioOutputs();
	float dfxgui_ExpandParameterValue(long inParameterIndex, float inValue);
	float dfxgui_ContractParameterValue(long inParameterIndex, float inValue);
	float GetParameter_minValue(long inParameterIndex);
	float GetParameter_maxValue(long inParameterIndex);
	float GetParameter_defaultValue(long inParameterIndex);
	DfxParam::ValueType GetParameterValueType(long inParameterIndex);
	DfxParam::Unit GetParameterUnit(long inParameterIndex);

	// the below methods all handle communication between the GUI component and the audio component
	double getparameter_f(long inParameterID);
	long getparameter_i(long inParameterID);
	bool getparameter_b(long inParameterID);
	double getparameter_gen(long inParameterIndex);
	void setparameter_f(long inParameterID, double inValue, bool inWrapWithAutomationGesture = false);
	void setparameter_i(long inParameterID, long inValue, bool inWrapWithAutomationGesture = false);
	void setparameter_b(long inParameterID, bool inValue, bool inWrapWithAutomationGesture = false);
	void setparameter_default(long inParameterID, bool inWrapWithAutomationGesture = false);
	void setparameters_default(bool inWrapWithAutomationGesture = false);
	bool getparametervaluestring(long inParameterID, char* outText);
	std::string getparameterunitstring(long inParameterIndex);
	std::string getparametername(long inParameterID);
	void randomizeparameter(long inParameterID, bool inWriteAutomation);
	void randomizeparameters(bool inWriteAutomation);
	void GenerateParameterAutomationSnapshot(long inParameterID);
	void GenerateParametersAutomationSnapshot();
	virtual std::optional<double> dfxgui_GetParameterValueFromString_f(long inParameterID, std::string const& inText);
	virtual std::optional<long> dfxgui_GetParameterValueFromString_i(long inParameterID, std::string const& inText);
	bool dfxgui_SetParameterValueWithString(long inParameterID, std::string const& inText);
	bool dfxgui_IsValidParamID(long inParameterID);
	void TextEntryForParameterValue(long inParameterID);
#ifdef TARGET_API_AUDIOUNIT
	AudioUnitParameter dfxgui_MakeAudioUnitParameter(AudioUnitParameterID inParameterID, AudioUnitScope inScope = kAudioUnitScope_Global, AudioUnitElement inElement = 0);
	std::vector<AudioUnitParameterID> CreateParameterList(AudioUnitScope inScope = kAudioUnitScope_Global);
#endif
	long dfxgui_GetPropertyInfo(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned long inItemIndex, 
								size_t& outDataSize, dfx::PropertyFlags& outFlags);
	long dfxgui_GetProperty(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned long inItemIndex, 
							void* outData, size_t& ioDataSize);
	template <typename T>
	std::optional<T> dfxgui_GetProperty(dfx::PropertyID inPropertyID, dfx::Scope inScope = dfx::kScope_Global, unsigned long inItemIndex = 0)
	{
		static_assert(std::is_trivially_copyable_v<T>);
		T value {};
		size_t dataSize = sizeof(value);
		auto const status = dfxgui_GetProperty(inPropertyID, inScope, inItemIndex, &value, dataSize);
		return ((status == dfx::kStatus_NoError) && (dataSize == sizeof(value))) ? std::make_optional(value) : std::nullopt;
	}
	long dfxgui_SetProperty(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned long inItemIndex, 
							void const* inData, size_t inDataSize);
	void LoadPresetFile();
	void SavePresetFile();
#if TARGET_PLUGIN_USES_MIDI
	void setmidilearning(bool inLearnMode);
	bool getmidilearning();
	void resetmidilearn();
	void setmidilearner(long inParameterIndex);
	long getmidilearner();
	bool ismidilearner(long inParameterIndex);
	void setparametermidiassignment(long inParameterIndex, dfx::ParameterAssignment const& inAssignment);
	dfx::ParameterAssignment getparametermidiassignment(long inParameterIndex);
	void parametermidiunassign(long inParameterIndex);
	void TextEntryForParameterMidiCC(long inParameterID);
	void HandleMidiLearnChange();
	void HandleMidiLearnerChange();
	virtual void midiLearningChanged(bool inLearnMode) {}
	virtual void midiLearnerChanged(long inParameterIndex) {}
	DGButton* CreateMidiLearnButton(long inXpos, long inYpos, DGImage* inImage, bool inDrawMomentaryState = false);
	DGButton* CreateMidiResetButton(long inXpos, long inYpos, DGImage* inImage);
	auto GetMidiLearnButton() const noexcept
	{
		return mMidiLearnButton;
	}
	auto GetMidiResetButton() const noexcept
	{
		return mMidiResetButton;
	}
#endif
	unsigned long getNumAudioChannels();

protected:
	std::vector<IDGControl*> mControlsList;

private:
	[[nodiscard]] bool handleContextualMenuClick(VSTGUI::CControl* inControl, VSTGUI::CButtonState const& inButtons);
	VSTGUI::COptionMenu createContextualMenu(IDGControl* inControl);
	VSTGUI::SharedPointer<VSTGUI::COptionMenu> createParameterContextualMenu(long inParameterID);
	long initClipboard();
	long copySettings();
	long pasteSettings(bool* inQueryPastabilityOnly);

	void addMousedOverControl(IDGControl* inMousedOverControl);
	void removeMousedOverControl(IDGControl* inMousedOverControl);

	IDGControl* mCurrentControl_mouseover = nullptr;

	std::list<IDGControl*> mMousedOverControlsList;

	VSTGUI::SharedPointer<DGImage> mBackgroundImage;

	bool mJustOpened = false;
	long mEditorOpenErr = dfx::kStatus_NoError;
	unsigned long mNumAudioChannels = 0;

	VSTGUI::SharedPointer<DGTextEntryDialog> mTextEntryDialog;

#if TARGET_PLUGIN_USES_MIDI
	DGButton* mMidiLearnButton = nullptr;
	DGButton* mMidiResetButton = nullptr;
#endif

#if TARGET_OS_MAC
	dfx::UniqueCFType<PasteboardRef> mClipboardRef;
#endif

#ifdef TARGET_API_AUDIOUNIT
	std::vector<AudioUnitParameterID> mAUParameterList;
	std::mutex mAUParameterListLock;
	AudioUnitParameterID mAUMaxParameterID = 0;
	dfx::UniqueOpaqueType<AUEventListenerRef, AUListenerDispose> mAUEventListener;
	AudioUnitEvent mStreamFormatPropertyAUEvent {};
	AudioUnitEvent mParameterListPropertyAUEvent {};
	AudioUnitEvent mMidiLearnPropertyAUEvent {};
	AudioUnitEvent mMidiLearnerPropertyAUEvent {};
	std::vector<AudioUnitEvent> mCustomPropertyAUEvents;
#endif

#ifdef TARGET_API_RTAS
	ITemplateProcess* m_Process = nullptr;
	std::vector<long> mParameterHighlightColors;
#endif
};


#pragma clang diagnostic pop
