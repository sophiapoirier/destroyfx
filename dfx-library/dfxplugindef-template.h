#ifndef __DFXPLUGIN_DEF_TEMPLATE_H
#define __DFXPLUGIN_DEF_TEMPLATE_H


#include "dfxplugin-prefix.h"


// required
#define PLUGIN_NAME_STRING	""
#define PLUGIN_ID	''
#define PLUGIN_VERSION_MAJOR	
#define PLUGIN_VERSION_MINOR	
#define PLUGIN_VERSION_BUGFIX	
#define PLUGIN_ENTRY_POINT	"Entry"
#define TARGET_PLUGIN_USES_MIDI	
#define TARGET_PLUGIN_IS_INSTRUMENT	
#define TARGET_PLUGIN_USES_DSPCORE	
#define TARGET_PLUGIN_HAS_GUI	
// only define one of the following 2
#define TARGET_API_AUDIOUNIT
#define TARGET_API_VST

// only necessary if using a custom GUI
#if TARGET_PLUGIN_HAS_GUI
	#define PLUGIN_EDITOR_ENTRY_POINT	"EditorEntry"
#endif

// only necessary for VST
#define VST_NUM_INPUTS	
#define VST_NUM_OUTPUTS	
// or you can just define this if numinputs and numoutputs match
#define VST_NUM_CHANNELS

// optional
#define PLUGIN_DESCRIPTION_STRING	""
#define PLUGIN_RES_ID	
#if TARGET_PLUGIN_HAS_GUI
	#define PLUGIN_EDITOR_DESCRIPTION_STRING	""
	#define PLUGIN_EDITOR_ID	''
	#define PLUGIN_EDITOR_RES_ID	
#endif
// if relavant
#define DFX_SUPPORT_OLD_VST_SETTINGS 


#endif
