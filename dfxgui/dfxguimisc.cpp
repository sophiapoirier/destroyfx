/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2018  Sophia Poirier

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

#include "dfxguimisc.h"

#include <algorithm>

#include "dfxmisc.h"

#if TARGET_OS_MAC
	#include <Carbon/Carbon.h>
#endif


//-----------------------------------------------------------------------------
DGColor const DGColor::kBlack(kBlackCColor);
DGColor const DGColor::kWhite(kWhiteCColor);


#if 0
#pragma mark DGGraphicsContext

//-----------------------------------------------------------------------------
DGGraphicsContext::DGGraphicsContext(TARGET_PLATFORM_GRAPHICS_CONTEXT inContext)
:	context(inContext)
{
	fontSize = fontAscent = fontDescent = 0.0f;
	isSnootPixel10 = false;

#if TARGET_OS_MAC
	portHeight = 0;
#endif
}

//-----------------------------------------------------------------------------
void DGGraphicsContext::setAlpha(float inAlpha)
{
// XXX use VSTGUI implementation
#if TARGET_OS_MAC && 0
	if (context != nullptr)
		CGContextSetAlpha(context, inAlpha);
#endif
}

//-----------------------------------------------------------------------------
void DGGraphicsContext::setAntialias(bool inShouldAntialias)
{
// XXX use VSTGUI implementation
#if TARGET_OS_MAC && 0
	if (context != nullptr)
	{
		CGContextSetShouldAntialias(context, inShouldAntialias);
		CGContextSetShouldSmoothFonts(context, inShouldAntialias);
		if (inShouldAntialias)
			CGContextSetAllowsAntialiasing(context, inShouldAntialias);
	}
#endif
}

//-----------------------------------------------------------------------------
void DGGraphicsContext::setAntialiasQuality(DGAntialiasQuality inQualityLevel)
{
// XXX use VSTGUI implementation
#if TARGET_OS_MAC && 0
	if (context != nullptr)
	{
		CGInterpolationQuality cgQuality;
		switch (inQualityLevel)
		{
			case kDGAntialiasQuality_none:
				cgQuality = kCGInterpolationNone;
				break;
			case kDGAntialiasQuality_low:
				cgQuality = kCGInterpolationLow;
				break;
			case kDGAntialiasQuality_high:
				cgQuality = kCGInterpolationHigh;
				break;
			case kDGAntialiasQuality_default:
			default:
				cgQuality = kCGInterpolationDefault;
				break;
		}
		CGContextSetInterpolationQuality(context, cgQuality);
	}
#endif
}

//-----------------------------------------------------------------------------
void DGGraphicsContext::setColor(DGColor inColor)
{
	setFillColor(inColor);
	setStrokeColor(inColor);
}

//-----------------------------------------------------------------------------
void DGGraphicsContext::setFillColor(DGColor inColor)
{
// XXX use VSTGUI implementation
#if TARGET_OS_MAC && 0
	if (context != nullptr)
		CGContextSetRGBFillColor(context, inColor.r, inColor.g, inColor.b, inColor.a);
#endif
}

//-----------------------------------------------------------------------------
void DGGraphicsContext::setStrokeColor(DGColor inColor)
{
// XXX use VSTGUI implementation
#if TARGET_OS_MAC && 0
	if (context != nullptr)
		CGContextSetRGBStrokeColor(context, inColor.r, inColor.g, inColor.b, inColor.a);
#endif
}

//-----------------------------------------------------------------------------
void DGGraphicsContext::setLineWidth(float inLineWidth)
{
// XXX use VSTGUI implementation
#if TARGET_OS_MAC && 0
	if (context != nullptr)
		CGContextSetLineWidth(context, inLineWidth);
#endif
}

//-----------------------------------------------------------------------------
void DGGraphicsContext::beginPath()
{
// XXX use VSTGUI implementation
#if TARGET_OS_MAC && 0
	if (context != nullptr)
		CGContextBeginPath(context);
#endif
}


//-----------------------------------------------------------------------------
void DGGraphicsContext::endPath()
{
// XXX use VSTGUI implementation
#if TARGET_OS_MAC && 0
	if (context != nullptr)
		CGContextClosePath(context);
#endif
}

//-----------------------------------------------------------------------------
void DGGraphicsContext::fillPath()
{
// XXX use VSTGUI implementation
#if TARGET_OS_MAC && 0
	if (context != nullptr)
		CGContextFillPath(context);
#endif
}

//-----------------------------------------------------------------------------
void DGGraphicsContext::strokePath()
{
// XXX use VSTGUI implementation
#if TARGET_OS_MAC && 0
	if (context != nullptr)
		CGContextStrokePath(context);
#endif
}

//-----------------------------------------------------------------------------
void DGGraphicsContext::fillRect(DGRect * inRect)
{
	if (inRect == nullptr)
		return;

// XXX use VSTGUI implementation
#if TARGET_OS_MAC && 0
	if (context != nullptr)
	{
		CGRect cgRect = inRect->convertToCGRect(portHeight);
		CGContextFillRect(context, cgRect);
	}
#endif
}

//-----------------------------------------------------------------------------
void DGGraphicsContext::strokeRect(DGRect * inRect, float inLineWidth)
{
	if (inRect == nullptr)
		return;

// XXX use VSTGUI implementation
#if TARGET_OS_MAC && 0
	if (context != nullptr)
	{
		CGRect cgRect = inRect->convertToCGRect(portHeight);
		const float halfLineWidth = inLineWidth / 2.0f;
		cgRect = CGRectInset(cgRect, halfLineWidth, halfLineWidth);	// CoreGraphics lines are positioned between pixels rather than on them
		// use the currently-set line width
		if (inLineWidth < 0.0f)
			CGContextStrokeRect(context, cgRect);
		// specify the line width
		else
			CGContextStrokeRectWithWidth(context, cgRect, inLineWidth);
	}
#endif
}

//-----------------------------------------------------------------------------
void DGGraphicsContext::moveToPoint(float inX, float inY)
{
// XXX use VSTGUI implementation
#if TARGET_OS_MAC && 0
	if (context != nullptr)
	{
		beginPath();
		CGContextMoveToPoint(context, inX, inY);
	}
#endif
}

//-----------------------------------------------------------------------------
void DGGraphicsContext::addLineToPoint(float inX, float inY)
{
// XXX use VSTGUI implementation
#if TARGET_OS_MAC && 0
	if (context != nullptr)
		CGContextAddLineToPoint(context, inX, inY);
#endif
}

//-----------------------------------------------------------------------------
void DGGraphicsContext::strokeLine(float inLineWidth)
{
// XXX use VSTGUI implementation
	// (re)set the line width
	if (inLineWidth >= 0.0f)
		setLineWidth(inLineWidth);
	endPath();
	strokePath();
}

//-----------------------------------------------------------------------------
void DGGraphicsContext::drawLine(float inStartX, float inStartY, float inEndX, float inEndY, float inLineWidth)
{
// XXX use VSTGUI implementation
	beginPath();
	// (re)set the line width
	if (inLineWidth >= 0.0f)
		setLineWidth(inLineWidth);
	moveToPoint(inStartX, inStartY);
	addLineToPoint(inEndX, inEndY);
	endPath();
	strokePath();
}


//-----------------------------------------------------------------------------
void DGGraphicsContext::setFont(const char * inFontName, float inFontSize)
{
	if (inFontName == nullptr)
		return;

	fontSize = inFontSize;	// remember the value
	if (strcmp(inFontName, kDGFontName_SnootPixel10) == 0)
		isSnootPixel10 = true;

// XXX use VSTGUI implementation
#if TARGET_OS_MAC && 0
	if (context != nullptr)
	{
		CGContextSelectFont(context, inFontName, inFontSize, kCGEncodingMacRoman);

#if 0
		const dfx::UniqueCFType<CFStringRef> fontCFName(CFStringCreateWithCString(kCFAllocatorDefault, inFontName, kCFStringEncodingUTF8));
		if (fontCFName)
		{
			ATSFontRef atsFont = ATSFontFindFromName(fontCFName.get(), kATSOptionFlagsDefault);
			ATSFontMetrics verticalMetrics = {0};
			status = ATSFontGetVerticalMetrics(atsFont, kATSOptionFlagsDefault, &verticalMetrics);
			if (status == noErr)
			{
				fontAscent = verticalMetrics.ascent;
				fontDescent = verticalMetrics.descent;
/*
printf("ascent = %.3f\n", verticalMetrics.ascent);
printf("descent = %.3f\n", verticalMetrics.descent);
printf("caps height = %.3f\n", verticalMetrics.capHeight);
printf("littles height = %.3f\n", verticalMetrics.xHeight);
*/
			}
		}
#endif
	}
#endif
}

//-----------------------------------------------------------------------------
void DGGraphicsContext::drawText(DGRect * inRegion, const char * inText, DGTextAlignment inAlignment)
{
	if ( (inText == nullptr) || (inRegion == nullptr) )
		return;

// XXX use VSTGUI implementation
#if TARGET_OS_MAC && 0
	if (context == nullptr)
		return;

	CGRect bounds = inRegion->convertToCGRect( getPortHeight() );
	float flippedBoundsOffset = bounds.size.height;
#ifndef FLIP_CG_COORDINATES
	if ( isCompositWindow() )
#endif
	CGContextTranslateCTM(context, 0.0f, flippedBoundsOffset);

	if (inAlignment != kDGTextAlign_left)
	{
		CGContextSetTextDrawingMode(context, kCGTextInvisible);
		CGContextShowTextAtPoint(context, 0.0f, 0.0f, inText, strlen(inText));
		CGPoint pt = CGContextGetTextPosition(context);
		if (inAlignment == kDGTextAlign_center)
		{
			float xoffset = (bounds.size.width - pt.x) / 2.0f;
			// don't make the left edge get cropped, just left-align if the text is too long
			if (xoffset > 0.0f)
				bounds.origin.x += xoffset;
		}
		else if (inAlignment == kDGTextAlign_right)
			bounds.origin.x += bounds.size.width - pt.x;
	}

	if (isSnootPixel10)
	{
		// it is a bitmapped font that should not be anti-aliased
		setAntialias(false);
		// XXX a hack for this font and CGContextShowText
		if (inAlignment == kDGTextAlign_left)
			bounds.origin.x -= 1.0f;
		else if (inAlignment == kDGTextAlign_right)
			bounds.origin.x += 2.0f;
	}

	CGContextSetTextDrawingMode(context, kCGTextFill);
	float textYoffset = 2.0f;
	// XXX this is a presumptious hack for vertical centering (only works when 1 point = 1 pixel)
	textYoffset += floorf( (bounds.size.height - fontSize) * 0.5f );
#ifndef FLIP_CG_COORDINATES
	if ( isCompositWindow() )
#endif
	textYoffset *= -1.0f;
	CGContextShowTextAtPoint(context, bounds.origin.x, bounds.origin.y+textYoffset, inText, strlen(inText));

#ifndef FLIP_CG_COORDINATES
	if ( isCompositWindow() )
#endif
	CGContextTranslateCTM(context, 0.0f, -flippedBoundsOffset);
#endif
// TARGET_OS_MAC
}

#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
OSStatus DGGraphicsContext::drawCFText(DGRect * inRegion, CFStringRef inText, DGTextAlignment inAlignment)
{
	if ( (inText == nullptr) || (inRegion == nullptr) )
		return paramErr;

	if (context == nullptr)
		return coreFoundationUnknownErr;	// XXX what error makes sense here?

// XXX do something to actually allow you to set the font ID and the font size and the font color
	const ThemeFontID themeFontID = kThemeLabelFont;
//kThemeSystemFont kThemeSystemFontDetail kThemeMiniSystemFont kThemeLabelFont

	// this function is only available in Mac OS X 10.3 or higher
	if (HIThemeDrawTextBox != nullptr)
	{
		HIRect bounds = inRegion->convertToCGRect( getPortHeight() );

		HIThemeTextInfo textInfo = {0};
		textInfo.version = 0;
		textInfo.state = kThemeStateActive;
		textInfo.fontID = themeFontID;
		textInfo.truncationPosition = kHIThemeTextTruncationEnd;
		textInfo.truncationMaxLines = 1;
		textInfo.verticalFlushness = kHIThemeTextVerticalFlushCenter;
		textInfo.options = 0;

		textInfo.horizontalFlushness = kHIThemeTextHorizontalFlushLeft;
		if (inAlignment == kDGTextAlign_center)
			textInfo.horizontalFlushness = kHIThemeTextHorizontalFlushCenter;
		else if (inAlignment == kDGTextAlign_right)
			textInfo.horizontalFlushness = kHIThemeTextHorizontalFlushRight;

		return HIThemeDrawTextBox(inText, &bounds, &textInfo, getPlatformGraphicsContext(), getHIThemeOrientation());
	}
	else
	{
		Rect bounds = inRegion->convertToMacRect();

		SetThemeTextColor(kThemeTextColorWhite, 32, true);	// XXX eh, is there a real way to get the graphics device bit-depth value?

		const ThemeDrawState themDrawState = kThemeStateActive;
		SInt16 justification = teFlushLeft;
		if (inAlignment == kDGTextAlign_center)
			justification = teCenter;
		else if (inAlignment == kDGTextAlign_right)
			justification = teFlushRight;

		// XXX center the text vertically (yah?)
		Point heightPoint;
		OSStatus status = GetThemeTextDimensions(inText, themeFontID, themDrawState, false, &heightPoint, nullptr);
		if (status == noErr)
			InsetRect( &bounds, 0, ((bounds.bottom-bounds.top) - heightPoint.v) / 2 );

		return DrawThemeTextBox(inText, themeFontID, themDrawState, false, &bounds, justification, nullptr);
	}
}
#endif
// TARGET_OS_MAC
#endif






#pragma mark -

#if 0
//-----------------------------------------------------------------------------
unsigned long dfx::ConvertVstGuiMouseButtons(long inButtons)
{
	return static_cast<unsigned long>(inButtons & (kLButton | kMButton | kRButton));
}

//-----------------------------------------------------------------------------
DGKeyModifiers dfx::ConvertVstGuiKeyModifiers(long inButtons)
{
	DGKeyModifiers resultKeys = 0;
	if (inButtons & kShift)
		resultKeys |= kDGKeyModifier_shift;
	if (inButtons & kControl)
		resultKeys |= kDGKeyModifier_accel;
	if (inButtons & kAlt)
		resultKeys |= kDGKeyModifier_alt;
	if (inButtons & kApple)
		resultKeys |= kDGKeyModifier_extra;
	return resultKeys;
}
#endif

//-----------------------------------------------------------------------------
SharedPointer<CFontDesc> dfx::CreateVstGuiFont(float inFontSize, char const* inFontName)
{
	if (inFontName)
	{
		return makeOwned<CFontDesc>(inFontName, inFontSize);
	}
	else
	{
		auto fontDesc = makeOwned<CFontDesc>(kSystemFont->getName(), inFontSize);
#if TARGET_OS_MAC
		// get the application font from the system "theme"
		auto const fontType = HIThemeGetUIFontType(kThemeApplicationFont);
		if (fontType != kCTFontNoFontType)
		{
			dfx::UniqueCFType<CTFontRef> const fontRef(CTFontCreateUIFontForLanguage(fontType, 0.0, nullptr));
			if (fontRef)
			{
				dfx::UniqueCFType<CFStringRef> const fontCFName(CTFontCopyFullName(fontRef.get()));
				if (fontCFName)
				{
					auto const fontCName = dfx::CreateCStringFromCFString(fontCFName.get(), kCFStringEncodingUTF8);
					if (fontCName)
					{
						fontDesc->setName(fontCName.get());
					}
				}
			}
		}
#endif
		return fontDesc;
	}
}

//-----------------------------------------------------------------------------
// XXX TODO: this doesn't support locale, assumes comma
std::string dfx::RemoveDigitSeparators(std::string const& inText)
{
	std::string resultText(inText.size(), '\0');
	resultText.erase(std::remove_copy(inText.cbegin(), inText.cend(), resultText.begin(), ','), resultText.end());
	return resultText;
}
