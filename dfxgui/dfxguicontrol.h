#ifndef __DFXGUI_CONTROL_H
#define __DFXGUI_CONTROL_H


#include "dfxguitools.h"
#include "AUCarbonViewControl.h"



class DfxGuiEditor;

/***********************************************************************
	DGControl
	base class for all control objects
***********************************************************************/

//-----------------------------------------------------------------------------
class DGControl : public DGItem
{
public:

	// control for a parameter
	DGControl(DfxGuiEditor *inOwnderEditor, AudioUnitParameterID inParameterID, DGRect *inRegion);
	// control with no actual parameter attached
	DGControl(DfxGuiEditor *inOwnerEditor, DGRect *inRegion, float inRange = 0.0f);
	virtual ~DGControl();

	virtual destroy() { delete this; }

	/* XXX add "settype", "gettype" (used to be in dgitem) */

	// ControlRefs will be implemented by Manager Class
	void setCarbonControl(ControlRef inCarbonControl)
		{	carbonControl = inCarbonControl;	}
	ControlRef getCarbonControl()
		{	return carbonControl;	}

	/* XXX marc says it can go? */
	// called by EventHandler callback; draws a clipping region if opaque == true, 
	// otherwise asks embedded DfxGuiControls for their clipping regions
	void clipRegion(bool drawing);
	void setVisible(bool inVisibility);

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
	bool isAUVPattached()
		{	return AUVPattached;	}

	void redraw();	// force a redraw

	/* XXX probably would go, since we will pass as argument to 
	   dfxguieditor or else not use at all.
	   in any case, set is kind of silly since only the child
	   sets, and it has access to isContinuous */
	bool isContinuousControl()
		{	return isContinuous;	}
	void setContinuousControl(bool inContinuity)
		{	isContinuous = inContinuity;	}

	/* XXX? mac? used? */
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

	void setRedrawTolerance(SInt32 inNewTolerance)
		{	redrawTolerance = inNewTolerance;	}
	bool mustUpdate(void);

 private:
	// common constructor stuff
	void init(DGRect *inRegion);

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
	SInt32				redrawTolerance;
	bool				pleaseUpdate;
};


#ifdef TARGET_API_AUDIOUNIT
//-----------------------------------------------------------------------------
// this gives some slight tweaks to Apple's AUCarbonViewControl class
class DGCarbonViewControl : public AUCarbonViewControl
{
public:
	DGCarbonViewControl(AUCarbonViewBase *ownerView, AUParameterListenerRef listener, ControlType type, const AUVParameter &param, ControlRef control);

	virtual void ControlToParameter();
	virtual void ParameterToControl(Float32 newValue);
};

#endif


#endif
// __DFXGUI_CONTROL_H
