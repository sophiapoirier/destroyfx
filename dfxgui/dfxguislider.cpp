/*------------------------------------------------------------------------
Destroy FX Library (version 1.0) is a collection of foundation code 
for creating audio software plug-ins.  
Copyright (C) 2002-2009  Sophia Poirier

This program is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

This program is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with this program.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, please visit http://destroyfx.org/ 
and use the contact form.
------------------------------------------------------------------------*/

#include "dfxguislider.h"


//-----------------------------------------------------------------------------
DGSlider::DGSlider(DfxGuiEditor *	inOwnerEditor,
					long			inParamID, 
					DGRect *		inRegion,
					DGAxis			inOrientation,
					DGImage *		inHandleImage, 
					DGImage *		inBackgroundImage)
:	DGControl(inOwnerEditor, inParamID, inRegion), 
	orientation(inOrientation), handleImage(inHandleImage), backgroundImage(inBackgroundImage)
{
	mouseOffset = 0;

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

		if (orientation & kDGAxis_horizontal)
		{
			shrinkForeBounds(0, (heightDiff/2)+(heightDiff%2), handleWidth, heightDiff);
			mouseOffset = handleWidth / 2;
		}
		else
		{
			shrinkForeBounds((widthDiff/2)+(widthDiff%2), handleHeight, widthDiff, handleHeight);
			mouseOffset = handleHeight / 2;
		}
	}

	setControlContinuous(true);
}

//-----------------------------------------------------------------------------
void DGSlider::draw(DGGraphicsContext * inContext)
{
	if (backgroundImage != NULL)
		backgroundImage->draw(getBounds(), inContext);

	SInt32 min = GetControl32BitMinimum(carbonControl);
	SInt32 max = GetControl32BitMaximum(carbonControl);
	SInt32 val = GetControl32BitValue(carbonControl);
	float valNorm = ((max-min) == 0) ? 0.0f : (float)(val-min) / (float)(max-min);

	if (handleImage != NULL)
	{
		DGRect drawRect(getForeBounds());
		long xoff = 0, yoff = 0;
		if (orientation & kDGAxis_horizontal)
		{
			xoff = (long) round( (float)(getForeBounds()->w) * valNorm );
		}
		else
		{
			yoff = (long) round( (float)(getForeBounds()->h) * (1.0f - valNorm) );
			drawRect.y -= handleImage->getHeight();	// XXX this is because this whole forebounds thing is goofy
		}
		handleImage->draw(&drawRect, inContext, -xoff, -yoff);
	}
}

//-----------------------------------------------------------------------------
void DGSlider::mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers, bool inIsDoubleClick)
{
	lastX = inXpos;
	lastY = inYpos;

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

	float o_X = (float) (getForeBounds()->x - getBounds()->x + mouseOffset);
	float o_Y = (float) (getForeBounds()->y - getBounds()->y - mouseOffset);

	if (inKeyModifiers & kDGKeyModifier_shift)	// slo-mo
	{
		if (orientation & kDGAxis_horizontal)
		{
			float diff = inXpos - lastX;
			diff /= getFineTuneFactor();
			val += (SInt32) (diff * (float)(max-min) / (float)(getForeBounds()->w));
		}
		else	// vertical mode
		{
			float diff = lastY - inYpos;
			diff /= getFineTuneFactor();
			val += (SInt32) (diff * (float)(max-min) / (float)(getForeBounds()->h));
		}
	}
	else	// regular movement
	{
		if (orientation & kDGAxis_horizontal)
		{
			float valnorm = (inXpos - o_X) / (float)(getForeBounds()->w);
			val = (SInt32)(valnorm * (float)(max-min)) + min;
		}
		else	// vertical mode
		{
			float valnorm = (inYpos - o_Y) / (float)(getForeBounds()->h);
			val = (SInt32)((1.0f - valnorm) * (float)(max-min)) + min;
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
