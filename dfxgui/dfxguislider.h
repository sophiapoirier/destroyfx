/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2021  Sophia Poirier

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

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#pragma once


#include "dfxguicontrol.h"


//-----------------------------------------------------------------------------
class DGSlider : public DGControl<VSTGUI::CSlider>
{
public:
	DGSlider(DfxGuiEditor* inOwnerEditor, long inParamID, DGRect const& inRegion, 
			 dfx::Axis inOrientation, DGImage* inHandleImage, DGImage* inBackgroundImage = nullptr, long inRangeMargin = 0);

#ifdef TARGET_API_RTAS
	void draw(VSTGUI::CDrawContext* inContext) override;
#endif
	VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons) override;
	VSTGUI::CMouseEventResult onMouseMoved(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons) override;
	VSTGUI::CMouseEventResult onMouseUp(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons) override;

	void setHandle(VSTGUI::CBitmap* inHandle) override;
	void setAlternateHandle(VSTGUI::CBitmap* inHandle);
	void setUseAlternateHandle(bool inEnable);

#if TARGET_PLUGIN_USES_MIDI
	void setMidiLearner(bool inEnable) override;
#endif

	CLASS_METHODS(DGSlider, VSTGUI::CSlider)

private:
	VSTGUI::SharedPointer<VSTGUI::CBitmap> mMainHandleImage;
	VSTGUI::SharedPointer<VSTGUI::CBitmap> mAlternateHandleImage;
};



#pragma mark -
//-----------------------------------------------------------------------------
class DGRangeSlider : public DGMultiControl<VSTGUI::CControl>
{
public:
	enum class PushStyle
	{
		Neither,
		Both,
		Lower,
		Upper
	};

	DGRangeSlider(DfxGuiEditor* inOwnerEditor, long inLowerParamID, long inUpperParamID, DGRect const& inRegion, 
				  DGImage* inLowerHandleImage, DGImage* inUpperHandleImage, 
				  DGImage* inBackgroundImage = nullptr, 
				  PushStyle inPushStyle = PushStyle::Neither, 
				  long inOvershoot = 0);

	void draw(VSTGUI::CDrawContext* inContext) override;
	VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons) override;
	VSTGUI::CMouseEventResult onMouseMoved(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons) override;
	VSTGUI::CMouseEventResult onMouseUp(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons) override;
	VSTGUI::CMouseEventResult onMouseCancel() override;

	void setAlternateHandles(VSTGUI::CBitmap* inLowerHandle, VSTGUI::CBitmap* inUpperHandle);

#if TARGET_PLUGIN_USES_MIDI
	void setMidiLearner(bool inEnable) override;
#endif

	CLASS_METHODS(DGRangeSlider, VSTGUI::CControl)

private:
	IDGControl* const mLowerControl;
	IDGControl* const mUpperControl;

	VSTGUI::SharedPointer<VSTGUI::CBitmap> const mLowerMainHandleImage, mUpperMainHandleImage;
	VSTGUI::SharedPointer<VSTGUI::CBitmap> mLowerAlternateHandleImage, mUpperAlternateHandleImage;

	VSTGUI::CCoord const mMinXPos;	// min X position in pixel
	VSTGUI::CCoord const mMaxXPos;	// max X position in pixel
	float const mEffectiveRange;
	PushStyle const mPushStyle;
	long const mOvershoot;	// how far (in pixel) you can click outside of being in between the points and still be close enough to be considered in between

	// mouse-down state
	bool mClickBetween = false;
	float mClickStartValue = 0.0f;
	float mClickOffsetInValue = 0.0f;
	float mLowerStartValue = 0.0f, mUpperStartValue = 0.0f;
	float mAnchorStartValue = 0.0f;
	IDGControl* mAnchorControl = nullptr;

	float mNewValue = 0.0f, mOldValue = 0.0f;
	float mFineTuneStartValue = 0.0f;
	VSTGUI::CCoord mOldPosY {};
	VSTGUI::CButtonState mOldButtons;
};



#pragma mark -
//-----------------------------------------------------------------------------
class DGXYBox : public DGMultiControl<VSTGUI::CControl>
{
public:
	DGXYBox(DfxGuiEditor* inOwnerEditor, long inParamIDX, long inParamIDY, DGRect const& inRegion, 
			DGImage* inHandleImage, DGImage* inBackgroundImage = nullptr, 
			int inStyle = VSTGUI::CSliderBase::kLeft | VSTGUI::CSliderBase::kBottom);

	void draw(VSTGUI::CDrawContext* inContext) override;
	VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons) override;
	VSTGUI::CMouseEventResult onMouseMoved(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons) override;
	VSTGUI::CMouseEventResult onMouseUp(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons) override;
	VSTGUI::CMouseEventResult onMouseCancel() override;
	bool onWheel(VSTGUI::CPoint const& inPos, VSTGUI::CMouseWheelAxis const& inAxis, float const& inDistance, VSTGUI::CButtonState const& inButtons) override;

	void setAlternateHandles(VSTGUI::CBitmap* inHandleX, VSTGUI::CBitmap* inHandleY);

	// If true, the handle is only placed at integral positions,
	// for pixel-perfect rendering.
	void setIntegralPosition(bool inParam) noexcept
	{
		mIntegralPosition = inParam;
	}

#if TARGET_PLUGIN_USES_MIDI
	void setMidiLearner(bool inEnable) override;
#endif

	CLASS_METHODS(DGXYBox, VSTGUI::CControl)

private:
	float invertIfStyle(float inValue, VSTGUI::CSliderBase::Style inStyle) const;
	float mapValueX(float inValue) const;
	float mapValueY(float inValue) const;

	IDGControl* const mControlX;
	IDGControl* const mControlY;
	VSTGUI::SharedPointer<VSTGUI::CBitmap> const mMainHandleImage;
	/*VSTGUI::CSliderBase::Style*/int const mStyle;
	VSTGUI::SharedPointer<VSTGUI::CBitmap> mAlternateHandleImageX, mAlternateHandleImageY;
	VSTGUI::CCoord const mHandleWidth, mHandleHeight;
	VSTGUI::CCoord const mMinXPos, mMaxXPos, mMinYPos, mMaxYPos;
	VSTGUI::CPoint const mMouseableOrigin;
	float const mEffectiveRangeX, mEffectiveRangeY;

	// mouse-down state
	float mStartValueX = 0.0f, mStartValueY = 0.0f;
	VSTGUI::CPoint mClickOffset;

	float mOldValueX = 0.0f, mOldValueY = 0.0f;
	VSTGUI::CButtonState mOldButtons;
	bool mIntegralPosition = false;
};



#pragma mark -
//-----------------------------------------------------------------------------
class DGAnimation : public DGControl<VSTGUI::CAnimKnob>
{
public:
	DGAnimation(DfxGuiEditor* inOwnerEditor, long inParamID, DGRect const& inRegion, 
				DGImage* inAnimationImage, long inNumAnimationFrames);

#ifdef TARGET_API_RTAS
	void draw(VSTGUI::CDrawContext* inContext) override;
#endif
	VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons) override;
	VSTGUI::CMouseEventResult onMouseMoved(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons) override;
	VSTGUI::CMouseEventResult onMouseUp(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons) override;

	void setMouseAxis(dfx::Axis inMouseAxis) noexcept
	{
		mMouseAxis = inMouseAxis;
	}

	CLASS_METHODS(DGAnimation, VSTGUI::CAnimKnob)

private:
	VSTGUI::CPoint constrainMousePosition(VSTGUI::CPoint const& inPos) const noexcept;

	dfx::Axis mMouseAxis = dfx::kAxis_Omni;
	VSTGUI::CPoint mEntryMousePos;
};
