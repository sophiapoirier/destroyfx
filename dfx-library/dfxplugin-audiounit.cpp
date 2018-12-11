/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2018  Sophia Poirier

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

#include <AudioUnit/AudioUnitCarbonView.h>
#include <cassert>

#include "dfx-au-utilities.h"
#include "dfxmisc.h"

#if DFXGUI_USE_COCOA_AU_VIEW
	#include "dfxgui-auviewfactory.h"
#endif



#pragma mark -
#pragma mark init
#pragma mark -

//-----------------------------------------------------------------------------
// this is called immediately after an instance of the plugin class is created
void DfxPlugin::PostConstructor()
{
	TARGET_API_BASE_CLASS::PostConstructor();
	mAUElementsHaveBeenCreated = true;

	do_PostConstructor();

	// make host see that current preset is 0
	update_preset(0);

	// make the global-scope element aware of the parameters' values
	// this must happen after AUBase::PostConstructor because the elements are created there
	for (long i = 0; i < getnumparameters(); i++)
	{
		if (!(getparameterattributes(i) & DfxParam::kAttribute_Unused))  // XXX should we do it like this, or override GetParameterList?
		{
			AUBase::SetParameter(i, kAudioUnitScope_Global, AudioUnitElement(0), getparameter_f(i), 0);
		}
	}

	// make sure that the input and output elements are initially set to a supported number of channels, 
	// if this plugin specifies a list of supported i/o channel-count pairs
	if (!mChannelconfigs.empty())
	{
		CAStreamBasicDescription curInStreamFormat;
		curInStreamFormat.mChannelsPerFrame = 0;
		if (Inputs().GetNumberOfElements() > 0)
		{
			curInStreamFormat = GetStreamFormat(kAudioUnitScope_Input, AudioUnitElement(0));
		}
		auto const& curOutStreamFormat = GetStreamFormat(kAudioUnitScope_Output, AudioUnitElement(0));
		bool currentFormatIsNotSupported = false;
		for (auto const& channelconfig : mChannelconfigs)
		{
			// compare the input channel count
			if ((static_cast<UInt32>(channelconfig.inChannels) != curInStreamFormat.NumberChannels()) && (channelconfig.inChannels >= 0))
			{
				currentFormatIsNotSupported = true;
			}
			// compare the output channel count
			else if ((static_cast<UInt32>(channelconfig.outChannels) != curOutStreamFormat.NumberChannels()) && (channelconfig.outChannels >= 0))
			{
				currentFormatIsNotSupported = true;
			}
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
			constexpr UInt32 defaultNumChannels = 2;
			// change the input channel count to the first supported one listed
			auto newNumChannels = (mChannelconfigs.front().inChannels < 0) ? defaultNumChannels : static_cast<UInt32>(mChannelconfigs.front().inChannels);
			CAStreamBasicDescription newStreamFormat(curInStreamFormat);
			newStreamFormat.ChangeNumberChannels(newNumChannels, newStreamFormat.IsInterleaved());
			if (Inputs().GetNumberOfElements() > 0)
			{
				AUBase::ChangeStreamFormat(kAudioUnitScope_Input, AudioUnitElement(0), curInStreamFormat, newStreamFormat);
			}
			// change the output channel count to the first supported one listed
			newNumChannels = (mChannelconfigs.front().outChannels < 0) ? defaultNumChannels : static_cast<UInt32>(mChannelconfigs.front().outChannels);
			newStreamFormat = CAStreamBasicDescription(curOutStreamFormat);
			newStreamFormat.ChangeNumberChannels(newNumChannels, newStreamFormat.IsInterleaved());
			AUBase::ChangeStreamFormat(kAudioUnitScope_Output, AudioUnitElement(0), curOutStreamFormat, newStreamFormat);
		}
		UpdateInPlaceProcessingState();
	}
}

//-----------------------------------------------------------------------------
// this is called immediately before an instance of the plugin class is deleted
void DfxPlugin::PreDestructor()
{
	do_PreDestructor();

	TARGET_API_BASE_CLASS::PreDestructor();
}

//-----------------------------------------------------------------------------
// this is like a second constructor, kind of
// it is called when the Audio Unit is expected to be ready to process audio
// this is where DSP-specific resources should be allocated
OSStatus DfxPlugin::Initialize()
{
	OSStatus status = noErr;

#if TARGET_PLUGIN_IS_INSTRUMENT
	AUChannelInfo const* auChannelConfigs = nullptr;
	auto numIOconfigs = SupportedNumChannels(&auChannelConfigs);
	AUChannelInfo auChannelConfigs_temp;
	// if this AU supports only specific i/o channel count configs, then check whether the current format is allowed
	if (!auChannelConfigs)
	{
		numIOconfigs = 0;
	}
	if ((numIOconfigs == 0) && !mChannelconfigs.empty())
	{
		auChannelConfigs_temp.inChannels = mChannelconfigs.front().inChannels;
		auChannelConfigs_temp.outChannels = mChannelconfigs.front().outChannels;
		auChannelConfigs = &auChannelConfigs_temp;
		numIOconfigs = 1;
	}
	if (numIOconfigs > 0)
	{
		SInt16 auNumInputs = 0;
		if (Inputs().GetNumberOfElements() > 0)
		{
			auNumInputs = static_cast<SInt16>(GetStreamFormat(kAudioUnitScope_Input, AudioUnitElement(0)).NumberChannels());
		}
		auto const auNumOutputs = static_cast<SInt16>(GetStreamFormat(kAudioUnitScope_Output, AudioUnitElement(0)).NumberChannels());
		bool foundMatch = false;
		for (UInt32 i = 0; (i < numIOconfigs) && !foundMatch; i++)
		{
			auto const configNumInputs = auChannelConfigs[i].inChannels;
			auto const configNumOutputs = auChannelConfigs[i].outChannels;
			// handle the special "wildcard" cases of negative AUChannelInfo values
			if ((configNumInputs < 0) && (configNumOutputs < 0))
			{
				// a -1,-2 AUChannelInfo pair signifies that any number of inputs and outputs is allowed
				if (((configNumInputs == -1) && (configNumOutputs == -2)) || ((configNumInputs == -2) && (configNumOutputs == -1)))
				{
					foundMatch = true;
				}
				// failing that, a -1,-1 pair signifies that any number of ins and outs are allowed, as long as they are equal
				else if (((configNumInputs == -1) && (configNumOutputs == -1)) && (auNumInputs == auNumOutputs))
				{
					foundMatch = true;
				}
				// any other pair of negative values are illegal, so skip this AUChannelInfo pair
				else
				{
					continue;
				}
			}
			// handle literal AUChannelInfo values (and maybe a wildcard on one of the scopes)
			else
			{
				bool const inputMatch = (auNumInputs == configNumInputs) || (configNumInputs == -1);
				bool const outputMatch = (auNumOutputs == configNumOutputs) || (configNumOutputs == -1);
				// if input and output are both allowed in this i/o pair description, then we found a match
				if (inputMatch && outputMatch)
				{
					foundMatch = true;
				}
			}
		}
		// if the current i/o counts don't match any of the allowed configs, return an error
		if (!foundMatch)
		{
			return kAudioUnitErr_FormatNotSupported;
		}
	}
#else
	// call the inherited class' Initialize routine
	status = TARGET_API_BASE_CLASS::Initialize();
#endif
// TARGET_PLUGIN_IS_INSTRUMENT

	// call our initialize routine
	if (status == noErr)
	{
		status = do_initialize();
		UpdateInPlaceProcessingState();
	}

	return status;
}

//-----------------------------------------------------------------------------
// this is the "destructor" partner to Initialize
// any DSP-specific resources should be released here
void DfxPlugin::Cleanup()
{
	do_cleanup();

	TARGET_API_BASE_CLASS::Cleanup();  // XXX doesn't actually do anything now, but maybe at some point?
}

//-----------------------------------------------------------------------------
// this is called when an audio stream is broken somehow 
// (playback stop/restart, change of playback position, etc.)
// any DSP state variables should be reset here 
// (contents of buffers, position trackers, IIR filter histories, etc.)
OSStatus DfxPlugin::Reset(AudioUnitScope /*inScope*/, AudioUnitElement /*inElement*/)
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
OSStatus DfxPlugin::GetPropertyInfo(AudioUnitPropertyID inPropertyID, 
									AudioUnitScope inScope, AudioUnitElement inElement, 
									UInt32& outDataSize, Boolean& outWritable)
{
	OSStatus status = noErr;

	switch (inPropertyID)
	{
		case kAudioUnitProperty_CocoaUI:
#if DFXGUI_USE_COCOA_AU_VIEW
			outDataSize = sizeof(AudioUnitCocoaViewInfo);
			outWritable = false;
			status = noErr;
#else
			status = kAudioUnitErr_PropertyNotInUse;
#endif
			break;

		case kAudioUnitProperty_AUHostIdentifier:
			outDataSize = sizeof(AUHostIdentifier);  // XXX update to AUHostVersionIdentifier
			outWritable = true;
			status = noErr;
			break;

		case kAudioUnitMigrateProperty_FromPlugin:
			outDataSize = sizeof(CFArrayRef);
			outWritable = false;
			status = noErr;
			break;

		case kAudioUnitMigrateProperty_OldAutomation:
			outDataSize = sizeof(AudioUnitParameterValueTranslation);
			outWritable = false;
			status = noErr;
			break;

		// get/set parameter values (current, min, max, etc.) using specific variable types
		case dfx::kPluginProperty_ParameterValue:
			outDataSize = sizeof(dfx::ParameterValueRequest);
			outWritable = true;
			break;

		// expand or contract a parameter value
		case dfx::kPluginProperty_ParameterValueConversion:
			outDataSize = sizeof(dfx::ParameterValueConversionRequest);
			outWritable = false;
			break;

		// get/set parameter value strings
		case dfx::kPluginProperty_ParameterValueString:
			outDataSize = sizeof(dfx::ParameterValueStringRequest);
			outWritable = true;
			break;

		// get parameter unit label
		case dfx::kPluginProperty_ParameterUnitLabel:
			outDataSize = dfx::kParameterUnitStringMaxLength;
			outWritable = false;
			break;

		// get parameter value type
		case dfx::kPluginProperty_ParameterValueType:
			outDataSize = sizeof(DfxParam::ValueType);
			outWritable = false;
			break;

		// get parameter unit
		case dfx::kPluginProperty_ParameterUnit:
			outDataSize = sizeof(DfxParam::Unit);
			outWritable = false;
			break;

		// randomize the parameters
		case dfx::kPluginProperty_RandomizeParameter:
			// when you "set" this "property", you send a boolean to say whether or not to write automation data
			outDataSize = sizeof(Boolean);
			outWritable = true;
			break;

	#if TARGET_PLUGIN_USES_MIDI
		// get/set the MIDI learn state
		case dfx::kPluginProperty_MidiLearn:
			outDataSize = sizeof(Boolean);
			outWritable = true;
			break;
		// clear MIDI parameter assignments
		case dfx::kPluginProperty_ResetMidiLearn:
			outDataSize = sizeof(Boolean);  // you don't need an input value for this property
			outWritable = true;
			break;
		// get/set the current MIDI learner parameter
		case dfx::kPluginProperty_MidiLearner:
			outDataSize = sizeof(int32_t);
			outWritable = true;
			break;
		// get/set the MIDI assignment for a parameter
		case dfx::kPluginProperty_ParameterMidiAssignment:
			outDataSize = sizeof(dfx::ParameterAssignment);
			outWritable = true;
			break;
	#endif

	#if LOGIC_AU_PROPERTIES_AVAILABLE
		case kLogicAUProperty_NodeOperationMode:
			outDataSize = sizeof(UInt32);
			outWritable = true;
			break;

		case kLogicAUProperty_NodePropertyDescriptions:
			outDataSize = sizeof(LogicAUNodePropertyDescription) * dfx_GetNumPluginProperties();
			outWritable = false;
			break;
	#endif

		default:
			if (inPropertyID >= dfx::kPluginProperty_StartID)
			{
				size_t dfxDataSize {};
				dfx::PropertyFlags dfxFlags {};
				status = dfx_GetPropertyInfo(inPropertyID, inScope, inElement, dfxDataSize, dfxFlags);
				if (status == noErr)
				{
					outDataSize = dfxDataSize;
					outWritable = dfxFlags & dfx::kPropertyFlag_Writable;
				}
			}
			else
			{
				status = TARGET_API_BASE_CLASS::GetPropertyInfo(inPropertyID, inScope, inElement, outDataSize, outWritable);
			}
			break;
	}

	return status;
}

//-----------------------------------------------------------------------------
// get specific information about Audio Unit Properties
// most properties are handled by inherited base class implementations
OSStatus DfxPlugin::GetProperty(AudioUnitPropertyID inPropertyID, 
								AudioUnitScope inScope, AudioUnitElement inElement, 
								void* outData)
{
	OSStatus status = noErr;

	switch (inPropertyID)
	{
#if DFXGUI_USE_COCOA_AU_VIEW
		case kAudioUnitProperty_CocoaUI:
		{
			auto const pluginBundle = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));
			if (pluginBundle)
			{
				AudioUnitCocoaViewInfo cocoaViewInfo {};
				cocoaViewInfo.mCocoaAUViewBundleLocation = CFBundleCopyBundleURL(pluginBundle);
				if (cocoaViewInfo.mCocoaAUViewBundleLocation)
				{
					cocoaViewInfo.mCocoaAUViewClass[0] = dfx::DGCocoaAUViewFactory_CopyClassName();
					if (cocoaViewInfo.mCocoaAUViewClass[0])
					{
						*static_cast<AudioUnitCocoaViewInfo*>(outData) = cocoaViewInfo;
						status = noErr;
					}
					else
					{
						status = coreFoundationUnknownErr;
					}
				}
				else
				{
					status = fnfErr;
				}
			}
			else
			{
				status = fnfErr;
			}
			break;
		}
#endif

		case kAudioUnitMigrateProperty_FromPlugin:
		{
			// VST counterpart description
			static AudioUnitOtherPluginDesc vstPluginMigrationDesc {};
			vstPluginMigrationDesc.format = kOtherPluginFormat_kVST;
			vstPluginMigrationDesc.plugin.mSubType = PLUGIN_ID;
			// create a CFArray of the VST counterpart description
			auto const descsArray = CFArrayCreateMutable(nullptr, 1, nullptr);
			if (descsArray)
			{
				CFArrayAppendValue(descsArray, &vstPluginMigrationDesc);
				*static_cast<CFArrayRef*>(outData) = descsArray;
			}
			else
			{
				status = coreFoundationUnknownErr;
			}
			break;
		}

		case kAudioUnitMigrateProperty_OldAutomation:
		{
			auto const pvt = static_cast<AudioUnitParameterValueTranslation*>(outData);
			if ((pvt->otherDesc.format == kOtherPluginFormat_kVST) && (pvt->otherDesc.plugin.mSubType == PLUGIN_ID))
			{
				pvt->auParamID = pvt->otherParamID;
				pvt->auValue = expandparametervalue(static_cast<long>(pvt->otherParamID), pvt->otherValue);
			}
			else
			{
				status = kAudioUnitErr_InvalidPropertyValue;
			}
			break;
		}

		// get parameter values (current, min, max, etc.) using specific variable types
		// XXX finish implementing all items
		case dfx::kPluginProperty_ParameterValue:
		{
			auto const request = static_cast<dfx::ParameterValueRequest*>(outData);
		#if LOGIC_AU_PROPERTIES_AVAILABLE
			if (isLogicNodeEndianReversed())
			{
				request->inValueItem = CFSwapInt32(request->inValueItem);
				request->inValueType = CFSwapInt32(request->inValueType);
			}
		#endif
			auto& value = request->value;
			auto const paramID = static_cast<long>(inElement);
			switch (request->inValueItem)
			{
				case dfx::ParameterValueItem::Current:
				{
					switch (request->inValueType)
					{
						case DfxParam::ValueType::Float:
							value.f = getparameter_f(paramID);
							break;
						case DfxParam::ValueType::Int:
							value.i = getparameter_i(paramID);
							break;
						case DfxParam::ValueType::Boolean:
							value.b = getparameter_b(paramID);
							break;
						default:
							assert(false);
							status = paramErr;
							break;
					}
					break;
				}
				case dfx::ParameterValueItem::Previous:
					assert(false);  // XXX TODO: implement this
					break;
				case dfx::ParameterValueItem::Default:
				{
					switch (request->inValueType)
					{
						case DfxParam::ValueType::Float:
							value.f = getparameterdefault_f(paramID);
							break;
						case DfxParam::ValueType::Int:
//							value.i = getparameterdefault_i(paramID);
							break;
						case DfxParam::ValueType::Boolean:
//							value.b = getparameterdefault_b(paramID);
							break;
						default:
							assert(false);
							status = paramErr;
							break;
					}
					break;
				}
				case dfx::ParameterValueItem::Min:
				{
					switch (request->inValueType)
					{
						case DfxParam::ValueType::Float:
							value.f = getparametermin_f(paramID);
							break;
						case DfxParam::ValueType::Int:
							value.i = getparametermin_i(paramID);
							break;
						case DfxParam::ValueType::Boolean:
//							value.b = getparametermin_b(paramID);
							value.b = false;
							break;
						default:
							assert(false);
							status = paramErr;
							break;
					}
					break;
				}
				case dfx::ParameterValueItem::Max:
				{
					switch (request->inValueType)
					{
						case DfxParam::ValueType::Float:
							value.f = getparametermax_f(paramID);
							break;
						case DfxParam::ValueType::Int:
							value.i = getparametermax_i(paramID);
							break;
						case DfxParam::ValueType::Boolean:
//							value.b = getparametermax_b(paramID);
							value.b = true;
							break;
						default:
							assert(false);
							status = paramErr;
							break;
					}
					break;
				}
				default:
					assert(false);
					status = paramErr;
					break;
			}
		#if LOGIC_AU_PROPERTIES_AVAILABLE
			if (isLogicNodeEndianReversed())
			{
				request->value.i = CFSwapInt64(request->value.i);
			}
		#endif
			break;
		}

		// expand or contract a parameter value
		case dfx::kPluginProperty_ParameterValueConversion:
		{
			auto const request = static_cast<dfx::ParameterValueConversionRequest*>(outData);
		#if LOGIC_AU_PROPERTIES_AVAILABLE
			if (isLogicNodeEndianReversed())
			{
				request->inConversionType = CFSwapInt32(request->inConversionType);
				dfx::ReverseBytes(&(request->inValue), sizeof(request->inValue));
			}
		#endif
			switch (request->inConversionType)
			{
				case dfx::ParameterValueConversionType::Expand:
					request->outValue = expandparametervalue(inElement, request->inValue);
					break;
				case dfx::ParameterValueConversionType::Contract:
					request->outValue = contractparametervalue(inElement, request->inValue);
					break;
				default:
					assert(false);
					status = paramErr;
					break;
			}
		#if LOGIC_AU_PROPERTIES_AVAILABLE
			if (isLogicNodeEndianReversed())
			{
				dfx::ReverseBytes(&(request->outValue), sizeof(request->outValue));
			}
		#endif
			break;
		}

		// get parameter value strings
		case dfx::kPluginProperty_ParameterValueString:
		{
			auto const request = static_cast<dfx::ParameterValueStringRequest*>(outData);
		#if LOGIC_AU_PROPERTIES_AVAILABLE
			if (isLogicNodeEndianReversed())
			{
				request->inStringIndex = CFSwapInt64(request->inStringIndex);
			}
		#endif
			if (!getparametervaluestring(inElement, request->inStringIndex, request->valueString))
			{
				status = paramErr;
			}
			break;
		}

		// get parameter unit label
		case dfx::kPluginProperty_ParameterUnitLabel:
			if (!parameterisvalid(inElement))
			{
				status = kAudioUnitErr_InvalidParameter;
			}
			else
			{
				getparameterunitstring(inElement, static_cast<char*>(outData));
			}
			break;

		// get parameter value type
		case dfx::kPluginProperty_ParameterValueType:
			if (!parameterisvalid(inElement))
			{
				status = kAudioUnitErr_InvalidParameter;
			}
			else
			{
				*static_cast<DfxParam::ValueType*>(outData) = getparametervaluetype(inElement);
			}
			break;

		// get parameter unit
		case dfx::kPluginProperty_ParameterUnit:
			if (!parameterisvalid(inElement))
			{
				status = kAudioUnitErr_InvalidParameter;
			}
			else
			{
				*static_cast<DfxParam::Unit*>(outData) = getparameterunit(inElement);
			}
			break;

	#if TARGET_PLUGIN_USES_MIDI
		// get the MIDI learn state
		case dfx::kPluginProperty_MidiLearn:
			*static_cast<Boolean*>(outData) = mDfxSettings->isLearning();
			break;
		// get the current MIDI learner parameter
		case dfx::kPluginProperty_MidiLearner:
			*static_cast<int32_t*>(outData) = mDfxSettings->getLearner();
			break;
		// get the MIDI assignment for a parameter
		case dfx::kPluginProperty_ParameterMidiAssignment:
			*static_cast<dfx::ParameterAssignment*>(outData) = mDfxSettings->getParameterAssignment(inElement);
			break;
	#endif

	#if LOGIC_AU_PROPERTIES_AVAILABLE
		case kLogicAUProperty_NodeOperationMode:
			*static_cast<UInt32*>(outData) = getSupportedLogicNodeOperationMode();
			break;

		case kLogicAUProperty_NodePropertyDescriptions:
		{
			auto const nodePropertyDescs = static_cast<LogicAUNodePropertyDescription*>(outData);
			for (long i = 0; i < dfx_GetNumPluginProperties(); i++)
			{
				nodePropertyDescs[i].mPropertyID = dfx::kPluginProperty_StartID + i;
				nodePropertyDescs[i].mEndianMode = kLogicAUNodePropertyEndianMode_DontTouch;
				nodePropertyDescs[i].mFlags = kLogicAUNodePropertyFlag_Synchronous | kLogicAUNodePropertyFlag_NeedsInitialization;
				switch (nodePropertyDescs[i].mPropertyID)
				{
					case dfx::kPluginProperty_ParameterValue:
						nodePropertyDescs[i].mFlags |= kLogicAUNodePropertyFlag_FullRoundTrip;
						break;
					case dfx::kPluginProperty_ParameterValueConversion:
						nodePropertyDescs[i].mFlags |= kLogicAUNodePropertyFlag_FullRoundTrip;
						break;
					case dfx::kPluginProperty_ParameterValueString:
						nodePropertyDescs[i].mFlags |= kLogicAUNodePropertyFlag_FullRoundTrip;
						break;
					case dfx::kPluginProperty_ParameterValueType:
					case dfx::kPluginProperty_ParameterUnit:
						nodePropertyDescs[i].mEndianMode = kLogicAUNodePropertyEndianMode_All32Bits;
						nodePropertyDescs[i].mFlags |= kLogicAUNodePropertyFlag_FullRoundTrip;
						break;
					case dfx::kPluginProperty_MidiLearner:
						nodePropertyDescs[i].mEndianMode = kLogicAUNodePropertyEndianMode_All32Bits;
						break;
					case dfx::kPluginProperty_ParameterMidiAssignment:
						nodePropertyDescs[i].mEndianMode = kLogicAUNodePropertyEndianMode_All32Bits;
						break;
					default:
					{
						size_t dfxDataSize {};
						dfx::PropertyFlags dfxFlags {};
						auto const dfxStatus = dfx_GetPropertyInfo(nodePropertyDescs[i].mPropertyID, dfx::kScope_Global, 0, dfxDataSize, dfxFlags);
						if (dfxStatus == dfx::kStatus_NoError)
						{
							if (dfxFlags & dfx::kPropertyFlag_BiDirectional)
							{
								nodePropertyDescs[i].mFlags |= kLogicAUNodePropertyFlag_FullRoundTrip;
							}
						}
						break;
					}
				}
			}
			break;
		}
	#endif

		default:
			if (inPropertyID >= dfx::kPluginProperty_StartID)
			{
				size_t dfxDataSize {};
				dfx::PropertyFlags dfxFlags {};
				status = dfx_GetPropertyInfo(inPropertyID, inScope, inElement, dfxDataSize, dfxFlags);
				if ((status == noErr) && !(dfxFlags & dfx::kPropertyFlag_Readable))
				{
					status = kAudioUnitErr_InvalidPropertyValue;
				}
				else
				{
					status = dfx_GetProperty(inPropertyID, inScope, inElement, outData);
				}
			}
			else
			{
				status = TARGET_API_BASE_CLASS::GetProperty(inPropertyID, inScope, inElement, outData);
			}
			break;
	}

	return status;
}

//-----------------------------------------------------------------------------
OSStatus DfxPlugin::SetProperty(AudioUnitPropertyID inPropertyID, 
								AudioUnitScope inScope, AudioUnitElement inElement, 
								void const* inData, UInt32 inDataSize)
{
	OSStatus status = noErr;

	switch (inPropertyID)
	{
		case kAudioUnitProperty_AUHostIdentifier:
		{
			auto const hostID = static_cast<AUHostIdentifier const*>(inData);  // XXX update to AUHostVersionIdentifier
			if (!hostID->hostName)
			{
				status = kAudioUnitErr_InvalidPropertyValue;
			}
			else
			{
#if 0
				fprintf(stderr, "\tSetProperty(AUHostIdentifier)\n");
				CFShow(hostID->hostName);
				fprintf(stderr, "version: major = %u, minor&bug = 0x%02X, stage = 0x%02X, non-released = %u\n", hostID->hostVersion.majorRev, hostID->hostVersion.minorAndBugRev, hostID->hostVersion.stage, hostID->hostVersion.nonRelRev);
#endif
			}
			break;
		}

	#if !TARGET_PLUGIN_IS_INSTRUMENT
		case kAudioUnitProperty_InPlaceProcessing:
			if ((mAudioProcessingAccumulatingOnly || (getnuminputs() != getnumoutputs())) 
				&& (*static_cast<UInt32 const*>(inData) != 0))
			{
				status = kAudioUnitErr_InvalidPropertyValue;
			}
			else
			{
				status = TARGET_API_BASE_CLASS::SetProperty(inPropertyID, inScope, inElement, inData, inDataSize);
			}
			break;
	#endif

		// set parameter values (current, min, max, etc.) using specific variable types
		// XXX finish implementing all items
		case dfx::kPluginProperty_ParameterValue:
		{
			auto const request = static_cast<dfx::ParameterValueRequest const*>(inData);
		#if LOGIC_AU_PROPERTIES_AVAILABLE
			if (isLogicNodeEndianReversed())
			{
				request->inValueItem = CFSwapInt32(request->inValueItem);
				request->inValueType = CFSwapInt32(request->inValueType);
				request->value.i = CFSwapInt64(request->value.i);
			}
		#endif
			auto const& value = request->value;
			long paramID = inElement;
			switch (request->inValueItem)
			{
				case dfx::ParameterValueItem::Current:
				{
					switch (request->inValueType)
					{
						case DfxParam::ValueType::Float:
							setparameter_f(paramID, value.f);
							postupdate_parameter(paramID);
							break;
						case DfxParam::ValueType::Int:
							setparameter_i(paramID, value.i);
							postupdate_parameter(paramID);
							break;
						case DfxParam::ValueType::Boolean:
							setparameter_b(paramID, value.b);
							postupdate_parameter(paramID);
							break;
						default:
							assert(false);
							status = paramErr;
							break;
					}
					break;
				}
				case dfx::ParameterValueItem::Previous:
					assert(false);  // XXX TODO: implement
					break;
				case dfx::ParameterValueItem::Default:
				{
					assert(false);  // XXX TODO: implement
					switch (request->inValueType)
					{
						case DfxParam::ValueType::Float:
//							setparameterdefault_f(paramID, value.f);
							break;
						case DfxParam::ValueType::Int:
//							setparameterdefault_i(paramID, value.i);
							break;
						case DfxParam::ValueType::Boolean:
//							setparameterdefault_b(paramID, value.b);
							break;
						default:
							assert(false);
							status = paramErr;
							break;
					}
					break;
				}
				case dfx::ParameterValueItem::Min:
				{
					assert(false);  // XXX TODO: implement
					switch (request->inValueType)
					{
						case DfxParam::ValueType::Float:
//							setparametermin_f(paramID, value.f);
							break;
						case DfxParam::ValueType::Int:
//							setparametermin_i(paramID, value.i);
							break;
						case DfxParam::ValueType::Boolean:
//							setparametermin_b(paramID, value.b);
							break;
						default:
							assert(false);
							status = paramErr;
							break;
					}
					break;
				}
				case dfx::ParameterValueItem::Max:
				{
					assert(false);  // XXX TODO: implement
					switch (request->inValueType)
					{
						case DfxParam::ValueType::Float:
//							setparametermax_f(paramID, value.f);
							break;
						case DfxParam::ValueType::Int:
//							setparametermax_i(paramID, value.i);
							break;
						case DfxParam::ValueType::Boolean:
//							setparametermax_b(paramID, value.b);
							break;
						default:
							assert(false);
							status = paramErr;
							break;
					}
					break;
				}
				default:
					assert(false);
					status = paramErr;
					break;
			}
			break;
		}

		// set parameter value strings
		case dfx::kPluginProperty_ParameterValueString:
		{
			auto const request = static_cast<dfx::ParameterValueStringRequest const*>(inData);
		#if LOGIC_AU_PROPERTIES_AVAILABLE
			if (isLogicNodeEndianReversed())
			{
				request->inStringIndex = CFSwapInt64(request->inStringIndex);
			}
		#endif
			if (!setparametervaluestring(inElement, request->inStringIndex, request->valueString))
			{
				status = paramErr;
			}
			break;
		}

		// randomize the parameters
		case dfx::kPluginProperty_RandomizeParameter:
			// when you "set" this "property", you send a bool to say whether or not to write automation data
			if (inElement == kAUParameterListener_AnyParameter)
			{
				randomizeparameters(*static_cast<Boolean const*>(inData));
			}
			else
			{
				randomizeparameter(inElement);
			}
			break;

	#if TARGET_PLUGIN_USES_MIDI
		// set the MIDI learn state
		case dfx::kPluginProperty_MidiLearn:
			mDfxSettings->setParameterMidiLearn(*static_cast<Boolean const*>(inData));
			break;
		// clear MIDI parameter assignments
		case dfx::kPluginProperty_ResetMidiLearn:
			// you don't need an input value for this property
			mDfxSettings->setParameterMidiReset();
			break;
		// set the current MIDI learner parameter
		case dfx::kPluginProperty_MidiLearner:
		{
			auto const paramID = *static_cast<int32_t const*>(inData);
			if (auto const assignmentData = settings_getLearningAssignData(paramID))
			{
				mDfxSettings->setLearner(paramID, assignmentData->mEventBehaviorFlags, 
										 assignmentData->mDataInt1, assignmentData->mDataInt2, 
										 assignmentData->mDataFloat1, assignmentData->mDataFloat2);
			}
			else if (getparametervaluetype(paramID) == DfxParam::ValueType::Float)
			{
				mDfxSettings->setLearner(paramID);
			}
			else
			{
				auto const numStates = getparametermax_i(paramID) - getparametermin_i(paramID) + 1;
				mDfxSettings->setLearner(paramID, dfx::kMidiEventBehaviorFlag_Toggle, numStates);
			}
			break;
		}
		// set the MIDI assignment for a parameter
		case dfx::kPluginProperty_ParameterMidiAssignment:
		{
			auto const paramAssignment = static_cast<dfx::ParameterAssignment const*>(inData);
			if (paramAssignment->mEventType == dfx::MidiEventType::None)
			{
				mDfxSettings->unassignParam(inElement);
			}
			else
			{
				mDfxSettings->assignParam(inElement, paramAssignment->mEventType, paramAssignment->mEventChannel,
										  paramAssignment->mEventNum, paramAssignment->mEventNum2, 
										  paramAssignment->mEventBehaviorFlags, 
										  paramAssignment->mDataInt1, paramAssignment->mDataInt2, 
										  paramAssignment->mDataFloat1, paramAssignment->mDataFloat2);
			}
			break;
		}
	#endif

	#if LOGIC_AU_PROPERTIES_AVAILABLE
		case kLogicAUProperty_NodeOperationMode:
			setCurrentLogicNodeOperationMode(*static_cast<UInt32 const*>(inData));
			break;
	#endif

		default:
			if (inPropertyID >= dfx::kPluginProperty_StartID)
			{
				size_t dfxDataSize {};
				dfx::PropertyFlags dfxFlags {};
				status = dfx_GetPropertyInfo(inPropertyID, inScope, inElement, dfxDataSize, dfxFlags);
				if ((status == noErr) && !(dfxFlags & dfx::kPropertyFlag_Writable))
				{
					status = kAudioUnitErr_PropertyNotWritable;
				}
				else
				{
					status = dfx_SetProperty(inPropertyID, inScope, inElement, inData, inDataSize);
				}
			}
			else
			{
				status = TARGET_API_BASE_CLASS::SetProperty(inPropertyID, inScope, inElement, inData, inDataSize);
			}
			break;
	}

	return status;
}

//-----------------------------------------------------------------------------
// should be a version 32-bit number hex-encoded like so:  
// 0xMMMMmmbb (M = major version, m = minor version, and b = bugfix)
OSStatus DfxPlugin::Version()
{
	return getpluginversion();
}

//-----------------------------------------------------------------------------
// give the host an array of the audio input/output channel configurations 
// that the plugin supports
// if the pointer passed in is null, then simply return the number of supported configurations
// if any n-to-n configuration (i.e. same number of ins and outs) is supported, return 0
UInt32 DfxPlugin::SupportedNumChannels(AUChannelInfo const** outInfo)
{
	if (!mChannelconfigs.empty() && outInfo)
	{
		*outInfo = mChannelconfigs.data();
	}
	return static_cast<UInt32>(mChannelconfigs.size());
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
	auto const pluginBundle = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));
	if (pluginBundle)
	{
		return CFBundleCopyResourceURL(pluginBundle, CFSTR(PLUGIN_ICON_FILE_NAME), nullptr, nullptr);
	}
#endif

	return nullptr;
}

#if TARGET_PLUGIN_HAS_GUI
//-----------------------------------------------------------------------------
int DfxPlugin::GetNumCustomUIComponents()
{
#if __LP64__
	return 0;
#else
	return 1;
#endif
}

//-----------------------------------------------------------------------------
void DfxPlugin::GetUIComponentDescs(ComponentDescription* inDescArray)
{
	if (!inDescArray)
	{
		return;
	}

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
OSStatus DfxPlugin::GetParameterInfo(AudioUnitScope inScope, 
			AudioUnitParameterID inParameterID, 
			AudioUnitParameterInfo& outParameterInfo)
{
	// we're only handling the global scope
	if (inScope != kAudioUnitScope_Global)
	{
		return kAudioUnitErr_InvalidScope;
	}

	if (!parameterisvalid(inParameterID))
	{
		return kAudioUnitErr_InvalidParameter;
	}

	// get the name into a temp string buffer that we know is large enough
	char tempname[dfx::kParameterNameMaxLength];
	getparametername(inParameterID, tempname);
	// then make sure to only copy as much as the ParameterInfo name C string can hold
	strlcpy(outParameterInfo.name, tempname, sizeof(outParameterInfo.name));
	// in case the parameter name was dfx::kParameterNameMaxLength or longer, 
	// make sure that the ParameterInfo name string is terminated
	outParameterInfo.name[sizeof(outParameterInfo.name) - 1] = 0;
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
//	if (presetisvalid(0))
	{
//		outParameterInfo.defaultValue = getpresetparameter_f(0, inParameterID);
	}
//	else
// XXX bzzzt!  no, now Bill Stewart at Apple says that our definition of default value is correct?
	{
		outParameterInfo.defaultValue = getparameterdefault_f(inParameterID);
	}
	// check if the parameter is used or not (stupid VST workaround)
	if (getparameterattributes(inParameterID) & DfxParam::kAttribute_Unused)
	{
		outParameterInfo.flags = 0;
//		return kAudioUnitErr_InvalidParameter;  // XXX ey?
	}
	// if the parameter is hidden, then indicate that it's not readable or writable...
	else if (getparameterattributes(inParameterID) & DfxParam::kAttribute_Hidden)
	{
		outParameterInfo.flags = 0;
	}
	// ...otherwise all parameters are readable and writable
	else
	{
		outParameterInfo.flags = kAudioUnitParameterFlag_IsReadable | kAudioUnitParameterFlag_IsWritable;
	}
	if (outParameterInfo.cfNameString)
	{
		outParameterInfo.flags |= kAudioUnitParameterFlag_HasCFNameString;
		CFRetain(outParameterInfo.cfNameString);
		outParameterInfo.flags |= kAudioUnitParameterFlag_CFNameRelease;
	}
	switch (getparametercurve(inParameterID))
	{
		// so far, this is all that AU offers as far as parameter distribution curves go
		case DfxParam::Curve::Log:
			outParameterInfo.flags |= kAudioUnitParameterFlag_DisplayLogarithmic;
			break;
		case DfxParam::Curve::SquareRoot:
			outParameterInfo.flags |= kAudioUnitParameterFlag_DisplaySquared;
			break;
		case DfxParam::Curve::Squared:
			outParameterInfo.flags |= kAudioUnitParameterFlag_DisplaySquareRoot;
			break;
		case DfxParam::Curve::Cubed:
			outParameterInfo.flags |= kAudioUnitParameterFlag_DisplayCubeRoot;
			break;
		case DfxParam::Curve::Exp:
			outParameterInfo.flags |= kAudioUnitParameterFlag_DisplayExponential;
			break;
// XXX currently no way to handle this with AU, except I guess by using HasName thingy, but I'd much rather not do that
// XXX or maybe estimate?  like:  if (curveSpec < 1.0) DisplaySquareRoot; else if (curveSpec < 2.5) DisplaySquared; else DisplayCubed
//		case DfxParam::Curve::Pow:
//			break;
		default:
			break;
	}

	if (getparametervaluetype(inParameterID) == DfxParam::ValueType::Float)
	{
		outParameterInfo.flags |= kAudioUnitParameterFlag_IsHighResolution;
	}

	// the complicated part:  getting the unit type 
	// (but easy if we use value strings for value display)
	outParameterInfo.unitName = nullptr;  // default to nothing, will be set if it is needed
	if (getparameterusevaluestrings(inParameterID))
	{
		outParameterInfo.unit = kAudioUnitParameterUnit_Indexed;
	}
	else
	{
		switch (getparameterunit(inParameterID))
		{
			case DfxParam::Unit::Percent:
				outParameterInfo.unit = kAudioUnitParameterUnit_Percent;
				break;
			case DfxParam::Unit::LinearGain:
				outParameterInfo.unit = kAudioUnitParameterUnit_LinearGain;
				break;
			case DfxParam::Unit::Decibles:
				outParameterInfo.unit = kAudioUnitParameterUnit_Decibels;
				break;
			case DfxParam::Unit::DryWetMix:
				outParameterInfo.unit = kAudioUnitParameterUnit_EqualPowerCrossfade;
				break;
			case DfxParam::Unit::Hz:
				outParameterInfo.unit = kAudioUnitParameterUnit_Hertz;
				break;
			case DfxParam::Unit::Seconds:
				outParameterInfo.unit = kAudioUnitParameterUnit_Seconds;
				break;
			case DfxParam::Unit::MS:
				outParameterInfo.unit = kAudioUnitParameterUnit_Milliseconds;
				break;
			case DfxParam::Unit::Samples:
				outParameterInfo.unit = kAudioUnitParameterUnit_SampleFrames;
				break;
			case DfxParam::Unit::Scalar:
				outParameterInfo.unit = kAudioUnitParameterUnit_Rate;
				break;
			case DfxParam::Unit::Divisor:
				outParameterInfo.unit = kAudioUnitParameterUnit_Generic;
				break;
			case DfxParam::Unit::Exponent:
				outParameterInfo.unit = kAudioUnitParameterUnit_Generic;
				break;
			case DfxParam::Unit::Semitones:
				outParameterInfo.unit = kAudioUnitParameterUnit_RelativeSemiTones;
				break;
			case DfxParam::Unit::Octaves:
				outParameterInfo.unit = kAudioUnitParameterUnit_Octaves;
				break;
			case DfxParam::Unit::Cents:
				outParameterInfo.unit = kAudioUnitParameterUnit_Cents;
				break;
			case DfxParam::Unit::Notes:
				outParameterInfo.unit = kAudioUnitParameterUnit_MIDINoteNumber;
				break;
			case DfxParam::Unit::Pan:
				outParameterInfo.unit = kAudioUnitParameterUnit_Pan;
				break;
			case DfxParam::Unit::BPM:
				outParameterInfo.unit = kAudioUnitParameterUnit_BPM;
				break;
			case DfxParam::Unit::Beats:
				outParameterInfo.unit = kAudioUnitParameterUnit_Beats;
				break;
			case DfxParam::Unit::List:
				outParameterInfo.unit = kAudioUnitParameterUnit_Indexed;
				break;
			case DfxParam::Unit::Custom:
			{
				outParameterInfo.unit = kAudioUnitParameterUnit_CustomUnit;
				char customUnitString[dfx::kParameterUnitStringMaxLength];
				customUnitString[0] = 0;
				getparameterunitstring(inParameterID, customUnitString);
				outParameterInfo.unitName = CFStringCreateWithCString(kCFAllocatorDefault, customUnitString, DfxParam::kDefaultCStringEncoding);
				break;
			}

			case DfxParam::Unit::Generic:
			default:
				// if we got to this point, try using the value type to determine the unit type
				switch (getparametervaluetype(inParameterID))
				{
					case DfxParam::ValueType::Boolean:
						outParameterInfo.unit = kAudioUnitParameterUnit_Boolean;
						break;
					case DfxParam::ValueType::Int:
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
OSStatus DfxPlugin::GetParameterValueStrings(AudioUnitScope inScope, AudioUnitParameterID inParameterID,  
											 CFArrayRef* outStrings)
{
	// we're only handling the global scope
	if (inScope != kAudioUnitScope_Global)
	{
		return kAudioUnitErr_InvalidScope;
	}

	if (!parameterisvalid(inParameterID))
	{
		return kAudioUnitErr_InvalidParameter;
	}

	if (getparameterusevaluestrings(inParameterID))
	{
		// this is just to say that the property is supported (GetPropertyInfo needs this)
		if (!outStrings)
		{
			return noErr;
		}

		// in case the min is not 0, get the total count of items in the array
		long const numStrings = getparametermax_i(inParameterID) - getparametermin_i(inParameterID) + 1;
		// create a CFArray of the strings (the host will destroy the CFArray)
		dfx::UniqueCFType array = CFArrayCreateMutable(kCFAllocatorDefault, numStrings, &kCFTypeArrayCallBacks);
		if (!array)
		{
			return coreFoundationUnknownErr;
		}
		for (long i = 0; i < numStrings; i++)
		{
			auto const paramValueString = getparametervaluecfstring(inParameterID, i);
			// if we don't actually have strings set, exit with an error
			if (!paramValueString)
			{
				return kAudioUnitErr_InvalidPropertyValue;
			}
			CFArrayAppendValue(array.get(), paramValueString);
		}
		*outStrings = array.release();
		return noErr;
	}

	return kAudioUnitErr_InvalidPropertyValue;
}

//-----------------------------------------------------------------------------
OSStatus DfxPlugin::SetParameter(AudioUnitParameterID inParameterID, 
			AudioUnitScope inScope, AudioUnitElement inElement, 
			Float32 inValue, UInt32 /*inBufferOffsetInFrames*/)
{
	if (inScope != kAudioUnitScope_Global)
	{
		return kAudioUnitErr_InvalidScope;
	}
	if (inElement != 0)
	{
		return kAudioUnitErr_InvalidElement;
	}
	if (!parameterisvalid(inParameterID))
	{
		return kAudioUnitErr_InvalidParameter;
	}

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
OSStatus DfxPlugin::GetPresets(CFArrayRef* outData) const
{
	// figure out how many valid (loaded) presets we actually have...
	long outNumPresets = 0;
	for (long i = 0; i < getnumpresets(); i++)
	{
//		if (presetnameisvalid(i))
		if (mPresets[i].getname_ptr() && (mPresets[i].getname_ptr()[0] != 0))
		{
			outNumPresets++;
		}
	}
	if (outNumPresets <= 0)  // woops, looks like we don't actually have any presets
	{
//		return kAudioUnitErr_PropertyNotInUse;
		return kAudioUnitErr_InvalidProperty;
	}

	// this is just to say that the property is supported (GetPropertyInfo needs this)
	if (!outData)
	{
		return noErr;
	}

	// allocate a mutable array large enough to hold them all
	auto const outArray = CFArrayCreateMutable(kCFAllocatorDefault, outNumPresets, &kCFAUPresetArrayCallBacks);
	if (!outArray)
	{
		*outData = nullptr;
		return coreFoundationUnknownErr;
	}

	// add the preset data (name and number) into the array
	for (long i = 0; i < getnumpresets(); i++)
	{
		if (presetnameisvalid(i))
		{
			// set the data as it should be
			dfx::UniqueCFAUPreset const aupreset(kCFAllocatorDefault, i, getpresetcfname(i));
			if (aupreset)
			{
				// insert the AUPreset into the output array
				CFArrayAppendValue(outArray, aupreset.get());
			}
		}
	}

	*outData = reinterpret_cast<CFArrayRef>(outArray);

	return noErr;
}

//-----------------------------------------------------------------------------
// this is called as a request to load a preset
OSStatus DfxPlugin::NewFactoryPresetSet(AUPreset const& inNewFactoryPreset)
{
	long newNumber = inNewFactoryPreset.presetNumber;

	if (!presetisvalid(newNumber))
	{
		return kAudioUnitErr_InvalidPropertyValue;
	}
	// for AU, we are using invalid preset names as a way of saying "not a real preset," 
	// even though it might be a valid (allocated) preset number
	if (!presetnameisvalid(newNumber))
	{
		return kAudioUnitErr_InvalidPropertyValue;
	}

	// try to load the preset
	if (!loadpreset(newNumber))
	{
		return kAudioUnitErr_InvalidPropertyValue;  // XXX is this a good error code to return?
	}

	return noErr;
}



#pragma mark -
#pragma mark state
#pragma mark -

// XXX I really should change this to something more like "DFX!-settings-data"
static CFStringRef const kDfxDataClassInfoKeyString = CFSTR("destroyfx-data");

//-----------------------------------------------------------------------------
// stores the values of all parameters values, state info, etc. into a CFPropertyListRef
OSStatus DfxPlugin::SaveState(CFPropertyListRef* outData)
{
	auto const status = TARGET_API_BASE_CLASS::SaveState(outData);
	if (status != noErr)
	{
		return status;
	}

#if TARGET_PLUGIN_USES_MIDI
	// fetch our special data
	if (auto const dfxData = mDfxSettings->save(true); !dfxData.empty())
	{
		// create a CF data storage thingy filled with our special data
		dfx::UniqueCFType const cfData = CFDataCreate(kCFAllocatorDefault, dfxData.data(), static_cast<CFIndex>(dfxData.size()));
		if (cfData)
		{
			// put the CF data storage thingy into the dfx-data section of the CF dictionary
			CFDictionarySetValue((CFMutableDictionaryRef)(*outData), kDfxDataClassInfoKeyString, cfData.get());
		}
	}
#endif

	return noErr;
}

//-----------------------------------------------------------------------------
// restores all parameter values, state info, etc. from the CFPropertyListRef
OSStatus DfxPlugin::RestoreState(CFPropertyListRef inData)
{
	auto const status = TARGET_API_BASE_CLASS::RestoreState(inData);
	// abort if the base implementation of RestoreState failed
	if (status != noErr)
	{
		return status;
	}

#if TARGET_PLUGIN_USES_MIDI
	// look for a data section keyed with our custom data key
	CFDataRef cfData = nullptr;
	auto dataFound = CFDictionaryGetValueIfPresent(reinterpret_cast<CFDictionaryRef>(inData), kDfxDataClassInfoKeyString, reinterpret_cast<void const**>(&cfData));

	// there was an error in AUBas::RestoreState or trying to find "destroyfx-data", 
	// but maybe some keys were missing and kAUPresetVSTDataKey is there...
	if (!dataFound || !cfData)
	{
		// failing that, try to see if old VST chunk data is being fed to us
		dataFound = CFDictionaryGetValueIfPresent((CFDictionaryRef)inData, CFSTR(kAUPresetVSTDataKey), reinterpret_cast<void const**>(&cfData));
	}

	bool success = false;
	if (dataFound && cfData)
	{
		// make sure that the settings item is the CoreFoundation type that we are expecting it to be
		if (CFGetTypeID(cfData) == CFDataGetTypeID())
		{
			// a pointer to our special data
			auto const dfxData = CFDataGetBytePtr(cfData);
			// the number of bytes of our data
			auto const dfxDataSize = static_cast<size_t>(CFDataGetLength(cfData));
			// try to restore the saved settings data
			success = mDfxSettings->restore(static_cast<void const*>(dfxData), dfxDataSize, true);

//			if (!success)
			{
//				return kAudioUnitErr_InvalidPropertyValue;
			}
		}
	}
	if (!success)
	{
#endif
// TARGET_PLUGIN_USES_MIDI

// XXX should we rethink this and load parameter settings always before mDfxSettings->restore()?
	// load the parameter settings that were restored 
	// by the inherited base class implementation of RestoreState
	for (long i = 0; i < getnumparameters(); i++)
	{
		setparameter_f(i, Globals()->GetParameter(i));
	}

#if TARGET_PLUGIN_USES_MIDI
	}
#endif

	// make any listeners aware of the changes in the parameter values
	for (long i = 0; i < getnumparameters(); i++)
	{
		postupdate_parameter(i);
	}


	return noErr;
}



#pragma mark -
#pragma mark dsp
#pragma mark -

//-----------------------------------------------------------------------------
// the host calls this to inform the plugin that it wants to start using 
// a different audio stream format (sample rate, num channels, etc.)
OSStatus DfxPlugin::ChangeStreamFormat(AudioUnitScope inScope, AudioUnitElement inElement, 
									   CAStreamBasicDescription const& inPrevFormat, CAStreamBasicDescription const& inNewFormat)
{
//fprintf(stderr, "\nDfxPlugin::ChangeStreamFormat,   new sr = %.3lf,   old sr = %.3lf\n\n", inNewFormat.mSampleRate, inPrevFormat.mSampleRate);
//fprintf(stderr, "\nDfxPlugin::ChangeStreamFormat,   new num channels = %lu,   old num channels = %lu\n\n", inNewFormat.NumberChannels(), inPrevFormat.NumberChannels());
	AUChannelInfo const* auChannelConfigs = nullptr;
	auto const numIOconfigs = SupportedNumChannels(&auChannelConfigs);
	// if this AU supports only specific i/o channel count configs, 
	// then try to check whether the incoming format is allowed
	if ((numIOconfigs > 0) && auChannelConfigs)
	{
		auto const newNumChannels = static_cast<SInt16>(inNewFormat.NumberChannels());
		bool foundMatch = false;
		for (UInt32 i = 0; (i < numIOconfigs) && !foundMatch; i++)
		{
			switch (inScope)
			{
				case kAudioUnitScope_Input:
					if ((newNumChannels == auChannelConfigs[i].inChannels) || (auChannelConfigs[i].inChannels < 0))
					{
						foundMatch = true;
					}
					break;
				case kAudioUnitScope_Output:
					if ((newNumChannels == auChannelConfigs[i].outChannels) || (auChannelConfigs[i].outChannels < 0))
					{
						foundMatch = true;
					}
					break;
				// XXX input and output scopes together at once?
				case kAudioUnitScope_Global:
					if (((newNumChannels == auChannelConfigs[i].inChannels) || (auChannelConfigs[i].inChannels < 0))
							&& ((newNumChannels == auChannelConfigs[i].outChannels) || (auChannelConfigs[i].outChannels < 0)))
					{
						foundMatch = true;
					}
					break;
				default:
					return kAudioUnitErr_InvalidScope;
			}
		}
		// if the incoming channel count can't possibly work in 
		// any of the allowed channel configs, return an error
		if (!foundMatch)
		{
			return kAudioUnitErr_FormatNotSupported;
		}
	}

	// use the inherited base class implementation
	auto const status = TARGET_API_BASE_CLASS::ChangeStreamFormat(inScope, inElement, inPrevFormat, inNewFormat);
	if (status == noErr)
	{
		updatesamplerate();  // XXX do this here, or does it get caught elsewhere?
	}

	return status;
}

//-----------------------------------------------------------------------------
void DfxPlugin::UpdateInPlaceProcessingState()
{
#if !TARGET_PLUGIN_IS_INSTRUMENT
	// we can't do in-place audio rendering if there are different numbers of audio inputs and outputs
	if (getnuminputs() != getnumoutputs())
	{
		SetProcessesInPlace(false);
		PropertyChanged(kAudioUnitProperty_InPlaceProcessing, kAudioUnitScope_Global, AudioUnitElement(0));
	}
#endif
}

#if TARGET_PLUGIN_IS_INSTRUMENT
//-----------------------------------------------------------------------------
OSStatus DfxPlugin::Render(AudioUnitRenderActionFlags& ioActionFlags, 
						   AudioTimeStamp const& inTimeStamp, UInt32 inFramesToProcess)
{
	OSStatus status = noErr;

	// do any pre-DSP prep
	preprocessaudio();

	// get the output element
	auto const theOutput = GetOutput(0);  // throws if there's an error
	UInt32 numOutputBuffers = theOutput->GetBufferList().mNumberBuffers;
	// set up our more convenient audio stream pointers
	for (UInt32 i = 0; i < numOutputBuffers; i++)
	{
		mOutputAudioStreams_au[i] = theOutput->GetFloat32ChannelData(i);
	}

	// do stuff to prepare the audio inputs, if we use any
	if (getnuminputs() > 0)
	{
		if (!HasInput(0))
		{
			return kAudioUnitErr_NoConnection;
		}
		auto const theInput = GetInput(0);
		status = theInput->PullInput(ioActionFlags, inTimeStamp, AudioUnitElement(0), inFramesToProcess);
		if (status != noErr)
		{
			return status;
		}

		auto const numInputBuffers = theInput->GetBufferList().mNumberBuffers;
		// set up our more convenient audio stream pointers
		for (UInt32 i = 0; i < numInputBuffers; i++)
		{
			mInputAudioStreams_au[i] = theInput->GetFloat32ChannelData(i);
		}
	}

	// now do the processing
	processaudio(mInputAudioStreams_au.data(), mOutputAudioStreams_au.data(), inFramesToProcess);

	// I don't know what the hell this is for
	ioActionFlags &= ~kAudioUnitRenderAction_OutputIsSilence;

	// do any post-DSP stuff
	postprocessaudio();

	return status;
}

#else
// !TARGET_PLUGIN_IS_INSTRUMENT

//-----------------------------------------------------------------------------
// this is the audio processing routine
OSStatus DfxPlugin::ProcessBufferLists(AudioUnitRenderActionFlags& ioActionFlags, 
									   AudioBufferList const& inBuffer, AudioBufferList& outBuffer, 
									   UInt32 inFramesToProcess)
{
	OSStatus result = noErr;

	// do any pre-DSP prep
	preprocessaudio();

	// clear the output buffer because we will accumulate output into it
	if (mAudioProcessingAccumulatingOnly)
	{
		AUBufferList::ZeroBuffer(outBuffer);
	}

#if TARGET_PLUGIN_USES_DSPCORE
	// if the plugin uses DSP cores, then we just call the 
	// inherited base class implementation, which handles "Kernels"
	result = TARGET_API_BASE_CLASS::ProcessBufferLists(ioActionFlags, inBuffer, outBuffer, inFramesToProcess);

#else
	UInt32 numInputBuffers = inBuffer.mNumberBuffers;
	UInt32 numOutputBuffers = outBuffer.mNumberBuffers;

	// set up our more convenient audio stream pointers
	for (UInt32 i = 0; i < numInputBuffers; i++)
	{
		mInputAudioStreams_au[i] = static_cast<float const*>(inBuffer.mBuffers[i].mData);
	}
	for (UInt32 i = 0; i < numOutputBuffers; i++)
	{
		mOutputAudioStreams_au[i] = static_cast<float*>(outBuffer.mBuffers[i].mData);
	}

	// now do the processing
	processaudio(mInputAudioStreams_au.data(), mOutputAudioStreams_au.data(), inFramesToProcess);

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
OSStatus DfxPlugin::PrepareInstrument(MusicDeviceInstrumentID inInstrument)
{
	return kAudioUnitErr_PropertyNotInUse;
}

//-----------------------------------------------------------------------------
OSStatus DfxPlugin::ReleaseInstrument(MusicDeviceInstrumentID inInstrument)
{
	return kAudioUnitErr_PropertyNotInUse;
}

//-----------------------------------------------------------------------------
OSStatus DfxPlugin::StartNote(MusicDeviceInstrumentID inInstrument, 
							  MusicDeviceGroupID inGroupID, NoteInstanceID* outNoteInstanceID, 
							  UInt32 inOffsetSampleFrame, MusicDeviceNoteParams const& inParams)
{
	handlemidi_noteon(inGroupID, static_cast<int>(inParams.mPitch), static_cast<int>(inParams.mVelocity), inOffsetSampleFrame);
	if (outNoteInstanceID)
	{
		*outNoteInstanceID = static_cast<NoteInstanceID>(inParams.mPitch);
	}
	return noErr;
}

//-----------------------------------------------------------------------------
OSStatus DfxPlugin::StopNote(MusicDeviceGroupID inGroupID, 
							 NoteInstanceID inNoteInstanceID, UInt32 inOffsetSampleFrame)
{
	handlemidi_noteoff(inGroupID, inNoteInstanceID, 0, inOffsetSampleFrame);
	return noErr;
}
#endif
// TARGET_PLUGIN_IS_INSTRUMENT

#endif
// TARGET_PLUGIN_USES_MIDI
