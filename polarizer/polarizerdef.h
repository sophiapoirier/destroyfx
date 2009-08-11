#ifndef __POLARIZER_DEF_H
#define __POLARIZER_DEF_H


#include "dfxplugin-prefix.h"


#define PLUGIN_NAME_STRING	"Polarizer"
#define PLUGIN_ID	'pola'
#define PLUGIN_VERSION_MAJOR	1
#define PLUGIN_VERSION_MINOR	1
#define PLUGIN_VERSION_BUGFIX	0
#define PLUGIN_VERSION_STRING	"1.1.0"
#define PLUGIN_ENTRY_POINT	"PolarizerEntry"
#define TARGET_PLUGIN_USES_MIDI	0
#define TARGET_PLUGIN_IS_INSTRUMENT	0
#define TARGET_PLUGIN_USES_DSPCORE	1
#define TARGET_PLUGIN_HAS_GUI	1

// only necessary if using a custom GUI
#define PLUGIN_EDITOR_ENTRY_POINT	"PolarizerEditorEntry"

// optional
#define PLUGIN_DESCRIPTION_STRING	"takes every nth sample and polarizes it; you can also implode your sound"
#define PLUGIN_EDITOR_DESCRIPTION_STRING	"official Destroy FX interface for DFX Polarizer"


#endif
