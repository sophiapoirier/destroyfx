#ifndef __POLARIZEREDITOR_H
#define __POLARIZEREDITOR_H

#include "dfxgui.h"


//-----------------------------------------------------------------------
class PolarizerEditor : public DfxGuiEditor
{
public:
	PolarizerEditor(AudioUnitCarbonView inInstance);
	virtual long open();
};

#endif