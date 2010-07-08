/*
	Destroy FX AU Utilities is a collection of helpful utility functions 
	for creating and hosting Audio Unit plugins.
	Copyright (C) 2003-2010  Sophia Poirier
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


#pragma mark Component Version

//-----------------------------------------------------------------------------
// This function will get the version of a Component from its 'thng' resource 
// rather than from opening it and using the kComponentVersionSelect selector.  
// This allows you to get the version without opening the component, which can 
// be much more efficient.  Note that, unlike GetComponentVersion or 
// CallComponentVersion, the first argument of this function is a Component, 
// not a ComponentInstance.  You can, however, cast a ComponentInstance to 
// Component for this function, if you want to do that for any reason.
OSErr GetComponentVersionFromResource(Component inComponent, SInt32 * outVersion)
{
	OSErr error;
	ComponentDescription desc;
	ResFileRefNum curRes, componentResFileID;
	ResFileRefNum thngResourceCount, i;
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






#pragma mark -
#pragma mark Factory Presets CFArray

//-----------------------------------------------------------------------------
// The following defines and implements CoreFoundation-like handling of 
// an AUPreset container object:  CFAUPreset
//-----------------------------------------------------------------------------

const UInt32 kCFAUPreset_CurrentVersion = 0;

typedef struct {
	AUPreset auPreset;
	UInt32 version;
	CFAllocatorRef allocator;
	CFIndex retainCount;
} CFAUPreset;

//-----------------------------------------------------------------------------
// create an instance of a CFAUPreset object
CFAUPresetRef CFAUPresetCreate(CFAllocatorRef inAllocator, SInt32 inPresetNumber, CFStringRef inPresetName)
{
	CFAUPreset * newPreset = (CFAUPreset*) CFAllocatorAllocate(inAllocator, sizeof(CFAUPreset), 0);
	if (newPreset != NULL)
	{
		newPreset->auPreset.presetNumber = inPresetNumber;
		newPreset->auPreset.presetName = NULL;
		// create our own a copy rather than retain the string, in case the input string is mutable, 
		// this will keep it from changing under our feet
		if (inPresetName != NULL)
			newPreset->auPreset.presetName = CFStringCreateCopy(inAllocator, inPresetName);
		newPreset->version = kCFAUPreset_CurrentVersion;
		newPreset->allocator = inAllocator;
		newPreset->retainCount = 1;
	}
	return (CFAUPresetRef)newPreset;
}

//-----------------------------------------------------------------------------
// retain a reference of a CFAUPreset object
CFAUPresetRef CFAUPresetRetain(CFAUPresetRef inPreset)
{
	if (inPreset != NULL)
	{
		CFAUPreset * incomingPreset = (CFAUPreset*) inPreset;
		// retain the input AUPreset's name string for this reference to the preset
		if (incomingPreset->auPreset.presetName != NULL)
			CFRetain(incomingPreset->auPreset.presetName);
		incomingPreset->retainCount += 1;
	}
	return inPreset;
}

//-----------------------------------------------------------------------------
// release a reference of a CFAUPreset object
void CFAUPresetRelease(CFAUPresetRef inPreset)
{
	CFAUPreset * incomingPreset = (CFAUPreset*) inPreset;
	// these situations shouldn't happen
	if (inPreset == NULL)
		return;
	if (incomingPreset->retainCount <= 0)
		return;

	// first release the name string, CF-style, since it's a CFString
	if (incomingPreset->auPreset.presetName != NULL)
		CFRelease(incomingPreset->auPreset.presetName);
	incomingPreset->retainCount -= 1;
	// check if this is the end of this instance's life
	if (incomingPreset->retainCount == 0)
	{
		// wipe out the data so that, if anyone tries to access stale memory later, it will be (semi)invalid
		incomingPreset->auPreset.presetName = NULL;
		incomingPreset->auPreset.presetNumber = 0;
		// and finally, free the memory for the CFAUPreset struct
		CFAllocatorDeallocate(incomingPreset->allocator, (void*)inPreset);
	}
}

//-----------------------------------------------------------------------------
// The following 4 functions are CFArray callbacks for use when creating 
// an AU's factory presets array to support kAudioUnitProperty_FactoryPresets.
//-----------------------------------------------------------------------------

const void * CFAUPresetArrayRetainCallBack(CFAllocatorRef inAllocator, const void * inPreset);
void CFAUPresetArrayReleaseCallBack(CFAllocatorRef inAllocator, const void * inPreset);
Boolean CFAUPresetArrayEqualCallBack(const void * inPreset1, const void * inPreset2);
CFStringRef CFAUPresetArrayCopyDescriptionCallBack(const void * inPreset);
void CFAUPresetArrayCallBacks_Init(CFArrayCallBacks * outArrayCallBacks);

//-----------------------------------------------------------------------------
// This function is called when an item (an AUPreset) is added to the CFArray, 
// or when a CFArray containing an AUPreset is retained.  
const void * CFAUPresetArrayRetainCallBack(CFAllocatorRef inAllocator, const void * inPreset)
{
	return CFAUPresetRetain(inPreset);
}

//-----------------------------------------------------------------------------
// This function is called when an item (an AUPreset) is removed from the CFArray 
// or when the array is released.
// Since a reference to the data belongs to the array, we need to release that here.
void CFAUPresetArrayReleaseCallBack(CFAllocatorRef inAllocator, const void * inPreset)
{
	CFAUPresetRelease(inPreset);
}

//-----------------------------------------------------------------------------
// This function is called when someone wants to compare to items (AUPresets) 
// in the CFArray to see if they are equal or not.
// For our AUPresets, we will compare based on the preset number and the name string.
Boolean CFAUPresetArrayEqualCallBack(const void * inPreset1, const void * inPreset2)
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
CFStringRef CFAUPresetArrayCopyDescriptionCallBack(const void * inPreset)
{
	AUPreset * preset = (AUPreset*) inPreset;
	return CFStringCreateWithFormat(kCFAllocatorDefault, NULL, 
									CFSTR("AUPreset:\npreset number = %d\npreset name = %@"), 
									preset->presetNumber, preset->presetName);
}

//-----------------------------------------------------------------------------
// this will initialize a CFArray callbacks structure to use the above callback functions
void CFAUPresetArrayCallBacks_Init(CFArrayCallBacks * outArrayCallBacks)
{
	if (outArrayCallBacks == NULL)
		return;
	// wipe the struct clean
	memset(outArrayCallBacks, 0, sizeof(*outArrayCallBacks));
	// set all of the values and function pointers in the callbacks struct
	outArrayCallBacks->version = 0;	// currently, 0 is the only valid version value for this
	outArrayCallBacks->retain = CFAUPresetArrayRetainCallBack;
	outArrayCallBacks->release = CFAUPresetArrayReleaseCallBack;
	outArrayCallBacks->copyDescription = CFAUPresetArrayCopyDescriptionCallBack;
	outArrayCallBacks->equal = CFAUPresetArrayEqualCallBack;
}

//-----------------------------------------------------------------------------
#ifdef __GNUC__
const CFArrayCallBacks kCFAUPresetArrayCallBacks;
static void kCFAUPresetArrayCallBacks_constructor() __attribute__((constructor));
static void kCFAUPresetArrayCallBacks_constructor()
{
	CFAUPresetArrayCallBacks_Init( (CFArrayCallBacks*) &kCFAUPresetArrayCallBacks );
}
#else
// XXX I'll use this for other compilers, even though I hate initializing structs with all arguments at once 
// (cuz what if you ever decide to change the order of the struct members or something like that?)
const CFArrayCallBacks kCFAUPresetArrayCallBacks = {
	0, 
	CFAUPresetArrayRetainCallBack, 
	CFAUPresetArrayReleaseCallBack, 
	CFAUPresetArrayCopyDescriptionCallBack, 
	CFAUPresetArrayEqualCallBack
};
#endif






#pragma mark -
#pragma mark AudioUnitOtherPluginDesc CFArray

//-----------------------------------------------------------------------------
// The following defines and implements CoreFoundation-like handling of 
// an AudioUnitOtherPluginDesc container object:  CFAUOtherPluginDesc
//-----------------------------------------------------------------------------

const UInt32 kCFAUOtherPluginDesc_CurrentVersion = 0;

typedef struct {
	AudioUnitOtherPluginDesc auOtherPluginDesc;
	UInt32 version;
	CFAllocatorRef allocator;
	CFIndex retainCount;
} CFAUOtherPluginDesc;

#if MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_3
	#define kOtherPluginFormat_AU	3
#endif

//-----------------------------------------------------------------------------
// create an instance of a CFAUOtherPluginDesc object
CFAUOtherPluginDescRef CFAUOtherPluginDescCreate(CFAllocatorRef inAllocator, UInt32 inFormat, OSType inTypeID, OSType inSubTypeID, OSType inManufacturerID)
{
	CFAUOtherPluginDesc * newDesc = (CFAUOtherPluginDesc*) CFAllocatorAllocate(inAllocator, sizeof(CFAUOtherPluginDesc), 0);
	if (newDesc != NULL)
	{
		newDesc->auOtherPluginDesc.format = inFormat;
		newDesc->auOtherPluginDesc.plugin.mType = inTypeID;
		newDesc->auOtherPluginDesc.plugin.mSubType = inSubTypeID;
		newDesc->auOtherPluginDesc.plugin.mManufacturer = inManufacturerID;
		newDesc->version = kCFAUOtherPluginDesc_CurrentVersion;
		newDesc->allocator = inAllocator;
		newDesc->retainCount = 1;
	}
	return (CFAUOtherPluginDescRef)newDesc;
}

//-----------------------------------------------------------------------------
CFAUOtherPluginDescRef CFAUOtherPluginDescCreateVST(CFAllocatorRef inAllocator, OSType inUniqueID)
{
	return CFAUOtherPluginDescCreate(inAllocator, kOtherPluginFormat_kVST, 0, inUniqueID, 0);
}

//-----------------------------------------------------------------------------
CFAUOtherPluginDescRef CFAUOtherPluginDescCreateMAS(CFAllocatorRef inAllocator, OSType inEffectID, OSType inVariantID, OSType inManufacturerID)
{
	return CFAUOtherPluginDescCreate(inAllocator, kOtherPluginFormat_kMAS, inEffectID, inVariantID, inManufacturerID);
}

//-----------------------------------------------------------------------------
// retain a reference of a CFAUOtherPluginDesc object
CFAUOtherPluginDescRef CFAUOtherPluginDescRetain(CFAUOtherPluginDescRef inDesc)
{
	if (inDesc != NULL)
	{
		CFAUOtherPluginDesc * incomingDesc = (CFAUOtherPluginDesc*) inDesc;
		incomingDesc->retainCount += 1;
	}
	return inDesc;
}

//-----------------------------------------------------------------------------
// release a reference of a CFAUOtherPluginDesc object
void CFAUOtherPluginDescRelease(CFAUOtherPluginDescRef inDesc)
{
	CFAUOtherPluginDesc * incomingDesc = (CFAUOtherPluginDesc*) inDesc;
	// these situations shouldn't happen
	if (inDesc == NULL)
		return;
	if (incomingDesc->retainCount <= 0)
		return;

	incomingDesc->retainCount -= 1;
	// check if this is the end of this instance's life
	if (incomingDesc->retainCount == 0)
	{
		// wipe out the data so that, if anyone tries to access stale memory later, it will be (semi)invalid
		memset(&(incomingDesc->auOtherPluginDesc), 0, sizeof(incomingDesc->auOtherPluginDesc));
		// and finally, free the memory for the CFAUOtherPluginDesc struct
		CFAllocatorDeallocate(incomingDesc->allocator, (void*)inDesc);
	}
}

//-----------------------------------------------------------------------------
// The following 4 functions are CFArray callbacks for use when creating an AU's 
// other plugin descriptions array to support kAudioUnitMigrateProperty_FromPlugin.
//-----------------------------------------------------------------------------

const void * CFAUOtherPluginDescArrayRetainCallBack(CFAllocatorRef inAllocator, const void * inDesc);
void CFAUOtherPluginDescArrayReleaseCallBack(CFAllocatorRef inAllocator, const void * inDesc);
Boolean CFAUOtherPluginDescArrayEqualCallBack(const void * inDesc1, const void * inDesc2);
CFStringRef CFAUOtherPluginDescArrayCopyDescriptionCallBack(const void * inDesc);
void CFAUOtherPluginDescArrayCallBacks_Init(CFArrayCallBacks * outArrayCallBacks);

//-----------------------------------------------------------------------------
// This function is called when an item (an AudioUnitOtherPluginDesc) is added to the CFArray, 
// or when a CFArray containing an AudioUnitOtherPluginDesc is retained.  
const void * CFAUOtherPluginDescArrayRetainCallBack(CFAllocatorRef inAllocator, const void * inDesc)
{
	return CFAUOtherPluginDescRetain(inDesc);
}

//-----------------------------------------------------------------------------
// This function is called when an item (an AudioUnitOtherPluginDesc) is removed from 
// the CFArray or when the array is released.
// Since a reference to the data belongs to the array, we need to release that here.
void CFAUOtherPluginDescArrayReleaseCallBack(CFAllocatorRef inAllocator, const void * inDesc)
{
	CFAUOtherPluginDescRelease(inDesc);
}

//-----------------------------------------------------------------------------
// This function is called when someone wants to compare to items (AudioUnitOtherPluginDescs) 
// in the CFArray to see if they are equal or not.
Boolean CFAUOtherPluginDescArrayEqualCallBack(const void * inDesc1, const void * inDesc2)
{
	return (memcmp(inDesc1, inDesc2, sizeof(AudioUnitOtherPluginDesc)) == 0);
}

//-----------------------------------------------------------------------------
// This function is called when someone wants to get a description of 
// a particular item (an AudioUnitOtherPluginDesc) as though it were a CF type.  
// That happens, for example, when using CFShow().  
// This will create and return a CFString that indicates that the object is 
// an AudioUnitOtherPluginDesc and shows each piece of its data.
CFStringRef CFAUOtherPluginDescArrayCopyDescriptionCallBack(const void * inDesc)
{
	AudioUnitOtherPluginDesc * desc = (AudioUnitOtherPluginDesc*) inDesc;
	CFStringRef descriptionString = NULL;

	CFStringRef pluginFormatString = NULL;
	Boolean releasePluginFormatString = false;
	switch (desc->format)
	{
		case kOtherPluginFormat_Undefined:
			pluginFormatString = CFSTR("undefined");
			break;
		case kOtherPluginFormat_kMAS:
			pluginFormatString = CFSTR("MAS");
			break;
		case kOtherPluginFormat_kVST:
			pluginFormatString = CFSTR("VST");
			break;
		case kOtherPluginFormat_AU:
			pluginFormatString = CFSTR("AU");
			break;
		default:
			pluginFormatString = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("unknown (%lu)"), desc->format);
			if (pluginFormatString != NULL)
				releasePluginFormatString = true;
			break;
	}

#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_3
	if (UTCreateStringForOSType != NULL)
	{
		CFStringRef typeString = UTCreateStringForOSType(desc->plugin.mType);
		CFStringRef subTypeString = UTCreateStringForOSType(desc->plugin.mSubType);
		CFStringRef manufacturerString = UTCreateStringForOSType(desc->plugin.mManufacturer);
		descriptionString = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, 
									CFSTR("AudioUnitOtherPluginDesc:\nplugin format = %@\ntype ID = %@\nsub-type ID = %@\nmanufacturer ID = %@"), 
									pluginFormatString, typeString, subTypeString, manufacturerString);
		if (typeString != NULL)
			CFRelease(typeString);
		if (subTypeString != NULL)
			CFRelease(subTypeString);
		if (manufacturerString != NULL)
			CFRelease(manufacturerString);
	}
	else
#endif
	{
		descriptionString = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, 
									CFSTR("AudioUnitOtherPluginDesc:\nplugin format = %@\ntype ID = %lu\nsub-type ID = %lu\nmanufacturer ID = %lu"), 
									pluginFormatString, desc->plugin.mType, desc->plugin.mSubType, desc->plugin.mManufacturer);
	}

	if (releasePluginFormatString)
		CFRelease(pluginFormatString);

	return descriptionString;
}

//-----------------------------------------------------------------------------
// this will initialize a CFArray callbacks structure to use the above callback functions
void CFAUOtherPluginDescArrayCallBacks_Init(CFArrayCallBacks * outArrayCallBacks)
{
	if (outArrayCallBacks == NULL)
		return;
	// wipe the struct clean
	memset(outArrayCallBacks, 0, sizeof(*outArrayCallBacks));
	// set all of the values and function pointers in the callbacks struct
	outArrayCallBacks->version = 0;	// currently, 0 is the only valid version value for this
	outArrayCallBacks->retain = CFAUOtherPluginDescArrayRetainCallBack;
	outArrayCallBacks->release = CFAUOtherPluginDescArrayReleaseCallBack;
	outArrayCallBacks->copyDescription = CFAUOtherPluginDescArrayCopyDescriptionCallBack;
	outArrayCallBacks->equal = CFAUOtherPluginDescArrayEqualCallBack;
}

//-----------------------------------------------------------------------------
#ifdef __GNUC__
const CFArrayCallBacks kCFAUOtherPluginDescArrayCallBacks;
static void kCFAUOtherPluginDescArrayCallBacks_constructor() __attribute__((constructor));
static void kCFAUOtherPluginDescArrayCallBacks_constructor()
{
	CFAUOtherPluginDescArrayCallBacks_Init( (CFArrayCallBacks*) &kCFAUOtherPluginDescArrayCallBacks );
}
#else
// XXX I'll use this for other compilers, even though I hate initializing structs with all arguments at once 
// (cuz what if you ever decide to change the order of the struct members or something like that?)
const CFArrayCallBacks kCFAUOtherPluginDescArrayCallBacks = {
	0, 
	CFAUOtherPluginDescArrayRetainCallBack, 
	CFAUOtherPluginDescArrayReleaseCallBack, 
	CFAUOtherPluginDescArrayCopyDescriptionCallBack, 
	CFAUOtherPluginDescArrayEqualCallBack
};
#endif






#pragma mark -
#pragma mark Parameter Change Notifications

//--------------------------------------------------------------------------
// These are convenience functions for sending parameter change notifications 
// to all parameter listeners.  
// Use this when a parameter value in an AU changes and you need to make 
// other entities (like the host, a GUI, etc.) aware of the change.
void AUParameterChange_TellListeners_ScopeElement(AudioUnit inAUComponentInstance, AudioUnitParameterID inParameterID, 
									AudioUnitScope inScope, AudioUnitElement inElement)
{
	// set up an AudioUnitParameter structure with all of the necessary values
	AudioUnitParameter dirtyParam = {0};
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
		AudioUnitEvent paramEvent = {0};
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






#pragma mark -
#pragma mark AU Plugin Name And Manufacturer Name

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
		*outNameString = CFStringCreateWithCString(kCFAllocatorDefault, pluginNameCString, kCFStringEncodingMacRoman);
	if (outManufacturerString != NULL)
		*outManufacturerString = CFStringCreateWithCString(kCFAllocatorDefault, manufacturerNameCString, kCFStringEncodingMacRoman);
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
	componentFullNamePString = (ConstStr255Param) (*componentNameHandle);
	if (componentFullNamePString == NULL)
		error = nilHandleErr;
	else
	{
		char * separatorByte;
		// convert the Component name Pascal string to a C string
		char componentFullNameCString[sizeof(Str255)];
		strncpy(componentFullNameCString, (const char*)(componentFullNamePString+1), componentFullNamePString[0]);
		componentFullNameCString[componentFullNamePString[0]] = 0;
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






#pragma mark -
#pragma mark Comparing Components

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






#pragma mark -
#pragma mark System Services Availability

//--------------------------------------------------------------------------
// check the version of Mac OS installed
// the version value of interest to us is 0x1030 for Panther
SInt32 GetMacOSVersion()
{
	SInt32 systemVersion = 0;
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
SInt32 GetQuickTimeVersion()
{
    SInt32 qtVersion = 0;
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
