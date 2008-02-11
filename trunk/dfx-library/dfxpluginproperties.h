/*------------------------------------------------------------------------
Destroy FX is a sovereign entity comprised of Sophia Poirier and Tom Murphy 7.  
These are our extended Audio Unit property IDs and types.  
written by Sophia Poirier, January 2003
------------------------------------------------------------------------*/

#ifndef __DFXPLUGIN_PROPERTIES_H
#define __DFXPLUGIN_PROPERTIES_H

#include "dfxparameter.h"

#include <AudioToolbox/AudioUnitUtilities.h>	// for kAUParameterListener_AnyParameter



//-----------------------------------------------------------------------------
// property IDs for Audio Unit property stuff
enum {
kDfxPluginProperty_startID = 64000,
	kDfxPluginProperty_ParameterValue = kDfxPluginProperty_startID,	// get/set parameter values (current, min, max, etc.) using specific variable types
	kDfxPluginProperty_ParameterValueConversion,	// expand or contract a parameter value
	kDfxPluginProperty_ParameterValueString,		// get/set parameter value strings
	kDfxPluginProperty_RandomizeParameter,			// randomize a parameter
	kDfxPluginProperty_MidiLearn,					// get/set the MIDI learn state
	kDfxPluginProperty_ResetMidiLearn,				// clear MIDI parameter assignments
	kDfxPluginProperty_MidiLearner,					// get/set the current MIDI learner parameter
	kDfxPluginProperty_ParameterMidiAssignment,		// get/set the MIDI assignment for a parameter
kDfxPluginProperty_endOfList,
	kDfxPluginProperty_NumProperties = kDfxPluginProperty_endOfList - kDfxPluginProperty_startID
};


//-----------------------------------------------------------------------------
// for kDfxPluginProperty_ParameterValue
enum {
	kDfxParameterValueItem_current = 0, 
	kDfxParameterValueItem_previous, 
	kDfxParameterValueItem_default, 
	kDfxParameterValueItem_min, 
	kDfxParameterValueItem_max
};
typedef uint32_t	DfxParameterValueItem;

typedef struct {
	DfxParameterValueItem inValueItem;
	DfxParamValueType inValueType;
	DfxParamValue value;
} DfxParameterValueRequest;


//-----------------------------------------------------------------------------
// for kDfxPluginProperty_ParameterValueConversion
enum {
	kDfxParameterValueConversion_expand = 0, 
	kDfxParameterValueConversion_contract
};
typedef uint32_t	DfxParameterValueConversionType;

typedef struct {
	DfxParameterValueConversionType inConversionType;
	double inValue;
	double outValue;
} DfxParameterValueConversionRequest;


//-----------------------------------------------------------------------------
// for kDfxPluginProperty_ParameterValueString
typedef struct {
	int64_t inStringIndex;
	char valueString[DFX_PARAM_MAX_VALUE_STRING_LENGTH];
} DfxParameterValueStringRequest;



#endif
// __DFXPLUGIN_PROPERTIES_H
