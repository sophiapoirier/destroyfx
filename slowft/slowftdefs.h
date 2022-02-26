
/* You need to redefine this stuff in order to make your plugin.
   see dfxplugin.h for details. */

#ifndef DFX_SLOWFT_DEF_H
#define DFX_SLOWFT_DEF_H


#include "dfxplugin-prefix.h"

#define PLUGIN_NAME_STRING	"SLOWFT"
#define PLUGIN_ID		FOURCC('D', 'F', 's', 'f')
#define PLUGIN_VERSION_MAJOR  1
#define PLUGIN_VERSION_MINOR  1
#define PLUGIN_VERSION_BUGFIX 0

#define TARGET_PLUGIN_USES_MIDI	1
#define TARGET_PLUGIN_IS_INSTRUMENT	0
#define TARGET_PLUGIN_USES_DSPCORE	1

#define PLUGIN_COPYRIGHT_YEAR_STRING   "2005-2022"

#define VST_NUM_CHANNELS 2

#ifndef TARGET_PLUGIN_HAS_GUI
#define TARGET_PLUGIN_HAS_GUI	0
#endif


#endif
