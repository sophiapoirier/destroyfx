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


#include <Carbon/Carbon.h>
#include <AudioUnit/AudioUnit.h>


#ifdef __cplusplus
extern "C" {
#endif



// this is for getting a Component's version from the Component's resource cache
extern OSErr GetComponentVersionFromResource(Component inComponent, long * outVersion);

// these are CFArray callbacks for use when creating an AU's FactoryPresets array
extern const void * auPresetCFArrayRetainCallback(CFAllocatorRef inAllocator, const void * inPreset);
extern void auPresetCFArrayReleaseCallback(CFAllocatorRef inAllocator, const void * inPreset);
extern Boolean auPresetCFArrayEqualCallback(const void * inPreset1, const void * inPreset2);
extern CFStringRef auPresetCFArrayCopyDescriptionCallback(const void * inPreset);
// and this will initialize a CFArray callbacks structure to use the above callback functions
extern void AUPresetCFArrayCallbacks_Init(CFArrayCallBacks * outArrayCallbacks);
extern const CFArrayCallBacks kAUPresetCFArrayCallbacks;

// these are convenience functions for sending parameter change notifications to all parameter listeners
extern void AUParameterChange_TellListeners_ScopeElement(AudioUnit inAUComponentInstance, AudioUnitParameterID inParameterID, 
									AudioUnitScope inScope, AudioUnitElement inElement);
// this one defaults to using global scope and element 0
extern void AUParameterChange_TellListeners(AudioUnit inAUComponentInstance, AudioUnitParameterID inParameterID);

// these are for getting the individual AU plugin name and manufacturer name strings (so you don't have to fetch and parse yourself)
// get yourself some CFStrings
extern OSStatus CopyAUNameAndManufacturerStrings(Component inAUComponent, CFStringRef * outNameString, CFStringRef * outManufacturerString);
// get yourself some C strings
extern OSStatus GetAUNameAndManufacturerCStrings(Component inAUComponent, char * outNameString, char * outManufacturerString);

// stuff for handling AU preset files...
// main
extern ComponentResult SaveAUStateToPresetFile(AudioUnit inAUComponentInstance, CFStringRef inDefaultAUPresetName, CFURLRef * outSavedAUPresetFileURL);
extern ComponentResult SaveAUStateToPresetFile_Bundle(AudioUnit inAUComponentInstance, CFStringRef inDefaultAUPresetName, CFURLRef * outSavedAUPresetFileURL, CFBundleRef inBundle);
extern CFTreeRef CFTreeCreateFromAUPresetFilesInDomain(Component inAUComponent, short inFileSystemDomain);
extern ComponentResult RestoreAUStateFromPresetFile(AudioUnit inAUComponentInstance, const CFURLRef inAUPresetFileURL);
extern ComponentResult CustomRestoreAUPresetFile(AudioUnit inAUComponentInstance);
// access
extern CFURLRef GetCFURLFromFileURLsTreeNode(const CFTreeRef inTree);
// handies
extern CFStringRef CopyAUPresetNameFromCFURL(const CFURLRef inAUPresetFileURL);
extern Boolean CFURLIsAUPreset(const CFURLRef inURL);
extern Boolean FSRefIsAUPreset(const FSRef * inFileRef);
extern OSStatus FindPresetsDirForAU(Component inAUComponent, short inFileSystemDomain, Boolean inCreateDir, FSRef * outDirRef);

// system services availability / version-checking stuff
extern long GetMacOSVersion();
extern long GetQuickTimeVersion();
extern UInt32 GetAudioToolboxFrameworkVersion();
extern Boolean IsAvailable_AU2rev1();



#ifdef __cplusplus
}
#endif
// end of extern "C"



#endif
// __DFX_AU_UTILITIES_H
