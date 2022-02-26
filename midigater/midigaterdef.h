/*------------------------------------------------------------------------
Copyright (C) 2002-2022  Sophia Poirier

This file is part of MIDI Gater.

MIDI Gater is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

MIDI Gater is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with MIDI Gater.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#ifndef DFX_MIDI_GATER_DEF_H
#define DFX_MIDI_GATER_DEF_H


#include "dfxplugin-prefix.h"


#define PLUGIN_NAME_STRING	"MIDI Gater"
#define PLUGIN_ID	FOURCC('M', 'g', 'a', 't')
#define PLUGIN_VERSION_MAJOR	2
#define PLUGIN_VERSION_MINOR	0
#define PLUGIN_VERSION_BUGFIX	1
#define PLUGIN_CLASS_NAME	MIDIGater
#define TARGET_PLUGIN_USES_MIDI	1
#define TARGET_PLUGIN_IS_INSTRUMENT	0
#define TARGET_PLUGIN_USES_DSPCORE	0
#define TARGET_PLUGIN_HAS_GUI	1
#define PLUGIN_BACKGROUND_IMAGE_FILENAME	"midi-gater-background.png"
#define PLUGIN_BUNDLE_IDENTIFIER	DESTROYFX_BUNDLE_ID_PREFIX "MIDIGater" DFX_BUNDLE_ID_SUFFIX
#define PLUGIN_COPYRIGHT_YEAR_STRING	"2001-2022"
#define VST_NUM_CHANNELS	2


#endif
