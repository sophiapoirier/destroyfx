#ifndef __DFXGUI_DISPLAY_H
#define __DFXGUI_DISPLAY_H


#include "dfxgui.h"


typedef enum {
	kDGTextAlign_left = 0,
	kDGTextAlign_center,
	kDGTextAlign_right
} DfxGuiTextAlignment;

typedef enum {
	kDGTextDisplayMouseAxis_horizontal = 1,
	kDGTextDisplayMouseAxis_vertical = 1 << 1
} DfxGuiTextDisplayMouseAxis;


typedef void (*displayTextProcedure) (float inValue, char * outText, void * inUserData);


#pragma mark -
//-----------------------------------------------------------------------------
class DGTextDisplay : public DGControl
{
public:
	DGTextDisplay(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion, 
					displayTextProcedure inTextProc, void * inUserData, DGImage * inBackground, 
					DfxGuiTextAlignment inTextAlignment = kDGTextAlign_left, float inFontSize = 12.0f, 
					DGColor inFontColor = kBlackDGColor, const char * inFontName = NULL);
	virtual ~DGTextDisplay();

	virtual void draw(CGContextRef inContext, long inPortHeight);
	void drawText(DGRect * inRegion, const char * inText, CGContextRef inContext, long inPortHeight);
#if MAC
	OSStatus drawCFText(DGRect * inRegion, const CFStringRef inText, CGContextRef inContext, long inPortHeight);
#endif

	virtual void mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers);
	virtual void mouseTrack(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers);

	void setTextAlignment(DfxGuiTextAlignment newAlignment)
		{	alignment = newAlignment;	}
	DfxGuiTextAlignment getTextAlignment()
		{	return alignment;	}
	void setFontSize(float newSize)
		{	fontSize = newSize;	}
	void setFontColor(DGColor newColor)
		{	fontColor = newColor;	}
	void setAntiAliasing(bool inAntiAlias)
		{	shouldAntiAlias = inAntiAlias;	}

protected:
	DGImage *				backgroundImage;
	displayTextProcedure	textProc;
	void *					textProcUserData;

	DfxGuiTextAlignment		alignment;
	float					fontSize;
	DGColor					fontColor;
	char *					fontName;
	bool					shouldAntiAlias;
	bool					isSnootPixel10;	// special Tom font
	float					fontAscent, fontDescent;

	DfxGuiTextDisplayMouseAxis	mouseAxis;	// flags indicating which directions you can mouse to adjust the control value
	float					lastX, lastY;
};



#pragma mark -
//-----------------------------------------------------------------------------
class DGStaticTextDisplay : public DGTextDisplay
{
public:
	DGStaticTextDisplay(DfxGuiEditor * inOwnerEditor, DGRect * inRegion, DGImage * inBackground, 
						DfxGuiTextAlignment inTextAlignment = kDGTextAlign_left, float inFontSize = 12.0f, 
						DGColor inFontColor = kBlackDGColor, const char * inFontName = NULL);
	virtual ~DGStaticTextDisplay();

	virtual void draw(CGContextRef inContext, long inPortHeight);

	void setText(const char * inNewText);
#if MAC
	void setCFText(CFStringRef inNewText);
#endif

protected:
	char * displayString;
#if MAC
	CFStringRef displayCFString;
#endif
};



#pragma mark -
//-----------------------------------------------------------------------------
class DGTextArrayDisplay : public DGTextDisplay
{
public:
	DGTextArrayDisplay(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion, long inNumStrings, 
						DfxGuiTextAlignment inTextAlignment = kDGTextAlign_left, DGImage * inBackground = NULL, 
						float inFontSize = 12.0f, DGColor inFontColor = kBlackDGColor, const char * inFontName = NULL);
	virtual ~DGTextArrayDisplay();
	virtual void post_embed();

	virtual void draw(CGContextRef inContext, long inPortHeight);

	void setText(long inStringNum, const char * inNewText);

protected:
	long numStrings;
	char ** displayStrings;
};



#pragma mark -
//-----------------------------------------------------------------------------
class DGAnimation : public DGTextDisplay
{
public:
	DGAnimation(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion, 
				DGImage * inAnimationImage, long inNumAnimationFrames, DGImage * inBackground = NULL);

	virtual void draw(CGContextRef inContext, long inPortHeight);

protected:
	DGImage * animationImage;
	long numAnimationFrames;
};



#endif
