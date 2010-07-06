/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2010  Sophia Poirier

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



bool createbuffer_f(float ** buffer, long currentBufferSize, long desiredBufferSize);
bool createbuffer_d(double ** buffer, long currentBufferSize, long desiredBufferSize);
bool createbuffer_i(long ** buffer, long currentBufferSize, long desiredBufferSize);
bool createbuffer_b(bool ** buffer, long currentBufferSize, long desiredBufferSize);
bool createbufferarray_f(float *** buffers, unsigned long currentNumBuffers, long currentBufferSize, 
						unsigned long desiredNumBuffers, long desiredBufferSize);
bool createbufferarrayarray_d(double **** buffers, unsigned long currentNumBufferArrays, unsigned long currentNumBuffers, 
							long currentBufferSize, unsigned long desiredNumBufferArrays, 
							unsigned long desiredNumBuffers, long desiredBufferSize);

void releasebuffer_f(float ** buffer);
void releasebuffer_d(double ** buffer);
void releasebuffer_i(long ** buffer);
void releasebuffer_b(bool ** buffer);
void releasebufferarray_f(float *** buffers, unsigned long numbuffers);
void releasebufferarrayarray_d(double **** buffers, unsigned long numbufferarrays, unsigned long numbuffers);

void clearbuffer_f(float * buffer, long buffersize, float value = 0.0f);
void clearbuffer_d(double * buffer, long buffersize, double value = 0.0);
void clearbuffer_i(long * buffer, long buffersize, long value = 0);
void clearbuffer_b(bool * buffer, long buffersize, bool value = false);
void clearbufferarray_f(float ** buffers, unsigned long numbuffers, long buffersize, float value = 0.0f);
void clearbufferarrayarray_d(double *** buffers, unsigned long numbufferarrays, unsigned long numbuffers, 
							long buffersize, double value = 0.0);

long launch_url(const char * inUrlString);
long launch_documentation();
const char * DFX_GetNameForMIDINote(long inMidiNote);
uint64_t DFX_GetMillisecondCount();

#if TARGET_OS_MAC
char * DFX_CreateCStringFromCFString(CFStringRef inCFString, CFStringEncoding inCStringEncoding = kCFStringEncodingUTF8);
#endif



#endif
