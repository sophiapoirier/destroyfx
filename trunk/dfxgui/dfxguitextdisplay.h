/*------------------------------------------------------------------------
Destroy FX Library (version 1.0) is a collection of foundation code 
for creating audio software plug-ins.  
Copyright (C) 2002-2009  Sophia Poirier

This program is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

This program is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with this program.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, please visit http://destroyfx.org/ 
and use the contact form.
------------------------------------------------------------------------*/

#ifndef __DFXGUI_TEXT_DISPLAY_H
#define __DFXGUI_TEXT_DISPLAY_H


#include "dfxguieditor.h"


const size_t kDGTextDisplay_stringSize = 256;


typedef void (*DGValue2TextProcedure) (float inValue, char * outText, void * inUserData);


#pragma mark -
//-----------------------------------------------------------------------------
class DGTextDisplay : public DGControl
{
public:
	DGTextDisplay(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion, 
					DGValue2TextProcedure inTextProc, void * inUserData, DGImage * inBackground, 
					DGTextAlignment inTextAlignment = kDGTextAlign_left, float inFontSize = 12.0f, 
					DGColor inFontColor = kDGColor_black, const char * inFontName = NULL);
	virtual ~DGTextDisplay();

	virtual void draw(DGGraphicsContext * inContext);
	void drawText(DGRect * inRegion, const char * inText, DGGraphicsContext * inContext);
#if TARGET_OS_MAC
	OSStatus drawCFText(DGRect * inRegion, CFStringRef inText, DGGraphicsContext * inContext);
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
	void setMouseAxis(DGAxis inMouseAxis)
		{	mouseAxis = inMouseAxis;	}

protected:
	DGImage *				backgroundImage;
	DGValue2TextProcedure	textProc;
	void *					textProcUserData;

	DGTextAlignment			alignment;
	float					fontSize;
	DGColor					fontColor;
	char *					fontName;
	bool					shouldAntiAlias;

	DGAxis					mouseAxis;	// flags indicating which directions you can mouse to adjust the control value
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
