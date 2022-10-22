/*
	Destroy FX AU Utilities is a collection of helpful utility functions 
	for creating and hosting Audio Unit plugins.
	Copyright (C) 2003-2022  Sophia Poirier
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
	
	To contact the author, please visit http://destroyfx.org 
	and use the contact form.
*/


#ifndef __OBJC__
	#error "you must compile the version of this file with a .m filename extension, not this file"
#elif !__has_feature(objc_arc)
	#error "you must compile this file with Automatic Reference Counting (ARC) enabled"
#endif

#include "dfx-au-utilities.h"

#import <AppKit/NSAlert.h>
#import <AppKit/NSButton.h>
#import <AppKit/NSImage.h>
#include <AudioToolbox/AudioUnitUtilities.h>  // for kAUParameterListener_AnyParameter
#import <Foundation/NSFileManager.h>

#include "dfx-au-utilities-private.h"


//--------------------------------------------------------------------------
// the file name extension that identifies a file as being an AU preset file
#define kAUPresetFileNameExtension	CFSTR("aupreset")
// the name of the directory in Library/Audio for AU preset files
static NSString* const kAUPresetsDirName = @"Presets";
// this is defined in the Carbon header MacErrors.h
static OSStatus const kDFX_coreFoundationUnknownErr = -4960;



#pragma mark -
#pragma mark Handling Files And Directories

//-----------------------------------------------------------------------------
// This function provides a CFURLRef for an AudioComponent's centralized preset files directory.
// 
// inAUComponent - The AudioComponent whose preset directory you want.
// inFileSystemDomain - The file system domain of the presets directory (there can be one in every valid domain).
// inCreateDir - If true, then create directories in the path to the location if they do not already exist.  
//		Otherwise, this function will fail if all of the directories in the complete path do not already exist.
// The result is null if something went wrong, otherwise it's a CFURL to which the caller owns a reference.
CFURLRef FindPresetsDirForAU(AudioComponent inAUComponent, DFXFileSystemDomain inFileSystemDomain, Boolean inCreateDir)
{
	NSSearchPathDomainMask const domainNS = (inFileSystemDomain == kDFXFileSystemDomain_Local) ? NSLocalDomainMask : NSUserDomainMask;
	OSStatus status = noErr;
	NSURL* url = nil;
	CFStringRef pluginNameCFString = NULL, manufacturerNameCFString = NULL;

	if (inAUComponent == NULL)
	{
		return NULL;
	}

	// get an URL for the Library directory
	url = [NSFileManager.defaultManager URLForDirectory:NSLibraryDirectory inDomain:domainNS appropriateForURL:nil create:inCreateDir error:nil];
	if (url == nil)
	{
		return NULL;
	}

	// Audio directory in the Library directory
	url = [url URLByAppendingPathComponent:@"Audio" isDirectory:YES];
	if (url == nil)
	{
		return NULL;
	}

	// Presets directory in the Audio directory
	url = [url URLByAppendingPathComponent:kAUPresetsDirName isDirectory:YES];
	if (url == nil)
	{
		return NULL;
	}

	// determine the name of the AU and the AU manufacturer so that we know the names of the presets sub-directories
	status = CopyAUNameAndManufacturerStrings(inAUComponent, &pluginNameCFString, &manufacturerNameCFString);
	if ((status != noErr) || (manufacturerNameCFString == NULL) || (pluginNameCFString == NULL))
	{
		return NULL;
	}

	// AU manufacturer directory in the Presets directory
	url = [url URLByAppendingPathComponent:(__bridge NSString*)manufacturerNameCFString isDirectory:YES];
	if (url != nil)
	{
		// the particular plugin's directory in the manufacturer directory
		url = [url URLByAppendingPathComponent:(__bridge NSString*)pluginNameCFString isDirectory:YES];
	}
	CFRelease(manufacturerNameCFString);
	CFRelease(pluginNameCFString);
	if (url == nil)
	{
		return NULL;
	}

	if (inCreateDir)
	{
		if (![NSFileManager.defaultManager createDirectoryAtURL:url withIntermediateDirectories:YES attributes:nil error:nil])
		{
			return NULL;
		}
	}

	return CFBridgingRetain(url);
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
		CFURLRef const baseNameURL = CFURLCreateCopyDeletingPathExtension(kCFAllocatorDefault, inAUPresetFileURL);
		if (baseNameURL != NULL)
		{
			// then chop off the parent directory path, keeping just the extensionless file name as a CFString
			baseFileNameString = CFURLCopyLastPathComponent(baseNameURL);
			CFRelease(baseNameURL);
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



#pragma mark -
#pragma mark Preset Files Tree

//-----------------------------------------------------------------------------
CFTreeRef CFTreeCreateFromAUPresetFilesInDomain(AudioComponent inAUComponent, DFXFileSystemDomain inFileSystemDomain)
{
	CFURLRef presetsDirURL = NULL;
	CFTreeRef tree = NULL;

	if (inAUComponent == NULL)
	{
		return NULL;
	}

	// first we need to find the directory for the AU's preset files in this domain
	presetsDirURL = FindPresetsDirForAU(inAUComponent, inFileSystemDomain, false);
	if (presetsDirURL == NULL)
	{
		return NULL;
	}

	// we start with a root tree that represents the parent directory of all AU preset files for this AU
	tree = CreateFileURLsTreeNode(presetsDirURL, kCFAllocatorDefault);
	if (tree != NULL)
	{
		// this will recursively traverse the preset files directory and all subdirectories and 
		// add tree nodes for all of those to the root tree
		CollectAllAUPresetFilesInDir(presetsDirURL, tree, inAUComponent);
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

	CFRelease(presetsDirURL);

	return tree;
}

//-----------------------------------------------------------------------------
// this is just a convenient way to get a reference a tree node's info pointer in its context, 
// which in the case of our CFURLs trees, is a CFURLRef
CFURLRef GetCFURLFromFileURLsTreeNode(CFTreeRef inTree)
{
	CFTreeContext treeContext = {0};
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
// given an URL to a file or directory and a CFAllocatorRef, this function will 
// initialize a CFTreeContext appropriately for our file URLs trees and then 
// create and return the new tree node, or null if anything went wrong.
CFTreeRef CreateFileURLsTreeNode(CFURLRef inItemURL, CFAllocatorRef inAllocator)
{
	CFTreeRef newNode = NULL;
	if (inItemURL != NULL)
	{
		CFTreeContext treeContext = {0};
		FileURLsCFTreeContext_Init(inItemURL, &treeContext);
		newNode = CFTreeCreate(inAllocator, &treeContext);
	}
	return newNode;
}

//-----------------------------------------------------------------------------
// This function does CreateFileURLsTreeNode and then also adds the new tree node 
// as a child to a parent tree, and if successful, returns a reference to the new tree node.
void AddFileItemToTree(CFURLRef inItemURL, CFTreeRef inParentTree, CFTreeRef* outItemNode)
{
	if ((inItemURL == NULL) || (inParentTree == NULL))
	{
		return;
	}
	CFTreeRef const newNode = CreateFileURLsTreeNode(inItemURL, CFGetAllocator(inParentTree));
	if (newNode != NULL)
	{
		CFTreeAppendChild(inParentTree, newNode);
		if (outItemNode)
		{
			*outItemNode = newNode;
		}
		else
		{
			// the new tree node was retained by the parent when it was added to the parent, so we can release it now
			CFRelease(newNode);
		}
	}
}

//-----------------------------------------------------------------------------
// This function will scan through a directory and examine every file that might be 
// an AU preset file.  If a child directory is encountered, then this function is 
// called recursively on that child directory so that we get everything.
// If any child sub-directories don't wind up having any AU preset files, then 
// they get removed from the tree.
// If the inAUComponent argument is not null, then AU preset files will be examined 
// to see if their ComponentDescription data matches that of the AudioComponent, and 
// if not, then those preset files will be excluded from the tree.
void CollectAllAUPresetFilesInDir(CFURLRef inDirURL, CFTreeRef inParentTree, AudioComponent inAUComponent)
{
	NSArray<NSURLResourceKey>* const resourceKeys = [NSArray arrayWithObjects:NSURLIsDirectoryKey, nil];

	if ((inDirURL == NULL) || (inParentTree == NULL))
	{
		return;
	}

	// first, we query the directory's contents
	NSArray<NSURL*>* const dirContents = [NSFileManager.defaultManager contentsOfDirectoryAtURL:(__bridge NSURL*)inDirURL includingPropertiesForKeys:resourceKeys options:NSDirectoryEnumerationSkipsHiddenFiles error:nil];
	if (dirContents == nil)
	{
		goto checkEmptyTree;
	}

	// this is the main loop through each directory item using the array of contents
	for (NSURL* const url in dirContents)
	{
		CFURLRef const urlCF = (__bridge CFURLRef)url;
		NSNumber* isDirectory = nil;
		BOOL const success = [url getResourceValue:&isDirectory forKey:NSURLIsDirectoryKey error:nil];
		// if the current item itself is a directory, then we recursively call this function on that sub-directory
		if (success && [isDirectory boolValue])
		{
			CFTreeRef newSubTree = NULL;
			AddFileItemToTree(urlCF, inParentTree, &newSubTree);
			if (newSubTree != NULL)
			{
				CollectAllAUPresetFilesInDir(urlCF, newSubTree, inAUComponent);
				CFRelease(newSubTree);
			}
		}
		// otherwise it's a file, so we add it (if it is an AU preset file)
		else if (CFURLIsAUPreset(urlCF))
		{
			// XXX should I also open and examine each file and make sure that it's a valid dictionary for the particular AU?
#if 1
			// we only do this examination if an AudioComponent argument was given
			if (inAUComponent != NULL)
			{
				// get the ComponentDescription from the AU preset file
				AudioComponentDescription presetDesc = {0};
				OSStatus const status = GetAUComponentDescriptionFromPresetFile(urlCF, &presetDesc);
				// if the preset's ComponentDescription doesn't match the AU's, then don't add this preset file
				if ((status == noErr) && !ComponentAndDescriptionMatch_Loosely(inAUComponent, &presetDesc))
				{
					continue;
				}
			}
#endif
			AddFileItemToTree(urlCF, inParentTree, NULL);
		}
	}

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
	(void)inContext;

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
OSStatus RestoreAUStateFromPresetFile(AudioComponentInstance inAUComponentInstance, CFURLRef inAUPresetFileURL)
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
		return (plistError != 0) ? plistError : kDFX_coreFoundationUnknownErr;
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
	SInt32 ignoredErrorCode = 0;
	if (outErrorCode == NULL)
	{
		outErrorCode = &ignoredErrorCode;
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
		*outErrorCode = kDFX_coreFoundationUnknownErr;
		return NULL;
	}
	success = CFReadStreamOpen(fileStream);
	if (!success)
	{
		*outErrorCode = kDFX_coreFoundationUnknownErr;
		CFRelease(fileStream);
		return NULL;
	}

	// recreate the dictionary using the file data
	propertyList = CFPropertyListCreateWithStream(kCFAllocatorDefault, fileStream, 0, kCFPropertyListImmutable, NULL, &error);
	if (propertyList == NULL)
	{
		if (error != NULL)
		{
			*outErrorCode = (SInt32)CFErrorGetCode(error);
			CFRelease(error);
		}
		else
		{
			*outErrorCode = kDFX_coreFoundationUnknownErr;
		}
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
	Boolean ignoredSuccess = false;

	if (outSuccess == NULL)
	{
		outSuccess = &ignoredSuccess;
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
OSStatus GetAUComponentDescriptionFromStateData(CFPropertyListRef inAUStateData, AudioComponentDescription* outComponentDescription)
{
	CFDictionaryRef auStateDictionary = NULL;
	AudioComponentDescription tempDesc = {0};
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
OSStatus GetAUComponentDescriptionFromPresetFile(CFURLRef inAUPresetFileURL, AudioComponentDescription* outComponentDescription)
{
	SInt32 plistError = 0;
	OSStatus status = noErr;
	CFPropertyListRef auStatePlist = NULL;
	AudioComponentDescription tempDesc = {0};

	if ((inAUPresetFileURL == NULL) || (outComponentDescription == NULL))
	{
		return kAudio_ParamError;
	}

	// the preset file's state data is stored as XML data, and so we first we need to convert it to a PropertyList
	plistError = 0;
	auStatePlist = CreatePropertyListFromXMLFile(inAUPresetFileURL, &plistError);
	if (auStatePlist == NULL)
	{
		return (plistError != 0) ? plistError : kDFX_coreFoundationUnknownErr;
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
OSStatus SaveAUStateToPresetFile(AudioComponentInstance inAUComponentInstance, CFStringRef inAUPresetNameString, CFURLRef* outSavedAUPresetFileURL, Boolean inPromptToReplaceFile)
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
OSStatus SaveAUStateToPresetFile_Bundle(AudioComponentInstance inAUComponentInstance, CFStringRef inAUPresetNameString, CFURLRef* outSavedAUPresetFileURL, Boolean inPromptToReplaceFile, CFBundleRef inBundle)
{
	OSStatus status = noErr;

	// get the absolute maximum length that a file name can be
	HFSUniStr255 const placeholderUniName;
	size_t const maxUnicodeNameLength = sizeof(placeholderUniName.unicode) / sizeof(UniChar);
	// this is how much longer the file name will be after the AU preset file name extension is appended
	CFIndex const presetFileNameExtensionLength = CFStringGetLength(kAUPresetFileNameExtension) + 1;  // +1 for the period before the extension
	// this is the maximum allowable length of the preset file's name without the extension
	CFIndex const maxNameLength = maxUnicodeNameLength - presetFileNameExtensionLength;

	CFPropertyListRef auStatePlist = NULL;
	CFStringRef presetFileNameString = NULL;
	CFURLRef presetFileDirURL = NULL;
	CFURLRef presetFileURL = NULL;

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
		return kAudio_ParamError;
	}

	status = CopyAUStatePropertyList(inAUComponentInstance, &auStatePlist);
	if (status != noErr)
	{
		return status;
	}
	if (auStatePlist == NULL)
	{
		return kDFX_coreFoundationUnknownErr;
	}
	// set the state data's name value to the requested name
	// do this before we mess around with the file name string (truncating, adding extension, etc.)
	SetAUPresetNameInStateData(&auStatePlist, inAUPresetNameString);

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
		return kDFX_coreFoundationUnknownErr;
	}

	// now we need to get the parent directory of where we will save this file
	presetFileDirURL = FindPresetsDirForAU(AudioComponentInstanceGetComponent(inAUComponentInstance), kDFXFileSystemDomain_User, true);
	if (presetFileDirURL == NULL)
	{
		CFRelease(auStatePlist);
		CFRelease(presetFileNameString);
		return kDFX_coreFoundationUnknownErr;
	}
	// create a CFURL of the requested file name in the proper AU presets directory
	presetFileURL = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, presetFileDirURL, presetFileNameString, false);
	CFRelease(presetFileNameString);
	CFRelease(presetFileDirURL);
	if (presetFileURL == NULL)
	{
		CFRelease(auStatePlist);
		return kDFX_coreFoundationUnknownErr;
	}

	// save the preset data to a file
	status = TryToSaveAUPresetFile(inAUComponentInstance, auStatePlist, &presetFileURL, inPromptToReplaceFile, inBundle);
	CFRelease(auStatePlist);
	if (status != noErr)
	{
		if (presetFileURL != NULL)
		{
			CFRelease(presetFileURL);
		}
		return status;
	}

	if (outSavedAUPresetFileURL != NULL)
	{
		*outSavedAUPresetFileURL = presetFileURL;
	}
	else if (presetFileURL != NULL)
	{
		CFRelease(presetFileURL);
	}

	return status;
}

//-----------------------------------------------------------------------------
// This is a convenience wrapper for CustomSaveAUPresetFile_Bundle for when 
// the caller is the main application.
OSStatus CustomSaveAUPresetFile(AudioComponentInstance inAUComponentInstance, CFURLRef inAUPresetFileURL, Boolean inPromptToReplaceFile)
{
	return CustomSaveAUPresetFile_Bundle(inAUComponentInstance, inAUPresetFileURL, inPromptToReplaceFile, NULL);
}

//-----------------------------------------------------------------------------
OSStatus CustomSaveAUPresetFile_Bundle(AudioComponentInstance inAUComponentInstance, CFURLRef inAUPresetFileURL, Boolean inPromptToReplaceFile, CFBundleRef inBundle)
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
		return kDFX_coreFoundationUnknownErr;
	}

	// set the state data's name value to the requested file name
	auPresetNameString = CopyAUPresetNameFromCFURL(inAUPresetFileURL);
	if (auPresetNameString != NULL)
	{
		SetAUPresetNameInStateData(&auStatePlist, auPresetNameString);
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
OSStatus CopyAUStatePropertyList(AudioComponentInstance inAUComponentInstance, CFPropertyListRef* outAUStatePlist)
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
		return kDFX_coreFoundationUnknownErr;
	}
	if (auStateDataSize != sizeof(*outAUStatePlist))
	{
		return kDFX_coreFoundationUnknownErr;
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
		return kDFX_coreFoundationUnknownErr;
	}
	success = CFWriteStreamOpen(fileStream);
	if (!success)
	{
		CFRelease(fileStream);
		return kDFX_coreFoundationUnknownErr;
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
			return (OSStatus)errorCode;
		}
		return kDFX_coreFoundationUnknownErr;
	}

	return (OSStatus)errorCode;
}

//-----------------------------------------------------------------------------
// This function takes an AU's state data, an URL to which to save the preset file, 
// and a CFBundle and then tries to save the AU's state data to the desired path.
OSStatus TryToSaveAUPresetFile(AudioComponentInstance inAUComponentInstance, CFPropertyListRef inAUStateData, CFURLRef* ioAUPresetFileURL, Boolean inPromptToReplaceFile, CFBundleRef inBundle)
{
	OSStatus const userCanceledErr = -128;  // this is defined in the Carbon header MacErrors.h
	Boolean fileAlreadyExists = false;

	if ((inAUStateData == NULL) || (ioAUPresetFileURL == NULL) || (*ioAUPresetFileURL == NULL))
	{
		return kAudio_ParamError;
	}

	// the file name needs the proper AU preset extension to be appended
	if (!CFURLIsAUPreset(*ioAUPresetFileURL))
	{
		CFURLRef const presetFullFileURL = CFURLCreateCopyAppendingPathExtension(kCFAllocatorDefault, *ioAUPresetFileURL, kAUPresetFileNameExtension);
		if (presetFullFileURL == NULL)
		{
			return kDFX_coreFoundationUnknownErr;
		}
		CFRelease(*ioAUPresetFileURL);
		*ioAUPresetFileURL = presetFullFileURL;
	}

	// check whether or not the file already exists
	fileAlreadyExists = CFURLResourceIsReachable(*ioAUPresetFileURL, NULL);
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
	return WritePropertyListToXMLFile(inAUStateData, *ioAUPresetFileURL);
}

//-----------------------------------------------------------------------------
// Given an URL represeting a requested-to-save AU preset file, 
// this function creates an alert dialog telling the user that an identically named 
// file already exists and asking the user if she really wants to replace that old file.
// A return value of true means that the user wants to replace the file, and 
// a return value of false means that the user does not want to replace the file.
Boolean ShouldReplaceExistingAUPresetFile(AudioComponentInstance inAUComponentInstance, CFURLRef inAUPresetFileURL, CFBundleRef inBundle)
{
	CFStringRef fileNameString = NULL;
	CFStringRef dirString = NULL;
	CFURLRef dirURL = NULL;
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

	dirURL = CFURLCreateCopyDeletingLastPathComponent(kCFAllocatorDefault, inAUPresetFileURL);
	if (dirURL != NULL)
	{
		dirString = CFURLCopyLastPathComponent(dirURL);
		CFRelease(dirURL);
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
		CFURLRef alertIconURL = NULL;
		UInt32 propertyDataSize = sizeof(alertIconURL);
		OSStatus const status = AudioUnitGetProperty(inAUComponentInstance, kAudioUnitProperty_IconLocation, 
													 kAudioUnitScope_Global, (AudioUnitElement)0, 
													 &alertIconURL, &propertyDataSize);
		if ((status == noErr) && (alertIconURL != NULL) && (propertyDataSize == sizeof(alertIconURL)))
		{
			NSImage* const iconImage = [[NSImage alloc] initWithContentsOfURL:(__bridge NSURL*)alertIconURL];
			CFRelease(alertIconURL);
			if (iconImage != nil)
			{
				alert.icon = iconImage;
			}
		}
	}

	// XXX ideally this would run as a sheet attached to the AU view's window, 
	// but handling the result asynchronously seems overly complicated for this
	alertResult = [alert runModal];

	CFRelease(titleString);
	CFRelease(messageString);
	CFRelease(replaceButtonString);
	CFRelease(cancelButtonString);

	return (alertResult == NSAlertFirstButtonReturn);
}

//-----------------------------------------------------------------------------
void SetAUPresetNameInStateData(CFPropertyListRef* ioAUStateData, CFStringRef inPresetName)
{
	if ((ioAUStateData == NULL) || (*ioAUStateData == NULL) || (inPresetName == NULL) || (CFGetTypeID(*ioAUStateData) != CFDictionaryGetTypeID()))
	{
		return;
	}

	CFMutableDictionaryRef const auStateDataMutableCopy = CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, (CFDictionaryRef)*ioAUStateData);
	if (auStateDataMutableCopy != NULL)
	{
		CFDictionarySetValue(auStateDataMutableCopy, CFSTR(kAUPresetNameKey), inPresetName);
		CFRelease(*ioAUStateData);
		*ioAUStateData = (CFPropertyListRef)auStateDataMutableCopy;
	}
}
