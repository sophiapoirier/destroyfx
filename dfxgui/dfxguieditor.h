/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2009  Sophia Poirier

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

#if TARGET_PLUGIN_USES_MIDI
	#include "dfxsettings.h"	// for DfxParameterAssignment
#endif

#ifdef TARGET_API_AUDIOUNIT
	#include "AUCarbonViewBase.h"
	typedef AudioUnitCarbonView	DGEditorListenerInstance;
	typedef AUCarbonViewBase	TARGET_API_EDITOR_BASE_CLASS;
	#define DFX_EDITOR_ENTRY	COMPONENT_ENTRY
#endif



class DGButton;
class DGSplashScreen;


/***********************************************************************
	DfxGuiEditor
***********************************************************************/

//-----------------------------------------------------------------------------
class DfxGuiEditor : public TARGET_API_EDITOR_BASE_CLASS
{
public:
	DfxGuiEditor(DGEditorListenerInstance inInstance);
	virtual ~DfxGuiEditor();

	// *** this one is for the child class of DfxGuiEditor to override
	virtual long OpenEditor() = 0;
	virtual void CloseEditor() { }
	virtual void post_open() { }
	virtual void dfxgui_EditorShown() { }

#ifdef TARGET_API_AUDIOUNIT
	// these are part of the AUCarbonViewBase interface
	virtual OSStatus CreateUI(Float32 inXOffset, Float32 inYOffset);
	virtual bool HandleEvent(EventHandlerCallRef inHandlerRef, EventRef inEvent);
	virtual ComponentResult Version();

	AUParameterListenerRef getParameterListener()
		{	return mParameterListener;	}
	AUEventListenerRef getAUEventListener()
		{	return auEventListener;	}
	virtual void HandleAUPropertyChange(void * inObject, AudioUnitProperty inAUProperty, UInt64 inEventHostTime)
		{ }
#endif

	void addImage(DGImage * inImage);
	void addControl(DGControl * inCtrl);
	void removeControl(DGControl * inControl);

	void do_idle();
	virtual void dfxgui_Idle() { }

#ifdef TARGET_API_AUDIOUNIT
	void HandleStreamFormatChange();
	void HandleMidiLearnChange();
#endif
	virtual void numAudioChannelsChanged(unsigned long inNewNumChannels)
		{ }

#if TARGET_OS_MAC
	virtual bool HandleMouseEvent(EventRef inEvent);
	virtual bool HandleKeyboardEvent(EventRef inEvent);
	virtual bool HandleCommandEvent(EventRef inEvent);
	virtual bool HandleControlEvent(EventRef inEvent);
	ControlDefSpec * getControlDefSpec()
		{	return &dgControlSpec;	}

	CFStringRef openTextEntryWindow(CFStringRef inInitialText = NULL);
	bool handleTextEntryCommand(UInt32 inCommandID);
	OSStatus openWindowTransparencyWindow();
	void heedWindowTransparencyWindowClose();
#endif

	void automationgesture_begin(long inParameterID);
	void automationgesture_end(long inParameterID);
#ifdef TARGET_API_AUDIOUNIT
	OSStatus SendAUParameterEvent(AudioUnitParameterID inParameterID, AudioUnitEventType inEventType);
#endif

	bool IsOpen()
		{	return mIsOpen;	}

	void DrawBackground(DGGraphicsContext * inContext);
	float getWindowTransparency();
	void setWindowTransparency(float inTransparencyLevel);

	// get/set the control that is currently under the mouse pointer, if any (returns NULL if none)
	DGControl * getCurrentControl_mouseover()
		{	return currentControl_mouseover;	}
	void setCurrentControl_mouseover(DGControl * inNewMousedOverControl);
	// *** override this if you want your GUI to react when the mouseovered control changes
	virtual void mouseovercontrolchanged(DGControl * currentControlUnderMouse) { }

	DGControl * getCurrentControl_clicked()
		{	return currentControl_clicked;	}

	// the below methods all handle communication between the GUI component and the music component
	double getparameter_f(long inParameterID);
	long getparameter_i(long inParameterID);
	bool getparameter_b(long inParameterID);
	void setparameter_f(long inParameterID, double inValue, bool inWrapWithAutomationGesture = false);
	void setparameter_i(long inParameterID, long inValue, bool inWrapWithAutomationGesture = false);
	void setparameter_b(long inParameterID, bool inValue, bool inWrapWithAutomationGesture = false);
	void setparameter_default(long inParameterID, bool inWrapWithAutomationGesture = false);
	void setparameters_default(bool inWrapWithAutomationGesture = false);
	void getparametervaluestring(long inParameterID, char * outText);
	void randomizeparameter(long inParameterID, bool inWriteAutomation = false);
	void randomizeparameters(bool inWriteAutomation = false);
	AudioUnitParameterID * CreateParameterList(AudioUnitScope inScope, UInt32 * outNumParameters);
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
		DGButton * CreateMidiLearnButton(long inXpos, long inYpos, DGImage * inImage, bool inDrawMomentaryState = false);
		DGButton * CreateMidiResetButton(long inXpos, long inYpos, DGImage * inImage);
	#endif
	unsigned long getNumAudioChannels();
	void installSplashScreenControl(DGSplashScreen * inControl);
	void removeSplashScreenControl();

	long copySettings();
	long pasteSettings(bool * inQueryPastabilityOnly = NULL);

protected:
	void SetBackgroundImage(DGImage * inBackgroundImage);
	void SetBackgroundColor(DGColor inBackgroundColor);
	DGBackgroundControl * getBackgroundControl()
		{	return backgroundControl;	}

	class DGControlsList
	{
	public:
		DGControl * control;
		DGControlsList * next;

		DGControlsList(DGControl * inControl, DGControlsList * inNextList)
			: control(inControl), next(inNextList) {}
		~DGControlsList()
		{
			if (control != NULL)
				delete control;
		}
	};
	DGControlsList * controlsList;

	class DGImagesList
	{
	public:
		DGImage * image;
		DGImagesList * next;

		DGImagesList(DGImage * inImage, DGImagesList * inNextList)
			: image(inImage), next(inNextList) {}
		~DGImagesList()
		{
			if (image != NULL)
				delete image;
		}
	};
	DGImagesList * imagesList;

#if TARGET_OS_MAC
	DGControl * getDGControlByCarbonControlRef(ControlRef inControl);
#endif

	long initClipboard();

private:
	void embedAllControlsInReverseOrder(DGControlsList * inControlsList);

	DGBackgroundControl * backgroundControl;

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

	bool mIsOpen;
	bool mJustOpened;
	unsigned long numAudioChannels;

	DGSplashScreen * splashScreenControl;
	#if TARGET_PLUGIN_USES_MIDI
		DGButton * midiLearnButton;
		DGButton * midiResetButton;
	#endif

#if TARGET_OS_MAC
	ControlDefSpec 		dgControlSpec;
	EventHandlerRef		windowEventHandlerRef;

	EventLoopTimerRef	idleTimer;

	ATSFontContainerRef	fontsATSContainer;	// the ATS font container for the fonts in the bundle's Resources directory
	bool		fontsWereActivated;	// memory of whether or not bundled fonts were loaded successfully

	PasteboardRef	clipboardRef;

	WindowRef	textEntryWindow;
	ControlRef	textEntryControl;
	CFStringRef	textEntryResultString;
	WindowRef	windowTransparencyWindow;
#endif

#ifdef TARGET_API_AUDIOUNIT
	AUEventListenerRef auEventListener;
	AudioUnitEvent streamFormatPropertyAUEvent;
	AudioUnitEvent midiLearnPropertyAUEvent;
#endif
};


#endif
