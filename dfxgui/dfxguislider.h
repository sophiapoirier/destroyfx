/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2018  Sophia Poirier

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

#pragma once


#include "dfxguieditor.h"


//-----------------------------------------------------------------------------
class DGSlider : public CSlider, public DGControl
{
public:
	DGSlider(DfxGuiEditor* inOwnerEditor, long inParamID, DGRect const& inRegion, 
			 DGAxis inOrientation, DGImage* inHandleImage, DGImage* inBackgroundImage = nullptr, long inRangeMargin = 0);

#ifdef TARGET_API_RTAS
	void draw(CDrawContext* inContext) override;
#endif
};



#pragma mark -
//-----------------------------------------------------------------------------
class DGAnimation : public CAnimKnob, public DGControl
{
public:
	DGAnimation(DfxGuiEditor* inOwnerEditor, long inParamID, DGRect const& inRegion,  
				DGImage* inAnimationImage, long inNumAnimationFrames, DGImage* inBackground = nullptr);

#ifdef TARGET_API_RTAS
	void draw(CDrawContext* inContext) override;
#endif
};
