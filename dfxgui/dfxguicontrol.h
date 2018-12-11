/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2003-2018  Sophia Poirier

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

#include "dfxguimisc.h"
#include "idfxguicontrol.h"



//-----------------------------------------------------------------------------
template <class T>
class DGControl : public IDGControl, public T
{
	static_assert(std::is_base_of<CControl, T>::value);

public:
	using Parent = T;

	template <typename... Args>
	DGControl(Args&&... args);

	VSTGUI::CControl* asCControl() final
	{
		return dynamic_cast<VSTGUI::CControl*>(this);
	}
	VSTGUI::CControl const* asCControl() const final
	{
		return dynamic_cast<VSTGUI::CControl const*>(this);
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

	long getNumStates() const noexcept
	{
		return mNumStates;
	}
	void setNumStates(long inNumStates);

	void setDrawAlpha(float inAlpha) final;
	float getDrawAlpha() const final;

	void setHelpText(char const* inText) final;
#if TARGET_OS_MAC
	void setHelpText(CFStringRef inText);
#endif

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
		explicit DiscreteValueConstrainer(DGControl<T>* inControl)
		:	mControl(inControl),
			mEntryValue(inControl ? inControl->getValue() : 0.0f)
		{
		}
		~DiscreteValueConstrainer()
		{
			if (mControl && (mControl->getNumStates() > 0) && (mControl->getValue() != mEntryValue))
			{
				mControl->setValue_i(mControl->getValue_i());
				if (mControl->isDirty())
				{
					mControl->valueChanged();
					mControl->invalid();
				}
			}
		}
	private:
		DGControl<T>* const mControl;
		float const mEntryValue;
	};

	bool getWraparoundValues() const noexcept
	{
		return mWraparoundValues;
	}
	void setWraparoundValues(bool inWraparoundPolicy) noexcept
	{
		mWraparoundValues = inWraparoundPolicy;
	}

private:
	void pullNumStatesFromParameter();

	DfxGuiEditor* const mOwnerEditor;

	long mNumStates = 0;
	bool mWraparoundValues = false;
};

#include "dfxguicontrol.hpp"



//-----------------------------------------------------------------------------
class DGNullControl : public DGControl<VSTGUI::CControl>
{
public:
	DGNullControl(DfxGuiEditor* inOwnerEditor, DGRect const& inRegion, DGImage* inBackgroundImage = nullptr);

	void draw(CDrawContext* inContext) override;

	CLASS_METHODS(DGNullControl, VSTGUI::CControl)
};



#if 0

//-----------------------------------------------------------------------------
class DGControl : public CControl
{
public:
	enum KeyModifiers : unsigned int
	{
		kKeyModifier_Accel = 1,  // command on Macs, control on PCs
		kKeyModifier_Alt = 1 << 1,  // option on Macs, alt on PCs
		kKeyModifier_Shift = 1 << 2,
		kKeyModifier_Extra = 1 << 3  // control on Macs
	};

	bool do_mouseWheel(long inDelta, dfx::Axis inAxis, dfx::KeyModifiers inKeyModifiers);
	virtual bool mouseWheel(long inDelta, dfx::Axis inAxis, dfx::KeyModifiers inKeyModifiers);

	void setMouseDragRange(float inMouseDragRange)
	{
		if (inMouseDragRange != 0.0f)  // to prevent division by zero
			mouseDragRange = inMouseDragRange;
	}
	float getMouseDragRange() const noexcept
	{
		return mouseDragRange;
	}

private:
	bool	isContinuous = false;
	float	mouseDragRange = kDefaultMouseDragRange;  // the range of pixels over which you can drag the mouse to adjust the control value
};

#endif
