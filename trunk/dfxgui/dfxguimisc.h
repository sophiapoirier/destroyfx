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

#if MAC
	void copyToCGRect(CGRect * outDestRect, long inDestPortHeight)
	{
		outDestRect->origin.x = x;
#ifdef FLIP_CG_COORDINATES
		outDestRect->origin.y = y;
#else
		outDestRect->origin.y = inDestPortHeight - (h + y);
#endif
		outDestRect->size.width = w;
		outDestRect->size.height = h;
	}
	CGRect convertToCGRect(long inOutputPortHeight)
	{
		CGRect outputRect;
		copyToCGRect(&outputRect, inOutputPortHeight);
		return outputRect;
	}
	void copyToRect(Rect * outDestRect)
	{
		outDestRect->left = x;
		outDestRect->top = y;
		outDestRect->right = x + w;
		outDestRect->bottom = y + h;
	}
	Rect convertToRect()
	{
		Rect outputRect;
		copyToRect(&outputRect);
		return outputRect;
	}
#endif
};



//-----------------------------------------------------------------------------
// 3-component RGB color represented with a float (range 0.0 to 1.0) 
// for each color component
struct DGColor
{
	float r;
	float g;
	float b;

	DGColor()
	:	r(0.0f), g(0.0f), b(0.0f) {}
	DGColor(float inRed, float inGreen, float inBlue)
	:	r(inRed), g(inGreen), b(inBlue) {}
	DGColor(const DGColor& inColor)
	:	r(inColor.r), g(inColor.g), b(inColor.b) {}
	DGColor& operator () (float inRed, float inGreen, float inBlue)
	{
		r = inRed;
		g = inGreen;
		b = inBlue;
		return *this;
	}

	DGColor& operator = (const DGColor inColor)
	{
		r = inColor.r;
		g = inColor.g;
		b = inColor.b;
		return *this;
	}
};
typedef struct DGColor DGColor;

const DGColor kBlackDGColor(0.0f, 0.0f, 0.0f);
const DGColor kWhiteDGColor(1.0f, 1.0f, 1.0f);



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
	virtual void draw(DGRect * inRect, CGContextRef inContext, long inPortHeight, long inXoffset = 0, long inYoffset = 0);

#if MAC
	CGImageRef getCGImage()
		{	return cgImage;	}
#endif

private:
#if MAC
	CGImageRef cgImage;
#endif
};

CGImageRef PreRenderCGImageBuffer(CGImageRef inCompressedImage);


#endif