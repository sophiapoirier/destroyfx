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

	virtual void draw(CGContextRef inContext, long inPortHeight);

	virtual void mouseDown(float, float, unsigned long, unsigned long) { }
	virtual void mouseTrack(float, float, unsigned long, unsigned long) { }
	virtual void mouseUp(float, float, unsigned long) { }

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

	void HandleNotesButton(long inNotesButtonType);

private:
	AUParameterListenerRef parameterListener;
	AudioUnitParameter tempoSyncAUP;
	DGSlider * seekRateSlider;
	DGSlider * seekRateRandMinSlider;
	DGTextDisplay * seekRateDisplay;
	DGTextDisplay * seekRateRandMinDisplay;

	ScrubbyHelpBox * helpbox;
	DGButton ** notesButtons;
	DGButton * midiLearnButton, * midiResetButton;
	DGControl * titleArea;
};


#endif