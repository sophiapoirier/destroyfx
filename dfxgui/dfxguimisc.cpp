#include "dfxguitools.h"


/***********************************************************************
	DGItem
	base class for items contained in the editor's linked list of GUI objects
***********************************************************************/

//-----------------------------------------------------------------------------
DGItem::DGItem()
{
	this->type = kDfxGuiType_None;
	this->itemID = 0;
	this->prev = NULL;
	this->next = NULL;
}

//-----------------------------------------------------------------------------
DGItem::~DGItem()
{
	if (this->next != NULL)
		delete this->next;
	this->next = NULL;
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
DGGraphic::DGGraphic(char *fileName)
{
	cgImage = NULL;
	loadImagePNG(fileName);

	setType(kDfxGuiType_Graphic);
}

//-----------------------------------------------------------------------------
DGGraphic::~DGGraphic()
{
	if (cgImage != NULL)
		CGImageRelease(cgImage);
	cgImage = NULL;
}

//-----------------------------------------------------------------------------
void DGGraphic::loadImagePNG(char *fileName)
{
	// no assumptions can be made about how long the reference is valid, 
	// and the caller should not attempt to release the CFBundleRef object
	CFBundleRef pluginBundleRef = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));
	if (pluginBundleRef != NULL)
	{
		short tempres = CurResFile();

		CFStringRef fileCFName = CFStringCreateWithCString(kCFAllocatorDefault, fileName, CFStringGetSystemEncoding());
		if (fileCFName != NULL)
		{
			CFURLRef resourceURL = CFBundleCopyResourceURL(pluginBundleRef, fileCFName, NULL, NULL);
			if (resourceURL != NULL)
			{
				CGDataProviderRef provider = CGDataProviderCreateWithURL(resourceURL);
				if (provider != NULL)
				{
					cgImage = CGImageCreateWithPNGDataProvider(provider, NULL, false,  kCGRenderingIntentDefault);
					CGDataProviderRelease(provider);
				}
				CFRelease(resourceURL);
			}
			CFRelease(fileCFName);
		}

		 UseResFile(tempres);
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
