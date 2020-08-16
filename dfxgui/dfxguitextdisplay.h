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

#pragma once


#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "dfxguicontrol.h"

namespace internal
{
// Some fonts have tweaks that we apply on a platform-by-platform basis
// to get pixel-perfect alignment.
enum class DGFontTweaks
{
	NONE,
	SNOOTORGPX10
};
}  // namespace internal

//-----------------------------------------------------------------------------
class DGTextDisplay : public DGControl<VSTGUI::CTextEdit>
{
public:
	static constexpr size_t kTextMaxLength = 256;

	DGTextDisplay(DfxGuiEditor* inOwnerEditor, long inParamID, DGRect const& inRegion, 
				  VSTGUI::CParamDisplayValueToStringProc inTextProc, void* inUserData, DGImage* inBackgroundImage, 
				  dfx::TextAlignment inTextAlignment = dfx::TextAlignment::Left, float inFontSize = 12.0f, 
				  DGColor inFontColor = VSTGUI::kBlackCColor, char const* inFontName = nullptr);

	bool onWheel(VSTGUI::CPoint const& inPos, VSTGUI::CMouseWheelAxis const& inAxis, float const& inDistance, VSTGUI::CButtonState const& inButtons) override;

	void setTextAlignment(dfx::TextAlignment inTextAlignment);
	dfx::TextAlignment getTextAlignment() const noexcept;

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
	static bool valueToTextProc_LinearToDb(float inValue, char outTextUTF8[], void* inUserData);
	static std::optional<float> textToValueProc_DbToLinear(std::string const& inText, DGTextDisplay* inTextDisplay);
	static float valueFromTextConvertProc_PercentToLinear(float inValue, DGTextDisplay* inTextDisplay);

	CLASS_METHODS(DGTextDisplay, VSTGUI::CTextEdit)

protected:
	void takeFocus() override;
	void drawPlatformText(VSTGUI::CDrawContext* inContext, VSTGUI::IPlatformString* inString, VSTGUI::CRect const& inRegion) override;
	VSTGUI::CRect platformGetSize() const override;

	bool valueToTextProcBridge(float inValue, char outTextUTF8[kTextMaxLength], CParamDisplay* inUserData);
	bool textToValueProcBridge(VSTGUI::UTF8StringPtr inText, float& outValue, VSTGUI::CTextEdit* textEdit);

	VSTGUI::CParamDisplayValueToStringProc const mValueToTextProc;
	void* const mValueToTextUserData;
	TextToValueProc mTextToValueProc;
	ValueFromTextConvertProc mValueFromTextConvertProc;

private:
	static bool valueToTextProc_Generic(float inValue, char outTextUTF8[], void* inUserData);
	static std::optional<float> textToValueProc_Generic(std::string const& inText, DGTextDisplay* inTextDisplay);

	internal::DGFontTweaks mFontTweaks = internal::DGFontTweaks::NONE;
};



#pragma mark -
//-----------------------------------------------------------------------------
class DGStaticTextDisplay : public DGControl<VSTGUI::CTextLabel>
{
public:
	DGStaticTextDisplay(DfxGuiEditor* inOwnerEditor, DGRect const& inRegion, DGImage* inBackgroundImage, 
						dfx::TextAlignment inTextAlignment = dfx::TextAlignment::Left, float inFontSize = 12.0f, 
						DGColor inFontColor = VSTGUI::kBlackCColor, char const* inFontName = nullptr);

#if TARGET_OS_MAC
	void setCFText(CFStringRef inText);
#endif

	CLASS_METHODS(DGStaticTextDisplay, VSTGUI::CTextLabel)

protected:
	void drawPlatformText(VSTGUI::CDrawContext* inContext, VSTGUI::IPlatformString* inString, VSTGUI::CRect const& inRegion) override;

private:
  	internal::DGFontTweaks mFontTweaks = internal::DGFontTweaks::NONE;
};



#pragma mark -
//-----------------------------------------------------------------------------
class DGTextArrayDisplay : public DGTextDisplay
{
public:
	DGTextArrayDisplay(DfxGuiEditor* inOwnerEditor, long inParamID, DGRect const& inRegion, long inNumStrings, 
					   dfx::TextAlignment inTextAlignment = dfx::TextAlignment::Left, DGImage* inBackground = nullptr, 
					   float inFontSize = 12.0f, DGColor inFontColor = VSTGUI::kBlackCColor, char const* inFontName = nullptr);

	void draw(VSTGUI::CDrawContext* inContext) override;

	using VSTGUI::CTextEdit::setText;
	void setText(long inStringNum, char const* inText);

	CLASS_METHODS(DGTextArrayDisplay, DGTextDisplay)

protected:
	std::vector<std::string> mDisplayStrings;
};
