/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2025  Sophia Poirier

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

To contact the author, use the contact form at http://destroyfx.org

Destroy FX is a sovereign entity comprised of Sophia Poirier and Tom Murphy 7.
These are some generally useful functions.
------------------------------------------------------------------------*/

#include "dfxmisc.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstring>
#include <functional>
#include <iterator>
#include <locale>
#include <span>
#include <string_view>
#include <string>

#include "dfx-base.h"

#if TARGET_OS_MAC
	#if !__OBJC__
		#error "you must compile the version of this file with a .mm filename extension, not this file"
	#elif !__has_feature(objc_arc)
		#error "you must compile this file with Automatic Reference Counting (ARC) enabled"
	#endif
	#import <Foundation/NSProcessInfo.h>
	#import <Foundation/NSString.h>
#endif


namespace dfx
{


//------------------------------------------------------
// this reverses the bytes in a stream of data, for correcting endian difference
void ReverseBytes(void* ioData, size_t inItemSize, size_t inItemCount)
{
	size_t const halfItemSize = inItemSize / 2;
	std::span dataBytes(static_cast<std::byte*>(ioData), inItemSize * inItemCount);

	for (size_t itemIndex = 0; itemIndex < inItemCount; itemIndex++)
	{
		for (size_t byteIndex = 0; byteIndex < halfItemSize; byteIndex++)
		{
			size_t const complementIndex = (inItemSize - 1) - byteIndex;
			std::swap(dataBytes[byteIndex], dataBytes[complementIndex]);
		}
		dataBytes = dataBytes.subspan(inItemSize);
	}
}

//------------------------------------------------------
std::string ToLower(std::string_view inText)
{
	auto const toLower = std::bind(std::tolower<std::string::value_type>, std::placeholders::_1, std::locale::classic());
	std::string result;
	std::ranges::transform(inText, std::back_inserter(result), toLower);
	return result;
}

//------------------------------------------------------
// adapted from Darwin https://github.com/apple/darwin-xnu/blob/main/osfmk/arm/strlcpy.c
size_t StrLCpy(char* dst, std::string_view src, size_t maxlen)
{
	if ((src.size() + 1) < maxlen)
	{
		std::memcpy(dst, src.data(), src.size());
		dst[src.size()] = '\0';
	}
	else if (maxlen != 0)
	{
		std::memcpy(dst, src.data(), maxlen - 1);
		dst[maxlen - 1] = '\0';
	}
	return src.size();
}


//------------------------------------------------------
unsigned int CompositePluginVersionNumberValue() noexcept
{
	return (PLUGIN_VERSION_MAJOR << 16) | (PLUGIN_VERSION_MINOR << 8) | PLUGIN_VERSION_BUGFIX;
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

//-----------------------------------------------------------------------------
bool IsHostValidator()
{
	auto const processName = NSProcessInfo.processInfo.processName;
	return [processName isEqual:@"auvaltool"] || [processName isEqual:@"pluginval"];
}
#endif  // TARGET_OS_MAC


}  // namespace
