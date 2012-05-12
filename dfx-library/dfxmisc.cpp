/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2012  Sophia Poirier

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

#if _WIN32
	#include <windows.h>
	// for ShellExecute
	#include <shellapi.h>
	#include <shlobj.h>
#endif



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

#if _WIN32
	return (long) ShellExecute(NULL, "open", inUrlString, NULL, NULL, SW_SHOWNORMAL);
#endif
}

#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
// this function looks for the plugin's documentation file in the appropriate system location, 
// within a given file system domain, and returns a CFURLRef for the file if it is found, 
// and NULL otherwise (or if some error is encountered along the way)
CFURLRef DFX_FindDocumentationFileInDomain(CFStringRef inDocsFileName, FSVolumeRefNum inDomain)
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

#if _WIN32
	#if (_WIN32_WINNT >= 0x0600) && 0	// XXX how to runtime check without symbol error if unavailable?
	OSVERSIONINFO versionInfo = {0};
	versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
	if ( GetVersionEx(&versionInfo) && (versionInfo.dwMajorVersion >= 6) )
		return GetTickCount64();
	else
	#endif
	return (UINT64) GetTickCount();
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
