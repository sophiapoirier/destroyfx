#ifndef __SKIDDER_DEF_H
#define __SKIDDER_DEF_H


#include "dfxplugin-prefix.h"


#define PLUGIN_NAME_STRING	"Skidder"
#define PLUGIN_ID	'skid'
#define PLUGIN_VERSION_MAJOR	1
#define PLUGIN_VERSION_MINOR	5
#define PLUGIN_VERSION_BUGFIX	0
#define PLUGIN_VERSION_STRING	"1.5.0"
#define PLUGIN_ENTRY_POINT	"SkidderEntry"
#define TARGET_PLUGIN_USES_MIDI	1
#define TARGET_PLUGIN_IS_INSTRUMENT	0
#define TARGET_PLUGIN_USES_DSPCORE	0
#ifndef TARGET_PLUGIN_HAS_GUI
	#define TARGET_PLUGIN_HAS_GUI	0
#endif

// only necessary if using a custom GUI
#define PLUGIN_EDITOR_ENTRY_POINT	"SkidderEditorEntry"

// optional
#define PLUGIN_DESCRIPTION_STRING	"Skidder turns your sound on and off"
#define PLUGIN_EDITOR_DESCRIPTION_STRING	"bland blue interface for DFX Skidder"


#endif
