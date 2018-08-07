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

#pragma once


#include <functional>
#include <string>
#include <vector>

#include "dfxguicontrol.h"


//-----------------------------------------------------------------------------
class DGTextDisplay : public DGControl<CTextEdit>
{
public:
	static constexpr size_t kTextMaxLength = 256;

	DGTextDisplay(DfxGuiEditor* inOwnerEditor, long inParamID, DGRect const& inRegion, 
				  CParamDisplayValueToStringProc inTextProc, void* inUserData, DGImage* inBackgroundImage, 
				  dfx::TextAlignment inTextAlignment = dfx::TextAlignment::Left, float inFontSize = 12.0f, 
				  DGColor inFontColor = kBlackCColor, char const* inFontName = nullptr);

#if 0
	CMouseEventResult onMouseDown(CPoint& inPos, CButtonState const& inButtons) override;
	CMouseEventResult onMouseMoved(CPoint& inPos, CButtonState const& inButtons) override;
#endif

	void setTextAlignment(dfx::TextAlignment inTextAlignment);
	dfx::TextAlignment getTextAlignment() const noexcept;
#if 0
	void setMouseAxis(dfx::Axis inMouseAxis) noexcept
	{
		mMouseAxis = inMouseAxis;
	}
#endif

	using TextToValueProc = std::function<bool(std::string const& inText, float& outValue, DGTextDisplay* textDisplay)>;
	void setTextToValueProc(const TextToValueProc& textToValueProc);
	void setTextToValueProc(TextToValueProc&& textToValueProc);

	void refreshText();  // trigger a re-conversion of the numerical value to text

	CLASS_METHODS(DGTextDisplay, CTextEdit)

protected:
	void drawPlatformText(CDrawContext* inContext, IPlatformString* inString, CRect const& inRegion) override;

	bool valueToTextProcBridge(float inValue, char outTextUTF8[kTextMaxLength], CParamDisplay* inUserData);
	bool textToValueProcBridge(UTF8StringPtr inText, float& outValue, CTextEdit* textEdit);

	CParamDisplayValueToStringProc mValueToTextProc = nullptr;
	void* mValueToTextUserData = nullptr;
	TextToValueProc mTextToValueProc;

#if 0
	dfx::Axis mMouseAxis = dfx::kAxis_Vertical;  // indicates which directions you can mouse to adjust the control's value
	float mLastX = 0.0f, mLastY = 0.0f;
#endif
};



#pragma mark -
//-----------------------------------------------------------------------------
class DGStaticTextDisplay : public DGControl<CTextLabel>
{
public:
	DGStaticTextDisplay(DGRect const& inRegion, DGImage* inBackgroundImage, 
						dfx::TextAlignment inTextAlignment = dfx::TextAlignment::Left, float inFontSize = 12.0f, 
						DGColor inFontColor = kBlackCColor, char const* inFontName = nullptr);

#if TARGET_OS_MAC
	void setCFText(CFStringRef inText);
#endif

	CLASS_METHODS(DGStaticTextDisplay, CTextLabel)

protected:
	void drawPlatformText(CDrawContext* inContext, IPlatformString* inString, CRect const& inRegion) override;
};



#pragma mark -
//-----------------------------------------------------------------------------
class DGTextArrayDisplay : public DGTextDisplay
{
public:
	DGTextArrayDisplay(DfxGuiEditor* inOwnerEditor, long inParamID, DGRect const& inRegion, long inNumStrings, 
					   dfx::TextAlignment inTextAlignment = dfx::TextAlignment::Left, DGImage* inBackground = nullptr, 
					   float inFontSize = 12.0f, DGColor inFontColor = kBlackCColor, char const* inFontName = nullptr);

	void draw(CDrawContext* inContext) override;

	using CTextEdit::setText;
	void setText(long inStringNum, char const* inText);

	CLASS_METHODS(DGTextArrayDisplay, DGTextDisplay)

protected:
	std::vector<std::string> mDisplayStrings;
};
