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
class DGControl
{
public:
	// control for a parameter
	DGControl(DfxGuiEditor *inOwnderEditor, AudioUnitParameterID inParameterID, DGRect *inRegion);
	// control with no actual parameter attached
	DGControl(DfxGuiEditor *inOwnerEditor, DGRect *inRegion, float inRange = 0.0f);
	virtual ~DGControl();

	// ControlRefs will be implemented by Manager Class
	void setCarbonControl(ControlRef inCarbonControl)
		{	carbonControl = inCarbonControl;	}
	ControlRef getCarbonControl()
		{	return carbonControl;	}

	// called by EventHandler callback; draws a clipping region
	void clipRegion(bool drawing);	// XXX marc says it can go?
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
	virtual void idle()
		{ }

	// checks if this or an embedded control is inside
	virtual bool isControlRef(ControlRef inControl);

	void redraw();	// force a redraw

	/* XXX probably would go, since we will pass as argument to 
	   dfxguieditor or else not use at all.
	   in any case, set is kind of silly since only the child
	   sets, and it has access to isContinuous */
	bool isContinuousControl()
		{	return isContinuous;	}
	void setContinuousControl(bool inContinuity)
		{	isContinuous = inContinuity;	}

	bool isAUVPattached()
		{	return AUVPattached;	}
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

	void setOffset(SInt32 x, SInt32 y);

	void setBounds(DGRect *r)
		{	where.set(r);	}
	void setForeBounds(SInt32 x, SInt32 y, SInt32 w, SInt32 h);
	void shrinkForeBounds(SInt32 x, SInt32 y, SInt32 w, SInt32 h);

 private:
	// common constructor stuff
	void init(DGRect *inRegion);

protected:
	DfxGuiEditor *		ownerEditor;
	AUVParameter 		auvp;
	float				Range;
	bool				AUVPattached;
	bool				isContinuous;
	AUCarbonViewControl * auv_control;

	ControlRef			carbonControl;	// the physical control, if any
	DGRect				where; 			// the bounds...
	DGRect				vizArea; 		// where the foreground displays
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
