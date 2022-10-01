/*------------------------------------------------------------------------
Copyright (C) 2001-2022  Tom Murphy 7 and Sophia Poirier

This file is part of Transverb.

Transverb is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Transverb is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Transverb.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#pragma once


#include <array>

#include "dfxgui.h"
#include "transverb-base.h"



//-----------------------------------------------------------------------------
class TransverbSpeedTuneButton final : public DGFineTuneButton
{
public:
	TransverbSpeedTuneButton(DfxGuiEditor* inOwnerEditor, dfx::ParameterID inParameterID, DGRect const& inRegion,
							 DGImage* inImage, float inValueChangeAmount)
	:	DGFineTuneButton(inOwnerEditor, inParameterID, inRegion, inImage, inValueChangeAmount)
	{}

	void onMouseDownEvent(VSTGUI::MouseDownEvent& ioEvent) override;

	void setTuneMode(unsigned int inTuneMode) noexcept
	{
		mTuneMode = inTuneMode;
	}

	CLASS_METHODS(TransverbSpeedTuneButton, DGFineTuneButton)

private:
	unsigned int mTuneMode {};
};



//-----------------------------------------------------------------------------
class TransverbEditor final : public DfxGuiEditor
{
public:
	explicit TransverbEditor(DGEditorListenerInstance inInstance);

	long OpenEditor() override;
	void PostOpenEditor() override;
	void CloseEditor() override;
	void parameterChanged(dfx::ParameterID inParameterID) override;
	void HandlePropertyChange(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned int inItemIndex) override;

private:
	void HandleSpeedModeButton(size_t inIndex, long inValue);
	void HandleSpeedModeChange(size_t inIndex);
	void HandleFreezeChange();

	std::array<DGButton*, dfx::TV::kNumDelays> mSpeedModeButtons {};
	std::array<TransverbSpeedTuneButton*, dfx::TV::kNumDelays> mSpeedDownButtons {}, mSpeedUpButtons {};
	std::array<DGTextDisplay*, dfx::TV::kNumDelays> mDistanceTextDisplays {};
};
