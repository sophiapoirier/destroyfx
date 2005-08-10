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

	virtual void draw(DGGraphicsContext * inContext);
	virtual void mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers, bool inIsDoubleClick);
	virtual void mouseTrack(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers);
	virtual void mouseUp(float inXpos, float inYpos, DGKeyModifiers inKeyModifiers);

	void setMouseOffset(long inOffset)
		{	mouseOffset = inOffset;	}

protected:
	DfxGuiSliderAxis	orientation;
	DGImage *	handleImage;
	DGImage *	backgroundImage;
	long		mouseOffset;	// for mouse tracking with click in the middle of the slider handle
	float		lastX;
	float		lastY;
};



#endif
