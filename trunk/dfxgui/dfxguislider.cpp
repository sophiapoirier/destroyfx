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

	SInt32 min = GetControl32BitMinimum(carbonControl);
	SInt32 max = GetControl32BitMaximum(carbonControl);
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
void DGSlider::mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers)
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
void DGSlider::mouseTrack(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers)
{
	SInt32 min = GetControl32BitMinimum(carbonControl);
	SInt32 max = GetControl32BitMaximum(carbonControl);
	SInt32 val = GetControl32BitValue(carbonControl);
	SInt32 oldval = val;

	DGRect fore;
	fore.set(getForeBounds());
	float o_X = (float) (fore.x - getBounds()->x + mouseOffset);
	float o_Y = (float) (fore.y - getBounds()->y - mouseOffset);

	if (inKeyModifiers & kDGKeyModifier_shift)	// slo-mo
	{
		if (orientation == kDGSliderAxis_vertical)
		{
			float diff = lastY - inYpos;
			diff /= fineTuneFactor;
			val += (SInt32) (diff * (float)(max-min) / (float)fore.h);
		}
		else	// horizontal mode
		{
			float diff = inXpos - lastX;
			diff /= fineTuneFactor;
			val += (SInt32) (diff * (float)(max-min) / (float)fore.w);
		}
	}
	else	// regular movement
	{
		if (orientation == kDGSliderAxis_vertical)
		{
			float valnorm = (inYpos - o_Y) / (float)fore.h;
			val = (SInt32)((1.0f - valnorm) * (float)(max-min)) + min;
		}
		else	// horizontal mode
		{
			float valnorm = (inXpos - o_X) / (float)fore.w;
			val = (SInt32)(valnorm * (float)(max-min)) + min;
		}
	}

	if (val > max)
		val = max;
	if (val < min)
		val = min;
	if (val != oldval)
		SetControl32BitValue(carbonControl, val);

	lastX = inXpos;
	lastY = inYpos;
}

//-----------------------------------------------------------------------------
void DGSlider::mouseUp(float inXpos, float inYpos, DGKeyModifiers inKeyModifiers)
{
	mouseTrack(inXpos, inYpos, 1, inKeyModifiers);
}

//-----------------------------------------------------------------------------
bool DGSlider::mouseWheel(long inDelta, DGMouseWheelAxis inAxis, DGKeyModifiers inKeyModifiers)
{
	lastX = 0.0f;
	lastY = 0.0f;
	float x = 0.0f, y = 0.0f;
	if (orientation == kDGSliderAxis_vertical)
		y = (float)(-inDelta);
	else
		x = (float)inDelta;
	if ( !(inKeyModifiers & kDGKeyModifier_shift) )
	{
		x *= fineTuneFactor;
		y *= fineTuneFactor;
	}
	mouseTrack(x, y, 1, inKeyModifiers | kDGKeyModifier_shift);

	return true;
}
