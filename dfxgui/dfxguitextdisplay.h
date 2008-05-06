#ifndef __DFXGUI_DISPLAY_H
#define __DFXGUI_DISPLAY_H


#include "dfxgui.h"


typedef enum {
	kDGTextDisplayMouseAxis_horizontal = 1,
	kDGTextDisplayMouseAxis_vertical = 1 << 1,
	kDGTextDisplayMouseAxis_omni = (kDGTextDisplayMouseAxis_horizontal | kDGTextDisplayMouseAxis_vertical)
} DfxGuiTextDisplayMouseAxis;

const size_t kDGTextDisplay_stringSize = 256;


typedef void (*displayTextProcedure) (float inValue, char * outText, void * inUserData);


#pragma mark -
//-----------------------------------------------------------------------------
class DGTextDisplay : public DGControl
{
public:
	DGTextDisplay(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion, 
					displayTextProcedure inTextProc, void * inUserData, DGImage * inBackground, 
					DGTextAlignment inTextAlignment = kDGTextAlign_left, float inFontSize = 12.0f, 
					DGColor inFontColor = kDGColor_black, const char * inFontName = NULL);
	virtual ~DGTextDisplay();

	virtual void draw(DGGraphicsContext * inContext);
	void drawText(DGRect * inRegion, const char * inText, DGGraphicsContext * inContext);
#if TARGET_OS_MAC
	OSStatus drawCFText(DGRect * inRegion, const CFStringRef inText, DGGraphicsContext * inContext);
#endif

	virtual void mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers, bool inIsDoubleClick);
	virtual void mouseTrack(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers);

	void setTextAlignment(DGTextAlignment newAlignment)
		{	alignment = newAlignment;	}
	DGTextAlignment getTextAlignment()
		{	return alignment;	}
	void setFontSize(float newSize)
		{	fontSize = newSize;	}
	void setFontColor(DGColor newColor)
		{	fontColor = newColor;	}
	void setAntiAliasing(bool inAntiAlias)
		{	shouldAntiAlias = inAntiAlias;	}
	void setMouseAxis(DfxGuiTextDisplayMouseAxis inMouseAxis)
		{	mouseAxis = inMouseAxis;	}

protected:
	DGImage *				backgroundImage;
	displayTextProcedure	textProc;
	void *					textProcUserData;

	DGTextAlignment			alignment;
	float					fontSize;
	DGColor					fontColor;
	char *					fontName;
	bool					shouldAntiAlias;

	DfxGuiTextDisplayMouseAxis	mouseAxis;	// flags indicating which directions you can mouse to adjust the control value
	float					lastX, lastY;
};



#pragma mark -
//-----------------------------------------------------------------------------
class DGStaticTextDisplay : public DGTextDisplay
{
public:
	DGStaticTextDisplay(DfxGuiEditor * inOwnerEditor, DGRect * inRegion, DGImage * inBackground, 
						DGTextAlignment inTextAlignment = kDGTextAlign_left, float inFontSize = 12.0f, 
						DGColor inFontColor = kDGColor_black, const char * inFontName = NULL);
	virtual ~DGStaticTextDisplay();

	virtual void draw(DGGraphicsContext * inContext);

	void setText(const char * inNewText);
#if TARGET_OS_MAC
	void setCFText(CFStringRef inNewText);
#endif

protected:
	char * displayString;
#if TARGET_OS_MAC
	CFStringRef displayCFString;
#endif
};



#pragma mark -
//-----------------------------------------------------------------------------
class DGTextArrayDisplay : public DGTextDisplay
{
public:
	DGTextArrayDisplay(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion, long inNumStrings, 
						DGTextAlignment inTextAlignment = kDGTextAlign_left, DGImage * inBackground = NULL, 
						float inFontSize = 12.0f, DGColor inFontColor = kDGColor_black, const char * inFontName = NULL);
	virtual ~DGTextArrayDisplay();
	virtual void post_embed();

	virtual void draw(DGGraphicsContext * inContext);

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

	virtual void draw(DGGraphicsContext * inContext);

protected:
	DGImage * animationImage;
	long numAnimationFrames;
};



#endif
