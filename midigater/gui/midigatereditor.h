/*------------------------------------------------------------------------
Copyright (C) 2001-2022  Sophia Poirier

This file is part of MIDI Gater.

MIDI Gater is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

MIDI Gater is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with MIDI Gater.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#pragma once

#include "dfxgui.h"

//-----------------------------------------------------------------------
class MIDIGaterEditor final : public DfxGuiEditor
{
public:
	explicit MIDIGaterEditor(DGEditorListenerInstance inInstance);
	void OpenEditor() override;
};
