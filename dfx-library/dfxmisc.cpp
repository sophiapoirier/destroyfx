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

#include "dfxmisc.h"

#include <stdio.h>

#if TARGET_OS_MAC
	#include <Carbon/Carbon.h>
#endif

#if WIN32
	#include <windows.h>
	// for ShellExecute
	#include <shellapi.h>
	#include <shlobj.h>
#endif



//-----------------------------------------------------------------------------
// handy helper function for creating an array of float values
// returns true if allocation was successful, false if allocation failed
bool createbuffer_f(float ** buffer, long currentBufferSize, long desiredBufferSize)
{
	// if the size of the buffer has changed, 
	// then delete & reallocate the buffes according to the new size
	if (desiredBufferSize != currentBufferSize)
		releasebuffer_f(buffer);

	if (*buffer == NULL)
		*buffer = (float*) malloc(desiredBufferSize * sizeof(float));

	// check if allocation was successful
	if (*buffer == NULL)
		return false;
	// we were successful if we reached this point
	return true;
}

//-----------------------------------------------------------------------------
// handy helper function for creating an array of double float values
// returns true if allocation was successful, false if allocation failed
bool createbuffer_d(double ** buffer, long currentBufferSize, long desiredBufferSize)
{
	// if the size of the buffer has changed, 
	// then delete & reallocate the buffes according to the new size
	if (desiredBufferSize != currentBufferSize)
		releasebuffer_d(buffer);

	if (*buffer == NULL)
		*buffer = (double*) malloc(desiredBufferSize * sizeof(double));

	// check if allocation was successful
	if (*buffer == NULL)
		return false;
	// we were successful if we reached this point
	return true;
}

//-----------------------------------------------------------------------------
// handy helper function for creating an array of long int values
// returns true if allocation was successful, false if allocation failed
bool createbuffer_i(long ** buffer, long currentBufferSize, long desiredBufferSize)
{
	// if the size of the buffer has changed, 
	// then delete & reallocate the buffes according to the new size
	if (desiredBufferSize != currentBufferSize)
		releasebuffer_i(buffer);

	if (*buffer == NULL)
		*buffer = (long*) malloc(desiredBufferSize * sizeof(long));

	// check if allocation was successful
	if (*buffer == NULL)
		return false;
	// we were successful if we reached this point
	return true;
}

//-----------------------------------------------------------------------------
// handy helper function for creating an array of boolean values
// returns true if allocation was successful, false if allocation failed
bool createbuffer_b(bool ** buffer, long currentBufferSize, long desiredBufferSize)
{
	// if the size of the buffer has changed, 
	// then delete & reallocate the buffes according to the new size
	if (desiredBufferSize != currentBufferSize)
		releasebuffer_b(buffer);

	if (*buffer == NULL)
		*buffer = (bool*) malloc(desiredBufferSize * sizeof(bool));

	// check if allocation was successful
	if (*buffer == NULL)
		return false;
	// we were successful if we reached this point
	return true;
}

//-----------------------------------------------------------------------------
// handy helper function for creating an array of arrays of float values
// returns true if allocation was successful, false if allocation failed
bool createbufferarray_f(float *** buffers, unsigned long currentNumBuffers, long currentBufferSize, 
						unsigned long desiredNumBuffers, long desiredBufferSize)
{
	// if the size of each buffer or the number of buffers have changed, 
	// then delete & reallocate the buffers according to the new sizes
	if ( (desiredBufferSize != currentBufferSize) || (desiredNumBuffers != currentNumBuffers) )
		releasebufferarray_f(buffers, currentNumBuffers);

	if (desiredNumBuffers <= 0)
		return true;	// XXX true?

	if (*buffers == NULL)
	{
		*buffers = (float**) malloc(desiredNumBuffers * sizeof(float*));
		// out of memory or something
		if (*buffers == NULL)
			return false;
		for (unsigned long i=0; i < desiredNumBuffers; i++)
			(*buffers)[i] = NULL;
	}
	for (unsigned long i=0; i < desiredNumBuffers; i++)
	{
		if ((*buffers)[i] == NULL)
			(*buffers)[i] = (float*) malloc(desiredBufferSize * sizeof(float));
		// check if the allocation was successful
		if ((*buffers)[i] == NULL)
			return false;
	}

	// we were successful if we reached this point
	return true;
}

//-----------------------------------------------------------------------------
// handy helper function for creating an array of arrays of double float values
// returns true if allocation was successful, false if allocation failed
bool createbufferarrayarray_d(double **** buffers, unsigned long currentNumBufferArrays, unsigned long currentNumBuffers, 
							long currentBufferSize, unsigned long desiredNumBufferArrays, 
							unsigned long desiredNumBuffers, long desiredBufferSize)
{
	// if the size of each buffer or the number of buffers have changed, 
	// then delete & reallocate the buffers according to the new sizes
	if ( (desiredBufferSize != currentBufferSize) 
			|| (desiredNumBuffers != currentNumBuffers) 
			|| (desiredNumBufferArrays != currentNumBufferArrays) )
		releasebufferarrayarray_d(buffers, currentNumBufferArrays, currentNumBuffers);

	if (desiredNumBufferArrays <= 0)
		return true;	// XXX true?

	unsigned long i, j;
	if (*buffers == NULL)
	{
		*buffers = (double***) malloc(desiredNumBufferArrays * sizeof(double**));
		// out of memory or something
		if (*buffers == NULL)
			return false;
		for (i=0; i < desiredNumBufferArrays; i++)
			(*buffers)[i] = NULL;
	}
	for (i=0; i < desiredNumBufferArrays; i++)
	{
		if ((*buffers)[i] == NULL)
			(*buffers)[i] = (double**) malloc(desiredNumBuffers * sizeof(double*));
		// check if the allocation was successful
		if ((*buffers)[i] == NULL)
			return false;
		for (j=0; j < desiredNumBuffers; j++)
			(*buffers)[i][j] = NULL;
	}
	for (i=0; i < desiredNumBufferArrays; i++)
	{
		for (j=0; j < desiredNumBuffers; j++)
		{
			if ((*buffers)[i][j] == NULL)
				(*buffers)[i][j] = (double*) malloc(desiredBufferSize * sizeof(double));
			// check if the allocation was successful
			if ((*buffers)[i][j] == NULL)
				return false;
		}
	}

	// we were successful if we reached this point
	return true;
}


//-------------------------------------------------------------------------
// handy helper function for safely deallocating an array of float values
void releasebuffer_f(float ** buffer)
{
	if (*buffer != NULL)
	{
		free(*buffer);
	}
	*buffer = NULL;
}

//-------------------------------------------------------------------------
// handy helper function for safely deallocating an array of double float values
void releasebuffer_d(double ** buffer)
{
	if (*buffer != NULL)
	{
		free(*buffer);
	}
	*buffer = NULL;
}

//-------------------------------------------------------------------------
// handy helper function for safely deallocating an array of long int values
void releasebuffer_i(long ** buffer)
{
	if (*buffer != NULL)
	{
		free(*buffer);
	}
	*buffer = NULL;
}

//-------------------------------------------------------------------------
// handy helper function for safely deallocating an array of boolean values
void releasebuffer_b(bool ** buffer)
{
	if (*buffer != NULL)
	{
		free(*buffer);
	}
	*buffer = NULL;
}

//-------------------------------------------------------------------------
// handy helper function for safely deallocating an array of arrays of float values
void releasebufferarray_f(float *** buffers, unsigned long numbuffers)
{
	if (*buffers != NULL)
	{
		for (unsigned long i=0; i < numbuffers; i++)
		{
			if ((*buffers)[i] != NULL)
				free((*buffers)[i]);
			(*buffers)[i] = NULL;
		}
		free(*buffers);
	}
	*buffers = NULL;
}

//-------------------------------------------------------------------------
// handy helper function for safely deallocating an array of arrays of double float values
void releasebufferarrayarray_d(double **** buffers, unsigned long numbufferarrays, unsigned long numbuffers)
{
	if (*buffers != NULL)
	{
		for (unsigned long i=0; i < numbufferarrays; i++)
		{
			if ((*buffers)[i] != NULL)
			{
				for (unsigned long j=0; j < numbuffers; j++)
				{
					if ((*buffers)[i][j] != NULL)
						free((*buffers)[i][j]);
					(*buffers)[i][j] = NULL;
				}
				free((*buffers)[i]);
			}
			(*buffers)[i] = NULL;
		}
		free(*buffers);
	}
	*buffers = NULL;
}


//-----------------------------------------------------------------------------
// handy helper function for safely zeroing the contents of an array of float values
void clearbuffer_f(float * buffer, long buffersize, float value)
{
	if (buffer != NULL)
	{
		for (long i=0; i < buffersize; i++)
			buffer[i] = value;
	}
}

//-----------------------------------------------------------------------------
// handy helper function for safely zeroing the contents of an array of double float values
void clearbuffer_d(double * buffer, long buffersize, double value)
{
	if (buffer != NULL)
	{
		for (long i=0; i < buffersize; i++)
			buffer[i] = value;
	}
}

//-----------------------------------------------------------------------------
// handy helper function for safely zeroing the contents of an array of long int values
void clearbuffer_i(long * buffer, long buffersize, long value)
{
	if (buffer != NULL)
	{
		for (long i=0; i < buffersize; i++)
			buffer[i] = value;
	}
}

//-----------------------------------------------------------------------------
// handy helper function for safely zeroing the contents of an array of boolean values
void clearbuffer_b(bool * buffer, long buffersize, bool value)
{
	if (buffer != NULL)
	{
		for (long i=0; i < buffersize; i++)
			buffer[i] = value;
	}
}

//-----------------------------------------------------------------------------
// handy helper function for safely zeroing the contents of an array of arrays of float values
void clearbufferarray_f(float ** buffers, unsigned long numbuffers, long buffersize, float value)
{
	if (buffers != NULL)
	{
		for (unsigned long i=0; i < numbuffers; i++)
		{
			if (buffers[i] != NULL)
			{
				for (long j=0; j < buffersize; j++)
					buffers[i][j] = value;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// handy helper function for safely zeroing the contents of an array of arrays of float values
void clearbufferarrayarray_d(double *** buffers, unsigned long numbufferarrays, unsigned long numbuffers, 
							long buffersize, double value)
{
	if (buffers != NULL)
	{
		for (unsigned long i=0; i < numbufferarrays; i++)
		{
			if (buffers[i] != NULL)
			{
				for (unsigned long j=0; j < numbuffers; j++)
				{
					if (buffers[i][j] != NULL)
					{
						for (long k=0; k < buffersize; k++)
							buffers[i][j][k] = value;
					}
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
// handy function to open up an URL in the user's default web browser
//  * Mac OS
// returns noErr (0) if successful, otherwise a non-zero error code is returned
//  * Windows
// returns a meaningless value greater than 32 if successful, 
// otherwise an error code ranging from 0 to 32 is returned
long launch_url(const char * inUrlString)
{
	if (inUrlString == NULL)
		return 3;

#if TARGET_OS_MAC
	CFURLRef urlcfurl = CFURLCreateWithBytes(kCFAllocatorDefault, (const UInt8*)inUrlString, (CFIndex)strlen(inUrlString), kCFStringEncodingASCII, NULL);
	if (urlcfurl != NULL)
	{
		OSStatus status = LSOpenCFURLRef(urlcfurl, NULL);	// try to launch the URL
		CFRelease(urlcfurl);
		return status;
	}
	return paramErr;	// couldn't create the CFURL, so return some error code
#endif

#if WIN32
	return (long) ShellExecute(NULL, "open", inUrlString, NULL, NULL, SW_SHOWNORMAL);
#endif
}

#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
// this function looks for the plugin's documentation file in the appropriate system location, 
// within a given file system domain, and returns a CFURLRef for the file if it is found, 
// and NULL otherwise (or if some error is encountered along the way)
CFURLRef DFX_FindDocumentationFileInDomain(CFStringRef inDocsFileName, short inDomain)
{
	if (inDocsFileName == NULL)
		return NULL;

	// first find the base directory for the system documentation directory
	FSRef docsDirRef;
	OSErr error = FSFindFolder(inDomain, kDocumentationFolderType, kDontCreateFolder, &docsDirRef);
	if (error == noErr)
	{
		// convert the FSRef of the documentation directory to a CFURLRef (for use in the next steps)
		CFURLRef docsDirURL = CFURLCreateFromFSRef(kCFAllocatorDefault, &docsDirRef);
		if (docsDirURL != NULL)
		{
			// create a CFURL for the "manufacturer name" directory within the documentation directory
			CFURLRef dfxDocsDirURL = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, docsDirURL, CFSTR(PLUGIN_CREATOR_NAME_STRING), true);
			CFRelease(docsDirURL);
			if (dfxDocsDirURL != NULL)
			{
				// create a CFURL for the documentation file within the "manufacturer name" directory
				CFURLRef docsFileURL = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, dfxDocsDirURL, inDocsFileName, false);
				CFRelease(dfxDocsDirURL);
				if (docsFileURL != NULL)
				{
					// check to see if the hypothetical documentation file actually exists 
					// (CFURLs can reference files that don't exist)
					SInt32 urlErrorCode = 0;
					CFBooleanRef docsFileExists = (CFBooleanRef) CFURLCreatePropertyFromResource(kCFAllocatorDefault, docsFileURL, kCFURLFileExists, &urlErrorCode);
					if (docsFileExists != NULL)
					{
						// only return the file's CFURL if the file exists
						if (docsFileExists == kCFBooleanTrue)
							return docsFileURL;
						CFRelease(docsFileExists);
					}
					CFRelease(docsFileURL);
				}
			}
		}
	}

	return NULL;
}
#endif

//-----------------------------------------------------------------------------
// XXX this function should really go somewhere else, like in that promised DFX utilities file or something like that
long launch_documentation()
{

#if TARGET_OS_MAC
	// no assumptions can be made about how long the reference is valid, 
	// and the caller should not attempt to release the CFBundleRef object
	CFBundleRef pluginBundleRef = CFBundleGetBundleWithIdentifier( CFSTR(PLUGIN_BUNDLE_IDENTIFIER) );
	if (pluginBundleRef != NULL)
	{
		CFStringRef docsFileName = CFSTR( PLUGIN_NAME_STRING" manual.html" );
	#ifdef PLUGIN_DOCUMENTATION_FILE_NAME
		docsFileName = CFSTR(PLUGIN_DOCUMENTATION_FILE_NAME);
	#endif
		CFStringRef docsSubdirName = NULL;
	#ifdef PLUGIN_DOCUMENTATION_SUBDIRECTORY_NAME
		docsSubdirName = CFSTR(PLUGIN_DOCUMENTATION_SUBDIRECTORY_NAME);
	#endif
		CFURLRef docsFileURL = CFBundleCopyResourceURL(pluginBundleRef, docsFileName, NULL, docsSubdirName);
		// if the documentation file is not found in the bundle, then search in appropriate system locations
		if (docsFileURL == NULL)
			docsFileURL = DFX_FindDocumentationFileInDomain(docsFileName, kUserDomain);
		if (docsFileURL == NULL)
			docsFileURL = DFX_FindDocumentationFileInDomain(docsFileName, kLocalDomain);
		if (docsFileURL == NULL)
			docsFileURL = DFX_FindDocumentationFileInDomain(docsFileName, kNetworkDomain);
		if (docsFileURL != NULL)
		{
// open the manual with the default application for the file type
#if 0
			OSStatus status = LSOpenCFURLRef(docsFileURL, NULL);
// open the manual with Apple's system Help Viewer
#else
		#if 1
			// starting in Mac OS X 10.5.7, we get an error if the help book is not registered
			// XXX please note that this also requires adding a CFBundleHelpBookFolder key/value to your Info.plist
			static bool helpBookRegistered = false;
			if (!helpBookRegistered)
			{
				CFURLRef bundleURL = CFBundleCopyBundleURL(pluginBundleRef);
				if (bundleURL != NULL)
				{
					FSRef bundleRef = {0};
					Boolean fsrefSuccess = CFURLGetFSRef(bundleURL, &bundleRef);
					if (fsrefSuccess)
					{
						OSStatus registerStatus = AHRegisterHelpBook(&bundleRef);
						if (registerStatus == noErr)
							helpBookRegistered = true;
					}
					CFRelease(bundleURL);
				}
			}
		#endif
			OSStatus status = coreFoundationUnknownErr;
			CFStringRef docsFileUrlString = CFURLGetString(docsFileURL);
			if (docsFileUrlString != NULL)
			{
				status = AHGotoPage(NULL, docsFileUrlString, NULL);
			}
#endif
			CFRelease(docsFileURL);
			return status;
		}
	}

	return fnfErr;	// file not found error
#endif

	return 0;
}

//-----------------------------------------------------------------------------
const char * DFX_GetNameForMIDINote(long inMidiNote)
{
	static char midiNoteName[16] = {0};
	const long kNumNotesInOctave = 12;
	const char * keyNames[kNumNotesInOctave] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
	const long keyNameIndex = inMidiNote % kNumNotesInOctave;
	const long octaveNumber = (inMidiNote / kNumNotesInOctave) - 1;
	sprintf(midiNoteName, "%s %ld", keyNames[keyNameIndex], octaveNumber);
	return midiNoteName;
}

//-----------------------------------------------------------------------------
uint64_t DFX_GetMillisecondCount()
{
#if TARGET_OS_MAC
	// convert from 1/60 second to millisecond values
	return (uint64_t)TickCount() * 100 / 6;
#endif

#if WIN32
	#if _WIN32_WINNT >= 0x0600
		return GetTickCount64();
	#else
		return (UINT64) GetTickCount();
	#endif
#endif
}

#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
char * DFX_CreateCStringFromCFString(CFStringRef inCFString, CFStringEncoding inCStringEncoding)
{
	CFIndex stringBufferSize = CFStringGetMaximumSizeForEncoding(CFStringGetLength(inCFString) + 1, inCStringEncoding);
	if (stringBufferSize <= 0)
		return NULL;
	char * outputString = (char*) malloc(stringBufferSize);
	if (outputString == NULL)
		return NULL;
	memset(outputString, 0, stringBufferSize);
	Boolean stringSuccess = CFStringGetCString(inCFString, outputString, stringBufferSize, inCStringEncoding);
	if (!stringSuccess)
	{
		free(outputString);
		outputString = NULL;
	}

	return outputString;
}
#endif
