/*------------------------------------------------------------------------
Copyright (C) 2001-2021  Sophia Poirier

This file is part of Buffer Override.

Buffer Override is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Buffer Override is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Buffer Override.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#pragma once


#include <array>

#include "dfxgui.h"


//-----------------------------------------------------------------------------
class BufferOverrideEditor final : public DfxGuiEditor
{
public:
	explicit BufferOverrideEditor(DGEditorListenerInstance inInstance);

	long OpenEditor() override;
	void CloseEditor() override;
	void parameterChanged(long inParameterID) override;
	void mouseovercontrolchanged(IDGControl* currentControlUnderMouse) override;

private:
	static constexpr size_t kHelpDisplayLineCount = 2;

	void HandleTempoSyncChange();
	void HandleTempoAutoChange();

	DGXYBox* mDivisorBufferBox = nullptr;
	DGSlider* mDivisorLFORateSlider = nullptr;
	DGSlider* mBufferLFORateSlider = nullptr;
	DGTextDisplay* mBufferSizeDisplay = nullptr;
	DGTextDisplay* mDivisorLFORateDisplay = nullptr;
	DGTextDisplay* mBufferLFORateDisplay = nullptr;

	std::array<DGStaticTextDisplay*, kHelpDisplayLineCount> mHelpDisplays {};
};
