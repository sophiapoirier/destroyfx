/*------------------------------------------------------------------------
Copyright (C) 2001-2010  Sophia Poirier

This file is part of RMS Buddy.

RMS Buddy is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

RMS Buddy is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with RMS Buddy.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "rmsbuddy.h"

#include <AudioUnit/LogicAUProperties.h>


// macro for boring Component entry point stuff
COMPONENT_ENTRY(RMSBuddy)

//-----------------------------------------------------------------------------
RMSBuddy::RMSBuddy(AudioComponentInstance inComponentInstance)
	: AUEffectBase(inComponentInstance, true)	// "true" to say that we can process audio in-place
{
	// initialize the arrays and array quantity counter
	averageRMS = NULL;
	totalSquaredCollection = NULL;
	absolutePeak = NULL;
	guiContinualRMS = NULL;
	guiContinualPeak = NULL;
	guiShareDataCache = NULL;
	numChannels = 0;

	// initialize our parameters
	// (this also adds them to the parameter list that is shared when queried by a host)
	for (AudioUnitParameterID i=0; i < kRMSBuddyParameter_NumParameters; i++)
	{
		AudioUnitParameterInfo paramInfo;
		if (GetParameterInfo(kAudioUnitScope_Global, i, paramInfo) == noErr)
			AUBase::SetParameter(i, kAudioUnitScope_Global, (AudioUnitElement)0, paramInfo.defaultValue, 0);
	}
}

//-----------------------------------------------------------------------------------------
// this is called when we need to prepare for audio processing (allocate DSP resources, etc.)
OSStatus RMSBuddy::Initialize()
{
	// call parent implementation first
	OSStatus status = AUEffectBase::Initialize();
	if (status == noErr)
	{
		numChannels = GetNumberOfChannels();

		// allocate dynamics data value arrays according to the current number of channels
		averageRMS = (double*) malloc(numChannels * sizeof(double));
		totalSquaredCollection = (double*) malloc(numChannels * sizeof(double));
		absolutePeak = (float*) malloc(numChannels * sizeof(float));
		guiContinualRMS = (double*) malloc(numChannels * sizeof(double));
		guiContinualPeak = (float*) malloc(numChannels * sizeof(float));
		guiShareDataCache = (RMSBuddyDynamicsData*) malloc(numChannels * sizeof(RMSBuddyDynamicsData));

		// since hosts aren't required to trigger Reset between Initializing and starting audio processing, 
		// it's a good idea to do it ourselves here
		Reset(kAudioUnitScope_Global, (AudioUnitElement)0);
	}
	return status;
}

//-----------------------------------------------------------------------------------------
// this is the sort of mini-destructor partner to Initialize, where we clean up DSP resources
void RMSBuddy::Cleanup()
{
	// release all of our dynamics data value arrays

	if (averageRMS != NULL)
		free(averageRMS);
	averageRMS = NULL;

	if (totalSquaredCollection != NULL)
		free(totalSquaredCollection);
	totalSquaredCollection = NULL;

	if (absolutePeak != NULL)
		free(absolutePeak);
	absolutePeak = NULL;

	if (guiContinualRMS != NULL)
		free(guiContinualRMS);
	guiContinualRMS = NULL;

	if (guiContinualPeak != NULL)
		free(guiContinualPeak);
	guiContinualPeak = NULL;

	if (guiShareDataCache != NULL)
		free(guiShareDataCache);
	guiShareDataCache = NULL;
}

//-----------------------------------------------------------------------------------------
// this is called to reset the DSP state (clear buffers, reset counters, etc.)
OSStatus RMSBuddy::Reset(AudioUnitScope inScope, AudioUnitElement inElement)
{
	// reset all of these things
	resetRMS();
	resetPeak();
	// reset continual values, too (the above functions only reset the average/absolute values)
	if (guiShareDataCache != NULL)
	{
		for (UInt32 ch=0; ch < numChannels; ch++)
		{
			guiShareDataCache[ch].continualRMS = 0.0;
			guiShareDataCache[ch].continualPeak = 0.0;
		}
	}

	resetGUIcounters();
	notifyGUI();	// make sure that the GUI catches these changes

	return noErr;
}

//-----------------------------------------------------------------------------------------
// reset the average RMS-related values and restart calculation of average RMS
void RMSBuddy::resetRMS()
{
	totalSamples = 0;
	for (UInt32 ch=0; ch < numChannels; ch++)
	{
		if (totalSquaredCollection != NULL)
			totalSquaredCollection[ch] = 0.0;
		if (averageRMS != NULL)
			averageRMS[ch] = 0.0;

		if (guiShareDataCache != NULL)
			guiShareDataCache[ch].averageRMS = 0.0;
	}
}

//-----------------------------------------------------------------------------------------
// reset the absolute peak-related values and restart calculation of absolute peak
void RMSBuddy::resetPeak()
{
	for (UInt32 ch=0; ch < numChannels; ch++)
	{
		if (absolutePeak != NULL)
			absolutePeak[ch] = 0.0f;

		if (guiShareDataCache != NULL)
			guiShareDataCache[ch].absolutePeak = 0.0;
	}
}

//-----------------------------------------------------------------------------------------
// reset the GUI-related continual values
void RMSBuddy::resetGUIcounters()
{
	guiSamplesCounter = 0;
	for (UInt32 ch=0; ch < numChannels; ch++)
	{
		if (guiContinualRMS != NULL)
			guiContinualRMS[ch] = 0.0;
		if (guiContinualPeak != NULL)
			guiContinualPeak[ch] = 0.0f;
	}
}

//-----------------------------------------------------------------------------------------
// post notification to the GUI that it's time to re-fetch data and refresh its display
void RMSBuddy::notifyGUI()
{
	PropertyChanged(kRMSBuddyProperty_DynamicsData, kAudioUnitScope_Global, (AudioUnitElement)0);
}

//-----------------------------------------------------------------------------------------
// get the details about a parameter
OSStatus RMSBuddy::GetParameterInfo(AudioUnitScope inScope, 
				AudioUnitParameterID inParameterID, AudioUnitParameterInfo & outParameterInfo)
{
	if (inScope != kAudioUnitScope_Global)
		return kAudioUnitErr_InvalidScope;

	CFBundleRef pluginBundleRef = CFBundleGetBundleWithIdentifier( CFSTR(RMS_BUDDY_BUNDLE_ID) );
	CFStringRef paramNameString = NULL;
	switch (inParameterID)
	{
		// the size, in milliseconds, of the RMS and peak analysis window / refresh rate
		case kRMSBuddyParameter_AnalysisWindowSize:
			outParameterInfo.flags = kAudioUnitParameterFlag_IsReadable 
									| kAudioUnitParameterFlag_IsWritable
									| kAudioUnitParameterFlag_DisplaySquareRoot;
			paramNameString = CFCopyLocalizedStringFromTableInBundle(CFSTR("analysis window"), 
											CFSTR("Localizable"), pluginBundleRef, CFSTR("parameter name"));
			FillInParameterName(outParameterInfo, paramNameString, true);
			outParameterInfo.unit = kAudioUnitParameterUnit_Milliseconds;
			outParameterInfo.minValue = 30.0f;
			outParameterInfo.maxValue = 1000.0f;
			outParameterInfo.defaultValue = 69.0f;
			return noErr;

		// reset the average RMS values
		case kRMSBuddyParameter_ResetRMS:
			outParameterInfo.flags = kAudioUnitParameterFlag_IsWritable;	// write-only means it's a "trigger"-type parameter
			paramNameString = CFCopyLocalizedStringFromTableInBundle(CFSTR("reset average RMS"), 
											CFSTR("Localizable"), pluginBundleRef, CFSTR("parameter name"));
			FillInParameterName(outParameterInfo, paramNameString, true);
			outParameterInfo.unit = kAudioUnitParameterUnit_Boolean;
			outParameterInfo.minValue = 0.0f;
			outParameterInfo.maxValue = 1.0f;
			outParameterInfo.defaultValue = 0.0f;
			return noErr;

		// reset the absolute peak values
		case kRMSBuddyParameter_ResetPeak:
			outParameterInfo.flags = kAudioUnitParameterFlag_IsWritable;	// write-only means it's a "trigger"-type parameter
			paramNameString = CFCopyLocalizedStringFromTableInBundle(CFSTR("reset absolute peak"), 
											CFSTR("Localizable"), pluginBundleRef, CFSTR("parameter name"));
			FillInParameterName(outParameterInfo, paramNameString, true);
			outParameterInfo.unit = kAudioUnitParameterUnit_Boolean;
			outParameterInfo.minValue = 0.0f;
			outParameterInfo.maxValue = 1.0f;
			outParameterInfo.defaultValue = 0.0f;
			return noErr;

		default:
			return kAudioUnitErr_InvalidParameter;
	}
}

//-----------------------------------------------------------------------------------------
// only overridden to insert special handling of "trigger" parameters
OSStatus RMSBuddy::SetParameter(AudioUnitParameterID inParameterID, AudioUnitScope inScope, 
				AudioUnitElement inElement, Float32 inValue, UInt32 inBufferOffsetInFrames)
{
	switch (inParameterID)
	{
		// trigger the resetting of average RMS
		case kRMSBuddyParameter_ResetRMS:
			resetRMS();
			break;

		// trigger the resetting of absolute peak
		case kRMSBuddyParameter_ResetPeak:
			resetPeak();
			break;

		default:
			break;
	}

	return AUBase::SetParameter(inParameterID, inScope, inElement, inValue, inBufferOffsetInFrames);
}

//-----------------------------------------------------------------------------------------
// get the details about a property
OSStatus RMSBuddy::GetPropertyInfo(AudioUnitPropertyID inPropertyID, AudioUnitScope inScope, 
				AudioUnitElement inElement, UInt32 & outDataSize, Boolean & outWritable)
{
	switch (inPropertyID)
	{
		case kRMSBuddyProperty_DynamicsData:
			outDataSize = sizeof(RMSBuddyDynamicsData);
			outWritable = false;
			return noErr;

		case kLogicAUProperty_NodeOperationMode:
			outDataSize = sizeof(UInt32);
			outWritable = true;
			return noErr;

		case kLogicAUProperty_NodePropertyDescriptions:
			outDataSize = sizeof(LogicAUNodePropertyDescription);
			outWritable = false;
			return noErr;

		// let non-custom properties fall through to the parent class' handler
		default:
			return AUEffectBase::GetPropertyInfo(inPropertyID, inScope, inElement, outDataSize, outWritable);
	}
}

//-----------------------------------------------------------------------------------------
// get the value/data of a property
OSStatus RMSBuddy::GetProperty(AudioUnitPropertyID inPropertyID, AudioUnitScope inScope, 
				AudioUnitElement inElement, void * outData)
{
	switch (inPropertyID)
	{
		// get the current dynamics analysis data for a specified audio channel
		case kRMSBuddyProperty_DynamicsData:
			{
				if (guiShareDataCache == NULL)
					return kAudioUnitErr_Uninitialized;

				UInt32 requestedChannel = inScope;
				// invalid channel number requested
				if (requestedChannel >= numChannels)
					return kAudioUnitErr_InvalidPropertyValue;
				// if we got this far, all is good, copy the data for the requester
				memcpy(outData, &(guiShareDataCache[requestedChannel]), sizeof(RMSBuddyDynamicsData));
			}
			return noErr;

		case kLogicAUProperty_NodeOperationMode:
			*(UInt32*)outData = kLogicAUNodeOperationMode_FullSupport;
			return noErr;

		case kLogicAUProperty_NodePropertyDescriptions:
			{
				LogicAUNodePropertyDescription * nodePropertyDescs = (LogicAUNodePropertyDescription*) outData;
				nodePropertyDescs->mPropertyID = kRMSBuddyProperty_DynamicsData;
				nodePropertyDescs->mEndianMode = kLogicAUNodePropertyEndianMode_All64Bits;
				nodePropertyDescs->mFlags = 0;
			}
			return noErr;

		// let non-custom properties fall through to the parent class' handler
		default:
			return AUEffectBase::GetProperty(inPropertyID, inScope, inElement, outData);
	}
}

//-----------------------------------------------------------------------------------------
// set the value/data of a property
OSStatus RMSBuddy::SetProperty(AudioUnitPropertyID inPropertyID, AudioUnitScope inScope, 
				AudioUnitElement inElement, const void * inData, UInt32 inDataSize)
{
	switch (inPropertyID)
	{
		case kRMSBuddyProperty_DynamicsData:
			return kAudioUnitErr_PropertyNotWritable;

		case kLogicAUProperty_NodeOperationMode:
			// in this plugin, we don't actually care about what mode we're running under
			return noErr;

		case kLogicAUProperty_NodePropertyDescriptions:
			return kAudioUnitErr_PropertyNotWritable;

		// let non-custom properties fall through to the parent class' handler
		default:
			return AUEffectBase::SetProperty(inPropertyID, inScope, inElement, inData, inDataSize);
	}
}

//-----------------------------------------------------------------------------------------
// indicate how many custom GUI components are recommended for this AU
int RMSBuddy::GetNumCustomUIComponents()
{
	return 1;
}

//-----------------------------------------------------------------------------------------
// give a Component description of the GUI component(s) that we recommend for this AU
void RMSBuddy::GetUIComponentDescs(ComponentDescription * inDescArray)
{
	if (inDescArray == NULL)
		return;

	inDescArray->componentType = kAudioUnitCarbonViewComponentType;
	inDescArray->componentSubType = RMS_BUDDY_PLUGIN_ID;
	inDescArray->componentManufacturer = RMS_BUDDY_MANUFACTURER_ID;
	inDescArray->componentFlags = 0;
	inDescArray->componentFlagsMask = 0;
}

//-----------------------------------------------------------------------------------------
// process audio
// In this plugin, we don't actually alter the audio stream at all.  
// We simply look at the input values.  
// The nice thing is about doing "in-place processing" is that it means that 
// the input and output buffers are the same, so we don't need to copy the 
// audio input stream to output or anything pointless like that.
OSStatus RMSBuddy::ProcessBufferLists(AudioUnitRenderActionFlags & ioActionFlags, 
						const AudioBufferList & inBuffer, AudioBufferList & outBuffer, 
						UInt32 inFramesToProcess)
{
	// bad number of input channels
	if (inBuffer.mNumberBuffers < numChannels)
		return kAudioUnitErr_FormatNotSupported;


	// the host might have changed the InPlaceProcessing property, 
	// in which case we'll need to copy the audio input to output
	if ( !ProcessesInPlace() )
		GetInput(0)->CopyBufferContentsTo(outBuffer);

	// increment the sample counter for the total number of samples since (re)starting average RMS analysis
	totalSamples += inFramesToProcess;
	double invTotalSamples = 1.0 / (double)totalSamples;	// it's slightly more efficient to only divide once

	// increment the sample counter for the GUI displays
	guiSamplesCounter += inFramesToProcess;

	// loop through each channel
	for (UInt32 ch=0; ch < numChannels; ch++)
	{
		// manage the buffer list data, get pointer to the audio input stream
		float * in = (float*) (inBuffer.mBuffers[ch].mData);

		// these will store the values for this processing buffer
		double continualRMS = 0.0;
		float continualPeak = 0.0f;
		// analyze every sample for this channel
		for (UInt32 i=0; i < inFramesToProcess; i++)
		{
			float inSquared = in[i] * in[i];
			// RMS is the sum of squared input values, then averaged and square-rooted, so here we square and sum
			continualRMS += inSquared;
			// check if this is the peak sample for this processing buffer
			if (inSquared > continualPeak)
				continualPeak = inSquared;	// by squaring, we don't need to fabs (and we can un-square later)
		}

		// accumulate this buffer's squared samples into the collection
		totalSquaredCollection[ch] += continualRMS;
		// update the average RMS value
		averageRMS[ch] = sqrt(totalSquaredCollection[ch] * invTotalSamples);

		// unsquare this now to get the real sample value
		continualPeak = (float) sqrt(continualPeak);
		// update absolute peak value, if it has been exceeded
		if (continualPeak > absolutePeak[ch])
			absolutePeak[ch] = continualPeak;

		// accumulate this processing buffer's RMS collection into the RMS collection for the GUI displays
		guiContinualRMS[ch] += continualRMS;
		// update the GUI continual peak values, if it has been exceeded
		if (continualPeak > guiContinualPeak[ch])
			guiContinualPeak[ch] = continualPeak;
	}
	// end of per-channel loop


	// figure out if it's time to tell the GUI to refresh its display
	unsigned long analysisWindow = (unsigned long) (AUEffectBase::GetParameter(kRMSBuddyParameter_AnalysisWindowSize) * GetSampleRate() * 0.001);
	unsigned long nextCount = guiSamplesCounter + inFramesToProcess;	// estimate the size after the next processing window
	if ( (guiSamplesCounter > analysisWindow) || 
			(abs(guiSamplesCounter-analysisWindow) < abs(nextCount-analysisWindow)) )	// round
	{
		double invGUItotal = 1.0 / (double)guiSamplesCounter;	// it's slightly more efficient to only divide once
		// store the current dynamics data into the GUI data share caches for each channel...
		for (UInt32 ch=0; ch < numChannels; ch++)
		{
			guiShareDataCache[ch].averageRMS = averageRMS[ch];
			guiShareDataCache[ch].continualRMS = sqrt(guiContinualRMS[ch] * invGUItotal);
			guiShareDataCache[ch].absolutePeak = absolutePeak[ch];
			guiShareDataCache[ch].continualPeak = guiContinualPeak[ch];
		}

		// ... and then post notification to the GUI
		notifyGUI();
		// now that we've posted notification, restart the GUI analysis cycle
		resetGUIcounters();
	}


	return noErr;
}
