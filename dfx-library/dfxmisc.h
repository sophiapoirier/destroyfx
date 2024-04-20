/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2024  Sophia Poirier

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

#pragma once


#include <algorithm>
#include <atomic>
#include <bit>
#include <cassert>
#include <concepts>
#include <cstring>
#include <initializer_list>
#include <memory>
#include <span>
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
// used in compile-time branches to enforce unreachable case
template <typename>
inline constexpr bool AlwaysFalse = false;



//-----------------------------------------------------------------------------
template <typename T>
requires std::atomic<T>::is_always_lock_free
using LockFreeAtomic = std::atomic<T>;


//-----------------------------------------------------------------------------
namespace detail
{
	template <typename T, auto D>
	requires std::is_pointer_v<T> && std::is_invocable_v<decltype(D), T>
	struct UniqueTypeDeleter
	{
		void operator()(T object) noexcept
		{
			D(object);
		}
	};
}

//-----------------------------------------------------------------------------
template <typename T, typename D = detail::UniqueTypeDeleter<T*, std::free>>
auto MakeUniqueMemoryBlock(size_t size) noexcept
{
	if constexpr (!std::is_void_v<T>)
	{
		assert((size % sizeof(T)) == 0);
	}
	return std::unique_ptr<T, D>(static_cast<T*>(std::malloc(size)), D());
}

//-----------------------------------------------------------------------------
template <typename T, auto D>
using UniqueOpaqueType = std::unique_ptr<typename std::remove_pointer_t<T>, detail::UniqueTypeDeleter<T, D>>;

#if TARGET_OS_MAC
//-----------------------------------------------------------------------------
template <typename T, typename D = detail::UniqueTypeDeleter<T, CFRelease>>
using UniqueCFType = std::unique_ptr<typename std::remove_pointer_t<T>, D>;

template <typename T, typename D = detail::UniqueTypeDeleter<T, CFRelease>>
auto MakeUniqueCFType(T object) noexcept
{
	return UniqueCFType<T, D>(object, D());
}
#endif



//-----------------------------------------------------------------------------
template <typename T>
inline constexpr bool IsTriviallySerializable = std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;
template <typename T>
concept TriviallySerializable = IsTriviallySerializable<T>;

//-----------------------------------------------------------------------------
template <TriviallySerializable SourceT, typename DestinationT>
requires (std::is_same_v<std::decay_t<SourceT>, std::decay_t<DestinationT>> || std::is_void_v<DestinationT>) &&
		 (!std::is_const_v<DestinationT>)
void MemCpyObject(SourceT const& inSource, DestinationT* outDestination) noexcept
{
	std::memcpy(outDestination, &inSource, sizeof(SourceT));
}

//-----------------------------------------------------------------------------
// take a pointer to byte-serialized data and copy it to an aligned and valid-object-lifetime-having instance
// (dereferencing such pointers is undefined behavior)
template <TriviallySerializable T>
requires std::is_nothrow_default_constructible_v<T> && (!std::is_const_v<T>)
T Enliven(T const* inData) noexcept
{
	T result {};
	std::memcpy(&result, inData, sizeof(T));
	return result;
}
//-----------------------------------------------------------------------------
template <TriviallySerializable T>
requires std::is_nothrow_default_constructible_v<T> && (!std::is_const_v<T>)
T Enliven(void const* inData) noexcept
{
	return Enliven(static_cast<T const*>(inData));
}

//-----------------------------------------------------------------------------
void ReverseBytes(void* ioData, size_t inItemSize, size_t inItemCount);

//-----------------------------------------------------------------------------
template <TriviallySerializable T>
void ReverseBytes(std::span<T> ioData)
{
	if constexpr (std::is_integral_v<T>)
	{
		std::ranges::transform(ioData, ioData.begin(), [](T value){ return std::byteswap(value); });
	}
	else
	{
		ReverseBytes(ioData.data(), sizeof(T), ioData.size());
	}
}

//-----------------------------------------------------------------------------
void ReverseBytes(TriviallySerializable auto& ioData)
{
	ReverseBytes(std::span(&ioData, 1));
}

std::string ToLower(std::string_view inText);
// Same as the nonstandard strlcat() function.
// Appends 'appendme' to the nul-terminated string in buf, assuming that
// buf has at least maxlen bytes allocated. If the input buffer is nul-
// terminated, then the output buffer always will be too.
size_t StrlCat(char* buf, std::string_view appendme, size_t maxlen);
// same as the nonstandard strlcpy() function
size_t StrLCpy(char* dst, std::string_view src, size_t maxlen);

unsigned int CompositePluginVersionNumberValue() noexcept;
bool LaunchURL(std::string const& inURL);
bool LaunchDocumentation();
std::string GetNameForMIDINote(int inMidiNote);

#if TARGET_OS_MAC
std::unique_ptr<char[]> CreateCStringFromCFString(CFStringRef inCFString, CFStringEncoding inCStringEncoding = kCFStringEncodingUTF8);
bool IsHostValidator();
#endif

// coming up with these sets of short parameter names can be annoying, so here are some sets we use repeatedly
inline constexpr std::initializer_list<std::string_view> kParameterNames_Tempo = {"tempo", "Tmpo"};
inline constexpr std::initializer_list<std::string_view> kParameterNames_TempoSync = {"tempo sync", "TmpoSnc", "TpoSnc", "Sync"};
inline constexpr std::initializer_list<std::string_view> kParameterNames_TempoAuto = {"sync to host tempo", "HstTmpo", "HstTmp", "HTmp"};
inline constexpr std::initializer_list<std::string_view> kParameterNames_DryWetMix = {"dry/wet mix", "DryWet", "DrWt"};
inline constexpr std::initializer_list<std::string_view> kParameterNames_Floor = {"floor", "Flor"};
inline constexpr std::initializer_list<std::string_view> kParameterNames_Freeze = {"freeze", "Frez"};
inline constexpr std::initializer_list<std::string_view> kParameterNames_Attack = {"attack", "Attk"};
inline constexpr std::initializer_list<std::string_view> kParameterNames_Release = {"release", "Releas", "Rles"};
inline constexpr std::initializer_list<std::string_view> kParameterNames_VelocityInfluence = {"velocity influence", "VelInfl", "VelInf", "Velo"};
inline constexpr std::initializer_list<std::string_view> kParameterNames_PitchBendRange = {"pitch bend range", "PtchBnd", "PtchBd", "PB"};
inline constexpr std::initializer_list<std::string_view> kParameterNames_MidiMode = {"MIDI mode", "MIDIMod", "MIDIMd", "MIDI"};

}  // namespace
