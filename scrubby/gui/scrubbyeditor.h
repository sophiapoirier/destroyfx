/*------------------------------------------------------------------------
Copyright (C) 2002-2018  Sophia Poirier

This file is part of Scrubby.

Scrubby is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Scrubby is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Scrubby.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#pragma once


#include <vector>

#include "dfxgui.h"


//--------------------------------------------------------------------------
class ScrubbyHelpBox : public DGStaticTextDisplay
{
public:
	ScrubbyHelpBox(DGRect const& inRegion, DGImage* inBackground);

	void draw(CDrawContext* inContext) override;

	void setDisplayItem(long inItemNum);

	CLASS_METHODS(ScrubbyHelpBox, DGStaticTextDisplay)

private:
	long mItemNum;
};


//-----------------------------------------------------------------------
class ScrubbyEditor : public DfxGuiEditor
{
public:
	ScrubbyEditor(DGEditorListenerInstance inInstance);

	long OpenEditor() override;
	void parameterChanged(long inParameterID) override;
	void mouseovercontrolchanged(IDGControl* currentControlUnderMouse) override;
	void numAudioChannelsChanged(unsigned long inNewNumChannels) override;

	void HandleNotesButton(long inNotesButtonType);
	void HandlePitchConstraintChange();

private:
	DGSlider* mSeekRateSlider = nullptr;
//	DGSlider* mSeekRateRandMinSlider = nullptr;
	DGTextDisplay* mSeekRateDisplay = nullptr;
	DGTextDisplay* mSeekRateRandMinDisplay = nullptr;

	ScrubbyHelpBox* mHelpBox = nullptr;
	std::vector<DGButton*> mNotesButtons;
	DGButton* mMidiLearnButton = nullptr, * mMidiResetButton = nullptr;
	IDGControl* mTitleArea = nullptr;
};
