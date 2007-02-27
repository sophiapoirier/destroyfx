/*------------------ by Sophia Poirier  ][  January 2001 -----------------*/

#ifndef __EQSYNC_H
#define __EQSYNC_H


#include "dfxplugin.h"


//----------------------------------------------------------------------------- 
// these are the plugin parameters:
enum
{
	kRate_sync,
	kSmooth,
	kTempo,
	kTempoAuto,
	ka0,
	ka1,
	ka2,
	kb1,
	kb2,

	kNumParameters
};


//----------------------------------------------------------------------------- 

class EQSync : public DfxPlugin
{
public:
	EQSync(TARGET_API_BASE_INSTANCE_TYPE inInstance);
	virtual ~EQSync();

	virtual void reset();
	virtual void processparameters();
	virtual void processaudio(const float ** in, float ** out, unsigned long inNumFrames, bool replacing=true);

	virtual bool createbuffers();
	virtual void releasebuffers();
	virtual void clearbuffers();


private:
	// the parameters
	double rate, smooth, userTempo;
	bool useHostTempo;
	float a0, a1, a2, b1, b2;

	long cycleSamples, smoothSamples, smoothDur;	// sample counters
	float * prevIn, * prevprevIn;	// these store the previous input samples' values
	float * prevOut, * prevprevOut;	// these store the previous output samples' values
	float preva0, preva1, preva2, prevb1, prevb2;	// these store the last random filter parameter values
	float cura0, cura1, cura2, curb1, curb2;	//these store the current random filter parameter values

	double currentTempoBPS;	// tempo in beats per second
	bool needResync;	// true when playback has just started up again

	unsigned long numBuffers;
};

#endif
