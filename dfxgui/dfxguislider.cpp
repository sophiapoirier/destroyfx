#include "dfxguislider.h"


//-----------------------------------------------------------------------------
DGSlider::DGSlider(DfxGuiEditor *		inOwnerEditor,
					AudioUnitParameterID inParamID, 
					DGRect *			inRegion,
					DfxGuiSliderStyle	inOrientation,
					DGImage *			inForeGround, 
					DGImage *			inBackground)
:	DGControl(inOwnerEditor, inParamID, inRegion), 
	orientation(inOrientation), ForeGround(inForeGround), BackGround(inBackground)
{
	if (inRegion->w == 0)
	{
		if (BackGround != NULL)
		{
			inRegion->w = BackGround->getWidth();
			setBounds(inRegion); 
		}
	}
	
	if (inRegion->h == 0)
	{
		if (BackGround != NULL)
		{
			inRegion->h = BackGround->getHeight();
			setBounds(inRegion);
		}
	}
		
	int handleWidth = (ForeGround == NULL) ? 0 : ForeGround->getWidth();
	int handleHeight = (ForeGround == NULL) ? 0 : ForeGround->getHeight();
	int widthDiff = inRegion->w - handleWidth;
	if (widthDiff < 0)
		widthDiff = 0;
	int heightDiff = inRegion->h - handleHeight;
	if (heightDiff < 0)
		heightDiff = 0;
	if (orientation == kDGSliderStyle_vertical)
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
	setContinuousControl(true);
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
	CGImageRef theBack = NULL;
	if (BackGround != NULL)
		theBack = BackGround->getCGImage();
	if (theBack != NULL)
	{
		getBounds()->copyToCGRect(&bounds, inPortHeight);
// XXX this is a very poor hack, Marc
#if 0
		bounds.size.width = (float) BackGround->getWidth();
		bounds.size.height = (float) BackGround->getHeight();
		bounds.origin.x -= (float)where.x - getDfxGuiEditor()->GetXOffset();
		bounds.origin.y -= (float) (BackGround->getHeight() - (where.y - getDfxGuiEditor()->GetYOffset()) - where.h);
#endif
		CGContextDrawImage(inContext, bounds, theBack);
	}
	else
		getDfxGuiEditor()->DrawBackground(inContext, inPortHeight);

	CGImageRef theFore = NULL;
	if (ForeGround != NULL)
		theFore = ForeGround->getCGImage();
	if (theFore != NULL)
	{
//		float valNorm = (max == 0) ? 0.0f : (float)val / (float)max;
		float valNorm = ((max-min) == 0) ? 0.0f : (float)(val-min) / (float)(max-min);
//printf("ControlMax = %ld,  ControlMin = %ld,  ControlValue = %ld,  valueF = %.3f\n", max, min, val, valNorm);
		getForeBounds()->copyToCGRect(&bounds, inPortHeight);
		if (orientation == kDGSliderStyle_vertical)
		{
			float slideRange = bounds.size.height;
			bounds.size.height = (float) ForeGround->getHeight();
//			bounds.size.width = (float) ForeGround->getWidth();
			bounds.origin.y += round(slideRange * valNorm);
		}
		else
		{
			float slideRange = bounds.size.width;
			bounds.size.width = (float) ForeGround->getWidth();
//			bounds.size.height = (float) ForeGround->getHeight();
			bounds.origin.x += round(slideRange * valNorm);
		}
		CGContextDrawImage(inContext, bounds, theFore);
	}
}

//-----------------------------------------------------------------------------
void DGSlider::mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, unsigned long inKeyModifiers)
{
	lastX = inXpos;
	lastY = inYpos;

	#if TARGET_PLUGIN_USES_MIDI
		if (isAUVPattached())
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
		if (orientation == kDGSliderStyle_vertical)
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
		if (orientation == kDGSliderStyle_vertical)
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
