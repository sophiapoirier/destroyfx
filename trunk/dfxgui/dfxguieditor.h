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

	// *** this one is for the child class to override
	virtual OSStatus open(float inXOffset, float inYOffset) = 0;

#ifdef TARGET_API_AUDIOUNIT
	// this gets called from AUCarbonViewBase
	OSStatus CreateUI(Float32 inXOffset, Float32 inYOffset);
	bool HandleEvent(EventRef inEvent);

	AUParameterListenerRef getParameterListener()
		{	return mParameterListener;	}
#endif

	void			addImage(DGImage *inImage);
	void			addControl(DGControl *inCtrl);

	void do_idle();
	virtual void idle() { }

	virtual void DrawBackground(CGContextRef inContext, UInt32 inPortHeight);

#if MAC
	virtual bool HandleMouseEvent(EventRef inEvent);
	virtual bool HandleKeyboardEvent(EventRef inEvent);
	virtual bool HandleCommandEvent(EventRef inEvent);
	virtual bool HandleControlEvent(EventRef inEvent);
#endif

	// get/set the control that is currently under the mouse pointer, if any (returns NULL if none)
	DGControl * getCurrentControl_mouseover()
		{	return currentControl_mouseover;	}
	void setCurrentControl_mouseover(DGControl *inNewMousedOverControl);
	// *** override this if you want your GUI to react when the mouseovered control changes
	virtual void mouseovercontrolchanged() { }

	// the below methods all handle communication between the GUI component and the music component
	DfxPlugin * getdfxplugin()
		{	return dfxplugin;	}
	float getparameter_f(long parameterID);
	double getparameter_d(long parameterID);
	long getparameter_i(long parameterID);
	bool getparameter_b(long parameterID);
	void getparametervaluestring(long parameterID, char *outText);
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
	void SetBackgroundImage(DGImage *inBackgroundImage)
		{	backgroundImage = inBackgroundImage;	}
	void SetBackgroundColor(DGColor inBackgroundColor)
		{	backgroundColor = inBackgroundColor;	}

private:
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

	DGImage *	backgroundImage;
	DGColor		backgroundColor;

	DGControl *	currentControl_clicked;
	DGControl *	currentControl_mouseover;

#if MAC
	DGControl * getDGControlByCarbonControlRef(ControlRef inControl);

	EventHandlerUPP		controlHandlerUPP;
	ControlDefSpec 		dgControlSpec;
	EventHandlerUPP		windowEventHandlerUPP;
	EventHandlerRef		windowEventEventHandlerRef;

	EventLoopTimerUPP	idleTimerUPP;
	EventLoopTimerRef	idleTimer;

	FSSpec		bundleResourceDirFSSpec;	// the FSSpec for the Resources directory in the plugin bundle
	bool		fontsWereActivated;	// memory of whether or not bundled fonts were loaded successfully
#endif

	DfxPlugin * dfxplugin;	// XXX bad thing for AU, maybe just for easy debugging sometimes
};



#endif
// __DFXGUI_H
