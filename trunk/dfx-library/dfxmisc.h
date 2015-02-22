/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2015  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Destroy FX Library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Destroy FX Library.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/

Destroy FX is a sovereign entity comprised of Sophia Poirier and Tom Murphy 7.
These are some generally useful functions.
------------------------------------------------------------------------*/

#ifndef __DFX_MISC_H
#define __DFX_MISC_H


#include "dfxdefines.h"

#ifdef __MACH__
	#include <CoreFoundation/CoreFoundation.h>
#endif



//-------------------------------------------------------------------------
// handy helper function for safely deallocating an array
template <typename T>
void dfx_releasebuffer(T ** ioBuffer)
{
    if (*ioBuffer != NULL)
        free(*ioBuffer);
    *ioBuffer = NULL;
}

//-----------------------------------------------------------------------------
// handy helper function for creating an array
// returns true if allocation was successful, false if allocation failed
template <typename T>
bool dfx_createbuffer(T ** ioBuffer, long inCurrentBufferSize, long inDesiredBufferSize)
{
	// if the size of the buffer has changed, 
	// then delete and reallocate the buffers according to the new size
	if (inDesiredBufferSize != inCurrentBufferSize)
		dfx_releasebuffer<T>(ioBuffer);

	if (inDesiredBufferSize <= 0)
		return false;

	if (*ioBuffer == NULL)
		*ioBuffer = (T*) malloc(inDesiredBufferSize * sizeof(T));

	if (*ioBuffer == NULL)
		return false;
	return true;
}

//-----------------------------------------------------------------------------
// handy helper function for safely zeroing the contents of an array
template <typename T>
void dfx_clearbuffer(T * ioBuffer, long inBufferSize, T inValue = 0)
{
	if (ioBuffer != NULL)
	{
		for (long i=0; i < inBufferSize; i++)
			ioBuffer[i] = inValue;
	}
}


//-------------------------------------------------------------------------
// handy helper function for safely deallocating a 2-dimensional array
template <typename T>
void dfx_releasebufferarray(T *** ioBuffers, unsigned long inNumBuffers)
{
    if (*ioBuffers != NULL)
    {
        for (unsigned long i=0; i < inNumBuffers; i++)
        {
            if ((*ioBuffers)[i] != NULL)
                free((*ioBuffers)[i]);
            (*ioBuffers)[i] = NULL;
        }
        free(*ioBuffers);
    }
    *ioBuffers = NULL;
}

//-----------------------------------------------------------------------------
// handy helper function for creating a 2-dimensional array
// returns true if allocation was successful, false if allocation failed
template <typename T>
bool dfx_createbufferarray(T *** ioBuffers, unsigned long inCurrentNumBuffers, long inCurrentBufferSize, 
							unsigned long inDesiredNumBuffers, long inDesiredBufferSize)
{
	// if the size of each buffer or the number of buffers have changed, 
	// then delete and reallocate the buffers according to the new sizes
	if ( (inDesiredBufferSize != inCurrentBufferSize) || (inDesiredNumBuffers != inCurrentNumBuffers) )
		dfx_releasebufferarray<T>(ioBuffers, inCurrentNumBuffers);

	if ((inDesiredNumBuffers <= 0) || (inDesiredBufferSize <=0))
		return false;

	if (*ioBuffers == NULL)
	{
		*ioBuffers = (T**) malloc(inDesiredNumBuffers * sizeof(T*));
		if (*ioBuffers == NULL)
			return false;
		for (unsigned long i=0; i < inDesiredNumBuffers; i++)
			(*ioBuffers)[i] = NULL;
	}
	for (unsigned long i=0; i < inDesiredNumBuffers; i++)
	{
		if ((*ioBuffers)[i] == NULL)
			(*ioBuffers)[i] = (T*) malloc(inDesiredBufferSize * sizeof(T));
		if ((*ioBuffers)[i] == NULL)
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// handy helper function for safely zeroing the contents of a 2-dimensional array's values
template <typename T>
void dfx_clearbufferarray(T ** ioBuffers, unsigned long inNumBuffers, long inBufferSize, T inValue = 0)
{
	if (ioBuffers != NULL)
	{
		for (unsigned long i=0; i < inNumBuffers; i++)
		{
			if (ioBuffers[i] != NULL)
			{
				for (long j=0; j < inBufferSize; j++)
					ioBuffers[i][j] = inValue;
			}
		}
	}
}


//-------------------------------------------------------------------------
// handy helper function for safely deallocating a 3-dimensional array
template <typename T>
void dfx_releasebufferarrayarray(T **** ioBuffers, unsigned long inNumBufferArrays, unsigned long inNumBuffers)
{
    if (*ioBuffers != NULL)
    {
        for (unsigned long i=0; i < inNumBufferArrays; i++)
        {
            if ((*ioBuffers)[i] != NULL)
            {
                for (unsigned long j=0; j < inNumBuffers; j++)
                {
                    if ((*ioBuffers)[i][j] != NULL)
                        free((*ioBuffers)[i][j]);
                    (*ioBuffers)[i][j] = NULL;
                }
                free((*ioBuffers)[i]);
            }
            (*ioBuffers)[i] = NULL;
        }
        free(*ioBuffers);
    }
    *ioBuffers = NULL;
}

//-----------------------------------------------------------------------------
// handy helper function for creating a 3-dimensional array
// returns true if allocation was successful, false if allocation failed
template <typename T>
bool dfx_createbufferarrayarray(T **** ioBuffers, unsigned long inCurrentNumBufferArrays, unsigned long inCurrentNumBuffers, 
							long inCurrentBufferSize, unsigned long inDesiredNumBufferArrays, 
							unsigned long inDesiredNumBuffers, long inDesiredBufferSize)
{
	// if the size of each buffer or the number of buffers have changed, 
	// then delete and reallocate the buffers according to the new sizes
	if ( (inDesiredBufferSize != inCurrentBufferSize) 
			|| (inDesiredNumBuffers != inCurrentNumBuffers) 
			|| (inDesiredNumBufferArrays != inCurrentNumBufferArrays) )
		dfx_releasebufferarrayarray<T>(ioBuffers, inCurrentNumBufferArrays, inCurrentNumBuffers);

	if ((inDesiredNumBufferArrays <= 0) || (inDesiredNumBuffers <= 0) || (inDesiredBufferSize <= 0))
		return false;

	unsigned long i, j;
	if (*ioBuffers == NULL)
	{
		*ioBuffers = (T***) malloc(inDesiredNumBufferArrays * sizeof(T**));
		if (*ioBuffers == NULL)
			return false;
		for (i=0; i < inDesiredNumBufferArrays; i++)
			(*ioBuffers)[i] = NULL;
	}
	for (i=0; i < inDesiredNumBufferArrays; i++)
	{
		if ((*ioBuffers)[i] == NULL)
			(*ioBuffers)[i] = (T**) malloc(inDesiredNumBuffers * sizeof(T*));
		if ((*ioBuffers)[i] == NULL)
			return false;
		for (j=0; j < inDesiredNumBuffers; j++)
			(*ioBuffers)[i][j] = NULL;
	}
	for (i=0; i < inDesiredNumBufferArrays; i++)
	{
		for (j=0; j < inDesiredNumBuffers; j++)
		{
			if ((*ioBuffers)[i][j] == NULL)
				(*ioBuffers)[i][j] = (T*) malloc(inDesiredBufferSize * sizeof(T));
			if ((*ioBuffers)[i][j] == NULL)
				return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// handy helper function for safely zeroing the values in a 3-dimensional array
template <typename T>
void dfx_clearbufferarrayarray(T *** ioBuffers, unsigned long inNumBufferArrays, unsigned long inNumBuffers, 
							long inBufferSize, T inValue = 0)
{
	if (ioBuffers != NULL)
	{
		for (unsigned long i=0; i < inNumBufferArrays; i++)
		{
			if (ioBuffers[i] != NULL)
			{
				for (unsigned long j=0; j < inNumBuffers; j++)
				{
					if (ioBuffers[i][j] != NULL)
					{
						for (long k=0; k < inBufferSize; k++)
							ioBuffers[i][j][k] = inValue;
					}
				}
			}
		}
	}
}



//-----------------------------------------------------------------------------
long launch_url(const char * inUrlString);
long launch_documentation();
const char * DFX_GetNameForMIDINote(long inMidiNote);
uint64_t DFX_GetMillisecondCount();

#if TARGET_OS_MAC
char * DFX_CreateCStringFromCFString(CFStringRef inCFString, CFStringEncoding inCStringEncoding = kCFStringEncodingUTF8);
#endif



#endif
