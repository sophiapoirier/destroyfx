/*--------------- by Marc Poirier  ][  June 2001 + February 2003 --------------*/

#include "rmsbuddy.h"

#include <AudioToolbox/AudioUnitUtilities.h>	// for AUParameterListenerNotify


// macro for boring Component entry point stuff
COMPONENT_ENTRY(RMSbuddy);

//-----------------------------------------------------------------------------
RMSbuddy::RMSbuddy(AudioUnit component)
	: AUInlineEffectBase(component)
{
	// initialize the arrays and array quantity counter
	numChannels = 0;

	averageRMS = NULL;
	totalSquaredCollection = NULL;
	absolutePeak = NULL;
	GUIcontinualRMS = NULL;
	GUIcontinualPeak = NULL;
	GUIshareDataCache = NULL;

	// initialize our parameter
	AudioUnitParameterInfo paramInfo;
	if (GetParameterInfo(kAudioUnitScope_Global, kAnalysisFrameSize, paramInfo) == noErr)
		AUBase::SetParameter(kAnalysisFrameSize, kAudioUnitScope_Global, (AudioUnitElement)0, paramInfo.defaultValue, 0);
}

//-----------------------------------------------------------------------------------------
RMSbuddy::~RMSbuddy()
{
	// nud
}

//-----------------------------------------------------------------------------------------
// this is called when we need to prepare for audio processing (allocate DSP resources, etc.)
ComponentResult RMSbuddy::Initialize()
{
	// call parent implementation first
	ComponentResult result = AUInlineEffectBase::Initialize();
	if (result == noErr)
	{
		// allocate dynamics data value arrays according to the current number of channels
		numChannels = GetNumberOfChannels();
		averageRMS = (double*) malloc(numChannels * sizeof(double));
		totalSquaredCollection = (double*) malloc(numChannels * sizeof(double));
		absolutePeak = (float*) malloc(numChannels * sizeof(float));
		GUIcontinualRMS = (double*) malloc(numChannels * sizeof(double));
		GUIcontinualPeak = (float*) malloc(numChannels * sizeof(float));
		GUIshareDataCache = (DynamicsData*) malloc(numChannels * sizeof(DynamicsData));

		// since hosts aren't required to trigger Reset between Initializing and starting audio processing, 
		// it's a good idea to do it ourselves here
		Reset(kAudioUnitScope_Global, (AudioUnitElement)0);
	}
	return result;
}

//-----------------------------------------------------------------------------------------
// this is the sort of mini-destructor partner to Initialize, where we clean up DSP resources
void RMSbuddy::Cleanup()
{
	// release all of our dynamics data value arrays and reset the array counter (numChannels)

	if (averageRMS != NULL)
		free(averageRMS);
	averageRMS = NULL;

	if (totalSquaredCollection != NULL)
		free(totalSquaredCollection);
	totalSquaredCollection = NULL;

	if (absolutePeak != NULL)
		free(absolutePeak);
	absolutePeak = NULL;

	if (GUIcontinualRMS != NULL)
		free(GUIcontinualRMS);
	GUIcontinualRMS = NULL;

	if (GUIcontinualPeak != NULL)
		free(GUIcontinualPeak);
	GUIcontinualPeak = NULL;

	if (GUIshareDataCache != NULL)
		free(GUIshareDataCache);
	GUIshareDataCache = NULL;

	numChannels = 0;
}

//-----------------------------------------------------------------------------------------
// this is called the reset the DSP state (clear buffers, reset counters, etc.)
ComponentResult RMSbuddy::Reset(AudioUnitScope inScope, AudioUnitElement inElement)
{
	// don't continue, our arrays will not be allocated yet! (Reset should only occur when Initialized)
	if ( !IsInitialized() )
		return kAudioUnitErr_Uninitialized;

	// reset all of these things
	resetRMS();
	resetPeak();
	// reset continual values, too (the above functions only reset the average/absolute values)
	for (unsigned long ch=0; ch < numChannels; ch++)
	{
		GUIshareDataCache[ch].continualRMS = 0.0;
		GUIshareDataCache[ch].continualPeak = 0.0f;
	}

	resetGUIcounters();
	notifyGUI();	// make sure that the GUI catches these changes

	return noErr;
}

//-----------------------------------------------------------------------------------------
// reset the average RMS-related values and restart calculation of average RMS
void RMSbuddy::resetRMS()
{
	totalSamples = 0;
	for (unsigned long ch=0; ch < numChannels; ch++)
	{
		totalSquaredCollection[ch] = 0.0;
		averageRMS[ch] = 0.0;

		GUIshareDataCache[ch].averageRMS = 0.0;
	}
}

//-----------------------------------------------------------------------------------------
// reset the absolute peak-related values and restart calculation of absolute peak
void RMSbuddy::resetPeak()
{
	for (unsigned long ch=0; ch < numChannels; ch++)
	{
		absolutePeak[ch] = 0.0f;

		GUIshareDataCache[ch].absolutePeak = 0.0f;
	}
}

//-----------------------------------------------------------------------------------------
// reset the GUI-related continual values
void RMSbuddy::resetGUIcounters()
{
	GUIsamplesCounter = 0;
	for (unsigned long ch=0; ch < numChannels; ch++)
	{
		GUIcontinualRMS[ch] = 0.0;
		GUIcontinualPeak[ch] = 0.0f;
	}
}

//-----------------------------------------------------------------------------------------
// post notification to the GUI that it's time to re-fetch data and refresh its display
void RMSbuddy::notifyGUI()
{
	// we use a parameter chane notification (using the fake parameter) rather than a property change notification 
	// because property change notifications cause immediate callbacks which should not be done from audio threads, 
	// whereas parameter change notifications enter a queue that is checked periodically by parameter listeners, 
	// and therefore return immediately and are fine for audio threads
	AudioUnitParameter messangerParam;
	messangerParam.mAudioUnit = GetComponentInstance();
	messangerParam.mParameterID = kTimeToUpdate;
	messangerParam.mScope = kAudioUnitScope_Global;
	messangerParam.mElement = 0;
	AUParameterListenerNotify(NULL, NULL, &messangerParam);
}

//-----------------------------------------------------------------------------------------
ComponentResult RMSbuddy::GetParameterInfo(AudioUnitScope inScope, 
						AudioUnitParameterID inParameterID, AudioUnitParameterInfo &outParameterInfo)
{
	// the size, in ms, of the RMS and peak analysis frame / refresh rate
	if (inParameterID == kAnalysisFrameSize)
	{
		strcpy(outParameterInfo.name, "analysis window");
		outParameterInfo.unit = kAudioUnitParameterUnit_Milliseconds;
		outParameterInfo.minValue = 30.0f;
		outParameterInfo.maxValue = 1000.0f;
		outParameterInfo.defaultValue = 69.0f;
		outParameterInfo.flags = kAudioUnitParameterFlag_IsReadable 
								| kAudioUnitParameterFlag_IsWritable;
		return noErr;
	}

	// a fake parameter (really an audio thread GUI notification mechanism)
	if (inParameterID == kTimeToUpdate)
	{
		outParameterInfo.flags = 0;
		return noErr;
	}

	return kAudioUnitErr_InvalidParameter;
}

//-----------------------------------------------------------------------------------------
// get the current value of a parameter
ComponentResult RMSbuddy::GetParameter(AudioUnitParameterID inParameterID, AudioUnitScope inScope, 
										AudioUnitElement inElement, Float32 &outValue)
{
	// it's a fake parameter, but if we don't at least say noErr for this one, the parameter listener system won't work
	if (inParameterID == kTimeToUpdate)
		return noErr;

	// let the AUBase systems handle our real parameter
	else if (inParameterID == kAnalysisFrameSize)
		return AUBase::GetParameter(inParameterID, inScope, inElement, outValue);

	else
		return kAudioUnitErr_InvalidParameter;
}

//-----------------------------------------------------------------------------------------
// get the details about a property
ComponentResult RMSbuddy::GetPropertyInfo(AudioUnitPropertyID inID, AudioUnitScope inScope, 
						AudioUnitElement inElement, UInt32 &outDataSize, Boolean &outWritable)
{
	switch (inID)
	{
		case kDynamicsDataProperty:
			outDataSize = sizeof(DynamicsData);
			outWritable = false;
			return noErr;

		case kResetRMSProperty:
		case kResetPeakProperty:
			outDataSize = sizeof(char);	// whatever, input data isn't actually needed to set these properties
			outWritable = true;
			return noErr;

		case kNumChannelsProperty:
			outDataSize = sizeof(unsigned long);
			outWritable = false;
			return noErr;

		// let non-custom properties fall through to the parent class' handler
		default:
			return AUInlineEffectBase::GetPropertyInfo(inID, inScope, inElement, outDataSize, outWritable);
	}
}

//-----------------------------------------------------------------------------------------
// get the value/data of a property
ComponentResult RMSbuddy::GetProperty(AudioUnitPropertyID inID, AudioUnitScope inScope, 
						AudioUnitElement inElement, void *outData)
{
	switch (inID)
	{
		// get the current dynamics analysis data for a specified audio channel
		case kDynamicsDataProperty:
			{
				if (outData == NULL)
					return kAudioUnitErr_InvalidPropertyValue;
				if ( !IsInitialized() )
					return kAudioUnitErr_Uninitialized;

				unsigned long requestedChannel = ((DynamicsData*)outData)->channel;
				// invalid channel number requested
				if (requestedChannel >= numChannels)
					return kAudioUnitErr_InvalidPropertyValue;
				// if we got this far, all is good, copy the data for the requester
				memcpy(outData, &(GUIshareDataCache[requestedChannel]), sizeof(DynamicsData));
			}
			return noErr;

		// this has no actual data, it's used for event messages
		case kResetRMSProperty:
			return noErr;

		// this has no actual data, it's used for event messages
		case kResetPeakProperty:
			return noErr;

		// get the number of audio channels being analyzed
		case kNumChannelsProperty:
			*((unsigned long*)outData) = numChannels;
			return noErr;

		// let non-custom properties fall through to the parent class' handler
		default:
			return AUInlineEffectBase::GetProperty(inID, inScope, inElement, outData);
	}
}

//-----------------------------------------------------------------------------------------
// set the value/data of a property
ComponentResult RMSbuddy::SetProperty(AudioUnitPropertyID inID, AudioUnitScope inScope, 
						AudioUnitElement inElement, const void *inData, UInt32 inDataSize)
{
	switch (inID)
	{
		case kDynamicsDataProperty:
			return kAudioUnitErr_PropertyNotWritable;

		// trigger the resetting of average RMS
		case kResetRMSProperty:
			resetRMS();
			return noErr;

		// trigger the resetting of absolute peak
		case kResetPeakProperty:
			resetPeak();
			return noErr;

		case kNumChannelsProperty:
			return kAudioUnitErr_PropertyNotWritable;

		// let non-custom properties fall through to the parent class' handler
		default:
			return AUInlineEffectBase::SetProperty(inID, inScope, inElement, inData, inDataSize);
	}
}

//-----------------------------------------------------------------------------------------
// indicate how many custom GUI components are recommended for this AU
int RMSbuddy::GetNumCustomUIComponents()
{
	return 1;
}

//-----------------------------------------------------------------------------------------
// give a Component description of the GUI component(s) that we recommend for this AU
void RMSbuddy::GetUIComponentDescs(ComponentDescription *inDescArray)
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
OSStatus RMSbuddy::ProcessBufferLists(AudioUnitRenderActionFlags &ioActionFlags, 
						const AudioBufferList &inBuffer, AudioBufferList &outBuffer, 
						UInt32 inFramesToProcess)
{
	// bad number of input channels
	if (inBuffer.mNumberBuffers < numChannels)
		return kAudioUnitErr_FormatNotSupported;


	// increment the sample counter for the total number of samples since (re)starting average RMS analysis
	totalSamples += inFramesToProcess;
	double invTotalSamples = 1.0 / (double)totalSamples;	// it's slightly more efficient to only divide once

	// increment the sample counter for the GUI displays
	GUIsamplesCounter += inFramesToProcess;

	// loop through each channel
	for (unsigned long ch=0; ch < numChannels; ch++)
	{
		// manage the buffer list data, get pointer to the audio input stream
		float *in = (float*) (inBuffer.mBuffers[ch].mData);
		outBuffer.mBuffers[ch].mDataByteSize = inFramesToProcess * sizeof(Float32);

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
		GUIcontinualRMS[ch] += continualRMS;
		// update the GUI continual peak values, if it has been exceeded
		if (continualPeak > GUIcontinualPeak[ch])
			GUIcontinualPeak[ch] = continualPeak;
	}
	// end of per-channel loop


	// figure out if it's time to tell the GUI to refresh its display
	unsigned long analysisFrame = (unsigned long) (AUEffectBase::GetParameter(kAnalysisFrameSize) * GetSampleRate() * 0.001);
	unsigned long nextCount = GUIsamplesCounter + inFramesToProcess;	// estimate the size after the next processing frame
	if ( (GUIsamplesCounter > analysisFrame) || 
			(abs(GUIsamplesCounter-analysisFrame) < abs(nextCount-analysisFrame)) )	// round
	{
		double invGUItotal = 1.0 / (double)GUIsamplesCounter;	// it's slightly more efficient to only divide once
		// store the current dynamics data into the GUI data share caches for each channel...
		for (unsigned long ch=0; ch < numChannels; ch++)
		{
			GUIshareDataCache[ch].averageRMS = averageRMS[ch];
			GUIshareDataCache[ch].continualRMS = sqrt(GUIcontinualRMS[ch] * invGUItotal);
			GUIshareDataCache[ch].absolutePeak = absolutePeak[ch];
			GUIshareDataCache[ch].continualPeak = GUIcontinualPeak[ch];
		}

		// ... and then post notification to the GUI
		notifyGUI();
		// now that we've posted notification, restart the GUI analysis cycle
		resetGUIcounters();
	}


	return noErr;
}
