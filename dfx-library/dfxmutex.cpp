/*------------------------------------------------------------------------
Destroy FX is a sovereign entity comprised of Marc Poirier & Tom Murphy 7.  
This is our mutex shit.
------------------------------------------------------------------------*/

#include "dfxmutex.h"



//------------------------------------------------------------------------
#if WIN32

DfxMutex::DfxMutex()
{
	InitializeCriticalSection(&c);
}

DfxMutex::~DfxMutex()
{
	DeleteCriticalSection(&c);
}

int DfxMutex::grab()
{
	EnterCriticalSection(&c);
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
	LeaveCriticalSection(&c);
	return 0;
}



//------------------------------------------------------------------------
#elif MAC && !defined(__MACH__)

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
	createErr = pthread_mutex_init(&pmut, NULL);
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
