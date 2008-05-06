#include "dfxguitools.h"
#include "dfxgui.h"


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
#if TARGET_OS_MAC
	if (context != NULL)
		CGContextSetAlpha(context, inAlpha);
#endif
}

//-----------------------------------------------------------------------------
void DGGraphicsContext::setAntialias(bool inShouldAntialias)
{
#if TARGET_OS_MAC
	if (context != NULL)
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
#if TARGET_OS_MAC
	if (context != NULL)
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
#if TARGET_OS_MAC
	if (context != NULL)
		CGContextSetRGBFillColor(context, inColor.r, inColor.g, inColor.b, inColor.a);
#endif
}

//-----------------------------------------------------------------------------
void DGGraphicsContext::setStrokeColor(DGColor inColor)
{
#if TARGET_OS_MAC
	if (context != NULL)
		CGContextSetRGBStrokeColor(context, inColor.r, inColor.g, inColor.b, inColor.a);
#endif
}

//-----------------------------------------------------------------------------
void DGGraphicsContext::setLineWidth(float inLineWidth)
{
#if TARGET_OS_MAC
	if (context != NULL)
		CGContextSetLineWidth(context, inLineWidth);
#endif
}

//-----------------------------------------------------------------------------
void DGGraphicsContext::beginPath()
{
#if TARGET_OS_MAC
	if (context != NULL)
		CGContextBeginPath(context);
#endif
}


//-----------------------------------------------------------------------------
void DGGraphicsContext::endPath()
{
#if TARGET_OS_MAC
	if (context != NULL)
		CGContextClosePath(context);
#endif
}

//-----------------------------------------------------------------------------
void DGGraphicsContext::fillPath()
{
#if TARGET_OS_MAC
	if (context != NULL)
		CGContextFillPath(context);
#endif
}

//-----------------------------------------------------------------------------
void DGGraphicsContext::strokePath()
{
#if TARGET_OS_MAC
	if (context != NULL)
		CGContextStrokePath(context);
#endif
}

//-----------------------------------------------------------------------------
void DGGraphicsContext::fillRect(DGRect * inRect)
{
	if (inRect == NULL)
		return;

#if TARGET_OS_MAC
	if (context != NULL)
	{
		CGRect cgRect = inRect->convertToCGRect(portHeight);
		CGContextFillRect(context, cgRect);
	}
#endif
}

//-----------------------------------------------------------------------------
void DGGraphicsContext::strokeRect(DGRect * inRect, float inLineWidth)
{
	if (inRect == NULL)
		return;

#if TARGET_OS_MAC
	if (context != NULL)
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
#if TARGET_OS_MAC
	if (context != NULL)
	{
		beginPath();
		CGContextMoveToPoint(context, inX, inY);
	}
#endif
}

//-----------------------------------------------------------------------------
void DGGraphicsContext::addLineToPoint(float inX, float inY)
{
#if TARGET_OS_MAC
	if (context != NULL)
		CGContextAddLineToPoint(context, inX, inY);
#endif
}

//-----------------------------------------------------------------------------
void DGGraphicsContext::strokeLine(float inLineWidth)
{
	// (re)set the line width
	if (inLineWidth >= 0.0f)
		setLineWidth(inLineWidth);
	endPath();
	strokePath();
}

//-----------------------------------------------------------------------------
void DGGraphicsContext::drawLine(float inStartX, float inStartY, float inEndX, float inEndY, float inLineWidth)
{
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
	if (inFontName == NULL)
		return;

	fontSize = inFontSize;	// remember the value
	if (strcmp(inFontName, kDGFontName_SnootPixel10) == 0)
		isSnootPixel10 = true;

#if TARGET_OS_MAC
	if (context != NULL)
	{
		CGContextSelectFont(context, inFontName, inFontSize, kCGEncodingMacRoman);

#if 0
		CFStringRef fontCFName = CFStringCreateWithCString(kCFAllocatorDefault, inFontName, kCFStringEncodingUTF8);
		if (fontCFName != NULL)
		{
			ATSFontRef atsFont = ATSFontFindFromName(fontCFName, kATSOptionFlagsDefault);
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
			CFRelease(fontCFName);
		}
#endif
	}
#endif
}

//-----------------------------------------------------------------------------
void DGGraphicsContext::drawText(DGRect * inRegion, const char * inText, DGTextAlignment inAlignment)
{
	if ( (inText == NULL) || (inRegion == NULL) )
		return;

#if TARGET_OS_MAC
	if (context == NULL)
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






#pragma mark -
#pragma mark DGImage

/***********************************************************************
	DGImage
	class for loading and containing images
***********************************************************************/

//-----------------------------------------------------------------------------
DGImage::DGImage(const char * inFileName, DfxGuiEditor * inEditor)
{
#if TARGET_OS_MAC
	cgImage = NULL;

	// no assumptions can be made about how long the reference is valid, 
	// and the caller should not attempt to release the CFBundleRef object
	CFBundleRef pluginBundleRef = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));
	CFStringRef fileCFName = CFStringCreateWithCString(kCFAllocatorDefault, inFileName, kCFStringEncodingUTF8);
	if ( (fileCFName != NULL) && (pluginBundleRef != NULL) )
	{
		CFURLRef imageResourceURL = CFBundleCopyResourceURL(pluginBundleRef, fileCFName, NULL, NULL);
		if (imageResourceURL != NULL)
		{
			CGDataProviderRef provider = CGDataProviderCreateWithURL(imageResourceURL);
			if (provider != NULL)
			{
//				char * fileExtension = strrchr(inFileName, '.');
//				if (fileExtension != NULL)
//					fileExtension += 1;	// advance past the .
				const bool shouldInterpolate = true;
				CFStringRef fileCFExtension = CFURLCopyPathExtension(imageResourceURL);
				if (fileCFExtension != NULL)
				{
					if (CFStringCompare(fileCFExtension, CFSTR("png"), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
						cgImage = CGImageCreateWithPNGDataProvider(provider, NULL, shouldInterpolate, kCGRenderingIntentDefault);
					else if ( (CFStringCompare(fileCFExtension, CFSTR("jpg"), kCFCompareCaseInsensitive) == kCFCompareEqualTo) || 
							(CFStringCompare(fileCFExtension, CFSTR("jpeg"), kCFCompareCaseInsensitive) == kCFCompareEqualTo) || 
							(CFStringCompare(fileCFExtension, CFSTR("jfif"), kCFCompareCaseInsensitive) == kCFCompareEqualTo) || 
							(CFStringCompare(fileCFExtension, CFSTR("jpe"), kCFCompareCaseInsensitive) == kCFCompareEqualTo) )
						cgImage = CGImageCreateWithJPEGDataProvider(provider, NULL, shouldInterpolate, kCGRenderingIntentDefault);
					CFRelease(fileCFExtension);
				}
				CGDataProviderRelease(provider);

				bool flipImage = false;
#ifdef FLIP_CG_COORDINATES
				flipImage = true;
#else
				if (inEditor != NULL)
				{
					if ( inEditor->IsCompositWindow() )
						flipImage = true;
				}
#endif
				// create an uncompressed, alpha-premultiplied bitmap image in memory
				if (cgImage != NULL)
					cgImage = PreRenderCGImageBuffer(cgImage, flipImage);
			}
			CFRelease(imageResourceURL);
		}
	}
	if (fileCFName != NULL)
		CFRelease(fileCFName);
#endif

	if (inEditor != NULL)
		inEditor->addImage(this);
}

//-----------------------------------------------------------------------------
DGImage::~DGImage()
{
#if TARGET_OS_MAC
	if (cgImage != NULL)
		CGImageRelease(cgImage);
	cgImage = NULL;
#endif
}

//-----------------------------------------------------------------------------
long DGImage::getWidth()
{
#if TARGET_OS_MAC
	if (cgImage != NULL)
		return CGImageGetWidth(cgImage);
#endif

	return 1;	// XXX sometimes safer than returning 0
}
	
//-----------------------------------------------------------------------------
long DGImage::getHeight()
{
#if TARGET_OS_MAC
	if (cgImage != NULL)
		return CGImageGetHeight(cgImage);
#endif

	return 1;	// XXX sometimes safer than returning 0
}

//-----------------------------------------------------------------------------
void DGImage::draw(DGRect * inRect, DGGraphicsContext * inContext, long inXoffset, long inYoffset)
{
	if ( (cgImage == NULL) && (inRect == NULL) && (inContext == NULL) )
		return;

//OSStatus status = HIViewDrawCGImage(inContext->getPlatformGraphicsContext(), HIRect * inBounds, cgImage);
	// convert the DGRect to a CGRect for the CG API stuffs
	CGRect drawRect = inRect->convertToCGRect( inContext->getPortHeight() );

// XXX figure out how to make this work
#if 0
	// XXX this is supposedly much faster for drawing part of an image in Mac OS X 10.4 or above
	if (CGImageCreateWithImageInRect != NULL)
	{
		CGImageRef subImage = CGImageCreateWithImageInRect(cgImage, drawRect);
		if (subImage != NULL)
		{
			drawRect.origin.x = drawRect.origin.y = 0.0f;
			CGContextDrawImage(inContext->getPlatformGraphicsContext(), drawRect, subImage);
			CFRelease(subImage);
			return;
		}
	}
#endif

	// avoid CoreGraphics' automatic scaled-image-to-fit-drawing-rectangle feature 
	// by specifying the drawing rect size as the size of the image, 
	// regardless of the actual drawing area size
	drawRect.size.width = (float) getWidth();
	drawRect.size.height = (float) getHeight();

#ifndef FLIP_CG_COORDINATES
	// this positions the image at the top of the region 
	// rather than at the bottom (upside-down CG coordinates)
	if (! inContext->isCompositWindow() )
		drawRect.origin.y -= (float) (getHeight() - inRect->h);
#endif
	// take offsets into account
	drawRect.origin.x -= (float) inXoffset;
#ifndef FLIP_CG_COORDINATES
	if (! inContext->isCompositWindow() )
		inYoffset *= -1.0f;	// we have to add because the Y axis is upside-down in CG
#endif
	drawRect.origin.y -= (float) inYoffset;

	// draw
	CGContextDrawImage(inContext->getPlatformGraphicsContext(), drawRect, cgImage);
}

//-----------------------------------------------------------------------------
void ReleasePreRenderedCGImageBuffer(void * info, const void * data, size_t size)
{
	free((void*)data);
}

//-----------------------------------------------------------------------------
// create an uncompressed, alpha-premultiplied bitmap image in memory
// so far as I can tell, this makes drawing PNG files with CGContextDrawImage about 25 times faster
CGImageRef PreRenderCGImageBuffer(CGImageRef inImage, bool inFlipImage)
{
	if (inImage == NULL)
		return NULL;

	// get image info
	size_t width = CGImageGetWidth(inImage);
	size_t height = CGImageGetHeight(inImage);

//	CGColorSpaceRef colorSpace = CGImageGetColorSpace(inImage);
	CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
//	CGImageAlphaInfo alphaInfo = CGImageGetAlphaInfo(inImage);
	const CGImageAlphaInfo alphaInfo = kCGImageAlphaPremultipliedFirst;
	const bool shouldInterpolate = true;

	const size_t bitsPerComponent = 8;	// number of bits per color component in a pixel
	const size_t bitsPerPixel = bitsPerComponent * 4;	// total number of bits in a pixel
	const size_t bytesPerRow = ((width * bitsPerPixel) + 7) / 8;

	if (colorSpace != NULL)
	{
		// create a bitmap graphic context pointing at a data buffer that is 
		// large enough to hold an uncompressed rendered version of the image
		size_t dataSize = bytesPerRow * height;
		void * buffer = malloc(dataSize);
		memset(buffer, 0, dataSize);
		CGContextRef context = CGBitmapContextCreate(buffer, width, height, bitsPerComponent, bytesPerRow, colorSpace, alphaInfo);
		if (context != NULL)
		{
			if (inFlipImage)
			{
				CGContextTranslateCTM(context, 0.0f, (float)height);
				CGContextScaleCTM(context, 1.0f, -1.0f);
			}
			// draw image into context
			CGRect drawRect = CGRectMake(0.0f, 0.0f, (float)width, (float)height);
			CGContextDrawImage(context, drawRect, inImage);
			CGContextRelease(context);

			// create data provider for this image buffer
			CGDataProviderRef provider = CGDataProviderCreateWithData(NULL, buffer, dataSize, ReleasePreRenderedCGImageBuffer);
			if (provider != NULL)
			{
				// create a CGImage with the data provider
				CGImageRef prerenderedImage = CGImageCreate(width, height, bitsPerComponent, bitsPerPixel, bytesPerRow, 
									colorSpace, alphaInfo, provider, NULL, shouldInterpolate, kCGRenderingIntentDefault);
				if (prerenderedImage != NULL)
				{
					CGImageRelease(inImage);
					inImage = prerenderedImage;
				}
				CGDataProviderRelease(provider);
			}
		}
		CGColorSpaceRelease(colorSpace);
	}

	return inImage;
}

/*
#include <QuickTime/ImageCompression.h>
//-----------------------------------------------------------------------------
CGImageRef DFXGUI_LoadImageQT(CFStringRef inFileName)
{
	CGImageRef imageRef = NULL;

	CFBundleRef pluginBundleRef = CFBundleGetBundleWithIdentifier( CFSTR(PLUGIN_BUNDLE_IDENTIFIER) );
	if (pluginBundleRef != NULL)
	{
		CFURLRef imageResourceURL = CFBundleCopyResourceURL(pluginBundleRef, inFileName, NULL, NULL);
		if (imageResourceURL != NULL)
		{
			Handle dataRef = NULL;
			OSType dataRefType = 0;
			OSErr error = QTNewDataReferenceFromCFURL(imageResourceURL, 0, &dataRef, &dataRefType);
			CFRelease(imageResourceURL);
			if ( (error == noErr) && (dataRef != NULL) )
			{
				GraphicsImportComponent graphicsImporter = NULL;
				error = GetGraphicsImporterForDataRef(dataRef, dataRefType, &graphicsImporter);
				if ( (error == noErr) && (graphicsImporter != NULL) )
				{
					error = GraphicsImportCreateCGImage(graphicsImporter, &imageRef, 0);
					CloseComponent(graphicsImporter);
				}
				DisposeHandle(dataRef);
			}
		}
	}

	return imageRef;
}
*/
