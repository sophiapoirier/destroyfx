/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2003-2011  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Destroy FX Library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Destroy FX Library.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#ifndef __DFXGUI_CONTROL_H
#define __DFXGUI_CONTROL_H


#include "dfxguimisc.h"



//-----------------------------------------------------------------------------
const float kDfxGui_DefaultFineTuneFactor = 10.0f;
const float kDfxGui_DefaultMouseDragRange = 200.0f;	// pixels
//const float kDfxGui_DefaultMouseDragValueIncrement = 0.1f;
//const float kDfxGui_DefaultMouseDragValueIncrement_descrete = 15.0f;



#ifdef TARGET_PLUGIN_USES_VSTGUI
typedef CControl	DGControl;
#else



class DfxGuiEditor;

//-----------------------------------------------------------------------------
class DGControl : public CControl
{
public:
	// control for a parameter
	DGControl(DfxGuiEditor * inOwnerEditor, long inParameterID, DGRect * inRegion);
	// control with no actual parameter attached
	DGControl(DfxGuiEditor * inOwnerEditor, DGRect * inRegion, float inRange);
	virtual ~DGControl();

	virtual void draw(CDrawContext * inContext)
		{ }
	// mouse position is relative to the control's bounds for ultra convenience
	void do_mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers, bool inIsDoubleClick);
	virtual void mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers, bool inIsDoubleClick)
		{ }
	void do_mouseTrack(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers);
	virtual void mouseTrack(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers)
		{ }
	void do_mouseUp(float inXpos, float inYpos, DGKeyModifiers inKeyModifiers);
	virtual void mouseUp(float inXpos, float inYpos, DGKeyModifiers inKeyModifiers)
		{ }
	bool do_mouseWheel(long inDelta, DGAxis inAxis, DGKeyModifiers inKeyModifiers);
	virtual bool mouseWheel(long inDelta, DGAxis inAxis, DGKeyModifiers inKeyModifiers);
	bool do_contextualMenuClick();
	virtual bool contextualMenuClick();

	// force a redraw of the control
	void redraw();

	// this will get called regularly by an idle timer
	virtual void idle()
		{ }

	void setRespondToMouse(bool inMousePolicy)
		{	shouldRespondToMouse = inMousePolicy;	}
	bool getRespondToMouse()
		{	return shouldRespondToMouse;	}
	void setRespondToMouseWheel(bool inMouseWheelPolicy)
		{	shouldRespondToMouseWheel = inMouseWheelPolicy;	}
	bool getRespondToMouseWheel()
		{	return shouldRespondToMouseWheel;	}
	void setWraparoundValues(bool inWraparoundPolicy)
		{	shouldWraparoundValues = inWraparoundPolicy;	}
	bool getWraparoundValues()
		{	return shouldWraparoundValues;	}

	DfxGuiEditor * getDfxGuiEditor()
		{	return ownerEditor;	}

	bool isContinuousControl()
		{	return isContinuous;	}
	void setControlContinuous(bool inContinuity);

	void setFineTuneFactor(float inFineTuneFactor)
	{
		if (inFineTuneFactor != 0.0f)	// to prevent division by zero
			fineTuneFactor = inFineTuneFactor;
	}
	float getFineTuneFactor()
		{	return fineTuneFactor;	}
	void setMouseDragRange(float inMouseDragRange)
	{
		if (inMouseDragRange != 0.0f)	// to prevent division by zero
			mouseDragRange = inMouseDragRange;
	}
	float getMouseDragRange()
		{	return mouseDragRange;	}

	bool isParameterAttached()
		{	return parameterAttached;	}
	long getParameterID();
	void setParameterID(long inParameterID);
	void setValue_i(long inValue);
	long getValue_i();
	virtual CFStringRef createStringFromValue();
	virtual bool setValueWithString(CFStringRef inString);
#ifdef TARGET_API_AUDIOUNIT
	CAAUParameter & getAUVP()
		{	return auvp;	}
	void createAUVcontrol();
	AUCarbonViewControl * getAUVcontrol()
		{	return auv_control;	}
#endif
	float getRange()
		{	return valueRange;	}
	DGRect * getBounds()
		{	return &controlPos;	}
	DGRect * getForeBounds()
		{	return &vizArea;	}

	void setOffset(long x, long y);

	void setBounds(DGRect * r)
		{	controlPos = *r;	}
	void setForeBounds(long x, long y, long w, long h);
	void shrinkForeBounds(long x, long y, long w, long h);

	void setDrawAlpha(float inAlpha);
	float getDrawAlpha()
		{	return drawAlpha;	}

#if TARGET_OS_MAC
	bool setHelpText(const char * inHelpText);
#endif

protected:
	DfxGuiEditor *		ownerEditor;

	float				valueRange;
	bool				parameterAttached;
	bool				isContinuous;

	DGRect				controlPos;	// the control's area
	DGRect				vizArea; 	// where the foreground displays
	float				drawAlpha;	// level of overall transparency to draw with (0.0 to 1.0)

private:
	// common constructor stuff
	void init(DGRect * inRegion);

	float				fineTuneFactor;	// slow-down factor for fine-tune control (if the control supports that)
	float				mouseDragRange;	// the range of pixels over which you can drag the mouse to adjust the control value

	bool				shouldRespondToMouse;
	bool				shouldRespondToMouseWheel;
	bool				currentlyIgnoringMouseTracking;
	bool				shouldWraparoundValues;
};


#ifdef TARGET_API_AUDIOUNIT
//-----------------------------------------------------------------------------
// this gives some slight tweaks to Apple's AUCarbonViewControl class
class DGCarbonViewControl : public AUCarbonViewControl
{
public:
	DGCarbonViewControl(AUCarbonViewBase * ownerView, AUParameterListenerRef listener, ControlType type, const CAAUParameter & param, ControlRef control);

	virtual void ControlToParameter();
	virtual void ParameterToControl(Float32 inNewValue);
};
#endif



#endif	// !TARGET_PLUGIN_USES_VSTGUI


#endif
// __DFXGUI_CONTROL_H
