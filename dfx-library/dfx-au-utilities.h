/* This source code is by Marc Poirier, 2003.  It is offered as public domain. */


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
// and this will init a CFArray callbacks struct to use the above callback functions
void auPresetCFArrayCallbacks_Init(CFArrayCallBacks * inArrayCallbacks);

// these are convenience functions for sending parameter change notifications to all parameter listeners
void AUParameterChange_TellListeners_ScopeElement(AudioUnit inComponentInstance, AudioUnitParameterID inParameterID, 
									AudioUnitScope inScope, AudioUnitElement inElement);
// this one defaults to using global scope and element 0
void AUParameterChange_TellListeners(AudioUnit inComponentInstance, AudioUnitParameterID inParameterID);



#ifdef __cplusplus
};
#endif
// end of extern "C"



#endif
// __DFX_AU_UTILITIES_H
