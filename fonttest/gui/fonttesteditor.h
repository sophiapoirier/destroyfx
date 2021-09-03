/*------------------------------------------------------------------------
Copyright (C) 2001-2021  Tom Murphy 7 and Sophia Poirier

This file is part of FontTest.

FontTest is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

FontTest is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with FontTest.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#pragma once


#include <array>

#include "dfxgui.h"
#include "fonttest-base.h"


//-----------------------------------------------------------------------------
class FontTestEditor final : public DfxGuiEditor
{
public:
	explicit FontTestEditor(DGEditorListenerInstance inInstance);

	long OpenEditor() override;
	void PostOpenEditor() override;
	void CloseEditor() override;
	void parameterChanged(long inParameterID) override;
	void HandlePropertyChange(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned long inItemIndex) override;

private:
};
