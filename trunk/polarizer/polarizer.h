/*------------------ by Marc Poirier  ][  January 2001 -----------------*/

#ifndef __POLARIZER_H
#define __POLARIZER_H

#include "dfxplugin.h"


// these are Polarizer's parameters
enum
{
	kSkip,
	kAmount,
	kImplode,

	NUM_PARAMETERS
};


//----------------------------------------------------------------------------- 
class PolarizerDSP : public DfxPluginCore
{
public:
	PolarizerDSP(DfxPlugin *inDfxPlugin);
	virtual void process(const float *in, float *out, unsigned long inNumFrames, bool replacing=true);
	virtual void reset();

private:
	long unaffectedSamples;	// sample counter
};

//----------------------------------------------------------------------------- 
class Polarizer : public DfxPlugin
{
public:
	Polarizer(TARGET_API_BASE_INSTANCE_TYPE inInstance);
};


#endif
