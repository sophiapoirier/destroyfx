/*
	DFX AU Utilities is a collection of helpful utility functions for 
	creating and hosting Audio Unit plugins.
	Copyright (C) 2003  Marc Genung Poirier

	This library is free software; you can redistribute it and/or 
	modify it under the terms of the GNU Lesser General Public 
	License as published by the Free Software Foundation; either 
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful, 
	but WITHOUT ANY WARRANTY; without even the implied warranty of 
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public 
	License along with this library; if not, write to the Free Software 
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

	To contact the author, visit http://destroyfx.org/ and use the contact form.
	Please refer to the file lgpl.txt for the complete license terms.
*/


#include "dfx-au-utilities.h"
#include "dfx-au-utilities-private.h"


//--------------------------------------------------------------------------
// the file name extension that identifies a file as being an AU preset file
#define kAUPresetFileNameExtension	CFSTR("aupreset")
// the name of the directory in Library/Audio for AU preset files
#define kAUPresetsDirName	CFSTR("Presets")



//-----------------------------------------------------------------------------
// this is the file name of the localizable strings file for the text for 
// some of the alert messages and whatnot in this code
#define kDfxLocalizableStringsTable	CFSTR("dfx-au-utilities-localizable")
// This global variable keeps track of the latest bundle that called SaveAUStateToPresetFile_Bundle.  
// We need to reference that bundle in several other places in order to retrieve localizable strings, 
// and it seems pretty safe to just assume that one global reference will work okay, and nicer than 
// passing the bundle ref as another argument to every little function that needs it.
CFBundleRef gCurrentBundle = NULL;



#pragma mark _________Handling_Files_And_Directories_________

//-----------------------------------------------------------------------------
// This function provides an FSRef for an AU Component's centralized preset files directory.
// 
// inAUComponent - The Component (must be an Audio Unit Component) whose preset directory you want.
//		You can cast a ComponentInstance or AudioUnit to Component for this argument.
// inFileSystemDomain - The file system domain of the presets directory (there can be one in every valid domain).
// inCreateDir - If true, then create directories in the path to the location if they do not already exist.  
//		Otherwise, this function will fail if all of the directories in the complete path do not already exist.
// outDirRef - On successful return, this will point to a valid FSRef of the AU's presets directory.
OSStatus FindPresetsDirForAU(Component inAUComponent, short inFileSystemDomain, Boolean inCreateDir, FSRef * outDirRef)
{
	OSStatus error;
	FSRef audioDirRef, presetsDirRef, manufacturerPresetsDirRef;
	CFStringRef pluginNameCFString, manufacturerNameCFString;

	if ( (inAUComponent == NULL) || (outDirRef == NULL) )
		return paramErr;

	// get an FSRef for the Audio support directory (in Library)
	error = FSFindFolder(inFileSystemDomain, kAudioSupportFolderType, inCreateDir, &audioDirRef);
	if (error != noErr)
		return error;

	// get an FSRef to the Presets directory in the Audio directory
	error = MakeFSRefInDir(&audioDirRef, kAUPresetsDirName, inCreateDir, &presetsDirRef);
	if (error != noErr)
		return error;

	// determine the name of the AU and the AU manufacturer so that we know the names of the presets sub-directories
	pluginNameCFString = manufacturerNameCFString = NULL;
	error = CopyAUNameAndManufacturerStrings(inAUComponent, &pluginNameCFString, &manufacturerNameCFString);
	if (error != noErr)
		return error;
	if ( (manufacturerNameCFString == NULL) || (pluginNameCFString == NULL) )
		return coreFoundationUnknownErr;

	// get an FSRef to the AU manufacturer directory in the Presets directory
	error = MakeFSRefInDir(&presetsDirRef, manufacturerNameCFString, inCreateDir, &manufacturerPresetsDirRef);
	if (error == noErr)
		// get an FSRef to the particular plugin's directory in the manufacturer directory
		error = MakeFSRefInDir(&manufacturerPresetsDirRef, pluginNameCFString, inCreateDir, outDirRef);
	CFRelease(manufacturerNameCFString);
	CFRelease(pluginNameCFString);

	return error;
}

//--------------------------------------------------------------------------
// This function copies the name of an AU preset file, minus its file name extension and 
// parent directory path, making it nicer for displaying to a user.
// The result is null if something went wrong, otherwise it's 
// a CFString to which the caller owns a reference.
CFStringRef CopyAUPresetNameFromCFURL(const CFURLRef inAUPresetFileURL)
{
	CFStringRef baseFileNameString = NULL;
	if (inAUPresetFileURL != NULL)
	{
		// first make a copy of the URL without the file name extension
		CFURLRef baseNameUrl = CFURLCreateCopyDeletingPathExtension(kCFAllocatorDefault, inAUPresetFileURL);
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
Boolean CFURLIsAUPreset(const CFURLRef inURL)
{
	Boolean result = false;
	if (inURL != NULL)
	{
		CFStringRef fileNameExtension = CFURLCopyPathExtension(inURL);
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
Boolean FSRefIsAUPreset(const FSRef * inFileRef)
{
	Boolean result = false;
	if (inFileRef != NULL)
	{
		CFURLRef fileUrl = CFURLCreateFromFSRef(kCFAllocatorDefault, inFileRef);
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
// XXX note:  implementation not finished, file items will be created as directories!
// If the child item does not already exist and inCreateItem is false, 
// then this function will fail.  
// Upon successful return, outItemRef points to a valid FSRef of the requested 
// child item and noErr is returned.  Otherwise, an error code is returned and 
// the FSRef pointed to by outItemRef is not altered.
OSStatus MakeFSRefInDir(const FSRef * inParentDirRef, CFStringRef inItemNameString, Boolean inCreateItem, FSRef * outItemRef)
{
	OSStatus error = noErr;
	HFSUniStr255 itemUniName;

	if ( (inParentDirRef == NULL) || (inItemNameString == NULL) || (outItemRef == NULL) )
		return paramErr;

	// first we need to convert the CFString of the file name to a HFS-style unicode file name
	memset(&itemUniName, 0, sizeof(itemUniName));
	TranslateCFStringToUnicodeString(inItemNameString, &itemUniName);
	// then we try to create an FSRef for the file
	error = FSMakeFSRefUnicode(inParentDirRef, itemUniName.length, itemUniName.unicode, kTextEncodingUnknown, outItemRef);
//kTextEncodingUnicodeDefault
//kCFStringEncodingUnicode
	// FSRefs can only be created for existing files or directories.  So making the FSRef failed 
	// and if creating missing items is desired, then try to create the child item.
	if ( (error != noErr) && inCreateItem )
	{
		error = FSCreateDirectoryUnicode(inParentDirRef, itemUniName.length, itemUniName.unicode, kFSCatInfoNone, NULL, outItemRef, NULL, NULL);
		// XXX need a way to differentiate directories and files
//		error = FSCreateFileUnicode(inParentDirRef, itemUniName.length, itemUniName.unicode, kFSCatInfoNone, NULL, outItemRef, NULL);
	}

	return error;
}

//-----------------------------------------------------------------------------
// convert a CFString to a unicode HFS file name string
void TranslateCFStringToUnicodeString(CFStringRef inCFString, HFSUniStr255 * outUniName)
{
	if ( (inCFString != NULL) && (outUniName != NULL) )
	{
		CFIndex cfnamelength = CFStringGetLength(inCFString);
		CFIndex maxlength = sizeof(outUniName->unicode) / sizeof(UniChar);
		// the length can't be more than 255 characters for an HFS file name
		if (cfnamelength > maxlength)
			cfnamelength = maxlength;
		outUniName->length = cfnamelength;
		// translate the CFString to a unicode string representation in the HFS file name string
		CFStringGetCharacters(inCFString, CFRangeMake(0, cfnamelength), outUniName->unicode);
	}
}



#pragma mark _________Preset_Files_Tree_________

//-----------------------------------------------------------------------------
CFTreeRef CFTreeCreateFromAUPresetFilesInDomain(Component inAUComponent, short inFileSystemDomain)
{
	OSStatus error = noErr;
	FSRef presetsDirRef;
	CFTreeRef tree;

	if (inAUComponent == NULL)
		return NULL;

	// first we need to find the directory for the AU's preset files in this domain
	error = FindPresetsDirForAU(inAUComponent, inFileSystemDomain, kDontCreateFolder, &presetsDirRef);
	if (error != noErr)
		return NULL;

	// we start with a root tree that represents the parent directory of all AU preset files for this AU
	tree = CreateFileURLsTreeNode(&presetsDirRef, kCFAllocatorDefault);
	if (tree != NULL)
	{
		// this will recursively traverse the preset files directory and all subdirectories and 
		// add tree nodes for all of those to the root tree
		CollectAllAUPresetFilesInDir(&presetsDirRef, tree);
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
CFURLRef GetCFURLFromFileURLsTreeNode(const CFTreeRef inTree)
{
	CFTreeContext treeContext;
	if (inTree == NULL)
		return NULL;
	// first get the context for this tree node
	treeContext.version = 0;	// supposedly you have to make sure that the version value is correct on input
	CFTreeGetContext(inTree, &treeContext);
	// the info pointer in the context is a CFURLRef
	return (CFURLRef) (treeContext.info);
}

//-----------------------------------------------------------------------------
// given an FSRef to a file or directory and a CFAllocatorRef, this function will 
// initialize a CFTreeContext appropriately for our file URLs trees, create a CFURL 
// out of the FSRef for the tree node, and then create and return the new tree node, 
// or null if anything went wrong.
CFTreeRef CreateFileURLsTreeNode(const FSRef * inItemRef, CFAllocatorRef inAllocator)
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
CFTreeRef AddFileItemToTree(const FSRef * inItemRef, CFTreeRef inParentTree)
{
	CFTreeRef newNode = NULL;
	if ( (inItemRef != NULL) && (inParentTree != NULL) )
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
// If any chil sub-directories don't wind up having any AU preset files, then 
// they get removed from the tree.
void CollectAllAUPresetFilesInDir(const FSRef * inDirRef, CFTreeRef inParentTree)
{
	OSErr error;
	FSIterator dirIterator;

	if ( (inDirRef == NULL) || (inParentTree == NULL) )
		return;

	// first, we create a directory iterator
	// in order for iterating a non-root-volume's directory contents to 
	// work with FSGetCatalogInfoBulk, we need to use the "flat" option, 
	// which means that sub-directories will not automatically be recursed, 
	// so we will need to do that ourselves
	error = FSOpenIterator(inDirRef, kFSIterateFlat, &dirIterator);
	// there's nothing for us to do if that failed
	if (error != noErr)
		goto checkEmptyTree;

	// this is the main loop through each directory item using the FS iterator
	do
	{
		// this will be set to the actual number of items returned by FSGetCatalogInfoBulk
		ItemCount actualNumItems = 0;
		// the info that we want:  node flags to check if the item is a directory, and finder info to get the HFS file type code
		const FSCatalogInfoBitmap infoRequestFlags = kFSCatInfoNodeFlags;
		// use FSGetCatalogInfoBulk to get those bits of FS catalog info and the FSRef for the current item
		FSCatalogInfo itemCatalogInfo;
		FSRef itemFSRef;
		error = FSGetCatalogInfoBulk(dirIterator, 1, &actualNumItems, NULL, infoRequestFlags, &itemCatalogInfo, &itemFSRef, NULL, NULL);
		// we need to have no error and to have retrieved an item's info
		if ( (error == noErr) && (actualNumItems > 0) )
		{
			// if the current item itself is a directory, then we recursively call this function on that sub-directory
			if (itemCatalogInfo.nodeFlags & kFSNodeIsDirectoryMask)
			{
				CFTreeRef newSubTree = AddFileItemToTree(&itemFSRef, inParentTree);
				CollectAllAUPresetFilesInDir(&itemFSRef, newSubTree);
			}
			// otherwise it's a file, so we add it (if it is an AU preset file)
			else
			{
				// only add this file if file name extension indicates that this is an AU preset file
				// XXX should I also open each file and make sure that it's a valid dictionary for the particular AU?
				if ( FSRefIsAUPreset(&itemFSRef) )
					AddFileItemToTree(&itemFSRef, inParentTree);
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
			CFTreeRemove(inParentTree);
	}
}

//-----------------------------------------------------------------------------
// Since CFTreeSortChildren only sorts 1 level of a tree and is not recurive, 
// this function allows us to do a recursive sorting of all sub-levels of a tree.
void SortCFTreeRecursively(CFTreeRef inTreeRoot, CFComparatorFunction inComparatorFunction, void * inContext)
{
	if ( (inTreeRoot != NULL) && (inComparatorFunction != NULL) )
	{
		CFTreeRef child, next;
		// first sort through our own tree level
		CFTreeSortChildren(inTreeRoot, inComparatorFunction, inContext);
		// then sort through any child levels recursively
		child = CFTreeGetFirstChild(inTreeRoot);
		if (child != NULL)
			SortCFTreeRecursively(child, inComparatorFunction, inContext);
		// and sort the next node in our own level recursively, too
		next = CFTreeGetNextSibling(inTreeRoot);
		if (next != NULL)
			SortCFTreeRecursively(next, inComparatorFunction, inContext);
	}
}

//-----------------------------------------------------------------------------
// This will compare nodes of one of our file URLs trees to see if one is equal to, 
// less than, or greater than the other.  This is based on the file or directory name, 
// alphabetically.  This comparator function is used when sorting one of these trees.
CFComparisonResult FileURLsTreeComparatorFunction(const void * inTree1, const void * inTree2, void * inContext)
{
//fprintf(stderr, "\tFileURLsTreeComparatorFunction:\n"); CFShow(inTree1); CFShow(inTree2); fprintf(stderr, "\n");
	CFTreeRef tree1 = (CFTreeRef)inTree1, tree2 = (CFTreeRef)inTree2;
	CFURLRef url1 = GetCFURLFromFileURLsTreeNode(tree1);
	CFURLRef url2 = GetCFURLFromFileURLsTreeNode(tree2);
	CFComparisonResult result = kCFCompareEqualTo;
	if ( (url1 != NULL) && (url2 != NULL) )
	{
		// we'll use the name of just the file or directory, not the whole path
		CFStringRef fileNameString1 = CFURLCopyLastPathComponent(url1);
		CFStringRef fileNameString2 = CFURLCopyLastPathComponent(url2);

		// and just rely on CFStringCompare to do the comparison, case-insensitively
		if ( (fileNameString1 != NULL) && (fileNameString2 != NULL) )
			result = CFStringCompare(fileNameString1, fileNameString2, kCFCompareCaseInsensitive);

		if (fileNameString1 != NULL)
			CFRelease(fileNameString1);
		if (fileNameString2 != NULL)
			CFRelease(fileNameString2);
	}

	// go recursive on this tree
	// XXX note:  this is inefficient and will result in many redundant sorting passes 
	// every time a node is compared more than once to some other node, but I think that that's okay
/*
	if (CFTreeGetChildCount(tree1) > 0)
		CFTreeSortChildren(tree1, FileURLsTreeComparatorFunction, NULL);
	if (CFTreeGetChildCount(tree2) > 0)
		CFTreeSortChildren(tree2, FileURLsTreeComparatorFunction, NULL);
*/

	return result;
}

//-----------------------------------------------------------------------------
// this will initialize a CFTree context structure to use the above callback functions
void FileURLsCFTreeContext_Init(const CFURLRef inURL, CFTreeContext * outTreeContext)
{
	if ( (outTreeContext == NULL) || (inURL == NULL) )
		return;
	// wipe the struct clean
	memset(outTreeContext, 0, sizeof(*outTreeContext));
	// set all of the values and function pointers in the callbacks struct
	outTreeContext->version = 0;
	outTreeContext->info = (void*) inURL;
	// the info is a CFType, so we can just use the regular CF functions for these
	outTreeContext->retain = CFRetain;
	outTreeContext->release = CFRelease;
	outTreeContext->copyDescription = CFCopyDescription;
}



#pragma mark _________Restoring_Preset_Files_________

//-----------------------------------------------------------------------------
// input:  Audio Unit ComponentInstance, CFURL to AU preset file to restore
// output:  ComponentResult error code
// Given an AU instance and an AU preset file, this function will try to restore the data 
// from the preset file as the new state of the AU.
ComponentResult RestoreAUStateFromPresetFile(AudioUnit inAUComponentInstance, const CFURLRef inAUPresetFileURL)
{
	SInt32 plistError;
	ComponentResult componentError;
	CFPropertyListRef auStatePlist;

	if ( (inAUComponentInstance == NULL) || (inAUPresetFileURL == NULL) )
		return paramErr;

	// the preset file's state data is stored as XML data, and so we first we need to convert it to a PropertyList
	plistError = 0;
	auStatePlist = CreatePropertyListFromXMLFile(inAUPresetFileURL, &plistError);
	if (auStatePlist == NULL)
		return (plistError != 0) ? plistError : coreFoundationUnknownErr;

	// attempt to apply the state data from the file to the AU
	componentError = AudioUnitSetProperty(inAUComponentInstance, kAudioUnitProperty_ClassInfo, 
						kAudioUnitScope_Global, (AudioUnitElement)0, &auStatePlist, sizeof(auStatePlist));

	CFRelease(auStatePlist);

	return componentError;
}

//-----------------------------------------------------------------------------
// Given an URL to a file with XML data contents, this function will attempt to convert that file's data 
// to a PropertyList.  If successful, a new PropertyList is returned, otherwise null is returned.  
// The caller owns a reference to the returned PropertyList and is responsible for releasing it.
// The outErrorCode argument can be null.  If it is not null, it *might* hold a non-zero error code 
// upon failed return, but this is not guaranteed, so you should set the variable's value to 0 
// before calling this, since the value reference by outErrorCode may not actually be altered..
CFPropertyListRef CreatePropertyListFromXMLFile(const CFURLRef inXMLFileURL, SInt32 * outErrorCode)
{
	CFDataRef fileData;
	CFPropertyListRef propertyList;
	Boolean success;
	CFStringRef errorString;

	// allow a null input for outErrorCode, and just point it to something, for safety
	SInt32 dummyErrorCode = 0;
	if (outErrorCode == NULL)
		outErrorCode = &dummyErrorCode;

	if (inXMLFileURL == NULL)
	{
		*outErrorCode = paramErr;
		return NULL;
	}
	
	// read the XML file
	fileData = NULL;
	success = CFURLCreateDataAndPropertiesFromResource(kCFAllocatorDefault, inXMLFileURL, &fileData, NULL, NULL, outErrorCode);
	if ( !success || (fileData == NULL) )
		return NULL;

	// recreate the dictionary using the XML data
	errorString = NULL;
	propertyList = CFPropertyListCreateFromXMLData(kCFAllocatorDefault, fileData, kCFPropertyListImmutable, &errorString);
	if (errorString != NULL)
		CFRelease(errorString);

	CFRelease(fileData);

	return propertyList;
}

// XXX should I put these in the private header?
// unique identifier of our AU preset file open dialogs for Navigation Services
const UInt32 kAUPresetOpenNavDialogKey = 'AUpo';
//-----------------------------------------------------------------------------
// This function is what you use when you want to allow the user to find an AU preset file 
// anywhere on their system and try to open it and restore its data as the current state 
// of the input AU instance.  It presents the user with a Navigation Services GetFile dialog.
ComponentResult CustomRestoreAUPresetFile(AudioUnit inAUComponentInstance)
{
	ComponentResult error = noErr;
	NavDialogCreationOptions dialogOptions;
	NavEventUPP eventProc;
	NavObjectFilterUPP filterProc;
	NavDialogRef dialog;

	if ( (inAUComponentInstance == NULL) )
		return paramErr;

	error = NavGetDefaultDialogCreationOptions(&dialogOptions);
	if (error != noErr)
		return error;
// XXX maybe consider allowing an All Documents option in the Type popup?
//	dialogOptions.optionFlags |= kNavNoTypePopup;
	// disallow multiple file selection (an AU only has one state at a given moment, so that wouldn't make sense)
	dialogOptions.optionFlags &= ~kNavAllowMultipleFiles;
	// this gives this dialog a unique identifier so that it has independent remembered settings for the calling app
	dialogOptions.preferenceKey = kAUPresetOpenNavDialogKey;
//	dialogOptions.modality = kWindowModalityNone;	// XXX is this a good option, or just weird?

	// create and run the standard Navigation Services GetFile dialog to allow the user to 
	// /find and choose an AU preset file to load
	eventProc = NewNavEventUPP(CustomOpenAUPresetNavEventHandler);
	filterProc = NewNavObjectFilterUPP(CustomOpenAUPresetNavFilterProc);
	error = NavCreateGetFileDialog(&dialogOptions, NULL, eventProc, NULL, filterProc, (void*)inAUComponentInstance, &dialog);
	if (error == noErr)
	{
		// XXX do we really want to do this?
		SetNavDialogAUPresetStartLocation(dialog, (Component)inAUComponentInstance, kDontCreateFolder);

		error = NavDialogRun(dialog);
	}
	if (eventProc != NULL)
		DisposeRoutineDescriptor(eventProc);
	if (filterProc != NULL)
		DisposeRoutineDescriptor(filterProc);

	return error;
}

//-----------------------------------------------------------------------------
// This is the event handler callback for the custom open AU preset file Nav Services dialog.  
// It handles open and reading the selected file and then applying the file's state data as 
// the new state for the AU instance.
// XXX is there some way to get errors from this as a return from the main calling function?
pascal void CustomOpenAUPresetNavEventHandler(NavEventCallbackMessage inCallbackSelector, NavCBRecPtr inCallbackParams, NavCallBackUserData inUserData)
{
	AudioUnit auComponentInstance = (AudioUnit) inUserData;
	NavDialogRef dialog = inCallbackParams->context;

	switch (inCallbackSelector)
	{
		case kNavCBStart:
			break;

		case kNavCBTerminate:
			if (dialog != NULL)
				NavDialogDispose(dialog);
			break;

		// the user did something action-packed
		case kNavCBUserAction:
		{
			OSStatus error;
			NavReplyRecord reply;

			// we're only interested in file open actions
			NavUserAction userAction = NavDialogGetUserAction(dialog);
			// XXX return userCanceled error for Cancel?
//			if (userAction == kNavUserActionCancel)
			if (userAction != kNavUserActionOpen)
				break;

			// get the file-choose response and check that it's valid (not a cancel or something like that)
			error = NavDialogGetReply(dialog, &reply);
			if (error == noErr)
			{
				if (reply.validRecord)
				{
					// grab the first response item (since there can only be 1) as an FSRef
					AEKeyword theKeyword;
					DescType actualType;
					Size actualSize;
					FSRef presetFileFSRef;
					error = AEGetNthPtr(&(reply.selection), 1, typeFSRef, &theKeyword, &actualType, 
										&presetFileFSRef, sizeof(presetFileFSRef), &actualSize);
					if (error == noErr)
					{
						// translate the FSRef to CFURL so that we can do the XML/plist stuff with it
						CFURLRef presetFileUrl = CFURLCreateFromFSRef(kCFAllocatorDefault, &presetFileFSRef);
						if (presetFileUrl != NULL)
						{
							// try to apply the data in the file as the AU's new state
							error = RestoreAUStateFromPresetFile(auComponentInstance, presetFileUrl);
							CFRelease(presetFileUrl);
						}
						else
							error = coreFoundationUnknownErr;
					}
				}
				NavDisposeReply(&reply);
			}
			break;
		}

		case kNavCBEvent:
			switch (inCallbackParams->eventData.eventDataParms.event->what)
			{
				// XXX do something with the update events?
				case updateEvt:
					break;
			}
			break;
	}
}

//-----------------------------------------------------------------------------
// This is the filter callback for the custom open AU preset file Nav Services dialog.  
// It handles filtering out non AU preset files from the file list display.  
// They will still appear in the list, but can not be selected.
// A response of true means "allow the file" and false means "disallow the file."
pascal Boolean CustomOpenAUPresetNavFilterProc(AEDesc * inItem, void * inInfo, void * inUserData, NavFilterModes inFilterMode)
{
	// default to allowing the item
	// only if we determine that a file, but not an AU preset file, do we want to reject it
	Boolean result = true;

	// the only items that we are filtering are files listed in the browser area
	if (inFilterMode == kNavFilteringBrowserList)
	{
		NavFileOrFolderInfo * info = (NavFileOrFolderInfo*) inInfo;
		// only interested in filtering files, all directories are fine
		if ( !(info->isFolder) )
		{
//fprintf(stderr, "AEDesc type = %.4s\n", (char*) &(inItem->descriptorType));
			// try to determine if this item is an AU preset file
			// if it is not, then reject it; otherwise, allow it
			AEDesc fsrefDesc;
			OSErr error = AECoerceDesc(inItem, typeFSRef, &fsrefDesc);
			if (error == noErr)
			{
				FSRef fileFSRef;
				error = AEGetDescData(&fsrefDesc, &fileFSRef, sizeof(fileFSRef));
				if (error == noErr)
				{
					CFURLRef fileUrl = CFURLCreateFromFSRef(kCFAllocatorDefault, &fileFSRef);
					if (fileUrl != NULL)
					{
						result = CFURLIsAUPreset(fileUrl);
						CFRelease(fileUrl);
					}
				}
			}
			AEDisposeDesc(&fsrefDesc);
		}
	}

	return result;
}

//-----------------------------------------------------------------------------
// This function takes a Navigation Services dialog and AU Component as input 
// and will attempt to set the start-off location for the dialog to the 
// standard preset files location, in the user domain, for that AU.  
// If inShouldCreateFolder is true, then, if the standard location directory 
// does not already exist, we will attempt to create it.  Otherwise, if the 
// directory does not already exist, this function will fail.
OSStatus SetNavDialogAUPresetStartLocation(NavDialogRef inDialog, Component inAUComponent, Boolean inShouldCreateFolder)
{
	OSStatus error;
	FSRef presetFileDirRef;

	if ( (inDialog == NULL) || (inAUComponent == NULL) )
		return paramErr;

	// get an FSRef to the standardized AU preset files location for this AU, in the user domain
	error = FindPresetsDirForAU(inAUComponent, kUserDomain, inShouldCreateFolder, &presetFileDirRef);
	if (error == noErr)
	{
		// we have to create an AEDesc thingy in order to use NavCustomControl to set the dialog's start location
		AEDesc defaultLocation;
		error = AECreateDesc(typeFSRef, &presetFileDirRef, sizeof(presetFileDirRef), &defaultLocation);
		if (error == noErr)
		{
			NavCustomControl(inDialog, kNavCtlSetLocation, &defaultLocation);
			AEDisposeDesc(&defaultLocation);
		}
	}

	return error;
}



#pragma mark _________Saving_Preset_Files_________

//-----------------------------------------------------------------------------
// This is a convenience wrapper for SaveAUStateToPresetFile_Bundle for when 
// the caller is the main application.
ComponentResult SaveAUStateToPresetFile(AudioUnit inAUComponentInstance)
{
	return SaveAUStateToPresetFile_Bundle(inAUComponentInstance, NULL);
}
//-----------------------------------------------------------------------------
// input:  Audio Unit ComponentInstance
// output:  ComponentResult error code
ComponentResult SaveAUStateToPresetFile_Bundle(AudioUnit inAUComponentInstance, CFBundleRef inBundle)
{
	ComponentResult error = noErr;
	CFPropertyListRef auStatePlist;
	UInt32 auStateDataSize = sizeof(auStatePlist);

	if (inAUComponentInstance == NULL)
		return paramErr;

	gCurrentBundle = inBundle;
	// null for inBundle means that the caller is the main application, 
	// so set our global bundle reference to the main application bundle
	if (inBundle == NULL)
		gCurrentBundle = CFBundleGetMainBundle();

/*
1st
get ClassInfo from AU
	if fails, give error
convert ClassInfo to XML/plist data
	if fails, give error
*/
	// get the current state data for the AU
	// if that fails, then there's no point in going any further since there is no data to save
	auStatePlist = NULL;
	error = AudioUnitGetProperty(inAUComponentInstance, kAudioUnitProperty_ClassInfo, 
						kAudioUnitScope_Global, (AudioUnitElement)0, &auStatePlist, &auStateDataSize);
	if (error != noErr)
		return error;
	// probably won't happen if noErr was returned, but just in case...
	if (auStatePlist == NULL)
		return coreFoundationUnknownErr;
	// something freaky:  the returned data size is less than what we supplied
	if (auStateDataSize < sizeof(auStatePlist))
		return coreFoundationUnknownErr;

/*
2nd:
Open a dialog
	open nib
*/
	// this will show the dialog(s) to the user and do all the handling of saving the state data to a file, etc.
	error = CreateSavePresetDialog((Component)inAUComponentInstance, auStatePlist);


	CFRelease(auStatePlist);

	return error;
}

//-----------------------------------------------------------------------------
// This function will take a PropertyList and CFURL referencing a file as input, 
// try to convert the PropertyList to XML data, and then write a file of the XML data 
// out to disk to the file referenced by the input CFURL.
OSStatus WritePropertyListToXMLFile(const CFPropertyListRef inPropertyList, const CFURLRef inXMLFileURL)
{
	SInt32 error = noErr;
	Boolean success;
	CFDataRef xmlData;

	if ( (inPropertyList == NULL) || (inXMLFileURL == NULL) )
		return paramErr;

	// convert the property list into XML data
	xmlData = CFPropertyListCreateXMLData(kCFAllocatorDefault, inPropertyList);
	if (xmlData == NULL)
		return coreFoundationUnknownErr;

	// write the XML data to the file
	success = CFURLWriteDataAndPropertiesToResource(inXMLFileURL, xmlData, NULL, &error);
	if ( !success && (error == noErr) )	// whuh?
		error = kCFURLUnknownError;

	CFRelease(xmlData);

	return error;
}

//-----------------------------------------------------------------------------
// this structs some data that we pass to the dialog callback functions as our UserData
typedef struct {
	WindowRef dialogWindow;
	CFPropertyListRef auStateData;
	Component auComponent;
	OSStatus dialogResult;
} SaveAUPresetFileDialogInfo;

// some constant values for the Carbon controls in our Save AU preset dialog nib
enum {
	// the text field where the preset file name is entered
	kPresetNameTextControlSignature = 'name',
	kPresetNameTextControlID = 128,
	// the radio button for choosing the file system domain
	kDomainChoiceControlSignature = 'doma',
	kDomainChoiceControlID = 129,

	// the control values for each option in the file system domain radio button
	kDomainChoiceValue_User = 1,
	kDomainChoiceValue_Local,
	kDomainChoiceValue_Network
};

#define kAUPresetSaveDialogNibName	CFSTR("au-preset-save-dialog")
#define kAUPresetSaveDialogNibWindowName	CFSTR("Save dialog")

//-----------------------------------------------------------------------------
// create, show, and run modally our dialog window
OSStatus CreateSavePresetDialog(Component inAUComponent, CFPropertyListRef inAUStatePlist)
{
	OSStatus error = noErr;
	IBNibRef nibRef = NULL;
	WindowRef dialogWindow = NULL;
	SaveAUPresetFileDialogInfo dialogInfo;
	EventHandlerUPP dialogEventHandlerUPP = NULL;
	EventTypeSpec dialogEventsSpec[] = { { kEventClassCommand, kEventCommandProcess } };
	EventHandlerRef dialogEventHandlerRef = NULL;

	if ( (inAUComponent == NULL) || (inAUStatePlist == NULL) )
		return paramErr;

	// find the dialog nib
	error = CreateNibReferenceWithCFBundle(gCurrentBundle, kAUPresetSaveDialogNibName, &nibRef);
	if (error != noErr)
		return error;

	// load the window that is contained in the nib
	error = CreateWindowFromNib(nibRef, kAUPresetSaveDialogNibWindowName, &dialogWindow);
	if (error != noErr)
		return error;

	// we don't need the nib reference anymore
	DisposeNibReference(nibRef);

	// initialize the values in our dialog info struct
	memset(&dialogInfo, 0, sizeof(dialogInfo));
	dialogInfo.auStateData = inAUStatePlist;
	dialogInfo.dialogWindow = dialogWindow;
	dialogInfo.auComponent = inAUComponent;
	dialogInfo.dialogResult = noErr;

	// install our dialog's event handler
	dialogEventHandlerUPP = NewEventHandlerUPP(SaveAUPresetFileDialogEventHandler);
	error = InstallWindowEventHandler(dialogWindow, dialogEventHandlerUPP, GetEventTypeCount(dialogEventsSpec), 
										dialogEventsSpec, (void*)(&dialogInfo), &dialogEventHandlerRef);
	if (error != noErr)
	{
		DisposeWindow(dialogWindow);
		return error;
	}

	// set the window title with the AU's name in there
	{
		char auNameCString[256];
		memset(auNameCString, 0, sizeof(auNameCString));
		error = GetAUNameAndManufacturerCStrings(inAUComponent, auNameCString, NULL);
		if (error == noErr)
		{
			CFStringRef dialogWindowTitle_firstPart = CFCopyLocalizedStringFromTableInBundle(CFSTR("Save preset file for"), CFSTR("dfx-au-utilities-localizable"), gCurrentBundle, CFSTR("window title of the regular (simple) save AU preset dialog.  (note:  the code will append the name of the AU after this string, so format the syntax accordingly)"));
			CFStringRef dialogWindowTitle = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%@ %s"), dialogWindowTitle_firstPart, auNameCString);
			CFRelease(dialogWindowTitle_firstPart);
			if (dialogWindowTitle != NULL)
			{
				SetWindowTitleWithCFString(dialogWindow, dialogWindowTitle);
				CFRelease(dialogWindowTitle);
			}
		}
	}

	// display the window now
	ShowWindow(dialogWindow);

	// set the file name text edit field as the initial focused control, for user convenience
	{
		ControlRef textFieldControl = NULL;
		ControlID textFieldControlID =	{ kPresetNameTextControlSignature, kPresetNameTextControlID };
		GetControlByID(dialogWindow, &textFieldControlID, &textFieldControl);
		SetKeyboardFocus(dialogWindow, textFieldControl, kControlFocusNextPart);
	}

	// run the dialog window modally
	// this will return when the dialog's event handler breaks the modal run loop
	RunAppModalLoopForWindow(dialogWindow);

/*
5th:
close and dispose dialog
return error code
*/
	// clean up all of the dialog stuff now that's it's finished
	RemoveEventHandler(dialogEventHandlerRef);
	if (dialogEventHandlerUPP != NULL)
		DisposeEventHandlerUPP(dialogEventHandlerUPP);
	HideWindow(dialogWindow);
	DisposeWindow(dialogWindow);

	return dialogInfo.dialogResult;
}

//-----------------------------------------------------------------------------
// This is the event handler callback for the (simple) Save AU preset file dialog.
pascal OSStatus SaveAUPresetFileDialogEventHandler(EventHandlerCallRef myHandler, EventRef inEvent, void * inUserData)
{
	OSStatus error = eventNotHandledErr;
	SaveAUPresetFileDialogInfo * dialogInfo = (SaveAUPresetFileDialogInfo*) inUserData;
	// this will be set true when it's time to quit this dialog
	Boolean exitModalLoop = false;
	HICommand command;

	if (GetEventClass(inEvent) != kEventClassCommand)
		return eventNotHandledErr;
	if (GetEventKind(inEvent) != kEventCommandProcess)
		return eventNotHandledErr;
	if (inUserData == NULL)
		return eventNotHandledErr;

/*
3rd:
catch dialog response
	save text input
		if Cancel hit, go to 5 and return userCanceledErr
		if Save hit, go to 4
		if Custom location, open Nav Services save dialog and go to 4-ish
*/
	// get the command's type info
	error = GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &command);
	if (error != noErr)
		return error;

	switch (command.commandID)
	{
		// the Save button was pushed, so try to save the requested file in the requested domain
		case kHICommandSave:
			{
				ControlRef textFieldControl = NULL;
				ControlID textFieldControlID =	{ kPresetNameTextControlSignature, kPresetNameTextControlID };
				CFStringRef presetNameString = NULL;

				ControlRef domainChoiceControl = NULL;
				ControlID domainChoiceControlID = { kDomainChoiceControlSignature, kDomainChoiceControlID };
				SInt32 domainChoice;
				short fsDomain;

				// get the file name text from the edit text field
				GetControlByID(dialogInfo->dialogWindow, &textFieldControlID, &textFieldControl);
				if (textFieldControl != NULL)
					GetControlData(textFieldControl, 0, kControlEditTextCFStringTag, sizeof(presetNameString), &presetNameString, NULL);

				// get the user's choice of file system domain from the domain choice control
				GetControlByID(dialogInfo->dialogWindow, &domainChoiceControlID, &domainChoiceControl);
				domainChoice = kDomainChoiceValue_User;
				if (domainChoiceControl != NULL)
					domainChoice = GetControl32BitValue(domainChoiceControl);
//fprintf(stderr, "domain choice = %ld\n", domainChoice);
				fsDomain = kUserDomain;
				if (domainChoice == kDomainChoiceValue_Local)
					fsDomain = kLocalDomain;
				else if (domainChoice == kDomainChoiceValue_Network)
					fsDomain = kNetworkDomain;

				// attempt to save out the AU state data to the requested file
				dialogInfo->dialogResult = TryToSaveAUPresetFile(dialogInfo->auComponent, dialogInfo->auStateData, presetNameString, fsDomain);
				if (presetNameString != NULL)
					CFRelease(presetNameString);

				// probably the file name was 0 characters long, so don't exit the dialog yet, 
				// the user probably just accidentally hit return
				if (dialogInfo->dialogResult == errFSMissingName)
					exitModalLoop = false;
				// probably there was an existing file conflict and the user chose not to replace the existing file
				else if (dialogInfo->dialogResult == userCanceledErr)
					exitModalLoop = false;
				// if there was a write access error and the domain choice was not User, 
				// tell the user and give them a chance to choose a different domain in which to save
				else if ( IsFileAccessError(dialogInfo->dialogResult) )
				{
					// XXX hmmm, should we still put up an error?  would this actually ever happen anyway?
					if (fsDomain == kUserDomain)
						exitModalLoop = true;
					else
					{
						HandleSaveAUPresetFileAccessError(domainChoiceControl);
						exitModalLoop = false;
					}
				}
				// anything else is reason to abort the dialog
				else
					exitModalLoop = true;

				error = noErr;
			}
			break;

		// the Cancel button was pushed, so abort the dialog
		case kHICommandCancel:
			exitModalLoop = true;
			dialogInfo->dialogResult = userCanceledErr;
			error = noErr;
			break;

		// the Choose Custom Location button was pushed, 
		// so exit this dialog and do the Nav Services PutFile dialog
		case kHICommandSaveAs:
			exitModalLoop = true;
			dialogInfo->dialogResult = CustomSaveAUPresetFile(dialogInfo->auStateData, dialogInfo->auComponent);
			break;
	}

	// this will kill the event run loop for the dialog
	if (exitModalLoop)
		QuitAppModalLoopForWindow(dialogInfo->dialogWindow);

	return error;
}

//-----------------------------------------------------------------------------
// This function takes an AU Component, its state data, a name for a preset file of the state data, 
// and a file system domain and then tries to save the AU's state data in the requested domain 
// with the requested file name (with the proper AU preset file name extension appended).
OSStatus TryToSaveAUPresetFile(Component inAUComponent, CFPropertyListRef inAUStateData, 
								CFStringRef inPresetNameString, short inFileSystemDomain)
{
	OSStatus error = noErr;

	HFSUniStr255 dummyuniname;
	// get the absolute maximum length that a file name can be
	size_t maxUnicodeNameLength = sizeof(dummyuniname.unicode) / sizeof(UniChar);
	// this is how much longer the file name will be after the AU preset file name extension is appended
	CFIndex presetFileNameExtensionLength = CFStringGetLength(kAUPresetFileNameExtension) - 1;	// -1 for the . before the extension
	// this is the maximum allowable length of the preset file's name without the extension
	CFIndex maxNameLength = maxUnicodeNameLength - presetFileNameExtensionLength;

	CFStringRef presetFileNameString;
	FSRef presetFileDirRef;
	CFURLRef presetFileDirUrl;
	CFURLRef presetBaseFileUrl;
	CFURLRef presetFullFileUrl;
	FSRef dummyFSRef;
	Boolean fileAlreadyExists;

	if ( (inAUComponent == NULL) || (inAUStateData == NULL) || (inPresetNameString == NULL) )
		return paramErr;

	// no file name text (probably the user just accidentally hit Save)
	if (CFStringGetLength(inPresetNameString) <= 0)
		return errFSMissingName;

// XXX this isn't defined in the current release headers yet, only the Mac OS X 10.3 pre-release headers
#ifndef kAUPresetNameKey
#define kAUPresetNameKey	"name"
#endif
	// set the state data's name value to the requested name
	// do this before we mess around with the file name string (truncating, adding extension, etc.)
	// XXX is this really a good idea?  I think so, but I'm not totally sure...
	if (CFGetTypeID(inAUStateData) == CFDictionaryGetTypeID())
		CFDictionarySetValue((CFMutableDictionaryRef)inAUStateData, CFSTR(kAUPresetNameKey), inPresetNameString);

//fprintf(stderr, "\tuser chosen name:\n"); CFShow(inPresetNameString);
	// if the requested file name is too long, truncate it
	if (CFStringGetLength(inPresetNameString) > maxNameLength)
		presetFileNameString = CFStringCreateWithSubstring(kCFAllocatorDefault, inPresetNameString, CFRangeMake(0, maxNameLength));
	// otherwise (just to make it so that we CFRelease this either way), just retain the input string
	else
	{
		presetFileNameString = inPresetNameString;
		CFRetain(presetFileNameString);
	}
	// something wacky happens, bail out
	if (presetFileNameString == NULL)
		return coreFoundationUnknownErr;
//fprintf(stderr, "\ttruncated name without extension:\n"); CFShow(presetFileNameString);

	// now we need to get the parent directory of where we will save this file
	error = FindPresetsDirForAU(inAUComponent, inFileSystemDomain, kCreateFolder, &presetFileDirRef);
	if (error != noErr)
		return error;
	// and convert that into a CFURL so that we can use CoreFoundation's 
	// PropertList and XML APIs for saving the data to a file
	presetFileDirUrl = CFURLCreateFromFSRef(kCFAllocatorDefault, &presetFileDirRef);
	if (presetFileDirUrl == NULL)
	{
		CFRelease(presetFileNameString);
		return coreFoundationUnknownErr;
	}
	// create a CFURL of the requested file name in the proper AU presets directory
	presetBaseFileUrl = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, presetFileDirUrl, presetFileNameString, false);
	CFRelease(presetFileDirUrl);
	CFRelease(presetFileNameString);
	if (presetBaseFileUrl == NULL)
		return coreFoundationUnknownErr;
	presetFullFileUrl = presetBaseFileUrl;
	// the file name is already formed as a proper AU preset file name, so no need to append the extension
	// maybe the user already added the .aupreset extension when entering in the name?
	if ( CFURLIsAUPreset(presetFullFileUrl) )
		CFRetain(presetFullFileUrl);	// this was we can CFRelease later on either way, without having to remember what happened
	// otherwise, the file name needs the proper AU preset extension to be appended
	else
		presetFullFileUrl = CFURLCreateCopyAppendingPathExtension(kCFAllocatorDefault, presetBaseFileUrl, kAUPresetFileNameExtension);
	CFRelease(presetBaseFileUrl);
	// uh oh, something went wrong
	if (presetFullFileUrl == NULL)
		return coreFoundationUnknownErr;
//fprintf(stderr, "\ttruncated name with extension:\n"); CFShow(presetFullFileUrl);

	// check whether or not the file already exists
	// note that, since an FSRef cannot be created for a file that doesn't yet exist, 
	// if this succeeds, then that means that a file with this path and name already exists
	fileAlreadyExists = CFURLGetFSRef(presetFullFileUrl, &dummyFSRef);
	if (fileAlreadyExists)
	{
		// present the user with a dialog asking whether or not they want to replace an already existing file
		// if not, then return with a "user canceled" error code
		if ( !ShouldReplaceExistingAUPresetFile(presetFullFileUrl) )
		{
			CFRelease(presetFullFileUrl);
			return userCanceledErr;
		}
	}

	// if we've made it this far, then it's time to try to 
	// write the AU state data out to the file as XML data
	error = WritePropertyListToXMLFile(inAUStateData, presetFullFileUrl);
	CFRelease(presetFullFileUrl);


/*
4th:
set the "name" value in the AU state dictionary to be the name chosen for the file (?)
truncate name if need be (no more than 255 characters with .aupreset extension)
make sure that the name is not 0 characters
create file
if already exists, ask if user wants to replace
		if no, go back to 3 with last name text filled in
write ClassInfo data out to disk
XXX	if fails, tell user why
		unless its a privileges error, in which case go back to 3 rather than return
*/


	return error;
}

//-----------------------------------------------------------------------------
// This is the event handler for the "file already exists" alert/dialog.  
// It's sole purpose is to map a Cancel command to the Cancel button, 
// because button is also the default button, thus making things complicated.
pascal OSStatus ShouldReplaceExistingAUPresetFileDialogEventHandler(EventHandlerCallRef myHandler, EventRef inEvent, void * inUserData)
{
	Boolean doCancel = false;

	if (inUserData == NULL)
		return eventNotHandledErr;

	// check an HI command event to see if it's a cancel event
	if (GetEventClass(inEvent) == kEventClassCommand)
	{
		if (GetEventKind(inEvent) == kEventCommandProcess)
		{
			// get the HI command
			HICommand command;
			OSStatus error = GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &command);
			if (error == noErr)
			{
				if (command.commandID == kHICommandCancel)
					doCancel = true;
			}
		}
	}

	// check a keyboard event to see if it's something equivalent to a typical cancel keyboard command
	else if (GetEventClass(inEvent) == kEventClassKeyboard)
	{
		if (GetEventKind(inEvent) == kEventRawKeyDown)
		{
			unsigned char charCode = 0;
			UInt32 modifiers = 0;
			GetEventParameter(inEvent, kEventParamKeyMacCharCodes, typeChar, NULL, sizeof(char), NULL, &charCode);
			GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32, NULL, sizeof(UInt32), NULL, &modifiers);
//fprintf(stderr, "charCode = %d, modifiers = %lu\n", charCode, modifiers);

			// treat it as a cancel command if it is ESC or command-.
			if ( (charCode == 27) || ((charCode == '.') && (modifiers == cmdKey)) )
				doCancel = true;
		}
	}

	// some sort of cancel event was received, so try to simulate a push of the Cancel button
	if (doCancel)
	{
		DialogRef dialog = (DialogRef)inUserData;

		// try to get a reference to the Cancel buton
		ControlRef cancelControl = NULL;
		OSStatus error = GetDialogItemAsControl(dialog, kAlertStdAlertCancelButton, &cancelControl);
		if (error == noErr)
		{
/*
CFStringRef ctitle = NULL;
error = CopyControlTitleAsCFString(cancelControl, &ctitle);
if (error == noErr)
{
	CFShow(ctitle);
	CFRelease(ctitle);
}
*/
			// then create a simulated "hit" event for the Cancel button
			EventRef cancelEvent;
			error = CreateEvent(NULL, kEventClassControl, kEventControlSimulateHit, GetCurrentEventTime(), kEventAttributeUserEvent, &cancelEvent);
			if (error == noErr)
			{
				UInt32 modifiers = 0;
				ControlPartCode hitPart = kControlIndicatorPart;
//				ControlPartCode hitPart = kControlButtonPart;
				SetEventParameter(cancelEvent, kEventParamDirectObject, typeControlRef, sizeof(cancelControl), &cancelControl);
				SetEventParameter(cancelEvent, kEventParamKeyModifiers, typeUInt32, sizeof(modifiers), &modifiers);
				SetEventParameter(cancelEvent, kEventParamControlPart, typeControlPartCode, sizeof(hitPart), &hitPart);

				// XXX this is not working
				SendEventToEventTarget(cancelEvent, GetControlEventTarget(cancelControl));
				ReleaseEvent(cancelEvent);

				return noErr;
			}
		}
	}

	return eventNotHandledErr;
}

//-----------------------------------------------------------------------------
// Given an URL represeting a requested-to-save AU preset file, 
// this function creates an alert dialog telling the user that an identically named 
// file already exists and asking the user if she really wants to replace that old file.
// A return value of true means that the user wants to replace the file, and 
// a return value of false means that the user does not want to replace the file.
Boolean ShouldReplaceExistingAUPresetFile(const CFURLRef inAUPresetFileURL)
{
	AlertStdCFStringAlertParamRec alertParams;
	CFStringRef filenamestring;
	CFStringRef dirstring;
	CFURLRef dirurl;
	CFStringRef titleString;
	CFStringRef messageString;
	DialogRef dialog;
	OSStatus alertErr;

	GetStandardAlertDefaultParams(&alertParams, kStdCFStringAlertVersionOne);
	alertParams.movable = true;
	alertParams.defaultText = CFCopyLocalizedStringFromTableInBundle(CFSTR("Replace"), CFSTR("dfx-au-utilities-localizable"), gCurrentBundle, 
				CFSTR("text for the button that will over-write an existing file when a save file file-already-exists conflict arises"));
	alertParams.cancelText = (CFStringRef) kAlertDefaultCancelText;
	alertParams.defaultButton = kAlertStdAlertCancelButton;
//	alertParams.cancelButton = kAlertStdAlertCancelButton;
	alertParams.cancelButton = 0;
	// do a bunch of wacky stuff to try to come up with a nice and informative message text for the dialog
	filenamestring = CFURLCopyLastPathComponent(inAUPresetFileURL);
	dirstring = NULL;
	dirurl = CFURLCreateCopyDeletingLastPathComponent(kCFAllocatorDefault, inAUPresetFileURL);
	if (dirurl != NULL)
	{
		dirstring = CFURLCopyFileSystemPath(dirurl, kCFURLPOSIXPathStyle);
		CFRelease(dirurl);
	}
	titleString = CFCopyLocalizedStringFromTableInBundle(CFSTR("Save"), CFSTR("dfx-au-utilities-localizable"), gCurrentBundle, 
								CFSTR("title of the alert window when a save file file-already-exists conflict arises"));
	messageString = NULL;
	// if we got strings for the file name and parent directory path, 
	// then we can make a nice and specific dialog message
	if ( (filenamestring != NULL) && (dirstring != NULL) )
	{
		CFStringRef messageOutlineString = CFCopyLocalizedStringFromTableInBundle(CFSTR("A file named \"%@\" already exists in \"%@\".  Do you want to replace it with the file that you are saving?"), CFSTR("dfx-au-utilities-localizable"), gCurrentBundle, 
				CFSTR("the message in the alert, specifying file name and location, for when file name and path are available as CFStrings"));
		messageString = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, messageOutlineString, filenamestring, dirstring);
		CFRelease(messageOutlineString);
	}
	// otherwise, we have to make it a general message
	else
	{
		messageString = CFCopyLocalizedStringFromTableInBundle(CFSTR("A file with the same name already exists in this location.  Do you want to replace it with the file that you are saving?"), CFSTR("dfx-au-utilities-localizable"), gCurrentBundle, 
							CFSTR("the message in the alert, general, for when file name and path are not available"));
	}
	if (filenamestring != NULL)
		CFRelease(filenamestring);
	if (dirstring != NULL)
		CFRelease(dirstring);

	// now that we have some message text for the user, we can create and show the dialog
	alertErr = CreateStandardAlert(kAlertCautionAlert, titleString, messageString, &alertParams, &dialog);
	CFRelease(alertParams.defaultText);
	CFRelease(titleString);
	CFRelease(messageString);

	if (alertErr == noErr)
	{
		DialogItemIndex itemHit;

		// create and install custom event handler stuff
		EventTypeSpec dialogEvents[] = { { kEventClassCommand, kEventCommandProcess }, {kEventClassKeyboard, kEventRawKeyDown} };
		EventHandlerUPP dialogEventHandlerUPP = NewEventHandlerUPP(ShouldReplaceExistingAUPresetFileDialogEventHandler);
		EventHandlerRef dialogEventHandlerRef = NULL;
		InstallEventHandler(GetWindowEventTarget(GetDialogWindow(dialog)), dialogEventHandlerUPP, 
							GetEventTypeCount(dialogEvents), dialogEvents, dialog, &dialogEventHandlerRef);

		// show the alert dialog
		alertErr = RunStandardAlert(dialog, NULL, &itemHit);

		// clean up custom event handler stuff
		if (dialogEventHandlerRef != NULL)
			RemoveEventHandler(dialogEventHandlerRef);
		if (dialogEventHandlerUPP != NULL)
			DisposeEventHandlerUPP(dialogEventHandlerUPP);

		// If the dialog ran okay and the user said "cancel", 
		// then say that the user wants to keep the file, 
		// but otherwise, say that the user wants to replace the file.  
		// This means that, if the dialog fails, then the default behavior 
		// is for the existing file to be replaced.  I think that this makes sense 
		// because, otherwise, the user's save request would be failing, 
		// but the user wouldn't know why, and the Save dialog wouldn't go away.
		if (alertErr == noErr)
		{
			if (itemHit == kAlertStdAlertCancelButton)
				return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// A handy little function that, given an error code value, returns true 
// if that code is a file write access error code, and false otherwise.
Boolean IsFileAccessError(OSStatus inErrorCode)
{
	if ( (inErrorCode == kCFURLResourceAccessViolationError) || (inErrorCode == afpAccessDenied) || 
			(inErrorCode == permErr) || (inErrorCode == wrPermErr) )
		return true;
	else
		return false;
}

//-----------------------------------------------------------------------------
// This function handles the situation in which there was a write access error 
// when trying to save an AU preset file to disk.  It shows an alert to the user 
// that says what happened and why and suggests trying to save the preset file 
// in the User domain.  Then, if the input ControlRef argument is non-null, 
// it will be set to the User choice automagically for the user.  
// So, obviously that input argument needs to be for the choice control of 
// the Save AU preset file dialog.
OSStatus HandleSaveAUPresetFileAccessError(ControlRef inDomainChoiceControl)
{
	OSStatus error;
	AlertStdCFStringAlertParamRec alertParams;
	CFStringRef alertTitle;
	CFStringRef alertMessage;
	DialogRef dialog;

	GetStandardAlertDefaultParams(&alertParams, kStdCFStringAlertVersionOne);
	alertParams.movable = true;
	alertTitle = CFCopyLocalizedStringFromTableInBundle(CFSTR("Save access error"), CFSTR("dfx-au-utilities-localizable"), gCurrentBundle, CFSTR("the alert window title for when an access privileges error occurs while trying to save a file"));
	alertMessage = CFCopyLocalizedStringFromTableInBundle(CFSTR("You do not have sufficient privileges to save files in that location.  Try saving your file in the User domain."), CFSTR("dfx-au-utilities-localizable"), gCurrentBundle, CFSTR("the content of the alert message text for an access privileges error"));
	error = CreateStandardAlert(kAlertNoteAlert, alertTitle, alertMessage, &alertParams, &dialog);
	CFRelease(alertTitle);
	CFRelease(alertMessage);
	if (error == noErr)
	{
		DialogItemIndex itemHit;
		error = RunStandardAlert(dialog, NULL, &itemHit);
	}

	// set the file system domain choice control to User automatically
	if (inDomainChoiceControl != NULL)
		SetControl32BitValue(inDomainChoiceControl, kDomainChoiceValue_User);

	return error;
}

// XXX should I put these in the private header?
// unique identifier of our AU preset file save dialogs for Navigation Services
const UInt32 kAUPresetSaveNavDialogKey = 'AUps';
//-----------------------------------------------------------------------------
// This is the function that you call if the user pushes the Choose Custom Location 
// in the regular (simple) Save AU preset dialog.  Then this function will create and 
// run a standard Navigation Services PutFile dialog so that the user can save the file anywhere.
// The input arguments are:  a PropertyList of the AU state data that you want to save and 
// the AU Component whose state data you are saving (you can cast a ComponentInstance or AudioUnit 
// to Component for this argument).  inAUComponent is only used to find the appropriate 
// standard default location to start the Save file dialog off in, so it can be null, 
// in which case the default starting location will not be set.
OSStatus CustomSaveAUPresetFile(CFPropertyListRef inAUStateData, Component inAUComponent)
{
	OSStatus error = noErr;
	NavDialogCreationOptions dialogOptions;
	NavEventUPP eventProc;
	NavDialogRef dialog;

	if (inAUStateData == NULL)
		return paramErr;

	error = NavGetDefaultDialogCreationOptions(&dialogOptions);
	if (error != noErr)
		return error;
	// AU preset files need the proper file name extention in order to be identified as AU preset files, 
	// so enable this flag so that the dialog behavior is to preserve the file name extension as the user types
	dialogOptions.optionFlags |= kNavPreserveSaveFileExtension;
	// this gives this dialog a unique identifier so that it has independent remembered settings for the calling app
	dialogOptions.preferenceKey = kAUPresetSaveNavDialogKey;

	eventProc = NewNavEventUPP(CustomSaveAUPresetNavEventHandler);
	error = NavCreatePutFileDialog(&dialogOptions, (OSType)0, (OSType)0, eventProc, (void*)inAUStateData, &dialog);
	if (error == noErr)
	{
		// set the initial file name shown in the dialog's file name text edit field
		CFStringRef defaultFileBaseName = CFCopyLocalizedStringFromTableInBundle(CFSTR("untitled"), CFSTR("dfx-au-utilities-localizable"), gCurrentBundle, CFSTR("the default preset file name for the Nav Services save file dialog"));
		CFStringRef defaultFileName = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%@.%@"), 
															defaultFileBaseName, kAUPresetFileNameExtension);
		CFRelease(defaultFileBaseName);
		if (defaultFileName != NULL)
		{
			NavDialogSetSaveFileName(dialog, defaultFileName);
			CFRelease(defaultFileName);
		}

		// if the input AU Component argument is valid, use it to find the 
		// standard preset files location for the AU in the user domain, 
		// and set the PutFile dialog to start off there
		if (inAUComponent != NULL)
			SetNavDialogAUPresetStartLocation(dialog, inAUComponent, kCreateFolder);

		// now show the dialog to the user
		error = NavDialogRun(dialog);
	}
	if (eventProc != NULL)
		DisposeRoutineDescriptor(eventProc);

	return error;
}

//-----------------------------------------------------------------------------
// XXX is there some way to get errors from this as a return from the main calling function?
// This is the event handler for the Navigation Services Save AU preset file dialog.
// It does the regular required stuff and will handle writing the file out to disk 
// when the user requests that.
pascal void CustomSaveAUPresetNavEventHandler(NavEventCallbackMessage inCallbackSelector, NavCBRecPtr inCallbackParams, NavCallBackUserData inUserData)
{
	CFPropertyListRef auStateData = (CFPropertyListRef) inUserData;
	NavDialogRef dialog = inCallbackParams->context;

	switch (inCallbackSelector)
	{
		// in case the dialog is non-modal and returns after the calling function returns, 
		// we need to own our own reference to the AU state data PropertyList
		case kNavCBStart:
			if (auStateData != NULL)
				CFRetain(auStateData);
			break;

		// the dialog is finished, so now we can release our reference to the AU state data PropertyList
		case kNavCBTerminate:
			if (dialog != NULL)
				NavDialogDispose(dialog);
			if (auStateData != NULL)
				CFRelease(auStateData);
			break;

		// "user actions" include the save request, which we need to respond to
		case kNavCBUserAction:
		{
			OSStatus error;
			NavReplyRecord reply;

			// anything other than Save, we are not interested in
			NavUserAction userAction = NavDialogGetUserAction(dialog);
			// XXX return userCanceled error for Cancel?
//			if (userAction == kNavUserActionCancel)
			if (userAction != kNavUserActionSaveAs)
				break;

			// find the parent dir and file name for the requested file to save to
			error = NavDialogGetReply(dialog, &reply);
			if (error == noErr)
			{
				if (reply.validRecord)
				{
					AEKeyword theKeyword;
					DescType actualType = 0;
					Size actualSize;
					FSRef parentDirFSRef;
// XXX should I be catching these errors in the dialogInfo->dialogResult ?
					error = AEGetNthPtr(&(reply.selection), 1, typeFSRef, &theKeyword, &actualType, 
										&parentDirFSRef, sizeof(parentDirFSRef), &actualSize);
//fprintf(stderr, "actual type = %.4s\n", (char*)&actualType);
					if (error == noErr)
					{
						// now we need a CFURL version of the parent directory so that we can use the CF file APIs 
						// for translating XML file data
						CFURLRef parentDirUrl = CFURLCreateFromFSRef(kCFAllocatorDefault, &parentDirFSRef);
						if (parentDirUrl != NULL)
						{
							// use the file name response value to create the URL for the file to save
							CFURLRef presetFileUrl = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, 
																		parentDirUrl, reply.saveFileName, false);
							CFRelease(parentDirUrl);
							if (presetFileUrl != NULL)
							{
								// with the full file URL, we can now try to translate the AU state data 
								// to XML data, and write that XML data out to the file
								error = WritePropertyListToXMLFile(auStateData, presetFileUrl);
								CFRelease(presetFileUrl);

								reply.translationNeeded = false;
								// always call NavCompleteSave() to complete
								if (error == noErr)
									error = NavCompleteSave(&reply, kNavTranslateInPlace);
							}
							else
								error = coreFoundationUnknownErr;
						}
						else
							error = coreFoundationUnknownErr;
					}
				}
				NavDisposeReply(&reply);
			}
			break;
		}

		case kNavCBEvent:
			switch (inCallbackParams->eventData.eventDataParms.event->what)
			{
				// XXX do something with the update events?
				case updateEvt:
					break;
			}
			break;
	}
}
