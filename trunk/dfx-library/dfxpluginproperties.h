/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2003-2018  Sophia Poirier

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

#pragma once

#include "dfxparameter.h"

#ifdef TARGET_API_AUDIOUNIT
	#include <AudioToolbox/AudioUnitUtilities.h>	// for kAUParameterListener_AnyParameter
#endif



namespace dfx
{


//-----------------------------------------------------------------------------
// property IDs for Audio Unit property stuff
enum : uint32_t
{
	kPluginProperty_StartID = 64000,

	kPluginProperty_ParameterValue = kPluginProperty_StartID,	// get/set parameter values (current, min, max, etc.) using specific variable types
	kPluginProperty_ParameterValueConversion,	// expand or contract a parameter value
	kPluginProperty_ParameterValueString,		// get/set parameter value strings
	kPluginProperty_ParameterUnitLabel,			// get parameter unit label
	kPluginProperty_ParameterValueType,			// get parameter value type
	kPluginProperty_ParameterUnit,				// get parameter unit
	kPluginProperty_RandomizeParameter,			// randomize a parameter
	kPluginProperty_MidiLearn,					// get/set the MIDI learn state
	kPluginProperty_ResetMidiLearn,				// clear MIDI parameter assignments
	kPluginProperty_MidiLearner,					// get/set the current MIDI learner parameter
	kPluginProperty_ParameterMidiAssignment,		// get/set the MIDI assignment for a parameter

	kPluginProperty_EndOfList,
	kPluginProperty_NumProperties = kPluginProperty_EndOfList - kPluginProperty_StartID
};
typedef uint32_t PropertyID;


//-----------------------------------------------------------------------------
enum : uint32_t
{
	kPropertyFlag_Readable = 1,
	kPropertyFlag_Writable = 1 << 1,
	kPropertyFlag_BiDirectional = 1 << 2
};
typedef uint32_t PropertyFlags;

#ifdef TARGET_API_AUDIOUNIT
enum : AudioUnitScope
{
	kScope_Global = kAudioUnitScope_Global,
	kScope_Input = kAudioUnitScope_Input,
	kScope_Output = kAudioUnitScope_Output
};
typedef ::AudioUnitScope Scope;
#else
enum Scope : uint32_t
{
	kScope_Global = 0,
	kScope_Input = 1,
	kScope_Output = 2
};
#endif


//-----------------------------------------------------------------------------
// for kPluginProperty_ParameterValue
enum class ParameterValueItem : uint32_t 
{
	Current, 
	Previous, 
	Default, 
	Min, 
	Max
};

struct ParameterValueRequest
{
	ParameterValueItem inValueItem {};
	DfxParam::ValueType inValueType {};
	DfxParam::Value value {};
};


//-----------------------------------------------------------------------------
// for kPluginProperty_ParameterValueConversion
enum class ParameterValueConversionType : uint32_t
{
	Expand, 
	Contract
};

struct ParameterValueConversionRequest
{
	ParameterValueConversionType inConversionType {};
	double inValue = 0.0;
	double outValue = 0.0;
};


//-----------------------------------------------------------------------------
// for kPluginProperty_ParameterValueString
struct ParameterValueStringRequest
{
	int64_t inStringIndex = 0;
	char valueString[dfx::kParameterValueStringMaxLength];
};


#if TARGET_PLUGIN_USES_MIDI

//------------------------------------------------------
enum class MidiEventType : uint32_t
{
	None,
	CC,
	PitchBend,
	Note
};

enum MidiEventBehaviorFlags : int32_t
{
	// parameter automation behavior mode flags
	//
	kMidiEventBehaviorFlag_None = 0,
	// use MIDI events to toggle the associated parameter between 0.0 and 1.0 
	// if no mDataInt1 value was specified, 
	// (good for controlling on/off buttons)
	// but if mDataInt1 is greater than 2, then MIDI events will toggle between 
	// paramater's states.  note-ons will cycle through the values and 
	// continuous MIDI events will move through the values in steps
	// (good for switches)
	// (if using notes events and this flag is not set, and neither is NoteHold, 
	// then note ranges are used to control associated parameters)
	kMidiEventBehaviorFlag_Toggle = 1,
	// send 1.0 on note on and 0.0 on note off if this flag is on, 
	// otherwise toggle 1.0 and 0.0 at each note on
	// (only relevent when using notes events)
	// (overrides Toggle setting for notes events, but not other events)
	kMidiEventBehaviorFlag_NoteHold = 1 << 1,
	// use a range other than 0.0 to 1.0 
	// the range is defined by mDataFloat1 and mDataFloat2
	kMidiEventBehaviorFlag_Range = 1 << 2  // TODO: currently unused/unimplemented
};

struct ParameterAssignment
{
	MidiEventType mEventType = MidiEventType::None;
	// the MIDI channel of the MIDI event assignment
	//    (so far, I'm not using channel information for anything)
	int32_t mEventChannel = 0;
	// the number of the MIDI event assigned to the parameter 
	//    (CC number, note number, etc.)
	int32_t mEventNum = 0;
	// a second MIDI event number for double-value assignments 
	//    (like 2 notes defining a note range)
	int32_t mEventNum2 = 0;
	// indicating the behavior of the event, i.e. toggle vs. hold for notes, etc.
	MidiEventBehaviorFlags mEventBehaviorFlags = kMidiEventBehaviorFlag_None;
	// bonus data slots
	// (context-specific)
	// (like for the number of steps in an indexed toggle assignment)
	int32_t mDataInt1 = 0;
	// (or the maximum step, within that range, to cycle up to)
	int32_t mDataInt2 = 0;
	// (or the minimum point in a float range)
	float mDataFloat1 = 0.0f;
	// (or the maximum point in a float range)
	float mDataFloat2 = 0.0f;
};

#endif  // TARGET_PLUGIN_USES_MIDI


}  // namespace dfx
