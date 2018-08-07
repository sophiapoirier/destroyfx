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

#include "dfxguieditor.h"
#include "dfxmisc.h"



//-----------------------------------------------------------------------------
template <class T>
template <typename... Args>
DGControl<T>::DGControl(Args&&... args)
:	T(std::forward<Args>(args)...),
	mOwnerEditor(dynamic_cast<DfxGuiEditor*>(T::getListener()))
{
	assert(asCControl()->getListener() ? (mOwnerEditor != nullptr) : true);
}

//-----------------------------------------------------------------------------
template <class T>
void DGControl<T>::setValue_gen(float inValue)
{
	asCControl()->setValue(inValue);
}

//-----------------------------------------------------------------------------
template <class T>
void DGControl<T>::setDefaultValue_gen(float inValue)
{
	asCControl()->setDefaultValue(inValue);
}

//-----------------------------------------------------------------------------
template <class T>
void DGControl<T>::redraw()
{
//	asCControl()->invalid(); // XXX CView::invalid calls setDirty(false), which can have undesired consequences for control value handling
	asCControl()->invalidRect(asCControl()->getViewSize());
}

//-----------------------------------------------------------------------------
template <class T>
long DGControl<T>::getParameterID() const
{
	return asCControl()->getTag();
}

//-----------------------------------------------------------------------------
template <class T>
void DGControl<T>::setParameterID(long inParameterID)
{
	asCControl()->setTag(inParameterID);
	if (mOwnerEditor)
	{
		asCControl()->setValue(mOwnerEditor->getparameter_gen(inParameterID));
	}
	asCControl()->setDirty();
}

//-----------------------------------------------------------------------------
template <class T>
bool DGControl<T>::isParameterAttached() const
{
	return (getParameterID() >= 0);
}

//-----------------------------------------------------------------------------
template <class T>
void DGControl<T>::setDrawAlpha(float inAlpha)
{
	asCControl()->setAlphaValue(inAlpha);
}

//-----------------------------------------------------------------------------
template <class T>
float DGControl<T>::getDrawAlpha() const
{
	return asCControl()->getAlphaValue();
}

//-----------------------------------------------------------------------------
template <class T>
bool DGControl<T>::setHelpText(char const* inText)
{
	assert(inText);
	return asCControl()->setAttribute(kCViewTooltipAttribute, strlen(inText) + 1, inText);
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
