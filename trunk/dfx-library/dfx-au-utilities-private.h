/*
	Destroy FX AU Utilities is a collection of helpful utility functions 
	for creating and hosting Audio Unit plugins.
	Copyright (C) 2003-2008  Sophia Poirier
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


#ifndef __DFX_AU_UTILITIES_PRIVATE_H
#define __DFX_AU_UTILITIES_PRIVATE_H


#include <Carbon/Carbon.h>
#include <AudioUnit/AudioUnit.h>


#ifdef __cplusplus
extern "C" {
#endif



// handling files and directories
OSStatus MakeFSRefInDir(const FSRef * inParentDirRef, CFStringRef inItemNameString, Boolean inCreateItem, FSRef * outItemRef);
void TranslateCFStringToUnicodeString(CFStringRef inCFString, HFSUniStr255 * outUniName);

// preset file trees
CFTreeRef CreateFileURLsTreeNode(const FSRef * inItemRef, CFAllocatorRef inAllocator);
CFTreeRef AddFileItemToTree(const FSRef * inItemRef, CFTreeRef inParentTree);
void CollectAllAUPresetFilesInDir(const FSRef * inDirRef, CFTreeRef inParentTree, Component inAUComponent);
void SortCFTreeRecursively(CFTreeRef inTreeRoot, CFComparatorFunction inComparatorFunction, void * inContext);
CFComparisonResult FileURLsTreeComparatorFunction(const void * inTree1, const void * inTree2, void * inContext);
void FileURLsCFTreeContext_Init(CFURLRef inURL, CFTreeContext * outTreeContext);

// restoring preset files
CFPropertyListRef CreatePropertyListFromXMLFile(CFURLRef inXMLFileURL, SInt32 * outErrorCode);
pascal void CustomOpenAUPresetNavEventHandler(NavEventCallbackMessage inCallbackSelector, NavCBRecPtr inCallbackParams, NavCallBackUserData inUserData);
pascal Boolean CustomOpenAUPresetNavFilterProc(AEDesc * inItem, void * inInfo, void * inUserData, NavFilterModes inFilterMode);
OSStatus SetNavDialogAUPresetStartLocation(NavDialogRef inDialog, Component inAUComponent, Boolean inShouldCreateFolder);

// saving preset files
OSStatus WritePropertyListToXMLFile(CFPropertyListRef inPropertyList, CFURLRef inXMLFileURL);
OSStatus CreateSavePresetDialog(Component inAUComponent, CFPropertyListRef inAUStatePlist, 
								CFStringRef inDefaultAUPresetName, CFURLRef * outSavedAUPresetFileURL);
pascal OSStatus SaveAUPresetFileDialogEventHandler(EventHandlerCallRef myHandler, EventRef inEvent, void * inUserData);
OSStatus TryToSaveAUPresetFile(Component inAUComponent, CFPropertyListRef inAUStateData, 
								CFStringRef inPresetNameString, short inFileSystemDomain, CFURLRef * outSavedAUPresetFileURL);
Boolean ShouldReplaceExistingAUPresetFile(CFURLRef inAUPresetFileURL);
pascal Boolean ShouldReplaceExistingAUPresetFileDialogFilterProc(DialogRef inDialog, EventRecord * inEvent, DialogItemIndex * outItemHit);
Boolean IsFileAccessError(OSStatus inErrorCode);
OSStatus HandleSaveAUPresetFileAccessError(ControlRef inDomainChoiceControl);
OSStatus CustomSaveAUPresetFile(CFPropertyListRef inAUStateData, Component inAUComponent, 
								CFStringRef inDefaultAUPresetName, CFURLRef * outSavedAUPresetFileURL);
pascal void CustomSaveAUPresetNavEventHandler(NavEventCallbackMessage inCallbackSelector, NavCBRecPtr inCallbackParams, NavCallBackUserData inUserData);



#ifdef __cplusplus
}
#endif
// end of extern "C"



#endif
// __DFX_AU_UTILITIES_PRIVATE_H
