/*------------------------------------------------------------------------
Destroy FX is a sovereign entity comprised of Sophia Poirier and Tom Murphy 7.
These are our general global defines and constants, to be included 
somewhere in the include tree for every file for a DfxPlugin.
------------------------------------------------------------------------*/

#ifndef __DFXDEFINES_H
#define __DFXDEFINES_H



/*-----------------------------------------------------------------------------*/
/* XXX Microsoft's crummy C compiler doesn't define __STDC__ when its own C extensions are enabled */
#ifdef _MSC_EXTENSIONS
	#ifndef __STDC__
		#define __STDC__	1
	#endif
#endif



/*-----------------------------------------------------------------------------*/
#define DESTROY_FX_RULEZ

#ifndef PLUGIN_CREATOR_NAME_STRING
	#define PLUGIN_CREATOR_NAME_STRING	"Destroy FX"
#endif

#define DESTROYFX_CREATOR_ID	'DFX!'
#ifndef PLUGIN_CREATOR_ID
	#define PLUGIN_CREATOR_ID	DESTROYFX_CREATOR_ID
#endif

#ifndef PLUGIN_COLLECTION_NAME
	#define PLUGIN_COLLECTION_NAME	"Super Destroy FX bipolar plugin pack"
#endif

/* XXX needs workaround for plugin names with white spaces */
#ifndef PLUGIN_BUNDLE_IDENTIFIER
	#define PLUGIN_BUNDLE_IDENTIFIER	"org.destroyfx."PLUGIN_NAME_STRING
#endif

#ifndef PLUGIN_ICON_FILE_NAME
	#define PLUGIN_ICON_FILE_NAME	"destroyfx.icns"
#endif

#define DESTROYFX_URL	"http://destroyfx.org/"
#define SMARTELECTRONIX_URL	"http://smartelectronix.com/"

#ifndef PLUGIN_HOMEPAGE_URL
	#define PLUGIN_HOMEPAGE_URL	DESTROYFX_URL
#endif



/*-----------------------------------------------------------------------------*/
#ifdef __STDC__
	/* to indicate "not a real parameter" or something like that */
	const long DFX_PARAM_INVALID_ID = -1;

	const long DFX_PARAM_MAX_NAME_LENGTH = 64;
	const long DFX_PRESET_MAX_NAME_LENGTH = 64;
	const long DFX_PARAM_MAX_VALUE_STRING_LENGTH = 256;
	const long DFX_PARAM_MAX_UNIT_STRING_LENGTH = 256;

	/* interpret fractional numbers as booleans (for plugin parameters) */
	#ifndef __cplusplus
		#include <stdbool.h>
	#endif
	inline bool FBOOL(float inValue)
	{
		return ( (inValue) != 0.0f );
	}
	inline bool DBOOL(double inValue)
	{
		return ( (inValue) != 0.0 );
	}
#endif



/*-----------------------------------------------------------------------------*/
/* class declarations */
#ifdef __cplusplus
	class DfxPlugin;
	class DfxPluginCore;
#endif



/*-----------------------------------------------------------------------------*/
/* Windows stuff */
#ifdef WIN32
#if WIN32
	#ifdef _MSC_VER
		/* turn off warnings about default but no cases in switch, unknown pragma, etc. 
		   if using Microsoft's C compiler */
		#pragma warning( disable : 4065 57 4200 4244 4068 4326 )
	#endif
	#include <windows.h>
#endif
#endif



#endif
/* __DFXDEFINES_H */
