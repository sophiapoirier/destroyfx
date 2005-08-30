#ifndef __MONOMAKER_EDITOR_H
#define __MONOMAKER_EDITOR_H

#include "dfxgui.h"


//-----------------------------------------------------------------------
class MonomakerEditor : public DfxGuiEditor
{
public:
	MonomakerEditor(DGEditorListenerInstance inInstance);
	virtual long open();
};

#endif
