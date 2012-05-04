/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2003-2011  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Destroy FX Library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Destroy FX Library.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/

Destroy FX is a sovereign entity comprised of Sophia Poirier and Tom Murphy 7.  
These are our extended Audio Unit property IDs and types.  
------------------------------------------------------------------------*/

#ifndef __DFXPLUGIN_PROPERTIES_H
#define __DFXPLUGIN_PROPERTIES_H

#include "dfxparameter.h"

#ifdef TARGET_API_AUDIOUNIT
	#include <AudioToolbox/AudioUnitUtilities.h>	// for kAUParameterListener_AnyParameter
#endif



//-----------------------------------------------------------------------------
// property IDs for Audio Unit property stuff
enum {
kDfxPluginProperty_StartID = 64000,
	kDfxPluginProperty_ParameterValue = kDfxPluginProperty_StartID,	// get/set parameter values (current, min, max, etc.) using specific variable types
	kDfxPluginProperty_ParameterValueConversion,	// expand or contract a parameter value
	kDfxPluginProperty_ParameterValueString,		// get/set parameter value strings
	kDfxPluginProperty_ParameterUnitLabel,			// get parameter unit label
	kDfxPluginProperty_ParameterValueType,			// get parameter value type
	kDfxPluginProperty_ParameterUnit,				// get parameter unit
	kDfxPluginProperty_RandomizeParameter,			// randomize a parameter
	kDfxPluginProperty_MidiLearn,					// get/set the MIDI learn state
	kDfxPluginProperty_ResetMidiLearn,				// clear MIDI parameter assignments
	kDfxPluginProperty_MidiLearner,					// get/set the current MIDI learner parameter
	kDfxPluginProperty_ParameterMidiAssignment,		// get/set the MIDI assignment for a parameter
kDfxPluginProperty_EndOfList,
	kDfxPluginProperty_NumProperties = kDfxPluginProperty_EndOfList - kDfxPluginProperty_StartID
};
typedef uint32_t	DfxPropertyID;


//-----------------------------------------------------------------------------
enum {
	kDfxPropertyFlag_Readable = 1,
	kDfxPropertyFlag_Writable = 1 << 1,
	kDfxPropertyFlag_BiDirectional = 1 << 2
};
typedef uint32_t	DfxPropertyFlags;

#ifdef TARGET_API_AUDIOUNIT
	enum {
		kDfxScope_Global = kAudioUnitScope_Global,
		kDfxScope_Input = kAudioUnitScope_Input,
		kDfxScope_Output = kAudioUnitScope_Output
	};
	typedef AudioUnitScope	DfxScope;
#else
	enum {
		kDfxScope_Global = 0,
		kDfxScope_Input = 1,
		kDfxScope_Output = 2
	};
typedef uint32_t	DfxScope;
#endif


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
