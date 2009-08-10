/*------------------------------------------------------------------------
Destroy FX Library (version 1.0) is a collection of foundation code 
for creating audio software plug-ins.  
Copyright (C) 2003-2009  Sophia Poirier

This program is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

This program is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with this program.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, please visit http://destroyfx.org/ 
and use the contact form.

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
kDfxPluginProperty_StartID = 64000,
	kDfxPluginProperty_ParameterValue = kDfxPluginProperty_StartID,	// get/set parameter values (current, min, max, etc.) using specific variable types
	kDfxPluginProperty_ParameterValueConversion,	// expand or contract a parameter value
	kDfxPluginProperty_ParameterValueString,		// get/set parameter value strings
	kDfxPluginProperty_RandomizeParameter,			// randomize a parameter
	kDfxPluginProperty_MidiLearn,					// get/set the MIDI learn state
	kDfxPluginProperty_ResetMidiLearn,				// clear MIDI parameter assignments
	kDfxPluginProperty_MidiLearner,					// get/set the current MIDI learner parameter
	kDfxPluginProperty_ParameterMidiAssignment,		// get/set the MIDI assignment for a parameter
kDfxPluginProperty_EndOfList,
	kDfxPluginProperty_NumProperties = kDfxPluginProperty_EndOfList - kDfxPluginProperty_StartID
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
