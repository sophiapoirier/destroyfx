#define PLUGIN_NAME_STRING	"MIDI Gater"
#define PLUGIN_DOUBLE_NAME_STRING	"Destroy FX: MIDI Gater"
#define PLUGIN_ID	'Mgat'
#define PLUGIN_VERSION	0x00010100
#define PLUGIN_ENTRY_POINT	"MidiGaterEntry"
#define TARGET_PLUGIN_USES_MIDI	1
#define TARGET_PLUGIN_IS_INSTRUMENT	0
#define TARGET_PLUGIN_USES_DSPCORE	0
#ifndef TARGET_PLUGIN_HAS_GUI
#define TARGET_PLUGIN_HAS_GUI	1
#endif

// only necessary if using a custom GUI
#define PLUGIN_EDITOR_DOUBLE_NAME_STRING	"Destroy FX: MIDI Gater editor"
#define PLUGIN_EDITOR_ENTRY_POINT	"MidiGaterEditorEntry"

// optional
#define PLUGIN_DESCRIPTION_STRING	"a MIDI-note-controlled gate"
#define PLUGIN_EDITOR_DESCRIPTION_STRING	"fruity interface for DFX MIDI Gater"


#define DFX_SUPPORT_OLD_VST_SETTINGS 0
