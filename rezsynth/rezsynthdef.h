#define PLUGIN_NAME_STRING	"Rez Synth"
#define PLUGIN_DOUBLE_NAME_STRING	"Destroy FX: Rez Synth"
#define PLUGIN_ID	'RezS'
#define PLUGIN_VERSION	0x00010300
#define PLUGIN_ENTRY_POINT	"RezSynthEntry"
#define TARGET_PLUGIN_USES_MIDI	1
#define TARGET_PLUGIN_IS_INSTRUMENT	0
#define TARGET_PLUGIN_USES_DSPCORE	0
#ifndef TARGET_PLUGIN_HAS_GUI
#define TARGET_PLUGIN_HAS_GUI	1
#endif

// only necessary if using a custom GUI
#define PLUGIN_EDITOR_DOUBLE_NAME_STRING	"Destroy FX: Rez Synth editor"
#define PLUGIN_EDITOR_ENTRY_POINT	"RezSynthEditorEntry"

// optional
#define PLUGIN_DESCRIPTION_STRING	"Rez Synth allows you to \"play\" resonant bandpass filter banks with MIDI notes"
#define PLUGIN_EDITOR_DESCRIPTION_STRING	"tiled interface for DFX Rez Synth"


#define DFX_SUPPORT_OLD_VST_SETTINGS 0
