/*--------------- by Marc Poirier  ][  June 2001 + February 2003 --------------*/

#include "rmsbuddy.h"

#include <AudioToolbox/AudioUnitUtilities.h>	// for AUParameterListenerNotify


// macro for boring entry point stuff
COMPONENT_ENTRY(RMSbuddy);

//-----------------------------------------------------------------------------
RMSbuddy::RMSbuddy(AudioUnit component)
	: AUInlineEffectBase(component)
{
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
ComponentResult RMSbuddy::Initialize()
{
	ComponentResult result = AUInlineEffectBase::Initialize();
	if (result == noErr)
		Reset(kAudioUnitScope_Global, (AudioUnitElement)0);
	return result;
}

//-----------------------------------------------------------------------------------------
ComponentResult RMSbuddy::Reset(AudioUnitScope inScope, AudioUnitElement inElement)
{
	// reset all of these things
	resetRMS();
	leftData.continualRMS = rightData.continualRMS = 0.0;

	resetPeak();
	leftData.continualPeak = rightData.continualPeak = 0.0f;

	resetGUIcounters();
	notifyGUI();

	return noErr;
}

//-----------------------------------------------------------------------------------------
void RMSbuddy::resetRMS()
{
	totalSamples = 0;
	totalSquaredCollection1 = totalSquaredCollection2 = 0.0;
	leftAverageRMS = rightAverageRMS = 0.0;

	leftData.averageRMS = rightData.averageRMS = 0.0;
}

//-----------------------------------------------------------------------------------------
void RMSbuddy::resetPeak()
{
	leftAbsolutePeak = rightAbsolutePeak = 0.0f;

	leftData.absolutePeak = rightData.absolutePeak = 0.0f;
}

//-----------------------------------------------------------------------------------------
void RMSbuddy::resetGUIcounters()
{
	GUIsamplesCounter = 0;
	GUIleftContinualRMS = GUIrightContinualRMS = 0.0;
	GUIleftContinualPeak = GUIrightContinualPeak = 0.0f;
}

//-----------------------------------------------------------------------------------------
void RMSbuddy::notifyGUI()
{
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

	if (inParameterID == kTimeToUpdate)
	{
		outParameterInfo.flags = 0;
		return noErr;
	}

	return kAudioUnitErr_InvalidParameter;
}

//-----------------------------------------------------------------------------------------
ComponentResult RMSbuddy::GetParameter(AudioUnitParameterID inParameterID, AudioUnitScope inScope, 
										AudioUnitElement inElement, Float32 &outValue)
{
	if (inParameterID == kTimeToUpdate)
		return noErr;
	else if (inParameterID == kAnalysisFrameSize)
		return AUBase::GetParameter(inParameterID, inScope, inElement, outValue);
	else
		return kAudioUnitErr_InvalidParameter;
}

//-----------------------------------------------------------------------------------------
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
			outDataSize = sizeof(char);
			outWritable = true;
			return noErr;
		default:
			return AUInlineEffectBase::GetPropertyInfo(inID, inScope, inElement, outDataSize, outWritable);
	}
}

//-----------------------------------------------------------------------------------------
ComponentResult RMSbuddy::GetProperty(AudioUnitPropertyID inID, AudioUnitScope inScope, 
						AudioUnitElement inElement, void *outData)
{
	switch (inID)
	{
		case kDynamicsDataProperty:
			{
				if (outData == NULL)
					return kAudioUnitErr_InvalidPropertyValue;

				DynamicsData *request = (DynamicsData*) outData;
				if (request->channel == 0)
					memcpy(request, &leftData, sizeof(DynamicsData));
				else if (request->channel == 1)
					memcpy(request, &rightData, sizeof(DynamicsData));
				else
					return kAudioUnitErr_InvalidPropertyValue;
			}
			return noErr;
		case kResetRMSProperty:
			return noErr;
		case kResetPeakProperty:
			return noErr;
		default:
			return AUInlineEffectBase::GetProperty(inID, inScope, inElement, outData);
	}
}

//-----------------------------------------------------------------------------------------
ComponentResult RMSbuddy::SetProperty(AudioUnitPropertyID inID, AudioUnitScope inScope, 
						AudioUnitElement inElement, const void *inData, UInt32 inDataSize)
{
	switch (inID)
	{
		case kDynamicsDataProperty:
			return kAudioUnitErr_PropertyNotWritable;
		case kResetRMSProperty:
			resetRMS();
			return noErr;
		case kResetPeakProperty:
			resetPeak();
			return noErr;
		default:
			return AUInlineEffectBase::SetProperty(inID, inScope, inElement, inData, inDataSize);
	}
}

//-----------------------------------------------------------------------------------------
UInt32 RMSbuddy::SupportedNumChannels(const AUChannelInfo **outInfo)
{
	if (outInfo != NULL)
	{
		static AUChannelInfo info;
		info.inChannels = 2;
		info.outChannels = 2;
		*outInfo = &info;
	}

	return 1;	// return the number of supported i/o configurations
}

//-----------------------------------------------------------------------------------------
int RMSbuddy::GetNumCustomUIComponents()
{
	return 1;
}

//-----------------------------------------------------------------------------------------
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
OSStatus RMSbuddy::ProcessBufferLists(AudioUnitRenderActionFlags &ioActionFlags, 
						const AudioBufferList &inBuffer, AudioBufferList &outBuffer, 
						UInt32 inFramesToProcess)
{
	if (inBuffer.mNumberBuffers < 2)
		return kAudioUnitErr_FormatNotSupported;

	float *inLeft = (float*) (inBuffer.mBuffers[0].mData);
	float *inRight = (float*) (inBuffer.mBuffers[1].mData);
	outBuffer.mBuffers[0].mDataByteSize = inFramesToProcess * sizeof(Float32);
	outBuffer.mBuffers[1].mDataByteSize = inFramesToProcess * sizeof(Float32);


	double leftContinualRMS = 0.0;
	float leftContinualPeak = 0.0f;
	for (UInt32 i=0; i < inFramesToProcess; i++)
	{
		float inSquared = inLeft[i] * inLeft[i];
		leftContinualRMS += inSquared;
		if (inSquared > leftContinualPeak)
			leftContinualPeak = inSquared;	// by squaring, we don't need to fabs (and can unsquare later)
	}

	double rightContinualRMS = 0.0;
	float rightContinualPeak = 0.0f;
	for (UInt32 i=0; i < inFramesToProcess; i++)
	{
		float inSquared = inRight[i] * inRight[i];
		rightContinualRMS += inSquared;
		if (inSquared > rightContinualPeak)
			rightContinualPeak = inSquared;
	}

	totalSamples += inFramesToProcess;
	totalSquaredCollection1 += leftContinualRMS;
	totalSquaredCollection2 += rightContinualRMS;

	// update the average RMS values
	double invTotal = 1.0 / (double)totalSamples;
	leftAverageRMS = sqrt(totalSquaredCollection1 * invTotal);
	rightAverageRMS = sqrt(totalSquaredCollection2 * invTotal);

	// unsquare these
	leftContinualPeak = (float) sqrt(leftContinualPeak);
	rightContinualPeak = (float) sqrt(rightContinualPeak);
	// update absolute peak values
	if (leftContinualPeak > leftAbsolutePeak)
		leftAbsolutePeak = leftContinualPeak;
	if (rightContinualPeak > rightAbsolutePeak)
		rightAbsolutePeak = rightContinualPeak;


	// increment the RMS informations for the GUI displays
	GUIsamplesCounter += inFramesToProcess;
	GUIleftContinualRMS += leftContinualRMS;
	GUIrightContinualRMS += rightContinualRMS;
	// update the GUI continual peak values
	if (leftContinualPeak > GUIleftContinualPeak)
		GUIleftContinualPeak = leftContinualPeak;
	if (rightContinualPeak > GUIrightContinualPeak)
		GUIrightContinualPeak = rightContinualPeak;

	// figure out if it's time to tell the GUI to refresh its display
	unsigned long analysisFrame = (unsigned long) (AUEffectBase::GetParameter(kAnalysisFrameSize) * GetSampleRate() * 0.001);
	unsigned long nextCount = GUIsamplesCounter + inFramesToProcess;	// estimate the size after the next processing frame
	if ( (GUIsamplesCounter > analysisFrame) || 
			(abs(GUIsamplesCounter-analysisFrame) < abs(nextCount-analysisFrame)) )	// round
	{
		double invGUItotal = 1.0 / (double)GUIsamplesCounter;
		leftData.averageRMS = leftAverageRMS;
		leftData.continualRMS = sqrt(GUIleftContinualRMS * invGUItotal);
		leftData.absolutePeak = leftAbsolutePeak;
		leftData.continualPeak = GUIleftContinualPeak;
		rightData.averageRMS = rightAverageRMS;
		rightData.continualRMS = sqrt(GUIrightContinualRMS * invGUItotal);
		rightData.absolutePeak = rightAbsolutePeak;
		rightData.continualPeak = GUIrightContinualPeak;

		resetGUIcounters();
		notifyGUI();
	}


	return noErr;
}
