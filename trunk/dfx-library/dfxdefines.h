/*------------------------------------------------------------------------
Destroy FX is a sovereign entity comprised of Marc Poirier & Tom Murphy 7.
These are our general global defines and constants, to be included 
somewhere in the include tree for every file for a DfxPlugin.
------------------------------------------------------------------------*/

#ifndef __DFXDEFINES_H
#define __DFXDEFINES_H



//-----------------------------------------------------------------------------
#define DESTROY_FX_RULEZ

#define DESTROYFX_NAME_STRING	"Destroy FX"
#define DESTROYFX_COLLECTION_NAME	"Super Destroy FX bipolar plugin pack"
#define DESTROYFX_URL "http://destroyfx.org/"
#define SMARTELECTRONIX_URL "http://www.smartelectronix.com/"

#define DESTROYFX_ID 'DFX!'

#define DFX_PARAM_MAX_NAME_LENGTH	64
#define DFX_PRESET_MAX_NAME_LENGTH	64
#define DFX_PARAM_MAX_VALUE_STRING_LENGTH	256
#define DFX_PARAM_MAX_UNIT_STRING_LENGTH	256

// interpret fractional numbers as booleans (for plugin parameters)
#define FBOOL(fvalue)	( (fvalue) != 0.0f )
#define DBOOL(dvalue)	( (dvalue) != 0.0 )



//-----------------------------------------------------------------------------
// class declarations
class DfxPlugin;
class DfxPluginCore;



#if WIN32
/* turn off warnings about default but no cases in switch, unknown pragma, etc. */
   #pragma warning( disable : 4065 57 4200 4244 4068 )
   #include <windows.h>
#endif


#endif
// __DFXDEFINES_H