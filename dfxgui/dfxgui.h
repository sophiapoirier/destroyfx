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

#ifndef __DFXGUI_H
#define __DFXGUI_H


#ifdef TARGET_PLUGIN_USES_VSTGUI
	#include "dfxgui-vstgui.h"

#else
	#include "dfxguieditor.h"
	#include "dfxguislider.h"
	#include "dfxguibutton.h"
	#include "dfxguitextdisplay.h"
#endif


#endif
