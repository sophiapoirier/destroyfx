#include "dfxguitools.h"


/***********************************************************************
	DGItem
	base class for items contained in the editor's linked list of GUI objects
***********************************************************************/

//-----------------------------------------------------------------------------
DGItem::DGItem()
{
	type = kDfxGuiType_none;
	itemID = 0;
	prev = NULL;
	next = NULL;
}

//-----------------------------------------------------------------------------
DGItem::~DGItem()
{
	if (next != NULL)
		delete next;
	next = NULL;
}

//-----------------------------------------------------------------------------
DGItem * DGItem::getByID(UInt32 inID)
{
	if (itemID == inID)
		return this;
	else if (next == NULL)
		return NULL;
	else
		return next->getByID(inID);
}

//-----------------------------------------------------------------------------
void DGItem::append(DGItem *nextItem)
{
	if (next == NULL)
	{
		next = nextItem;
		nextItem->setPrev(this);
	}
	else
		next->append(nextItem);
}



/***********************************************************************
	DGGraphic
	class for loading and containing images
***********************************************************************/

//-----------------------------------------------------------------------------
DGGraphic::DGGraphic(const char *inFileName)
{
	cgImage = NULL;
	loadImageFile(inFileName);

	setType(kDfxGuiType_graphic);
}

//-----------------------------------------------------------------------------
DGGraphic::~DGGraphic()
{
	if (cgImage != NULL)
		CGImageRelease(cgImage);
	cgImage = NULL;
}

//-----------------------------------------------------------------------------
void DGGraphic::loadImageFile(const char *inFileName)
{
	// no assumptions can be made about how long the reference is valid, 
	// and the caller should not attempt to release the CFBundleRef object
	CFBundleRef pluginBundleRef = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));
	if (pluginBundleRef != NULL)
	{
		CFStringRef fileCFName = CFStringCreateWithCString(kCFAllocatorDefault, inFileName, CFStringGetSystemEncoding());
		if (fileCFName != NULL)
		{
			CFURLRef imageResourceURL = CFBundleCopyResourceURL(pluginBundleRef, fileCFName, NULL, NULL);
			if (imageResourceURL != NULL)
			{
				CGDataProviderRef provider = CGDataProviderCreateWithURL(imageResourceURL);
				if (provider != NULL)
				{
//					char *fileExtension = strrchr(inFileName, '.');
//					if (fileExtension != NULL)
//						fileExtension += 1;	// advance past the .

					CFStringRef fileCFExtension = CFURLCopyPathExtension(imageResourceURL);
					if (fileCFExtension != NULL)
					{
						if (CFStringCompare(fileCFExtension, CFSTR("png"), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
							cgImage = CGImageCreateWithPNGDataProvider(provider, NULL, false, kCGRenderingIntentDefault);
						else if ( (CFStringCompare(fileCFExtension, CFSTR("jpg"), kCFCompareCaseInsensitive) == kCFCompareEqualTo) || 
								(CFStringCompare(fileCFExtension, CFSTR("jpeg"), kCFCompareCaseInsensitive) == kCFCompareEqualTo) || 
								(CFStringCompare(fileCFExtension, CFSTR("jpe"), kCFCompareCaseInsensitive) == kCFCompareEqualTo) )
							cgImage = CGImageCreateWithJPEGDataProvider(provider, NULL, false, kCGRenderingIntentDefault);
						CFRelease(fileCFExtension);
					}

					CGDataProviderRelease(provider);
				}
				CFRelease(imageResourceURL);
			}
			CFRelease(fileCFName);
		}
	}
}

//-----------------------------------------------------------------------------
SInt32 DGGraphic::getWidth()
{
	if (cgImage != NULL)
		return CGImageGetWidth(cgImage);
	else
		return 1;
}
	
//-----------------------------------------------------------------------------
SInt32 DGGraphic::getHeight()
{
	if (cgImage != NULL)
		return CGImageGetHeight(cgImage);
	else
		return 1;
}

//-----------------------------------------------------------------------------
void DGGraphic::draw(CGContextRef context, UInt32 portHeight, DGRect *inRect, float value)
{
	if (cgImage != NULL)
		CGContextDrawImage(context, inRect->convertToCGRect(portHeight), cgImage);
}
