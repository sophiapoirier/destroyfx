#ifndef __DFXGUI_TOOLS_H
#define __DFXGUI_TOOLS_H


#include <Carbon/Carbon.h>

#include "dfxdefines.h"


//-----------------------------------------------------------------------------
typedef enum {
	kDGKeyModifier_accel = 1,	// command on Macs, control on PCs
	kDGKeyModifier_alt = 1 << 1,	// option on Macs, alt on PCs
	kDGKeyModifier_shift = 1 << 2,
	kDGKeyModifier_extra = 1 << 3,	// control on Macs
} DGKeyModifiers;


/***********************************************************************
	DGRect
	a rectangular region class
***********************************************************************/

//-----------------------------------------------------------------------------
class DGRect
{
public:
	DGRect()
		{	x = y = w = h = 0;	}
	DGRect(SInt32 inX, SInt32 inY, SInt32 inWidth, SInt32 inHeight)
		{	x = inX;	y = inY;	w = inWidth;	h = inHeight;	}

	void offset(SInt32 inOffsetX, SInt32 inOffsetY, SInt32 inWidthGrow = 0, SInt32 inHeightGrow = 0)
		{	x += inOffsetX;	y += inOffsetY;	w += inWidthGrow;	h += inHeightGrow;	}
	void moveTo(SInt32 inX, SInt32 inY)
		{	x = inX;	y = inY;	}
	void resize(SInt32 inWidth, SInt32 inHeight)
		{	w = inWidth;	h = inHeight;	}

	void set(DGRect *inSourceRect)
		{	x = inSourceRect->x;	y = inSourceRect->y;	w = inSourceRect->w;	h = inSourceRect->h;	}
	void set(SInt32 inX, SInt32 inY, SInt32 inWidth, SInt32 inHeight)
		{	x = inX;	y = inY;	w = inWidth;	h = inHeight;	}

	void copyToCGRect(CGRect *outDestRect, UInt32 inDestPortHeight)
	{
		outDestRect->origin.x = x;
		outDestRect->origin.y = inDestPortHeight - y - h;
		outDestRect->size.width = w;
		outDestRect->size.height = h;
	}
	CGRect convertToCGRect(UInt32 inOutputPortHeight)
	{
		CGRect outputRect;
		copyToCGRect(&outputRect, inOutputPortHeight);
		return outputRect;
	}
	void copyToRect(Rect *outDestRect)
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

	SInt32	x;
	SInt32	y;
	SInt32	w;
	SInt32	h;
};



//-----------------------------------------------------------------------------
// XXX use floats (0.0 to 1.0) instead of ints
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

	DGColor& operator = (DGColor newColor)
	{
		r = newColor.r;
		g = newColor.g;
		b = newColor.b;
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

/* XXX should be "stacked" or "indexed" so that one bitmap might hold
   several clipped regions that can be drawn. (but we use overloading
   or default params so that it behaves like a single image when not
   using those features) */

//-----------------------------------------------------------------------------
class DGImage
{
public:
	DGImage(const char *inFileName);
	virtual ~DGImage();

	// passive API (for controls that want to draw images by themselves)
	CGImageRef getCGImage()
		{	return cgImage;	}

	/* XXX int? */
	size_t getWidth();
	size_t getHeight();

	// active API (for more than images...)
	/* XXX float is not even used */
	/* probably a better type is
	   void draw(int x, int y);

	   .. and also something like
	   void drawex(int x, int y, int xindex, int yindex) 
	   .. for stacked images.
	*/
	virtual void draw(CGContextRef context, UInt32 portHeight, DGRect* rect, float value);

private:
	CGImageRef cgImage;
};


CGImageRef PreRenderCGImageBuffer(CGImageRef inCompressedImage);


#endif
