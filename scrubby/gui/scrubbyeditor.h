#ifndef __SCRUBBYEDITOR_H
#define __SCRUBBYEDITOR_H

#include "dfxgui.h"
#include "dfxguimulticontrols.h"
#include "multikick.hpp"
#include "indexbitmap.hpp"
#include "scrubbykeyboard.hpp"

#include "scrubby.hpp"


const CColor kBrownTextCColor = {187, 173, 131, 0};

//-----------------------------------------------------------------------
class ScrubbyEditor : public AEffGUIEditor, public CControlListener
{
public:
	ScrubbyEditor(AudioEffect *effect);
	virtual ~ScrubbyEditor();

protected:
	virtual long getRect(ERect **erect);
	virtual long open(void *ptr);
	virtual void close();

	virtual void setParameter(long index, float value);
	virtual void valueChanged(CDrawContext* context, CControl* control);

    virtual void idle();

private:
	// controls
	CHorizontalSliderChunky *octaveMinFader;
	CHorizontalSliderChunky *octaveMaxFader;
	CHorizontalRangeSlider *seekRateFader;
	CHorizontalRangeSlider *seekDurFader;
	CHorizontalSlider *seekRangeFader;
	CHorizontalSlider *tempoFader;
	CHorizontalSlider *predelayFader;
	//
	MultiKick *speedModeButton;
	MultiKick *freezeButton;
	MultiKick *tempoSyncButton;
	MultiKick *stereoButton;
	MultiKick *pitchConstraintButton;
	COnOffButton *littleTempoSyncButton;
	//
	ScrubbyKeyboard *keyboard;
	CKickButton *transposeDownButton;
	CKickButton *transposeUpButton;
	CKickButton *majorChordButton;
	CKickButton *minorChordButton;
	CKickButton *allNotesButton;
	CKickButton *noneNotesButton;
	//
	COnOffButton *midiLearnButton;
	CKickButton *midiResetButton;
	IndexBitmap *help;
	CWebLink *DestroyFXlink;
	CWebLink *SmartElectronixLink;

	// parameter value display boxes
	CParamDisplay *octaveMinDisplay;
	CParamDisplay *octaveMaxDisplay;
	CParamDisplay *seekRateDisplay;
	CParamDisplay *seekRateRandMinDisplay;
	CParamDisplay *seekDurDisplay;
	CParamDisplay *seekDurRandMinDisplay;
	CParamDisplay *seekRangeDisplay;
	CParamDisplay *predelayDisplay;
	CTextEdit *tempoTextEdit;

	// graphics
	CBitmap *gBackground;
	CBitmap *gFaderHandle;
	CBitmap *gGlowingFaderHandle;
	CBitmap *gFaderHandleLeft;
	CBitmap *gGlowingFaderHandleLeft;
	CBitmap *gFaderHandleRight;
	CBitmap *gGlowingFaderHandleRight;
	//
	CBitmap *gSpeedModeButton;
	CBitmap *gFreezeButton;
	CBitmap *gTempoSyncButton;
	CBitmap *gStereoButton;
	CBitmap *gPitchConstraintButton;
	CBitmap *gLittleTempoSyncButton;
	//
	CBitmap *gKeyboardOff;
	CBitmap *gKeyboardOn;
	CBitmap *gTransposeDownButton;
	CBitmap *gTransposeUpButton;
	CBitmap *gMajorChordButton;
	CBitmap *gMinorChordButton;
	CBitmap *gAllNotesButton;
	CBitmap *gNoneNotesButton;
	//
	CBitmap *gMidiLearnButton;
	CBitmap *gMidiResetButton;
	CBitmap *gHelp;
	CBitmap *gDestroyFXlink;
	CBitmap *gSmartElectronixLink;

	bool isOpen;

	char *tempoString;
	static void seekRateDisplayConvert(float value, char *string, void *temporatestring);
	static float theTempoSync;	// a static version of fTempoSync
	char *tempoRateString, *tempoRateRandMinString;

	ScrubbyChunk *chunk;
	bool seekRateClickBetween, seekDurClickBetween;
	CHorizontalSlider **faders;
};

#endif