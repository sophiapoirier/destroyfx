#ifndef __TOM7_TRANSVERB_EDITOR_H
#define __TOM7_TRANSVERB_EDITOR_H


#include "dfxgui.h"
#include "dfxguibutton.h"



//-----------------------------------------------------------------------------
class TransverbSpeedTuneButton : public DGFineTuneButton
{
public:
	TransverbSpeedTuneButton(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion, DGImage * inImage, 
								float inValueChangeAmount, long inTuneMode)
	:	DGFineTuneButton(inOwnerEditor, inParamID, inRegion, inImage, inValueChangeAmount), 
		tuneMode(inTuneMode)
	{
	}

	virtual void mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, unsigned long inKeyModifiers);

	void setTuneMode(long inTuneMode)
	{
		tuneMode = inTuneMode;
	}

private:
	long tuneMode;
};



//-----------------------------------------------------------------------------
class TransverbEditor : public DfxGuiEditor
{
public:
	TransverbEditor(AudioUnitCarbonView inInstance);
	virtual ~TransverbEditor();
	
	virtual long open(float inXOffset, float inYOffset);

private:
	AUParameterListenerRef parameterListener;
	AudioUnitParameter speed1modeAUP, speed2modeAUP;
	TransverbSpeedTuneButton * speed1UpButton, * speed1DownButton, * speed2UpButton, * speed2DownButton;
};


#endif
