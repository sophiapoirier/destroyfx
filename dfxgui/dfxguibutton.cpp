#include "dfxguibutton.h"


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

	mouseIsDown = false;
	setControlContinuous(false);
}

//-----------------------------------------------------------------------------
DGButton::~DGButton()
{
}

//-----------------------------------------------------------------------------
void DGButton::draw(CGContextRef inContext, long inPortHeight)
{
	if (buttonImage != NULL)
	{
		long xoff = 0;
		if (drawMomentaryState && mouseIsDown)
			xoff = buttonImage->getWidth() / 2;
		long yoff = GetControl32BitValue(getCarbonControl()) * (buttonImage->getHeight() / numStates);

		buttonImage->draw(getBounds(), inContext, inPortHeight, xoff, yoff);
	}
}

//-----------------------------------------------------------------------------
void DGButton::mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, unsigned long inKeyModifiers)
{
	if (mode == kDGButtonType_picturereel)
		return;

	entryValue = newValue = GetControl32BitValue(carbonControl);
	SInt32 max = GetControl32BitMaximum(carbonControl);

	setMouseIsDown(true);
	#if TARGET_PLUGIN_USES_MIDI
		if (isParameterAttached())
			getDfxGuiEditor()->setmidilearner(getParameterID());
	#endif

	switch (mode)
	{
		case kDGButtonType_pushbutton:
			newValue = max;
			entryValue = 0;	// just to make sure it's like that
			break;
		case kDGButtonType_incbutton:
			newValue = (newValue + 1) % (max + 1);
			break;
		case kDGButtonType_decbutton:
			newValue = (newValue - 1 + max) % (max + 1);
			break;
		case kDGButtonType_radiobutton:
			newValue = (long)inXpos / (getBounds()->w / numStates);
			if (newValue >= numStates)
				newValue = numStates - 1;
			else if (newValue < 0)
				newValue = 0;
			break;
		default:
			break;
	}

	if (newValue != entryValue)
		SetControl32BitValue(carbonControl, newValue);

	if (userProcedure != NULL)
		userProcedure(newValue, userProcData);
}

//-----------------------------------------------------------------------------
void DGButton::mouseTrack(float inXpos, float inYpos, unsigned long inMouseButtons, unsigned long inKeyModifiers)
{
	if (mode == kDGButtonType_picturereel)
		return;

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
		}
		else
		{
			if ( (userProcedure != NULL) && (newValue != currentValue) )
				userProcedure(newValue, userProcData);
		}
		if (newValue != currentValue)
			SetControl32BitValue(getCarbonControl(), newValue);
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
				SetControl32BitValue(getCarbonControl(), entryValue);
				if (userReleaseProcedure != NULL)
					userReleaseProcedure(GetControl32BitValue(carbonControl), userReleaseProcData);
			}
		}
	}

}

//-----------------------------------------------------------------------------
void DGButton::mouseUp(float inXpos, float inYpos, unsigned long inKeyModifiers)
{
	if (mode == kDGButtonType_picturereel)
		return;

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
void DGFineTuneButton::draw(CGContextRef inContext, long inPortHeight)
{
	if (buttonImage != NULL)
	{
		long yoff = (mouseIsDown) ? (buttonImage->getHeight() / 2) : 0;
		buttonImage->draw(getBounds(), inContext, inPortHeight, 0, yoff);
	}
}

//-----------------------------------------------------------------------------
void DGFineTuneButton::mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, unsigned long inKeyModifiers)
{
	#if TARGET_PLUGIN_USES_MIDI
		if (isParameterAttached())
			getDfxGuiEditor()->setmidilearner(getParameterID());
	#endif

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
void DGFineTuneButton::mouseTrack(float inXpos, float inYpos, unsigned long inMouseButtons, unsigned long inKeyModifiers)
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
void DGFineTuneButton::mouseUp(float inXpos, float inYpos, unsigned long inKeyModifiers)
{
	if (mouseIsDown)
	{
		mouseIsDown = false;
		redraw();
	}
}






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
void DGWebLink::mouseUp(float inXpos, float inYpos, unsigned long inKeyModifiers)
{
	// only launch the URL if the mouse pointer is still in the button's area
	if ( getMouseIsDown() && (urlString != NULL) )
		launch_url(urlString);

	DGButton::mouseUp(inXpos, inYpos, inKeyModifiers);
}
