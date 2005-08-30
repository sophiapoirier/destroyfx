#define PLUGIN_NAME_STRING	"Monomaker"
#define PLUGIN_ID	'mono'
#define PLUGIN_VERSION	0x00010100
#define PLUGIN_ENTRY_POINT	"MonomakerEntry"
#define TARGET_PLUGIN_USES_MIDI	0
#define TARGET_PLUGIN_IS_INSTRUMENT	0
#define TARGET_PLUGIN_USES_DSPCORE	0
#define TARGET_PLUGIN_HAS_GUI	1

// only necessary if using a custom GUI
#define PLUGIN_EDITOR_ENTRY_POINT	"MonomakerEditorEntry"

// optional
#define PLUGIN_DESCRIPTION_STRING	"a simple mono-merging and stereo-recentering utility"
#define PLUGIN_EDITOR_DESCRIPTION_STRING	"virtual reality interface for DFX Monomaker"
