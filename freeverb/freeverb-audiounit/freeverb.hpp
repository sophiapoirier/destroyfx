// Freeverb3 user interface declaration
// Based on Apple Audio Unit Development Kit Examples
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// This code is public domain
// 
// Audio Unit implementation written by Sophia Poirier, September 2002
// http://destroyfx.org/


#ifndef __FREEVERB_H
#define __FREEVERB_H


#include "AUEffectBase.h"
#include "revmodel.hpp"
#include "freeverb-au-def.h"


enum
{
	KMode, KRoomSize, KDamp, KWidth, KWet, KDry,
	KNumParams
};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class Freeverb : public AUEffectBase
{
public:
	Freeverb(AudioUnit component);
	virtual ~Freeverb();

	virtual ComponentResult GetParameterInfo(AudioUnitScope inScope, 
								AudioUnitParameterID inParameterID, 
								AudioUnitParameterInfo & outParameterInfo);
	virtual ComponentResult SetParameter(AudioUnitParameterID inID, AudioUnitScope inScope,
								AudioUnitElement inElement, Float32 inValue, UInt32 inBufferOffsetInFrames);

	virtual UInt32 SupportedNumChannels(const AUChannelInfo ** outInfo);
	virtual ComponentResult	Version()
		{	return PLUGIN_VERSION;	}

	virtual ComponentResult Reset(AudioUnitScope inScope, AudioUnitElement inElement);
	virtual OSStatus ProcessBufferLists(AudioUnitRenderActionFlags & ioActionFlags, 
						const AudioBufferList & inBuffer, AudioBufferList & outBuffer, 
						UInt32 inFramesToProcess);

private:
	revmodel model;
	bool needUpdate;
};


#endif