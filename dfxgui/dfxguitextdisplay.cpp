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

#include "dfxguitextdisplay.h"

#include <cassert>
#include <stdio.h>
#include <string.h>

#include "dfxguieditor.h"


namespace
{

//-----------------------------------------------------------------------------
static bool DFXGUI_GenericValueToTextProc(float inValue, char outTextUTF8[], void* /*inUserData*/)
{
	snprintf(outTextUTF8, DGTextDisplay::kTextMaxLength, "%.2f", inValue);
	return true;
}

//-----------------------------------------------------------------------------
static bool DFXGUI_GenericTextToValueProc(std::string const& inText, float& outValue, DGTextDisplay* textDisplay)
{
	auto const paramID = textDisplay->getParameterID();
	double value_d {};
	auto const success = textDisplay->getOwnerEditor()->dfxgui_GetParameterValueFromString_f(paramID, inText, value_d);
	if (success)
	{
		outValue = static_cast<float>(value_d);
	}
	return success;
}

//-----------------------------------------------------------------------------
static VSTGUI::CHoriTxtAlign DFXGUI_TextAlignmentToVSTGUI(dfx::TextAlignment inTextAlignment)
{
	switch (inTextAlignment)
	{
		case dfx::TextAlignment::Left:
			return kLeftText;
		case dfx::TextAlignment::Center:
			return kCenterText;
		case dfx::TextAlignment::Right:
			return kRightText;
		default:
			assert(false);
			return {};
	}
}

//-----------------------------------------------------------------------------
static void DFXGUI_ConfigureTextDisplay(CTextLabel* inTextDisplay, 
										DGRect const& inRegion, DGImage* inBackgroundImage, 
										dfx::TextAlignment inTextAlignment, 
										float inFontSize, DGColor inFontColor, char const* inFontName)
{
	inTextDisplay->setTransparency(true);
	if (!inBackgroundImage)
	{
		inTextDisplay->setBackOffset(inRegion.getTopLeft());
	}

	inTextDisplay->setHoriAlign(DFXGUI_TextAlignmentToVSTGUI(inTextAlignment));
	inTextDisplay->setFontColor(inFontColor);

	if (auto const fontDesc = dfx::CreateVstGuiFont(inFontSize, inFontName))
	{
		inTextDisplay->setFont(fontDesc);
	}

	bool const isSnootPixel10 = inFontName && (strcmp(inFontName, dfx::kFontName_SnootPixel10) == 0);
	if (isSnootPixel10)
	{
		// XXX a hack for this font
		if (inTextAlignment == dfx::TextAlignment::Left)
		{
			inTextDisplay->setTextInset(CPoint(-1.0, 0.0));
		}
		else if (inTextAlignment == dfx::TextAlignment::Right)
		{
			inTextDisplay->setTextInset(CPoint(-2.0, 0.0));
		}
	}
	inTextDisplay->setAntialias(!isSnootPixel10);
}

//-----------------------------------------------------------------------------
static DGRect DFXGUI_GetTextDrawRegion(CTextLabel* inTextDisplay, DGRect const& inRegion)
{
	auto textArea = inRegion;
	if (inTextDisplay->getFont()->getName() == dfx::kFontName_SnootPixel10)
	{
		textArea.offset(0, -2);
		textArea.setHeight(textArea.getHeight() + 2);
	}
	return textArea;
}

}  // namespace



#pragma mark -
#pragma mark DGTextDisplay

//-----------------------------------------------------------------------------
// Text Display
//-----------------------------------------------------------------------------
DGTextDisplay::DGTextDisplay(DfxGuiEditor*					inOwnerEditor,
							long							inParamID, 
							DGRect const&					inRegion,
							CParamDisplayValueToStringProc	inTextProc, 
							void*							inUserData,
							DGImage*						inBackgroundImage, 
							dfx::TextAlignment				inTextAlignment, 
							float							inFontSize, 
							DGColor							inFontColor, 
							char const*						inFontName)
:	DGControl<CTextEdit>(inRegion, inOwnerEditor, inParamID, nullptr, inBackgroundImage)
{
	DFXGUI_ConfigureTextDisplay(this, inRegion, inBackgroundImage, inTextAlignment, inFontSize, inFontColor, inFontName);

	if (!inTextProc)
	{
		inTextProc = DFXGUI_GenericValueToTextProc;
	}
	if (!inUserData)
	{
		inUserData = this;
	}
	mValueToTextProc = inTextProc;
	mValueToTextUserData = inUserData;
	setValueToStringFunction(std::bind(&DGTextDisplay::valueToTextProcBridge, this, 
									   std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	refreshText();  // trigger an initial value->text conversion

	mTextToValueProc = DFXGUI_GenericTextToValueProc;
	setStringToValueFunction(std::bind(&DGTextDisplay::textToValueProcBridge, this, 
									   std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

#if 0
//-----------------------------------------------------------------------------
CMouseEventResult DGTextDisplay::onMouseDown(CPoint& inPos, const CButtonState& inButtons)
{
	mLastX = inPos.x;
	mLastY = inPos.y;
	return CTextEdit::onMouseDown(inPos, inButtons);
}

//-----------------------------------------------------------------------------
CMouseEventResult DGTextDisplay::onMouseMoved(CPoint& inPos, const CButtonState& inButtons)
{
	const long min = GetControl32BitMinimum(carbonControl);
	const long max = GetControl32BitMaximum(carbonControl);
	long val = GetControl32BitValue(carbonControl);
	const long oldval = val;

	float diff = 0.0f;
	if (mMouseAxis & dfx::kAxis_Horizontal)
		diff += inPos.x - mLastX;
	if (mMouseAxis & dfx::kAxis_Vertical)
		diff += mLastY - inPos.y;
	if (inButtons.getModifierState() & kZoomModifier)	// slo-mo
		diff /= getFineTuneFactor();
	val += (long) (diff * (float)(max-min) / getMouseDragRange());

	if (val > max)
		val = max;
	if (val < min)
		val = min;
	if (val != oldval)
		SetControl32BitValue(carbonControl, val);

	mLastX = inPos.x;
	mLastY = inPos.y;
	return CTextEdit::onMouseMoved(inPos, inButtons);
}
#endif

//-----------------------------------------------------------------------------
void DGTextDisplay::setTextAlignment(dfx::TextAlignment inTextAlignment)
{
	setHoriAlign(DFXGUI_TextAlignmentToVSTGUI(inTextAlignment));
}

//-----------------------------------------------------------------------------
dfx::TextAlignment DGTextDisplay::getTextAlignment() const noexcept
{
	switch (getHoriAlign())
	{
		case kRightText:
			return dfx::TextAlignment::Right;
		case kCenterText:
			return dfx::TextAlignment::Center;
		case kLeftText:
			return dfx::TextAlignment::Left;
		default:
			assert(false);
			return {};
	}
}

//-----------------------------------------------------------------------------
void DGTextDisplay::setTextToValueProc(const TextToValueProc& textToValueProc)
{
	mTextToValueProc = textToValueProc;
}

//-----------------------------------------------------------------------------
void DGTextDisplay::setTextToValueProc(TextToValueProc&& textToValueProc)
{
	mTextToValueProc = std::move(textToValueProc);
}

//-----------------------------------------------------------------------------
void DGTextDisplay::refreshText()
{
	setValue(getValue());
}

//-----------------------------------------------------------------------------
void DGTextDisplay::drawPlatformText(CDrawContext* inContext, IPlatformString* inString, CRect const& inRegion)
{
	auto const textArea = DFXGUI_GetTextDrawRegion(this, inRegion);
	CTextEdit::drawPlatformText(inContext, inString, textArea);
}

//-----------------------------------------------------------------------------
bool DGTextDisplay::valueToTextProcBridge(float inValue, char outTextUTF8[256], CParamDisplay* /*inUserData*/)
{
	if (isParameterAttached())
	{
		inValue = getOwnerEditor()->dfxgui_ExpandParameterValue(getParameterID(), inValue);
	}
	return mValueToTextProc(inValue, outTextUTF8, mValueToTextUserData);
}

//-----------------------------------------------------------------------------
bool DGTextDisplay::textToValueProcBridge(UTF8StringPtr inText, float& outValue, CTextEdit* textEdit)
{
	assert(inText);
	assert(dynamic_cast<DGTextDisplay*>(textEdit) == this);
	if (!inText)
	{
		return false;
	}

	auto const success = mTextToValueProc(dfx::RemoveDigitSeparators(inText).c_str(), outValue, dynamic_cast<DGTextDisplay*>(textEdit));
	if (success && isParameterAttached())
	{
		outValue = getOwnerEditor()->dfxgui_ContractParameterValue(getParameterID(), outValue);
	}

	return success;
}






#pragma mark -
#pragma mark DGStaticTextDisplay

//-----------------------------------------------------------------------------
DGStaticTextDisplay::DGStaticTextDisplay(DGRect const& inRegion, DGImage* inBackgroundImage, 
										 dfx::TextAlignment inTextAlignment, float inFontSize, 
										 DGColor inFontColor, char const* inFontName)
:	DGControl<CTextLabel>(inRegion, nullptr, inBackgroundImage)
{
	DFXGUI_ConfigureTextDisplay(this, inRegion, inBackgroundImage, inTextAlignment, inFontSize, inFontColor, inFontName);

	setMouseEnabled(false);
}

#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
void DGStaticTextDisplay::setCFText(CFStringRef inText)
{
	if (inText)
	{
		if (auto const cString = dfx::CreateCStringFromCFString(inText))
		{
			setText(cString.get());
		}
	}
	else
	{
		setText(nullptr);
	}
}
#endif

//-----------------------------------------------------------------------------
void DGStaticTextDisplay::drawPlatformText(CDrawContext* inContext, IPlatformString* inString, CRect const& inRegion)
{
	auto const textArea = DFXGUI_GetTextDrawRegion(this, inRegion);
	CTextLabel::drawPlatformText(inContext, inString, textArea);
}






#pragma mark -
#pragma mark DGTextArrayDisplay

//-----------------------------------------------------------------------------
// Static Text Display
//-----------------------------------------------------------------------------
DGTextArrayDisplay::DGTextArrayDisplay(DfxGuiEditor* inOwnerEditor, long inParamID, DGRect const& inRegion, 
									   long inNumStrings, dfx::TextAlignment inTextAlignment, DGImage* inBackground, 
									   float inFontSize, DGColor inFontColor, char const* inFontName)
:	DGTextDisplay(inOwnerEditor, inParamID, inRegion, nullptr, nullptr, inBackground, 
				  inTextAlignment, inFontSize, inFontColor, inFontName), 
	mDisplayStrings(std::max(inNumStrings, 1L))
{
	setMouseEnabled(false);
}

//-----------------------------------------------------------------------------
void DGTextArrayDisplay::setText(long inStringNum, char const* inText)
{
	assert(inText);
	if ((inStringNum < 0) || (inStringNum >= static_cast<long>(mDisplayStrings.size())))
	{
		return;
	}

	mDisplayStrings[inStringNum].assign(inText);
	setDirty();
}

//-----------------------------------------------------------------------------
void DGTextArrayDisplay::draw(CDrawContext* inContext)
{
	if (auto const image = getDrawBackground())
	{
		image->draw(inContext, getViewSize());
	}

	// TODO: consolidate this logic and DGButton::getValue_i into DGControl?
	long stringIndex = 0;
	if (isParameterAttached())
	{
		stringIndex = std::lround(getOwnerEditor()->dfxgui_ExpandParameterValue(getParameterID(), getValue()));
	}
	else
	{
		auto const maxValue_f = static_cast<float>(mDisplayStrings.size() - 1);
		stringIndex = static_cast<long>((getValue() * maxValue_f) + DfxParam::kIntegerPadding);
	}

	if ((stringIndex >= 0) && (static_cast<size_t>(stringIndex) < mDisplayStrings.size()))
	{
		drawPlatformText(inContext, UTF8String(mDisplayStrings[stringIndex]).getPlatformString(), getViewSize());
	}

	setDirty(false);
}
