/*------------------------------------------------------------------------
Destroy FX Library (version 1.0) is a collection of foundation code 
for creating audio software plug-ins.  
Copyright (C) 2002-2009  Sophia Poirier

This program is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

This program is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with this program.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, please visit http://destroyfx.org/ 
and use the contact form.
------------------------------------------------------------------------*/

#include "dfxguibutton.h"


#pragma mark DGButton
//-----------------------------------------------------------------------------
// Button
//-----------------------------------------------------------------------------
DGButton::DGButton(DfxGuiEditor *		inOwnerEditor,
					long				inParameterID, 
					DGRect *			inRegion,
					DGImage *			inImage, 
					long				inNumStates, 
					DfxGuiBottonMode	inMode, 
					bool				inDrawMomentaryState)
:	DGControl(inOwnerEditor, inParameterID, inRegion), 
	buttonImage(inImage), numStates(inNumStates), mode(inMode), drawMomentaryState(inDrawMomentaryState)
{
	init();
}

//-----------------------------------------------------------------------------
DGButton::DGButton(DfxGuiEditor *		inOwnerEditor,
					DGRect *			inRegion,
					DGImage *			inImage, 
					long				inNumStates, 
					DfxGuiBottonMode	inMode, 
					bool				inDrawMomentaryState)
:	DGControl(inOwnerEditor, inRegion, ((inNumStates <= 0) ? 0.0f : (float)(inNumStates-1)) ), 
	buttonImage(inImage), numStates(inNumStates), mode(inMode), drawMomentaryState(inDrawMomentaryState)
{
	init();
}

//-----------------------------------------------------------------------------
// common constructor stuff
void DGButton::init()
{
	userProcedure = NULL;
	userProcData = NULL;
	userReleaseProcedure = NULL;
	userReleaseProcData = NULL;
	useReleaseProcedureOnlyAtEndWithNoCancel = false;

	orientation = kDGAxis_horizontal;

	mouseIsDown = false;
	setControlContinuous(false);

	if (mode == kDGButtonType_picturereel)
		setRespondToMouse(false);

	if ( (mode == kDGButtonType_incbutton) || (mode == kDGButtonType_decbutton) )
		setWraparoundValues(true);
}

//-----------------------------------------------------------------------------
DGButton::~DGButton()
{
}

//-----------------------------------------------------------------------------
void DGButton::post_embed()
{
	if ( isParameterAttached() && (carbonControl != NULL) )
		SetControl32BitMaximum( carbonControl, (SInt32) (getAUVP().ParamInfo().minValue) + (numStates - 1) );
}

//-----------------------------------------------------------------------------
void DGButton::draw(DGGraphicsContext * inContext)
{
	if (buttonImage != NULL)
	{
		long xoff = 0;
		if (drawMomentaryState && mouseIsDown)
			xoff = buttonImage->getWidth() / 2;
		long yoff = (GetControl32BitValue(carbonControl) - GetControl32BitMinimum(carbonControl)) * (buttonImage->getHeight() / numStates);

		buttonImage->draw(getBounds(), inContext, xoff, yoff);
	}
}

//-----------------------------------------------------------------------------
void DGButton::mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers, bool inIsDoubleClick)
{
	entryValue = newValue = GetControl32BitValue(carbonControl);
	SInt32 min = GetControl32BitMinimum(carbonControl);
	SInt32 max = GetControl32BitMaximum(carbonControl);

	setMouseIsDown(true);

	switch (mode)
	{
		case kDGButtonType_pushbutton:
			newValue = max;
			entryValue = 0;	// just to make sure it's like that
			break;
		case kDGButtonType_incbutton:
			if ( (inMouseButtons & (1<<1)) || (inKeyModifiers & kDGKeyModifier_alt) )
				newValue = entryValue - 1;
			else
				newValue = entryValue + 1;
			break;
		case kDGButtonType_decbutton:
			if ( (inMouseButtons & (1<<1)) || (inKeyModifiers & kDGKeyModifier_alt) )
				newValue = entryValue + 1;
			else
				newValue = entryValue - 1;
			break;
		case kDGButtonType_radiobutton:
			if (orientation & kDGAxis_horizontal)
				newValue = (long)inXpos / (getBounds()->w / numStates);
			else
				newValue = (long)inYpos / (getBounds()->h / numStates);
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
		SetControl32BitValue(carbonControl, newValue);

	if (userProcedure != NULL)
		userProcedure(newValue, userProcData);
}

//-----------------------------------------------------------------------------
void DGButton::mouseTrack(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers)
{
	SInt32 currentValue = GetControl32BitValue(carbonControl);

	if ( getBounds()->isInside_zerobase((long)inXpos, (long)inYpos) )
	{
		setMouseIsDown(true);

		if (mode == kDGButtonType_radiobutton)
		{
			if (orientation & kDGAxis_horizontal)
				newValue = (long)inXpos / (getBounds()->w / numStates);
			else
				newValue = (long)inYpos / (getBounds()->h / numStates);
			if (newValue >= numStates)
				newValue = numStates - 1;
			else if (newValue < 0)
				newValue = 0;
			newValue += GetControl32BitMinimum(carbonControl);	// offset
		}
		else
		{
			if ( (userProcedure != NULL) && (newValue != currentValue) )
				userProcedure(newValue, userProcData);
		}
		if (newValue != currentValue)
			SetControl32BitValue(carbonControl, newValue);
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
				SetControl32BitValue(carbonControl, entryValue);
				if ( (userReleaseProcedure != NULL) && !useReleaseProcedureOnlyAtEndWithNoCancel )
					userReleaseProcedure(GetControl32BitValue(carbonControl), userReleaseProcData);
			}
		}
	}
}

//-----------------------------------------------------------------------------
void DGButton::mouseUp(float inXpos, float inYpos, DGKeyModifiers inKeyModifiers)
{
	setMouseIsDown(false);

	if (mode == kDGButtonType_pushbutton)
	{
		if (GetControl32BitValue(carbonControl) != 0)
			SetControl32BitValue(carbonControl, 0);
	}

	if ( getBounds()->isInside_zerobase((long)inXpos, (long)inYpos) )
	{
		if (userReleaseProcedure != NULL)
			userReleaseProcedure(GetControl32BitValue(carbonControl), userReleaseProcData);
	}
}

//-----------------------------------------------------------------------------
void DGButton::setMouseIsDown(bool newMouseState)
{
	bool oldstate = mouseIsDown;
	mouseIsDown = newMouseState;
	if ( (oldstate != newMouseState) && drawMomentaryState )
		redraw();
}

//-----------------------------------------------------------------------------
bool DGButton::mouseWheel(long inDelta, DGAxis inAxis, DGKeyModifiers inKeyModifiers)
{
	switch (mode)
	{
		case kDGButtonType_incbutton:
		case kDGButtonType_decbutton:
		case kDGButtonType_pushbutton:
			{
				DGKeyModifiers fakeModifiers = inKeyModifiers;
				if (inDelta < 0)
					fakeModifiers |= kDGKeyModifier_alt;
				mouseDown(0.0f, 0.0f, 1, fakeModifiers, false);
				mouseUp(0.0f, 0.0f, inKeyModifiers);
			}
			return true;

		default:
			return DGControl::mouseWheel(inDelta, inAxis, inKeyModifiers);
	}
}

//-----------------------------------------------------------------------------
void DGButton::setButtonImage(DGImage * inImage)
{
	if (inImage != buttonImage)
	{
		buttonImage = inImage;
		redraw();
	}
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
:	DGControl(inOwnerEditor, inParameterID, inRegion), 
	buttonImage(inImage), valueChangeAmount(inValueChangeAmount)
{
	mouseIsDown = false;
	setControlContinuous(true);
}

//-----------------------------------------------------------------------------
DGFineTuneButton::~DGFineTuneButton()
{
}

//-----------------------------------------------------------------------------
void DGFineTuneButton::draw(DGGraphicsContext * inContext)
{
	if (buttonImage != NULL)
	{
		long yoff = (mouseIsDown) ? (buttonImage->getHeight() / 2) : 0;
		buttonImage->draw(getBounds(), inContext, 0, yoff);
	}
}

//-----------------------------------------------------------------------------
void DGFineTuneButton::mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers, bool inIsDoubleClick)
{
	// figure out all of the values that we'll be using
	entryValue = GetControl32BitValue(carbonControl);
	SInt32 min = GetControl32BitMinimum(carbonControl);
	SInt32 max = GetControl32BitMaximum(carbonControl);
	newValue = entryValue + (long) (valueChangeAmount * (float)(max - min));
	if (newValue > max)
		newValue = max;
	if (newValue < min)
		newValue = min;

	mouseIsDown = false;	// "dirty" it for mouseTrack
	mouseTrack(inXpos, inYpos, inMouseButtons, inKeyModifiers);
}

//-----------------------------------------------------------------------------
void DGFineTuneButton::mouseTrack(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers)
{
	bool oldMouseDown = mouseIsDown;

	if ( getBounds()->isInside_zerobase((long)inXpos, (long)inYpos) )
	{
		mouseIsDown = true;
		if (!oldMouseDown)
		{
			if ( newValue != GetControl32BitValue(carbonControl) )
				SetControl32BitValue(carbonControl, newValue);
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
			if ( entryValue != GetControl32BitValue(carbonControl) )
				SetControl32BitValue(carbonControl, entryValue);
			else
				redraw();	// at least make sure that redrawing occurs for mouseIsDown change
		}
	}

}

//-----------------------------------------------------------------------------
void DGFineTuneButton::mouseUp(float inXpos, float inYpos, DGKeyModifiers inKeyModifiers)
{
	if (mouseIsDown)
	{
		mouseIsDown = false;
		redraw();
	}
}






#pragma mark -
#pragma mark DGValueSpot

//-----------------------------------------------------------------------------
// Value Hot-Spot
//-----------------------------------------------------------------------------
DGValueSpot::DGValueSpot(DfxGuiEditor *	inOwnerEditor, 
						long			inParameterID, 
						DGRect *		inRegion, 
						DGImage *		inImage, 
						double			inValue)
:	DGControl(inOwnerEditor, inRegion, 1.0f), 
	buttonImage(inImage), valueToSet(inValue), 
	mParameterID_unattached(inParameterID)
{
}

//-----------------------------------------------------------------------------
void DGValueSpot::mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers, bool inIsDoubleClick)
{
	mouseTrack(inXpos, inYpos, inMouseButtons, inKeyModifiers);
}

//-----------------------------------------------------------------------------
void DGValueSpot::mouseTrack(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers)
{
	if ( getBounds()->isInside_zerobase((long)inXpos, (long)inYpos) )
	{
		if (! GetControl32BitValue(carbonControl) )
			getDfxGuiEditor()->setparameter_f(mParameterID_unattached, valueToSet, true);
		SetControl32BitValue(carbonControl, 1);
	}
	else
	{
		SetControl32BitValue(carbonControl, 0);
	}
}

//-----------------------------------------------------------------------------
void DGValueSpot::mouseUp(float inXpos, float inYpos, DGKeyModifiers inKeyModifiers)
{
	SetControl32BitValue(carbonControl, 0);
}

//-----------------------------------------------------------------------------
void DGValueSpot::draw(DGGraphicsContext * inContext)
{
	if (buttonImage != NULL)
	{
		long yoff = GetControl32BitValue(carbonControl) * (buttonImage->getHeight() / 2);
		buttonImage->draw(getBounds(), inContext, 0, yoff);
	}
}






#pragma mark -
#pragma mark DGWebLink

//-----------------------------------------------------------------------------
// Web Link
//-----------------------------------------------------------------------------
DGWebLink::DGWebLink(DfxGuiEditor * inOwnerEditor, DGRect * inRegion, DGImage * inImage, const char * inURL)
:	DGButton(inOwnerEditor, inRegion, inImage, 2, kDGButtonType_pushbutton), 
	urlString(NULL)
{
	if (inURL != NULL)
	{
		urlString = (char*) malloc(strlen(inURL) + 4);
		strcpy(urlString, inURL);
	}

	setRespondToMouseWheel(false);
}

//-----------------------------------------------------------------------------
DGWebLink::~DGWebLink()
{
	if (urlString != NULL)
		free(urlString);
	urlString = NULL;
}

// XXX we should really move stuff like launch_url into a separate DFX utilities file or something like that
#include "dfxplugin.h"	// for the launch_url function
//-----------------------------------------------------------------------------
// The mouse behaviour of our web link button is more like that of a standard 
// push button control:  the action occurs upon releasing the mouse button, 
// and only if the mouse pointer is still in the control's region at that point.  
// This allows the user to accidentally push the button, but avoid the 
// associated action (launching an URL) by moving the mouse pointer away 
// before releasing the mouse button.
void DGWebLink::mouseUp(float inXpos, float inYpos, DGKeyModifiers inKeyModifiers)
{
	// only launch the URL if the mouse pointer is still in the button's area
	if ( getMouseIsDown() && (urlString != NULL) )
		launch_url(urlString);

	DGButton::mouseUp(inXpos, inYpos, inKeyModifiers);
}






#pragma mark -
#pragma mark DGSplashScreen

//-----------------------------------------------------------------------------
// "about" splash screen
//-----------------------------------------------------------------------------
DGSplashScreen::DGSplashScreen(DfxGuiEditor *	inOwnerEditor,
								DGRect *		inRegion, 
								DGImage *		inSplashImage)
:	DGControl(inOwnerEditor, inRegion, 1), 
	splashImage(inSplashImage)
{
	splashDisplay = NULL;
}

//-----------------------------------------------------------------------------
void DGSplashScreen::mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers, bool inIsDoubleClick)
{
	if (splashDisplay == NULL)
	{
		CGRect editorRect = {0};
		HIViewGetBounds(getDfxGuiEditor()->GetCarbonPane(), &editorRect);
		if (splashImage != NULL)
		{
			long splashWidth = splashImage->getWidth();
			long splashHeight = splashImage->getHeight();
			long splashX = (editorRect.size.width - splashWidth) / 2;
			long splashY = (editorRect.size.height - splashHeight) / 2;
			DGRect splashRegion(splashX, splashY, splashWidth, splashHeight);
			splashDisplay = new DGSplashScreenDisplay(this, &splashRegion, splashImage);
			if (splashDisplay != NULL)
				splashDisplay->embed();
		}
	}
	else
	{
		if (splashDisplay->getCarbonControl() != NULL)
			ShowControl( splashDisplay->getCarbonControl() );
	}

	getDfxGuiEditor()->installSplashScreenControl(this);
}

//-----------------------------------------------------------------------------
void DGSplashScreen::removeSplashDisplay()
{
	if (splashDisplay != NULL)
	{
		if (splashDisplay->getCarbonControl() != NULL)
			HideControl( splashDisplay->getCarbonControl() );
	}
}

//-----------------------------------------------------------------------------
// splash screen display
//-----------------------------------------------------------------------------
DGSplashScreen::DGSplashScreenDisplay::DGSplashScreenDisplay(DGSplashScreen *	inParentControl, 
																DGRect *		inRegion, 
																DGImage *		inSplashImage)
:	DGControl(inParentControl->getDfxGuiEditor(), inRegion, 1), 
	parentSplashControl(inParentControl), splashImage(inSplashImage)
{
}

//-----------------------------------------------------------------------------
void DGSplashScreen::DGSplashScreenDisplay::draw(DGGraphicsContext * inContext)
{
	if (splashImage != NULL)
		splashImage->draw(getBounds(), inContext);
}
