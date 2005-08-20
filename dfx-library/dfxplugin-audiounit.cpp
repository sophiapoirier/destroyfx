/*------------------------------------------------------------------------
Destroy FX is a sovereign entity comprised of Marc Poirier & Tom Murphy 7.  
This is our class for E-Z plugin-making and E-Z multiple-API support.
This is where we connect the Audio Unit API to our DfxPlugin system.
written by Marc Poirier, October 2002
------------------------------------------------------------------------*/

#include "dfxplugin.h"
#include "dfx-au-utilities.h"



#pragma mark -
#pragma mark init
#pragma mark -

//-----------------------------------------------------------------------------
// this is called immediately after an instance of the plugin class is created
void DfxPlugin::PostConstructor()
{
	TARGET_API_BASE_CLASS::PostConstructor();
	auElementsHaveBeenCreated = true;

	dfxplugin_postconstructor();

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
		CAStreamBasicDescription curInStreamFormat(0.0, 0, 0, 0, 0, 0, 0, 0);
		if ( Inputs().GetNumberOfElements() > 0 )
			curInStreamFormat = GetStreamFormat(kAudioUnitScope_Input, (AudioUnitElement)0);
		const CAStreamBasicDescription curOutStreamFormat = GetStreamFormat(kAudioUnitScope_Output, (AudioUnitElement)0);
		bool currentFormatIsNotSupported = false;
		for (long i=0; i < numchannelconfigs; i++)
		{
			// compare the input channel count
			if ( ((UInt32)(channelconfigs[i].inChannels) != curInStreamFormat.mChannelsPerFrame) && (channelconfigs[i].inChannels >= 0) )
				currentFormatIsNotSupported = true;
			// compare the output channel count
			else if ( ((UInt32)(channelconfigs[i].outChannels) != curOutStreamFormat.mChannelsPerFrame) && (channelconfigs[i].outChannels >= 0) )
				currentFormatIsNotSupported = true;

			#if !TARGET_PLUGIN_IS_INSTRUMENT
				// we can't do in-place audio rendering if there are different numbers of audio inputs and outputs
				// XXX or is there a better selective way of handling this (depending on current stream format)?
				if (channelconfigs[i].inChannels != channelconfigs[i].outChannels)
					SetProcessesInPlace(false);
			#endif
		}
		// if the current format is not supported, then set the format to the first supported i/o pair in our list
		if (currentFormatIsNotSupported)
		{
			// change the input channel count to the first supported one listed
			CAStreamBasicDescription newStreamFormat(curInStreamFormat);
			newStreamFormat.mChannelsPerFrame = (UInt32) (channelconfigs[0].inChannels);
			if ( Inputs().GetNumberOfElements() > 0 )
				AUBase::ChangeStreamFormat(kAudioUnitScope_Input, (AudioUnitElement)0, curInStreamFormat, newStreamFormat);
			// change the output channel count to the first supported one listed
			newStreamFormat = CAStreamBasicDescription(curOutStreamFormat);
			newStreamFormat.mChannelsPerFrame = (UInt32) (channelconfigs[0].outChannels);
			AUBase::ChangeStreamFormat(kAudioUnitScope_Output, (AudioUnitElement)0, curOutStreamFormat, newStreamFormat);
		}
	}

// XXX some stuff that might worth adding an accessor for at some point or something...
#if 0
	unsigned char * appname = LMGetCurApName();
	if (appname != NULL)
		printf( "app name = %.*s\n", appname[0], (char*)&(appname[1]) );

	ProcessSerialNumber currentProcess = { 0, kCurrentProcess };
	CFStringRef processName = NULL;
	OSStatus status = CopyProcessName(&currentProcess, &processName);
	if ( (status == noErr) && (processName != NULL) )
	{
		CFIndex stringsize = (CFStringGetLength(processName) * sizeof(UniChar)) + 1;
		char * cname = (char*) malloc(stringsize * sizeof(char));
		cname[0] = 0;
		Boolean success = CFStringGetCString(processName, cname, stringsize, CFStringGetSystemEncoding());
		if (success)
			printf("process name = %s\n", cname);
		free(cname);
		CFRelease(processName);
	}
#endif
}

//-----------------------------------------------------------------------------
// this is called immediately before an instance of the plugin class is deleted
void DfxPlugin::PreDestructor()
{
	dfxplugin_predestructor();

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
			auNumInputs = (SInt16) GetStreamFormat(kAudioUnitScope_Input, (AudioUnitElement)0).mChannelsPerFrame;
		SInt16 auNumOutputs = (SInt16) GetStreamFormat(kAudioUnitScope_Output, (AudioUnitElement)0).mChannelsPerFrame;
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
		result = do_initialize();

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

	#if TARGET_PLUGIN_USES_MIDI
//		returns an array of AudioUnitMIDIControlMapping's, specifying a default mapping of
//		MIDI controls and/or NRPN's to AudioUnit scopes/elements/parameters.
		case kAudioUnitProperty_MIDIControlMapping:	// XXX deprecated in QuickTime 7
			if (inScope == kAudioUnitScope_Global)
			{
				outDataSize = sizeof(AudioUnitMIDIControlMapping*);
				outWritable = false;
			}
			else
				result = kAudioUnitErr_InvalidScope;
			break;
	#endif

		// this allows a GUI component to get a pointer to our DfxPlugin class instance
		case kDfxPluginProperty_PluginPtr:
			outDataSize = sizeof(DfxPlugin*);
			outWritable = false;
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
		case kDfxPluginProperty_RandomizeParameters:
			// when you "set" this "property", you send a bool to say whether or not to write automation data
			outDataSize = sizeof(bool);
			outWritable = true;
			break;

	#if TARGET_PLUGIN_USES_MIDI
		// get/set the MIDI learn state
		case kDfxPluginProperty_MidiLearn:
			outDataSize = sizeof(bool);
			outWritable = true;
			break;
		// clear MIDI parameter assignments
		case kDfxPluginProperty_ResetMidiLearn:
			outDataSize = 0;	// you don't need an input value for this property
			outWritable = true;
			break;
		// get/set the current MIDI learner parameter
		case kDfxPluginProperty_MidiLearner:
			outDataSize = sizeof(long);
			outWritable = true;
			break;
	#endif

		default:
			result = TARGET_API_BASE_CLASS::GetPropertyInfo(inPropertyID, inScope, inElement, outDataSize, outWritable);
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
				else if (inElement != 0)
					result = kAudioUnitErr_InvalidElement;
				else
				{
					AudioUnitParameterNameInfo * ioClumpInfo = (AudioUnitParameterNameInfo*) outData;
					if (ioClumpInfo->inID == kAudioUnitClumpID_System)	// this ID value is reserved
						result = kAudioUnitErr_InvalidPropertyValue;
					else
					{
						ioClumpInfo->outName = CopyClumpName(ioClumpInfo->inID);
						if (ioClumpInfo->outName == NULL)
							result = kAudioUnitErr_InvalidPropertyValue;
					}
				}
			}
			break;

		case kAudioUnitMigrateProperty_FromPlugin:
			{
				// VST counterpart description
				static AudioUnitOtherPluginDesc vstPluginMigrationDesc;
				memset(&vstPluginMigrationDesc, 0, sizeof(vstPluginMigrationDesc));
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
					pvt->auValue = expandparametervalue_index((long)(pvt->otherParamID), pvt->otherValue);
				}
				else
					result = kAudioUnitErr_InvalidPropertyValue;
			}
			break;

	#if TARGET_PLUGIN_USES_MIDI
		case kAudioUnitProperty_MIDIControlMapping:	// XXX deprecated in QuickTime 7
			if (inScope != kAudioUnitScope_Global)
				result = kAudioUnitErr_InvalidScope;
			else
			{
				for (long i=0; i < numParameters; i++)
				{
					aumidicontrolmap[i].midiNRPN = 0xFFFF;	// this means "none"
					aumidicontrolmap[i].midiControl = 0xFF;	// this means "none"
					// but we might still have a CC assignment to share...
					if (dfxsettings != NULL)
					{
						if (dfxsettings->getParameterAssignmentType(i) == kParamEventCC)
							aumidicontrolmap[i].midiControl = dfxsettings->getParameterAssignmentNum(i);
					}
					aumidicontrolmap[i].scope = kAudioUnitScope_Global;
					aumidicontrolmap[i].element = (AudioUnitElement) 0;
					aumidicontrolmap[i].parameter = (AudioUnitParameterID) i;
				}
				*(AudioUnitMIDIControlMapping**)outData = aumidicontrolmap;
			}
			break;
	#endif

		// this allows a GUI component to get a pointer to our DfxPlugin class instance
		case kDfxPluginProperty_PluginPtr:
			*(DfxPlugin**)outData = this;
			break;

		// get parameter values (current, min, max, etc.) using specific variable types
		// XXX finish implementing all items
		case kDfxPluginProperty_ParameterValue:
			{
				DfxParameterValueRequest * request = (DfxParameterValueRequest*) outData;
				DfxParamValue * value = &(request->value);
				long paramID = request->parameterID;
				switch (request->valueItem)
				{
					case kDfxParameterValueItem_current:
						{
							switch (request->valueType)
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
							switch (request->valueType)
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
							switch (request->valueType)
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
							switch (request->valueType)
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
			}
			break;

		// expand or contract a parameter value
		case kDfxPluginProperty_ParameterValueConversion:
			{
				DfxParameterValueConversionRequest * request = (DfxParameterValueConversionRequest*) outData;
				switch (request->conversionType)
				{
					case kDfxParameterValueConversion_expand:
						request->outValue = expandparametervalue_index(request->parameterID, request->inValue);
						break;
					case kDfxParameterValueConversion_contract:
						request->outValue = contractparametervalue_index(request->parameterID, request->inValue);
						break;
					default:
						result = paramErr;
						break;
				}
			}
			break;

		// get parameter value strings
		case kDfxPluginProperty_ParameterValueString:
			{
				DfxParameterValueStringRequest * request = (DfxParameterValueStringRequest*) outData;
				if ( !getparametervaluestring(request->parameterID, request->stringIndex, request->valueString) )
					result = paramErr;
			}
			break;

	#if TARGET_PLUGIN_USES_MIDI
		// get the MIDI learn state
		case kDfxPluginProperty_MidiLearn:
			*(bool*)outData = dfxsettings->isLearning();
			break;
		// get the current MIDI learner parameter
		case kDfxPluginProperty_MidiLearner:
			*(long*)outData = dfxsettings->getLearner();
			break;
	#endif

		default:
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

		case kAudioUnitMigrateProperty_FromPlugin:
			result = kAudioUnitErr_PropertyNotWritable;
			break;

		case kAudioUnitMigrateProperty_OldAutomation:
			result = kAudioUnitErr_PropertyNotWritable;
			break;

	#if TARGET_PLUGIN_USES_MIDI
		case kAudioUnitProperty_MIDIControlMapping:	// XXX deprecated in QuickTime 7
			result = kAudioUnitErr_PropertyNotWritable;
			break;
	#endif

		case kDfxPluginProperty_PluginPtr:
			result = kAudioUnitErr_PropertyNotWritable;
			break;

		case kDfxPluginProperty_ParameterValueConversion:
			result = kAudioUnitErr_PropertyNotWritable;
			break;

		// set parameter values (current, min, max, etc.) using specific variable types
		// XXX finish implementing all items
		case kDfxPluginProperty_ParameterValue:
			{
				DfxParameterValueRequest * request = (DfxParameterValueRequest*) inData;
				DfxParamValue * value = &(request->value);
				long paramID = request->parameterID;
				switch (request->valueItem)
				{
					case kDfxParameterValueItem_current:
						{
							switch (request->valueType)
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
							switch (request->valueType)
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
							switch (request->valueType)
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
							switch (request->valueType)
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

		// set parameter value strings
		case kDfxPluginProperty_ParameterValueString:
			{
				DfxParameterValueStringRequest * request = (DfxParameterValueStringRequest*) inData;
				if ( !setparametervaluestring(request->parameterID, request->stringIndex, request->valueString) )
					result = paramErr;
			}
			break;

		// randomize the parameters
		case kDfxPluginProperty_RandomizeParameters:
			// when you "set" this "property", you send a bool to say whether or not to write automation data
			randomizeparameters( *(bool*)inData );
			break;

	#if TARGET_PLUGIN_USES_MIDI
		// set the MIDI learn state
		case kDfxPluginProperty_MidiLearn:
			dfxsettings->setParameterMidiLearn( *(bool*)inData );
			break;
		// clear MIDI parameter assignments
		case kDfxPluginProperty_ResetMidiLearn:
			// you don't need an input value for this property
			dfxsettings->setParameterMidiReset();
			break;
		// set the current MIDI learner parameter
		case kDfxPluginProperty_MidiLearner:
			dfxsettings->setLearner( *(long*)inData );
			break;
	#endif

		default:
			result = TARGET_API_BASE_CLASS::SetProperty(inPropertyID, inScope, inElement, inData, inDataSize);
			break;
	}

	return result;
}

//-----------------------------------------------------------------------------
// should be a version 32-bit number hex-encoded like so:  
// 0xMMMMmmbb (M = major version, m = minor version, and b = bugfix)
ComponentResult	DfxPlugin::Version()
{
	return PLUGIN_VERSION;
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
	inDescArray->componentManufacturer = DESTROYFX_ID;
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
	if (getparameterattributes(inParameterID) & kDfxParamAttribute_unused)
	{
		outParameterInfo.flags = 0;
//		return kAudioUnitErr_InvalidParameter;	// XXX ey?
	}
	// if the parameter is hidden, then indicate that it's not readable or writable...
	else if (getparameterattributes(inParameterID) & kDfxParamAttribute_hidden)
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
	switch (getparametercurve(inParameterID))
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

	// the complicated part:  getting the unit type 
	// (but easy if we use value strings for value display)
	outParameterInfo.unitName = NULL;	// default to nothing, will be set if it is needed
	if (getparameterusevaluestrings(inParameterID))
		outParameterInfo.unit = kAudioUnitParameterUnit_Indexed;
	else
	{
		switch (getparameterunit(inParameterID))
		{
			case kDfxParamUnit_generic:
				outParameterInfo.unit = kAudioUnitParameterUnit_Generic;
				break;
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
					outParameterInfo.unitName = CFStringCreateWithCString(kCFAllocatorDefault, customUnitString, CFStringGetSystemEncoding());
				}
				break;

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

#if 0
	// setup up the callbacks for the CFArray's value handling
	CFArrayCallBacks arrayCallbacks;
	AUPresetCFArrayCallbacks_Init(&arrayCallbacks);
	// ...and then allocate a mutable array large enough to hold them all
	CFMutableArrayRef outArray = CFArrayCreateMutable(kCFAllocatorDefault, outNumPresets, &arrayCallbacks);
#else
	// ...and then allocate a mutable array large enough to hold them all
	CFMutableArrayRef outArray = CFArrayCreateMutable(kCFAllocatorDefault, outNumPresets, &kAUPresetCFArrayCallbacks);
#endif

	// add the preset data (name and number) into the array
	for (long i=0; i < numPresets; i++)
	{
//		if (presetnameisvalid(i))
		if ( (presets[i].getname_ptr() != NULL) && (presets[i].getname_ptr()[0] != 0) )
		{
			AUPreset aupreset;
			// set the data as it should be
			aupreset.presetNumber = i;
//			aupreset.presetName = getpresetcfname(i);
			aupreset.presetName = presets[i].getcfname();
			// insert the AUPreset into the output array
			CFArrayAppendValue(outArray, &aupreset);
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
	unsigned long dfxdatasize;	// the number of bytes of our data
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
	// but maybe some keys were missing and "vstdata" is there...
	if ( !dataFound || (cfdata == NULL) )
	{
		// failing that, try to see if old VST chunk data is being fed to us
		dataFound = CFDictionaryGetValueIfPresent((CFDictionaryRef)inData, CFSTR("vstdata"), (const void**)&cfdata);

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
		// a pointer to our special data
		const UInt8 * dfxdata = CFDataGetBytePtr(cfdata);
		// the number of bytes of our data
		unsigned long dfxdatasize = (unsigned) CFDataGetLength(cfdata);
		// try to restore the saved settings data
		success = dfxsettings->restore((void*)dfxdata, dfxdatasize, true);

		#if DEBUG_VST_SETTINGS_IMPORT
		if (success)
			fprintf(stderr, "settings data was successfully loaded\n");
		else
			fprintf(stderr, "settings data failed to load\n");
		#endif

//		if (!success)
//			return kAudioUnitErr_InvalidPropertyValue;
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
//fprintf(stderr, "\nDfxPlugin::ChangeStreamFormat,   new num channels = %lu,   old num channels = %lu\n\n", inNewFormat.mChannelsPerFrame, inPrevFormat.mChannelsPerFrame);
	const AUChannelInfo * auChannelConfigs = NULL;
	UInt32 numIOconfigs = SupportedNumChannels(&auChannelConfigs);
	// if this AU supports only specific i/o channel count configs, 
	// then try to check whether the incoming format is allowed
	if ( (numIOconfigs > 0) && (auChannelConfigs != NULL) )
	{
		SInt16 newNumChannels = (SInt16) (inNewFormat.mChannelsPerFrame);
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
	return TARGET_API_BASE_CLASS::ChangeStreamFormat(inScope, inElement, inPrevFormat, inNewFormat);
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
	AudioBufferList & outBuffers = theOutput->GetBufferList();
	UInt32 outNumBuffers = outBuffers.mNumberBuffers;
	// set up our more convenient audio stream pointers
	for (UInt32 i=0; i < outNumBuffers; i++)
	{
		outputsP[i] = (float*) (outBuffers.mBuffers[i].mData);
//		outBuffers.mBuffers[i].mDataByteSize = inFramesToProcess * sizeof(Float32);
	}

	// do stuff to prepare the audio inputs, if we use any
	if (getnuminputs() > 0)
	{
		if ( !HasInput(0) )
			return kAudioUnitErr_NoConnection;
		AUInputElement * theInput = GetInput(0);
		result = theInput->PullInput(ioActionFlags, inTimeStamp, (AudioUnitElement)0, inFramesToProcess);
		if (result != noErr)
			return result;

		AudioBufferList & inBuffers = theInput->GetBufferList();
		UInt32 inNumBuffers = inBuffers.mNumberBuffers;
		// set up our more convenient audio stream pointers
		for (UInt32 i=0; i < inNumBuffers; i++)
			inputsP[i] = (float*) (inBuffers.mBuffers[i].mData);
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

#if TARGET_PLUGIN_USES_DSPCORE
	// if the plugin uses DSP cores, then we just call the 
	// inherited base class implementation, which handles "Kernels"
	result = TARGET_API_BASE_CLASS::ProcessBufferLists(ioActionFlags, inBuffer, outBuffer, inFramesToProcess);

#else
	UInt32 inNumBuffers = inBuffer.mNumberBuffers;
	UInt32 outNumBuffers = outBuffer.mNumberBuffers;

	// set up our more convenient audio stream pointers
	for (UInt32 i=0; i < inNumBuffers; i++)
		inputsP[i] = (float*) (inBuffer.mBuffers[i].mData);
	for (UInt32 i=0; i < outNumBuffers; i++)
	{
		outputsP[i] = (float*) (outBuffer.mBuffers[i].mData);
//		outBuffer.mBuffers[i].mDataByteSize = inFramesToProcess * sizeof(Float32);
	}

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
void DfxPlugin::HandleNoteOn(int inChannel, UInt8 inNoteNumber, 
						UInt8 inVelocity, long inFrameOffset)
{
	handlemidi_noteon(inChannel, inNoteNumber, inVelocity, inFrameOffset);
}

//-----------------------------------------------------------------------------
void DfxPlugin::HandleNoteOff(int inChannel, UInt8 inNoteNumber, 
						UInt8 inVelocity, long inFrameOffset)
{
	handlemidi_noteoff(inChannel, inNoteNumber, inVelocity, inFrameOffset);
}

//-----------------------------------------------------------------------------
void DfxPlugin::HandleAllNotesOff(int inChannel)
{
	handlemidi_allnotesoff(inChannel, 0);
}

//-----------------------------------------------------------------------------
void DfxPlugin::HandlePitchWheel(int inChannel, UInt8 inPitchLSB, UInt8 inPitchMSB, 
						long inFrameOffset)
{
	handlemidi_pitchbend(inChannel, inPitchLSB, inPitchMSB, inFrameOffset);
}

//-----------------------------------------------------------------------------
void DfxPlugin::HandleControlChange(int inChannel, UInt8 inController, 
						UInt8 inValue, long inFrameOffset)
{
	handlemidi_cc(inChannel, inController, inValue, inFrameOffset);
}

//-----------------------------------------------------------------------------
void DfxPlugin::HandleProgramChange(int inChannel, UInt8 inProgramNum)
{
	handlemidi_programchange(inChannel, inProgramNum, 0);
	// XXX maybe this is really all we want to do?
	loadpreset(inProgramNum);
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
						MusicDeviceGroupID inGroupID, NoteInstanceID & outNoteInstanceID, 
						UInt32 inOffsetSampleFrame, const MusicDeviceNoteParams & inParams)
{
	ComponentResult result = HandleMidiEvent(kMidiNoteOn, (UInt8)inGroupID, (UInt8)(inParams.mPitch), (UInt8)(inParams.mVelocity), inOffsetSampleFrame);
	if (result == noErr)
		outNoteInstanceID = (NoteInstanceID) (inParams.mPitch);
	return result;
}

//-----------------------------------------------------------------------------
ComponentResult DfxPlugin::StopNote(MusicDeviceGroupID inGroupID, 
						NoteInstanceID inNoteInstanceID, UInt32 inOffsetSampleFrame)
{
	return HandleMidiEvent(kMidiNoteOff, (UInt8)inGroupID, (UInt8)inNoteInstanceID, 0, inOffsetSampleFrame);
}
#endif
// TARGET_PLUGIN_IS_INSTRUMENT

#endif
// TARGET_PLUGIN_USES_MIDI
