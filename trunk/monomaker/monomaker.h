/*------------------- by Marc Poirier  ][  March 2001 -------------------*/

#ifndef __MONOMAKER_H
#define __MONOMAKER_H


#ifndef __DFXPLUGIN_H
#include "dfxplugin.h"
#endif


//----------------------------------------------------------------------------- 
// these are the plugin parameters:
enum {
	kMonomerge,
	kMonomergeMode,
	kPan,
	kPanMode,

	NUM_PARAMETERS
};

enum {
	kMonomergeMode_linear,
	kMonomergeMode_equalpower,
	kNumMonomergeModes
};

enum {
	kPanMode_recenter,
	kPanMode_balance,
	kNumPanModes
};


//----------------------------------------------------------------------------- 

class Monomaker : public DfxPlugin
{
public:
	Monomaker(TARGET_API_BASE_INSTANCE_TYPE inInstance);
	virtual void processaudio(const float **in, float **out, unsigned long inNumFrames, bool replacing=true);
};

#endif