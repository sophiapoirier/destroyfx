#define PLUGIN_NAME_STRING	"DFX GEOMETER"
#define PLUGIN_DOUBLE_NAME_STRING	"Destroy FX: Geometer"
#define PLUGIN_ID	'DFgr'
#define PLUGIN_VERSION	0x00010100
#define PLUGIN_ENTRY_POINT	"GeometerEntry"
#define TARGET_PLUGIN_USES_MIDI	1
#define TARGET_PLUGIN_IS_INSTRUMENT	0
#define TARGET_PLUGIN_USES_DSPCORE	1
#ifndef TARGET_PLUGIN_HAS_GUI
#define TARGET_PLUGIN_HAS_GUI	1
#endif

// only necessary if using a custom GUI
#define PLUGIN_EDITOR_DOUBLE_NAME_STRING	"Destroy FX: Geometer editor"
#define PLUGIN_EDITOR_ENTRY_POINT	"GeometerEditorEntry"

// optional
#define PLUGIN_DESCRIPTION_STRING	"visually oriented waveform geometry"
#define PLUGIN_EDITOR_DESCRIPTION_STRING	"interactive interface for DFX GEOMETER"


#define DFX_SUPPORT_OLD_VST_SETTINGS 1
