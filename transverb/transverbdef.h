#ifndef __TOM7_TRANSVERB_DEF_H
#define __TOM7_TRANSVERB_DEF_H


#include "dfxplugin-prefix.h"


#define PLUGIN_NAME_STRING	"Transverb"
#define PLUGIN_ID	'DFtv'
#define PLUGIN_VERSION_MAJOR	1
#define PLUGIN_VERSION_MINOR	5
#define PLUGIN_VERSION_BUGFIX	0
#define PLUGIN_VERSION_STRING	"1.5.0"
#define PLUGIN_ENTRY_POINT	"TransverbEntry"
#define TARGET_PLUGIN_USES_MIDI	1
#define TARGET_PLUGIN_IS_INSTRUMENT	0
#define TARGET_PLUGIN_USES_DSPCORE	1
#define TARGET_PLUGIN_HAS_GUI	1

// only necessary if using a custom GUI
#define PLUGIN_EDITOR_ENTRY_POINT	"TransverbEditorEntry"

// optional
#define PLUGIN_DESCRIPTION_STRING	"like a delay that can play back the delay buffer at different speeds"
#define PLUGIN_EDITOR_DESCRIPTION_STRING	"pretty blue interface for DFX TRANSVERB"


#define USING_HERMITE	1
#define DFX_SUPPORT_OLD_VST_SETTINGS
#define PRINT_FUNCTION_ALERTS 0


#endif
