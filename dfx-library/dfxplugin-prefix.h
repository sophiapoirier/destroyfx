/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2009-2021  Sophia Poirier and Tom Murphy 7

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
This file should be included by any prefix header that you use for your plugin.
------------------------------------------------------------------------*/

#ifndef _DFXPLUGIN_PREFIX_H
#define _DFXPLUGIN_PREFIX_H

/*-----------------------------------------------------------------------------*/
/* General stuff */

/* Give a plugin ID using character constants, like
   FOURCC('D', 'F', 'x', 'p')
   The result always in big-endian order (FOURCC(1, 2, 3, 4) == 0x01020304).
*/
#define FOURCC(a, b, c, d) (((unsigned int)(a) << 24) | ((unsigned int)(b) << 16) | \
                            ((unsigned int)(c) << 8) | ((unsigned int)(d)))

/*-----------------------------------------------------------------------------*/
/* Mac stuff */
#define DESTROYFX_BUNDLE_ID_PREFIX	"org.destroyfx."

#ifdef TARGET_API_AUDIOUNIT
	#define DFX_BUNDLE_ID_SUFFIX	".AU"
#elif defined(TARGET_API_VST)
	#define DFX_BUNDLE_ID_SUFFIX	".VST"
#elif defined(TARGET_API_RTAS)
	#define DFX_BUNDLE_ID_SUFFIX	".RTAS"
#endif


/*-----------------------------------------------------------------------------*/
/* Windows stuff */
#ifdef _WIN32
#if _WIN32
	#ifndef WIN32
		#define WIN32	1
	#endif
	#ifdef _MSC_VER
		/* turn off warnings about default but no cases in switch, unknown pragma, etc. 
		   if using Microsoft's C compiler */
		#pragma warning( disable : 4065 57 4200 4244 4068 4326 )
		/* turn off warnings about not using secure versions of C Run-Time Library functions */
		#define _CRT_SECURE_NO_WARNINGS	1
		#define _CRT_SECURE_NO_DEPRECATE	1
	#endif
#endif
#endif

#ifdef _WIN64
#if _WIN64
	#ifndef WIN64
		#define WIN64	1
	#endif
#endif
#endif


/*-----------------------------------------------------------------------------*/
/* VST stuff */
#ifdef TARGET_API_VST
	/* require VST 2.4 (the first version with 64-bit executable support) */
	#define VST_FORCE_DEPRECATED	1
#endif


/*-----------------------------------------------------------------------------*/
/* RTAS/AudioSuite stuff */
#ifdef TARGET_API_RTAS
	#ifdef __MACH__
		#include "../ProToolsSDK/AlturaPorts/TDMPlugIns/common/Mac/CommonPlugInHeaders.pch"
	#endif
#endif


#endif
