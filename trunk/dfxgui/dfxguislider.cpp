#include "dfxguislider.h"


//-----------------------------------------------------------------------------
DGSlider::DGSlider(DfxGuiEditor *		inOwnerEditor,
					long				inParamID, 
					DGRect *			inRegion,
					DfxGuiSliderAxis	inOrientation,
					DGImage *			inHandleImage, 
					DGImage *			inBackgroundImage)
:	DGControl(inOwnerEditor, inParamID, inRegion), 
	orientation(inOrientation), handleImage(inHandleImage), backgroundImage(inBackgroundImage)
{
	if (handleImage != NULL)
	{
		int handleWidth = handleImage->getWidth();
		int handleHeight = handleImage->getHeight();
		int widthDiff = inRegion->w - handleWidth;
		if (widthDiff < 0)
			widthDiff = 0;
		int heightDiff = inRegion->h - handleHeight;
		if (heightDiff < 0)
			heightDiff = 0;

		if (orientation == kDGSliderAxis_vertical)
		{
			shrinkForeBounds((widthDiff/2)+(widthDiff%2), handleHeight, widthDiff, handleHeight);
			mouseOffset = handleHeight / 2;
		}
		else
		{
			shrinkForeBounds(0, (heightDiff/2)+(heightDiff%2), handleWidth, heightDiff);
			mouseOffset = handleWidth / 2;
		}
	}

	setControlContinuous(true);
}

//-----------------------------------------------------------------------------
DGSlider::~DGSlider()
{
}

//-----------------------------------------------------------------------------
void DGSlider::draw(CGContextRef inContext, long inPortHeight)
{
	if (backgroundImage != NULL)
		backgroundImage->draw(getBounds(), inContext, inPortHeight);
	else
		getDfxGuiEditor()->DrawBackground(inContext, inPortHeight);

	SInt32 max = GetControl32BitMaximum(carbonControl);
	SInt32 min = GetControl32BitMinimum(carbonControl);
	SInt32 val = GetControl32BitValue(carbonControl);
	float valNorm = ((max-min) == 0) ? 0.0f : (float)(val-min) / (float)(max-min);

	if (handleImage != NULL)
	{
		DGRect drawRect(getForeBounds());
		long xoff = 0, yoff = 0;
		if (orientation == kDGSliderAxis_vertical)
		{
			yoff = (long) round( (float)(getForeBounds()->h) * (1.0f - valNorm) );
			drawRect.y -= handleImage->getHeight();	// XXX this is because this whole forebounds thing is goofy
		}
		else
			xoff = (long) round( (float)(getForeBounds()->w) * valNorm );
		handleImage->draw(&drawRect, inContext, inPortHeight, -xoff, -yoff);
	}
}

//-----------------------------------------------------------------------------
void DGSlider::mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, unsigned long inKeyModifiers)
{
	lastX = inXpos;
	lastY = inYpos;

	#if TARGET_PLUGIN_USES_MIDI
		if (isParameterAttached())
			getDfxGuiEditor()->setmidilearner(getParameterID());
	#endif

	if ( !(inKeyModifiers & kDGKeyModifier_shift) )
		mouseTrack(inXpos, inYpos, inMouseButtons, inKeyModifiers);
}

//-----------------------------------------------------------------------------
void DGSlider::mouseTrack(float inXpos, float inYpos, unsigned long inMouseButtons, unsigned long inKeyModifiers)
{
	SInt32 max = GetControl32BitMaximum(carbonControl);
	SInt32 val = GetControl32BitValue(carbonControl);
	SInt32 oldval = val;

	DGRect fore;
	fore.set(getForeBounds());
	SInt32 o_X = fore.x - getBounds()->x + mouseOffset;
	SInt32 o_Y = fore.y - getBounds()->y - mouseOffset;

	if (inKeyModifiers & kDGKeyModifier_shift)	// slo-mo
	{
		if (orientation == kDGSliderAxis_vertical)
		{
			float diff = lastY - inYpos;
			diff /= fineTuneFactor;
			val += (SInt32) (diff * (float)max / (float)fore.h);
		}
		else	// horizontal mode
		{
			float diff = inXpos - lastX;
			diff /= fineTuneFactor;
			val += (SInt32) (diff * (float)max / (float)fore.w);
		}
	}
	else	// regular movement
	{
		if (orientation == kDGSliderAxis_vertical)
		{
			float valnorm = (inYpos - (float)o_Y) / (float)fore.h;
			val = (SInt32)((float)max * (1.0f - valnorm));
		}
		else	// horizontal mode
		{
			float valnorm = (inXpos - (float)o_X) / (float)fore.w;
			val = (SInt32)((float)max * valnorm);
		}
	}
	
	if (val > max)
		val = max;
	if (val < 0)
		val = 0;
	if (val != oldval)
		SetControl32BitValue(carbonControl, val);

	lastX = inXpos;
	lastY = inYpos;
}

//-----------------------------------------------------------------------------
void DGSlider::mouseUp(float inXpos, float inYpos, unsigned long inKeyModifiers)
{
	mouseTrack(inXpos, inYpos, 1, inKeyModifiers);
}
