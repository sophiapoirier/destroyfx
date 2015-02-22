/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2015  Sophia Poirier

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

#ifndef __DFXGUI_BUTTON_H
#define __DFXGUI_BUTTON_H


#include "dfxguieditor.h"



//-----------------------------------------------------------------------------
typedef void (*DGButtonUserProcedure) (long inValue, void * inUserData);


//-----------------------------------------------------------------------------
class DGButton : public CControl, public DGControl
{
public:
	DGButton(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion, DGImage * inImage, long inNumStates, 
				DfxGuiBottonMode inMode, bool inDrawMomentaryState = false);
	DGButton(DfxGuiEditor * inOwnerEditor, DGRect * inRegion, DGImage * inImage, long inNumStates, 
				DfxGuiBottonMode inMode, bool inDrawMomentaryState = false);

	virtual void draw(CDrawContext * inContext) VSTGUI_OVERRIDE_VMETHOD;
	virtual CMouseEventResult onMouseDown(CPoint & inPos, const CButtonState & inButtons) VSTGUI_OVERRIDE_VMETHOD;
	virtual CMouseEventResult onMouseMoved(CPoint & inPos, const CButtonState & inButtons) VSTGUI_OVERRIDE_VMETHOD;
	virtual CMouseEventResult onMouseUp(CPoint & inPos, const CButtonState & inButtons) VSTGUI_OVERRIDE_VMETHOD;
	virtual bool onWheel(const CPoint & inPos, const float & inDistance, const CButtonState & inButtons) VSTGUI_OVERRIDE_VMETHOD;


	void setMouseIsDown(bool newMouseState);
	bool getMouseIsDown()
		{	return mouseIsDown;	}

	long getNumStates()
		{	return numStates;	}
	void setNumStates(long inNumStates)
		{	if (inNumStates > 0) numStates = inNumStates;	}
	void setButtonImage(DGImage * inImage);
	long getValue_i();
	void setValue_i(long inValue);

	virtual void setUserProcedure(DGButtonUserProcedure inProc, void * inUserData);
	virtual void setUserReleaseProcedure(DGButtonUserProcedure inProc, void * inUserData, bool inOnlyAtEndWithNoCancel = false);

	void setOrientation(DGAxis inOrientation)
		{	orientation = inOrientation;	}

	CLASS_METHODS(DGButton, CControl)

protected:
	DGButtonUserProcedure userProcedure;
	void * userProcData;
	DGButtonUserProcedure userReleaseProcedure;
	void * userReleaseProcData;
	bool useReleaseProcedureOnlyAtEndWithNoCancel;

	long numStates;
	DfxGuiBottonMode mode;
	bool drawMomentaryState;
	DGAxis orientation;
	bool mouseIsDown;
	long entryValue, newValue;
	DGRect controlPos;

private:
	void init(long inNumStates, DfxGuiBottonMode inMode, bool inDrawMomentaryState);
};



//-----------------------------------------------------------------------------
class DGFineTuneButton : public CControl, public DGControl
{
public:
	DGFineTuneButton(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion, 
						DGImage * inImage, float inValueChangeAmount = 0.0001f);

	virtual void draw(CDrawContext * inContext) VSTGUI_OVERRIDE_VMETHOD;
	virtual CMouseEventResult onMouseDown(CPoint & inPos, const CButtonState & inButtons) VSTGUI_OVERRIDE_VMETHOD;
	virtual CMouseEventResult onMouseMoved(CPoint & inPos, const CButtonState & inButtons) VSTGUI_OVERRIDE_VMETHOD;
	virtual CMouseEventResult onMouseUp(CPoint & inPos, const CButtonState & inButtons) VSTGUI_OVERRIDE_VMETHOD;

	void setValueChangeAmount(float inChangeAmount);

	CLASS_METHODS(DGFineTuneButton, CControl)

private:
	float valueChangeAmount;
	bool mouseIsDown;
	float entryValue, newValue;
};



//-----------------------------------------------------------------------------
class DGValueSpot : public CControl, public DGControl
{
public:
	DGValueSpot(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion, DGImage * inImage, double inValue);

	virtual void draw(CDrawContext * inContext) VSTGUI_OVERRIDE_VMETHOD;
	virtual CMouseEventResult onMouseDown(CPoint & inPos, const CButtonState & inButtons) VSTGUI_OVERRIDE_VMETHOD;
	virtual CMouseEventResult onMouseMoved(CPoint & inPos, const CButtonState & inButtons) VSTGUI_OVERRIDE_VMETHOD;
	virtual CMouseEventResult onMouseUp(CPoint & inPos, const CButtonState & inButtons) VSTGUI_OVERRIDE_VMETHOD;

	CLASS_METHODS(DGValueSpot, CControl)

private:
	const float valueToSet;
	CPoint oldMousePos;
	bool buttonIsPressed;
};



//-----------------------------------------------------------------------------
class DGWebLink : public DGButton
{
public:
	DGWebLink(DfxGuiEditor * inOwnerEditor, DGRect * inRegion, DGImage * inImage, const char * inURL);
	virtual ~DGWebLink();

	virtual CMouseEventResult onMouseDown(CPoint & inPos, const CButtonState & inButtons) VSTGUI_OVERRIDE_VMETHOD;
	virtual CMouseEventResult onMouseMoved(CPoint & inPos, const CButtonState & inButtons) VSTGUI_OVERRIDE_VMETHOD;
	virtual CMouseEventResult onMouseUp(CPoint & inPos, const CButtonState & inButtons) VSTGUI_OVERRIDE_VMETHOD;
	virtual bool onWheel(const CPoint & inPos, const float & inDistance, const CButtonState & inButtons) VSTGUI_OVERRIDE_VMETHOD
		{	return false;	}

	CLASS_METHODS(DGWebLink, DGButton)

private:
	char * urlString;
};



//-----------------------------------------------------------------------------
class DGSplashScreen : public CSplashScreen, public DGControl
{
public:
	DGSplashScreen(DfxGuiEditor * inOwnerEditor, DGRect * inClickRegion, DGImage * inSplashImage);

	CLASS_METHODS(DGSplashScreen, CSplashScreen)
};



#endif
