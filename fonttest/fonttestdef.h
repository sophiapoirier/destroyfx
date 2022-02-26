/*------------------------------------------------------------------------
Copyright (C) 2021-2022  Tom Murphy 7 and Sophia Poirier

This file is part of FontTest.

FontTest is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

FontTest is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with FontTest.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#ifndef DFX_FONTTESTDEF_H
#define DFX_FONTTESTDEF_H


#include "dfxplugin-prefix.h"


#define PLUGIN_NAME_STRING	"FontTest"
#define PLUGIN_ID		FOURCC('D', 'F', 'f', '?')

#define PLUGIN_VERSION_MAJOR	1
#define PLUGIN_VERSION_MINOR	0
#define PLUGIN_VERSION_BUGFIX	0
#define PLUGIN_CLASS_NAME	FontTest
#define TARGET_PLUGIN_USES_MIDI	0
#define TARGET_PLUGIN_IS_INSTRUMENT	0
#define TARGET_PLUGIN_USES_DSPCORE	0
#define TARGET_PLUGIN_HAS_GUI	1
#define PLUGIN_BACKGROUND_IMAGE_FILENAME	"fonttest-background.png"


#define PLUGIN_COPYRIGHT_YEAR_STRING	"2021-2022"

#define VST_NUM_CHANNELS 2


#endif
