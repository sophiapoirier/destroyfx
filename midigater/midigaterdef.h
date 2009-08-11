#ifndef __MIDI_GATER_DEF_H
#define __MIDI_GATER_DEF_H


#include "dfxplugin-prefix.h"


#define PLUGIN_NAME_STRING	"MIDI Gater"
#define PLUGIN_ID	'Mgat'
#define PLUGIN_VERSION_MAJOR	1
#define PLUGIN_VERSION_MINOR	1
#define PLUGIN_VERSION_BUGFIX	0
#define PLUGIN_VERSION_STRING	"1.1.0"
#define PLUGIN_ENTRY_POINT	"MidiGaterEntry"
#define TARGET_PLUGIN_USES_MIDI	1
#define TARGET_PLUGIN_IS_INSTRUMENT	0
#define TARGET_PLUGIN_USES_DSPCORE	0
#define TARGET_PLUGIN_HAS_GUI	1

// only necessary if using a custom GUI
#define PLUGIN_EDITOR_ENTRY_POINT	"MidiGaterEditorEntry"

// optional
#define PLUGIN_DESCRIPTION_STRING	"a MIDI-note-controlled gate"
#define PLUGIN_EDITOR_DESCRIPTION_STRING	"Nord Modular rip-off interface for DFX MIDI Gater"


#define PLUGIN_BUNDLE_IDENTIFIER	DESTROYFX_BUNDLE_ID_PREFIX "MIDIGater" DFX_BUNDLE_ID_SUFFIX


#endif
