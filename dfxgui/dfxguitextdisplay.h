/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2023  Sophia Poirier

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


#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "dfxguicontrol.h"


//-----------------------------------------------------------------------------
namespace detail
{
DGRect AdjustTextViewForPlatform(char const* inFontName,
								 DGRect const& inRect) noexcept;
}


//-----------------------------------------------------------------------------
class DGTextDisplay : public DGControl<VSTGUI::CTextEdit>
{
public:
	static constexpr size_t kTextMaxLength = 256;

	// inBackgroundImage may be null
	DGTextDisplay(DfxGuiEditor* inOwnerEditor, dfx::ParameterID inParameterID, DGRect const& inRegion, 
				  VSTGUI::CParamDisplayValueToStringProc inTextProc, void* inUserData, DGImage* inBackgroundImage, 
				  dfx::TextAlignment inTextAlignment = dfx::TextAlignment::Left, float inFontSize = 12.f,
				  DGColor inFontColor = DGColor::kBlack, char const* inFontName = nullptr);

	void onMouseDownEvent(VSTGUI::MouseDownEvent& ioEvent) override;
	void onMouseWheelEvent(VSTGUI::MouseWheelEvent& ioEvent) override;

	void setTextAlignment(dfx::TextAlignment inTextAlignment);
	dfx::TextAlignment getTextAlignment() const noexcept;

	// toggle text editability without affecting mouse wheel functionality
	void setTextEditEnabled(bool inEnable) noexcept;

	// customization point to override the default text entry numerical value parsing with your own
	using TextToValueProc = std::function<std::optional<float>(std::string const& inText, DGTextDisplay* inTextDisplay)>;
	void setTextToValueProc(TextToValueProc const& inTextToValueProc);
	void setTextToValueProc(TextToValueProc&& inTextToValueProc);

	// if you don't need to parse actual text input, but you want to be able to transform the value parsed, 
	// specify this to hook in your transformation
	using ValueFromTextConvertProc = std::function<float(float inValue, DGTextDisplay* inTextDisplay)>;
	void setValueFromTextConvertProc(ValueFromTextConvertProc const& inValueFromTextConvertProc);
	void setValueFromTextConvertProc(ValueFromTextConvertProc&& inValueFromTextConvertProc);

	void refreshText();  // trigger a re-conversion of the numerical value to text

	// some common text<->value translation routines for reuse
	// HACK: as a rather unpleasant hack, you can provide an intptr_t "user data" as a "precision offset", 
	// a delta that will be applied to the printf-style fractional precision value
	static bool valueToTextProc_LinearToDb(float inValue, char outTextUTF8[], void* inPrecisionOffset);
	static bool valueToTextProc_Percent(float inValue, char outTextUTF8[], void* inUserData);
	static bool valueToTextProc_LinearToPercent(float inValue, char outTextUTF8[], void* inUserData);
	static std::optional<float> textToValueProc_Generic(std::string const& inText, DGTextDisplay* inTextDisplay);
	static std::optional<float> textToValueProc_DbToLinear(std::string const& inText, DGTextDisplay* inTextDisplay);
	static float valueFromTextConvertProc_PercentToLinear(float inValue, DGTextDisplay* inTextDisplay);

	CLASS_METHODS(DGTextDisplay, VSTGUI::CTextEdit)

protected:
	void takeFocus() override;
	void drawPlatformText(VSTGUI::CDrawContext* inContext, VSTGUI::IPlatformString* inString, VSTGUI::CRect const& inRegion) override;

	bool valueToTextProcBridge(float inValue, char outTextUTF8[kTextMaxLength], CParamDisplay* inUserData);
	bool textToValueProcBridge(VSTGUI::UTF8StringPtr inText, float& outValue, VSTGUI::CTextEdit* textEdit);

	VSTGUI::CParamDisplayValueToStringProc const mValueToTextProc;
	void* const mValueToTextUserData;
	TextToValueProc mTextToValueProc;
	ValueFromTextConvertProc mValueFromTextConvertProc;

private:
	static bool valueToTextProc_Generic(float inValue, char outTextUTF8[], void* inUserData);

	int const mYOffsetTweak = 0;
	bool const mIsBitmapFont;
	bool mTextEditEnabled = true;
};



#pragma mark -
//-----------------------------------------------------------------------------
class DGStaticTextDisplay : public DGControl<VSTGUI::CTextLabel>
{
public:
	// inBackgroundImage may be null
	DGStaticTextDisplay(DfxGuiEditor* inOwnerEditor, DGRect const& inRegion, DGImage* inBackgroundImage, 
						dfx::TextAlignment inTextAlignment = dfx::TextAlignment::Left, float inFontSize = 12.f, 
						DGColor inFontColor = DGColor::kBlack, char const* inFontName = nullptr);

#if TARGET_OS_MAC
	void setCFText(CFStringRef inText);
#endif

	CLASS_METHODS(DGStaticTextDisplay, VSTGUI::CTextLabel)

protected:
	void drawPlatformText(VSTGUI::CDrawContext* inContext, VSTGUI::IPlatformString* inString, VSTGUI::CRect const& inRegion) override;

private:
	int const mYOffsetTweak = 0;
	bool const mIsBitmapFont;
};



#pragma mark -
//-----------------------------------------------------------------------------
class DGTextArrayDisplay : public DGTextDisplay
{
public:
	DGTextArrayDisplay(DfxGuiEditor* inOwnerEditor, dfx::ParameterID inParameterID, DGRect const& inRegion, size_t inNumStrings, 
					   dfx::TextAlignment inTextAlignment = dfx::TextAlignment::Left, DGImage* inBackground = nullptr, 
					   float inFontSize = 12.f, DGColor inFontColor = DGColor::kBlack, char const* inFontName = nullptr);

	void draw(VSTGUI::CDrawContext* inContext) override;

	using VSTGUI::CTextEdit::setText;
	void setText(size_t inStringNum, char const* inText);

	CLASS_METHODS(DGTextArrayDisplay, DGTextDisplay)

protected:
	std::vector<std::string> mDisplayStrings;
};



#pragma mark -
//-----------------------------------------------------------------------------
class DGHelpBox : public DGStaticTextDisplay
{
public:
	using TextForControlProc = std::function<std::string(IDGControl*)>;

	DGHelpBox(DfxGuiEditor* inOwnerEditor, DGRect const& inRegion, 
			  TextForControlProc const& inTextForControlProc, 
			  DGImage* inBackground = nullptr, DGColor inFontColor = DGColor::kBlack);

	void draw(VSTGUI::CDrawContext* inContext) override;

	void setHeaderFontColor(DGColor inColor);
	void setTextMargin(VSTGUI::CPoint const& inMargin);
	void setLineSpacing(VSTGUI::CCoord inSpacing);
	// additional line spacing only after the header line
	void setHeaderSpacing(VSTGUI::CCoord inSpacing);

	CLASS_METHODS(DGHelpBox, DGStaticTextDisplay)

private:
	DfxGuiEditor* const mOwnerEditor;
	TextForControlProc const mTextForControlProc;
	DGColor mHeaderFontColor;
	VSTGUI::CPoint mTextMargin;
	VSTGUI::CCoord mLineSpacing = 1;
	VSTGUI::CCoord mHeaderSpacing = 2;
};



#pragma mark -
//-----------------------------------------------------------------------------
class DGPopUpMenu : public DGControl<VSTGUI::COptionMenu>
{
public:
	DGPopUpMenu(DfxGuiEditor* inOwnerEditor, dfx::ParameterID inParameterID, DGRect const& inRegion,
				dfx::TextAlignment inTextAlignment = dfx::TextAlignment::Left, float inFontSize = 12.f,
				DGColor inFontColor = DGColor::kBlack, char const* inFontName = nullptr,
				std::optional<DGColor> inBackgroundColor = {});

	CLASS_METHODS(DGPopUpMenu, VSTGUI::COptionMenu)
};
