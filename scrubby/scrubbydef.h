#define PLUGIN_NAME_STRING	"Scrubby"
#define PLUGIN_DOUBLE_NAME_STRING	"Destroy FX: Scrubby"
#define PLUGIN_ID	'scub'
#define PLUGIN_VERSION	0x00010100
#define PLUGIN_ENTRY_POINT	"ScrubbyEntry"
#define TARGET_PLUGIN_USES_MIDI	1
#define TARGET_PLUGIN_IS_INSTRUMENT	0
#define TARGET_PLUGIN_USES_DSPCORE	0
#ifndef TARGET_PLUGIN_HAS_GUI
#define TARGET_PLUGIN_HAS_GUI	1
#endif

// only necessary if using a custom GUI
#if TARGET_PLUGIN_HAS_GUI
	#define PLUGIN_EDITOR_DOUBLE_NAME_STRING	"Destroy FX: Scrubby editor"
	#define PLUGIN_EDITOR_ENTRY_POINT	"ScrubbyEditorEntry"
#endif

// optional
#define PLUGIN_DESCRIPTION_STRING	"Scrubby goes zipping around through time, doing whatever it takes to reach its destinations on time"
#define PLUGIN_RES_ID	3000
#if TARGET_PLUGIN_HAS_GUI
	#define PLUGIN_EDITOR_ID	'scuV'
	#define PLUGIN_EDITOR_DESCRIPTION_STRING	"mud brown interface for DFX Scrubby"
	#define PLUGIN_EDITOR_RES_ID	9000
#endif


#define DFX_SUPPORT_OLD_VST_SETTINGS 0
