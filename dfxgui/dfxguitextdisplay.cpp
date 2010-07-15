/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2010  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Destroy FX Library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Destroy FX Library.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#if !__LP64__

#include "dfxguitextdisplay.h"


//-----------------------------------------------------------------------------
static void DFXGUI_GenericValue2TextProc(float inValue, char * outText, void * inUserData);
static void DFXGUI_GenericValue2TextProc(float inValue, char * outText, void * inUserData)
{
	if (outText != NULL)
		sprintf(outText, "%.2f", inValue);
}



#pragma mark -
#pragma mark DGTextDisplay

//-----------------------------------------------------------------------------
// Text Display
//-----------------------------------------------------------------------------
DGTextDisplay::DGTextDisplay(DfxGuiEditor *			inOwnerEditor,
							long					inParamID, 
							DGRect *				inRegion,
							DGValue2TextProcedure	inTextProc, 
							void *					inUserData,
							DGImage *				inBackgroundImage, 
							DGTextAlignment			inTextAlignment, 
							float					inFontSize, 
							DGColor					inFontColor, 
							const char *			inFontName)
:	DGControl(inOwnerEditor, inParamID, inRegion), 
	backgroundImage(inBackgroundImage), alignment(inTextAlignment), fontSize(inFontSize), fontColor(inFontColor)
{
	if (inTextProc == NULL)
		textProc = DFXGUI_GenericValue2TextProc;
	else
		textProc = inTextProc;
	if (inUserData == NULL)
		textProcUserData = this;
	else
		textProcUserData = inUserData;

	fontName = NULL;
	if (inFontName != NULL)
	{
		fontName = (char*) malloc(strlen(inFontName) + 1);
		strcpy(fontName, inFontName);
	}
	else
	{
		// get the application font from the system "theme"
		Str255 appfontname;
		OSStatus themeErr = GetThemeFont(kThemeApplicationFont, smSystemScript, appfontname, NULL, NULL);
		if (themeErr == noErr)
		{
			fontName = (char*) malloc(sizeof(appfontname));
			// Pascal-string to C-string conversion
			memcpy(fontName, &(appfontname[1]), appfontname[0]);
			fontName[appfontname[0]] = 0;
		}
	}

	mouseAxis = kDGAxis_vertical;
	shouldAntiAlias = true;

	setControlContinuous(true);
}

//-----------------------------------------------------------------------------
DGTextDisplay::~DGTextDisplay()
{
	backgroundImage = NULL;
	textProc = NULL;
	textProcUserData = NULL;
	if (fontName != NULL)
		free(fontName);
	fontName = NULL;
}


//-----------------------------------------------------------------------------
void DGTextDisplay::draw(DGGraphicsContext * inContext)
{
	if (backgroundImage == NULL)
	{
// XXX hmmm, I need to do something else to check on this; we may just want to draw on top of the background
#if 0
		inContext->setFillColor( DGColor(59.0f/255.0f, 83.0f/255.0f, 165.0f/255.0f) );
		inContext->fillRect( getBounds() );
#endif
	}
	else
//		backgroundImage->draw(getBounds(), inContext, getBounds()->x - (long)(getDfxGuiEditor()->GetXOffset()), getBounds()->y - (long)(getDfxGuiEditor()->GetYOffset()));	// draw underneath-style
		backgroundImage->draw(getBounds(), inContext);

	if (textProc != NULL)
	{
		char text[kDGTextDisplay_stringSize];
		text[0] = 0;
		textProc(getAUVP().GetValue(), text, textProcUserData);
		drawText(getBounds(), text, inContext);
	}
}

//-----------------------------------------------------------------------------
void DGTextDisplay::drawText(DGRect * inRegion, const char * inText, DGGraphicsContext * inContext)
{
	if ( (inText == NULL) || (inRegion == NULL) )
		return;

	inContext->setFont(fontName, fontSize);
	inContext->setColor(fontColor);
	inContext->setAntialias(shouldAntiAlias);

	inContext->drawText(inRegion, inText, alignment);
}

#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
OSStatus DGTextDisplay::drawCFText(DGRect * inRegion, CFStringRef inText, DGGraphicsContext * inContext)
{
	if ( (inText == NULL) || (inRegion == NULL) )
		return paramErr;

	inContext->setFont(fontName, fontSize);
	inContext->setColor(fontColor);
	inContext->setAntialias(shouldAntiAlias);

	return inContext->drawCFText(inRegion, inText, alignment);
}
#endif

//-----------------------------------------------------------------------------
void DGTextDisplay::mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers, bool inIsDoubleClick)
{
	lastX = inXpos;
	lastY = inYpos;
}

//-----------------------------------------------------------------------------
void DGTextDisplay::mouseTrack(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers)
{
	SInt32 min = GetControl32BitMinimum(carbonControl);
	SInt32 max = GetControl32BitMaximum(carbonControl);
	SInt32 val = GetControl32BitValue(carbonControl);
	SInt32 oldval = val;

	float diff = 0.0f;
	if (mouseAxis & kDGAxis_horizontal)
		diff += inXpos - lastX;
	if (mouseAxis & kDGAxis_vertical)
		diff += lastY - inYpos;
	if (inKeyModifiers & kDGKeyModifier_shift)	// slo-mo
		diff /= getFineTuneFactor();
	val += (SInt32) (diff * (float)(max-min) / getMouseDragRange());

	if (val > max)
		val = max;
	if (val < min)
		val = min;
	if (val != oldval)
		SetControl32BitValue(carbonControl, val);

	lastX = inXpos;
	lastY = inYpos;
}






#pragma mark -
#pragma mark DGStaticTextDisplay

//-----------------------------------------------------------------------------
DGStaticTextDisplay::DGStaticTextDisplay(DfxGuiEditor * inOwnerEditor, DGRect * inRegion, DGImage * inBackground, 
										DGTextAlignment inTextAlignment, float inFontSize, 
										DGColor inFontColor, const char * inFontName)
:	DGTextDisplay(inOwnerEditor, DFX_PARAM_INVALID_ID, inRegion, NULL, NULL, inBackground, 
					inTextAlignment, inFontSize, inFontColor, inFontName), 
	displayString(NULL)
{
	displayString = (char*) malloc(kDGTextDisplay_stringSize);
	displayString[0] = 0;
	parameterAttached = false;	// XXX good enough?
#if TARGET_OS_MAC
	displayCFString = NULL;
#endif
	setRespondToMouse(false);
}

//-----------------------------------------------------------------------------
DGStaticTextDisplay::~DGStaticTextDisplay()
{
	if (displayString != NULL)
		free(displayString);
	displayString = NULL;

#if TARGET_OS_MAC
	if (displayCFString != NULL)
		CFRelease(displayCFString);
	displayCFString = NULL;
#endif
}

//-----------------------------------------------------------------------------
void DGStaticTextDisplay::setText(const char * inNewText)
{
	if (inNewText == NULL)
		return;

#if TARGET_OS_MAC
	if (displayCFString != NULL)
		CFRelease(displayCFString);
	displayCFString = NULL;
#endif

	strcpy(displayString, inNewText);
	redraw();
}

#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
void DGStaticTextDisplay::setCFText(CFStringRef inNewText)
{
	if (inNewText == NULL)
		return;

	if (displayCFString != NULL)
		CFRelease(displayCFString);
	displayCFString = inNewText;
	CFRetain(displayCFString);

	Boolean success = CFStringGetCString(inNewText, displayString, kDGTextDisplay_stringSize, kCFStringEncodingUTF8);
	if (success)
		redraw();
}
#endif

//-----------------------------------------------------------------------------
void DGStaticTextDisplay::draw(DGGraphicsContext * inContext)
{
	if (backgroundImage == NULL)
	{
// XXX hmmm, I need to do something else to check on this; we may just want to draw on top of the background
#if 0
		inContext->setFillColor( DGColor(59.0f/255.0f, 83.0f/255.0f, 165.0f/255.0f) );
		inContext->fillRect( getBounds() );
#endif
	}
	else
//		backgroundImage->draw(getBounds(), inContext, getBounds()->x - (long)(getDfxGuiEditor()->GetXOffset()), getBounds()->y - (long)(getDfxGuiEditor()->GetYOffset()));	// draw underneath-style
		backgroundImage->draw(getBounds(), inContext);

#if TARGET_OS_MAC
	if (displayCFString != NULL)
		drawCFText(getBounds(), displayCFString, inContext);
	else
#endif
	drawText(getBounds(), displayString, inContext);
}






#pragma mark -
#pragma mark DGTextArrayDisplay

//-----------------------------------------------------------------------------
// Static Text Display
//-----------------------------------------------------------------------------
DGTextArrayDisplay::DGTextArrayDisplay(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion, 
						long inNumStrings, DGTextAlignment inTextAlignment, DGImage * inBackground, 
						float inFontSize, DGColor inFontColor, const char * inFontName)
:	DGTextDisplay(inOwnerEditor, inParamID, inRegion, NULL, NULL, inBackground, 
					inTextAlignment, inFontSize, inFontColor, inFontName), 
	numStrings(inNumStrings), 
	displayStrings(NULL)
{
	if (numStrings <= 0)
		numStrings = 1;
	displayStrings = (char**) malloc(numStrings * sizeof(char*));
	for (long i=0; i < numStrings; i++)
	{
		displayStrings[i] = (char*) malloc(kDGTextDisplay_stringSize);
		displayStrings[i][0] = 0;
	}

	setControlContinuous(false);
	setRespondToMouse(false);
}

//-----------------------------------------------------------------------------
DGTextArrayDisplay::~DGTextArrayDisplay()
{
	if (displayStrings != NULL)
	{
		for (long i=0; i < numStrings; i++)
		{
			if (displayStrings[i] != NULL)
				free(displayStrings[i]);
			displayStrings[i] = NULL;
		}
		free(displayStrings);
	}
	displayStrings = NULL;
}

//-----------------------------------------------------------------------------
void DGTextArrayDisplay::post_embed()
{
	if ( isParameterAttached() && (carbonControl != NULL) )
		SetControl32BitMaximum( carbonControl, numStrings - 1 + GetControl32BitMinimum(carbonControl) );
}

//-----------------------------------------------------------------------------
void DGTextArrayDisplay::setText(long inStringNum, const char * inNewText)
{
	if ( (inStringNum < 0) || (inStringNum >= numStrings) )
		return;
	if (inNewText == NULL)
		return;

	strcpy(displayStrings[inStringNum], inNewText);
//	redraw();
}

//-----------------------------------------------------------------------------
void DGTextArrayDisplay::draw(DGGraphicsContext * inContext)
{
	if (backgroundImage == NULL)
	{
// XXX hmmm, I need to do something else to check on this; we may just want to draw on top of the background
#if 0
		inContext->setFillColor( DGColor(59.0f/255.0f, 83.0f/255.0f, 165.0f/255.0f) );
		inContext->fillRect( getBounds() );
#endif
	}
	else
		backgroundImage->draw(getBounds(), inContext);

	long stringIndex = GetControl32BitValue(carbonControl) - GetControl32BitMinimum(carbonControl);
	if ( (stringIndex >= 0) && (stringIndex < numStrings) )
		drawText(getBounds(), displayStrings[stringIndex], inContext);
}






#pragma mark -
#pragma mark DGAnimation

// XXX Yeah, I know that it's weird to have this be sub-classed from DGTextDisplay since 
// it involves no text, but the mouse handling in DGTextDisplay is exactly what I want here.  
// I should think of a better abstraction scheme, though...
//-----------------------------------------------------------------------------
// Animation
//-----------------------------------------------------------------------------
DGAnimation::DGAnimation(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion, 
						DGImage * inAnimationImage, long inNumAnimationFrames, DGImage * inBackground)
:	DGTextDisplay(inOwnerEditor, inParamID, inRegion, NULL, NULL, inBackground), 
	animationImage(inAnimationImage), numAnimationFrames(inNumAnimationFrames)
{
	if (numAnimationFrames < 1)
		numAnimationFrames = 1;
}

//-----------------------------------------------------------------------------
void DGAnimation::draw(DGGraphicsContext * inContext)
{
	if (backgroundImage != NULL)
		backgroundImage->draw(getBounds(), inContext);

	if (animationImage != NULL)
	{
		SInt32 min = GetControl32BitMinimum(carbonControl);
		SInt32 max = GetControl32BitMaximum(carbonControl);
		SInt32 val = GetControl32BitValue(carbonControl);
		float valNorm = ((max-min) == 0) ? 0.0f : (float)(val-min) / (float)(max-min);
		long frameIndex = (long) ( valNorm * (float)(numAnimationFrames-1) );

		long yoff = frameIndex * (animationImage->getHeight() / numAnimationFrames);
		animationImage->draw(getBounds(), inContext, 0, yoff);
	}
}

#endif // !__LP64__
