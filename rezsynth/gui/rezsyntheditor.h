#ifndef __REZ_SYNTH_EDITOR_H
#define __REZ_SYNTH_EDITOR_H


#include "dfxgui.h"


//-----------------------------------------------------------------------
class RezSynthEditor : public DfxGuiEditor
{
public:
	RezSynthEditor(AudioUnitCarbonView inInstance);
	virtual ~RezSynthEditor();

	virtual long OpenEditor();

private:
	AUParameterListenerRef parameterListener;
	AudioUnitParameter sepModeAUP;
	DGSlider * sepAmountSlider;
	DGTextDisplay * sepAmountDisplay;
};


#endif
