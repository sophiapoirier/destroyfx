#include "dfxguibutton.h"


//-----------------------------------------------------------------------------
DGButton::DGButton(DfxGuiEditor *		inOwnerEditor,
					AudioUnitParameterID paramID, 
					DGRect *			inWhere,
					DGGraphic *			inForeGround, 
					DGGraphic *			inBackground, 
					long				inNumStates, 
					DfxGuiBottonMode	inMode, 
					bool				inKick)
:	DGControl(inOwnerEditor, paramID, inWhere), 
	numStates(inNumStates), kick(inKick)
{
	ForeGround = inForeGround;
	BackGround = inBackground;
	mode = inMode;

	userProcedure = NULL;
	userProcData = NULL;
	userReleaseProcedure = NULL;
	userReleaseProcData = NULL;

	alpha = 1.0f;
	mouseIsDown = false;
}

//-----------------------------------------------------------------------------
DGButton::DGButton(DfxGuiEditor *		inOwnerEditor,
					DGRect *			inWhere,
					DGGraphic *			inForeGround, 
					DGGraphic *			inBackground,
					long				inNumStates, 
					DfxGuiBottonMode	inMode, 
					bool				inKick)
:	DGControl(inOwnerEditor, inWhere, ((inNumStates < 1) ? 0.0f : (float)(inNumStates-1)) ), 
	numStates(inNumStates), kick(inKick)
{
	ForeGround = inForeGround;
	BackGround = inBackground;
	mode = inMode;

	userProcedure = NULL;
	userProcData = NULL;
	userReleaseProcedure = NULL;
	userReleaseProcData = NULL;

	alpha = 1.0f;
	mouseIsDown = false;
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
if (kick && mouseIsDown)
	bounds.origin.x -= (float) (CGImageGetWidth(theButton) / 2);
bounds.origin.y -= (float) ((max - value) * (CGImageGetHeight(theButton) / numStates));
//		CGContextSetAlpha(context, alpha);
		CGContextDrawImage(context, bounds, theButton);
	}
}

//-----------------------------------------------------------------------------
void DGButton::mouseDown(Point *P, bool with_option, bool with_shift)
{
	if (mode == kPictureReel)
		return;

	ControlRef carbonControl = getCarbonControl();

	SInt32 value = GetControl32BitValue(carbonControl);
	SInt32 max = GetControl32BitMaximum(carbonControl);
	
	setMouseIsDown(true);
	#if TARGET_PLUGIN_USES_MIDI
		if (isAUVPattached())
			getDfxGuiEditor()->setmidilearner(getAUVP().mParameterID);
	#endif

	switch (mode)
	{
		case kPushButton:
			value = max;
			break;
		case kIncButton:
			value = (value + 1) % (max + 1);
			break;
		case kDecButton:
			value = (value - 1 + max) % (max + 1);
			break;
		case kRadioButton:
			value = P->h / (getBounds()->w / numStates);
			if (value >= numStates)
				value = numStates - 1;
			else if (value < 0)
				value = 0;
			break;
		default:
			return;
	}
	SetControl32BitValue(carbonControl, value);

	if (userProcedure != NULL)
		userProcedure(value, userProcData);
}

//-----------------------------------------------------------------------------
void DGButton::mouseTrack(Point *P, bool with_option, bool with_shift)
{
	if (mode == kPictureReel)
		return;

	if (mode = kRadioButton)
	{
		SInt32 oldvalue = GetControl32BitValue(getCarbonControl());
		SInt32 newvalue = P->h / (getBounds()->w / numStates);
		if (newvalue >= numStates)
			newvalue = numStates - 1;
		else if (newvalue < 0)
			newvalue = 0;
		if (newvalue != oldvalue)
			SetControl32BitValue(getCarbonControl(), newvalue);
	}

	if ( (P->h >= 0) && (P->h <= getBounds()->w) && (P->v >= 0) && (P->v <= getBounds()->h) )
	{
//printf("mouse is down and in the zone\n");
		setMouseIsDown(true);
	}
	else
	{
//printf("mouse is down and out of it\n");
		setMouseIsDown(false);
	}
}

//-----------------------------------------------------------------------------
void DGButton::mouseUp(Point *P, bool with_option, bool with_shift)
{
	if (mode == kPictureReel)
		return;

	ControlRef carbonControl = getCarbonControl();
	
	setMouseIsDown(false);
	
	switch (mode)
	{
		case kPushButton:
			SetControl32BitValue(carbonControl, 0);
			break;
		case kIncButton:
			Draw1Control(carbonControl);
			break;
		case kDecButton:
			Draw1Control(carbonControl);
			break;
		default:
			break;
	}

	if (userReleaseProcedure != NULL)
		userReleaseProcedure(GetControl32BitValue(carbonControl), userReleaseProcData);
}

//-----------------------------------------------------------------------------
void DGButton::setMouseIsDown(bool newMouseState)
{
	if (mouseIsDown != newMouseState)
		pleaseUpdate = true;
	mouseIsDown = newMouseState;
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
