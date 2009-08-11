#ifndef __EQ_SYNC_EDITOR_H
#define __EQ_SYNC_EDITOR_H

#include "dfxgui.h"


//--------------------------------------------------------------------------
class EQSyncEditor : public DfxGuiEditor
{
public:
	EQSyncEditor(AudioUnitCarbonView inInstance);
	virtual long OpenEditor();
};

#endif