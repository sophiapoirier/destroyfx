/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2022  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 2 of the License, or 
(at your option) any later version.

Destroy FX Library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Destroy FX Library.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org

Destroy FX is a sovereign entity comprised of Sophia Poirier and Tom Murphy 7.  
This is our class for E-Z plugin-making and E-Z multiple-API support.
This is where we connect the Audio Unit API to our DfxPlugin system.
------------------------------------------------------------------------*/

#include "dfxplugin.h"

#include <algorithm>
#include <AudioToolbox/AudioUnitUtilities.h>  // for kAUParameterListener_AnyParameter
#include <cassert>
#include <CoreServices/CoreServices.h>
#include <cstring>
#include <type_traits>

#include "dfx-au-utilities.h"
#include "dfxmath.h"
#include "dfxmisc.h"

#if TARGET_PLUGIN_HAS_GUI
	#include "dfxgui-auviewfactory.h"
#endif


static constexpr UInt32 kBaseClumpID = kAudioUnitClumpID_System + 1;



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

	// make host see that default preset is in effect
	if (getnumpresets() > 0)
	{
		postupdate_preset();
	}

	// make the global-scope element aware of the parameters' values
	// this must happen after AUBase::PostConstructor because the elements are created there
	for (dfx::ParameterID i = 0; i < getnumparameters(); i++)
	{
		if (!hasparameterattribute(i, DfxParam::kAttribute_Unused))  // XXX should we do it like this, or override GetParameterList?
		{
			TARGET_API_BASE_CLASS::SetParameter(i, kAudioUnitScope_Global, AudioUnitElement(0), getparameter_f(i), 0);
		}
	}

	// make sure that the input and output elements are initially set to a supported number of channels; 
	// if this plugin specifies supported I/O channel count configs and the current format is not supported,  
	// then set the format to the first supported I/O pair in our list
	if (!mChannelConfigs.empty() && !ischannelcountsupported(getnuminputs(), getnumoutputs()))
	{
		constexpr UInt32 defaultNumChannels = 2;
		auto const channelConfig = mChannelConfigs.front();
		// change the input channel count to the first supported one listed
		if (Inputs().GetNumberOfElements() > 0)
		{
			auto const newInNumChannels = (channelConfig.inChannels < 0) ? defaultNumChannels : static_cast<UInt32>(channelConfig.inChannels);
			auto const& curInStreamFormat = GetStreamFormat(kAudioUnitScope_Input, AudioUnitElement(0));
			auto const newInStreamFormat = ausdk::ASBD::CreateCommonFloat32(curInStreamFormat.mSampleRate, newInNumChannels);
			TARGET_API_BASE_CLASS::ChangeStreamFormat(kAudioUnitScope_Input, AudioUnitElement(0), curInStreamFormat, newInStreamFormat);
		}
		// change the output channel count to the first supported one listed
		auto const newOutNumChannels = (channelConfig.outChannels < 0) ? defaultNumChannels : static_cast<UInt32>(channelConfig.outChannels);
		auto const& curOutStreamFormat = GetStreamFormat(kAudioUnitScope_Output, AudioUnitElement(0));
		auto const newOutStreamFormat = ausdk::ASBD::CreateCommonFloat32(curOutStreamFormat.mSampleRate, newOutNumChannels);
		TARGET_API_BASE_CLASS::ChangeStreamFormat(kAudioUnitScope_Output, AudioUnitElement(0), curOutStreamFormat, newOutStreamFormat);
	}

	UpdateInPlaceProcessingState();
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
#if TARGET_PLUGIN_IS_INSTRUMENT
	// if this AU supports only specific I/O channel count configs, then check whether the current format is allowed
	AUSDK_Require(ischannelcountsupported(getnuminputs(), getnumoutputs()), kAudioUnitErr_FormatNotSupported);
#endif

#if TARGET_PLUGIN_USES_DSPCORE
	cacheDSPCoreParameterValues();
#endif

	auto const status = TARGET_API_BASE_CLASS::Initialize();

	if (status == noErr)
	{
		try
		{
			do_initialize();
			UpdateInPlaceProcessingState();
		}
		catch (...)
		{
			return kAudioUnitErr_FailedInitialization;
		}
	}

	return status;
}

//-----------------------------------------------------------------------------
// this is the "destructor" partner to Initialize
// any DSP-specific resources should be released here
void DfxPlugin::Cleanup()
{
	do_cleanup();

	TARGET_API_BASE_CLASS::Cleanup();
}

//-----------------------------------------------------------------------------
// this is called when an audio stream timeline is broken somehow 
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
									UInt32& outDataSize, bool& outWritable)
{
	OSStatus status = noErr;

	switch (inPropertyID)
	{
		case kAudioUnitProperty_CocoaUI:
#if TARGET_PLUGIN_HAS_GUI
			outDataSize = sizeof(AudioUnitCocoaViewInfo);
			outWritable = false;
			status = noErr;
#else
			status = kAudioUnitErr_PropertyNotInUse;
#endif
			break;

		case kAudioUnitProperty_ParameterIDName:
			outDataSize = sizeof(AudioUnitParameterIDName);
			outWritable = false;
			status = noErr;
			break;

		case kAudioUnitProperty_AUHostIdentifier:
			outDataSize = sizeof(AUHostVersionIdentifier);
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

		case dfx::kPluginProperty_ParameterUseValueStrings:
			outDataSize = sizeof(Boolean);
			outWritable = false;
			break;

		case dfx::kPluginProperty_ParameterAttributes:
			outDataSize = sizeof(DfxParam::Attribute);
			outWritable = false;
			break;

		case dfx::kPluginProperty_ParameterGroup:
			outDataSize = sizeof(size_t);
			outWritable = false;
			break;

		case dfx::kPluginProperty_ParameterGroupName:
			outDataSize = dfx::kParameterGroupStringMaxLength;
			outWritable = false;
			break;

		// randomize the parameters
		case dfx::kPluginProperty_RandomizeParameter:
			// when you "set" this "property", you send a boolean to say whether or not to write automation data
			// (XXX but now it is ignored...)
			outDataSize = sizeof(Boolean);
			outWritable = true;
			break;

		case dfx::kPluginProperty_SmoothedAudioValueTime:
			outDataSize = sizeof(double);
			outWritable = true;
			break;

	#if DEBUG
		case dfx::kPluginProperty_DfxPluginInstance:
			outDataSize = sizeof(this);
			outWritable = false;
			break;
	#endif

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
			outDataSize = sizeof(uint32_t);
			outWritable = true;
			break;
		// get/set the MIDI assignment for a parameter
		case dfx::kPluginProperty_ParameterMidiAssignment:
			outDataSize = sizeof(dfx::ParameterAssignment);
			outWritable = true;
			break;
		case dfx::kPluginProperty_MidiAssignmentsUseChannel:
			outDataSize = sizeof(Boolean);
			outWritable = true;
			break;
		case dfx::kPluginProperty_MidiAssignmentsSteal:
			outDataSize = sizeof(Boolean);
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
					outDataSize = static_cast<UInt32>(dfxDataSize);
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

#if LOGIC_AU_PROPERTIES_AVAILABLE
template <typename T>
requires std::is_scalar_v<T>
consteval auto DFX_EndianModeForScalarType()
{
	if constexpr (sizeof(T) == sizeof(uint32_t))
	{
		return kLogicAUNodePropertyEndianMode_All32Bits;
	}
	else if constexpr (sizeof(T) == sizeof(uint64_t))
	{
		return kLogicAUNodePropertyEndianMode_All64Bits;
	}
}
#endif

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
#if TARGET_PLUGIN_HAS_GUI
		case kAudioUnitProperty_CocoaUI:
		{
			auto const pluginBundle = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));
			assert(pluginBundle);
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

		case kAudioUnitProperty_ParameterIDName:
		{
			auto& parameterIDName = *static_cast<AudioUnitParameterIDName*>(outData);
			auto const parameterID = parameterIDName.inID;
			if (inScope != kAudioUnitScope_Global)
			{
				status = kAudioUnitErr_InvalidScope;
			}
			else if (inElement != 0)
			{
				status = kAudioUnitErr_InvalidElement;
			}
			else if (parameterIDName.inDesiredLength == kAudioUnitParameterName_Full)
			{
				parameterIDName.outName = getparametercfname(parameterID);
				CFRetain(parameterIDName.outName);
			}
			else if (parameterIDName.inDesiredLength <= 0)
			{
				status = kAudio_ParamError;
			}
			else
			{
				auto const shortName = getparametername(parameterID, static_cast<size_t>(parameterIDName.inDesiredLength));
				parameterIDName.outName = CFStringCreateWithCString(kCFAllocatorDefault, shortName.c_str(), DfxParam::kDefaultCStringEncoding);
			}
			break;
		}

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
				pvt->auValue = expandparametervalue(pvt->otherParamID, pvt->otherValue);
			}
			else
			{
				status = kAudioUnitErr_InvalidPropertyValue;
			}
			break;
		}

		// get parameter values (current, min, max, etc.) using specific variable types
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
			dfx::ParameterID const parameterID = inElement;
			switch (request->inValueItem)
			{
				case dfx::ParameterValueItem::Current:
				{
					switch (request->inValueType)
					{
						case DfxParam::ValueType::Float:
							value.f = getparameter_f(parameterID);
							break;
						case DfxParam::ValueType::Int:
							value.i = getparameter_i(parameterID);
							break;
						case DfxParam::ValueType::Boolean:
							value.b = getparameter_b(parameterID);
							break;
						default:
							assert(false);
							status = kAudio_ParamError;
							break;
					}
					break;
				}
				case dfx::ParameterValueItem::Default:
				{
					switch (request->inValueType)
					{
						case DfxParam::ValueType::Float:
							value.f = getparameterdefault_f(parameterID);
							break;
						case DfxParam::ValueType::Int:
//							value.i = getparameterdefault_i(parameterID);
							break;
						case DfxParam::ValueType::Boolean:
//							value.b = getparameterdefault_b(parameterID);
							break;
						default:
							assert(false);
							status = kAudio_ParamError;
							break;
					}
					break;
				}
				case dfx::ParameterValueItem::Min:
				{
					switch (request->inValueType)
					{
						case DfxParam::ValueType::Float:
							value.f = getparametermin_f(parameterID);
							break;
						case DfxParam::ValueType::Int:
							value.i = getparametermin_i(parameterID);
							break;
						case DfxParam::ValueType::Boolean:
//							value.b = getparametermin_b(parameterID);
							value.b = false;
							break;
						default:
							assert(false);
							status = kAudio_ParamError;
							break;
					}
					break;
				}
				case dfx::ParameterValueItem::Max:
				{
					switch (request->inValueType)
					{
						case DfxParam::ValueType::Float:
							value.f = getparametermax_f(parameterID);
							break;
						case DfxParam::ValueType::Int:
							value.i = getparametermax_i(parameterID);
							break;
						case DfxParam::ValueType::Boolean:
//							value.b = getparametermax_b(parameterID);
							value.b = true;
							break;
						default:
							assert(false);
							status = kAudio_ParamError;
							break;
					}
					break;
				}
				default:
					assert(false);
					status = kAudio_ParamError;
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
				dfx::ReverseBytes(request->inValue);
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
					status = kAudio_ParamError;
					break;
			}
		#if LOGIC_AU_PROPERTIES_AVAILABLE
			if (isLogicNodeEndianReversed())
			{
				dfx::ReverseBytes(request->outValue);
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
			if (auto const valueString = getparametervaluestring(inElement, request->inStringIndex))
			{
				strlcpy(request->valueString, valueString->c_str(), std::size(request->valueString));
			}
			else
			{
				status = kAudio_ParamError;
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
				strlcpy(static_cast<char*>(outData), getparameterunitstring(inElement).c_str(), dfx::kParameterUnitStringMaxLength);
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

		case dfx::kPluginProperty_ParameterUseValueStrings:
			if (!parameterisvalid(inElement))
			{
				status = kAudioUnitErr_InvalidParameter;
			}
			else
			{
				*static_cast<Boolean*>(outData) = getparameterusevaluestrings(inElement);
			}
			break;

		case dfx::kPluginProperty_ParameterAttributes:
			if (!parameterisvalid(inElement))
			{
				status = kAudioUnitErr_InvalidParameter;
			}
			else
			{
				*static_cast<DfxParam::Attribute*>(outData) = getparameterattributes(inElement);
			}
			break;

		case dfx::kPluginProperty_ParameterGroup:
			if (!parameterisvalid(inElement))
			{
				status = kAudioUnitErr_InvalidParameter;
			}
			else
			{
				if (auto const groupIndex = getparametergroup(inElement))
				{
					*static_cast<size_t*>(outData) = *groupIndex;
				}
				else
				{
					status = kAudioUnitErr_PropertyNotInUse;
				}
			}
			break;

		case dfx::kPluginProperty_ParameterGroupName:
			if (auto const groupName = getparametergroupname(inElement); !groupName.empty())
			{
				strlcpy(static_cast<char*>(outData), groupName.c_str(), dfx::kParameterGroupStringMaxLength);
			}
			else
			{
				status = kAudioUnitErr_InvalidPropertyValue;
			}
			break;

		case dfx::kPluginProperty_SmoothedAudioValueTime:
			if (auto const value = getSmoothedAudioValueTime())
			{
				*static_cast<double*>(outData) = *value;
			}
			else
			{
				status = kAudioUnitErr_PropertyNotInUse;
			}
			break;

	#if DEBUG
		case dfx::kPluginProperty_DfxPluginInstance:
			*static_cast<DfxPlugin**>(outData) = this;
			break;
	#endif

	#if TARGET_PLUGIN_USES_MIDI
		// get the MIDI learn state
		case dfx::kPluginProperty_MidiLearn:
			*static_cast<Boolean*>(outData) = getmidilearning();
			break;
		// get the current MIDI learner parameter
		case dfx::kPluginProperty_MidiLearner:
			*static_cast<uint32_t*>(outData) = getmidilearner();
			break;
		// get the MIDI assignment for a parameter
		case dfx::kPluginProperty_ParameterMidiAssignment:
			*static_cast<dfx::ParameterAssignment*>(outData) = getparametermidiassignment(inElement);
			break;
		case dfx::kPluginProperty_MidiAssignmentsUseChannel:
			*static_cast<Boolean*>(outData) = getMidiAssignmentsUseChannel();
			break;
		case dfx::kPluginProperty_MidiAssignmentsSteal:
			*static_cast<Boolean*>(outData) = getMidiAssignmentsSteal();
			break;
	#endif

	#if LOGIC_AU_PROPERTIES_AVAILABLE
		case kLogicAUProperty_NodeOperationMode:
			*static_cast<UInt32*>(outData) = getSupportedLogicNodeOperationMode();
			break;

		case kLogicAUProperty_NodePropertyDescriptions:
		{
			auto const nodePropertyDescs = static_cast<LogicAUNodePropertyDescription*>(outData);
			for (size_t i = 0; i < dfx_GetNumPluginProperties(); i++)
			{
				auto& nodePropertyDesc = nodePropertyDescs[i];
				nodePropertyDesc.mPropertyID = dfx::kPluginProperty_StartID + i;
				nodePropertyDesc.mEndianMode = kLogicAUNodePropertyEndianMode_DontTouch;
				nodePropertyDesc.mFlags = kLogicAUNodePropertyFlag_Synchronous | kLogicAUNodePropertyFlag_NeedsInitialization;
				switch (nodePropertyDesc.mPropertyID)
				{
					case dfx::kPluginProperty_ParameterValue:
						nodePropertyDesc.mFlags |= kLogicAUNodePropertyFlag_FullRoundTrip;
						break;
					case dfx::kPluginProperty_ParameterValueConversion:
						nodePropertyDesc.mFlags |= kLogicAUNodePropertyFlag_FullRoundTrip;
						break;
					case dfx::kPluginProperty_ParameterValueString:
						nodePropertyDesc.mFlags |= kLogicAUNodePropertyFlag_FullRoundTrip;
						break;
					case dfx::kPluginProperty_ParameterValueType:
					case dfx::kPluginProperty_ParameterUnit:
						nodePropertyDesc.mEndianMode = kLogicAUNodePropertyEndianMode_All32Bits;
						nodePropertyDesc.mFlags |= kLogicAUNodePropertyFlag_FullRoundTrip;
						break;
					case dfx::kPluginProperty_ParameterAttributes;
						nodePropertyDesc.mEndianMode = kLogicAUNodePropertyEndianMode_All32Bits;
						break;
					case dfx::kPluginProperty_ParameterGroup:
						nodePropertyDesc.mEndianMode = DFX_EndianModeForScalarType<size_t>();
						break;
					case dfx::kPluginProperty_SmoothedAudioValueTime:
						nodePropertyDesc.mEndianMode = kLogicAUNodePropertyEndianMode_All64Bits;
						break;
					case dfx::kPluginProperty_MidiLearner:
						nodePropertyDesc.mEndianMode = kLogicAUNodePropertyEndianMode_All32Bits;
						break;
					case dfx::kPluginProperty_ParameterMidiAssignment:
						nodePropertyDesc.mEndianMode = kLogicAUNodePropertyEndianMode_All32Bits;
						break;
					default:
					{
						size_t dfxDataSize {};
						dfx::PropertyFlags dfxFlags {};
						auto const dfxStatus = dfx_GetPropertyInfo(nodePropertyDesc.mPropertyID, dfx::kScope_Global, 0, dfxDataSize, dfxFlags);
						if (dfxStatus == dfx::kStatus_NoError)
						{
							if (dfxFlags & dfx::kPropertyFlag_BiDirectional)
							{
								nodePropertyDesc.mFlags |= kLogicAUNodePropertyFlag_FullRoundTrip;
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
			auto const hostID = static_cast<AUHostVersionIdentifier const*>(inData);
			if (!hostID->hostName)
			{
				status = kAudioUnitErr_InvalidPropertyValue;
			}
			else
			{
#if 0
				fprintf(stderr, "\tSetProperty(AUHostIdentifier)\n");
				CFShow(hostID->hostName);
				fprintf(stderr, "version: 0x%X\n", hostID->hostVersion);
#endif
			}
			break;
		}

	#if !TARGET_PLUGIN_IS_INSTRUMENT
		case kAudioUnitProperty_InPlaceProcessing:
			if ((!mInPlaceAudioProcessingAllowed || (getnuminputs() != getnumoutputs())) 
				&& (*static_cast<UInt32 const*>(inData) != 0))
			{
				status = kAudioUnitErr_CannotDoInCurrentContext;
			}
			else
			{
				status = TARGET_API_BASE_CLASS::SetProperty(inPropertyID, inScope, inElement, inData, inDataSize);
			}
			break;
	#endif

		// set parameter values (current, min, max, etc.) using specific variable types
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
			dfx::ParameterID const parameterID = inElement;
			switch (request->inValueItem)
			{
				case dfx::ParameterValueItem::Current:
				{
					switch (request->inValueType)
					{
						case DfxParam::ValueType::Float:
							setparameter_f(parameterID, value.f);
							postupdate_parameter(parameterID);
							break;
						case DfxParam::ValueType::Int:
							setparameter_i(parameterID, value.i);
							postupdate_parameter(parameterID);
							break;
						case DfxParam::ValueType::Boolean:
							setparameter_b(parameterID, value.b);
							postupdate_parameter(parameterID);
							break;
						default:
							assert(false);
							status = kAudio_ParamError;
							break;
					}
					break;
				}
				case dfx::ParameterValueItem::Default:
				case dfx::ParameterValueItem::Min:
				case dfx::ParameterValueItem::Max:
					assert(false);
					status = kAudioUnitErr_PropertyNotWritable;
					break;
				default:
					assert(false);
					status = kAudio_ParamError;
					break;
			}
			break;
		}

		// set parameter value strings (WHY???)
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
				status = kAudio_ParamError;
			}
			break;
		}

		// randomize the parameters
		case dfx::kPluginProperty_RandomizeParameter:
			// when you "set" this "property", you send a bool to say whether or not to write automation data
			// (XXX but now it is ignored...)
			if (inElement == kAUParameterListener_AnyParameter)
			{
				randomizeparameters();
			}
			else
			{
				randomizeparameter(inElement);
			}
			break;

		case dfx::kPluginProperty_SmoothedAudioValueTime:
			if (mSmoothedAudioValues.empty())
			{
				status = kAudioUnitErr_PropertyNotInUse;
			}
			else
			{
				setSmoothedAudioValueTime(*static_cast<double const*>(inData));
			}
			break;

	#if TARGET_PLUGIN_USES_MIDI
		// set the MIDI learn state
		case dfx::kPluginProperty_MidiLearn:
			setmidilearning(*static_cast<Boolean const*>(inData));
			break;
		// clear MIDI parameter assignments
		case dfx::kPluginProperty_ResetMidiLearn:
			// you don't need an input value for this property
			resetmidilearn();
			break;
		// set the current MIDI learner parameter
		case dfx::kPluginProperty_MidiLearner:
			setmidilearner(*static_cast<uint32_t const*>(inData));
			break;
		// set the MIDI assignment for a parameter
		case dfx::kPluginProperty_ParameterMidiAssignment:
			setparametermidiassignment(inElement, *static_cast<dfx::ParameterAssignment const*>(inData));
			break;
		case dfx::kPluginProperty_MidiAssignmentsUseChannel:
			setMidiAssignmentsUseChannel(*static_cast<Boolean const*>(inData));
			break;
		case dfx::kPluginProperty_MidiAssignmentsSteal:
			setMidiAssignmentsSteal(*static_cast<Boolean const*>(inData));
			break;
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
void DfxPlugin::PropertyChanged(AudioUnitPropertyID inPropertyID, 
								AudioUnitScope inScope, AudioUnitElement inElement)
{
#if TARGET_PLUGIN_USES_MIDI
	if (isrenderthread())
	{
		if (inPropertyID == dfx::kPluginProperty_MidiLearn)
		{
			mMidiLearnChangedInProcessHasPosted.clear(std::memory_order_relaxed);
			return;
		}
		if (inPropertyID == dfx::kPluginProperty_MidiLearner)
		{
			mMidiLearnerChangedInProcessHasPosted.clear(std::memory_order_relaxed);
			return;
		}
	}
#endif

	// NOTE: this will bite you if running debug builds in hosts that offline render audio (hence the validator exception)
	// NOTE: exception for Connection property because AUGraph seems to sometimes disconnect from the audio I/O thread
	assert(!isrenderthread() || (inPropertyID == kAudioUnitProperty_MakeConnection) || dfx::IsHostValidator());  // this method is not realtime-safe

	return TARGET_API_BASE_CLASS::PropertyChanged(inPropertyID, inScope, inElement);
}

//-----------------------------------------------------------------------------
// give the host an array of the audio input/output channel configurations 
// that the plugin supports
// if the pointer passed in is null, then simply return the number of supported configurations
// if any n-to-n configuration (i.e. same number of ins and outs) is supported, return 0
UInt32 DfxPlugin::SupportedNumChannels(AUChannelInfo const** outInfo)
{
	if (!mChannelConfigs.empty() && outInfo)
	{
		*outInfo = mChannelConfigs.data();
	}
	return static_cast<UInt32>(mChannelConfigs.size());
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
	assert(pluginBundle);
	if (pluginBundle)
	{
		return CFBundleCopyResourceURL(pluginBundle, CFSTR(PLUGIN_ICON_FILE_NAME), nullptr, nullptr);
	}
#endif

	return nullptr;
}



#pragma mark -
#pragma mark parameters
#pragma mark -

//-----------------------------------------------------------------------------
// get specific information about the properties of a parameter
OSStatus DfxPlugin::GetParameterInfo(AudioUnitScope inScope, 
									 AudioUnitParameterID inParameterID, 
									 AudioUnitParameterInfo& outParameterInfo)
{
	AUSDK_Require(inScope == kAudioUnitScope_Global, kAudioUnitErr_InvalidScope);
	AUSDK_Require(parameterisvalid(inParameterID), kAudioUnitErr_InvalidParameter);

	// then make sure to only copy as much as the ParameterInfo name C string can hold
	strlcpy(outParameterInfo.name, getparametername(inParameterID).c_str(), std::size(outParameterInfo.name));
	// in case the parameter name was dfx::kParameterNameMaxLength or longer, 
	// make sure that the ParameterInfo name string is nul-terminated
	outParameterInfo.name[sizeof(outParameterInfo.name) - 1] = '\0';
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
	// check if the parameter is used or not (workaround for plugin formats that require indexed parameters)
	if (hasparameterattribute(inParameterID, DfxParam::kAttribute_Unused))
	{
		return kAudioUnitErr_InvalidParameter;
	}
	// if the parameter is hidden, then indicate that it's not readable or writable...
	else if (hasparameterattribute(inParameterID, DfxParam::kAttribute_Hidden))
	{
		outParameterInfo.flags = kAudioUnitParameterFlag_ExpertMode;
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
				outParameterInfo.unitName = CFStringCreateWithCString(kCFAllocatorDefault, getparameterunitstring(inParameterID).c_str(), DfxParam::kDefaultCStringEncoding);
				break;
			}

			case DfxParam::Unit::Generic:
				// if we got to this point, try using the value type to determine the unit type
				switch (getparametervaluetype(inParameterID))
				{
					case DfxParam::ValueType::Float:
						outParameterInfo.unit = kAudioUnitParameterUnit_Generic;
						break;
					case DfxParam::ValueType::Boolean:
						outParameterInfo.unit = kAudioUnitParameterUnit_Boolean;
						break;
					case DfxParam::ValueType::Int:
						outParameterInfo.unit = kAudioUnitParameterUnit_Indexed;
						break;
				}
		}
	}

	if (auto const groupIndex = getparametergroup(inParameterID))
	{
		HasClump(outParameterInfo, kBaseClumpID + static_cast<UInt32>(*groupIndex));
	}


	return noErr;
}

//-----------------------------------------------------------------------------
// give the host an array of CFStrings with the display values for an indexed parameter
OSStatus DfxPlugin::GetParameterValueStrings(AudioUnitScope inScope, AudioUnitParameterID inParameterID,  
											 CFArrayRef* outStrings)
{
	AUSDK_Require(inScope == kAudioUnitScope_Global, kAudioUnitErr_InvalidScope);
	AUSDK_Require(parameterisvalid(inParameterID), kAudioUnitErr_InvalidParameter);

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
		auto array = dfx::MakeUniqueCFType(CFArrayCreateMutable(kCFAllocatorDefault, numStrings, &kCFTypeArrayCallBacks));
		if (!array)
		{
			return coreFoundationUnknownErr;
		}
		for (long i = 0; i < numStrings; i++)
		{
			auto const paramValueString = getparametervaluecfstring(inParameterID, i);
			// if we don't actually have strings set, exit with an error
			AUSDK_Require(paramValueString != nullptr, kAudioUnitErr_InvalidPropertyValue);
			CFArrayAppendValue(array.get(), paramValueString);
		}
		*outStrings = array.release();
		return noErr;
	}

	return kAudioUnitErr_InvalidPropertyValue;
}

//-----------------------------------------------------------------------------
OSStatus DfxPlugin::CopyClumpName(AudioUnitScope inScope, UInt32 inClumpID, 
								  UInt32 inDesiredNameLength, CFStringRef* outClumpName)
{
	*outClumpName = nullptr;
	if (inClumpID < kBaseClumpID)
	{
		return TARGET_API_BASE_CLASS::CopyClumpName(inScope, inClumpID, inDesiredNameLength, outClumpName);
	}
	AUSDK_Require(inScope == kAudioUnitScope_Global, kAudioUnitErr_InvalidScope);

	auto const clumpName = getparametergroupname(inClumpID - kBaseClumpID);
	AUSDK_Require(!clumpName.empty(), kAudioUnitErr_InvalidPropertyValue);
	*outClumpName = CFStringCreateWithCString(kCFAllocatorDefault, clumpName.c_str(), DfxParam::kDefaultCStringEncoding);
	return noErr;
}

//-----------------------------------------------------------------------------
OSStatus DfxPlugin::SetParameter(AudioUnitParameterID inParameterID, 
								 AudioUnitScope inScope, AudioUnitElement inElement, 
								 Float32 inValue, UInt32 /*inBufferOffsetInFrames*/)
{
	AUSDK_Require(inScope == kAudioUnitScope_Global, kAudioUnitErr_InvalidScope);
	AUSDK_Require(inElement == 0, kAudioUnitErr_InvalidElement);
	AUSDK_Require(parameterisvalid(inParameterID), kAudioUnitErr_InvalidParameter);

	setparameter_f(inParameterID, inValue);
	return noErr;
//	return TARGET_API_BASE_CLASS::SetParameter(inParameterID, inScope, inElement, inValue, inBufferOffsetInFrames);
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
	CFIndex validNumPresets = 0;
	for (size_t i = 0; i < getnumpresets(); i++)
	{
//		if (presetnameisvalid(i))
		if (!mPresets[i].getname().empty())
		{
			validNumPresets++;
		}
	}
	// whoops, looks like we don't actually have any presets
	AUSDK_Require(validNumPresets > 0, kAudioUnitErr_PropertyNotInUse);

	// this is just to say that the property is supported (GetPropertyInfo needs this)
	if (!outData)
	{
		return noErr;
	}

	// allocate a mutable array large enough to hold them all
	auto const presetsArray = CFArrayCreateMutable(kCFAllocatorDefault, validNumPresets, &kCFAUPresetArrayCallBacks);
	if (!presetsArray)
	{
		*outData = nullptr;
		return coreFoundationUnknownErr;
	}

	// add the preset data (name and number) into the array
	for (size_t i = 0; i < getnumpresets(); i++)
	{
		if (presetnameisvalid(i))
		{
			auto const aupreset = dfx::MakeUniqueCFAUPreset(kCFAllocatorDefault, static_cast<SInt32>(i), getpresetcfname(i));
			if (aupreset)
			{
				// insert the AUPreset into the output array
				CFArrayAppendValue(presetsArray, aupreset.get());
			}
		}
	}

	*outData = reinterpret_cast<CFArrayRef>(presetsArray);

	return noErr;
}

//-----------------------------------------------------------------------------
// this is called as a request to load a preset
OSStatus DfxPlugin::NewFactoryPresetSet(AUPreset const& inNewFactoryPreset)
{
	AUSDK_Require(inNewFactoryPreset.presetNumber >= 0, kAudioUnitErr_InvalidPropertyValue);
	auto const presetIndex = dfx::math::ToIndex(inNewFactoryPreset.presetNumber);

	AUSDK_Require(presetisvalid(presetIndex), kAudioUnitErr_InvalidPropertyValue);
	// for AU, we are using invalid preset names as a way of saying "not a real preset," 
	// even though it might be a valid (allocated) preset number
	AUSDK_Require(presetnameisvalid(presetIndex), kAudioUnitErr_InvalidPropertyValue);

	// try to load the preset
	AUSDK_Require(loadpreset(presetIndex), kAudioUnitErr_InvalidPropertyValue);

	return noErr;
}



#pragma mark -
#pragma mark state
#pragma mark -

//-----------------------------------------------------------------------------
// stores the values of all parameters values, state info, etc. into a CFPropertyListRef
OSStatus DfxPlugin::SaveState(CFPropertyListRef* outData)
{
	AUSDK_Require_noerr(TARGET_API_BASE_CLASS::SaveState(outData));

#if TARGET_PLUGIN_USES_MIDI
	// fetch our special data
	if (auto const dfxData = mDfxSettings->save(true); !dfxData.empty())
	{
		// create a CF data storage thingy filled with our special data
		auto const cfData = dfx::MakeUniqueCFType(CFDataCreate(kCFAllocatorDefault, 
															   reinterpret_cast<const UInt8*>(dfxData.data()), 
															   static_cast<CFIndex>(dfxData.size())));
		if (cfData)
		{
			// put the CF data storage thingy into the dfx-data section of the CF dictionary
			auto const dict = const_cast<CFMutableDictionaryRef>(reinterpret_cast<CFDictionaryRef>(*outData));
			CFDictionarySetValue(dict, DfxSettings::kDfxDataAUClassInfoKeyString, cfData.get());
		}
	}
#endif

	return noErr;
}

//-----------------------------------------------------------------------------
// restores all parameter values, state info, etc. from the CFPropertyListRef
OSStatus DfxPlugin::RestoreState(CFPropertyListRef inData)
{
	AUSDK_Require_noerr(TARGET_API_BASE_CLASS::RestoreState(inData));

#if TARGET_PLUGIN_USES_MIDI
	// look for our custom data section
	auto const dict = reinterpret_cast<CFDictionaryRef>(inData);
	auto cfData = static_cast<CFDataRef>(CFDictionaryGetValue(dict, DfxSettings::kDfxDataAUClassInfoKeyString));

	// failed to find our custom data, but maybe old VST chunk data was provided...
	if (!cfData)
	{
		// TODO: this implementation may be incorrect and the data may be the entire fxProgram, not just the chunk
		cfData = static_cast<CFDataRef>(CFDictionaryGetValue(dict, CFSTR(kAUPresetVSTPresetKey)));
	}

	bool success = false;
	if (cfData && (CFGetTypeID(cfData) == CFDataGetTypeID()))
	{
		auto const dfxData = CFDataGetBytePtr(cfData);
		auto const dfxDataSize = static_cast<size_t>(CFDataGetLength(cfData));
		success = mDfxSettings->restore(dfxData, dfxDataSize, true);
		//AUSDK_Require(success, kAudioUnitErr_InvalidPropertyValue);
	}
	if (!success)
	{
#endif  // TARGET_PLUGIN_USES_MIDI

// XXX should we rethink this and load parameter settings always before mDfxSettings->restore()?
	// load the parameter settings that were restored 
	// by the inherited base class implementation of RestoreState
	for (dfx::ParameterID i = 0; i < getnumparameters(); i++)
	{
		setparameter_f(i, Globals()->GetParameter(i));
	}

#if TARGET_PLUGIN_USES_MIDI
	}
#endif

	// make any listeners aware of the changes in the parameter values
	for (dfx::ParameterID i = 0; i < getnumparameters(); i++)
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
									   AudioStreamBasicDescription const& inPrevFormat, 
									   AudioStreamBasicDescription const& inNewFormat)
{
//fprintf(stderr, "\nDfxPlugin::ChangeStreamFormat, new sr = %.3lf, old sr = %.3lf\n\n", inNewFormat.mSampleRate, inPrevFormat.mSampleRate);
//fprintf(stderr, "\nDfxPlugin::ChangeStreamFormat, new num channels = %lu, old num channels = %lu\n\n", inNewFormat.mChannelsPerFrame, inPrevFormat.mChannelsPerFrame);
	// if this AU supports only specific I/O channel count configs, 
	// then check whether the incoming format is allowed
	if (!mChannelConfigs.empty())
	{
		auto const newNumChannels = static_cast<SInt16>(inNewFormat.mChannelsPerFrame);
		bool foundMatch = false;
		for (size_t i = 0; (i < mChannelConfigs.size()) && !foundMatch; i++)
		{
			switch (inScope)
			{
				case kAudioUnitScope_Input:
					if ((newNumChannels == mChannelConfigs[i].inChannels) || (mChannelConfigs[i].inChannels < 0))
					{
						foundMatch = true;
					}
					break;
				case kAudioUnitScope_Output:
					if ((newNumChannels == mChannelConfigs[i].outChannels) || (mChannelConfigs[i].outChannels < 0))
					{
						foundMatch = true;
					}
					break;
				// XXX input and output scopes together at once?
				case kAudioUnitScope_Global:
					if (ischannelcountsupported(inNewFormat.mChannelsPerFrame, inNewFormat.mChannelsPerFrame))
					{
						foundMatch = true;
					}
					break;
				default:
					return kAudioUnitErr_InvalidScope;
			}
		}
		// fail if the incoming channel count cannot work in any of the allowed channel configs
		AUSDK_Require(foundMatch, kAudioUnitErr_FormatNotSupported);
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
	// or the audio rendering is summed into the output
	if ((getnuminputs() != getnumoutputs()) || !mInPlaceAudioProcessingAllowed)
	{
		auto const entryProcessesInPlace = ProcessesInPlace();
		SetProcessesInPlace(false);
		if (ProcessesInPlace() != entryProcessesInPlace)
		{
			PropertyChanged(kAudioUnitProperty_InPlaceProcessing, kAudioUnitScope_Global, AudioUnitElement(0));
		}
	}
#endif
}

#if TARGET_PLUGIN_IS_INSTRUMENT
//-----------------------------------------------------------------------------
OSStatus DfxPlugin::Render(AudioUnitRenderActionFlags& ioActionFlags, 
						   AudioTimeStamp const& inTimeStamp, UInt32 inFramesToProcess)
{
	// do any pre-DSP prep
	preprocessaudio(inFramesToProcess);

	// get the output element
	auto& theOutput = Output(0);  // throws if there's an error
	auto const numOutputBuffers = theOutput.GetBufferList().mNumberBuffers;
	// set up our more convenient audio stream pointers
	for (UInt32 i = 0; i < numOutputBuffers; i++)
	{
		mOutputAudioStreams_au[i] = theOutput.GetFloat32ChannelData(i);
	}

	// do stuff to prepare the audio inputs, if we use any
	if (getnuminputs() > 0)
	{
		AUSDK_Require(HasInput(0), kAudioUnitErr_NoConnection);
		auto& theInput = Input(0);
		AUSDK_Require_noerr(theInput.PullInput(ioActionFlags, inTimeStamp, AudioUnitElement(0), inFramesToProcess));

		auto const numInputBuffers = theInput.GetBufferList().mNumberBuffers;
		// set up our more convenient audio stream pointers
		for (UInt32 i = 0; i < numInputBuffers; i++)
		{
			mInputAudioStreams_au[i] = theInput.GetFloat32ChannelData(i);
		}
	}

	// now do the processing
	processaudio(mInputAudioStreams_au.data(), mOutputAudioStreams_au.data(), inFramesToProcess);

	// TODO: actually handle the silence flag
	ioActionFlags &= ~kAudioUnitRenderAction_OutputIsSilence;

	// do any post-DSP stuff
	postprocessaudio();

	return noErr;
}

#else  // !TARGET_PLUGIN_IS_INSTRUMENT

//-----------------------------------------------------------------------------
// this is the audio processing routine
OSStatus DfxPlugin::ProcessBufferLists(AudioUnitRenderActionFlags& ioActionFlags, 
									   AudioBufferList const& inBuffer, AudioBufferList& outBuffer, 
									   UInt32 inFramesToProcess)
{
	OSStatus status = noErr;

	// do any pre-DSP prep
	preprocessaudio(inFramesToProcess);

	// clear the output buffer because we will accumulate output into it
	if (!mInPlaceAudioProcessingAllowed)
	{
		ausdk::AUBufferList::ZeroBuffer(outBuffer);
	}

#if TARGET_PLUGIN_USES_DSPCORE
	AudioBufferList const* inputBufferPtr = &inBuffer;
	// if in special fan-out kernel mode, copy the first input audio channel into all of 
	// our input copy audio buffers, so that each kernel gets its own input channel buffer 
	// (averting any clobbering by one channel's render if processing in-place)
	if (asymmetricalchannels())
	{
		mAsymmetricalInputBufferList.PrepareBuffer(GetStreamFormat(kAudioUnitScope_Output, 0), inFramesToProcess);
		inputBufferPtr = &(mAsymmetricalInputBufferList.GetBufferList());
		auto const& srcAudioBuffer = inBuffer.mBuffers[0];
		for (UInt32 ch = 0; ch < inputBufferPtr->mNumberBuffers; ch++)
		{
			assert(srcAudioBuffer.mDataByteSize == inputBufferPtr->mBuffers[ch].mDataByteSize);
			auto const numBytes = std::min(srcAudioBuffer.mDataByteSize, inputBufferPtr->mBuffers[ch].mDataByteSize);
			memcpy(inputBufferPtr->mBuffers[ch].mData, srcAudioBuffer.mData, numBytes);
		}
	}

	// if the plugin uses DSP cores, then we just call the 
	// inherited base class implementation, which handles "Kernels"
	status = TARGET_API_BASE_CLASS::ProcessBufferLists(ioActionFlags, *inputBufferPtr, outBuffer, inFramesToProcess);

#else
	auto const numInputBuffers = inBuffer.mNumberBuffers;
	auto const numOutputBuffers = outBuffer.mNumberBuffers;

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
#endif  // end of if/else TARGET_PLUGIN_USES_DSPCORE

	// TODO: allow effects to communicate their output silence status, or calculate time-out from tail size and latency?
	bool const effectHasTail = !SupportsTail() || (gettailsize_samples() > 0) || (getlatency_samples() > 0);
	if (effectHasTail)
	{
		ioActionFlags &= ~kAudioUnitRenderAction_OutputIsSilence;
	}

	// do any post-DSP stuff
	postprocessaudio();

	return status;
}
#endif  // TARGET_PLUGIN_IS_INSTRUMENT



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
OSStatus DfxPlugin::HandleChannelPressure(UInt8 inChannel, UInt8 inValue, UInt32 inStartFrame)
{
	handlemidi_channelaftertouch(inChannel, inValue, inStartFrame);
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
#endif  // TARGET_PLUGIN_IS_INSTRUMENT

#endif  // TARGET_PLUGIN_USES_MIDI
