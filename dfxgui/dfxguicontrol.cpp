#include "dfxguicontrol.h"
#include "dfxgui.h"


//-----------------------------------------------------------------------------
DGControl::DGControl(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion)
:	ownerEditor(inOwnerEditor)
{
	auvp = AUVParameter(ownerEditor->GetEditAudioUnit(), (AudioUnitParameterID)inParamID, kAudioUnitScope_Global, (AudioUnitElement)0);
	parameterAttached = true;
	Range = auvp.ParamInfo().maxValue - auvp.ParamInfo().minValue;
	
	init(inRegion);
}

//-----------------------------------------------------------------------------
DGControl::DGControl(DfxGuiEditor * inOwnerEditor, DGRect * inRegion, float inRange)
:	ownerEditor(inOwnerEditor), Range(inRange)
{
	auvp = AUVParameter();	// an empty AUVParameter
	parameterAttached = false;

	init(inRegion);
}

//-----------------------------------------------------------------------------
// common constructor stuff
void DGControl::init(DGRect * inRegion)
{
	where.set(inRegion);
	vizArea.set(inRegion);

	carbonControl = NULL;
	auv_control = NULL;

	isContinuous = false;
	fineTuneFactor = 12.0f;

	drawAlpha = 1.0f;

	// add this control to the owner editor's list of controls
	getDfxGuiEditor()->addControl(this);
}

//-----------------------------------------------------------------------------
DGControl::~DGControl()
{
	if (carbonControl != NULL)
		DisposeControl(carbonControl);
	if (auv_control != NULL)
		delete auv_control;
}


//-----------------------------------------------------------------------------
void DGControl::do_draw(CGContextRef inContext, long inPortHeight)
{
	// redraw the background behind the control in case the control background has any transparency
	// this is handled automatically if the window is in compositing mode, though
	if ( !(getDfxGuiEditor()->IsWindowCompositing()) )
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
		if ( getDfxGuiEditor()->IsWindowCompositing() )
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
		SetControl32BitMaximum(carbonControl, 0x3FFFFFFF);
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
		auvp = AUVParameter(getDfxGuiEditor()->GetEditAudioUnit(), (AudioUnitParameterID)inParameterID, 
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






#include "dfxpluginproperties.h"

//-----------------------------------------------------------------------------
DGCarbonViewControl::DGCarbonViewControl(AUCarbonViewBase * inOwnerView, AUParameterListenerRef inListener, 
										ControlType inType, const AUVParameter &inAUVParam, ControlRef inControl)
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
	if ( !(inEditor->IsWindowCompositing()) )
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
