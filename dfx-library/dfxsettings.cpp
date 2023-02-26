/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2023  Sophia Poirier

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
Welcome to our settings persistence mess.
------------------------------------------------------------------------*/

#include "dfxsettings.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <vector>

#include "dfxmath.h"
#include "dfxmidi.h"
#include "dfxmisc.h"
#include "dfxplugin.h"


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark -
#pragma mark init / destroy
#pragma mark -
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//-----------------------------------------------------------------------------
DfxSettings::DfxSettings(uint32_t inMagic, DfxPlugin& inPlugin, size_t inSizeofExtendedData)
:	mPlugin(inPlugin),
	// TODO C++23: integer literal suffix UZ
	mNumParameters(std::max(inPlugin.getnumparameters(), size_t(1))),	// we need at least one parameter
	mNumPresets(std::max(inPlugin.getnumpresets(), size_t(1))),	// we need at least one set of parameters
	mSizeOfExtendedData(inSizeofExtendedData),
	mParameterIDMap(mNumParameters, dfx::kParameterID_Invalid),
	mParameterAssignments(mNumParameters)
{
	// default to each parameter having its ID equal its index
	std::iota(mParameterIDMap.begin(), mParameterIDMap.end(), 0);

	// calculate some data sizes that are useful to know
	mSizeOfPreset = sizeOfGenPreset(mNumParameters);
	mSizeOfParameterIDs = sizeof(mParameterIDMap.front()) * mNumParameters;
	mSizeOfPresetChunk = mSizeOfPreset 				// 1 preset
						 + sizeof(SettingsInfo) 	// the special data header info
						 + mSizeOfParameterIDs		// the table of parameter IDs
						 + (sizeof(dfx::ParameterAssignment) * mNumParameters)  // the MIDI events assignment array
						 + inSizeofExtendedData;
	mSizeOfChunk = (mSizeOfPreset * mNumPresets)	// all of the presets
					+ sizeof(SettingsInfo)			// the special data header info
					+ mSizeOfParameterIDs			// the table of parameter IDs
					+ (sizeof(dfx::ParameterAssignment) * mNumParameters)  // the MIDI events assignment array
					+ inSizeofExtendedData;

	// set all of the header infos
	mSettingsInfo.mMagic = inMagic;
	mSettingsInfo.mVersion = mPlugin.getpluginversion();
	mSettingsInfo.mLowestLoadableVersion = 0;
	mSettingsInfo.mStoredHeaderSize = sizeof(SettingsInfo);
	mSettingsInfo.mNumStoredParameters = static_cast<uint32_t>(mNumParameters);
	mSettingsInfo.mNumStoredPresets = static_cast<uint32_t>(mNumPresets);
	mSettingsInfo.mStoredParameterAssignmentSize = sizeof(dfx::ParameterAssignment);
	mSettingsInfo.mStoredExtendedDataSize = static_cast<uint32_t>(inSizeofExtendedData);

	clearAssignments();  // initialize all of the parameters to have no MIDI event assignments

	// allow for further constructor stuff, if necessary
	mPlugin.settings_init();
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark -
#pragma mark settings
#pragma mark -
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//-----------------------------------------------------------------------------
template <typename FromT, typename ToT>
requires (!std::is_const_v<ToT>)
using CopyConstT = std::conditional_t<std::is_const_v<FromT>, std::add_const_t<ToT>, ToT>;

//-----------------------------------------------------------------------------
template <typename T>
T* OffsetAddress(T* inAddress, ptrdiff_t inOffset) noexcept
{
	return reinterpret_cast<T*>(reinterpret_cast<CopyConstT<T, std::byte>*>(inAddress) + inOffset);
}

//-----------------------------------------------------------------------------
template <typename T>
T* OffsetAddressToType(CopyConstT<T, void>* inAddress, ptrdiff_t inOffset) noexcept
{
	return static_cast<T*>(OffsetAddress(inAddress, inOffset));
}

//-----------------------------------------------------------------------------
template <dfx::TriviallySerializable T>
class SerializedObject
{
public:
	using difference_type = std::ptrdiff_t;
	using value_type = T;
	using reference = T&;
	using iterator_category = std::input_iterator_tag;

	template <typename ToT>
	using CopyConstT = CopyConstT<T, ToT>;

	explicit SerializedObject(CopyConstT<void>* inData) noexcept
	:	mData(static_cast<T*>(inData))
	{
	}

	auto operator*() const noexcept
	requires (!std::is_const_v<T>)
	{
		return AssignmentWrapper(mData);
	}
	auto operator*() const noexcept
	requires std::is_const_v<T>
	{
		return dfx::Enliven(mData);
	}

	auto operator[](std::size_t inIndex) const noexcept
	requires (!std::is_const_v<T>)
	{
		return AssignmentWrapper(mData + inIndex);
	}
	[[nodiscard]] T operator[](std::size_t inIndex) const noexcept
	requires std::is_const_v<T>
	{
		return dfx::Enliven(mData + inIndex);
	}

	SerializedObject begin() const noexcept
	{
		return *this;
	}
	SerializedObject& operator++() noexcept
	{
		this->offsetAddress(sizeof(T));
		return *this;
	}
	SerializedObject operator++(int) noexcept
	{
		auto const entrySelf = *this;
		operator++();
		return entrySelf;
	}

	auto getByteAddress() const noexcept
	{
		return reinterpret_cast<CopyConstT<std::byte>*>(mData);
	}

	void offsetAddress(ptrdiff_t inOffset) noexcept
	{
		mData = OffsetAddress(mData, inOffset);
	}

	static constexpr auto sizeOfType() noexcept
	{
		return sizeof(T);
	}

private:
	class AssignmentWrapper
	{
	public:
		explicit AssignmentWrapper(T* inData) noexcept
		requires (!std::is_const_v<T>)
		:	mData(inData)
		{
		}
		AssignmentWrapper& operator=(T const& inOther) noexcept
		{
			dfx::MemCpyObject(inOther, mData);
			return *this;
		}
	private:
		T* const mData;
	};

	T* mData;
};

#define GET_SERIALIZED_HEADER_FIELD(inData, inMemberName) \
	dfx::Enliven(OffsetAddressToType<decltype(SettingsInfo::inMemberName)>(inData, offsetof(SettingsInfo, inMemberName)))

//-----------------------------------------------------------------------------
// this gets called when the host wants to save settings data, 
// like when saving a session document or preset files
std::vector<std::byte> DfxSettings::save(bool inIsPreset) const
{
	std::vector<std::byte> data(inIsPreset ? mSizeOfPresetChunk : mSizeOfChunk, std::byte{0});
	SerializedObject<SettingsInfo> const sharedHeader(data.data());

	// and a few references to elements within that data, just for ease of use
	SerializedObject<uint32_t> const sharedParameterIDs(data.data() + sizeof(SettingsInfo));
	auto const firstSharedPresetByteAddress = sharedParameterIDs.getByteAddress() + mSizeOfParameterIDs;
	auto sharedPresetName = reinterpret_cast<GenPresetNameElementT*>(firstSharedPresetByteAddress + offsetof(GenPreset, mName));
	SerializedObject<GenPresetParameterValueT> sharedPresetParameterValues(firstSharedPresetByteAddress + offsetof(GenPreset, mParameterValues));
	// TODO C++23: integer literal suffix UZ
	auto const savePresetCount = inIsPreset ? size_t(1) : mNumPresets;
	SerializedObject<dfx::ParameterAssignment> const sharedParameterAssignments(firstSharedPresetByteAddress + (mSizeOfPreset * savePresetCount));

	// first store the special chunk infos
	auto settingsInfoWithAdjustedPresetCount = mSettingsInfo;
	if (inIsPreset)
	{
		settingsInfoWithAdjustedPresetCount.mNumStoredPresets = 1;
	}
	*sharedHeader = settingsInfoWithAdjustedPresetCount;

	// store the parameters' IDs
	std::copy(mParameterIDMap.cbegin(), mParameterIDMap.cend(), sharedParameterIDs.begin());

	// store only one preset setting if inIsPreset is true
	if (inIsPreset)
	{
		dfx::StrLCpy(sharedPresetName, mPlugin.getpresetname(mPlugin.getcurrentpresetnum()), std::extent_v<GenPresetNameT>);
		for (dfx::ParameterID i = 0; i < mNumParameters; i++)
		{
			sharedPresetParameterValues[i] = mPlugin.getparameter_f(i);
		}
	}
	// otherwise store the entire bank of presets and the MIDI event assignments
	else
	{
		for (size_t j = 0; j < mNumPresets; j++)
		{
			// copy the preset name to the chunk
			dfx::StrLCpy(sharedPresetName, mPlugin.getpresetname(j), std::extent_v<GenPresetNameT>);
			// copy all of the parameters for this preset to the chunk
			for (dfx::ParameterID i = 0; i < mNumParameters; i++)
			{
				sharedPresetParameterValues[i] = mPlugin.getpresetparameter_f(j, i);
			}
			// point to the next preset in the data array for the host
			sharedPresetName = OffsetAddress(sharedPresetName, mSizeOfPreset);
			sharedPresetParameterValues.offsetAddress(mSizeOfPreset);
		}
	}

	// store the parameters' MIDI event assignments
	std::copy(mParameterAssignments.cbegin(), mParameterAssignments.cend(), sharedParameterAssignments.begin());

	// reverse the order of bytes in the data being sent to the host, if necessary
	[[maybe_unused]] auto const endianSuccess = correctEndian(data.data(), data.size() - mSizeOfExtendedData, false, inIsPreset);
	assert(endianSuccess);

	// allow for the storage of extra data
	if (mSizeOfExtendedData > 0)
	{
		mPlugin.settings_saveExtendedData(data.data() + data.size() - mSizeOfExtendedData, inIsPreset);
	}

	return data;
}


#ifdef DFX_SUPPORT_OLD_VST_SETTINGS
//-----------------------------------------------------------------------------
// for backwerds compaxibilitee
inline bool DFX_IsOldVstVersionNumber(uint32_t inVersion) noexcept
{
	return (inVersion < 0x00010000);
}

constexpr size_t kDfxOldPresetNameMaxLength = 32;
#endif

//-----------------------------------------------------------------------------
// this gets called when the host wants to load settings data, 
// like when restoring settings while opening a song, 
// or loading a preset file
bool DfxSettings::restore(void const* inData, size_t const inDataSize, bool inIsPreset)
try
{
	static_assert(std::numeric_limits<float>::is_iec559, "floating point values cannot be deserialized if the representation is inconsistent");

	auto const require = [](bool condition)
	{
		if (!condition)
		{
			throw std::invalid_argument("bad data");
		}
	};

	constexpr auto kMinimumHeaderSize = offsetof(SettingsInfo, mGlobalBehaviorFlags);
	require(inDataSize >= kMinimumHeaderSize);

	// create our own copy of the data before we muck with it (e.g. reversing endianness, etc.)
	auto const incomingData_copy = dfx::MakeUniqueMemoryBlock<void>(inDataSize);
	require(incomingData_copy.get());
	std::memcpy(incomingData_copy.get(), inData, inDataSize);

	// un-reverse the order of bytes in the received data, if necessary
	require(correctEndian(incomingData_copy.get(), inDataSize, true, inIsPreset));

	auto const validateRange = [&incomingData_copy, inDataSize, this](void const* address, size_t length, char const* name)
	{
		this->validateRange(incomingData_copy.get(), inDataSize, address, length, name);
	};

	// the start of the chunk data:  the SettingsInfo header
	auto const storedHeaderSize = GET_SERIALIZED_HEADER_FIELD(incomingData_copy.get(), mStoredHeaderSize);
	validateRange(incomingData_copy.get(), storedHeaderSize, "header");
	require(storedHeaderSize >= kMinimumHeaderSize);
	auto const storedMagic = GET_SERIALIZED_HEADER_FIELD(incomingData_copy.get(), mMagic);
	auto const storedVersion = GET_SERIALIZED_HEADER_FIELD(incomingData_copy.get(), mVersion);
	auto const storedLowestLoadableVersion = GET_SERIALIZED_HEADER_FIELD(incomingData_copy.get(), mLowestLoadableVersion);
	auto const numStoredParameters = GET_SERIALIZED_HEADER_FIELD(incomingData_copy.get(), mNumStoredParameters);
	auto const numStoredPresets = GET_SERIALIZED_HEADER_FIELD(incomingData_copy.get(), mNumStoredPresets);
	auto const storedParameterAssignmentSize = GET_SERIALIZED_HEADER_FIELD(incomingData_copy.get(), mStoredParameterAssignmentSize);
	auto const storedExtendedDataSize = GET_SERIALIZED_HEADER_FIELD(incomingData_copy.get(), mStoredExtendedDataSize);
	std::optional<decltype(SettingsInfo::mGlobalBehaviorFlags)> storedGlobalBehaviorFlags;
	// check for availability of later extensions to the header
	if (storedHeaderSize >= (offsetof(SettingsInfo, mGlobalBehaviorFlags) + sizeof(SettingsInfo::mGlobalBehaviorFlags)))
	{
		storedGlobalBehaviorFlags = GET_SERIALIZED_HEADER_FIELD(incomingData_copy.get(), mGlobalBehaviorFlags);
	}

	// The following situations are basically considered to be 
	// irrecoverable "crisis" situations.  Regardless of what 
	// crisis behavior has been chosen, any of the following errors 
	// will prompt an unsuccessful exit because these are big problems.  
	// Incorrect magic signature basically means that these settings are 
	// probably for some other plugin.  And the whole point of setting a 
	// lowest loadable version value is that it should be taken seriously.
	require(storedMagic == mSettingsInfo.mMagic);
	require((storedVersion >= mSettingsInfo.mLowestLoadableVersion) &&
			(mSettingsInfo.mVersion >= storedLowestLoadableVersion));  // TODO: does this second test make sense?

#ifdef DFX_SUPPORT_OLD_VST_SETTINGS
	// we started using hex format versions (like below) with the advent 
	// of the glorious DfxPlugin
	// versions lower than 0x00010000 indicate inferior settings
	auto const oldVST = DFX_IsOldVstVersionNumber(storedVersion);
#endif

	// figure out how many presets we should try to load 
	// if the incoming chunk doesn't match what we're expecting
	// TODO C++23: integer literal suffix UZ
	auto const copyPresets = std::min(static_cast<size_t>(numStoredPresets), inIsPreset ? size_t(1) : mNumPresets);
	// figure out how much of the dfx::ParameterAssignment structure we can import
	auto const copyParameterAssignmentSize = std::min(storedParameterAssignmentSize, mSettingsInfo.mStoredParameterAssignmentSize);

	// check for conflicts and keep track of them
	CrisisReasonFlags crisisFlags = kCrisisReasonFlag_None;
	if (storedVersion < mSettingsInfo.mVersion)
	{
		crisisFlags = crisisFlags | kCrisisReasonFlag_LowerVersion;
	}
	else if (storedVersion > mSettingsInfo.mVersion)
	{
		crisisFlags = crisisFlags | kCrisisReasonFlag_HigherVersion;
	}
	if (numStoredParameters < mNumParameters)
	{
		crisisFlags = crisisFlags | kCrisisReasonFlag_FewerParameters;
	}
	else if (numStoredParameters > mNumParameters)
	{
		crisisFlags = crisisFlags | kCrisisReasonFlag_MoreParameters;
	}
	if (inIsPreset)
	{
		if (inDataSize < mSizeOfPresetChunk)
		{
			crisisFlags = crisisFlags | kCrisisReasonFlag_SmallerByteSize;
		}
		else if (inDataSize > mSizeOfPresetChunk)
		{
			crisisFlags = crisisFlags | kCrisisReasonFlag_LargerByteSize;
		}
	}
	else
	{
		if (inDataSize < mSizeOfChunk)
		{
			crisisFlags = crisisFlags | kCrisisReasonFlag_SmallerByteSize;
		}
		else if (inDataSize > mSizeOfChunk)
		{
			crisisFlags = crisisFlags | kCrisisReasonFlag_LargerByteSize;
		}
		if (numStoredPresets < mNumPresets)
		{
			crisisFlags = crisisFlags | kCrisisReasonFlag_FewerPresets;
		}
		else if (numStoredPresets > mNumPresets)
		{
			crisisFlags = crisisFlags | kCrisisReasonFlag_MorePresets;
		}
	}
	// handle the crisis situations (if any) and stop trying to load if we're told to
	require(handleCrisis(crisisFlags) != CrisisError::QuitError);

	if (storedGlobalBehaviorFlags)
	{
		setUseChannel(*storedGlobalBehaviorFlags & kGlobalBehaviorFlag_UseChannel);
		setSteal(*storedGlobalBehaviorFlags & kGlobalBehaviorFlag_StealAssignments);
	}

	// point to the next data element after the chunk header:  the first parameter ID
	SerializedObject<uint32_t const> const newParameterIDs(OffsetAddress(incomingData_copy.get(), storedHeaderSize));
	size_t const sizeOfStoredParameterIDs = newParameterIDs.sizeOfType() * numStoredParameters;
	validateRange(newParameterIDs.getByteAddress(), sizeOfStoredParameterIDs, "parameter IDs");
	// XXX plain contiguous data container copy required for passing to getParameterIndexFromMap
	std::vector<uint32_t> newParameterIDs_copy(numStoredParameters, dfx::kParameterID_Invalid);
	std::copy_n(newParameterIDs.begin(), numStoredParameters, newParameterIDs_copy.begin());
	// create a mapping table for corresponding the incoming parameters to the 
	// destination parameters (in case the parameter IDs don't all match up)
	//  [ the index of parameterMap is the same as our parameter tag/index and the value 
	//    is the tag/index of the incoming parameter that corresponds, if any ]
	std::vector<dfx::ParameterID> parameterMap(mNumParameters, dfx::kParameterID_Invalid);
	for (size_t i = 0; i < mParameterIDMap.size(); i++)
	{
		parameterMap[i] = getParameterIndexFromMap(mParameterIDMap[i], newParameterIDs_copy);
	}

	// point to the next data element after the parameter IDs:  the first preset name
	auto const firstNewPresetByteAddress = newParameterIDs.getByteAddress() + sizeOfStoredParameterIDs;
	auto newPresetName = reinterpret_cast<GenPresetNameElementT const*>(firstNewPresetByteAddress + offsetof(GenPreset, mName));
	SerializedObject<GenPresetParameterValueT const> newPresetParameterValues(firstNewPresetByteAddress + offsetof(GenPreset, mParameterValues));
#ifdef DFX_SUPPORT_OLD_VST_SETTINGS
	if (oldVST)
	{
		newPresetParameterValues.offsetAddress(dfx::math::ToSigned(kDfxOldPresetNameMaxLength) - dfx::math::ToSigned(dfx::kPresetNameMaxLength));
	}
	size_t const sizeOfStoredPreset = sizeOfGenPreset(numStoredParameters) + (oldVST ? kDfxOldPresetNameMaxLength : 0) - (oldVST ? dfx::kPresetNameMaxLength : 0);
#else
	auto const sizeOfStoredPreset = sizeOfGenPreset(numStoredParameters);
#endif
	validateRange(firstNewPresetByteAddress, sizeOfStoredPreset * numStoredPresets, "presets");

	auto const getPresetNameWithFallback = [](std::string_view presetName) -> std::string_view
	{
		return presetName.empty() ? "(unnamed)" : presetName;
	};

	// the chunk being received only contains one preset
	if (inIsPreset)
	{
		// in Audio Unit, this is handled already in AUBase::RestoreState, 
		// and we are not really loading a "preset,"
		// we are restoring the last user state
		#ifndef TARGET_API_AUDIOUNIT
		// copy the preset name from the chunk
		mPlugin.setpresetname(mPlugin.getcurrentpresetnum(), getPresetNameWithFallback(newPresetName));
		#endif
		// copy all of the parameters that we can for this preset from the chunk
		for (dfx::ParameterID i = 0; i < parameterMap.size(); i++)
		{
			auto const mappedParameterID = parameterMap[i];
			if ((mappedParameterID != dfx::kParameterID_Invalid) && (mappedParameterID < numStoredParameters))
			{
			#ifdef DFX_SUPPORT_OLD_VST_SETTINGS
				// handle old-style generic VST 0-to-1 normalized parameter values
				if (oldVST)
				{
					mPlugin.setparameter_gen(i, newPresetParameterValues[mappedParameterID]);
				}
				else
			#endif
				{
					mPlugin.setparameter_f(i, newPresetParameterValues[mappedParameterID]);
				}
				// allow for additional tweaking of the stored parameter setting
				mPlugin.settings_doChunkRestoreSetParameterStuff(i, newPresetParameterValues[mappedParameterID], storedVersion, {});
			}
		}
		// point past the preset
		newPresetName = OffsetAddress(newPresetName, sizeOfStoredPreset);
		newPresetParameterValues.offsetAddress(sizeOfStoredPreset);
	}
	// the chunk being received has all of the presets plus the MIDI event assignments
	else
	{
		// we're loading an entire bank of presets plus the MIDI event assignments, 
		// so cycle through all of the presets and load them up, as many as we can
		for (size_t j = 0; j < copyPresets; j++)
		{
			// copy the preset name from the chunk
			mPlugin.setpresetname(j, getPresetNameWithFallback(newPresetName));
			// copy all of the parameters that we can for this preset from the chunk
			for (dfx::ParameterID i = 0; i < parameterMap.size(); i++)
			{
				auto const mappedParameterID = parameterMap[i];
				if ((mappedParameterID != dfx::kParameterID_Invalid) && (mappedParameterID < numStoredParameters))
				{
				#ifdef DFX_SUPPORT_OLD_VST_SETTINGS
					// handle old-style generic VST 0-to-1 normalized parameter values
					if (oldVST)
					{
						mPlugin.setpresetparameter_gen(j, i, newPresetParameterValues[mappedParameterID]);
					}
					else
				#endif
					{
						mPlugin.setpresetparameter_f(j, i, newPresetParameterValues[mappedParameterID]);
					}
					// allow for additional tweaking of the stored parameter setting
					mPlugin.settings_doChunkRestoreSetParameterStuff(i, newPresetParameterValues[mappedParameterID], storedVersion, j);
				}
			}
			// point to the next preset in the received data array
			newPresetName = OffsetAddress(newPresetName, sizeOfStoredPreset);
			newPresetParameterValues.offsetAddress(sizeOfStoredPreset);
		}
	}

	// point to the last chunk data element, the MIDI event assignment array
	// (offset by the number of stored presets that were skipped, if any)
	auto const newParameterAssignments = firstNewPresetByteAddress + (numStoredPresets * sizeOfStoredPreset);
	size_t sizeOfStoredParameterAssignments = 0;  // until we establish that they are present
#ifdef DFX_SUPPORT_OLD_VST_SETTINGS
	if (!(oldVST && inIsPreset))
#endif
	{
		// completely clear our table of parameter assignments before loading the new
		// table since the new one might not have all of the data members
		clearAssignments();
		sizeOfStoredParameterAssignments = storedParameterAssignmentSize * numStoredParameters;
		validateRange(newParameterAssignments, sizeOfStoredParameterAssignments, "parameter assignments");
		// and load up as many of them as we can
		for (size_t i = 0; i < std::min(parameterMap.size(), mParameterAssignments.size()); i++)
		{
			auto const mappedParameterID = parameterMap[i];
			if ((mappedParameterID != dfx::kParameterID_Invalid) && (mappedParameterID < numStoredParameters))
			{
				mParameterAssignments[i] = {};
				std::memcpy(&(mParameterAssignments[i]),
							newParameterAssignments + (mappedParameterID * storedParameterAssignmentSize),
							copyParameterAssignmentSize);
			}
		}
	}

	// allow for the retrieval of extra data
	if (storedExtendedDataSize > 0)
	{
		auto const newExtendedData = newParameterAssignments + sizeOfStoredParameterAssignments;
		validateRange(newExtendedData, storedExtendedDataSize, "extended data");
		mPlugin.settings_restoreExtendedData(newExtendedData, storedExtendedDataSize, storedVersion, inIsPreset);
	}

	return true;
}
catch (...)
{
	return false;
}

//-----------------------------------------------------------------------------
bool DfxSettings::minimalValidate(void const* inData, size_t inDataSize) const noexcept
{
	SettingsInfo settingsInfo;
	if (!inData || (inDataSize < sizeof(settingsInfo)))
	{
		return false;
	}
	std::memcpy(&settingsInfo, inData, sizeof(settingsInfo));

	if constexpr (!serializationIsNativeEndian())
	{
		dfx::ReverseBytes(settingsInfo.mMagic);
		dfx::ReverseBytes(settingsInfo.mVersion);
		dfx::ReverseBytes(settingsInfo.mStoredHeaderSize);
	}

	return (settingsInfo.mMagic == mSettingsInfo.mMagic) && (settingsInfo.mVersion == mSettingsInfo.mVersion) && (settingsInfo.mStoredHeaderSize == mSettingsInfo.mStoredHeaderSize);
}

//-----------------------------------------------------------------------------
// this function, if called for the non-reference endian architecture, 
// will reverse the order of bytes in each variable/value of the data 
// to correct endian differences and make a uniform data chunk
bool DfxSettings::correctEndian(void* const ioData, size_t const inDataSize, bool inIsReversed, bool inIsPreset) const
try
{
/*
// XXX another idea...
void blah(long long x)
{
	int n = sizeof(x);
	while (n--)
	{
		write(f, x & 0xFF, 1);
		x >>= 8;
	}
}
*/
	if constexpr (serializationIsNativeEndian())
	{
		return true;
	}

	// start by looking at the header info
	auto const dataHeaderAddress = ioData;
	// we need to know how big the header is before dealing with it
	auto storedVersion = GET_SERIALIZED_HEADER_FIELD(dataHeaderAddress, mVersion);
	auto storedHeaderSize = GET_SERIALIZED_HEADER_FIELD(dataHeaderAddress, mStoredHeaderSize);
	auto numStoredParameters = GET_SERIALIZED_HEADER_FIELD(dataHeaderAddress, mNumStoredParameters);
	auto numStoredPresets = GET_SERIALIZED_HEADER_FIELD(dataHeaderAddress, mNumStoredPresets);
	auto storedParameterAssignmentSize = GET_SERIALIZED_HEADER_FIELD(dataHeaderAddress, mStoredParameterAssignmentSize);
	// correct the values' endian byte order order if the data was received byte-swapped
	if (inIsReversed)
	{
		dfx::ReverseBytes(storedVersion);
		dfx::ReverseBytes(storedHeaderSize);
		dfx::ReverseBytes(numStoredParameters);
		dfx::ReverseBytes(numStoredPresets);
		dfx::ReverseBytes(storedParameterAssignmentSize);
	}

	auto const validateRange = [ioData, inDataSize, this](void const* address, size_t length, char const* name)
	{
		this->validateRange(ioData, inDataSize, address, length, name);
	};

	// reverse the order of bytes of the header values
	validateRange(dataHeaderAddress, storedHeaderSize, "header");
	constexpr auto headerItemSize = sizeof(SettingsInfo::mMagic);
	if ((storedHeaderSize % headerItemSize) != 0)
	{
		debugAlertCorruptData("header divisibility", storedHeaderSize, inDataSize);
		return false;
	}
	dfx::ReverseBytes(dataHeaderAddress, headerItemSize, storedHeaderSize / headerItemSize);

	// reverse the byte order for each of the parameter IDs
	auto const dataParameterIDs = OffsetAddressToType<uint32_t>(dataHeaderAddress, storedHeaderSize);
	validateRange(dataParameterIDs, sizeof(*dataParameterIDs) * numStoredParameters, "parameter IDs");
	dfx::ReverseBytes(dataParameterIDs, numStoredParameters);

	// reverse the order of bytes for each parameter value, 
	// but no need to mess with the preset names since they are char arrays
	auto dataPresets = OffsetAddressToType<GenPreset>(dataParameterIDs, sizeof(*dataParameterIDs) * numStoredParameters);
	auto sizeOfStoredPreset = sizeOfGenPreset(numStoredParameters);
#ifdef DFX_SUPPORT_OLD_VST_SETTINGS
	if (DFX_IsOldVstVersionNumber(storedVersion))
	{
		// back up the pointer to account for shorter preset names
		dataPresets = OffsetAddress(dataPresets, dfx::math::ToSigned(kDfxOldPresetNameMaxLength) - dfx::math::ToSigned(dfx::kPresetNameMaxLength));
		// and shrink the size to account for shorter preset names
		if (sizeOfStoredPreset < dfx::kPresetNameMaxLength)
		{
			debugAlertCorruptData("old VST presets", sizeOfStoredPreset, inDataSize);
			return false;
		}
		sizeOfStoredPreset += kDfxOldPresetNameMaxLength;
		sizeOfStoredPreset -= dfx::kPresetNameMaxLength;
	}
#endif
	validateRange(dataPresets, sizeOfStoredPreset * numStoredPresets, "presets");
	for (uint32_t i = 0; i < numStoredPresets; i++)
	{
		dfx::ReverseBytes(dataPresets->mParameterValues, numStoredParameters);
		// point to the next preset in the data array
		dataPresets = OffsetAddress(dataPresets, sizeOfStoredPreset);
	}
#ifdef DFX_SUPPORT_OLD_VST_SETTINGS
	if (DFX_IsOldVstVersionNumber(storedVersion))
	{
		// advance the pointer to compensate for backing up earlier
		dataPresets = OffsetAddress(dataPresets, dfx::math::ToSigned(dfx::kPresetNameMaxLength) - dfx::math::ToSigned(kDfxOldPresetNameMaxLength));
	}
#endif

#ifdef DFX_SUPPORT_OLD_VST_SETTINGS
	if (!(DFX_IsOldVstVersionNumber(storedVersion) && inIsPreset))
#endif
	{
		// and reverse the byte order of each event assignment
		auto dataParameterAssignment = reinterpret_cast<dfx::ParameterAssignment*>(dataPresets);
		validateRange(dataParameterAssignment, storedParameterAssignmentSize * numStoredParameters, "parameter assignments");
		auto const copyParameterAssignmentSize = std::min(storedParameterAssignmentSize, mSettingsInfo.mStoredParameterAssignmentSize);
		for (uint32_t i = 0; i < numStoredParameters; ++i)
		{
			dfx::ParameterAssignment pa;
			std::memcpy(&pa, dataParameterAssignment, copyParameterAssignmentSize);
			dfx::ReverseBytes(pa.mEventType);
			dfx::ReverseBytes(pa.mEventChannel);
			dfx::ReverseBytes(pa.mEventNum);
			dfx::ReverseBytes(pa.mEventNum2);
			dfx::ReverseBytes(pa.mEventBehaviorFlags);
			dfx::ReverseBytes(pa.mDataInt1);
			dfx::ReverseBytes(pa.mDataInt2);
			dfx::ReverseBytes(pa.mDataFloat1);
			dfx::ReverseBytes(pa.mDataFloat2);
			std::memcpy(dataParameterAssignment, &pa, copyParameterAssignmentSize);
			dataParameterAssignment = OffsetAddress(dataParameterAssignment, storedParameterAssignmentSize);
		}
	}

	return true;
}
catch (...)
{
	return false;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark -
#pragma mark Audio Unit -specific stuff
#pragma mark -
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#ifdef TARGET_API_AUDIOUNIT
static CFStringRef const kDfxSettings_ParameterIDKey = CFSTR("parameter ID");
static CFStringRef const kDfxSettings_MidiAssignmentsKey = CFSTR("DFX! MIDI assignments");
static CFStringRef const kDfxSettings_MidiAssignment_mEventTypeKey = CFSTR("event type");
static CFStringRef const kDfxSettings_MidiAssignment_mEventChannelKey = CFSTR("event channel");
static CFStringRef const kDfxSettings_MidiAssignment_mEventNumKey = CFSTR("event number");
static CFStringRef const kDfxSettings_MidiAssignment_mEventNum2Key = CFSTR("event number 2");
static CFStringRef const kDfxSettings_MidiAssignment_mEventBehaviorFlagsKey = CFSTR("event behavior flags");
static CFStringRef const kDfxSettings_MidiAssignment_mDataInt1Key = CFSTR("integer data 1");
static CFStringRef const kDfxSettings_MidiAssignment_mDataInt2Key = CFSTR("integer data 2");
static CFStringRef const kDfxSettings_MidiAssignment_mDataFloat1Key = CFSTR("float data 1");
static CFStringRef const kDfxSettings_MidiAssignment_mDataFloat2Key = CFSTR("float data 2");

namespace
{

//-----------------------------------------------------------------------------------------
bool DFX_AddNumberToCFDictionary(void const* inNumber, CFNumberType inType, CFMutableDictionaryRef inDictionary, void const* inDictionaryKey)
{
	if (!inNumber || !inDictionary || !inDictionaryKey)
	{
		return false;
	}

	auto const cfNumber = dfx::MakeUniqueCFType(CFNumberCreate(kCFAllocatorDefault, inType, inNumber));
	if (cfNumber)
	{
		CFDictionarySetValue(inDictionary, inDictionaryKey, cfNumber.get());
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------------------
bool DFX_AddNumberToCFDictionary_i(SInt64 inNumber, CFMutableDictionaryRef inDictionary, void const* inDictionaryKey)
{
	return DFX_AddNumberToCFDictionary(&inNumber, kCFNumberSInt64Type, inDictionary, inDictionaryKey);
}

//-----------------------------------------------------------------------------------------
bool DFX_AddNumberToCFDictionary_f(Float64 inNumber, CFMutableDictionaryRef inDictionary, void const* inDictionaryKey)
{
	return DFX_AddNumberToCFDictionary(&inNumber, kCFNumberFloat64Type, inDictionary, inDictionaryKey);
}

}  // namespace

//-----------------------------------------------------------------------------------------
std::optional<SInt64> DFX_GetNumberFromCFDictionary_i(CFDictionaryRef inDictionary, void const* inDictionaryKey)
{
	constexpr CFNumberType numberType = kCFNumberSInt64Type;

	if (!inDictionary || !inDictionaryKey)
	{
		return {};
	}

	auto const cfNumber = static_cast<CFNumberRef>(CFDictionaryGetValue(inDictionary, inDictionaryKey));
	if (cfNumber && (CFGetTypeID(cfNumber) == CFNumberGetTypeID()))
	{
		if (CFNumberGetType(cfNumber) == numberType)
		{
			SInt64 resultNumber {};
			if (CFNumberGetValue(cfNumber, numberType, &resultNumber))
			{
				return resultNumber;
			}
		}
	}

	return {};
}

//-----------------------------------------------------------------------------------------
std::optional<Float64> DFX_GetNumberFromCFDictionary_f(CFDictionaryRef inDictionary, void const* inDictionaryKey)
{
	constexpr CFNumberType numberType = kCFNumberFloat64Type;

	if (!inDictionary || !inDictionaryKey)
	{
		return {};
	}

	auto const cfNumber = static_cast<CFNumberRef>(CFDictionaryGetValue(inDictionary, inDictionaryKey));
	if (cfNumber && (CFGetTypeID(cfNumber) == CFNumberGetTypeID()))
	{
		if (CFNumberGetType(cfNumber) == numberType)
		{
			Float64 resultNumber {};
			if (CFNumberGetValue(cfNumber, numberType, &resultNumber))
			{
				return resultNumber;
			}
		}
	}

	return {};
}

//-----------------------------------------------------------------------------------------
bool DfxSettings::saveMidiAssignmentsToDictionary(CFMutableDictionaryRef inDictionary) const
{
	if (!inDictionary)
	{
		return false;
	}

	size_t assignmentsFoundCount = 0;
	for (dfx::ParameterID i = 0; i < mNumParameters; i++)
	{
		if (getParameterAssignmentType(i) != dfx::MidiEventType::None)
		{
			assignmentsFoundCount++;
		}
	}
	// nothing to do
	if (assignmentsFoundCount == 0)
	{
		return true;
	}

	auto const assignmentsCFArray = dfx::MakeUniqueCFType(CFArrayCreateMutable(kCFAllocatorDefault, mNumParameters, &kCFTypeArrayCallBacks));
	if (!assignmentsCFArray)
	{
		return false;
	}

	for (dfx::ParameterID i = 0; i < mNumParameters; i++)
	{
		auto const assignmentCFDictionary = dfx::MakeUniqueCFType(CFDictionaryCreateMutable(kCFAllocatorDefault, 10, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
		if (assignmentCFDictionary)
		{
			if (getMappedParameterID(i) == dfx::kParameterID_Invalid)
			{
				continue;
			}
			DFX_AddNumberToCFDictionary_i(getMappedParameterID(i), assignmentCFDictionary.get(), kDfxSettings_ParameterIDKey);
			DFX_AddNumberToCFDictionary_i(static_cast<SInt64>(mParameterAssignments[i].mEventType), 
										  assignmentCFDictionary.get(), kDfxSettings_MidiAssignment_mEventTypeKey);
#define ADD_ASSIGNMENT_VALUE_TO_DICT(inMember, inTypeSuffix)	\
			DFX_AddNumberToCFDictionary_##inTypeSuffix(mParameterAssignments[i].inMember, assignmentCFDictionary.get(), kDfxSettings_MidiAssignment_##inMember##Key);
			ADD_ASSIGNMENT_VALUE_TO_DICT(mEventChannel, i)
			ADD_ASSIGNMENT_VALUE_TO_DICT(mEventNum, i)
			ADD_ASSIGNMENT_VALUE_TO_DICT(mEventNum2, i)
			ADD_ASSIGNMENT_VALUE_TO_DICT(mEventBehaviorFlags, i)
			ADD_ASSIGNMENT_VALUE_TO_DICT(mDataInt1, i)
			ADD_ASSIGNMENT_VALUE_TO_DICT(mDataInt2, i)
			ADD_ASSIGNMENT_VALUE_TO_DICT(mDataFloat1, f)
			ADD_ASSIGNMENT_VALUE_TO_DICT(mDataFloat2, f)
#undef ADD_ASSIGNMENT_VALUE_TO_DICT
			CFArraySetValueAtIndex(assignmentsCFArray.get(), i, assignmentCFDictionary.get());
		}
	}
	auto const arraySize = CFArrayGetCount(assignmentsCFArray.get());
	if (arraySize > 0)
	{
		CFDictionarySetValue(inDictionary, kDfxSettings_MidiAssignmentsKey, assignmentsCFArray.get());
	}

	return (dfx::math::ToUnsigned(arraySize) == assignmentsFoundCount);
}

//-----------------------------------------------------------------------------------------
bool DfxSettings::restoreMidiAssignmentsFromDictionary(CFDictionaryRef inDictionary)
{
	if (!inDictionary)
	{
		return false;
	}

	auto const assignmentsCFArray = static_cast<CFArrayRef>(CFDictionaryGetValue(inDictionary, kDfxSettings_MidiAssignmentsKey));
	// nothing to do
	if (!assignmentsCFArray)
	{
		return true;
	}
	if (CFGetTypeID(assignmentsCFArray) != CFArrayGetTypeID())
	{
		return false;
	}

	// completely clear our table of parameter assignments before loading the new 
	// table since the new one might not have all of the data members
	clearAssignments();

	auto const arraySize = CFArrayGetCount(assignmentsCFArray);
	for (CFIndex i = 0; i < arraySize; i++)
	{
		auto const assignmentCFDictionary = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(assignmentsCFArray, i));
		if (assignmentCFDictionary)
		{
			auto const parameterID_optional = DFX_GetNumberFromCFDictionary_i(assignmentCFDictionary, kDfxSettings_ParameterIDKey);
			if (!parameterID_optional || (*parameterID_optional < 0))
			{
				continue;
			}
			auto const parameterID = getParameterIndexFromMap(static_cast<dfx::ParameterID>(*parameterID_optional));
			if (!isValidParameterID(parameterID))
			{
				continue;
			}
#define GET_ASSIGNMENT_VALUE_FROM_DICT(inMember, inTypeSuffix)	\
			{	\
				auto const optionalValue = DFX_GetNumberFromCFDictionary_##inTypeSuffix(assignmentCFDictionary, kDfxSettings_MidiAssignment_##inMember##Key);	\
				mParameterAssignments[parameterID].inMember = static_cast<decltype(mParameterAssignments[parameterID].inMember)>(optionalValue.value_or(0));	\
				numberSuccess = optionalValue.has_value();	\
			}
			bool numberSuccess = false;
			GET_ASSIGNMENT_VALUE_FROM_DICT(mEventType, i)
			if (!numberSuccess)
			{
				unassignParameter(parameterID);
				continue;
			}
			GET_ASSIGNMENT_VALUE_FROM_DICT(mEventChannel, i)
			GET_ASSIGNMENT_VALUE_FROM_DICT(mEventNum, i)
			GET_ASSIGNMENT_VALUE_FROM_DICT(mEventNum2, i)
			GET_ASSIGNMENT_VALUE_FROM_DICT(mEventBehaviorFlags, i)
			GET_ASSIGNMENT_VALUE_FROM_DICT(mDataInt1, i)
			GET_ASSIGNMENT_VALUE_FROM_DICT(mDataInt2, i)
			GET_ASSIGNMENT_VALUE_FROM_DICT(mDataFloat1, f)
			GET_ASSIGNMENT_VALUE_FROM_DICT(mDataFloat2, f)
#undef GET_ASSIGNMENT_VALUE_FROM_DICT
		}
	}
	// this seems like a good enough sign that we at least partially succeeded
	return (arraySize > 0);
}
#endif  // TARGET_API_AUDIOUNIT



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark -
#pragma mark MIDI learn
#pragma mark -
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//-----------------------------------------------------------------------------------------
void DfxSettings::handleCC(int inMidiChannel, int inControllerNumber, int inValue, size_t inOffsetFrames)
{
	// don't allow the "all notes off" CC because almost every sequencer uses that when playback stops
	if (inControllerNumber == DfxMidi::kCC_AllNotesOff)
	{
		return;
	}

	handleMidi_assignParameter(dfx::MidiEventType::CC, inMidiChannel, inControllerNumber, inOffsetFrames);
	handleMidi_automateParameters(dfx::MidiEventType::CC, inMidiChannel, inControllerNumber, inValue, inOffsetFrames);
}

//-----------------------------------------------------------------------------------------
void DfxSettings::handleChannelAftertouch(int inMidiChannel, int inValue, size_t inOffsetFrames)
{
	if (!mAllowChannelAftertouchEvents)
	{
		return;
	}

	constexpr int fakeEventNum = 0;  // not used for this type of event
	handleMidi_assignParameter(dfx::MidiEventType::ChannelAftertouch, inMidiChannel, fakeEventNum, inOffsetFrames);
	constexpr int fakeByte2 = 0;  // not used for this type of event
	handleMidi_automateParameters(dfx::MidiEventType::ChannelAftertouch, inMidiChannel, inValue, fakeByte2, inOffsetFrames);
}

//-----------------------------------------------------------------------------------------
void DfxSettings::handlePitchBend(int inMidiChannel, int inValueLSB, int inValueMSB, size_t inOffsetFrames)
{
	if (!mAllowPitchbendEvents)
	{
		return;
	}

	// do this because MIDI byte 2 is not used to specify 
	// type for pitchbend as it does for some other events, 
	// and stuff below assumes that byte 2 means that
	constexpr int fakeEventNum = 0;
	handleMidi_assignParameter(dfx::MidiEventType::PitchBend, inMidiChannel, fakeEventNum, inOffsetFrames);
	handleMidi_automateParameters(dfx::MidiEventType::PitchBend, inMidiChannel, inValueLSB, inValueMSB, inOffsetFrames);
}

//-----------------------------------------------------------------------------------------
void DfxSettings::handleNoteOn(int inMidiChannel, int inNoteNumber, int inVelocity, size_t inOffsetFrames)
{
	if (!mAllowNoteEvents)
	{
		return;
	}

	handleMidi_assignParameter(dfx::MidiEventType::Note, inMidiChannel, inNoteNumber, inOffsetFrames);
	handleMidi_automateParameters(dfx::MidiEventType::Note, inMidiChannel, inNoteNumber, inVelocity, inOffsetFrames, true);
}

//-----------------------------------------------------------------------------------------
void DfxSettings::handleNoteOff(int inMidiChannel, int inNoteNumber, int inVelocity, size_t inOffsetFrames)
{
	if (!mAllowNoteEvents)
	{
		return;
	}

	bool allowAssignment = true;
	// don't use note-offs if we're using note toggle control, but not note-hold style
	if ((mLearnerEventBehaviorFlags & dfx::kMidiEventBehaviorFlag_Toggle) && 
		!(mLearnerEventBehaviorFlags & dfx::kMidiEventBehaviorFlag_NoteHold))
	{
		allowAssignment = false;
	}
	// don't use note-offs if we're using note ranges, not note toggle control
	if (!(mLearnerEventBehaviorFlags & dfx::kMidiEventBehaviorFlag_Toggle))
	{
		allowAssignment = false;
	}

	if (allowAssignment)
	{
		handleMidi_assignParameter(dfx::MidiEventType::Note, inMidiChannel, inNoteNumber, inOffsetFrames);
	}
	handleMidi_automateParameters(dfx::MidiEventType::Note, inMidiChannel, inNoteNumber, inVelocity, inOffsetFrames, false);
}

//-----------------------------------------------------------------------------------------
void DfxSettings::handleAllNotesOff(int inMidiChannel, size_t inOffsetFrames)
{
	if (!mAllowNoteEvents)
	{
		return;
	}

	for (int i = 0; i < DfxMidi::kNumNotes; i++)
	{
		handleMidi_automateParameters(dfx::MidiEventType::Note, inMidiChannel, i, 0, inOffsetFrames, false);
	}
}

//-----------------------------------------------------------------------------------------
// assign an incoming MIDI event to the learner parameter
void DfxSettings::handleMidi_assignParameter(dfx::MidiEventType inEventType, int inMidiChannel, int inByte1, size_t inOffsetFrames)
{
	// we don't need to make an assignment to a parameter if MIDI learning is off
	if (!mMidiLearn || !isValidParameterID(mLearner))
	{
		return;
	}

	auto const handleAssignment = [this, inEventType, inMidiChannel, inOffsetFrames](int eventNum, int eventNum2)
	{
		// assign the learner parameter to the event that sent the message
		assignParameter(mLearner, inEventType, inMidiChannel, eventNum, eventNum2, 
						mLearnerEventBehaviorFlags, mLearnerDataInt1, mLearnerDataInt2, 
						mLearnerDataFloat1, mLearnerDataFloat2);
		// this is an invitation to do something more, if necessary
		mPlugin.settings_doLearningAssignStuff(mLearner, inEventType, inMidiChannel, eventNum, 
											   inOffsetFrames, eventNum2, mLearnerEventBehaviorFlags, 
											   mLearnerDataInt1, mLearnerDataInt2, mLearnerDataFloat1, mLearnerDataFloat2);
		// and then deactivate the current learner, the learning is complete
		setLearner(dfx::kParameterID_Invalid);
		if (getDeactivateLearningUponLearnt())
		{
			setLearning(false);
		}
	};

	// see whether we are setting up a note range for parameter control 
	if ((inEventType == dfx::MidiEventType::Note) && !(mLearnerEventBehaviorFlags & dfx::kMidiEventBehaviorFlag_Toggle))
	{
		if (mNoteRangeHalfwayDone)
		{
			// only use this note if it's different from the first note in the range
			if (inByte1 != mHalfwayNoteNum)
			{
				mNoteRangeHalfwayDone = false;
				int note1 {}, note2 {};
				if (inByte1 > mHalfwayNoteNum)
				{
					note1 = mHalfwayNoteNum;
					note2 = inByte1;
				}
				else
				{
					note1 = inByte1;
					note2 = mHalfwayNoteNum;
				}
				handleAssignment(note1, note2);
			}
		}
		else
		{
			mNoteRangeHalfwayDone = true;
			mHalfwayNoteNum = inByte1;
		}
	}
	else
	{
		constexpr int fakeEventNum2 = 0;  // not used in this case
		handleAssignment(inByte1, fakeEventNum2);
	}
}

//-----------------------------------------------------------------------------------------
// automate assigned parameters in response to a MIDI event
void DfxSettings::handleMidi_automateParameters(dfx::MidiEventType inEventType, int inMidiChannel, 
												int inByte1, int inByte2, size_t inOffsetFrames, bool inIsNoteOn)
{
	float valueNormalized = static_cast<float>(inByte2) * DfxMidi::kValueScalar;
	if (inEventType == dfx::MidiEventType::ChannelAftertouch)
	{
		valueNormalized = static_cast<float>(inByte1) * DfxMidi::kValueScalar;
	}
	else if (inEventType == dfx::MidiEventType::PitchBend)
	{
		valueNormalized = static_cast<float>((DfxMidi::calculatePitchBendScalar(inByte1, inByte2) + 1.0) * 0.5);
	}

	switch (inEventType)
	{
		case dfx::MidiEventType::ChannelAftertouch:
		case dfx::MidiEventType::PitchBend:
			// do this because MIDI byte 2 is not used to indicate an 
			// event type for these events as it does for other events, 
			// and stuff below assumes that byte 2 means that, so this 
			// keeps byte 2 consistent for these events' assignments
			inByte1 = 0;
			break;
		default:
			break;
	}

	// search for parameters that have this MIDI event assigned to them and, 
	// if any are found, automate them with the event message's value
	for (dfx::ParameterID parameterID = 0; parameterID < mNumParameters; parameterID++)
	{
		auto const& pa = mParameterAssignments.at(parameterID);

		// if the event type doesn't match what this parameter has assigned to it, 
		// skip to the next parameter parameter
		if (pa.mEventType != inEventType)
		{
			continue;
		}
		// if the type matches but not the channel and we're using channels, 
		// skip to the next parameter
		if (mUseChannel && (pa.mEventChannel != inMidiChannel))
		{
			continue;
		}

		if (inEventType == dfx::MidiEventType::Note)
		{
			// toggle the parameter on or off
			// (when using notes, this flag overrides Toggle)
			if (pa.mEventBehaviorFlags & dfx::kMidiEventBehaviorFlag_NoteHold)
			{
				// don't automate this parameter if the note does not match its assignment
				if (pa.mEventNum != inByte1)
				{
					continue;
				}
				valueNormalized = inIsNoteOn ? 1.f : 0.f;
			}
			// toggle the parameter's states
			else if (pa.mEventBehaviorFlags & dfx::kMidiEventBehaviorFlag_Toggle)
			{
				// don't automate this parameter if the note does not match its assignment
				if (pa.mEventNum != inByte1)
				{
					continue;
				}
				// don't use note-offs in non-hold note toggle mode
				if (!inIsNoteOn)
				{
					continue;
				}

				auto const numSteps = std::max(pa.mDataInt1, 2);  // we need at least 2 states to toggle with
				auto maxSteps = pa.mDataInt2;
				// use the total number of steps if a maximum step wasn't specified
				if (maxSteps <= 0)
				{
					maxSteps = numSteps;
				}
				// get the current state of the parameter
				auto currentStep = static_cast<int>(mPlugin.getparameter_gen(parameterID) * (static_cast<float>(numSteps) - 0.01f));
				// cycle to the next state, wraparound if necessary (using maxSteps)
				currentStep = (currentStep + 1) % maxSteps;
				// get the 0-to-1 normalized parameter value version of that state
				valueNormalized = static_cast<float>(currentStep) / static_cast<float>(numSteps - 1);
			}
			// otherwise use a note range
			else
			{
				// don't automate this parameter if the note is not in its range
				if ((inByte1 < pa.mEventNum) || (inByte1 > pa.mEventNum2))
				{
					continue;
				}
				valueNormalized = static_cast<float>(inByte1 - pa.mEventNum) / static_cast<float>(pa.mEventNum2 - pa.mEventNum);
			}
		}
		else
		{
			// since it's not a note, if the event number doesn't 
			// match this parameter's assignment, don't use it
			if (pa.mEventNum != inByte1)
			{
				continue;
			}

			// recalculate valueNormalized to toggle the parameter's states
			if (pa.mEventBehaviorFlags & dfx::kMidiEventBehaviorFlag_Toggle)
			{
				auto const numSteps = std::max(pa.mDataInt1, 2);  // we need at least 2 states to toggle with
				auto maxSteps = pa.mDataInt2;
				// use the total number of steps if a maximum step wasn't specified
				if (maxSteps <= 0)
				{
					maxSteps = numSteps;
				}
				// get the current state of the incoming value 
				// (using maxSteps range to keep within allowable range, if desired)
				auto const currentStep = static_cast<int>(valueNormalized * (static_cast<float>(maxSteps) - 0.01f));
				// constrain the continuous value to a stepped state value 
				// (using numSteps to scale out to the real parameter value)
				valueNormalized = static_cast<float>(currentStep) / static_cast<float>(numSteps - 1);
			}
		}

		// automate the parameter with the value if we've reached this point
		mPlugin.setparameter_gen(parameterID, valueNormalized);
		mPlugin.postupdate_parameter(parameterID);  // notify listeners of internal parameter change
		// this is an invitation to do something more, if necessary
		mPlugin.settings_doMidiAutomatedSetParameterStuff(parameterID, valueNormalized, inOffsetFrames);

	}  // end of parameters loop (for automation)
}



//-----------------------------------------------------------------------------
// clear all parameter assignments from the the CCs
void DfxSettings::clearAssignments()
{
	for (dfx::ParameterID i = 0; i < mNumParameters; i++)
	{
		unassignParameter(i);
	}
}

//-----------------------------------------------------------------------------
// assign a CC to a parameter
void DfxSettings::assignParameter(dfx::ParameterID inParameterID, dfx::MidiEventType inEventType, int inEventChannel, 
								  int inEventNum, int inEventNum2, dfx::MidiEventBehaviorFlags inEventBehaviorFlags, 
								  int inDataInt1, int inDataInt2, float inDataFloat1, float inDataFloat2)
{
	// bail if the parameter index is not valid
	if (!isValidParameterID(inParameterID))
	{
		return;
	}
	// bail if inEventNum is not a valid MIDI value
	if ((inEventNum < 0) || (inEventNum > DfxMidi::kMaxValue))
	{
		return;
	}

	// if we're note-toggling, set up a bogus "range" for comparing with note range assignments
	if ((inEventType == dfx::MidiEventType::Note) && (inEventBehaviorFlags & dfx::kMidiEventBehaviorFlag_Toggle))
	{
		inEventNum2 = inEventNum;
	}

	// first unassign the MIDI event from any other previous 
	// parameter assignment(s) if using stealing
	if (mStealAssignments)
	{
		for (dfx::ParameterID i = 0; i < mNumParameters; i++)
		{
			auto const& pa = mParameterAssignments.at(i);
			// skip this parameter if the event type doesn't match
			if (pa.mEventType != inEventType)
			{
				continue;
			}
			// if the type matches but not the channel and we're using channels, 
			// skip this parameter
			if (mUseChannel && (pa.mEventChannel != inEventChannel))
			{
				continue;
			}

			// it's a note, so we have to do complicated stuff
			if (inEventType == dfx::MidiEventType::Note)
			{
				// lower note overlaps with existing note assignment
				if ((pa.mEventNum >= inEventNum) && (pa.mEventNum <= inEventNum2))
				{
					unassignParameter(i);
				}
				// upper note overlaps with existing note assignment
				else if ((pa.mEventNum2 >= inEventNum) && (pa.mEventNum2 <= inEventNum2))
				{
					unassignParameter(i);
				}
				// current note range consumes the entire existing assignment
				else if ((pa.mEventNum <= inEventNum) && (pa.mEventNum2 >= inEventNum2))
				{
					unassignParameter(i);
				}
			}

			// not a note, so it's simple:  
			// just delete the assignment if the event number matches
			else if (pa.mEventNum == inEventNum)
			{
				unassignParameter(i);
			}
		}
	}

	// then assign the event to the desired parameter
	mParameterAssignments[inParameterID].mEventType = inEventType;
	mParameterAssignments[inParameterID].mEventChannel = inEventChannel;
	mParameterAssignments[inParameterID].mEventNum = inEventNum;
	mParameterAssignments[inParameterID].mEventNum2 = inEventNum2;
	mParameterAssignments[inParameterID].mEventBehaviorFlags = inEventBehaviorFlags;
	mParameterAssignments[inParameterID].mDataInt1 = inDataInt1;
	mParameterAssignments[inParameterID].mDataInt2 = inDataInt2;
	mParameterAssignments[inParameterID].mDataFloat1 = inDataFloat1;
	mParameterAssignments[inParameterID].mDataFloat2 = inDataFloat2;
}

//-----------------------------------------------------------------------------
// remove any MIDI event assignment that a parameter might have
void DfxSettings::unassignParameter(dfx::ParameterID inParameterID)
{
	// return if what we got is not a valid parameter index
	if (!isValidParameterID(inParameterID))
	{
		return;
	}

	// clear the MIDI event assignment for this parameter
	mParameterAssignments[inParameterID].mEventType = dfx::MidiEventType::None;
	mParameterAssignments[inParameterID].mEventChannel = 0;
	mParameterAssignments[inParameterID].mEventNum = 0;
	mParameterAssignments[inParameterID].mEventNum2 = 0;
	mParameterAssignments[inParameterID].mEventBehaviorFlags = dfx::kMidiEventBehaviorFlag_None;
	mParameterAssignments[inParameterID].mDataInt1 = 0;
	mParameterAssignments[inParameterID].mDataInt2 = 0;
	mParameterAssignments[inParameterID].mDataFloat1 = 0.0f;
	mParameterAssignments[inParameterID].mDataFloat2 = 0.0f;
}

//-----------------------------------------------------------------------------
// turn MIDI learn mode on or off
void DfxSettings::setLearning(bool inLearnMode)
{
	// erase the current learner if the state of MIDI learn is being toggled
	if (inLearnMode != mMidiLearn)
	{
		setLearner(dfx::kParameterID_Invalid);
	}
	// or if it's being asked to be turned off, irregardless
	else if (!inLearnMode)
	{
		setLearner(dfx::kParameterID_Invalid);
	}

	mMidiLearn = inLearnMode;

	mPlugin.postupdate_midilearn();
}

//-----------------------------------------------------------------------------
// define the actively learning parameter during MIDI learn mode
void DfxSettings::setLearner(dfx::ParameterID inParameterID, dfx::MidiEventBehaviorFlags inEventBehaviorFlags, 
							 int inDataInt1, int inDataInt2, float inDataFloat1, float inDataFloat2)
{
	// allow this invalid parameter tag, and then exit
	if (inParameterID == dfx::kParameterID_Invalid)
	{
		mLearner = inParameterID;
		mLearnerEventBehaviorFlags = dfx::kMidiEventBehaviorFlag_None;
	}
	else
	{
		// return if what we got is not a valid parameter index
		if (!isValidParameterID(inParameterID))
		{
			return;
		}

		// cancel note range assignment if we're switching to a new learner
		if (mLearner != inParameterID)
		{
			mNoteRangeHalfwayDone = false;
		}

		// only set the learner if MIDI learn is on
		if (mMidiLearn)
		{
			mLearner = inParameterID;
			mLearnerEventBehaviorFlags = inEventBehaviorFlags;
			mLearnerDataInt1 = inDataInt1;
			mLearnerDataInt2 = inDataInt2;
			mLearnerDataFloat1 = inDataFloat1;
			mLearnerDataFloat2 = inDataFloat2;
		}
	}

	mPlugin.postupdate_midilearner();
}

//-----------------------------------------------------------------------------
// a plugin editor should call this upon a value change of a "MIDI learn" control 
// to turn MIDI learning on and off
void DfxSettings::setParameterMidiLearn(bool inValue)
{
	setLearning(inValue);
}

//-----------------------------------------------------------------------------
// a plugin editor should call this upon a value change of a "MIDI reset" control 
// to clear MIDI event assignments
void DfxSettings::setParameterMidiReset()
{
	// if we're in MIDI learn mode and a parameter has been selected, 
	// then erase its MIDI event assigment (if it has one)
	if (mMidiLearn && (mLearner != dfx::kParameterID_Invalid))
	{
		unassignParameter(mLearner);
		setLearner(dfx::kParameterID_Invalid);
	}
	// otherwise erase all of the MIDI event assignments
	else
	{
		clearAssignments();
	}
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark -
#pragma mark misc
#pragma mark -
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//-----------------------------------------------------------------------------
dfx::ParameterAssignment DfxSettings::getParameterAssignment(dfx::ParameterID inParameterID) const
{
	// return a no-assignment structure if what we got is not a valid parameter index
	if (!isValidParameterID(inParameterID))
	{
		return {};
	}

	return mParameterAssignments[inParameterID];
}

//-----------------------------------------------------------------------------
dfx::MidiEventType DfxSettings::getParameterAssignmentType(dfx::ParameterID inParameterID) const
{
	// return no-assignment if what we got is not a valid parameter index
	if (!isValidParameterID(inParameterID))
	{
		return dfx::MidiEventType::None;
	}

	return mParameterAssignments[inParameterID].mEventType;
}

//-----------------------------------------------------------------------------
// given a parameter ID, find the tag (index) for that parameter in a table of parameter IDs
dfx::ParameterID DfxSettings::getParameterIndexFromMap(dfx::ParameterID inParameterID, std::span<uint32_t const> inSearchIDs)
{
	auto const foundID = std::find(inSearchIDs.begin(), inSearchIDs.end(), inParameterID);
	if (foundID != inSearchIDs.end())
	{
		return static_cast<dfx::ParameterID>(std::distance(inSearchIDs.begin(), foundID));
	}
	return dfx::kParameterID_Invalid;
}

//-----------------------------------------------------------------------------
// search using the internal table
dfx::ParameterID DfxSettings::getParameterIndexFromMap(dfx::ParameterID inParameterID) const
{
	return getParameterIndexFromMap(inParameterID, mParameterIDMap);
}

//-----------------------------------------------------------------------------
size_t DfxSettings::sizeOfGenPreset(size_t inParameterCount) noexcept
{
	constexpr auto sizeOfParameterValue = sizeof(std::remove_extent_t<decltype(GenPreset::mParameterValues)>);
	return sizeof(GenPreset) + (sizeOfParameterValue * inParameterCount) - sizeof(GenPreset::mParameterValues);
}


//-----------------------------------------------------------------------------
void DfxSettings::setSteal(bool inMode) noexcept
{
	mStealAssignments = inMode;
	if (inMode)
	{
		mSettingsInfo.mGlobalBehaviorFlags |= kGlobalBehaviorFlag_StealAssignments;
	}
	else
	{
		mSettingsInfo.mGlobalBehaviorFlags &= ~kGlobalBehaviorFlag_StealAssignments;
	}
}

//-----------------------------------------------------------------------------
void DfxSettings::setUseChannel(bool inMode) noexcept
{
	mUseChannel = inMode;
	if (inMode)
	{
		mSettingsInfo.mGlobalBehaviorFlags |= kGlobalBehaviorFlag_UseChannel;
	}
	else
	{
		mSettingsInfo.mGlobalBehaviorFlags &= ~kGlobalBehaviorFlag_UseChannel;
	}
}


//-----------------------------------------------------------------------------
// this is called to investigate what to do when a data chunk is received in 
// restore() that doesn't match the characteristics of what we are expecting
DfxSettings::CrisisError DfxSettings::handleCrisis(CrisisReasonFlags inFlags) const
{
	// no need to continue on if there is no crisis situation
	if (inFlags == kCrisisReasonFlag_None)
	{
		return CrisisError::NoError;
	}

	switch (mCrisisBehavior)
	{
		case CrisisBehavior::LoadWhatYouCan:
			return CrisisError::NoError;

		case CrisisBehavior::DontLoad:
			return CrisisError::QuitError;

		case CrisisBehavior::LoadButComplain:
			mPlugin.settings_crisisAlert(inFlags);
			return CrisisError::ComplainError;

		case CrisisBehavior::CrashTheHostApplication:
			std::abort();
			// if the host is still alive, then we have failed...
			return CrisisError::FailedCrashError;
	}
	assert(false);  // unhandled case
	return CrisisError::NoError;
}

//-----------------------------------------------------------------------------
// use this to pre-test for out-of-bounds memory addressing, probably from corrupt data
void DfxSettings::validateRange(void const* inData, size_t inDataSize, void const* inAddress, size_t inAddressSize, char const* inDataItemName) const
{
	auto const dataStart = reinterpret_cast<uintptr_t>(inData);
	uintptr_t dataEnd {};
	auto overflowed = __builtin_add_overflow(dataStart, inDataSize, &dataEnd);
	auto const addressStart = reinterpret_cast<uintptr_t>(inAddress);
	uintptr_t addressEnd {};
	overflowed |= __builtin_add_overflow(addressStart, inAddressSize, &addressEnd);

	if ((inAddressSize == 0) && (addressStart >= dataStart) && (addressStart <= dataEnd))
	{
		return;
	}
	if (overflowed || (inAddressSize > inDataSize) || (addressStart < dataStart) || (addressStart >= dataEnd) || (addressEnd > dataEnd))
	{
		debugAlertCorruptData(inDataItemName, inAddressSize, inDataSize);
		throw std::invalid_argument("bad data");
	}
};

//-----------------------------------------------------------------------------
void DfxSettings::debugAlertCorruptData(char const* inDataItemName, size_t inDataItemSize, size_t inDataTotalSize) const
{
#if TARGET_OS_MAC
	CFStringRef const title = CFSTR("settings data fuct");
	auto const message = dfx::MakeUniqueCFType(CFStringCreateWithFormat(kCFAllocatorDefault, nullptr, 
																		CFSTR("This should never happen.  Please inform the computerists at " DESTROYFX_URL " if you see this message.  Please tell them: \n\ndata item name = %s \ndata item size = %zu \ntotal data size = %zu"),
																		inDataItemName, inDataItemSize, inDataTotalSize));
	if (message)
	{
#ifdef TARGET_API_AUDIOUNIT
		auto const iconURL = dfx::MakeUniqueCFType(mPlugin.CopyIconLocation());
#else
		dfx::UniqueCFType<CFURLRef> const iconURL;
#endif
		CFUserNotificationDisplayNotice(0.0, kCFUserNotificationStopAlertLevel, iconURL.get(), nullptr, nullptr, title, message.get(), nullptr);
	}
#elif TARGET_OS_WIN32
	std::array<char, 512> msg {};
	snprintf(msg.data(), msg.size(),
			 "Something is wrong with the settings data! "
			 "Info for bug reports: name: %s size: %zu total: %zu",
			 inDataItemName, inDataItemSize, inDataTotalSize);
	MessageBoxA(nullptr, msg.data(), "DFX Error!", 0);
#else
	#warning "implementation missing"
#endif
}
