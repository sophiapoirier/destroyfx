/*------------------ by Marc Poirier  ][  January 2001 -----------------*/

#ifndef __EQSYNC_H
#define __EQSYNC_H


#ifndef __DFXPLUGIN_H
#include "dfxplugin.h"
#endif


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

	NUM_PARAMETERS
};

//----------------------------------------------------------------------------- 
// constants

const float TEMPO_MIN = 39.0f;
const float TEMPO_MAX = 480.0f;

// this is for converting from parameter entries to the real values
#define tempoScaled(A) ( paramRangeScaled((A), TEMPO_MIN, TEMPO_MAX) )
#define tempoUnscaled(A) ( paramRangeUnscaled((A), TEMPO_MIN, TEMPO_MAX) )


//----------------------------------------------------------------------------- 

class EQsync : public DfxPlugin
{
friend class EQsyncEditor;
public:
	EQsync(TARGET_API_BASE_INSTANCE_TYPE inInstance);
	virtual ~EQsync();

	virtual void reset();
	virtual void processparameters();
	virtual void processaudio(const float **in, float **out, unsigned long inNumFrames, bool replacing=true);

	virtual bool createbuffers();
	virtual void releasebuffers();
	virtual void clearbuffers();


protected:
	// the parameters
	float rate, smooth, userTempo;
	bool useHostTempo;
	float a0, a1, a2, b1, b2;

	long cycleSamples, smoothSamples, smoothDur;	// sample counters
	float *prevIn, *prevprevIn;	// these store the previous input samples' values
	float *prevOut, *prevprevOut;	// these store the previous output samples' values
	float preva0, preva1, preva2, prevb1, prevb2;	// these store the last random filter parameter values
	float cura0, cura1, cura2, curb1, curb2;	//these store the current random filter parameter values

	float currentTempoBPS;	// tempo in beats per second
	bool needResync;	// true when playback has just started up again

	unsigned long numBuffers;
};

#endif
