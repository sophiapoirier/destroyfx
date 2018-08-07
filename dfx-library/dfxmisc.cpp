/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2018  Sophia Poirier

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

#include <algorithm>
#include <stdio.h>

#include "dfxdefines.h"

#if TARGET_OS_MAC
	#include <Carbon/Carbon.h>
#endif

#if _WIN32
	#include <windows.h>
	// for ShellExecute
	#include <shellapi.h>
	#include <shlobj.h>
#endif


namespace dfx
{


//------------------------------------------------------
long CompositePluginVersionNumberValue()
{
	return (PLUGIN_VERSION_MAJOR << 16) | (PLUGIN_VERSION_MINOR << 8) | PLUGIN_VERSION_BUGFIX;
}

//------------------------------------------------------
// this reverses the bytes in a stream of data, for correcting endian difference
void ReverseBytes(void* ioData, size_t inItemSize, size_t inItemCount)
{
	size_t const half = (inItemSize / 2) + (inItemSize % 2);
	auto dataBytes = static_cast<char*>(ioData);

	for (size_t c = 0; c < inItemCount; c++)
	{
		for (size_t i = 0; i < half; i++)
		{
			auto const temp = dataBytes[i];
			size_t const complementIndex = (inItemSize - 1) - i;
			dataBytes[i] = dataBytes[complementIndex];
			dataBytes[complementIndex] = temp;
		}
		dataBytes += inItemSize;
	}
}

//-----------------------------------------------------------------------------
// handy function to open up an URL in the user's default web browser
//  * macOS
// returns noErr (0) if successful, otherwise a non-zero error code is returned
//  * Windows
// returns a meaningless value greater than 32 if successful, 
// otherwise an error code ranging from 0 to 32 is returned
long LaunchURL(std::string const& inURL)
{
	if (inURL.empty())
	{
		return 3;
	}

#if TARGET_OS_MAC
	UniqueCFType const cfURL = CFURLCreateWithBytes(kCFAllocatorDefault, reinterpret_cast<UInt8 const*>(inURL.data()), static_cast<CFIndex>(inURL.length()), kCFStringEncodingASCII, nullptr);
	if (cfURL)
	{
		auto const status = LSOpenCFURLRef(cfURL.get(), nullptr);  // try to launch the URL
		return status;
	}
	return paramErr;  // couldn't create the CFURL, so return some error code
#endif

#if _WIN32
	return static_cast<long>(ShellExecute(nullptr, "open", inURL.c_str(), nullptr, nullptr, SW_SHOWNORMAL));
#endif
}

#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
// this function looks for the plugin's documentation file in the appropriate system location, 
// within a given file system domain, and returns a CFURLRef for the file if it is found, 
// and null otherwise (or if some error is encountered along the way)
UniqueCFType<CFURLRef> DFX_FindDocumentationFileInDomain(CFStringRef inDocsFileName, FSVolumeRefNum inDomain)
{
	if (!inDocsFileName)
	{
		return {};
	}

	// first find the base directory for the system documentation directory
	FSRef docsDirRef;
	auto const error = FSFindFolder(inDomain, kDocumentationFolderType, kDontCreateFolder, &docsDirRef);
	if (error == noErr)
	{
		// convert the FSRef of the documentation directory to a CFURLRef (for use in the next steps)
		UniqueCFType const docsDirURL = CFURLCreateFromFSRef(kCFAllocatorDefault, &docsDirRef);
		if (docsDirURL)
		{
			// create a CFURL for the "manufacturer name" directory within the documentation directory
			UniqueCFType const dfxDocsDirURL = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, docsDirURL.get(), CFSTR(PLUGIN_CREATOR_NAME_STRING), true);
			if (dfxDocsDirURL)
			{
				// create a CFURL for the documentation file within the "manufacturer name" directory
				UniqueCFType docsFileURL = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, dfxDocsDirURL.get(), inDocsFileName, false);
				if (docsFileURL)
				{
					// check to see if the hypothetical documentation file actually exists 
					// (CFURLs can reference files that don't exist)
					SInt32 urlErrorCode = 0;
					UniqueCFType const docsFileCFExists = static_cast<CFBooleanRef>(CFURLCreatePropertyFromResource(kCFAllocatorDefault, docsFileURL.get(), kCFURLFileExists, &urlErrorCode));
					if (docsFileCFExists)
					{
						// only return the file's CFURL if the file exists
						bool const docsFileExists = (docsFileCFExists.get() == kCFBooleanTrue);
						if (docsFileExists)
						{
							return docsFileURL;
						}
					}
				}
			}
		}
	}

	return {};
}
#endif

//-----------------------------------------------------------------------------
// XXX this function should really go somewhere else, like in that promised DFX utilities file or something like that
long LaunchDocumentation()
{

#if TARGET_OS_MAC
	// no assumptions can be made about how long the reference is valid, 
	// and the caller should not attempt to release the CFBundleRef object
	auto const pluginBundleRef = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));
	if (pluginBundleRef)
	{
		CFStringRef const docsFileName = CFSTR(PLUGIN_NAME_STRING " manual.html");
	#ifdef PLUGIN_DOCUMENTATION_FILE_NAME
		docsFileName = CFSTR(PLUGIN_DOCUMENTATION_FILE_NAME);
	#endif
		CFStringRef docsSubdirName = nullptr;
	#ifdef PLUGIN_DOCUMENTATION_SUBDIRECTORY_NAME
		docsSubdirName = CFSTR(PLUGIN_DOCUMENTATION_SUBDIRECTORY_NAME);
	#endif
		UniqueCFType docsFileURL = CFBundleCopyResourceURL(pluginBundleRef, docsFileName, nullptr, docsSubdirName);
		// if the documentation file is not found in the bundle, then search in appropriate system locations
		if (!docsFileURL)
		{
			docsFileURL = DFX_FindDocumentationFileInDomain(docsFileName, kUserDomain);
		}
		if (!docsFileURL)
		{
			docsFileURL = DFX_FindDocumentationFileInDomain(docsFileName, kLocalDomain);
		}
		if (!docsFileURL)
		{
			docsFileURL = DFX_FindDocumentationFileInDomain(docsFileName, kNetworkDomain);
		}
		if (docsFileURL)
		{
// open the manual with the default application for the file type
#if 1
			auto const status = LSOpenCFURLRef(docsFileURL.get(), nullptr);
// open the manual with Apple's system Help Viewer
#else
			// XXX I don't know why the Help Viewer code is not working anymore (Help Viewer can't load the page, since 10.6)
		#if 1
			// starting in Mac OS X 10.5.7, we get an error if the help book is not registered
			// XXX please note that this also requires adding a CFBundleHelpBookFolder key/value to your Info.plist
			static bool helpBookRegistered = false;
			if (!helpBookRegistered)
			{
				UniqueCFType const bundleURL = CFBundleCopyBundleURL(pluginBundleRef);
				if (bundleURL)
				{
					OSStatus registerStatus = paramErr;
					if (AHRegisterHelpBookWithURL)  // available starting in Mac OS X 10.6
					{
						registerStatus = AHRegisterHelpBookWithURL(bundleURL.get());
					}
					else
					{
						FSRef bundleRef;
						if (CFURLGetFSRef(bundleURL.get(), &bundleRef))
						{
							registerStatus = AHRegisterHelpBook(&bundleRef);
						}
					}
					if (registerStatus == noErr)
					{
						helpBookRegistered = true;
					}
				}
			}
		#endif
			OSStatus status = coreFoundationUnknownErr;
			if (auto const docsFileUrlString = CFURLGetString(docsFileURL.get()))
			{
				status = AHGotoPage(nullptr, docsFileUrlString, nullptr);
			}
#endif
			return status;
		}
	}

	return fnfErr;  // file not found error
#endif

	return 0;
}

//-----------------------------------------------------------------------------
char const* GetNameForMIDINote(long inMidiNote)
{
	static char midiNoteName[16] = {0};
	constexpr long kNumNotesInOctave = 12;
	static char const* const keyNames[kNumNotesInOctave] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
	long const keyNameIndex = inMidiNote % kNumNotesInOctave;
	long const octaveNumber = (inMidiNote / kNumNotesInOctave) - 1;
	snprintf(midiNoteName, sizeof(midiNoteName), "%s %ld", keyNames[keyNameIndex], octaveNumber);
	return midiNoteName;
}

//-----------------------------------------------------------------------------
uint64_t GetMillisecondCount()
{
#if TARGET_OS_MAC
	// convert from 1/60 second to millisecond values
	return static_cast<uint64_t>(TickCount()) * 100 / 6;
#endif

#if _WIN32
	#if (_WIN32_WINNT >= 0x0600) && 0  // XXX how to runtime check without symbol error if unavailable?
	OSVERSIONINFO versionInfo = {0};
	versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
	if (GetVersionEx(&versionInfo) && (versionInfo.dwMajorVersion >= 6))
	{
		return GetTickCount64();
	}
	else
	#endif
	{
		return static_cast<UINT64>(GetTickCount());
	}
#endif
}

#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
std::unique_ptr<char[]> CreateCStringFromCFString(CFStringRef inCFString, CFStringEncoding inCStringEncoding)
{
	auto const stringBufferSize = CFStringGetMaximumSizeForEncoding(CFStringGetLength(inCFString) + 1, inCStringEncoding);
	if (stringBufferSize <= 0)
	{
		return {};
	}
	auto outputString = std::make_unique<char[]>(stringBufferSize);
	if (outputString)
	{
		std::fill_n(outputString.get(), stringBufferSize, 0);
		auto const stringSuccess = CFStringGetCString(inCFString, outputString.get(), stringBufferSize, inCStringEncoding);
		if (!stringSuccess)
		{
			outputString.reset();
		}
	}
	return outputString;
}
#endif


}  // namespace
