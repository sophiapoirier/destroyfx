/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code
for creating audio processing plug-ins.
Copyright (C) 2002-2024  Sophia Poirier

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

#include "dfxguitextdisplay.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <string_view>
#include <utility>

#include "dfxguieditor.h"
#include "dfxguimisc.h"
#include "dfxmath.h"


//-----------------------------------------------------------------------------
static constexpr VSTGUI::CHoriTxtAlign DFXGUI_TextAlignmentToVSTGUI(dfx::TextAlignment inTextAlignment) noexcept
{
	switch (inTextAlignment)
	{
		case dfx::TextAlignment::Left:
			return VSTGUI::kLeftText;
		case dfx::TextAlignment::Center:
			return VSTGUI::kCenterText;
		case dfx::TextAlignment::Right:
			return VSTGUI::kRightText;
	}
	std::unreachable();
}

//-----------------------------------------------------------------------------
static bool DFXGUI_IsBitmapFont(char const* inFontName) noexcept
{
	return [](std::string_view fontName)
	{
		return (fontName == dfx::kFontName_Snooty10px) || 
			   (fontName == dfx::kFontName_Pasement9px) ||
			   (fontName == dfx::kFontName_Wetar16px);
	}(inFontName ? inFontName : std::string_view{});
}

//-----------------------------------------------------------------------------
// For unknown reasons on windows, text renders at a different
// vertical position compared to Mac, and some letters can be cut off
// (e.g. the lowercase 'g' in DFX Wetar). Since the effect is
// constant, we just work around this with some platform-specific
// tweaks. We don't fully understand what's going on here, but note
// that both the "YOffsetTweak" and "ViewAdjustments" are necessary
// (it would naively seem that they could be combined, but this will
// result in clipping, or 1 offset pixel yielding two pixel movement
// in rendering?).
//
// The way to adjust these is to make sure the text looks right on
// mac in the fonttest plugin, then fiddle with these on windows
// until it also looks right (exactly in the box with nothing cut off).
static int DFXGUI_GetYOffsetTweak(char const* inFontName) noexcept
{
#if TARGET_OS_WIN32
	if (inFontName && (std::strcmp(inFontName, dfx::kFontName_Snooty10px) == 0))
	{
		return -2;
	}
#endif
	return 0;
}

//-----------------------------------------------------------------------------
static std::tuple<int, int, int> GetPlatformViewAdjustments(char const* inFontName) noexcept
{
#if TARGET_OS_WIN32
	if (inFontName == nullptr)
	{
		return std::make_tuple(0, 0, 0);
	}

	std::string const name(inFontName);
	if (name == dfx::kFontName_Snooty10px)
	{
		return std::make_tuple(0, 1, 0);
	}
	if (name == dfx::kFontName_Pasement9px)
	{
		return std::make_tuple(0, 0, 0);
	}
	if (name == dfx::kFontName_Wetar16px)
	{
		return std::make_tuple(0, -3, 3);
	}
#endif

	return std::make_tuple(0, 0, 0);
}

//-----------------------------------------------------------------------------
DGRect detail::AdjustTextViewForPlatform(char const* inFontName,
										 DGRect const& inRect) noexcept
{
	DGRect adjustedRect = inRect;
	auto const [x, y, h] = GetPlatformViewAdjustments(inFontName);
	adjustedRect.offset(x, y);
	adjustedRect.setHeight(adjustedRect.getHeight() + h);
	return adjustedRect;
}

//-----------------------------------------------------------------------------
// Inverse of the above.
static DGRect UnAdjustTextViewForPlatform(char const* inFontName,
										  DGRect const& inRect) noexcept
{
	DGRect adjustedRect = inRect;
	auto const [x, y, h] = GetPlatformViewAdjustments(inFontName);
	adjustedRect.offset(-x, -y);
	adjustedRect.setHeight(adjustedRect.getHeight() - h);
	return inRect;
}

//-----------------------------------------------------------------------------
// common constructor-time setup
static void DFXGUI_ConfigureTextDisplay(DfxGuiEditor* inOwnerEditor,
										VSTGUI::CParamDisplay* inTextDisplay,
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

	inTextDisplay->setAntialias(!DFXGUI_IsBitmapFont(inFontName));

	auto const adjustedRegion = detail::AdjustTextViewForPlatform(inFontName, inTextDisplay->getViewSize());
	inTextDisplay->setViewSize(adjustedRegion, false);
}

//-----------------------------------------------------------------------------
static DGRect DFXGUI_GetTextDrawRegion(DGRect const& inRegion, int inYOffsetTweak, bool inIsBitmapFont)
{
	auto textArea = inRegion;
	if (inIsBitmapFont)
	{
		textArea.makeIntegral();
	}

	textArea.top += inYOffsetTweak;

	return textArea;
}


#pragma mark -
#pragma mark DGTextDisplay

//-----------------------------------------------------------------------------
// Text Display
//-----------------------------------------------------------------------------
DGTextDisplay::DGTextDisplay(DfxGuiEditor*							inOwnerEditor,
							 dfx::ParameterID						inParameterID,
							 DGRect const&							inRegion,
							 VSTGUI::CParamDisplayValueToStringProc	inTextProc,
							 void*									inUserData,
							 DGImage*								inBackgroundImage,
							 dfx::TextAlignment						inTextAlignment,
							 float									inFontSize,
							 DGColor								inFontColor,
							 char const*							inFontName)
:	DGControl<VSTGUI::CTextEdit>(inRegion, inOwnerEditor, dfx::ParameterID_ToVST(inParameterID), nullptr, inBackgroundImage),
	mValueToTextProc(inTextProc ? inTextProc : valueToTextProc_Generic),
	mValueToTextUserData(inUserData),
	mYOffsetTweak(DFXGUI_GetYOffsetTweak(inFontName)),
	mIsBitmapFont(DFXGUI_IsBitmapFont(inFontName))
{
	DFXGUI_ConfigureTextDisplay(inOwnerEditor, this, inTextAlignment, inFontSize, inFontColor, inFontName);

	setValueToStringFunction(std::bind_front(&DGTextDisplay::valueToTextProcBridge, this));
	refreshText();	// trigger an initial value->text conversion

	mTextToValueProc = textToValueProc_Generic;
	setStringToValueFunction(std::bind_front(&DGTextDisplay::textToValueProcBridge, this));
}

//-----------------------------------------------------------------------------
void DGTextDisplay::onMouseDownEvent(VSTGUI::MouseDownEvent& ioEvent)
{
	if (mTextEditEnabled)
	{
		DGControl<VSTGUI::CTextEdit>::onMouseDownEvent(ioEvent);
	}
}

//-----------------------------------------------------------------------------
void DGTextDisplay::onMouseWheelEvent(VSTGUI::MouseWheelEvent& ioEvent)
{
	if (getFrame()->getFocusView() == this)
	{
		VSTGUI::CTextEdit::onMouseWheelEvent(ioEvent);
	}
	else
	{
		DGControl<VSTGUI::CTextEdit>::onMouseWheelEvent(ioEvent);
	}
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
	}
	std::unreachable();
}

//-----------------------------------------------------------------------------
void DGTextDisplay::setTextEditEnabled(bool inEnable) noexcept
{
	mTextEditEnabled = inEnable;
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
	return std::snprintf(outTextUTF8, DGTextDisplay::kTextMaxLength, "%.2f", inValue) > 0;
}

//-----------------------------------------------------------------------------
bool DGTextDisplay::valueToTextProc_LinearToDb(float inValue, char outTextUTF8[], void* inPrecisionOffset)
{
	constexpr auto units = "dB";
	if (inValue <= 0.0f)
	{
		return std::snprintf(outTextUTF8, DGTextDisplay::kTextMaxLength, "-%s %s", dfx::kInfinityUTF8, units) > 0;
	}

	auto const decibelValue = dfx::math::Linear2dB(inValue);
	auto precisionOffset = reinterpret_cast<intptr_t>(inPrecisionOffset);  // HACK :(
	precisionOffset = (std::abs(precisionOffset) > 15) ? 0 : precisionOffset;  // reasonable range check to avert misuse with actual pointer
	auto const prefix = (decibelValue >= (0.01f / std::pow(10.f, precisionOffset))) ? "+" : "";
	int precision = (std::fabs(decibelValue) >= 100.0f) ? 0 : ((std::fabs(decibelValue) >= 10.0f) ? 1 : 2);
	precision = std::max(precision + static_cast<int>(precisionOffset), 0);
	return std::snprintf(outTextUTF8, DGTextDisplay::kTextMaxLength, "%s%.*f %s", prefix, precision, decibelValue, units) > 0;
}

//-----------------------------------------------------------------------------
bool DGTextDisplay::valueToTextProc_Percent(float inValue, char outTextUTF8[], void* /*inUserData*/)
{
	int const precision = ((inValue >= 0.1f) && (inValue <= 99.9f)) ? 1 : 0;
	return std::snprintf(outTextUTF8, DGTextDisplay::kTextMaxLength, "%.*f%%", precision, inValue) > 0;
}

//-----------------------------------------------------------------------------
bool DGTextDisplay::valueToTextProc_LinearToPercent(float inValue, char outTextUTF8[], void* inUserData)
{
	return valueToTextProc_Percent(inValue * 100.0f, outTextUTF8, inUserData);
}

//-----------------------------------------------------------------------------
std::optional<float> DGTextDisplay::textToValueProc_Generic(std::string const& inText, DGTextDisplay* inTextDisplay)
{
	auto const parameterID = inTextDisplay->getParameterID();
	auto const value_d = inTextDisplay->getOwnerEditor()->dfxgui_GetParameterValueFromString_f(parameterID, inText);
	return value_d.transform([](auto value){ return static_cast<float>(value); });
}

//-----------------------------------------------------------------------------
std::optional<float> DGTextDisplay::textToValueProc_DbToLinear(std::string const& inText, DGTextDisplay* inTextDisplay)
{
	if (inText.empty())
	{
		return {};
	}
	if (inText.starts_with('-'))
	{
		constexpr size_t signOffset = 1;
		auto const unsignedText = std::string_view(inText).substr(signOffset);
		if (unsignedText.starts_with(dfx::kInfinityUTF8) || unsignedText.starts_with("inf") || unsignedText.starts_with("Inf") || unsignedText.starts_with("INF"))
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
	if (auto* const frame = getFrame())
	{
		frame->invalid();
	}
#endif
	DGControl<VSTGUI::CTextEdit>::takeFocus();
}

//-----------------------------------------------------------------------------
void DGTextDisplay::drawPlatformText(VSTGUI::CDrawContext* inContext, VSTGUI::IPlatformString* inString, VSTGUI::CRect const& inRegion)
{
	auto const textArea = DFXGUI_GetTextDrawRegion(inRegion, mYOffsetTweak, mIsBitmapFont);
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
	if (!mTextEditEnabled)
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




#pragma mark -
#pragma mark DGStaticTextDisplay

//-----------------------------------------------------------------------------
DGStaticTextDisplay::DGStaticTextDisplay(DfxGuiEditor* inOwnerEditor, DGRect const& inRegion, DGImage* inBackgroundImage,
										 dfx::TextAlignment inTextAlignment, float inFontSize,
										 DGColor inFontColor, char const* inFontName)
:	DGControl<VSTGUI::CTextLabel>(inRegion, nullptr, inBackgroundImage),
	mYOffsetTweak(DFXGUI_GetYOffsetTweak(inFontName)),
	mIsBitmapFont(DFXGUI_IsBitmapFont(inFontName))
{
	DFXGUI_ConfigureTextDisplay(inOwnerEditor, this, inTextAlignment, inFontSize, inFontColor, inFontName);

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
	auto const textArea = DFXGUI_GetTextDrawRegion(inRegion, mYOffsetTweak, mIsBitmapFont);
	VSTGUI::CTextLabel::drawPlatformText(inContext, inString, textArea);
}






#pragma mark -
#pragma mark DGTextArrayDisplay

//-----------------------------------------------------------------------------
// Static Text Display
//-----------------------------------------------------------------------------
DGTextArrayDisplay::DGTextArrayDisplay(DfxGuiEditor* inOwnerEditor, dfx::ParameterID inParameterID, DGRect const& inRegion,
									   size_t inNumStrings, dfx::TextAlignment inTextAlignment, DGImage* inBackground,
									   float inFontSize, DGColor inFontColor, char const* inFontName)
:	DGTextDisplay(inOwnerEditor, inParameterID, inRegion, nullptr, nullptr, inBackground,
				  inTextAlignment, inFontSize, inFontColor, inFontName),
	mDisplayStrings(std::max(inNumStrings, 1uz))
{
	setMouseEnabled(false);
}

//-----------------------------------------------------------------------------
void DGTextArrayDisplay::setText(size_t inStringNum, char const* inText)
{
	assert(inText);
	if (inStringNum >= mDisplayStrings.size())
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
	size_t stringIndex = 0;
	if (isParameterAttached())
	{
		stringIndex = dfx::math::RoundToIndex(getOwnerEditor()->dfxgui_ExpandParameterValue(getParameterID(), getValueNormalized()));
	}
	else
	{
		auto const maxValue_f = static_cast<float>(mDisplayStrings.size() - 1);
		stringIndex = static_cast<size_t>((getValueNormalized() * maxValue_f) + DfxParam::kIntegerPadding);
	}

	if (stringIndex < mDisplayStrings.size())
	{
		drawPlatformText(inContext, VSTGUI::UTF8String(mDisplayStrings[stringIndex]).getPlatformString(), getViewSize());
	}

	setDirty(false);
}






#pragma mark -
#pragma mark DGTextArrayDisplay

//-----------------------------------------------------------------------------
// built-in help text region
//-----------------------------------------------------------------------------
DGHelpBox::DGHelpBox(DfxGuiEditor* inOwnerEditor, DGRect const& inRegion,
					 TextForControlProc const& inTextForControlProc,
					 DGImage* inBackground, DGColor inFontColor)
:	DGStaticTextDisplay(inOwnerEditor, inRegion, inBackground, dfx::TextAlignment::Left,
						dfx::kFontSize_Snooty10px, inFontColor, dfx::kFontName_Snooty10px),
	mOwnerEditor(inOwnerEditor),  // DGStaticTextDisplay does not store this
	mTextForControlProc(inTextForControlProc),
	mHeaderFontColor(inFontColor)
{
	assert(inOwnerEditor);
	assert(inTextForControlProc);

	// HACK part 1: undo "view platform offset", because at the control level, that includes the background,
	// but what we more narrowly want is to offset the individual regions of each line of text
	auto const adjustedRegion = UnAdjustTextViewForPlatform(getFont()->getName(), getViewSize());
	setViewSize(adjustedRegion, false);
}

//-----------------------------------------------------------------------------
// TODO: allow custom styling rather than hardcoding layout, font, and colors
void DGHelpBox::draw(VSTGUI::CDrawContext* inContext)
{
	auto const text = mTextForControlProc(mOwnerEditor->getCurrentControl_mouseover());
	if (text.empty())
	{
		setDirty(false);
		return;
	}

	if (auto const image = getDrawBackground())
	{
		image->draw(inContext, getViewSize());
	}

	auto const fontHeight = getFont()->getSize();
	DGRect textArea(getViewSize());
	textArea.setSize(textArea.getWidth() - mTextMargin.x, fontHeight + 2);
	textArea.offset(mTextMargin);
	// HACK part 2: apply "view platform offset" to the text draw region itself
	textArea = detail::AdjustTextViewForPlatform(getFont()->getName(), textArea);

	std::istringstream stream(text);
	std::string line;
	bool headerDrawn = false;
	while (std::getline(stream, line))
	{
		if (!std::exchange(headerDrawn, true))
		{
			auto const entryFontColor = getFontColor();
			setFontColor(mHeaderFontColor);
			drawPlatformText(inContext, VSTGUI::UTF8String(line).getPlatformString(), textArea);
			textArea.offset(1, 0);
			drawPlatformText(inContext, VSTGUI::UTF8String(line).getPlatformString(), textArea);
			textArea.offset(-1, fontHeight + mLineSpacing + mHeaderSpacing);
			setFontColor(entryFontColor);
		}
		else
		{
			drawPlatformText(inContext, VSTGUI::UTF8String(line).getPlatformString(), textArea);
			textArea.offset(0, fontHeight + mLineSpacing);
		}
	}

	setDirty(false);
}

//-----------------------------------------------------------------------------
void DGHelpBox::setHeaderFontColor(DGColor inColor)
{
	if (std::exchange(mHeaderFontColor, inColor) != inColor)
	{
		setDirty();
	}
}

//-----------------------------------------------------------------------------
void DGHelpBox::setTextMargin(VSTGUI::CPoint const& inMargin)
{
	if (std::exchange(mTextMargin, inMargin) != inMargin)
	{
		setDirty();
	}
}

//-----------------------------------------------------------------------------
void DGHelpBox::setLineSpacing(VSTGUI::CCoord inSpacing)
{
	if (std::exchange(mLineSpacing, inSpacing) != inSpacing)
	{
		setDirty();
	}
}

//-----------------------------------------------------------------------------
void DGHelpBox::setHeaderSpacing(VSTGUI::CCoord inSpacing)
{
	if (std::exchange(mHeaderSpacing, inSpacing) != inSpacing)
	{
		setDirty();
	}
}






#pragma mark -
#pragma mark DGPopUpMenu

//-----------------------------------------------------------------------------
// pop-up menu
//-----------------------------------------------------------------------------
DGPopUpMenu::DGPopUpMenu(DfxGuiEditor* inOwnerEditor, dfx::ParameterID inParameterID, DGRect const& inRegion,
						 dfx::TextAlignment inTextAlignment, float inFontSize,
						 DGColor inFontColor, char const* inFontName,
						 std::optional<DGColor> inBackgroundColor)
:	DGControl<VSTGUI::COptionMenu>(inRegion, inOwnerEditor, dfx::ParameterID_ToVST(inParameterID),
								   nullptr, nullptr, VSTGUI::COptionMenu::kCheckStyle)
{
	DFXGUI_ConfigureTextDisplay(inOwnerEditor, this, inTextAlignment, inFontSize, inFontColor, inFontName);

	if (inBackgroundColor)
	{
		setBackColor(*inBackgroundColor);
		setTransparency(false);
	}

	if (isParameterAttached())
	{
		auto const minValue = std::lround(inOwnerEditor->GetParameter_minValue(inParameterID));
		auto const maxValue = std::lround(inOwnerEditor->GetParameter_maxValue(inParameterID));
		auto const currentValue = inOwnerEditor->getparameter_i(inParameterID);
		for (auto stringIndex = minValue; stringIndex <= maxValue; stringIndex++)
		{
			auto const valueString = inOwnerEditor->getparametervaluestring(inParameterID, stringIndex).value_or(std::to_string(stringIndex));
			addEntry(valueString.c_str());
		}
		setValue(currentValue - minValue);
	}
}
