/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2018  Sophia Poirier

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

#include <cassert>
#include <cmath>

#include "dfxguieditor.h"
#include "dfxmisc.h"



//-----------------------------------------------------------------------------
template <class T>
template <typename... Args>
DGControl<T>::DGControl(Args&&... args)
:	T(std::forward<Args>(args)...),
	mOwnerEditor(dynamic_cast<DfxGuiEditor*>(T::getListener()))
{
	assert(T::getListener() ? (mOwnerEditor != nullptr) : true);
	assert(isParameterAttached() ? (mOwnerEditor != nullptr) : true);

	pullNumStatesFromParameter();
}

//-----------------------------------------------------------------------------
template <class T>
void DGControl<T>::setValue_gen(float inValue)
{
	T::setValueNormalized(inValue);
}

//-----------------------------------------------------------------------------
template <class T>
void DGControl<T>::setDefaultValue_gen(float inValue)
{
	T::setDefaultValue((inValue * T::getRange()) + T::getMin());
}

//-----------------------------------------------------------------------------
template <class T>
long DGControl<T>::getValue_i()
{
	assert(getNumStates() > 0);

	auto const valueNormalized = T::getValueNormalized();
	if (isParameterAttached())
	{
		return std::lround(getOwnerEditor()->dfxgui_ExpandParameterValue(getParameterID(), valueNormalized));
	}

	auto const maxValue_f = static_cast<float>(getNumStates() - 1);
	return static_cast<long>((valueNormalized * maxValue_f) + DfxParam::kIntegerPadding);
}

//-----------------------------------------------------------------------------
template <class T>
void DGControl<T>::setValue_i(long inValue)
{
	assert(getNumStates() > 0);

	float newValue_f = 0.0f;
	if (isParameterAttached())
	{
		newValue_f = getOwnerEditor()->dfxgui_ContractParameterValue(getParameterID(), static_cast<float>(inValue));
	}
	else
	{
		auto const maxValue = getNumStates() - 1;
		if (inValue >= maxValue)
		{
			newValue_f = 1.0f;
		}
		else if (inValue <= 0)
		{
			newValue_f = 0.0f;
		}
		else
		{
			float const maxValue_f = static_cast<float>(maxValue) + DfxParam::kIntegerPadding;
			if (maxValue_f > 0.0f)  // avoid division by zero
			{
				newValue_f = static_cast<float>(inValue) / maxValue_f;
			}
		}
	}

	T::setValueNormalized(newValue_f);
	T::bounceValue();
}

//-----------------------------------------------------------------------------
template <class T>
void DGControl<T>::redraw()
{
//	T::invalid();  // XXX CView::invalid calls setDirty(false), which can have undesired consequences for control value handling
	T::invalidRect(T::getViewSize());
}

//-----------------------------------------------------------------------------
template <class T>
long DGControl<T>::getParameterID() const
{
	return T::getTag();
}

//-----------------------------------------------------------------------------
template <class T>
void DGControl<T>::setParameterID(long inParameterID)
{
	T::setTag(inParameterID);
	pullNumStatesFromParameter();
	if (mOwnerEditor)
	{
		setValue_gen(mOwnerEditor->getparameter_gen(inParameterID));
	}
	T::setDirty();
}

//-----------------------------------------------------------------------------
template <class T>
bool DGControl<T>::isParameterAttached() const
{
	return (getParameterID() >= 0);
}

//-----------------------------------------------------------------------------
template <class T>
void DGControl<T>::setNumStates(long inNumStates)
{
	if (inNumStates >= 0)
	{
		mNumStates = inNumStates;
		T::setDirty();
	}
	else
	{
		assert(false);
	}
}

//-----------------------------------------------------------------------------
template <class T>
void DGControl<T>::setDrawAlpha(float inAlpha)
{
	T::setAlphaValue(inAlpha);
}

//-----------------------------------------------------------------------------
template <class T>
float DGControl<T>::getDrawAlpha() const
{
	return T::getAlphaValue();
}

//-----------------------------------------------------------------------------
template <class T>
bool DGControl<T>::setHelpText(char const* inText)
{
	assert(inText);
	return T::setAttribute(kCViewTooltipAttribute, strlen(inText) + 1, inText);
}

#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
template <class T>
bool DGControl<T>::setHelpText(CFStringRef inText)
{
	assert(inText);
	if (auto const cString = dfx::CreateCStringFromCFString(inText))
	{
		return setHelpText(cString.get());
	}
	return false;
}
#endif

//-----------------------------------------------------------------------------
template <class T>
void DGControl<T>::pullNumStatesFromParameter()
{
	if (isParameterAttached() && mOwnerEditor)
	{
		long numStates = 0;
		auto const paramID = getParameterID();
		if (mOwnerEditor->GetParameterValueType(paramID) != DfxParam::ValueType::Float)
		{
			auto const valueRange = mOwnerEditor->GetParameter_maxValue(paramID) - mOwnerEditor->GetParameter_minValue(paramID);
			numStates = std::lround(valueRange) + 1;
		}
		setNumStates(numStates);
	}
}
