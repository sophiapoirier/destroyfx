#ifndef __POLARIZER_EDITOR_H
#define __POLARIZER_EDITOR_H

#include "dfxgui.h"


//-----------------------------------------------------------------------
class PolarizerEditor : public DfxGuiEditor
{
public:
	PolarizerEditor(AudioUnitCarbonView inInstance);
	virtual long OpenEditor();
};

#endif