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

#include "dfxguibutton.h"


//-----------------------------------------------------------------------------
// DGButton
//-----------------------------------------------------------------------------
DGButton::DGButton(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion, DGImage * inImage, long inNumStates, 
					DfxGuiBottonMode inMode, bool inDrawMomentaryState)
:	CControl(*inRegion, inOwnerEditor, inParamID, inImage), 
	DGControl(this, inOwnerEditor)
{
	init(inNumStates, inMode, inDrawMomentaryState);
}

//-----------------------------------------------------------------------------
DGButton::DGButton(DfxGuiEditor * inOwnerEditor, DGRect * inRegion, DGImage * inImage, long inNumStates, 
					DfxGuiBottonMode inMode, bool inDrawMomentaryState)
:	CControl(*inRegion, inOwnerEditor, DFX_PARAM_INVALID_ID, inImage), 
	DGControl(this, inOwnerEditor)
{
	init(inNumStates, inMode, inDrawMomentaryState);
}

//-----------------------------------------------------------------------------
// common constructor stuff
void DGButton::init(long inNumStates, DfxGuiBottonMode inMode, bool inDrawMomentaryState)
{
	numStates = inNumStates;
	mode = inMode;
	drawMomentaryState = inDrawMomentaryState;

	orientation = kDGAxis_horizontal;
	mouseIsDown = false;
	userProcedure = NULL;
	userProcData = NULL;
	userReleaseProcedure = NULL;
	userReleaseProcData = NULL;
	useReleaseProcedureOnlyAtEndWithNoCancel = false;

	if (mode == kDGButtonType_picturereel)
		setMouseEnabled(false);

	if ( (mode == kDGButtonType_incbutton) || (mode == kDGButtonType_decbutton) )
		setWraparoundValues(true);
}

//-----------------------------------------------------------------------------
void DGButton::draw(CDrawContext * inContext)
{
	if (getBackground() != NULL)
	{
		long xoff = 0;
		if (drawMomentaryState && mouseIsDown)
			xoff = getBackground()->getWidth() / 2;
		const long yoff = getValue_i() * (getBackground()->getHeight() / numStates);

#if (VSTGUI_VERSION_MAJOR < 4)
		if ( getTransparency() )
			getBackground()->drawTransparent(inContext, size, CPoint(xoff, yoff));
		else
#endif
			getBackground()->draw(inContext, size, CPoint(xoff, yoff));
	}

#ifdef TARGET_API_RTAS
	getOwnerEditor()->drawControlHighlight(inContext, this);
#endif

	setDirty(false);
}

//-----------------------------------------------------------------------------
CMouseEventResult DGButton::onMouseDown(CPoint & inPos, const CButtonState & inButtons)
{
	if ( !inButtons.isLeftButton() )
		return kMouseEventNotHandled;

	beginEdit();

	entryValue = newValue = getValue_i();
	const long min = 0;
	const long max = numStates - 1;

	setMouseIsDown(true);

	switch (mode)
	{
		case kDGButtonType_pushbutton:
			newValue = max;
			entryValue = 0;	// just to make sure it's like that
			break;
		case kDGButtonType_incbutton:
			if (inButtons.getModifierState() & kAlt)
				newValue = entryValue - 1;
			else
				newValue = entryValue + 1;
			break;
		case kDGButtonType_decbutton:
			if (inButtons.getModifierState() & kAlt)
				newValue = entryValue + 1;
			else
				newValue = entryValue - 1;
			break;
		case kDGButtonType_radiobutton:
			if (orientation & kDGAxis_horizontal)
				newValue = (long) ((inPos.x - size.left) / (size.width() / numStates));
			else
				newValue = (long) ((inPos.y - size.top) / (size.height() / numStates));
			newValue += min;	// offset
			break;
		default:
			break;
	}

	// wrap around
	if ( getWraparoundValues() )
	{
		if (newValue > max)
			newValue = min;
		else if (newValue < min)
			newValue = max;
	}
	// limit
	else
	{
		if (newValue > max)
			newValue = max;
		else if (newValue < min)
			newValue = min;
	}
	if (newValue != entryValue)
	{
		setValue_i(newValue);
		if ( isDirty() && (getListener() != NULL) )
			getListener()->valueChanged(this);
	}

	if (userProcedure != NULL)
		userProcedure(newValue, userProcData);

	return kMouseEventHandled;
}

//-----------------------------------------------------------------------------
CMouseEventResult DGButton::onMouseMoved(CPoint & inPos, const CButtonState & inButtons)
{
	if ( !inButtons.isLeftButton() )
		return kMouseEventHandled;

	long currentValue = getValue_i();

	if ( size.pointInside(inPos) )
	{
		setMouseIsDown(true);

		if (mode == kDGButtonType_radiobutton)
		{
			if (orientation & kDGAxis_horizontal)
				newValue = (long) ((inPos.x - size.left) / (size.width() / numStates));
			else
				newValue = (long) ((inPos.y - size.top) / (size.height() / numStates));
			if (newValue >= numStates)
				newValue = numStates - 1;
			else if (newValue < 0)
				newValue = 0;
		}
		else
		{
			if ( (userProcedure != NULL) && (newValue != currentValue) )
				userProcedure(newValue, userProcData);
		}
		if (newValue != currentValue)
			setValue_i(newValue);
	}

	else
	{
		setMouseIsDown(false);
		if (mode == kDGButtonType_radiobutton)
		{
		}
		else
		{
			if (entryValue != currentValue)
			{
				setValue_i(entryValue);
				if ( (userReleaseProcedure != NULL) && !useReleaseProcedureOnlyAtEndWithNoCancel )
					userReleaseProcedure(entryValue, userReleaseProcData);
			}
		}
	}

	if ( isDirty() && (getListener() != NULL) )
		getListener()->valueChanged(this);

	return kMouseEventHandled;
}

//-----------------------------------------------------------------------------
CMouseEventResult DGButton::onMouseUp(CPoint & inPos, const CButtonState & inButtons)
{
	setMouseIsDown(false);

	if (mode == kDGButtonType_pushbutton)
		setValue_i(0);

	if ( isDirty() && (getListener() != NULL) )
		getListener()->valueChanged(this);

	if ( size.pointInside(inPos) )
	{
		if (userReleaseProcedure != NULL)
			userReleaseProcedure(getValue_i(), userReleaseProcData);
	}

	endEdit();

	return kMouseEventHandled;
}

//-----------------------------------------------------------------------------
bool DGButton::onWheel(const CPoint & inPos, const float & inDistance, const CButtonState & inButtons)
{
	if (! getMouseEnabled() )
		return false;
	// not controlling a parameter
	if (!isParameterAttached())
		return false;

	switch (mode)
	{
		case kDGButtonType_incbutton:
		case kDGButtonType_decbutton:
		case kDGButtonType_pushbutton:
			{
				CButtonState fakeButtons = inButtons;
				// go backwards
				if (inDistance < 0.0f)
					fakeButtons |= kAlt;
				CPoint fakePos(0, 0);
				onMouseDown(fakePos, fakeButtons);
				onMouseUp(fakePos, inButtons);
			}
			return true;

		default:
			return CControl::onWheel(inPos, inDistance, inButtons);	// XXX need an actual default implementation
	}
}

//-----------------------------------------------------------------------------
void DGButton::setMouseIsDown(bool newMouseState)
{
	const bool oldstate = mouseIsDown;
	mouseIsDown = newMouseState;
	if ( (oldstate != newMouseState) && drawMomentaryState )
		redraw();
}

//-----------------------------------------------------------------------------
void DGButton::setButtonImage(DGImage * inImage)
{
	setBackground(inImage);
}

//-----------------------------------------------------------------------------
long DGButton::getValue_i()
{
	if (isParameterAttached())
	{
		return getOwnerEditor()->dfxgui_ExpandParameterValue(getParameterID(), getValue());
	}
	else
	{
		float maxValue_f = (float) (numStates - 1);
		return (long) ((getValue() * maxValue_f) + kDfxParam_IntegerPadding);
	}
}

//-----------------------------------------------------------------------------
void DGButton::setValue_i(long inValue)
{
	float newValue_f = 0.0f;
	if (isParameterAttached())
	{
		newValue_f = getOwnerEditor()->dfxgui_ContractParameterValue(getParameterID(), (float)inValue);
	}
	else
	{
		long maxValue = numStates - 1;
		if (inValue >= maxValue)
			newValue_f = 1.0f;
		else if (inValue <= 0)
			newValue_f = 0.0f;
		else
		{
			float maxValue_f = (float)maxValue + kDfxParam_IntegerPadding;
			if (maxValue_f > 0.0f)	// avoid division by zero
				newValue_f = (float)inValue / maxValue_f;
		}
	}
	setValue(newValue_f);
}

//-----------------------------------------------------------------------------
void DGButton::setUserProcedure(DGButtonUserProcedure inProc, void * inUserData)
{
	userProcedure = inProc;
	userProcData = inUserData;
}

//-----------------------------------------------------------------------------
void DGButton::setUserReleaseProcedure(DGButtonUserProcedure inProc, void * inUserData, bool inOnlyAtEndWithNoCancel)
{
	userReleaseProcedure = inProc;
	userReleaseProcData = inUserData;
	useReleaseProcedureOnlyAtEndWithNoCancel = inOnlyAtEndWithNoCancel;
}






#pragma mark -
#pragma mark DGFineTuneButton

//-----------------------------------------------------------------------------
// Fine-tune Button
//-----------------------------------------------------------------------------
DGFineTuneButton::DGFineTuneButton(DfxGuiEditor *	inOwnerEditor,
									long			inParameterID, 
									DGRect *		inRegion,
									DGImage *		inImage, 
									float			inValueChangeAmount)
:	CControl(*inRegion, inOwnerEditor, inParameterID, inImage), 
	DGControl(this, inOwnerEditor)
{
	setValueChangeAmount(inValueChangeAmount);
	mouseIsDown = false;
}

//-----------------------------------------------------------------------------
void DGFineTuneButton::draw(CDrawContext * inContext)
{
	if (getBackground() != NULL)
	{
		const long yoff = mouseIsDown ? size.height() : 0;
#if (VSTGUI_VERSION_MAJOR < 4)
		if ( getTransparency() )
			getBackground()->drawTransparent(inContext, size, CPoint(0, yoff));
		else
#endif
			getBackground()->draw(inContext, size, CPoint(0, yoff));
	}

	setDirty(false);
}

//-----------------------------------------------------------------------------
CMouseEventResult DGFineTuneButton::onMouseDown(CPoint & inPos, const CButtonState & inButtons)
{
	if ( !inButtons.isLeftButton() )
		return kMouseEventNotHandled;

	beginEdit();

	// figure out all of the values that we'll be using
	entryValue = getValue();
	newValue = entryValue + valueChangeAmount;
	if (newValue > getMax())
		newValue = getMax();
	if (newValue < getMin())
		newValue = getMin();

	mouseIsDown = false;	// "dirty" it for onMouseMoved
	return onMouseMoved(inPos, inButtons);
}

//-----------------------------------------------------------------------------
CMouseEventResult DGFineTuneButton::onMouseMoved(CPoint & inPos, const CButtonState & inButtons)
{
	const bool oldMouseDown = mouseIsDown;

	if ( size.pointInside(inPos) )
	{
		mouseIsDown = true;
		if (!oldMouseDown)
		{
			if ( newValue != getValue() )
				setValue(newValue);
// XXX or do I prefer it not to do the mouse-down state when nothing is happening anyway?
//			else
//				redraw();	// at least make sure that redrawing occurs for mouseIsDown change
		}
	}

	else
	{
		mouseIsDown = false;
		if (oldMouseDown)
		{
			if ( entryValue != getValue() )
				setValue(entryValue);
			else
				redraw();	// at least make sure that redrawing occurs for mouseIsDown change
		}
	}

	if ( isDirty() && (getListener() != NULL) )
		getListener()->valueChanged(this);

	return kMouseEventHandled;
}

//-----------------------------------------------------------------------------
CMouseEventResult DGFineTuneButton::onMouseUp(CPoint & inPos, const CButtonState & inButtons)
{
	if (mouseIsDown)
	{
		mouseIsDown = false;
		redraw();
	}

	endEdit();

	return kMouseEventHandled;
}

//-----------------------------------------------------------------------------
void DGFineTuneButton::setValueChangeAmount(float inChangeAmount)
{
	DfxGuiEditor* editor = getOwnerEditor();
	if (editor != NULL)
	{
		valueChangeAmount = editor->dfxgui_ContractParameterValue(getParameterID(), 
								inChangeAmount + editor->GetParameter_minValue(getParameterID()));
	}
	else
		valueChangeAmount = inChangeAmount;
}






#pragma mark -

//-----------------------------------------------------------------------------
// DGValueSpot
//-----------------------------------------------------------------------------
DGValueSpot::DGValueSpot(DfxGuiEditor *	inOwnerEditor, 
							long		inParamID, 
							DGRect *	inRegion, 
							DGImage *	inImage, 
							double		inValue)
:	CControl(*inRegion, inOwnerEditor, inParamID, inImage), 
	DGControl(this, inOwnerEditor), 
	valueToSet(inOwnerEditor->dfxgui_ContractParameterValue(inParamID, inValue))
{
	setTransparency(true);
	if (inImage == NULL)
	{
		CPoint backgroundOffset(inRegion->left, inRegion->top);
		setBackOffset(backgroundOffset);
	}

	buttonIsPressed = false;
}

//------------------------------------------------------------------------
void DGValueSpot::draw(CDrawContext * inContext)
{
	if (getBackground() != NULL)
	{
		const long yoff = buttonIsPressed ? size.height() : 0;
#if (VSTGUI_VERSION_MAJOR < 4)
		if ( getTransparency() )
			getBackground()->drawTransparent(inContext, size, CPoint(0, yoff));
		else
#endif
			getBackground()->draw(inContext, size, CPoint(0, yoff));
	}

#ifdef TARGET_API_RTAS
	getOwnerEditor()->drawControlHighlight(inContext, this);
#endif

	setDirty(false);
}

//-----------------------------------------------------------------------------
CMouseEventResult DGValueSpot::onMouseDown(CPoint & inPos, const CButtonState & inButtons)
{
	if ( !inButtons.isLeftButton() )
		return kMouseEventNotHandled;

	beginEdit();

	oldMousePos(-1, -1);

	return onMouseMoved(inPos, inButtons);
}

//-----------------------------------------------------------------------------
CMouseEventResult DGValueSpot::onMouseMoved(CPoint & inPos, const CButtonState & inButtons)
{
	if (inButtons.isLeftButton())
	{
		const bool oldButtonIsPressed = buttonIsPressed;
		if ( size.pointInside(inPos) && !(size.pointInside(oldMousePos)) )
		{
			setValue(valueToSet);
			buttonIsPressed = true;
		}
		else if ( !(size.pointInside(inPos)) && size.pointInside(oldMousePos) )
		{
			buttonIsPressed = false;
		}
		oldMousePos = inPos;

		if ( isDirty() && (getListener() != NULL) )
			getListener()->valueChanged(this);
		if (oldButtonIsPressed != buttonIsPressed)
			redraw();
	}

	return kMouseEventHandled;
}

//-----------------------------------------------------------------------------
CMouseEventResult DGValueSpot::onMouseUp(CPoint & inPos, const CButtonState & inButtons)
{
	endEdit();

	buttonIsPressed = false;
	redraw();

	return kMouseEventHandled;
}






#pragma mark -

//-----------------------------------------------------------------------------
// DGWebLink
//-----------------------------------------------------------------------------
DGWebLink::DGWebLink(DfxGuiEditor *		inOwnerEditor,
						DGRect *		inRegion,
						DGImage *		inImage, 
						const char *	inURL)
:	DGButton(inOwnerEditor, inRegion, inImage, 2, kDGButtonType_pushbutton), 
	urlString(NULL)
{
	if (inURL != NULL)
	{
		urlString = (char*) malloc(strlen(inURL) + 4);
		strcpy(urlString, inURL);
	}
}

//-----------------------------------------------------------------------------
DGWebLink::~DGWebLink()
{
	if (urlString != NULL)
		free(urlString);
	urlString = NULL;
}

//-----------------------------------------------------------------------------
CMouseEventResult DGWebLink::onMouseDown(CPoint & inPos, const CButtonState & inButtons)
{
	if ( !inButtons.isLeftButton() )
		return kMouseEventNotHandled;

	return kMouseEventHandled;
}

//-----------------------------------------------------------------------------
CMouseEventResult DGWebLink::onMouseMoved(CPoint & inPos, const CButtonState & inButtons)
{
	return kMouseEventHandled;
}

//-----------------------------------------------------------------------------
// The mouse behaviour of our web link button is more like that of a standard 
// push button control:  the action occurs upon releasing the mouse button, 
// and only if the mouse pointer is still in the control's region at that point.  
// This allows the user to accidentally push the button, but avoid the 
// associated action (launching an URL) by moving the mouse pointer away 
// before releasing the mouse button.
CMouseEventResult DGWebLink::onMouseUp(CPoint & inPos, const CButtonState & inButtons)
{
	// only launch the URL if the mouse pointer is still in the button's area
	if ( size.pointInside(inPos) && (urlString != NULL) )
		launch_url(urlString);

	return kMouseEventHandled;
}






#pragma mark -
#pragma mark DGSplashScreen

//-----------------------------------------------------------------------------
// "about" splash screen
//-----------------------------------------------------------------------------
DGSplashScreen::DGSplashScreen(DfxGuiEditor *	inOwnerEditor,
								DGRect *		inClickRegion, 
								DGImage *		inSplashImage)
:	CSplashScreen(*inClickRegion, inOwnerEditor, DFX_PARAM_INVALID_ID, 
					inSplashImage, *inClickRegion), 
	DGControl(this, inOwnerEditor)
{
	setTransparency(true);
	CPoint backgroundOffset(inClickRegion->left, inClickRegion->top);
	setBackOffset(backgroundOffset);

	if (inSplashImage != NULL)
	{
		const CCoord splashWidth = inSplashImage->getWidth();
		const CCoord splashHeight = inSplashImage->getHeight();
		const CCoord splashX = (inOwnerEditor->GetWidth() - splashWidth) / 2;
		const CCoord splashY = (inOwnerEditor->GetHeight() - splashHeight) / 2;
		DGRect splashRegion(splashX, splashY, splashWidth, splashHeight);
		setDisplayArea(splashRegion);
		if (modalView != NULL)
			modalView->setViewSize(splashRegion, false);
	}
}
