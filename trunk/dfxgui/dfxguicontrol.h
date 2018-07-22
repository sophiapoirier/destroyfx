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

	void setDrawAlpha(float inAlpha);
	float getDrawAlpha() const;

	bool setHelpText(char const* inText);
#if TARGET_OS_MAC
	bool setHelpText(CFStringRef inText);
#endif

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



//-----------------------------------------------------------------------------
class DGNullControl : public CControl, public DGControl
{
public:
	DGNullControl(DfxGuiEditor* inOwnerEditor, DGRect const& inRegion, DGImage* inBackgroundImage = nullptr);

	void draw(CDrawContext* inContext) override;

	CLASS_METHODS(DGNullControl, CControl);
};

#else  // !TARGET_PLUGIN_USES_VSTGUI



//-----------------------------------------------------------------------------
class DGControl : public CControl
{
public:
	enum KeyModifiers : unsigned int
	{
		kKeyModifier_Accel = 1,  // command on Macs, control on PCs
		kKeyModifier_Alt = 1 << 1,  // option on Macs, alt on PCs
		kKeyModifier_Shift = 1 << 2,
		kKeyModifier_Extra = 1 << 3  // control on Macs
	};

	// control for a parameter
	DGControl(DfxGuiEditor* inOwnerEditor, long inParameterID, DGRect const& inRegion);
	// control with no actual parameter attached
	DGControl(DfxGuiEditor* inOwnerEditor, DGRect const& inRegion, float inRange);

	// mouse position is relative to the control's bounds for ultra convenience
	void do_mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, dfx::KeyModifiers inKeyModifiers, bool inIsDoubleClick);
	virtual void mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, dfx::KeyModifiers inKeyModifiers, bool inIsDoubleClick) {}
	void do_mouseTrack(float inXpos, float inYpos, unsigned long inMouseButtons, dfx::KeyModifiers inKeyModifiers);
	virtual void mouseTrack(float inXpos, float inYpos, unsigned long inMouseButtons, dfx::KeyModifiers inKeyModifiers) {}
	void do_mouseUp(float inXpos, float inYpos, dfx::KeyModifiers inKeyModifiers);
	virtual void mouseUp(float inXpos, float inYpos, dfx::KeyModifiers inKeyModifiers) {}
	bool do_mouseWheel(long inDelta, dfx::Axis inAxis, dfx::KeyModifiers inKeyModifiers);
	virtual bool mouseWheel(long inDelta, dfx::Axis inAxis, dfx::KeyModifiers inKeyModifiers);

	void setRespondToMouseWheel(bool inMouseWheelPolicy)
	{	shouldRespondToMouseWheel = inMouseWheelPolicy;	}
	bool getRespondToMouseWheel() const
	{	return shouldRespondToMouseWheel;	}
	void setWraparoundValues(bool inWraparoundPolicy)
	{	shouldWraparoundValues = inWraparoundPolicy;	}
	bool getWraparoundValues() const
	{	return shouldWraparoundValues;	}

	bool isContinuousControl() const
	{	return isContinuous;	}
	void setControlContinuous(bool inContinuity);

	void setMouseDragRange(float inMouseDragRange)
	{
		if (inMouseDragRange != 0.0f)	// to prevent division by zero
			mouseDragRange = inMouseDragRange;
	}
	float getMouseDragRange() const
	{	return mouseDragRange;	}

private:
	bool				isContinuous = false;
	float				mouseDragRange = kDefaultMouseDragRange;  // the range of pixels over which you can drag the mouse to adjust the control value
	bool				shouldRespondToMouseWheel = true;
	bool				currentlyIgnoringMouseTracking = false;
	bool				shouldWraparoundValues = false;
};



#endif	// !TARGET_PLUGIN_USES_VSTGUI
