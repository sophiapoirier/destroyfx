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
	if (inRegion->w == 0)
	{
		if (backgroundImage != NULL)
		{
			inRegion->w = backgroundImage->getWidth();
			setBounds(inRegion); 
		}
	}
	
	if (inRegion->h == 0)
	{
		if (backgroundImage != NULL)
		{
			inRegion->h = backgroundImage->getHeight();
			setBounds(inRegion);
		}
	}
		
	int handleWidth = (handleImage == NULL) ? 0 : handleImage->getWidth();
	int handleHeight = (handleImage == NULL) ? 0 : handleImage->getHeight();
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

	fineTuneFactor = 12.0f;
	setControlContinuous(true);
}

//-----------------------------------------------------------------------------
DGSlider::~DGSlider()
{
}

//-----------------------------------------------------------------------------
void DGSlider::draw(CGContextRef inContext, UInt32 inPortHeight)
{
	ControlRef carbonControl = getCarbonControl();
	SInt32 max = GetControl32BitMaximum(carbonControl);
	SInt32 min = GetControl32BitMinimum(carbonControl);
	SInt32 val = GetControl32BitValue(carbonControl);

	CGRect bounds;
	CGImageRef backgroundCGImage = NULL;
	if (backgroundImage != NULL)
		backgroundCGImage = backgroundImage->getCGImage();
	if (backgroundCGImage != NULL)
	{
		getBounds()->copyToCGRect(&bounds, inPortHeight);
		CGContextDrawImage(inContext, bounds, backgroundCGImage);
	}
	else
		getDfxGuiEditor()->DrawBackground(inContext, inPortHeight);

	CGImageRef handleCGImage = NULL;
	if (handleImage != NULL)
		handleCGImage = handleImage->getCGImage();
	if (handleCGImage != NULL)
	{
//		float valNorm = (max == 0) ? 0.0f : (float)val / (float)max;
		float valNorm = ((max-min) == 0) ? 0.0f : (float)(val-min) / (float)(max-min);
//printf("ControlMax = %ld,  ControlMin = %ld,  ControlValue = %ld,  valueF = %.3f\n", max, min, val, valNorm);
		getForeBounds()->copyToCGRect(&bounds, inPortHeight);
		if (orientation == kDGSliderAxis_vertical)
		{
			float slideRange = bounds.size.height;
			bounds.size.height = (float) handleImage->getHeight();
//			bounds.size.width = (float) handleImage->getWidth();
			bounds.origin.y += round(slideRange * valNorm);
		}
		else
		{
			float slideRange = bounds.size.width;
			bounds.size.width = (float) handleImage->getWidth();
//			bounds.size.height = (float) handleImage->getHeight();
			bounds.origin.x += round(slideRange * valNorm);
		}
		CGContextDrawImage(inContext, bounds, handleCGImage);
	}
}

//-----------------------------------------------------------------------------
void DGSlider::mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, unsigned long inKeyModifiers)
{
	lastX = inXpos;
	lastY = inYpos;

	#if TARGET_PLUGIN_USES_MIDI
		if (isParameterAttached())
			getDfxGuiEditor()->setmidilearner(getAUVP().mParameterID);
	#endif

	if ( !(inKeyModifiers & kDGKeyModifier_shift) )
		mouseTrack(inXpos, inYpos, inMouseButtons, inKeyModifiers);
}

//-----------------------------------------------------------------------------
void DGSlider::mouseTrack(float inXpos, float inYpos, unsigned long inMouseButtons, unsigned long inKeyModifiers)
{
	ControlRef carbonControl = getCarbonControl();
	SInt32 max = GetControl32BitMaximum(carbonControl);
	SInt32 val = GetControl32BitValue(carbonControl);
	SInt32 oldval = val;

	DGRect fore;
	fore.set(getForeBounds());
	DGRect back;
	back.set(getBounds());
	SInt32 o_X = fore.x - back.x + mouseOffset;
	SInt32 o_Y = fore.y - back.y - mouseOffset;

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
