/*------------------------------------------------------------------------
Copyright (C) 2002-2020  Tom Murphy 7 and Sophia Poirier

This file is part of BrokenFFT.

BrokenFFT is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

BrokenFFT is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with BrokenFFT.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#ifndef _DFX_BROKENFFT_DEF_H
#define _DFX_BROKENFFT_DEF_H


#include "dfxplugin-prefix.h"


#define PLUGIN_NAME_STRING	"BrokenFFT"
#define PLUGIN_ID		FOURCC('D', 'F', 'b', 'f')
#define PLUGIN_VERSION_MAJOR	3
#define PLUGIN_VERSION_MINOR	0
#define PLUGIN_VERSION_BUGFIX	0
#define PLUGIN_VERSION_STRING	"3.0.0"
#define PLUGIN_CLASS_NAME	BrokenFFT
#define PLUGIN_ENTRY_POINT	"BrokenFFTEntry"
#define TARGET_PLUGIN_USES_MIDI	1
#define TARGET_PLUGIN_IS_INSTRUMENT	0
#define TARGET_PLUGIN_USES_DSPCORE	1
// XXX yes but not yet
#define TARGET_PLUGIN_HAS_GUI	1
#define PLUGIN_BACKGROUND_IMAGE_FILENAME	"brokenfft-background.png"
#define PLUGIN_BUNDLE_IDENTIFIER	DESTROYFX_BUNDLE_ID_PREFIX PLUGIN_NAME_STRING DFX_BUNDLE_ID_SUFFIX
#define PLUGIN_COPYRIGHT_YEAR_STRING	"2002-2020"
#define VST_NUM_CHANNELS	1

// optional
#define PLUGIN_DESCRIPTION_STRING	"frequency domain circuit bending"


#undef DFX_SUPPORT_OLD_VST_SETTINGS


#endif
