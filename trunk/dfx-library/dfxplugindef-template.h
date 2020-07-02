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


#ifndef _DFXPLUGIN_DEF_TEMPLATE_H
#define _DFXPLUGIN_DEF_TEMPLATE_H


#include "dfxplugin-prefix.h"


// required
#define PLUGIN_NAME_STRING	""
// should be unique among all plugins (!) Official Destroy FX plugins use 'D' 'F'
// for the first two characters
#define PLUGIN_ID		FOURCC('A', 'B', 'C', 'D')
#define PLUGIN_VERSION_MAJOR	1
#define PLUGIN_VERSION_MINOR	0
#define PLUGIN_VERSION_BUGFIX	0
#define PLUGIN_VERSION_STRING	"1.0.0"
#define PLUGIN_CLASS_NAME	CoolPlugin
#define PLUGIN_ENTRY_POINT	"Entry"
#define PLUGIN_BUNDLE_IDENTIFIER	DESTROYFX_BUNDLE_ID_PREFIX "" DFX_BUNDLE_ID_SUFFIX
#define PLUGIN_COPYRIGHT_YEAR_STRING	"20XX"
// Define to 1 or 0
#define TARGET_PLUGIN_USES_MIDI	0
#define TARGET_PLUGIN_IS_INSTRUMENT 0
#define TARGET_PLUGIN_USES_DSPCORE 0	
#define TARGET_PLUGIN_HAS_GUI 0

// only necessary for VST
// #define VST_NUM_INPUTS	2
// #define VST_NUM_OUTPUTS	2
// or you can just define this if numinputs and numoutputs match
#define VST_NUM_CHANNELS 2

// optional
#define PLUGIN_DESCRIPTION_STRING	""
#define PLUGIN_RES_ID	
#if TARGET_PLUGIN_HAS_GUI
	#define PLUGIN_BACKGROUND_IMAGE_FILENAME	".png"
	#define PLUGIN_EDITOR_DESCRIPTION_STRING	""
#endif

// if relevant
#define DFX_SUPPORT_OLD_VST_SETTINGS 1


#endif
