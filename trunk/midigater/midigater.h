/*-------------- by Sophia Poirier  ][  November 2001 -------------*/

#ifndef __MIDIGATER_H
#define __MIDIGATER_H

#include "dfxplugin.h"


//----------------------------------------------------------------------------- 
// these are the plugin parameters:
enum
{
	kAttackSlope,
	kReleaseSlope,
	kVelInfluence,
	kFloor,

	kNumParameters
};


//----------------------------------------------------------------------------- 
class MidiGater : public DfxPlugin
{
public:
	MidiGater(TARGET_API_BASE_INSTANCE_TYPE inInstance);

	virtual void reset();
	virtual void processparameters();
	virtual void processaudio(const float ** inAudio, float ** outAudio, unsigned long inNumFrames, bool inReplacing=true);

private:
	void processUnaffected(const float ** inAudio, float ** outAudio, long inNumFramesToProcess, long inOffset, unsigned long inNumChannels);

	float attackSlope_seconds, releaseSlope_seconds, velInfluence, floor;	// parameter values
	int unaffectedState, unaffectedFadeSamples;
};


#endif