/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2015  Sophia Poirier

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

#ifndef __DFXGUI_MISC_H
#define __DFXGUI_MISC_H


#include "dfxgui-base.h"

#include "vstgui.h"

#include "dfxdefines.h"

#if TARGET_OS_MAC
	#include <ApplicationServices/ApplicationServices.h>
#endif


#define FLIP_CG_COORDINATES


//-----------------------------------------------------------------------------
class DGRect : public CRect
{
public:
	DGRect()
	:	CRect() {}
	DGRect(CCoord inX, CCoord inY, CCoord inWidth, CCoord inHeight)
	:	CRect(inX, inY, inX + inWidth, inY + inHeight)
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
	bool isInside(CCoord inX, CCoord inY)
	{
		return pointInside( CPoint(inX, inY) );
	}
	bool isInside_zerobase(CCoord inX, CCoord inY)
	{
		return pointInside( CPoint(inX + left, inY + top) );
	}

/* XXX necessary?
	void offset(CCoord inOffsetX, CCoord inOffsetY, CCoord inWidthGrow, CCoord inHeightGrow)
	{
		offset(inOffsetX, inOffsetY);
		setWidth( getWidth() + inWidthGrow );
		setHeight( getHeight() + inHeightGrow );
	}
*/

#if TARGET_OS_MAC
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
	void copyToMacRect(Rect * outDestRect)
	{
		outDestRect->left = left;
		outDestRect->top = top;
		outDestRect->right = left + getWidth();
		outDestRect->bottom = top + getHeight();
	}
	Rect convertToMacRect()
	{
		Rect outputRect;
		copyToMacRect(&outputRect);
		return outputRect;
	}
#endif	// TARGET_OS_MAC
};



#ifdef TARGET_API_RTAS

#if WINDOWS_VERSION
	#define sRect RECT
#elif MAC_VERSION
	#define sRect Rect
#endif

USING_NAMESPACE_VSTGUI

//-----------------------------------------------------------------------------
enum {
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
	DGColor()
	{
		red = 0;
		green = 0;
		blue = 0;
		alpha = 0xFF;
	}
	DGColor(int inRed, int inGreen, int inBlue, int inAlpha = 0xFF)
	{
		red = static_cast<uint8_t>(inRed);
		green = static_cast<uint8_t>(inGreen);
		blue = static_cast<uint8_t>(inBlue);
		alpha = static_cast<uint8_t>(inAlpha);
	}
	DGColor(float inRed, float inGreen, float inBlue, float inAlpha = 1.0f)
	{
		const float fixedScalar = static_cast<float>(0xFF);
		red = static_cast<uint8_t>( lrintf(inRed * fixedScalar) );
		green = static_cast<uint8_t>( lrintf(inGreen * fixedScalar) );
		blue = static_cast<uint8_t>( lrintf(inBlue * fixedScalar) );
		alpha = static_cast<uint8_t>( lrintf(inAlpha * fixedScalar) );
	}
	DGColor(const CColor& inColor)
	{
		red = inColor.red;
		green = inColor.green;
		blue = inColor.blue;
		alpha = inColor.alpha;
	}
};

//-----------------------------------------------------------------------------
static const DGColor kDGColor_white(kWhiteCColor);
static const DGColor kDGColor_black(kBlackCColor);



#if TARGET_OS_MAC
	typedef CGContextRef TARGET_PLATFORM_GRAPHICS_CONTEXT;
#endif

#if 0
//-----------------------------------------------------------------------------
// XXX replace with CDrawContext
class DGGraphicsContext
{
public:
	DGGraphicsContext(TARGET_PLATFORM_GRAPHICS_CONTEXT inContext);

	TARGET_PLATFORM_GRAPHICS_CONTEXT getPlatformGraphicsContext()
		{	return context;	}

	void setAlpha(float inAlpha);
	void setAntialias(bool inShouldAntialias);
	void setAntialiasQuality(DGAntialiasQuality inQualityLevel);
	void setColor(DGColor inColor);
	void setFillColor(DGColor inColor);
	void setStrokeColor(DGColor inColor);
	void setLineWidth(float inLineWidth);

	void beginPath();
	void endPath();
	void fillPath();
	void strokePath();

	void fillRect(DGRect * inRect);
	void strokeRect(DGRect * inRect, float inLineWidth = -1.0f);

	void moveToPoint(float inX, float inY);
	void addLineToPoint(float inX, float inY);
	void strokeLine(float inLineWidth = -1.0f);
	void drawLine(float inStartX, float inStartY, float inEndX, float inEndY, float inLineWidth = -1.0f);

	void setFont(const char * inFontName, float inFontSize);
	void drawText(DGRect * inRegion, const char * inText, DGTextAlignment inAlignment = kDGTextAlign_left);
#if TARGET_OS_MAC
	OSStatus drawCFText(DGRect * inRegion, CFStringRef inText, DGTextAlignment inAlignment);
#endif

#if TARGET_OS_MAC
	long getPortHeight()
		{	return portHeight;	}
	void setPortHeight(long inPortHeight)
		{	portHeight = inPortHeight;	}
	bool isCompositWindow()
		{	return (portHeight == 0) ? true : false;	}
	void endQDContext(CGrafPtr inPort)
	{
		if ( (inPort != NULL) && (context != NULL) )
			QDEndCGContext(inPort, &context);
	}
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

class DfxGuiEditor;

/* XXX should be "stacked" or "indexed" so that one bitmap might hold
   several clipped regions that can be drawn. (but we use overloading
   or default params so that it behaves like a single image when not
   using those features) */
//-----------------------------------------------------------------------------
class DGImage : public CBitmap
{
public:
	DGImage(const char * inFileName, long inResourceID, DfxGuiEditor * inEditor = NULL);

	/* probably a better type is
	   void draw(int x, int y);

	   .. and also something like
	   void drawex(int x, int y, int xindex, int yindex) 
	   .. for stacked images.
	*/
/*	virtual void draw(DGRect * inRect, DGGraphicsContext * inContext, 
						long inXoffset = 0, long inYoffset = 0, 
						float inXscalar = 1.0f, float inYscalar = 1.0f);
*/
};



//-----------------------------------------------------------------------------
unsigned long DFXGUI_ConvertVstGuiMouseButtons(long inButtons);
DGKeyModifiers DFXGUI_ConvertVstGuiKeyModifiers(long inButtons);

CFontRef DFXGUI_CreateVstGuiFont(float inFontSize, const char* inFontName = NULL);



#endif
