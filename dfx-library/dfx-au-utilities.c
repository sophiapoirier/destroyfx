/* This source code is by Marc Poirier, 2003.  It is offered as public domain. */

#include "dfx-au-utilities.h"


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
	if ( (error != noErr) || (componentResFileID <= 0) )
		return error;
	UseResFile(componentResFileID);

	short thngResourceCount = Count1Resources(kComponentResourceType);
	error = ResError();	// catch any error from Count1Resources
	// only go on if we successfully found at least 1 thng resource
	if ( (thngResourceCount <= 0) || (error != noErr) )
	{
		UseResFile(curRes);	// revert
		CloseComponentResFile(componentResFileID);
		return error;
	}

	int versionFound = 0;
	// loop through all of the Component thng resources trying to 
	// find one that matches this Component description
	short i;
	for (i=0; i < thngResourceCount; i++)
	{
		// try to get a handle to this code resource
		Handle thngResourceHandle = Get1IndResource(kComponentResourceType, i+1);
		if (thngResourceHandle == NULL)
			continue;
		ExtComponentResource * componentThng = (ExtComponentResource*) (*thngResourceHandle);
		if (componentThng == NULL)
			goto cleanupRes;
		// it's not a v2 extended resource, probably just v1, so it won't have the version value
		if (GetHandleSize(thngResourceHandle) < (Size)sizeof(ExtComponentResource))
			goto cleanupRes;

		// check to see if this is the thng resource for the particular Component that we are looking at
		// (there often is more than one Component described in the resource)
		if ( (componentThng->cd.componentType != desc.componentType) 
				|| (componentThng->cd.componentSubType != desc.componentSubType) 
				|| (componentThng->cd.componentManufacturer != desc.componentManufacturer) )
			goto cleanupRes;

		// the version was successfully retrieved; output it and break out of this loop
		*outVersion = componentThng->componentVersion;
		versionFound = 1;
	cleanupRes:
		ReleaseResource(thngResourceHandle);
		if (versionFound)
			break;
	}

	UseResFile(curRes);	// revert
	CloseComponentResFile(componentResFileID);

	if (!versionFound)
		return resFNotFound;
	return noErr;
}



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
// this will init a CFArray callbacks struct to use the above callback functions
void auPresetCFArrayCallbacks_Init(CFArrayCallBacks * inArrayCallbacks)
{
	if (inArrayCallbacks == NULL)
		return;
	// wipe the struct clean
	memset(inArrayCallbacks, 0, sizeof(*inArrayCallbacks));
	// set all of the values and function pointers in the callbacks struct
	inArrayCallbacks->version = 0;	// currently, 0 is the only valid version value for this
	inArrayCallbacks->retain = auPresetCFArrayRetainCallback;
	inArrayCallbacks->release = auPresetCFArrayReleaseCallback;
	inArrayCallbacks->copyDescription = auPresetCFArrayCopyDescriptionCallback;
	inArrayCallbacks->equal = auPresetCFArrayEqualCallback;
}
