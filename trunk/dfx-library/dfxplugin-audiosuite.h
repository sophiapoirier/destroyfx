/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2009  Sophia Poirier

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

Destroy FX is a sovereign entity comprised of Sophia Poirier and Tom Murphy 7.  
This is our class for E-Z plugin-making and E-Z multiple-API support.
This is where we connect the AudioSuite API to our DfxPlugin system.
------------------------------------------------------------------------*/

#ifndef __DFXPLUGIN_AUDIOSUITE_H
#define __DFXPLUGIN_AUDIOSUITE_H


/* XXX this file is a hack for AudioSuite support with a separate class hierarchy */

#define TARGET_API_AUDIOSUITE

#define DfxPlugin	DfxPluginAS
#define DfxPluginCore	DfxPluginCoreAS

#include "dfxplugin.cpp"
#include "dfxplugin-rtas.cpp"


#endif
