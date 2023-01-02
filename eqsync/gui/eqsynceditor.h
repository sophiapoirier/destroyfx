/*------------------------------------------------------------------------
Copyright (C) 2001-2023  Sophia Poirier

This file is part of EQ Sync.

EQ Sync is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

EQ Sync is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with EQ Sync.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#pragma once

#include "dfxgui.h"


//--------------------------------------------------------------------------
class EQSyncEditor final : public DfxGuiEditor
{
public:
	using DfxGuiEditor::DfxGuiEditor;
	void OpenEditor() override;
	void parameterChanged(dfx::ParameterID inParameterID) override;

private:
	void HandleTempoAutoChange();
};
