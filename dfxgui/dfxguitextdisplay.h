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
	kDGTextDisplayMouseAxis_vertical = 1 << 1,
} DfxGuiTextDisplayMouseAxis;


typedef void (*displayTextProcedure) (float value, char * outText, void * userData);


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

	virtual void mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, unsigned long inKeyModifiers);
	virtual void mouseTrack(float inXpos, float inYpos, unsigned long inMouseButtons, unsigned long inKeyModifiers);

	void setTextAlignment(DfxGuiTextAlignment newAlignment)
		{	alignment = newAlignment;	}
	DfxGuiTextAlignment getTextAlignment()
		{	return alignment;	}
	void setFontSize(float newSize)
		{	fontSize = newSize;	}
	void setFontColor(DGColor newColor)
		{	fontColor = newColor;	}
	void setMouseDragRange(float inMouseDragRange)
	{
		if (inMouseDragRange != 0.0f)	// to prevent division by zero
			mouseDragRange = inMouseDragRange;
	}

protected:
	DGImage *				backgroundImage;
	displayTextProcedure	textProc;
	void *					textProcUserData;
	DfxGuiTextAlignment		alignment;
	float					fontSize;
	DGColor					fontColor;
	char *					fontName;
	DfxGuiTextDisplayMouseAxis	mouseAxis;	// flags indicating which directions you can mouse to adjust the control value
	float					mouseDragRange;	// the range of pixels over which you can drag the mouse to adjust the control value
	float					lastX, lastY;
};


//-----------------------------------------------------------------------------
class DGStaticTextDisplay : public DGTextDisplay
{
public:
	DGStaticTextDisplay(DfxGuiEditor * inOwnerEditor, DGRect * inRegion, DGImage * inBackground, 
						DfxGuiTextAlignment inTextAlignment = kDGTextAlign_left, float inFontSize = 12.0f, 
						DGColor inFontColor = kBlackDGColor, const char * inFontName = NULL);
	virtual ~DGStaticTextDisplay();

	virtual void draw(CGContextRef inContext, long inPortHeight);

	virtual void mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, unsigned long inKeyModifiers)
		{ }
	virtual void mouseTrack(float inXpos, float inYpos, unsigned long inMouseButtons, unsigned long inKeyModifiers)
		{ }
	virtual void mouseUp(float inXpos, float inYpos, unsigned long inKeyModifiers)
		{ }

	void setText(const char * inNewText);

protected:
	char * displayString;
};


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

	virtual void mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, unsigned long inKeyModifiers)
		{ }
	virtual void mouseTrack(float inXpos, float inYpos, unsigned long inMouseButtons, unsigned long inKeyModifiers)
		{ }
	virtual void mouseUp(float inXpos, float inYpos, unsigned long inKeyModifiers)
		{ }

	void setText(long inStringNum, const char * inNewText);

protected:
	long numStrings;
	char ** displayStrings;
};


#endif