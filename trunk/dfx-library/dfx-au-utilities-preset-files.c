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
#define kAUPresetFileNameExtension	CFSTR("aupreset")
#define kAUPresetsDirName	CFSTR("Presets")



//-----------------------------------------------------------------------------
#define kDfxLocalizableStringsTable	CFSTR("dfx-au-utilities-localizable")
CFBundleRef gCurrentBundle = NULL;



#pragma mark _________Handling_Files_And_Directories_________

//-----------------------------------------------------------------------------
OSStatus FindPresetsDirForAU(Component inComponent, short inFileSystemDomain, Boolean inCreateDir, FSRef * outDirRef)
{
	if ( (inComponent == NULL) || (outDirRef == NULL) )
		return paramErr;

	OSStatus error;

	// get an FSRef for the Audio support directory (in Library)
	FSRef audioDirRef;
	error = FSFindFolder(inFileSystemDomain, kAudioSupportFolderType, inCreateDir, &audioDirRef);
	if (error != noErr)
		return error;

	// get an FSRef to the Presets directory in the Audio directory
	FSRef presetsDirRef;
	error = MakeFSRefInDir(&audioDirRef, kAUPresetsDirName, inCreateDir, &presetsDirRef);
	if (error != noErr)
		return error;

	// determine the name of the AU and the AU manufacturer so that we know the names of the presets sub-directories
	CFStringRef pluginNameCFString = NULL, manufacturerNameCFString = NULL;
	error = GetComponentNameAndManufacturerStrings(inComponent, &pluginNameCFString, &manufacturerNameCFString);
	if (error != noErr)
		return error;
	if ( (manufacturerNameCFString == NULL) || (pluginNameCFString == NULL) )
		return coreFoundationUnknownErr;

	// get an FSRef to the AU manufacturer directory in the Presets directory
	FSRef manufacturerPresetsDirRef;
	error = MakeFSRefInDir(&presetsDirRef, manufacturerNameCFString, inCreateDir, &manufacturerPresetsDirRef);
	if (error == noErr)
		// get an FSRef to the particular plugin's directory in the manufacturer directory
		error = MakeFSRefInDir(&manufacturerPresetsDirRef, pluginNameCFString, inCreateDir, outDirRef);

	CFRelease(manufacturerNameCFString);
	CFRelease(pluginNameCFString);

	return error;
}

//-----------------------------------------------------------------------------
OSStatus GetComponentNameAndManufacturerStrings(Component inComponent, CFStringRef * outNameString, CFStringRef * outManufacturerString)
{
	if ( (inComponent == NULL) || ((outNameString == NULL) && (outManufacturerString == NULL)) )
		return paramErr;

	char pluginNameCString[256], manufacturerNameCString[256];
	memset(pluginNameCString, 0, sizeof(pluginNameCString));
	memset(manufacturerNameCString, 0, sizeof(manufacturerNameCString));
	OSStatus error = GetComponentNameAndManufacturerCStrings(inComponent, pluginNameCString, manufacturerNameCString);
	if (error != noErr)
		return error;

	if (outNameString != NULL)
		*outNameString = CFStringCreateWithCString(kCFAllocatorDefault, pluginNameCString, CFStringGetSystemEncoding());
	if (outManufacturerString != NULL)
		*outManufacturerString = CFStringCreateWithCString(kCFAllocatorDefault, manufacturerNameCString, CFStringGetSystemEncoding());
	if (outNameString != NULL)
	{
		if (*outNameString == NULL)
			return coreFoundationUnknownErr;
	}
	if (outManufacturerString != NULL)
	{
		if (*outManufacturerString == NULL)
			return coreFoundationUnknownErr;
	}

	return noErr;
}

//-----------------------------------------------------------------------------
OSStatus GetComponentNameAndManufacturerCStrings(Component inComponent, char * outNameString, char * outManufacturerString)
{
	if ( (inComponent == NULL) || ((outNameString == NULL) && (outManufacturerString == NULL)) )
		return paramErr;

	OSStatus error = noErr;

	Handle componentNameHandle = NewHandle(sizeof(void*));
	if (componentNameHandle == NULL)
		return nilHandleErr;
	ComponentDescription dummycd;
	error = GetComponentInfo(inComponent, &dummycd, componentNameHandle, NULL, NULL);
	if (error != noErr)
		return error;
	HLock(componentNameHandle);
	ConstStr255Param componentFullNamePString = (ConstStr255Param) (*componentNameHandle);
	if (componentFullNamePString == NULL)
		error = nilHandleErr;
	else
	{
		// convert the Component name Pascal string to a C string
		char componentFullNameCString[256];
		memset(componentFullNameCString, 0, sizeof(componentFullNameCString));
		memcpy(componentFullNameCString, componentFullNamePString+1, componentFullNamePString[0]);
		char * separatorByte = strchr(componentFullNameCString, ':');
		if (separatorByte == NULL)
			error = internalComponentErr;
		else
		{
			separatorByte[0] = 0;
			char * manufacturerNameCString = componentFullNameCString;
			char * pluginNameCString = separatorByte + 1;
			while ( isspace(*pluginNameCString) )
				pluginNameCString++;

			if (outNameString != NULL)
				strcpy(outNameString, pluginNameCString);
			if (outManufacturerString != NULL)
				strcpy(outManufacturerString, manufacturerNameCString);
		}
	}
	DisposeHandle(componentNameHandle);

	return error;
}

//--------------------------------------------------------------------------
CFStringRef CopyAUPresetNameFromCFURL(const CFURLRef inAUPresetUrl)
{
	CFStringRef baseFileNameString = NULL;
	if (inAUPresetUrl != NULL)
	{
		CFURLRef baseNameUrl = CFURLCreateCopyDeletingPathExtension(kCFAllocatorDefault, inAUPresetUrl);
		if (baseNameUrl != NULL)
		{
			baseFileNameString = CFURLCopyLastPathComponent(baseNameUrl);
			CFRelease(baseNameUrl);
		}
	}
	return baseFileNameString;
}

//--------------------------------------------------------------------------
Boolean CFURLIsAUPreset(const CFURLRef inUrl)
{
	if (inUrl != NULL)
	{
		CFStringRef fileNameExtension = CFURLCopyPathExtension(inUrl);
		if (fileNameExtension != NULL)
			return (CFStringCompare(fileNameExtension, kAUPresetFileNameExtension, kCFCompareCaseInsensitive) == kCFCompareEqualTo);
	}
	return false;
}

//--------------------------------------------------------------------------
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
// This function will, given a reference to a directory and a name for a file, 
// create a reference for that file in the directory.  
// This won't work unless the file actually exists already in the directory.
OSStatus MakeFSRefInDir(const FSRef * inParentDirRef, CFStringRef inItemNameString, Boolean inCreateItem, FSRef * outItemRef)
{
	if ( (inParentDirRef == NULL) || (inItemNameString == NULL) || (outItemRef == NULL) )
		return paramErr;

	OSStatus error = noErr;
	// first we need to convert the CFString of the file name to a HFS-style unicode file name
	HFSUniStr255 itemUniName;
	memset(&itemUniName, 0, sizeof(itemUniName));
	TranslateCFStringToUnicodeString(inItemNameString, &itemUniName);
	// then we try to create an FSRef for the file
	error = FSMakeFSRefUnicode(inParentDirRef, itemUniName.length, itemUniName.unicode, kTextEncodingUnknown, outItemRef);
//kTextEncodingUnicodeDefault
//kCFStringEncodingUnicode
	if ( (error != noErr) && inCreateItem )
	{
		error = FSCreateDirectoryUnicode(inParentDirRef, itemUniName.length, itemUniName.unicode, kFSCatInfoNone, NULL, outItemRef, NULL, NULL);
//		error = FSCreateFileUnicode(inParentDirRef, itemUniName.length, itemUniName.unicode, kFSCatInfoNone, NULL, outItemRef, NULL);
	}

	return error;
}

//-----------------------------------------------------------------------------
// convert a CFString to a unicode HFS file name string
void TranslateCFStringToUnicodeString(CFStringRef inCFString, HFSUniStr255 * outUniName)
{
	if ( (inCFString == NULL) || (outUniName == NULL) )
		return;

	CFIndex cfnamelength = CFStringGetLength(inCFString);
	CFIndex maxlength = sizeof(outUniName->unicode) / sizeof(UniChar);
	// the length can't be more than 255 characters for an HFS file name
	if (cfnamelength > maxlength)
		cfnamelength = maxlength;
	outUniName->length = cfnamelength;
	// translate the CFString to a unicode string representation in the HFS file name string
	CFStringGetCharacters(inCFString, CFRangeMake(0, cfnamelength), outUniName->unicode);
}



#pragma mark _________Preset_Files_Tree_________

//-----------------------------------------------------------------------------
CFTreeRef CFTreeCreateFromAUPresetFilesInDomain(Component inComponent, short inFileSystemDomain)
{
	if (inComponent == NULL)
		return NULL;

	OSStatus error = noErr;
	FSRef presetsDirRef;
	error = FindPresetsDirForAU(inComponent, inFileSystemDomain, kDontCreateFolder, &presetsDirRef);
	if (error != noErr)
		return NULL;

	CFTreeRef tree = CreateFileURLsTreeNode(&presetsDirRef, kCFAllocatorDefault);
	if (tree != NULL)
	{
		CollectAllAUPresetFilesInDir(&presetsDirRef, tree);
//		CFTreeSortChildren(tree, FileURLsTreeComparatorFunction, NULL);
		SortCFTreeRecursively(tree, FileURLsTreeComparatorFunction, NULL);

		// the presets directory for this plugin exists, but there are no preset files in it
		if (CFTreeGetChildCount(tree) <= 0)
		{
			CFRelease(tree);
			tree = NULL;
		}
	}

	return tree;
}

//-----------------------------------------------------------------------------
CFURLRef GetCFURLFromFileURLsTreeNode(const CFTreeRef inTree)
{
	if (inTree == NULL)
		return NULL;
	CFTreeContext treeContext;
	treeContext.version = 0;
	CFTreeGetContext(inTree, &treeContext);
	return (CFURLRef) (treeContext.info);
}

//-----------------------------------------------------------------------------
CFTreeRef CreateFileURLsTreeNode(const FSRef * inItemRef, CFAllocatorRef inAllocator)
{
	CFURLRef itemUrl = CFURLCreateFromFSRef(kCFAllocatorDefault, inItemRef);
	if (itemUrl == NULL)
		return NULL;
	CFTreeContext treeContext;
	FileURLsCFTreeContext_Init(itemUrl, &treeContext);
	CFTreeRef newNode = CFTreeCreate(inAllocator, &treeContext);
	CFRelease(itemUrl);
	return newNode;
}

//-----------------------------------------------------------------------------
CFTreeRef AddFileItemToTree(const FSRef * inItemRef, CFTreeRef inParentTree)
{
	CFTreeRef newNode = CreateFileURLsTreeNode(inItemRef, CFGetAllocator(inParentTree));
	if (newNode != NULL)
	{
		CFTreeAppendChild(inParentTree, newNode);
		CFRelease(newNode);
	}
	return newNode;
}

//-----------------------------------------------------------------------------
// This function will scan through a directory and examine every file that might be 
// an AU preset file.  If a child directory is encountered, then this function is 
// called recursively on that child directory so that we get everything.
void CollectAllAUPresetFilesInDir(const FSRef * inDirRef, CFTreeRef inParentTree)
{
	if ( (inDirRef == NULL) || (inParentTree == NULL) )
		return;

	// first, we create a directory iterator
	// in order for iterating a non-root-volume's directory contents to 
	// work with FSGetCatalogInfoBulk, we need to use the "flat" option, 
	// which means that sub-directories' will not automatically be recursed
	FSIterator dirIterator;
	long error = FSOpenIterator(inDirRef, kFSIterateFlat, &dirIterator);
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
			// otherwise it's a file, so we add it, if it is an AU preset file
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
	if (CFTreeGetChildCount(inParentTree) <= 0)
	{
		if (CFTreeFindRoot(inParentTree) != inParentTree)
			CFTreeRemove(inParentTree);
	}
}

//-----------------------------------------------------------------------------
void SortCFTreeRecursively(CFTreeRef inTreeRoot, CFComparatorFunction inComparatorFunction, void * inContext)
{
	if (inTreeRoot != NULL)
	{
		CFTreeSortChildren(inTreeRoot, inComparatorFunction, inContext);
		CFTreeRef child = CFTreeGetFirstChild(inTreeRoot);
		if (child != NULL)
			SortCFTreeRecursively(child, inComparatorFunction, inContext);
		CFTreeRef next = CFTreeGetNextSibling(inTreeRoot);
		if (next != NULL)
			SortCFTreeRecursively(next, inComparatorFunction, inContext);
	}
}

//-----------------------------------------------------------------------------
CFComparisonResult FileURLsTreeComparatorFunction(const void * inTree1, const void * inTree2, void * inContext)
{
//fprintf(stderr, "\tFileURLsTreeComparatorFunction:\n"); CFShow(inTree1); CFShow(inTree2); fprintf(stderr, "\n");
	CFTreeRef tree1 = (CFTreeRef)inTree1, tree2 = (CFTreeRef)inTree2;
	CFURLRef url1 = GetCFURLFromFileURLsTreeNode(tree1);
	CFURLRef url2 = GetCFURLFromFileURLsTreeNode(tree2);
	CFComparisonResult result = kCFCompareEqualTo;
	if ( (url1 != NULL) && (url2 != NULL) )
	{
		CFStringRef fileNameString1 = CFURLCopyLastPathComponent(url1);
		CFStringRef fileNameString2 = CFURLCopyLastPathComponent(url2);

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
void FileURLsCFTreeContext_Init(const CFURLRef inUrl, CFTreeContext * outTreeContext)
{
	if ( (outTreeContext == NULL) || (inUrl == NULL) )
		return;
	// wipe the struct clean
	memset(outTreeContext, 0, sizeof(*outTreeContext));
	// set all of the values and function pointers in the callbacks struct
	outTreeContext->version = 0;
	outTreeContext->info = (void*) inUrl;
	outTreeContext->retain = CFRetain;
	outTreeContext->release = CFRelease;
	outTreeContext->copyDescription = CFCopyDescription;
}



#pragma mark _________Restoring_Preset_Files_________

//-----------------------------------------------------------------------------
// input:  Audio Unit ComponentInstance, CFURL to AU preset file to restore
// output:  ComponentResult error code
ComponentResult RestoreAUStateFromPresetFile(AudioUnit inAUComponentInstance, const CFURLRef inPresetFileURL)
{
	if ( (inAUComponentInstance == NULL) || (inPresetFileURL == NULL) )
		return paramErr;

	CFPropertyListRef auStatePlist = CreatePropertyListFromXMLFile(inPresetFileURL);
	if (auStatePlist == NULL)
		return coreFoundationUnknownErr;

	ComponentResult error = AudioUnitSetProperty(inAUComponentInstance, kAudioUnitProperty_ClassInfo, 
						kAudioUnitScope_Global, (AudioUnitElement)0, &auStatePlist, sizeof(auStatePlist));

	CFRelease(auStatePlist);

	return error;
}

//-----------------------------------------------------------------------------
CFPropertyListRef CreatePropertyListFromXMLFile(const CFURLRef inFileURL)
{
	if (inFileURL == NULL)
		return NULL;

	// read the XML file
	CFDataRef fileData = NULL;
	SInt32 errorCode = 0;
	Boolean success = CFURLCreateDataAndPropertiesFromResource(kCFAllocatorDefault, inFileURL, &fileData, NULL, NULL, &errorCode);
	if ( !success || (fileData == NULL) )
		return NULL;

	// reconstitute the dictionary using the XML data
	CFStringRef errorString = NULL;
	CFPropertyListRef propertyList = CFPropertyListCreateFromXMLData(kCFAllocatorDefault, fileData, kCFPropertyListImmutable, &errorString);
	if (errorString != NULL)
		CFRelease(errorString);

	CFRelease(fileData);

	return propertyList;
}

const UInt32 kAUPresetOpenNavDialogKey = 'AUpo';
//-----------------------------------------------------------------------------
ComponentResult CustomRestoreAUPresetFile(AudioUnit inAUComponentInstance)
{
	if ( (inAUComponentInstance == NULL) )
		return paramErr;

	ComponentResult error = noErr;

	NavDialogCreationOptions dialogOptions;
	error = NavGetDefaultDialogCreationOptions(&dialogOptions);
	if (error != noErr)
		return error;
//	dialogOptions.optionFlags |= kNavNoTypePopup;
	dialogOptions.optionFlags &= ~kNavAllowMultipleFiles;
	dialogOptions.preferenceKey = kAUPresetOpenNavDialogKey;
	dialogOptions.modality = kWindowModalityNone;

	NavEventUPP eventProc = NewNavEventUPP(CustomOpenAUPresetNavEventHandler);
	NavObjectFilterUPP filterProc = NewNavObjectFilterUPP(CustomOpenAUPresetNavFilterProc);
	NavDialogRef dialog;
	error = NavCreateGetFileDialog(&dialogOptions, NULL, eventProc, NULL, filterProc, (void*)inAUComponentInstance, &dialog);
	if (error == noErr)
	{
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

		case kNavCBUserAction:
		{
			NavUserAction userAction = NavDialogGetUserAction(dialog);
			if (userAction != kNavUserActionOpen)
				break;

			NavReplyRecord reply;
			OSStatus error = NavDialogGetReply(dialog, &reply);
			if (error == noErr)
			{
				if (reply.validRecord)
				{
					AEKeyword theKeyword;
					DescType actualType;
					Size actualSize;
					FSRef presetFileFSRef;
					error = AEGetNthPtr(&(reply.selection), 1, typeFSRef, &theKeyword, &actualType, 
										&presetFileFSRef, sizeof(presetFileFSRef), &actualSize);
					if (error == noErr)
					{
						CFURLRef presetFileUrl = CFURLCreateFromFSRef(kCFAllocatorDefault, &presetFileFSRef);
						if (presetFileUrl != NULL)
						{
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
			switch (((inCallbackParams->eventData).eventDataParms).event->what)
			{
				case updateEvt:
//					MyHandleUpdateEvent(window, (EventRecord*)inCallbackParams->eventData.event);
					break;
			}
			break;
	}
}

//-----------------------------------------------------------------------------
Boolean CustomOpenAUPresetNavFilterProc(AEDesc * inItem, void * inInfo, void * inUserData, NavFilterModes inFilterMode)
{
	Boolean result = true;

	if (inFilterMode == kNavFilteringBrowserList)
	{
		NavFileOrFolderInfo * info = (NavFileOrFolderInfo*) inInfo;
		if ( !(info->isFolder) )
		{
//fprintf(stderr, "AEDesc type = %.4s\n", (char*) &(inItem->descriptorType));
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
OSStatus SetNavDialogAUPresetStartLocation(NavDialogRef inDialog, Component inAUComponent, Boolean inShouldCreateFolder)
{
	if ( (inDialog == NULL) || (inAUComponent == NULL) )
		return paramErr;

	FSRef presetFileDirRef;
	OSStatus error = FindPresetsDirForAU(inAUComponent, kUserDomain, inShouldCreateFolder, &presetFileDirRef);
	if (error == noErr)
	{
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
ComponentResult SaveAUStateToPresetFile(AudioUnit inAUComponentInstance)
{
	return SaveAUStateToPresetFile_Bundle(inAUComponentInstance, NULL);
}
//-----------------------------------------------------------------------------
// input:  Audio Unit ComponentInstance
// output:  ComponentResult error code
ComponentResult SaveAUStateToPresetFile_Bundle(AudioUnit inAUComponentInstance, CFBundleRef inBundle)
{
	if (inAUComponentInstance == NULL)
		return paramErr;

	gCurrentBundle = inBundle;
	if (gCurrentBundle == NULL)
		gCurrentBundle = CFBundleGetMainBundle();

	ComponentResult error = noErr;

/*
1st
get ClassInfo from AU
	if fails, give error
convert ClassInfo to XML/plist data
	if fails, give error
*/
	CFPropertyListRef auStatePlist = NULL;
	UInt32 auStateDataSize = sizeof(auStatePlist);
	error = AudioUnitGetProperty(inAUComponentInstance, kAudioUnitProperty_ClassInfo, 
						kAudioUnitScope_Global, (AudioUnitElement)0, &auStatePlist, &auStateDataSize);
	if (error != noErr)
		return error;
	if (auStatePlist == NULL)
		return coreFoundationUnknownErr;
	if (auStateDataSize < sizeof(auStatePlist))
		return coreFoundationUnknownErr;

/*
2nd:
Open a dialog
	open nib
*/
	error = CreateSavePresetDialog((Component)inAUComponentInstance, auStatePlist, inBundle);


	CFRelease(auStatePlist);

	return error;
}

//-----------------------------------------------------------------------------
OSStatus WritePropertyListToXMLFile(const CFPropertyListRef inPropertyList, const CFURLRef inFileURL)
{
	if ( (inPropertyList == NULL) || (inFileURL == NULL) )
		return paramErr;

	// convert the property list into XML data
	CFDataRef xmlData = CFPropertyListCreateXMLData(kCFAllocatorDefault, inPropertyList);
	if (xmlData == NULL)
		return coreFoundationUnknownErr;

	// write the XML data to the file
	SInt32 error = noErr;
	Boolean success = CFURLWriteDataAndPropertiesToResource(inFileURL, xmlData, NULL, &error);
	if ( !success && (error == noErr) )	// whuh?
		error = kCFURLUnknownError;

	CFRelease(xmlData);

	return error;
}

//-----------------------------------------------------------------------------
typedef struct {
	WindowRef dialogWindow;
	CFPropertyListRef auStateData;
	OSStatus dialogResult;
	Component auComponent;
} SaveAUPresetFileDialogInfo;

enum {
	kPresetNameTextControlSignature = 'name',
	kPresetNameTextControlID = 128,
	kDomainChoiceControlSignature = 'doma',
	kDomainChoiceControlID = 129,

	kDomainChoiceValue_User = 1,
	kDomainChoiceValue_Local,
	kDomainChoiceValue_Network
};

#define kAUPresetSaveDialogNibName	CFSTR("au-preset-save-dialog")
#define kAUPresetSaveDialogNibWindowName	CFSTR("Save dialog")

//-----------------------------------------------------------------------------
// create, show, and run modally our dialog window
OSStatus CreateSavePresetDialog(Component inAUComponent, CFPropertyListRef inAUStatePlist, CFBundleRef inBundle)
{
	if ( (inAUComponent == NULL) || (inAUStatePlist == NULL) )
		return paramErr;

	OSStatus error = noErr;

	// find the dialog nib
	IBNibRef nibRef = NULL;
	if (inBundle == NULL)
		error = CreateNibReference(kAUPresetSaveDialogNibName, &nibRef);
	else
		error = CreateNibReferenceWithCFBundle(inBundle, kAUPresetSaveDialogNibName, &nibRef);
	if (error != noErr)
		return error;

	// load the window inside it
	WindowRef dialogWindow = NULL;
	error = CreateWindowFromNib(nibRef, kAUPresetSaveDialogNibWindowName, &dialogWindow);
	if (error != noErr)
		return error;

	// we don't need the nib reference anymore
	DisposeNibReference(nibRef);

	SaveAUPresetFileDialogInfo dialogInfo;
	memset(&dialogInfo, 0, sizeof(dialogInfo));
	dialogInfo.auStateData = inAUStatePlist;
	dialogInfo.dialogWindow = dialogWindow;
	dialogInfo.auComponent = inAUComponent;

	// install our event handler
	EventHandlerUPP dialogEventHandlerUPP = NewEventHandlerUPP(SaveAUPresetFileDialogEventHandler);
	EventTypeSpec dialogEventsSpec[] = { { kEventClassCommand, kEventCommandProcess } };
	error = InstallWindowEventHandler(dialogWindow, dialogEventHandlerUPP, GetEventTypeCount(dialogEventsSpec), 
										dialogEventsSpec, (void*)(&dialogInfo), NULL);
	if (error != noErr)
		return error;

	// set the window title with the AU's name in there
	char auNameCString[256];
	memset(auNameCString, 0, sizeof(auNameCString));
	error = GetComponentNameAndManufacturerCStrings(inAUComponent, auNameCString, NULL);
	if (error == noErr)
	{
		CFStringRef dialogWindowTitle_firstPart = CFCopyLocalizedStringFromTableInBundle(CFSTR("Save preset file for"), CFSTR("dfx-au-utilities-localizable"), inBundle, CFSTR("window title of the regular (simple) save AU preset dialog.  (note:  the code will append the name of the AU after this string, so format the syntax accordingly)"));
		CFStringRef dialogWindowTitle = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%@ %s"), dialogWindowTitle_firstPart, auNameCString);
		CFRelease(dialogWindowTitle_firstPart);
		if (dialogWindowTitle != NULL)
		{
			SetWindowTitleWithCFString(dialogWindow, dialogWindowTitle);
			CFRelease(dialogWindowTitle);
		}
	}

	// show the window
	ShowWindow(dialogWindow);

	ControlRef textFieldControl = NULL;
	ControlID textFieldControlID =	{ kPresetNameTextControlSignature, kPresetNameTextControlID };
	GetControlByID(dialogWindow, &textFieldControlID, &textFieldControl);
	SetKeyboardFocus(dialogWindow, textFieldControl, kControlFocusNextPart);

	// run modally
	RunAppModalLoopForWindow(dialogWindow);

/*
5th:
close and dispose dialog
return error code
*/
	HideWindow(dialogWindow);
	DisposeWindow(dialogWindow);
	DisposeEventHandlerUPP(dialogEventHandlerUPP);

	return dialogInfo.dialogResult;
}

//-----------------------------------------------------------------------------
// dialog event handler
pascal OSStatus SaveAUPresetFileDialogEventHandler(EventHandlerCallRef myHandler, EventRef inEvent, void * inUserData)
{
	if (GetEventClass(inEvent) != kEventClassCommand)
		return eventClassIncorrectErr;
	if (GetEventKind(inEvent) != kEventCommandProcess)
		return eventKindIncorrectErr;
	if (inUserData == NULL)
		return paramErr;

	OSStatus error = eventNotHandledErr;
	SaveAUPresetFileDialogInfo * dialogInfo = (SaveAUPresetFileDialogInfo*)inUserData;
	Boolean exitModalLoop = false;

/*
3rd:
catch dialog response
	save text input
		if Cancel hit, go to 5 and return userCanceledErr
		if Save hit, go to 4
		if Custom location, open Nav Services save dialog and go to 4-ish
*/
	// get the HI command
	HICommand command;
	error = GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &command);
	if (error != noErr)
		return error;

	// look for save or cancel commands
	switch (command.commandID)
	{
		case kHICommandSave:
			{
				ControlRef textFieldControl = NULL;
				ControlID textFieldControlID =	{ kPresetNameTextControlSignature, kPresetNameTextControlID };
				GetControlByID(dialogInfo->dialogWindow, &textFieldControlID, &textFieldControl);
				CFStringRef presetNameString = NULL;
				GetControlData(textFieldControl, 0, kControlEditTextCFStringTag, sizeof(presetNameString), &presetNameString, NULL);

				ControlRef domainChoiceControl = NULL;
				ControlID domainChoiceControlID = { kDomainChoiceControlSignature, kDomainChoiceControlID };
				GetControlByID(dialogInfo->dialogWindow, &domainChoiceControlID, &domainChoiceControl);
				SInt32 domainChoice = GetControl32BitValue(domainChoiceControl);
//fprintf(stderr, "domain choice = %ld\n", domainChoice);
				short fsDomain = kUserDomain;
				if (domainChoice == kDomainChoiceValue_Local)
					fsDomain = kLocalDomain;
				else if (domainChoice == kDomainChoiceValue_Network)
					fsDomain = kNetworkDomain;

				dialogInfo->dialogResult = TryToSaveAUPresetFile(dialogInfo->auComponent, dialogInfo->auStateData, presetNameString, fsDomain);
				if (presetNameString != NULL)
					CFRelease(presetNameString);

				OSStatus dresult = dialogInfo->dialogResult;
				// probably the file name was 0 characters long
				if (dresult == errFSMissingName)
					exitModalLoop = false;
				// probably there was an existing file conflict and the user chose not to replace the existing file
				else if (dresult == userCanceledErr)
					exitModalLoop = false;
				else if ( IsFileAccessError(dresult) )
				{
					if (fsDomain == kUserDomain)
						exitModalLoop = true;
					else
					{
						HandleSaveAUPresetFileAccessError(domainChoiceControl);
						exitModalLoop = false;
					}
				}
				else
					exitModalLoop = true;

				error = noErr;
			}
			break;
		case kHICommandCancel:
			exitModalLoop = true;
			dialogInfo->dialogResult = userCanceledErr;
			error = noErr;
			break;
		case kHICommandSaveAs:
			exitModalLoop = true;
			dialogInfo->dialogResult = CustomSaveAUPresetFile(dialogInfo->auComponent, dialogInfo->auStateData);
			break;
	}

	if (exitModalLoop)
		QuitAppModalLoopForWindow(dialogInfo->dialogWindow);

	return error;
}

//-----------------------------------------------------------------------------
OSStatus TryToSaveAUPresetFile(Component inAUComponent, CFPropertyListRef inAUStateData, 
								CFStringRef inPresetNameString, short inFileSystemDomain)
{
	if ( (inAUComponent == NULL) || (inAUStateData == NULL) || (inPresetNameString == NULL) )
		return paramErr;

	if (CFStringGetLength(inPresetNameString) <= 0)
		return errFSMissingName;

// XXX this isn't defined in the current release headers yet, only the Mac OS X 10.3 pre-release headers
#ifndef kAUPresetNameKey
#define kAUPresetNameKey	"name"
#endif
	// XXX is this really a good idea?  I think so, but I'm not totally sure...
	if (CFGetTypeID(inAUStateData) == CFDictionaryGetTypeID())
		CFDictionarySetValue((CFMutableDictionaryRef)inAUStateData, CFSTR(kAUPresetNameKey), inPresetNameString);

	OSStatus error = noErr;

//fprintf(stderr, "\tuser chosen name:\n"); CFShow(inPresetNameString);
	HFSUniStr255 dummyuniname;
	size_t maxUnicodeNameLength = sizeof(dummyuniname.unicode) / sizeof(UniChar);
	CFIndex presetFileNameExtensionLength = CFStringGetLength(kAUPresetFileNameExtension) - 1;	// -1 for the . before the extension
	CFIndex maxNameLength = maxUnicodeNameLength - presetFileNameExtensionLength;
	CFStringRef presetFileNameString = NULL;
	if (CFStringGetLength(inPresetNameString) > maxNameLength)
		presetFileNameString = CFStringCreateWithSubstring(kCFAllocatorDefault, inPresetNameString, CFRangeMake(0, maxNameLength));
	else
	{
		presetFileNameString = inPresetNameString;
		CFRetain(presetFileNameString);
	}
	if (presetFileNameString == NULL)
		return coreFoundationUnknownErr;
//fprintf(stderr, "\ttruncated name without extension:\n"); CFShow(presetFileNameString);

	FSRef presetFileDirRef;
	error = FindPresetsDirForAU(inAUComponent, inFileSystemDomain, kCreateFolder, &presetFileDirRef);
	if (error != noErr)
		return error;
	CFURLRef presetFileDirUrl = CFURLCreateFromFSRef(kCFAllocatorDefault, &presetFileDirRef);
	if (presetFileDirUrl == NULL)
	{
		CFRelease(presetFileNameString);
		return coreFoundationUnknownErr;
	}
	CFURLRef presetBaseFileUrl = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, presetFileDirUrl, presetFileNameString, false);
	CFRelease(presetFileDirUrl);
	CFRelease(presetFileNameString);
	if (presetBaseFileUrl == NULL)
		return coreFoundationUnknownErr;
	CFURLRef presetFullFileUrl = presetBaseFileUrl;
	// maybe the user already added the .aupreset extension when entering in the name?
	if ( CFURLIsAUPreset(presetFullFileUrl) )
		CFRetain(presetFullFileUrl);
	else
		presetFullFileUrl = CFURLCreateCopyAppendingPathExtension(kCFAllocatorDefault, presetBaseFileUrl, kAUPresetFileNameExtension);
	CFRelease(presetBaseFileUrl);
	if (presetFullFileUrl == NULL)
		return coreFoundationUnknownErr;
//fprintf(stderr, "\ttruncated name with extension:\n"); CFShow(presetFullFileUrl);

	// check whether or not the file already exists
	FSRef dummyFSRef;
	// note that, since an FSRef cannot be created for a file that doesn't yet exist, 
	// if this succeeds, then that means that a file with this path and name already exists
	Boolean fileAlreadyExists = CFURLGetFSRef(presetFullFileUrl, &dummyFSRef);
	if (fileAlreadyExists)
	{
		if ( !ShouldReplaceExistingAUPresetFile(presetFullFileUrl) )
		{
			CFRelease(presetFullFileUrl);
			return userCanceledErr;
		}
	}

	error = WritePropertyListToXMLFile(inAUStateData, presetFullFileUrl);
	CFRelease(presetFullFileUrl);
//	if (error == kCFURLResourceAccessViolationError)


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
Boolean ShouldReplaceExistingAUPresetFile(const CFURLRef inFileUrl)
{
	AlertStdCFStringAlertParamRec alertParams;
	GetStandardAlertDefaultParams(&alertParams, kStdCFStringAlertVersionOne);
	alertParams.movable = true;
	alertParams.defaultText = CFCopyLocalizedStringFromTableInBundle(CFSTR("Replace"), CFSTR("dfx-au-utilities-localizable"), gCurrentBundle, 
				CFSTR("text for the button that will over-write an existing file when a save file file-already-exists conflict arises"));
	alertParams.cancelText = (CFStringRef) kAlertDefaultCancelText;
	alertParams.defaultButton = kAlertStdAlertCancelButton;
//	alertParams.cancelButton = kAlertStdAlertCancelButton;
	alertParams.cancelButton = 0;
	DialogRef dialog;
	CFStringRef filenamestring = CFURLCopyLastPathComponent(inFileUrl);
	CFStringRef dirstring = NULL;
	CFURLRef dirurl = CFURLCreateCopyDeletingLastPathComponent(kCFAllocatorDefault, inFileUrl);
	if (dirurl != NULL)
	{
		dirstring = CFURLCopyFileSystemPath(dirurl, kCFURLPOSIXPathStyle);
		CFRelease(dirurl);
	}
	CFStringRef titleString = CFCopyLocalizedStringFromTableInBundle(CFSTR("Save"), CFSTR("dfx-au-utilities-localizable"), gCurrentBundle, 
								CFSTR("title of the alert window when a save file file-already-exists conflict arises"));
	CFStringRef messageString = NULL;
	if ( (filenamestring != NULL) && (dirstring != NULL) )
	{
		CFStringRef messageOutlineString = CFCopyLocalizedStringFromTableInBundle(CFSTR("A file named \"%@\" already exists in \"%@\".  Do you want to replace it with the file that you are saving?"), CFSTR("dfx-au-utilities-localizable"), gCurrentBundle, 
				CFSTR("the message in the alert, specifying file name and location, for when file name and path are available as CFStrings"));
		messageString = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, messageOutlineString, filenamestring, dirstring);
		CFRelease(messageOutlineString);
	}
	else
	{
		messageString = CFCopyLocalizedStringFromTableInBundle(CFSTR("A file with the same name already exists in this location.  Do you want to replace it with the file that you are saving?"), CFSTR("dfx-au-utilities-localizable"), gCurrentBundle, 
							CFSTR("the message in the alert, general, for when file name and path are not available"));
	}
	if (filenamestring != NULL)
		CFRelease(filenamestring);
	if (dirstring != NULL)
		CFRelease(dirstring);
	OSStatus alertErr = CreateStandardAlert(kAlertNoteAlert, titleString, messageString, &alertParams, &dialog);
	CFRelease(alertParams.defaultText);
	CFRelease(titleString);
	CFRelease(messageString);
	if (alertErr == noErr)
	{
		DialogItemIndex itemHit;
		alertErr = RunStandardAlert(dialog, NULL, &itemHit);
		if (alertErr == noErr)
		{
			if (itemHit == kAlertStdAlertCancelButton)
				return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
Boolean IsFileAccessError(OSStatus inErrorCode)
{
	if ( (inErrorCode == kCFURLResourceAccessViolationError) || (inErrorCode == afpAccessDenied) || 
			(inErrorCode == permErr) || (inErrorCode == wrPermErr) )
		return true;
	else
		return false;
}

//-----------------------------------------------------------------------------
OSStatus HandleSaveAUPresetFileAccessError(ControlRef inDomainChoiceControl)
{
	AlertStdCFStringAlertParamRec alertParams;
	GetStandardAlertDefaultParams(&alertParams, kStdCFStringAlertVersionOne);
	DialogRef dialog;
	CFStringRef alertTitle = CFCopyLocalizedStringFromTableInBundle(CFSTR("Save access error"), CFSTR("dfx-au-utilities-localizable"), gCurrentBundle, 
								CFSTR("the alert window title for when an access privileges error occurs while trying to save a file"));
	CFStringRef alertMessage = CFCopyLocalizedStringFromTableInBundle(CFSTR("You do not have sufficient privileges to save files in that location.  Try saving your file in the User domain."), CFSTR("dfx-au-utilities-localizable"), gCurrentBundle, 
								CFSTR("the content of the alert message text for an access privileges error"));
	OSStatus error = CreateStandardAlert(kAlertNoteAlert, alertTitle, alertMessage, &alertParams, &dialog);
	CFRelease(alertTitle);
	CFRelease(alertMessage);
	if (error == noErr)
	{
		DialogItemIndex itemHit;
		error = RunStandardAlert(dialog, NULL, &itemHit);
	}

	if (inDomainChoiceControl != NULL)
		SetControl32BitValue(inDomainChoiceControl, kDomainChoiceValue_User);

	return error;
}

const UInt32 kAUPresetSaveNavDialogKey = 'AUps';
//-----------------------------------------------------------------------------
OSStatus CustomSaveAUPresetFile(Component inAUComponent, CFPropertyListRef inAUStateData)
{
	if ( (inAUComponent == NULL) || (inAUStateData == NULL) )
		return paramErr;

	OSStatus error = noErr;

	NavDialogCreationOptions dialogOptions;
	error = NavGetDefaultDialogCreationOptions(&dialogOptions);
	if (error != noErr)
		return error;
	dialogOptions.optionFlags |= kNavPreserveSaveFileExtension;
	dialogOptions.preferenceKey = kAUPresetSaveNavDialogKey;
//	dialogOptions.modality = kWindowModalityNone;	// why does this not work and make the dialog not appear?

	NavEventUPP eventProc = NewNavEventUPP(CustomSaveAUPresetNavEventHandler);
	NavDialogRef dialog;
	error = NavCreatePutFileDialog(&dialogOptions, (OSType)0, (OSType)0, eventProc, (void*)inAUStateData, &dialog);
	if (error == noErr)
	{
		SetNavDialogAUPresetStartLocation(dialog, inAUComponent, kCreateFolder);

		CFStringRef defaultFileBaseName = CFCopyLocalizedStringFromTableInBundle(CFSTR("untitled"), CFSTR("dfx-au-utilities-localizable"), gCurrentBundle, 
											CFSTR("the default preset file name for the Nav Services save file dialog"));
		CFStringRef defaultFileName = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%@.%@"), 
															defaultFileBaseName, kAUPresetFileNameExtension);
		CFRelease(defaultFileBaseName);
		if (defaultFileName != NULL)
		{
			NavDialogSetSaveFileName(dialog, defaultFileName);
			CFRelease(defaultFileName);
		}

		error = NavDialogRun(dialog);
	}
	if (eventProc != NULL)
		DisposeRoutineDescriptor(eventProc);

	return error;
}

//-----------------------------------------------------------------------------
pascal void CustomSaveAUPresetNavEventHandler(NavEventCallbackMessage inCallbackSelector, NavCBRecPtr inCallbackParams, NavCallBackUserData inUserData)
{
	CFPropertyListRef auStateData = (CFPropertyListRef) inUserData;
	NavDialogRef dialog = inCallbackParams->context;

	switch (inCallbackSelector)
	{
		case kNavCBStart:
			if (auStateData != NULL)
				CFRetain(auStateData);
			break;

		case kNavCBTerminate:
			if (dialog != NULL)
				NavDialogDispose(dialog);
			if (auStateData != NULL)
				CFRelease(auStateData);
			break;

		case kNavCBUserAction:
		{
			NavUserAction userAction = NavDialogGetUserAction(dialog);
			if (userAction != kNavUserActionSaveAs)
				break;

			NavReplyRecord reply;
			OSStatus error = NavDialogGetReply(dialog, &reply);
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
						CFURLRef parentDirUrl = CFURLCreateFromFSRef(kCFAllocatorDefault, &parentDirFSRef);
						if (parentDirUrl != NULL)
						{
							CFURLRef presetFileUrl = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, 
																		parentDirUrl, reply.saveFileName, false);
							CFRelease(parentDirUrl);
							if (presetFileUrl != NULL)
							{
CFShow(presetFileUrl);
//								if (reply.replacing)
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
			switch (((inCallbackParams->eventData).eventDataParms).event->what)
			{
				case updateEvt:
//					MyHandleUpdateEvent(window, (EventRecord*)inCallbackParams->eventData.event);
					break;
			}
			break;
	}
}
