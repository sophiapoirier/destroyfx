#include "dfxguibutton.h"


//-----------------------------------------------------------------------------
DGButton::DGButton(DfxGuiEditor *		inOwnerEditor,
					AudioUnitParameterID inParamID, 
					DGRect *			inWhere,
					DGGraphic *			inForeGround, 
					DGGraphic *			inBackground, 
					long				inNumStates, 
					DfxGuiBottonMode	inMode, 
					bool				inDrawMomentaryState)
:	DGControl(inOwnerEditor, inParamID, inWhere), 
	numStates(inNumStates), mode(inMode), drawMomentaryState(inDrawMomentaryState)
{
	ForeGround = inForeGround;
	BackGround = inBackground;

	userProcedure = NULL;
	userProcData = NULL;
	userReleaseProcedure = NULL;
	userReleaseProcData = NULL;

	alpha = 1.0f;
	mouseIsDown = false;
	setType(kDfxGuiType_Button);
	setContinuousControl(false);
}

//-----------------------------------------------------------------------------
DGButton::DGButton(DfxGuiEditor *		inOwnerEditor,
					DGRect *			inWhere,
					DGGraphic *			inForeGround, 
					DGGraphic *			inBackground,
					long				inNumStates, 
					DfxGuiBottonMode	inMode, 
					bool				inDrawMomentaryState)
:	DGControl(inOwnerEditor, inWhere, ((inNumStates <= 0) ? 0.0f : (float)(inNumStates-1)) ), 
	numStates(inNumStates), mode(inMode), drawMomentaryState(inDrawMomentaryState)
{
	ForeGround = inForeGround;
	BackGround = inBackground;

	userProcedure = NULL;
	userProcData = NULL;
	userReleaseProcedure = NULL;
	userReleaseProcData = NULL;

	alpha = 1.0f;
	mouseIsDown = false;
	setType(kDfxGuiType_Button);
	setContinuousControl(false);
}

//-----------------------------------------------------------------------------
DGButton::~DGButton()
{
	ForeGround = NULL;
	BackGround = NULL;
}

//-----------------------------------------------------------------------------
void DGButton::draw(CGContextRef context, UInt32 portHeight)
{
	SInt32 value = GetControl32BitValue(getCarbonControl());
	SInt32 max = GetControl32BitMaximum(getCarbonControl());
	CGRect bounds;
	getBounds()->copyToCGRect(&bounds, portHeight);

	CGImageRef theBack = (BackGround == NULL) ? NULL : BackGround->getImage();
	if (theBack != NULL)
		CGContextDrawImage(context, bounds, theBack);

	CGImageRef theButton = (ForeGround == NULL) ? NULL : ForeGround->getImage();
	if (theButton != NULL)
	{
bounds.size.width = CGImageGetWidth(theButton);
bounds.size.height = CGImageGetHeight(theButton);
if (drawMomentaryState && mouseIsDown)
	bounds.origin.x -= (float) (CGImageGetWidth(theButton) / 2);
bounds.origin.y -= (float) ((max - value) * (CGImageGetHeight(theButton) / numStates));
//		CGContextSetAlpha(context, alpha);
		CGContextDrawImage(context, bounds, theButton);
	}
}

//-----------------------------------------------------------------------------
void DGButton::mouseDown(Point inPos, bool with_option, bool with_shift)
{
	if (mode == kPictureReel)
		return;

	ControlRef carbonControl = getCarbonControl();
	entryValue = newValue = GetControl32BitValue(carbonControl);
	SInt32 max = GetControl32BitMaximum(carbonControl);
	
	setMouseIsDown(true);
	#if TARGET_PLUGIN_USES_MIDI
		if (isAUVPattached())
			getDfxGuiEditor()->setmidilearner(getAUVP().mParameterID);
	#endif

	switch (mode)
	{
		case kPushButton:
			newValue = max;
			entryValue = 0;	// just to make sure it's like that
			break;
		case kIncButton:
			newValue = (newValue + 1) % (max + 1);
			break;
		case kDecButton:
			newValue = (newValue - 1 + max) % (max + 1);
			break;
		case kRadioButton:
			newValue = inPos.h / (getBounds()->w / numStates);
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
void DGButton::mouseTrack(Point inPos, bool with_option, bool with_shift)
{
	if (mode == kPictureReel)
		return;

	ControlRef carbonControl = getCarbonControl();
	SInt32 currentValue = GetControl32BitValue(carbonControl);

	if ( (inPos.h >= 0) && (inPos.h <= getBounds()->w) && (inPos.v >= 0) && (inPos.v <= getBounds()->h) )
	{
		setMouseIsDown(true);

		if (mode == kRadioButton)
		{
			newValue = inPos.h / (getBounds()->w / numStates);
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
		if (mode == kRadioButton)
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
void DGButton::mouseUp(Point inPos, bool with_option, bool with_shift)
{
	if (mode == kPictureReel)
		return;

	ControlRef carbonControl = getCarbonControl();
	
	setMouseIsDown(false);
	
	if (mode == kPushButton)
	{
		if (GetControl32BitValue(carbonControl) != 0)
			SetControl32BitValue(carbonControl, 0);
	}

	if ( (inPos.h >= 0) && (inPos.h <= getBounds()->w) && (inPos.v >= 0) && (inPos.v <= getBounds()->h) )
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
void DGButton::setUserProcedure(buttonUserProcedure inProc, void *inUserData)
{
	userProcedure = inProc;
	userProcData = inUserData;
}

//-----------------------------------------------------------------------------
void DGButton::setUserReleaseProcedure(buttonUserProcedure inProc, void *inUserData)
{
	userReleaseProcedure = inProc;
	userReleaseProcData = inUserData;
}
