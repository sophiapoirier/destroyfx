/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2023  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 2 of the License, or 
(at your option) any later version.

Destroy FX Library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Destroy FX Library.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#include "dfxguicontrol.h"

#include "dfxmath.h"
#include "dfxplugin-base.h"



//-----------------------------------------------------------------------------
void detail::onMouseWheelEvent(IDGControl* inControl, VSTGUI::MouseWheelEvent& ioEvent)
{
	inControl->onMouseWheelEditing();

	if (inControl->getNumStates() > 0)
	{
		auto const delta = mouseWheelEventIntegralCompositeDelta(ioEvent);
		auto const newValue = inControl->getValue_i() + delta;
		inControl->setValue_i(newValue);
	}
	else
	{
		auto const compositeDelta = static_cast<float>(ioEvent.deltaX + ioEvent.deltaY);  // TODO: limit controls to one axis?
		auto const cControl = inControl->asCControl();
		auto const changeAmountDivisor = (VSTGUI::buttonStateFromEventModifiers(ioEvent.modifiers) & cControl->kZoomModifier) ? inControl->getFineTuneFactor() : 1.f;
		auto const delta = compositeDelta * cControl->getWheelInc() / changeAmountDivisor;
		cControl->setValueNormalized(cControl->getValueNormalized() + delta);
	}
	inControl->notifyIfChanged();

	ioEvent.consumed = true;
}

//-----------------------------------------------------------------------------
int detail::mouseWheelEventIntegralCompositeDelta(VSTGUI::MouseWheelEvent const& inEvent)
{
	auto const compositeDelta = inEvent.deltaX + inEvent.deltaY;
	return dfx::math::IsZero(compositeDelta) ? 0 : ((compositeDelta < 0.) ? -1 : 1);
}



#pragma mark DGNullControl

//-----------------------------------------------------------------------------
DGNullControl::DGNullControl(DfxGuiEditor* inOwnerEditor, DGRect const& inRegion, DGImage* inBackgroundImage)
:	DGControl<VSTGUI::CControl>(inRegion, inOwnerEditor, dfx::ParameterID_ToVST(dfx::kParameterID_Invalid), inBackgroundImage)
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
	else if (mBackgroundColor)
	{
		inContext->setDrawMode(VSTGUI::kAliasing);
		inContext->setFillColor(*mBackgroundColor);
		inContext->drawRect(getViewSize(), VSTGUI::kDrawFilled);
	}
	setDirty(false);
}

//-----------------------------------------------------------------------------
void DGNullControl::setBackgroundColor(DGColor inColor)
{
	mBackgroundColor = inColor;
}
