#ifndef __SCRUBBY_DEF_H
#define __SCRUBBY_DEF_H


#include "dfxplugin-prefix.h"


#define PLUGIN_NAME_STRING	"Scrubby"
#define PLUGIN_ID	'scub'
#define PLUGIN_VERSION_MAJOR	1
#define PLUGIN_VERSION_MINOR	1
#define PLUGIN_VERSION_BUGFIX	0
#define PLUGIN_VERSION_STRING	"1.1.0"
#define PLUGIN_ENTRY_POINT	"ScrubbyEntry"
#define TARGET_PLUGIN_USES_MIDI	1
#define TARGET_PLUGIN_IS_INSTRUMENT	0
#define TARGET_PLUGIN_USES_DSPCORE	0
#define TARGET_PLUGIN_HAS_GUI	1

// only necessary if using a custom GUI
#define PLUGIN_EDITOR_ENTRY_POINT	"ScrubbyEditorEntry"

// optional
#define PLUGIN_DESCRIPTION_STRING	"Scrubby goes zipping around through time, doing whatever it takes to reach its destinations on time"
#define PLUGIN_EDITOR_DESCRIPTION_STRING	"mud brown interface for DFX Scrubby"


#endif
