#ifndef __MIDIGATER_EDITOR_H
#define __MIDIGATER_EDITOR_H

#include "dfxgui.h"

//-----------------------------------------------------------------------
class MidiGaterEditor : public DfxGuiEditor
{
public:
	MidiGaterEditor(AudioUnitCarbonView inInstance);
	virtual OSStatus open(Float32 inXOffset, Float32 inYOffset);
};

#endif