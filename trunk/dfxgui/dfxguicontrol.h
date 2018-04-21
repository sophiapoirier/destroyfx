/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2003-2018  Sophia Poirier

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

#pragma once


#include "dfxguimisc.h"



//-----------------------------------------------------------------------------
class DfxGuiEditor;



#ifdef TARGET_PLUGIN_USES_VSTGUI

//-----------------------------------------------------------------------------
class DGControl
{
public:
	DGControl(CControl* inControl, DfxGuiEditor* inOwnerEditor);

	CControl* getCControl() const noexcept
	{
		return mControl;
	}
	DfxGuiEditor* getOwnerEditor() const noexcept
	{
		return mOwnerEditor;
	}

	void setValue_gen(float inValue);
	void setDefaultValue_gen(float inValue);
	void redraw();
	long getParameterID() const;
	void setParameterID(long inParameterID);
	bool isParameterAttached() const;

protected:
	static constexpr float kDefaultFineTuneFactor = 10.0f;
	//static constexpr float kDefaultMouseDragRange = 200.0f;  // pixels
	//static constexpr float kDefaultMouseDragValueIncrement = 0.1f;
	//static constexpr float kDefaultMouseDragValueIncrement_descrete = 15.0f;

	bool getWraparoundValues() const noexcept
	{
		return wraparoundValues;
	}
	void setWraparoundValues(bool inWraparoundPolicy) noexcept
	{
		wraparoundValues = inWraparoundPolicy;
	}

private:
	CControl* const mControl;
	DfxGuiEditor* const mOwnerEditor;

	bool wraparoundValues = false;
};

#else  // !TARGET_PLUGIN_USES_VSTGUI



//-----------------------------------------------------------------------------
class DGControl : public CControl
{
public:
	// control for a parameter
	DGControl(DfxGuiEditor * inOwnerEditor, long inParameterID, DGRect * inRegion);
	// control with no actual parameter attached
	DGControl(DfxGuiEditor * inOwnerEditor, DGRect * inRegion, float inRange);

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
	virtual bool contextualMenuClick()
		{	return false;	}

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



#endif	// !TARGET_PLUGIN_USES_VSTGUI
