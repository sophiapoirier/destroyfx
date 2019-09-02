/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2019  Sophia Poirier

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

#include "dfxguibutton.h"

#include <algorithm>
#include <cmath>

#include "dfxguieditor.h"


//-----------------------------------------------------------------------------
// DGButton
//-----------------------------------------------------------------------------
DGButton::DGButton(DfxGuiEditor* inOwnerEditor, long inParamID, DGRect const& inRegion, DGImage* inImage, 
				   Mode inMode, bool inDrawMomentaryState)
:	DGControl<VSTGUI::CControl>(inRegion, inOwnerEditor, inParamID, inImage), 
	mMode(inMode), 
	mDrawMomentaryState(inDrawMomentaryState)
{
	setMouseEnabled(mMode != Mode::PictureReel);
	setWraparoundValues((mMode == Mode::Increment) || (mMode == Mode::Decrement));
}

//-----------------------------------------------------------------------------
DGButton::DGButton(DfxGuiEditor* inOwnerEditor, DGRect const& inRegion, DGImage* inImage, 
				   long inNumStates, Mode inMode, bool inDrawMomentaryState)
:	DGButton(inOwnerEditor, dfx::kParameterID_Invalid, inRegion, inImage, inMode, inDrawMomentaryState)
{
	constexpr long minNumStates = 2;
	assert(inNumStates >= minNumStates);
	setNumStates(std::max(inNumStates, minNumStates));
}

//-----------------------------------------------------------------------------
void DGButton::draw(VSTGUI::CDrawContext* inContext)
{
	if (auto const image = getDrawBackground())
	{
		long const xoff = (mDrawMomentaryState && mMouseIsDown) ? (std::lround(image->getWidth()) / 2) : 0;
		long const yoff = getValue_i() * (std::lround(image->getHeight()) / getNumStates());

		image->draw(inContext, getViewSize(), VSTGUI::CPoint(xoff, yoff));
	}

#ifdef TARGET_API_RTAS
	getOwnerEditor()->drawControlHighlight(inContext, this);
#endif

	setDirty(false);
}

//-----------------------------------------------------------------------------
VSTGUI::CMouseEventResult DGButton::onMouseDown(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons)
{
	if (!inButtons.isLeftButton())
	{
		return VSTGUI::kMouseEventNotHandled;
	}

	beginEdit();

	mEntryValue = mNewValue = getValue_i();
	long const min = 0;
	long const max = getNumStates() - 1;
	auto const isDirectionReversed = inButtons.isAltSet();

	setMouseIsDown(true);

	switch (mMode)
	{
		case Mode::Momentary:
			mNewValue = max;
			mEntryValue = 0;  // just to make sure it's like that
			break;
		case Mode::Increment:
			mNewValue = mEntryValue + (isDirectionReversed ? -1 : 1);
			break;
		case Mode::Decrement:
			mNewValue = mEntryValue + (isDirectionReversed ? 1 : -1);
			break;
		case Mode::Radio:
			if (mOrientation & dfx::kAxis_Horizontal)
			{
				mNewValue = std::lround(inPos.x - getViewSize().left) / (std::lround(getWidth()) / getNumStates());
			}
			else
			{
				mNewValue = std::lround(inPos.y - getViewSize().top) / (std::lround(getHeight()) / getNumStates());
			}
			mNewValue += min;  // offset
			break;
		default:
			break;
	}

	// wrap around
	if (getWraparoundValues())
	{
		if (mNewValue > max)
		{
			mNewValue = min;
		}
		else if (mNewValue < min)
		{
			mNewValue = max;
		}
	}
	// limit
	else
	{
		mNewValue = std::clamp(mNewValue, min, max);
	}
	if (mNewValue != mEntryValue)
	{
		setValue_i(mNewValue);
		if (isDirty())
		{
			valueChanged();
			invalid();
		}
		if (mUserProcedure)
		{
			mUserProcedure(mNewValue, mUserProcData);
		}
	}

	return VSTGUI::kMouseEventHandled;
}

//-----------------------------------------------------------------------------
VSTGUI::CMouseEventResult DGButton::onMouseMoved(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons)
{
	if (!isEditing())
	{
		return VSTGUI::kMouseEventNotHandled;
	}

	if (!inButtons.isLeftButton())
	{
		return VSTGUI::kMouseEventHandled;
	}

	auto const currentValue = getValue_i();

	if (hitTest(inPos))
	{
		setMouseIsDown(true);

		if (mMode == Mode::Radio)
		{
			if (mOrientation & dfx::kAxis_Horizontal)
			{
				mNewValue = std::lround(inPos.x - getViewSize().left) / (std::lround(getWidth()) / getNumStates());
			}
			else
			{
				mNewValue = std::lround(inPos.y - getViewSize().top) / (std::lround(getHeight()) / getNumStates());
			}
			mNewValue = std::clamp(mNewValue, 0L, getNumStates() - 1);
		}
		if (mNewValue != currentValue)
		{
			setValue_i(mNewValue);
			if (mUserProcedure && (mNewValue != currentValue))
			{
				mUserProcedure(mNewValue, mUserProcData);
			}
		}
	}

	else
	{
		setMouseIsDown(false);
		if (mMode == Mode::Radio)
		{
		}
		else
		{
			if (mEntryValue != currentValue)
			{
				setValue_i(mEntryValue);
				if (mUserReleaseProcedure && !mUseReleaseProcedureOnlyAtEndWithNoCancel)
				{
					mUserReleaseProcedure(mEntryValue, mUserReleaseProcData);
				}
			}
		}
	}

	if (isDirty())
	{
		valueChanged();
		invalid();
	}

	return VSTGUI::kMouseEventHandled;
}

//-----------------------------------------------------------------------------
VSTGUI::CMouseEventResult DGButton::onMouseUp(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& /*inButtons*/)
{
	setMouseIsDown(false);

	if (mMode == Mode::Momentary)
	{
		setValue_i(mEntryValue);
	}

	if (isDirty())
	{
		valueChanged();
		invalid();
	}

	if (hitTest(inPos))
	{
		if (mUserReleaseProcedure)
		{
			mUserReleaseProcedure(getValue_i(), mUserReleaseProcData);
		}
	}

	endEdit();

	return VSTGUI::kMouseEventHandled;
}

//-----------------------------------------------------------------------------
bool DGButton::onWheel(VSTGUI::CPoint const& inPos, float const& inDistance, VSTGUI::CButtonState const& inButtons)
{
	if (!getMouseEnabled())
	{
		return false;
	}
	// not controlling a parameter
	if (!isParameterAttached())
	{
		return false;
	}

	switch (mMode)
	{
		case Mode::Increment:
		case Mode::Decrement:
		case Mode::Momentary:
		{
			VSTGUI::CButtonState fakeButtons = inButtons;
			// go backwards
			if (inDistance < 0.0f)
			{
				fakeButtons |= VSTGUI::kAlt;
			}
			VSTGUI::CPoint fakePos(0, 0);
			onMouseDown(fakePos, fakeButtons);
			onMouseUp(fakePos, inButtons);
		}
		return true;

		default:
			return Parent::onWheel(inPos, inDistance, inButtons);  // XXX need an actual default implementation
	}
}

//-----------------------------------------------------------------------------
void DGButton::setMouseIsDown(bool newMouseState)
{
	auto const oldState = std::exchange(mMouseIsDown, newMouseState);
	if ((oldState != newMouseState) && mDrawMomentaryState)
	{
		redraw();
	}
}

//-----------------------------------------------------------------------------
void DGButton::setButtonImage(DGImage* inImage)
{
	setBackground(inImage);
	setDirty();  // parent implementation does not do this if mouse control is disabled
}

//-----------------------------------------------------------------------------
void DGButton::setUserProcedure(DGButtonUserProcedure inProc, void* inUserData)
{
	mUserProcedure = inProc;
	mUserProcData = inUserData;
}

//-----------------------------------------------------------------------------
void DGButton::setUserReleaseProcedure(DGButtonUserProcedure inProc, void* inUserData, bool inOnlyAtEndWithNoCancel)
{
	mUserReleaseProcedure = inProc;
	mUserReleaseProcData = inUserData;
	mUseReleaseProcedureOnlyAtEndWithNoCancel = inOnlyAtEndWithNoCancel;
}






#pragma mark -
#pragma mark DGFineTuneButton

//-----------------------------------------------------------------------------
// Fine-tune Button
//-----------------------------------------------------------------------------
DGFineTuneButton::DGFineTuneButton(DfxGuiEditor*	inOwnerEditor,
									long			inParameterID, 
									DGRect const&	inRegion,
									DGImage*		inImage, 
									float			inValueChangeAmount)
:	DGControl<VSTGUI::CControl>(inRegion, inOwnerEditor, inParameterID, inImage), 
	mValueChangeAmount(inValueChangeAmount)
{
}

//-----------------------------------------------------------------------------
void DGFineTuneButton::draw(VSTGUI::CDrawContext* inContext)
{
	if (auto const image = getDrawBackground())
	{
		VSTGUI::CCoord const yoff = mMouseIsDown ? getHeight() : 0;
		image->draw(inContext, getViewSize(), VSTGUI::CPoint(0, yoff));
	}

	setDirty(false);
}

//-----------------------------------------------------------------------------
VSTGUI::CMouseEventResult DGFineTuneButton::onMouseDown(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons)
{
	if (!inButtons.isLeftButton())
	{
		return VSTGUI::kMouseEventNotHandled;
	}

	beginEdit();

	// figure out all of the values that we'll be using
	mEntryValue = getValue();
	mNewValue = std::clamp(mEntryValue + mValueChangeAmount, getMin(), getMax());

	mMouseIsDown = false;  // "dirty" it for onMouseMoved
	return onMouseMoved(inPos, inButtons);
}

//-----------------------------------------------------------------------------
VSTGUI::CMouseEventResult DGFineTuneButton::onMouseMoved(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& /*inButtons*/)
{
	if (!isEditing())
	{
		return VSTGUI::kMouseEventNotHandled;
	}

	auto const oldMouseDown = mMouseIsDown;
	mMouseIsDown = hitTest(inPos);

	if (mMouseIsDown && !oldMouseDown)
	{
		if (mNewValue != getValue())
		{
			setValue(mNewValue);
		}
// XXX or do I prefer it not to do the mouse-down state when nothing is happening anyway?
//		else
		{
//			redraw();  // at least make sure that redrawing occurs for mMouseIsDown change
		}
	}
	else if (!mMouseIsDown && oldMouseDown)
	{
		if (mEntryValue != getValue())
		{
			setValue(mEntryValue);
		}
		else
		{
			redraw();  // at least make sure that redrawing occurs for mMouseIsDown change
		}
	}

	if (isDirty())
	{
		valueChanged();
		invalid();
	}

	return VSTGUI::kMouseEventHandled;
}

//-----------------------------------------------------------------------------
VSTGUI::CMouseEventResult DGFineTuneButton::onMouseUp(VSTGUI::CPoint& /*inPos*/, VSTGUI::CButtonState const& /*inButtons*/)
{
	if (std::exchange(mMouseIsDown, false))
	{
		redraw();
	}

	endEdit();

	return VSTGUI::kMouseEventHandled;
}






#pragma mark -

//-----------------------------------------------------------------------------
// DGValueSpot
//-----------------------------------------------------------------------------
DGValueSpot::DGValueSpot(DfxGuiEditor*	inOwnerEditor, 
						 long			inParamID, 
						 DGRect const&	inRegion, 
						 DGImage*		inImage, 
						 double			inValue)
:	DGControl<VSTGUI::CControl>(inRegion, inOwnerEditor, inParamID, inImage), 
	mValueToSet(inOwnerEditor->dfxgui_ContractParameterValue(inParamID, inValue))
{
	setTransparency(true);
}

//------------------------------------------------------------------------
void DGValueSpot::draw(VSTGUI::CDrawContext* inContext)
{
	if (auto const image = getDrawBackground())
	{
		VSTGUI::CCoord const yoff = mButtonIsPressed ? getHeight() : 0;
		image->draw(inContext, getViewSize(), VSTGUI::CPoint(0, yoff));
	}

#ifdef TARGET_API_RTAS
	getOwnerEditor()->drawControlHighlight(inContext, this);
#endif

	setDirty(false);
}

//-----------------------------------------------------------------------------
VSTGUI::CMouseEventResult DGValueSpot::onMouseDown(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons)
{
	if (!inButtons.isLeftButton())
	{
		return VSTGUI::kMouseEventNotHandled;
	}

	beginEdit();

	mLastMousePos(-1, -1);

	return onMouseMoved(inPos, inButtons);
}

//-----------------------------------------------------------------------------
VSTGUI::CMouseEventResult DGValueSpot::onMouseMoved(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons)
{
	if (!isEditing())
	{
		return VSTGUI::kMouseEventNotHandled;
	}

	if (inButtons.isLeftButton())
	{
		auto const oldButtonIsPressed = mButtonIsPressed;
		if (hitTest(inPos) && !hitTest(mLastMousePos))
		{
			setValue(mValueToSet);
			mButtonIsPressed = true;
		}
		else if (!hitTest(inPos) && hitTest(mLastMousePos))
		{
			mButtonIsPressed = false;
		}
		mLastMousePos = inPos;

		if (isDirty())
		{
			valueChanged();
			invalid();
		}
		if (oldButtonIsPressed != mButtonIsPressed)
		{
			redraw();
		}
	}

	return VSTGUI::kMouseEventHandled;
}

//-----------------------------------------------------------------------------
VSTGUI::CMouseEventResult DGValueSpot::onMouseUp(VSTGUI::CPoint& /*inPos*/, VSTGUI::CButtonState const& /*inButtons*/)
{
	endEdit();

	mButtonIsPressed = false;
	redraw();

	return VSTGUI::kMouseEventHandled;
}






#pragma mark -

//-----------------------------------------------------------------------------
// DGWebLink
//-----------------------------------------------------------------------------
DGWebLink::DGWebLink(DfxGuiEditor*	inOwnerEditor,
					 DGRect const&	inRegion,
					 DGImage*		inImage, 
					 char const*	inURL)
:	DGButton(inOwnerEditor, inRegion, inImage, 2, Mode::Momentary), 
	mURL(inURL)
{
}

//-----------------------------------------------------------------------------
VSTGUI::CMouseEventResult DGWebLink::onMouseDown(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons)
{
	if (!inButtons.isLeftButton())
	{
		return VSTGUI::kMouseEventNotHandled;
	}

	beginEdit();

	return onMouseMoved(inPos, inButtons);
}

//-----------------------------------------------------------------------------
VSTGUI::CMouseEventResult DGWebLink::onMouseMoved(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& /*inButtons*/)
{
	if (!isEditing())
	{
		return VSTGUI::kMouseEventNotHandled;
	}

	setValue(hitTest(inPos) ? getMax() : getMin());
	if (isDirty())
	{
		invalid();
	}

	return VSTGUI::kMouseEventHandled;
}

//-----------------------------------------------------------------------------
// The mouse behavior of our web link button is more like that of a standard 
// push button control:  the action occurs upon releasing the mouse button, 
// and only if the mouse pointer is still in the control's region at that point.  
// This allows the user to accidentally push the button, but avoid the 
// associated action (launching an URL) by moving the mouse pointer away 
// before releasing the mouse button.
VSTGUI::CMouseEventResult DGWebLink::onMouseUp(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& /*inButtons*/)
{
	setValue(getMin());
	if (isDirty())
	{
		invalid();
	}

	// only launch the URL if the mouse pointer is still in the button's region
	if (hitTest(inPos) && !mURL.empty())
	{
		dfx::LaunchURL(mURL);
	}

	endEdit();

	return VSTGUI::kMouseEventHandled;
}






#pragma mark -
#pragma mark DGSplashScreen

//-----------------------------------------------------------------------------
// "about" splash screen
//-----------------------------------------------------------------------------
DGSplashScreen::DGSplashScreen(DfxGuiEditor*	inOwnerEditor,
							   DGRect const&	inClickRegion, 
							   DGImage*			inSplashImage)
:	DGControl<VSTGUI::CSplashScreen>(inClickRegion, inOwnerEditor, dfx::kParameterID_Invalid, inSplashImage, inClickRegion)
{
	setTransparency(true);

	if (inSplashImage)
	{
		VSTGUI::CCoord const splashWidth = inSplashImage->getWidth();
		VSTGUI::CCoord const splashHeight = inSplashImage->getHeight();
		VSTGUI::CCoord const splashX = (inOwnerEditor->GetWidth() - splashWidth) / 2;
		VSTGUI::CCoord const splashY = (inOwnerEditor->GetHeight() - splashHeight) / 2;
		DGRect const splashRegion(splashX, splashY, splashWidth, splashHeight);
		setDisplayArea(splashRegion);
		if (modalView)
		{
			modalView->setViewSize(splashRegion, false);
		}
	}
}
