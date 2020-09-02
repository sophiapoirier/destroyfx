/*------------------------------------------------------------------------
Copyright (C) 2000-2020  Sophia Poirier

This file is part of Skidder.

Skidder is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Skidder is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Skidder.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#pragma once

#include "dfxgui.h"


//-----------------------------------------------------------------------
class SkidderEditor final : public DfxGuiEditor
{
public:
	SkidderEditor(DGEditorListenerInstance inInstance);

	long OpenEditor() override;
	void parameterChanged(long inParameterID) override;
	void numAudioChannelsChanged(unsigned long inNumChannels) override;

private:
	std::pair<long, long> GetActiveRateParameterIDs();
	void UpdateRandomMinimumDisplays();
	void HandleTempoSyncChange();
	void HandleTempoAutoChange();
	void HandleMidiModeChange();
	void SetParameterAlpha(long inParameterID, float inAlpha);

	// controls
	DGRangeSlider* mRateSlider = nullptr;
	DGTextDisplay* mRateDisplay = nullptr;
	DGTextDisplay* mRateRandMinDisplay = nullptr;
	DGTextDisplay* mPulsewidthRandMinDisplay = nullptr;
	DGTextDisplay* mFloorRandMinDisplay = nullptr;
};
