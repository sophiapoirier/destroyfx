#ifndef __DFXGUI_SLIDER_H
#define __DFXGUI_SLIDER_H


#include "dfxgui.h"


typedef enum {
	kDGSliderAxis_vertical,
	kDGSliderAxis_horizontal
} DfxGuiSliderAxis;


//-----------------------------------------------------------------------------
class DGSlider : public DGControl
{
public:
	DGSlider(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion, 
				DfxGuiSliderAxis inOrientation, DGImage * inHandleImage, DGImage * inBackgroundImage);
	virtual ~DGSlider();

	virtual void draw(CGContextRef inContext, long inPortHeight);
	virtual void mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, unsigned long inKeyModifiers);
	virtual void mouseTrack(float inXpos, float inYpos, unsigned long inMouseButtons, unsigned long inKeyModifiers);
	virtual void mouseUp(float inXpos, float inYpos, unsigned long inKeyModifiers);

	void setMouseOffset(long inOffset)
		{	mouseOffset = inOffset;	}

	/* XXX example - call this instead of new */
	static DGSlider * create(DfxGuiEditor * dge, long param, DGRect * pos, DfxGuiSliderAxis axis, DGImage * grabby, DGImage * back)
	{
		DGSlider * s = new DGSlider(dge, param, pos, axis, grabby, back);
		dge->addControl(s);
		return s;
	}


protected:
	DfxGuiSliderAxis	orientation;
	DGImage *	handleImage;
	DGImage *	backgroundImage;
	float		fineTuneFactor;	// slow-down factor for shift control
	long		mouseOffset;	// for mouse tracking with click in the middle of the slider handle
	float		lastX;
	float		lastY;
};



#endif
