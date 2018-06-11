/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2018  Sophia Poirier

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


#include <string>

#include "dfxguicontrol.h"



//-----------------------------------------------------------------------------
typedef void (*DGButtonUserProcedure) (long inValue, void* inUserData);


//-----------------------------------------------------------------------------
class DGButton : public CControl, public DGControl
{
public:
	enum class Mode
	{
		Momentary,
		Increment,
		Decrement,
		Radio, 
		PictureReel
	};

	DGButton(DfxGuiEditor* inOwnerEditor, long inParamID, DGRect const& inRegion, DGImage* inImage, 
			 long inNumStates, Mode inMode, bool inDrawMomentaryState = false);
	DGButton(DfxGuiEditor* inOwnerEditor, DGRect const& inRegion, DGImage* inImage, 
			 long inNumStates, Mode inMode, bool inDrawMomentaryState = false);

	void draw(CDrawContext* inContext) override;
	CMouseEventResult onMouseDown(CPoint& inPos, CButtonState const& inButtons) override;
	CMouseEventResult onMouseMoved(CPoint& inPos, CButtonState const& inButtons) override;
	CMouseEventResult onMouseUp(CPoint& inPos, CButtonState const& inButtons) override;
	bool onWheel(CPoint const& inPos, float const& inDistance, CButtonState const& inButtons) override;


	void setMouseIsDown(bool newMouseState);
	bool getMouseIsDown() const noexcept
	{
		return mMouseIsDown;
	}

	// TODO: move these discrete control methods into DGControl?
	long getNumStates() const noexcept
	{
		return mNumStates;
	}
	void setNumStates(long inNumStates);
	void setButtonImage(DGImage* inImage);
	long getValue_i();
	void setValue_i(long inValue);

	virtual void setUserProcedure(DGButtonUserProcedure inProc, void* inUserData);
	virtual void setUserReleaseProcedure(DGButtonUserProcedure inProc, void* inUserData, bool inOnlyAtEndWithNoCancel = false);

	void setOrientation(dfx::Axis inOrientation) noexcept
	{
		mOrientation = inOrientation;
	}

	CLASS_METHODS(DGButton, CControl)

protected:
	DGButtonUserProcedure mUserProcedure = nullptr;
	void* mUserProcData = nullptr;
	DGButtonUserProcedure mUserReleaseProcedure = nullptr;
	void* mUserReleaseProcData = nullptr;
	bool mUseReleaseProcedureOnlyAtEndWithNoCancel = false;

	long mNumStates = 1;
	Mode mMode {};
	bool const mDrawMomentaryState;
	dfx::Axis mOrientation = dfx::kAxis_Horizontal;
	bool mMouseIsDown = false;
	long mEntryValue = 0, mNewValue = 0;
};



//-----------------------------------------------------------------------------
class DGFineTuneButton : public CControl, public DGControl
{
public:
	DGFineTuneButton(DfxGuiEditor* inOwnerEditor, long inParamID, DGRect const& inRegion, 
					 DGImage* inImage, float inValueChangeAmount = 0.0001f);

	void draw(CDrawContext* inContext) override;
	CMouseEventResult onMouseDown(CPoint& inPos, CButtonState const& inButtons) override;
	CMouseEventResult onMouseMoved(CPoint& inPos, CButtonState const& inButtons) override;
	CMouseEventResult onMouseUp(CPoint& inPos, CButtonState const& inButtons) override;

	CLASS_METHODS(DGFineTuneButton, CControl)

protected:
	float const mValueChangeAmount;
	bool mMouseIsDown = false;
	float mEntryValue = 0.0f, mNewValue = 0.0f;
};



//-----------------------------------------------------------------------------
class DGValueSpot : public CControl, public DGControl
{
public:
	DGValueSpot(DfxGuiEditor* inOwnerEditor, long inParamID, DGRect const& inRegion, DGImage* inImage, double inValue);

	void draw(CDrawContext* inContext) override;
	CMouseEventResult onMouseDown(CPoint& inPos, CButtonState const& inButtons) override;
	CMouseEventResult onMouseMoved(CPoint& inPos, CButtonState const& inButtons) override;
	CMouseEventResult onMouseUp(CPoint& inPos, CButtonState const& inButtons) override;

	CLASS_METHODS(DGValueSpot, CControl)

private:
	float const mValueToSet;
	CPoint mLastMousePos;
	bool mButtonIsPressed = false;
};



//-----------------------------------------------------------------------------
class DGWebLink : public DGButton
{
public:
	DGWebLink(DfxGuiEditor* inOwnerEditor, DGRect const& inRegion, DGImage* inImage, char const* inURL);

	CMouseEventResult onMouseDown(CPoint& inPos, CButtonState const& inButtons) override;
	CMouseEventResult onMouseMoved(CPoint& inPos, CButtonState const& inButtons) override;
	CMouseEventResult onMouseUp(CPoint& inPos, CButtonState const& inButtons) override;
	bool onWheel(CPoint const&, float const&, CButtonState const&) override
	{
		return false;
	}

	CLASS_METHODS(DGWebLink, DGButton)

private:
	std::string const mURL;
};



//-----------------------------------------------------------------------------
class DGSplashScreen : public CSplashScreen, public DGControl
{
public:
	DGSplashScreen(DfxGuiEditor* inOwnerEditor, DGRect const& inClickRegion, DGImage* inSplashImage);

	CLASS_METHODS(DGSplashScreen, CSplashScreen)
};
