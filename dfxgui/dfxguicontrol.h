/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2003-2020  Sophia Poirier

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

#pragma once


#include <type_traits>
#include <unordered_set>
#include <vector>

#include "dfxguimisc.h"
#include "idfxguicontrol.h"

#if TARGET_OS_MAC
	#include <CoreFoundation/CoreFoundation.h>
#endif



//-----------------------------------------------------------------------------
template <class T>
class DGControl : public IDGControl, public T
{
	static_assert(std::is_base_of_v<VSTGUI::CControl, T>);

public:
	template <typename... Args>
	explicit DGControl(Args&&... args);

	VSTGUI::CControl* asCControl() final
	{
		return this;
	}
	VSTGUI::CControl const* asCControl() const final
	{
		return this;
	}

	DfxGuiEditor* getOwnerEditor() const noexcept final
	{
		return mOwnerEditor;
	}

	void setValue_gen(float inValue) final;
	void setDefaultValue_gen(float inValue) final;
	long getValue_i() final;
	void setValue_i(long inValue) final;

	void redraw() final;

	long getParameterID() const final;
	void setParameterID(long inParameterID) final;
	bool isParameterAttached() const final;

	long getNumStates() const final
	{
		return mNumStates;
	}
	void setNumStates(long inNumStates);

	float getFineTuneFactor() const override
	{
		return kDefaultFineTuneFactor;
	}

	bool notifyIfChanged() final;

	bool onWheel(VSTGUI::CPoint const& inPos, VSTGUI::CMouseWheelAxis const& inAxis, float const& inDistance, VSTGUI::CButtonState const& inButtons) override;
	void onMouseWheelEditing() final
	{
		mMouseWheelEditingSupport.onMouseWheelEditing();
	}
	void invalidateMouseWheelEditingTimer() final
	{
		mMouseWheelEditingSupport.invalidateMouseWheelEditingTimer();
	}

	void setDrawAlpha(float inAlpha) final;
	float getDrawAlpha() const final;

	void setHelpText(char const* inText) final;
#if TARGET_OS_MAC
	void setHelpText(CFStringRef inText);
#endif

	std::vector<IDGControl*> getChildren() override { return {}; }
	std::vector<IDGControl const*> getChildren() const override { return {}; }

#if TARGET_PLUGIN_USES_MIDI
	void setMidiLearner(bool inEnable) override {}
#endif

protected:
	static constexpr float kDefaultFineTuneFactor = 10.0f;
	//static constexpr float kDefaultMouseDragRange = 200.0f;  // pixels
	//static constexpr float kDefaultMouseDragValueIncrement = 0.1f;
	//static constexpr float kDefaultMouseDragValueIncrement_descrete = 15.0f;

	class DiscreteValueConstrainer
	{
	public:
		explicit DiscreteValueConstrainer(IDGControl* inControl)
		:	mControl(inControl),
			mEntryValue(inControl ? inControl->asCControl()->getValue() : 0.0f),
			mEntryEditing(inControl && inControl->asCControl()->isEditing())
		{
			// the editing state is a counter, so this is essentially us bumping the editing reference count
			if (mEntryEditing)
			{
				mControl->asCControl()->beginEdit();
			}
		}
		~DiscreteValueConstrainer()
		{
			if (mControl && (mControl->getNumStates() > 0) && (mControl->asCControl()->getValue() != mEntryValue))
			{
				mControl->setValue_i(mControl->getValue_i());
				if (mControl->asCControl()->isDirty())
				{
					mControl->asCControl()->valueChanged();
					mControl->asCControl()->invalid();
				}
			}
			// and by bumping that reference count, here we extend the edit cycle lifespan through the preceding value change
			if (mEntryEditing)
			{
				mControl->asCControl()->endEdit();
			}
		}
	private:
		IDGControl* const mControl;
		float const mEntryValue;
		bool const mEntryEditing;
	};

	DGRect rectLocalToFrame(DGRect rect) const
	{
		return rect.offset(T::getViewSize().getTopLeft());
	}

	// Apple on Mac, Control on others.
	static bool isPlatformMetaSet(const VSTGUI::CButtonState &button)
	{
		#if TARGET_OS_MAC
		return button.isAppleSet();
		#else
		return button.isControlSet();
		#endif
	}

private:
	void initValues();
	void pullNumStatesFromParameter();

	DfxGuiEditor* const mOwnerEditor;

	long mNumStates = 0;

	class MouseWheelEditingSupport : protected VSTGUI::CMouseWheelEditingSupport
	{
	public:
		explicit MouseWheelEditingSupport(IDGControl* inControl)
		:	mControl(inControl)
		{
		}
		void onMouseWheelEditing()
		{
			VSTGUI::CMouseWheelEditingSupport::onMouseWheelEditing(mControl->asCControl());
		}
		void invalidateMouseWheelEditingTimer()
		{
			VSTGUI::CMouseWheelEditingSupport::invalidMouseWheelEditTimer(mControl->asCControl());
		}
	private:
		IDGControl* const mControl;
	} mMouseWheelEditingSupport;
};



//-----------------------------------------------------------------------------
template <class T>
class DGMultiControl : /*public IDGMultiControl,*/ public DGControl<T>
{
public:
	using DGControl<T>::DGControl;

	std::vector<IDGControl*> getChildren() override
	{
		return {mChildren.cbegin(), mChildren.cend()};
	}
	std::vector<IDGControl const*> getChildren() const override
	{
		return {mChildren.cbegin(), mChildren.cend()};
	}

	void setViewSize(VSTGUI::CRect const& inPos, bool inInvalidate = true) override;
	bool onWheel(VSTGUI::CPoint const& inPos, VSTGUI::CMouseWheelAxis const& inAxis, float const& inDistance, VSTGUI::CButtonState const& inButtons) override;

	void setDirty_all(bool inValue);
	bool isDirty() const override;

	bool checkDefaultValue_all(VSTGUI::CButtonState inButtons);
	void beginEdit_all();
	void endEdit_all();
	bool isEditing_any() const;

	IDGControl* getControlByParameterID(long inParameterID);

protected:
	IDGControl* addChild(long inParameterID);
	// TODO: C++20 use std::span
	void addChildren(std::vector<long> const& inParameterID);

	template <typename Proc>
	void forEachChild(Proc inProc);

	void notifyIfChanged_all();

private:
	class DGMultiControlChild final : public DGControl<VSTGUI::CControl>
	{
	public:
		DGMultiControlChild(DfxGuiEditor* inOwnerEditor, DGRect const& inRegion, long inParameterID);
		void draw(VSTGUI::CDrawContext*) override {}
		CLASS_METHODS(DGMultiControlChild, VSTGUI::CControl)
	};

	std::unordered_set<IDGControl*> mChildren;
};



#include "dfxguicontrol.hpp"



//-----------------------------------------------------------------------------
class DGNullControl final : public DGControl<VSTGUI::CControl>
{
public:
	DGNullControl(DfxGuiEditor* inOwnerEditor, DGRect const& inRegion, DGImage* inBackgroundImage = nullptr);

	void draw(VSTGUI::CDrawContext* inContext) override;

	CLASS_METHODS(DGNullControl, VSTGUI::CControl)
};
