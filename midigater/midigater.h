/*-------------- by Marc Poirier  ][  November 2001 -------------*/

#ifndef __MIDIGATER_H
#define __MIDIGATER_H

#include "dfxplugin.h"


//----------------------------------------------------------------------------- 
// enums

// these are the plugin parameters:
enum
{
	kSlope,
	kVelInfluence,
	kFloor,

	NUM_PARAMETERS
};

// these are the 3 states of the unaffected audio input between notes
enum
{
	unFadeIn,
	unFlat,
	unFadeOut
};


//----------------------------------------------------------------------------- 
// constants
#define SLOPE_MAX 3000.0f
#define UNAFFECTED_FADE_DUR 18
const float UNAFFECTED_FADE_STEP = 1.0f / (float)UNAFFECTED_FADE_DUR;


//----------------------------------------------------------------------------- 

class MidiGater : public DfxPlugin
{
public:
	MidiGater(TARGET_API_BASE_INSTANCE_TYPE inInstance);
	virtual ~MidiGater();

	virtual void reset();
	virtual void processparameters();
	virtual void processaudio(const float **in, float **out, unsigned long inNumFrames, bool replacing=true);

private:
	void processUnaffected(const float **in, float **out, long numFramesToProcess, long offset, unsigned long numChannels);

	float slope_seconds, velInfluence, floor;	// parameters
	int unaffectedState, unaffectedFadeSamples;
};

#endif