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

#include "dfxguislider.h"


//-----------------------------------------------------------------------------
DGSlider::DGSlider(DfxGuiEditor* inOwnerEditor, long inParamID, DGRect const& inRegion, 
				   DGAxis inOrientation, DGImage* inHandleImage, DGImage* inBackgroundImage, long inRangeMargin)
:	CSlider(inRegion, inOwnerEditor, inParamID, 
			CPoint((inOrientation & kDGAxis_Horizontal) ? inRangeMargin : 0, (inOrientation & kDGAxis_Vertical) ? inRangeMargin : 0), 
			(inOrientation & kDGAxis_Horizontal) ? (inRegion.getWidth() - (inRangeMargin * 2)) : (inRegion.getHeight() - (inRangeMargin * 2)), 
			inHandleImage, inBackgroundImage, CPoint(0, 0), 
			(inOrientation & kDGAxis_Horizontal) ? (kLeft | kHorizontal) : (kBottom | kVertical)), 
	DGControl(this, inOwnerEditor)
{
	setTransparency(true);
	if (!inBackgroundImage)
	{
		setBackOffset(inRegion.getTopLeft());
	}

	setZoomFactor(kDefaultFineTuneFactor);

	if (inHandleImage)
	{
		auto centeredHandleOffset = getOffsetHandle();
		if (getStyle() & kHorizontal)
		{
			centeredHandleOffset.y = (getHeight() - inHandleImage->getHeight()) / 2;
		}
		else
		{
			centeredHandleOffset.x = (getWidth() - inHandleImage->getWidth()) / 2;
		}
		setOffsetHandle(centeredHandleOffset);
	}
}

#ifdef TARGET_API_RTAS
//-----------------------------------------------------------------------------
void DGSlider::draw(CDrawContext * inContext)
{
	CSlider::draw(inContext);

	getOwnerEditor()->drawControlHighlight(inContext, this);
}
#endif






#pragma mark -
#pragma mark DGAnimation

//-----------------------------------------------------------------------------
// DGAnimation
//-----------------------------------------------------------------------------
DGAnimation::DGAnimation(DfxGuiEditor*	inOwnerEditor, 
						 long			inParamID, 
						 DGRect const&	inRegion, 
						 DGImage*		inAnimationImage, 
						 long			inNumAnimationFrames, 
						 DGImage*		inBackground)
:	CAnimKnob(inRegion, inOwnerEditor, inParamID, 
			  inNumAnimationFrames, inRegion.getHeight(), inAnimationImage), 
	DGControl(this, inOwnerEditor)
{
	setTransparency(true);
	if (!inBackground)
	{
		setBackOffset(inRegion.getTopLeft());
	}

	setZoomFactor(kDefaultFineTuneFactor);
}

#ifdef TARGET_API_RTAS
//------------------------------------------------------------------------
void DGAnimation::draw(CDrawContext * inContext)
{
	CAnimKnob::draw(inContext);

	getOwnerEditor()->drawControlHighlight(inContext, this);
}
#endif
