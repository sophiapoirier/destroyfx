#ifndef __POLARIZEREDITOR_H
#define __POLARIZEREDITOR_H

#include "dfxgui.h"
#include "multikick.hpp"


//-----------------------------------------------------------------------
class PolarizerEditor : public AEffGUIEditor, public CControlListener
{
public:
	PolarizerEditor(AudioEffect *effect);
	virtual ~PolarizerEditor();

protected:
	virtual long getRect(ERect **rect);
	virtual long open(void *ptr);
	virtual void close();

	virtual void setParameter(long index, float value);
	virtual void valueChanged(CDrawContext* context, CControl* control);

private:
	// controls
	CVerticalSlider *leapFader;
	CVerticalSlider *amountFader;
	MultiKick *implodeButton;
	CWebLink *DestroyFXlink;

	// parameter value display boxes
	CParamDisplay *leapDisplay;
	CParamDisplay *amountDisplay;

	// graphics
	CBitmap *gBackground;
	CBitmap *gFaderSlide;
	CBitmap *gFaderHandle;
	CBitmap *gImplodeButton;
	CBitmap *gDestroyFXlink;
};

#endif