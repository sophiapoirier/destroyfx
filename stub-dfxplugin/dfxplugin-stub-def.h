/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2021  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 2 of the License, or 
(at your option) any later version.

Destroy FX Library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Destroy FX Library.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/

This file includes many important #defines for making a DfxPlugin.  
Make this a "prefix" or "entry" header file of your project, 
or somehow define this stuff another way (in compiler settings, or whatever).
------------------------------------------------------------------------*/


#ifndef _DFXPLUGIN_STUB_DEF_H
#define _DFXPLUGIN_STUB_DEF_H


#include "dfxplugin-prefix.h"


// the name of the plugin
#define PLUGIN_NAME_STRING	"DfxPlugin stub"
// a four-byte ID for the plugin
#define PLUGIN_ID		FOURCC('s', 't', 'u', 'b')
// the version
#define PLUGIN_VERSION_MAJOR	1
#define PLUGIN_VERSION_MINOR	0
#define PLUGIN_VERSION_BUGFIX	0
#define PLUGIN_VERSION_STRING	"1.0.0"
// your subclass of DfxPlugin
#define PLUGIN_CLASS_NAME	DfxStub
// Audio Unit entry point:  plugin class name appended with "Entry"
#define PLUGIN_ENTRY_POINT	"DfxStubEntry"
// macOS bundle identifier
#define PLUGIN_BUNDLE_IDENTIFIER	DESTROYFX_BUNDLE_ID_PREFIX "DfxStub" DFX_BUNDLE_ID_SUFFIX
// copyright year(s) published in versioning metadata
#define PLUGIN_COPYRIGHT_YEAR_STRING	"2002-2021"
// 0 or 1
#define TARGET_PLUGIN_USES_MIDI	1
// 0 or 1	(1 implied 1 is also true for TARGET_PLUGIN_USES_MIDI)
#define TARGET_PLUGIN_IS_INSTRUMENT	0
// 0 or 1	(whether or not the plugin uses a DSP processing core class, 1-in/1-out)
#define TARGET_PLUGIN_USES_DSPCORE	0
// 0 or 1	(whether or not the plugin has its own custom graphical interface)
#define TARGET_PLUGIN_HAS_GUI	1

#if TARGET_PLUGIN_HAS_GUI
	// the file name of the GUI's background image
	#define PLUGIN_BACKGROUND_IMAGE_FILENAME	"dfxplugin-stub-background.png"
#endif


// only define one of these for a given build, not both
#define TARGET_API_AUDIOUNIT
#define TARGET_API_VST


#ifdef TARGET_API_VST
	// the number of ins and outs in VST2 must be fixed
	#define VST_NUM_INPUTS 2
	#define VST_NUM_OUTPUTS 2
	// or if they're both equal, simply this will do:
//	#define VST_NUM_CHANNELS 2
#endif


#endif
