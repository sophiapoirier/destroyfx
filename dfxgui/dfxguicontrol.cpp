/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2018  Sophia Poirier

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

#include "dfxguicontrol.h"

#include "dfxguieditor.h"
#include "dfxmisc.h"



#ifdef TARGET_PLUGIN_USES_VSTGUI

//-----------------------------------------------------------------------------
DGControl::DGControl(CControl* inControl, DfxGuiEditor* inOwnerEditor)
:	mControl(inControl),
	mOwnerEditor(inOwnerEditor) 
{
}

//-----------------------------------------------------------------------------
void DGControl::setValue_gen(float inValue)
{
	getCControl()->setValue(inValue);
}

//-----------------------------------------------------------------------------
void DGControl::setDefaultValue_gen(float inValue)
{
	getCControl()->setDefaultValue(inValue);
}

//-----------------------------------------------------------------------------
void DGControl::redraw()
{
//	getCControl()->invalid(); // XXX CView::invalid calls setDirty(false), which can have undesired consequences for control value handling
	getCControl()->invalidRect(getCControl()->getViewSize());
}

//-----------------------------------------------------------------------------
long DGControl::getParameterID() const
{
	return getCControl()->getTag();
}

//-----------------------------------------------------------------------------
void DGControl::setParameterID(long inParameterID)
{
	getCControl()->setTag(inParameterID);
	getCControl()->setValue(mOwnerEditor->getparameter_gen(inParameterID));
	getCControl()->setDirty();  // it might not happen if the new parameter value is the same as the old value, so make sure it happens
}

//-----------------------------------------------------------------------------
bool DGControl::isParameterAttached() const
{
	return (getParameterID() >= 0);
}

//-----------------------------------------------------------------------------
void DGControl::setDrawAlpha(float inAlpha)
{
	getCControl()->setAlphaValue(inAlpha);
}

//-----------------------------------------------------------------------------
float DGControl::getDrawAlpha() const
{
	return getCControl()->getAlphaValue();
}

//-----------------------------------------------------------------------------
bool DGControl::setHelpText(char const* inText)
{
	assert(inText);
	return getCControl()->setAttribute(kCViewTooltipAttribute, strlen(inText) + 1, inText);
}

#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
bool DGControl::setHelpText(CFStringRef inText)
{
	assert(inText);
	if (auto const cString = dfx::CreateCStringFromCFString(inText))
	{
		return setHelpText(cString.get());
	}
	return false;
}
#endif



#pragma mark -
#pragma mark DGNullControl

//-----------------------------------------------------------------------------
DGNullControl::DGNullControl(DfxGuiEditor* inOwnerEditor, DGRect const& inRegion, DGImage* inBackgroundImage)
:	CControl(inRegion, inOwnerEditor, dfx::kParameterID_Invalid, inBackgroundImage), 
	DGControl(this, inOwnerEditor)
{
	setMouseEnabled(false);
}

//-----------------------------------------------------------------------------
void DGNullControl::draw(CDrawContext* inContext)
{
	if (getBackground())
	{
		getBackground()->draw(inContext, getViewSize());
	}
	setDirty(false);
}

#else  // !TARGET_PLUGIN_USES_VSTGUI



#pragma mark -
#pragma mark DGControl old

//-----------------------------------------------------------------------------
#ifdef TARGET_API_RTAS
#if 0	// XXX do this?
int32_t CControl::kZoomModifier = kControl;
int32_t CControl::kDefaultValueModifier = kShift;
#endif
#endif

//-----------------------------------------------------------------------------
DGControl::DGControl(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion)
:	CControl(*inRegion, inOwnerEditor, inParamID)
{
#ifdef TARGET_API_AUDIOUNIT
	auvp = CAAUParameter(ownerEditor->GetEditAudioUnit(), (AudioUnitParameterID)inParamID, kAudioUnitScope_Global, (AudioUnitElement)0);
	parameterAttached = true;
	valueRange = auvp.ParamInfo().maxValue - auvp.ParamInfo().minValue;
#else
	valueRange = 1.0f;	// XXX implement
#endif

	init(inRegion);
}

//-----------------------------------------------------------------------------
DGControl::DGControl(DfxGuiEditor * inOwnerEditor, DGRect * inRegion, float inRange)
:	CControl(*inRegion, inOwnerEditor, DFX_PARAM_INVALID_ID), 
	valueRange(inRange)
{
#ifdef TARGET_API_AUDIOUNIT
	parameterAttached = false;
#endif

	init(inRegion);
}

//-----------------------------------------------------------------------------
// common constructor stuff
void DGControl::init(DGRect * inRegion)
{
	controlPos = *inRegion;
	vizArea = *inRegion;

	isContinuous = false;
	fineTuneFactor = kDefaultFineTuneFactor;
	mouseDragRange = kDefaultMouseDragRange;

	shouldRespondToMouse = true;
	shouldRespondToMouseWheel = true;
	currentlyIgnoringMouseTracking = false;
	shouldWraparoundValues = false;

	drawAlpha = 1.0f;

	// add this control to the owner editor's list of controls
	if (getDfxGuiEditor() != NULL)
		getDfxGuiEditor()->addControl(this);
}


//-----------------------------------------------------------------------------
// force a redraw
void DGControl::redraw()
{
	invalid();
}

//-----------------------------------------------------------------------------
void DGControl::setControlContinuous(bool inContinuity)
{
	bool oldContinuity = isContinuous;
	isContinuous = inContinuity;
	if (inContinuity != oldContinuity)
	{
		// XXX do anything?
	}
}

//-----------------------------------------------------------------------------
void DGControl::do_mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, dfx::KeyModifiers inKeyModifiers, bool inIsDoubleClick)
{
	if (! getRespondToMouse() )
		return;

	currentlyIgnoringMouseTracking = false;

	// do this to make touch automation work
	// AUCarbonViewControl::HandleEvent will catch ControlClick but not ControlContextualMenuClick
	if ( isParameterAttached() )//&& (inEventKind == kEventControlContextualMenuClick) )
		getDfxGuiEditor()->automationgesture_begin( getParameterID() );

#if TARGET_PLUGIN_USES_MIDI
	if (isParameterAttached())
		getDfxGuiEditor()->setmidilearner( getParameterID() );
#endif

	// set the default value of the parameter
	if ( (inKeyModifiers & dfx::kKeyModifier_Accel) && isParameterAttached() )
	{
		getDfxGuiEditor()->setparameter_default( getParameterID() );
		currentlyIgnoringMouseTracking = true;
		return;
	}

	mouseDown(inXpos, inYpos, inMouseButtons, inKeyModifiers, inIsDoubleClick);
}

//-----------------------------------------------------------------------------
void DGControl::do_mouseTrack(float inXpos, float inYpos, unsigned long inMouseButtons, dfx::KeyModifiers inKeyModifiers)
{
	if (! getRespondToMouse() )
		return;

	if (!currentlyIgnoringMouseTracking)
		mouseTrack(inXpos, inYpos, inMouseButtons, inKeyModifiers);
}

//-----------------------------------------------------------------------------
void DGControl::do_mouseUp(float inXpos, float inYpos, dfx::KeyModifiers inKeyModifiers)
{
	if (! getRespondToMouse() )
		return;

	if (!currentlyIgnoringMouseTracking)
		mouseUp(inXpos, inYpos, inKeyModifiers);

	currentlyIgnoringMouseTracking = false;

	// do this to make touch automation work
	if ( isParameterAttached() )
		getDfxGuiEditor()->automationgesture_end( getParameterID() );
}

//-----------------------------------------------------------------------------
bool DGControl::do_mouseWheel(long inDelta, dfx::Axis inAxis, dfx::KeyModifiers inKeyModifiers)
{
	if (! getRespondToMouseWheel() )
		return false;
	if (! getRespondToMouse() )
		return false;

	if ( isParameterAttached() )
		getDfxGuiEditor()->automationgesture_begin( getParameterID() );

	bool wheelResult = mouseWheel(inDelta, inAxis, inKeyModifiers);

	if ( isParameterAttached() )
		getDfxGuiEditor()->automationgesture_end( getParameterID() );

	return wheelResult;
}

//-----------------------------------------------------------------------------
// a default implementation of mouse wheel handling that should work for most controls
bool DGControl::mouseWheel(long inDelta, dfx::Axis inAxis, dfx::KeyModifiers inKeyModifiers)
{
ControlRef carbonControl = NULL;	// XXX just quieting errors for now
	SInt32 min = GetControl32BitMinimum(carbonControl);
	SInt32 max = GetControl32BitMaximum(carbonControl);
	SInt32 oldValue = GetControl32BitValue(carbonControl);
	SInt32 newValue = oldValue;

	if ( isContinuousControl() )
	{
		float diff = (float)inDelta;
		if (inKeyModifiers & dfx::kKeyModifier_Shift)	// slo-mo
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
#ifdef TARGET_API_AUDIOUNIT
		if (auv_control != NULL)
			delete auv_control;
		auv_control = NULL;
#endif
	}
	else if ( !parameterAttached || (inParameterID != getParameterID()) )	// only do this if it's a change
	{
		parameterAttached = true;
#ifdef TARGET_API_AUDIOUNIT
		auvp = CAAUParameter(getDfxGuiEditor()->GetEditAudioUnit(), (AudioUnitParameterID)inParameterID, 
							kAudioUnitScope_Global, (AudioUnitElement)0);
		createAUVcontrol();
#endif
		redraw();	// it might not happen if the new parameter value is the same as the old value, so make sure it happens
	}
}

//-----------------------------------------------------------------------------
long DGControl::getParameterID()
{
	if ( isParameterAttached() )	// XXX necessary check?
		return getTag();
	else
		return DFX_PARAM_INVALID_ID;
}

//-----------------------------------------------------------------------------
void DGControl::setOffset(long x, long y)
{
	controlPos.offset(x, y);
	vizArea.offset(x, y);
}

//-----------------------------------------------------------------------------
void DGControl::setForeBounds(long x, long y, long w, long h)
{
	vizArea.set(x, y, w, h);
}

//-----------------------------------------------------------------------------
void DGControl::shrinkForeBounds(long inXoffset, long inYoffset, long inWidthShrink, long inHeightShrink)
{
	vizArea.offset(inXoffset, inYoffset);
	vizArea.setWidth( getWidth() - inWidthShrink );
	vizArea.setHeight( getHeight() - inHeightShrink );
}






#pragma mark -
#pragma mark DGBackgroundControl old
//-----------------------------------------------------------------------------
void DGBackgroundControl::draw(CDrawContext * inContext)
{
	DGRect drawRect((long)(getDfxGuiEditor()->GetXOffset()), (long)(getDfxGuiEditor()->GetYOffset()), getWidth(), getHeight());

	// draw the background image, if there is one
	if (backgroundImage != NULL)
	{
		backgroundImage->draw(&drawRect, inContext);
	}

	if (dragIsActive)
	{
		const float dragHiliteThickness = 2.0f;	// XXX is there a proper way to query this?
		if (HIThemeSetStroke != NULL)
		{
//auto const status = HIThemeBrushCreateCGColor(kThemeBrushDragHilite, CGColorRef * outColor);
			auto const status = HIThemeSetStroke(kThemeBrushDragHilite, NULL, inContext->getPlatformGraphicsContext(), inContext->getHIThemeOrientation());
			if (status == noErr)
			{
				CGRect cgRect = drawRect.convertToCGRect( inContext->getPortHeight() );
				const float halfLineWidth = dragHiliteThickness / 2.0f;
				cgRect = CGRectInset(cgRect, halfLineWidth, halfLineWidth);	// CoreGraphics lines are positioned between pixels rather than on them
				CGContextStrokeRect(inContext->getPlatformGraphicsContext(), cgRect);
			}
		}
		else
		{
			RGBColor dragHiliteColor;
			auto const error = GetDragHiliteColor(getDfxGuiEditor()->GetCarbonWindow(), &dragHiliteColor);
			if (error == noErr)
			{
				const float rgbScalar = 1.0f / (float)0xFFFF;
				DGColor strokeColor((float)(dragHiliteColor.red) * rgbScalar, (float)(dragHiliteColor.green) * rgbScalar, (float)(dragHiliteColor.blue) * rgbScalar);
				inContext->setStrokeColor(strokeColor);
				inContext->strokeRect(&drawRect, dragHiliteThickness);
			}
		}
	}
}

//-----------------------------------------------------------------------------
void DGBackgroundControl::setDragActive(bool inActiveStatus)
{
	bool oldStatus = dragIsActive;
	dragIsActive = inActiveStatus;
	if (oldStatus != inActiveStatus)
		redraw();
}



#endif	// !TARGET_PLUGIN_USES_VSTGUI
