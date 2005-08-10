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
			newValue = (long)inXpos / (getBounds()->w / numStates);
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
			newValue = (long)inXpos / (getBounds()->w / numStates);
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
bool DGButton::mouseWheel(long inDelta, DGMouseWheelAxis inAxis, DGKeyModifiers inKeyModifiers)
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
void DGButton::setValue(long inValue)
{
	if (getValue() != inValue)
		SetControl32BitValue(carbonControl, inValue);
}

//-----------------------------------------------------------------------------
long DGButton::getValue()
{
	return GetControl32BitValue(carbonControl);
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
void DGButton::setUserProcedure(buttonUserProcedure inProc, void * inUserData)
{
	userProcedure = inProc;
	userProcData = inUserData;
}

//-----------------------------------------------------------------------------
void DGButton::setUserReleaseProcedure(buttonUserProcedure inProc, void * inUserData)
{
	userReleaseProcedure = inProc;
	userReleaseProcData = inUserData;
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
