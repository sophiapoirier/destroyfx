/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2021  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 2 of the License, or 
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
#include <cassert>
#include <cctype>
#include <cstring>
#include <functional>
#include <locale>
#include <string_view>
#include <string>

#include "dfxdefines.h"

#if TARGET_OS_MAC
	#include <CoreServices/CoreServices.h>
	#import <Foundation/NSFileManager.h>
	#if !__OBJC__
		#error "you must compile the version of this file with a .mm filename extension, not this file"
	#elif !__has_feature(objc_arc)
		#error "you must compile this file with Automatic Reference Counting (ARC) enabled"
	#endif
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
// this reverses the bytes in a stream of data, for correcting endian difference
void ReverseBytes(void* ioData, size_t inItemSize, size_t inItemCount)
{
	size_t const half = (inItemSize / 2) + (inItemSize % 2);
	auto dataBytes = static_cast<std::byte*>(ioData);

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

//------------------------------------------------------
std::string ToLower(std::string_view inText)
{
	auto const toLower = std::bind(std::tolower<std::string::value_type>, std::placeholders::_1, std::locale::classic());
	std::string result;
	std::transform(inText.cbegin(), inText.cend(), std::back_inserter(result), toLower);
	return result;
}

//------------------------------------------------------
size_t StrlCat(char* buf, std::string_view appendme, size_t maxlen)
{
	const size_t appendlen = appendme.size();
	const size_t buflen = strnlen(buf, maxlen);
	// no more space
	if (buflen >= maxlen) return maxlen + appendlen;
	const size_t remaining = maxlen - buflen;
	// remaining > 1
	// Bytes to copy, not including terminating nul.
	const size_t copylen =
		appendlen < remaining ? appendlen : maxlen - 1;
	memcpy(&buf[buflen], appendme.data(), copylen);
	buf[buflen + copylen] = '\0';
	return buflen + appendlen;
}

//------------------------------------------------------
// adapted from Darwin https://github.com/apple/darwin-xnu/blob/master/osfmk/arm/strlcpy.c
size_t StrLCpy(char* dst, std::string_view src, size_t maxlen)
{
	if ((src.size() + 1) < maxlen)
	{
		memcpy(dst, src.data(), src.size());
		dst[src.size()] = '\0';
	}
	else if (maxlen != 0)
	{
		memcpy(dst, src.data(), maxlen - 1);
		dst[maxlen - 1] = '\0';
	}
	return src.size();
}


//------------------------------------------------------
long CompositePluginVersionNumberValue()
{
	return (PLUGIN_VERSION_MAJOR << 16) | (PLUGIN_VERSION_MINOR << 8) | PLUGIN_VERSION_BUGFIX;
}

//-----------------------------------------------------------------------------
// handy function to open up an URL in the user's default web browser
// returns whether it was successful
bool LaunchURL(std::string const& inURL)
{
	if (inURL.empty())
	{
		return false;
	}

#if TARGET_OS_MAC
	auto const cfURL = MakeUniqueCFType(CFURLCreateWithBytes(kCFAllocatorDefault, reinterpret_cast<UInt8 const*>(inURL.data()), static_cast<CFIndex>(inURL.length()), kCFStringEncodingASCII, nullptr));
	if (cfURL)
	{
		auto const status = LSOpenCFURLRef(cfURL.get(), nullptr);  // try to launch the URL
		return status == noErr;
	}
#endif

#if _WIN32
	// Return value is a fake HINSTANCE that will be >32 (if successful) or some error code
	// otherwise. If we care about error handling, should update to ShellExecuteEx.
	auto const ret = reinterpret_cast<intptr_t>(ShellExecute(nullptr, "open", inURL.c_str(), 
															 nullptr, nullptr, SW_SHOWNORMAL));
	return ret > 32;
#endif

	return false;
}

#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
// this function looks for the plugin's documentation file in the appropriate system location, 
// within a given file system domain, and returns a CFURLRef for the file if it is found, 
// and null otherwise (or if some error is encountered along the way)
UniqueCFType<CFURLRef> DFX_FindDocumentationFileInDomain(CFStringRef inDocsFileName, NSSearchPathDomainMask inDomain)
{
	assert(inDocsFileName);

	// first find the base directory for the system documentation directory
	auto const docsDirURL = [NSFileManager.defaultManager URLForDirectory:NSDocumentationDirectory inDomain:inDomain appropriateForURL:nil create:NO error:nil];
	if (docsDirURL)
	{
		// create a CFURL for the "manufacturer name" directory within the documentation directory
		auto const dfxDocsDirURL = MakeUniqueCFType(CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, (__bridge CFURLRef)docsDirURL, CFSTR(PLUGIN_CREATOR_NAME_STRING), true));
		if (dfxDocsDirURL)
		{
			// create a CFURL for the documentation file within the "manufacturer name" directory
			auto docsFileURL = MakeUniqueCFType(CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, dfxDocsDirURL.get(), inDocsFileName, false));
			if (docsFileURL)
			{
				// check to see if the hypothetical documentation file actually exists 
				// (CFURLs can reference files that don't exist)
				// and only return the CFURL if the file does exists
				if (CFURLResourceIsReachable(docsFileURL.get(), nullptr))
				{
					return docsFileURL;
				}
			}
		}
	}

	return {};
}
#endif

//-----------------------------------------------------------------------------
// XXX this function should really go somewhere else, like in that promised DFX utilities file or something like that
bool LaunchDocumentation()
{
#if TARGET_OS_MAC
	// no assumptions can be made about how long the reference is valid, 
	// and the caller should not attempt to release the CFBundleRef object
	auto const pluginBundleRef = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));
	assert(pluginBundleRef);
	if (pluginBundleRef)
	{
		CFStringRef docsFileName = CFSTR(PLUGIN_NAME_STRING " manual.html");
	#ifdef PLUGIN_DOCUMENTATION_FILE_NAME
		docsFileName = CFSTR(PLUGIN_DOCUMENTATION_FILE_NAME);
	#endif
		CFStringRef docsSubdirName = nullptr;
	#ifdef PLUGIN_DOCUMENTATION_SUBDIRECTORY_NAME
		docsSubdirName = CFSTR(PLUGIN_DOCUMENTATION_SUBDIRECTORY_NAME);
	#endif
		auto docsFileURL = MakeUniqueCFType(CFBundleCopyResourceURL(pluginBundleRef, docsFileName, nullptr, docsSubdirName));
		// if the documentation file is not found in the bundle, then search in appropriate system locations
		if (!docsFileURL)
		{
			docsFileURL = DFX_FindDocumentationFileInDomain(docsFileName, NSUserDomainMask);
		}
		if (!docsFileURL)
		{
			docsFileURL = DFX_FindDocumentationFileInDomain(docsFileName, NSLocalDomainMask);
		}
		if (!docsFileURL)
		{
			docsFileURL = DFX_FindDocumentationFileInDomain(docsFileName, NSNetworkDomainMask);
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
				auto const bundleURL = MakeUniqueCFType(CFBundleCopyBundleURL(pluginBundleRef));
				if (bundleURL)
				{
					if (AHRegisterHelpBookWithURL)  // available starting in Mac OS X 10.6
					{
						helpBookRegistered = (AHRegisterHelpBookWithURL(bundleURL.get()) == noErr);
					}
					else
					{
						FSRef bundleRef {};
						if (CFURLGetFSRef(bundleURL.get(), &bundleRef))
						{
							helpBookRegistered = (AHRegisterHelpBook(&bundleRef) == noErr);
						}
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
			return status == noErr;
		}
	}
#else
	// XXX this will load latest docs on our website which may not match the version of the running software
	// TODO: embed the documentation into Windows builds somehow?
	auto docsFileName = ToLower(PLUGIN_NAME_STRING ".html");
	while (true)
	{
		auto const isSpace = std::bind(std::isspace<std::string::value_type>, std::placeholders::_1, std::locale::classic());
		auto const foundCharacter = std::find_if(docsFileName.cbegin(), docsFileName.cend(), isSpace);
		if (foundCharacter == docsFileName.end())
		{
			break;
		}
		docsFileName.erase(foundCharacter);
	}
	return LaunchURL(DESTROYFX_URL "/docs/" + docsFileName);
#endif  // TARGET_OS_MAC

	return false;
}

//-----------------------------------------------------------------------------
std::string GetNameForMIDINote(int inMidiNote)
{
	constexpr int kNumNotesInOctave = 12;
	constexpr char const* const keyNames[kNumNotesInOctave] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
	auto const keyNameIndex = inMidiNote % kNumNotesInOctave;
	auto const octaveNumber = (inMidiNote / kNumNotesInOctave) - 1;
	return std::string(keyNames[keyNameIndex]) + " " + std::to_string(octaveNumber);
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
