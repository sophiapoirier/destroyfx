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
#include <cmath>
#include <optional>

#include "dfxmisc.h"

#if TARGET_OS_MAC
	#if !__OBJC__
		#error "you must compile the version of this file with a .mm filename extension, not this file"
	#endif
	#import <AppKit/NSColor.h>
	#import <AppKit/NSColorSpace.h>
	#include <Carbon/Carbon.h>
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
}






#pragma mark -

//-----------------------------------------------------------------------------
VSTGUI::SharedPointer<VSTGUI::CFontDesc> dfx::CreateVstGuiFont(float inFontSize, char const* inFontName)
{
	if (inFontName)
	{
		return VSTGUI::makeOwned<VSTGUI::CFontDesc>(inFontName, inFontSize);
	}
	else
	{
		auto fontDesc = VSTGUI::makeOwned<VSTGUI::CFontDesc>(VSTGUI::kSystemFont->getName(), inFontSize);
#if TARGET_OS_MAC
		// get the application font from the system "theme"
		auto const fontType = HIThemeGetUIFontType(kThemeApplicationFont);
		if (fontType != kCTFontNoFontType)
		{
			dfx::UniqueCFType const fontRef = CTFontCreateUIFontForLanguage(fontType, 0.0, nullptr);
			if (fontRef)
			{
				dfx::UniqueCFType const fontCFName = CTFontCopyFullName(fontRef.get());
				if (fontCFName)
				{
					auto const fontCName = dfx::CreateCStringFromCFString(fontCFName.get(), kCFStringEncodingUTF8);
					if (fontCName)
					{
						fontDesc->setName(fontCName.get());
					}
				}
			}
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
		if (isspace(resultText.back()))
		{
			resultText.pop_back();
		}
		else if (isspace(resultText.front()))
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
