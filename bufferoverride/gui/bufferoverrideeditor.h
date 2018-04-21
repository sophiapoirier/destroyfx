/*------------------------------------------------------------------------
Copyright (C) 2001-2018  Sophia Poirier

This file is part of Buffer Override.

Buffer Override is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
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


#include "dfxgui.h"


//-----------------------------------------------------------------------------
class BufferOverrideEditor : public DfxGuiEditor
{
public:
	BufferOverrideEditor(DGEditorListenerInstance inInstance);

	long OpenEditor() override;
	void parameterChanged(long inParameterID) override;
	void mouseovercontrolchanged(DGControl* currentControlUnderMouse) override;

private:
	DGSlider* mBufferSizeSlider = nullptr;
	DGSlider* mDivisorLFORateSlider = nullptr;
	DGSlider* mBufferLFORateSlider = nullptr;
	DGTextDisplay* mBufferSizeDisplay = nullptr;
	DGTextDisplay* mDivisorLFORateDisplay = nullptr;
	DGTextDisplay* mBufferLFORateDisplay = nullptr;

	DGStaticTextDisplay* mHelpDisplay = nullptr;
};
