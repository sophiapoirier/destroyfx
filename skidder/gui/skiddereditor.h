#ifndef __SKIDDEREDITOR_H
#define __SKIDDEREDITOR_H

#include "dfxgui.h"
#include "dfxguimulticontrols.h"

#ifndef __SKIDDER_H
#include "skidder.hpp"
#endif


const CColor kBackgroundCColor = {59, 73, 114, 0};
//const CColor kMyLighBlueCColor = {91, 102, 137};
const CColor kMyPaleGreenCColor = {108, 156, 132};
const CColor kMyDarkBlueCColor = {45, 57, 90};

//-----------------------------------------------------------------------
class SkidderEditor : public AEffGUIEditor, public CControlListener
{
public:
	SkidderEditor(AudioEffect *effect);
	virtual ~SkidderEditor();

protected:
	virtual long getRect(ERect **rect);
	virtual long open(void *ptr);
	virtual void close();

	virtual void setParameter(long index, float value);
	virtual void valueChanged(CDrawContext* context, CControl* control);

    virtual void idle();

private:
	float calculateTheCycleRate();

	// controls
	CHorizontalSlider *rateFader;
	CHorizontalSlider *rateRandFactorFader;
	CHorizontalSlider *tempoFader;
	CHorizontalRangeSlider *pulsewidthFader;
	CHorizontalSlider *slopeFader;
	CHorizontalRangeSlider *floorFader;
	CHorizontalSlider *panFader;
	CHorizontalSlider *noiseFader;
	COnOffButton *tempoSyncButton;
	COnOffButton *midiLearnButton;
	CKickButton *midiResetButton;
	CWebLink *goButton;
	CWebLink *DestroyFXlink;
	CWebLink *SmartElectronixLink;

	// parameter value display boxes
	CParamDisplay *rateDisplay;
	CParamDisplay *rateRandFactorDisplay;
	CParamDisplay *rangeDisplay;
	CParamDisplay *rateRandRangeDisplay;
	CTextEdit *tempoTextEdit;
	CNumberBox *pulsewidthDisplay;
	CParamDisplay *pulsewidthRandMinDisplay;
	CParamDisplay *randomMinimumDisplay;
	CParamDisplay *slopeDisplay;
	CParamDisplay *floorDisplay;
	CParamDisplay *floorRandMinDisplay;
	CParamDisplay *randomMinimum2Display;
	CParamDisplay *panDisplay;
	CParamDisplay *noiseDisplay;
	CParamDisplay *goDisplay;

	char *tempoString;

	// graphics
	CBitmap *gBackground;
	CBitmap *gFaderSlide;
	CBitmap *gFaderHandle;
	CBitmap *gGlowingFaderHandle;
	CBitmap *gFaderHandleLeft;
	CBitmap *gGlowingFaderHandleLeft;
	CBitmap *gFaderHandleRight;
	CBitmap *gGlowingFaderHandleRight;
	CBitmap *gTempoSyncButton;
	CBitmap *gMidiLearnButton;
	CBitmap *gMidiResetButton;
	CBitmap *gGoButton;
	CBitmap *gDestroyFXlink;
	CBitmap *gSmartElectronixLink;

#ifdef MSKIDDER
	CMultiToggle *midiModeButton;
	COnOffButton *velocityButton;
	CBitmap *gMidiModeButton;
	CBitmap *gVelocityButton;
#endif

	bool isOpen;

	static void rateDisplayConvert(float value, char *string, void *temporatestring);
	static float theTempoSync;
	char *tempoRateString;
	float theCycleRate;	// the rate in Hz of the skidding cycles
//	SkidderChunk *chunk;
	DfxSettings *chunk;
	long goError;
	CHorizontalSlider **faders;
};

#endif