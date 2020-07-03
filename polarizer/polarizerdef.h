/*------------------------------------------------------------------------
Copyright (C) 2002-2020  Sophia Poirier

This file is part of Polarizer.

Polarizer is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Polarizer is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Polarizer.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#ifndef DFX_POLARIZER_DEF_H
#define DFX_POLARIZER_DEF_H


#include "dfxplugin-prefix.h"


#define PLUGIN_NAME_STRING	"Polarizer"
#define PLUGIN_ID	FOURCC('p', 'o', 'l', 'a')
#define PLUGIN_VERSION_MAJOR	1
#define PLUGIN_VERSION_MINOR	1
#define PLUGIN_VERSION_BUGFIX	0
#define PLUGIN_VERSION_STRING	"1.1.0"
#define PLUGIN_CLASS_NAME	Polarizer
#define PLUGIN_ENTRY_POINT	"PolarizerEntry"
#define TARGET_PLUGIN_USES_MIDI	0
#define TARGET_PLUGIN_IS_INSTRUMENT	0
#define TARGET_PLUGIN_USES_DSPCORE	1
#define TARGET_PLUGIN_HAS_GUI	1
#define PLUGIN_BACKGROUND_IMAGE_FILENAME	"polarizer-background.png"
#define PLUGIN_BUNDLE_IDENTIFIER	DESTROYFX_BUNDLE_ID_PREFIX PLUGIN_NAME_STRING DFX_BUNDLE_ID_SUFFIX
#define PLUGIN_COPYRIGHT_YEAR_STRING	"2001-2020"
#define VST_NUM_CHANNELS	2

// optional
#define PLUGIN_DESCRIPTION_STRING	"takes every Nth sample and polarizes it; you can also implode your sound"


#endif
