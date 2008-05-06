#ifndef __DFXGUI_TOOLS_H
#define __DFXGUI_TOOLS_H


#include <Carbon/Carbon.h>

#include "dfxdefines.h"
#define FLIP_CG_COORDINATES


//-----------------------------------------------------------------------------
enum {
	kDGKeyModifier_accel = 1,	// command on Macs, control on PCs
	kDGKeyModifier_alt = 1 << 1,	// option on Macs, alt on PCs
	kDGKeyModifier_shift = 1 << 2,
	kDGKeyModifier_extra = 1 << 3	// control on Macs
};
typedef unsigned long	DGKeyModifiers;

typedef enum {
	kDGMouseWheelAxis_vertical,
	kDGMouseWheelAxis_horizontal
} DGMouseWheelAxis;

typedef enum {
	kDGAntialiasQuality_default,
	kDGAntialiasQuality_none,
	kDGAntialiasQuality_low,
	kDGAntialiasQuality_high
} DGAntialiasQuality;

typedef enum {
	kDGTextAlign_left = 0,
	kDGTextAlign_center,
	kDGTextAlign_right
} DGTextAlignment;


//-----------------------------------------------------------------------------
// a rectangular region defined with horizontal position (x), vertical position (y), width, and height
struct DGRect
{
	long x;	// horizontal placement
	long y;	// vertical placement
	long w;	// width
	long h;	// height

	DGRect()
	{
		x = y = w = h = 0;
	}
	DGRect(long inX, long inY, long inWidth, long inHeight)
	{
		set(inX, inY, inWidth, inHeight);
	}
	DGRect(DGRect * inSourceRect)
	{
		set(inSourceRect);
	}

	void set(DGRect * inSourceRect)
	{
		set(inSourceRect->x, inSourceRect->y, inSourceRect->w, inSourceRect->h);
	}
	void set(long inX, long inY, long inWidth, long inHeight)
	{
		x = inX;
		y = inY;
		w = inWidth;
		h = inHeight;
	}

	void offset(long inOffsetX, long inOffsetY, long inWidthGrow = 0, long inHeightGrow = 0)
	{
		x += inOffsetX;
		y += inOffsetY;
		w += inWidthGrow;
		h += inHeightGrow;
	}
	void moveTo(long inX, long inY)
	{
		x = inX;
		y = inY;
	}
	void resize(long inWidth, long inHeight)
	{
		w = inWidth;
		h = inHeight;
	}

	bool isInside(long inX, long inY)
	{
		return ( ((inX >= x) && (inX < (x + w))) && ((inY >= y) && (inY < (y + h))) );
	}
	bool isInside_zerobase(long inX, long inY)
	{
		return ( ((inX >= 0) && (inX < w)) && ((inY >= 0) && (inY < h)) );
	}

	DGRect& operator = (const DGRect inRect)
	{
		set(inRect.x, inRect.y, inRect.w, inRect.h);
		return *this;
	}
	bool operator != (const DGRect& otherRect) const
	{
		return ( (x != otherRect.x) || (y != otherRect.y) || (w != otherRect.w) || (h != otherRect.h) );
	}
	bool operator == (const DGRect& otherRect) const
	{
		return ( (x == otherRect.x) && (y == otherRect.y) && (w == otherRect.w) && (h == otherRect.h) );
	}

#if TARGET_OS_MAC
	void copyToCGRect(CGRect * outDestRect, long inDestPortHeight)
	{
		outDestRect->origin.x = x;
#ifdef FLIP_CG_COORDINATES
		outDestRect->origin.y = y;
#else
		if (inDestPortHeight)
			outDestRect->origin.y = inDestPortHeight - (h + y);
		else
			outDestRect->origin.y = y;
#endif
		outDestRect->size.width = w;
		outDestRect->size.height = h;
	}
	CGRect convertToCGRect(long inDestPortHeight)
	{
		CGRect outputRect;
		copyToCGRect(&outputRect, inDestPortHeight);
		return outputRect;
	}
	void copyToMacRect(Rect * outDestRect)
	{
		outDestRect->left = x;
		outDestRect->top = y;
		outDestRect->right = x + w;
		outDestRect->bottom = y + h;
	}
	Rect convertToMacRect()
	{
		Rect outputRect;
		copyToMacRect(&outputRect);
		return outputRect;
	}
#endif
};



//-----------------------------------------------------------------------------
// 3-component RGB color + alpha represented with a float (range 0.0 to 1.0) 
// for each color component
struct DGColor
{
	float r;
	float g;
	float b;
	float a;

	DGColor()
	:	r(0.0f), g(0.0f), b(0.0f), a(1.0f) {}
	DGColor(float inRed, float inGreen, float inBlue, float inAlpha = 1.0f)
	:	r(inRed), g(inGreen), b(inBlue), a(inAlpha) {}
	DGColor(const DGColor& inColor)
	:	r(inColor.r), g(inColor.g), b(inColor.b), a(inColor.a) {}
	DGColor& operator () (float inRed, float inGreen, float inBlue, float inAlpha = 1.0f)
	{
		r = inRed;
		g = inGreen;
		b = inBlue;
		a = inAlpha;
		return *this;
	}

	DGColor& operator = (const DGColor inColor)
	{
		r = inColor.r;
		g = inColor.g;
		b = inColor.b;
		a = inColor.a;
		return *this;
	}
};
typedef struct DGColor DGColor;

const DGColor kDGColor_black(0.0f, 0.0f, 0.0f, 1.0f);
const DGColor kDGColor_white(1.0f, 1.0f, 1.0f, 1.0f);



#if TARGET_OS_MAC
	typedef CGContextRef TARGET_PLATFORM_GRAPHICS_CONTEXT;
#endif

//-----------------------------------------------------------------------------
// 
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
class DGImage
{
public:
	DGImage(const char * inFileName, DfxGuiEditor * inEditor = NULL);
	virtual ~DGImage();

	long getWidth();
	long getHeight();

	/* probably a better type is
	   void draw(int x, int y);

	   .. and also something like
	   void drawex(int x, int y, int xindex, int yindex) 
	   .. for stacked images.
	*/
	virtual void draw(DGRect * inRect, DGGraphicsContext * inContext, long inXoffset = 0, long inYoffset = 0);

#if TARGET_OS_MAC
	CGImageRef getCGImage()
		{	return cgImage;	}
#endif

private:
#if TARGET_OS_MAC
	CGImageRef cgImage;
#endif
};

CGImageRef PreRenderCGImageBuffer(CGImageRef inCompressedImage, bool inFlipImage);


#endif
