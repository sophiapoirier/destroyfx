#ifndef __DFXGUI_H
#define __DFXGUI_H


#include "AUCarbonViewBase.h"

#include "dfxguitools.h"
#include "dfxguicontrol.h"



/***********************************************************************
	DfxGuiEditor
***********************************************************************/

//-----------------------------------------------------------------------------
class DfxGuiEditor : public AUCarbonViewBase
{
public:
	DfxGuiEditor(AudioUnitCarbonView inInstance);
	virtual ~DfxGuiEditor();

	// this gets called from AUCarbonViewBase
	virtual OSStatus CreateUI(Float32 inXOffset, Float32 inYOffset);
	virtual bool HandleEvent(EventRef inEvent);
	// *** this one is for the child class to override
	virtual OSStatus open(Float32 inXOffset, Float32 inYOffset) = 0;

	UInt32			requestItemID();
	// Images
	void			addImage(DGGraphic *inImage);
	DGGraphic *		getImageByID(UInt32 inID);
	// Controls
	void			addControl(DGControl *inCtrl);
	DGControl *		getControlByID(UInt32 inID);
	DGControl *		getDGControlByCarbonControlRef(ControlRef inControl);
	DGControl *		getControls()
		{	return Controls;	}

	AUParameterListenerRef getParameterListener()
		{	return mParameterListener;	}

	// get/set the control that is currently being moused (actively tweaked), if any (returns NULL if none)
	DGControl * getCurrentControl_clicked()
		{	return currentControl_clicked;	}
	void setCurrentControl_clicked(DGControl *inNewClickedControl)
		{	currentControl_clicked = inNewClickedControl;	}
	// get/set the control that is currently idly under the mouse pointer, if any (returns NULL if none)
	DGControl * getCurrentControl_mouseover()
		{	return currentControl_mouseover;	}
	void setCurrentControl_mouseover(DGControl *inNewMousedOverControl);
	// *** override this if you want your GUI to react when the mouseovered control changes
	virtual void mouseovercontrolchanged()
		{ }

	void SetBackgroundImage(DGGraphic *inBackgroundImage)
		{	backgroundImage = inBackgroundImage;	}
	void SetBackgroundColor(DGColor inBackgroundColor)
		{	backgroundColor = inBackgroundColor;	}
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

	// Relaxed drawing occurs when User adjusts parameters.
	void setRelaxed(bool inRelaxation)
		{	relaxed = inRelaxation;	}
	bool isRelaxed(void)
		{	return relaxed;	}
	
	void idle();

private:
	UInt32				itemCount;
	DGGraphic *			Images;
	DGControl *			Controls;
	bool				relaxed;
	EventLoopTimerUPP	idleTimerUPP;
	EventLoopTimerRef	idleTimer;

	DGGraphic *			backgroundImage;
	DGColor				backgroundColor;

	EventHandlerUPP		controlHandlerUPP;
	ControlDefSpec 		dgControlSpec;
	EventHandlerUPP		windowEventHandlerUPP;
	EventHandlerRef		windowEventEventHandlerRef;
	DGControl *			currentControl_clicked;
	DGControl *			currentControl_mouseover;

	FSSpec		bundleResourceDirFSSpec;	// the FSSpec for the Resources directory in the plugin bundle
	bool		fontsWereActivated;	// memory of whether or not bundled fonts were loaded successfully

	DfxPlugin *dfxplugin;	// XXX bad thing
};



#endif
// __DFXGUI_H