// Freeverb3 user interface declaration
// Based on Apple Audio Unit Development Kit Examples
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk/
// This code is public domain
// 
// Audio Unit implementation written by Sophia Poirier, September 2002, May 2016
// http://destroyfx.org/


#pragma once


#include <memory>

#include "AUEffectBase.h"
#include "revmodel.hpp"


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class FreeverbAU : public AUEffectBase
{
public:
	FreeverbAU(AudioComponentInstance inComponentInstance);

	OSStatus Initialize() override;
	OSStatus Reset(AudioUnitScope inScope, AudioUnitElement inElement) override;

	OSStatus GetParameterInfo(AudioUnitScope inScope, 
							  AudioUnitParameterID inParameterID, 
							  AudioUnitParameterInfo& outParameterInfo) override;

	UInt32 SupportedNumChannels(const AUChannelInfo** outInfo) override;

	OSStatus ProcessBufferLists(AudioUnitRenderActionFlags& ioActionFlags, 
								const AudioBufferList& inBuffer, AudioBufferList& outBuffer, 
								UInt32 inFramesToProcess) override;

private:
	std::unique_ptr<freeverb::ReverbModel> mModel;
	bool mInputSilentSoFar = true;
};
