#ifndef __DFXGUI_DISPLAY_H
#define __DFXGUI_DISPLAY_H


#include "dfxgui.h"


typedef enum {
	kDGTextAlign_left = 0,
	kDGTextAlign_center,
	kDGTextAlign_right
} DfxGuiTextAlignment;


typedef void (*displayTextProcedure) (Float32 value, char * outText, void * userData);


//-----------------------------------------------------------------------------
class DGTextDisplay : public DGControl
{
public:
	DGTextDisplay(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion, 
					displayTextProcedure inTextProc, void * inUserData, DGImage * inBackground, 
					float inFontSize = 12.0f, DfxGuiTextAlignment inTextAlignment = kDGTextAlign_left, 
					DGColor inFontColor = kBlackDGColor, const char * inFontName = NULL);
	virtual ~DGTextDisplay();

	virtual void draw(CGContextRef context, UInt32 portHeight);
	virtual void drawText(CGContextRef context, CGRect& inBounds, const char * inString);

	void setTextAlignment(DfxGuiTextAlignment newAlignment)
		{	alignment = newAlignment;	}
	DfxGuiTextAlignment getTextAlignment()
		{	return alignment;	}
	void setFontSize(float newSize)
		{	fontSize = newSize;	}
	void setFontColor(DGColor newColor)
		{	fontColor = newColor;	}
	
protected:
	DGImage *				backgroundImage;
	displayTextProcedure	textProc;
	void *					textProcUserData;
	char *					fontName;
	float					fontSize;
	DfxGuiTextAlignment		alignment;
	DGColor					fontColor;
};


//-----------------------------------------------------------------------------
class DGStaticTextDisplay : public DGTextDisplay
{
public:
	DGStaticTextDisplay(DfxGuiEditor * inOwnerEditor, DGRect * inRegion, DGImage * inBackground, 
						float inFontSize = 12.0f, DfxGuiTextAlignment inTextAlignment = kDGTextAlign_left, 
						DGColor inFontColor = kBlackDGColor, const char * inFontName = NULL);
	virtual ~DGStaticTextDisplay();

	virtual void draw(CGContextRef context, UInt32 portHeight);

	void setText(const char * inNewText);

protected:
	char * displayString;
};


#endif