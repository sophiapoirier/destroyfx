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
#include <stdio.h>
#include <string.h>

#include "dfxguieditor.h"

using DGFontTweaks = internal::DGFontTweaks;

//-----------------------------------------------------------------------------
static bool DFXGUI_GenericValueToTextProc(float inValue, char outTextUTF8[], void* /*inUserData*/)
{
	return snprintf(outTextUTF8, DGTextDisplay::kTextMaxLength, "%.2f", inValue) > 0;
}

//-----------------------------------------------------------------------------
static bool DFXGUI_GenericTextToValueProc(std::string const& inText, float& outValue, DGTextDisplay* textDisplay)
{
	auto const paramID = textDisplay->getParameterID();
	auto const value_d = textDisplay->getOwnerEditor()->dfxgui_GetParameterValueFromString_f(paramID, inText);
	if (value_d)
	{
		outValue = static_cast<float>(*value_d);
	}
	return value_d.has_value();
}

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
static DGFontTweaks DFXGUI_ConfigureTextDisplay(DfxGuiEditor* inOwnerEditor, 
												VSTGUI::CTextLabel* inTextDisplay, 
												DGRect const& inRegion, 
												DGImage* inBackgroundImage, 
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

	const DGFontTweaks fontTweaks = inFontName && strcmp(inFontName, dfx::kFontName_SnootPixel10) == 0 ?
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
	}
	return fontTweaks;
}

//-----------------------------------------------------------------------------
static DGRect DFXGUI_GetTextDrawRegion(DGFontTweaks fontTweaks, DGRect const& inRegion)
{
	auto textArea = inRegion;
	switch (fontTweaks)
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
	mValueToTextProc(inTextProc ? inTextProc : DFXGUI_GenericValueToTextProc),
	mValueToTextUserData(inUserData ? inUserData : this)
{
	mFontTweaks = DFXGUI_ConfigureTextDisplay(inOwnerEditor, this, inRegion, inBackgroundImage, inTextAlignment, inFontSize, inFontColor, inFontName);

	setValueToStringFunction(std::bind(&DGTextDisplay::valueToTextProcBridge, this, 
									   std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	refreshText();  // trigger an initial value->text conversion

	mTextToValueProc = DFXGUI_GenericTextToValueProc;
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
void DGTextDisplay::setTextToValueProc(TextToValueProc const& textToValueProc)
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

	auto const success = mTextToValueProc(dfx::SanitizeNumericalInput(inText).c_str(), outValue, dynamic_cast<DGTextDisplay*>(textEdit));
	if (success && isParameterAttached())
	{
		outValue = getOwnerEditor()->dfxgui_ContractParameterValue(getParameterID(), outValue);
	}

	return success;
}

//-----------------------------------------------------------------------------
// This is called by VSTGUI when the text display transitions to the platform's text
// editor. On windows, we move the text editor up so that it lands where the text
// label was. (XXX presumably there is a less hacky way to do this??)
VSTGUI::CRect DGTextDisplay::platformGetSize() const
{
	VSTGUI::CRect rect = DGControl<VSTGUI::CTextEdit>::platformGetSize();
	switch (mFontTweaks)
	{
		case DGFontTweaks::SNOOTORGPX10:
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
	mFontTweaks = DFXGUI_ConfigureTextDisplay(inOwnerEditor, this, inRegion, inBackgroundImage, inTextAlignment, inFontSize, inFontColor, inFontName);

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
