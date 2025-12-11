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
------------------------------------------------------------------------*/

#include "dfxguimisc.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstring>
#include <functional>
#include <locale>
#include <optional>
#include <span>
#include <utility>

#include "dfx-base.h"
#include "dfxmisc.h"

#if TARGET_OS_MAC
	#if !__OBJC__
		#error "you must compile the version of this file with a .mm filename extension, not this file"
	#elif !__has_feature(objc_arc)
		#error "you must compile this file with Automatic Reference Counting (ARC) enabled"
	#endif
	#import <AppKit/AppKit.h>
	#include <CoreFoundation/CoreFoundation.h>
	#include <CoreServices/CoreServices.h>
	#include "lib/platform/mac/macfactory.h"
#endif

#if TARGET_OS_WIN32
	#include <windows.h>
	// for ShellExecute
	#include <shellapi.h>
	#include <shlobj.h>
#endif


//-----------------------------------------------------------------------------
DGColor DGColor::withAlpha(float inAlpha) const
{
	DGColor color(*this);
	color.alpha = componentFromFloat(inAlpha);
	return color;
}

//-----------------------------------------------------------------------------
DGColor DGColor::darker(float inAmount) const
{
	if (inAmount < 0.0f)  // circumvent possible division by zero
	{
		return brighter(std::fabs(inAmount));
	}
	inAmount = kMaxValue_f / (kMaxValue_f + inAmount);

	auto const darken = [inAmount](auto component)
	{
		return componentFromFloat(inAmount * componentToFloat(component));
	};
	return DGColor(darken(red), darken(green), darken(blue), alpha);
}

//-----------------------------------------------------------------------------
DGColor DGColor::brighter(float inAmount) const
{
	if (inAmount < 0.0f)  // circumvent possible division by zero
	{
		return darker(std::fabs(inAmount));
	}
	inAmount = kMaxValue_f / (kMaxValue_f + inAmount);

	auto const brighten = [inAmount](auto component)
	{
		return componentFromFloat(kMaxValue_f - (inAmount * (kMaxValue_f - componentToFloat(component))));
	};
	return DGColor(brighten(red), brighten(green), brighten(blue), alpha);
}

//-----------------------------------------------------------------------------
DGColor DGColor::getSystem(System inSystemColorID)
{
	using OptionalColor = std::optional<DGColor>;

#if TARGET_OS_MAC
	auto const fromNSColor = [](NSColor* inColor) -> OptionalColor
	{
		if (inColor)
		{
			if (auto const rgbColor = [inColor colorUsingColorSpace:NSColorSpace.sRGBColorSpace])
			{
//std::printf("%lf %lf %lf %lf\n", rgbColor.redComponent * 255., rgbColor.greenComponent * 255., rgbColor.blueComponent * 255., rgbColor.alphaComponent);
				return DGColor(static_cast<float>(rgbColor.redComponent), static_cast<float>(rgbColor.greenComponent), 
							   static_cast<float>(rgbColor.blueComponent), static_cast<float>(rgbColor.alphaComponent));
			}
			// failure workaround: fill a bitmap context with the color to snoop it
			dfx::UniqueOpaqueType<CGColorSpaceRef, CGColorSpaceRelease> const colorSpace(CGColorSpaceCreateWithName(kCGColorSpaceSRGB));
			if (colorSpace)
			{
				constexpr size_t width = 1, height = 1, bitsPerComponent = 8, bytesPerRow = 0;
				dfx::UniqueOpaqueType<CGContextRef, CGContextRelease> const bitmapContext(CGBitmapContextCreate(nullptr, width, height, bitsPerComponent, bytesPerRow, colorSpace.get(), kCGImageAlphaPremultipliedLast));
				if (bitmapContext)
				{
					CGContextSetFillColorWithColor(bitmapContext.get(), inColor.CGColor);
					CGContextFillRect(bitmapContext.get(), CGRectMake(0, 0, width, height));
					if (auto const bitmapDataPtr = static_cast<uint8_t*>(CGBitmapContextGetData(bitmapContext.get())))
					{
						std::span const bitmapData(bitmapDataPtr, CGColorSpaceGetNumberOfComponents(colorSpace.get()));
						assert(bitmapData.size() == 3);
						return DGColor(bitmapData[0], bitmapData[1], bitmapData[2]);
					}
				}
			}
		}
		return {};
	};
	auto const fromNSColorProperty = [&fromNSColor](SEL inSelector) -> OptionalColor
	{
//NSLog(@"%@: ", NSStringFromSelector(inSelector));
		if (inSelector && [NSColor respondsToSelector:inSelector])
		{
			return fromNSColor([NSColor performSelector:inSelector]);
		}
		return {};
	};
	#define DFX_SELECTOR(x) @selector(x)
#else
	auto const fromNSColorProperty = []
	{
		return OptionalColor();
	};
	#define DFX_SELECTOR(x)
#endif  // TARGET_OS_MAC

	// NOTE: these fallback colors are based on those of macOS 10.13.5
	switch (inSystemColorID)
	{
		case System::WindowBackground:
			// TODO: why does converting this to components fail?
			return fromNSColorProperty(DFX_SELECTOR(windowBackgroundColor)).value_or(DGColor(239, 239, 239));
		case System::WindowFrame:
			return fromNSColorProperty(DFX_SELECTOR(windowFrameColor)).value_or(DGColor(170, 170, 170));
		case System::WindowTitle:
			return fromNSColorProperty(DFX_SELECTOR(windowFrameTextColor)).value_or(DGColor(0, 0, 0));
		case System::Label:
			return fromNSColorProperty(DFX_SELECTOR(labelColor)).value_or(DGColor(0.0f, 0.0f, 0.0f, 0.85f));
		case System::Control:
			return fromNSColorProperty(DFX_SELECTOR(controlColor)).value_or(DGColor(255, 255, 255));  // controlBackgroundColor?
		case System::ControlText:
			return fromNSColorProperty(DFX_SELECTOR(controlTextColor)).value_or(DGColor(0, 0, 0));
		case System::Text:
			return fromNSColorProperty(DFX_SELECTOR(textColor)).value_or(DGColor(0, 0, 0));
		case System::TextBackground:
			return fromNSColorProperty(DFX_SELECTOR(textBackgroundColor)).value_or(DGColor(255, 255, 255));
		case System::Accent:
			return fromNSColorProperty(DFX_SELECTOR(controlAccentColor)).value_or(DGColor(0, 122, 255));
		case System::AccentPressed:
#if TARGET_OS_MAC
			if (@available(macOS 10_14, *))
			{
				#if (MAC_OS_X_VERSION_MAX_ALLOWED <= MAC_OS_X_VERSION_10_13_4)
				constexpr NSInteger NSColorSystemEffectPressed = 1;
				#endif
				if (auto const color = fromNSColor([[NSColor controlAccentColor] colorWithSystemEffect:NSColorSystemEffectPressed]))
				{
					return *color;
				}
			}
#endif
			return DGColor(0.000000179f, 0.437901825f, 0.919842184f);
		case System::AccentControlText:
			// TODO: what is the correct system color?
			return fromNSColorProperty(DFX_SELECTOR(alternateSelectedControlTextColor)).value_or(DGColor(0, 0, 0));
		case System::FocusIndicator:
			return fromNSColorProperty(DFX_SELECTOR(keyboardFocusIndicatorColor)).value_or(DGColor(59, 153, 252));
		case System::ScrollBarColor:
			return fromNSColorProperty(DFX_SELECTOR(scrollBarColor)).value_or(DGColor(170, 170, 170));
	}
#undef DFX_SELECTOR

	std::unreachable();
}






#pragma mark -

//-----------------------------------------------------------------------------
std::string dfx::GetNameForMIDINote(int inMidiNote)
{
	constexpr auto keyNames = std::to_array({ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" });
	constexpr int kNumNotesInOctave = static_cast<int>(keyNames.size());
	static_assert(kNumNotesInOctave == 12);
	auto const keyNameIndex = inMidiNote % kNumNotesInOctave;
	auto const octaveNumber = (inMidiNote / kNumNotesInOctave) - 1;
	return std::string(keyNames[keyNameIndex]) + " " + std::to_string(octaveNumber);
}

//-----------------------------------------------------------------------------
std::string dfx::SanitizeNumericalInput(std::string_view inText)
{
	// remove digit separators
	// XXX TODO: this doesn't support locale, assumes comma
	std::string resultText(inText);
	std::erase_if(resultText, std::bind_front(std::equal_to<>{}, ','));

	// trim white space and any other noise (with respect to numerical parsing)
	while (!resultText.empty())
	{
		if (std::isspace(resultText.back()))
		{
			resultText.pop_back();
		}
		else if (std::isspace(resultText.front()))
		{
			resultText.erase(0, 1);
		}
		else if (resultText.starts_with(kPlusMinusUTF8))
		{
			resultText.erase(0, std::strlen(kPlusMinusUTF8));
		}
		else
		{
			break;
		}
	}

	return resultText;
}

//-----------------------------------------------------------------------------
// open up an URL in the user's default web browser
// returns whether it was successful
bool dfx::LaunchURL(std::string_view inURL)
{
	if (inURL.empty())
	{
		return false;
	}

#if TARGET_OS_MAC
	auto const cfURL = dfx::MakeUniqueCFType(CFURLCreateWithBytes(kCFAllocatorDefault, reinterpret_cast<UInt8 const*>(inURL.data()), static_cast<CFIndex>(inURL.length()), kCFStringEncodingASCII, nullptr));
	if (cfURL)
	{
		auto const status = LSOpenCFURLRef(cfURL.get(), nullptr);  // try to launch the URL
		return status == noErr;
	}
#endif

#if TARGET_OS_WIN32
	// Return value is a fake HINSTANCE that will be >32 (if successful) or some error code
	// otherwise. If we care about error handling, should update to ShellExecuteEx.
	auto const status = reinterpret_cast<intptr_t>(ShellExecute(nullptr, "open",
																std::string(inURL).c_str(),
																nullptr, nullptr, SW_SHOWNORMAL));
	return status > 32;
#endif

	return false;
}

#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
// this function looks for the plugin's documentation file in the appropriate system location, 
// within a given file system domain, and returns a CFURLRef for the file if it is found, 
// and null otherwise (or if some error is encountered along the way)
dfx::UniqueCFType<CFURLRef> DFX_FindDocumentationFileInDomain(CFStringRef inDocsFileName, NSSearchPathDomainMask inDomain)
{
	assert(inDocsFileName);

	// first find the base directory for the system documentation directory
	auto const docsDirURL = [NSFileManager.defaultManager URLForDirectory:NSDocumentationDirectory inDomain:inDomain appropriateForURL:nil create:NO error:nil];
	if (docsDirURL)
	{
		// create a CFURL for the "manufacturer name" directory within the documentation directory
		auto const dfxDocsDirURL = dfx::MakeUniqueCFType(CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, (__bridge CFURLRef)docsDirURL, CFSTR(PLUGIN_CREATOR_NAME_STRING), true));
		if (dfxDocsDirURL)
		{
			// create a CFURL for the documentation file within the "manufacturer name" directory
			auto docsFileURL = dfx::MakeUniqueCFType(CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, dfxDocsDirURL.get(), inDocsFileName, false));
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
bool dfx::LaunchDocumentation()
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
		auto docsFileURL = dfx::MakeUniqueCFType(CFBundleCopyResourceURL(pluginBundleRef, docsFileName, nullptr, docsSubdirName));
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
				auto const bundleURL = dfx::MakeUniqueCFType(CFBundleCopyBundleURL(pluginBundleRef));
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
	auto docsFileName = dfx::ToLower(PLUGIN_NAME_STRING ".html");
	auto const isSpace = std::bind(std::isspace<std::string::value_type>, std::placeholders::_1, std::locale::classic());
	std::erase_if(docsFileName, isSpace);
	return dfx::LaunchURL(DESTROYFX_URL "/docs/" + docsFileName);
#endif  // TARGET_OS_MAC

	return false;
}

//-----------------------------------------------------------------------------
void dfx::InitGUI()
{
#if TARGET_OS_MAC
	static_assert((MAC_OS_X_VERSION_MIN_REQUIRED <= MAC_OS_X_VERSION_10_15) || TARGET_CPU_ARM64, "this workaround can be retired");
	auto const processInfo = [NSProcessInfo processInfo];
	if ([processInfo respondsToSelector:@selector(operatingSystemVersion)])
	{
		auto const systemVersion = processInfo.operatingSystemVersion;
		if ((systemVersion.majorVersion == 10) && (systemVersion.minorVersion == 15))
		{
			// workaround for macOS 10.15 bug causing aliased text to render doubled/smeared
			// https://github.com/steinbergmedia/vstgui/issues/141
			VSTGUI::getPlatformFactory().asMacFactory()->setUseAsynchronousLayerDrawing(false);
		}
	}
#endif
}
