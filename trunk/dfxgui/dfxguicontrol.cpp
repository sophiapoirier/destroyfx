#include "dfxguicontrol.h"
#include "dfxgui.h"


//-----------------------------------------------------------------------------
DGControl::DGControl(DfxGuiEditor *inOwnerEditor, AudioUnitParameterID inParamID, DGRect *inRegion)
:	ownerEditor(inOwnerEditor)
{
	auvp = AUVParameter(ownerEditor->GetEditAudioUnit(), inParamID, kAudioUnitScope_Global, (AudioUnitElement)0);
	AUVPattached = true;
	Range = auvp.ParamInfo().maxValue - auvp.ParamInfo().minValue;
	
	init(inRegion);
}

//-----------------------------------------------------------------------------
DGControl::DGControl(DfxGuiEditor *inOwnerEditor, DGRect *inRegion, float inRange)
:	ownerEditor(inOwnerEditor), Range(inRange)
{
	auvp = AUVParameter();	// an empty AUVParameter
	AUVPattached = false;

	init(inRegion);
}

//-----------------------------------------------------------------------------
// common constructor stuff
void DGControl::init(DGRect *inRegion)
{
	where.set(inRegion);
	vizArea.set(inRegion);

	carbonControl = NULL;
	auv_control = NULL;

	setContinuousControl(false);
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
// force a redraw
void DGControl::redraw()
{
	if (carbonControl != NULL)
		Draw1Control(carbonControl);
}

//-----------------------------------------------------------------------------
void DGControl::createAUVcontrol()
{
	AUCarbonViewControl::ControlType ctype;
	ctype = isContinuousControl() ? AUCarbonViewControl::kTypeContinuous : AUCarbonViewControl::kTypeDiscrete;
	if (auv_control != NULL)
		delete auv_control;
	auv_control = new DGCarbonViewControl(getDfxGuiEditor(), getDfxGuiEditor()->getParameterListener(), ctype, getAUVP(), carbonControl);
	auv_control->Bind();
}

//-----------------------------------------------------------------------------
void DGControl::setParameterID(AudioUnitParameterID inParameterID)
{
	if (inParameterID == 0xFFFFFFFF)	// XXX do something about this
		AUVPattached = false;
	else if (inParameterID != auvp.mParameterID)	// only do this if it's a change
	{
		AUVPattached = true;
		auvp = AUVParameter(getDfxGuiEditor()->GetEditAudioUnit(), inParameterID, kAudioUnitScope_Global, (AudioUnitElement)0);
		createAUVcontrol();
	}
}

//-----------------------------------------------------------------------------
long DGControl::getParameterID()
{
	if (isAUVPattached())
		return getAUVP().mParameterID;
	else
		return DFX_PARAM_INVALID_ID;
}

//-----------------------------------------------------------------------------
void DGControl::setOffset(SInt32 x, SInt32 y)
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
bool DGControl::isControlRef(ControlRef inControl)
{
	if (carbonControl == inControl)
		return true;
	return false;
}

//-----------------------------------------------------------------------------
void DGControl::setForeBounds(SInt32 x, SInt32 y, SInt32 w, SInt32 h)
{
	vizArea.set(x, y, w, h);
}

//-----------------------------------------------------------------------------
void DGControl::shrinkForeBounds(SInt32 inXoffset, SInt32 inYoffset, SInt32 inWidthShrink, SInt32 inHeightShrink)
{
	vizArea.offset(inXoffset, inYoffset, -inWidthShrink, -inHeightShrink);
}






#include "dfxpluginproperties.h"

//-----------------------------------------------------------------------------
DGCarbonViewControl::DGCarbonViewControl(AUCarbonViewBase *inOwnerView, AUParameterListenerRef inListener, 
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
