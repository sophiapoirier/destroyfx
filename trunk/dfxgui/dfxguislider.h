/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2015  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Destroy FX Library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Destroy FX Library.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#ifndef __DFXGUI_SLIDER_H
#define __DFXGUI_SLIDER_H


#include "dfxguieditor.h"


#ifdef TARGET_PLUGIN_USES_VSTGUI

//-----------------------------------------------------------------------------
class DGSlider : public CSlider, public DGControl
{
public:
	DGSlider(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion, 
				DGAxis inOrientation, DGImage * inHandleImage, DGImage * inBackgroundImage = NULL);

#ifdef TARGET_API_RTAS
	virtual void draw(CDrawContext * inContext) VSTGUI_OVERRIDE_VMETHOD;
#endif
};



#pragma mark -
//-----------------------------------------------------------------------------
class DGAnimation : public CAnimKnob, public DGControl
{
public:
	DGAnimation(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion, 
				DGImage * inAnimationImage, long inNumAnimationFrames, DGImage * inBackground = NULL);

#ifdef TARGET_API_RTAS
	virtual void draw(CDrawContext * inContext) VSTGUI_OVERRIDE_VMETHOD;
#endif
};

#else

//-----------------------------------------------------------------------------
class DGSlider : public DGControl
{
public:
	DGSlider(DfxGuiEditor * inOwnerEditor, long inParamID, DGRect * inRegion, 
				DGAxis inOrientation, DGImage * inHandleImage, DGImage * inBackgroundImage = NULL);

	virtual void draw(DGGraphicsContext * inContext);
	virtual void mouseDown(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers, bool inIsDoubleClick);
	virtual void mouseTrack(float inXpos, float inYpos, unsigned long inMouseButtons, DGKeyModifiers inKeyModifiers);
	virtual void mouseUp(float inXpos, float inYpos, DGKeyModifiers inKeyModifiers);

	void setMouseOffset(long inOffset)
		{	mouseOffset = inOffset;	}

protected:
	DGAxis		orientation;
	DGImage *	handleImage;
	DGImage *	backgroundImage;
	long		mouseOffset;	// for mouse tracking with click in the middle of the slider handle
	float		lastX;
	float		lastY;
};

#endif	// !TARGET_PLUGIN_USES_VSTGUI



#endif
