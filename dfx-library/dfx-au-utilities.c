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

#include <AudioToolbox/AudioUnitUtilities.h>	// for AUParameterListenerNotify


#pragma mark _________Component_Version_________

//-----------------------------------------------------------------------------
// This function will get the version of a Component from its 'thng' resource 
// rather than from opening it and using the kComponentVersionSelect selector.  
// This allows you to get the version without opening the component, which can 
// be much more efficient.  Note that, unlike GetComponentVersion or 
// CallComponentVersion, the first argument of this function is a Component, 
// not a ComponentInstance.  You can, however, cast a ComponentInstance to 
// Component for this function, if you want to do that for any reason.
OSErr GetComponentVersionFromResource(Component inComponent, long * outVersion)
{
	if ( (inComponent == NULL) || (outVersion == NULL) )
		return paramErr;

	// first we need to get the ComponentDescription so that we know 
	// what the Component's type, sub-type, and manufacturer codes are
	ComponentDescription desc;
	OSErr error = GetComponentInfo(inComponent, &desc, NULL, NULL, NULL);
	if (error != noErr)
		return error;

	// remember the current resource file (because we will change it)
	short curRes = CurResFile();
	short componentResFileID = kResFileNotOpened;
	error = OpenAComponentResFile(inComponent, &componentResFileID);
	// error or invalid resource ID, abort
	if (error != noErr)
		return error;
	// this shouldn't happen without an error, but...
	if (componentResFileID <= 0)
		return resFNotFound;
	UseResFile(componentResFileID);

	short thngResourceCount = Count1Resources(kComponentResourceType);
	error = ResError();	// catch any error from Count1Resources
	// only go on if we successfully found at least 1 thng resource
	// (again, this shouldn't happen without an error, but just in case...)
	if ( (thngResourceCount <= 0) && (error == noErr) )
		error = resNotFound;
	if (error != noErr)
	{
		UseResFile(curRes);	// revert
		CloseComponentResFile(componentResFileID);
		return error;
	}

	Boolean versionFound = false;
	// loop through all of the Component thng resources trying to 
	// find one that matches this Component description
	short i;
	for (i=0; i < thngResourceCount; i++)
	{
		// try to get a handle to this code resource
		Handle thngResourceHandle = Get1IndResource(kComponentResourceType, i+1);
		if (thngResourceHandle == NULL)
			continue;
		HLock(thngResourceHandle);
		ExtComponentResource * componentThng = (ExtComponentResource*) (*thngResourceHandle);
		if (componentThng == NULL)
			goto cleanupRes;
		// it's not a v2 extended resource, probably just v1, so it won't have the version value
		if (GetHandleSize(thngResourceHandle) < (Size)sizeof(ExtComponentResource))
			goto cleanupRes;

		// check to see if this is the thng resource for the particular Component that we are looking at
		// (there often is more than one Component described in the resource)
		if ( (componentThng->cd.componentType == desc.componentType) 
				&& (componentThng->cd.componentSubType == desc.componentSubType) 
				&& (componentThng->cd.componentManufacturer == desc.componentManufacturer) )
		{
			// the version was successfully retrieved; output it and break out of this loop
			*outVersion = componentThng->componentVersion;
			versionFound = true;
		}
	cleanupRes:
		ReleaseResource(thngResourceHandle);
		if (versionFound)
			break;
	}

	UseResFile(curRes);	// revert
	CloseComponentResFile(componentResFileID);

	if (!versionFound)
		return resNotFound;
	return noErr;
}






#pragma mark _________Factory_Presets_CFArray_________

//-----------------------------------------------------------------------------
// The following 4 functions are CFArray callbacks for use when creating 
// an AU's factory presets array to support kAudioUnitProperty_FactoryPresets.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// This function is called when an item (an AUPreset) is added to the CFArray.  
// At this time, we need to make a fully independent copy of the AUPreset so that 
// the data will be guaranteed to be valid regardless of what the AU does, and 
// regardless of whether the AU instance is still open or not.
// This means that we need to allocate memory for an AUPreset struct just for the 
// array and create a copy of the preset name string.
const void * auPresetCFArrayRetainCallback(CFAllocatorRef inAllocator, const void * inPreset)
{
	AUPreset * preset = (AUPreset*) inPreset;
	// first allocate new memory for the array's copy of this AUPreset
	AUPreset * newPreset = (AUPreset*) CFAllocatorAllocate(inAllocator, sizeof(AUPreset), 0);
	// set the number of the new copy to that of the input AUPreset
	newPreset->presetNumber = preset->presetNumber;
	// create a copy of the input AUPreset's name string for this new copy of the preset
	newPreset->presetName = CFStringCreateCopy(kCFAllocatorDefault, preset->presetName);
	// return the pointer to the new memory, which belongs to the array, rather than the input pointer
	return newPreset;
}

//-----------------------------------------------------------------------------
// This function is called when an item (an AUPreset) is removed from the CFArray 
// or when the array is released.
// Since the memory for the data belongs to the array, we need to release it all here.
void auPresetCFArrayReleaseCallback(CFAllocatorRef inAllocator, const void * inPreset)
{
	AUPreset * preset = (AUPreset*) inPreset;
	// first release the name string, CF-style, since it's a CFString
	if (preset->presetName != NULL)
		CFRelease(preset->presetName);
	// wipe out the data so that, if anyone tries to access stale memory later, it will be invalid
	preset->presetName = NULL;
	preset->presetNumber = 0;
	// and finally, free the memory for the AUPreset struct
	CFAllocatorDeallocate(inAllocator, (void*)inPreset);
}

//-----------------------------------------------------------------------------
// This function is called when someone wants to compare to items (AUPresets) 
// in the CFArray to see if they are equal or not.
// For our AUPresets, we will compare based on the preset number and the name string.
Boolean auPresetCFArrayEqualCallback(const void * inPreset1, const void * inPreset2)
{
	AUPreset * preset1 = (AUPreset*) inPreset1;
	AUPreset * preset2 = (AUPreset*) inPreset2;
	// the two presets are only equal if they have the same preset number and 
	// if the two name strings are the same (which we rely on the CF function to compare)
	return (preset1->presetNumber == preset2->presetNumber) && 
			(CFStringCompare(preset1->presetName, preset2->presetName, 0) == kCFCompareEqualTo);
}

//-----------------------------------------------------------------------------
// This function is called when someone wants to get a description of 
// a particular item (an AUPreset) as though it were a CF type.  
// That happens, for example, when using CFShow().  
// This will create and return a CFString that indicates that 
// the object is an AUPreset and tells the preset number and preset name.
CFStringRef auPresetCFArrayCopyDescriptionCallback(const void * inPreset)
{
	AUPreset * preset = (AUPreset*) inPreset;
	return CFStringCreateWithFormat(kCFAllocatorDefault, NULL, 
									CFSTR("AUPreset:\npreset number = %d\npreset name = %@"), 
									preset->presetNumber, preset->presetName);
}

//-----------------------------------------------------------------------------
// this will initialize a CFArray callbacks structure to use the above callback functions
void auPresetCFArrayCallbacks_Init(CFArrayCallBacks * outArrayCallbacks)
{
	if (outArrayCallbacks == NULL)
		return;
	// wipe the struct clean
	memset(outArrayCallbacks, 0, sizeof(*outArrayCallbacks));
	// set all of the values and function pointers in the callbacks struct
	outArrayCallbacks->version = 0;	// currently, 0 is the only valid version value for this
	outArrayCallbacks->retain = auPresetCFArrayRetainCallback;
	outArrayCallbacks->release = auPresetCFArrayReleaseCallback;
	outArrayCallbacks->copyDescription = auPresetCFArrayCopyDescriptionCallback;
	outArrayCallbacks->equal = auPresetCFArrayEqualCallback;
}

//-----------------------------------------------------------------------------
#if 1
const CFArrayCallBacks kAUPresetCFArrayCallbacks = {
	version: 0, 
	retain: auPresetCFArrayRetainCallback, 
	release: auPresetCFArrayReleaseCallback, 
	copyDescription: auPresetCFArrayCopyDescriptionCallback, 
	equal: auPresetCFArrayEqualCallback
};
#elif 0
const CFArrayCallBacks kAUPresetCFArrayCallbacks = {
	.version = 0, 
	.retain = auPresetCFArrayRetainCallback, 
	.release = auPresetCFArrayReleaseCallback, 
	.copyDescription = auPresetCFArrayCopyDescriptionCallback, 
	.equal = auPresetCFArrayEqualCallback
};
#else
	#if defined(__GNUC__)
		const CFArrayCallBacks kAUPresetCFArrayCallbacks;
		static void kAUPresetCFArrayCallbacks_constructor() __attribute__((constructor));
		static void kAUPresetCFArrayCallbacks_constructor()
		{
			auPresetCFArrayCallbacks_Init( (CFArrayCallBacks*) &kAUPresetCFArrayCallbacks );
		}
	#endif
#endif





#pragma mark _________Parameter_Change_Notifications_________

//--------------------------------------------------------------------------
// These are convenience functions for sending parameter change notifications 
// to all parameter listeners.  
// Use this when a parameter value in an AU changes and you need to make 
// other entities (like the host, a GUI, etc.) aware of the change.
void AUParameterChange_TellListeners_ScopeElement(AudioUnit inAUComponentInstance, AudioUnitParameterID inParameterID, 
									AudioUnitScope inScope, AudioUnitElement inElement)
{
	// set up an AudioUnitParameter struct with all of the necessary values
	AudioUnitParameter dirtyparam;
	memset(&dirtyparam, 0, sizeof(dirtyparam));	// zero out the struct
	dirtyparam.mAudioUnit = inAUComponentInstance;
	dirtyparam.mParameterID = inParameterID;
	dirtyparam.mScope = inScope;
	dirtyparam.mElement = inElement;
	// then send a parameter change notification to all parameter listeners
	AUParameterListenerNotify(NULL, NULL, &dirtyparam);
}

//--------------------------------------------------------------------------
// this one defaults to using global scope and element 0 
// (which are what most effects use for all of their parameters)
void AUParameterChange_TellListeners(AudioUnit inAUComponentInstance, AudioUnitParameterID inParameterID)
{
	AUParameterChange_TellListeners_ScopeElement(inAUComponentInstance, inParameterID, kAudioUnitScope_Global, (AudioUnitElement)0);
}
