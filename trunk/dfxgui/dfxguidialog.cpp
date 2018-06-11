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

#include "dfxguidialog.h"

#include <cassert>
#include <cmath>
#include <experimental/optional>
#include <vector>


//-----------------------------------------------------------------------------
constexpr CCoord kContentMargin = 20.0;
constexpr CCoord kButtonSpacing = 12.0;
constexpr CCoord kButtonHeight = 20.0;
constexpr CCoord kTextLabelHeight = 14.0;
constexpr CCoord kTextEditHeight = 20.0;


//-----------------------------------------------------------------------------
static bool DFXGUI_PressButton(CTextButton* inButton, bool inState)
{
	if (inButton)
	{
		auto mousePos = inButton->getMouseableArea().getCenter();
		if (inState)
		{
			inButton->onMouseDown(mousePos, CButtonState(kLButton));
		}
		else
		{
			inButton->onMouseUp(mousePos, CButtonState());
		}
		return true;
	}
	return false;
}


//-----------------------------------------------------------------------------
// Dialog Button
//-----------------------------------------------------------------------------
class DGDialogButton : public CTextButton
{
public:
	DGDialogButton(IControlListener* inListener, DGRect const& inRegion, DGDialog::Selection inSelection, UTF8StringPtr inTitle)
	:	CTextButton(inRegion, inListener, inSelection, inTitle, CTextButton::kKickStyle)
	{
		sizeToFit();
		auto fitSize = getViewSize();
		fitSize.right = std::round(fitSize.right + 0.5);  // round up
		setViewSize(fitSize);
		setMouseableArea(fitSize);
	}

	void takeFocus() override
	{
		setFrameColor(DGColor::kFocusHighlight);
		setFrameColorHighlighted(DGColor::kFocusHighlight);
	}

	void looseFocus() override
	{
		setFrameColor(kBlackCColor);
		setFrameColorHighlighted(kBlackCColor);
	}

	int32_t onKeyDown(VstKeyCode& inKeyCode) override
	{
		if (auto const result = handleKeyEvent(inKeyCode.virt, true))
		{
			return *result;
		}
		return CTextButton::onKeyDown(inKeyCode);
	}

	int32_t onKeyUp(VstKeyCode& inKeyCode) override
	{
		if (auto const result = handleKeyEvent(inKeyCode.virt, false))
		{
			return *result;
		}
		return CTextButton::onKeyUp(inKeyCode);
	}

	DGDialog::Selection getSelection() const
	{
		return static_cast<DGDialog::Selection>(getTag());
	}

	void setX(CCoord inXpos)
	{
		CRect newSize(getViewSize());
		newSize.moveTo(inXpos, newSize.top);
		setViewSize(newSize);
		setMouseableArea(newSize);
	}

	void setWidth(CCoord inWidth)
	{
		CRect newSize(getViewSize());
		newSize.setWidth(inWidth);
		setViewSize(newSize);
		setMouseableArea(newSize);
	}

	CLASS_METHODS(DGDialogButton, CTextButton)

private:
	std::experimental::optional<int32_t> handleKeyEvent(unsigned char inVirtualKey, bool inIsPressed)
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
};






#pragma mark -
#pragma mark DGDialogTextEdit

//-----------------------------------------------------------------------------
// Dialog Text Edit
//-----------------------------------------------------------------------------
class DGDialogTextEdit : public CTextEdit
{
public:
	DGDialogTextEdit(CRect const& inRegion, IControlListener* inListener)
	:	CTextEdit(inRegion, inListener, dfx::kParameterID_Invalid)
	{
		setFontColor(kBlackCColor);
		setBackColor(kWhiteCColor);
		setFrameColor(kBlackCColor);
		setHoriAlign(kCenterText);
		setStyle(kRoundRectStyle);
		setRoundRectRadius(3.0);
	}

	void draw(CDrawContext* inContext) override
	{
		CTextEdit::draw(inContext);

		// add a thicker control-focus highlight (mimic macOS native text input field)
		if (getPlatformTextEdit())
		{
			CCoord const highlightThickness = getFrameWidth() * 2.0;
			auto highlightRect = getViewSize();
			highlightRect.inset(highlightThickness / 2.0, highlightThickness / 2.0);
			auto const path = owned(inContext->createRoundRectGraphicsPath(highlightRect, getRoundRectRadius()));
			if (path)
			{
				inContext->setLineStyle(kLineSolid);
				inContext->setLineWidth(highlightThickness);
				inContext->setFrameColor(DGColor::kFocusHighlight);
				inContext->drawGraphicsPath(path, CDrawContext::kPathStroked);
			}
		}
	}

	CLASS_METHODS(DGDialogTextEdit, CTextEdit)
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
:	CViewContainer(inRegion)
{
	if (!inMessage.empty())
	{
		// TODO: split text into multiple lines when it is too long to fit on a single line
		DGRect const pos(kContentMargin, kContentMargin, getWidth() - (kContentMargin * 2.0), kTextLabelHeight);
		if (auto const label = new CTextLabel(pos, UTF8String(inMessage)))
		{
			label->setFontColor(kBlackCColor);
			label->setBackColor(kTransparentCColor);
			label->setFrameColor(kTransparentCColor);
			label->setHoriAlign(kLeftText);
			if (auto const currentFont = label->getFont())
			{
				auto const newFont = makeOwned<CFontDesc>(*currentFont);
				if (newFont)
				{
					newFont->setStyle(newFont->getStyle() | kBoldFace);
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
		okButton->setFrameWidth(okButton->getFrameWidth() * 1.5);  // bolder outline
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

	auto leftButtonX = kContentMargin;
	// arrange the buttons from right to left
	for (size_t i = 0; i < buttons.size(); i++)
	{
		auto const button = buttons[i];
		DGDialogButton* prevButton = (i == 0) ? nullptr : buttons[i - 1];
		CCoord rightPos;
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
		button->setAutosizeFlags(kAutosizeRight);
		leftButtonX = button->getViewSize().left;
	}
	if (leftButtonX < kContentMargin)
	{
		auto const extraWidth = std::round(kContentMargin - leftButtonX);
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
	return CViewContainer::onKeyDown(inKeyCode);
}

//-----------------------------------------------------------------------------
int32_t DGDialog::onKeyUp(VstKeyCode& inKeyCode)
{
	if (handleKeyEvent(inKeyCode.virt, false))
	{
		return dfx::kKeyEventHandled;
	}
	return CViewContainer::onKeyUp(inKeyCode);
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
void DGDialog::drawBackgroundRect(CDrawContext* inContext, CRect const& inUpdateRect)
{
	inContext->setFillColor(MakeCColor(180, 180, 180, 210));
	inContext->setFrameColor(kBlackCColor);
	inContext->setDrawMode(kAliasing);
	inContext->setLineWidth(1.0);
	inContext->setLineStyle(kLineSolid);

	// not sure why we should ignore supplied region argument, however this is what the base class does
	CRect drawRect(getViewSize());
	drawRect.moveTo(0.0, 0.0);
	inContext->drawRect(drawRect, kDrawFilledAndStroked);
}

//-----------------------------------------------------------------------------
bool DGDialog::attached(CView* inParent)
{
	auto const result = CViewContainer::attached(inParent);

	if (result && inParent && inParent->getFrame())
	{
		CRect viewSize = getViewSize();
		auto const& parentSize = inParent->getFrame()->getViewSize();
		viewSize.centerInside(parentSize);

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
void DGDialog::valueChanged(CControl* inControl)
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
bool DGDialog::runModal(CFrame* inFrame, Listener* inListener)
{
	assert(inListener);
	mListener = inListener;
	return runModal(inFrame);
}

//-----------------------------------------------------------------------------
bool DGDialog::runModal(CFrame* inFrame, DialogChoiceSelectedCallback&& inCallback)
{
	assert(inCallback);
	mDialogChoiceSelectedCallback = std::move(inCallback);
	return runModal(inFrame);
}

//-----------------------------------------------------------------------------
bool DGDialog::runModal(CFrame* inFrame)
{
	assert(inFrame);
	return inFrame->setModalView(this);
}

//-----------------------------------------------------------------------------
void DGDialog::close()
{
	mListener = nullptr;
	mDialogChoiceSelectedCallback = nullptr;

	if (getFrame() && (getFrame()->getModalView() == this))
	{
		getFrame()->setModalView(nullptr);
	}
}

//-----------------------------------------------------------------------------
CTextButton* DGDialog::getButton(Selection inSelection) const
{
	auto const& children = getChildren();
	for (auto const& child : children)
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
	constexpr CCoord labelEditHeightOffset = (kTextEditHeight - kTextLabelHeight) / 2.0;

	DGRect pos(getViewSize());
	pos.moveTo(0.0, 0.0);
	pos.inset(kContentMargin, kContentMargin);
	pos.setHeight(kTextLabelHeight);
	pos.setY(getHeight() - pos.getHeight() - labelEditHeightOffset - kButtonHeight - (kContentMargin * 2.0));
	CTextLabel* label = inTextEntryLabel ? new CTextLabel(pos, inTextEntryLabel) : nullptr;
	if (label)
	{
		label->setFontColor(kBlackCColor);
		label->setBackColor(kTransparentCColor);
		label->setFrameColor(kTransparentCColor);
		label->setHoriAlign(kLeftText);
		label->sizeToFit();
		addView(label);
	}

	if (label)
	{
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
CMessageResult DGTextEntryDialog::notify(CBaseObject* inSender, IdStringPtr inMessage)
{
	// allow return key in the text edit to additionally trigger the default button
	if (auto const textEdit = dynamic_cast<CTextEdit*>(inSender))
	{
		if ((strcmp(inMessage, kMsgLooseFocus) == 0) && textEdit->bWasReturnPressed)
		{
			if (DFXGUI_PressButton(getButton(kSelection_OK), true))
			{
				return kMessageNotified;
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
		mTextEdit->setText(UTF8String(inText));
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
