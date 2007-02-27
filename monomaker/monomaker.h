/*------------------- by Sophia Poirier  ][  March 2001 -------------------*/

#ifndef __MONOMAKER_H
#define __MONOMAKER_H


#include "dfxplugin.h"


//----------------------------------------------------------------------------- 
// these are the plugin parameters:
enum {
	kInputSelection,
	kMonomerge,
	kMonomergeMode,
	kPan,
	kPanMode,
//	kPanLaw,

	kNumParameters
};

enum {
	kInputSelection_stereo,
	kInputSelection_swap,
	kInputSelection_left,
	kInputSelection_right,
	kNumInputSelections
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
	virtual void processaudio(const float ** in, float ** out, unsigned long inNumFrames, bool replacing=true);
};

#endif