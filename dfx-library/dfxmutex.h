/*------------------------------------------------------------------------
Destroy FX is a sovereign entity comprised of Marc Poirier & Tom Murphy 7.  
This is our mutex shit.
------------------------------------------------------------------------*/

#ifndef __DFXMUTEX_H
#define __DFXMUTEX_H



#if WIN32

struct dfxmutex {
	CRITICAL_SECTION c;
	dfxmutex() { InitializeCriticalSection(&c); }
	~dfxmutex() { DeleteCriticalSection(&c); }
	void grab() { EnterCriticalSection(&c); }
	// map the TryEnterCriticalSection to the sort of error that every other mutex API uses
	int try_grab() { return (TryEnterCriticalSection(&cs) == 0) ? 3 : 0; }
	void release() { LeaveCriticalSection(&c); }
};



#elif MAC

#ifdef __MACH__

// POSIX
#include <pthread.h>
struct dfxmutex {
	int createErr, deleteErr;
	pthread_mutex_t pmut;
	dfxmutex() { createErr = pthread_mutex_init(&pmut, NULL); }
	~dfxmutex() { deleteErr = pthread_mutex_destroy(&pmut); }
	int grab () { return pthread_mutex_lock(&pmut); }
	int try_grab () { return pthread_mutex_trylock(&pmut); }
	int release () { return pthread_mutex_unlock(&pmut); }
};

#else

// Multiprocessing Services
#include <Multiprocessing.h>
#include <MacErrors.h>
struct dfxmutex {
	OSStatus createErr, deleteErr;
	MPCriticalRegionID mpcr;
	Duration timeout;	// in ms
	dfxmutex()
	{
		if ( MPLibraryIsLoaded() )
			createErr = MPCreateCriticalRegion(&mpcr);
		else
			createErr = kMPInsufficientResourcesErr;
	}
	~dfxmutex()
	{
		if (createErr == noErr)
			deleteErr = MPDeleteCriticalRegion(mpcr);
	}
	OSStatus grab()
	{
		if (createErr == noErr) 
			return MPEnterCriticalRegion(mpcr, kDurationForever);
		else
			return createErr;
	}
	OSStatus try_grab()
	{
		if (createErr == noErr) 
			return MPEnterCriticalRegion(mpcr, kDurationImmediate);
		else
			return createErr;
	}
	OSStatus release()
	{
		if (createErr == noErr) 
			return MPExitCriticalRegion(mpcr);
		else
			return createErr;
	}
};

#endif
// __MACH__



#else

/* other platforms don't have mutexes implemented, will just have
   to get "lucky"... */

struct dfxmutex {
	dfxmutex() {}
	~dfxmutex() {}
	void grab () {}
	void release () {}
};



#endif
// platforms



#endif
