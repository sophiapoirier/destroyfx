#ifndef __BUFFEROVERRIDE_EDITOR_H
#define __BUFFEROVERRIDE_EDITOR_H


#include "dfxgui.h"
#include "dfxguislider.h"
#include "dfxguibutton.h"
#include "dfxguidisplay.h"


//-----------------------------------------------------------------------------
class BufferOverrideEditor : public DfxGuiEditor
{
public:
	BufferOverrideEditor(AudioUnitCarbonView inInstance);
	virtual ~BufferOverrideEditor();

	virtual long open();
	virtual void mouseovercontrolchanged();

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
