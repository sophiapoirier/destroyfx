
/* You need to redefine this stuff in order to make your plugin.
   see dfxplugin.h for details. */

#define PLUGIN_NAME_STRING	"WINDOWINGSTUB"
#define PLUGIN_ID	'DFws'
#define PLUGIN_VERSION	0x00010100
#define PLUGIN_ENTRY_POINT	"WindowingstubEntry"

#define TARGET_PLUGIN_USES_MIDI	1
#define TARGET_PLUGIN_IS_INSTRUMENT	0
#define TARGET_PLUGIN_USES_DSPCORE	1

#ifndef TARGET_PLUGIN_HAS_GUI
#define TARGET_PLUGIN_HAS_GUI	1
#endif

/* only necessary if using a custom GUI */
#define PLUGIN_EDITOR_ENTRY_POINT	"WindowingstubEditorEntry"

// optional
#define PLUGIN_DESCRIPTION_STRING	"i forgot to change the description"
#define PLUGIN_EDITOR_DESCRIPTION_STRING	"i forgot to change the gui description too"

#define DFX_SUPPORT_OLD_VST_SETTINGS 1
