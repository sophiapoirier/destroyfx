/*------------------------------------------------------------------------
Copyright (C) 2004-2021  Sophia Poirier

This file is part of Turntablist.

Turntablist is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 2 of the License, or 
(at your option) any later version.

Turntablist is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Turntablist.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#ifndef DFX_TURNTABLIST_DEF_H
#define DFX_TURNTABLIST_DEF_H


#include "dfxplugin-prefix.h"


#define PLUGIN_NAME_STRING	"Turntablist"
#define PLUGIN_ID	FOURCC('T', 'T', 'P', '0')
#define PLUGIN_VERSION_MAJOR	1
#define PLUGIN_VERSION_MINOR	1
#define PLUGIN_VERSION_BUGFIX	0
#define PLUGIN_VERSION_STRING	"1.1.0"
#define PLUGIN_CLASS_NAME	Turntablist
#define PLUGIN_ENTRY_POINT	"TurntablistEntry"
#define TARGET_PLUGIN_USES_MIDI	1
#define TARGET_PLUGIN_IS_INSTRUMENT	1
#define TARGET_PLUGIN_USES_DSPCORE	0
#define TARGET_PLUGIN_HAS_GUI	1
#define PLUGIN_BACKGROUND_IMAGE_FILENAME	"background.png"
#define PLUGIN_BUNDLE_IDENTIFIER	DESTROYFX_BUNDLE_ID_PREFIX PLUGIN_NAME_STRING DFX_BUNDLE_ID_SUFFIX
#define PLUGIN_COPYRIGHT_YEAR_STRING	"2004-2021"

#define PLUGIN_DESCRIPTION_STRING	PLUGIN_NAME_STRING " is an instrument that can load a sample and perform turntable style effects on it.  You can scratch, change pitch, and even spin the audio up and down, just like on a real turntable."


#endif
