#include "dfxguidisplay.h"


//-----------------------------------------------------------------------------
void genericDisplayTextProcedure(float value, char * outText, void * userData);
void genericDisplayTextProcedure(float value, char * outText, void * userData)
{
	if (outText != NULL)
		sprintf(outText, "%.1f", value);
}



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

	fontName = (char*) malloc(256);
	if (inFontName != NULL)
		strcpy(fontName, inFontName);
	else
	{
		// get the application font from the system "theme"
		Str255 appfontname;
		OSStatus themeErr = GetThemeFont(kThemeApplicationFont, smSystemScript, appfontname, NULL, NULL);
		if (themeErr == noErr)
		{
			// cheapo Pascal-string to C-string conversion
			appfontname[appfontname[0]+1] = 0;
			strcpy( fontName, (char*) &(appfontname[1]) );
		}
		else
		{
			// what else can we do?
			free(fontName);
			fontName = NULL;
		}
	}

	mouseAxis = kDGTextDisplayMouseAxis_vertical;
	mouseDragRange = 333.0f;	// pixels

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
void DGTextDisplay::draw(CGContextRef inContext, long inPortHeight)
{
	if (backgroundImage == NULL)
	{
// XXX hmmm, I need to do something else to check on this; we may just want to draw on top of the background
#if 0
		CGRect fillRect = getBounds()->convertToCGRect(inPortHeight);
		CGContextSetRGBFillColor(inContext, 59.0f/255.0f, 83.0f/255.0f, 165.0f/255.0f, 1.0f);
		CGContextFillRect(inContext, fillRect);
#endif
	}
	else
//		backgroundImage->draw(getBounds(), inContext, inPortHeight, getBounds()->x - (long)(getDfxGuiEditor()->GetXOffset()), getBounds()->y - (long)(getDfxGuiEditor()->GetYOffset()));	// draw underneath-style
		backgroundImage->draw(getBounds(), inContext, inPortHeight);

	if (textProc != NULL)
	{
		char text[256];
		text[0] = 0;
		textProc(auvp.GetValue(), text, textProcUserData);
		drawText(getBounds(), text, inContext, inPortHeight);
	}
}

//-----------------------------------------------------------------------------
void DGTextDisplay::drawText(DGRect * inRegion, const char * inText, CGContextRef inContext, long inPortHeight)
{
	if ( (inText == NULL) || (inRegion == NULL) )
		return;

	CGRect bounds = inRegion->convertToCGRect(inPortHeight);
#ifdef FLIP_CG_COORDINATES
	bounds.origin.y *= -1.0f;
	CGContextScaleCTM(inContext, 1.0f, -1.0f);
	CGContextTranslateCTM(inContext, 0.0f, -bounds.size.height);
#endif

	bool drawSmoothText = true;
	if (fontName != NULL)
	{
		CGContextSelectFont(inContext, fontName, fontSize, kCGEncodingMacRoman);
		if (strcmp(fontName, "snoot.org pixel10") == 0)
			drawSmoothText = false;
	}
	CGContextSetShouldSmoothFonts(inContext, drawSmoothText);
	CGContextSetShouldAntialias(inContext, drawSmoothText);	// it appears that I gotta do this, too
	CGContextSetRGBFillColor(inContext, fontColor.r, fontColor.g, fontColor.b, 1.0f);

	if (alignment != kDGTextAlign_left)
	{
		CGContextSetTextDrawingMode(inContext, kCGTextInvisible);
		CGContextShowTextAtPoint(inContext, 0.0f, 0.0f, inText, strlen(inText));
		CGPoint pt = CGContextGetTextPosition(inContext);
		if (alignment == kDGTextAlign_center)
			bounds.origin.x += (bounds.size.width - pt.x) / 2.0f;
		else if (alignment == kDGTextAlign_right)
			bounds.origin.x += bounds.size.width - pt.x;
	}
	CGContextSetTextDrawingMode(inContext, kCGTextFill);
	CGContextShowTextAtPoint(inContext, bounds.origin.x, bounds.origin.y+2.0f, inText, strlen(inText));

#ifdef FLIP_CG_COORDINATES
	CGContextTranslateCTM(inContext, 0.0f, bounds.size.height);
	CGContextScaleCTM(inContext, 1.0f, -1.0f);
#endif
}

//-----------------------------------------------------------------------------
void DGTextDisplay::mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, unsigned long inKeyModifiers)
{
	lastX = inXpos;
	lastY = inYpos;
}

//-----------------------------------------------------------------------------
void DGTextDisplay::mouseTrack(float inXpos, float inYpos, unsigned long inMouseButtons, unsigned long inKeyModifiers)
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
		diff /= fineTuneFactor;
	val += (SInt32) (diff * (float)(max-min) / mouseDragRange);

	if (val > max)
		val = max;
	if (val < min)
		val = min;
	if (val != oldval)
		SetControl32BitValue(carbonControl, val);

	lastX = inXpos;
	lastY = inYpos;
}






//-----------------------------------------------------------------------------
DGStaticTextDisplay::DGStaticTextDisplay(DfxGuiEditor * inOwnerEditor, DGRect * inRegion, DGImage * inBackground, 
										DfxGuiTextAlignment inTextAlignment, float inFontSize, 
										DGColor inFontColor, const char * inFontName)
:	DGTextDisplay(inOwnerEditor, DFX_PARAM_INVALID_ID, inRegion, NULL, NULL, inBackground, 
					inTextAlignment, inFontSize, inFontColor, inFontName), 
	displayString(NULL)
{
	displayString = (char*) malloc(256);
	displayString[0] = 0;
	parameterAttached = false;	// XXX good enough?
}

//-----------------------------------------------------------------------------
DGStaticTextDisplay::~DGStaticTextDisplay()
{
	if (displayString != NULL)
		free(displayString);
	displayString = NULL;
}

//-----------------------------------------------------------------------------
void DGStaticTextDisplay::setText(const char * inNewText)
{
	if (inNewText == NULL)
		return;
	strcpy(displayString, inNewText);
	redraw();
}

//-----------------------------------------------------------------------------
void DGStaticTextDisplay::draw(CGContextRef inContext, long inPortHeight)
{
	if (backgroundImage == NULL)
	{
// XXX hmmm, I need to do something else to check on this; we may just want to draw on top of the background
#if 0
		CGRect fillRect = getBounds()->convertToCGRect(inPortHeight);
		CGContextSetRGBFillColor(inContext, 59.0f/255.0f, 83.0f/255.0f, 165.0f/255.0f, 1.0f);
		CGContextFillRect(inContext, fillRect);
#endif
	}
	else
//		backgroundImage->draw(getBounds(), inContext, inPortHeight, getBounds()->x - (long)(getDfxGuiEditor()->GetXOffset()), getBounds()->y - (long)(getDfxGuiEditor()->GetYOffset()));	// draw underneath-style
		backgroundImage->draw(getBounds(), inContext, inPortHeight);

	drawText(getBounds(), displayString, inContext, inPortHeight);
}
