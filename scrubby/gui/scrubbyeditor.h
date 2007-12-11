#ifndef __SCRUBBY_EDITOR_H
#define __SCRUBBY_EDITOR_H


#include "dfxgui.h"
#include "dfxguislider.h"
#include "dfxguibutton.h"
#include "dfxguidisplay.h"


//--------------------------------------------------------------------------
class ScrubbyHelpBox : public DGTextDisplay
{
public:
	ScrubbyHelpBox(DfxGuiEditor * inOwnerEditor, DGRect * inRegion, DGImage * inBackground);

	virtual void draw(DGGraphicsContext * inContext);

	void setDisplayItem(long inItemNum);

private:
	long itemNum;
};


//-----------------------------------------------------------------------
class ScrubbyEditor : public DfxGuiEditor
{
public:
	ScrubbyEditor(AudioUnitCarbonView inInstance);
	virtual ~ScrubbyEditor();

	virtual long open();
	virtual void mouseovercontrolchanged(DGControl * currentControlUnderMouse);
	virtual void numAudioChannelsChanged(unsigned long inNewNumChannels);

	void HandleNotesButton(long inNotesButtonType);
	void HandlePitchConstraintChange();

private:
	AUParameterListenerRef parameterListener;
	AudioUnitParameter tempoSyncAUP;
	DGSlider * seekRateSlider;
	DGSlider * seekRateRandMinSlider;
	DGTextDisplay * seekRateDisplay;
	DGTextDisplay * seekRateRandMinDisplay;

	AudioUnitParameter speedModeAUP, pitchConstraintAUP;

	ScrubbyHelpBox * helpbox;
	DGButton ** notesButtons;
	DGButton * midiLearnButton, * midiResetButton;
	DGControl * titleArea;
};


#endif