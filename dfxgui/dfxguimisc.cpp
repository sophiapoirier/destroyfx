#include "dfxguitools.h"
#include "dfxgui.h"


/***********************************************************************
	DGImage
	class for loading and containing images
***********************************************************************/

//-----------------------------------------------------------------------------
DGImage::DGImage(const char * inFileName, DfxGuiEditor * inEditor)
{
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
							(CFStringCompare(fileCFExtension, CFSTR("jpe"), kCFCompareCaseInsensitive) == kCFCompareEqualTo) )
						cgImage = CGImageCreateWithJPEGDataProvider(provider, NULL, shouldInterpolate, kCGRenderingIntentDefault);
					CFRelease(fileCFExtension);
				}
				CGDataProviderRelease(provider);

				// create an uncompressed, alpha-premultiplied bitmap image in memory
				if (cgImage != NULL)
					cgImage = PreRenderCGImageBuffer(cgImage);
			}
			CFRelease(imageResourceURL);
		}
	}
	if (fileCFName != NULL)
		CFRelease(fileCFName);

	if (inEditor != NULL)
		inEditor->addImage(this);
}

//-----------------------------------------------------------------------------
DGImage::~DGImage()
{
	if (cgImage != NULL)
		CGImageRelease(cgImage);
	cgImage = NULL;
}

//-----------------------------------------------------------------------------
unsigned long DGImage::getWidth()
{
	if (cgImage != NULL)
		return CGImageGetWidth(cgImage);
	else
		return 1;	// XXX sometimes safer than returning 0
}
	
//-----------------------------------------------------------------------------
unsigned long DGImage::getHeight()
{
	if (cgImage != NULL)
		return CGImageGetHeight(cgImage);
	else
		return 1;	// XXX sometimes safer than returning 0
}

//-----------------------------------------------------------------------------
void DGImage::draw(CGContextRef inContext, UInt32 inPortHeight, DGRect * inRect)
{
	if ( (cgImage != NULL) && (inRect != NULL) && (inContext != NULL) )
		CGContextDrawImage(inContext, inRect->convertToCGRect(inPortHeight), cgImage);
}

//-----------------------------------------------------------------------------
void ReleasePreRenderedCGImageBuffer(void *info, const void *data, size_t size)
{
	free((void*)data);
}

//-----------------------------------------------------------------------------
// create an uncompressed, alpha-premultiplied bitmap image in memory
// so far as I can tell, this makes drawing PNG files with CGContextDrawImage about 25 times faster
CGImageRef PreRenderCGImageBuffer(CGImageRef inImage)
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
			// draw image into context
			CGRect drawRect = CGRectMake(0, 0, width, height);
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
