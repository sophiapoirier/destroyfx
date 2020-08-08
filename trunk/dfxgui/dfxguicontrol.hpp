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

#include <algorithm>
#include <cassert>
#include <cmath>
#include <memory>
#include <numeric>
#include <type_traits>

#include "dfxguieditor.h"
#include "dfxmisc.h"



//-----------------------------------------------------------------------------
template <class T>
template <typename... Args>
DGControl<T>::DGControl(Args&&... args)
:	T(std::forward<Args>(args)...),
	mOwnerEditor(dynamic_cast<DfxGuiEditor*>(T::getListener())),
	mMouseWheelEditingSupport(this)
{
	assert(T::getListener() ? (mOwnerEditor != nullptr) : true);
	assert(isParameterAttached() ? (mOwnerEditor != nullptr) : true);

	initValues();
	T::setOldValue(T::getValue());
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
	if (inParameterID != getParameterID())
	{
		T::setTag(inParameterID);
		initValues();
		T::invalid();
	}
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
bool DGControl<T>::notifyIfChanged()
{
	auto const entryDirty = T::isDirty();
	if (entryDirty)
	{
		T::valueChanged();
		T::invalid();
	}
	return entryDirty;
}

//-----------------------------------------------------------------------------
namespace detail
{
	bool onWheel(IDGControl* inControl, VSTGUI::CPoint const& inPos, VSTGUI::CMouseWheelAxis const& inAxis, float const& inDistance, VSTGUI::CButtonState const& inButtons);
}

//-----------------------------------------------------------------------------
template <class T>
bool DGControl<T>::onWheel(VSTGUI::CPoint const& inPos, VSTGUI::CMouseWheelAxis const& inAxis, float const& inDistance, VSTGUI::CButtonState const& inButtons)
{
	return detail::onWheel(this, inPos, inAxis, inDistance, inButtons);
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
void DGControl<T>::setHelpText(char const* inText)
{
	assert(inText);
	T::setTooltipText(inText);
}

#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
template <class T>
void DGControl<T>::setHelpText(CFStringRef inText)
{
	assert(inText);
	if (auto const cString = dfx::CreateCStringFromCFString(inText))
	{
		setHelpText(cString.get());
	}
}
#endif

//-----------------------------------------------------------------------------
template <class T>
void DGControl<T>::initValues()
{
	pullNumStatesFromParameter();

	if (isParameterAttached() && mOwnerEditor)
	{
		auto const value_norm = [this]
		{
#ifdef TARGET_API_RTAS
			if (auto const process = mOwnerEditor->dfxgui_GetEffectInstance())
			{
				auto const parameterIndex_rtas = dfx::ParameterID_ToRTAS(getParameterID());
				long value_rtas {};
				process->GetControlValue(parameterIndex_rtas, &value_rtas);
				return ConvertToVSTValue(value_rtas);
			}
#endif
			return mOwnerEditor->getparameter_gen(getParameterID());
		}();
		auto const defaultValue_norm = [this]
		{
#ifdef TARGET_API_RTAS
			if (auto const process = mOwnerEditor->dfxgui_GetEffectInstance())
			{
				auto const parameterIndex_rtas = dfx::ParameterID_ToRTAS(getParameterID());
				long defaultValue_rtas {};
				process->GetControlDefaultValue(parameterIndex_rtas, &defaultValue_rtas);
				return ConvertToVSTValue(defaultValue_rtas);
			}
#endif
			auto const defaultValue = mOwnerEditor->GetParameter_defaultValue(getParameterID());
			return mOwnerEditor->dfxgui_ContractParameterValue(getParameterID(), defaultValue);
		}();
		setValue_gen(value_norm);
		setDefaultValue_gen(defaultValue_norm);
	}
	else
	{
		T::setValue(T::getMin());
		T::setDefaultValue(T::getMin() + (T::getRange() / 2.0f));
	}
}

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



#pragma mark -
#pragma mark DGMultiControl

//-----------------------------------------------------------------------------
template <class T>
void DGMultiControl<T>::setViewSize(VSTGUI::CRect const& inPos, bool inInvalidate)
{
	DGControl<T>::setViewSize(inPos, inInvalidate);
	forEachChild([inPos, inInvalidate](auto&& child){ child->asCControl()->setViewSize(inPos, inInvalidate); });
}

//-----------------------------------------------------------------------------
template <class T>
bool DGMultiControl<T>::onWheel(VSTGUI::CPoint const& inPos, VSTGUI::CMouseWheelAxis const& inAxis, float const& inDistance, VSTGUI::CButtonState const& inButtons)
{
	auto result = DGControl<T>::onWheel(inPos, inAxis, inDistance, inButtons);
	forEachChild([inPos, inAxis, inDistance, inButtons, &result](auto&& child)
	{
		result |= detail::onWheel(child, inPos, inAxis, inDistance, inButtons);
	});
	return result;
}

//-----------------------------------------------------------------------------
template <class T>
void DGMultiControl<T>::setDirty_all(bool inValue)
{
	DGControl<T>::setDirty(inValue);
	forEachChild([inValue](auto&& child){ child->asCControl()->setDirty(inValue); });
}

//-----------------------------------------------------------------------------
template <class T>
bool DGMultiControl<T>::isDirty() const
{
	return DGControl<T>::isDirty() || std::any_of(mChildren.cbegin(), mChildren.cend(), [](auto const& child){ return child->asCControl()->isDirty(); });
}

//-----------------------------------------------------------------------------
template <class T>
bool DGMultiControl<T>::checkDefaultValue_all(VSTGUI::CButtonState inButtons)
{
	bool any = DGControl<T>::checkDefaultValue(inButtons);
	for (IDGControl* child : mChildren)
	{
		// checkDefaultValue has desired side effects, so always execute it prior to testing the existing result, 
		// so that execution cannot be skipped during the logical OR operation
		any = child->asCControl()->checkDefaultValue(inButtons) || any;
	}
	return any;
}

//-----------------------------------------------------------------------------
template <class T>
void DGMultiControl<T>::beginEdit_all()
{
	DGControl<T>::beginEdit();
	forEachChild([](auto&& child){ child->asCControl()->beginEdit(); });
}

//-----------------------------------------------------------------------------
template <class T>
void DGMultiControl<T>::endEdit_all()
{
	DGControl<T>::endEdit();
	forEachChild([](auto&& child){ child->asCControl()->endEdit(); });
}

//-----------------------------------------------------------------------------
template <class T>
bool DGMultiControl<T>::isEditing_any() const
{
	return DGControl<T>::isEditing() || std::any_of(mChildren.cbegin(), mChildren.cend(), [](auto const& child){ return child->asCControl()->isEditing(); });
}

//-----------------------------------------------------------------------------
template <class T>
IDGControl* DGMultiControl<T>::getControlByParameterID(long inParameterID)
{
	if (DGControl<T>::getParameterID() == inParameterID)
	{
		return this;
	}
	auto const foundChild = std::find_if(mChildren.cbegin(), mChildren.cend(), 
										 [inParameterID](auto&& child){ return child->getParameterID() == inParameterID; });
	return (foundChild != mChildren.cend()) ? *foundChild : nullptr;
}

//-----------------------------------------------------------------------------
template <class T>
IDGControl* DGMultiControl<T>::addChild(long inParameterID)
{
	assert(inParameterID >= 0);
	assert(inParameterID != this->getParameterID());
	auto child = std::make_unique<DGMultiControlChild>(DGControl<T>::getOwnerEditor(), DGControl<T>::getViewSize(), inParameterID);
	[[maybe_unused]] auto const [element, inserted] = mChildren.insert(child.get());
	assert(inserted);

	child->asCControl()->setVisible(false);
	child->asCControl()->setMouseEnabled(false);
	child->asCControl()->setWantsFocus(false);

	return DGControl<T>::getOwnerEditor()->addControl(child.release());
}

//-----------------------------------------------------------------------------
template <class T>
void DGMultiControl<T>::addChildren(std::vector<long> const& inParameterIDs)
{
	assert(std::unordered_set<long>(inParameterIDs.cbegin(), inParameterIDs.cend()).size() == inParameterIDs.size());
	std::for_each(inParameterIDs.cbegin(), inParameterIDs.cend(), [this](auto parameterID){ addChild(parameterID); });
}

//-----------------------------------------------------------------------------
template <class T>
template <typename Proc>
void DGMultiControl<T>::forEachChild(Proc inProc)
{
	static_assert(std::is_invocable_v<Proc, IDGControl*>);
	for (auto&& child : getChildren())
	{
		inProc(child);
	}
}

//-----------------------------------------------------------------------------
template <class T>
void DGMultiControl<T>::notifyIfChanged_all()
{
	auto const baseChanged = DGControl<T>::notifyIfChanged();
	bool anyChildChanged = false;
	forEachChild([&anyChildChanged](auto&& child){ anyChildChanged |= child->notifyIfChanged(); });
	// due to being identified as invisible, the children's invalidation will be ignored
	if (!baseChanged && anyChildChanged)
	{
		DGControl<T>::redraw();
	}
}



#pragma mark -
#pragma mark DGMultiControlChild

//-----------------------------------------------------------------------------
template <class T>
DGMultiControl<T>::DGMultiControlChild::DGMultiControlChild(DfxGuiEditor* inOwnerEditor,
															DGRect const& inRegion,
															long inParameterID)
:	DGControl<VSTGUI::CControl>(inRegion, inOwnerEditor, inParameterID)
{
}
