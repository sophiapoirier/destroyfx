/*------------------ by Marc Poirier  ][  January 2001 -----------------*/

#ifndef __POLARIZER_H
#define __POLARIZER_H

#ifndef __DFXPLUGIN_H
#include "dfxplugin.h"
#endif


// these are the plugin parameters:
enum
{
	kSkip,
	kAmount,
	kImplode,

	NUM_PARAMETERS
};

//----------------------------------------------------------------------------- 
// constants

#define SKIPMIN 1
#define SKIPMAX 81
// this is for converting from parameter entries to the real values
#define leapScaled(A)   ( (long)(powf((A),1.5f)*(float)(SKIPMAX-SKIPMIN)) + SKIPMIN )


//----------------------------------------------------------------------------- 
class PolarizerDSP : public DfxPluginCore
{
public:
	PolarizerDSP(TARGET_API_CORE_INSTANCE_TYPE *inInstance);
	virtual void process(const float *in, float *out, unsigned long inNumFrames, bool replacing=true);
	virtual void reset();
	virtual void processparameters();

private:
	// the parameters
	long leapSize;
	float polarizedAmp;
	bool implode;

	long unaffectedSamples;	// sample counter
};

//----------------------------------------------------------------------------- 
class Polarizer : public DfxPlugin
{
public:
	Polarizer(TARGET_API_BASE_INSTANCE_TYPE inInstance);
};



#endif
