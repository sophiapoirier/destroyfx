/*
	Destroy FX AU Utilities is a collection of helpful utility functions 
	for creating and hosting Audio Unit plugins.
	Copyright (C) 2003-2021  Sophia Poirier
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


#ifndef DFX_AU_UTILITIES_H
#define DFX_AU_UTILITIES_H


#include <AudioToolbox/AUComponent.h>
#include <AudioToolbox/AudioComponent.h>
#include <CoreFoundation/CoreFoundation.h>


#ifdef __cplusplus
extern "C" {
#endif



/* these handle a CoreFoundation-like container object for AUPreset called CFAUPreset */
typedef struct CFAUPreset const* CFAUPresetRef;
extern CFAUPresetRef CFAUPresetCreate(CFAllocatorRef inAllocator, SInt32 inPresetNumber, CFStringRef inPresetName);
extern CFAUPresetRef CFAUPresetRetain(CFAUPresetRef inPreset);
extern void CFAUPresetRelease(CFAUPresetRef inPreset);
extern CFArrayCallBacks const kCFAUPresetArrayCallBacks;

/* these handle a CoreFoundation-like container object for AudioUnitOtherPluginDesc called CFAUOtherPluginDesc */
typedef struct CFAUOtherPluginDesc const* CFAUOtherPluginDescRef;
extern CFAUOtherPluginDescRef CFAUOtherPluginDescCreate(CFAllocatorRef inAllocator, UInt32 inFormat, OSType inTypeID, OSType inSubTypeID, OSType inManufacturerID);
extern CFAUOtherPluginDescRef CFAUOtherPluginDescCreateVST(CFAllocatorRef inAllocator, OSType inUniqueID);
extern CFAUOtherPluginDescRef CFAUOtherPluginDescCreateMAS(CFAllocatorRef inAllocator, OSType inEffectID, OSType inVariantID, OSType inManufacturerID);
extern CFAUOtherPluginDescRef CFAUOtherPluginDescRetain(CFAUOtherPluginDescRef inDesc);
extern void CFAUOtherPluginDescRelease(CFAUOtherPluginDescRef inDesc);
extern CFArrayCallBacks const kCFAUOtherPluginDescArrayCallBacks;

/* these are convenience functions for sending parameter change notifications to all parameter listeners */
extern void AUParameterChange_TellListeners_ScopeElement(AudioComponentInstance inAUComponentInstance, 
														 AudioUnitParameterID inParameterID, 
														 AudioUnitScope inScope, AudioUnitElement inElement);
/* this one defaults to using global scope and element 0 */
extern void AUParameterChange_TellListeners(AudioComponentInstance inAUComponentInstance, AudioUnitParameterID inParameterID);

/* this is for getting the individual AU plugin name and manufacturer name strings (so you don't have to fetch and parse yourself) */
extern OSStatus CopyAUNameAndManufacturerStrings(AudioComponent inAUComponent, CFStringRef* outNameString, CFStringRef* outManufacturerString);

/* comparing ComponentDescriptions */
/* determine if two ComponentDescriptions are basically equal */
extern Boolean ComponentDescriptionsMatch(AudioComponentDescription const* inComponentDescription1, AudioComponentDescription const* inComponentDescription2);
/* determine if two ComponentDescriptions have matching manufacturer and sub-type codes */
extern Boolean ComponentDescriptionsMatch_Loosely(AudioComponentDescription const* inComponentDescription1, AudioComponentDescription const* inComponentDescription2);
/* determine if an AudioComponentDescription basically matches that of a particular AudioComponent */
extern Boolean ComponentAndDescriptionMatch(AudioComponent inComponent, AudioComponentDescription const* inComponentDescription);
/* determine if an AudioComponentDescription matches only the sub-type manufacturer codes of a particular AudioComponent */
extern Boolean ComponentAndDescriptionMatch_Loosely(AudioComponent inComponent, AudioComponentDescription const* inComponentDescription);

/* stuff for handling AU preset files... */
typedef enum
{
	kDFXFileSystemDomain_Local,
	kDFXFileSystemDomain_User
} DFXFileSystemDomain;
/* main */
extern OSStatus SaveAUStateToPresetFile(AudioComponentInstance inAUComponentInstance, CFStringRef inAUPresetNameString, CFURLRef* outSavedAUPresetFileURL, Boolean inPromptToReplaceFile);
extern OSStatus SaveAUStateToPresetFile_Bundle(AudioComponentInstance inAUComponentInstance, CFStringRef inAUPresetNameString, CFURLRef* outSavedAUPresetFileURL, Boolean inPromptToReplaceFile, CFBundleRef inBundle);
extern OSStatus CustomSaveAUPresetFile(AudioComponentInstance inAUComponentInstance, CFURLRef inAUPresetFileURL, Boolean inPromptToReplaceFile);
extern OSStatus CustomSaveAUPresetFile_Bundle(AudioComponentInstance inAUComponentInstance, CFURLRef inAUPresetFileURL, Boolean inPromptToReplaceFile, CFBundleRef inBundle);
extern CFTreeRef CFTreeCreateFromAUPresetFilesInDomain(AudioComponent inAUComponent, DFXFileSystemDomain inFileSystemDomain);
extern OSStatus RestoreAUStateFromPresetFile(AudioComponentInstance inAUComponentInstance, CFURLRef inAUPresetFileURL);
extern OSStatus GetAUComponentDescriptionFromStateData(CFPropertyListRef inAUStateData, AudioComponentDescription* outComponentDescription);
extern OSStatus GetAUComponentDescriptionFromPresetFile(CFURLRef inAUPresetFileURL, AudioComponentDescription* outComponentDescription);
/* access */
extern CFURLRef GetCFURLFromFileURLsTreeNode(CFTreeRef inTree);
/* handies */
extern CFStringRef CopyAUPresetNameFromCFURL(CFURLRef inAUPresetFileURL);
extern Boolean CFURLIsAUPreset(CFURLRef inURL);
extern CFURLRef FindPresetsDirForAU(AudioComponent inAUComponent, DFXFileSystemDomain inFileSystemDomain, Boolean inCreateDir);



#ifdef __cplusplus
}
/* end of extern "C" */

#if (__cplusplus >= 201103L)
#include <memory>
namespace dfx
{
	static inline auto MakeUniqueCFAUPreset(CFAllocatorRef inAllocator, SInt32 inPresetNumber, CFStringRef inPresetName) noexcept
	{
		return std::unique_ptr<CFAUPreset const, void(*)(CFAUPresetRef)>(CFAUPresetCreate(inAllocator, inPresetNumber, inPresetName), ::CFAUPresetRelease);
	}

	static inline auto MakeUniqueCFAUOtherPluginDesc(CFAllocatorRef inAllocator, UInt32 inFormat, OSType inTypeID, OSType inSubTypeID, OSType inManufacturerID) noexcept
	{
		return std::unique_ptr<CFAUOtherPluginDesc const, void(*)(CFAUOtherPluginDescRef)>(CFAUOtherPluginDescCreate(inAllocator, inFormat, inTypeID, inSubTypeID, inManufacturerID), ::CFAUOtherPluginDescRelease);
	}
}
#endif

#endif



#endif
/* DFX_AU_UTILITIES_H */
