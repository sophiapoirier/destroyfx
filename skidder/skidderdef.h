#define PLUGIN_NAME_STRING	"Skidder"
#define PLUGIN_DOUBLE_NAME_STRING	"Destroy FX: Skidder"
#define PLUGIN_ID	'skid'
#define PLUGIN_VERSION	0x00010500
#define PLUGIN_ENTRY_POINT	"SkidderEntry"
#define TARGET_PLUGIN_USES_MIDI	1
#define TARGET_PLUGIN_IS_INSTRUMENT	0
#define TARGET_PLUGIN_USES_DSPCORE	0
#ifndef TARGET_PLUGIN_HAS_GUI
#define TARGET_PLUGIN_HAS_GUI	1
#endif

// only necessary if using a custom GUI
#define PLUGIN_EDITOR_DOUBLE_NAME_STRING	"Destroy FX: Skidder editor"
#define PLUGIN_EDITOR_ENTRY_POINT	"SkidderEditorEntry"

// optional
#define PLUGIN_DESCRIPTION_STRING	"Skidder turns your sound on and off"
#define PLUGIN_EDITOR_DESCRIPTION_STRING	"bland blue interface for DFX Skidder"


#define DFX_SUPPORT_OLD_VST_SETTINGS	0

#define MSKIDDER	1
