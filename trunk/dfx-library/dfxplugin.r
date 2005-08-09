/*------------------------------------------------------------------------
Destroy FX is a sovereign entity comprised of Marc Poirier & Tom Murphy 7.  
This is our Audio Unit resource stuff.
written by Marc Poirier, October 2002
------------------------------------------------------------------------*/

#include "AudioUnit.r"

#include "dfxdefines.h"


//----------------------------------------------------------------------------- 
// general

// define both of these so that we produce the ComponentResources that indicate 
// "universal binaries" (executables with code for PowerPC and x86)
#define TARGET_REZ_MAC_PPC	1
#define TARGET_REZ_MAC_X86	1


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

#define COMP_MANUF	'DFX!'

#ifdef PLUGIN_VERSION
	#define VERSION	PLUGIN_VERSION
#else
	#define VERSION	0x00010000
#endif

#ifdef PLUGIN_DOUBLE_NAME_STRING
	#define NAME	PLUGIN_DOUBLE_NAME_STRING
#else
	#define NAME	DESTROYFX_NAME_STRING": "PLUGIN_NAME_STRING
#endif

#ifdef PLUGIN_DESCRIPTION_STRING
	#define DESCRIPTION	PLUGIN_DESCRIPTION_STRING
#else
	#define DESCRIPTION	DESTROYFX_NAME_STRING" "PLUGIN_NAME_STRING" = super destroy noise machine"
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

#define COMP_MANUF	'DFX!'

#ifdef PLUGIN_VERSION
	#define VERSION	PLUGIN_VERSION
#else
	#define VERSION	0x00010000
#endif

#ifdef PLUGIN_EDITOR_DOUBLE_NAME_STRING
	#define NAME	PLUGIN_EDITOR_DOUBLE_NAME_STRING
#else
	#define NAME	DESTROYFX_NAME_STRING": "PLUGIN_NAME_STRING" editor"
#endif

#ifdef PLUGIN_EDITOR_DESCRIPTION_STRING
	#define DESCRIPTION	PLUGIN_EDITOR_DESCRIPTION_STRING
#else
	#define DESCRIPTION	DESTROYFX_NAME_STRING" "PLUGIN_NAME_STRING" picturesque yet functional interface"
#endif

#define ENTRY_POINT	PLUGIN_EDITOR_ENTRY_POINT

#include "AUResources.r"

#endif
// TARGET_PLUGIN_HAS_GUI
