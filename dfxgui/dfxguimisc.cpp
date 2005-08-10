#include "dfxguitools.h"
#include "dfxgui.h"


//-----------------------------------------------------------------------------
DGGraphicsContext::DGGraphicsContext(TARGET_PLATFORM_GRAPHICS_CONTEXT inContext)
:	context(inContext)
{
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
void DGGraphicsContext::setFillColor(DGColor inColor, float inAlpha)
{
#if TARGET_OS_MAC
	if (context != NULL)
		CGContextSetRGBFillColor(context, inColor.r, inColor.g, inColor.b, inAlpha);
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
void DGGraphicsContext::strokeRect(DGRect * inRect)
{
#if TARGET_OS_MAC
	if (context != NULL)
	{
		CGRect cgRect = inRect->convertToCGRect(portHeight);
		cgRect = CGRectInset(cgRect, 0.5f, 0.5f);	// CoreGraphics lines are positioned between pixels rather than on them
		CGContextStrokeRect(context, cgRect);
//		CGContextStrokeRectWithWidth(inContext, cgRect, inLineWidth);
	}
#endif
}

//-----------------------------------------------------------------------------
void DGGraphicsContext::strokeLine(float inStartX, float inStartY, float inEndX, float inEndY)
{
#if TARGET_OS_MAC
	if (context != NULL)
	{
		CGContextBeginPath(context);
		CGContextMoveToPoint(context, inStartX, inStartY);
		CGContextAddLineToPoint(context, inEndX, inEndY);
		CGContextClosePath(context);
		CGContextStrokePath(context);
	}
#endif
}



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
	CFStringRef fileCFName = CFStringCreateWithCString(kCFAllocatorDefault, inFileName, CFStringGetSystemEncoding());
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
