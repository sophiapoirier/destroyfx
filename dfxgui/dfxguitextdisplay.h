#ifndef __DFXGUI_DISPLAY_H
#define __DFXGUI_DISPLAY_H


#include "dfxgui.h"


typedef enum {
	kDGTextAlign_left = 0,
	kDGTextAlign_center,
	kDGTextAlign_right
} DfxGuiTextAlignmentStyle;


typedef void (*displayTextProcedure) (Float32 value, char *outText, void *userData);


//-----------------------------------------------------------------------------
class DGTextDisplay : public DGControl
{
public:
	DGTextDisplay(DfxGuiEditor*, AudioUnitParameterID, DGRect*, displayTextProcedure, void *inUserData, DGGraphic*, const char *inFontName = NULL);
	virtual ~DGTextDisplay();

	virtual void draw(CGContextRef context, UInt32 portHeight);
	virtual void drawText(CGContextRef context, CGRect& inBounds, const char *inString);
	virtual void mouseDown(Point inPos, bool, bool);
	virtual void mouseTrack(Point inPos, bool, bool);
	virtual void mouseUp(Point inPos, bool, bool);

	void setTextAlignmentStyle(DfxGuiTextAlignmentStyle newStyle)
		{	alignment = newStyle;	}
	DfxGuiTextAlignmentStyle getTextAlignmentStyle()
		{	return alignment;	}
	void setFontSize(float newSize)
		{	fontSize = newSize;	}
	void setFontColor(DGColor newColor)
		{	fontColor = newColor;	}
	
protected:
	DGGraphic *				BackGround;
	displayTextProcedure	textProc;
	void *					textProcUserData;
	char *					fontName;
	float					fontSize;
	DGColor					fontColor;
	DfxGuiTextAlignmentStyle	alignment;
	SInt32					last_Y;
};


//-----------------------------------------------------------------------------
class DGStaticTextDisplay : public DGTextDisplay
{
public:
	DGStaticTextDisplay(DfxGuiEditor*, DGRect*, DGGraphic*, const char *inFontName = NULL);
	virtual ~DGStaticTextDisplay();

	virtual void draw(CGContextRef context, UInt32 portHeight);
	virtual void mouseDown(Point *P, bool, bool) {}
	virtual void mouseTrack(Point *P, bool, bool) {}
	virtual void mouseUp(Point *P, bool, bool) {}

	virtual void setText(const char *inNewText);

protected:
	char *displayString;
};


#endif