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
void CollectAllAUPresetFilesInDir(const FSRef * inDirRef, CFTreeRef inParentTree);
void SortCFTreeRecursively(CFTreeRef inTreeRoot, CFComparatorFunction inComparatorFunction, void * inContext);
CFComparisonResult FileURLsTreeComparatorFunction(const void * inTree1, const void * inTree2, void * inContext);
void FileURLsCFTreeContext_Init(const CFURLRef inURL, CFTreeContext * outTreeContext);

// restoring preset files
CFPropertyListRef CreatePropertyListFromXMLFile(const CFURLRef inXMLFileURL, SInt32 * outErrorCode);
pascal void CustomOpenAUPresetNavEventHandler(NavEventCallbackMessage inCallbackSelector, NavCBRecPtr inCallbackParams, NavCallBackUserData inUserData);
pascal Boolean CustomOpenAUPresetNavFilterProc(AEDesc * inItem, void * inInfo, void * inUserData, NavFilterModes inFilterMode);
OSStatus SetNavDialogAUPresetStartLocation(NavDialogRef inDialog, Component inAUComponent, Boolean inShouldCreateFolder);

// saving preset files
OSStatus WritePropertyListToXMLFile(const CFPropertyListRef inPropertyList, const CFURLRef inXMLFileURL);
OSStatus CreateSavePresetDialog(Component inAUComponent, CFPropertyListRef inAUStatePlist);
pascal OSStatus SaveAUPresetFileDialogEventHandler(EventHandlerCallRef myHandler, EventRef inEvent, void * inUserData);
OSStatus TryToSaveAUPresetFile(Component inAUComponent, CFPropertyListRef inAUStateData, 
								CFStringRef inPresetNameString, short inFileSystemDomain);
Boolean ShouldReplaceExistingAUPresetFile(const CFURLRef inAUPresetFileURL);
Boolean IsFileAccessError(OSStatus inErrorCode);
OSStatus HandleSaveAUPresetFileAccessError(ControlRef inDomainChoiceControl);
OSStatus CustomSaveAUPresetFile(CFPropertyListRef inAUStateData, Component inAUComponent);
pascal void CustomSaveAUPresetNavEventHandler(NavEventCallbackMessage inCallbackSelector, NavCBRecPtr inCallbackParams, NavCallBackUserData inUserData);



#ifdef __cplusplus
};
#endif
// end of extern "C"



#endif
// __DFX_AU_UTILITIES_PRIVATE_H
