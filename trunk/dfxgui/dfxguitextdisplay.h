#ifndef __DFXGUI_DISPLAY_H
#define __DFXGUI_DISPLAY_H


#include "dfxgui.h"


enum TextAlignmentStyle {
	kTextAlign_left = 0,
	kTextAlign_center,
	kTextAlign_right
};


typedef void (*displayTextProcedure) (Float32 value, char *outText, void *userData);


//-----------------------------------------------------------------------------
class DGTextDisplay : public DGControl
{
public:
	DGTextDisplay(DfxGuiEditor*, AudioUnitParameterID, DGRect*, displayTextProcedure, void *userData, DGGraphic*, const char *inFontName = NULL);
	virtual ~DGTextDisplay();

	virtual void draw(CGContextRef context, UInt32 portHeight);
	virtual void drawText(CGContextRef context, CGRect& inBounds, const char *inString);
	virtual void mouseDown(Point inPos, bool, bool);
	virtual void mouseTrack(Point inPos, bool, bool);
	virtual void mouseUp(Point inPos, bool, bool);

	void setTextAlignmentStyle(TextAlignmentStyle newStyle)
		{	alignment = newStyle;	}
	TextAlignmentStyle getTextAlignmentStyle()
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
	TextAlignmentStyle		alignment;
	SInt32					last_Y;
};


//-----------------------------------------------------------------------------
class DGStaticTextDisplay : public DGTextDisplay
{
public:
	DGStaticTextDisplay(DfxGuiEditor*, DGRect*, DGGraphic*, char *inFontName = NULL);
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