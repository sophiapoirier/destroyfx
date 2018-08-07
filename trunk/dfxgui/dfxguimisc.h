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


#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <string>

#include "dfxdefines.h"
#include "dfxgui-base.h"

#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wunused-parameter"
	#pragma clang diagnostic ignored "-Wnon-virtual-dtor"
	#include "vstgui.h"
#pragma clang diagnostic pop

#if TARGET_OS_MAC
	#include <ApplicationServices/ApplicationServices.h>
#endif


#define FLIP_CG_COORDINATES


//-----------------------------------------------------------------------------
class DGRect : public CRect
{
public:
	DGRect() = default;
	DGRect(CCoord inX, CCoord inY, CCoord inWidth, CCoord inHeight)
	:	CRect(inX, inY, inX + inWidth, inY + inHeight)
	{
	}
	DGRect(DGRect const& other)
	:	CRect(other)
	{
	}
	DGRect(CRect const& other)
	:	CRect(other)
	{
	}
	void set(CCoord inX, CCoord inY, CCoord inWidth, CCoord inHeight)
	{
		left = inX;
		top = inY;
		right = inX + inWidth;
		bottom = inY + inHeight;
	}
	void setX(CCoord inX)
	{
		moveTo(inX, top);
	}
	void setY(CCoord inY)
	{
		moveTo(left, inY);
	}
	void setSize(CCoord inWidth, CCoord inHeight)
	{
		CRect::setSize(CPoint(inWidth, inHeight));
	}
	bool isInside(CCoord inX, CCoord inY)
	{
		return pointInside(CPoint(inX, inY));
	}
	bool isInside_local(CCoord inX, CCoord inY)
	{
		return pointInside(CPoint(inX + left, inY + top));
	}

#if 0 && TARGET_OS_MAC
	void copyToCGRect(CGRect * outDestRect, long inOutputPortHeight)
	{
		outDestRect->origin.x = left;
#ifdef FLIP_CG_COORDINATES
		outDestRect->origin.y = top;
#else
		if (inOutputPortHeight)
			outDestRect->origin.y = inOutputPortHeight - (getHeight() + top);
		else
			outDestRect->origin.y = top;
#endif
		outDestRect->size.width = getWidth();
		outDestRect->size.height = getHeight();
	}
	CGRect convertToCGRect(long inOutputPortHeight)
	{
		CGRect outputRect;
		copyToCGRect(&outputRect, inOutputPortHeight);
		return outputRect;
	}
#endif	// TARGET_OS_MAC
};



#ifdef TARGET_API_RTAS

USING_NAMESPACE_VSTGUI

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
class DGColor : public CColor
{
public:
	static DGColor const kBlack;
	static DGColor const kWhite;

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
		FocusIndicator
	};

	DGColor() = default;
	DGColor(int inRed, int inGreen, int inBlue, int inAlpha = 0xFF)
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
	DGColor(CColor const& inColor)
	{
		red = inColor.red;
		green = inColor.green;
		blue = inColor.blue;
		alpha = inColor.alpha;
	}

	DGColor withAlpha(int inAlpha) const;
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

	static ComponentType componentFromInt(int inValue)
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

	static float componentToFloat(ComponentType inValue)
	{
		return static_cast<float>(inValue) / static_cast<float>(kMaxValue);
	}
};



#if 0

#if TARGET_OS_MAC
	typedef CGContextRef TARGET_PLATFORM_GRAPHICS_CONTEXT;
#endif

//-----------------------------------------------------------------------------
// XXX replace with CDrawContext
class DGGraphicsContext
{
public:
	DGGraphicsContext(TARGET_PLATFORM_GRAPHICS_CONTEXT inContext);

	TARGET_PLATFORM_GRAPHICS_CONTEXT getPlatformGraphicsContext()
		{	return context;	}

	void setFont(const char * inFontName, float inFontSize);
	void drawText(DGRect * inRegion, const char * inText, dfx::TextAlignment inAlignment = kDGTextAlign_left);
#if TARGET_OS_MAC
	OSStatus drawCFText(DGRect * inRegion, CFStringRef inText, dfx::TextAlignment inAlignment);
#endif

#if TARGET_OS_MAC
	long getPortHeight()
		{	return portHeight;	}
	void setPortHeight(long inPortHeight)
		{	portHeight = inPortHeight;	}
	bool isCompositWindow()
		{	return (portHeight == 0) ? true : false;	}
	HIThemeOrientation getHIThemeOrientation()
	{
	#ifndef FLIP_CG_COORDINATES
		if (! isCompositWindow() )
			return kHIThemeOrientationInverted;
	#endif
		return kHIThemeOrientationNormal;
	}
#endif

	TARGET_PLATFORM_GRAPHICS_CONTEXT context;

private:
	float fontSize, fontAscent, fontDescent;
	bool isSnootPixel10;	// special Tom font

#if TARGET_OS_MAC
	long portHeight;
#endif
};
#endif



/***********************************************************************
	DGImage
	class for loading and containing images
***********************************************************************/

/* XXX should be "stacked" or "indexed" so that one bitmap might hold
   several clipped regions that can be drawn. (but we use overloading
   or default params so that it behaves like a single image when not
   using those features) */
//-----------------------------------------------------------------------------
class DGImage : public CBitmap
{
public:
	using CBitmap::CBitmap;

/*	probably a better interface is:
	void draw(CCoord x, CCoord y);
	and also something like this for stacked images:
	void drawex(CCoord x, CCoord y, int xIndex, int yIndex); 
*/
};



//-----------------------------------------------------------------------------
namespace dfx
{

static std::string const kInfinityUTF8(u8"\U0000221E");

SharedPointer<CFontDesc> CreateVstGuiFont(float inFontSize, const char* inFontName = nullptr);

std::string RemoveDigitSeparators(std::string const& inText);

}  // namespace dfx
