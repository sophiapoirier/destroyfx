#ifndef __MIDIGATEREDITOR_H
#define __MIDIGATEREDITOR_H

#include "dfxgui.h"


const CColor kMyFontCColor  = {81, 78, 75, 0};

//-----------------------------------------------------------------------
class MidiGaterEditor : public AEffGUIEditor, public CControlListener
{
public:
	MidiGaterEditor(AudioEffect *effect);
	virtual ~MidiGaterEditor();

protected:
	virtual long getRect(ERect **rect);
	virtual long open(void *ptr);
	virtual void close();

	virtual void setParameter(long index, float value);
	virtual void valueChanged(CDrawContext* context, CControl* control);

private:
	// controls
	CHorizontalSlider *slopeFader;
	CHorizontalSlider *velInfluenceFader;
	CHorizontalSlider *floorFader;
	CWebLink *DestroyFXlink;

	// parameter value display boxes
	CParamDisplay *slopeDisplay;
	CParamDisplay *velInfluenceDisplay;
	CParamDisplay *floorDisplay;

	// graphics
	CBitmap *gBackground;
	CBitmap *gSlopeFaderHandle;
	CBitmap *gVelInfluenceFaderHandle;
	CBitmap *gFloorFaderHandle;
	CBitmap *gDestroyFXlink;
};

#endif