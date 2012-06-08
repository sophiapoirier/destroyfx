/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2012  Sophia Poirier

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


#ifdef TARGET_PLUGIN_USES_VSTGUI

//-----------------------------------------------------------------------------
class DGButton : public CControl
{
public:
	DGButton(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion, DGImage * inImage, long inNumStates, 
				DfxGuiBottonMode inMode, bool inDrawMomentaryState = false);
	DGButton(DfxGuiEditor * inOwnerEditor, DGRect * inRegion, DGImage * inImage, long inNumStates, 
				DfxGuiBottonMode inMode, bool inDrawMomentaryState = false);

	virtual void draw(CDrawContext * inContext);
	virtual CMouseEventResult onMouseDown(CPoint & inPos, const CButtonState & inButtons);
	virtual CMouseEventResult onMouseMoved(CPoint & inPos, const CButtonState & inButtons);
	virtual CMouseEventResult onMouseUp(CPoint & inPos, const CButtonState & inButtons);
	virtual bool onWheel(const CPoint & inPos, const float & inDistance, const CButtonState & inButtons);


	void setMouseIsDown(bool newMouseState);
	bool getMouseIsDown()
		{	return mouseIsDown;	}

	long getNumStates()
		{	return numStates;	}
	void setNumStates(long inNumStates)
		{	if (inNumStates > 0) numStates = inNumStates;	}
	void setButtonImage(DGImage * inButtonImage);
	long getValue_i();
	void setValue_i(long inValue);
	void setParameterID(long inParameterID);

	virtual void setUserProcedure(DGButtonUserProcedure inProc, void * inUserData);

	void setOrientation(DGAxis inOrientation)
		{	orientation = inOrientation;	}

	CLASS_METHODS(DGButton, CControl)

protected:
	DGRect * getBounds()
		{	return &controlPos;	}
	bool getWraparoundValues()
		{	return wraparoundValues;	}

	DGButtonUserProcedure userProcedure;
	void * userProcData;

	long numStates;
	DfxGuiBottonMode mode;
	bool drawMomentaryState;
	DGAxis orientation;
	bool mouseIsDown;
	long entryValue, newValue;
	DGRect controlPos;
	bool wraparoundValues;

private:
	void init(long inNumStates, DfxGuiBottonMode inMode, bool inDrawMomentaryState);
};



//-----------------------------------------------------------------------------
class DGValueSpot : public CControl
{
public:
	DGValueSpot(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion, DGImage * inImage, double inValue);
	virtual void draw(CDrawContext * inContext);
	virtual CMouseEventResult onMouseDown(CPoint & inPos, const CButtonState & inButtons);
	virtual CMouseEventResult onMouseMoved(CPoint & inPos, const CButtonState & inButtons);
	virtual CMouseEventResult onMouseUp(CPoint & inPos, const CButtonState & inButtons);

	CLASS_METHODS(DGValueSpot, CControl)

private:
	double valueToSet;
	CPoint oldMousePos;
	bool buttonIsPressed;
};



//-----------------------------------------------------------------------------
class DGWebLink : public DGButton
{
public:
	DGWebLink(DfxGuiEditor * inOwnerEditor, DGRect * inRegion, DGImage * inImage, const char * inURL);
	virtual ~DGWebLink();

	virtual CMouseEventResult onMouseDown(CPoint & inPos, const CButtonState & inButtons);
	virtual CMouseEventResult onMouseMoved(CPoint & inPos, const CButtonState & inButtons);
	virtual CMouseEventResult onMouseUp(CPoint & inPos, const CButtonState & inButtons);
	virtual bool onWheel(const CPoint & inPos, const float & inDistance, const CButtonState & inButtons)
		{	return false;	}

private:
	char * urlString;
};

#else



//-----------------------------------------------------------------------------
class DGButton : public DGControl
{
public:
	DGButton(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion, DGImage * inImage, 
				long inNumStates, DfxGuiBottonMode inMode, bool inDrawMomentaryState = false);
	DGButton(DfxGuiEditor * inOwnerEditor, DGRect * inRegion, DGImage * inImage, 
				long inNumStates, DfxGuiBottonMode inMode, bool inDrawMomentaryState = false);
	virtual ~DGButton();
	virtual void post_embed();

	virtual void draw(DGGraphicsContext * inContext);
	virtual void mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers, bool inIsDoubleClick);
	virtual void mouseTrack(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers);
	virtual void mouseUp(float inXpos, float inYpos, DGKeyModifiers inKeyModifiers);
	virtual bool mouseWheel(long inDelta, DGAxis inAxis, DGKeyModifiers inKeyModifiers);

	void setMouseIsDown(bool newMouseState);
	bool getMouseIsDown()
		{	return mouseIsDown;	}

	void setNumStates(long inNumStates)
		{	if (inNumStates > 0) numStates = inNumStates;	}
	long getNumStates()
		{	return numStates;	}

	void setButtonImage(DGImage * inImage);

	void setUserProcedure(DGButtonUserProcedure inProc, void * inUserData);
	void setUserReleaseProcedure(DGButtonUserProcedure inProc, void * inUserData, bool inOnlyAtEndWithNoCancel = false);

	void setOrientation(DGAxis inOrientation)
		{	orientation = inOrientation;	}

protected:
	DGImage * buttonImage;

	DGButtonUserProcedure userProcedure;
	DGButtonUserProcedure userReleaseProcedure;
	void * userProcData;
	void * userReleaseProcData;
	bool useReleaseProcedureOnlyAtEndWithNoCancel;

	long numStates;
	DfxGuiBottonMode mode;
	bool drawMomentaryState;
	DGAxis orientation;
	bool mouseIsDown;
	long entryValue, newValue;

private:
	void init();
};


//-----------------------------------------------------------------------------
class DGFineTuneButton : public DGControl
{
public:
	DGFineTuneButton(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion, 
						DGImage * inImage, float inValueChangeAmount = 0.0001f);
	virtual ~DGFineTuneButton();

	virtual void draw(DGGraphicsContext * inContext);
	virtual void mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers, bool inIsDoubleClick);
	virtual void mouseTrack(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers);
	virtual void mouseUp(float inXpos, float inYpos, DGKeyModifiers inKeyModifiers);

	void setValueChangeAmount(float inChangeAmount)
		{	valueChangeAmount = inChangeAmount;	}

protected:
	DGImage * buttonImage;
	float valueChangeAmount;
	bool mouseIsDown;
	long entryValue, newValue;
};


//-----------------------------------------------------------------------------
class DGValueSpot : public DGControl
{
public:
	DGValueSpot(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion, DGImage * inImage, double inValue);
	virtual void draw(DGGraphicsContext * inContext);
	virtual void mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers, bool inIsDoubleClick);
	virtual void mouseTrack(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers);
	virtual void mouseUp(float inXpos, float inYpos, DGKeyModifiers inKeyModifiers);

private:
	DGImage * buttonImage;
	float valueToSet;
	long mParameterID_unattached;
};


//-----------------------------------------------------------------------------
class DGWebLink : public DGButton
{
public:
	DGWebLink(DfxGuiEditor * inOwnerEditor, DGRect * inRegion, DGImage * inImage, const char * inURL);
	virtual ~DGWebLink();

	virtual void mouseUp(float inXpos, float inYpos, DGKeyModifiers inKeyModifiers);

private:
	char * urlString;
};


//-----------------------------------------------------------------------------
class DGSplashScreen : public DGControl
{
public:
	DGSplashScreen(DfxGuiEditor * inOwnerEditor, DGRect * inRegion, DGImage * inSplashImage);
	virtual void mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers, bool inIsDoubleClick);

	void removeSplashDisplay();

private:
	DGImage * splashImage;
	class DGSplashScreenDisplay : public DGControl
	{
		public:
			DGSplashScreenDisplay(DGSplashScreen * inParentControl, DGRect * inRegion, DGImage * inSplashImage);
			virtual void draw(DGGraphicsContext * inContext);
		private:
			DGSplashScreen * parentSplashControl;
			DGImage * splashImage;
	};
	DGSplashScreenDisplay * splashDisplay;
};

#endif	// !TARGET_PLUGIN_USES_VSTGUI



#endif
