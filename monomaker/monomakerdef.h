#define PLUGIN_NAME_STRING	"Monomaker"
#define PLUGIN_DOUBLE_NAME_STRING	"Destroy FX: Monomaker"
#define PLUGIN_ID	'mono'
#define PLUGIN_VERSION	0x00010100
#define PLUGIN_ENTRY_POINT	"MonomakerEntry"
#define TARGET_PLUGIN_USES_MIDI	0
#define TARGET_PLUGIN_IS_INSTRUMENT	0
#define TARGET_PLUGIN_USES_DSPCORE	0
#ifndef TARGET_PLUGIN_HAS_GUI
#define TARGET_PLUGIN_HAS_GUI	1
#endif

// only necessary if using a custom GUI
#if TARGET_PLUGIN_HAS_GUI
	#define PLUGIN_EDITOR_DOUBLE_NAME_STRING	"Destroy FX: Monomaker editor"
	#define PLUGIN_EDITOR_ENTRY_POINT	"MonomakerEditorEntry"
#endif

// optional
#define PLUGIN_DESCRIPTION_STRING	"a simple mono-merging and stereo-recentering utility"
#define PLUGIN_RES_ID	3000
#if TARGET_PLUGIN_HAS_GUI
	#define PLUGIN_EDITOR_ID	'monV'
	#define PLUGIN_EDITOR_DESCRIPTION_STRING	"virtual reality interface for DFX Monomaker"
	#define PLUGIN_EDITOR_RES_ID	9000
#endif


#define DFX_SUPPORT_OLD_VST_SETTINGS 1
