#include "dfxguidisplay.h"


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
							DGTextAlignment			inTextAlignment, 
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

	mouseAxis = kDGTextDisplayMouseAxis_vertical;
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
OSStatus DGTextDisplay::drawCFText(DGRect * inRegion, const CFStringRef inText, DGGraphicsContext * inContext)
{
	if ( (inText == NULL) || (inRegion == NULL) )
		return paramErr;

	inContext->setAntialias(shouldAntiAlias);
	inContext->setColor(fontColor);

// XXX do something to actually allow you to set the font ID and the font size and the font color
	const ThemeFontID themeFontID = kThemeLabelFont;
//kThemeSystemFont kThemeSystemFontDetail kThemeMiniSystemFont kThemeLabelFont

	// this function is only available in Mac OS X 10.3 or higher
	if (HIThemeDrawTextBox != NULL)
	{
		HIRect bounds = inRegion->convertToCGRect( inContext->getPortHeight() );

		HIThemeTextInfo textInfo = {0};
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

		const ThemeDrawState themDrawState = kThemeStateActive;
		SInt16 justification = teFlushLeft;
		if (alignment == kDGTextAlign_center)
			justification = teCenter;
		else if (alignment == kDGTextAlign_right)
			justification = teFlushRight;

		// XXX center the text vertically (yah?)
		Point heightPoint;
		OSStatus status = GetThemeTextDimensions(inText, themeFontID, themDrawState, false, &heightPoint, NULL);
		if (status == noErr)
			InsetRect( &bounds, 0, ((bounds.bottom-bounds.top) - heightPoint.v) / 2 );

		return DrawThemeTextBox(inText, themeFontID, themDrawState, false, &bounds, justification, NULL);
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
