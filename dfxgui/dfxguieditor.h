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
	OSStatus CreateUI(Float32 inXOffset, Float32 inYOffset);
	// this one is for the child class to override
	virtual OSStatus open(Float32 inXOffset, Float32 inYOffset)
		{	return noErr;	}

	UInt32				requestItemID();
	// Images
	void				addImage(DGGraphic *inImage);
	DGGraphic *			getImageByID(UInt32 inID);
	// Controls
	void				addControl(DGControl *inCtrl);
	DGControl *			getControlByID(UInt32 inID);
	DGControl *			getControlByControlRef(ControlRef inControl);
	DGControl *			getControls(){ return Controls; }
	AUParameterListenerRef	getParameterListener()
		{	return mParameterListener;	}

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
	void					setRelaxed(bool r) { relaxed = r; }
	bool					isRelaxed(void) { return relaxed; }
	
	void					idle();

	UInt32					X;	// X coordinate offset for view
	UInt32					Y;	// Y coordinate offset for view

protected:
	DfxPlugin *dfxplugin;

private:
	UInt32					itemCount;
	DGGraphic *				Images;
	DGControl *				Controls;
	ControlDefSpec 			dgControlSpec;
	bool					relaxed;
	EventLoopTimerRef		timer;

	FSSpec		bundleResourceDirFSSpec;	// the FSSpec for the Resources directory in the plugin bundle
	bool		fontsWereActivated;	// memory of whether or not bundled fonts were loaded successfully
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
	DGControl(DfxGuiEditor*, AudioUnitParameterID, DGRect*);
	// control with no actual parameter attached
	DGControl(DfxGuiEditor*, DGRect*, float Range = 0.0f);
	virtual ~DGControl();

	void init(DGRect *inWhere);

	// ControlRefs will be implemented by Manager Class
	virtual void		setCarbonControl(ControlRef	inCarbonControl) {carbonControl = inCarbonControl;}
	virtual ControlRef	getCarbonControl() { return carbonControl; }

	// Called by EventHandler Callback, draws a clipping region if opaque == true
	// otherwise asks embedded DfxGuiControls for their clipping regions
	virtual void	clipRegion(bool drawing);
	virtual void	setVisible(bool viz);

	// The methods you should implement in derived controls
	virtual void	draw(CGContextRef context, UInt32 portHeight) {}
	virtual void	mouseDown(Point *P, bool, bool) {}	// P is relative to controlBounds, so no hassle
	virtual void 	mouseTrack(Point *P, bool, bool) {}
	virtual void 	mouseUp(Point *P, bool, bool) {}

	// Timer driven function.
	// If you derive this, always call this superclass DGControl::idle()
	virtual void	idle();

	// checks if this or an embedded control is inside
	virtual bool	isControlRef(ControlRef inControl);

	// if you do not want parameter attached controls
	virtual bool	providesForeignControls() { return false; }
	virtual void	initForeignControls(ControlDefSpec *inControlSpec) {}
	virtual bool	isAUVPattached() { return AUVPattached; }

	void redraw();	// force a redraw

	bool isContinuousControl()
		{	return isContinuous;	}
	void setContinuousControl(bool continuity)
		{	isContinuous = continuity;	}

	// for the clipping region
	bool	getOpaque() { return opaque; }
	void	setOpaque(bool o) { opaque = o; }

	// accessor functions to be called by derived controls
	virtual AUVParameter& 		getAUVP() { return auvp; }
	void createAUVcontrol();
	AUCarbonViewControl *		getAUVcontrol()
		{	return auv_control;	}
	void setParameterID(AudioUnitParameterID inParameterID);
	virtual float				getRange() { return Range; }
	virtual DGRect *			getBounds() { return &where; }
	virtual DGRect *			getForeBounds() { return &vizArea; }
	virtual DfxGuiEditor *		getDfxGuiEditor() { return ownerEditor; }
	virtual DGControl *			getDaddy() { return Daddy; }
	virtual DGControl *			getChild(ControlRef inControl);

	virtual void				setDaddy(DGControl *inDaddy) { this->Daddy = inDaddy; }
	virtual void				setOffset(SInt32 x, SInt32 y);
	
	virtual void				setBounds(DGRect* r) { where.set (r); }
	virtual void				setForeBounds(DGRect* r) { vizArea.set (r); }
	virtual void				setForeBounds(SInt32 x, SInt32 y, SInt32 w, SInt32 h);
	virtual void				shrinkForeBounds(SInt32 x, SInt32 y, SInt32 w, SInt32 h);
	
	virtual void				setLastUpdated(SInt32 v) { lastUpdatedValue = v; }
	virtual SInt32				getLastUpdated(void) { return lastUpdatedValue; }
	virtual void				setTolerance(SInt32 t) { tolerance = t; }
	virtual bool				mustUpdate(void);

protected:
	DfxGuiEditor *		ownerEditor;
	AUVParameter 		auvp;
	DGControl *			Daddy;
	DGControl *			children;
	UInt32				id;
	float				Range;
	bool				AUVPattached;
	bool isContinuous;
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