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


#ifndef DFX_AU_UTILITIES_PRIVATE_H
#define DFX_AU_UTILITIES_PRIVATE_H


#include <AudioToolbox/AudioComponent.h>
#include <CoreFoundation/CoreFoundation.h>


#ifdef __cplusplus
extern "C" {
#endif



// preset file trees
CFTreeRef CreateFileURLsTreeNode(CFURLRef inItemURL, CFAllocatorRef inAllocator);
CFTreeRef AddFileItemToTree(CFURLRef inItemURL, CFTreeRef inParentTree);
void CollectAllAUPresetFilesInDir(CFURLRef inDirURL, CFTreeRef inParentTree, AudioComponent inAUComponent);
void SortCFTreeRecursively(CFTreeRef inTreeRoot, CFComparatorFunction inComparatorFunction, void* inContext);
CFComparisonResult FileURLsTreeComparatorFunction(void const* inTree1, void const* inTree2, void* inContext);
void FileURLsCFTreeContext_Init(CFURLRef inURL, CFTreeContext* outTreeContext);

// restoring preset files
CFPropertyListRef CreatePropertyListFromXMLFile(CFURLRef inXMLFileURL, SInt32* outErrorCode);

// saving preset files
OSStatus CopyAUStatePropertyList(AudioComponentInstance inAUComponentInstance, CFPropertyListRef* outAUStatePlist);
OSStatus WritePropertyListToXMLFile(CFPropertyListRef inPropertyList, CFURLRef inXMLFileURL);
OSStatus TryToSaveAUPresetFile(AudioComponentInstance inAUComponentInstance, CFPropertyListRef inAUStateData, CFURLRef* ioAUPresetFileURL, Boolean inPromptToReplaceFile, CFBundleRef inBundle);
Boolean ShouldReplaceExistingAUPresetFile(AudioComponentInstance inAUComponentInstance, CFURLRef inAUPresetFileURL, CFBundleRef inBundle);
void SetAUPresetNameInStateData(CFPropertyListRef* ioAUStateData, CFStringRef inPresetName);



#ifdef __cplusplus
}
#endif
// end of extern "C"



#endif
// DFX_AU_UTILITIES_PRIVATE_H
