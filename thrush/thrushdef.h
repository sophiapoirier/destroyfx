/*------------------------------------------------------------------------
Copyright (C) 2001-2021  Sophia Poirier and Keith Fullerton Whitman

This file is part of Thrush.

Thrush is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Thrush is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Thrush.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#ifndef DFX_THRUSH_DEF_H
#define DFX_THRUSH_DEF_H


#include "dfxplugin-prefix.h"

#define PLUGIN_NAME_STRING	"Thrush"
#define PLUGIN_ID	FOURCC('t', 'h', 's', 'h')
#define PLUGIN_VERSION_MAJOR	1
#define PLUGIN_VERSION_MINOR	0
#define PLUGIN_VERSION_BUGFIX	0
#define PLUGIN_CLASS_NAME	Thrush
#define PLUGIN_ENTRY_POINT	"ThrushFactory"
#define TARGET_PLUGIN_USES_MIDI	0
#define TARGET_PLUGIN_IS_INSTRUMENT	0
#define TARGET_PLUGIN_USES_DSPCORE	0
#define TARGET_PLUGIN_HAS_GUI	0
//#define PLUGIN_BACKGROUND_IMAGE_FILENAME	"thrush-background.png"
#define PLUGIN_COPYRIGHT_YEAR_STRING	"2001-2021"
#define VST_NUM_CHANNELS	2


#endif
