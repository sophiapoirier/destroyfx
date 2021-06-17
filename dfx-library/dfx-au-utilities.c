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


#include "dfx-au-utilities.h"

#include <AudioToolbox/AudioUnitUtilities.h>  // for AUEventListenerNotify and AUParameterListenerNotify
#include <CoreServices/CoreServices.h>  // for UTCreateStringForOSType


#pragma mark -
#pragma mark Factory Presets CFArray

//-----------------------------------------------------------------------------
// The following defines and implements CoreFoundation-like handling of 
// an AUPreset container object:  CFAUPreset
//-----------------------------------------------------------------------------

UInt32 const kCFAUPreset_CurrentVersion = 0;

typedef struct
{
	AUPreset auPreset;
	UInt32 version;
	CFAllocatorRef allocator;
	CFIndex retainCount;
} CFAUPreset;

//-----------------------------------------------------------------------------
// create an instance of a CFAUPreset object
CFAUPresetRef CFAUPresetCreate(CFAllocatorRef inAllocator, SInt32 inPresetNumber, CFStringRef inPresetName)
{
	CFAUPreset* const newPreset = (CFAUPreset*)CFAllocatorAllocate(inAllocator, sizeof(CFAUPreset), 0);
	if (newPreset != NULL)
	{
		newPreset->auPreset.presetNumber = inPresetNumber;
		newPreset->auPreset.presetName = NULL;
		// create our own a copy rather than retain the string, in case the input string is mutable, 
		// this will keep it from changing under our feet
		if (inPresetName != NULL)
		{
			newPreset->auPreset.presetName = CFStringCreateCopy(inAllocator, inPresetName);
		}
		newPreset->version = kCFAUPreset_CurrentVersion;
		newPreset->allocator = inAllocator;
		newPreset->retainCount = 1;
	}
	// clang static analyzer does not know how to introspect this custom type and warns:
	// "Object with a +0 retain count returned to caller where a +1 (owning) retain count is expected"
#ifndef __clang_analyzer__
	return (CFAUPresetRef)newPreset;
#endif
}

//-----------------------------------------------------------------------------
// retain a reference of a CFAUPreset object
CFAUPresetRef CFAUPresetRetain(CFAUPresetRef inPreset)
{
	if (inPreset != NULL)
	{
		CFAUPreset* const incomingPreset = (CFAUPreset*)inPreset;
		// retain the input AUPreset's name string for this reference to the preset
		if (incomingPreset->auPreset.presetName != NULL)
		{
			CFRetain(incomingPreset->auPreset.presetName);
		}
		incomingPreset->retainCount += 1;
	}
	return inPreset;
}

//-----------------------------------------------------------------------------
// release a reference of a CFAUPreset object
void CFAUPresetRelease(CFAUPresetRef inPreset)
{
	CFAUPreset* const incomingPreset = (CFAUPreset*)inPreset;
	// these situations shouldn't happen
	if (inPreset == NULL)
	{
		return;
	}
	if (incomingPreset->retainCount <= 0)
	{
		return;
	}

	// first release the name string, CF-style, since it's a CFString
	if (incomingPreset->auPreset.presetName != NULL)
	{
		CFRelease(incomingPreset->auPreset.presetName);
	}
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

void const* CFAUPresetArrayRetainCallBack(CFAllocatorRef inAllocator, void const* inPreset);
void CFAUPresetArrayReleaseCallBack(CFAllocatorRef inAllocator, void const* inPreset);
Boolean CFAUPresetArrayEqualCallBack(void const* inPreset1, void const* inPreset2);
CFStringRef CFAUPresetArrayCopyDescriptionCallBack(void const* inPreset);
void CFAUPresetArrayCallBacks_Init(CFArrayCallBacks* outArrayCallBacks);

//-----------------------------------------------------------------------------
// This function is called when an item (an AUPreset) is added to the CFArray, 
// or when a CFArray containing an AUPreset is retained.  
void const* CFAUPresetArrayRetainCallBack(CFAllocatorRef inAllocator, void const* inPreset)
{
	(void)inAllocator;
	return CFAUPresetRetain(inPreset);
}

//-----------------------------------------------------------------------------
// This function is called when an item (an AUPreset) is removed from the CFArray 
// or when the array is released.
// Since a reference to the data belongs to the array, we need to release that here.
void CFAUPresetArrayReleaseCallBack(CFAllocatorRef inAllocator, void const* inPreset)
{
	(void)inAllocator;
	CFAUPresetRelease(inPreset);
}

//-----------------------------------------------------------------------------
// This function is called when someone wants to compare to items (AUPresets) 
// in the CFArray to see if they are equal or not.
// For our AUPresets, we will compare based on the preset number and the name string.
Boolean CFAUPresetArrayEqualCallBack(void const* inPreset1, void const* inPreset2)
{
	AUPreset const* const preset1 = (AUPreset const*)inPreset1;
	AUPreset const* const preset2 = (AUPreset const*)inPreset2;
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
CFStringRef CFAUPresetArrayCopyDescriptionCallBack(void const* inPreset)
{
	AUPreset const* const preset = (AUPreset const*)inPreset;
	return CFStringCreateWithFormat(kCFAllocatorDefault, NULL, 
									CFSTR("AUPreset:\npreset number = %d\npreset name = %@"), 
									(int)preset->presetNumber, preset->presetName);
}

//-----------------------------------------------------------------------------
// this will initialize a CFArray callbacks structure to use the above callback functions
void CFAUPresetArrayCallBacks_Init(CFArrayCallBacks* outArrayCallBacks)
{
	if (outArrayCallBacks == NULL)
	{
		return;
	}
	// wipe the struct clean
	memset(outArrayCallBacks, 0, sizeof(*outArrayCallBacks));
	// set all of the values and function pointers in the callbacks struct
	outArrayCallBacks->version = 0;  // currently, 0 is the only valid version value for this
	outArrayCallBacks->retain = CFAUPresetArrayRetainCallBack;
	outArrayCallBacks->release = CFAUPresetArrayReleaseCallBack;
	outArrayCallBacks->copyDescription = CFAUPresetArrayCopyDescriptionCallBack;
	outArrayCallBacks->equal = CFAUPresetArrayEqualCallBack;
}

//-----------------------------------------------------------------------------
#ifdef __GNUC__
CFArrayCallBacks const kCFAUPresetArrayCallBacks;
static void kCFAUPresetArrayCallBacks_constructor() __attribute__((constructor));
static void kCFAUPresetArrayCallBacks_constructor()
{
	CFAUPresetArrayCallBacks_Init((CFArrayCallBacks*)&kCFAUPresetArrayCallBacks);
}
#else
// XXX I'll use this for other compilers, even though I hate initializing structs with all arguments at once 
// (cuz what if you ever decide to change the order of the struct members or something like that?)
CFArrayCallBacks const kCFAUPresetArrayCallBacks = 
{
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

UInt32 const kCFAUOtherPluginDesc_CurrentVersion = 0;

typedef struct
{
	AudioUnitOtherPluginDesc auOtherPluginDesc;
	UInt32 version;
	CFAllocatorRef allocator;
	CFIndex retainCount;
} CFAUOtherPluginDesc;

//-----------------------------------------------------------------------------
// create an instance of a CFAUOtherPluginDesc object
CFAUOtherPluginDescRef CFAUOtherPluginDescCreate(CFAllocatorRef inAllocator, UInt32 inFormat, OSType inTypeID, OSType inSubTypeID, OSType inManufacturerID)
{
	CFAUOtherPluginDesc* const newDesc = (CFAUOtherPluginDesc*)CFAllocatorAllocate(inAllocator, sizeof(CFAUOtherPluginDesc), 0);
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
#ifndef __clang_analyzer__
	return (CFAUOtherPluginDescRef)newDesc;
#endif
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
		CFAUOtherPluginDesc* const incomingDesc = (CFAUOtherPluginDesc*)inDesc;
		incomingDesc->retainCount += 1;
	}
	return inDesc;
}

//-----------------------------------------------------------------------------
// release a reference of a CFAUOtherPluginDesc object
void CFAUOtherPluginDescRelease(CFAUOtherPluginDescRef inDesc)
{
	CFAUOtherPluginDesc* const incomingDesc = (CFAUOtherPluginDesc*)inDesc;
	// these situations shouldn't happen
	if (inDesc == NULL)
	{
		return;
	}
	if (incomingDesc->retainCount <= 0)
	{
		return;
	}

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

void const* CFAUOtherPluginDescArrayRetainCallBack(CFAllocatorRef inAllocator, void const* inDesc);
void CFAUOtherPluginDescArrayReleaseCallBack(CFAllocatorRef inAllocator, void const* inDesc);
Boolean CFAUOtherPluginDescArrayEqualCallBack(void const* inDesc1, void const* inDesc2);
CFStringRef CFAUOtherPluginDescArrayCopyDescriptionCallBack(void const* inDesc);
void CFAUOtherPluginDescArrayCallBacks_Init(CFArrayCallBacks* outArrayCallBacks);

//-----------------------------------------------------------------------------
// This function is called when an item (an AudioUnitOtherPluginDesc) is added to the CFArray, 
// or when a CFArray containing an AudioUnitOtherPluginDesc is retained.  
void const* CFAUOtherPluginDescArrayRetainCallBack(CFAllocatorRef inAllocator, void const* inDesc)
{
	(void)inAllocator;
	return CFAUOtherPluginDescRetain(inDesc);
}

//-----------------------------------------------------------------------------
// This function is called when an item (an AudioUnitOtherPluginDesc) is removed from 
// the CFArray or when the array is released.
// Since a reference to the data belongs to the array, we need to release that here.
void CFAUOtherPluginDescArrayReleaseCallBack(CFAllocatorRef inAllocator, void const* inDesc)
{
	(void)inAllocator;
	CFAUOtherPluginDescRelease(inDesc);
}

//-----------------------------------------------------------------------------
// This function is called when someone wants to compare to items (AudioUnitOtherPluginDescs) 
// in the CFArray to see if they are equal or not.
Boolean CFAUOtherPluginDescArrayEqualCallBack(void const* inDesc1, void const* inDesc2)
{
	return (memcmp(inDesc1, inDesc2, sizeof(AudioUnitOtherPluginDesc)) == 0);
}

//-----------------------------------------------------------------------------
// This function is called when someone wants to get a description of 
// a particular item (an AudioUnitOtherPluginDesc) as though it were a CF type.  
// That happens, for example, when using CFShow().  
// This will create and return a CFString that indicates that the object is 
// an AudioUnitOtherPluginDesc and shows each piece of its data.
CFStringRef CFAUOtherPluginDescArrayCopyDescriptionCallBack(void const* inDesc)
{
	AudioUnitOtherPluginDesc const* const desc = (AudioUnitOtherPluginDesc const*)inDesc;
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
			pluginFormatString = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("unknown (%u)"), (unsigned int)(desc->format));
			if (pluginFormatString != NULL)
			{
				releasePluginFormatString = true;
			}
			break;
	}

	CFStringRef const typeString = UTCreateStringForOSType(desc->plugin.mType);
	CFStringRef const subTypeString = UTCreateStringForOSType(desc->plugin.mSubType);
	CFStringRef const manufacturerString = UTCreateStringForOSType(desc->plugin.mManufacturer);
	descriptionString = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, 
												 CFSTR("AudioUnitOtherPluginDesc:\nplugin format = %@\ntype ID = %@\nsub-type ID = %@\nmanufacturer ID = %@"), 
												 pluginFormatString, typeString, subTypeString, manufacturerString);
	if (typeString != NULL)
	{
		CFRelease(typeString);
	}
	if (subTypeString != NULL)
	{
		CFRelease(subTypeString);
	}
	if (manufacturerString != NULL)
	{
		CFRelease(manufacturerString);
	}

	if (releasePluginFormatString)
	{
		CFRelease(pluginFormatString);
	}

	return descriptionString;
}

//-----------------------------------------------------------------------------
// this will initialize a CFArray callbacks structure to use the above callback functions
void CFAUOtherPluginDescArrayCallBacks_Init(CFArrayCallBacks* outArrayCallBacks)
{
	if (outArrayCallBacks == NULL)
	{
		return;
	}
	// wipe the struct clean
	memset(outArrayCallBacks, 0, sizeof(*outArrayCallBacks));
	// set all of the values and function pointers in the callbacks struct
	outArrayCallBacks->version = 0;  // currently, 0 is the only valid version value for this
	outArrayCallBacks->retain = CFAUOtherPluginDescArrayRetainCallBack;
	outArrayCallBacks->release = CFAUOtherPluginDescArrayReleaseCallBack;
	outArrayCallBacks->copyDescription = CFAUOtherPluginDescArrayCopyDescriptionCallBack;
	outArrayCallBacks->equal = CFAUOtherPluginDescArrayEqualCallBack;
}

//-----------------------------------------------------------------------------
#ifdef __GNUC__
CFArrayCallBacks const kCFAUOtherPluginDescArrayCallBacks;
static void kCFAUOtherPluginDescArrayCallBacks_constructor() __attribute__((constructor));
static void kCFAUOtherPluginDescArrayCallBacks_constructor()
{
	CFAUOtherPluginDescArrayCallBacks_Init((CFArrayCallBacks*)&kCFAUOtherPluginDescArrayCallBacks);
}
#else
// XXX I'll use this for other compilers, even though I hate initializing structs with all arguments at once 
// (cuz what if you ever decide to change the order of the struct members or something like that?)
CFArrayCallBacks const kCFAUOtherPluginDescArrayCallBacks = 
{
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
void AUParameterChange_TellListeners_ScopeElement(AudioComponentInstance inAUComponentInstance, AudioUnitParameterID inParameterID, 
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
	{
		AUParameterListenerNotify(NULL, NULL, &dirtyParam);
	}
}

//--------------------------------------------------------------------------
// this one defaults to using global scope and element 0 
// (which are what most effects use for all of their parameters)
void AUParameterChange_TellListeners(AudioComponentInstance inAUComponentInstance, AudioUnitParameterID inParameterID)
{
	AUParameterChange_TellListeners_ScopeElement(inAUComponentInstance, inParameterID, kAudioUnitScope_Global, (AudioUnitElement)0);
}






#pragma mark -
#pragma mark AU Plugin Name And Manufacturer Name

//-----------------------------------------------------------------------------
// This function will get an AU's plugin name and manufacturer name strings for you 
// as separate strings.  In an AU's AudioComponent resource, these are stored as one string, 
// delimited by a colon, so this function just does the work of fetching the string, 
// parsing the plugin name and manufacturer name, and translating those to individual strings.
// One of the string arguments can be NULL, if you are not interested in that string.
OSStatus CopyAUNameAndManufacturerStrings(AudioComponent inAUComponent, CFStringRef* outNameString, CFStringRef* outManufacturerString)
{
	// these are defined in the Carbon header MacErrors.h
	OSStatus const coreFoundationUnknownErr = -4960;
	OSStatus const internalComponentErr = -2070;

	OSStatus status = noErr;
	CFStringRef componentFullNameString = NULL;
	CFArrayRef namesArray = NULL;

	// one input string or the other can be null, but not both
	if ((inAUComponent == NULL) || ((outNameString == NULL) && (outManufacturerString == NULL)))
	{
		return kAudio_ParamError;
	}

	// first we need to fetch the AudioComponent name string
	status = AudioComponentCopyName(inAUComponent, &componentFullNameString);
	if (status != noErr)
	{
		return status;
	}
	if (componentFullNameString == NULL)
	{
		return coreFoundationUnknownErr;
	}

	// the manufacturer string is everything before the first : character, 
	// and everything after that and any immediately following white space 
	// is the plugin name string
	namesArray = CFStringCreateArrayBySeparatingStrings(kCFAllocatorDefault, componentFullNameString, CFSTR(":"));
	CFRelease(componentFullNameString);
	if (namesArray == NULL)
	{
		return coreFoundationUnknownErr;
	}
	if (CFArrayGetCount(namesArray) < 2)
	{
		CFRelease(namesArray);
		return internalComponentErr;
	}

	// copy any of the requested strings for output
	if (outManufacturerString != NULL)
	{
		*outManufacturerString = (CFStringRef)CFArrayGetValueAtIndex(namesArray, 0);
		if (*outManufacturerString == NULL)
		{
			CFRelease(namesArray);
			return coreFoundationUnknownErr;
		}
		CFRetain(*outManufacturerString);
	}
	if (outNameString != NULL)
	{
		*outNameString = NULL;
		CFStringRef nameString = (CFStringRef)CFArrayGetValueAtIndex(namesArray, 1);
		if (nameString != NULL)
		{
			CFMutableStringRef mutableNameString = CFStringCreateMutableCopy(kCFAllocatorDefault, CFStringGetLength(nameString), nameString);
			if (mutableNameString != NULL)
			{
				CFStringTrimWhitespace(mutableNameString);
				*outNameString = mutableNameString;
			}
		}
		if (*outNameString == NULL)
		{
			CFRelease(namesArray);
			if (outManufacturerString != NULL)
			{
				CFRelease(*outManufacturerString);
			}
			return coreFoundationUnknownErr;
		}
	}

	CFRelease(namesArray);

	return status;
}






#pragma mark -
#pragma mark Comparing Components

//--------------------------------------------------------------------------
// general implementation for ComponentDescriptionsMatch() and ComponentDescriptionsMatch_Loosely()
// if inIgnoreType is true, then the type code is ignored in the ComponentDescriptions
Boolean ComponentDescriptionsMatch_General(AudioComponentDescription const* inComponentDescription1, AudioComponentDescription const* inComponentDescription2, Boolean inIgnoreType);
Boolean ComponentDescriptionsMatch_General(AudioComponentDescription const* inComponentDescription1, AudioComponentDescription const* inComponentDescription2, Boolean inIgnoreType)
{
	if ((inComponentDescription1 == NULL) || (inComponentDescription2 == NULL))
	{
		return false;
	}

	if ((inComponentDescription1->componentSubType == inComponentDescription2->componentSubType) 
		&& (inComponentDescription1->componentManufacturer == inComponentDescription2->componentManufacturer))
	{
		// only sub-type and manufacturer IDs need to be equal
		if (inIgnoreType)
		{
			return true;
		}
		// type, sub-type, and manufacturer IDs all need to be equal in order to call this a match
		else if (inComponentDescription1->componentType == inComponentDescription2->componentType)
		{
			return true;
		}
	}

	return false;
}

//--------------------------------------------------------------------------
// general implementation for ComponentAndDescriptionMatch() and ComponentAndDescriptionMatch_Loosely()
// if inIgnoreType is true, then the type code is ignored in the ComponentDescriptions
Boolean ComponentAndDescriptionMatch_General(AudioComponent inComponent, AudioComponentDescription const* inComponentDescription, Boolean inIgnoreType);
Boolean ComponentAndDescriptionMatch_General(AudioComponent inComponent, AudioComponentDescription const* inComponentDescription, Boolean inIgnoreType)
{
	OSErr status = noErr;
	AudioComponentDescription desc = {0};

	if ((inComponent == NULL) || (inComponentDescription == NULL))
	{
		return false;
	}

	// get the AudioComponentDescription of the input AudioComponent
	status = AudioComponentGetDescription(inComponent, &desc);
	if (status != noErr)
	{
		return false;
	}

	// check if the AudioComponent's AudioComponentDescription matches the input AudioComponentDescription
	return ComponentDescriptionsMatch_General(&desc, inComponentDescription, inIgnoreType);
}

//--------------------------------------------------------------------------
// determine if 2 ComponentDescriptions are basically equal
// (by that, I mean that the important identifying values are compared, 
// but not the AudioComponentDescription flags)
Boolean ComponentDescriptionsMatch(AudioComponentDescription const* inComponentDescription1, AudioComponentDescription const* inComponentDescription2)
{
	return ComponentDescriptionsMatch_General(inComponentDescription1, inComponentDescription2, false);
}

//--------------------------------------------------------------------------
// determine if 2 ComponentDescriptions have matching sub-type and manufacturer codes
Boolean ComponentDescriptionsMatch_Loose(AudioComponentDescription const* inComponentDescription1, AudioComponentDescription const* inComponentDescription2)
{
	return ComponentDescriptionsMatch_General(inComponentDescription1, inComponentDescription2, true);
}

//--------------------------------------------------------------------------
// determine if a AudioComponentDescription basically matches that of a particular AudioComponent
Boolean ComponentAndDescriptionMatch(AudioComponent inComponent, AudioComponentDescription const* inComponentDescription)
{
	return ComponentAndDescriptionMatch_General(inComponent, inComponentDescription, false);
}

//--------------------------------------------------------------------------
// determine if a AudioComponentDescription matches only the sub-type and manufacturer codes of a particular AudioComponent
Boolean ComponentAndDescriptionMatch_Loosely(AudioComponent inComponent, AudioComponentDescription const* inComponentDescription)
{
	return ComponentAndDescriptionMatch_General(inComponent, inComponentDescription, true);
}
