#include "dfxguislider.h"


//-----------------------------------------------------------------------------
DGSlider::DGSlider(DfxGuiEditor *		inOwnerEditor,
					AudioUnitParameterID inParamID, 
					DGRect *			inRegion,
					DfxGuiSliderStyle	inOrientation,
					DGGraphic *			inForeGround, 
					DGGraphic *			inBackground)
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
		
	int handleWidth = CGImageGetWidth(ForeGround->getCGImage());
	int handleHeight = CGImageGetHeight(ForeGround->getCGImage());
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

	fineTuneFactor = 6.0f;
	setContinuousControl(true);
	setType(kDfxGuiType_slider);
}

//-----------------------------------------------------------------------------
DGSlider::~DGSlider()
{
	ForeGround = NULL;
	BackGround = NULL;
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
#ifdef SLIDERS_USE_BACKGROUND
		bounds.size.width = (float) CGImageGetWidth(theBack);
		bounds.size.height = (float) CGImageGetHeight(theBack);
		bounds.origin.x -= (float)where.x - getDfxGuiEditor()->GetXOffset();
		bounds.origin.y -= (float) (CGImageGetHeight(theBack) - (where.y - getDfxGuiEditor()->GetYOffset()) - where.h);
#endif
		CGContextDrawImage(inContext, bounds, theBack);
	}

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
			bounds.size.height = (float) CGImageGetHeight(theFore);
//			bounds.size.width = (float) CGImageGetWidth(theFore);
			bounds.origin.y += round(slideRange * valNorm);
		}
		else
		{
			float slideRange = bounds.size.width;
			bounds.size.width = (float) CGImageGetWidth(theFore);
//			bounds.size.height = (float) CGImageGetHeight(theFore);
			bounds.origin.x += round(slideRange * valNorm);
		}
		CGContextDrawImage(inContext, bounds, theFore);
	}
}

//-----------------------------------------------------------------------------
void DGSlider::mouseDown(Point inPos, bool with_option, bool with_shift)
{
	sldr_X = inPos.h;
	sldr_Y = inPos.v;

	#if TARGET_PLUGIN_USES_MIDI
		if (isAUVPattached())
			getDfxGuiEditor()->setmidilearner(getAUVP().mParameterID);
	#endif

	if (!with_shift)
		mouseTrack(inPos, with_option, with_shift);
}

//-----------------------------------------------------------------------------
void DGSlider::mouseTrack(Point inPos, bool with_option, bool with_shift)
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

	if (with_shift)	// slo-mo
	{
		if (orientation == kDGSliderStyle_vertical)
		{
			float diff = (float) (sldr_Y - inPos.v);
			diff /= fineTuneFactor;
			val += (SInt32) (diff * (float)max / (float)fore.h);
		}
		else	// horizontal mode
		{
			float diff = (float) (inPos.h - sldr_X);
			diff /= fineTuneFactor;
			val += (SInt32) (diff * (float)max / (float)fore.w);
		}
	}
	else	// regular movement
	{
		if (orientation == kDGSliderStyle_vertical)
		{
			float valnorm = (float)(inPos.v - o_Y) / (float)fore.h;
			val = (SInt32)((float)max * (1.0f - valnorm));
		}
		else	// horizontal mode
		{
			float valnorm = (float)(inPos.h - o_X) / (float)fore.w;
			val = (SInt32)((float)max * valnorm);
		}
	}
	
	if (val > max)
		val = max;
	if (val < 0)
		val = 0;
	if (val != oldval)
		SetControl32BitValue(carbonControl, val);

	sldr_X = inPos.h;
	sldr_Y = inPos.v;
}

//-----------------------------------------------------------------------------
void DGSlider::mouseUp(Point inPos, bool with_option, bool with_shift)
{
	mouseTrack(inPos, with_option, with_shift);
}
