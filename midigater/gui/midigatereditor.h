#ifndef __MIDI_GATER_EDITOR_H
#define __MIDI_GATER_EDITOR_H

#include "dfxgui.h"

//-----------------------------------------------------------------------
class MidiGaterEditor : public DfxGuiEditor
{
public:
	MidiGaterEditor(DGEditorListenerInstance inInstance);
	virtual long OpenEditor();
};

#endif