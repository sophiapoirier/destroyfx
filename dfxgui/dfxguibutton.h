/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2024  Sophia Poirier

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

To contact the author, use the contact form at http://destroyfx.org
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

	DGButton(DfxGuiEditor* inOwnerEditor, dfx::ParameterID inParameterID, DGRect const& inRegion, DGImage* inImage, 
			 Mode inMode, bool inDrawMomentaryState = false);
	DGButton(DfxGuiEditor* inOwnerEditor, DGRect const& inRegion, DGImage* inImage, 
			 size_t inNumStates, Mode inMode, bool inDrawMomentaryState = false);

	void draw(VSTGUI::CDrawContext* inContext) override;
	void onMouseDownEvent(VSTGUI::MouseDownEvent& ioEvent) override;
	void onMouseMoveEvent(VSTGUI::MouseMoveEvent& ioEvent) override;
	void onMouseUpEvent(VSTGUI::MouseUpEvent& ioEvent) override;
	void onMouseCancelEvent(VSTGUI::MouseCancelEvent& ioEvent) override;
	void onMouseWheelEvent(VSTGUI::MouseWheelEvent& ioEvent) override;


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
	static constexpr long kMinValue = 0;
	long getMaxValue() const;
	long getRadioValue(VSTGUI::CPoint const& inPos) const;
	long getMouseableRange() const;  // the salient view dimension per the orientation

	bool mMouseIsDown = false;
	long mEntryValue = 0, mNewValue = 0;
	std::vector<VSTGUI::CCoord> mRadioThresholds;
};



//-----------------------------------------------------------------------------
// shortcut for creating an on/off button with dimensions derived from its image
class DGToggleImageButton : public DGButton
{
public:
	DGToggleImageButton(DfxGuiEditor* inOwnerEditor, dfx::ParameterID inParameterID, VSTGUI::CCoord inXpos, VSTGUI::CCoord inYpos, 
						DGImage* inImage, bool inDrawMomentaryState = false);
	DGToggleImageButton(DfxGuiEditor* inOwnerEditor, VSTGUI::CCoord inXpos, VSTGUI::CCoord inYpos, 
						DGImage* inImage, bool inDrawMomentaryState = false);

	CLASS_METHODS(DGToggleImageButton, DGButton)

private:
	static constexpr size_t kNumStates = 2;

	static DGRect makeRegion(VSTGUI::CCoord inXpos, VSTGUI::CCoord inYpos, DGImage* inImage, bool inDrawMomentaryState);
};



//-----------------------------------------------------------------------------
class DGFineTuneButton : public DGControl<VSTGUI::CControl>
{
public:
	DGFineTuneButton(DfxGuiEditor* inOwnerEditor, dfx::ParameterID inParameterID, DGRect const& inRegion, 
					 DGImage* inImage, float inValueChangeAmount = 0.0001f);

	void draw(VSTGUI::CDrawContext* inContext) override;
	void onMouseDownEvent(VSTGUI::MouseDownEvent& ioEvent) override;
	void onMouseMoveEvent(VSTGUI::MouseMoveEvent& ioEvent) override;
	void onMouseUpEvent(VSTGUI::MouseUpEvent& ioEvent) override;
	void onMouseCancelEvent(VSTGUI::MouseCancelEvent& ioEvent) override;

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
	DGValueSpot(DfxGuiEditor* inOwnerEditor, dfx::ParameterID inParameterID, DGRect const& inRegion, DGImage* inImage, double inValue);

	void draw(VSTGUI::CDrawContext* inContext) override;
	void onMouseDownEvent(VSTGUI::MouseDownEvent& ioEvent) override;
	void onMouseMoveEvent(VSTGUI::MouseMoveEvent& ioEvent) override;
	void onMouseUpEvent(VSTGUI::MouseUpEvent& ioEvent) override;
	void onMouseCancelEvent(VSTGUI::MouseCancelEvent& ioEvent) override;

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

	void onMouseDownEvent(VSTGUI::MouseDownEvent& ioEvent) override;
	void onMouseMoveEvent(VSTGUI::MouseMoveEvent& ioEvent) override;
	void onMouseUpEvent(VSTGUI::MouseUpEvent& ioEvent) override;
	void onMouseCancelEvent(VSTGUI::MouseCancelEvent& ioEvent) override;
	void onMouseWheelEvent(VSTGUI::MouseWheelEvent&) override {}

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
