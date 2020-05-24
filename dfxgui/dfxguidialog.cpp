/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2015-2020  Sophia Poirier

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

#include "dfxguidialog.h"

#include <cassert>
#include <cmath>
#include <optional>
#include <vector>


//-----------------------------------------------------------------------------
constexpr VSTGUI::CCoord kContentMargin = 20.0;
constexpr VSTGUI::CCoord kButtonSpacing = 12.0;
constexpr VSTGUI::CCoord kButtonHeight = 20.0;
constexpr VSTGUI::CCoord kTextLabelHeight = 14.0;
constexpr VSTGUI::CCoord kTextEditHeight = 20.0;
constexpr VSTGUI::CCoord kFocusIndicatorThickness = 2.4;


//-----------------------------------------------------------------------------
namespace
{
static bool DFXGUI_PressButton(VSTGUI::CTextButton* inButton, bool inState)
{
	if (inButton)
	{
		auto mousePos = inButton->getMouseableArea().getCenter();
		if (inState)
		{
			inButton->onMouseDown(mousePos, VSTGUI::CButtonState(VSTGUI::kLButton));
		}
		else
		{
			inButton->onMouseUp(mousePos, VSTGUI::CButtonState());
		}
		return true;
	}
	return false;
}
}  // namespace


//-----------------------------------------------------------------------------
// Dialog Button
//-----------------------------------------------------------------------------
class DGDialogButton : public VSTGUI::CTextButton
{
public:
	DGDialogButton(VSTGUI::IControlListener* inListener, DGRect const& inRegion, DGDialog::Selection inSelection, VSTGUI::UTF8StringPtr inTitle)
	:	VSTGUI::CTextButton(inRegion, inListener, dfx::kParameterID_Invalid, inTitle, VSTGUI::CTextButton::kKickStyle),
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

	int32_t onKeyDown(VstKeyCode& inKeyCode) override
	{
		if (auto const result = handleKeyEvent(inKeyCode.virt, true))
		{
			return *result;
		}
		return VSTGUI::CTextButton::onKeyDown(inKeyCode);
	}

	int32_t onKeyUp(VstKeyCode& inKeyCode) override
	{
		if (auto const result = handleKeyEvent(inKeyCode.virt, false))
		{
			return *result;
		}
		return VSTGUI::CTextButton::onKeyUp(inKeyCode);
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

	CLASS_METHODS(DGDialogButton, VSTGUI::CTextButton)

private:
	std::optional<int32_t> handleKeyEvent(unsigned char inVirtualKey, bool inIsPressed)
	{
		// let the parent dialog handle this key
		if (inVirtualKey == VKEY_RETURN)
		{
			return dfx::kKeyEventNotHandled;
		}
		else if (inVirtualKey == VKEY_SPACE)
		{
			DFXGUI_PressButton(this, inIsPressed);
			return dfx::kKeyEventHandled;
		}
		return {};
	}

	DGDialog::Selection const mSelection;
	bool const mIsDefaultButton;
};






#pragma mark -
#pragma mark DGDialogTextEdit

//-----------------------------------------------------------------------------
// Dialog Text Edit
//-----------------------------------------------------------------------------
class DGDialogTextEdit : public VSTGUI::CTextEdit
{
public:
	DGDialogTextEdit(VSTGUI::CRect const& inRegion, VSTGUI::IControlListener* inListener)
	:	VSTGUI::CTextEdit(inRegion, inListener, dfx::kParameterID_Invalid)
	{
		setFontColor(DGColor::getSystem(DGColor::System::Text));
		setBackColor(DGColor::getSystem(DGColor::System::TextBackground));
		setFrameColor(DGColor::getSystem(DGColor::System::WindowFrame));
		setHoriAlign(VSTGUI::kCenterText);
		setStyle(kRoundRectStyle);
		setRoundRectRadius(3.0);
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

	CLASS_METHODS(DGDialogTextEdit, VSTGUI::CTextEdit)
};






#pragma mark -
#pragma mark DGDialog

//-----------------------------------------------------------------------------
// Dialog base class
//-----------------------------------------------------------------------------
const DGDialog::Buttons DGDialog::kButtons_OK = kButtons_OKBit;
const DGDialog::Buttons DGDialog::kButtons_OKCancel = kButtons_OKBit | kButtons_CancelBit;
const DGDialog::Buttons DGDialog::kButtons_OKCancelOther = kButtons_OKBit | kButtons_CancelBit | kButtons_OtherBit;

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
		if (auto const label = new VSTGUI::CTextLabel(pos, VSTGUI::UTF8String(inMessage)))
		{
			label->setFontColor(DGColor::getSystem(DGColor::System::WindowTitle));
			label->setBackColor(VSTGUI::kTransparentCColor);
			label->setFrameColor(VSTGUI::kTransparentCColor);
			label->setHoriAlign(VSTGUI::kLeftText);
			if (auto const currentFont = label->getFont())
			{
				auto const newFont = VSTGUI::makeOwned<VSTGUI::CFontDesc>(*currentFont);
				if (newFont)
				{
					newFont->setStyle(newFont->getStyle() | VSTGUI::kBoldFace);
					label->setFont(newFont);
				}
			}
			addView(label);
		}
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
int32_t DGDialog::onKeyDown(VstKeyCode& inKeyCode)
{
	if (handleKeyEvent(inKeyCode.virt, true))
	{
		return dfx::kKeyEventHandled;
	}
	return VSTGUI::CViewContainer::onKeyDown(inKeyCode);
}

//-----------------------------------------------------------------------------
int32_t DGDialog::onKeyUp(VstKeyCode& inKeyCode)
{
	if (handleKeyEvent(inKeyCode.virt, false))
	{
		return dfx::kKeyEventHandled;
	}
	return VSTGUI::CViewContainer::onKeyUp(inKeyCode);
}

//-----------------------------------------------------------------------------
bool DGDialog::handleKeyEvent(unsigned char inVirtualKey, bool inIsPressed)
{
	if (inVirtualKey == VKEY_RETURN)
	{
		return DFXGUI_PressButton(getButton(kSelection_OK), inIsPressed);
	}
	else if (inVirtualKey == VKEY_ESCAPE)
	{
		return DFXGUI_PressButton(getButton(kSelection_Cancel), inIsPressed);
	}
	return false;
}

//-----------------------------------------------------------------------------
void DGDialog::drawBackgroundRect(VSTGUI::CDrawContext* inContext, VSTGUI::CRect const& inUpdateRect)
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
	}

	return result;
}

//-----------------------------------------------------------------------------
void DGDialog::valueChanged(VSTGUI::CControl* inControl)
{
	auto const button = dynamic_cast<DGDialogButton*>(inControl);
	if (button && (button->getValue() > button->getMin()))
	{
		bool completed = true;  // default to yes in case there are no handlers, we are not trapped in this dialog
		if (mDialogChoiceSelectedCallback)
		{
			completed = mDialogChoiceSelectedCallback(this, button->getSelection());
		}
		else if (mListener)
		{
			completed = mListener->dialogChoiceSelected(this, button->getSelection());
		}
		else
		{
			assert(false);
		}
		if (completed)
		{
			close();
		}
	}
}

//-----------------------------------------------------------------------------
bool DGDialog::runModal(VSTGUI::CFrame* inFrame, Listener* inListener)
{
	assert(inListener);
	mListener = inListener;
	return runModal(inFrame);
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
	assert(inFrame);
	mModalViewSessionID.reset();
	if (auto const modalViewSessionID = inFrame->beginModalViewSession(this))
	{
		mModalViewSessionID = *modalViewSessionID;
		remember();  // for retain balance, because ending the modal view session will forget this during view removal
	}
	return mModalViewSessionID.has_value();
}

//-----------------------------------------------------------------------------
void DGDialog::close()
{
	mListener = nullptr;
	mDialogChoiceSelectedCallback = nullptr;

	if (getFrame() && mModalViewSessionID)
	{
		[[maybe_unused]] auto const success = getFrame()->endModalViewSession(*mModalViewSessionID);
		assert(success);
	}
	mModalViewSessionID.reset();
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
DGTextEntryDialog::DGTextEntryDialog(long inParamID, std::string const& inMessage, 
									 char const* inTextEntryLabel, Buttons inButtons, 
									 char const* inOkButtonTitle, char const* inCancelButtonTitle, char const* inOtherButtonTitle)
:	DGDialog(DGRect(0.0, 0.0, 247.0, 134.0), inMessage, inButtons, inOkButtonTitle, inCancelButtonTitle, inOtherButtonTitle), 
	mParameterID(inParamID)
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
		label->sizeToFit();
		addView(label);

		pos.left = std::round(label->getViewSize().right + kButtonSpacing);
		pos.right = getWidth() - kContentMargin;
	}

	pos.setHeight(kTextEditHeight);
	pos.offset(0.0, -labelEditHeightOffset);
	mTextEdit = new DGDialogTextEdit(pos, this);
	if (mTextEdit)
	{
		addView(mTextEdit, getView(0));
	}
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
		if ((strcmp(inMessage, VSTGUI::kMsgLooseFocus) == 0) && textEdit->bWasReturnPressed)
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
long DGTextEntryDialog::getParameterID() const noexcept
{
	return mParameterID;
}
