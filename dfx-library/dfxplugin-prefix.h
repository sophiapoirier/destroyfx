/*------------------------------------------------------------------------
Destroy FX Library (version 1.0) is a collection of foundation code 
for creating audio software plug-ins.  
Copyright (C) 2009  Sophia Poirier

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
This file should be included by any prefix header that you use for your plugin.
------------------------------------------------------------------------*/

#ifndef __DFXPLUGIN_PREFIX_H
#define __DFXPLUGIN_PREFIX_H


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
#ifdef WIN32
#if WIN32
	#ifdef _MSC_VER
		/* turn off warnings about default but no cases in switch, unknown pragma, etc. 
		   if using Microsoft's C compiler */
		#pragma warning( disable : 4065 57 4200 4244 4068 4326 )
		/* turn off warnings about not using secure versions of C Run-Time Library functions */
		#define _CRT_SECURE_NO_WARNINGS	1
	#endif
#endif
#endif


/*-----------------------------------------------------------------------------*/
/* VST stuff */
#ifdef TARGET_API_VST
	/* the Mac x86 baseline VST API version is VST 2.4 */
	#ifndef VST_FORCE_DEPRECATED
		#if defined(__MACH__) && !__ppc__
			#define VST_FORCE_DEPRECATED	1
		#else
			#define VST_FORCE_DEPRECATED	0
		#endif
	#endif

	#define TARGET_PLUGIN_USES_VSTGUI
#endif


/*-----------------------------------------------------------------------------*/
/* VSTGUI stuff */
#ifdef TARGET_PLUGIN_USES_VSTGUI
	#ifdef __MACH__
		#include "vstplugsquartz.h"
	#endif

	#if WIN32
		#define USE_LIBPNG	1
	#endif
#endif


#endif
