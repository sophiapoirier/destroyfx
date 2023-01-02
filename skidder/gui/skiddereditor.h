/*------------------------------------------------------------------------
Copyright (C) 2000-2023  Sophia Poirier

This file is part of Skidder.

Skidder is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Skidder is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Skidder.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#pragma once

#include <string>
#include <utility>

#include "dfxgui.h"


//-----------------------------------------------------------------------
class SkidderEditor final : public DfxGuiEditor
{
public:
	using DfxGuiEditor::DfxGuiEditor;

	void OpenEditor() override;
	void CloseEditor() override;
	void parameterChanged(dfx::ParameterID inParameterID) override;
	void outputChannelsChanged(size_t inChannelCount) override;
	void mouseovercontrolchanged(IDGControl* currentControlUnderMouse) override;

private:
	std::pair<dfx::ParameterID, dfx::ParameterID> GetActiveRateParameterIDs();
	void UpdateRandomMinimumDisplays();
	void HandleTempoSyncChange();
	void HandleTempoAutoChange();
	void HandleCrossoverModeChange();
	void HandleMidiModeChange();
	std::string GetHelpForControl(IDGControl* inControl) const;

	DGRangeSlider* mRateSlider = nullptr;
	DGTextDisplay* mRateDisplay = nullptr;
	DGTextDisplay* mRateRandMinDisplay = nullptr;
	DGTextDisplay* mPulsewidthRandMinDisplay = nullptr;
	DGTextDisplay* mFloorRandMinDisplay = nullptr;
	DGHelpBox* mHelpBox = nullptr;
	IDGControl* mTitleArea = nullptr;
};
