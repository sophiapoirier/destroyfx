#include "dfxguidisplay.h"


const size_t kDGTextDisplay_stringSize = 256;

//-----------------------------------------------------------------------------
void genericDisplayTextProcedure(float inValue, char * outText, void * inUserData);
void genericDisplayTextProcedure(float inValue, char * outText, void * inUserData)
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
							displayTextProcedure	inTextProc, 
							void *					inUserData,
							DGImage *				inBackgroundImage, 
							DfxGuiTextAlignment		inTextAlignment, 
							float					inFontSize, 
							DGColor					inFontColor, 
							const char *			inFontName)
:	DGControl(inOwnerEditor, inParamID, inRegion), 
	backgroundImage(inBackgroundImage), alignment(inTextAlignment), fontSize(inFontSize), fontColor(inFontColor)
{
	if (inTextProc == NULL)
		textProc = genericDisplayTextProcedure;
	else
		textProc = inTextProc;
	if (inUserData == NULL)
		textProcUserData = this;
	else
		textProcUserData = inUserData;

	isSnootPixel10 = false;
	fontName = (char*) malloc(kDGTextDisplay_stringSize);
	if (inFontName != NULL)
	{
		strcpy(fontName, inFontName);
		if (strcmp(fontName, "snoot.org pixel10") == 0)
			isSnootPixel10 = true;
	}
	else
	{
		// get the application font from the system "theme"
		Str255 appfontname;
		OSStatus themeErr = GetThemeFont(kThemeApplicationFont, smSystemScript, appfontname, NULL, NULL);
		if (themeErr == noErr)
		{
			// Pascal-string to C-string conversion
			memcpy(fontName, &(appfontname[1]), appfontname[0]);
			fontName[appfontname[0]] = 0;
		}
		else
		{
			// what else can we do?
			free(fontName);
			fontName = NULL;
		}
	}

	mouseAxis = kDGTextDisplayMouseAxis_vertical;
	setMouseDragRange(333.0f);	// pixels

	if (fontName != NULL)
	{
		CFStringRef fontCFName = CFStringCreateWithCString(kCFAllocatorDefault, fontName, kCFStringEncodingUTF8);
		if (fontCFName != NULL)
		{
			ATSFontRef atsfont = ATSFontFindFromName(fontCFName, kATSOptionFlagsDefault);
			ATSFontMetrics horizontalMetrics;
			OSStatus metstat = ATSFontGetHorizontalMetrics(atsfont, kATSOptionFlagsDefault, &horizontalMetrics);
			if (metstat == noErr)
			{
				fontAscent = horizontalMetrics.ascent;
				fontDescent = horizontalMetrics.descent;
/*
printf("ascent = %.3f\n", horizontalMetrics.ascent);
printf("descent = %.3f\n", horizontalMetrics.descent);
printf("caps height = %.3f\n", horizontalMetrics.capHeight);
printf("littles height = %.3f\n", horizontalMetrics.xHeight);
*/
			}
			ATSFontMetrics verticalMetrics;
			metstat = ATSFontGetVerticalMetrics(atsfont, kATSOptionFlagsDefault, &verticalMetrics);
			CFRelease(fontCFName);
		}
	}

	shouldAntiAlias = true;
	if (isSnootPixel10)
		shouldAntiAlias = false;

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
		textProc(auvp.GetValue(), text, textProcUserData);
		drawText(getBounds(), text, inContext);
	}
}

//-----------------------------------------------------------------------------
void DGTextDisplay::drawText(DGRect * inRegion, const char * inText, DGGraphicsContext * inContext)
{
	if ( (inText == NULL) || (inRegion == NULL) )
		return;

	CGRect bounds = inRegion->convertToCGRect( inContext->getPortHeight() );
	float flippedBoundsOffset = bounds.size.height;
#ifndef FLIP_CG_COORDINATES
	if ( inContext->isCompositWindow() )
#endif
	CGContextTranslateCTM(inContext->getPlatformGraphicsContext(), 0.0f, flippedBoundsOffset);

	if (fontName != NULL)
		CGContextSelectFont(inContext->getPlatformGraphicsContext(), fontName, fontSize, kCGEncodingMacRoman);
	inContext->setAntialias(shouldAntiAlias);
	inContext->setFillColor(fontColor);

	if (alignment != kDGTextAlign_left)
	{
		CGContextSetTextDrawingMode(inContext->getPlatformGraphicsContext(), kCGTextInvisible);
		CGContextShowTextAtPoint(inContext->getPlatformGraphicsContext(), 0.0f, 0.0f, inText, strlen(inText));
		CGPoint pt = CGContextGetTextPosition(inContext->getPlatformGraphicsContext());
		if (alignment == kDGTextAlign_center)
		{
			float xoffset = (bounds.size.width - pt.x) / 2.0f;
			// don't make the left edge get cropped, just left-align if the text is too long
			if (xoffset > 0.0f)
				bounds.origin.x += xoffset;
		}
		else if (alignment == kDGTextAlign_right)
			bounds.origin.x += bounds.size.width - pt.x;
	}

	// XXX a hack for this font and CGContextShowText
	if (isSnootPixel10)
	{
		if (alignment == kDGTextAlign_left)
			bounds.origin.x -= 1.0f;
		else if (alignment == kDGTextAlign_right)
			bounds.origin.x += 2.0f;
	}

//	if (bold)	// XXX implement this for real
//		CGContextSetTextDrawingMode(inContext->getPlatformGraphicsContext(), kCGTextFillStroke);
//	else
		CGContextSetTextDrawingMode(inContext->getPlatformGraphicsContext(), kCGTextFill);
	float textYoffset = 2.0f;
#ifndef FLIP_CG_COORDINATES
	if ( inContext->isCompositWindow() )
#endif
	textYoffset *= -1.0f;
	CGContextShowTextAtPoint(inContext->getPlatformGraphicsContext(), bounds.origin.x, bounds.origin.y+textYoffset, inText, strlen(inText));

#ifndef FLIP_CG_COORDINATES
	if ( inContext->isCompositWindow() )
#endif
	CGContextTranslateCTM(inContext->getPlatformGraphicsContext(), 0.0f, -flippedBoundsOffset);
}

#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
OSStatus DGTextDisplay::drawCFText(DGRect * inRegion, const CFStringRef inText, DGGraphicsContext * inContext)
{
	if ( (inText == NULL) || (inRegion == NULL) )
		return paramErr;

	inContext->setAntialias(shouldAntiAlias);
	inContext->setFillColor(fontColor);

// XXX do something to actually allow you to set the font ID and the font size and the font color
	ThemeFontID themeFontID = kThemeLabelFont;
//kThemeSystemFont kThemeSystemFontDetail kThemeMiniSystemFont kThemeLabelFont

	// this function is only available in Mac OS X 10.3 or higher
	if (HIThemeDrawTextBox != NULL)
	{
		HIRect bounds = inRegion->convertToCGRect( inContext->getPortHeight() );

		HIThemeTextInfo textInfo;
		memset(&textInfo, 0, sizeof(textInfo));
		textInfo.version = 0;
		textInfo.state = kThemeStateActive;
		textInfo.fontID = themeFontID;
		textInfo.truncationPosition = kHIThemeTextTruncationEnd;
		textInfo.truncationMaxLines = 1;
		textInfo.verticalFlushness = kHIThemeTextVerticalFlushCenter;
		textInfo.options = 0;

		textInfo.horizontalFlushness = kHIThemeTextHorizontalFlushLeft;
		if (alignment == kDGTextAlign_center)
			textInfo.horizontalFlushness = kHIThemeTextHorizontalFlushCenter;
		else if (alignment == kDGTextAlign_right)
			textInfo.horizontalFlushness = kHIThemeTextHorizontalFlushRight;

		return HIThemeDrawTextBox(inText, &bounds, &textInfo, inContext->getPlatformGraphicsContext(), inContext->getHIThemeOrientation());
	}
	else
	{
		Rect bounds = inRegion->convertToMacRect();

		SetThemeTextColor(kThemeTextColorWhite, 32, true);	// XXX eh, is there a real way to get the graphics device bit-depth value?

		SInt16 justification = teFlushLeft;
		if (alignment == kDGTextAlign_center)
			justification = teCenter;
		else if (alignment == kDGTextAlign_right)
			justification = teFlushRight;

		return DrawThemeTextBox(inText, themeFontID, kThemeStateActive, false, &bounds, justification, NULL);
	}
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
	if (mouseAxis & kDGTextDisplayMouseAxis_horizontal)
		diff += inXpos - lastX;
	if (mouseAxis & kDGTextDisplayMouseAxis_vertical)
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
										DfxGuiTextAlignment inTextAlignment, float inFontSize, 
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
//	if (success)
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
						long inNumStrings, DfxGuiTextAlignment inTextAlignment, DGImage * inBackground, 
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

	setMouseDragRange(120.0f);  // number of pixels
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
