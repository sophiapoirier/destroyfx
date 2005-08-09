#include "dfxguicontrol.h"
#include "dfxgui.h"

#include "dfxpluginproperties.h"


const SInt32 kContinuousControlMaxValue = 0x0FFFFFFF - 1;
const float kDefaultFineTuneFactor = 12.0f;
const float kDefaultMouseDragRange = 333.0f;	// pixels

//-----------------------------------------------------------------------------
DGControl::DGControl(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion)
:	ownerEditor(inOwnerEditor)
{
	auvp = CAAUParameter(ownerEditor->GetEditAudioUnit(), (AudioUnitParameterID)inParamID, kAudioUnitScope_Global, (AudioUnitElement)0);
	parameterAttached = true;
	Range = auvp.ParamInfo().maxValue - auvp.ParamInfo().minValue;
	
	init(inRegion);
}

//-----------------------------------------------------------------------------
DGControl::DGControl(DfxGuiEditor * inOwnerEditor, DGRect * inRegion, float inRange)
:	ownerEditor(inOwnerEditor), Range(inRange)
{
	auvp = CAAUParameter();	// an empty CAAUParameter
	parameterAttached = false;

	init(inRegion);
}

//-----------------------------------------------------------------------------
// common constructor stuff
void DGControl::init(DGRect * inRegion)
{
	where.set(inRegion);
	vizArea.set(inRegion);

#if MAC
	carbonControl = NULL;
	helpText = NULL;
	mouseTrackingRegion = NULL;
	isFirstDraw = true;
#endif
#ifdef TARGET_API_AUDIOUNIT
	auv_control = NULL;
#endif

	isContinuous = false;
	fineTuneFactor = kDefaultFineTuneFactor;
	mouseDragRange = kDefaultMouseDragRange;

	shouldRespondToMouse = true;
	shouldRespondToMouseWheel = true;
	currentlyIgnoringMouseTracking = false;
	shouldWraparoundValues = false;

	drawAlpha = 1.0f;

	// add this control to the owner editor's list of controls
	getDfxGuiEditor()->addControl(this);
}

//-----------------------------------------------------------------------------
DGControl::~DGControl()
{
#if MAC
	if (mouseTrackingRegion != NULL)
		ReleaseMouseTrackingRegion(mouseTrackingRegion);
	mouseTrackingRegion = NULL;
#endif
	if (auv_control != NULL)
		delete auv_control;
	auv_control = NULL;
	if (carbonControl != NULL)
		DisposeControl(carbonControl);
	carbonControl = NULL;
}


//-----------------------------------------------------------------------------
void DGControl::do_draw(CGContextRef inContext, long inPortHeight)
{
#if MAC
	// XXX a hack to workaround creating mouse tracking regions when 
	// the portion of the window where the control is doesn't exist yet
	if (isFirstDraw)
		initMouseTrackingRegion();
	isFirstDraw = false;
#endif

	// redraw the background behind the control in case the control background has any transparency
	// this is handled automatically if the window is in compositing mode, though
	if ( !(getDfxGuiEditor()->IsCompositWindow()) )
		getDfxGuiEditor()->DrawBackground(inContext, inPortHeight);

	CGContextSetAlpha(inContext, drawAlpha);

// XXX quicky hack to work around compositing problems...  can I think of a better solution?
DGRect oldbounds(getBounds());
DGRect oldfbounds(getForeBounds());
FixControlCompositingOffset(getBounds(), carbonControl, getDfxGuiEditor());
FixControlCompositingOffset(getForeBounds(), carbonControl, getDfxGuiEditor());
	// then have the child control class do its drawing
	draw(inContext, inPortHeight);
getBounds()->set(&oldbounds);
getForeBounds()->set(&oldfbounds);
}

//-----------------------------------------------------------------------------
// force a redraw
void DGControl::redraw()
{
	if (carbonControl != NULL)
	{
		if ( getDfxGuiEditor()->IsCompositWindow() )
			HIViewSetNeedsDisplay(carbonControl, true);
		else
			Draw1Control(carbonControl);
	}
}

//-----------------------------------------------------------------------------
void DGControl::embed()
{
	setOffset( (long) (getDfxGuiEditor()->GetXOffset()), (long) (getDfxGuiEditor()->GetYOffset()) );
	Rect carbonControlRect;
	getBounds()->copyToRect(&carbonControlRect);
	ControlRef newCarbonControl = NULL;
	OSStatus error = CreateCustomControl(getDfxGuiEditor()->GetCarbonWindow(), &carbonControlRect, 
									getDfxGuiEditor()->getControlDefSpec(), NULL, &newCarbonControl);
	if ( (error != noErr) || (newCarbonControl == NULL) )
		return;	// XXX what else can we do?

	setCarbonControl(newCarbonControl);
	initCarbonControlValueRange();

	// XXX do I maybe want to do this after creating the AUVControl (to make ControlInitialize happen better)?
	getDfxGuiEditor()->EmbedControl(newCarbonControl);
	if ( isParameterAttached() )
		createAUVcontrol();
	else
		SetControl32BitValue(newCarbonControl, GetControl32BitMinimum(newCarbonControl));
/*
UInt32 feat = 0;
GetControlFeatures(newCarbonControl, &feat);
for (int i=0; i < 32; i++)
{
if (feat & (1 << i)) printf("control feature bit %d is active\n", i);
}
*/

	// if the help text was set before we created the Carbon control, 
	// then we retained it so that we can set it now, having created the Carbon control
	if (helpText != NULL)
	{
		setHelpText(helpText);
		CFRelease(helpText);
	}
	helpText = NULL;

	// call any child control class' extra init stuff
	post_embed();
}

//-----------------------------------------------------------------------------
void DGControl::createAUVcontrol()
{
	if (carbonControl == NULL)
		return;

	AUCarbonViewControl::ControlType ctype = isContinuousControl() ? AUCarbonViewControl::kTypeContinuous : AUCarbonViewControl::kTypeDiscrete;
	if (auv_control != NULL)
		delete auv_control;
	auv_control = new DGCarbonViewControl(getDfxGuiEditor(), getDfxGuiEditor()->getParameterListener(), ctype, getAUVP(), carbonControl);
	auv_control->Bind();
}

//-----------------------------------------------------------------------------
void DGControl::setControlContinuous(bool inContinuity)
{
	bool oldContinuity = isContinuous;
	isContinuous = inContinuity;
	if (inContinuity != oldContinuity)
	{
		// do this before doing the value range thing to avoid spurious parameter value changing
		// XXX causes crashes - fix
//		if (auv_control != NULL)
//			delete auv_control;
		initCarbonControlValueRange();
		createAUVcontrol();
	}
}

//-----------------------------------------------------------------------------
void DGControl::initCarbonControlValueRange()
{
	if (carbonControl == NULL)
		return;

	if ( isContinuousControl() )
	{
		SetControl32BitMinimum(carbonControl, 0);
		SetControl32BitMaximum(carbonControl, kContinuousControlMaxValue);
	}
	else
	{
		if ( isParameterAttached() )
		{
			SetControl32BitMinimum( carbonControl, (SInt32) (getAUVP().ParamInfo().minValue) );
			SetControl32BitMaximum( carbonControl, (SInt32) (getAUVP().ParamInfo().maxValue) );
		}
		else
		{
			SetControl32BitMinimum(carbonControl, 0);
			SetControl32BitMaximum(carbonControl, (SInt32) (getRange() + 0.001f) );
		}
	}
}

#if MAC
//-----------------------------------------------------------------------------
void DGControl::initMouseTrackingRegion()
{
	if (getCarbonControl() == NULL)
		return;

	// handle control mouse-overs by creating mouse tracking region for the control
	RgnHandle controlRegion = NewRgn();
	if (controlRegion != NULL)
	{
		OSStatus status = GetControlRegion(getCarbonControl(), kControlEntireControl, controlRegion);
		if (status != noErr)
		{
			Rect mouseRegionBounds;
			GetControlBounds(getCarbonControl(), &mouseRegionBounds);
			if ( getDfxGuiEditor()->IsCompositWindow() )
			{
				Rect paneBounds;
				GetControlBounds(getDfxGuiEditor()->GetCarbonPane(), &paneBounds);
				OffsetRect(&mouseRegionBounds, paneBounds.left, paneBounds.top);
			}
			RectRgn(controlRegion, &mouseRegionBounds);
		}
		MouseTrackingRegionID mouseTrackingRegionID;
		mouseTrackingRegionID.signature = DESTROYFX_ID;
		mouseTrackingRegionID.id = (SInt32)this;
		EventTargetRef targetToNotify = GetControlEventTarget(getCarbonControl());	// can be NULL (which means use the window's event target)
		status = CreateMouseTrackingRegion(getDfxGuiEditor()->GetCarbonWindow(), controlRegion, NULL, kMouseTrackingOptionsLocalClip, mouseTrackingRegionID, this, targetToNotify, &mouseTrackingRegion);
		DisposeRgn(controlRegion);
	}
}
#endif

//-----------------------------------------------------------------------------
void DGControl::do_mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers, bool inIsDoubleClick)
{
	if (! getRespondToMouse() )
		return;

	currentlyIgnoringMouseTracking = false;

	#if TARGET_PLUGIN_USES_MIDI
		if ( isParameterAttached() )
			getDfxGuiEditor()->setmidilearner( getParameterID() );
	#endif

	// set the defaul value of the parameter
	if ( (inKeyModifiers & kDGKeyModifier_accel) && isParameterAttached() )
	{
		getDfxGuiEditor()->setparameter_default( getParameterID() );
		currentlyIgnoringMouseTracking = true;
		return;
	}

	mouseDown(inXpos, inYpos, inMouseButtons, inKeyModifiers, inIsDoubleClick);
}

//-----------------------------------------------------------------------------
void DGControl::do_mouseTrack(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers)
{
	if (! getRespondToMouse() )
		return;

	if (!currentlyIgnoringMouseTracking)
		mouseTrack(inXpos, inYpos, inMouseButtons, inKeyModifiers);
}

//-----------------------------------------------------------------------------
void DGControl::do_mouseUp(float inXpos, float inYpos, DGKeyModifiers inKeyModifiers)
{
	if (! getRespondToMouse() )
		return;

	if (!currentlyIgnoringMouseTracking)
		mouseUp(inXpos, inYpos, inKeyModifiers);

	currentlyIgnoringMouseTracking = false;
}

//-----------------------------------------------------------------------------
bool DGControl::do_mouseWheel(long inDelta, DGMouseWheelAxis inAxis, DGKeyModifiers inKeyModifiers)
{
	if (! getRespondToMouseWheel() )
		return false;
	if (! getRespondToMouse() )
		return false;

	return mouseWheel(inDelta, inAxis, inKeyModifiers);
}

//-----------------------------------------------------------------------------
// a default implementation of mouse wheel handling that should work for most controls
bool DGControl::mouseWheel(long inDelta, DGMouseWheelAxis inAxis, DGKeyModifiers inKeyModifiers)
{
	SInt32 min = GetControl32BitMinimum(carbonControl);
	SInt32 max = GetControl32BitMaximum(carbonControl);
	SInt32 oldValue = GetControl32BitValue(carbonControl);
	SInt32 newValue = oldValue;

	if ( isContinuousControl() )
	{
		float diff = (float)inDelta;
		if (inKeyModifiers & kDGKeyModifier_shift)	// slo-mo
			diff /= getFineTuneFactor();
		newValue = oldValue + (SInt32)(diff * (float)(max-min) / getMouseDragRange());
	}
	else
	{
		if (inDelta > 0)
			newValue = oldValue + 1;
		else if (inDelta < 0)
			newValue = oldValue - 1;

		// wrap around
		if ( getWraparoundValues() )
		{
			if (newValue > max)
				newValue = min;
			else if (newValue < min)
				newValue = max;
		}
	}

	if (newValue > max)
		newValue = max;
	if (newValue < min)
		newValue = min;
	if (newValue != oldValue)
		SetControl32BitValue(carbonControl, newValue);

	return true;
}

//-----------------------------------------------------------------------------
void DGControl::setParameterID(long inParameterID)
{
	if (inParameterID == DFX_PARAM_INVALID_ID)
	{
		parameterAttached = false;
		if (auv_control != NULL)
			delete auv_control;
	}
	else if ( !parameterAttached || (inParameterID != getParameterID()) )	// only do this if it's a change
	{
		parameterAttached = true;
		auvp = CAAUParameter(getDfxGuiEditor()->GetEditAudioUnit(), (AudioUnitParameterID)inParameterID, 
							kAudioUnitScope_Global, (AudioUnitElement)0);
		createAUVcontrol();
		redraw();	// it might not happen if the new parameter value is the same as the old value, so make sure it happens
	}
}

//-----------------------------------------------------------------------------
long DGControl::getParameterID()
{
	if (isParameterAttached())
		return getAUVP().mParameterID;
	else
		return DFX_PARAM_INVALID_ID;
}

//-----------------------------------------------------------------------------
void DGControl::setOffset(long x, long y)
{
	where.offset(x, y);
	vizArea.offset(x, y);
}

//-----------------------------------------------------------------------------
void DGControl::setVisible(bool inVisibility)
{
	if (carbonControl != NULL)
	{
		if (inVisibility)
		{
			ShowControl(carbonControl);
			redraw();
		}
		else
			HideControl(carbonControl);
	}
}

//-----------------------------------------------------------------------------
void DGControl::setDrawAlpha(float inAlpha)
{
	float oldalpha = drawAlpha;
	drawAlpha = inAlpha;
	if (oldalpha != inAlpha)
		redraw();
}

//-----------------------------------------------------------------------------
bool DGControl::isControlRef(ControlRef inControl)
{
	if (carbonControl == inControl)
		return true;
	return false;
}

//-----------------------------------------------------------------------------
void DGControl::setForeBounds(long x, long y, long w, long h)
{
	vizArea.set(x, y, w, h);
}

//-----------------------------------------------------------------------------
void DGControl::shrinkForeBounds(long inXoffset, long inYoffset, long inWidthShrink, long inHeightShrink)
{
	vizArea.offset(inXoffset, inYoffset, -inWidthShrink, -inHeightShrink);
}

//-----------------------------------------------------------------------------
OSStatus DGControl::setHelpText(CFStringRef inHelpText)
{
	if (inHelpText == NULL)
		return paramErr;

	// get rid of any previously retained help string
	if ( (helpText != NULL) && (helpText != inHelpText) )
	{
		CFRelease(helpText);
		helpText = NULL;
	}

	if (carbonControl == NULL)
	{
		// the Carbon control is probably null because it has not been created yet, 
		// so we can retain the string and then try setting the help text after 
		// we have created the Carbon control
		// I actually create a copy rather than retain in case the CFString is a 
		// CFSTR compile-time constant string, which you're apparently not supposed to CFRetain()
		helpText = CFStringCreateCopy(kCFAllocatorDefault, inHelpText);
		return errItemNotControl;
	}

	HMHelpContentRec helpContent;
	memset(&helpContent, 0, sizeof(helpContent));
	helpContent.version = kMacHelpVersion;
	MacSetRect(&(helpContent.absHotRect), 0, 0, 0, 0);
	helpContent.tagSide = kHMDefaultSide;
	helpContent.content[kHMMinimumContentIndex].contentType = kHMCFStringContent;
	helpContent.content[kHMMinimumContentIndex].u.tagCFString = inHelpText;
	helpContent.content[kHMMaximumContentIndex].contentType = kHMNoContent;

	return HMSetControlHelpContent(carbonControl, &helpContent);
}






//-----------------------------------------------------------------------------
DGCarbonViewControl::DGCarbonViewControl(AUCarbonViewBase * inOwnerView, AUParameterListenerRef inListener, 
										ControlType inType, const CAAUParameter &inAUVParam, ControlRef inControl)
:	AUCarbonViewControl(inOwnerView, inListener, inType, inAUVParam, inControl)
{
}

//-----------------------------------------------------------------------------
void DGCarbonViewControl::ControlToParameter()
{
	if (mType == kTypeContinuous)
	{
		double controlValue = GetValueFract();
		Float32 paramValue;
		DfxParameterValueConversionRequest request;
		UInt32 dataSize = sizeof(request);
		request.parameterID = mParam.mParameterID;
		request.conversionType = kDfxParameterValueConversion_expand;
		request.inValue = controlValue;
		if (AudioUnitGetProperty(GetOwnerView()->GetEditAudioUnit(), kDfxPluginProperty_ParameterValueConversion, 
								kAudioUnitScope_Global, (AudioUnitElement)0, &request, &dataSize) 
								== noErr)
			paramValue = request.outValue;
		else
			paramValue = AUParameterValueFromLinear(controlValue, &mParam);
		mParam.SetValue(mListener, this, paramValue);
	}
	else
		AUCarbonViewControl::ControlToParameter();
}

//-----------------------------------------------------------------------------
void DGCarbonViewControl::ParameterToControl(Float32 paramValue)
{
	if (mType == kTypeContinuous)
	{
		DfxParameterValueConversionRequest request;
		UInt32 dataSize = sizeof(request);
		request.parameterID = mParam.mParameterID;
		request.conversionType = kDfxParameterValueConversion_contract;
		request.inValue = paramValue;
		if (AudioUnitGetProperty(GetOwnerView()->GetEditAudioUnit(), kDfxPluginProperty_ParameterValueConversion, 
								kAudioUnitScope_Global, (AudioUnitElement)0, &request, &dataSize) 
								== noErr)
			SetValueFract(request.outValue);
		else
			SetValueFract(AUParameterValueToLinear(paramValue, &mParam));
	}
	else
		AUCarbonViewControl::ParameterToControl(paramValue);
}






#if MAC
//-----------------------------------------------------------------------------
CGPoint GetControlCompositingOffset(ControlRef inControl, DfxGuiEditor * inEditor)
{
	CGPoint offset;
	offset.x = offset.y = 0.0f;
	if ( (inControl == NULL) || (inEditor == NULL) )
		return offset;
	if ( !(inEditor->IsCompositWindow()) )
		return offset;

	OSStatus error;
	HIRect controlRect;
	error = HIViewGetBounds(inControl, &controlRect);
	if (error == noErr)
	{
		HIRect frameRect;
		error = HIViewGetFrame(inControl, &frameRect);
		if (error == noErr)
		{
			offset.x = controlRect.origin.x - frameRect.origin.x;
			offset.y = controlRect.origin.y - frameRect.origin.y;
		}
	}
	return offset;
}

//-----------------------------------------------------------------------------
void FixControlCompositingOffset(DGRect * inRect, ControlRef inControl, DfxGuiEditor * inEditor)
{
	if (inRect != NULL)
	{
		CGPoint offsetAmount = GetControlCompositingOffset(inControl, inEditor);
		inRect->offset( (long)(offsetAmount.x), (long)(offsetAmount.y) );
	}
}
#endif
