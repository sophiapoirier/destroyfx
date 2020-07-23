/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2020  Sophia Poirier

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
------------------------------------------------------------------------*/

#include "dfxguimisc.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <optional>
#include <string_view>

#include "dfxmisc.h"
#include "dfxgui-base.h"

#if TARGET_OS_WIN32
#include <windows.h>
// Some platform-specific vstgui peeking in order to load custom fonts.
#include "lib/platform/win32/win32support.h"
#endif

#if TARGET_OS_MAC
	#if !__OBJC__
		#error "you must compile the version of this file with a .mm filename extension, not this file"
	#elif !__has_feature(objc_arc)
		#error "you must compile this file with Automatic Reference Counting (ARC) enabled"
	#endif
	#import <AppKit/NSColor.h>
	#import <AppKit/NSColorSpace.h>
	#import <AppKit/NSFont.h>
#endif


// XXX
#include "dfxtrace.h"

char const* const dfx::kPlusMinusUTF8 = reinterpret_cast<char const*>(u8"\U000000B1");


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
		return componentFromFloat(kMaxValue - (inAmount * (kMaxValue_f - componentToFloat(component))));
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
			auto const rgbColor = [inColor colorUsingColorSpace:NSColorSpace.sRGBColorSpace];
			if (rgbColor)
			{
//printf("%lf %lf %lf %lf\n", rgbColor.redComponent * 255.0, rgbColor.greenComponent * 255.0, rgbColor.blueComponent * 255.0, rgbColor.alphaComponent);
				return DGColor(static_cast<float>(rgbColor.redComponent), static_cast<float>(rgbColor.greenComponent), 
							   static_cast<float>(rgbColor.blueComponent), static_cast<float>(rgbColor.alphaComponent));
			}
			else if ([inColor respondsToSelector:@selector(CGColor)])
			{
				// failure workaround: fill a bitmap context with the color to snoop it
				dfx::UniqueOpaqueType<CGColorSpaceRef, CGColorSpaceRelease> const colorSpace(CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB));
				if (colorSpace)
				{
					constexpr size_t width = 1, height = 1;
					dfx::UniqueOpaqueType<CGContextRef, CGContextRelease> const bitmapContext(CGBitmapContextCreate(nullptr, width, height, 8, 0, colorSpace.get(), kCGImageAlphaPremultipliedLast));
					if (bitmapContext)
					{
						CGContextSetFillColorWithColor(bitmapContext.get(), inColor.CGColor);
						CGContextFillRect(bitmapContext.get(), CGRectMake(0, 0, width, height));
						if (auto const bitmapData = static_cast<uint8_t*>(CGBitmapContextGetData(bitmapContext.get())))
						{
							return DGColor(bitmapData[0], bitmapData[1], bitmapData[2], bitmapData[3]);
						}
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
	auto const fromNSColorProperty = []()
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
	}
#undef DFX_SELECTOR

	assert(false);
	return kBlack;
}






#pragma mark -

#if TARGET_OS_WIN32
// TODO: VSTGUI does have support for this, but it uses APIs that are only
// available in Windows 10 version 1703 (April 2017). Once this version
// is widely used we can retire this hack, but currently some 35% of users
// are on Windows 8 or earlier!
static bool InstallFontResource(const char *resource_name) {
  HRSRC rsrc = FindResourceA(VSTGUI::GetInstance(), resource_name, "TTF");
  if (rsrc == nullptr) {
    TRACE("FindResourceA failed");
    return false;
  }

  DWORD rsize = SizeofResource(VSTGUI::GetInstance(), rsrc);
  HGLOBAL data_load = LoadResource(VSTGUI::GetInstance(), rsrc);
  if (data_load == nullptr) {
    TRACE("LoadResource failed?");
    return false;
  }
  // (Note that this is not really a "lock" and there is no corresponding
  // unlock.)
  HGLOBAL data = LockResource(data_load);
  if (data == nullptr) {
    TRACE("LockResource failed??");
    return false;
  }

  // It would be nice if we could just do
  //  AddFontMemResourceEx(data, rsize, 0, &num_loaded);
  // but, while this successfully installs the font from memory, the
  // font is not enumerable and so it cannot be found by its name
  // when VSTGUI tries to create it. Weirdly there is no option to
  // do this for the memory version, but it is possible with the
  // version that reads from a file. So, write the resource data to
  // a temporary file and load it from there.
  
  char temp_path[MAX_PATH + 1] = {};
  if (!GetTempPathA(MAX_PATH, temp_path)) {
    TRACE("GetTempPathA failed");
    return false;
  }

  // Note that the temp path may not exist, or may not be writable;
  // it just comes from the environment. So this and the file operations
  // can definitely fail.
  char temp_filename[MAX_PATH + 15] = {};
  if (!GetTempFileNameA(temp_path, "dfx_font_", 0, temp_filename)) {
    TRACE("GetTempFileName failed");
    return false;
  }
  
  HANDLE temp_fh = CreateFileA((LPTSTR)temp_filename,
			       GENERIC_WRITE,
			       0,
			       nullptr,
			       CREATE_ALWAYS,
			       FILE_ATTRIBUTE_NORMAL,
			       nullptr);
  if (temp_fh == INVALID_HANDLE_VALUE) {
    TRACE("CreateFile failed for temp file");
    return false;
  }

  // Write the data to the temp file.
  DWORD bytes_written = 0;
  if (!WriteFile(temp_fh, data, rsize, &bytes_written, nullptr)) {
    TRACE("WriteFile failed");
    return false;
  }
  if (bytes_written != rsize) {
    // (Sanity checking; WriteFile should have returned false.)
    TRACE("Incomplete WriteFile");
    return false;
  }

  if (!CloseHandle(temp_fh)) {
    TRACE("Couldn't close the temp file handle??");
    return false;
  }

  if (!AddFontResourceExA(temp_filename,
			  /* We DO want enumerable, but it can be private
			     to this process. */
			  0 /* FR_PRIVATE */,
			  /* Reserved, must be zero */
			  0)) {
    TRACE("AddFontResourceEx failed");
    return false;
  }

  // XXX: Couple problems here. If you install with FR_PRIVATE, then VSTGUI
  // doesn't find the font. :( Perhaps it is "not the same process" somehow?
  // But if we pass a value of 0, then it works, it's just also available to
  // other programs for the remainder of the boot. (It seems a bit rude to
  // have this phantom font installed that the user could use in a document
  // but then it goes away on the next boot?)
  // - We can't actually delete the temp file. Looks like you need to pair
  //   this with RemoveFontResourceExA, and there is somd kind of internal
  //   reference counting.
  // - So we could instead move this in to the DFXGUI wrapper, and also
  //   uninstall/delete the temp file when we unload that. Then we'd only
  //   have the issue that the user can use the font in another application
  //   during the period of time they have the plugin GUI open, which is
  //   not so bad (and actually not uncommon on windows I think).
  // - Should check to make sure that if the user has the font installed
  //   normally, we don't end up interfering with it, like uninstalling
  //   it when we unregister?
  
  // Success! Clean up the temp file.
  if (!DeleteFileA(temp_filename)) {
    TRACE("Couldn't delete temp file");
    // (But we did succeed in installing the font...)
    return false;
  }
  
  TRACE("Font successfully installed");
  return true;
}

static void InstallFontOnce(const std::string_view inFontName) {
  // Here, 'static' is just used for thread-safe one-time initialization. We
  // don't bother checking the return value because there's nothing to do
  // about failure (GUI will fall back to some system font, which is fine).
  if (inFontName == dfx::kFontName_SnootPixel10) {
    [[maybe_unused]] static auto once = InstallFontResource("px10.ttf");
  } else if (inFontName == dfx::kFontName_BoringBoron) {
    [[maybe_unused]] static auto once = InstallFontResource("bboron.ttf");
  }
}
#endif

//-----------------------------------------------------------------------------
VSTGUI::SharedPointer<VSTGUI::CFontDesc> dfx::CreateVstGuiFont(float inFontSize, char const* inFontName)
{
  TRACE("CreateVstGuiFont");
  
	if (inFontName)
	{
#if TARGET_OS_WIN32
		InstallFontOnce(inFontName);
#endif
		return VSTGUI::makeOwned<VSTGUI::CFontDesc>(inFontName, inFontSize);
	}
	else
	{
		auto fontDesc = VSTGUI::makeOwned<VSTGUI::CFontDesc>(VSTGUI::kSystemFont->getName(), inFontSize);
#if TARGET_OS_MAC
		if (auto const fontName = [NSFont systemFontOfSize:NSFont.systemFontSize].displayName)
		{
			fontDesc->setName(fontName.UTF8String);
		}
#endif
		return fontDesc;
	}
}

//-----------------------------------------------------------------------------
std::string dfx::SanitizeNumericalInput(std::string const& inText)
{
	// remove digit separators
	// XXX TODO: this doesn't support locale, assumes comma
	std::string resultText(inText.size(), '\0');
	resultText.erase(std::remove_copy(inText.cbegin(), inText.cend(), resultText.begin(), ','), resultText.cend());

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
		else if (resultText.find(kPlusMinusUTF8) == 0)
		{
			resultText.erase(0, strlen(kPlusMinusUTF8));
		}
		else
		{
			break;
		}
	}

	return resultText;
}
