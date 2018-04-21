/*------------------------------------------------------------------------
Copyright (C) 2001-2018  Tom Murphy 7 and Sophia Poirier

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


#include "dfxgui.h"



//-----------------------------------------------------------------------------
class TransverbSpeedTuneButton : public DGFineTuneButton
{
public:
	TransverbSpeedTuneButton(DfxGuiEditor* inOwnerEditor, long inParamID, DGRect const& inRegion, DGImage* inImage, 
							 float inValueChangeAmount, long inTuneMode)
	:	DGFineTuneButton(inOwnerEditor, inParamID, inRegion, inImage, inValueChangeAmount), 
		mTuneMode(inTuneMode)
	{}

	CMouseEventResult onMouseDown(CPoint& inPos, CButtonState const& inButtons) override;

	void setTuneMode(long inTuneMode) noexcept
	{
		mTuneMode = inTuneMode;
	}

private:
	long mTuneMode;
};



//-----------------------------------------------------------------------------
class TransverbEditor : public DfxGuiEditor
{
public:
	TransverbEditor(DGEditorListenerInstance inInstance);

	long OpenEditor() override;
	void parameterChanged(long inParameterID) override;

private:
	TransverbSpeedTuneButton* mSpeed1UpButton = nullptr, * mSpeed1DownButton = nullptr;
	TransverbSpeedTuneButton* mSpeed2UpButton = nullptr, * mSpeed2DownButton = nullptr;
	DGTextDisplay* mDistance1TextDisplay = nullptr, * mDistance2TextDisplay = nullptr;
};
