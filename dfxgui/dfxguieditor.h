#ifndef __DFXGUI_H
#define __DFXGUI_H


#include "AUCarbonViewBase.h"

#include "dfxguitools.h"



typedef struct DGColor {
	int r;
	int g;
	int b;
};

const DGColor kBlackDGColor = { 0, 0, 0 };
const DGColor kWhiteDGColor = { 255, 255, 255 };


class DGControl;


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

	UInt32 X;	// X coordinate offset for view
	UInt32 Y;	// Y coordinate offset for view

private:
	UInt32				itemCount;
	DGGraphic *			Images;
	DGControl *			Controls;
	bool				relaxed;
	EventLoopTimerUPP	idleTimerUPP;
	EventLoopTimerRef	idleTimer;

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



/***********************************************************************
	DGControl
	base class for all control objects
***********************************************************************/

//-----------------------------------------------------------------------------
class DGControl : public DGItem
{
public:
	// control for a parameter
	DGControl(DfxGuiEditor *inOwnderEditor, AudioUnitParameterID inParameterID, DGRect *inPos);
	// control with no actual parameter attached
	DGControl(DfxGuiEditor *inOwnderEditor, DGRect *inPos, float Range = 0.0f);
	virtual ~DGControl();

	// common constructor stuff
	void init(DGRect *inWhere);

	// ControlRefs will be implemented by Manager Class
	void setCarbonControl(ControlRef inCarbonControl)
		{	carbonControl = inCarbonControl;	}
	ControlRef getCarbonControl()
		{	return carbonControl;	}

	// called by EventHandler callback; draws a clipping region if opaque == true, 
	// otherwise asks embedded DfxGuiControls for their clipping regions
	void clipRegion(bool drawing);
	void setVisible(bool viz);

	// The methods you should implement in derived controls
	virtual void draw(CGContextRef context, UInt32 portHeight)
		{ }
	// *** mouse position is relative to controlBounds for ultra convenience
	virtual void mouseDown(Point inPos, bool, bool)
		{ }
	virtual void mouseTrack(Point inPos, bool, bool)
		{ }
	virtual void mouseUp(Point inPos, bool, bool)
		{ }

// XXX fix this up
//	virtual void do_idle();
	// *** idle timer function
	virtual void idle();
//		{ }

	// checks if this or an embedded control is inside
	virtual bool isControlRef(ControlRef inControl);

	// if you do not want parameter attached controls
	virtual bool providesForeignControls()
		{	return false;	}
	virtual void initForeignControls(ControlDefSpec *inControlSpec)
		{ }
	bool isAUVPattached()
		{	return AUVPattached;	}

	void redraw();	// force a redraw

	bool isContinuousControl()
		{	return isContinuous;	}
	void setContinuousControl(bool inContinuity)
		{	isContinuous = inContinuity;	}

	// for the clipping region
	bool isOpaque()
		{	return opaque;	}
	void setOpaque(bool inNewOpaque)
		{	opaque = inNewOpaque;	}

	// accessor functions to be called by derived controls
	AUVParameter & getAUVP()
		{	return auvp;	}
	void createAUVcontrol();
	AUCarbonViewControl * getAUVcontrol()
		{	return auv_control;	}
	long getParameterID();
	void setParameterID(AudioUnitParameterID inParameterID);
	float getRange()
		{	return Range;	}
	DGRect * getBounds()
		{	return &where;	}
	DGRect * getForeBounds()
		{	return &vizArea;	}
	DfxGuiEditor * getDfxGuiEditor()
		{	return ownerEditor;	}
	DGControl * getDaddy()
		{	return Daddy;	}
	virtual DGControl * getChild(ControlRef inControl);

	void setDaddy(DGControl *inDaddy)
		{	Daddy = inDaddy;	}
	void setOffset(SInt32 x, SInt32 y);

	void setBounds(DGRect *r)
		{	where.set(r);	}
	void setForeBounds(SInt32 x, SInt32 y, SInt32 w, SInt32 h);
	void shrinkForeBounds(SInt32 x, SInt32 y, SInt32 w, SInt32 h);

	void setTolerance(SInt32 inNewTolerance)
		{	tolerance = inNewTolerance;	}
	bool mustUpdate(void);

protected:
	DfxGuiEditor *		ownerEditor;
	AUVParameter 		auvp;
	DGControl *			Daddy;
	DGControl *			children;
	UInt32				id;
	float				Range;
	bool				AUVPattached;
	bool				isContinuous;
	AUCarbonViewControl * auv_control;

	bool				opaque;			// is it a drawing control or just a virtual region?
	ControlRef			carbonControl;	// the physical control, if any
	DGRect				where; 			// the bounds...
	DGRect				vizArea; 		// where the foreground displays
	SInt32				lastUpdatedValue;
	SInt32				tolerance;
	bool				becameVisible;	// mustUpdate after setVisible(true)
	bool				pleaseUpdate;
};


#endif
// __DFXGUI_H