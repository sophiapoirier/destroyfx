/*--------------- by Marc Poirier  ][  June 2001 + February 2003 + November 2003 --------------*/

#include "rmsbuddy.h"

#include <AudioToolbox/AudioUnitUtilities.h>	// for AUParameterListenerNotify


// macro for boring Component entry point stuff
COMPONENT_ENTRY(RMSBuddy)

#ifdef RMSBUDDY_ENABLE_RESIZE_TEST
unsigned long _numChannels = 1;
#endif
//-----------------------------------------------------------------------------
RMSBuddy::RMSBuddy(AudioUnit component)
	: AUEffectBase(component, true)	// "true" to say that we can process audio in-place
{
	// initialize the arrays and array quantity counter
	// (choosing 2 since that's the default stream format and that at least sets it 
	// to something in case the UI gets created before we get Initialized)
	numChannels = 2;

	averageRMS = NULL;
	totalSquaredCollection = NULL;
	absolutePeak = NULL;
	guiContinualRMS = NULL;
	guiContinualPeak = NULL;
	guiShareDataCache = NULL;

	// initialize our parameter
	AudioUnitParameterInfo paramInfo;
	if (GetParameterInfo(kAudioUnitScope_Global, kRMSBuddyParameter_AnalysisWindowSize, paramInfo) == noErr)
		AUBase::SetParameter(kRMSBuddyParameter_AnalysisWindowSize, kAudioUnitScope_Global, (AudioUnitElement)0, paramInfo.defaultValue, 0);
}

//-----------------------------------------------------------------------------------------
// this is called when we need to prepare for audio processing (allocate DSP resources, etc.)
ComponentResult RMSBuddy::Initialize()
{
	// call parent implementation first
	ComponentResult result = AUEffectBase::Initialize();
	if (result == noErr)
	{
		unsigned long oldNumChannels = numChannels;
		numChannels = GetNumberOfChannels();
		if (numChannels != oldNumChannels)
			PropertyChanged(kRMSBuddyProperty_NumChannels, kAudioUnitScope_Global, (AudioUnitElement)0);

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
	return result;
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
// this is called the reset the DSP state (clear buffers, reset counters, etc.)
ComponentResult RMSBuddy::Reset(AudioUnitScope inScope, AudioUnitElement inElement)
{
	// reset all of these things
	resetRMS();
	resetPeak();
	// reset continual values, too (the above functions only reset the average/absolute values)
	if (guiShareDataCache != NULL)
	{
		for (unsigned long ch=0; ch < numChannels; ch++)
		{
			guiShareDataCache[ch].outContinualRMS = 0.0;
			guiShareDataCache[ch].outContinualPeak = 0.0f;
		}
	}

	resetGUIcounters();
	notifyGUI();	// make sure that the GUI catches these changes

	return noErr;
}

#ifdef RMSBUDDY_ENABLE_RESIZE_TEST
ComponentResult RMSBuddy::RestoreState(CFPropertyListRef inData)
{
	_numChannels++;
	PropertyChanged(kRMSBuddyProperty_NumChannels, kAudioUnitScope_Global, (AudioUnitElement)0);
	return AUBase::RestoreState(inData);
}
#endif

//-----------------------------------------------------------------------------------------
// reset the average RMS-related values and restart calculation of average RMS
void RMSBuddy::resetRMS()
{
	totalSamples = 0;
	for (unsigned long ch=0; ch < numChannels; ch++)
	{
		if (totalSquaredCollection != NULL)
			totalSquaredCollection[ch] = 0.0;
		if (averageRMS != NULL)
			averageRMS[ch] = 0.0;

		if (guiShareDataCache != NULL)
			guiShareDataCache[ch].outAverageRMS = 0.0;
	}
}

//-----------------------------------------------------------------------------------------
// reset the absolute peak-related values and restart calculation of absolute peak
void RMSBuddy::resetPeak()
{
	for (unsigned long ch=0; ch < numChannels; ch++)
	{
		if (absolutePeak != NULL)
			absolutePeak[ch] = 0.0f;

		if (guiShareDataCache != NULL)
			guiShareDataCache[ch].outAbsolutePeak = 0.0f;
	}
}

//-----------------------------------------------------------------------------------------
// reset the GUI-related continual values
void RMSBuddy::resetGUIcounters()
{
	guiSamplesCounter = 0;
	for (unsigned long ch=0; ch < numChannels; ch++)
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
	// we use a parameter chane notification (using the fake parameter) rather than a property change notification 
	// because property change notifications cause immediate callbacks which should not be done from audio threads, 
	// whereas parameter change notifications enter a queue that is checked periodically by parameter listeners, 
	// and therefore return immediately and are fine for audio threads
	AudioUnitParameter messengerParam;
	messengerParam.mAudioUnit = GetComponentInstance();
	messengerParam.mParameterID = kRMSBuddyParameter_TimeToUpdate;
	messengerParam.mScope = kAudioUnitScope_Global;
	messengerParam.mElement = 0;
	AUParameterListenerNotify(NULL, NULL, &messengerParam);
}

//-----------------------------------------------------------------------------------------
ComponentResult RMSBuddy::GetParameterInfo(AudioUnitScope inScope, 
						AudioUnitParameterID inParameterID, AudioUnitParameterInfo & outParameterInfo)
{
	// the size, in ms, of the RMS and peak analysis window / refresh rate
	if (inParameterID == kRMSBuddyParameter_AnalysisWindowSize)
	{
		outParameterInfo.flags = kAudioUnitParameterFlag_IsReadable 
								| kAudioUnitParameterFlag_IsWritable
								| kAudioUnitParameterFlag_DisplaySquareRoot;

		CFBundleRef pluginBundleRef = CFBundleGetBundleWithIdentifier( CFSTR(RMS_BUDDY_BUNDLE_ID) );
		CFStringRef paramNameString = CFCopyLocalizedStringFromTableInBundle(CFSTR("analysis window"), 
										CFSTR("Localizable"), pluginBundleRef, CFSTR("parameter name"));
		FillInParameterName(outParameterInfo, paramNameString, true);

		outParameterInfo.unit = kAudioUnitParameterUnit_Milliseconds;
		outParameterInfo.minValue = 30.0f;
		outParameterInfo.maxValue = 1000.0f;
		outParameterInfo.defaultValue = 69.0f;

		return noErr;
	}

	// a fake parameter (really an audio thread GUI notification mechanism)
	if (inParameterID == kRMSBuddyParameter_TimeToUpdate)
	{
		memset(&outParameterInfo, 0, sizeof(outParameterInfo));
		return noErr;
	}

	return kAudioUnitErr_InvalidParameter;
}

//-----------------------------------------------------------------------------------------
// get the current value of a parameter
ComponentResult RMSBuddy::GetParameter(AudioUnitParameterID inParameterID, AudioUnitScope inScope, 
										AudioUnitElement inElement, Float32 & outValue)
{
	// it's a fake parameter, but if we don't at least say noErr for this one, the parameter listener system won't work
	if (inParameterID == kRMSBuddyParameter_TimeToUpdate)
		return noErr;

	// let the AUBase systems handle our real parameter
	else if (inParameterID == kRMSBuddyParameter_AnalysisWindowSize)
		return AUBase::GetParameter(inParameterID, inScope, inElement, outValue);

	else
		return kAudioUnitErr_InvalidParameter;
}

//-----------------------------------------------------------------------------------------
// get the details about a property
ComponentResult RMSBuddy::GetPropertyInfo(AudioUnitPropertyID inPropertyID, AudioUnitScope inScope, 
						AudioUnitElement inElement, UInt32 & outDataSize, Boolean & outWritable)
{
	switch (inPropertyID)
	{
		case kRMSBuddyProperty_DynamicsData:
			outDataSize = sizeof(RMSBuddyDynamicsData);
			outWritable = false;
			return noErr;

		case kRMSBuddyProperty_ResetRMS:
		case kRMSBuddyProperty_ResetPeak:
			outDataSize = sizeof(char);	// whatever, input data isn't actually needed to set these properties
			outWritable = true;
			return noErr;

		case kRMSBuddyProperty_NumChannels:
			outDataSize = sizeof(unsigned long);
			outWritable = false;
			return noErr;

		// let non-custom properties fall through to the parent class' handler
		default:
			return AUEffectBase::GetPropertyInfo(inPropertyID, inScope, inElement, outDataSize, outWritable);
	}
}

//-----------------------------------------------------------------------------------------
// get the value/data of a property
ComponentResult RMSBuddy::GetProperty(AudioUnitPropertyID inPropertyID, AudioUnitScope inScope, 
						AudioUnitElement inElement, void * outData)
{
	switch (inPropertyID)
	{
		// get the current dynamics analysis data for a specified audio channel
		case kRMSBuddyProperty_DynamicsData:
			{
				if (guiShareDataCache == NULL)
					return kAudioUnitErr_Uninitialized;

				unsigned long requestedChannel = ((RMSBuddyDynamicsData*)outData)->inChannel;
				// invalid channel number requested
				if (requestedChannel >= numChannels)
					return kAudioUnitErr_InvalidPropertyValue;
				// if we got this far, all is good, copy the data for the requester
				memcpy(outData, &(guiShareDataCache[requestedChannel]), sizeof(RMSBuddyDynamicsData));
			}
			return noErr;

		// this has no actual data, it's used for event messages
		case kRMSBuddyProperty_ResetRMS:
			return noErr;

		// this has no actual data, it's used for event messages
		case kRMSBuddyProperty_ResetPeak:
			return noErr;

		// get the number of audio channels being analyzed
		case kRMSBuddyProperty_NumChannels:
			*((unsigned long*)outData) = numChannels;
#ifdef RMSBUDDY_ENABLE_RESIZE_TEST
			*((unsigned long*)outData) = _numChannels;
#endif
			return noErr;

		// let non-custom properties fall through to the parent class' handler
		default:
			return AUEffectBase::GetProperty(inPropertyID, inScope, inElement, outData);
	}
}

//-----------------------------------------------------------------------------------------
// set the value/data of a property
ComponentResult RMSBuddy::SetProperty(AudioUnitPropertyID inPropertyID, AudioUnitScope inScope, 
						AudioUnitElement inElement, const void * inData, UInt32 inDataSize)
{
	switch (inPropertyID)
	{
		case kRMSBuddyProperty_DynamicsData:
			return kAudioUnitErr_PropertyNotWritable;

		// trigger the resetting of average RMS
		case kRMSBuddyProperty_ResetRMS:
			resetRMS();
			return noErr;

		// trigger the resetting of absolute peak
		case kRMSBuddyProperty_ResetPeak:
			resetPeak();
			return noErr;

		case kRMSBuddyProperty_NumChannels:
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
// in this plugin, we don't actually alter the audio stream at all
// we simply look at the input values
// the nice thing is about this being an "inline effect" is that it means that 
// the input and output buffers are the same, so we don't need to copy the 
// audio input stream to output or anything pointless like that
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
	for (unsigned long ch=0; ch < numChannels; ch++)
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
		for (unsigned long ch=0; ch < numChannels; ch++)
		{
			guiShareDataCache[ch].outAverageRMS = averageRMS[ch];
			guiShareDataCache[ch].outContinualRMS = sqrt(guiContinualRMS[ch] * invGUItotal);
			guiShareDataCache[ch].outAbsolutePeak = absolutePeak[ch];
			guiShareDataCache[ch].outContinualPeak = guiContinualPeak[ch];
		}

		// ... and then post notification to the GUI
		notifyGUI();
		// now that we've posted notification, restart the GUI analysis cycle
		resetGUIcounters();
	}


	return noErr;
}
