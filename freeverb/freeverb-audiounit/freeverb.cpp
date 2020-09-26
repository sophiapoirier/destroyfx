// Freeverb3 user interface implementation
// Based on Apple Audio Unit Development Kit Examples
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk/
// This code is public domain
// 
// Audio Unit implementation written by Sophia Poirier, September 2002, May 2016
// http://destroyfx.org/


#include "freeverb.h"

#include <array>


using namespace freeverb;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static constexpr size_t kMonoChannelCount = 1;
static constexpr size_t kStereoChannelCount = 2;

enum : AudioUnitParameterID
{
	kParam_FreezeMode,
	kParam_RoomSize,
	kParam_Damping,
	kParam_Width,
	kParam_WetLevel,
	kParam_DryLevel,
	kNumParams
};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
AUDIOCOMPONENT_ENTRY(AUBaseProcessFactory, FreeverbAU)

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
FreeverbAU::FreeverbAU(AudioComponentInstance inComponentInstance)
:	AUEffectBase(inComponentInstance)
{
	// initialize the parameters to their default values
	for (AudioUnitParameterID i = 0; i < kNumParams; i++)
	{
		AudioUnitParameterInfo paramInfo;
		if (GetParameterInfo(kAudioUnitScope_Global, i, paramInfo) == noErr)
		{
			SetParameter(i, paramInfo.defaultValue);
		}
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus FreeverbAU::Initialize()
{
	const auto status = AUEffectBase::Initialize();

	if (status == noErr)
	{
		mModel = std::make_unique<ReverbModel>(GetSampleRate());
		Reset(kAudioUnitScope_Global, AudioUnitElement(0));
	}

	return status;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus FreeverbAU::Reset(AudioUnitScope inScope, AudioUnitElement inElement)
{
	if (mModel)
	{
		mModel->clear();
	}

	mInputSilentSoFar = true;

	return noErr;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus FreeverbAU::GetParameterInfo(AudioUnitScope inScope, AudioUnitParameterID inParameterID,
									  AudioUnitParameterInfo& outParameterInfo)
{
	if (inScope != kAudioUnitScope_Global)
	{
		return kAudioUnitErr_InvalidScope;
	}

	outParameterInfo.flags = kAudioUnitParameterFlag_IsReadable 
							| kAudioUnitParameterFlag_IsWritable;

	const auto initParam = [&](CFStringRef name, AudioUnitParameterUnit unitID,
							   AudioUnitParameterValue minValue, AudioUnitParameterValue maxValue, 
							   AudioUnitParameterValue defaultValue, bool curveValueRange)
	{
		FillInParameterName(outParameterInfo, name, false);
		outParameterInfo.unit = unitID;
		outParameterInfo.minValue = minValue;
		outParameterInfo.maxValue = maxValue;
		outParameterInfo.defaultValue = defaultValue;
		if (curveValueRange)
		{
			outParameterInfo.flags |= kAudioUnitParameterFlag_DisplayCubeRoot;
		}
	};

	switch (inParameterID)
	{
		case kParam_FreezeMode:
			initParam(CFSTR("Freeze"), kAudioUnitParameterUnit_Boolean, 0.0f, 1.0f, kFreezeModeDefault ? 1.0f : 0.0f, false);
			break;
		case kParam_RoomSize:
			initParam(CFSTR("Room Size"), kAudioUnitParameterUnit_Generic, kRoomSizeMin, kRoomSizeMax, kRoomSizeDefault, false);
			break;
		case kParam_Damping:
			initParam(CFSTR("Damping"), kAudioUnitParameterUnit_Percent, 0.0f, 100.0f, kDampingDefault * 100.0f, false);
			break;
		case kParam_WetLevel:
			initParam(CFSTR("Wet Level"), kAudioUnitParameterUnit_LinearGain, 0.0f, kWetLevelMax, kWetLevelDefault, true);
			break;
		case kParam_DryLevel:
			initParam(CFSTR("Dry Level"), kAudioUnitParameterUnit_LinearGain, 0.0f, kDryLevelMax, kDryLevelDefault, true);
			break;
		case kParam_Width:
			initParam(CFSTR("Width"), kAudioUnitParameterUnit_Percent, 0.0f, 100.0f, kWidthDefault * 100.0f, false);
			break;
		default:
			return kAudioUnitErr_InvalidParameter;
	}

	return noErr;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// state that Freeverb supports only mono or stereo-in/stereo-out processing
UInt32 FreeverbAU::SupportedNumChannels(const AUChannelInfo** outInfo)
{
	static constexpr std::array<AUChannelInfo, 2> channelInfo = 
	{{
		{ kMonoChannelCount, kMonoChannelCount },
		{ kStereoChannelCount, kStereoChannelCount }
	}};

	if (outInfo != nullptr)
	{
		*outInfo = channelInfo.data();
	}
	return channelInfo.size();
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// this is where the audio processing is done
OSStatus FreeverbAU::ProcessBufferLists(AudioUnitRenderActionFlags& ioActionFlags, 
										const AudioBufferList& inBuffer, AudioBufferList& outBuffer, 
										UInt32 inFramesToProcess)
{
	// update internal parameter values
	mModel->setFreezeMode(GetParameter(kParam_FreezeMode));
	mModel->setRoomSize(GetParameter(kParam_RoomSize));
	mModel->setDamping(GetParameter(kParam_Damping) * 0.01f);
	mModel->setWetLevel(GetParameter(kParam_WetLevel));
	mModel->setDryLevel(GetParameter(kParam_DryLevel));
	mModel->setWidth(GetParameter(kParam_Width) * 0.01f);

	if ((inBuffer.mNumberBuffers >= kStereoChannelCount) && (outBuffer.mNumberBuffers >= kStereoChannelCount))
	{
		const auto in1 = static_cast<const float*>(inBuffer.mBuffers[0].mData);
		const auto in2 = static_cast<const float*>(inBuffer.mBuffers[1].mData);
		auto out1 = static_cast<float*>(outBuffer.mBuffers[0].mData);
		auto out2 = static_cast<float*>(outBuffer.mBuffers[1].mData);
		mModel->process(in1, in2, out1, out2, inFramesToProcess);
	}
	else
	{
		const auto in = static_cast<const float*>(inBuffer.mBuffers[0].mData);
		auto out = static_cast<float*>(outBuffer.mBuffers[0].mData);
		mModel->process(in, out, inFramesToProcess);
	}

	if (!(ioActionFlags & kAudioUnitRenderAction_OutputIsSilence))
	{
		mInputSilentSoFar = false;
	}
	if (!mInputSilentSoFar)
	{
		ioActionFlags &= ~kAudioUnitRenderAction_OutputIsSilence;
	}
	
	return noErr;
}
