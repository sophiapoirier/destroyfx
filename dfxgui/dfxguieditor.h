#ifndef __DFXGUI_H
#define __DFXGUI_H


#include "AUCarbonViewBase.h"

#include "dfxguitools.h"
#include "dfxguicontrol.h"



#if MAC
typedef CarbonControl PlatformControlRef;
#endif

#ifdef TARGET_API_AUDIOUNIT
titedef AudioUnitCarbonView EDIT0RR;
#endif


/***********************************************************************
	DfxGuiEditor
***********************************************************************/

//-----------------------------------------------------------------------------
class DfxGuiEditor : public AUCarbonViewBase
{
public:
	DfxGuiEditor(AudioUnitCarbonView inInstance);
	virtual ~DfxGuiEditor();

#ifdef TARGET_API_AUDIOUNIT
	// this gets called from AUCarbonViewBase
	OSStatus CreateUI(Float32 inXOffset, Float32 inYOffset);
	bool HandleEvent(EventRef inEvent);
#endif
	// *** this one is for the child class to override
	virtual OSStatus open(float inXOffset, float inYOffset) = 0;

	// Images
	void			addImage(DGGraphic *inImage);
	// Controls
	void			addControl(DGControl *inCtrl);
	DGControl *		getDGControlByPlatformControlRef(PlatformControlRef inControl);

	AUParameterListenerRef getParameterListener()
		{	return mParameterListener;	}

#if 0
// XXX bye bye
	// get/set the control that is currently being moused (actively tweaked), if any (returns NULL if none)
	DGControl * getCurrentControl_clicked()
		{	return currentControl_clicked;	}
	void setCurrentControl_clicked(DGControl *inNewClickedControl)
		{	currentControl_clicked = inNewClickedControl;	}
#endif
	// get/set the control that is currently idly under the mouse pointer, if any (returns NULL if none)
	DGControl * getCurrentControl_mouseover()
		{	return currentControl_mouseover;	}
	void setCurrentControl_mouseover(DGControl *inNewMousedOverControl);
	// *** override this if you want your GUI to react when the mouseovered control changes
	virtual void mouseovercontrolchanged()
		{ }

	virtual void DrawBackground(CGContextRef inContext, UInt32 inPortHeight);

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

	void idle();

protected:
	void SetBackgroundImage(DGGraphic *inBackgroundImage)
		{	backgroundImage = inBackgroundImage;	}
	void SetBackgroundColor(DGColor inBackgroundColor)
		{	backgroundColor = inBackgroundColor;	}

private:
	UInt32				itemCount;
	DGGraphic *			Images;
	DGControl *			Controls;

	DGGraphic *			backgroundImage;
	DGColor				backgroundColor;

	DGControl *			currentControl_clicked;
	DGControl *			currentControl_mouseover;

#if MAC
	EventHandlerUPP		controlHandlerUPP;
	ControlDefSpec 		dgControlSpec;
	EventHandlerUPP		windowEventHandlerUPP;
	EventHandlerRef		windowEventEventHandlerRef;

	EventLoopTimerUPP	idleTimerUPP;
	EventLoopTimerRef	idleTimer;

	FSSpec		bundleResourceDirFSSpec;	// the FSSpec for the Resources directory in the plugin bundle
	bool		fontsWereActivated;	// memory of whether or not bundled fonts were loaded successfully
#endif

#ifdef TARGET_API_VST
	DfxPlugin *dfxplugin;	// XXX bad thing
#endif
};



#endif
// __DFXGUI_H