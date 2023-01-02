/*------------------------------------------------------------------------
Copyright (C) 2001-2023  Sophia Poirier

This file is part of Rez Synth.

Rez Synth is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Rez Synth is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rez Synth.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#pragma once


#include "dfxgui.h"


//-----------------------------------------------------------------------
class RezSynthEditor final : public DfxGuiEditor
{
public:
	using DfxGuiEditor::DfxGuiEditor;

	void OpenEditor() override;
	void CloseEditor() override;
	void parameterChanged(dfx::ParameterID inParameterID) override;
	void mouseovercontrolchanged(IDGControl* currentControlUnderMouse) override;

private:
	enum class Section
	{
		Bands,
		Envelope,
		MidiNotes,
		MidiBends,
		Mix
	};

	void HandleLegatoChange();
	std::string GetHelpForControl(IDGControl* inControl) const;
	static Section ParameterToSection(dfx::ParameterID inParameterID) noexcept;

	DGSlider* mSepAmountSlider = nullptr, * mBandwidthAmountSlider = nullptr;
	DGTextDisplay* mSepAmountDisplay = nullptr, * mBandwidthAmountDisplay = nullptr;
	DGHelpBox* mHelpBox = nullptr;
	IDGControl* mTitleArea = nullptr;
};
