/*------------------------------------------------------------------------
Copyright (C) 2002-2018  Tom Murphy 7 and Sophia Poirier

This file is part of Transverb.

Transverb is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Transverb is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Transverb.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#ifndef TOM7_TRANSVERB_DEF_H
#define TOM7_TRANSVERB_DEF_H


#include "dfxplugin-prefix.h"


#define PLUGIN_NAME_STRING	"Transverb"
#define PLUGIN_ID	'DFtv'
#define PLUGIN_VERSION_MAJOR	1
#define PLUGIN_VERSION_MINOR	5
#define PLUGIN_VERSION_BUGFIX	0
#define PLUGIN_VERSION_STRING	"1.5.0"
#define PLUGIN_CLASS_NAME	Transverb
#define PLUGIN_ENTRY_POINT	"TransverbEntry"
#define TARGET_PLUGIN_USES_MIDI	1
#define TARGET_PLUGIN_IS_INSTRUMENT	0
#define TARGET_PLUGIN_USES_DSPCORE	1
#define TARGET_PLUGIN_HAS_GUI	1
#define PLUGIN_BACKGROUND_IMAGE_FILENAME	"transverb-background.png"

// optional
#define PLUGIN_DESCRIPTION_STRING	"like a delay that can play back the delay buffer at different speeds"


#define PLUGIN_COPYRIGHT_YEAR_STRING	"2001-2018"


#define USING_HERMITE	1
#define DFX_SUPPORT_OLD_VST_SETTINGS


#endif
