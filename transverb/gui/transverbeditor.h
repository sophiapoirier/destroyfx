#ifndef __TOM7_TRANSVERB_EDITOR_H
#define __TOM7_TRANSVERB_EDITOR_H


#include "dfxgui.h"


// ____________________________________________________________________________
class TransverbEditor : public DfxGuiEditor
{
public:
	TransverbEditor(AudioUnitCarbonView inInstance);
	virtual ~TransverbEditor();
	
	virtual long open(float inXOffset, float inYOffset);
};


#endif
