#ifndef __EQSYNCEDITOR_H
#define __EQSYNCEDITOR_H

#include "dfxgui.h"


//--------------------------------------------------------------------------
class EQSyncEditor : public DfxGuiEditor
{
public:
	EQSyncEditor(AudioUnitCarbonView inInstance);
	virtual long open();
};

#endif