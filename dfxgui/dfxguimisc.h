#ifndef __DFXGUI_TOOLS_H
#define __DFXGUI_TOOLS_H


#include <Carbon/Carbon.h>

#include "dfxdefines.h"


//-----------------------------------------------------------------------------
typedef enum {
	kDfxGuiType_none = 0,
	kDfxGuiType_graphic,
	kDfxGuiType_slider,
	kDfxGuiType_button,
	kDfxGuiType_display
} DfxGuiType;



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
struct DGColor {
	int r;
	int g;
	int b;

	DGColor()
	:	r(0), g(0), b(0) {}
	DGColor(int inRed, int inGreen, int inBlue)
	:	r(inRed), g(inGreen), b(inBlue) {}
	DGColor(const DGColor& inColor)
	:	r(inColor.r), g(inColor.g), b(inColor.b) {}
	DGColor& operator () (int inRed, int inGreen, int inBlue)
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

const DGColor kBlackDGColor(0, 0, 0);
const DGColor kWhiteDGColor(255, 255, 255);



/***********************************************************************
	Destructible
	.. has a destroy () method to free itself.
***********************************************************************/

//-----------------------------------------------------------------------------

class Destructible {
 public:
  virtual void destroy () {};
}



/***********************************************************************
	DGGraphic
	class for loading and containing images
***********************************************************************/

//-----------------------------------------------------------------------------
class DGGraphic : public Destructible
{
public:
	DGGraphic(const char *inFileName);
	virtual ~DGGraphic();
	
	virtual destroy() { delete this; }

	// passive API (for controls that want to draw images by themselves)
	CGImageRef getCGImage()
		{	return cgImage;	}
	size_t getWidth();
	size_t getHeight();

	// active API (for more than images...)
	virtual void draw(CGContextRef context, UInt32 portHeight, DGRect* rect, float value);

private:
	void loadImageFile(const char *inFileName);

	CGImageRef cgImage;
};


#endif
