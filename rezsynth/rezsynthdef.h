#ifndef __REZ_SYNTH_DEF_H
#define __REZ_SYNTH_DEF_H


#include "dfxplugin-prefix.h"


#define PLUGIN_NAME_STRING	"Rez Synth"
#define PLUGIN_ID	'RezS'
#define PLUGIN_VERSION_MAJOR	1
#define PLUGIN_VERSION_MINOR	3
#define PLUGIN_VERSION_BUGFIX	0
#define PLUGIN_VERSION_STRING	"1.3.0"
#define PLUGIN_ENTRY_POINT	"RezSynthEntry"
#define TARGET_PLUGIN_USES_MIDI	1
#define TARGET_PLUGIN_IS_INSTRUMENT	0
#define TARGET_PLUGIN_USES_DSPCORE	0
#define TARGET_PLUGIN_HAS_GUI	1

// only necessary if using a custom GUI
#define PLUGIN_EDITOR_ENTRY_POINT	"RezSynthEditorEntry"

// optional
#define PLUGIN_DESCRIPTION_STRING	"Rez Synth allows you to \"play\" resonant bandpass filter banks with MIDI notes"
#define PLUGIN_EDITOR_DESCRIPTION_STRING	"tiled interface for DFX Rez Synth"


#define PLUGIN_BUNDLE_IDENTIFIER	DESTROYFX_BUNDLE_ID_PREFIX "RezSynth" DFX_BUNDLE_ID_SUFFIX


#endif
