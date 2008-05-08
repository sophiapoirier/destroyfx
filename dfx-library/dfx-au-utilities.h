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


#ifndef __DFX_AU_UTILITIES_H
#define __DFX_AU_UTILITIES_H


#include <Carbon/Carbon.h>
#include <AudioUnit/AudioUnit.h>


#ifdef __cplusplus
extern "C" {
#endif



/* this is for getting a Component's version from the Component's resource cache */
extern OSErr GetComponentVersionFromResource(Component inComponent, long * outVersion);

/* these handle a CoreFoundation-like container object for AUPreset called CFAUPreset */
typedef const struct CFAUPreset * CFAUPresetRef;
extern CFAUPresetRef CFAUPresetCreate(CFAllocatorRef inAllocator, SInt32 inPresetNumber, CFStringRef inPresetName);
extern CFAUPresetRef CFAUPresetRetain(CFAUPresetRef inPreset);
extern void CFAUPresetRelease(CFAUPresetRef inPreset);
extern const CFArrayCallBacks kCFAUPresetArrayCallBacks;

/* these handle a CoreFoundation-like container object for AudioUnitOtherPluginDesc called CFAUOtherPluginDesc */
typedef const struct CFAUOtherPluginDesc * CFAUOtherPluginDescRef;
extern CFAUOtherPluginDescRef CFAUOtherPluginDescCreate(CFAllocatorRef inAllocator, UInt32 inFormat, OSType inTypeID, OSType inSubTypeID, OSType inManufacturerID);
extern CFAUOtherPluginDescRef CFAUOtherPluginDescCreateVST(CFAllocatorRef inAllocator, OSType inUniqueID);
extern CFAUOtherPluginDescRef CFAUOtherPluginDescCreateMAS(CFAllocatorRef inAllocator, OSType inEffectID, OSType inVariantID, OSType inManufacturerID);
extern CFAUOtherPluginDescRef CFAUOtherPluginDescRetain(CFAUOtherPluginDescRef inDesc);
extern void CFAUOtherPluginDescRelease(CFAUOtherPluginDescRef inDesc);
extern const CFArrayCallBacks kCFAUOtherPluginDescArrayCallBacks;

/* these are convenience functions for sending parameter change notifications to all parameter listeners */
extern void AUParameterChange_TellListeners_ScopeElement(AudioUnit inAUComponentInstance, AudioUnitParameterID inParameterID, 
									AudioUnitScope inScope, AudioUnitElement inElement);
/* this one defaults to using global scope and element 0 */
extern void AUParameterChange_TellListeners(AudioUnit inAUComponentInstance, AudioUnitParameterID inParameterID);

/* these are for getting the individual AU plugin name and manufacturer name strings (so you don't have to fetch and parse yourself) */
/* get yourself some CFStrings */
extern OSStatus CopyAUNameAndManufacturerStrings(Component inAUComponent, CFStringRef * outNameString, CFStringRef * outManufacturerString);
/* get yourself some C strings */
extern OSStatus GetAUNameAndManufacturerCStrings(Component inAUComponent, char * outNameString, char * outManufacturerString);

/* comparing ComponentDescriptions */
/* determine if 2 ComponentDescriptions are basically equal */
extern Boolean ComponentDescriptionsMatch(const ComponentDescription * inComponentDescription1, const ComponentDescription * inComponentDescription2);
/* determine if 2 ComponentDescriptions have matching manufacturer and sub-type codes */
extern Boolean ComponentDescriptionsMatch_Loosely(const ComponentDescription * inComponentDescription1, const ComponentDescription * inComponentDescription2);
/* determine if a ComponentDescription basically matches that of a particular Component */
extern Boolean ComponentAndDescriptionMatch(Component inComponent, const ComponentDescription * inComponentDescription);
/* determine if a ComponentDescription matches only the sub-type manufacturer codes of a particular Component */
extern Boolean ComponentAndDescriptionMatch_Loosely(Component inComponent, const ComponentDescription * inComponentDescription);

/* stuff for handling AU preset files... */
/* main */
extern ComponentResult SaveAUStateToPresetFile(AudioUnit inAUComponentInstance, CFStringRef inDefaultAUPresetName, CFURLRef * outSavedAUPresetFileURL);
extern ComponentResult SaveAUStateToPresetFile_Bundle(AudioUnit inAUComponentInstance, CFStringRef inDefaultAUPresetName, CFURLRef * outSavedAUPresetFileURL, CFBundleRef inBundle);
extern CFTreeRef CFTreeCreateFromAUPresetFilesInDomain(Component inAUComponent, short inFileSystemDomain);
extern ComponentResult RestoreAUStateFromPresetFile(AudioUnit inAUComponentInstance, CFURLRef inAUPresetFileURL);
extern ComponentResult CustomRestoreAUPresetFile(AudioUnit inAUComponentInstance);
extern OSStatus GetAUComponentDescriptionFromStateData(CFPropertyListRef inAUStateData, ComponentDescription * outComponentDescription);
extern OSStatus GetAUComponentDescriptionFromPresetFile(CFURLRef inAUPresetFileURL, ComponentDescription * outComponentDescription);
/* access */
extern CFURLRef GetCFURLFromFileURLsTreeNode(CFTreeRef inTree);
/* handies */
extern CFStringRef CopyAUPresetNameFromCFURL(CFURLRef inAUPresetFileURL);
extern Boolean CFURLIsAUPreset(CFURLRef inURL);
extern Boolean FSRefIsAUPreset(const FSRef * inFileRef);
extern OSStatus FindPresetsDirForAU(Component inAUComponent, short inFileSystemDomain, Boolean inCreateDir, FSRef * outDirRef);

/* system services availability / version-checking stuff */
extern long GetMacOSVersion();
extern long GetQuickTimeVersion();
extern UInt32 GetAudioToolboxFrameworkVersion();
extern Boolean IsAvailable_AU2rev1();
extern Boolean IsTransportStateProcSafe();



#ifdef __cplusplus
}
#endif
/* end of extern "C" */



#endif
/* __DFX_AU_UTILITIES_H */
