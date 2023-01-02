/*------------------------------------------------------------------------
Copyright (C) 2002-2023  Sophia Poirier

This file is part of Scrubby.

Scrubby is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 2 of the License, or 
(at your option) any later version.

Scrubby is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Scrubby.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#pragma once


#include <vector>

#include "dfxgui.h"


//-----------------------------------------------------------------------
class ScrubbyEditor final : public DfxGuiEditor
{
public:
	using DfxGuiEditor::DfxGuiEditor;

	void OpenEditor() override;
	void CloseEditor() override;
	void parameterChanged(dfx::ParameterID inParameterID) override;
	void mouseovercontrolchanged(IDGControl* currentControlUnderMouse) override;
	void outputChannelsChanged(size_t inChannelCount) override;

private:
	std::pair<dfx::ParameterID, dfx::ParameterID> GetActiveSeekRateParameterIDs();
	void UpdateRandomMinimumDisplays();
	void HandleNotesButton(size_t inNotesButtonType);
	void HandlePitchConstraintChange();
	void HandleTempoSyncChange();
	void HandleTempoAutoChange();
	std::string GetHelpForControl(IDGControl* inControl) const;

	DGRangeSlider* mSeekRateSlider = nullptr;
	DGTextDisplay* mSeekRateDisplay = nullptr;
	DGTextDisplay* mSeekRateRandMinDisplay = nullptr;
	DGTextDisplay* mSeekDurRandMinDisplay = nullptr;

	DGHelpBox* mHelpBox = nullptr;
	std::vector<DGButton*> mNotesButtons;
	IDGControl* mTitleArea = nullptr;
};
