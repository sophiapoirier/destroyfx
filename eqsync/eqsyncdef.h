/*------------------------------------------------------------------------
Copyright (C) 2002-2021  Sophia Poirier

This file is part of EQ Sync.

EQ Sync is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

EQ Sync is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with EQ Sync.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#ifndef DFX_EQ_SYNC_DEF_H
#define DFX_EQ_SYNC_DEF_H


#include "dfxplugin-prefix.h"


#define PLUGIN_NAME_STRING	"EQ Sync"
#define PLUGIN_ID	FOURCC('E', 'Q', 's', 'y')
#define PLUGIN_VERSION_MAJOR	1
#define PLUGIN_VERSION_MINOR	1
#define PLUGIN_VERSION_BUGFIX	0
#define PLUGIN_VERSION_STRING	"1.1.0"
#define PLUGIN_CLASS_NAME	EQSync
#define PLUGIN_ENTRY_POINT	"EQSyncEntry"
#define TARGET_PLUGIN_USES_MIDI	0
#define TARGET_PLUGIN_IS_INSTRUMENT	0
#define TARGET_PLUGIN_USES_DSPCORE	0
#define TARGET_PLUGIN_HAS_GUI	1
#define PLUGIN_BACKGROUND_IMAGE_FILENAME	"eq-sync-background-panther.png"
#define PLUGIN_BUNDLE_IDENTIFIER	DESTROYFX_BUNDLE_ID_PREFIX "EQSync" DFX_BUNDLE_ID_SUFFIX
#define PLUGIN_COPYRIGHT_YEAR_STRING	"2001-2021"
#define VST_NUM_CHANNELS	2

// optional
#define PLUGIN_DESCRIPTION_STRING	PLUGIN_NAME_STRING " is a ridiculous equalizer.  You have no control over the EQ curve, if you try to adjust the faders they will escape, and the EQ parameters don't make any sense anyway.  The EQ setting changes on its own according to your song's tempo."


#endif
