#ifndef __GEOMETER_DEF_H
#define __GEOMETER_DEF_H


#include "dfxplugin-prefix.h"


#define PLUGIN_NAME_STRING	"Geometer"
#define PLUGIN_ID	'DFgr'
#define PLUGIN_VERSION_MAJOR	1
#define PLUGIN_VERSION_MINOR	2
#define PLUGIN_VERSION_BUGFIX	0
#define PLUGIN_VERSION_STRING	"1.2.0"
#define PLUGIN_ENTRY_POINT	"GeometerEntry"
#define TARGET_PLUGIN_USES_MIDI	1
#define TARGET_PLUGIN_IS_INSTRUMENT	0
#define TARGET_PLUGIN_USES_DSPCORE	1
#define TARGET_PLUGIN_HAS_GUI	1

// only necessary if using a custom GUI
#define PLUGIN_EDITOR_ENTRY_POINT	"GeometerEditorEntry"

// optional
#define PLUGIN_DESCRIPTION_STRING	"visually oriented waveform geometry"
#define PLUGIN_EDITOR_DESCRIPTION_STRING	"interactive interface for DFX GEOMETER"


#define DFX_SUPPORT_OLD_VST_SETTINGS


#endif
