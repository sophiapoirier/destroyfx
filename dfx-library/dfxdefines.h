/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2022  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 2 of the License, or 
(at your option) any later version.

Destroy FX Library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Destroy FX Library.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/

Destroy FX is a sovereign entity comprised of Sophia Poirier and Tom Murphy 7.
These are our general global defines and constants, to be included 
somewhere in the include tree for every file for a DfxPlugin.
------------------------------------------------------------------------*/

#ifndef _DFX_DEFINES_H
#define _DFX_DEFINES_H



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
	#define PLUGIN_COLLECTION_NAME	"Super Destroy FX upsetting+delightful plugin pack"
#endif

/* XXX needs workaround for plugin names with white spaces */
#ifndef PLUGIN_BUNDLE_IDENTIFIER
	#define PLUGIN_BUNDLE_IDENTIFIER	DESTROYFX_BUNDLE_ID_PREFIX PLUGIN_NAME_STRING DFX_BUNDLE_ID_SUFFIX
#endif

#ifndef PLUGIN_ICON_FILE_NAME
	#define PLUGIN_ICON_FILE_NAME	"destroyfx.icns"
#endif

#define DESTROYFX_URL	"http://destroyfx.org"

#ifndef PLUGIN_HOMEPAGE_URL
	#define PLUGIN_HOMEPAGE_URL	DESTROYFX_URL
#endif



/*-----------------------------------------------------------------------------*/
#ifdef __STDC__
	#if _WIN32 && !defined(TARGET_API_RTAS)
		#include <windows.h>
	#endif
	#ifdef _MSC_VER
		#ifdef TARGET_API_RTAS
			typedef unsigned int	uint32_t;
			typedef __int64	int64_t;
			typedef unsigned __int64	uint64_t;
		#else
			typedef INT32	int32_t;
			typedef UINT32	uint32_t;
			typedef INT64	int64_t;
			typedef UINT64	uint64_t;
		#endif
	#else
		#include <stdint.h>
	#endif
#endif



/*-----------------------------------------------------------------------------*/
#ifdef __cplusplus

#include <cstddef>

namespace dfx
{
	// to indicate "not a real parameter" or something like that
	static constexpr long kParameterID_Invalid = -1;

	static constexpr size_t kParameterNameMaxLength = 64;
	static constexpr size_t kPresetNameMaxLength = 64;
	static constexpr size_t kParameterValueStringMaxLength = 256;
	static constexpr size_t kParameterUnitStringMaxLength = 256;
}

#endif



#endif
