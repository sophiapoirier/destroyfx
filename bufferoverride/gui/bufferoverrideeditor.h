#ifndef __BUFFEROVERRIDEEDITOR_H
#define __BUFFEROVERRIDEEDITOR_H


#include "dfxgui.h"
#include "dfxguislider.h"
#include "dfxguibutton.h"
#include "dfxguidisplay.h"


// ____________________________________________________________________________
class BufferOverrideEditor : public DfxGuiEditor
{
public:
	BufferOverrideEditor(AudioUnitCarbonView inInstance);
	virtual ~BufferOverrideEditor();
	
	virtual OSStatus open(Float32 inXOffset, Float32 inYOffset);
	virtual void mouseovercontrolchanged();

	AUParameterListenerRef parameterListener;
	AudioUnitParameter bufferSizeTempoSyncAUP, divisorLFOtempoSyncAUP, bufferLFOtempoSyncAUP;

	DGSlider *bufferSizeSlider;
	DGSlider *divisorLFOrateSlider;
	DGSlider *bufferLFOrateSlider;
	DGTextDisplay *bufferSizeDisplay;
	DGTextDisplay *divisorLFOrateDisplay;
	DGTextDisplay *bufferLFOrateDisplay;

	DGButton *midilearnButton;
	DGButton *midiresetButton;
	DGStaticTextDisplay *helpDisplay;
};


#endif
