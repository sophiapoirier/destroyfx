/*------------------------------------------------------------------------
Copyright (C) 2001-2022  Sophia Poirier

This file is part of RMS Buddy.

RMS Buddy is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 2 of the License, or 
(at your option) any later version.

RMS Buddy is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with RMS Buddy.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#include "rmsbuddy.h"

#include <algorithm>
#include <AudioToolbox/AudioUnitUtilities.h>
#include <cassert>
#include <cmath>
#include <memory>
#include <span>
#include <type_traits>

#include "rmsbuddy-base.h"


static CFStringRef const kRMSBuddyBundleID = CFSTR("org.destroyfx.RMSBuddy");
static constexpr UInt32 kBaseClumpID = kAudioUnitClumpID_System + 1;

using namespace dfx::RMS;


// macro for boring Component entry point stuff
AUSDK_COMPONENT_ENTRY(ausdk::AUBaseFactory, RMSBuddy)

//-----------------------------------------------------------------------------
RMSBuddy::RMSBuddy(AudioComponentInstance inComponentInstance)
:	ausdk::AUEffectBase(inComponentInstance, true),
	mMinMeterValueDb(LinearToDecibels(1.0f / std::pow(2.0f, 24.0f)))  // smallest 24-bit audio value
{
	// initialize our parameters
	// (this also adds them to the parameter list that is shared when queried by a host)
	for (AudioUnitParameterID paramID = 0; paramID < kParameter_BaseCount; paramID++)
	{
		AudioUnitParameterInfo paramInfo {};
		if (GetParameterInfo(kAudioUnitScope_Global, paramID, paramInfo) == noErr)
		{
			CFRelease(paramInfo.cfNameString);
			AUBase::SetParameter(paramID, kAudioUnitScope_Global, AudioUnitElement{0}, paramInfo.defaultValue, 0);
		}
	}
}

//-----------------------------------------------------------------------------------------
// this is called when we need to prepare for audio processing (allocate DSP resources, etc.)
OSStatus RMSBuddy::Initialize()
{
	auto const status = AUEffectBase::Initialize();
	if (status == noErr)
	{
		HandleChannelCount();

		// hosts aren't required to trigger Reset between Initializing and starting audio processing, 
		// so it's a good idea to do it ourselves here
		Reset(kAudioUnitScope_Global, AudioUnitElement{0});
	}
	return status;
}

//-----------------------------------------------------------------------------------------
// this is the sort of mini-destructor partner to Initialize, where we clean up DSP resources
void RMSBuddy::Cleanup()
{
	mAverageRMS = {};
	mTotalSquaredCollection = {};
	mAbsolutePeak = {};
	mContinualRMS = {};
	mContinualPeak = {};
}

//-----------------------------------------------------------------------------------------
// this is called to reset the DSP state (clear buffers, reset counters, etc.)
OSStatus RMSBuddy::Reset(AudioUnitScope inScope, AudioUnitElement inElement)
{
	// reset all of these things
	ResetRMS();
	ResetPeak();
	// reset continual values, too (the above functions only reset the average/absolute values)
	for (UInt32 ch = 0; ch < mChannelCount; ch++)
	{
		SetMeter(ch, kChannelParameter_ContinualRMS, 0.0f);
		SetMeter(ch, kChannelParameter_ContinualPeak, 0.0f);
	}

	RestartAnalysisWindow();

	return noErr;
}

//-----------------------------------------------------------------------------------------
OSStatus RMSBuddy::GetParameterList(AudioUnitScope inScope, AudioUnitParameterID* outParameterList, UInt32& outNumParameters)
{
	if (inScope != kAudioUnitScope_Global)
	{
		return AUEffectBase::GetParameterList(inScope, outParameterList, outNumParameters);
	}

	outNumParameters = kParameter_BaseCount + (kChannelParameter_Count * mChannelCount);
	if (outParameterList)
	{
		UInt32 totalParameterCount{};
		auto status = AUEffectBase::GetParameterList(inScope, nullptr, totalParameterCount);
		if (status != noErr)
		{
			return status;
		}
		assert(outNumParameters <= totalParameterCount);
		std::vector<AudioUnitParameterID> totalParameterList(totalParameterCount);
		status = AUEffectBase::GetParameterList(inScope, totalParameterList.data(), totalParameterCount);
		if (status != noErr)
		{
			return status;
		}
		std::copy_n(totalParameterList.data(), outNumParameters, outParameterList);
	}
	return noErr;
}

//-----------------------------------------------------------------------------------------
// get the details about a parameter
OSStatus RMSBuddy::GetParameterInfo(AudioUnitScope inScope, AudioUnitParameterID inParameterID, 
									AudioUnitParameterInfo& outParameterInfo)
{
	if (inScope != kAudioUnitScope_Global)
	{
		return kAudioUnitErr_InvalidScope;
	}

	auto const pluginBundleRef = CFBundleGetBundleWithIdentifier(kRMSBuddyBundleID);
	switch (inParameterID)
	{
		case kParameter_AnalysisWindowSize:
		{
			outParameterInfo.flags = kAudioUnitParameterFlag_IsReadable 
									| kAudioUnitParameterFlag_IsWritable
									| kAudioUnitParameterFlag_DisplaySquareRoot;
			auto const paramNameString = CFCopyLocalizedStringFromTableInBundle(CFSTR("analysis window"), CFSTR("Localizable"), 
																				pluginBundleRef, CFSTR("parameter name"));
			FillInParameterName(outParameterInfo, paramNameString, true);
			outParameterInfo.unit = kAudioUnitParameterUnit_Milliseconds;
			outParameterInfo.minValue = 30.0f;
			outParameterInfo.maxValue = 1000.0f;
			outParameterInfo.defaultValue = 69.0f;
			return noErr;
		}

		case kParameter_ResetRMS:
		{
			outParameterInfo.flags = kAudioUnitParameterFlag_IsWritable;  // write-only means it's a "trigger"-type parameter
			auto const paramNameString = CFCopyLocalizedStringFromTableInBundle(CFSTR("reset average RMS"), CFSTR("Localizable"), 
																				pluginBundleRef, CFSTR("parameter name"));
			FillInParameterName(outParameterInfo, paramNameString, true);
			outParameterInfo.unit = kAudioUnitParameterUnit_Boolean;
			outParameterInfo.minValue = 0.0f;
			outParameterInfo.maxValue = 1.0f;
			outParameterInfo.defaultValue = 0.0f;
			return noErr;
		}

		case kParameter_ResetPeak:
		{
			outParameterInfo.flags = kAudioUnitParameterFlag_IsWritable;  // write-only means it's a "trigger"-type parameter
			auto const paramNameString = CFCopyLocalizedStringFromTableInBundle(CFSTR("reset absolute peak"), CFSTR("Localizable"), 
																				pluginBundleRef, CFSTR("parameter name"));
			FillInParameterName(outParameterInfo, paramNameString, true);
			outParameterInfo.unit = kAudioUnitParameterUnit_Boolean;
			outParameterInfo.minValue = 0.0f;
			outParameterInfo.maxValue = 1.0f;
			outParameterInfo.defaultValue = 0.0f;
			return noErr;
		}

		default:
		{
			auto const channelAndID = GetChannelAndIDFromParameterID(inParameterID);
			assert(channelAndID);
			if (!channelAndID)
			{
				return kAudioUnitErr_InvalidParameter;
			}
			auto const [channelIndex, channelParamID] = *channelAndID;
			if (channelIndex >= mChannelCount)
			{
				return kAudioUnitErr_InvalidParameter;
			}
			outParameterInfo.flags = kAudioUnitParameterFlag_IsReadable 
									| kAudioUnitParameterFlag_MeterReadOnly 
									| kAudioUnitParameterFlag_HasName 
									| kAudioUnitParameterFlag_OmitFromPresets;
			outParameterInfo.unit = kAudioUnitParameterUnit_Decibels;
			outParameterInfo.minValue = mMinMeterValueDb;
			outParameterInfo.maxValue = 12.0f;
			outParameterInfo.defaultValue = outParameterInfo.minValue;
			HasClump(outParameterInfo, kBaseClumpID + channelIndex);
			switch (channelParamID)
			{
				case kChannelParameter_AverageRMS:
				{
					auto const paramNameString = CFCopyLocalizedStringFromTableInBundle(CFSTR("average RMS"), CFSTR("Localizable"), 
																						pluginBundleRef, CFSTR("parameter name"));
					FillInParameterName(outParameterInfo, paramNameString, true);
					return noErr;
				}
				case kChannelParameter_ContinualRMS:
				{
					auto const paramNameString = CFCopyLocalizedStringFromTableInBundle(CFSTR("continual RMS"), CFSTR("Localizable"), 
																						pluginBundleRef, CFSTR("parameter name"));
					FillInParameterName(outParameterInfo, paramNameString, true);
					return noErr;
				}
				case kChannelParameter_AbsolutePeak:
				{
					auto const paramNameString = CFCopyLocalizedStringFromTableInBundle(CFSTR("absolute peak"), CFSTR("Localizable"), 
																						pluginBundleRef, CFSTR("parameter name"));
					FillInParameterName(outParameterInfo, paramNameString, true);
					return noErr;
				}
				case kChannelParameter_ContinualPeak:
				{
					auto const paramNameString = CFCopyLocalizedStringFromTableInBundle(CFSTR("continual peak"), CFSTR("Localizable"), 
																						pluginBundleRef, CFSTR("parameter name"));
					FillInParameterName(outParameterInfo, paramNameString, true);
					return noErr;
				}
				default:
					assert(false);
					return kAudioUnitErr_InvalidParameter;
			}
		}
	}
}

//-----------------------------------------------------------------------------------------
OSStatus RMSBuddy::CopyClumpName(AudioUnitScope inScope, UInt32 inClumpID, UInt32 inDesiredNameLength, CFStringRef* outClumpName)
{
	if (inClumpID < kBaseClumpID)
	{
		return AUEffectBase::CopyClumpName(inScope, inClumpID, inDesiredNameLength, outClumpName);
	}
	if (inScope != kAudioUnitScope_Global)
	{
		return kAudioUnitErr_InvalidScope;
	}

	auto const pluginBundleRef = CFBundleGetBundleWithIdentifier(kRMSBuddyBundleID);
	if (mChannelCount <= 1)
	{
		*outClumpName = CFSTR("");
	}
	else if (mChannelCount == 2)
	{
		if ((inClumpID - kBaseClumpID) == 0)
		{
			*outClumpName = CFCopyLocalizedStringFromTableInBundle(CFSTR("left"), CFSTR("Localizable"), 
																   pluginBundleRef, CFSTR("left channel parameter clump name"));
		}
		else
		{
			*outClumpName = CFCopyLocalizedStringFromTableInBundle(CFSTR("right"), CFSTR("Localizable"), 
																   pluginBundleRef, CFSTR("right channel parameter clump name"));
		}
	}
	else
	{
		auto const makeUniqueCFString = [](CFStringRef inString)
		{
			return std::unique_ptr<std::remove_pointer_t<CFStringRef>, void(*)(CFTypeRef)>(inString, CFRelease);
		};
		auto const clumpNameFormat = makeUniqueCFString(CFCopyLocalizedStringFromTableInBundle(CFSTR("channel %u"), 
																							   CFSTR("Localizable"), 
																							   pluginBundleRef, 
																							   CFSTR("parameter clump name")));
		auto const clumpNumber = (inClumpID - kBaseClumpID) + 1;
		*outClumpName = CFStringCreateWithFormat(kCFAllocatorDefault, nullptr, clumpNameFormat.get(), clumpNumber);
	}
	return noErr;
}

//-----------------------------------------------------------------------------------------
// only overridden to insert special handling of "trigger" parameters
OSStatus RMSBuddy::SetParameter(AudioUnitParameterID inParameterID, AudioUnitScope inScope, AudioUnitElement inElement, 
								Float32 inValue, UInt32 inBufferOffsetInFrames)
{
	switch (inParameterID)
	{
		case kParameter_ResetRMS:
			ResetRMS();
			break;

		case kParameter_ResetPeak:
			ResetPeak();
			break;

		default:
			break;
	}

	return AUBase::SetParameter(inParameterID, inScope, inElement, inValue, inBufferOffsetInFrames);
}

//-----------------------------------------------------------------------------------------
// get the details about a property
OSStatus RMSBuddy::GetPropertyInfo(AudioUnitPropertyID inPropertyID, AudioUnitScope inScope, AudioUnitElement inElement, 
								   UInt32& outDataSize, bool& outWritable)
{
	switch (inPropertyID)
	{
		case kAudioUnitProperty_ParameterStringFromValue:
			outDataSize = sizeof(AudioUnitParameterStringFromValue);
			outWritable = false;
			return noErr;

		default:
			return AUEffectBase::GetPropertyInfo(inPropertyID, inScope, inElement, outDataSize, outWritable);
	}
}

//-----------------------------------------------------------------------------------------
// get the value/data of a property
OSStatus RMSBuddy::GetProperty(AudioUnitPropertyID inPropertyID, AudioUnitScope inScope, AudioUnitElement inElement, 
							   void* outData)
{
	switch (inPropertyID)
	{
		case kAudioUnitProperty_ParameterStringFromValue:
		{
			auto const parameterStringFromValue = static_cast<AudioUnitParameterStringFromValue*>(outData);
			auto const paramID = parameterStringFromValue->inParamID;
			if (parameterStringFromValue->inParamID < kChannelParameter_Base)
			{
				return AUEffectBase::GetProperty(inPropertyID, inScope, inElement, outData);
			}
			auto const paramValue = parameterStringFromValue->inValue ? *parameterStringFromValue->inValue : GetParameter(paramID);
			parameterStringFromValue->outString = [&]()-> CFStringRef
			{
				if (paramValue <= mMinMeterValueDb)
				{
					constexpr UniChar minusInfinity[] = { '-', 0x221E };
					return CFStringCreateWithCharacters(kCFAllocatorDefault, minusInfinity, std::size(minusInfinity));
				}
				return nullptr;
			}();
			return noErr;
		}

		default:
			return AUEffectBase::GetProperty(inPropertyID, inScope, inElement, outData);
	}
}

//-----------------------------------------------------------------------------------------
// set the value/data of a property
OSStatus RMSBuddy::SetProperty(AudioUnitPropertyID inPropertyID, AudioUnitScope inScope, AudioUnitElement inElement, 
							   void const* inData, UInt32 inDataSize)
{
	switch (inPropertyID)
	{
		case kAudioUnitProperty_ParameterStringFromValue:
			return kAudioUnitErr_PropertyNotWritable;

		default:
			return AUEffectBase::SetProperty(inPropertyID, inScope, inElement, inData, inDataSize);
	}
}

//-----------------------------------------------------------------------------------------
CFURLRef RMSBuddy::CopyIconLocation()
{
	if (auto const pluginBundleRef = CFBundleGetBundleWithIdentifier(kRMSBuddyBundleID))
	{
		return CFBundleCopyResourceURL(pluginBundleRef, CFSTR("destroyfx.icns"), nullptr, nullptr);
	}
	return nullptr;
}

//-----------------------------------------------------------------------------------------
// process audio
// In this plugin, we don't actually alter the audio stream at all.  
// We simply look at the input values.  
// The nice thing is about doing "in-place processing" is that it means that 
// the input and output buffers are the same, so we don't even need to copy 
// the audio input stream to output.
OSStatus RMSBuddy::ProcessBufferLists(AudioUnitRenderActionFlags& ioActionFlags, 
									  AudioBufferList const& inBuffer, AudioBufferList& outBuffer, 
									  UInt32 inFramesToProcess)
{
	// bad number of input channels
	if (inBuffer.mNumberBuffers < mChannelCount)
	{
		return kAudioUnitErr_FormatNotSupported;
	}

	// the host might have changed the InPlaceProcessing property, 
	// in which case we'll need to copy the audio input to output
	if (!ProcessesInPlace())
	{
		Input(0).CopyBufferContentsTo(outBuffer);
	}

	mTotalSamples += inFramesToProcess;
	auto const invTotalSamples = 1.0 / static_cast<double>(mTotalSamples);  // it's more efficient to only divide once

	mAnalysisWindowSampleCounter += inFramesToProcess;

	for (UInt32 ch = 0; ch < mChannelCount; ch++)
	{
		// get pointer to the audio input stream
		std::span const inAudio(static_cast<float const*>(inBuffer.mBuffers[ch].mData), inFramesToProcess);

		// these will store the values for this channel's processing buffer
		double continualRMS = 0.0;
		float continualPeak = 0.0f;
		// analyze every sample for this channel
		for (auto const inputValue : inAudio)
		{
			auto const inSquared = inputValue * inputValue;
			// RMS is the sum of squared input values, then averaged and square-rooted, so here we square and sum
			continualRMS += inSquared;
			// check if this is the peak sample for this processing buffer
			continualPeak = std::max(continualPeak, inSquared);  // by squaring, we don't need to fabs (and we can un-square later)
		}

		// accumulate this buffer's squared samples into the collection
		mTotalSquaredCollection[ch] += continualRMS;
		// update the average RMS value
		mAverageRMS[ch] = std::sqrt(mTotalSquaredCollection[ch] * invTotalSamples);

		// unsquare this now to get the real sample value
		continualPeak = std::sqrt(continualPeak);
		// update absolute peak value, if it has been exceeded
		mAbsolutePeak[ch] = std::max(mAbsolutePeak[ch], continualPeak);

		// accumulate this processing buffer's RMS collection into the RMS collection for the GUI displays
		mContinualRMS[ch] += continualRMS;
		// update the GUI continual peak values, if it has been exceeded
		mContinualPeak[ch] = std::max(mContinualPeak[ch], continualPeak);
	}


	// figure out if it's time to tell the GUI to refresh its display
	auto const analysisWindow = static_cast<uint64_t>(AUEffectBase::GetParameter(kParameter_AnalysisWindowSize) * GetSampleRate() * 0.001);
	auto const nextCount = mAnalysisWindowSampleCounter + inFramesToProcess;  // predict the total after the next processing window
	if ((mAnalysisWindowSampleCounter > analysisWindow) || 
		(std::labs(static_cast<long>(mAnalysisWindowSampleCounter) - static_cast<long>(analysisWindow)) < std::labs(static_cast<long>(nextCount) - static_cast<long>(analysisWindow))))  // round
	{
		auto const invGUItotal = 1.0 / static_cast<double>(mAnalysisWindowSampleCounter);  // it's more efficient to only divide once
		// publish the current dynamics data for each channel...
		for (UInt32 ch = 0; ch < mChannelCount; ch++)
		{
			SetMeter(ch, kChannelParameter_AverageRMS, mAverageRMS[ch]);
			SetMeter(ch, kChannelParameter_ContinualRMS, std::sqrt(mContinualRMS[ch] * invGUItotal));
			SetMeter(ch, kChannelParameter_AbsolutePeak, mAbsolutePeak[ch]);
			SetMeter(ch, kChannelParameter_ContinualPeak, mContinualPeak[ch]);
		}

		// now that we've posted notification, restart the GUI analysis cycle
		RestartAnalysisWindow();
	}


	return noErr;
}

//-----------------------------------------------------------------------------------------
AudioUnitParameterID RMSBuddy::GetParameterIDFromChannelAndID(UInt32 inChannelIndex, AudioUnitParameterID inID)
{
	return kChannelParameter_Base + (inChannelIndex * kChannelParameter_Count) + inID;
}

//-----------------------------------------------------------------------------------------
std::optional<RMSBuddy::ChannelParameterDesc> RMSBuddy::GetChannelAndIDFromParameterID(AudioUnitParameterID inParameterID)
{
	if (inParameterID < kChannelParameter_Base)
	{
		return std::nullopt;
	}
	inParameterID -= kChannelParameter_Base;
	return std::make_pair(inParameterID / kChannelParameter_Count, inParameterID % kChannelParameter_Count);
}

//-----------------------------------------------------------------------------------------
float RMSBuddy::LinearToDecibels(float inLinearValue)
{
	return 20.0f * std::log10(inLinearValue);
}

//-----------------------------------------------------------------------------------------
void RMSBuddy::HandleChannelCount()
{
	auto const previousChannelCount = std::exchange(mChannelCount, GetNumberOfChannels());
	if (mChannelCount != previousChannelCount)
	{
		mAverageRMS.assign(mChannelCount, 0.0);
		mTotalSquaredCollection.assign(mChannelCount, 0.0);
		mAbsolutePeak.assign(mChannelCount, 0.0f);
		mContinualRMS.assign(mChannelCount, 0.0);
		mContinualPeak.assign(mChannelCount, 0.0f);

		for (UInt32 ch = previousChannelCount; ch < mChannelCount; ch++)
		{
			for (AudioUnitParameterID channelParamID = 0; channelParamID < kChannelParameter_Count; channelParamID++)
			{
				auto const parameterID = GetParameterIDFromChannelAndID(ch, channelParamID);
				AUBase::SetParameter(parameterID, kAudioUnitScope_Global, AudioUnitElement{0}, mMinMeterValueDb, 0);
			}
		}

		PropertyChanged(kAudioUnitProperty_ParameterList, kAudioUnitScope_Global, AudioUnitElement{0});
	}
}

//-----------------------------------------------------------------------------------------
void RMSBuddy::SetMeter(UInt32 inChannelIndex, AudioUnitParameterID inID, AudioUnitParameterValue inLinearValue)
{
	auto const paramID = GetParameterIDFromChannelAndID(inChannelIndex, inID);
	auto const decibelValue = std::max(LinearToDecibels(inLinearValue), mMinMeterValueDb);
	AudioUnitParameter const auParam = { GetComponentInstance(), paramID, kAudioUnitScope_Global, AudioUnitElement{0} };
	AUParameterSet(nullptr, nullptr, &auParam, decibelValue, 0);
}

//-----------------------------------------------------------------------------------------
// reset the average RMS-related values and restart calculation of average RMS
void RMSBuddy::ResetRMS()
{
	mTotalSamples = 0;
	std::fill(mAverageRMS.begin(), mAverageRMS.end(), 0.0);
	std::fill(mTotalSquaredCollection.begin(), mTotalSquaredCollection.end(), 0.0);
	for (UInt32 ch = 0; ch < mChannelCount; ch++)
	{
		SetMeter(ch, kChannelParameter_AverageRMS, mAverageRMS[0]);
	}
}

//-----------------------------------------------------------------------------------------
// reset the absolute peak-related values and restart calculation of absolute peak
void RMSBuddy::ResetPeak()
{
	std::fill(mAbsolutePeak.begin(), mAbsolutePeak.end(), 0.0f);
	for (UInt32 ch = 0; ch < mChannelCount; ch++)
	{
		SetMeter(ch, kChannelParameter_AbsolutePeak, mAbsolutePeak[ch]);
	}
}

//-----------------------------------------------------------------------------------------
// reset the GUI-related continual values
void RMSBuddy::RestartAnalysisWindow()
{
	mAnalysisWindowSampleCounter = 0;
	std::fill(mContinualRMS.begin(), mContinualRMS.end(), 0.0);
	std::fill(mContinualPeak.begin(), mContinualPeak.end(), 0.0f);
}
