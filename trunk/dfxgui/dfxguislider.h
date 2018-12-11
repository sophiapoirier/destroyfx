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

#pragma once


#include "dfxguicontrol.h"


//-----------------------------------------------------------------------------
class DGSlider : public DGControl<CSlider>
{
public:
	DGSlider(DfxGuiEditor* inOwnerEditor, long inParamID, DGRect const& inRegion, 
			 dfx::Axis inOrientation, DGImage* inHandleImage, DGImage* inBackgroundImage = nullptr, long inRangeMargin = 0);

#ifdef TARGET_API_RTAS
	void draw(CDrawContext* inContext) override;
#endif
	CMouseEventResult onMouseDown(CPoint& inPos, CButtonState const& inButtons) override;
	CMouseEventResult onMouseMoved(CPoint& inPos, CButtonState const& inButtons) override;
	CMouseEventResult onMouseUp(CPoint& inPos, CButtonState const& inButtons) override;

	void setHandle(CBitmap* inHandle) override;
	void setAlternateHandle(CBitmap* inHandle);
	void setUseAlternateHandle(bool inEnable);

#if TARGET_PLUGIN_USES_MIDI
	void setMidiLearner(bool inEnable) override;
#endif

	CLASS_METHODS(DGSlider, CSlider)

private:
	SharedPointer<CBitmap> mMainHandleImage;
	SharedPointer<CBitmap> mAlternateHandleImage;
};



#pragma mark -
//-----------------------------------------------------------------------------
class DGAnimation : public DGControl<CAnimKnob>
{
public:
	DGAnimation(DfxGuiEditor* inOwnerEditor, long inParamID, DGRect const& inRegion,  
				DGImage* inAnimationImage, long inNumAnimationFrames, DGImage* inBackground = nullptr);

#ifdef TARGET_API_RTAS
	void draw(CDrawContext* inContext) override;
#endif
	CMouseEventResult onMouseDown(CPoint& inPos, CButtonState const& inButtons) override;
	CMouseEventResult onMouseMoved(CPoint& inPos, CButtonState const& inButtons) override;
	CMouseEventResult onMouseUp(CPoint& inPos, CButtonState const& inButtons) override;

	void setMouseAxis(dfx::Axis inMouseAxis) noexcept
	{
		mMouseAxis = inMouseAxis;
	}

	CLASS_METHODS(DGAnimation, CAnimKnob)

private:
	CPoint constrainMousePosition(CPoint const& inPos) const noexcept;

	dfx::Axis mMouseAxis = dfx::kAxis_Omni;
	CPoint mEntryMousePos;
};
