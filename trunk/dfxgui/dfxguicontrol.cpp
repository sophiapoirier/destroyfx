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

#include "dfxmisc.h"



#pragma mark DGNullControl

//-----------------------------------------------------------------------------
DGNullControl::DGNullControl(DfxGuiEditor* inOwnerEditor, DGRect const& inRegion, DGImage* inBackgroundImage)
:	DGControl<CControl>(inRegion, inOwnerEditor, dfx::kParameterID_Invalid, inBackgroundImage)
{
	setMouseEnabled(false);
}

//-----------------------------------------------------------------------------
void DGNullControl::draw(CDrawContext* inContext)
{
	if (auto const image = getDrawBackground())
	{
		image->draw(inContext, getViewSize());
	}
	setDirty(false);
}



#if 0

#pragma mark -
#pragma mark DGControl old

//-----------------------------------------------------------------------------
bool DGControl::do_mouseWheel(long inDelta, dfx::Axis inAxis, KeyModifiers inKeyModifiers)
{
	if (!getMouseEnabled())
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
bool DGControl::mouseWheel(long inDelta, dfx::Axis inAxis, KeyModifiers inKeyModifiers)
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
			diff /= kDefaultFineTuneFactor;
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






#pragma mark -
#pragma mark DGBackgroundControl old
//-----------------------------------------------------------------------------
void DGBackgroundControl::draw(CDrawContext* inContext)
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
//auto const status = HIThemeBrushCreateCGColor(kThemeBrushDragHilite, CGColorRef* outColor);
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

	setDirty(false);
}

//-----------------------------------------------------------------------------
void DGBackgroundControl::setDragActive(bool inActiveStatus)
{
	bool oldStatus = dragIsActive;
	dragIsActive = inActiveStatus;
	if (oldStatus != inActiveStatus)
		redraw();
}

#endif
