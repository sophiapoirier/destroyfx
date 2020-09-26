// Freeverb3 user interface implementation
// Based on Apple Audio Unit Development Kit Examples
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// This code is public domain
// 
// Audio Unit implementation written by Sophia Poirier, September 2002
// http://destroyfx.org/

#include "freeverb.hpp"



#pragma mark ___init___

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENT_ENTRY(Freeverb)

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Freeverb::Freeverb(AudioUnit component)
	: AUInlineEffectBase(component)
{
	// initialize the parameters to their default values
	for (long index=0; index < KNumParams; index++)
	{
		AudioUnitParameterInfo paramInfo;
		if (GetParameterInfo(kAudioUnitScope_Global, index, paramInfo) == noErr)
			AUEffectBase::SetParameter(index, paramInfo.defaultValue);
	}

    Reset(kAudioUnitScope_Global, (AudioUnitElement)0);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Freeverb::~Freeverb()
{
	// nothing here
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ComponentResult Freeverb::Reset(AudioUnitScope inScope, AudioUnitElement inElement)
{
	model.mute();
	needUpdate = true;

	return noErr;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ComponentResult Freeverb::GetParameterInfo(AudioUnitScope inScope, AudioUnitParameterID inParameterID,
											AudioUnitParameterInfo &outParameterInfo)
{
	if (inScope != kAudioUnitScope_Global)
		return kAudioUnitErr_InvalidScope;

	ComponentResult result = noErr;

	outParameterInfo.flags = kAudioUnitParameterFlag_IsReadable 
							| kAudioUnitParameterFlag_IsWritable;

	switch (inParameterID)
	{
		case KMode:
			strcpy(outParameterInfo.name, "Mode");
			outParameterInfo.unit = kAudioUnitParameterUnit_Indexed;
			outParameterInfo.minValue = 0.0f;
			outParameterInfo.maxValue = 1.0f;
			outParameterInfo.defaultValue = initialmode;
			break;

		case KRoomSize:
			strcpy(outParameterInfo.name, "Room size");
			outParameterInfo.unit = kAudioUnitParameterUnit_Meters;
			outParameterInfo.minValue = offsetroom;
			outParameterInfo.maxValue = offsetroom + scaleroom;
			outParameterInfo.defaultValue = (scaleroom * initialroom) + offsetroom;
			break;

		case KDamp:
			strcpy(outParameterInfo.name, "Damping");
			outParameterInfo.unit = kAudioUnitParameterUnit_Percent;
			outParameterInfo.minValue = 0.0f;
			outParameterInfo.maxValue = 100.0f;
			outParameterInfo.defaultValue = initialdamp * 100.0f;
			break;

		case KWet:
			strcpy(outParameterInfo.name, "Wet level");
//			outParameterInfo.unit = kAudioUnitParameterUnit_Decibels;
			outParameterInfo.unit = kAudioUnitParameterUnit_LinearGain;
			outParameterInfo.minValue = 0.0f;
			outParameterInfo.maxValue = scalewet;
			outParameterInfo.defaultValue = initialwet;
			break;

		case KDry:
			strcpy(outParameterInfo.name, "Dry level");
//			outParameterInfo.unit = kAudioUnitParameterUnit_Decibels;
			outParameterInfo.unit = kAudioUnitParameterUnit_LinearGain;
			outParameterInfo.minValue = 0.0f;
			outParameterInfo.maxValue = scaledry;
			outParameterInfo.defaultValue = initialdry;
			break;

		case KWidth:
			strcpy(outParameterInfo.name, "Width");
			outParameterInfo.unit = kAudioUnitParameterUnit_Percent;
			outParameterInfo.minValue = 0.0f;
			outParameterInfo.maxValue = 100.0f;
			outParameterInfo.defaultValue = initialwidth * 100.0f;
			break;

		default:
			result = kAudioUnitErr_InvalidParameter;
			break;
	}
	
	return result;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// set up text values for the 2 states of the mode parameter
ComponentResult Freeverb::GetParameterValueStrings(AudioUnitScope inScope, AudioUnitParameterID inParameterID, 
													CFArrayRef *outStrings)
{
	if (inScope != kAudioUnitScope_Global)
		return kAudioUnitErr_InvalidScope;

	if (inParameterID == KMode)
	{
		*outStrings = CFStringCreateArrayBySeparatingStrings( NULL, CFSTR("Normal.Freeze"), CFSTR(".") );
		return noErr;
	}

	return kAudioUnitErr_InvalidProperty;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ComponentResult Freeverb::SetParameter(AudioUnitParameterID inID, AudioUnitScope inScope,
									 AudioUnitElement inElement, Float32 inValue, UInt32 inBufferOffsetInFrames)
{
	if (inScope == kAudioUnitScope_Global)
	{
		// when these parameters change, the reverb model needs to recalculate some values
		switch (inID)
		{
			case KRoomSize:
			case KDamp:
			case KWet:
			case KWidth:
			case KMode:
				needUpdate = true;
				break;
		}
	}

	return AUBase::SetParameter(inID, inScope, inElement, inValue, inBufferOffsetInFrames);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// state that Freeverb supports only stereo-in/stereo-out processing
UInt32 Freeverb::SupportedNumChannels(const AUChannelInfo **outInfo)
{
	if (outInfo)
	{
		static AUChannelInfo info;
		info.inChannels = 2;
		info.outChannels = 2;
		*outInfo = &info;
	}

	return 1;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// this is where the audio processing is done
OSStatus Freeverb::ProcessBufferLists(AudioUnitRenderActionFlags &ioActionFlags, 
								const AudioBufferList &inBuffer, AudioBufferList &outBuffer, 
								UInt32 inFramesToProcess)
{
	// update internal parameter values
	model.setmode(GetParameter(KMode));
	model.setroomsize(GetParameter(KRoomSize));
	model.setdamp(GetParameter(KDamp));
	model.setwet(GetParameter(KWet));
	model.setdry(GetParameter(KDry));
	model.setwidth(GetParameter(KWidth));
	if (needUpdate)
		model.update();
	needUpdate = false;


	UInt32 inNumBuffers = inBuffer.mNumberBuffers;
	UInt32 outNumBuffers = outBuffer.mNumberBuffers;
	float *in1, *in2, *out1, *out2;

	// can't have less than 1 in or out stream (not likely to happen; that would be ridiculous)
	if ( (inNumBuffers < 1) || (outNumBuffers < 1) )
		return kAudioUnitErr_FormatNotSupported;

	in1 = (float*)inBuffer.mBuffers[0].mData;
	out1 = (float*)outBuffer.mBuffers[0].mData;

	// set up for either stereo or "double mono" processing
	if (inNumBuffers >= 2)
		in2 = (float*)inBuffer.mBuffers[1].mData;
	else
		in2 = in1;

	outBuffer.mBuffers[0].mDataByteSize = inFramesToProcess * sizeof(Float32);
	if (outNumBuffers >= 2)
	{
		out2 =  (float*)outBuffer.mBuffers[1].mData;
		outBuffer.mBuffers[1].mDataByteSize = inFramesToProcess * sizeof(Float32);
	}
	else
		out2 = out1;

	// now do the processing
	model.processreplace(in1, in2, out1, out2, inFramesToProcess, 1);

	// I don't know what the hell this is for
	ioActionFlags &= ~kAudioUnitRenderAction_OutputIsSilence;
	
	return noErr;
}
