/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2020  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Destroy FX Library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Destroy FX Library.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "dfxguicontrol.h"



//-----------------------------------------------------------------------------
bool detail::onWheel(IDGControl* inControl, VSTGUI::CPoint const& /*inPos*/, VSTGUI::CMouseWheelAxis const& /*inAxis*/, 
					 float const& inDistance, VSTGUI::CButtonState const& inButtons)
{
	inControl->onMouseWheelEditing();

	if (inControl->getNumStates() > 0)
	{
		long const delta = (inDistance < 0.0f) ? -1 : 1;
		auto newValue = inControl->getValue_i() + delta;
		inControl->setValue_i(newValue);
	}
	else
	{
		auto const cControl = inControl->asCControl();
		auto const delta = inDistance * cControl->getWheelInc() / ((inButtons & cControl->kZoomModifier) ? inControl->getFineTuneFactor() : 1.0f);
		cControl->setValueNormalized(cControl->getValueNormalized() + delta);
	}
	inControl->notifyIfChanged();

	return true;
}



#pragma mark DGNullControl

//-----------------------------------------------------------------------------
DGNullControl::DGNullControl(DfxGuiEditor* inOwnerEditor, DGRect const& inRegion, DGImage* inBackgroundImage)
:	DGControl<VSTGUI::CControl>(inRegion, inOwnerEditor, dfx::kParameterID_Invalid, inBackgroundImage)
{
	setMouseEnabled(false);
}

//-----------------------------------------------------------------------------
void DGNullControl::draw(VSTGUI::CDrawContext* inContext)
{
	if (auto const image = getDrawBackground())
	{
		image->draw(inContext, getViewSize());
	}
	setDirty(false);
}
