#ifndef __DFXGUI_H
#define __DFXGUI_H


#include "AUCarbonViewBase.h"

#include "dfxguitools.h"
#include "dfxguicontrol.h"



#ifdef TARGET_API_AUDIOUNIT
	typedef AudioUnitCarbonView DGEditorListenerInstance;
#endif
#ifdef TARGET_API_VST
	typedef DfxPlugin * DGEditorListenerInstance;
#endif


/***********************************************************************
	DfxGuiEditor
***********************************************************************/

//-----------------------------------------------------------------------------
class DfxGuiEditor : public AUCarbonViewBase
{
public:
	DfxGuiEditor(DGEditorListenerInstance inInstance);
	virtual ~DfxGuiEditor();

	// *** this one is for the child class of DfxGuiEditor to override
	virtual long open() = 0;
	virtual void post_open() { }

#ifdef TARGET_API_AUDIOUNIT
	// these are part of the AUCarbonViewBase interface
	virtual OSStatus CreateUI(Float32 inXOffset, Float32 inYOffset);
	virtual bool HandleEvent(EventHandlerCallRef inHandlerRef, EventRef inEvent);
	virtual ComponentResult Version();

	AUParameterListenerRef getParameterListener()
		{	return mParameterListener;	}
#endif

	void addImage(DGImage * inImage);
	void addControl(DGControl * inCtrl);

	void do_idle();
	virtual void idle() { }

#ifdef TARGET_API_AUDIOUNIT
	void HandleStreamFormatChange();
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
	void getparametervaluestring(long inParameterID, char * outText);
	void randomizeparameter(long inParameterID, bool inWriteAutomation = false);
	void randomizeparameters(bool inWriteAutomation = false);
	#if TARGET_PLUGIN_USES_MIDI
		void setmidilearning(bool inNewLearnMode);
		bool getmidilearning();
		void resetmidilearn();
		void setmidilearner(long inParameterIndex);
		long getmidilearner();
		bool ismidilearner(long inParameterIndex);
	#endif
	unsigned long getNumAudioChannels();

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

	unsigned long numAudioChannels;

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
#endif
};


long launch_documentation();


#endif
// __DFXGUI_H
