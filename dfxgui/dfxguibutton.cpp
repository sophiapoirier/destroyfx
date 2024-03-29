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

#include "dfxguibutton.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <utility>

#include "dfxguieditor.h"
#include "dfxmath.h"


using namespace std::placeholders;


#pragma mark DGButton

//-----------------------------------------------------------------------------
// DGButton
//-----------------------------------------------------------------------------
DGButton::DGButton(DfxGuiEditor* inOwnerEditor, dfx::ParameterID inParameterID, DGRect const& inRegion, DGImage* inImage, 
				   Mode inMode, bool inDrawMomentaryState)
:	DGControl<VSTGUI::CControl>(inRegion, inOwnerEditor, dfx::ParameterID_ToVST(inParameterID), inImage), 
	mMode(inMode), 
	mDrawMomentaryState(inDrawMomentaryState),
	mWraparoundValues((mMode == Mode::Increment) || (mMode == Mode::Decrement))
{
	setMouseEnabled(mMode != Mode::PictureReel);
}

//-----------------------------------------------------------------------------
DGButton::DGButton(DfxGuiEditor* inOwnerEditor, DGRect const& inRegion, DGImage* inImage, 
				   size_t inNumStates, Mode inMode, bool inDrawMomentaryState)
:	DGButton(inOwnerEditor, dfx::kParameterID_Invalid, inRegion, inImage, inMode, inDrawMomentaryState)
{
	constexpr size_t minNumStates = 2;
	assert(inNumStates >= minNumStates);
	setNumStates(std::max(inNumStates, minNumStates));
}

//-----------------------------------------------------------------------------
void DGButton::draw(VSTGUI::CDrawContext* inContext)
{
	if (auto const image = getDrawBackground())
	{
		long const xoff = (mDrawMomentaryState && mMouseIsDown) ? (std::lround(image->getWidth()) / 2) : 0;
		long const yoff = getValue_i() * (std::lround(image->getHeight()) / dfx::math::ToSigned(getNumStates()));

		image->draw(inContext, getViewSize(), VSTGUI::CPoint(xoff, yoff));
	}

#ifdef TARGET_API_RTAS
	getOwnerEditor()->drawControlHighlight(inContext, this);
#endif

	setDirty(false);
}

//-----------------------------------------------------------------------------
void DGButton::onMouseDownEvent(VSTGUI::MouseDownEvent& ioEvent)
{
	if (!ioEvent.buttonState.isLeft())
	{
		return;
	}

	beginEdit();

	mEntryValue = mNewValue = getValue_i();
	auto const isDirectionReversed = ioEvent.modifiers.has(VSTGUI::ModifierKey::Alt);

	setMouseIsDown(true);

	switch (mMode)
	{
		case Mode::Momentary:
			mNewValue = getMaxValue();
			mEntryValue = kMinValue;  // just to make sure it's like that
			break;
		case Mode::Increment:
			mNewValue = mEntryValue + (isDirectionReversed ? -1 : 1);
			break;
		case Mode::Decrement:
			mNewValue = mEntryValue + (isDirectionReversed ? 1 : -1);
			break;
		case Mode::Radio:
			mNewValue = getRadioValue(ioEvent.mousePosition) + kMinValue;
			break;
		default:
			break;
	}

	mNewValue = constrainValue(mNewValue);
	if (mNewValue != mEntryValue)
	{
		setValue_i(mNewValue);
		notifyIfChanged();
		if (mUserProcedure)
		{
			mUserProcedure(mNewValue);
		}
	}

	ioEvent.consumed = true;
}

//-----------------------------------------------------------------------------
void DGButton::onMouseMoveEvent(VSTGUI::MouseMoveEvent& ioEvent)
{
	if (!isEditing())
	{
		return;
	}

	if (!ioEvent.buttonState.isLeft())
	{
		ioEvent.consumed = true;
		return;
	}

	auto const currentValue = getValue_i();

	if (hitTest(ioEvent.mousePosition))
	{
		setMouseIsDown(true);

		if (mMode == Mode::Radio)
		{
			mNewValue = getRadioValue(ioEvent.mousePosition);
		}
		if (mNewValue != currentValue)
		{
			setValue_i(mNewValue);
			if (mUserProcedure && (mNewValue != currentValue))
			{
				mUserProcedure(mNewValue);
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
					mUserReleaseProcedure(mEntryValue);
				}
			}
		}
	}

	notifyIfChanged();

	ioEvent.consumed = true;
}

//-----------------------------------------------------------------------------
void DGButton::onMouseUpEvent(VSTGUI::MouseUpEvent& ioEvent)
{
	setMouseIsDown(false);

	if (mMode == Mode::Momentary)
	{
		setValue_i(mEntryValue);
	}

	notifyIfChanged();

	if (hitTest(ioEvent.mousePosition))
	{
		if (mUserReleaseProcedure)
		{
			mUserReleaseProcedure(getValue_i());
		}
	}

	endEdit();

	ioEvent.consumed = true;
}

//-----------------------------------------------------------------------------
void DGButton::onMouseCancelEvent(VSTGUI::MouseCancelEvent& ioEvent)
{
	if (isEditing())
	{
		setValue_i(mEntryValue);
		notifyIfChanged();
		endEdit();
	}

	ioEvent.consumed = true;
}

//-----------------------------------------------------------------------------
void DGButton::onMouseWheelEvent(VSTGUI::MouseWheelEvent& ioEvent)
{
	// not controlling a parameter
	if (!isParameterAttached())
	{
		return;
	}

	onMouseWheelEditing();

	long newValue {};
	if (mMode == Mode::Momentary)
	{
		newValue = getMaxValue();
		setValue_i(newValue);
		if (isDirty())
		{
			valueChanged();
		}
		setValue_i(kMinValue);
	}
	else
	{
		auto const delta = detail::mouseWheelEventIntegralCompositeDelta(ioEvent);
		newValue = constrainValue(getValue_i() + delta);
		if (newValue != getValue_i())
		{
			setValue_i(newValue);
		}
	}

	if (notifyIfChanged())
	{
		if (mUserProcedure)
		{
			mUserProcedure(newValue);
		}
		if (mUserReleaseProcedure)
		{
			mUserReleaseProcedure(getValue_i());
		}
	}

	ioEvent.consumed = true;
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
void DGButton::setUserProcedure(UserProcedure const& inProc)
{
	assert(inProc);
	mUserProcedure = inProc;
}

//-----------------------------------------------------------------------------
void DGButton::setUserProcedure(UserProcedure&& inProc)
{
	assert(inProc);
	mUserProcedure = std::move(inProc);
}

//-----------------------------------------------------------------------------
void DGButton::setUserReleaseProcedure(UserProcedure const& inProc, bool inOnlyAtEndWithNoCancel)
{
	assert(inProc);
	mUserReleaseProcedure = inProc;
	mUseReleaseProcedureOnlyAtEndWithNoCancel = inOnlyAtEndWithNoCancel;
}

//-----------------------------------------------------------------------------
void DGButton::setUserReleaseProcedure(UserProcedure&& inProc, bool inOnlyAtEndWithNoCancel)
{
	assert(inProc);
	mUserReleaseProcedure = std::move(inProc);
	mUseReleaseProcedureOnlyAtEndWithNoCancel = inOnlyAtEndWithNoCancel;
}

void DGButton::setOrientation(dfx::Axis inOrientation) noexcept
{
	assert(mMode == Mode::Radio);  // the only applicable mode
	mOrientation = inOrientation;
}

//-----------------------------------------------------------------------------
long DGButton::constrainValue(long inValue) const
{
	auto const max = getMaxValue();

	if (mWraparoundValues)
	{
		if (inValue > max)
		{
			return kMinValue;
		}
		else if (inValue < kMinValue)
		{
			return max;
		}
	}

	return std::clamp(inValue, kMinValue, max);
}

//-----------------------------------------------------------------------------
void DGButton::setRadioThresholds(std::vector<VSTGUI::CCoord> const& inThresholds)
{
	assert(mMode == Mode::Radio);
	assert((inThresholds.size() + 1) == getNumStates());
	assert(std::ranges::all_of(inThresholds, std::bind(std::less<>{}, _1, getMouseableRange())));
	assert(std::ranges::all_of(inThresholds, std::bind(std::greater<>{}, _1, 0)));

	mRadioThresholds = inThresholds;
}

//-----------------------------------------------------------------------------
long DGButton::getMaxValue() const
{
	return static_cast<long>(getNumStates()) - 1;
}

//-----------------------------------------------------------------------------
long DGButton::getRadioValue(VSTGUI::CPoint const& inPos) const
{
	assert(mMode == Mode::Radio);

	auto const result = [inPos, this]() -> long
	{
		auto const pos = (mOrientation & dfx::kAxis_Horizontal) ? (inPos.x - getViewSize().left) : (inPos.y - getViewSize().top);
		if (!mRadioThresholds.empty())
		{
			return std::ranges::count_if(mRadioThresholds, std::bind(std::less_equal<>{}, _1, pos));
		}
		return std::lround(pos) / (getMouseableRange() / dfx::math::ToSigned(getNumStates()));
	}();
	return std::clamp(result, kMinValue, getMaxValue());
}

//-----------------------------------------------------------------------------
long DGButton::getMouseableRange() const
{
	assert(mMode == Mode::Radio);
	return std::lround((mOrientation & dfx::kAxis_Horizontal) ? getWidth() : getHeight());
}






#pragma mark -
#pragma mark DGToggleImageButton

//-----------------------------------------------------------------------------
// Toggle Button
//-----------------------------------------------------------------------------
DGToggleImageButton::DGToggleImageButton(DfxGuiEditor* inOwnerEditor, 
										 dfx::ParameterID inParameterID, 
										 VSTGUI::CCoord inXpos, VSTGUI::CCoord inYpos, 
										 DGImage* inImage, 
										 bool inDrawMomentaryState)
:	DGButton(inOwnerEditor, inParameterID, makeRegion(inXpos, inYpos, inImage, inDrawMomentaryState), inImage, 
			 Mode::Increment, inDrawMomentaryState)
{
	assert(inImage);
}

//-----------------------------------------------------------------------------
DGToggleImageButton::DGToggleImageButton(DfxGuiEditor* inOwnerEditor, 
										 VSTGUI::CCoord inXpos, VSTGUI::CCoord inYpos, 
										 DGImage* inImage, 
										 bool inDrawMomentaryState)
:	DGButton(inOwnerEditor, makeRegion(inXpos, inYpos, inImage, inDrawMomentaryState), inImage, 
			 kNumStates, Mode::Increment, inDrawMomentaryState)
{
	assert(inImage);
}

//-----------------------------------------------------------------------------
DGRect DGToggleImageButton::makeRegion(VSTGUI::CCoord inXpos, VSTGUI::CCoord inYpos, DGImage* inImage, bool inDrawMomentaryState)
{
	auto const width = inImage->getWidth() / (inDrawMomentaryState ? 2 : 1);
	return DGRect(inXpos, inYpos, width, inImage->getHeight() / kNumStates);
}






#pragma mark -
#pragma mark DGFineTuneButton

//-----------------------------------------------------------------------------
// Fine-tune Button
//-----------------------------------------------------------------------------
DGFineTuneButton::DGFineTuneButton(DfxGuiEditor*	inOwnerEditor,
								   dfx::ParameterID	inParameterID, 
								   DGRect const&	inRegion,
								   DGImage*			inImage, 
								   float			inValueChangeAmount)
:	DGControl<VSTGUI::CControl>(inRegion, inOwnerEditor, dfx::ParameterID_ToVST(inParameterID), inImage), 
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
void DGFineTuneButton::onMouseDownEvent(VSTGUI::MouseDownEvent& ioEvent)
{
	if (!ioEvent.buttonState.isLeft())
	{
		return;
	}

	beginEdit();

	// figure out all of the values that we'll be using
	mEntryValue = getValue();
	mNewValue = std::clamp(mEntryValue + mValueChangeAmount, getMin(), getMax());

	mMouseIsDown = false;  // "dirty" it for onMouseMoveEvent

	VSTGUI::MouseMoveEvent mouseMoveEvent(ioEvent);
	onMouseMoveEvent(mouseMoveEvent);
	ioEvent.consumed = mouseMoveEvent.consumed;
}

//-----------------------------------------------------------------------------
void DGFineTuneButton::onMouseMoveEvent(VSTGUI::MouseMoveEvent& ioEvent)
{
	if (!isEditing())
	{
		return;
	}

	auto const oldMouseDown = mMouseIsDown;
	mMouseIsDown = hitTest(ioEvent.mousePosition);

	if (mMouseIsDown && !oldMouseDown)
	{
		if (mNewValue != getValue())
		{
			setValue(mNewValue);
		}
		else
		{
//			redraw();  // at least make sure that redrawing occurs for mMouseIsDown change
			// XXX or do I prefer it not to do the mouse-down state when nothing is happening anyway?
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

	notifyIfChanged();

	ioEvent.consumed = true;
}

//-----------------------------------------------------------------------------
void DGFineTuneButton::onMouseUpEvent(VSTGUI::MouseUpEvent& ioEvent)
{
	if (std::exchange(mMouseIsDown, false))
	{
		redraw();
	}

	endEdit();

	ioEvent.consumed = true;
}

//-----------------------------------------------------------------------------
void DGFineTuneButton::onMouseCancelEvent(VSTGUI::MouseCancelEvent& ioEvent)
{
	if (isEditing())
	{
		setValue(mEntryValue);
		notifyIfChanged();
		endEdit();
	}

	ioEvent.consumed = true;
}






#pragma mark -
#pragma mark DGValueSpot

//-----------------------------------------------------------------------------
// DGValueSpot
//-----------------------------------------------------------------------------
DGValueSpot::DGValueSpot(DfxGuiEditor*		inOwnerEditor, 
						 dfx::ParameterID	inParameterID, 
						 DGRect const&		inRegion, 
						 DGImage*			inImage, 
						 double				inValue)
:	DGControl<VSTGUI::CControl>(inRegion, inOwnerEditor, dfx::ParameterID_ToVST(inParameterID), inImage), 
	mValueToSet(inOwnerEditor->dfxgui_ContractParameterValue(inParameterID, inValue))
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
void DGValueSpot::onMouseDownEvent(VSTGUI::MouseDownEvent& ioEvent)
{
	if (!ioEvent.buttonState.isLeft())
	{
		return;
	}

	beginEdit();

	mEntryValue = getValue();
	mLastMousePos(-1, -1);

	VSTGUI::MouseMoveEvent mouseMoveEvent(ioEvent);
	onMouseMoveEvent(mouseMoveEvent);
	ioEvent.consumed = mouseMoveEvent.consumed;
}

//-----------------------------------------------------------------------------
void DGValueSpot::onMouseMoveEvent(VSTGUI::MouseMoveEvent& ioEvent)
{
	if (!isEditing())
	{
		return;
	}

	if (ioEvent.buttonState.isLeft())
	{
		auto const oldButtonIsPressed = mButtonIsPressed;
		if (hitTest(ioEvent.mousePosition) && !hitTest(mLastMousePos))
		{
			setValue(mValueToSet);
			mButtonIsPressed = true;
		}
		else if (!hitTest(ioEvent.mousePosition) && hitTest(mLastMousePos))
		{
			mButtonIsPressed = false;
		}
		mLastMousePos = ioEvent.mousePosition;

		notifyIfChanged();
		if (oldButtonIsPressed != mButtonIsPressed)
		{
			redraw();
		}
	}

	ioEvent.consumed = true;
}

//-----------------------------------------------------------------------------
void DGValueSpot::onMouseUpEvent(VSTGUI::MouseUpEvent& ioEvent)
{
	endEdit();

	mButtonIsPressed = false;
	redraw();

	ioEvent.consumed = true;
}

//-----------------------------------------------------------------------------
void DGValueSpot::onMouseCancelEvent(VSTGUI::MouseCancelEvent& ioEvent)
{
	if (isEditing())
	{
		setValue(mEntryValue);
		notifyIfChanged();
		endEdit();
	}

	ioEvent.consumed = true;
}






#pragma mark -
#pragma mark DGWebLink

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
void DGWebLink::onMouseDownEvent(VSTGUI::MouseDownEvent& ioEvent)
{
	if (!ioEvent.buttonState.isLeft())
	{
		return;
	}

	beginEdit();

	VSTGUI::MouseMoveEvent mouseMoveEvent(ioEvent);
	onMouseMoveEvent(mouseMoveEvent);
	ioEvent.consumed = mouseMoveEvent.consumed;
}

//-----------------------------------------------------------------------------
void DGWebLink::onMouseMoveEvent(VSTGUI::MouseMoveEvent& ioEvent)
{
	if (!isEditing())
	{
		return;
	}

	setValue(hitTest(ioEvent.mousePosition) ? getMax() : getMin());
	if (isDirty())
	{
		invalid();
	}

	ioEvent.consumed = true;
}

//-----------------------------------------------------------------------------
// The mouse behavior of our web link button is more like that of a standard 
// push button control:  the action occurs upon releasing the mouse button, 
// and only if the mouse pointer is still in the control's region at that point.  
// This allows the user to accidentally push the button, but avoid the 
// associated action (launching an URL) by moving the mouse pointer away 
// before releasing the mouse button.
void DGWebLink::onMouseUpEvent(VSTGUI::MouseUpEvent& ioEvent)
{
	setValue(getMin());
	if (isDirty())
	{
		invalid();
	}

	// only launch the URL if the mouse pointer is still in the button's region
	if (hitTest(ioEvent.mousePosition) && !mURL.empty())
	{
		dfx::LaunchURL(mURL);
	}

	endEdit();

	ioEvent.consumed = true;
}

//-----------------------------------------------------------------------------
void DGWebLink::onMouseCancelEvent(VSTGUI::MouseCancelEvent& ioEvent)
{
	if (isEditing())
	{
		setValue(getMin());
		if (isDirty())
		{
			invalid();
		}
		endEdit();
	}

	ioEvent.consumed = true;
}






#pragma mark -
#pragma mark DGSplashScreen

//-----------------------------------------------------------------------------
// "about" splash screen
//-----------------------------------------------------------------------------
DGSplashScreen::DGSplashScreen(DfxGuiEditor*	inOwnerEditor,
							   DGRect const&	inClickRegion, 
							   DGImage*			inSplashImage)
:	DGControl<VSTGUI::CSplashScreen>(inClickRegion, inOwnerEditor, dfx::ParameterID_ToVST(dfx::kParameterID_Invalid), inSplashImage, inClickRegion)
{
	assert(inSplashImage);

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
