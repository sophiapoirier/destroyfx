/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2009  Sophia Poirier

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
This is our class for E-Z plugin-making and E-Z multiple-API support.
This is where we connect the Audio Unit API to our DfxPlugin system.
------------------------------------------------------------------------*/

#include "dfxplugin.h"
#include "dfx-au-utilities.h"
#include <AudioUnit/AudioUnitCarbonView.h>



#pragma mark -
#pragma mark init
#pragma mark -

//-----------------------------------------------------------------------------
// this is called immediately after an instance of the plugin class is created
void DfxPlugin::PostConstructor()
{
	TARGET_API_BASE_CLASS::PostConstructor();
	auElementsHaveBeenCreated = true;

	dfx_PostConstructor();

	// make host see that current preset is 0
	update_preset(0);

	// make the global-scope element aware of the parameters' values
	// this must happen after AUBase::PostConstructor because the elements are created there
	for (long i=0; i < numParameters; i++)
	{
		if ( !(getparameterattributes(i) & kDfxParamAttribute_unused) )	// XXX should we do it like this, or override GetParameterList?
			AUBase::SetParameter(i, kAudioUnitScope_Global, (AudioUnitElement)0, getparameter_f(i), 0);
	}

	// make sure that the input and output elements are initially set to a supported number of channels, 
	// if this plugin specifies a list of supported i/o channel-count pairs
	if (channelconfigs != NULL)
	{
		CAStreamBasicDescription curInStreamFormat;
		curInStreamFormat.mChannelsPerFrame = 0;
		if ( Inputs().GetNumberOfElements() > 0 )
			curInStreamFormat = GetStreamFormat(kAudioUnitScope_Input, (AudioUnitElement)0);
		CAStreamBasicDescription curOutStreamFormat = GetStreamFormat(kAudioUnitScope_Output, (AudioUnitElement)0);
		bool currentFormatIsNotSupported = false;
		for (long i=0; i < numchannelconfigs; i++)
		{
			// compare the input channel count
			if ( ((UInt32)(channelconfigs[i].inChannels) != curInStreamFormat.NumberChannels()) && (channelconfigs[i].inChannels >= 0) )
				currentFormatIsNotSupported = true;
			// compare the output channel count
			else if ( ((UInt32)(channelconfigs[i].outChannels) != curOutStreamFormat.NumberChannels()) && (channelconfigs[i].outChannels >= 0) )
				currentFormatIsNotSupported = true;
			// if neither check failed, then we are matching this channel config and therefore are okay
			else
			{
				currentFormatIsNotSupported = false;
				break;
			}
		}
		// if the current format is not supported, then set the format to the first supported i/o pair in our list
		if (currentFormatIsNotSupported)
		{
			const UInt32 defaultNumChannels = 2;
			// change the input channel count to the first supported one listed
			UInt32 newNumChannels = (channelconfigs[0].inChannels < 0) ? defaultNumChannels : (UInt32)(channelconfigs[0].inChannels);
			CAStreamBasicDescription newStreamFormat(curInStreamFormat);
			newStreamFormat.ChangeNumberChannels(newNumChannels, newStreamFormat.IsInterleaved());
			if ( Inputs().GetNumberOfElements() > 0 )
				AUBase::ChangeStreamFormat(kAudioUnitScope_Input, (AudioUnitElement)0, curInStreamFormat, newStreamFormat);
			// change the output channel count to the first supported one listed
			newNumChannels = (channelconfigs[0].outChannels < 0) ? defaultNumChannels : (UInt32)(channelconfigs[0].outChannels);
			newStreamFormat = CAStreamBasicDescription(curOutStreamFormat);
			newStreamFormat.ChangeNumberChannels(newNumChannels, newStreamFormat.IsInterleaved());
			AUBase::ChangeStreamFormat(kAudioUnitScope_Output, (AudioUnitElement)0, curOutStreamFormat, newStreamFormat);
		}
		UpdateInPlaceProcessingState();
	}

// XXX some stuff that might worth adding an accessor for at some point or something...
#if 0
	const unsigned char * appname = LMGetCurApName();
	if (appname != NULL)
		printf( "app name = %.*s\n", appname[0], (char*)&(appname[1]) );

	ProcessSerialNumber currentProcess = { 0, kCurrentProcess };
	CFStringRef processName = NULL;
	OSStatus status = CopyProcessName(&currentProcess, &processName);
	if ( (status == noErr) && (processName != NULL) )
	{
		char * cname = DFX_CreateCStringFromCFString(processName, kCFStringEncodingUTF8);
		if (cname != NULL)
		{
			printf("process name = %s\n", cname);
			free(cname);
		}
		CFRelease(processName);
	}
#endif
}

//-----------------------------------------------------------------------------
// this is called immediately before an instance of the plugin class is deleted
void DfxPlugin::PreDestructor()
{
	dfx_PreDestructor();

	TARGET_API_BASE_CLASS::PreDestructor();
}

//-----------------------------------------------------------------------------
// this is like a second constructor, kind of
// it is called when the Audio Unit is expected to be ready to process audio
// this is where DSP-specific resources should be allocated
ComponentResult DfxPlugin::Initialize()
{
	ComponentResult result = noErr;

#if TARGET_PLUGIN_IS_INSTRUMENT
	const AUChannelInfo * auChannelConfigs = NULL;
	UInt32 numIOconfigs = SupportedNumChannels(&auChannelConfigs);
	AUChannelInfo auChannelConfigs_temp;
	// if this AU supports only specific i/o channel count configs, then check whether the current format is allowed
	if (auChannelConfigs == NULL)
		numIOconfigs = 0;
	if ( (numIOconfigs == 0) && (numchannelconfigs > 0) )
	{
		auChannelConfigs_temp.inChannels = channelconfigs[0].inChannels;
		auChannelConfigs_temp.outChannels = channelconfigs[0].outChannels;
		auChannelConfigs = &auChannelConfigs_temp;
		numIOconfigs = 1;
	}
	if (numIOconfigs > 0)
	{
		SInt16 auNumInputs = 0;
		if ( Inputs().GetNumberOfElements() > 0 )
			auNumInputs = (SInt16) GetStreamFormat(kAudioUnitScope_Input, (AudioUnitElement)0).NumberChannels();
		SInt16 auNumOutputs = (SInt16) GetStreamFormat(kAudioUnitScope_Output, (AudioUnitElement)0).NumberChannels();
		bool foundMatch = false;
		for (UInt32 i=0; (i < numIOconfigs) && !foundMatch; i++)
		{
			SInt16 configNumInputs = auChannelConfigs[i].inChannels;
			SInt16 configNumOutputs = auChannelConfigs[i].outChannels;
			// handle the special "wildcard" cases of negative AUChannelInfo values
			if ( (configNumInputs < 0) && (configNumOutputs < 0) )
			{
				// a -1,-2 AUChannelInfo pair signifies that any number of inputs and outputs is allowed
				if ( ((configNumInputs == -1) && (configNumOutputs == -2)) || ((configNumInputs == -2) && (configNumOutputs == -1)) )
					foundMatch = true;
				// failing that, a -1,-1 pair signifies that any number of ins and outs are allowed, as long as they are equal
				else if ( ((configNumInputs == -1) && (configNumOutputs == -1)) && (auNumInputs == auNumOutputs) )
					foundMatch = true;
				// any other pair of negative values are illegal, so skip this AUChannelInfo pair
				else
					continue;
			}
			// handle literal AUChannelInfo values (and maybe a wildcard on one of the scopes)
			else
			{
				bool inputMatch = (auNumInputs == configNumInputs) || (configNumInputs == -1);
				bool outputMatch = (auNumOutputs == configNumOutputs) || (configNumOutputs == -1);
				// if input and output are both allowed in this i/o pair description, then we found a match
				if (inputMatch && outputMatch)
					foundMatch = true;
			}
		}
		// if the current i/o counts don't match any of the allowed configs, return an error
		if ( !foundMatch )
			return kAudioUnitErr_FormatNotSupported;
	}
#else
	// call the inherited class' Initialize routine
	result = TARGET_API_BASE_CLASS::Initialize();
#endif
// TARGET_PLUGIN_IS_INSTRUMENT

	// call our initialize routine
	if (result == noErr)
	{
		result = do_initialize();
		UpdateInPlaceProcessingState();
	}

	return result;
}

//-----------------------------------------------------------------------------
// this is the "destructor" partner to Initialize
// any DSP-specific resources should be released here
void DfxPlugin::Cleanup()
{
	do_cleanup();

	TARGET_API_BASE_CLASS::Cleanup();	// XXX doesn't actually do anything now, but maybe at some point?
}

//-----------------------------------------------------------------------------
// this is called when an audio stream is broken somehow 
// (playback stop/restart, change of playback position, etc.)
// any DSP state variables should be reset here 
// (contents of buffers, position trackers, IIR filter histories, etc.)
ComponentResult DfxPlugin::Reset(AudioUnitScope inScope, AudioUnitElement inElement)
{
	do_reset();
	return noErr;
}



#pragma mark -
#pragma mark info
#pragma mark -

//-----------------------------------------------------------------------------
// get basic information about Audio Unit Properties 
// (whether they're supported, writability, and data size)
// most properties are handled by inherited base class implementations
ComponentResult DfxPlugin::GetPropertyInfo(AudioUnitPropertyID inPropertyID, 
					AudioUnitScope inScope, AudioUnitElement inElement, 
					UInt32 & outDataSize, Boolean & outWritable)
{
	ComponentResult result = noErr;

	switch (inPropertyID)
	{
		case kAudioUnitProperty_ParameterClumpName:
			outDataSize = sizeof(AudioUnitParameterNameInfo);
			outWritable = false;
			result = noErr;
			break;

		case kAudioUnitProperty_AUHostIdentifier:
			outDataSize = sizeof(AUHostIdentifier);	// XXX update to AUHostVersionIdentifier
			outWritable = true;
			result = noErr;
			break;

		case kAudioUnitMigrateProperty_FromPlugin:
			outDataSize = sizeof(CFArrayRef);
			outWritable = false;
			result = noErr;
			break;

		case kAudioUnitMigrateProperty_OldAutomation:
			outDataSize = sizeof(AudioUnitParameterValueTranslation);
			outWritable = false;
			result = noErr;
			break;

		// get/set parameter values (current, min, max, etc.) using specific variable types
		case kDfxPluginProperty_ParameterValue:
			outDataSize = sizeof(DfxParameterValueRequest);
			outWritable = true;
			break;

		// expand or contract a parameter value
		case kDfxPluginProperty_ParameterValueConversion:
			outDataSize = sizeof(DfxParameterValueConversionRequest);
			outWritable = false;
			break;

		// get/set parameter value strings
		case kDfxPluginProperty_ParameterValueString:
			outDataSize = sizeof(DfxParameterValueStringRequest);
			outWritable = false;
			break;

		// randomize the parameters
		case kDfxPluginProperty_RandomizeParameter:
			// when you "set" this "property", you send a boolean to say whether or not to write automation data
			outDataSize = sizeof(Boolean);
			outWritable = true;
			break;

	#if TARGET_PLUGIN_USES_MIDI
		// get/set the MIDI learn state
		case kDfxPluginProperty_MidiLearn:
			outDataSize = sizeof(Boolean);
			outWritable = true;
			break;
		// clear MIDI parameter assignments
		case kDfxPluginProperty_ResetMidiLearn:
			outDataSize = sizeof(Boolean);	// you don't need an input value for this property
			outWritable = true;
			break;
		// get/set the current MIDI learner parameter
		case kDfxPluginProperty_MidiLearner:
			outDataSize = sizeof(int32_t);
			outWritable = true;
			break;
		// get/set the MIDI assignment for a parameter
		case kDfxPluginProperty_ParameterMidiAssignment:
			outDataSize = sizeof(DfxParameterAssignment);
			outWritable = true;
			break;
	#endif

		case kLogicAUProperty_NodeOperationMode:
			outDataSize = sizeof(UInt32);
			outWritable = true;
			break;

		case kLogicAUProperty_NodePropertyDescriptions:
			outDataSize = sizeof(LogicAUNodePropertyDescription) * dfx_GetNumPluginProperties();
			outWritable = false;
			break;

		default:
			if (inPropertyID >= kDfxPluginProperty_StartID)
			{
				size_t dfxDataSize = 0;
				DfxPropertyFlags dfxFlags = 0;
				result = dfx_GetPropertyInfo(inPropertyID, inScope, inElement, dfxDataSize, dfxFlags);
				if (result == noErr)
				{
					outDataSize = dfxDataSize;
					outWritable = dfxFlags & kDfxPropertyFlag_Writable;
				}
			}
			else
			{
				result = TARGET_API_BASE_CLASS::GetPropertyInfo(inPropertyID, inScope, inElement, outDataSize, outWritable);
			}
			break;
	}

	return result;
}

//-----------------------------------------------------------------------------
// get specific information about Audio Unit Properties
// most properties are handled by inherited base class implementations
ComponentResult DfxPlugin::GetProperty(AudioUnitPropertyID inPropertyID, 
					AudioUnitScope inScope, AudioUnitElement inElement, 
					void * outData)
{
	ComponentResult result = noErr;

	switch (inPropertyID)
	{
		case kAudioUnitProperty_ParameterClumpName:
			{
				if (inScope != kAudioUnitScope_Global)
					result = kAudioUnitErr_InvalidScope;
				// XXX the Cocoa Generic AUView (prior to Mac OS X 10.5) sends bogus element values for this property, so ignore it
//				else if (inElement != 0)
//					result = kAudioUnitErr_InvalidElement;
				else
				{
					AudioUnitParameterNameInfo * ioClumpInfo = (AudioUnitParameterNameInfo*) outData;
					if (ioClumpInfo->inID == kAudioUnitClumpID_System)	// this ID value is reserved
						result = kAudioUnitErr_InvalidPropertyValue;
					else
					{
						ioClumpInfo->outName = CopyParameterGroupName(ioClumpInfo->inID);
						if (ioClumpInfo->outName == NULL)
							result = kAudioUnitErr_InvalidPropertyValue;
					}
				}
			}
			break;

		case kAudioUnitMigrateProperty_FromPlugin:
			{
				// VST counterpart description
				static AudioUnitOtherPluginDesc vstPluginMigrationDesc = {0};
				vstPluginMigrationDesc.format = kOtherPluginFormat_kVST;
				vstPluginMigrationDesc.plugin.mSubType = PLUGIN_ID;
				// create a CFArray of the VST counterpart description
				CFMutableArrayRef descsArray = CFArrayCreateMutable(NULL, 1, NULL);
				if (descsArray != NULL)
				{
					CFArrayAppendValue(descsArray, &vstPluginMigrationDesc);
					*((CFArrayRef*)outData) = descsArray;
				}
				else
					result = coreFoundationUnknownErr;
			}
			break;

		case kAudioUnitMigrateProperty_OldAutomation:
			{
				AudioUnitParameterValueTranslation * pvt = (AudioUnitParameterValueTranslation*)outData;
				if ( (pvt->otherDesc.format == kOtherPluginFormat_kVST) && (pvt->otherDesc.plugin.mSubType == PLUGIN_ID) )
				{
					pvt->auParamID = pvt->otherParamID;
					pvt->auValue = expandparametervalue((long)(pvt->otherParamID), pvt->otherValue);
				}
				else
					result = kAudioUnitErr_InvalidPropertyValue;
			}
			break;

		// get parameter values (current, min, max, etc.) using specific variable types
		// XXX finish implementing all items
		case kDfxPluginProperty_ParameterValue:
			{
				DfxParameterValueRequest * request = (DfxParameterValueRequest*) outData;
				if ( isLogicNodeEndianReversed() )
				{
					request->inValueItem = CFSwapInt32(request->inValueItem);
					request->inValueType = CFSwapInt32(request->inValueType);
				}
				DfxParamValue * value = &(request->value);
				long paramID = inElement;
				switch (request->inValueItem)
				{
					case kDfxParameterValueItem_current:
						{
							switch (request->inValueType)
							{
								case kDfxParamValueType_float:
									value->f = getparameter_f(paramID);
									break;
								case kDfxParamValueType_int:
									value->i = getparameter_i(paramID);
									break;
								case kDfxParamValueType_boolean:
									value->b = getparameter_b(paramID);
									break;
								default:
									result = paramErr;
									break;
							}
						}
						break;
					case kDfxParameterValueItem_previous:
						// XXX implement this
						break;
					case kDfxParameterValueItem_default:
						{
							switch (request->inValueType)
							{
								case kDfxParamValueType_float:
									value->f = getparameterdefault_f(paramID);
									break;
								case kDfxParamValueType_int:
//									value->i = getparameterdefault_i(paramID);
									break;
								case kDfxParamValueType_boolean:
//									value->b = getparameterdefault_b(paramID);
									break;
								default:
									result = paramErr;
									break;
							}
						}
						break;
					case kDfxParameterValueItem_min:
						{
							switch (request->inValueType)
							{
								case kDfxParamValueType_float:
									value->f = getparametermin_f(paramID);
									break;
								case kDfxParamValueType_int:
									value->i = getparametermin_i(paramID);
									break;
								case kDfxParamValueType_boolean:
//									value->b = getparametermin_b(paramID);
									value->b = false;
									break;
								default:
									result = paramErr;
									break;
							}
						}
						break;
					case kDfxParameterValueItem_max:
						{
							switch (request->inValueType)
							{
								case kDfxParamValueType_float:
									value->f = getparametermax_f(paramID);
									break;
								case kDfxParamValueType_int:
									value->i = getparametermax_i(paramID);
									break;
								case kDfxParamValueType_boolean:
//									value->b = getparametermax_b(paramID);
									value->b = true;
									break;
								default:
									result = paramErr;
									break;
							}
						}
						break;
					default:
						result = paramErr;
						break;
				}
				if ( isLogicNodeEndianReversed() )
				{
					request->value.i = CFSwapInt64(request->value.i);
				}
			}
			break;

		// expand or contract a parameter value
		case kDfxPluginProperty_ParameterValueConversion:
			{
				DfxParameterValueConversionRequest * request = (DfxParameterValueConversionRequest*) outData;
				if ( isLogicNodeEndianReversed() )
				{
					request->inConversionType = CFSwapInt32(request->inConversionType);
					DFX_ReverseBytes( &(request->inValue), sizeof(request->inValue) );
				}
				switch (request->inConversionType)
				{
					case kDfxParameterValueConversion_expand:
						request->outValue = expandparametervalue(inElement, request->inValue);
						break;
					case kDfxParameterValueConversion_contract:
						request->outValue = contractparametervalue(inElement, request->inValue);
						break;
					default:
						result = paramErr;
						break;
				}
				if ( isLogicNodeEndianReversed() )
				{
					DFX_ReverseBytes( &(request->outValue), sizeof(request->outValue) );
				}
			}
			break;

		// get parameter value strings
		case kDfxPluginProperty_ParameterValueString:
			{
				DfxParameterValueStringRequest * request = (DfxParameterValueStringRequest*) outData;
				if ( isLogicNodeEndianReversed() )
				{
					request->inStringIndex = CFSwapInt64(request->inStringIndex);
				}
				if ( !getparametervaluestring(inElement, request->inStringIndex, request->valueString) )
					result = paramErr;
			}
			break;

	#if TARGET_PLUGIN_USES_MIDI
		// get the MIDI learn state
		case kDfxPluginProperty_MidiLearn:
			*(Boolean*)outData = dfxsettings->isLearning();
			break;
		// get the current MIDI learner parameter
		case kDfxPluginProperty_MidiLearner:
			*(int32_t*)outData = dfxsettings->getLearner();
			break;
		// get the MIDI assignment for a parameter
		case kDfxPluginProperty_ParameterMidiAssignment:
			*(DfxParameterAssignment*)outData = dfxsettings->getParameterAssignment(inElement);
			break;
	#endif

		case kLogicAUProperty_NodeOperationMode:
			*(UInt32*)outData = getSupportedLogicNodeOperationMode();
			break;

		case kLogicAUProperty_NodePropertyDescriptions:
			{
				LogicAUNodePropertyDescription * nodePropertyDescs = (LogicAUNodePropertyDescription*) outData;
				for (long i=0; i < dfx_GetNumPluginProperties(); i++)
				{
					nodePropertyDescs[i].mPropertyID = kDfxPluginProperty_StartID + i;
					nodePropertyDescs[i].mEndianMode = kLogicAUNodePropertyEndianMode_DontTouch;
					nodePropertyDescs[i].mFlags = kLogicAUNodePropertyFlag_Synchronous | kLogicAUNodePropertyFlag_NeedsInitialization;
					switch ( nodePropertyDescs[i].mPropertyID )
					{
						case kDfxPluginProperty_ParameterValue:
							nodePropertyDescs[i].mFlags |= kLogicAUNodePropertyFlag_FullRoundTrip;
							break;
						case kDfxPluginProperty_ParameterValueConversion:
							nodePropertyDescs[i].mFlags |= kLogicAUNodePropertyFlag_FullRoundTrip;
							break;
						case kDfxPluginProperty_ParameterValueString:
							nodePropertyDescs[i].mFlags |= kLogicAUNodePropertyFlag_FullRoundTrip;
							break;
						case kDfxPluginProperty_MidiLearner:
							nodePropertyDescs[i].mEndianMode = kLogicAUNodePropertyEndianMode_All32Bits;
							break;
						case kDfxPluginProperty_ParameterMidiAssignment:
							nodePropertyDescs[i].mEndianMode = kLogicAUNodePropertyEndianMode_All32Bits;
							break;
						default:
							{
								size_t dfxDataSize = 0;
								DfxPropertyFlags dfxFlags = 0;
								long dfxResult = dfx_GetPropertyInfo(nodePropertyDescs[i].mPropertyID, kDfxScope_Global, 0, dfxDataSize, dfxFlags);
								if (dfxResult == kDfxErr_NoError)
								{
									if (dfxFlags & kDfxPropertyFlag_BiDirectional)
										nodePropertyDescs[i].mFlags |= kLogicAUNodePropertyFlag_FullRoundTrip;
								}
							}
							break;
					}
				}
			}
			break;

		default:
			if (inPropertyID >= kDfxPluginProperty_StartID)
			{
				size_t dfxDataSize = 0;
				DfxPropertyFlags dfxFlags = 0;
				result = dfx_GetPropertyInfo(inPropertyID, inScope, inElement, dfxDataSize, dfxFlags);
				if ( (result == noErr) && !(dfxFlags & kDfxPropertyFlag_Readable) )
					result = kAudioUnitErr_InvalidPropertyValue;
				else
					result = dfx_GetProperty(inPropertyID, inScope, inElement, outData);
			}
			else
				result = TARGET_API_BASE_CLASS::GetProperty(inPropertyID, inScope, inElement, outData);
			break;
	}

	return result;
}

//-----------------------------------------------------------------------------
ComponentResult DfxPlugin::SetProperty(AudioUnitPropertyID inPropertyID, 
					AudioUnitScope inScope, AudioUnitElement inElement, 
					const void * inData, UInt32 inDataSize)
{
	ComponentResult result = noErr;

	switch (inPropertyID)
	{
		case kAudioUnitProperty_ParameterClumpName:
			result = kAudioUnitErr_PropertyNotWritable;
			break;

		case kAudioUnitProperty_AUHostIdentifier:
			{
				const AUHostIdentifier * hostID = (AUHostIdentifier*) inData;	// XXX update to AUHostVersionIdentifier
				if (hostID->hostName == NULL)
					result = kAudioUnitErr_InvalidPropertyValue;
				else
				{
#if 0
					fprintf(stderr, "\tSetProperty(AUHostIdentifier)\n");
					CFShow(hostID->hostName);
					fprintf(stderr, "version: major = %u, minor&bug = 0x%02X, stage = 0x%02X, non-released = %u\n", hostID->hostVersion.majorRev, hostID->hostVersion.minorAndBugRev, hostID->hostVersion.stage, hostID->hostVersion.nonRelRev);
#endif
				}
			}
			break;

		case kAudioUnitMigrateProperty_FromPlugin:
			result = kAudioUnitErr_PropertyNotWritable;
			break;

		case kAudioUnitMigrateProperty_OldAutomation:
			result = kAudioUnitErr_PropertyNotWritable;
			break;

	#if !TARGET_PLUGIN_IS_INSTRUMENT
		case kAudioUnitProperty_InPlaceProcessing:
			if ( ((audioProcessingAccumulatingOnly) || (getnuminputs() != getnumoutputs())) 
					&& (*(UInt32*)inData != 0) )
				result = kAudioUnitErr_InvalidPropertyValue;
			else
				result = TARGET_API_BASE_CLASS::SetProperty(inPropertyID, inScope, inElement, inData, inDataSize);
			break;
	#endif

		// set parameter values (current, min, max, etc.) using specific variable types
		// XXX finish implementing all items
		case kDfxPluginProperty_ParameterValue:
			{
				DfxParameterValueRequest * request = (DfxParameterValueRequest*) inData;
				if ( isLogicNodeEndianReversed() )
				{
					request->inValueItem = CFSwapInt32(request->inValueItem);
					request->inValueType = CFSwapInt32(request->inValueType);
					request->value.i = CFSwapInt64(request->value.i);
				}
				DfxParamValue * value = &(request->value);
				long paramID = inElement;
				switch (request->inValueItem)
				{
					case kDfxParameterValueItem_current:
						{
							switch (request->inValueType)
							{
								case kDfxParamValueType_float:
									setparameter_f(paramID, value->f);
									postupdate_parameter(paramID);
									break;
								case kDfxParamValueType_int:
									setparameter_i(paramID, value->i);
									postupdate_parameter(paramID);
									break;
								case kDfxParamValueType_boolean:
									setparameter_b(paramID, value->b);
									postupdate_parameter(paramID);
									break;
								default:
									result = paramErr;
									break;
							}
						}
						break;
					case kDfxParameterValueItem_previous:
						// XXX implement this
						break;
					case kDfxParameterValueItem_default:
						{
							switch (request->inValueType)
							{
								case kDfxParamValueType_float:
//									setparameterdefault_f(paramID, value->f);
									break;
								case kDfxParamValueType_int:
//									setparameterdefault_i(paramID, value->i);
									break;
								case kDfxParamValueType_boolean:
//									setparameterdefault_b(paramID, value->b);
									break;
								default:
									result = paramErr;
									break;
							}
						}
						break;
					case kDfxParameterValueItem_min:
						{
							switch (request->inValueType)
							{
								case kDfxParamValueType_float:
//									setparametermin_f(paramID, value->f);
									break;
								case kDfxParamValueType_int:
//									setparametermin_i(paramID, value->i);
									break;
								case kDfxParamValueType_boolean:
//									setparametermin_b(paramID, value->b);
									break;
								default:
									result = paramErr;
									break;
							}
						}
						break;
					case kDfxParameterValueItem_max:
						{
							switch (request->inValueType)
							{
								case kDfxParamValueType_float:
//									setparametermax_f(paramID, value->f);
									break;
								case kDfxParamValueType_int:
//									setparametermax_i(paramID, value->i);
									break;
								case kDfxParamValueType_boolean:
//									setparametermax_b(paramID, value->b);
									break;
								default:
									result = paramErr;
									break;
							}
						}
						break;
					default:
						result = paramErr;
						break;
				}
			}
			break;

		case kDfxPluginProperty_ParameterValueConversion:
			result = kAudioUnitErr_PropertyNotWritable;
			break;

		// set parameter value strings
		case kDfxPluginProperty_ParameterValueString:
			{
				DfxParameterValueStringRequest * request = (DfxParameterValueStringRequest*) inData;
				if ( isLogicNodeEndianReversed() )
				{
					request->inStringIndex = CFSwapInt64(request->inStringIndex);
				}
				if ( !setparametervaluestring(inElement, request->inStringIndex, request->valueString) )
					result = paramErr;
			}
			break;

		// randomize the parameters
		case kDfxPluginProperty_RandomizeParameter:
			// when you "set" this "property", you send a bool to say whether or not to write automation data
			if (inElement == kAUParameterListener_AnyParameter)
				randomizeparameters( *(Boolean*)inData );
			else
				randomizeparameter(inElement);
			break;

	#if TARGET_PLUGIN_USES_MIDI
		// set the MIDI learn state
		case kDfxPluginProperty_MidiLearn:
			dfxsettings->setParameterMidiLearn( *(Boolean*)inData );
			break;
		// clear MIDI parameter assignments
		case kDfxPluginProperty_ResetMidiLearn:
			// you don't need an input value for this property
			dfxsettings->setParameterMidiReset();
			break;
		// set the current MIDI learner parameter
		case kDfxPluginProperty_MidiLearner:
			dfxsettings->setLearner( *(int32_t*)inData );
			break;
		// set the MIDI assignment for a parameter
		case kDfxPluginProperty_ParameterMidiAssignment:
			{
				DfxParameterAssignment * paramAssignment = (DfxParameterAssignment*)inData;
				if (paramAssignment->eventType == kParamEventNone)
					dfxsettings->unassignParam(inElement);
				else
				{
					dfxsettings->assignParam(inElement, paramAssignment->eventType, paramAssignment->eventChannel, 
											paramAssignment->eventNum, paramAssignment->eventNum2, 
											paramAssignment->eventBehaviourFlags, 
											paramAssignment->data1, paramAssignment->data2, 
											paramAssignment->fdata1, paramAssignment->fdata2);
				}
			}
			break;
	#endif

		case kLogicAUProperty_NodeOperationMode:
			setCurrentLogicNodeOperationMode( *(UInt32*)inData );
			break;

		case kLogicAUProperty_NodePropertyDescriptions:
			result = kAudioUnitErr_PropertyNotWritable;
			break;

		default:
			if (inPropertyID >= kDfxPluginProperty_StartID)
			{
				size_t dfxDataSize = 0;
				DfxPropertyFlags dfxFlags = 0;
				result = dfx_GetPropertyInfo(inPropertyID, inScope, inElement, dfxDataSize, dfxFlags);
				if ( (result == noErr) && !(dfxFlags & kDfxPropertyFlag_Writable) )
					result = kAudioUnitErr_PropertyNotWritable;
				else
					result = dfx_SetProperty(inPropertyID, inScope, inElement, inData, inDataSize);
			}
			else
			{
				result = TARGET_API_BASE_CLASS::SetProperty(inPropertyID, inScope, inElement, inData, inDataSize);
			}
			break;
	}

	return result;
}

//-----------------------------------------------------------------------------
// should be a version 32-bit number hex-encoded like so:  
// 0xMMMMmmbb (M = major version, m = minor version, and b = bugfix)
ComponentResult	DfxPlugin::Version()
{
	return getpluginversion();
}

//-----------------------------------------------------------------------------
// give the host an array of the audio input/output channel configurations 
// that the plugin supports
// if the pointer passed in is NULL, then simply return the number of supported configurations
// if any n-to-n configuration (i.e. same number of ins and outs) is supported, return 0
UInt32 DfxPlugin::SupportedNumChannels(const AUChannelInfo ** outInfo)
{
	if (channelconfigs != NULL)
	{
		if (outInfo != NULL)
		{
			*outInfo = channelconfigs;
		}
		return (UInt32)numchannelconfigs;
	}
	return 0;
}

//-----------------------------------------------------------------------------
Float64 DfxPlugin::GetLatency()
{
	return getlatency_seconds();
}

//-----------------------------------------------------------------------------
Float64 DfxPlugin::GetTailTime()
{
	return gettailsize_seconds();
}

//-----------------------------------------------------------------------------
CFURLRef DfxPlugin::CopyIconLocation()
{
#ifdef PLUGIN_ICON_FILE_NAME
	CFBundleRef pluginBundle = CFBundleGetBundleWithIdentifier( CFSTR(PLUGIN_BUNDLE_IDENTIFIER) );
	if (pluginBundle != NULL)
	{
		return CFBundleCopyResourceURL(pluginBundle, CFSTR(PLUGIN_ICON_FILE_NAME), NULL, NULL);
	}
#endif

	return NULL;
}

#if TARGET_PLUGIN_HAS_GUI
//-----------------------------------------------------------------------------
int DfxPlugin::GetNumCustomUIComponents()
{
	return 1;
}

//-----------------------------------------------------------------------------
void DfxPlugin::GetUIComponentDescs(ComponentDescription * inDescArray)
{
	if (inDescArray == NULL)
		return;

	inDescArray->componentType = kAudioUnitCarbonViewComponentType;
	#ifdef PLUGIN_EDITOR_ID
		inDescArray->componentSubType = PLUGIN_EDITOR_ID;
	#else
		inDescArray->componentSubType = PLUGIN_ID;
	#endif
	inDescArray->componentManufacturer = PLUGIN_CREATOR_ID;
	inDescArray->componentFlags = 0;
	inDescArray->componentFlagsMask = 0;
}
#endif



#pragma mark -
#pragma mark parameters
#pragma mark -

//-----------------------------------------------------------------------------
// get specific information about the properties of a parameter
ComponentResult DfxPlugin::GetParameterInfo(AudioUnitScope inScope, 
					AudioUnitParameterID inParameterID, 
					AudioUnitParameterInfo & outParameterInfo)
{
	// we're only handling the global scope
	if (inScope != kAudioUnitScope_Global)
		return kAudioUnitErr_InvalidScope;

	if ( !parameterisvalid(inParameterID) )
		return kAudioUnitErr_InvalidParameter;

	// get the name into a temp string buffer that we know is large enough
	char tempname[DFX_PARAM_MAX_NAME_LENGTH];
	getparametername(inParameterID, tempname);
	// then make sure to only copy as much as the ParameterInfo name C string can hold
	strncpy(outParameterInfo.name, tempname, sizeof(outParameterInfo.name));
	// in case the parameter name was DFX_PARAM_MAX_NAME_LENGTH or longer, 
	// make sure that the ParameterInfo name string is terminated
	outParameterInfo.name[sizeof(outParameterInfo.name)-1] = 0;
	//
	outParameterInfo.cfNameString = getparametercfname(inParameterID);
	outParameterInfo.minValue = getparametermin_f(inParameterID);
	outParameterInfo.maxValue = getparametermax_f(inParameterID);
	// XXX this might need a better solution (extend DfxParam to store initial value?)
	// In AU, the "default value" means the value of the parameter when the plugin is first created.  
	// This is not what I like to think of as the "default value" (I consider it to be a value that 
	// is the closest thing to a "reset" or neutral or middle state that the parameter can have).  
	// And hence, this is not what our getparameterdefault_x will return.  
	// In most cases, our plugins store their initial state as the first built-in preset, 
	// so here we are going to try to see if there is a first preset and, if so, rely on the 
	// parameter value in there as the AU default value, but failing that, we'll use our DfxParam 
	// default value.
//	if ( presetisvalid(0) )
//		outParameterInfo.defaultValue = getpresetparameter_f(0, inParameterID);
//	else
// XXX bzzzt!  no, now Bill Stewart says that our definition of default value is correct?
		outParameterInfo.defaultValue = getparameterdefault_f(inParameterID);
	// check if the parameter is used or not (stupid VST workaround)
	if ( getparameterattributes(inParameterID) & kDfxParamAttribute_unused )
	{
		outParameterInfo.flags = 0;
//		return kAudioUnitErr_InvalidParameter;	// XXX ey?
	}
	// if the parameter is hidden, then indicate that it's not readable or writable...
	else if ( getparameterattributes(inParameterID) & kDfxParamAttribute_hidden )
		outParameterInfo.flags = 0;
	// ...otherwise all parameters are readable and writable
	else
		outParameterInfo.flags = kAudioUnitParameterFlag_IsReadable 
								| kAudioUnitParameterFlag_IsWritable;
	if (outParameterInfo.cfNameString != NULL)
	{
		outParameterInfo.flags |= kAudioUnitParameterFlag_HasCFNameString;
		CFRetain(outParameterInfo.cfNameString);
		outParameterInfo.flags |= kAudioUnitParameterFlag_CFNameRelease;
	}
	switch ( getparametercurve(inParameterID) )
	{
		// so far, this is all that AU offers as far as parameter distribution curves go
		case kDfxParamCurve_log:
			outParameterInfo.flags |= kAudioUnitParameterFlag_DisplayLogarithmic;
			break;
		case kDfxParamCurve_sqrt:
			outParameterInfo.flags |= kAudioUnitParameterFlag_DisplaySquared;
			break;
		case kDfxParamCurve_squared:
			outParameterInfo.flags |= kAudioUnitParameterFlag_DisplaySquareRoot;
			break;
		case kDfxParamCurve_cubed:
			outParameterInfo.flags |= kAudioUnitParameterFlag_DisplayCubeRoot;
			break;
		case kDfxParamCurve_exp:
			outParameterInfo.flags |= kAudioUnitParameterFlag_DisplayExponential;
			break;
// XXX currently no way to handle this with AU, except I guess by using HasName thingy, but I'd much rather not do that
// XXX or maybe estimate?  like:  if (curveSpec < 1.0) DisplaySquareRoot; else if (curveSpec < 2.5) DisplaySquared; else DisplayCubed
//		case kDfxParamCurve_pow:
//			break;
		default:
			break;
	}

	if ( getparametervaluetype(inParameterID) == kDfxParamValueType_float )
		outParameterInfo.flags |= kAudioUnitParameterFlag_IsHighResolution;

	// the complicated part:  getting the unit type 
	// (but easy if we use value strings for value display)
	outParameterInfo.unitName = NULL;	// default to nothing, will be set if it is needed
	if ( getparameterusevaluestrings(inParameterID) )
		outParameterInfo.unit = kAudioUnitParameterUnit_Indexed;
	else
	{
		switch ( getparameterunit(inParameterID) )
		{
			case kDfxParamUnit_quantity:
				outParameterInfo.unit = kAudioUnitParameterUnit_Indexed;	// XXX assume it's an integer?
				break;
			case kDfxParamUnit_percent:
				outParameterInfo.unit = kAudioUnitParameterUnit_Percent;
				break;
			case kDfxParamUnit_portion:
				outParameterInfo.unit = kAudioUnitParameterUnit_Generic;
				break;
			case kDfxParamUnit_lineargain:
				outParameterInfo.unit = kAudioUnitParameterUnit_LinearGain;
				break;
			case kDfxParamUnit_decibles:
				outParameterInfo.unit = kAudioUnitParameterUnit_Decibels;
				break;
			case kDfxParamUnit_drywetmix:
				outParameterInfo.unit = kAudioUnitParameterUnit_EqualPowerCrossfade;
				break;
			case kDfxParamUnit_hz:
				outParameterInfo.unit = kAudioUnitParameterUnit_Hertz;
				break;
			case kDfxParamUnit_seconds:
				outParameterInfo.unit = kAudioUnitParameterUnit_Seconds;
				break;
			case kDfxParamUnit_ms:
				outParameterInfo.unit = kAudioUnitParameterUnit_Milliseconds;
				break;
			case kDfxParamUnit_samples:
				outParameterInfo.unit = kAudioUnitParameterUnit_SampleFrames;
				break;
			case kDfxParamUnit_scalar:
				outParameterInfo.unit = kAudioUnitParameterUnit_Rate;
				break;
			case kDfxParamUnit_divisor:
				outParameterInfo.unit = kAudioUnitParameterUnit_Generic;
				break;
			case kDfxParamUnit_exponent:
				outParameterInfo.unit = kAudioUnitParameterUnit_Generic;
				break;
			case kDfxParamUnit_semitones:
				outParameterInfo.unit = kAudioUnitParameterUnit_RelativeSemiTones;
				break;
			case kDfxParamUnit_octaves:
				outParameterInfo.unit = kAudioUnitParameterUnit_Octaves;
				break;
			case kDfxParamUnit_cents:
				outParameterInfo.unit = kAudioUnitParameterUnit_Cents;
				break;
			case kDfxParamUnit_notes:
				outParameterInfo.unit = kAudioUnitParameterUnit_MIDINoteNumber;
				break;
			case kDfxParamUnit_pan:
				outParameterInfo.unit = kAudioUnitParameterUnit_Pan;
				break;
			case kDfxParamUnit_bpm:
				outParameterInfo.unit = kAudioUnitParameterUnit_BPM;
				break;
			case kDfxParamUnit_beats:
				outParameterInfo.unit = kAudioUnitParameterUnit_Beats;
				break;
			case kDfxParamUnit_index:
				outParameterInfo.unit = kAudioUnitParameterUnit_Indexed;
				break;
			case kDfxParamUnit_custom:
				{
					outParameterInfo.unit = kAudioUnitParameterUnit_CustomUnit;
					char customUnitString[DFX_PARAM_MAX_UNIT_STRING_LENGTH];
					customUnitString[0] = 0;
					getparameterunitstring(inParameterID, customUnitString);
					outParameterInfo.unitName = CFStringCreateWithCString(kCFAllocatorDefault, customUnitString, kDFX_DefaultCStringEncoding);
				}
				break;

			case kDfxParamUnit_generic:
			default:
				// if we got to this point, try using the value type to determine the unit type
				switch (getparametervaluetype(inParameterID))
				{
					case kDfxParamValueType_boolean:
						outParameterInfo.unit = kAudioUnitParameterUnit_Boolean;
						break;
					case kDfxParamValueType_int:
						outParameterInfo.unit = kAudioUnitParameterUnit_Indexed;
						break;
					default:
						outParameterInfo.unit = kAudioUnitParameterUnit_Generic;
						break;
				}
		}
	}


	return noErr;
}

//-----------------------------------------------------------------------------
// give the host an array of CFStrings with the display values for an indexed parameter
ComponentResult DfxPlugin::GetParameterValueStrings(AudioUnitScope inScope, 
					AudioUnitParameterID inParameterID, CFArrayRef * outStrings)
{
	// we're only handling the global scope
	if (inScope != kAudioUnitScope_Global)
		return kAudioUnitErr_InvalidScope;

	if (!parameterisvalid(inParameterID))
		return kAudioUnitErr_InvalidParameter;

	if (getparameterusevaluestrings(inParameterID))
	{
		// this is just to say that the property is supported (GetPropertyInfo needs this)
		if (outStrings == NULL)
			return noErr;

		CFStringRef * paramValueStrings = getparametervaluecfstrings(inParameterID);
		// if we don't actually have strings set, exit with an error
		if (paramValueStrings == NULL)
			return kAudioUnitErr_InvalidPropertyValue;
		// in case the min is not 0, get the total count of items in the array
		long numStrings = getparametermax_i(inParameterID) - getparametermin_i(inParameterID) + 1;
		// create a CFArray of the strings (the host will destroy the CFArray)
		*outStrings = CFArrayCreate(kCFAllocatorDefault, (const void**)paramValueStrings, numStrings, &kCFTypeArrayCallBacks);
		return noErr;
	}

	return kAudioUnitErr_InvalidPropertyValue;
}

//-----------------------------------------------------------------------------
ComponentResult DfxPlugin::SetParameter(AudioUnitParameterID inParameterID, 
					AudioUnitScope inScope, AudioUnitElement inElement, 
					Float32 inValue, UInt32 inBufferOffsetInFrames)
{
	if (inScope != kAudioUnitScope_Global)
		return kAudioUnitErr_InvalidScope;
	if (inElement != 0)
		return kAudioUnitErr_InvalidElement;
	if ( !parameterisvalid(inParameterID) )
		return kAudioUnitErr_InvalidParameter;

	setparameter_f(inParameterID, inValue);
	return noErr;
//	return AUBase::SetParameter(inParameterID, inScope, inElement, inValue, inBufferOffsetInFrames);
}



#pragma mark -
#pragma mark presets
#pragma mark -

//-----------------------------------------------------------------------------
// Returns an array of AUPreset that contain a number and name for each of the presets.  
// The number of each preset must be greater (or equal to) zero.  
// The CFArrayRef should be released by the caller.
ComponentResult DfxPlugin::GetPresets(CFArrayRef * outData) const
{
	// figure out how many valid (loaded) presets we actually have...
	long outNumPresets = 0;
	for (long i=0; i < numPresets; i++)
	{
//		if (presetnameisvalid(i))
		if ( (presets[i].getname_ptr() != NULL) && (presets[i].getname_ptr()[0] != 0) )
			outNumPresets++;
	}
	if (outNumPresets <= 0)	// woops, looks like we don't actually have any presets
//		return kAudioUnitErr_PropertyNotInUse;
		return kAudioUnitErr_InvalidProperty;

	// this is just to say that the property is supported (GetPropertyInfo needs this)
	if (outData == NULL)
		return noErr;

	// allocate a mutable array large enough to hold them all
	CFMutableArrayRef outArray = CFArrayCreateMutable(kCFAllocatorDefault, outNumPresets, &kCFAUPresetArrayCallBacks);
	if (outArray == NULL)
	{
		*outData = NULL;
		return coreFoundationUnknownErr;
	}

	// add the preset data (name and number) into the array
	for (long i=0; i < numPresets; i++)
	{
//		if (presetnameisvalid(i))
		if ( (presets[i].getname_ptr() != NULL) && (presets[i].getname_ptr()[0] != 0) )
		{
			// set the data as it should be
			CFAUPresetRef aupreset = CFAUPresetCreate(kCFAllocatorDefault, i, presets[i].getcfname());//getpresetcfname(i)
			if (aupreset != NULL)
			{
				// insert the AUPreset into the output array
				CFArrayAppendValue(outArray, aupreset);
				CFAUPresetRelease(aupreset);
			}
		}
	}

	*outData = (CFArrayRef)outArray;

	return noErr;
}

//-----------------------------------------------------------------------------
// this is called as a request to load a preset
OSStatus DfxPlugin::NewFactoryPresetSet(const AUPreset & inNewFactoryPreset)
{
	long newNumber = inNewFactoryPreset.presetNumber;

	if ( !presetisvalid(newNumber) )
		return kAudioUnitErr_InvalidPropertyValue;
	// for AU, we are using invalid preset names as a way of saying "not a real preset," 
	// even though it might be a valid (allocated) preset number
	if ( !presetnameisvalid(newNumber) )
		return kAudioUnitErr_InvalidPropertyValue;

	// try to load the preset
	if ( !loadpreset(newNumber) )
		return kAudioUnitErr_InvalidPropertyValue;	// XXX is this a good error code to return?

	return noErr;
}



#pragma mark -
#pragma mark state
#pragma mark -

// XXX I really should change this to something more like "DFX!-settings-data"
static const CFStringRef kDfxDataClassInfoKeyString = CFSTR("destroyfx-data");

//-----------------------------------------------------------------------------
// stores the values of all parameters values, state info, etc. into a CFPropertyListRef
ComponentResult DfxPlugin::SaveState(CFPropertyListRef * outData)
{
	ComponentResult result = TARGET_API_BASE_CLASS::SaveState(outData);
	if (result != noErr)
		return result;

#if TARGET_PLUGIN_USES_MIDI
	void * dfxdata = NULL;	// a pointer to our special data
	size_t dfxdatasize;	// the number of bytes of our data
	// fetch our special data
	dfxdatasize = dfxsettings->save(&dfxdata, true);
	if ( (dfxdatasize > 0) && (dfxdata != NULL) )
	{
		// create a CF data storage thingy filled with our special data
		CFDataRef cfdata = CFDataCreate(kCFAllocatorDefault, (const UInt8*)dfxdata, (CFIndex)dfxdatasize);
		if (cfdata != NULL)
		{
			// put the CF data storage thingy into the dfx-data section of the CF dictionary
			CFDictionarySetValue((CFMutableDictionaryRef)(*outData), kDfxDataClassInfoKeyString, cfdata);
			// cfdata belongs to us no more, bye bye...
			CFRelease(cfdata);
		}
	}
#endif

	return noErr;
}

#define DEBUG_VST_SETTINGS_IMPORT	0
//-----------------------------------------------------------------------------
// restores all parameter values, state info, etc. from the CFPropertyListRef
ComponentResult DfxPlugin::RestoreState(CFPropertyListRef inData)
{
#if DEBUG_VST_SETTINGS_IMPORT
fprintf(stderr, "\tDfxPlugin::RestoreState()\n");
#endif
	ComponentResult result = TARGET_API_BASE_CLASS::RestoreState(inData);
	// abort if the base implementation of RestoreState failed
	if (result != noErr)
	{
	#if DEBUG_VST_SETTINGS_IMPORT
		fprintf(stderr, "AUBase::RestoreState failed with error %ld, not attempting destroyfx-data\n", result);
	#endif
		return result;
	}

#if TARGET_PLUGIN_USES_MIDI
	// look for a data section keyed with our custom data key
	CFDataRef cfdata = NULL;
	Boolean dataFound = CFDictionaryGetValueIfPresent((CFDictionaryRef)inData, kDfxDataClassInfoKeyString, (const void**)&cfdata);

	#if DEBUG_VST_SETTINGS_IMPORT
	fprintf(stderr, "AUBase::RestoreState succeeded\n");
	if ( !dataFound || (cfdata == NULL) )
		fprintf(stderr, "but destroyfx-data was not found\n");
	else
		fprintf(stderr, "destroyfx-data successfully found\n");
	#endif

	// there was an error in AUBas::RestoreState or trying to find "destroyfx-data", 
	// but maybe some keys were missing and kAUPresetVSTDataKey is there...
	if ( !dataFound || (cfdata == NULL) )
	{
		// failing that, try to see if old VST chunk data is being fed to us
		dataFound = CFDictionaryGetValueIfPresent((CFDictionaryRef)inData, CFSTR(kAUPresetVSTDataKey), (const void**)&cfdata);

	#if DEBUG_VST_SETTINGS_IMPORT
	fprintf(stderr, "trying vstdata...\n");
	if ( !dataFound || (cfdata == NULL) )
		fprintf(stderr, "vstdata was not found\n");
	else
		fprintf(stderr, "vstdata was found, attempting to load...\n");
	#endif
	}

	bool success = false;
	if ( dataFound && (cfdata != NULL) )
	{
		// make sure that the settings item is the CoreFoundation type that we are expecting it to be
		if ( CFGetTypeID(cfdata) == CFDataGetTypeID() )
		{
			// a pointer to our special data
			const UInt8 * dfxdata = CFDataGetBytePtr(cfdata);
			// the number of bytes of our data
			size_t dfxdatasize = (size_t) CFDataGetLength(cfdata);
			// try to restore the saved settings data
			success = dfxsettings->restore((void*)dfxdata, dfxdatasize, true);

			#if DEBUG_VST_SETTINGS_IMPORT
			if (success)
				fprintf(stderr, "settings data was successfully loaded\n");
			else
				fprintf(stderr, "settings data failed to load\n");
			#endif

//			if (!success)
//				return kAudioUnitErr_InvalidPropertyValue;
		}
	}
	if (!success)
	{
#endif
// TARGET_PLUGIN_USES_MIDI

// XXX should we rethink this and load parameter settings always before dfxsettings->restore()?
	// load the parameter settings that were restored 
	// by the inherited base class implementation of RestoreState
	for (long i=0; i < numParameters; i++)
		setparameter_f(i, Globals()->GetParameter(i));

#if TARGET_PLUGIN_USES_MIDI
	}
#endif

	// make any listeners aware of the changes in the parameter values
	for (long i=0; i < numParameters; i++)
		postupdate_parameter(i);


	return noErr;
}



#pragma mark -
#pragma mark dsp
#pragma mark -

//-----------------------------------------------------------------------------
// the host calls this to inform the plugin that it wants to start using 
// a different audio stream format (sample rate, num channels, etc.)
ComponentResult DfxPlugin::ChangeStreamFormat(AudioUnitScope inScope, AudioUnitElement inElement, 
				const CAStreamBasicDescription & inPrevFormat, const CAStreamBasicDescription & inNewFormat)
{
//fprintf(stderr, "\nDfxPlugin::ChangeStreamFormat,   new sr = %.3lf,   old sr = %.3lf\n\n", inNewFormat.mSampleRate, inPrevFormat.mSampleRate);
//fprintf(stderr, "\nDfxPlugin::ChangeStreamFormat,   new num channels = %lu,   old num channels = %lu\n\n", inNewFormat.NumberChannels(), inPrevFormat.NumberChannels());
	const AUChannelInfo * auChannelConfigs = NULL;
	UInt32 numIOconfigs = SupportedNumChannels(&auChannelConfigs);
	// if this AU supports only specific i/o channel count configs, 
	// then try to check whether the incoming format is allowed
	if ( (numIOconfigs > 0) && (auChannelConfigs != NULL) )
	{
		SInt16 newNumChannels = (SInt16) (inNewFormat.NumberChannels());
		bool foundMatch = false;
		for (UInt32 i=0; (i < numIOconfigs) && !foundMatch; i++)
		{
			switch (inScope)
			{
				case kAudioUnitScope_Input:
					if ( (newNumChannels == auChannelConfigs[i].inChannels) || (auChannelConfigs[i].inChannels < 0) )
						foundMatch = true;
					break;
				case kAudioUnitScope_Output:
					if ( (newNumChannels == auChannelConfigs[i].outChannels) || (auChannelConfigs[i].outChannels < 0) )
						foundMatch = true;
					break;
				// XXX input and output scopes together at once?
				case kAudioUnitScope_Global:
					if ( ((newNumChannels == auChannelConfigs[i].inChannels) || (auChannelConfigs[i].inChannels < 0))
							&& ((newNumChannels == auChannelConfigs[i].outChannels) || (auChannelConfigs[i].outChannels < 0)) )
						foundMatch = true;
					break;
				default:
					return kAudioUnitErr_InvalidScope;
			}
		}
		// if the incoming channel count can't possibly work in 
		// any of the allowed channel configs, return an error
		if ( !foundMatch )
			return kAudioUnitErr_FormatNotSupported;
	}

	// use the inherited base class implementation
	ComponentResult result = TARGET_API_BASE_CLASS::ChangeStreamFormat(inScope, inElement, inPrevFormat, inNewFormat);

	if (result == noErr)
	{
		updatesamplerate();	// XXX do this here, or does it get caught elsewhere?
	}

	return result;
}

//-----------------------------------------------------------------------------
void DfxPlugin::UpdateInPlaceProcessingState()
{
#if !TARGET_PLUGIN_IS_INSTRUMENT
	// we can't do in-place audio rendering if there are different numbers of audio inputs and outputs
	if ( getnuminputs() != getnumoutputs() )
	{
		SetProcessesInPlace(false);
		PropertyChanged(kAudioUnitProperty_InPlaceProcessing, kAudioUnitScope_Global, (AudioUnitElement)0);
	}
#endif
}

#if TARGET_PLUGIN_IS_INSTRUMENT
//-----------------------------------------------------------------------------
ComponentResult DfxPlugin::Render(AudioUnitRenderActionFlags & ioActionFlags, 
							const AudioTimeStamp & inTimeStamp, UInt32 inFramesToProcess)
{
	ComponentResult result = noErr;

	// do any pre-DSP prep
	preprocessaudio();

	// get the output element
	AUOutputElement * theOutput = GetOutput(0);	// throws if there's an error
	UInt32 numOutputBuffers = theOutput->GetBufferList().mNumberBuffers;
	// set up our more convenient audio stream pointers
	for (UInt32 i=0; i < numOutputBuffers; i++)
		outputsP[i] = theOutput->GetChannelData(i);

	// do stuff to prepare the audio inputs, if we use any
	if (getnuminputs() > 0)
	{
		if ( !HasInput(0) )
			return kAudioUnitErr_NoConnection;
		AUInputElement * theInput = GetInput(0);
		result = theInput->PullInput(ioActionFlags, inTimeStamp, (AudioUnitElement)0, inFramesToProcess);
		if (result != noErr)
			return result;

		UInt32 numInputBuffers = theInput->GetBufferList().mNumberBuffers;
		// set up our more convenient audio stream pointers
		for (UInt32 i=0; i < numInputBuffers; i++)
			inputsP[i] = theInput->GetChannelData(i);
	}

	// now do the processing
	processaudio((const float**)inputsP, outputsP, inFramesToProcess);

	// I don't know what the hell this is for
	ioActionFlags &= ~kAudioUnitRenderAction_OutputIsSilence;

	// do any post-DSP stuff
	postprocessaudio();

	return result;
}

#else
// !TARGET_PLUGIN_IS_INSTRUMENT

//-----------------------------------------------------------------------------
// this is the audio processing routine
OSStatus DfxPlugin::ProcessBufferLists(AudioUnitRenderActionFlags & ioActionFlags, 
				const AudioBufferList & inBuffer, AudioBufferList & outBuffer, 
				UInt32 inFramesToProcess)
{
	OSStatus result = noErr;

	// do any pre-DSP prep
	preprocessaudio();

	// clear the output buffer because we will accumulate output into it
	if (audioProcessingAccumulatingOnly)
		AUBufferList::ZeroBuffer(outBuffer);

#if TARGET_PLUGIN_USES_DSPCORE
	// if the plugin uses DSP cores, then we just call the 
	// inherited base class implementation, which handles "Kernels"
	result = TARGET_API_BASE_CLASS::ProcessBufferLists(ioActionFlags, inBuffer, outBuffer, inFramesToProcess);

#else
	UInt32 numInputBuffers = inBuffer.mNumberBuffers;
	UInt32 numOutputBuffers = outBuffer.mNumberBuffers;

	// set up our more convenient audio stream pointers
	for (UInt32 i=0; i < numInputBuffers; i++)
		inputsP[i] = (float*) (inBuffer.mBuffers[i].mData);
	for (UInt32 i=0; i < numOutputBuffers; i++)
		outputsP[i] = (float*) (outBuffer.mBuffers[i].mData);

	// now do the processing
	processaudio((const float**)inputsP, outputsP, inFramesToProcess);

	// I don't know what the hell this is for
	ioActionFlags &= ~kAudioUnitRenderAction_OutputIsSilence;

#endif
// end of if/else TARGET_PLUGIN_USES_DSPCORE

	// do any post-DSP stuff
	postprocessaudio();

	return result;
}
#endif
// TARGET_PLUGIN_IS_INSTRUMENT



#pragma mark -
#pragma mark MIDI
#pragma mark -

#if TARGET_PLUGIN_USES_MIDI

//-----------------------------------------------------------------------------
OSStatus DfxPlugin::HandleNoteOn(UInt8 inChannel, UInt8 inNoteNumber, 
						UInt8 inVelocity, UInt32 inStartFrame)
{
	handlemidi_noteon(inChannel, inNoteNumber, inVelocity, inStartFrame);
	return noErr;
}

//-----------------------------------------------------------------------------
OSStatus DfxPlugin::HandleNoteOff(UInt8 inChannel, UInt8 inNoteNumber, 
						UInt8 inVelocity, UInt32 inStartFrame)
{
	handlemidi_noteoff(inChannel, inNoteNumber, inVelocity, inStartFrame);
	return noErr;
}

//-----------------------------------------------------------------------------
OSStatus DfxPlugin::HandleAllNotesOff(UInt8 inChannel)
{
	handlemidi_allnotesoff(inChannel, 0);
	return noErr;
}

//-----------------------------------------------------------------------------
OSStatus DfxPlugin::HandlePitchWheel(UInt8 inChannel, UInt8 inPitchLSB, UInt8 inPitchMSB, 
						UInt32 inStartFrame)
{
	handlemidi_pitchbend(inChannel, inPitchLSB, inPitchMSB, inStartFrame);
	return noErr;
}

//-----------------------------------------------------------------------------
OSStatus DfxPlugin::HandleControlChange(UInt8 inChannel, UInt8 inController, 
						UInt8 inValue, UInt32 inStartFrame)
{
	handlemidi_cc(inChannel, inController, inValue, inStartFrame);
	return noErr;
}

//-----------------------------------------------------------------------------
OSStatus DfxPlugin::HandleProgramChange(UInt8 inChannel, UInt8 inProgramNum)
{
	handlemidi_programchange(inChannel, inProgramNum, 0);
	// XXX maybe this is really all we want to do?
	loadpreset(inProgramNum);
	return noErr;
}

#if TARGET_PLUGIN_IS_INSTRUMENT
//-----------------------------------------------------------------------------
ComponentResult DfxPlugin::PrepareInstrument(MusicDeviceInstrumentID inInstrument)
{
	return kAudioUnitErr_PropertyNotInUse;
}

//-----------------------------------------------------------------------------
ComponentResult DfxPlugin::ReleaseInstrument(MusicDeviceInstrumentID inInstrument)
{
	return kAudioUnitErr_PropertyNotInUse;
}

//-----------------------------------------------------------------------------
ComponentResult DfxPlugin::StartNote(MusicDeviceInstrumentID inInstrument, 
						MusicDeviceGroupID inGroupID, NoteInstanceID * outNoteInstanceID, 
						UInt32 inOffsetSampleFrame, const MusicDeviceNoteParams & inParams)
{
	handlemidi_noteon(inGroupID, (int)(inParams.mPitch), (int)(inParams.mVelocity), inOffsetSampleFrame);
	if (outNoteInstanceID != NULL)
		*outNoteInstanceID = (NoteInstanceID) (inParams.mPitch);
	return noErr;
}

//-----------------------------------------------------------------------------
ComponentResult DfxPlugin::StopNote(MusicDeviceGroupID inGroupID, 
						NoteInstanceID inNoteInstanceID, UInt32 inOffsetSampleFrame)
{
	handlemidi_noteoff(inGroupID, inNoteInstanceID, 0, inOffsetSampleFrame);
	return noErr;
}
#endif
// TARGET_PLUGIN_IS_INSTRUMENT

#endif
// TARGET_PLUGIN_USES_MIDI
