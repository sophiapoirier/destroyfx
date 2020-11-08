/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2020  Sophia Poirier

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
#include <cmath>
#include <stdio.h>
#include <string.h>

#include "dfxguieditor.h"
#include "dfxmath.h"

using DGFontTweaks = internal::DGFontTweaks;

//-----------------------------------------------------------------------------
static VSTGUI::CHoriTxtAlign DFXGUI_TextAlignmentToVSTGUI(dfx::TextAlignment inTextAlignment)
{
	switch (inTextAlignment)
	{
		case dfx::TextAlignment::Left:
			return VSTGUI::kLeftText;
		case dfx::TextAlignment::Center:
			return VSTGUI::kCenterText;
		case dfx::TextAlignment::Right:
			return VSTGUI::kRightText;
		default:
			assert(false);
			return {};
	}
}

//-----------------------------------------------------------------------------
// Constructor-time setup. Returns computed font tweaks enum to be saved for draw-time tweaking.
[[nodiscard]] static DGFontTweaks DFXGUI_ConfigureTextDisplay(DfxGuiEditor* inOwnerEditor, 
															  VSTGUI::CTextLabel* inTextDisplay, 
															  dfx::TextAlignment inTextAlignment, 
															  float inFontSize, DGColor inFontColor, 
															  char const* inFontName)
{
	inTextDisplay->setTransparency(true);

	inTextDisplay->setHoriAlign(DFXGUI_TextAlignmentToVSTGUI(inTextAlignment));
	inTextDisplay->setFontColor(inFontColor);

	if (auto const fontDesc = inOwnerEditor->CreateVstGuiFont(inFontSize, inFontName))
	{
		inTextDisplay->setFont(fontDesc);
	}

	DGFontTweaks const fontTweaks = inFontName && strcmp(inFontName, dfx::kFontName_SnootPixel10) == 0 ?
	  DGFontTweaks::SNOOTORGPX10 : DGFontTweaks::NONE;

	switch (fontTweaks)
	{
		case DGFontTweaks::NONE:
			inTextDisplay->setAntialias(true);
			break;
		case DGFontTweaks::SNOOTORGPX10:
			inTextDisplay->setAntialias(false);	  
#if TARGET_OS_MAC
			if (inTextAlignment == dfx::TextAlignment::Left)
			{
				inTextDisplay->setTextInset({-1.0, 0.0});
			}
			else if (inTextAlignment == dfx::TextAlignment::Right)
			{
				inTextDisplay->setTextInset({-2.0, 0.0});
			}
#endif
			break;
	}
	return fontTweaks;
}

//-----------------------------------------------------------------------------
static DGRect DFXGUI_GetTextDrawRegion(DGFontTweaks inFontTweaks, DGRect const& inRegion)
{
	auto textArea = inRegion;
	switch (inFontTweaks)
	{
		case DGFontTweaks::SNOOTORGPX10:
#if TARGET_OS_MAC
			textArea.offset(0, -2);
#endif
			textArea.setHeight(textArea.getHeight() + 2);
			textArea.makeIntegral();
			break;
		default:
			break;
	}
	return textArea;
}


#pragma mark -
#pragma mark DGTextDisplay

//-----------------------------------------------------------------------------
// Text Display
//-----------------------------------------------------------------------------
DGTextDisplay::DGTextDisplay(DfxGuiEditor*							inOwnerEditor,
							 long									inParamID, 
							 DGRect const&							inRegion,
							 VSTGUI::CParamDisplayValueToStringProc	inTextProc, 
							 void*									inUserData,
							 DGImage*								inBackgroundImage, 
							 dfx::TextAlignment						inTextAlignment, 
							 float									inFontSize, 
							 DGColor								inFontColor, 
							 char const*							inFontName)
:	DGControl<VSTGUI::CTextEdit>(inRegion, inOwnerEditor, inParamID, nullptr, inBackgroundImage),
	mValueToTextProc(inTextProc ? inTextProc : valueToTextProc_Generic),
	mValueToTextUserData(inUserData ? inUserData : this)
{
	mFontTweaks = DFXGUI_ConfigureTextDisplay(inOwnerEditor, this, inTextAlignment, inFontSize, inFontColor, inFontName);

	setValueToStringFunction(std::bind(&DGTextDisplay::valueToTextProcBridge, this, 
									   std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	refreshText();  // trigger an initial value->text conversion

	mTextToValueProc = textToValueProc_Generic;
	setStringToValueFunction(std::bind(&DGTextDisplay::textToValueProcBridge, this, 
									   std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

//-----------------------------------------------------------------------------
bool DGTextDisplay::onWheel(VSTGUI::CPoint const& inPos, VSTGUI::CMouseWheelAxis const& inAxis, float const& inDistance, VSTGUI::CButtonState const& inButtons)
{
	if (getFrame()->getFocusView() == this)
	{
		return VSTGUI::CTextEdit::onWheel(inPos, inAxis, inDistance, inButtons);
	}
	return DGControl<VSTGUI::CTextEdit>::onWheel(inPos, inAxis, inDistance, inButtons);
}

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
		case VSTGUI::kRightText:
			return dfx::TextAlignment::Right;
		case VSTGUI::kCenterText:
			return dfx::TextAlignment::Center;
		case VSTGUI::kLeftText:
			return dfx::TextAlignment::Left;
		default:
			assert(false);
			return {};
	}
}

//-----------------------------------------------------------------------------
void DGTextDisplay::setTextToValueProc(TextToValueProc const& inTextToValueProc)
{
	assert(inTextToValueProc);
	mTextToValueProc = inTextToValueProc;
}

//-----------------------------------------------------------------------------
void DGTextDisplay::setTextToValueProc(TextToValueProc&& inTextToValueProc)
{
	assert(inTextToValueProc);
	mTextToValueProc = std::move(inTextToValueProc);
}

//-----------------------------------------------------------------------------
void DGTextDisplay::setValueFromTextConvertProc(ValueFromTextConvertProc const& inValueFromTextConvertProc)
{
	mValueFromTextConvertProc = inValueFromTextConvertProc;
}

//-----------------------------------------------------------------------------
void DGTextDisplay::setValueFromTextConvertProc(ValueFromTextConvertProc&& inValueFromTextConvertProc)
{
	mValueFromTextConvertProc = std::move(inValueFromTextConvertProc);
}

//-----------------------------------------------------------------------------
void DGTextDisplay::refreshText()
{
	setValue(getValue());
	redraw();
}

//-----------------------------------------------------------------------------
bool DGTextDisplay::valueToTextProc_Generic(float inValue, char outTextUTF8[], void* /*inUserData*/)
{
	return snprintf(outTextUTF8, DGTextDisplay::kTextMaxLength, "%.2f", inValue) > 0;
}

//-----------------------------------------------------------------------------
bool DGTextDisplay::valueToTextProc_LinearToDb(float inValue, char outTextUTF8[], void* /*inUserData*/)
{
	constexpr auto units = "dB";
	if (inValue <= 0.0f)
	{
		return snprintf(outTextUTF8, DGTextDisplay::kTextMaxLength, "-%s %s", VSTGUI::kInfiniteSymbol, units) > 0;
	}

	auto const decibelValue = dfx::math::Linear2dB(inValue);
	auto const prefix = (decibelValue >= 0.01f) ? "+" : "";
	int const precision = (std::fabs(decibelValue) >= 100.0f) ? 0 : ((std::fabs(decibelValue) >= 10.0f) ? 1 : 2);
	return snprintf(outTextUTF8, DGTextDisplay::kTextMaxLength, "%s%.*f %s", prefix, precision, decibelValue, units) > 0;
}

//-----------------------------------------------------------------------------
bool DGTextDisplay::valueToTextProc_Percent(float inValue, char outTextUTF8[], void* /*inUserData*/)
{
	int const precision = ((inValue >= 0.1f) && (inValue <= 99.9f)) ? 1 : 0;
	return snprintf(outTextUTF8, DGTextDisplay::kTextMaxLength, "%.*f%%", precision, inValue) > 0;
}

//-----------------------------------------------------------------------------
bool DGTextDisplay::valueToTextProc_LinearToPercent(float inValue, char outTextUTF8[], void* inUserData)
{
	return valueToTextProc_Percent(inValue * 100.0f, outTextUTF8, inUserData);
}

//-----------------------------------------------------------------------------
std::optional<float> DGTextDisplay::textToValueProc_Generic(std::string const& inText, DGTextDisplay* inTextDisplay)
{
	auto const paramID = inTextDisplay->getParameterID();
	auto const value_d = inTextDisplay->getOwnerEditor()->dfxgui_GetParameterValueFromString_f(paramID, inText);
	return value_d ? std::make_optional(static_cast<float>(*value_d)) : std::nullopt;
}

//-----------------------------------------------------------------------------
std::optional<float> DGTextDisplay::textToValueProc_DbToLinear(std::string const& inText, DGTextDisplay* inTextDisplay)
{
	if (inText.empty())
	{
		return {};
	}
	if (inText.front() == '-')
	{
		auto const beginsWith = [&inText](auto const& matchText)
		{
			constexpr size_t offset = 1;
			return (inText.length() >= (strlen(matchText) + offset)) && (inText.compare(offset, strlen(matchText), matchText) == 0);
		};
		if (beginsWith(VSTGUI::kInfiniteSymbol) || beginsWith("inf") || beginsWith("Inf") || beginsWith("INF"))
		{
			return 0.0f;
		}
	}

	if (auto const value = textToValueProc_Generic(inText, inTextDisplay))
	{
		return dfx::math::Db2Linear(*value);
	}
	return {};
}

//-----------------------------------------------------------------------------
float DGTextDisplay::valueFromTextConvertProc_PercentToLinear(float inValue, DGTextDisplay* /*inTextDisplay*/)
{
	return inValue / 100.0f;
}

//-----------------------------------------------------------------------------
void DGTextDisplay::takeFocus()
{
#if TARGET_OS_WIN32
	// XXX For unknown reasons on Windows, interacting with a text edit
	// control for the second time in a row causes the rest of the plugin window
	// to go blank. "Fix" this by invalidating the whole plugin UI when a text
	// edit takes focus. If we fix something else about invalidating the display,
	// might be worth revisiting whether this is needed.
	if (auto* frame = getFrame())
	{
		frame->invalid();
	}
#endif
	DGControl<VSTGUI::CTextEdit>::takeFocus();
}

//-----------------------------------------------------------------------------
void DGTextDisplay::drawPlatformText(VSTGUI::CDrawContext* inContext, VSTGUI::IPlatformString* inString, VSTGUI::CRect const& inRegion)
{
	auto const textArea = DFXGUI_GetTextDrawRegion(mFontTweaks, inRegion);
	VSTGUI::CTextEdit::drawPlatformText(inContext, inString, textArea);
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
bool DGTextDisplay::textToValueProcBridge(VSTGUI::UTF8StringPtr inText, float& outValue, VSTGUI::CTextEdit* textEdit)
{
	assert(inText);
	assert(textEdit == this);
	if (!inText)
	{
		return false;
	}

	auto const dgTextEdit = dynamic_cast<DGTextDisplay*>(textEdit);
	assert(dgTextEdit);
	auto const valueFromText = mTextToValueProc(dfx::SanitizeNumericalInput(inText), dgTextEdit);
	outValue = valueFromText.value_or(0.0f);
	if (valueFromText && isParameterAttached())
	{
		if (mValueFromTextConvertProc)
		{
			outValue = mValueFromTextConvertProc(outValue, dgTextEdit);
		}
		outValue = getOwnerEditor()->dfxgui_ContractParameterValue(getParameterID(), outValue);
	}

	return valueFromText.has_value();
}

//-----------------------------------------------------------------------------
// This is called by VSTGUI when the text display transitions to the platform's text
// editor. We move the text editor up so that it lands where the text label was.
// (XXX presumably there is a less hacky way to do this??)
VSTGUI::CRect DGTextDisplay::platformGetSize() const
{
	VSTGUI::CRect rect = DGControl<VSTGUI::CTextEdit>::platformGetSize();
	switch (mFontTweaks)
	{
		case DGFontTweaks::SNOOTORGPX10:
#if TARGET_OS_MAC
			rect.top -= 1;
#endif
#if TARGET_OS_WIN32
			rect.top -= 3;
#endif
			break;
		default:
			break;
	}
	return rect;
}




#pragma mark -
#pragma mark DGStaticTextDisplay

//-----------------------------------------------------------------------------
DGStaticTextDisplay::DGStaticTextDisplay(DfxGuiEditor* inOwnerEditor, DGRect const& inRegion, DGImage* inBackgroundImage, 
										 dfx::TextAlignment inTextAlignment, float inFontSize, 
										 DGColor inFontColor, char const* inFontName)
:	DGControl<VSTGUI::CTextLabel>(inRegion, nullptr, inBackgroundImage)
{
	mFontTweaks = DFXGUI_ConfigureTextDisplay(inOwnerEditor, this, inTextAlignment, inFontSize, inFontColor, inFontName);

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
void DGStaticTextDisplay::drawPlatformText(VSTGUI::CDrawContext* inContext, VSTGUI::IPlatformString* inString, VSTGUI::CRect const& inRegion)
{
	auto const textArea = DFXGUI_GetTextDrawRegion(mFontTweaks, inRegion);
	VSTGUI::CTextLabel::drawPlatformText(inContext, inString, textArea);
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
void DGTextArrayDisplay::draw(VSTGUI::CDrawContext* inContext)
{
	if (auto const image = getDrawBackground())
	{
		image->draw(inContext, getViewSize());
	}

	// TODO: consolidate this logic and DGButton::getValue_i into DGControl?
	long stringIndex = 0;
	if (isParameterAttached())
	{
		stringIndex = std::lround(getOwnerEditor()->dfxgui_ExpandParameterValue(getParameterID(), getValueNormalized()));
	}
	else
	{
		auto const maxValue_f = static_cast<float>(mDisplayStrings.size() - 1);
		stringIndex = static_cast<long>((getValueNormalized() * maxValue_f) + DfxParam::kIntegerPadding);
	}

	if ((stringIndex >= 0) && (static_cast<size_t>(stringIndex) < mDisplayStrings.size()))
	{
		drawPlatformText(inContext, VSTGUI::UTF8String(mDisplayStrings[stringIndex]).getPlatformString(), getViewSize());
	}

	setDirty(false);
}
