/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2015-2023  Sophia Poirier

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


#include <optional>
#include <string>

#include "dfx-base.h"
#include "dfxguimisc.h"


//-----------------------------------------------------------------------------
namespace detail
{
class DGModalSession
{
public:
	DGModalSession(VSTGUI::CFrame* inFrame, VSTGUI::CView* inView);
	~DGModalSession();

	DGModalSession(DGModalSession const&) = delete;
	DGModalSession(DGModalSession&&) = delete;
	DGModalSession& operator=(DGModalSession const&) = delete;
	DGModalSession& operator=(DGModalSession&&) = delete;

	bool isSessionActive() const noexcept;

private:
	VSTGUI::CView* const mView;
	std::optional<VSTGUI::ModalViewSessionID> mModalViewSessionID;
};
}


//-----------------------------------------------------------------------------
class DGDialog : public VSTGUI::CViewContainer, public VSTGUI::IControlListener
{
public:
	using Buttons = unsigned int;
	static Buttons const kButtons_OK;
	static Buttons const kButtons_OKCancel;
	static Buttons const kButtons_OKCancelOther;

	enum Selection
	{
		kSelection_OK,
		kSelection_Cancel,
		kSelection_Other,
	};

	using DialogChoiceSelectedCallback = std::function<bool(Selection)>;

	DGDialog(DGRect const& inRegion, std::string const& inMessage, Buttons inButtons = kButtons_OK, 
			 char const* inOkButtonTitle = nullptr, char const* inCancelButtonTitle = nullptr, char const* inOtherButtonTitle = nullptr);

	// CView override
	void onKeyboardEvent(VSTGUI::KeyboardEvent& ioEvent) override;

	// CViewContainer overrides
	void drawBackgroundRect(VSTGUI::CDrawContext* inContext, VSTGUI::CRect const& inUpdateRect) override;
	bool attached(VSTGUI::CView* inParent) override;

	// IControlListener override
	void valueChanged(VSTGUI::CControl* inControl) override;

	bool runModal(VSTGUI::CFrame* inFrame, DialogChoiceSelectedCallback&& inCallback);
	bool runModal(VSTGUI::CFrame* inFrame);

	VSTGUI::CTextButton* getButton(Selection inSelection) const;

	CLASS_METHODS_NOCOPY(DGDialog, VSTGUI::CViewContainer)

private:
	enum : Buttons
	{
		kButtons_OKBit = 1 << kSelection_OK,
		kButtons_CancelBit = 1 << kSelection_Cancel,
		kButtons_OtherBit = 1 << kSelection_Other,
	};

	void close();

	DialogChoiceSelectedCallback mDialogChoiceSelectedCallback;
	std::optional<detail::DGModalSession> mModalSession;
};


//-----------------------------------------------------------------------------
class DGTextEntryDialog : public DGDialog
{
public:
	DGTextEntryDialog(dfx::ParameterID inParameterID, std::string const& inMessage, 
					  char const* inTextEntryLabel = nullptr, Buttons inButtons = kButtons_OKCancel, 
					  char const* inOkButtonTitle = nullptr, char const* inCancelButtonTitle = nullptr, char const* inOtherButtonTitle = nullptr);
	explicit DGTextEntryDialog(std::string const& inMessage, 
							   char const* inTextEntryLabel = nullptr, Buttons inButtons = kButtons_OKCancel, 
							   char const* inOkButtonTitle = nullptr, char const* inCancelButtonTitle = nullptr, char const* inOtherButtonTitle = nullptr);

	// CBaseObject override
	VSTGUI::CMessageResult notify(VSTGUI::CBaseObject* inSender, VSTGUI::IdStringPtr inMessage) override;

	void setText(std::string const& inText);
	std::string getText() const;

	dfx::ParameterID getParameterID() const noexcept;

	bool runModal(VSTGUI::CFrame* inFrame, std::function<bool(Selection, std::string const&, dfx::ParameterID)>&& inCallback);
	// callback only invoked when text entry submitted with OK button
	bool runModal(VSTGUI::CFrame* inFrame, std::function<bool(std::string const&, dfx::ParameterID)>&& inCallback);

	CLASS_METHODS_NOCOPY(DGTextEntryDialog, DGDialog)

private:
	dfx::ParameterID const mParameterID;
	VSTGUI::CTextEdit* mTextEdit = nullptr;
};


//-----------------------------------------------------------------------------
class DGTextScrollDialog : public VSTGUI::CScrollView
{
public:
	DGTextScrollDialog(DGRect const& inRegion, std::string const& inMessage);

	// CView overrides
	void onMouseDownEvent(VSTGUI::MouseDownEvent& ioEvent) override;
	void onKeyboardEvent(VSTGUI::KeyboardEvent& ioEvent) override;

	// CViewContainer overrides
	bool attached(VSTGUI::CView* inParent) override;

	bool runModal(VSTGUI::CFrame* inFrame);

	CLASS_METHODS_NOCOPY(DGTextScrollDialog, VSTGUI::CScrollView)

protected:
	// CScrollView override
	void recalculateSubViews() override;

private:
	std::optional<detail::DGModalSession> mModalSession;
};
