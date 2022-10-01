/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2018-2022  Sophia Poirier

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

#pragma once


#include <vector>

#include "dfxdefines.h"



//-----------------------------------------------------------------------------
class DfxGuiEditor;
namespace VSTGUI
{
	class CControl;
}



//-----------------------------------------------------------------------------
class IDGControl
{
public:
	virtual ~IDGControl() = default;

	virtual VSTGUI::CControl* asCControl() = 0;
	virtual VSTGUI::CControl const* asCControl() const = 0;
	virtual DfxGuiEditor* getOwnerEditor() const noexcept = 0;

	virtual void setValue_gen(float inValue) = 0;
	virtual void setDefaultValue_gen(float inValue) = 0;
	virtual long getValue_i() = 0;
	virtual void setValue_i(long inValue) = 0;
	virtual void redraw() = 0;
	virtual dfx::ParameterID getParameterID() const = 0;
	virtual void setParameterID(dfx::ParameterID inParameterID) = 0;
	virtual bool isParameterAttached() const = 0;
	virtual size_t getNumStates() const = 0;
	virtual float getFineTuneFactor() const = 0;
	virtual bool notifyIfChanged() = 0;
	virtual void onMouseWheelEditing() = 0;
	virtual void invalidateMouseWheelEditingTimer() = 0;

	virtual void setDrawAlpha(float inAlpha) = 0;
	virtual float getDrawAlpha() const = 0;

	virtual void setHelpText(char const* inText) = 0;

	// TODO: C++20 use std::ranges::view?
	virtual std::vector<IDGControl*> getChildren() = 0;
	virtual std::vector<IDGControl const*> getChildren() const = 0;

#if TARGET_PLUGIN_USES_MIDI
	virtual void setMidiLearner(bool inEnable) = 0;
#endif
};
