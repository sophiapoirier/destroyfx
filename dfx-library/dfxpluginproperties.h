/*------------------------------------------------------------------------
Destroy FX is a sovereign entity comprised of Marc Poirier & Tom Murphy 7.  
These are our extended Audio Unit property IDs and types.  
written by Marc Poirier, January 2003
------------------------------------------------------------------------*/

#ifndef __DFXPLUGIN_PROPERTIES_H
#define __DFXPLUGIN_PROPERTIES_H

#include "dfxparameter.h"


// property IDs for Audio Unit property stuff
enum DfxPluginProperties {
	kDfxPluginProperty_PluginPtr = 64000,	// get a pointer to the DfxPlugin
	kDfxPluginProperty_ParameterValue,		// get/set parameter values (current, min, max, etc.) using specific variable types
	kDfxPluginProperty_ParameterValueConversion,	// expand or contract a parameter value
	kDfxPluginProperty_ParameterValueString,	// get/set parameter value strings
	kDfxPluginProperty_RandomizeParameters,	// randomize the parameters
	kDfxPluginProperty_MidiLearn,			// get/set the MIDI learn state
	kDfxPluginProperty_ResetMidiLearn,		// clear MIDI parameter assignments
	kDfxPluginProperty_MidiLearner			// get/set the current MIDI learner parameter
};


// for kDfxPluginProperty_ParameterValue
enum DfxParameterValueItem {
	kDfxParameterValueItem_current, 
	kDfxParameterValueItem_previous, 
	kDfxParameterValueItem_default, 
	kDfxParameterValueItem_min, 
	kDfxParameterValueItem_max
};
struct DfxParameterValueRequest {
	long parameterID;
	DfxParameterValueItem valueItem;
	DfxParamValueType valueType;
	DfxParamValue value;
};


// for kDfxPluginProperty_ParameterValueConversion
enum DfxParameterValueConversionType {
	kDfxParameterValueConversion_expand, 
	kDfxParameterValueConversion_contract
};
struct DfxParameterValueConversionRequest {
	long parameterID;
	DfxParameterValueConversionType conversionType;
	double inValue;
	double outValue;
};


// for kDfxPluginProperty_ParameterValueString
struct DfxParameterValueStringRequest {
	long parameterID;
	long stringIndex;
	char valueString[DFX_PARAM_MAX_VALUE_STRING_LENGTH];
};


#endif
// __DFXPLUGIN_PROPERTIES_H
