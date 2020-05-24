/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2020  Sophia Poirier

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

	setZoomFactor(getFineTuneFactor());

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
	VSTGUI::CSlider::draw(inContext);

	getOwnerEditor()->drawControlHighlight(inContext, this);
}
#endif

//-----------------------------------------------------------------------------
VSTGUI::CMouseEventResult DGSlider::onMouseDown(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons)
{
	DiscreteValueConstrainer const dvc(this);
	return VSTGUI::CSlider::onMouseDown(inPos, ConstrainButtons(inButtons, getNumStates()));
}

//-----------------------------------------------------------------------------
VSTGUI::CMouseEventResult DGSlider::onMouseMoved(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons)
{
	DiscreteValueConstrainer const dvc(isEditing() ? this : nullptr);
	return VSTGUI::CSlider::onMouseMoved(inPos, ConstrainButtons(inButtons, getNumStates()));
}

//-----------------------------------------------------------------------------
VSTGUI::CMouseEventResult DGSlider::onMouseUp(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons)
{
	DiscreteValueConstrainer const dvc(this);
	return VSTGUI::CSlider::onMouseUp(inPos, ConstrainButtons(inButtons, getNumStates()));
}

//-----------------------------------------------------------------------------
void DGSlider::setHandle(VSTGUI::CBitmap* inHandle)
{
	VSTGUI::CSlider::setHandle(inHandle);
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
		VSTGUI::CSlider::setHandle(handle);
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
#pragma mark DGRangeSlider

//-----------------------------------------------------------------------------
DGRangeSlider::DGRangeSlider(DfxGuiEditor* inOwnerEditor, long inLowerParamID, long inUpperParamID, DGRect const& inRegion, 
							 DGImage* inLowerHandleImage, DGImage* inUpperHandleImage, 
							 DGImage* inBackgroundImage, PushStyle inPushStyle, long inOvershoot)
:	DGMultiControl<VSTGUI::CControl>(inRegion, 
									 inOwnerEditor, 
									 inUpperParamID,
									 inBackgroundImage),
	mLowerControl(addChild(inLowerParamID)),
	mUpperControl(this),
	mLowerMainHandleImage(inLowerHandleImage),
	mUpperMainHandleImage(inUpperHandleImage),
	mMinXPos(inRegion.left + (inLowerHandleImage ? inLowerHandleImage->getWidth() : 0)),
	mMaxXPos(inRegion.right - (inUpperHandleImage ? inUpperHandleImage->getWidth() : 0)),
	mEffectiveRange(static_cast<float>(mMaxXPos - mMinXPos)),
	mPushStyle(inPushStyle),
	mOvershoot(inOvershoot)
{
	assert(!inLowerHandleImage == !inUpperHandleImage);

	setTransparency(true);
}

//-----------------------------------------------------------------------------
void DGRangeSlider::draw(VSTGUI::CDrawContext* inContext)
{
	if (auto const image = getDrawBackground())
	{
		image->draw(inContext, getViewSize());
	}

	auto const getHandleImage = [midiLearner = getOwnerEditor()->getmidilearner()](auto control, auto const& mainImage, auto const& alternateImage)
	{
		bool const useAlternateImage = (control->getParameterID() == midiLearner) && alternateImage;
		return useAlternateImage ? alternateImage.get() : mainImage.get();
	};
	auto const lowerActiveHandleImage = getHandleImage(mLowerControl, mLowerMainHandleImage, mLowerAlternateHandleImage);
	auto const upperActiveHandleImage = getHandleImage(mUpperControl, mUpperMainHandleImage, mUpperAlternateHandleImage);
	if (lowerActiveHandleImage && upperActiveHandleImage)
	{
		auto const [lowerValue, upperValue] = [this]() -> std::pair<float, float>
		{
			auto const lowerValue = mLowerControl->asCControl()->getValueNormalized();
			auto const upperValue = mUpperControl->asCControl()->getValueNormalized();
			if (lowerValue > upperValue)
			{
				if (mPushStyle == PushStyle::Lower)
				{
					return {lowerValue, lowerValue};
				}
				return {upperValue, upperValue};
			}
			return {lowerValue, upperValue};
		}();
		auto const lowerPosX = std::floor(lowerValue * mEffectiveRange);
		auto const upperPosX = std::floor(upperValue * mEffectiveRange);
		DGRect const lowerRect(lowerPosX, 0, lowerActiveHandleImage->getWidth(), getHeight());
		DGRect const upperRect(upperPosX + lowerActiveHandleImage->getWidth(), 0, upperActiveHandleImage->getWidth(), getHeight());

		auto const rectLocalToFrame = [this](DGRect rect)
		{
			return rect.offset(getViewSize().getTopLeft());
		};
		lowerActiveHandleImage->draw(inContext, rectLocalToFrame(lowerRect));
		upperActiveHandleImage->draw(inContext, rectLocalToFrame(upperRect));
	}

#ifdef TARGET_API_RTAS
	getOwnerEditor()->drawControlHighlight(inContext, this);
#endif

	setDirty_all(false);
}

//-----------------------------------------------------------------------------
VSTGUI::CMouseEventResult DGRangeSlider::onMouseDown(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons)
{
	if (!inButtons.isLeftButton())
	{
		return VSTGUI::kMouseEventNotHandled;
	}

	if (checkDefaultValue_all(inButtons))
	{
		return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
	}

	DiscreteValueConstrainer const dvc1(mLowerControl);
	DiscreteValueConstrainer const dvc2(mUpperControl);

	beginEdit_all();

	mLowerStartValue = mLowerControl->asCControl()->getValueNormalized();
	mUpperStartValue = mUpperControl->asCControl()->getValueNormalized();
	mClickStartValue = static_cast<float>(inPos.x - mMinXPos) / mEffectiveRange;
	mOldPosY = inPos.y;

	mAnchorControl = [this]()
	{
		auto const lowerDifference = std::fabs(mClickStartValue - mLowerStartValue);
		auto const upperDifference = std::fabs(mClickStartValue - mUpperStartValue);
		if (lowerDifference < upperDifference)
		{
			return mUpperControl;
		}
		if (lowerDifference > upperDifference)
		{
			return mLowerControl;
		}
		return (mClickStartValue < mLowerStartValue) ? mUpperControl : mLowerControl;
	}();
	mAnchorStartValue = mAnchorControl->asCControl()->getValueNormalized();

	// this determines if the mouse click was between the 2 points (or close, by 1 pixel)
	float const pixelValue = static_cast<float>(mOvershoot) / mEffectiveRange;
	mClickBetween = ((mClickStartValue >= (mLowerStartValue - pixelValue)) && (mClickStartValue <= (mUpperStartValue + pixelValue)));

	// the following stuff allows you click within a handle and have the value not "jump" at all
	mClickOffsetInValue = [this]()
	{
		if (mLowerMainHandleImage && mUpperMainHandleImage)
		{
			if (mAnchorControl == mLowerControl)
			{
				float const handleWidthInValue = static_cast<float>(mUpperMainHandleImage->getWidth()) / mEffectiveRange;
				float const startDiff = mClickStartValue - mUpperStartValue;
				if ((startDiff >= 0.0f) && (startDiff <= handleWidthInValue))
				{
					return -startDiff;
				}
				return -handleWidthInValue / 2.0f;
			}
			else
			{
				float const handleWidthInValue = static_cast<float>(mLowerMainHandleImage->getWidth()) / mEffectiveRange;
				float const startDiff = mLowerStartValue - mClickStartValue;
				if ((startDiff >= 0.0f) && (startDiff <= handleWidthInValue))
				{
					return startDiff;
				}
				return handleWidthInValue / 2.0f;
			}
		}
		return 0.0f;
	}();

	mNewValue = mClickStartValue + mClickOffsetInValue;
	mOldValue = mNewValue;
	mFineTuneStartValue = [this, inButtons]()
	{
		if (mClickBetween || (inButtons.getModifierState() & ~kZoomModifier))
		{
			return mNewValue;
		}
		return (mAnchorControl == mLowerControl) ? mUpperStartValue : mLowerStartValue;
	}();
	mOldButtons = inButtons;

#if TARGET_PLUGIN_USES_MIDI
	if (getOwnerEditor()->getmidilearning())
	{
		auto const primaryParameterID = ((mAnchorControl == mLowerControl) ? mUpperControl : mLowerControl)->getParameterID();
		getOwnerEditor()->setmidilearner(primaryParameterID);
	}
#endif

	return onMouseMoved(inPos, inButtons);
}

//-----------------------------------------------------------------------------
VSTGUI::CMouseEventResult DGRangeSlider::onMouseMoved(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons)
{
	DiscreteValueConstrainer const dvc1(mLowerControl->asCControl()->isEditing() ? mLowerControl : nullptr);
	DiscreteValueConstrainer const dvc2(mUpperControl->asCControl()->isEditing() ? mUpperControl : nullptr);

	if (!(inButtons.isLeftButton() && isEditing_any()))
	{
		return VSTGUI::kMouseEventNotHandled;
	}

	bool const isFineTune = (inButtons & kZoomModifier);
	if ((mOldButtons != inButtons) && isFineTune)
	{
		mOldValue = mFineTuneStartValue = mNewValue;
		mOldButtons = inButtons;
	}
	else if (!isFineTune)
	{
		mOldValue = mNewValue;
		mOldButtons = inButtons;
	}
	mNewValue = (static_cast<float>(inPos.x - mMinXPos) / mEffectiveRange) + mClickOffsetInValue;
	if (isFineTune)
	{
		mNewValue = mFineTuneStartValue + ((mNewValue - mOldValue) / getFineTuneFactor());
	}

	auto const [lowerValue, upperValue] = [this, inPos, inButtons, isFineTune]() -> std::pair<float, float>
	{
		// unfortunately VSTGUI on macOS hijacks control-left-button and remaps it to right-button (even after mouse-down)
		auto const controlKeyPressed = TARGET_OS_MAC ? inButtons.isAppleSet() : inButtons.isControlSet();

		// move both parameters to the new value
		if (controlKeyPressed && !inButtons.isAltSet())
		{
			return {mNewValue, mNewValue};
		}

		// reverso axis convergence/divergence mode
		if (controlKeyPressed && inButtons.isAltSet())
		{
			//auto const diff = currentXpos - oldXpos;
			auto const diff = mOldPosY - inPos.y;
			auto const changeAmountDivisor = isFineTune ? getFineTuneFactor() : 1.0f;
			float const changeAmount = (static_cast<float>(diff) / mEffectiveRange) / changeAmountDivisor;
			auto lowerValue = mLowerControl->asCControl()->getValueNormalized() - changeAmount;
			auto upperValue = mUpperControl->asCControl()->getValueNormalized() + changeAmount;
			// prevent the upper from crossing below the lower and vice versa
			if (lowerValue > upperValue)
			{
				auto const jointValue = (lowerValue + upperValue) / 2.0f;
				lowerValue = jointValue;
				upperValue = jointValue;
			}
			return {lowerValue, upperValue};
		}

		// move both parameters, preserving relationship
		if ((inButtons.isAltSet() && !controlKeyPressed) || mClickBetween)
		{
			// the click offset stuff is annoying in this mode, so remove it
			auto const valueChange = mNewValue - mClickStartValue - mClickOffsetInValue;
			return {mLowerStartValue + valueChange, mUpperStartValue + valueChange};
		}

		// no key command moves both parameters to the new value and then 
		// moves the appropriate parameter after that point, anchoring at the new value
		if (mAnchorControl == mLowerControl)
		{
			if (mNewValue < mAnchorStartValue)
			{
				if ((mPushStyle == PushStyle::Upper) || (mPushStyle == PushStyle::Both))
				{
					return {mNewValue, mNewValue};
				}
				else
				{
					return {mAnchorStartValue, mAnchorStartValue};
				}
			}
			else
			{
				return {mAnchorStartValue, mNewValue};
			}
		}
		else
		{
			if (mNewValue > mAnchorStartValue)
			{
				if ((mPushStyle == PushStyle::Lower) || (mPushStyle == PushStyle::Both))
				{
					return {mNewValue, mAnchorStartValue};
				}
				else
				{
					return {mAnchorStartValue, mAnchorStartValue};
				}
			}
			else
			{
				return {mNewValue, mAnchorStartValue};
			}
		}
	}();
	mLowerControl->asCControl()->setValueNormalized(lowerValue);
	mUpperControl->asCControl()->setValueNormalized(upperValue);

	mOldPosY = inPos.y;

	notifyIfChanged_all();

	return VSTGUI::kMouseEventHandled;
}

//-----------------------------------------------------------------------------
VSTGUI::CMouseEventResult DGRangeSlider::onMouseUp(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons)
{
	auto const endControl = [](IDGControl* dgControl)
	{
		if (dgControl->asCControl()->isEditing())
		{
			dgControl->asCControl()->endEdit();
		}
	};
	endControl(mLowerControl);
	endControl(mUpperControl);

	return VSTGUI::kMouseEventHandled;
}

//-----------------------------------------------------------------------------
VSTGUI::CMouseEventResult DGRangeSlider::onMouseCancel()
{
	auto const cancelControl = [](IDGControl* dgControl, auto startValue)
	{
		auto const cControl = dgControl->asCControl();
		if (cControl->isEditing())
		{
			cControl->setValueNormalized(startValue);
			cControl->endEdit();
		}
	};
	cancelControl(mLowerControl, mLowerStartValue);
	cancelControl(mUpperControl, mUpperStartValue);
	notifyIfChanged_all();

	return VSTGUI::kMouseEventHandled;
}

//-----------------------------------------------------------------------------
void DGRangeSlider::setAlternateHandles(VSTGUI::CBitmap* inLowerHandle, VSTGUI::CBitmap* inUpperHandle)
{
	assert(!inLowerHandle == !inUpperHandle);
	assert((inLowerHandle && inUpperHandle) ? (inLowerHandle->getSize() == inUpperHandle->getSize()) : true);
	assert((mUpperMainHandleImage && inUpperHandle) ? (mUpperMainHandleImage->getSize() == inUpperHandle->getSize()) : true);
	mLowerAlternateHandleImage = inLowerHandle;
	mUpperAlternateHandleImage = inUpperHandle;
}

#if TARGET_PLUGIN_USES_MIDI
//-----------------------------------------------------------------------------
void DGRangeSlider::setMidiLearner(bool inEnable)
{
	redraw();
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

	setZoomFactor(getFineTuneFactor());
}

#ifdef TARGET_API_RTAS
//------------------------------------------------------------------------
void DGAnimation::draw(VSTGUI::CDrawContext* inContext)
{
	VSTGUI::CAnimKnob::draw(inContext);

	getOwnerEditor()->drawControlHighlight(inContext, this);
}
#endif

//------------------------------------------------------------------------
VSTGUI::CMouseEventResult DGAnimation::onMouseDown(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons)
{
	DiscreteValueConstrainer const dvc(this);
	mEntryMousePos = inPos;
	return VSTGUI::CAnimKnob::onMouseDown(inPos, ConstrainButtons(inButtons, getNumStates()));
}

//------------------------------------------------------------------------
VSTGUI::CMouseEventResult DGAnimation::onMouseMoved(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons)
{
	DiscreteValueConstrainer const dvc(isEditing() ? this : nullptr);
	inPos = constrainMousePosition(inPos);
	return VSTGUI::CAnimKnob::onMouseMoved(inPos, ConstrainButtons(inButtons, getNumStates()));
}

//------------------------------------------------------------------------
VSTGUI::CMouseEventResult DGAnimation::onMouseUp(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons)
{
	DiscreteValueConstrainer const dvc(this);
	inPos = constrainMousePosition(inPos);
	return VSTGUI::CAnimKnob::onMouseUp(inPos, ConstrainButtons(inButtons, getNumStates()));
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
