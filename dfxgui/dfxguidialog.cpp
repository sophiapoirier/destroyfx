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

#include <cmath>
#include <vector>


//-----------------------------------------------------------------------------
constexpr CCoord kContentMargin = 20.0;
constexpr CCoord kButtonSpacing = 12.0;
constexpr CCoord kButtonHeight = 20.0;
constexpr CCoord kTextLabelHeight = 14.0;
constexpr CCoord kTextEditHeight = 20.0;



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
	:	CTextEdit(inRegion, inListener, kDfxParameterID_Invalid)
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
				inContext->setFrameColor(MakeCColor(104, 154, 211));
				inContext->drawGraphicsPath(path, CDrawContext::kPathStroked);
			}
		}
	}
};






#pragma mark -
#pragma mark DGDialog

//-----------------------------------------------------------------------------
// Dialog base class
//-----------------------------------------------------------------------------
DGDialog::DGDialog(Listener* inListener, 
				   DGRect const& inRegion, 
				   std::string const& inMessage, 
				   Buttons inButtons, 
				   char const* inOkButtonTitle, 
				   char const* inCancelButtonTitle, 
				   char const* inOtherButtonTitle)
:	CViewContainer(inRegion), 
	mListener(inListener)
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
	if (!inCancelButtonTitle)
	{
		inCancelButtonTitle = "Cancel";
	}
	if (!inOtherButtonTitle)
	{
		inOtherButtonTitle = "something else";
	}

	DGDialogButton* okButton = nullptr;
	DGDialogButton* otherButton = nullptr;
	std::vector<DGDialogButton*> buttons;
	DGRect const buttonPos(0.0, inRegion.bottom - kButtonHeight - kContentMargin, kButtonHeight, kButtonHeight);
	if (inButtons & kButtons_okBit)
	{
		okButton = new DGDialogButton(this, buttonPos, kSelection_ok, inOkButtonTitle);
		addView(okButton);
		buttons.push_back(okButton);
	}
	if (inButtons & kButtons_cancelBit)
	{
		auto const cancelButton = new DGDialogButton(this, buttonPos, kSelection_cancel, inCancelButtonTitle);
		addView(cancelButton);
		buttons.push_back(cancelButton);
		if (okButton && (okButton->getWidth() < cancelButton->getWidth()))
		{
			okButton->setWidth(cancelButton->getWidth());  // balance
		}
	}
	if (inButtons & kButtons_otherBit)
	{
		otherButton = new DGDialogButton(this, buttonPos, kSelection_other, inOtherButtonTitle);
		addView(otherButton);
		buttons.push_back(otherButton);
	}

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
		button->setX(rightPos - button->getWidth());
	}
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

		constexpr bool shoudlInvalidate = true;
		setViewSize(viewSize, shoudlInvalidate);
		setMouseableArea(viewSize);
	}

	return result;
}

//-----------------------------------------------------------------------------
void DGDialog::valueChanged(CControl* inControl)
{
	auto const button = dynamic_cast<DGDialogButton*>(inControl);
	if (button && mListener)
	{
		mListener->dialogChoiceSelected(this, button->getSelection());
		close();
	}
}

//-----------------------------------------------------------------------------
void DGDialog::close()
{
	invalid();

	if (getFrame() && (getFrame()->getModalView() == this))
	{
		getFrame()->setModalView(nullptr);
	}
}






#pragma mark -
#pragma mark DGTextEntryDialog

//-----------------------------------------------------------------------------
// Text-Entry Dialog
//-----------------------------------------------------------------------------
DGTextEntryDialog::DGTextEntryDialog(Listener* inListener, std::string const& inMessage, 
									 char const* inTextEntryLabel, Buttons inButtons)
:	DGDialog(inListener, DGRect(0.0, 0.0, 247.0, 134.0), inMessage, inButtons)
{
	CCoord const labelEditHeightOffset = (kTextEditHeight - kTextLabelHeight) / 2.0;

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
		addView(mTextEdit);
	}
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
void DGTextEntryDialog::setParameterID(long inParameterID) noexcept
{
	mParameterID = inParameterID;
}

//-----------------------------------------------------------------------------
long DGTextEntryDialog::getParameterID() const noexcept
{
	return mParameterID;
}
