/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2021  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 2 of the License, or 
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


#include <functional>
#include <string>
#include <vector>

#include "dfxguicontrol.h"



//-----------------------------------------------------------------------------
class DGButton : public DGControl<VSTGUI::CControl>
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
			 Mode inMode, bool inDrawMomentaryState = false);
	DGButton(DfxGuiEditor* inOwnerEditor, DGRect const& inRegion, DGImage* inImage, 
			 long inNumStates, Mode inMode, bool inDrawMomentaryState = false);

	void draw(VSTGUI::CDrawContext* inContext) override;
	VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons) override;
	VSTGUI::CMouseEventResult onMouseMoved(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons) override;
	VSTGUI::CMouseEventResult onMouseUp(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons) override;
	VSTGUI::CMouseEventResult onMouseCancel() override;
	bool onWheel(VSTGUI::CPoint const& inPos, VSTGUI::CMouseWheelAxis const& inAxis, 
				 float const& inDistance, VSTGUI::CButtonState const& inButtons) override;


	void setMouseIsDown(bool newMouseState);
	bool getMouseIsDown() const noexcept
	{
		return mMouseIsDown;
	}

	void setButtonImage(DGImage* inImage);

	using UserProcedure = std::function<void(long)>;
	void setUserProcedure(UserProcedure const& inProc);
	void setUserProcedure(UserProcedure&& inProc);
	void setUserReleaseProcedure(UserProcedure const& inProc, bool inOnlyAtEndWithNoCancel = false);
	void setUserReleaseProcedure(UserProcedure&& inProc, bool inOnlyAtEndWithNoCancel = false);

	void setOrientation(dfx::Axis inOrientation) noexcept;
	void setRadioThresholds(std::vector<VSTGUI::CCoord> const& inThresholds);

	CLASS_METHODS(DGButton, VSTGUI::CControl)

protected:
	long constrainValue(long inValue) const;

	UserProcedure mUserProcedure = nullptr;
	UserProcedure mUserReleaseProcedure = nullptr;
	bool mUseReleaseProcedureOnlyAtEndWithNoCancel = false;

	Mode const mMode;
	bool const mDrawMomentaryState;
	bool const mWraparoundValues;
	dfx::Axis mOrientation = dfx::kAxis_Horizontal;

private:
	long getRadioValue(VSTGUI::CPoint const& inPos) const;
	long getRange() const;  // the salient view dimension per the orientation

	bool mMouseIsDown = false;
	long mEntryValue = 0, mNewValue = 0;
	std::vector<VSTGUI::CCoord> mRadioThresholds;
};



//-----------------------------------------------------------------------------
// shortcut for create an on/off button with dimensions derived from its image
class DGToggleImageButton : public DGButton
{
public:
	DGToggleImageButton(DfxGuiEditor* inOwnerEditor, long inParameterID, VSTGUI::CCoord inXpos, VSTGUI::CCoord inYpos, 
						DGImage* inImage, bool inDrawMomentaryState = false);
	DGToggleImageButton(DfxGuiEditor* inOwnerEditor, VSTGUI::CCoord inXpos, VSTGUI::CCoord inYpos, 
						DGImage* inImage, bool inDrawMomentaryState = false);

	CLASS_METHODS(DGToggleImageButton, DGButton)

private:
	static constexpr long kNumStates = 2;

	static DGRect makeRegion(VSTGUI::CCoord inXpos, VSTGUI::CCoord inYpos, DGImage* inImage, bool inDrawMomentaryState);
};



//-----------------------------------------------------------------------------
class DGFineTuneButton : public DGControl<VSTGUI::CControl>
{
public:
	DGFineTuneButton(DfxGuiEditor* inOwnerEditor, long inParameterID, DGRect const& inRegion, 
					 DGImage* inImage, float inValueChangeAmount = 0.0001f);

	void draw(VSTGUI::CDrawContext* inContext) override;
	VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons) override;
	VSTGUI::CMouseEventResult onMouseMoved(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons) override;
	VSTGUI::CMouseEventResult onMouseUp(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons) override;
	VSTGUI::CMouseEventResult onMouseCancel() override;

	CLASS_METHODS(DGFineTuneButton, VSTGUI::CControl)

protected:
	float const mValueChangeAmount;
	bool mMouseIsDown = false;
	float mEntryValue = 0.0f, mNewValue = 0.0f;
};



//-----------------------------------------------------------------------------
class DGValueSpot : public DGControl<VSTGUI::CControl>
{
public:
	DGValueSpot(DfxGuiEditor* inOwnerEditor, long inParamID, DGRect const& inRegion, DGImage* inImage, double inValue);

	void draw(VSTGUI::CDrawContext* inContext) override;
	VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons) override;
	VSTGUI::CMouseEventResult onMouseMoved(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons) override;
	VSTGUI::CMouseEventResult onMouseUp(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons) override;
	VSTGUI::CMouseEventResult onMouseCancel() override;

	CLASS_METHODS(DGValueSpot, VSTGUI::CControl)

private:
	float const mValueToSet;
	float mEntryValue = 0.0f;
	VSTGUI::CPoint mLastMousePos;
	bool mButtonIsPressed = false;
};



//-----------------------------------------------------------------------------
class DGWebLink : public DGButton
{
public:
	DGWebLink(DfxGuiEditor* inOwnerEditor, DGRect const& inRegion, DGImage* inImage, char const* inURL);

	VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons) override;
	VSTGUI::CMouseEventResult onMouseMoved(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons) override;
	VSTGUI::CMouseEventResult onMouseUp(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons) override;
	VSTGUI::CMouseEventResult onMouseCancel() override;
	bool onWheel(VSTGUI::CPoint const&, VSTGUI::CMouseWheelAxis const&, float const&, VSTGUI::CButtonState const&) override
	{
		return false;
	}

	CLASS_METHODS(DGWebLink, DGButton)

private:
	std::string const mURL;
};



//-----------------------------------------------------------------------------
class DGSplashScreen : public DGControl<VSTGUI::CSplashScreen>
{
public:
	DGSplashScreen(DfxGuiEditor* inOwnerEditor, DGRect const& inClickRegion, DGImage* inSplashImage);

	CLASS_METHODS(DGSplashScreen, VSTGUI::CSplashScreen)
};
