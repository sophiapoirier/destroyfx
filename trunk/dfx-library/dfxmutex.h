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
This is our mutex shit.
------------------------------------------------------------------------*/

#ifndef __DFX_MUTEX_H
#define __DFX_MUTEX_H


#if WIN32 && !defined(PLUGIN_SDK_BUILD)
	// Win32 API
	#include <Windows.h>
#elif WIN32 && defined(PLUGIN_SDK_BUILD)
#elif TARGET_OS_MAC && !defined(__MACH__)
	// Multiprocessing Services
	#include <Multiprocessing.h>
#else
	// POSIX threads library
	#include <pthread.h>
#endif



class DfxMutex
{
public:
	DfxMutex();
	~DfxMutex();
	int grab();
	int try_grab();
	int release();

private:

#if WIN32 && !defined(PLUGIN_SDK_BUILD)
	CRITICAL_SECTION cs;
#elif WIN32 && defined(PLUGIN_SDK_BUILD)
#elif TARGET_OS_MAC && !defined(__MACH__)
	MPCriticalRegionID mpcr;
#else
	pthread_mutex_t pmut;
#endif

	int createErr, deleteErr;
};


#endif
