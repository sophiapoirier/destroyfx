#define PLUGIN_NAME_STRING	"DFX Transverb"
#define PLUGIN_DOUBLE_NAME_STRING	"Destroy FX: Transverb"
#define PLUGIN_ID	'DFtv'
#define PLUGIN_VERSION	0x00010500
#define PLUGIN_ENTRY_POINT	"TransverbEntry"
#define TARGET_PLUGIN_USES_MIDI	1
#define TARGET_PLUGIN_IS_INSTRUMENT	0
#define TARGET_PLUGIN_USES_DSPCORE	1
#ifndef TARGET_PLUGIN_HAS_GUI
#define TARGET_PLUGIN_HAS_GUI	1
#endif

// only necessary if using a custom GUI
#define PLUGIN_EDITOR_DOUBLE_NAME_STRING	"Destroy FX: Transverb editor"
#define PLUGIN_EDITOR_ENTRY_POINT	"TransverbEditorEntry"

// optional
#define PLUGIN_DESCRIPTION_STRING	"like a delay that can play back the delay buffer at different speeds"
#define PLUGIN_EDITOR_DESCRIPTION_STRING	"pretty blue interface for DFX TRANSVERB"


#define USING_HERMITE	1
#define PRINT_FUNCTION_ALERTS 0
