/*------------------------------------------------------------------------
Copyright (C) 2001-2020  Tom Murphy 7 and Sophia Poirier

This file is part of Transverb.

Transverb is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Transverb is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Transverb.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#pragma once


#include <array>

#include "dfxgui.h"
#include "transverb-base.h"



//-----------------------------------------------------------------------------
class TransverbSpeedTuneButton final : public DGFineTuneButton
{
public:
	TransverbSpeedTuneButton(DfxGuiEditor* inOwnerEditor, long inParamID, DGRect const& inRegion, DGImage* inImage, 
							 float inValueChangeAmount)
	:	DGFineTuneButton(inOwnerEditor, inParamID, inRegion, inImage, inValueChangeAmount)
	{}

	VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint& inPos, VSTGUI::CButtonState const& inButtons) override;

	void setTuneMode(long inTuneMode) noexcept
	{
		mTuneMode = inTuneMode;
	}

private:
	long mTuneMode {};
};



//-----------------------------------------------------------------------------
class TransverbEditor final : public DfxGuiEditor
{
public:
	explicit TransverbEditor(DGEditorListenerInstance inInstance);

	long OpenEditor() override;
	void post_open() override;
	void parameterChanged(long inParameterID) override;
	void HandlePropertyChange(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned long inItemIndex) override;

private:
	static void HandleSpeedModeButton(size_t inIndex, long inValue, void* inEditor);
	void HandleSpeedModeChange(size_t inIndex);

	std::array<DGButton*, dfx::TV::kNumDelays> mSpeedModeButtons;
	std::array<TransverbSpeedTuneButton*, dfx::TV::kNumDelays> mSpeedDownButtons, mSpeedUpButtons;
	std::array<DGTextDisplay*, dfx::TV::kNumDelays> mDistanceTextDisplays;
};
