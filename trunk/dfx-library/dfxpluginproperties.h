/*------------------------------------------------------------------------
Destroy FX is a sovereign entity comprised of Sophia Poirier and Tom Murphy 7.  
These are our extended Audio Unit property IDs and types.  
written by Sophia Poirier, January 2003
------------------------------------------------------------------------*/

#ifndef __DFXPLUGIN_PROPERTIES_H
#define __DFXPLUGIN_PROPERTIES_H

#include "dfxparameter.h"

#include <AudioToolbox/AudioUnitUtilities.h>	// for kAUParameterListener_AnyParameter


// property IDs for Audio Unit property stuff
typedef enum {
	kDfxPluginProperty_PluginPtr = 64000,	// get a pointer to the DfxPlugin
	kDfxPluginProperty_ParameterValue,		// get/set parameter values (current, min, max, etc.) using specific variable types
	kDfxPluginProperty_ParameterValueConversion,	// expand or contract a parameter value
	kDfxPluginProperty_ParameterValueString,	// get/set parameter value strings
	kDfxPluginProperty_RandomizeParameter,	// randomize a parameter
	kDfxPluginProperty_MidiLearn,			// get/set the MIDI learn state
	kDfxPluginProperty_ResetMidiLearn,		// clear MIDI parameter assignments
	kDfxPluginProperty_MidiLearner			// get/set the current MIDI learner parameter
} DfxPluginProperties;


// for kDfxPluginProperty_ParameterValue
typedef enum {
	kDfxParameterValueItem_current, 
	kDfxParameterValueItem_previous, 
	kDfxParameterValueItem_default, 
	kDfxParameterValueItem_min, 
	kDfxParameterValueItem_max
} DfxParameterValueItem;

typedef struct {
	long parameterID;
	DfxParameterValueItem valueItem;
	DfxParamValueType valueType;
	DfxParamValue value;
} DfxParameterValueRequest;


// for kDfxPluginProperty_ParameterValueConversion
typedef enum {
	kDfxParameterValueConversion_expand, 
	kDfxParameterValueConversion_contract
} DfxParameterValueConversionType;

typedef struct {
	long parameterID;
	DfxParameterValueConversionType conversionType;
	double inValue;
	double outValue;
} DfxParameterValueConversionRequest;


// for kDfxPluginProperty_ParameterValueString
typedef struct {
	long parameterID;
	long stringIndex;
	char valueString[DFX_PARAM_MAX_VALUE_STRING_LENGTH];
} DfxParameterValueStringRequest;


#endif
// __DFXPLUGIN_PROPERTIES_H
