#ifndef __MIDIGATER_EDITOR_H
#define __MIDIGATER_EDITOR_H

#include "dfxgui.h"

//-----------------------------------------------------------------------
class MidiGaterEditor : public DfxGuiEditor
{
public:
	MidiGaterEditor(AudioUnitCarbonView inInstance);
	virtual long open(float inXOffset, float inYOffset);
};

#endif