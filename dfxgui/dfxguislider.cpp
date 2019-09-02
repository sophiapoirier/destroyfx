/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2019  Sophia Poirier

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

#include <cassert>

#include "dfxguieditor.h"


//-----------------------------------------------------------------------------
static VSTGUI::CButtonState ConstrainButtons(VSTGUI::CButtonState const& inButtons, long inNumStates)
{
	if (inNumStates > 0)
	{
		return inButtons & ~VSTGUI::CControl::kZoomModifier;
	}
	return inButtons;
}



#pragma mark -
#pragma mark DGSlider

//-----------------------------------------------------------------------------
DGSlider::DGSlider(DfxGuiEditor* inOwnerEditor, long inParamID, DGRect const& inRegion, 
				   dfx::Axis inOrientation, DGImage* inHandleImage, DGImage* inBackgroundImage, long inRangeMargin)
:	DGControl<VSTGUI::CSlider>(inRegion, 
							   inOwnerEditor, 
							   inParamID, 
							   VSTGUI::CPoint((inOrientation & dfx::kAxis_Horizontal) ? inRangeMargin : 0, 
											  (inOrientation & dfx::kAxis_Vertical) ? inRangeMargin : 0), 
							   (inOrientation & dfx::kAxis_Horizontal) ? (inRegion.getWidth() - (inRangeMargin * 2)) : (inRegion.getHeight() - (inRangeMargin * 2)), 
							   inHandleImage, 
							   inBackgroundImage, 
							   VSTGUI::CPoint(0, 0), 
							   (inOrientation & dfx::kAxis_Horizontal) ? (kLeft | kHorizontal) : (kBottom | kVertical)),
	mMainHandleImage(inHandleImage)
{
	setTransparency(true);

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
void DGSlider::draw(VSTGUI::CDrawContext* inContext)
{
	Parent::draw(inContext);

	getOwnerEditor()->drawControlHighlight(inContext, this);
}
#endif

//-----------------------------------------------------------------------------
VSTGUI::CMouseEventResult DGSlider::onMouseDown(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons)
{
	DiscreteValueConstrainer const dvc(this);
	return Parent::onMouseDown(inPos, ConstrainButtons(inButtons, getNumStates()));
}

//-----------------------------------------------------------------------------
VSTGUI::CMouseEventResult DGSlider::onMouseMoved(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons)
{
	DiscreteValueConstrainer const dvc(isEditing() ? this : nullptr);
	return Parent::onMouseMoved(inPos, ConstrainButtons(inButtons, getNumStates()));
}

//-----------------------------------------------------------------------------
VSTGUI::CMouseEventResult DGSlider::onMouseUp(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons)
{
	DiscreteValueConstrainer const dvc(this);
	return Parent::onMouseUp(inPos, ConstrainButtons(inButtons, getNumStates()));
}

//-----------------------------------------------------------------------------
void DGSlider::setHandle(VSTGUI::CBitmap* inHandle)
{
	Parent::setHandle(inHandle);
	mMainHandleImage = inHandle;
}

//-----------------------------------------------------------------------------
void DGSlider::setAlternateHandle(VSTGUI::CBitmap* inHandle)
{
	mAlternateHandleImage = inHandle;
}

//-----------------------------------------------------------------------------
void DGSlider::setUseAlternateHandle(bool inEnable)
{
	if (auto const handle = inEnable ? mAlternateHandleImage.get() : mMainHandleImage.get())
	{
		bool const changed = (getHandle() != handle);
		Parent::setHandle(handle);
		if (changed)
		{
			redraw();
		}
	}
}

#if TARGET_PLUGIN_USES_MIDI
//-----------------------------------------------------------------------------
void DGSlider::setMidiLearner(bool inEnable)
{
	setUseAlternateHandle(inEnable);
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
:	DGControl<VSTGUI::CAnimKnob>(inRegion, inOwnerEditor, inParamID, 
								 inNumAnimationFrames, inRegion.getHeight(), inAnimationImage)
{
	setTransparency(true);

	setZoomFactor(kDefaultFineTuneFactor);
}

#ifdef TARGET_API_RTAS
//------------------------------------------------------------------------
void DGAnimation::draw(VSTGUI::CDrawContext* inContext)
{
	Parent::draw(inContext);

	getOwnerEditor()->drawControlHighlight(inContext, this);
}
#endif

//------------------------------------------------------------------------
VSTGUI::CMouseEventResult DGAnimation::onMouseDown(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons)
{
	DiscreteValueConstrainer const dvc(this);
	mEntryMousePos = inPos;
	return Parent::onMouseDown(inPos, ConstrainButtons(inButtons, getNumStates()));
}

//------------------------------------------------------------------------
VSTGUI::CMouseEventResult DGAnimation::onMouseMoved(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons)
{
	DiscreteValueConstrainer const dvc(isEditing() ? this : nullptr);
	inPos = constrainMousePosition(inPos);
	return Parent::onMouseMoved(inPos, ConstrainButtons(inButtons, getNumStates()));
}

//------------------------------------------------------------------------
VSTGUI::CMouseEventResult DGAnimation::onMouseUp(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons)
{
	DiscreteValueConstrainer const dvc(this);
	inPos = constrainMousePosition(inPos);
	return Parent::onMouseUp(inPos, ConstrainButtons(inButtons, getNumStates()));
}

//------------------------------------------------------------------------
VSTGUI::CPoint DGAnimation::constrainMousePosition(VSTGUI::CPoint const& inPos) const noexcept
{
	VSTGUI::CPoint resultPos(mEntryMousePos);
	if (mMouseAxis & dfx::kAxis_Horizontal)
	{
		resultPos.x = inPos.x;
	}
	if (mMouseAxis & dfx::kAxis_Vertical)
	{
		resultPos.y = inPos.y;
	}
	return resultPos;
}
