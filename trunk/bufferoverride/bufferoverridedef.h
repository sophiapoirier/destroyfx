#define PLUGIN_NAME_STRING	"Buffer Override"
#define PLUGIN_ID	'buff'
#define PLUGIN_VERSION	0x00020100
#define PLUGIN_ENTRY_POINT	"BufferOverrideEntry"
#define TARGET_PLUGIN_USES_MIDI	1
#define TARGET_PLUGIN_IS_INSTRUMENT	0
#define TARGET_PLUGIN_USES_DSPCORE	0
#ifndef TARGET_PLUGIN_HAS_GUI
#define TARGET_PLUGIN_HAS_GUI	1
#endif

// only necessary if using a custom GUI
#define PLUGIN_EDITOR_ENTRY_POINT	"BufferOverrideEditorEntry"

// optional
#define PLUGIN_DESCRIPTION_STRING	"overcome your audio processing buffer size and then (unsuccessfully) override that new buffer size to be a smaller buffer size"
#define PLUGIN_EDITOR_DESCRIPTION_STRING	"pretty pink interface for DFX Buffer Override"


#define DFX_SUPPORT_OLD_VST_SETTINGS 0
