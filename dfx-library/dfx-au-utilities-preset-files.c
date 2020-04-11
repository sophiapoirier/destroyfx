/*
	Destroy FX AU Utilities is a collection of helpful utility functions 
	for creating and hosting Audio Unit plugins.
	Copyright (C) 2003-2020  Sophia Poirier
	All rights reserved.
	
	Redistribution and use in source and binary forms, with or without 
	modification, are permitted provided that the following conditions 
	are met:
	
	*	Redistributions of source code must retain the above 
		copyright notice, this list of conditions and the 
		following disclaimer.
	*	Redistributions in binary form must reproduce the above 
		copyright notice, this list of conditions and the 
		following disclaimer in the documentation and/or other 
		materials provided with the distribution.
	*	Neither the name of Destroy FX nor the names of its 
		contributors may be used to endorse or promote products 
		derived from this software without specific prior 
		written permission.
	
	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
	FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE 
	COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
	STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
	OF THE POSSIBILITY OF SUCH DAMAGE.
	
	To contact the author, please visit http://destroyfx.org/ 
	and use the contact form.
*/


#ifndef __OBJC__
	#error "you must compile the version of this file with a .m filename extension, not this file"
#endif

#include "dfx-au-utilities.h"

#import <AppKit/NSAlert.h>
#import <AppKit/NSButton.h>
#import <AppKit/NSImage.h>
#include <AudioToolbox/AudioUnitUtilities.h>  // for kAUParameterListener_AnyParameter

#include "dfx-au-utilities-private.h"


//--------------------------------------------------------------------------
// the file name extension that identifies a file as being an AU preset file
#define kAUPresetFileNameExtension	CFSTR("aupreset")
// the name of the directory in Library/Audio for AU preset files
#define kAUPresetsDirName	CFSTR("Presets")



#pragma mark -
#pragma mark Handling Files And Directories

//-----------------------------------------------------------------------------
// This function provides an FSRef for an AU Component's centralized preset files directory.
// 
// inAUComponent - The Component (must be an Audio Unit Component) whose preset directory you want.
//		You can cast a ComponentInstance or AudioUnit to Component for this argument.
// inFileSystemDomain - The file system domain of the presets directory (there can be one in every valid domain).
// inCreateDir - If true, then create directories in the path to the location if they do not already exist.  
//		Otherwise, this function will fail if all of the directories in the complete path do not already exist.
// outDirRef - On successful return, this will point to a valid FSRef of the AU's presets directory.
OSStatus FindPresetsDirForAU(Component inAUComponent, FSVolumeRefNum inFileSystemDomain, Boolean inCreateDir, FSRef* outDirRef)
{
	OSStatus status = noErr;
	FSRef audioDirRef, presetsDirRef, manufacturerPresetsDirRef;
	CFStringRef pluginNameCFString = NULL, manufacturerNameCFString = NULL;

	if ((inAUComponent == NULL) || (outDirRef == NULL))
	{
		return kAudio_ParamError;
	}

	// get an FSRef for the Audio support directory (in Library)
	status = FSFindFolder(inFileSystemDomain, kAudioSupportFolderType, inCreateDir, &audioDirRef);
	if (status != noErr)
	{
		return status;
	}

	// get an FSRef to the Presets directory in the Audio directory
	status = MakeDirectoryFSRef(&audioDirRef, kAUPresetsDirName, inCreateDir, &presetsDirRef);
	if (status != noErr)
	{
		return status;
	}

	// determine the name of the AU and the AU manufacturer so that we know the names of the presets sub-directories
	pluginNameCFString = manufacturerNameCFString = NULL;
	status = CopyAUNameAndManufacturerStrings(inAUComponent, &pluginNameCFString, &manufacturerNameCFString);
	if (status != noErr)
	{
		return status;
	}
	if ((manufacturerNameCFString == NULL) || (pluginNameCFString == NULL))
	{
		return coreFoundationUnknownErr;
	}

	// get an FSRef to the AU manufacturer directory in the Presets directory
	status = MakeDirectoryFSRef(&presetsDirRef, manufacturerNameCFString, inCreateDir, &manufacturerPresetsDirRef);
	if (status == noErr)
	{
		// get an FSRef to the particular plugin's directory in the manufacturer directory
		status = MakeDirectoryFSRef(&manufacturerPresetsDirRef, pluginNameCFString, inCreateDir, outDirRef);
	}
	CFRelease(manufacturerNameCFString);
	CFRelease(pluginNameCFString);

	return status;
}

//--------------------------------------------------------------------------
// This function copies the name of an AU preset file, minus its file name extension and 
// parent directory path, making it nicer for displaying to a user.
// The result is null if something went wrong, otherwise it's 
// a CFString to which the caller owns a reference.
CFStringRef CopyAUPresetNameFromCFURL(CFURLRef inAUPresetFileURL)
{
	CFStringRef baseFileNameString = NULL;
	if (inAUPresetFileURL != NULL)
	{
		// first make a copy of the URL without the file name extension
		CFURLRef const baseNameUrl = CFURLCreateCopyDeletingPathExtension(kCFAllocatorDefault, inAUPresetFileURL);
		if (baseNameUrl != NULL)
		{
			// then chop off the parent directory path, keeping just the extensionless file name as a CFString
			baseFileNameString = CFURLCopyLastPathComponent(baseNameUrl);
			CFRelease(baseNameUrl);
		}
	}
	return baseFileNameString;
}

//--------------------------------------------------------------------------
// This function tells you if a file represented by a CFURL is an AU preset file.  
// This is determined by looking at the file name extension.
// The result is true if the file is an AU preset, false otherwise.
Boolean CFURLIsAUPreset(CFURLRef inURL)
{
	Boolean result = false;
	if (inURL != NULL)
	{
		CFStringRef const fileNameExtension = CFURLCopyPathExtension(inURL);
		if (fileNameExtension != NULL)
		{
			result = (CFStringCompare(fileNameExtension, kAUPresetFileNameExtension, kCFCompareCaseInsensitive) == kCFCompareEqualTo);
			CFRelease(fileNameExtension);
		}
	}
	return result;
}

//--------------------------------------------------------------------------
// This function tells you if a file represented by an FSRef is an AU preset file.
// The result is true if the file is an AU preset, false otherwise.
// This function is a convenience wrapper that just converts the FSRef to a CFURL 
// and then calls CFURLIsAUPreset.
Boolean FSRefIsAUPreset(FSRef const* inFileRef)
{
	Boolean result = false;
	if (inFileRef != NULL)
	{
		CFURLRef const fileUrl = CFURLCreateFromFSRef(kCFAllocatorDefault, inFileRef);
		if (fileUrl != NULL)
		{
			result = CFURLIsAUPreset(fileUrl);
			CFRelease(fileUrl);
		}
	}
	return result;
}

//-----------------------------------------------------------------------------
// This function will, given a reference to a parent directory and a name for 
// a child file or directory item, create a reference for that child item 
// in the parent directory.
// If the child item does not already exist and inCreateItem is true, 
// then this function will attempt to create the item.  
// If the child item does not already exist and inCreateItem is false, 
// then this function will fail.  
// Upon successful return, outItemRef points to a valid FSRef of the requested 
// child item and noErr is returned.  Otherwise, an error code is returned and 
// the FSRef pointed to by outItemRef is not altered.
OSStatus MakeDirectoryFSRef(FSRef const* inParentDirRef, CFStringRef inItemNameString, Boolean inCreateItem, FSRef* outItemRef)
{
	OSStatus status = noErr;
	HFSUniStr255 itemUniName = {0};

	if ((inParentDirRef == NULL) || (inItemNameString == NULL) || (outItemRef == NULL))
	{
		return kAudio_ParamError;
	}

	// first we need to convert the CFString of the file name to a HFS-style unicode file name
	TranslateCFStringToUnicodeString(inItemNameString, &itemUniName);
	// then we try to create an FSRef for the file
	status = FSMakeFSRefUnicode(inParentDirRef, itemUniName.length, itemUniName.unicode, kTextEncodingUnknown, outItemRef);
	// FSRefs can only be created for existing files or directories.  So making the FSRef failed 
	// and if creating missing items is desired, then try to create the child item.
	if ((status != noErr) && inCreateItem)
	{
		status = FSCreateDirectoryUnicode(inParentDirRef, itemUniName.length, itemUniName.unicode, kFSCatInfoNone, NULL, outItemRef, NULL, NULL);
	}

	return status;
}

//-----------------------------------------------------------------------------
// convert a CFString to a unicode HFS file name string
void TranslateCFStringToUnicodeString(CFStringRef inCFString, HFSUniStr255* outUniName)
{
	if ((inCFString != NULL) && (outUniName != NULL))
	{
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4
	#if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_4
		if (FSGetHFSUniStrFromString != NULL)
	#endif
		{
			OSStatus const status = FSGetHFSUniStrFromString(inCFString, outUniName);
			if (status == noErr)
			{
				return;
			}
		}
#endif
		size_t const maxUnicodeFileNameLength = sizeof(outUniName->unicode) / sizeof(outUniName->unicode[0]);
		size_t const cfNameLength = (size_t)CFStringGetLength(inCFString);
		// the length can't be more than 255 characters for an HFS file name
		if (cfNameLength > maxUnicodeFileNameLength)
		{
			CFIndex characterIndex = 0;
			while ((size_t)characterIndex <= maxUnicodeFileNameLength)
			{
				CFRange const composedCharacterRange = CFStringGetRangeOfComposedCharactersAtIndex(inCFString, characterIndex);
				outUniName->length = (u_int16_t)characterIndex;
				characterIndex = composedCharacterRange.location + composedCharacterRange.length;
			}
		}
		else
		{
			outUniName->length = (u_int16_t)cfNameLength;
		}
		// translate the CFString to a unicode string representation in the HFS file name string
		CFStringGetCharacters(inCFString, CFRangeMake(0, (CFIndex)(outUniName->length)), outUniName->unicode);
	}
}



#pragma mark -
#pragma mark Preset Files Tree

//-----------------------------------------------------------------------------
CFTreeRef CFTreeCreateFromAUPresetFilesInDomain(Component inAUComponent, FSVolumeRefNum inFileSystemDomain)
{
	OSStatus status = noErr;
	FSRef presetsDirRef;
	CFTreeRef tree = NULL;

	if (inAUComponent == NULL)
	{
		return NULL;
	}

	// first we need to find the directory for the AU's preset files in this domain
	status = FindPresetsDirForAU(inAUComponent, inFileSystemDomain, kDontCreateFolder, &presetsDirRef);
	if (status != noErr)
	{
		return NULL;
	}

	// we start with a root tree that represents the parent directory of all AU preset files for this AU
	tree = CreateFileURLsTreeNode(&presetsDirRef, kCFAllocatorDefault);
	if (tree != NULL)
	{
		// this will recursively traverse the preset files directory and all subdirectories and 
		// add tree nodes for all of those to the root tree
		CollectAllAUPresetFilesInDir(&presetsDirRef, tree, inAUComponent);
		// it's nice to alphabetize the tree nodes in each level of the tree, don't you think?
//		CFTreeSortChildren(tree, FileURLsTreeComparatorFunction, NULL);
		SortCFTreeRecursively(tree, FileURLsTreeComparatorFunction, NULL);

		// This means that the presets directory for this plugin exists, but there are no preset files in it.
		// The tree is of no use in this case, so we're better off releasing it and just returning null.
		if (CFTreeGetChildCount(tree) <= 0)
		{
			CFRelease(tree);
			tree = NULL;
		}
	}

	return tree;
}

//-----------------------------------------------------------------------------
// this is just a convenient way to get a reference a tree node's info pointer in its context, 
// which in the case of our CFURLs trees, is a CFURLRef
CFURLRef GetCFURLFromFileURLsTreeNode(CFTreeRef inTree)
{
	CFTreeContext treeContext;
	if (inTree == NULL)
	{
		return NULL;
	}
	// first get the context for this tree node
	treeContext.version = 0;  // supposedly you have to make sure that the version value is correct on input
	CFTreeGetContext(inTree, &treeContext);
	// the info pointer in the context is a CFURLRef
	return (CFURLRef)(treeContext.info);
}

//-----------------------------------------------------------------------------
// given an FSRef to a file or directory and a CFAllocatorRef, this function will 
// initialize a CFTreeContext appropriately for our file URLs trees, create a CFURL 
// out of the FSRef for the tree node, and then create and return the new tree node, 
// or null if anything went wrong.
CFTreeRef CreateFileURLsTreeNode(FSRef const* inItemRef, CFAllocatorRef inAllocator)
{
	CFTreeRef newNode = NULL;
	if (inItemRef != NULL)
	{
		// we'll need a CFURL representation of the file or directory for the tree
		CFURLRef itemUrl = CFURLCreateFromFSRef(kCFAllocatorDefault, inItemRef);
		if (itemUrl != NULL)
		{
			CFTreeContext treeContext;
			FileURLsCFTreeContext_Init(itemUrl, &treeContext);
			newNode = CFTreeCreate(inAllocator, &treeContext);
			// the CFURL that we created got retained when the tree node was created, so we can release it now
			CFRelease(itemUrl);
		}
	}
	return newNode;
}

//-----------------------------------------------------------------------------
// This function does CreateFileURLsTreeNode and then also adds the new tree node 
// as a child to a parent tree, and if successful, returns a reference to the new tree node.
CFTreeRef AddFileItemToTree(FSRef const* inItemRef, CFTreeRef inParentTree)
{
	CFTreeRef newNode = NULL;
	if ((inItemRef != NULL) && (inParentTree != NULL))
	{
		newNode = CreateFileURLsTreeNode(inItemRef, CFGetAllocator(inParentTree));
		if (newNode != NULL)
		{
			CFTreeAppendChild(inParentTree, newNode);
			// the new tree node was retained by the parent when it was added to the parent, so we can release it now
			CFRelease(newNode);
		}
	}
	return newNode;
}

//-----------------------------------------------------------------------------
// This function will scan through a directory and examine every file that might be 
// an AU preset file.  If a child directory is encountered, then this function is 
// called recursively on that child directory so that we get everything.
// If any child sub-directories don't wind up having any AU preset files, then 
// they get removed from the tree.
// If the inAUComponent argument is not null, then AU preset files will be examined 
// to see if their ComponentDescription data matches that of the AU Component, and 
// if not, then those preset files will be excluded from the tree.
void CollectAllAUPresetFilesInDir(FSRef const* inDirRef, CFTreeRef inParentTree, Component inAUComponent)
{
	OSErr error = noErr;
	FSIterator dirIterator = NULL;

	if ((inDirRef == NULL) || (inParentTree == NULL))
	{
		return;
	}

	// first, we create a directory iterator
	// in order for iterating a non-root-volume's directory contents to 
	// work with FSGetCatalogInfoBulk, we need to use the "flat" option, 
	// which means that sub-directories will not automatically be recursed, 
	// so we will need to do that ourselves
	error = FSOpenIterator(inDirRef, kFSIterateFlat, &dirIterator);
	// there's nothing for us to do if that failed
	if (error != noErr)
	{
		goto checkEmptyTree;
	}

	// this is the main loop through each directory item using the FS iterator
	do
	{
		// this will be set to the actual number of items returned by FSGetCatalogInfoBulk
		ItemCount actualNumItems = 0;
		// the info that we want:  node flags to check if the item is a directory, and finder info to get the HFS file type code
		FSCatalogInfoBitmap const infoRequestFlags = kFSCatInfoNodeFlags;
		// use FSGetCatalogInfoBulk to get those bits of FS catalog info and the FSRef for the current item
		FSCatalogInfo itemCatalogInfo;
		FSRef itemFSRef;
		error = FSGetCatalogInfoBulk(dirIterator, 1, &actualNumItems, NULL, infoRequestFlags, &itemCatalogInfo, &itemFSRef, NULL, NULL);
		// we need to have no error and to have retrieved an item's info
		if ((error == noErr) && (actualNumItems > 0))
		{
			// if the current item itself is a directory, then we recursively call this function on that sub-directory
			if (itemCatalogInfo.nodeFlags & kFSNodeIsDirectoryMask)
			{
				CFTreeRef const newSubTree = AddFileItemToTree(&itemFSRef, inParentTree);
				CollectAllAUPresetFilesInDir(&itemFSRef, newSubTree, inAUComponent);
			}
			// otherwise it's a file, so we add it (if it is an AU preset file)
			else
			{
				// only add this file if file name extension indicates that this is an AU preset file
				if (FSRefIsAUPreset(&itemFSRef))
				{
					// XXX should I also open and examine each file and make sure that it's a valid dictionary for the particular AU?
#if 1
					// we only do this examination if an AU Component argument was given
					if (inAUComponent != NULL)
					{
						CFURLRef const itemUrl = CFURLCreateFromFSRef(kCFAllocatorDefault, &itemFSRef);
						if (itemUrl != NULL)
						{
							// get the ComponentDescription from the AU preset file
							ComponentDescription presetDesc;
							OSStatus const status = GetAUComponentDescriptionFromPresetFile(itemUrl, &presetDesc);
							CFRelease(itemUrl);
							if (status == noErr)
							{
								// if the preset's ComponentDescription doesn't match the AU's, then don't add this preset file
								if (!ComponentAndDescriptionMatch_Loosely(inAUComponent, &presetDesc))
								{
									continue;
								}
							}
						}
					}
#endif
					AddFileItemToTree(&itemFSRef, inParentTree);
				}
			}
		}
	// the iteration loop keeps going until an error is returned (probably meaning "no more items")
	} while (error == noErr);

	// clean up
	FSCloseIterator(dirIterator);

checkEmptyTree:
	// if no items got added to the parent, then there's no point in keeping the parent tree, since it's empty...
	if (CFTreeGetChildCount(inParentTree) <= 0)
	{
		// ... unless the parent is actually the root tree, in which case we would break our other code if we release that
		if (CFTreeFindRoot(inParentTree) != inParentTree)
		{
			CFTreeRemove(inParentTree);
		}
	}
}

//-----------------------------------------------------------------------------
// Since CFTreeSortChildren only sorts 1 level of a tree and is not recurive, 
// this function allows us to do a recursive sorting of all sub-levels of a tree.
void SortCFTreeRecursively(CFTreeRef inTreeRoot, CFComparatorFunction inComparatorFunction, void* inContext)
{
	if ((inTreeRoot != NULL) && (inComparatorFunction != NULL))
	{
		CFTreeRef child, next;
		// first sort through our own tree level
		CFTreeSortChildren(inTreeRoot, inComparatorFunction, inContext);
		// then sort through any child levels recursively
		child = CFTreeGetFirstChild(inTreeRoot);
		if (child != NULL)
		{
			SortCFTreeRecursively(child, inComparatorFunction, inContext);
		}
		// and sort the next node in our own level recursively, too
		next = CFTreeGetNextSibling(inTreeRoot);
		if (next != NULL)
		{
			SortCFTreeRecursively(next, inComparatorFunction, inContext);
		}
	}
}

//-----------------------------------------------------------------------------
// This will compare nodes of one of our file URLs trees to see if one is equal to, 
// less than, or greater than the other.  This is based on the file or directory name, 
// alphabetically.  This comparator function is used when sorting one of these trees.
CFComparisonResult FileURLsTreeComparatorFunction(void const* inTree1, void const* inTree2, void* inContext)
{
	CFTreeRef const tree1 = (CFTreeRef)inTree1, tree2 = (CFTreeRef)inTree2;
	CFURLRef const url1 = GetCFURLFromFileURLsTreeNode(tree1);
	CFURLRef const url2 = GetCFURLFromFileURLsTreeNode(tree2);
	CFComparisonResult result = kCFCompareEqualTo;
	if ((url1 != NULL) && (url2 != NULL))
	{
		// we'll use the name of just the file or directory, not the whole path
		CFStringRef const fileNameString1 = CFURLCopyLastPathComponent(url1);
		CFStringRef const fileNameString2 = CFURLCopyLastPathComponent(url2);

		// and just rely on CFStringCompare to do the comparison, case-insensitively
		if ((fileNameString1 != NULL) && (fileNameString2 != NULL))
		{
			result = CFStringCompare(fileNameString1, fileNameString2, kCFCompareCaseInsensitive);
		}

		if (fileNameString1 != NULL)
		{
			CFRelease(fileNameString1);
		}
		if (fileNameString2 != NULL)
		{
			CFRelease(fileNameString2);
		}
	}

	// go recursive on this tree
	// XXX note:  this is inefficient and will result in many redundant sorting passes 
	// every time a node is compared more than once to some other node, but I think that that's okay
/*
	if (CFTreeGetChildCount(tree1) > 0)
	{
		CFTreeSortChildren(tree1, FileURLsTreeComparatorFunction, NULL);
	}
	if (CFTreeGetChildCount(tree2) > 0)
	{
		CFTreeSortChildren(tree2, FileURLsTreeComparatorFunction, NULL);
	}
*/

	return result;
}

//-----------------------------------------------------------------------------
// this will initialize a CFTree context structure to use the above callback functions
void FileURLsCFTreeContext_Init(CFURLRef inURL, CFTreeContext* outTreeContext)
{
	if ((outTreeContext == NULL) || (inURL == NULL))
	{
		return;
	}
	// wipe the struct clean
	memset(outTreeContext, 0, sizeof(*outTreeContext));
	// set all of the values and function pointers in the callbacks struct
	outTreeContext->version = 0;
	outTreeContext->info = (void*)inURL;
	// the info is a CFType, so we can just use the regular CF functions for these
	outTreeContext->retain = CFRetain;
	outTreeContext->release = CFRelease;
	outTreeContext->copyDescription = CFCopyDescription;
}



#pragma mark -
#pragma mark Restoring Preset Files

//-----------------------------------------------------------------------------
// input:  Audio Unit ComponentInstance, CFURL to AU preset file to restore
// output:  OSStatus error code
// Given an AU instance and an AU preset file, this function will try to restore the data 
// from the preset file as the new state of the AU.
OSStatus RestoreAUStateFromPresetFile(AudioUnit inAUComponentInstance, CFURLRef inAUPresetFileURL)
{
	SInt32 plistError = 0;
	OSStatus auError = noErr;
	CFPropertyListRef auStatePlist = NULL;

	if ((inAUComponentInstance == NULL) || (inAUPresetFileURL == NULL))
	{
		return kAudio_ParamError;
	}

	// the preset file's state data is stored as XML data, and so we first we need to convert it to a PropertyList
	plistError = 0;
	auStatePlist = CreatePropertyListFromXMLFile(inAUPresetFileURL, &plistError);
	if (auStatePlist == NULL)
	{
		return (plistError != 0) ? plistError : coreFoundationUnknownErr;
	}

	// attempt to apply the state data from the file to the AU
	auError = AudioUnitSetProperty(inAUComponentInstance, kAudioUnitProperty_ClassInfo, 
								   kAudioUnitScope_Global, (AudioUnitElement)0, &auStatePlist, sizeof(auStatePlist));

	CFRelease(auStatePlist);

	// in case the AU itself or you don't already do this upon restoring settings, 
	// it is necessary to send out notifications to any parameter listeners so that 
	// all parameter controls and whatnot reflect the new state
	if (auError == noErr)
	{
		AUParameterChange_TellListeners_ScopeElement(inAUComponentInstance, (AudioUnitParameterID)kAUParameterListener_AnyParameter, kAudioUnitScope_Global, (AudioUnitElement)0);
		AUParameterChange_TellListeners_ScopeElement(inAUComponentInstance, (AudioUnitParameterID)kAUParameterListener_AnyParameter, kAudioUnitScope_Input, (AudioUnitElement)0);
		AUParameterChange_TellListeners_ScopeElement(inAUComponentInstance, (AudioUnitParameterID)kAUParameterListener_AnyParameter, kAudioUnitScope_Output, (AudioUnitElement)0);
	}

	return auError;
}

//-----------------------------------------------------------------------------
// Given an URL to a file with XML data contents, this function will attempt to convert that file's data 
// to a PropertyList.  If successful, a new PropertyList is returned, otherwise null is returned.  
// The caller owns a reference to the returned PropertyList and is responsible for releasing it.
// The outErrorCode argument can be null.  If it is not null, it *might* hold a non-zero error code 
// upon failed return, but this is not guaranteed, so you should set the variable's value to 0 
// before calling this, since the value reference by outErrorCode may not actually be altered..
CFPropertyListRef CreatePropertyListFromXMLFile(CFURLRef inXMLFileURL, SInt32* outErrorCode)
{
	CFReadStreamRef fileStream = NULL;
	CFPropertyListRef propertyList = NULL;
	Boolean success = false;
	CFErrorRef error = NULL;

	// allow a null input for outErrorCode, and just point it to something, for safety
	SInt32 dummyErrorCode;
	if (outErrorCode == NULL)
	{
		outErrorCode = &dummyErrorCode;
	}
	*outErrorCode = noErr;

	if (inXMLFileURL == NULL)
	{
		*outErrorCode = kAudio_ParamError;
		return NULL;
	}

	// open the XML file
	fileStream = CFReadStreamCreateWithFile(kCFAllocatorDefault, inXMLFileURL);
	if (fileStream == NULL)
	{
		*outErrorCode = coreFoundationUnknownErr;
		return NULL;
	}
	success = CFReadStreamOpen(fileStream);
	if (!success)
	{
		*outErrorCode = coreFoundationUnknownErr;
		CFRelease(fileStream);
		return NULL;
	}

	// recreate the dictionary using the file data
	propertyList = CFPropertyListCreateWithStream(kCFAllocatorDefault, fileStream, 0, kCFPropertyListImmutable, NULL, &error);
	if (error != NULL)
	{
		*outErrorCode = CFErrorGetCode(error);
		CFRelease(error);
		propertyList = NULL;
	}

	CFReadStreamClose(fileStream);
	CFRelease(fileStream);

	return propertyList;
}

//-----------------------------------------------------------------------------
// this is just a little helper function used by GetAUComponentDescriptionFromPresetFile()
SInt32 GetDictionarySInt32Value(CFDictionaryRef inAUStateDictionary, CFStringRef inDictionaryKey, Boolean* outSuccess)
{
	CFNumberRef cfNumber = NULL;
	SInt32 numberValue = 0;
	Boolean dummySuccess;

	if (outSuccess == NULL)
	{
		outSuccess = &dummySuccess;
	}
	if ((inAUStateDictionary == NULL) || (inDictionaryKey == NULL))
	{
		*outSuccess = false;
		return 0;
	}

	cfNumber = (CFNumberRef)CFDictionaryGetValue(inAUStateDictionary, inDictionaryKey);
	if (cfNumber == NULL)
	{
		*outSuccess = false;
		return 0;
	}
	*outSuccess = CFNumberGetValue(cfNumber, kCFNumberSInt32Type, &numberValue);
	if (*outSuccess)
	{
		return numberValue;
	}
	return 0;
}

//-----------------------------------------------------------------------------
OSStatus GetAUComponentDescriptionFromStateData(CFPropertyListRef inAUStateData, ComponentDescription* outComponentDescription)
{
	CFDictionaryRef auStateDictionary = NULL;
	ComponentDescription tempDesc = {0};
	SInt32 versionValue = 0;
	Boolean gotValue = false;

	if ((inAUStateData == NULL) || (outComponentDescription == NULL))
	{
		return kAudio_ParamError;
	}

	// the property list for AU state data must be of the dictionary type
	if (CFGetTypeID(inAUStateData) != CFDictionaryGetTypeID())
	{
		return kAudioUnitErr_InvalidPropertyValue;
	}
	auStateDictionary = (CFDictionaryRef)inAUStateData;

	// first check to make sure that the version of the AU state data is one that we know understand
	// XXX should I really do this?  later versions would probably still hold these ID keys, right?
	versionValue = GetDictionarySInt32Value(auStateDictionary, CFSTR(kAUPresetVersionKey), &gotValue);
	if (!gotValue)
	{
		return kAudioUnitErr_InvalidPropertyValue;
	}
#define kCurrentSavedStateVersion 0
	if (versionValue != kCurrentSavedStateVersion)
	{
		return kAudioUnitErr_InvalidPropertyValue;
	}

	// grab the ComponentDescription values from the AU state data
	tempDesc.componentType = (OSType)GetDictionarySInt32Value(auStateDictionary, CFSTR(kAUPresetTypeKey), NULL);
	tempDesc.componentSubType = (OSType)GetDictionarySInt32Value(auStateDictionary, CFSTR(kAUPresetSubtypeKey), NULL);
	tempDesc.componentManufacturer = (OSType)GetDictionarySInt32Value(auStateDictionary, CFSTR(kAUPresetManufacturerKey), NULL);
	// zero values are illegit for specific ComponentDescriptions, so zero for any value means that there was an error
	if ((tempDesc.componentType == 0) || (tempDesc.componentSubType == 0) || (tempDesc.componentManufacturer == 0))
	{
		return kAudioUnitErr_InvalidPropertyValue;
	}

	*outComponentDescription = tempDesc;
	return noErr;
}

//-----------------------------------------------------------------------------
// input:  CFURL of an AU preset file to restore
// output:  Audio Unit ComponentDescription, OSStatus error code
// Given an AU preset file, this function will try to copy the preset's 
// creator AU's ComponentDescription info from the preset file.
OSStatus GetAUComponentDescriptionFromPresetFile(CFURLRef inAUPresetFileURL, ComponentDescription* outComponentDescription)
{
	SInt32 plistError = 0;
	OSStatus status = noErr;
	CFPropertyListRef auStatePlist = NULL;
	ComponentDescription tempDesc = {0};

	if ((inAUPresetFileURL == NULL) || (outComponentDescription == NULL))
	{
		return kAudio_ParamError;
	}

	// the preset file's state data is stored as XML data, and so we first we need to convert it to a PropertyList
	plistError = 0;
	auStatePlist = CreatePropertyListFromXMLFile(inAUPresetFileURL, &plistError);
	if (auStatePlist == NULL)
	{
		return (plistError != 0) ? plistError : coreFoundationUnknownErr;
	}

	status = GetAUComponentDescriptionFromStateData(auStatePlist, &tempDesc);

	CFRelease(auStatePlist);
	if (status == noErr)
	{
		*outComponentDescription = tempDesc;
	}

	return status;
}



#pragma mark -
#pragma mark Saving Preset Files

//-----------------------------------------------------------------------------
// This is a convenience wrapper for SaveAUStateToPresetFile_Bundle for when 
// the caller is the main application.
OSStatus SaveAUStateToPresetFile(AudioUnit inAUComponentInstance, CFStringRef inAUPresetNameString, CFURLRef* outSavedAUPresetFileURL, Boolean inPromptToReplaceFile)
{
	return SaveAUStateToPresetFile_Bundle(inAUComponentInstance, inAUPresetNameString, outSavedAUPresetFileURL, inPromptToReplaceFile, NULL);
}

//-----------------------------------------------------------------------------
// input:
// inAUComponentInstance: Audio Unit ComponentInstance
// inAUPresetNameString is required
// outSavedAUPresetFileURL is optional and can be NULL
// inPromptToReplaceFile specifies whether you want to display dialog prompt about replacing an existing file
// inBundle can be NULL, in which case the main application bundle is used
// output:  OSStatus error code
OSStatus SaveAUStateToPresetFile_Bundle(AudioUnit inAUComponentInstance, CFStringRef inAUPresetNameString, CFURLRef* outSavedAUPresetFileURL, Boolean inPromptToReplaceFile, CFBundleRef inBundle)
{
	OSStatus status = noErr;

	HFSUniStr255 dummyUniName;
	// get the absolute maximum length that a file name can be
	size_t const maxUnicodeNameLength = sizeof(dummyUniName.unicode) / sizeof(UniChar);
	// this is how much longer the file name will be after the AU preset file name extension is appended
	CFIndex const presetFileNameExtensionLength = CFStringGetLength(kAUPresetFileNameExtension) - 1;  // -1 for the . before the extension
	// this is the maximum allowable length of the preset file's name without the extension
	CFIndex const maxNameLength = maxUnicodeNameLength - presetFileNameExtensionLength;

	CFPropertyListRef auStatePlist = NULL;
	CFStringRef presetFileNameString = NULL;
	FSRef presetFileDirRef;
	CFURLRef presetFileDirUrl = NULL;
	CFURLRef presetFileUrl = NULL;

	if ((inAUComponentInstance == NULL) || (inAUPresetNameString == NULL))
	{
		return kAudio_ParamError;
	}

	// null for inBundle means that the caller is the main application, 
	// so set our bundle reference to the main application bundle
	if (inBundle == NULL)
	{
		inBundle = CFBundleGetMainBundle();
	}

	// no file name text (probably the user just accidentally hit Save)
	if (CFStringGetLength(inAUPresetNameString) <= 0)
	{
		return errFSMissingName;
	}

	status = CopyAUStatePropertyList(inAUComponentInstance, &auStatePlist);
	if (status != noErr)
	{
		return status;
	}
	if (auStatePlist == NULL)
	{
		return coreFoundationUnknownErr;
	}
	// set the state data's name value to the requested name
	// do this before we mess around with the file name string (truncating, adding extension, etc.)
	auStatePlist = SetAUPresetNameInStateData(auStatePlist, inAUPresetNameString);

	// if the requested file name is too long, truncate it
	if (CFStringGetLength(inAUPresetNameString) > maxNameLength)
	{
		presetFileNameString = CFStringCreateWithSubstring(kCFAllocatorDefault, inAUPresetNameString, CFRangeMake(0, maxNameLength));
	}
	// otherwise (just to make it so that we CFRelease this either way), just retain the input string
	else
	{
		presetFileNameString = inAUPresetNameString;
		CFRetain(presetFileNameString);
	}
	if (presetFileNameString == NULL)
	{
		CFRelease(auStatePlist);
		return coreFoundationUnknownErr;
	}

	// now we need to get the parent directory of where we will save this file
	status = FindPresetsDirForAU((Component)inAUComponentInstance, kUserDomain, kCreateFolder, &presetFileDirRef);
	if (status != noErr)
	{
		CFRelease(auStatePlist);
		CFRelease(presetFileNameString);
		return status;
	}
	// and convert that into a CFURL so that we can use CoreFoundation's 
	// PropertList and XML APIs for saving the data to a file
	presetFileDirUrl = CFURLCreateFromFSRef(kCFAllocatorDefault, &presetFileDirRef);
	if (presetFileDirUrl == NULL)
	{
		CFRelease(auStatePlist);
		CFRelease(presetFileNameString);
		return coreFoundationUnknownErr;
	}
	// create a CFURL of the requested file name in the proper AU presets directory
	presetFileUrl = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, presetFileDirUrl, presetFileNameString, false);
	CFRelease(presetFileNameString);
	CFRelease(presetFileDirUrl);
	if (presetFileUrl == NULL)
	{
		CFRelease(auStatePlist);
		return coreFoundationUnknownErr;
	}

	// save the preset data to a file
	status = TryToSaveAUPresetFile(inAUComponentInstance, auStatePlist, &presetFileUrl, inPromptToReplaceFile, inBundle);
	CFRelease(auStatePlist);
	if (status != noErr)
	{
		if (presetFileUrl != NULL)
		{
			CFRelease(presetFileUrl);
		}
		return status;
	}

	if (outSavedAUPresetFileURL != NULL)
	{
		*outSavedAUPresetFileURL = presetFileUrl;
	}
	else if (presetFileUrl != NULL)
	{
		CFRelease(presetFileUrl);
	}



	return status;
}

//-----------------------------------------------------------------------------
// This is a convenience wrapper for CustomSaveAUPresetFile_Bundle for when 
// the caller is the main application.
OSStatus CustomSaveAUPresetFile(AudioUnit inAUComponentInstance, CFURLRef inAUPresetFileURL, Boolean inPromptToReplaceFile)
{
	return CustomSaveAUPresetFile_Bundle(inAUComponentInstance, inAUPresetFileURL, inPromptToReplaceFile, NULL);
}

//-----------------------------------------------------------------------------
OSStatus CustomSaveAUPresetFile_Bundle(AudioUnit inAUComponentInstance, CFURLRef inAUPresetFileURL, Boolean inPromptToReplaceFile, CFBundleRef inBundle)
{
	OSStatus status = noErr;
	CFPropertyListRef auStatePlist = NULL;
	CFStringRef auPresetNameString = NULL;

	if ((inAUComponentInstance == NULL) || (inAUPresetFileURL == NULL))
	{
		return kAudio_ParamError;
	}

	// null for inBundle means that the caller is the main application, 
	// so set our bundle reference to the main application bundle
	if (inBundle == NULL)
	{
		inBundle = CFBundleGetMainBundle();
	}

	status = CopyAUStatePropertyList(inAUComponentInstance, &auStatePlist);
	if (status != noErr)
	{
		return status;
	}
	if (auStatePlist == NULL)
	{
		return coreFoundationUnknownErr;
	}

	// set the state data's name value to the requested file name
	auPresetNameString = CopyAUPresetNameFromCFURL(inAUPresetFileURL);
	if (auPresetNameString != NULL)
	{
		auStatePlist = SetAUPresetNameInStateData(auStatePlist, auPresetNameString);
		CFRelease(auPresetNameString);
	}

	// save the preset data to a file
	CFRetain(inAUPresetFileURL);  // retain the caller's URL before the next function could release and replace it
	status = TryToSaveAUPresetFile(inAUComponentInstance, auStatePlist, &inAUPresetFileURL, inPromptToReplaceFile, inBundle);
	if (inAUPresetFileURL != NULL)
	{
		CFRelease(inAUPresetFileURL);  // release the extra retain on the URL (possibly the caller's, or possibly new)
	}

	CFRelease(auStatePlist);

	return status;
}

//-----------------------------------------------------------------------------
OSStatus CopyAUStatePropertyList(AudioUnit inAUComponentInstance, CFPropertyListRef* outAUStatePlist)
{
	OSStatus status = noErr;
	UInt32 auStateDataSize = sizeof(*outAUStatePlist);

	if ((inAUComponentInstance == NULL) || (outAUStatePlist == NULL))
	{
		return kAudio_ParamError;
	}

	*outAUStatePlist = NULL;
	status = AudioUnitGetProperty(inAUComponentInstance, kAudioUnitProperty_ClassInfo, 
								  kAudioUnitScope_Global, (AudioUnitElement)0, outAUStatePlist, &auStateDataSize);
	if (status != noErr)
	{
		return status;
	}
	if (*outAUStatePlist == NULL)
	{
		return coreFoundationUnknownErr;
	}
	if (auStateDataSize != sizeof(*outAUStatePlist))
	{
		return coreFoundationUnknownErr;
	}

	return status;
}

//-----------------------------------------------------------------------------
// This function will take a PropertyList and CFURL referencing a file as input, 
// try to convert the PropertyList to XML data, and then write a file of the XML data 
// out to disk to the file referenced by the input CFURL.
OSStatus WritePropertyListToXMLFile(CFPropertyListRef inPropertyList, CFURLRef inXMLFileURL)
{
	CFIndex errorCode = noErr;
	CFWriteStreamRef fileStream = NULL;
	Boolean success = false;
	CFErrorRef errorRef = NULL;
	CFIndex bytesWritten = 0;

	if ((inPropertyList == NULL) || (inXMLFileURL == NULL))
	{
		return kAudio_ParamError;
	}

	// open the output file for writing
	fileStream = CFWriteStreamCreateWithFile(kCFAllocatorDefault, inXMLFileURL);
	if (fileStream == NULL)
	{
		return coreFoundationUnknownErr;
	}
	success = CFWriteStreamOpen(fileStream);
	if (!success)
	{
		CFRelease(fileStream);
		return coreFoundationUnknownErr;
	}

	// write the property list to the file
	bytesWritten = CFPropertyListWrite(inPropertyList, fileStream, kCFPropertyListXMLFormat_v1_0, 0, &errorRef);
	CFWriteStreamClose(fileStream);
	CFRelease(fileStream);
	if (bytesWritten <= 0)
	{
		if (errorRef != NULL)
		{
			errorCode = CFErrorGetCode(errorRef);
			CFRelease(errorRef);
			return errorCode;
		}
		return coreFoundationUnknownErr;
	}

	return errorCode;
}

//-----------------------------------------------------------------------------
// This function takes an AU's state data, an URL to which to save the preset file, 
// and a CFBundle and then tries to save the AU's state data to the desired path.
OSStatus TryToSaveAUPresetFile(AudioUnit inAUComponentInstance, CFPropertyListRef inAUStateData, CFURLRef* ioAUPresetFileURL, Boolean inPromptToReplaceFile, CFBundleRef inBundle)
{
	OSStatus status = noErr;
	FSRef dummyFSRef;
	Boolean fileAlreadyExists = false;

	if ((inAUStateData == NULL) || (ioAUPresetFileURL == NULL) || (*ioAUPresetFileURL == NULL))
	{
		return kAudio_ParamError;
	}

	// the file name needs the proper AU preset extension to be appended
	if (!CFURLIsAUPreset(*ioAUPresetFileURL))
	{
		CFURLRef const presetFullFileUrl = CFURLCreateCopyAppendingPathExtension(kCFAllocatorDefault, *ioAUPresetFileURL, kAUPresetFileNameExtension);
		if (presetFullFileUrl == NULL)
		{
			return coreFoundationUnknownErr;
		}
		CFRelease(*ioAUPresetFileURL);
		*ioAUPresetFileURL = presetFullFileUrl;
	}

	// check whether or not the file already exists
	// note that, since an FSRef cannot be created for a file that doesn't yet exist, 
	// if this succeeds, then that means that a file with this path and name already exists
	fileAlreadyExists = CFURLGetFSRef(*ioAUPresetFileURL, &dummyFSRef);
	if (fileAlreadyExists && inPromptToReplaceFile)
	{
		// present the user with a dialog asking whether or not they want to replace an already existing file
		// if not, then return with a "user canceled" error code
		if (!ShouldReplaceExistingAUPresetFile(inAUComponentInstance, *ioAUPresetFileURL, inBundle))
		{
			return userCanceledErr;
		}
	}

	// if we've made it this far, then it's time to try to 
	// write the AU state data out to the file as XML data
	status = WritePropertyListToXMLFile(inAUStateData, *ioAUPresetFileURL);

	return status;
}

//-----------------------------------------------------------------------------
// Given an URL represeting a requested-to-save AU preset file, 
// this function creates an alert dialog telling the user that an identically named 
// file already exists and asking the user if she really wants to replace that old file.
// A return value of true means that the user wants to replace the file, and 
// a return value of false means that the user does not want to replace the file.
Boolean ShouldReplaceExistingAUPresetFile(AudioUnit inAUComponentInstance, CFURLRef inAUPresetFileURL, CFBundleRef inBundle)
{
	CFStringRef fileNameString = NULL;
	CFStringRef dirString = NULL;
	CFURLRef dirUrl = NULL;
	CFStringRef titleString = NULL;
	CFStringRef messageString = NULL;
	CFStringRef replaceButtonString = NULL;
	NSButton* replaceButton = NULL;
	CFStringRef cancelButtonString = NULL;
	NSButton* cancelButton = NULL;
	NSAlert* alert = nil;
	NSModalResponse alertResult = 0;

	// come up with a nice and informative message text for the dialog
	fileNameString = CFURLCopyLastPathComponent(inAUPresetFileURL);
	// if we got the string for the file name, then we can make a nice and specific dialog message
	if (fileNameString != NULL)
	{
		CFStringRef const titleOutlineString = CFCopyLocalizedStringFromTableInBundle(CFSTR("\\U201C%@\\U201D already exists.  Do you want to replace it?"), 
																					  CFSTR("dfx-au-utilities"), inBundle,
																					  CFSTR("title of the alert, specifying file name, when a save file file-already-exists conflict arises"));
		titleString = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, titleOutlineString, fileNameString);
		CFRelease(fileNameString);
		CFRelease(titleOutlineString);
	}
	// otherwise we have to make it a general message
	if (titleString == NULL)
	{
		titleString = CFCopyLocalizedStringFromTableInBundle(CFSTR("That already exists.  Do you want to replace it?"), 
															 CFSTR("dfx-au-utilities"), inBundle, 
															 CFSTR("title of the alert, general, for when file name is not available"));
	}

	dirUrl = CFURLCreateCopyDeletingLastPathComponent(kCFAllocatorDefault, inAUPresetFileURL);
	if (dirUrl != NULL)
	{
		dirString = CFURLCopyLastPathComponent(dirUrl);
		CFRelease(dirUrl);
	}
	if (dirString != NULL)
	{
		CFStringRef const messageOutlineString = CFCopyLocalizedStringFromTableInBundle(CFSTR("A file or folder with the same name already exists in the folder %@.  Replacing it will overwrite its current contents."), 
																						CFSTR("dfx-au-utilities"), inBundle, 
																						CFSTR("the message in the alert, specifying the file location"));
		messageString = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, messageOutlineString, dirString);
		CFRelease(dirString);
		CFRelease(messageOutlineString);
	}
	if (messageString == NULL)
	{
		messageString = CFCopyLocalizedStringFromTableInBundle(CFSTR("A file or folder with the same name already exists in the folder.  Replacing it will overwrite its current contents."), 
															   CFSTR("dfx-au-utilities"), inBundle, 
															   CFSTR("the message in the alert, general, for when file location is not available"));
	}

	replaceButtonString = CFCopyLocalizedStringFromTableInBundle(CFSTR("Replace"), CFSTR("dfx-au-utilities"), inBundle, 
																 CFSTR("text for the button that will overwrite an existing file when a save file file-already-exists conflict arises"));
	cancelButtonString = CFCopyLocalizedStringFromTableInBundle(CFSTR("Cancel"), CFSTR("dfx-au-utilities"), inBundle, 
																CFSTR("text for the button to not overwrite an existing file when a save file file-already-exists conflict arises"));

	// now that we have some message text for the user, we can create and show the dialog
	alert = [[NSAlert alloc] init];
	alert.messageText = (__bridge NSString*)titleString;
	alert.informativeText = (__bridge NSString*)messageString;
	alert.alertStyle = NSAlertStyleCritical;

	replaceButton = [alert addButtonWithTitle:(__bridge NSString*)replaceButtonString];
	replaceButton.keyEquivalent = @"r";
	replaceButton.keyEquivalentModifierMask = NSEventModifierFlagCommand;
	replaceButton.window.initialFirstResponder = replaceButton;

	cancelButton = [alert addButtonWithTitle:(__bridge NSString*)cancelButtonString];
	cancelButton.keyEquivalent = @"\r";  // TODO: this makes it the default button, but escape key no longer binds to it

	// get the AU's icon with which to badge the alert, if available
	if (inAUComponentInstance != NULL)
	{
		CFURLRef alertIconUrl = NULL;
		UInt32 propertyDataSize = sizeof(alertIconUrl);
		OSStatus const status = AudioUnitGetProperty(inAUComponentInstance, kAudioUnitProperty_IconLocation, 
													 kAudioUnitScope_Global, (AudioUnitElement)0, 
													 &alertIconUrl, &propertyDataSize);
		if ((status == noErr) && (alertIconUrl != NULL) && (propertyDataSize == sizeof(alertIconUrl)))
		{
			NSImage* const iconImage = [[NSImage alloc] initWithContentsOfURL:(__bridge NSURL*)alertIconUrl];
			CFRelease(alertIconUrl);
			if (iconImage != nil)
			{
				alert.icon = iconImage;
#if !__has_feature(objc_arc)
				[iconImage release];
#endif
			}
		}
	}

	// XXX ideally this would run as a sheet attached to the AU view's window, 
	// but handling the result asynchronously seems overly complicated for this
	alertResult = [alert runModal];
#if !__has_feature(objc_arc)
	[alert release];
#endif

	CFRelease(titleString);
	CFRelease(messageString);
	CFRelease(replaceButtonString);
	CFRelease(cancelButtonString);

	return (alertResult == NSAlertFirstButtonReturn);
}

//-----------------------------------------------------------------------------
CFPropertyListRef SetAUPresetNameInStateData(CFPropertyListRef inAUStateData, CFStringRef inPresetName)
{
	if ((inAUStateData == NULL) || (inPresetName == NULL))
	{
		return inAUStateData;
	}

	if (CFGetTypeID(inAUStateData) == CFDictionaryGetTypeID())
	{
		CFMutableDictionaryRef const auStateDataMutableCopy = CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, (CFDictionaryRef)inAUStateData);
		if (auStateDataMutableCopy != NULL)
		{
			CFDictionarySetValue(auStateDataMutableCopy, CFSTR(kAUPresetNameKey), inPresetName);
			CFRelease(inAUStateData);
			return (CFPropertyListRef)auStateDataMutableCopy;
		}
	}

	return inAUStateData;
}
