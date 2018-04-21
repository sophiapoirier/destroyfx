/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2015-2018  Sophia Poirier

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


#include <string>

#include "dfxdefines.h"
#include "dfxguimisc.h"


//-----------------------------------------------------------------------------
class DGDialog : public CViewContainer, public IControlListener
{
public:
	typedef enum
	{
		kSelection_ok,
		kSelection_cancel,
		kSelection_other,
	} Selection;

	typedef enum
	{
		kButtons_okBit = 1 << kSelection_ok,
		kButtons_cancelBit = 1 << kSelection_cancel,
		kButtons_otherBit = 1 << kSelection_other,

		kButtons_ok = kButtons_okBit,
		kButtons_okCancel = kButtons_okBit | kButtons_cancelBit,
		kButtons_okCancelOther = kButtons_okBit | kButtons_cancelBit | kButtons_otherBit,
	} Buttons;

	class Listener
	{
	public:
		virtual ~Listener() = default;
		virtual void dialogChoiceSelected(DGDialog* inDialog, Selection inSelection) = 0;
	};

	DGDialog(Listener* inListener, DGRect const& inRegion, std::string const& inMessage, Buttons inButtons = kButtons_ok, 
			 char const* inOkButtonTitle = nullptr, char const* inCancelButtonTitle = nullptr, char const* inOtherButtonTitle = nullptr);

	// CViewContainer overrides
	void drawBackgroundRect(CDrawContext* inContext, CRect const& inUpdateRect) override;
	bool attached(CView* inParent) override;

	// IControlListener override
	void valueChanged(CControl* inControl) override;

	void close();

	CLASS_METHODS(DGDialog, CViewContainer)

private:
	Listener* const mListener;
};


//-----------------------------------------------------------------------------
class DGTextEntryDialog : public DGDialog
{
public:
	DGTextEntryDialog(Listener* inListener, std::string const& inMessage, 
					  char const* inTextEntryLabel = nullptr, Buttons inButtons = kButtons_okCancel);

	void setText(std::string const& inText);
	std::string getText() const;

	void setParameterID(long inParameterID) noexcept;
	long getParameterID() const noexcept;

private:
	CTextEdit* mTextEdit = nullptr;
	long mParameterID = kDfxParameterID_Invalid;
};
