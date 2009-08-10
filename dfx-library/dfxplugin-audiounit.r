/*------------------------------------------------------------------------
Destroy FX Library (version 1.0) is a collection of foundation code 
for creating audio software plug-ins.  
Copyright (C) 2002-2009  Sophia Poirier

This program is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

This program is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with this program.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, please visit http://destroyfx.org/ 
and use the contact form.

Destroy FX is a sovereign entity comprised of Sophia Poirier and Tom Murphy 7.  
This is our Audio Unit resource stuff.
written by Sophia Poirier, October 2002
------------------------------------------------------------------------*/

#include "AudioUnit.r"

#include "dfxdefines.h"



#if defined(PLUGIN_VERSION_MAJOR) && defined(PLUGIN_VERSION_MINOR) && defined(PLUGIN_VERSION_BUGFIX)
	#define PLUGIN_VERSION_COMPOSITE	((PLUGIN_VERSION_MAJOR * 0x10000) | (PLUGIN_VERSION_MINOR * 0x100) | PLUGIN_VERSION_BUGFIX)
#else
	#define PLUGIN_VERSION_COMPOSITE	0x00010000
#endif


//----------------------------------------------------------------------------- 
// resources for the base plugin component

// Note that resource IDs must be spaced 2 apart for the 'STR ' name and description
#ifdef PLUGIN_RES_ID
	#define RES_ID	PLUGIN_RES_ID
#else
	#define RES_ID	3000
#endif

#if TARGET_PLUGIN_IS_INSTRUMENT
	#define COMP_TYPE	kAudioUnitType_MusicDevice
#elif TARGET_PLUGIN_USES_MIDI
	#define COMP_TYPE	kAudioUnitType_MusicEffect
#else
	#define COMP_TYPE	kAudioUnitType_Effect
#endif

#define COMP_SUBTYPE	PLUGIN_ID

#define COMP_MANUF	PLUGIN_CREATOR_ID

#define VERSION	PLUGIN_VERSION_COMPOSITE

#define NAME	PLUGIN_CREATOR_NAME_STRING": "PLUGIN_NAME_STRING

#ifdef PLUGIN_DESCRIPTION_STRING
	#define DESCRIPTION	PLUGIN_DESCRIPTION_STRING
#else
	#define DESCRIPTION	PLUGIN_CREATOR_NAME_STRING" "PLUGIN_NAME_STRING" = super destroy noise machine"
#endif

#define ENTRY_POINT	PLUGIN_ENTRY_POINT

#include "AUResources.r"



#if TARGET_PLUGIN_HAS_GUI
//----------------------------------------------------------------------------- 
// resources for the plugin editor component

#ifdef PLUGIN_EDITOR_RES_ID
	#define RES_ID	PLUGIN_EDITOR_RES_ID
#else
	#define RES_ID	9000
#endif

#define COMP_TYPE	kAudioUnitCarbonViewComponentType

#ifdef PLUGIN_EDITOR_ID
	#define COMP_SUBTYPE	PLUGIN_EDITOR_ID
#else
	// if not defined, use the base plugin ID
	#define COMP_SUBTYPE	PLUGIN_ID
#endif

#define COMP_MANUF	PLUGIN_CREATOR_ID

#define VERSION	PLUGIN_VERSION_COMPOSITE

#define NAME	PLUGIN_CREATOR_NAME_STRING": "PLUGIN_NAME_STRING" editor"

#ifdef PLUGIN_EDITOR_DESCRIPTION_STRING
	#define DESCRIPTION	PLUGIN_EDITOR_DESCRIPTION_STRING
#else
	#define DESCRIPTION	PLUGIN_CREATOR_NAME_STRING" "PLUGIN_NAME_STRING" picturesque yet functional interface"
#endif

#define ENTRY_POINT	PLUGIN_EDITOR_ENTRY_POINT

#include "AUResources.r"

#endif
// TARGET_PLUGIN_HAS_GUI
