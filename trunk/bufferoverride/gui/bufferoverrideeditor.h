/*------------------------------------------------------------------------
Copyright (C) 2001-2015  Sophia Poirier

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

#ifndef __BUFFER_OVERRIDE_EDITOR_H
#define __BUFFER_OVERRIDE_EDITOR_H


#include "dfxgui.h"


//-----------------------------------------------------------------------------
class BufferOverrideEditor : public DfxGuiEditor
{
public:
	BufferOverrideEditor(DGEditorListenerInstance inInstance);

	virtual long OpenEditor();
	virtual void parameterChanged(long inParameterID);
	virtual void mouseovercontrolchanged(DGControl * currentControlUnderMouse);

private:
	DGSlider * bufferSizeSlider;
	DGSlider * divisorLFOrateSlider;
	DGSlider * bufferLFOrateSlider;
	DGTextDisplay * bufferSizeDisplay;
	DGTextDisplay * divisorLFOrateDisplay;
	DGTextDisplay * bufferLFOrateDisplay;

	DGStaticTextDisplay * helpDisplay;
};


#endif
