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


#ifndef __DFX_AU_UTILITIES_H
#define __DFX_AU_UTILITIES_H


#ifdef __cplusplus
extern "C" {
#endif


#include <Carbon/Carbon.h>
#include <AudioUnit/AudioUnit.h>



// this is for getting a Component's version from the Component's resource cache
OSErr GetComponentVersionFromResource(Component inComponent, long * outVersion);

// these are CFArray callbacks for use when creating an AU's FactoryPresets array
const void * auPresetCFArrayRetainCallback(CFAllocatorRef inAllocator, const void * inPreset);
void auPresetCFArrayReleaseCallback(CFAllocatorRef inAllocator, const void * inPreset);
Boolean auPresetCFArrayEqualCallback(const void * inPreset1, const void * inPreset2);
CFStringRef auPresetCFArrayCopyDescriptionCallback(const void * inPreset);
// and this will initialize a CFArray callbacks structure to use the above callback functions
void auPresetCFArrayCallbacks_Init(CFArrayCallBacks * outArrayCallbacks);
extern const CFArrayCallBacks kAUPresetCFArrayCallbacks;

// these are convenience functions for sending parameter change notifications to all parameter listeners
void AUParameterChange_TellListeners_ScopeElement(AudioUnit inAUComponentInstance, AudioUnitParameterID inParameterID, 
									AudioUnitScope inScope, AudioUnitElement inElement);
// this one defaults to using global scope and element 0
void AUParameterChange_TellListeners(AudioUnit inAUComponentInstance, AudioUnitParameterID inParameterID);

// main
CFTreeRef CFTreeCreateFromAUPresetFilesInDomain(Component inComponent, short inFileSystemDomain);
ComponentResult SaveAUStateToPresetFile(AudioUnit inAUComponentInstance);
ComponentResult SaveAUStateToPresetFile_Bundle(AudioUnit inAUComponentInstance, CFBundleRef inBundle);
ComponentResult RestoreAUStateFromPresetFile(AudioUnit inAUComponentInstance, const CFURLRef inPresetFileURL);
ComponentResult CustomRestoreAUPresetFile(AudioUnit inAUComponentInstance);
// access
CFURLRef GetCFURLFromFileURLsTreeNode(const CFTreeRef inTree);
// handies
CFStringRef CopyAUPresetNameFromCFURL(const CFURLRef inAUPresetUrl);
Boolean CFURLIsAUPreset(const CFURLRef inUrl);
Boolean FSRefIsAUPreset(const FSRef * inFileRef);
OSStatus FindPresetsDirForAU(Component inComponent, short inFileSystemDomain, Boolean inCreateDir, FSRef * outDirRef);
OSStatus GetComponentNameAndManufacturerStrings(Component inComponent, CFStringRef * outNameString, CFStringRef * outManufacturerString);
OSStatus GetComponentNameAndManufacturerCStrings(Component inComponent, char * outNameString, char * outManufacturerString);



#ifdef __cplusplus
};
#endif
// end of extern "C"



#endif
// __DFX_AU_UTILITIES_H
