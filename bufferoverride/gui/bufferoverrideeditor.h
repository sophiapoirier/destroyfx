#ifndef __BUFFER_OVERRIDE_EDITOR_H
#define __BUFFER_OVERRIDE_EDITOR_H


#include "dfxgui.h"


//-----------------------------------------------------------------------------
class BufferOverrideEditor : public DfxGuiEditor
{
public:
	BufferOverrideEditor(AudioUnitCarbonView inInstance);
	virtual ~BufferOverrideEditor();

	virtual long OpenEditor();
	virtual void mouseovercontrolchanged(DGControl * currentControlUnderMouse);

private:
	AUParameterListenerRef parameterListener;
	AudioUnitParameter bufferSizeTempoSyncAUP, divisorLFOtempoSyncAUP, bufferLFOtempoSyncAUP;

	DGSlider * bufferSizeSlider;
	DGSlider * divisorLFOrateSlider;
	DGSlider * bufferLFOrateSlider;
	DGTextDisplay * bufferSizeDisplay;
	DGTextDisplay * divisorLFOrateDisplay;
	DGTextDisplay * bufferLFOrateDisplay;

	DGButton * midilearnButton;
	DGButton * midiresetButton;
	DGStaticTextDisplay * helpDisplay;
};


#endif
