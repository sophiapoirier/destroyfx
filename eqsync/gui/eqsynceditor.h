#ifndef __EQSYNCEDITOR_H
#define __EQSYNCEDITOR_H

#include "vstgui.h"


//--------------------------------------------------------------------------
class EQsyncEditor : public AEffGUIEditor, public CControlListener
{
public:
	EQsyncEditor(AudioEffect *effect);
	virtual ~EQsyncEditor();

protected:
	virtual long getRect(ERect **rect);
	virtual long open(void *ptr);
	virtual void close();

	virtual void setParameter(long index, float value);
	virtual void valueChanged(CDrawContext* context, CControl* control);

private:
	// controls
	CHorizontalSlider *tempoRateFader;
	CHorizontalSlider *tempoFader;
	CHorizontalSlider *smoothFader;
	CVerticalSlider *a0Fader;
	CVerticalSlider *a1Fader;
	CVerticalSlider *a2Fader;
	CVerticalSlider *b1Fader;
	CVerticalSlider *b2Fader;
	CHorizontalSwitch *DestroyFXlink;

	// parameter value display boxes
	CParamDisplay *tempoRateDisplay;
	CTextEdit *tempoTextEdit;
	CParamDisplay *smoothDisplay;

	char *tempoString, *tempoRateString;

	// graphics
	CBitmap *gBackground;
	CBitmap *gFaderSlide;
	CBitmap *gFaderHandle;
	CBitmap *gTallFaderSlide;
	CBitmap *gTallFaderHandle;
	CBitmap *gDestroyFXlink;
};

#endif