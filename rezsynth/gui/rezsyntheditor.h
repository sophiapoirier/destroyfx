#ifndef __REZ_SYNTH_EDITOR_H
#define __REZ_SYNTH_EDITOR_H


#include "dfxgui.h"
#include "dfxguislider.h"
#include "dfxguidisplay.h"


//-----------------------------------------------------------------------------
class RezSynthSlider : public DGSlider
{
public:
	RezSynthSlider(DfxGuiEditor * inOwnerEditor, AudioUnitParameterID inParamID, DGRect * inRegion, 
					DfxGuiSliderAxis inOrientation, DGImage * inHandle, DGImage * inBackground)
	:	DGSlider(inOwnerEditor, inParamID, inRegion, inOrientation, inHandle, inBackground)
		{ }
	virtual void draw(CGContextRef inContext, long inPortHeight)
	{
		// this just makes sure to redraw the GUI background underneath the partially-transparent slider background
		getDfxGuiEditor()->DrawBackground(inContext, inPortHeight);
		DGSlider::draw(inContext, inPortHeight);
	}
};


//-----------------------------------------------------------------------
class RezSynthEditor : public DfxGuiEditor
{
public:
	RezSynthEditor(AudioUnitCarbonView inInstance);
	virtual ~RezSynthEditor();

	virtual long open();

private:
	AUParameterListenerRef parameterListener;
	AudioUnitParameter sepModeAUP;
	RezSynthSlider * sepAmountSlider;
	DGTextDisplay * sepAmountDisplay;
};


#endif
