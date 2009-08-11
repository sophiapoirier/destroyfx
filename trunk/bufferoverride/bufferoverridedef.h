#ifndef __BUFFER_OVERRIDE_DEF_H
#define __BUFFER_OVERRIDE_DEF_H


#include "dfxplugin-prefix.h"


#define PLUGIN_NAME_STRING	"Buffer Override"
#define PLUGIN_ID	'buff'
#define PLUGIN_VERSION_MAJOR	2
#define PLUGIN_VERSION_MINOR	1
#define PLUGIN_VERSION_BUGFIX	0
#define PLUGIN_VERSION_STRING	"2.1.0"
#define PLUGIN_ENTRY_POINT	"BufferOverrideEntry"
#define TARGET_PLUGIN_USES_MIDI	1
#define TARGET_PLUGIN_IS_INSTRUMENT	0
#define TARGET_PLUGIN_USES_DSPCORE	0
#define TARGET_PLUGIN_HAS_GUI	1

// only necessary if using a custom GUI
#define PLUGIN_EDITOR_ENTRY_POINT	"BufferOverrideEditorEntry"

// optional
#define PLUGIN_DESCRIPTION_STRING	"overcome your audio processing buffer size and then (unsuccessfully) override that new buffer size to be a smaller buffer size"
#define PLUGIN_EDITOR_DESCRIPTION_STRING	"pretty pink interface for DFX Buffer Override"


#define PLUGIN_BUNDLE_IDENTIFIER	DESTROYFX_BUNDLE_ID_PREFIX "BufferOverride" DFX_BUNDLE_ID_SUFFIX


#endif
