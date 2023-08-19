/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2023  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 2 of the License, or 
(at your option) any later version.

Destroy FX Library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Destroy FX Library.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#include "dfxguislider.h"

#include <cassert>
#include <cmath>
#include <utility>

#include "dfxguieditor.h"
#include "dfxmath.h"


namespace
{

//-----------------------------------------------------------------------------
// TODO: migrate fully to VSTGUI::Modifiers
static VSTGUI::CButtonState ConstrainButtons(VSTGUI::MouseEvent const& inEvent, size_t inNumStates)
{
	auto const buttons = VSTGUI::buttonStateFromMouseEvent(inEvent);
	if (inNumStates > 0)
	{
		return buttons & ~VSTGUI::CControl::kZoomModifier;
	}
	return buttons;
}

//-----------------------------------------------------------------------------
// TODO: migrate fully to VSTGUI::MouseEvent
static void ApplyMouseEventResult(VSTGUI::CMouseEventResult inMouseEventResult, VSTGUI::MouseEvent& ioEvent)
{
	switch (inMouseEventResult)
	{
		case VSTGUI::kMouseEventHandled:
			ioEvent.consumed = true;
			break;
		case VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents:
			ioEvent.consumed = true;
			if (ioEvent.type == VSTGUI::EventType::MouseDown)
			{
				VSTGUI::castMouseDownEvent(ioEvent).ignoreFollowUpMoveAndUpEvents(true);
			}
			else
			{
				assert(false);
			}
			break;
		case VSTGUI::kMouseMoveEventHandledButDontNeedMoreEvents:
			ioEvent.consumed = true;
			if (ioEvent.type == VSTGUI::EventType::MouseMove)
			{
				VSTGUI::castMouseMoveEvent(ioEvent).ignoreFollowUpMoveAndUpEvents(true);
			}
			else
			{
				assert(false);
			}
			break;
		default:
			break;
	}
}

//-----------------------------------------------------------------------------
static void EndControl(IDGControl* inControl)
{
	if (inControl->asCControl()->isEditing())
	{
		inControl->asCControl()->endEdit();
	}
}

//-----------------------------------------------------------------------------
static void CancelControl(IDGControl* inControl, float inStartValue)
{
	auto const cControl = inControl->asCControl();
	if (cControl->isEditing())
	{
		cControl->setValueNormalized(inStartValue);
		cControl->endEdit();
	}
}

}  // namespace



#pragma mark -
#pragma mark DGSlider

//-----------------------------------------------------------------------------
DGSlider::DGSlider(DfxGuiEditor* inOwnerEditor, dfx::ParameterID inParameterID, DGRect const& inRegion, 
				   dfx::Axis inOrientation, DGImage* inHandleImage, DGImage* inBackgroundImage, int inRangeMargin)
:	DGControl<VSTGUI::CSlider>(inRegion, 
							   inOwnerEditor, 
							   dfx::ParameterID_ToVST(inParameterID), 
							   VSTGUI::CPoint((inOrientation & dfx::kAxis_Horizontal) ? inRangeMargin : 0, 
											  (inOrientation & dfx::kAxis_Vertical) ? inRangeMargin : 0), 
							   dfx::math::IRound((inOrientation & dfx::kAxis_Horizontal) ? inRegion.getWidth() : inRegion.getHeight()) - (inRangeMargin * 2), 
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
void DGSlider::onMouseDownEvent(VSTGUI::MouseDownEvent& ioEvent)
{
	DiscreteValueConstrainer const dvc(this);
	ApplyMouseEventResult(VSTGUI::CSlider::onMouseDown(ioEvent.mousePosition, ConstrainButtons(ioEvent, getNumStates())), ioEvent);
}

//-----------------------------------------------------------------------------
void DGSlider::onMouseMoveEvent(VSTGUI::MouseMoveEvent& ioEvent)
{
	DiscreteValueConstrainer const dvc(isEditing() ? this : nullptr);
	ApplyMouseEventResult(VSTGUI::CSlider::onMouseMoved(ioEvent.mousePosition, ConstrainButtons(ioEvent, getNumStates())), ioEvent);
}

//-----------------------------------------------------------------------------
void DGSlider::onMouseUpEvent(VSTGUI::MouseUpEvent& ioEvent)
{
	DiscreteValueConstrainer const dvc(this);
	ApplyMouseEventResult(VSTGUI::CSlider::onMouseUp(ioEvent.mousePosition, ConstrainButtons(ioEvent, getNumStates())), ioEvent);
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
DGRangeSlider::DGRangeSlider(DfxGuiEditor* inOwnerEditor, dfx::ParameterID inLowerParameterID, dfx::ParameterID inUpperParameterID, 
							 DGRect const& inRegion, 
							 DGImage* inLowerHandleImage, DGImage* inUpperHandleImage, 
							 DGImage* inBackgroundImage, PushStyle inPushStyle, 
							 std::optional<VSTGUI::CCoord> inOvershoot)
:	DGMultiControl<VSTGUI::CControl>(inRegion, 
									 inOwnerEditor, 
									 dfx::ParameterID_ToVST(inUpperParameterID),
									 inBackgroundImage),
	mLowerControl(addChild(inLowerParameterID)),
	mUpperControl(this),
	mLowerMainHandleImage(inLowerHandleImage),
	mUpperMainHandleImage(inUpperHandleImage),
	mMinXPos(inRegion.left + (inLowerHandleImage ? inLowerHandleImage->getWidth() : 0)),
	mMaxXPos(inRegion.right - (inUpperHandleImage ? inUpperHandleImage->getWidth() : 0)),
	mEffectiveRange(static_cast<float>(mMaxXPos - mMinXPos)),
	mPushStyle(inPushStyle),
	mOvershoot(inOvershoot.value_or(((inLowerHandleImage ? inLowerHandleImage->getWidth() : 0.) / 3.) +
									((inUpperHandleImage ? inUpperHandleImage->getWidth() : 0.) / 3.)))
{
	assert(!inLowerHandleImage == !inUpperHandleImage);

	setTransparency(true);
}

//-----------------------------------------------------------------------------
void DGRangeSlider::draw(VSTGUI::CDrawContext* inContext)
{
	if (auto const image = getDrawBackground())
	{
		image->draw(inContext, getViewSize(), mBackgroundOffset);
	}

#if TARGET_PLUGIN_USES_MIDI
	auto const getHandleImage = [midiLearner = getOwnerEditor()->getmidilearner()](auto control, auto const& mainImage, auto const& alternateImage)
	{
		bool const useAlternateImage = (control->getParameterID() == midiLearner) && alternateImage;
		return useAlternateImage ? alternateImage.get() : mainImage.get();
	};
#else
	auto const getHandleImage = [](auto, auto const& mainImage, auto const&)
	{
		return mainImage.get();
	};
#endif
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

		lowerActiveHandleImage->draw(inContext, rectLocalToFrame(lowerRect));
		upperActiveHandleImage->draw(inContext, rectLocalToFrame(upperRect));
	}

#ifdef TARGET_API_RTAS
	getOwnerEditor()->drawControlHighlight(inContext, this);
#endif

	setDirty_all(false);
}

//-----------------------------------------------------------------------------
void DGRangeSlider::onMouseDownEvent(VSTGUI::MouseDownEvent& ioEvent)
{
	if (!ioEvent.buttonState.isLeft())
	{
		return;
	}

	beginEdit_all();

	mLowerStartValue = mLowerControl->asCControl()->getValueNormalized();
	mUpperStartValue = mUpperControl->asCControl()->getValueNormalized();
	mClickStartValue = static_cast<float>(ioEvent.mousePosition.x - mMinXPos) / mEffectiveRange;
	mOldPosY = ioEvent.mousePosition.y;

	bool const valuesInverted = (mLowerStartValue > mUpperStartValue);
	auto const effectiveLowerStartValue = (valuesInverted && (mPushStyle == PushStyle::Upper)) ? mUpperStartValue : mLowerStartValue;
	auto const effectiveUpperStartValue = (valuesInverted && (mPushStyle == PushStyle::Lower)) ? mLowerStartValue : mUpperStartValue;

	mAnchorControl = [this, effectiveLowerStartValue, effectiveUpperStartValue]
	{
		auto const lowerDifference = std::fabs(mClickStartValue - effectiveLowerStartValue);
		auto const upperDifference = std::fabs(mClickStartValue - effectiveUpperStartValue);
		if (lowerDifference < upperDifference)
		{
			return mUpperControl;
		}
		if (lowerDifference > upperDifference)
		{
			return mLowerControl;
		}
		return (mClickStartValue < effectiveLowerStartValue) ? mUpperControl : mLowerControl;
	}();
	mAnchorStartValue = mAnchorControl->asCControl()->getValueNormalized();

	// this determines if the mouse click was between the two points
	float const overshootInValue = static_cast<float>(mOvershoot) / mEffectiveRange;
	mClickBetween = ((mClickStartValue >= (effectiveLowerStartValue - overshootInValue)) && (mClickStartValue <= (effectiveUpperStartValue + overshootInValue)));

	// the following stuff allows you click within a handle and have the value not "jump" at all
	mClickOffsetInValue = [this]
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
	mFineTuneStartValue = [this, buttons = VSTGUI::buttonStateFromMouseEvent(ioEvent)]
	{
		if (mClickBetween || (buttons.getModifierState() & ~kZoomModifier))
		{
			return mNewValue;
		}
		return (mAnchorControl == mLowerControl) ? mUpperStartValue : mLowerStartValue;
	}();
	mOldModifiers = ioEvent.modifiers;

#if TARGET_PLUGIN_USES_MIDI
	if (getOwnerEditor()->getmidilearning())
	{
		auto const primaryParameterID = ((mAnchorControl == mLowerControl) ? mUpperControl : mLowerControl)->getParameterID();
		getOwnerEditor()->setmidilearner(primaryParameterID);
	}
#endif

	VSTGUI::MouseMoveEvent mouseMoveEvent(ioEvent);
	onMouseMoveEvent(mouseMoveEvent);
	ioEvent.consumed = mouseMoveEvent.consumed;
}

//-----------------------------------------------------------------------------
void DGRangeSlider::onMouseMoveEvent(VSTGUI::MouseMoveEvent& ioEvent)
{
	if (!(ioEvent.buttonState.isLeft() && isEditing_any()))
	{
		return;
	}

	DiscreteValueConstrainer const dvc1(mLowerControl->asCControl()->isEditing() ? mLowerControl : nullptr);
	DiscreteValueConstrainer const dvc2(mUpperControl->asCControl()->isEditing() ? mUpperControl : nullptr);

	bool const isFineTune = (VSTGUI::buttonStateFromMouseEvent(ioEvent) & kZoomModifier);
	if ((mOldModifiers != ioEvent.modifiers) && isFineTune)
	{
		mOldValue = mFineTuneStartValue = mNewValue;
		mOldModifiers = ioEvent.modifiers;
	}
	else if (!isFineTune)
	{
		mOldValue = mNewValue;
		mOldModifiers = ioEvent.modifiers;
	}
	mNewValue = (static_cast<float>(ioEvent.mousePosition.x - mMinXPos) / mEffectiveRange) + mClickOffsetInValue;
	if (isFineTune)
	{
		mNewValue = mFineTuneStartValue + ((mNewValue - mOldValue) / getFineTuneFactor());
	}

	auto const [lowerValue, upperValue] = [this, pos = ioEvent.mousePosition, modifiers = ioEvent.modifiers, isFineTune]() -> std::pair<float, float>
	{
		// unfortunately VSTGUI on macOS hijacks control-left-button and remaps it to right-button (even after mouse-down)
		auto const controlKeyPressed = isPlatformControlKeySet(modifiers);

		// move both parameters to the new value
		if (controlKeyPressed && !modifiers.has(VSTGUI::ModifierKey::Alt))
		{
			return {mNewValue, mNewValue};
		}

		// reverso axis convergence/divergence mode
		if (controlKeyPressed && modifiers.has(VSTGUI::ModifierKey::Alt))
		{
			//auto const diff = currentXpos - oldXpos;
			auto const diff = mOldPosY - pos.y;
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
		if ((modifiers.has(VSTGUI::ModifierKey::Alt) && !controlKeyPressed) || mClickBetween)
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

	mOldPosY = ioEvent.mousePosition.y;

	notifyIfChanged_all();

	ioEvent.consumed = true;
}

//-----------------------------------------------------------------------------
void DGRangeSlider::onMouseUpEvent(VSTGUI::MouseUpEvent& ioEvent)
{
	EndControl(mLowerControl);
	EndControl(mUpperControl);

	ioEvent.consumed = true;
}

//-----------------------------------------------------------------------------
void DGRangeSlider::onMouseCancelEvent(VSTGUI::MouseCancelEvent& ioEvent)
{
	CancelControl(mLowerControl, mLowerStartValue);
	CancelControl(mUpperControl, mUpperStartValue);
	notifyIfChanged_all();

	ioEvent.consumed = true;
}

//-----------------------------------------------------------------------------
void DGRangeSlider::setBackgroundOffset(VSTGUI::CPoint const& inOffset)
{
	mBackgroundOffset = inOffset;
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
void DGRangeSlider::setMidiLearner(bool /*inEnable*/)
{
	redraw();
}
#endif






#pragma mark -
#pragma mark DGXYBox

//-----------------------------------------------------------------------------
DGXYBox::DGXYBox(DfxGuiEditor* inOwnerEditor, dfx::ParameterID inParameterIDX, dfx::ParameterID inParameterIDY, 
				 DGRect const& inRegion, DGImage* inHandleImage, DGImage* inBackgroundImage, int inStyle)
:	DGMultiControl<VSTGUI::CControl>(inRegion, 
									 inOwnerEditor, 
									 dfx::ParameterID_ToVST(inParameterIDX), 
									 inBackgroundImage),
	mControlX(this), 
	mControlY(addChild(inParameterIDY)), 
	mMainHandleImage(inHandleImage),
	mStyle(inStyle),
	mHandleWidth(inHandleImage ? inHandleImage->getWidth() : 1), 
	mHandleHeight(inHandleImage ? inHandleImage->getHeight() : 1), 
	mMinXPos(inRegion.left), 
	mMaxXPos(inRegion.right - mHandleWidth), 
	mMinYPos(inRegion.top), 
	mMaxYPos(inRegion.bottom - mHandleHeight), 
	mMouseableOrigin(mMinXPos + (mHandleWidth / 2)/* - 1*/, mMinYPos + (mHandleHeight / 2)/* - 1*/), 
	mEffectiveRangeX(static_cast<float>(mMaxXPos - mMinXPos)), 
	mEffectiveRangeY(static_cast<float>(mMaxYPos - mMinYPos))
{
	assert((inStyle & VSTGUI::CSliderBase::kLeft) ^ (inStyle & VSTGUI::CSliderBase::kRight));
	assert((inStyle & VSTGUI::CSliderBase::kBottom) ^ (inStyle & VSTGUI::CSliderBase::kTop));
	assert(!(inStyle & (VSTGUI::CSliderBase::kHorizontal | VSTGUI::CSliderBase::kVertical)));

	setTransparency(true);
}

//-----------------------------------------------------------------------------
void DGXYBox::draw(VSTGUI::CDrawContext* inContext)
{
	if (auto const image = getDrawBackground())
	{
		image->draw(inContext, getViewSize());
	}

	auto const valueX = mapValueX(mControlX->asCControl()->getValueNormalized());
	auto const valueY = mapValueY(mControlY->asCControl()->getValueNormalized());

	double const posX = valueX * mEffectiveRangeX;
	double const posY = valueY * mEffectiveRangeY;

	DGRect rect(posX, posY, mHandleWidth, mHandleHeight);
	if (mIntegralPosition)
	{
		rect.makeIntegral();
	}

	VSTGUI::CRect const boundingRect(mMinXPos - getViewSize().left, 
									 mMinYPos - getViewSize().top, 
									 mMaxXPos + mHandleWidth - getViewSize().left, 
									 mMaxYPos + mHandleHeight - getViewSize().top);
	rect.bound(boundingRect);

	auto const activeHandleImage = [this]
	{
#if TARGET_PLUGIN_USES_MIDI
		auto const midiLearner = getOwnerEditor()->getmidilearner();
		if ((mControlX->getParameterID() == midiLearner) && mAlternateHandleImageX)
		{
			return mAlternateHandleImageX.get();
		}
		if ((mControlY->getParameterID() == midiLearner) && mAlternateHandleImageY)
		{
			return mAlternateHandleImageY.get();
		}
#endif
		return mMainHandleImage.get();
	}();
	if (activeHandleImage)
	{
		activeHandleImage->draw(inContext, rectLocalToFrame(rect));
	}

#ifdef TARGET_API_RTAS
	getOwnerEditor()->drawControlHighlight(inContext, this);
#endif

	setDirty_all(false);
}

//-----------------------------------------------------------------------------
void DGXYBox::onMouseDownEvent(VSTGUI::MouseDownEvent& ioEvent)
{
	if (!ioEvent.buttonState.isLeft())
	{
		return;
	}

	beginEdit_all();

	mStartValueX = mOldValueX = mControlX->asCControl()->getValueNormalized();
	mStartValueY = mOldValueY = mControlY->asCControl()->getValueNormalized();
	mOldModifiers = ioEvent.modifiers;

	// the following stuff allows you click within a handle and have the value not "jump" at all
	auto const handleWidthInValue = static_cast<float>(mHandleWidth) / mEffectiveRangeX;
	auto const handleHeightInValue = static_cast<float>(mHandleHeight) / mEffectiveRangeY;
	auto const clickValueX = mapValueX(static_cast<float>(ioEvent.mousePosition.x - mMouseableOrigin.x) / mEffectiveRangeX);
	auto const clickValueY = mapValueY(static_cast<float>(ioEvent.mousePosition.y - mMouseableOrigin.y) / mEffectiveRangeY);
	auto const startDiffX = clickValueX - mStartValueX;
	auto const startDiffY = clickValueY - mStartValueY;
	mClickOffset = {};
	if (std::fabs(startDiffX) < (handleWidthInValue * 0.5f))
	{
		mClickOffset.x = (startDiffX * mEffectiveRangeX) * ((mStyle & VSTGUI::CSliderBase::kLeft) ? -1 : 1);
	}
	if (std::fabs(startDiffY) < (handleHeightInValue * 0.5f))
	{
		mClickOffset.y = (startDiffY * mEffectiveRangeY) * ((mStyle & VSTGUI::CSliderBase::kTop) ? -1 : 1);
	}

#if TARGET_PLUGIN_USES_MIDI
	if (getOwnerEditor()->getmidilearning())
	{
		auto const currentLearner = getOwnerEditor()->getmidilearner();
		auto const firstLearner = mControlX->getParameterID();
		auto const alternateLearner = mControlY->getParameterID();
		auto const newLearner = (currentLearner == firstLearner) ? alternateLearner : firstLearner;
		getOwnerEditor()->setmidilearner(newLearner);
	}
#endif

	VSTGUI::MouseMoveEvent mouseMoveEvent(ioEvent);
	onMouseMoveEvent(mouseMoveEvent);
	ioEvent.consumed = mouseMoveEvent.consumed;
}

//-----------------------------------------------------------------------------
void DGXYBox::onMouseMoveEvent(VSTGUI::MouseMoveEvent& ioEvent)
{
	if (!(ioEvent.buttonState.isLeft() && isEditing_any()))
	{
		return;
	}

	DiscreteValueConstrainer const dvcX(mControlX->asCControl()->isEditing() ? mControlX : nullptr);
	DiscreteValueConstrainer const dvcY(mControlY->asCControl()->isEditing() ? mControlY : nullptr);

	bool const isFineTune = (VSTGUI::buttonStateFromMouseEvent(ioEvent) & kZoomModifier);
	if ((mOldModifiers != ioEvent.modifiers) && isFineTune)
	{
		mOldValueX = mControlX->asCControl()->getValueNormalized();
		mOldValueY = mControlY->asCControl()->getValueNormalized();
		mOldModifiers = ioEvent.modifiers;
	}
	else if (!isFineTune)
	{
		mOldValueX = mControlX->asCControl()->getValueNormalized();
		mOldValueY = mControlY->asCControl()->getValueNormalized();
	}

	if (!lockX(ioEvent.modifiers))
	{
		auto const value = [this, pos = ioEvent.mousePosition, isFineTune]
		{
			auto const newValue = mapValueX(static_cast<float>(pos.x - mMouseableOrigin.x + mClickOffset.x) / mEffectiveRangeX);
			if (isFineTune)
			{
				return mOldValueX + ((newValue - mOldValueX) / getFineTuneFactor());
			}
			return newValue;
		}();
		mControlX->asCControl()->setValueNormalized(value);
	}

	if (!lockY(ioEvent.modifiers))
	{
		auto const value = [this, pos = ioEvent.mousePosition, isFineTune]
		{
			auto const newValue = mapValueY(static_cast<float>(pos.y - mMouseableOrigin.y + mClickOffset.y) / mEffectiveRangeY);
			if (isFineTune)
			{
				return mOldValueY + ((newValue - mOldValueY) / getFineTuneFactor());
			}
			return newValue;
		}();
		mControlY->asCControl()->setValueNormalized(value);
	}

	notifyIfChanged_all();

	ioEvent.consumed = true;
}

//-----------------------------------------------------------------------------
void DGXYBox::onMouseUpEvent(VSTGUI::MouseUpEvent& ioEvent)
{
	EndControl(mControlX);
	EndControl(mControlY);

	ioEvent.consumed = true;
}

//-----------------------------------------------------------------------------
void DGXYBox::onMouseCancelEvent(VSTGUI::MouseCancelEvent& ioEvent)
{
	CancelControl(mControlX, mStartValueX);
	CancelControl(mControlY, mStartValueY);
	notifyIfChanged_all();

	ioEvent.consumed = true;
}

//-----------------------------------------------------------------------------
void DGXYBox::onMouseWheelEvent(VSTGUI::MouseWheelEvent& ioEvent)
{
	bool anyConsumed = false;

	if (!dfx::math::IsZero(ioEvent.deltaX) && !lockX(ioEvent.modifiers))
	{
		auto const entryDeltaY = std::exchange(ioEvent.deltaY, 0.);
		detail::onMouseWheelEvent(mControlX, ioEvent);
		ioEvent.deltaY = entryDeltaY;
		anyConsumed |= ioEvent.consumed;
	}
	if (!dfx::math::IsZero(ioEvent.deltaY) && !lockY(ioEvent.modifiers))
	{
		auto const entryDeltaX = std::exchange(ioEvent.deltaX, 0.);
		detail::onMouseWheelEvent(mControlY, ioEvent);
		ioEvent.deltaX = entryDeltaX;
		anyConsumed |= ioEvent.consumed;
	}

	ioEvent.consumed = anyConsumed;
}

//-----------------------------------------------------------------------------
void DGXYBox::setAlternateHandles(VSTGUI::CBitmap* inHandleX, VSTGUI::CBitmap* inHandleY)
{
	assert(!inHandleX == !inHandleY);
	assert((inHandleX && inHandleY) ? (inHandleX->getSize() == inHandleY->getSize()) : true);
	assert((mMainHandleImage && inHandleX) ? (mMainHandleImage->getSize() == inHandleX->getSize()) : true);

	mAlternateHandleImageX = inHandleX;
	mAlternateHandleImageY = inHandleY;
}

#if TARGET_PLUGIN_USES_MIDI
//-----------------------------------------------------------------------------
void DGXYBox::setMidiLearner(bool /*inEnable*/)
{
	redraw();
}
#endif

//-----------------------------------------------------------------------------
float DGXYBox::invertIfStyle(float inValue, VSTGUI::CSliderBase::Style inStyle) const
{
	return (mStyle & inStyle) ? (1.0f - inValue) : inValue;
}

//-----------------------------------------------------------------------------
float DGXYBox::mapValueX(float inValue) const
{
	return invertIfStyle(inValue, VSTGUI::CSliderBase::kRight);
}

//-----------------------------------------------------------------------------
float DGXYBox::mapValueY(float inValue) const
{
	return invertIfStyle(inValue, VSTGUI::CSliderBase::kBottom);
}

//-----------------------------------------------------------------------------
// option/alt key locks the X axis from mouse changes
bool DGXYBox::lockX(VSTGUI::Modifiers inModifiers)
{
	return inModifiers.has(VSTGUI::ModifierKey::Alt);
}

//-----------------------------------------------------------------------------
// control key locks the Y axis from mouse changes
bool DGXYBox::lockY(VSTGUI::Modifiers inModifiers)
{
	return isPlatformControlKeySet(inModifiers);
}






#pragma mark -
#pragma mark DGAnimation

//-----------------------------------------------------------------------------
// DGAnimation
//-----------------------------------------------------------------------------
DGAnimation::DGAnimation(DfxGuiEditor*		inOwnerEditor, 
						 dfx::ParameterID	inParameterID, 
						 DGRect const&		inRegion, 
						 DGMultiFrameImage*	inAnimationImage)
:	DGControl<VSTGUI::CAnimKnob>(inRegion, inOwnerEditor, dfx::ParameterID_ToVST(inParameterID), inAnimationImage)
{
	assert(inAnimationImage);

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
void DGAnimation::onMouseDownEvent(VSTGUI::MouseDownEvent& ioEvent)
{
	DiscreteValueConstrainer const dvc(this);
	mEntryMousePos = ioEvent.mousePosition;
	ApplyMouseEventResult(VSTGUI::CAnimKnob::onMouseDown(ioEvent.mousePosition, ConstrainButtons(ioEvent, getNumStates())), ioEvent);
}

//------------------------------------------------------------------------
void DGAnimation::onMouseMoveEvent(VSTGUI::MouseMoveEvent& ioEvent)
{
	DiscreteValueConstrainer const dvc(isEditing() ? this : nullptr);
	auto pos = constrainMousePosition(ioEvent.mousePosition);
	ApplyMouseEventResult(VSTGUI::CAnimKnob::onMouseMoved(pos, ConstrainButtons(ioEvent, getNumStates())), ioEvent);
}

//------------------------------------------------------------------------
void DGAnimation::onMouseUpEvent(VSTGUI::MouseUpEvent& ioEvent)
{
	DiscreteValueConstrainer const dvc(this);
	auto pos = constrainMousePosition(ioEvent.mousePosition);
	ApplyMouseEventResult(VSTGUI::CAnimKnob::onMouseUp(pos, ConstrainButtons(ioEvent, getNumStates())), ioEvent);
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
