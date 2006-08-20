/*------------------------------------------------------------------------
Destroy FX is a sovereign entity comprised of Sophia Poirier and Tom Murphy 7.  
This is our mutex shit.
------------------------------------------------------------------------*/

#include "dfxmutex.h"



//------------------------------------------------------------------------
#if WIN32

DfxMutex::DfxMutex()
{
	InitializeCriticalSection(&cs);
}

DfxMutex::~DfxMutex()
{
	DeleteCriticalSection(&cs);
}

int DfxMutex::grab()
{
	EnterCriticalSection(&cs);
	return 0;
}

int DfxMutex::try_grab()
{
	BOOL success = TryEnterCriticalSection(&cs);
	// map the TryEnterCriticalSection result to the sort of error that every other mutex API uses
	return (success == 0) ? -1 : 0;
}

int DfxMutex::release()
{
	LeaveCriticalSection(&cs);
	return 0;
}



//------------------------------------------------------------------------
#elif TARGET_OS_MAC && !defined(__MACH__)

#include <MacErrors.h>

DfxMutex::DfxMutex()
{
	if ( MPLibraryIsLoaded() )
		createErr = MPCreateCriticalRegion(&mpcr);
	else
		createErr = kMPInsufficientResourcesErr;
}

DfxMutex::~DfxMutex()
{
	if (createErr == noErr)
		deleteErr = MPDeleteCriticalRegion(mpcr);
}

int DfxMutex::grab()
{
	if (createErr == noErr) 
		return MPEnterCriticalRegion(mpcr, kDurationForever);
	else
		return createErr;
}

int DfxMutex::try_grab()
{
	if (createErr == noErr) 
		return MPEnterCriticalRegion(mpcr, kDurationImmediate);
	else
		return createErr;
}

int DfxMutex::release()
{
	if (createErr == noErr) 
		return MPExitCriticalRegion(mpcr);
	else
		return createErr;
}



//------------------------------------------------------------------------
#else

DfxMutex::DfxMutex()
{
	pthread_mutexattr_t pmutAttr, * pmutAttrPtr = NULL;
	int attrInitErr = pthread_mutexattr_init(&pmutAttr);
	if (attrInitErr == 0)
	{
		int attrSetTypeErr = pthread_mutexattr_settype(&pmutAttr, PTHREAD_MUTEX_RECURSIVE);
		if (attrSetTypeErr == 0)
			pmutAttrPtr = &pmutAttr;
	}
	createErr = pthread_mutex_init(&pmut, pmutAttrPtr);
	if (attrInitErr == 0)
		pthread_mutexattr_destroy(&pmutAttr);
}

DfxMutex::~DfxMutex()
{
	deleteErr = pthread_mutex_destroy(&pmut);
}

int DfxMutex::grab()
{
	return pthread_mutex_lock(&pmut);
}

int DfxMutex::try_grab()
{
	return pthread_mutex_trylock(&pmut);
}

int DfxMutex::release()
{
	return pthread_mutex_unlock(&pmut);
}



#endif
// platforms
