/*
	DFX AU Utilities is a collection of helpful utility functions for 
	creating and hosting Audio Unit plugins.
	Copyright (C) 2003-2004  Marc Genung Poirier
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


#include "dfx-au-utilities.h"

#include <AudioToolbox/AudioUnitUtilities.h>	// for AUEventListenerNotify and AUParameterListenerNotify


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
	OSErr error;
	ComponentDescription desc;
	short curRes, componentResFileID;
	short thngResourceCount, i;
	Boolean versionFound = false;

	if ( (inComponent == NULL) || (outVersion == NULL) )
		return paramErr;

	// first we need to get the ComponentDescription so that we know 
	// what the Component's type, sub-type, and manufacturer codes are
	error = GetComponentInfo(inComponent, &desc, NULL, NULL, NULL);
	if (error != noErr)
		return error;

	// remember the current resource file (because we will change it)
	curRes = CurResFile();
	componentResFileID = kResFileNotOpened;
	error = OpenAComponentResFile(inComponent, &componentResFileID);
	// error or invalid resource ID, abort
	if (error != noErr)
		return error;
	// this shouldn't happen without an error, but...
	if (componentResFileID <= 0)
		return resFNotFound;
	UseResFile(componentResFileID);

	thngResourceCount = Count1Resources(kComponentResourceType);
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

	// loop through all of the Component thng resources trying to 
	// find one that matches this Component description
	for (i=0; i < thngResourceCount; i++)
	{
		ExtComponentResource * componentThng;

		// try to get a handle to this code resource
		Handle thngResourceHandle = Get1IndResource(kComponentResourceType, i+1);
		if (thngResourceHandle == NULL)
			continue;
		HLock(thngResourceHandle);
		componentThng = (ExtComponentResource*) (*thngResourceHandle);
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
void AUPresetCFArrayCallbacks_Init(CFArrayCallBacks * outArrayCallbacks)
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
#ifdef __GNUC__
// XXX what is the best way to do this?
#if 0
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
// this seems to be the only way of the 3 that is fully valid C, but unfortunately only for GCC
const CFArrayCallBacks kAUPresetCFArrayCallbacks;
static void kAUPresetCFArrayCallbacks_constructor() __attribute__((constructor));
static void kAUPresetCFArrayCallbacks_constructor()
{
	AUPresetCFArrayCallbacks_Init( (CFArrayCallBacks*) &kAUPresetCFArrayCallbacks );
}
#endif
#else
// XXX I'll use this for other compilers, even though I hate initializing structs with all arguments at once 
// (cuz what if I ever decided to change the order of the struct members or something like that?)
const CFArrayCallBacks kAUPresetCFArrayCallbacks = {
	0, 
	auPresetCFArrayRetainCallback, 
	auPresetCFArrayReleaseCallback, 
	auPresetCFArrayCopyDescriptionCallback, 
	auPresetCFArrayEqualCallback
};
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
	// set up an AudioUnitParameter structure with all of the necessary values
	AudioUnitParameter dirtyParam;
	memset(&dirtyParam, 0, sizeof(dirtyParam));	// zero out the struct
	dirtyParam.mAudioUnit = inAUComponentInstance;
	dirtyParam.mParameterID = inParameterID;
	dirtyParam.mScope = inScope;
	dirtyParam.mElement = inElement;

/*
// XXX actually, using AUEventListenerNotify is not necessary since it does the exact same thing as AUParameterListenerNotify in this case
	// then send a parameter change notification to all parameter listeners
	// do the new-fangled way, if it's available on the user's system
	if (AUEventListenerNotify != NULL)
	{
		// set up an AudioUnitEvent structure, which includes the AudioUnitParameter structure
		AudioUnitEvent paramEvent;
		memset(&paramEvent, 0, sizeof(paramEvent));	// zero out the struct
		paramEvent.mEventType = kAudioUnitEvent_ParameterValueChange;
		paramEvent.mArgument.mParameter = dirtyParam;
		AUEventListenerNotify(NULL, NULL, &paramEvent);
	}
	// if that's unavailable, then send notification the old way
	else
*/
		AUParameterListenerNotify(NULL, NULL, &dirtyParam);
}

//--------------------------------------------------------------------------
// this one defaults to using global scope and element 0 
// (which are what most effects use for all of their parameters)
void AUParameterChange_TellListeners(AudioUnit inAUComponentInstance, AudioUnitParameterID inParameterID)
{
	AUParameterChange_TellListeners_ScopeElement(inAUComponentInstance, inParameterID, kAudioUnitScope_Global, (AudioUnitElement)0);
}






#pragma mark _________AU_Plugin_Name_And_Manufacturer_Name_________

//-----------------------------------------------------------------------------
//Tthis is a wrapper function for GetAUNameAndManufacturerCStrings, 
// since often it's handier to have CFStrings than C strings.
// One of the string arguments can be NULL, if you are not interested in that string.
OSStatus CopyAUNameAndManufacturerStrings(Component inAUComponent, CFStringRef * outNameString, CFStringRef * outManufacturerString)
{
	OSStatus error;
	char pluginNameCString[sizeof(Str255)], manufacturerNameCString[sizeof(Str255)];

	// one input string or the other can be null, but not both
	if ( (inAUComponent == NULL) || ((outNameString == NULL) && (outManufacturerString == NULL)) )
		return paramErr;

	// initialize some C string buffers for storing the C string versions of the AU name strings
	memset(pluginNameCString, 0, sizeof(pluginNameCString));
	memset(manufacturerNameCString, 0, sizeof(manufacturerNameCString));
	// this will get us C string versions of the AU name strings
	error = GetAUNameAndManufacturerCStrings(inAUComponent, pluginNameCString, manufacturerNameCString);
	if (error != noErr)
		return error;

	// for each input CFString that is not null, we want to provide a CFString representation of the C string
	if (outNameString != NULL)
		*outNameString = CFStringCreateWithCString(kCFAllocatorDefault, pluginNameCString, CFStringGetSystemEncoding());
	if (outManufacturerString != NULL)
		*outManufacturerString = CFStringCreateWithCString(kCFAllocatorDefault, manufacturerNameCString, CFStringGetSystemEncoding());
	// if there was any problem creating any of the requested CFStrings, return an error
	// XXX but what if one was created and not the other?  we will be giving a misleading error code, and potentially leaking memory...
	if (outNameString != NULL)
	{
		if (*outNameString == NULL)
			return coreFoundationUnknownErr;
	}
	if (outManufacturerString != NULL)
	{
		if (*outManufacturerString == NULL)
			return coreFoundationUnknownErr;
	}

	return noErr;
}

//-----------------------------------------------------------------------------
// This function will get an AU's plugin name and manufacturer name strings for you 
// as separate strings.  In an AU's Component resource, these are stored as one Pascal string, 
// delimited by a colon, so this function just does the work of fetching the Pascal string, 
// parsing the plugin name and manufacturer name, and translating those to individual C strings.
// One of the string arguments can be NULL, if you are not interested in that string.
OSStatus GetAUNameAndManufacturerCStrings(Component inAUComponent, char * outNameString, char * outManufacturerString)
{
	OSStatus error = noErr;
	Handle componentNameHandle;
	ConstStr255Param componentFullNamePString;
	ComponentDescription dummydesc;

	// one input string or the other can be null, but not both
	if ( (inAUComponent == NULL) || ((outNameString == NULL) && (outManufacturerString == NULL)) )
		return paramErr;

	// first we need to create a handle and then try to fetch the Component name string resource into that handle
	componentNameHandle = NewHandle(sizeof(void*));
	if (componentNameHandle == NULL)
		return nilHandleErr;
	error = GetComponentInfo(inAUComponent, &dummydesc, componentNameHandle, NULL, NULL);
	if (error != noErr)
		return error;
	// dereferencing the name resource handle gives us a Pascal string pointer
	HLock(componentNameHandle);
	componentFullNamePString = (ConstStr255Param) (*componentNameHandle);
	if (componentFullNamePString == NULL)
		error = nilHandleErr;
	else
	{
		char * separatorByte;
		// convert the Component name Pascal string to a C string
		char componentFullNameCString[sizeof(Str255)];
		CopyPascalStringToC(componentFullNamePString, componentFullNameCString);
		// the manufacturer string is everything before the first : character, 
		// and everything after that and any immediately following white space 
		// is the plugin name string
		separatorByte = strchr(componentFullNameCString, ':');
		if (separatorByte == NULL)
			error = internalComponentErr;
		else
		{
			// point to right after the : character for the plugin name string...
			char * pluginNameCString = separatorByte + 1;
			// this will terminate the manufacturer name string right before the : character
			char * manufacturerNameCString = componentFullNameCString;
			separatorByte[0] = 0;
			// ...and then also skip over any white space immediately following the : delimiter
			while ( isspace(*pluginNameCString) )
				pluginNameCString++;

			// copy any of the requested strings for output
			if (outNameString != NULL)
				strcpy(outNameString, pluginNameCString);
			if (outManufacturerString != NULL)
				strcpy(outManufacturerString, manufacturerNameCString);
		}
	}
	DisposeHandle(componentNameHandle);

	return error;
}






#pragma mark _________Comparing_Components_________

//--------------------------------------------------------------------------
// general implementation for ComponentDescriptionsMatch() and ComponentDescriptionsMatch_Loosely()
// if inIgnoreType is true, then the type code is ignored in the ComponentDescriptions
Boolean ComponentDescriptionsMatch_General(const ComponentDescription * inComponentDescription1, const ComponentDescription * inComponentDescription2, Boolean inIgnoreType);
Boolean ComponentDescriptionsMatch_General(const ComponentDescription * inComponentDescription1, const ComponentDescription * inComponentDescription2, Boolean inIgnoreType)
{
	if ( (inComponentDescription1 == NULL) || (inComponentDescription2 == NULL) )
		return FALSE;

	if ( (inComponentDescription1->componentSubType == inComponentDescription2->componentSubType) 
			&& (inComponentDescription1->componentManufacturer == inComponentDescription2->componentManufacturer) )
	{
		// only sub-type and manufacturer IDs need to be equal
		if (inIgnoreType)
			return TRUE;
		// type, sub-type, and manufacturer IDs all need to be equal in order to call this a match
		else if (inComponentDescription1->componentType == inComponentDescription2->componentType)
			return TRUE;
	}

	return FALSE;
}

//--------------------------------------------------------------------------
// general implementation for ComponentAndDescriptionMatch() and ComponentAndDescriptionMatch_Loosely()
// if inIgnoreType is true, then the type code is ignored in the ComponentDescriptions
Boolean ComponentAndDescriptionMatch_General(Component inComponent, const ComponentDescription * inComponentDescription, Boolean inIgnoreType);
Boolean ComponentAndDescriptionMatch_General(Component inComponent, const ComponentDescription * inComponentDescription, Boolean inIgnoreType)
{
	OSErr status;
	ComponentDescription desc;

	if ( (inComponent == NULL) || (inComponentDescription == NULL) )
		return FALSE;

	// get the ComponentDescription of the input Component
	status = GetComponentInfo(inComponent, &desc, NULL, NULL, NULL);
	if (status != noErr)
		return FALSE;

	// check if the Component's ComponentDescription matches the input ComponentDescription
	return ComponentDescriptionsMatch_General(&desc, inComponentDescription, inIgnoreType);
}

//--------------------------------------------------------------------------
// determine if 2 ComponentDescriptions are basically equal
// (by that, I mean that the important identifying values are compared, 
// but not the ComponentDescription flags)
Boolean ComponentDescriptionsMatch(const ComponentDescription * inComponentDescription1, const ComponentDescription * inComponentDescription2)
{
	return ComponentDescriptionsMatch_General(inComponentDescription1, inComponentDescription2, FALSE);
}

//--------------------------------------------------------------------------
// determine if 2 ComponentDescriptions have matching sub-type and manufacturer codes
Boolean ComponentDescriptionsMatch_Loose(const ComponentDescription * inComponentDescription1, const ComponentDescription * inComponentDescription2)
{
	return ComponentDescriptionsMatch_General(inComponentDescription1, inComponentDescription2, TRUE);
}

//--------------------------------------------------------------------------
// determine if a ComponentDescription basically matches that of a particular Component
Boolean ComponentAndDescriptionMatch(Component inComponent, const ComponentDescription * inComponentDescription)
{
	return ComponentAndDescriptionMatch_General(inComponent, inComponentDescription, FALSE);
}

//--------------------------------------------------------------------------
// determine if a ComponentDescription matches only the sub-type and manufacturer codes of a particular Component
Boolean ComponentAndDescriptionMatch_Loosely(Component inComponent, const ComponentDescription * inComponentDescription)
{
	return ComponentAndDescriptionMatch_General(inComponent, inComponentDescription, TRUE);
}






#pragma mark _________System_Services_Availability_________

//--------------------------------------------------------------------------
// check the version of Mac OS installed
// the version value of interest to us is 0x1030 for Panther
long GetMacOSVersion()
{
	long systemVersion = 0;
	OSErr error = Gestalt(gestaltSystemVersion, &systemVersion);
	if (error == noErr)
	{
		systemVersion &= 0xFFFF;	// you are supposed to ignore the higher 16 bits for this Gestalt value
		return systemVersion;
	}
	else
		return 0;
}

//--------------------------------------------------------------------------
// check the version of QuickTime installed
// the version value of interest to us is 0x06408000 (6.4 release)
long GetQuickTimeVersion()
{
    long qtVersion = 0;
    OSErr error = Gestalt(gestaltQuickTime, &qtVersion);
    if (error == noErr)
		return qtVersion;
	else
		return 0;
}

//--------------------------------------------------------------------------
// check the version of the AudioToolbox.framework installed
// the version value of interest to us is 0x01300000 (1.3)
UInt32 GetAudioToolboxFrameworkVersion()
{
	CFBundleRef audioToolboxBundle = CFBundleGetBundleWithIdentifier(CFSTR("com.apple.audio.toolbox.AudioToolbox"));
	if (audioToolboxBundle != NULL)
	{
		UInt32 audioToolboxVersion = CFBundleGetVersionNumber(audioToolboxBundle);
		return audioToolboxVersion;
	}
	else
		return 0;
}

//--------------------------------------------------------------------------
// check for the availability of AU 2.0 rev 1 system frameworks
Boolean IsAvailable_AU2rev1()
{
	// the Audio Unit 2.0 rev 1 frameworks are available with Mac OS X 10.3 
	// or QuickTime 6.4 for Mac OS X 10.2, or more specifically, AudioToolbox.framework 1.3
	if (GetAudioToolboxFrameworkVersion() >= 0x01300000)
		return true;
	// in case that fails (possibly due to error, not wrong version value), try checking these
	if ( (GetMacOSVersion() >= 0x1030) || (GetQuickTimeVersion() >= 0x06408000) )
		return true;
	return false;
}

//--------------------------------------------------------------------------
Boolean IsTransportStateProcSafe()
{
	CFBundleRef applicationBundle = CFBundleGetMainBundle();
	if (applicationBundle != NULL)
	{
		CFStringRef applicationBundleID = CFBundleGetIdentifier(applicationBundle);
		UInt32 applicationVersionNumber = CFBundleGetVersionNumber(applicationBundle);
		CFStringRef applicationVersionString = (CFStringRef) CFBundleGetValueForInfoDictionaryKey(applicationBundle, kCFBundleVersionKey);
//if (applicationBundleID != NULL) CFShow(applicationBundleID);
//fprintf(stderr, "application version number = 0x%08lX\n\n", applicationVersionNumber);
//if (applicationVersionString != NULL) { fprintf(stderr, "application version string:  "); CFShow(applicationVersionString); }
		if (applicationBundleID != NULL)
		{
			const CFOptionFlags compareOptions = kCFCompareCaseInsensitive;
			if ( (CFStringCompare(applicationBundleID, CFSTR("info.emagic.Logic"), compareOptions) == kCFCompareEqualTo) 
				|| (CFStringCompare(applicationBundleID, CFSTR("de.emagic.Logic"), compareOptions) == kCFCompareEqualTo) )
			{
				if (applicationVersionNumber > 0)
				{
					if (applicationVersionNumber < 0x06428000)
						return false;
				}
				else if (applicationVersionString != NULL)
				{
					const CFStringRef logicFirstGoodVersionString = CFSTR("6.4.2");
					if ( CFStringCompareWithOptions(applicationVersionString, logicFirstGoodVersionString, 
							CFRangeMake(0, CFStringGetLength(logicFirstGoodVersionString)), 0) == kCFCompareLessThan )
						return false;
				}
			}
			else if ( CFStringCompare(applicationBundleID, CFSTR("com.apple.garageband"), compareOptions) == kCFCompareEqualTo )
			{
				const CFStringRef garageBandBadMajorVersionString = CFSTR("1.0");
				if (applicationVersionString != NULL)
				{
					if ( CFStringCompareWithOptions(applicationVersionString, garageBandBadMajorVersionString, 
							CFRangeMake(0, CFStringGetLength(garageBandBadMajorVersionString)), 0) == kCFCompareEqualTo )
						return false;
				}
			}
		}
	}

	return true;
}
