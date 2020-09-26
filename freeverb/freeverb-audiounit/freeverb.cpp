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



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMPONENT_ENTRY(Freeverb)

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Freeverb::Freeverb(AudioUnit component)
	: AUEffectBase(component, true)
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
											AudioUnitParameterInfo & outParameterInfo)
{
	if (inScope != kAudioUnitScope_Global)
		return kAudioUnitErr_InvalidScope;

	ComponentResult result = noErr;

	outParameterInfo.flags = kAudioUnitParameterFlag_IsReadable 
							| kAudioUnitParameterFlag_IsWritable
							| kAudioUnitParameterFlag_HasCFNameString;

#define INIT_AU_PARAM(paramID, cstr, unitID, min, max, def) \
	case (paramID):	\
		strcpy(outParameterInfo.name, (cstr));	\
		outParameterInfo.cfNameString = CFSTR(cstr);	\
		outParameterInfo.unit = kAudioUnitParameterUnit_##unitID;	\
		outParameterInfo.minValue = (min);	\
		outParameterInfo.maxValue = (max);	\
		outParameterInfo.defaultValue = (def);	\
		break;
	switch (inParameterID)
	{
		INIT_AU_PARAM(KMode, "Freeze", Boolean, 0.0f, 1.0f, initialmode);
		INIT_AU_PARAM(KRoomSize, "Room size", Meters, offsetroom, offsetroom + scaleroom, (scaleroom * initialroom) + offsetroom);
		INIT_AU_PARAM(KDamp, "Damping", Percent, 0.0f, 100.0f, initialdamp * 100.0f);
		INIT_AU_PARAM(KWet, "Wet level", LinearGain, 0.0f, scalewet, initialwet);
		INIT_AU_PARAM(KDry, "Dry level", LinearGain, 0.0f, scaledry, initialdry);
		INIT_AU_PARAM(KWidth, "Width", Percent, 0.0f, 100.0f, initialwidth * 100.0f);

		default:
			result = kAudioUnitErr_InvalidParameter;
			break;
	}
#undef INIT_AU_PARAM

	if ( (inParameterID == KWet) || (inParameterID == KDry) )
		outParameterInfo.flags |= kAudioUnitParameterFlag_DisplayCubed;

	return result;
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
UInt32 Freeverb::SupportedNumChannels(const AUChannelInfo ** outInfo)
{
	if (outInfo != NULL)
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
OSStatus Freeverb::ProcessBufferLists(AudioUnitRenderActionFlags & ioActionFlags, 
								const AudioBufferList & inBuffer, AudioBufferList & outBuffer, 
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


	float * in1 = (float*)(inBuffer.mBuffers[0].mData);
	float * in2 = (float*)(inBuffer.mBuffers[1].mData);
	float * out1 = (float*)(outBuffer.mBuffers[0].mData);
	float * out2 = (float*)(outBuffer.mBuffers[1].mData);

	// now do the processing
	model.processreplace(in1, in2, out1, out2, inFramesToProcess, 1);

	// I don't know what the hell this is for
	ioActionFlags &= ~kAudioUnitRenderAction_OutputIsSilence;
	
	return noErr;
}
