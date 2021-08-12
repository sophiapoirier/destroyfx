/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2021  Sophia Poirier

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

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#pragma once


#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <string>

#include "dfxdefines.h"
#include "dfxgui-base.h"


//-----------------------------------------------------------------------------
class DGRect : public VSTGUI::CRect
{
public:
	constexpr DGRect() = default;
	constexpr DGRect(VSTGUI::CCoord inX, VSTGUI::CCoord inY, VSTGUI::CCoord inWidth, VSTGUI::CCoord inHeight)
	:	VSTGUI::CRect(inX, inY, inX + inWidth, inY + inHeight)
	{
	}
	constexpr DGRect(DGRect const& other)
	:	VSTGUI::CRect(other)
	{
	}
	constexpr DGRect(VSTGUI::CRect const& other)
	:	VSTGUI::CRect(other)
	{
	}
	void set(VSTGUI::CCoord inX, VSTGUI::CCoord inY, VSTGUI::CCoord inWidth, VSTGUI::CCoord inHeight)
	{
		left = inX;
		top = inY;
		right = inX + inWidth;
		bottom = inY + inHeight;
	}
	void setX(VSTGUI::CCoord inX)
	{
		moveTo(inX, top);
	}
	void setY(VSTGUI::CCoord inY)
	{
		moveTo(left, inY);
	}
	void setSize(VSTGUI::CCoord inWidth, VSTGUI::CCoord inHeight)
	{
		VSTGUI::CRect::setSize({inWidth, inHeight});
	}
	constexpr bool isInside(VSTGUI::CCoord inX, VSTGUI::CCoord inY)
	{
		return pointInside({inX, inY});
	}
	constexpr bool isInside_local(VSTGUI::CCoord inX, VSTGUI::CCoord inY)
	{
		return pointInside({inX + left, inY + top});
	}
};



#ifdef TARGET_API_RTAS

//-----------------------------------------------------------------------------
enum
{
	eHighlight_None = -1,
	eHighlight_Red,	// = 0, same as what Pro Tools passes in
	eHighlight_Blue,
	eHighlight_Green,
	eHighlight_Yellow
};

#endif
// TARGET_API_RTAS



//-----------------------------------------------------------------------------
// 3-component RGB color + alpha
class DGColor : public VSTGUI::CColor
{
public:
	static constexpr auto kBlack = VSTGUI::kBlackCColor;
	static constexpr auto kWhite = VSTGUI::kWhiteCColor;

	enum class System
	{
		WindowBackground,
		WindowFrame,
		WindowTitle,
		Label,
		Control,
		ControlText,
		Text,
		TextBackground,
		Accent,
		AccentPressed,
		AccentControlText,
		FocusIndicator,
		ScrollBarColor
	};

	constexpr DGColor() = default;
	constexpr DGColor(int inRed, int inGreen, int inBlue, int inAlpha = 0xFF)
	{
		red = componentFromInt(inRed);
		green = componentFromInt(inGreen);
		blue = componentFromInt(inBlue);
		alpha = componentFromInt(inAlpha);
	}
	DGColor(float inRed, float inGreen, float inBlue, float inAlpha = 1.0f)
	{
		red = componentFromFloat(inRed);
		green = componentFromFloat(inGreen);
		blue = componentFromFloat(inBlue);
		alpha = componentFromFloat(inAlpha);
	}
	constexpr DGColor(VSTGUI::CColor const& inColor)
	{
		red = inColor.red;
		green = inColor.green;
		blue = inColor.blue;
		alpha = inColor.alpha;
	}

	constexpr DGColor withAlpha(int inAlpha) const noexcept
	{
		return DGColor(red, green, blue, componentFromInt(inAlpha));
	}
	DGColor withAlpha(float inAlpha) const;

	DGColor darker(float inAmount) const;
	DGColor brighter(float inAmount) const;

	static DGColor getSystem(System inSystemColorID);

private:
	using ComponentType = decltype(red);
	static constexpr auto kMinValue = std::numeric_limits<ComponentType>::min();
	static constexpr auto kMaxValue = std::numeric_limits<ComponentType>::max();
	static constexpr float kMinValue_f = 0.0f;
	static constexpr float kMaxValue_f = 1.0f;

	static constexpr ComponentType componentFromInt(int inValue) noexcept
	{
		constexpr auto inputMin = static_cast<int>(kMinValue);
		constexpr auto inputMax = static_cast<int>(kMaxValue);
		assert(inValue >= inputMin);
		assert(inValue <= inputMax);
		return static_cast<ComponentType>(std::clamp(inValue, inputMin, inputMax));
	}

	static ComponentType componentFromFloat(float inValue)
	{
		constexpr auto inputMin = kMinValue_f;
		constexpr auto inputMax = kMaxValue_f;
		assert(inValue >= inputMin);
		assert(inValue <= inputMax);
		constexpr auto fixedScalar = static_cast<float>(kMaxValue);
		return static_cast<ComponentType>(std::lround(std::clamp(inValue, inputMin, inputMax) * fixedScalar));
	}

	static constexpr float componentToFloat(ComponentType inValue)
	{
		return static_cast<float>(inValue) / static_cast<float>(kMaxValue);
	}
};



/***********************************************************************
	DGImage
	class for loading and containing images
***********************************************************************/

/* XXX should be "stacked" or "indexed" so that one bitmap might hold
   several clipped regions that can be drawn. (but we use overloading
   or default params so that it behaves like a single image when not
   using those features) */
//-----------------------------------------------------------------------------
class DGImage : public VSTGUI::CBitmap
{
public:
	using VSTGUI::CBitmap::CBitmap;

/*	probably a better interface is:
	void draw(VSTGUI::CCoord x, VSTGUI::CCoord y);
	and also something like this for stacked images:
	void drawex(VSTGUI::CCoord x, VSTGUI::CCoord y, int xIndex, int yIndex);
*/
};



//-----------------------------------------------------------------------------
namespace dfx
{

static inline char const* const kPlusMinusUTF8 = reinterpret_cast<char const*>(u8"\U000000B1");
static inline auto const kInfinityUTF8 = VSTGUI::kInfiniteSymbol;

std::string SanitizeNumericalInput(std::string const& inText);

// perform some platform fixups for the GUI library
void InitGUI();

}  // namespace dfx
