#include "dfxguidisplay.h"


//-----------------------------------------------------------------------------
void genericDisplayTextProcedure(Float32 value, char *outText, void *userData);
void genericDisplayTextProcedure(Float32 value, char *outText, void *userData)
{
	if (outText != NULL)
		sprintf(outText, "%.1f", value);
}



//-----------------------------------------------------------------------------
DGTextDisplay::DGTextDisplay(DfxGuiEditor *			inOwnerEditor,
							AudioUnitParameterID	inParamID, 
							DGRect *				inWhere,
							displayTextProcedure	inTextProc, 
							void *					inUserData,
							DGGraphic *				inBackground, 
							char *					inFontName)
:	DGControl(inOwnerEditor, inParamID, inWhere)
{
	BackGround = inBackground;
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

	fontSize = 14.0f;
	fontColor.r = 103; fontColor.g = 161; fontColor.b = 215;

	setType(kDfxGuiType_Display);
	setContinuousControl(true);
	alignment = kTextAlign_right;
}

//-----------------------------------------------------------------------------
DGTextDisplay::~DGTextDisplay()
{
	BackGround = NULL;
	textProc = NULL;
	textProcUserData = NULL;
	if (fontName != NULL)
		free(fontName);
	fontName = NULL;
}


//-----------------------------------------------------------------------------
void DGTextDisplay::mouseDown(Point *P, bool, bool)
{
	last_Y = P->v;
}

//-----------------------------------------------------------------------------
void DGTextDisplay::mouseTrack(Point *P, bool with_option, bool with_shift)
{
	ControlRef carbonControl = getCarbonControl();
	SInt32 max = GetControl32BitMaximum(carbonControl);
	SInt32 val = GetControl32BitValue(carbonControl);

	SInt32 precision = 4;
	if (with_shift)
		precision = 10;
//	SInt32 subtle = (SInt32) getResolution();
	SInt32 subtle = 10;
	if ( P->h > (getBounds()->w / 2) )
		subtle = 1;

	SInt32 dy = last_Y - P->v;
	if (abs(dy) > precision)
	{
		dy /= precision;
		val += dy * subtle;
		if (val > max)
			val = max;
		if (val < 0)
			val = 0;
		SetControl32BitValue(carbonControl, val);
		last_Y = P->v;
	}
}

//-----------------------------------------------------------------------------
void DGTextDisplay::mouseUp(Point *P, bool, bool)
{
}

//-----------------------------------------------------------------------------
void DGTextDisplay::draw(CGContextRef context, UInt32 portHeight)
{
	CGRect bounds;
	getBounds()->copyToCGRect(&bounds, portHeight);

	CGImageRef theBack = NULL;
	if (BackGround != NULL)
		theBack = BackGround->getImage();
	if (theBack == NULL)
	{
		CGContextSetRGBFillColor(context, 59.0f/255.0f, 83.0f/255.0f, 165.0f/255.0f, 1.0f);
		CGContextFillRect(context, bounds);
	}
	else
	{
CGRect whole;
whole = bounds;
whole.size.width = (float) CGImageGetWidth(theBack);
whole.size.height = (float) CGImageGetHeight(theBack);
whole.origin.x -= (float) (where.x - getDfxGuiEditor()->X);
whole.origin.y -= (float) (CGImageGetHeight(theBack) - (where.y - getDfxGuiEditor()->Y) - where.h);
//		CGContextDrawImage(context, bounds, theBack);
		CGContextDrawImage(context, whole, theBack);
	}

	if (textProc != NULL)
	{
		char text[256];
		text[0] = 0;
		textProc(auvp.GetValue(), text, textProcUserData);
		drawText(context, bounds, text);
	}
}

//-----------------------------------------------------------------------------
void DGTextDisplay::drawText(CGContextRef context, CGRect& inBounds, const char *inString)
{
	if (inString == NULL)
		return;

	if (fontName != NULL)
	{
		CGContextSelectFont(context, fontName, fontSize, kCGEncodingMacRoman);
		if (strcmp(fontName, "snoot.org pixel10") == 0)
		{
			CGContextSetShouldSmoothFonts(context, false);
			CGContextSetShouldAntialias(context, false);	// it appears that I gotta do this, too
		}
	}
	CGContextSetRGBFillColor(context, (float)fontColor.r/255.0f, (float)fontColor.g/255.0f, (float)fontColor.b/255.0f, 1.0f);

	if (alignment != kTextAlign_left)
	{
		CGContextSetTextDrawingMode(context, kCGTextInvisible);
		CGContextShowTextAtPoint(context, 0, 0, inString, strlen(inString));
		CGPoint pt = CGContextGetTextPosition(context);
		if (alignment == kTextAlign_center)
			inBounds.origin.x += (inBounds.size.width - pt.x) / 2.0f;
		else if (alignment == kTextAlign_right)
			inBounds.origin.x += inBounds.size.width - pt.x;
	}
	CGContextSetTextDrawingMode(context, kCGTextFill);
	CGContextShowTextAtPoint(context, inBounds.origin.x, inBounds.origin.y+2.0f, inString, strlen(inString));
}






//-----------------------------------------------------------------------------
DGStaticTextDisplay::DGStaticTextDisplay(DfxGuiEditor *inOwnerEditor, DGRect *inWhere, DGGraphic *inBackground, char *inFontName)
:	DGTextDisplay(inOwnerEditor, 0xFFFFFFFF, inWhere, NULL, NULL, inBackground, inFontName), 
	displayString(NULL)
{
	displayString = (char*) malloc(256);
	displayString[0] = 0;
	AUVPattached = false;	// XXX good enough?
}

//-----------------------------------------------------------------------------
DGStaticTextDisplay::~DGStaticTextDisplay()
{
	if (displayString != NULL)
		free(displayString);
	displayString = NULL;
}

//-----------------------------------------------------------------------------
void DGStaticTextDisplay::setText(const char *inNewText)
{
	if (inNewText == NULL)
		return;
	strcpy(displayString, inNewText);
	redraw();
}

//-----------------------------------------------------------------------------
void DGStaticTextDisplay::draw(CGContextRef context, UInt32 portHeight)
{
	CGRect bounds;
	getBounds()->copyToCGRect(&bounds, portHeight);

	CGImageRef theBack = NULL;
	if (BackGround != NULL)
		theBack = BackGround->getImage();
	if (theBack == NULL)
	{
		CGContextSetRGBFillColor(context, 59.0f/255.0f, 83.0f/255.0f, 165.0f/255.0f, 1.0f);
		CGContextFillRect(context, bounds);
	}
	else
	{
CGRect whole;
whole = bounds;
whole.size.width = (float) CGImageGetWidth(theBack);
whole.size.height = (float) CGImageGetHeight(theBack);
whole.origin.x -= (float) (where.x - getDfxGuiEditor()->X);
whole.origin.y -= (float) (CGImageGetHeight(theBack) - (where.y - getDfxGuiEditor()->Y) - where.h);
//		CGContextDrawImage(context, bounds, theBack);
		CGContextDrawImage(context, whole, theBack);
	}

	drawText(context, bounds, displayString);
}
