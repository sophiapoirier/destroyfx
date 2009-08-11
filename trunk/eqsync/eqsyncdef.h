#ifndef __EQ_SYNC_DEF_H
#define __EQ_SYNC_DEF_H


#include "dfxplugin-prefix.h"


#define PLUGIN_NAME_STRING	"EQ Sync"
#define PLUGIN_ID	'EQsy'
#define PLUGIN_VERSION_MAJOR	1
#define PLUGIN_VERSION_MINOR	1
#define PLUGIN_VERSION_BUGFIX	0
#define PLUGIN_VERSION_STRING	"1.1.0"
#define PLUGIN_ENTRY_POINT	"EQSyncEntry"
#define TARGET_PLUGIN_USES_MIDI	0
#define TARGET_PLUGIN_IS_INSTRUMENT	0
#define TARGET_PLUGIN_USES_DSPCORE	0
#define TARGET_PLUGIN_HAS_GUI	1

// only necessary if using a custom GUI
#define PLUGIN_EDITOR_ENTRY_POINT	"EQSyncEditorEntry"

// optional
#define PLUGIN_DESCRIPTION_STRING	"EQ Sync is a ridiculous equalizer.  You have no control over the EQ curve, if you try to adjust the faders they will escape, and the EQ parameters don't make any sense anyway.  The EQ setting changes on its own according to your song's tempo."
#define PLUGIN_EDITOR_DESCRIPTION_STRING	"lickable interface for DFX EQ Sync"


#define PLUGIN_BUNDLE_IDENTIFIER	DESTROYFX_BUNDLE_ID_PREFIX "EQSync" DFX_BUNDLE_ID_SUFFIX


#endif
