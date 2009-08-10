/*------------------------------------------------------------------------
Destroy FX Library (version 1.0) is a collection of foundation code 
for creating audio software plug-ins.  
Copyright (C) 2002-2009  Sophia Poirier

This program is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

This program is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with this program.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, please visit http://destroyfx.org/ 
and use the contact form.

Destroy FX is a sovereign entity comprised of Sophia Poirier and Tom Murphy 7.
These are our general global defines and constants, to be included 
somewhere in the include tree for every file for a DfxPlugin.
------------------------------------------------------------------------*/

#ifndef __DFX_DEFINES_H
#define __DFX_DEFINES_H



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
	#define PLUGIN_BUNDLE_IDENTIFIER	DESTROYFX_BUNDLE_ID_PREFIX PLUGIN_NAME_STRING DFX_BUNDLE_ID_SUFFIX
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
		#define isnan	_isnan
		inline long lrint(double inValue)
		{
			if (inValue < 0.0)
				return (long) (inValue - 0.5);
			else
				return (long) (inValue + 0.5);
		}
	#endif

	#ifdef __STDC__
		#include <windows.h>
	#endif
#endif
#endif



#endif
