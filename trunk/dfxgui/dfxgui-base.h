/*------------------------------------------------------------------------
Destroy FX Library (version 1.0) is a collection of foundation code 
for creating audio software plug-ins.  
Copyright (C) 2009  Sophia Poirier

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

#ifndef __DFXGUI_BASE_H
#define __DFXGUI_BASE_H



//-----------------------------------------------------------------------------
enum {
	kDGKeyModifier_accel = 1,	// command on Macs, control on PCs
	kDGKeyModifier_alt = 1 << 1,	// option on Macs, alt on PCs
	kDGKeyModifier_shift = 1 << 2,
	kDGKeyModifier_extra = 1 << 3	// control on Macs
};
typedef unsigned long	DGKeyModifiers;

typedef enum {
	kDGAxis_horizontal = 1,
	kDGAxis_vertical = 1 << 1,
	kDGAxis_omni = (kDGAxis_horizontal | kDGAxis_vertical)
} DGAxis;

typedef enum {
	kDGAntialiasQuality_default = 0,
	kDGAntialiasQuality_none,
	kDGAntialiasQuality_low,
	kDGAntialiasQuality_high
} DGAntialiasQuality;

typedef enum {
	kDGTextAlign_left = 0,
	kDGTextAlign_center,
	kDGTextAlign_right
} DGTextAlignment;

typedef enum {
	kDGButtonType_pushbutton = 0,
	kDGButtonType_incbutton,
	kDGButtonType_decbutton,
	kDGButtonType_radiobutton, 
	kDGButtonType_picturereel
} DfxGuiBottonMode;

#define kDGFontName_SnootPixel10	"snoot.org pixel10"
const float kDGFontSize_SnootPixel10 = 14.0f;



#endif
