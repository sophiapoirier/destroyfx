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
	DGControl(DfxGuiEditor * inOwnderEditor, long inParameterID, DGRect * inRegion);
	// control with no actual parameter attached
	DGControl(DfxGuiEditor * inOwnerEditor, DGRect * inRegion, float inRange);
	virtual ~DGControl();

#if MAC
	// ControlRefs will be implemented by Manager Class
	void setCarbonControl(ControlRef inCarbonControl)
		{	carbonControl = inCarbonControl;	}
	ControlRef getCarbonControl()
		{	return carbonControl;	}
	void initCarbonControlValueRange();
	// checks if this or an embedded control is inside
	bool isControlRef(ControlRef inControl);
#endif

	// The methods you should implement in derived controls
	virtual void draw(CGContextRef context, long portHeight)
		{ }
	// *** mouse position is relative to controlBounds for ultra convenience
	virtual void mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, unsigned long inKeyModifiers)
		{ }
	virtual void mouseTrack(float inXpos, float inYpos, unsigned long inMouseButtons, unsigned long inKeyModifiers)
		{ }
	virtual void mouseUp(float inXpos, float inYpos, unsigned long inKeyModifiers)
		{ }

	// *** this will get called regularly by an idle timer
	virtual void idle()
		{ }

	void embed();

	void setVisible(bool inVisibility);

	// force a redraw of the control
	void redraw();

	bool isContinuousControl()
		{	return isContinuous;	}
	void setControlContinuous(bool inContinuity);

	void setFineTuneFactor(float inFineTuneFactor)
	{
		if (inFineTuneFactor != 0.0f)	// to prevent division by zero
			fineTuneFactor = inFineTuneFactor;
	}

	bool isParameterAttached()
		{	return parameterAttached;	}
	long getParameterID();
	void setParameterID(long inParameterID);
#ifdef TARGET_API_AUDIOUNIT
	AUVParameter & getAUVP()
		{	return auvp;	}
	void createAUVcontrol();
	AUCarbonViewControl * getAUVcontrol()
		{	return auv_control;	}
#endif
	float getRange()
		{	return Range;	}
	DGRect * getBounds()
		{	return &where;	}
	DGRect * getForeBounds()
		{	return &vizArea;	}
	DfxGuiEditor * getDfxGuiEditor()
		{	return ownerEditor;	}

	void setOffset(long x, long y);

	void setBounds(DGRect * r)
		{	where.set(r);	}
	void setForeBounds(long x, long y, long w, long h);
	void shrinkForeBounds(long x, long y, long w, long h);

 private:
	// common constructor stuff
	void init(DGRect * inRegion);

protected:
	DfxGuiEditor *		ownerEditor;
	float				Range;
	bool				parameterAttached;
	bool				isContinuous;
	float				fineTuneFactor;	// slow-down factor for fine-tune control (if the control supports that)

	DGRect				where;		// the control's area
	DGRect				vizArea; 	// where the foreground displays

#ifdef TARGET_API_AUDIOUNIT
	AUVParameter 		auvp;
	AUCarbonViewControl * auv_control;
#endif
#if MAC
	ControlRef			carbonControl;
#endif
};


#ifdef TARGET_API_AUDIOUNIT
//-----------------------------------------------------------------------------
// this gives some slight tweaks to Apple's AUCarbonViewControl class
class DGCarbonViewControl : public AUCarbonViewControl
{
public:
	DGCarbonViewControl(AUCarbonViewBase * ownerView, AUParameterListenerRef listener, ControlType type, const AUVParameter &param, ControlRef control);

	virtual void ControlToParameter();
	virtual void ParameterToControl(Float32 newValue);
};

#endif


#endif
// __DFXGUI_CONTROL_H
