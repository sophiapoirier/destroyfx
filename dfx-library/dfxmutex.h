/*------------------------------------------------------------------------
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
