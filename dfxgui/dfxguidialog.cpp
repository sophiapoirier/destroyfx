/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2015-2024  Sophia Poirier

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

#include "dfxguidialog.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <optional>
#include <utility>
#include <vector>

#include "dfxplugin-base.h"


//-----------------------------------------------------------------------------
constexpr VSTGUI::CCoord kContentMargin = 20.0;
constexpr VSTGUI::CCoord kButtonSpacing = 12.0;
constexpr VSTGUI::CCoord kButtonHeight = 20.0;
constexpr VSTGUI::CCoord kTextLabelHeight = 14.0;
constexpr VSTGUI::CCoord kTextEditHeight = 20.0;
constexpr VSTGUI::CCoord kFocusIndicatorThickness = 2.4;


//-----------------------------------------------------------------------------
// a function rather than a global constant to avoid static initializer order dependency
static auto DFXGUI_GetDialogFont() noexcept
{
	return VSTGUI::kSystemFont;
}

//-----------------------------------------------------------------------------
static bool DFXGUI_PressButton(VSTGUI::CTextButton* inButton, bool inState)
{
	if (inButton)
	{
		auto const mousePos = inButton->getMouseableArea().getCenter();
		if (inState)
		{
			VSTGUI::MouseDownEvent event(mousePos, VSTGUI::MouseButton::Left);
			event.clickCount = 1;
			inButton->onMouseDownEvent(event);
			inButton->valueChanged();  // required to trigger the change handler upon mouse-down rather than mouse-up
		}
		else
		{
			VSTGUI::MouseUpEvent event(mousePos, {});
			inButton->onMouseUpEvent(event);
		}
		return true;
	}
	return false;
}


//-----------------------------------------------------------------------------
// Dialog Button
//-----------------------------------------------------------------------------
class DGDialogButton final : public VSTGUI::CTextButton
{
public:
	DGDialogButton(VSTGUI::IControlListener* inListener, DGRect const& inRegion, DGDialog::Selection inSelection, VSTGUI::UTF8StringPtr inTitle)
	:	VSTGUI::CTextButton(inRegion, inListener, dfx::ParameterID_ToVST(dfx::kParameterID_Invalid), inTitle, VSTGUI::CTextButton::kKickStyle),
		mSelection(inSelection),
		mIsDefaultButton(inSelection == DGDialog::kSelection_OK)
	{
		sizeToFit();
		auto fitSize = getViewSize();
		fitSize.right = std::round(fitSize.right + 0.5);  // round up
		setViewSize(fitSize);
		setMouseableArea(fitSize);

		auto const textColor = DGColor::getSystem(mIsDefaultButton ? DGColor::System::AccentControlText : DGColor::System::ControlText);
		setTextColor(textColor);
		setTextColorHighlighted(DGColor::getSystem(DGColor::System::AccentControlText));
		setFrameColorHighlighted(DGColor::getSystem(DGColor::System::AccentPressed));
		setFont(DFXGUI_GetDialogFont());

		constexpr float gradientDarkAmount = 0.6f;
		constexpr float accentFillAlpha = 0.72f;
		constexpr double gradientStart = 0.3, gradientEnd = 1.0;
		auto const fillColor = mIsDefaultButton ? DGColor::getSystem(DGColor::System::Accent).withAlpha(accentFillAlpha) : DGColor::getSystem(DGColor::System::Control);
		auto const fillColorTail = mIsDefaultButton ? fillColor.darker(gradientDarkAmount) : fillColor;
		setGradient(VSTGUI::owned(VSTGUI::CGradient::create(gradientStart, gradientEnd, fillColor, fillColorTail)));
		auto const fillColorHighlighted = DGColor::getSystem(DGColor::System::AccentPressed).withAlpha(accentFillAlpha);
		setGradientHighlighted(VSTGUI::owned(VSTGUI::CGradient::create(gradientStart, gradientEnd, fillColorHighlighted, fillColorHighlighted.darker(gradientDarkAmount))));

		looseFocus();  // HACK: trigger remaining frame styling
	}

	void takeFocus() override
	{
		setFrameColor(DGColor::getSystem(DGColor::System::FocusIndicator));
		setFrameWidth(kFocusIndicatorThickness);
	}

	void looseFocus() override
	{
		setFrameColor(DGColor::getSystem(mIsDefaultButton ? DGColor::System::Accent : DGColor::System::WindowFrame));
		setFrameWidth(1.0);
	}

	void onKeyboardEvent(VSTGUI::KeyboardEvent& ioEvent) override
	{
		switch (ioEvent.virt)
		{
			case VSTGUI::VirtualKey::Return:
				// let the parent dialog handle this key
				break;
			case VSTGUI::VirtualKey::Space:
				DFXGUI_PressButton(this, ioEvent.type == VSTGUI::EventType::KeyDown);
				ioEvent.consumed = true;
				break;
			default:
				VSTGUI::CTextButton::onKeyboardEvent(ioEvent);
				break;
		}
	}

	DGDialog::Selection getSelection() const noexcept
	{
		return mSelection;
	}

	void setX(VSTGUI::CCoord inXpos)
	{
		VSTGUI::CRect newSize(getViewSize());
		newSize.moveTo(inXpos, newSize.top);
		setViewSize(newSize);
		setMouseableArea(newSize);
	}

	void setWidth(VSTGUI::CCoord inWidth)
	{
		VSTGUI::CRect newSize(getViewSize());
		newSize.setWidth(inWidth);
		setViewSize(newSize);
		setMouseableArea(newSize);
	}

	CLASS_METHODS_NOCOPY(DGDialogButton, VSTGUI::CTextButton)

private:
	DGDialog::Selection const mSelection;
	bool const mIsDefaultButton;
};






#pragma mark -
#pragma mark DGDialogTextEdit

//-----------------------------------------------------------------------------
// Dialog Text Edit
//-----------------------------------------------------------------------------
class DGDialogTextEdit final : public VSTGUI::CTextEdit
{
public:
	DGDialogTextEdit(VSTGUI::CRect const& inRegion, VSTGUI::IControlListener* inListener)
	:	VSTGUI::CTextEdit(inRegion, inListener, dfx::ParameterID_ToVST(dfx::kParameterID_Invalid))
	{
		setFontColor(DGColor::getSystem(DGColor::System::Text));
		setBackColor(DGColor::getSystem(DGColor::System::TextBackground));
		setFrameColor(DGColor::getSystem(DGColor::System::WindowFrame));
		setHoriAlign(VSTGUI::kCenterText);
		setStyle(kRoundRectStyle);
		setRoundRectRadius(3.0);
		setFont(DFXGUI_GetDialogFont());
	}

	void draw(VSTGUI::CDrawContext* inContext) override
	{
		VSTGUI::CTextEdit::draw(inContext);

		// add a thicker control-focus highlight (mimic macOS native text input field)
		if (getPlatformTextEdit())
		{
			auto highlightRect = getViewSize();
			highlightRect.inset(kFocusIndicatorThickness / 2.0, kFocusIndicatorThickness / 2.0);
			if (auto const path = VSTGUI::owned(inContext->createRoundRectGraphicsPath(highlightRect, getRoundRectRadius())))
			{
				inContext->setLineStyle(VSTGUI::kLineSolid);
				inContext->setLineWidth(kFocusIndicatorThickness);
				inContext->setFrameColor(DGColor::getSystem(DGColor::System::FocusIndicator));
				inContext->drawGraphicsPath(path, VSTGUI::CDrawContext::kPathStroked);
			}
		}
	}

	void onKeyboardEvent(VSTGUI::KeyboardEvent& ioEvent) override
	{
		if (ioEvent.virt == VSTGUI::VirtualKey::Escape)
		{
			// HACK: Workaround for bug where pressing the dialog's OK button in Logic produces
			// a platform text edit cancel event. This event is mapped to an ESC key event,
			// so by consuming those, we avert the text-discarding effect of the cancellation.
			// This is definitely a hack, but I don't think there is a good reason for this
			// text edit object itself to be cancellable (we handle that at the dialog level).
			if (ioEvent.type == VSTGUI::EventType::KeyDown)
			{
				ioEvent.consumed = true;
			}
			// HACK: unfortunately the Logic bug workaround results in failing to act on 
			// actual ESC key presses in the text edit field, but a real key press event 
			// will also be followed by a key-up event, and so we can work around the 
			// workaround by taking the pair of actions upon ESC key-up events
			else
			{
				auto const entryConsumed = ioEvent.consumed;
				ioEvent.type = VSTGUI::EventType::KeyDown;
				VSTGUI::CTextEdit::onKeyboardEvent(ioEvent);
				bool const downConsumed = std::exchange(ioEvent.consumed, entryConsumed);
				ioEvent.type = VSTGUI::EventType::KeyUp;
				VSTGUI::CTextEdit::onKeyboardEvent(ioEvent);
				ioEvent.consumed = ioEvent.consumed || downConsumed;
			}
		}
		else
		{
			VSTGUI::CTextEdit::onKeyboardEvent(ioEvent);
		}
	}

	CLASS_METHODS(DGDialogTextEdit, VSTGUI::CTextEdit)
};






#pragma mark -
#pragma mark DGDialog

//-----------------------------------------------------------------------------
// Dialog base class
//-----------------------------------------------------------------------------
DGDialog::Buttons const DGDialog::kButtons_OK = kButtons_OKBit;
DGDialog::Buttons const DGDialog::kButtons_OKCancel = kButtons_OKBit | kButtons_CancelBit;
DGDialog::Buttons const DGDialog::kButtons_OKCancelOther = kButtons_OKBit | kButtons_CancelBit | kButtons_OtherBit;

//-----------------------------------------------------------------------------
DGDialog::DGDialog(DGRect const& inRegion, 
				   std::string const& inMessage, 
				   Buttons inButtons, 
				   char const* inOkButtonTitle, 
				   char const* inCancelButtonTitle, 
				   char const* inOtherButtonTitle)
:	VSTGUI::CViewContainer(inRegion)
{
	if (!inMessage.empty())
	{
		// TODO: split text into multiple lines when it is too long to fit on a single line
		DGRect const pos(kContentMargin, kContentMargin, getWidth() - (kContentMargin * 2.0), kTextLabelHeight);
		auto const label = new VSTGUI::CMultiLineTextLabel(pos);
		label->setText(VSTGUI::UTF8String(inMessage));
		label->setFontColor(DGColor::getSystem(DGColor::System::WindowTitle));
		label->setBackColor(VSTGUI::kTransparentCColor);
		label->setFrameColor(VSTGUI::kTransparentCColor);
		label->setHoriAlign(VSTGUI::kLeftText);
		label->setLineLayout(VSTGUI::CMultiLineTextLabel::LineLayout::wrap);
//		label->setAutoHeight(true);
		auto const font = VSTGUI::makeOwned<VSTGUI::CFontDesc>(*DFXGUI_GetDialogFont());
		font->setStyle(font->getStyle() | VSTGUI::kBoldFace);
		label->setFont(font);
		addView(label);
	}

	if (!inOkButtonTitle)
	{
		inOkButtonTitle = "OK";
	}
	else
	{
		assert(inButtons & kButtons_OKBit);
	}
	if (!inCancelButtonTitle)
	{
		inCancelButtonTitle = "Cancel";
	}
	else
	{
		assert(inButtons & kButtons_CancelBit);
	}
	if (!inOtherButtonTitle)
	{
		inOtherButtonTitle = "something else";
	}
	else
	{
		assert(inButtons & kButtons_OtherBit);
	}

	DGDialogButton* okButton = nullptr;
	DGDialogButton* otherButton = nullptr;
	std::vector<DGDialogButton*> buttons;
	DGRect const buttonPos(0.0, inRegion.bottom - kButtonHeight - kContentMargin, kButtonHeight, kButtonHeight);
	if (inButtons & kButtons_OKBit)
	{
		okButton = new DGDialogButton(this, buttonPos, kSelection_OK, inOkButtonTitle);
		addView(okButton, getView(0));
		buttons.push_back(okButton);
	}
	if (inButtons & kButtons_CancelBit)
	{
		auto const cancelButton = new DGDialogButton(this, buttonPos, kSelection_Cancel, inCancelButtonTitle);
		addView(cancelButton, getView(0));
		buttons.push_back(cancelButton);
		if (okButton && (okButton->getWidth() < cancelButton->getWidth()))
		{
			okButton->setWidth(cancelButton->getWidth());  // visual balance
		}
	}
	if (inButtons & kButtons_OtherBit)
	{
		otherButton = new DGDialogButton(this, buttonPos, kSelection_Other, inOtherButtonTitle);
		addView(otherButton, getView(0));
		buttons.push_back(otherButton);
	}
	assert(!buttons.empty());

	auto leftmostButtonX = kContentMargin;
	// arrange the buttons from right to left
	for (size_t i = 0; i < buttons.size(); i++)
	{
		auto const button = buttons[i];
		assert(button);
		DGDialogButton* const prevButton = (i == 0) ? nullptr : buttons[i - 1];
		VSTGUI::CCoord rightPos {};
		if (prevButton)
		{
			auto const spacer = (button == otherButton) ? std::max(kContentMargin, kButtonSpacing) : kButtonSpacing;
			rightPos = prevButton->getViewSize().left - spacer;
		}
		else
		{
			rightPos = inRegion.right - kContentMargin;
		}
		button->setX(std::round(rightPos - button->getWidth()));
		button->setAutosizeFlags(VSTGUI::kAutosizeRight);
		leftmostButtonX = button->getViewSize().left;
	}
	if (leftmostButtonX < kContentMargin)
	{
		auto const extraWidth = std::round(kContentMargin - leftmostButtonX);
		auto dialogSize = getViewSize();
		dialogSize.setWidth(dialogSize.getWidth() + extraWidth);
		setViewSize(dialogSize);
		setMouseableArea(dialogSize);
	}
}

//-----------------------------------------------------------------------------
void DGDialog::onKeyboardEvent(VSTGUI::KeyboardEvent& ioEvent)
{
	auto const handled = [this, &ioEvent]
	{
		bool const isPressed = (ioEvent.type == VSTGUI::EventType::KeyDown);
		if (ioEvent.virt == VSTGUI::VirtualKey::Return)
		{
			return DFXGUI_PressButton(getButton(kSelection_OK), isPressed);
		}
		if (ioEvent.virt == VSTGUI::VirtualKey::Escape)
		{
			return DFXGUI_PressButton(getButton(kSelection_Cancel), isPressed);
		}
		return false;
	}();
	if (handled)
	{
		ioEvent.consumed = true;
	}
	else
	{
		VSTGUI::CViewContainer::onKeyboardEvent(ioEvent);
	}
}

//-----------------------------------------------------------------------------
void DGDialog::drawBackgroundRect(VSTGUI::CDrawContext* inContext, VSTGUI::CRect const& /*inUpdateRect*/)
{
	inContext->setFillColor(DGColor::getSystem(DGColor::System::WindowBackground));
	inContext->setFrameColor(DGColor::getSystem(DGColor::System::WindowFrame));
	inContext->setDrawMode(VSTGUI::kAliasing);
	inContext->setLineWidth(1.0);
	inContext->setLineStyle(VSTGUI::kLineSolid);

	// not sure why we should ignore supplied region argument, however this is what the base class does
	VSTGUI::CRect drawRect(getViewSize());
	drawRect.moveTo(0.0, 0.0);
	inContext->drawRect(drawRect, VSTGUI::kDrawFilledAndStroked);
}

//-----------------------------------------------------------------------------
bool DGDialog::attached(VSTGUI::CView* inParent)
{
	auto const result = VSTGUI::CViewContainer::attached(inParent);

	if (result && inParent && inParent->getFrame())
	{
		auto viewSize = getViewSize();
		auto const& parentSize = inParent->getFrame()->getViewSize();
		viewSize.centerInside(parentSize).makeIntegral();

		// constrain to fit in the frame, if necessary
		if (viewSize.getWidth() > parentSize.getWidth())
		{
			viewSize.left = parentSize.left;
			viewSize.right = parentSize.right;
		}
		if (viewSize.getHeight() > parentSize.getHeight())
		{
			viewSize.top = parentSize.top;
			viewSize.bottom = parentSize.bottom;
		}

		constexpr bool shouldInvalidate = true;
		setViewSize(viewSize, shouldInvalidate);
		setMouseableArea(viewSize);

		// enabling auto-height annoyingly only works after the view is attached
		std::vector<VSTGUI::CMultiLineTextLabel*> multiLineLabels;
		getChildViewsOfType<VSTGUI::CMultiLineTextLabel>(multiLineLabels);
		std::ranges::for_each(multiLineLabels, [](auto label){ label->setAutoHeight(true); });
	}

	return result;
}

//-----------------------------------------------------------------------------
void DGDialog::valueChanged(VSTGUI::CControl* inControl)
{
	auto const button = dynamic_cast<DGDialogButton*>(inControl);
	if (button && (button->getValue() > button->getMin()))
	{
		bool completed = true;  // default to yes in case there is no handler, we are not trapped in this dialog
		if (mDialogChoiceSelectedCallback)
		{
			completed = mDialogChoiceSelectedCallback(button->getSelection());
		}
		if (completed)
		{
			close();
		}
	}
}

//-----------------------------------------------------------------------------
bool DGDialog::runModal(VSTGUI::CFrame* inFrame, DialogChoiceSelectedCallback&& inCallback)
{
	assert(inCallback);
	mDialogChoiceSelectedCallback = std::move(inCallback);
	return runModal(inFrame);
}

//-----------------------------------------------------------------------------
bool DGDialog::runModal(VSTGUI::CFrame* inFrame)
{
	return mModalSession.emplace(inFrame, this).isSessionActive();
}

//-----------------------------------------------------------------------------
void DGDialog::close()
{
	mDialogChoiceSelectedCallback = nullptr;

	mModalSession.reset();
}

//-----------------------------------------------------------------------------
VSTGUI::CTextButton* DGDialog::getButton(Selection inSelection) const
{
	for (auto const& child : getChildren())
	{
		if (auto const button = dynamic_cast<DGDialogButton*>(child.get()))
		{
			if (button->getSelection() == inSelection)
			{
				return button;
			}
		}
	}
	return nullptr;
}






#pragma mark -
#pragma mark DGTextEntryDialog

//-----------------------------------------------------------------------------
// Text-Entry Dialog
//-----------------------------------------------------------------------------
DGTextEntryDialog::DGTextEntryDialog(dfx::ParameterID inParameterID, std::string const& inMessage, 
									 char const* inTextEntryLabel, Buttons inButtons, 
									 char const* inOkButtonTitle, char const* inCancelButtonTitle, char const* inOtherButtonTitle)
:	DGDialog(DGRect(0.0, 0.0, 247.0, 134.0), inMessage, inButtons, inOkButtonTitle, inCancelButtonTitle, inOtherButtonTitle), 
	mParameterID(inParameterID)
{
	constexpr VSTGUI::CCoord labelEditHeightOffset = (kTextEditHeight - kTextLabelHeight) / 2.0;

	DGRect pos(getViewSize());
	pos.moveTo(0.0, 0.0);
	pos.inset(kContentMargin, kContentMargin);
	pos.setHeight(kTextLabelHeight);
	pos.setY(getHeight() - pos.getHeight() - labelEditHeightOffset - kButtonHeight - (kContentMargin * 2.0));
	if (VSTGUI::CTextLabel* const label = inTextEntryLabel ? new VSTGUI::CTextLabel(pos, inTextEntryLabel) : nullptr)
	{
		label->setFontColor(DGColor::getSystem(DGColor::System::Label));
		label->setBackColor(VSTGUI::kTransparentCColor);
		label->setFrameColor(VSTGUI::kTransparentCColor);
		label->setHoriAlign(VSTGUI::kLeftText);
		label->setFont(DFXGUI_GetDialogFont());
		label->sizeToFit();
		addView(label);

		pos.left = std::round(label->getViewSize().right + kButtonSpacing);
		pos.right = getWidth() - kContentMargin;
	}

	pos.setHeight(kTextEditHeight);
	pos.offset(0.0, -labelEditHeightOffset);
	mTextEdit = new DGDialogTextEdit(pos, this);
	addView(mTextEdit, getView(0));
}

//-----------------------------------------------------------------------------
DGTextEntryDialog::DGTextEntryDialog(std::string const& inMessage, 
									 char const* inTextEntryLabel, Buttons inButtons, 
									 char const* inOkButtonTitle, char const* inCancelButtonTitle, char const* inOtherButtonTitle)
:	DGTextEntryDialog(dfx::kParameterID_Invalid, inMessage, inTextEntryLabel, inButtons, 
					  inOkButtonTitle, inCancelButtonTitle, inOtherButtonTitle)
{
}

//-----------------------------------------------------------------------------
VSTGUI::CMessageResult DGTextEntryDialog::notify(VSTGUI::CBaseObject* inSender, VSTGUI::IdStringPtr inMessage)
{
	// allow return key in the text edit to additionally trigger the default button
	if (auto const textEdit = dynamic_cast<VSTGUI::CTextEdit*>(inSender))
	{
		if ((std::strcmp(inMessage, VSTGUI::kMsgLooseFocus) == 0) && textEdit->bWasReturnPressed)
		{
			if (DFXGUI_PressButton(getButton(kSelection_OK), true))
			{
				return VSTGUI::kMessageNotified;
			}
		}
	}
	return DGDialog::notify(inSender, inMessage);
}

//-----------------------------------------------------------------------------
void DGTextEntryDialog::setText(std::string const& inText)
{
	if (mTextEdit)
	{
		mTextEdit->setText(VSTGUI::UTF8String(inText));
	}
}

//-----------------------------------------------------------------------------
std::string DGTextEntryDialog::getText() const
{
	std::string result;
	if (mTextEdit)
	{
		result = mTextEdit->getText();
	}
	return result;
}

//-----------------------------------------------------------------------------
dfx::ParameterID DGTextEntryDialog::getParameterID() const noexcept
{
	return mParameterID;
}

//-----------------------------------------------------------------------------
bool DGTextEntryDialog::runModal(VSTGUI::CFrame* inFrame,
								 std::function<bool(Selection, std::string const&, dfx::ParameterID)>&& inCallback)
{
	assert(inCallback);
	return DGDialog::runModal(inFrame, [this, callback = std::move(inCallback)](DGDialog::Selection inSelection)
	{
		return callback(inSelection, getText(), getParameterID());
	});
}

//-----------------------------------------------------------------------------
bool DGTextEntryDialog::runModal(VSTGUI::CFrame* inFrame,
								 std::function<bool(std::string const&, dfx::ParameterID)>&& inCallback)
{
	assert(inCallback);
	return DGDialog::runModal(inFrame, [this, callback = std::move(inCallback)](DGDialog::Selection inSelection)
	{
		if (inSelection == DGDialog::kSelection_OK)
		{
			return callback(getText(), getParameterID());
		}
		return true;
	});
}






#pragma mark -
#pragma mark DGTextScrollDialog

//-----------------------------------------------------------------------------
// Text Scroll Dialog
//-----------------------------------------------------------------------------
DGTextScrollDialog::DGTextScrollDialog(DGRect const& inRegion, std::string const& inMessage)
:	VSTGUI::CScrollView(inRegion, inRegion, 
						VSTGUI::CScrollView::kVerticalScrollbar | VSTGUI::CScrollView::kAutoHideScrollbars | VSTGUI::CScrollView::kOverlayScrollbars, 
						DFXGUI_GetDialogFont()->getSize())  // seems reasonable to size the scrollbar relative to the font
{
	assert(!inMessage.empty());

	setBackgroundColor(DGColor::getSystem(DGColor::System::TextBackground));

	DGRect const pos(kContentMargin, 0., getWidth() - (kContentMargin * 2.), getHeight());
	auto const label = new VSTGUI::CMultiLineTextLabel(pos);
	label->setText(VSTGUI::UTF8String(inMessage));
	label->setFontColor(DGColor::getSystem(DGColor::System::Text));
	label->setBackColor(VSTGUI::kTransparentCColor);
	label->setFrameColor(VSTGUI::kTransparentCColor);
	label->setHoriAlign(VSTGUI::kLeftText);
	label->setLineLayout(VSTGUI::CMultiLineTextLabel::LineLayout::wrap);
//	label->setAutoHeight(true);
	label->setFont(DFXGUI_GetDialogFont());
	addView(label);
}

//-----------------------------------------------------------------------------
void DGTextScrollDialog::onMouseDownEvent(VSTGUI::MouseDownEvent& ioEvent)
{
	mModalSession.reset();
	ioEvent.consumed = true;
	ioEvent.ignoreFollowUpMoveAndUpEvents(true);
}

//-----------------------------------------------------------------------------
void DGTextScrollDialog::onKeyboardEvent(VSTGUI::KeyboardEvent& ioEvent)
{
	if (ioEvent.type != VSTGUI::EventType::KeyDown)
	{
		return VSTGUI::CScrollView::onKeyboardEvent(ioEvent);
	}
	switch (ioEvent.virt)
	{
		case VSTGUI::VirtualKey::Return:
		case VSTGUI::VirtualKey::Escape:
			mModalSession.reset();
			ioEvent.consumed = true;
			break;
		default:
			VSTGUI::CScrollView::onKeyboardEvent(ioEvent);
			break;
	}
}

//-----------------------------------------------------------------------------
bool DGTextScrollDialog::attached(VSTGUI::CView* inParent)
{
	auto const result = VSTGUI::CViewContainer::attached(inParent);

	if (result && inParent && inParent->getFrame())
	{
		// enabling auto-height annoyingly only works after the view is attached
		std::vector<VSTGUI::CMultiLineTextLabel*> multiLineLabels;
		getChildViewsOfType<VSTGUI::CMultiLineTextLabel>(multiLineLabels, true);
		std::ranges::for_each(multiLineLabels, [](auto label){ label->setAutoHeight(true); });
	}

	return result;
}

//-----------------------------------------------------------------------------
bool DGTextScrollDialog::runModal(VSTGUI::CFrame* inFrame)
{
	return mModalSession.emplace(inFrame, this).isSessionActive();
}

//-----------------------------------------------------------------------------
void DGTextScrollDialog::recalculateSubViews()
{
	VSTGUI::CScrollView::recalculateSubViews();

	// the scrollbar controls are created in this method, so we need to apply our styling every time this is called
	auto const scrollbar = getVerticalScrollbar();
	assert(scrollbar);
	scrollbar->setFrameColor(VSTGUI::kTransparentCColor);
	scrollbar->setBackgroundColor(VSTGUI::kTransparentCColor);
	scrollbar->setScrollerColor(DGColor::getSystem(DGColor::System::ScrollBarColor));
}






#pragma mark -
#pragma mark DGModalSession

//-----------------------------------------------------------------------------
// Modal Session
//-----------------------------------------------------------------------------
detail::DGModalSession::DGModalSession(VSTGUI::CFrame* inFrame, VSTGUI::CView* inView)
:	mView(inView)
{
	assert(inFrame);
	assert(inView);
	if (auto const modalViewSessionID = inFrame->beginModalViewSession(inView))
	{
		mModalViewSessionID = *modalViewSessionID;
		inView->remember();  // for retain balance, because ending the modal view session will forget this during view removal
	}
}

//-----------------------------------------------------------------------------
detail::DGModalSession::~DGModalSession()
{
	if (mView->getFrame() && mModalViewSessionID)
	{
		[[maybe_unused]] auto const success = mView->getFrame()->endModalViewSession(*mModalViewSessionID);
		assert(success);
	}
}

//-----------------------------------------------------------------------------
bool detail::DGModalSession::isSessionActive() const noexcept
{
	return mModalViewSessionID.has_value();
}
