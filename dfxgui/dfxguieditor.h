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

#ifdef TARGET_API_AUDIOUNIT
	// these are part of the AUCarbonViewBase interface
	virtual OSStatus CreateUI(Float32 inXOffset, Float32 inYOffset);
	virtual bool HandleEvent(EventRef inEvent);
	virtual ComponentResult Version();

	AUParameterListenerRef getParameterListener()
		{	return mParameterListener;	}
#endif

	void addImage(DGImage * inImage);
	void addControl(DGControl * inCtrl);

	void do_idle();
	virtual void idle() { }

#if TARGET_OS_MAC
	virtual bool HandleMouseEvent(EventRef inEvent);
	virtual bool HandleKeyboardEvent(EventRef inEvent);
	virtual bool HandleCommandEvent(EventRef inEvent);
	virtual bool HandleControlEvent(EventRef inEvent);
	ControlDefSpec * getControlDefSpec()
		{	return &dgControlSpec;	}
#endif

	void automationgesture_begin(long inParameterID);
	void automationgesture_end(long inParameterID);
#ifdef TARGET_API_AUDIOUNIT
	OSStatus SendAUParameterEvent(AudioUnitParameterID inParameterID, AudioUnitEventType inEventType);
#endif

	void DrawBackground(DGGraphicsContext * inContext);

	// get/set the control that is currently under the mouse pointer, if any (returns NULL if none)
	DGControl * getCurrentControl_mouseover()
		{	return currentControl_mouseover;	}
	void setCurrentControl_mouseover(DGControl * inNewMousedOverControl);
	// *** override this if you want your GUI to react when the mouseovered control changes
	virtual void mouseovercontrolchanged(DGControl * currentControlUnderMouse) { }

	DGControl * getCurrentControl_clicked()
		{	return currentControl_clicked;	}

	// the below methods all handle communication between the GUI component and the music component
	DfxPlugin * getdfxplugin()
		{	return dfxplugin;	}
	double getparameter_f(long inParameterID);
	long getparameter_i(long inParameterID);
	bool getparameter_b(long inParameterID);
	void setparameter_f(long inParameterID, double inValue);
	void setparameter_i(long inParameterID, long inValue);
	void setparameter_b(long inParameterID, bool inValue);
	void setparameter_default(long inParameterID);
	void getparametervaluestring(long inParameterID, char * outText);
	void randomizeparameters(bool writeAutomation = false);
	#if TARGET_PLUGIN_USES_MIDI
		void setmidilearning(bool newLearnMode);
		bool getmidilearning();
		void resetmidilearn();
		void setmidilearner(long parameterIndex);
		long getmidilearner();
		bool ismidilearner(long parameterIndex);
	#endif

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

	DfxPlugin * dfxplugin;	// XXX bad thing for AU, maybe just for easy debugging sometimes

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

#if TARGET_OS_MAC
	ControlDefSpec 		dgControlSpec;
	EventHandlerRef		windowEventHandlerRef;

	EventLoopTimerRef	idleTimer;

	ATSFontContainerRef	fontsATSContainer;	// the ATS font container for the fonts in the bundle's Resources directory
	bool		fontsWereActivated;	// memory of whether or not bundled fonts were loaded successfully
#endif
};


long launch_documentation();


#endif
// __DFXGUI_H
