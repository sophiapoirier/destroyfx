#define PLUGIN_NAME_STRING	"EQ Sync"
#define PLUGIN_DOUBLE_NAME_STRING	"Destroy FX: EQ Sync"
#define PLUGIN_ID	'EQsy'
#define PLUGIN_VERSION	0x00010100
#define PLUGIN_ENTRY_POINT	"EQsyncEntry"
#define TARGET_PLUGIN_USES_MIDI	0
#define TARGET_PLUGIN_IS_INSTRUMENT	0
#define TARGET_PLUGIN_USES_DSPCORE	0
#ifndef TARGET_PLUGIN_HAS_GUI
#define TARGET_PLUGIN_HAS_GUI	1
#endif

// only necessary if using a custom GUI
#define PLUGIN_EDITOR_DOUBLE_NAME_STRING	"Destroy FX: EQ Sync editor"
#define PLUGIN_EDITOR_ENTRY_POINT	"EQsyncEditorEntry"

// optional
#define PLUGIN_DESCRIPTION_STRING	"EQ Sync is a ridiculous equalizer.  You have no control over the EQ curve, if you try to adjust the faders they will escape, and the EQ parameters don't make any sense anyway.  The EQ setting changes on its own according to your song's tempo."
#define PLUGIN_EDITOR_DESCRIPTION_STRING	"lickable interface for DFX EQ Sync"


#define DFX_SUPPORT_OLD_VST_SETTINGS 0
