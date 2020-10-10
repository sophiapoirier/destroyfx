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

Destroy FX is a sovereign entity comprised of Sophia Poirier and Tom Murphy 7.
These are some generally useful functions.
------------------------------------------------------------------------*/

#pragma once


#include <array>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#ifdef __MACH__
	#include <CoreFoundation/CoreFoundation.h>
#endif


namespace dfx
{


//-----------------------------------------------------------------------------
namespace detail
{
	template <typename T, auto D>
	struct UniqueTypeDeleter
	{
		static_assert(std::is_pointer_v<T>);
		static_assert(std::is_invocable_v<decltype(D), T>);
		void operator()(T object)
		{
			D(object);
		}
	};
}

//-----------------------------------------------------------------------------
template <typename T, typename D = detail::UniqueTypeDeleter<T*, std::free>>
class UniqueMemoryBlock : public std::unique_ptr<T, D>
{
public:
	explicit UniqueMemoryBlock(size_t size)
	:	std::unique_ptr<T, D>(static_cast<T*>(std::malloc(size)), D())
	{
	}
};

//-----------------------------------------------------------------------------
template <typename T, auto D>
using UniqueOpaqueType = std::unique_ptr<typename std::remove_pointer_t<T>, detail::UniqueTypeDeleter<T, D>>;

#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
template <typename T, typename D = detail::UniqueTypeDeleter<T, CFRelease>>
class UniqueCFType : public std::unique_ptr<typename std::remove_pointer_t<T>, D>
{
public:
	UniqueCFType(T object = nullptr) noexcept
	:	std::unique_ptr<typename std::remove_pointer_t<T>, D>(object, D())
	{
	}
};
#endif



//-----------------------------------------------------------------------------
template <typename T>
inline constexpr bool IsTriviallySerializable = std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;

//-----------------------------------------------------------------------------
void ReverseBytes(void* ioData, size_t inItemSize, size_t inItemCount);
template <typename T>
void ReverseBytes(T* ioData, size_t inItemCount)
{
	static_assert(IsTriviallySerializable<T>);
	ReverseBytes(ioData, sizeof(T), inItemCount);
}
template <typename T>
void ReverseBytes(T& ioData)
{
	ReverseBytes(&ioData, 1);
}

// Same as the nonstandard strlcat() function.
// Appends 'appendme' to the nul-terminated string in buf, assuming that
// buf has at least maxlen bytes allocated. If the input buffer is nul-
// terminated, then the output buffer always will be too.
size_t StrlCat(char* buf, std::string_view appendme, size_t maxlen);
// same as the nonstandard strlcpy() function
size_t StrLCpy(char* dst, std::string_view src, size_t maxlen);

long CompositePluginVersionNumberValue();
bool LaunchURL(std::string const& inURL);
bool LaunchDocumentation();
std::string GetNameForMIDINote(int inMidiNote);

#if TARGET_OS_MAC
std::unique_ptr<char[]> CreateCStringFromCFString(CFStringRef inCFString, CFStringEncoding inCStringEncoding = kCFStringEncodingUTF8);
#endif

// coming up with these sets of short parameter names can be annoying, so here are some sets we use repeatedly
// TODO: C++20 just use std::vector (and eliminate MakeParameterNames) since it gains constexpr constructors
static constexpr std::array<std::string_view, 2> kParameterNames_Tempo = {"tempo", "Tmpo"};
static constexpr std::array<std::string_view, 4> kParameterNames_TempoSync = {"tempo sync", "TmpoSnc", "TpoSnc", "Sync"};
static constexpr std::array<std::string_view, 4> kParameterNames_TempoAuto = {"sync to host tempo", "HstTmpo", "HstTmp", "HTmp"};
static constexpr std::array<std::string_view, 3> kParameterNames_DryWetMix = {"dry/wet mix", "DryWet", "DrWt"};
static constexpr std::array<std::string_view, 2> kParameterNames_Floor = {"floor", "Flor"};
static constexpr std::array<std::string_view, 2> kParameterNames_Freeze = {"freeze", "Frez"};
static constexpr std::array<std::string_view, 2> kParameterNames_Attack = {"attack", "Attk"};
static constexpr std::array<std::string_view, 3> kParameterNames_Release = {"release", "Releas", "Rles"};
static constexpr std::array<std::string_view, 4> kParameterNames_VelocityInfluence = {"velocity influence", "VelInfl", "VelInf", "Velo"};
static constexpr std::array<std::string_view, 4> kParameterNames_PitchBendRange = {"pitch bend range", "PtchBnd", "PtchBd", "PB"};
static constexpr std::array<std::string_view, 4> kParameterNames_MidiMode = {"MIDI mode", "MIDIMod", "MIDIMd", "MIDI"};
// convenience function to produce the required initparameter_* argument from the above compile-time representation
template <auto Size>
auto MakeParameterNames(std::array<std::string_view, Size> const& inNamesArray)
{
	return std::vector(inNamesArray.cbegin(), inNamesArray.cend());
}

}  // namespace
