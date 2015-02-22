/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2015  Sophia Poirier

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

#ifndef __DFXGUI_EDITOR_H
#define __DFXGUI_EDITOR_H


#include "dfxgui-base.h"

#include "dfxguimisc.h"
#include "dfxguicontrol.h"

#include "dfxpluginproperties.h"
#include "dfxmisc.h"

#if TARGET_PLUGIN_USES_MIDI
	#include "dfxsettings.h"	// for DfxParameterAssignment
#endif

#include "vstgui.h"
#if (VSTGUI_VERSION_MAJOR < 4)
	#include "ctooltipsupport.h"
	#define CButtonState	long
	#define VSTGUI_OVERRIDE_VMETHOD
#endif

#ifdef TARGET_API_VST
	#include "dfxplugin.h"
	typedef DfxPlugin *	DGEditorListenerInstance;
#endif
#ifdef TARGET_API_RTAS
	#include "dfxplugin-base.h"
	typedef ITemplateProcess * DGEditorListenerInstance;
#endif
#ifdef TARGET_API_AUDIOUNIT
	#include "AUCarbonViewBase.h"
	typedef AudioUnit	DGEditorListenerInstance;
	typedef AUCarbonViewBase	TARGET_API_EDITOR_BASE_CLASS;
#endif

#ifdef TARGET_API_VST
	#include "aeffguieditor.h"
	#define TARGET_API_EDITOR_BASE_CLASS	AEffGUIEditor
	#define TARGET_API_EDITOR_INDEX_TYPE	VstInt32
#else
	#include "plugguieditor.h"
	#define TARGET_API_EDITOR_BASE_CLASS	PluginGUIEditor
	#if (VSTGUI_VERSION_MAJOR < 4)
		#define TARGET_API_EDITOR_INDEX_TYPE	long
	#else
		#define TARGET_API_EDITOR_INDEX_TYPE	int32_t
	#endif
#endif



//-----------------------------------------------------------------------------
#if defined(TARGET_API_AUDIOUNIT) || defined(TARGET_API_RTAS)
	#define DFX_EDITOR_ENTRY(PluginEditorClass)												\
		DfxGuiEditor * DFXGUI_NewEditorInstance(DGEditorListenerInstance inProcessInstance)	\
		{																					\
			return new PluginEditorClass(inProcessInstance);								\
		}
#endif

#ifdef TARGET_API_VST
	#define DFX_EDITOR_ENTRY(PluginEditorClass)												\
		AEffEditor * DFXGUI_NewEditorInstance(DGEditorListenerInstance inEffectInstance)	\
		{																					\
			return new PluginEditorClass(inEffectInstance);									\
		}
#endif



//-----------------------------------------------------------------------------
#ifdef TARGET_API_AUDIOUNIT
	const Float32 kDfxGui_NotificationInterval = 42.0 * kEventDurationMillisecond;	// 24 fps
	const EventTimerInterval kDfxGui_IdleTimerInterval = kDfxGui_NotificationInterval;
#endif

enum {
#ifdef TARGET_API_RTAS
	kKeyModifier_DefaultValue = kAlt,
	kKeyModifier_FineControl = kControl
#else
	kKeyModifier_DefaultValue = kControl,
	kKeyModifier_FineControl = kShift
#endif
};






//-----------------------------------------------------------------------------
class DGButton;


/***********************************************************************
	DfxGuiEditor
***********************************************************************/

//-----------------------------------------------------------------------------
class DfxGuiEditor : public TARGET_API_EDITOR_BASE_CLASS, public CControlListener, public IMouseObserver, public CBaseObject
{
public:
	DfxGuiEditor(DGEditorListenerInstance inInstance);
	virtual ~DfxGuiEditor();

	// VSTGUI overrides
	virtual bool open(void * inWindow) VSTGUI_OVERRIDE_VMETHOD;
	virtual void close() VSTGUI_OVERRIDE_VMETHOD;
	virtual void setParameter(TARGET_API_EDITOR_INDEX_TYPE inParameterIndex, float inValue) VSTGUI_OVERRIDE_VMETHOD;
	virtual void valueChanged(CControl * inControl) VSTGUI_OVERRIDE_VMETHOD;
	virtual int32_t controlModifierClicked(CControl* inControl, CButtonState inButtons) VSTGUI_OVERRIDE_VMETHOD;
#ifndef TARGET_API_VST
	virtual void beginEdit(TARGET_API_EDITOR_INDEX_TYPE inParameterIndex) VSTGUI_OVERRIDE_VMETHOD;
	virtual void endEdit(TARGET_API_EDITOR_INDEX_TYPE inParameterIndex) VSTGUI_OVERRIDE_VMETHOD;
#endif
	virtual void idle() VSTGUI_OVERRIDE_VMETHOD;
	// CBaseObject
	virtual CMessageResult notify(CBaseObject* inSender, IdStringPtr inMessage) VSTGUI_OVERRIDE_VMETHOD;

	// *** this one is for the child class of DfxGuiEditor to override
	virtual long OpenEditor() = 0;
	virtual void CloseEditor() { }
	virtual void post_open() { }
	virtual void dfxgui_EditorShown() { }

#ifdef TARGET_API_AUDIOUNIT
	long dfxgui_GetParameterInfo(AudioUnitParameterID inParameterID, AudioUnitParameterInfo & outParameterInfo);
	AUEventListenerRef getAUEventListener()
		{	return auEventListener;	}
	virtual void HandleAUPropertyChange(void * inObject, AudioUnitProperty inAUProperty, UInt64 inEventHostTime)
		{ }
#if !__LP64__
	void SetOwnerAUCarbonView(AUCarbonViewBase * inAUCarbonView)
		{	mOwnerAUCarbonView = inAUCarbonView;	}
	AUCarbonViewBase * GetOwnerAUCarbonView()
		{	return mOwnerAUCarbonView;	}
#endif
#endif

	void addImage(DGImage * inImage);
	void addControl(DGControl * inCtrl);
	void removeControl(DGControl * inControl);
	DGControl * getNextControlFromParameterID(long inParameterID, DGControl * inPreviousControl = NULL);
	long GetWidth();
	long GetHeight();
	DGImage * GetBackgroundImage()
		{	return backgroundImage;	}

	void do_idle();
	virtual void dfxgui_Idle() { }

#ifdef TARGET_API_AUDIOUNIT
	void HandleStreamFormatChange();
	void HandleParameterListChange();
	#if TARGET_PLUGIN_USES_MIDI
	void HandleMidiLearnChange();
	#endif
#endif
	virtual void numAudioChannelsChanged(unsigned long inNewNumChannels)
		{ }

#if TARGET_OS_MAC
#if defined(TARGET_API_AUDIOUNIT) && 0
	virtual bool HandleMouseEvent(EventRef inEvent);
	virtual bool HandleKeyboardEvent(EventRef inEvent);
	virtual bool HandleCommandEvent(EventRef inEvent);
	virtual bool HandleControlEvent(EventRef inEvent);
#endif

	CFStringRef openTextEntryWindow(CFStringRef inInitialText = NULL);
	bool handleTextEntryCommand(UInt32 inCommandID);
#endif

	void automationgesture_begin(long inParameterID);
	void automationgesture_end(long inParameterID);
#ifdef TARGET_API_AUDIOUNIT
	OSStatus SendAUParameterEvent(AudioUnitParameterID inParameterID, AudioUnitEventType inEventType);
#endif
	virtual void parameterChanged(long inParameterID)
		{ }

	bool IsOpen();
	DGEditorListenerInstance dfxgui_GetEffectInstance();
	long dfxgui_GetEditorOpenErrorCode()
		{	return mEditorOpenErr;	}

	// get/set the control that is currently under the mouse pointer, if any (returns NULL if none)
	DGControl * getCurrentControl_mouseover()
		{	return currentControl_mouseover;	}
	void setCurrentControl_mouseover(DGControl * inNewMousedOverControl);
	// *** override this if you want your GUI to react when the mouseovered control changes
	virtual void mouseovercontrolchanged(DGControl * currentControlUnderMouse) { }
	// IMouseObserver overrides
	virtual void onMouseEntered(CView* inView, CFrame* inFrame) VSTGUI_OVERRIDE_VMETHOD;
	virtual void onMouseExited(CView* inView, CFrame* inFrame) VSTGUI_OVERRIDE_VMETHOD;
	virtual CMouseEventResult onMouseDown(CFrame* inFrame, const CPoint& inPos, const CButtonState& inButtons) VSTGUI_OVERRIDE_VMETHOD;
	virtual CMouseEventResult onMouseMoved(CFrame* inFrame, const CPoint& inPos, const CButtonState& inButtons) VSTGUI_OVERRIDE_VMETHOD;

	DGControl * getCurrentControl_clicked()
		{	return currentControl_clicked;	}

#ifdef TARGET_API_RTAS
	void GetBackgroundRect(sRect * outRect);
	void SetBackgroundRect(sRect * inRect);
	void GetControlIndexFromPoint(long inXpos, long inYpos, long * outControlIndex);	// Called by CProcess::ChooseControl
	void SetControlHighlight(long inControlIndex, short inIsHighlighted, short inColor);
	void drawControlHighlight(CDrawContext * inContext, CControl * inControl);

	// VSTGUI: needed the following so that the algorithm is updated while the mouse is down
	virtual void doIdleStuff();
#endif

#if PLUGGUI
	bool isOpen()
		{	return (systemWindow != NULL);	}
#endif

	// VST/RTAS abstraction methods
	long GetNumParameters();
	long GetNumAudioOutputs();
	float dfxgui_ExpandParameterValue(long inParameterIndex, float inValue);
	float dfxgui_ContractParameterValue(long inParameterIndex, float inValue);
	float GetParameter_minValue(long inParameterIndex);
	float GetParameter_maxValue(long inParameterIndex);
	float GetParameter_defaultValue(long inParameterIndex);
	DfxParamValueType GetParameterValueType(long inParameterIndex);
	DfxParamUnit GetParameterUnit(long inParameterIndex);

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
	bool getparametervaluestring(long inParameterID, char * outText);
	char * getparameterunitstring(long inParameterIndex);
	char * getparametername(long inParameterID);
	void randomizeparameter(long inParameterID, bool inWriteAutomation = false);
	void randomizeparameters(bool inWriteAutomation = false);
	bool dfxgui_IsValidParamID(long inParameterID);
#ifdef TARGET_API_AUDIOUNIT
	AudioUnitParameter dfxgui_MakeAudioUnitParameter(AudioUnitParameterID inParameterID, AudioUnitScope inScope = kAudioUnitScope_Global, AudioUnitElement inElement = 0);
	AudioUnitParameterID * CreateParameterList(AudioUnitScope inScope, UInt32 * outNumParameters);
#endif
	long dfxgui_GetPropertyInfo(DfxPropertyID inPropertyID, DfxScope inScope, unsigned long inItemIndex, 
								size_t & outDataSize, DfxPropertyFlags & outFlags);
	long dfxgui_GetProperty(DfxPropertyID inPropertyID, DfxScope inScope, unsigned long inItemIndex, 
							void * outData, size_t & ioDataSize);
	long dfxgui_SetProperty(DfxPropertyID inPropertyID, DfxScope inScope, unsigned long inItemIndex, 
							const void * inData, size_t inDataSize);
	#if TARGET_PLUGIN_USES_MIDI
		void setmidilearning(bool inNewLearnMode);
		bool getmidilearning();
		void resetmidilearn();
		void setmidilearner(long inParameterIndex);
		long getmidilearner();
		bool ismidilearner(long inParameterIndex);
		void setparametermidiassignment(long inParameterIndex, DfxParameterAssignment inAssignment);
		DfxParameterAssignment getparametermidiassignment(long inParameterIndex);
		void parametermidiunassign(long inParameterIndex);
		DGButton* CreateMidiLearnButton(long inXpos, long inYpos, DGImage* inImage, bool inDrawMomentaryState = false);
		DGButton* CreateMidiResetButton(long inXpos, long inYpos, DGImage* inImage);
		DGButton* GetMidiLearnButton() const
			{	return midiLearnButton;	}
		DGButton* GetMidiResetButton() const
			{	return midiResetButton;	}
	#endif
	unsigned long getNumAudioChannels();

	long copySettings();
	long pasteSettings(bool * inQueryPastabilityOnly = NULL);

protected:
	void SetBackgroundImage(DGImage * inBackgroundImage);

	class DGControlsList
	{
	public:
		DGControl * control;
		DGControlsList * next;

		DGControlsList(DGControl * inControl, DGControlsList * inNextList)
			: control(inControl), next(inNextList) {}
	};
	DGControlsList * controlsList;

	class DGImagesList
	{
	public:
		OwningPointer<DGImage> image;
		DGImagesList * next;

		DGImagesList(DGImage * inImage, DGImagesList * inNextList)
			: image(inImage), next(inNextList) {}
	};
	DGImagesList * imagesList;

	bool handleContextualMenuClick(CControl* inControl, CButtonState const& inButtons);
	long initClipboard();

private:
	DGControl *	currentControl_clicked;
	DGControl *	currentControl_mouseover;

	class DGMousedOverControlsList
	{
	public:
		DGMousedOverControlsList(DGControl * inControl, DGMousedOverControlsList * inNextList)
			: control(inControl), next(inNextList) {}
		DGControl * control;
		DGMousedOverControlsList * next;
	};
	DGMousedOverControlsList * mousedOverControlsList;
	void addMousedOverControl(DGControl * inMousedOverControl);
	void removeMousedOverControl(DGControl * inMousedOverControl);

	DGImage * backgroundImage;

	bool mJustOpened;
	long mEditorOpenErr;
	unsigned long numAudioChannels;

	CTooltipSupport * mTooltipSupport;

	#if TARGET_PLUGIN_USES_MIDI
		DGButton * midiLearnButton;
		DGButton * midiResetButton;
	#endif

#if TARGET_OS_MAC
	ATSFontContainerRef	fontsATSContainer;	// the ATS font container for the fonts in the bundle's Resources directory
	bool		fontsWereActivated;	// memory of whether or not bundled fonts were loaded successfully

	PasteboardRef	clipboardRef;

#if !__LP64__
	WindowRef	textEntryWindow;
	ControlRef	textEntryControl;
	CFStringRef	textEntryResultString;
#endif
#endif

#ifdef TARGET_API_AUDIOUNIT
#if !__LP64__
	AUCarbonViewBase * mOwnerAUCarbonView;
#endif
	AudioUnitParameterID * auParameterList;
	UInt32 auParameterListSize;
	AudioUnitParameterID auMaxParameterID;
	AUEventListenerRef auEventListener;
	AudioUnitEvent streamFormatPropertyAUEvent;
	AudioUnitEvent parameterListPropertyAUEvent;
	AudioUnitEvent midiLearnPropertyAUEvent;
#endif

#ifdef TARGET_API_RTAS
	ITemplateProcess * m_Process;
	long * parameterHighlightColors;
#endif
};



#endif
