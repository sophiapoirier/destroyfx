#ifndef __MONOMAKEREDITOR_H
#define __MONOMAKEREDITOR_H

#include "dfxgui.h"


//const CColor kBackgroundCColor  = {64, 54, 40, 0};
const CColor kBackgroundCColor  = {42, 34, 22, 0};

//-----------------------------------------------------------------------
class MonomakerEditor : public AEffGUIEditor, public CControlListener
{
public:
	MonomakerEditor(AudioEffect *effect);
	virtual ~MonomakerEditor();

protected:
	virtual long getRect(ERect **rect);
	virtual long open(void *ptr);
	virtual void close();

	virtual void setParameter(long index, float value);
	virtual void valueChanged(CDrawContext* context, CControl* control);

private:
	// controls
	CHorizontalSlider *monomergeFader;
	CHorizontalSlider *panFader;
	CMovieBitmap *monomergeMovie;
	CMovieBitmap *panMovie;
	CWebLink *DestroyFXlink;

	// parameter value display boxes
	CParamDisplay *monomergeDisplay;
	CParamDisplay *panDisplay;

	// graphics
	CBitmap *gBackground;
	CBitmap *gMonomergeFaderSlide;
	CBitmap *gPanFaderSlide;
	CBitmap *gFaderHandle;
	CBitmap *gMonomergeMovie;
	CBitmap *gPanMovie;
	CBitmap *gDestroyFXlink;
};

#endif