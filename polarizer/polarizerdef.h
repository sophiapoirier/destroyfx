#define PLUGIN_NAME_STRING	"Polarizer"
#define PLUGIN_ID	'pola'
#define PLUGIN_VERSION	0x00010100
#define PLUGIN_ENTRY_POINT	"PolarizerEntry"
#define TARGET_PLUGIN_USES_MIDI	0
#define TARGET_PLUGIN_IS_INSTRUMENT	0
#define TARGET_PLUGIN_USES_DSPCORE	1
#ifndef TARGET_PLUGIN_HAS_GUI
#define TARGET_PLUGIN_HAS_GUI	1
#endif

// only necessary if using a custom GUI
#define PLUGIN_EDITOR_ENTRY_POINT	"PolarizerEditorEntry"

// optional
#define PLUGIN_DESCRIPTION_STRING	"takes every nth sample and polarizes it; you can also implode your sound"
#define PLUGIN_EDITOR_DESCRIPTION_STRING	"official DFX interface for DFX Polarizer"


#define DFX_SUPPORT_OLD_VST_SETTINGS 0
