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
	DGTextDisplay(DfxGuiEditor*, AudioUnitParameterID, DGRect*, displayTextProcedure, void *userData, DGGraphic*, char *inFontName = NULL);
	virtual ~DGTextDisplay();

	virtual void draw(CGContextRef context, UInt32 portHeight);
	virtual void mouseDown(Point *P, bool, bool);
	virtual void mouseTrack(Point *P, bool, bool);
	virtual void mouseUp(Point *P, bool, bool);
	virtual void setTextAlignmentStyle(TextAlignmentStyle newStyle)
		{	alignment = newStyle;	}
	virtual TextAlignmentStyle getTextAlignmentStyle()
		{	return alignment;	}
	void setFontSize(float newSize)
		{	fontSize = newSize;	}
	void setFontColor(DGColor newColor)
		{	fontColor = newColor;	}
	
private:
	DGGraphic *				BackGround;
	displayTextProcedure	textProc;
	void *					textProcUserData;
	char *					fontName;
	float					fontSize;
	DGColor					fontColor;
	TextAlignmentStyle		alignment;
	SInt32					last_Y;
};


#endif