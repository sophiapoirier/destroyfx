
/* You need to redefine this stuff in order to make your plugin.
   see dfxplugin.h for details. */

#define PLUGIN_NAME_STRING	"SLOWFT"
#define PLUGIN_ID		FOURCC('D', 'F', 's', 'f')
#define PLUGIN_VERSION	0x00010100
#define PLUGIN_ENTRY_POINT	"SlowftEntry"

#define TARGET_PLUGIN_USES_MIDI	1
#define TARGET_PLUGIN_IS_INSTRUMENT	0
#define TARGET_PLUGIN_USES_DSPCORE	1

#ifndef TARGET_PLUGIN_HAS_GUI
#define TARGET_PLUGIN_HAS_GUI	0
#endif

// optional
#define PLUGIN_DESCRIPTION_STRING	"i forgot to change the description"
#define PLUGIN_EDITOR_DESCRIPTION_STRING	"i forgot to change the gui description too"
