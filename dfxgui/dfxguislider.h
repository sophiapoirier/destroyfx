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
class DGAnimation : public DGControl<VSTGUI::CAnimKnob>
{
public:
	DGAnimation(DfxGuiEditor* inOwnerEditor, long inParamID, DGRect const& inRegion,  
				DGImage* inAnimationImage, long inNumAnimationFrames, DGImage* inBackground = nullptr);

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
