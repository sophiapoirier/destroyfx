/*------------------------------------------------------------------------
Destroy FX is a sovereign entity comprised of Marc Poirier & Tom Murphy 7.  
This is our mutex shit.
------------------------------------------------------------------------*/

#ifndef __DFXMUTEX_H
#define __DFXMUTEX_H

/* mutexes broken with DfxPlugin??? need help! */
#if 0 // WIN32

struct dfxmutex {
	CRITICAL_SECTION c;
	dfxmutex() { InitializeCriticalSection(&c); }
	~dfxmutex() { DeleteCriticalSection(&c); }
	void grab() { EnterCriticalSection(&c); }
	void release() { LeaveCriticalSection(&c); }
};

/*
#elif MAC

// Multiprocessing Services
#include <Multiprocessing.h>
struct dfxmutex {
	OSStatus initErr, deleteErr, enterErr, exitErr;
	MPCriticalRegionID c;
	Duration timeout;	// in ms   (available constants:  kDurationImmediate, kDurationForever, kDurationMillisecond, kDurationMicrosecond)
	dfxmutex() { initErr = MPCreateCriticalRegion(&c); }
	~dfxmutex() { deleteErr = MPDeleteCriticalRegion(c); }
	void grab () { enterErr = MPEnterCriticalRegion(c, kDurationForever); }
	void release () { exitErr = MPExitCriticalRegion(c); }
};

// POSIX
// can this even work for CFM?  perhaps only mach-o
#include <pthread.h>
//#include <Carbon/Carbon.h>
struct dfxmutex {
	int initErr, deleteErr, enterErr, exitErr;
	pthread_mutex_t c;
	dfxmutex() { initErr = pthread_mutex_init(&c, NULL); }
	~dfxmutex() { deleteErr = pthread_mutex_destroy(&c); }
	void grab () { enterErr = pthread_mutex_lock(&c); }
	void release () { exitErr = pthread_mutex_unlock(&c);
					//pthread_testcancel();
	}
};
*/

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



#endif
