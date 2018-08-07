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
	typedef unsigned int Buttons;
	static const Buttons kButtons_OK;
	static const Buttons kButtons_OKCancel;
	static const Buttons kButtons_OKCancelOther;

	enum Selection
	{
		kSelection_OK,
		kSelection_Cancel,
		kSelection_Other,
	};

	using DialogChoiceSelectedCallback = std::function<bool(DGDialog*, Selection)>;

	class Listener
	{
	public:
		virtual ~Listener() = default;
		virtual bool dialogChoiceSelected(DGDialog* inDialog, Selection inSelection) = 0;
	};

	DGDialog(DGRect const& inRegion, std::string const& inMessage, Buttons inButtons = kButtons_OK, 
			 char const* inOkButtonTitle = nullptr, char const* inCancelButtonTitle = nullptr, char const* inOtherButtonTitle = nullptr);

	// CView override
	int32_t onKeyDown(VstKeyCode& inKeyCode) override;
	int32_t onKeyUp(VstKeyCode& inKeyCode) override;

	// CViewContainer overrides
	void drawBackgroundRect(CDrawContext* inContext, CRect const& inUpdateRect) override;
	bool attached(CView* inParent) override;

	// IControlListener override
	void valueChanged(CControl* inControl) override;

	bool runModal(CFrame* inFrame, Listener* inListener);
	bool runModal(CFrame* inFrame, DialogChoiceSelectedCallback&& inCallback);
	void close();

	CTextButton* getButton(Selection inSelection) const;

	CLASS_METHODS(DGDialog, CViewContainer)

private:
	enum : Buttons
	{
		kButtons_OKBit = 1 << kSelection_OK,
		kButtons_CancelBit = 1 << kSelection_Cancel,
		kButtons_OtherBit = 1 << kSelection_Other,
	};

	bool runModal(CFrame* inFrame);
	bool handleKeyEvent(unsigned char inVirtualKey, bool inIsPressed);

	Listener* mListener = nullptr;
	DialogChoiceSelectedCallback mDialogChoiceSelectedCallback;
};


//-----------------------------------------------------------------------------
class DGTextEntryDialog : public DGDialog
{
public:
	DGTextEntryDialog(long inParamID, std::string const& inMessage, 
					  char const* inTextEntryLabel = nullptr, Buttons inButtons = kButtons_OKCancel, 
					  char const* inOkButtonTitle = nullptr, char const* inCancelButtonTitle = nullptr, char const* inOtherButtonTitle = nullptr);
	explicit DGTextEntryDialog(std::string const& inMessage, 
							   char const* inTextEntryLabel = nullptr, Buttons inButtons = kButtons_OKCancel, 
							   char const* inOkButtonTitle = nullptr, char const* inCancelButtonTitle = nullptr, char const* inOtherButtonTitle = nullptr);

	// CBaseObject override
	CMessageResult notify(CBaseObject* inSender, IdStringPtr inMessage) override;

	void setText(std::string const& inText);
	std::string getText() const;

	long getParameterID() const noexcept;

	CLASS_METHODS(DGTextEntryDialog, DGDialog)

private:
	long const mParameterID;
	CTextEdit* mTextEdit = nullptr;
};
