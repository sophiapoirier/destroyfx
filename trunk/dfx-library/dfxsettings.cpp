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
Welcome to our settings persistance mess.
------------------------------------------------------------------------*/

#include "dfxsettings.h"

#include <algorithm>
#include <cassert>
#include <numeric>
#include <experimental/optional>
#include <stdio.h>  // for FILE stuff
#include <vector>

#include "dfxmidi.h"
#include "dfxmisc.h"
#include "dfxplugin.h"


//------------------------------------------------------
static bool DFX_GetEnvBool(char const* inVarName, bool inFallbackValue);


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark -
#pragma mark init / destroy
#pragma mark -
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//-----------------------------------------------------------------------------
DfxSettings::DfxSettings(long inMagic, DfxPlugin* inPlugin, size_t inSizeofExtendedData)
:	mPlugin(inPlugin),
	mNumParameters(std::max(inPlugin->getnumparameters(), 1L)),	// we need at least 1 parameter
	mNumPresets(std::max(inPlugin->getnumpresets(), 1L)),	// we need at least 1 set of parameters
	mSizeOfExtendedData(inSizeofExtendedData),
	mParameterIDs(mNumParameters, kDfxParameterID_Invalid),
	mParameterAssignments(mNumParameters)
{
	assert(inPlugin);

	// default to each parameter having its ID equal its index
	std::iota(mParameterIDs.begin(), mParameterIDs.end(), 0);

	// calculate some data sizes that are useful to know
	mSizeOfPreset = sizeof(GenPreset) + (sizeof(*GenPreset::mParameterValues) * mNumParameters) - sizeof(GenPreset::mParameterValues);
	mSizeOfParameterIDs = sizeof(mParameterIDs.front()) * mNumParameters;
	mSizeOfPresetChunk = mSizeOfPreset 				// 1 preset
						 + sizeof(SettingsInfo) 	// the special data header info
						 + mSizeOfParameterIDs		// the table of parameter IDs
						 + (sizeof(DfxParameterAssignment) * mNumParameters)  // the MIDI events assignment array
						 + inSizeofExtendedData;
	mSizeOfChunk = (mSizeOfPreset * mNumPresets)	// all of the presets
					+ sizeof(SettingsInfo)			// the special data header info
					+ mSizeOfParameterIDs			// the table of parameter IDs
					+ (sizeof(DfxParameterAssignment) * mNumParameters)  // the MIDI events assignment array
					+ inSizeofExtendedData;

	// set all of the header infos
	mSettingsInfo.mMagic = inMagic;
	mSettingsInfo.mVersion = mPlugin->getpluginversion();
	mSettingsInfo.mLowestLoadableVersion = 0;
	mSettingsInfo.mStoredHeaderSize = sizeof(SettingsInfo);
	mSettingsInfo.mNumStoredParameters = mNumParameters;
	mSettingsInfo.mNumStoredPresets = mNumPresets;
	mSettingsInfo.mStoredParameterAssignmentSize = sizeof(DfxParameterAssignment);
	mSettingsInfo.mStoredExtendedDataSize = inSizeofExtendedData;

	clearAssignments();  // initialize all of the parameters to have no MIDI event assignments

	// default to allowing MIDI event assignment sharing instead of stealing them, 
	// unless the user has defined the environment variable DFX_PARAM_STEALMIDI
	mStealAssignments = DFX_GetEnvBool("DFX_PARAM_STEALMIDI", false);

	// default to ignoring MIDI channel in MIDI event assignments and automation, 
	// unless the user has defined the environment variable DFX_PARAM_USECHANNEL
	mUseChannel = DFX_GetEnvBool("DFX_PARAM_USECHANNEL", false);

	// allow for further constructor stuff, if necessary
	mPlugin->settings_init();
}

//-----------------------------------------------------------------------------
DfxSettings::~DfxSettings()
{
	// wipe out the signature
	mSettingsInfo.mMagic = 0;

	// allow for further destructor stuff, if necessary
	mPlugin->settings_cleanup();
}

//------------------------------------------------------
// this interprets a UNIX environment variable string as a boolean
static bool DFX_GetEnvBool(char const* inVarName, bool inFallbackValue)
{
	auto const env = getenv(inVarName);

	// return the default value if the getenv failed
	if (!env)
	{
		return inFallbackValue;
	}

	switch (env[0])
	{
		case 't':
		case 'T':
		case '1':
			return true;

		case 'f':
		case 'F':
		case '0':
			return false;

		default:
			return inFallbackValue;
	}
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark -
#pragma mark settings
#pragma mark -
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//-----------------------------------------------------------------------------
// this gets called when the host wants to save settings data, 
// like when saving a session document or preset files
std::vector<uint8_t> DfxSettings::save(bool inIsPreset)
{
	std::vector<uint8_t> data(inIsPreset ? mSizeOfPresetChunk : mSizeOfChunk, 0);
	auto const sharedChunk = reinterpret_cast<SettingsInfo*>(data.data());

	// and a few pointers to elements within that data, just for ease of use
	auto const firstSharedParameterID = reinterpret_cast<int32_t*>(data.data() + sizeof(SettingsInfo));
	auto const firstSharedPreset = reinterpret_cast<GenPreset*>(reinterpret_cast<char*>(firstSharedParameterID) + mSizeOfParameterIDs);
	auto const firstSharedParameterAssignment = reinterpret_cast<DfxParameterAssignment*>(reinterpret_cast<char*>(firstSharedPreset) + (mSizeOfPreset * mNumPresets));

	// first store the special chunk infos
	sharedChunk->mMagic = mSettingsInfo.mMagic;
	sharedChunk->mVersion = mSettingsInfo.mVersion;
	sharedChunk->mLowestLoadableVersion = mSettingsInfo.mLowestLoadableVersion;
	sharedChunk->mStoredHeaderSize = mSettingsInfo.mStoredHeaderSize;
	sharedChunk->mNumStoredParameters = mSettingsInfo.mNumStoredParameters;
	sharedChunk->mNumStoredPresets = inIsPreset ? 1 : mSettingsInfo.mNumStoredPresets;
	sharedChunk->mStoredParameterAssignmentSize = mSettingsInfo.mStoredParameterAssignmentSize;
	sharedChunk->mStoredExtendedDataSize = mSettingsInfo.mStoredExtendedDataSize;

	// store the parameters' IDs
	for (size_t i = 0; i < mParameterIDs.size(); i++)
	{
		firstSharedParameterID[i] = mParameterIDs[i];
	}

	// store only one preset setting if inIsPreset is true
	if (inIsPreset)
	{
		memset(firstSharedPreset->mName, 0, sizeof(firstSharedPreset->mName));
		mPlugin->getpresetname(mPlugin->getcurrentpresetnum(), firstSharedPreset->mName);
		for (long i = 0; i < mNumParameters; i++)
		{
			firstSharedPreset->mParameterValues[i] = mPlugin->getparameter_f(i);
		}

		auto const tempSharedParameterAssignment = reinterpret_cast<DfxParameterAssignment*>(reinterpret_cast<char*>(firstSharedPreset) + mSizeOfPreset);
		// store the parameters' MIDI event assignments
		for (long i = 0; i < mNumParameters; i++)
		{
			tempSharedParameterAssignment[i] = mParameterAssignments[i];
		}
	}
	// otherwise store the entire bank of presets and the MIDI event assignments
	else
	{
		auto tempSharedPresets = firstSharedPreset;
		for (long j = 0; j < mNumPresets; j++)
		{
			// copy the preset name to the chunk
			mPlugin->getpresetname(j, tempSharedPresets->mName);
			// copy all of the parameters for this preset to the chunk
			for (long i = 0; i < mNumParameters; i++)
			{
				tempSharedPresets->mParameterValues[i] = mPlugin->getpresetparameter_f(j, i);
			}
			// point to the next preset in the data array for the host
			tempSharedPresets = reinterpret_cast<GenPreset*>(reinterpret_cast<char*>(tempSharedPresets) + mSizeOfPreset);
		}

		// store the parameters' MIDI event assignments
		for (long i = 0; i < mNumParameters; i++)
		{
			firstSharedParameterAssignment[i] = mParameterAssignments[i];
		}
	}

	// reverse the order of bytes in the data being sent to the host, if necessary
	correctEndian(data.data(), data.size(), false, inIsPreset);
	// allow for the storage of extra data
	mPlugin->settings_saveExtendedData(data.data() + data.size() - mSizeOfExtendedData, inIsPreset);

	return data;
}


//-----------------------------------------------------------------------------
// for backwerds compaxibilitee
inline bool DFX_IsOldVstVersionNumber(long inVersion)
{
	return (inVersion < 0x00010000);
}
#ifdef DFX_SUPPORT_OLD_VST_SETTINGS
constexpr size_t kDfxOldPresetNameMaxLength = 32;
#endif

//-----------------------------------------------------------------------------
// this gets called when the host wants to load settings data, 
// like when restoring settings while opening a song, 
// or loading a preset file
bool DfxSettings::restore(void const* inData, size_t inBufferSize, bool inIsPreset)
{
	// create our own copy of the data before we muck with it (e.g. reversing endianness, etc.)
	dfx::UniqueMemoryBlock<void> const incomingData_copy(inBufferSize);
	if (!incomingData_copy)
	{
		return false;
	}
	memcpy(incomingData_copy.get(), inData, inBufferSize);

	// un-reverse the order of bytes in the received data, if necessary
	auto const endianSuccess = correctEndian(incomingData_copy.get(), inBufferSize, true, inIsPreset);
	if (!endianSuccess)
	{
		return false;
	}

	// point to the start of the chunk data:  the SettingsInfo header
	auto const newSettingsInfo = static_cast<SettingsInfo*>(incomingData_copy.get());

	// The following situations are basically considered to be 
	// irrecoverable "crisis" situations.  Regardless of what 
	// crisis behavior has been chosen, any of the following errors 
	// will prompt an unsuccessful exit because these are big problems.  
	// Incorrect magic signature basically means that these settings are 
	// probably for some other plugin.  And the whole point of setting a 
	// lowest loadable version value is that it should be taken seriously.
	if (newSettingsInfo->mMagic != mSettingsInfo.mMagic)
	{
		return false;
	}
	if ((newSettingsInfo->mVersion < mSettingsInfo.mLowestLoadableVersion) || 
		(mSettingsInfo.mVersion < newSettingsInfo->mLowestLoadableVersion))  // XXX ummm does this second part make any sense?
	{
		return false;
	}

#ifdef DFX_SUPPORT_OLD_VST_SETTINGS
	// we started using hex format versions (like below) with the advent 
	// of the glorious DfxPlugin
	// versions lower than 0x00010000 indicate inferior settings
	auto const oldVST = DFX_IsOldVstVersionNumber(newSettingsInfo->mVersion);
#endif

	// these just make the values easier to work with (no need for newSettingsInfo-> so often)
	auto const numStoredParameters = newSettingsInfo->mNumStoredParameters;
	auto const numStoredPresets = newSettingsInfo->mNumStoredPresets;
	auto const storedHeaderSize = newSettingsInfo->mStoredHeaderSize;

	// figure out how many presets we should try to load 
	// if the incoming chunk doesn't match what we're expecting
	long const copyPresets = inIsPreset ? 1 : std::min(static_cast<long>(numStoredPresets), mNumPresets);
	// figure out how much of the DfxParameterAssignment structure we can import
	auto const copyParameterAssignmentSize = std::min(newSettingsInfo->mStoredParameterAssignmentSize, mSettingsInfo.mStoredParameterAssignmentSize);

	// check for conflicts and keep track of them
	CrisisReasonFlags crisisFlags = kCrisisReasonFlag_None;
	if (newSettingsInfo->mVersion < mSettingsInfo.mVersion)
	{
		crisisFlags = crisisFlags | kCrisisReasonFlag_LowerVersion;
	}
	else if (newSettingsInfo->mVersion > mSettingsInfo.mVersion)
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
		if (inBufferSize < mSizeOfPresetChunk)
		{
			crisisFlags = crisisFlags | kCrisisReasonFlag_SmallerByteSize;
		}
		else if (inBufferSize > mSizeOfPresetChunk)
		{
			crisisFlags = crisisFlags | kCrisisReasonFlag_LargerByteSize;
		}
	}
	else
	{
		if (inBufferSize < mSizeOfChunk)
		{
			crisisFlags = crisisFlags | kCrisisReasonFlag_SmallerByteSize;
		}
		else if (inBufferSize > mSizeOfChunk)
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
	// handle the crisis situations (if any) and abort loading if we're told to
	if (handleCrisis(crisisFlags) == CrisisError::AbortError)
	{
		return false;
	}

	// point to the next data element after the chunk header:  the first parameter ID
	auto const newParameterIDs = reinterpret_cast<int32_t*>(reinterpret_cast<char*>(newSettingsInfo) + storedHeaderSize);
	// create a mapping table for corresponding the incoming parameters to the 
	// destination parameters (in case the parameter IDs don't all match up)
	//  [ the index of paramMap is the same as our parameter tag/index and the value 
	//    is the tag/index of the incoming parameter that corresponds, if any ]
	std::vector<long> paramMap(mNumParameters, kDfxParameterID_Invalid);
	for (size_t tag = 0; tag < mParameterIDs.size(); tag++)
	{
		paramMap[tag] = getParameterTagFromID(mParameterIDs[tag], numStoredParameters, newParameterIDs);
	}

	// point to the next data element after the parameter IDs:  the first preset name
	auto newPreset = reinterpret_cast<GenPreset*>(reinterpret_cast<char*>(newParameterIDs) + (sizeof(mParameterIDs.front()) * numStoredParameters));
	// handy for incrementing the data pointer
	size_t const sizeofStoredPreset = sizeof(GenPreset) + (sizeof(*GenPreset::mParameterValues) * numStoredParameters) - sizeof(GenPreset::mParameterValues);

	// the chunk being received only contains one preset
	if (inIsPreset)
	{
		// in Audio Unit, this is handled already in AUBase::RestoreState, 
		// and we are not really loading a "preset,"
		// we are restoring the last user state
		#ifndef TARGET_API_AUDIOUNIT
		// copy the preset name from the chunk
		mPlugin->setpresetname(mPlugin->getcurrentpresetnum(), newPreset->mName);
		#endif
	#ifdef DFX_SUPPORT_OLD_VST_SETTINGS
		// back up the pointer to account for shorter preset names
		if (oldVST)
		{
			newPreset = reinterpret_cast<GenPreset*>(reinterpret_cast<char*>(newPreset) + kDfxOldPresetNameMaxLength - kDfxPresetNameMaxLength);
		}
	#endif
		// copy all of the parameters that we can for this preset from the chunk
		for (long i = 0; i < static_cast<long>(paramMap.size()); i++)
		{
			auto const mappedTag = paramMap[i];
			if (mappedTag != kDfxParameterID_Invalid)
			{
			#ifdef DFX_SUPPORT_OLD_VST_SETTINGS
				// handle old-style generic VST 0.0 to 1.0 parameter values
				if (oldVST)
				{
					mPlugin->setparameter_gen(i, newPreset->mParameterValues[mappedTag]);
				}
				else
			#endif
				{
					mPlugin->setparameter_f(i, newPreset->mParameterValues[mappedTag]);
				}
				// allow for additional tweaking of the stored parameter setting
				mPlugin->settings_doChunkRestoreSetParameterStuff(i, newPreset->mParameterValues[mappedTag], newSettingsInfo->mVersion);
			}
		}
		// point to the next preset in the received data array
		newPreset = reinterpret_cast<GenPreset*>(reinterpret_cast<char*>(newPreset) + sizeofStoredPreset);
	}

	// the chunk being received has all of the presets plus the MIDI event assignments
	else
	{
		// we're loading an entire bank of presets plus the MIDI event assignments, 
		// so cycle through all of the presets and load them up, as many as we can
		for (long j = 0; j < copyPresets; j++)
		{
			// copy the preset name from the chunk
			mPlugin->setpresetname(j, newPreset->mName);
		#ifdef DFX_SUPPORT_OLD_VST_SETTINGS
			// back up the pointer to account for shorter preset names
			if (oldVST)
			{
				newPreset = reinterpret_cast<GenPreset*>(reinterpret_cast<char*>(newPreset) + kDfxOldPresetNameMaxLength - kDfxPresetNameMaxLength);
			}
		#endif
			// copy all of the parameters that we can for this preset from the chunk
			for (long i = 0; i < static_cast<long>(paramMap.size()); i++)
			{
				auto const mappedTag = paramMap[i];
				if (mappedTag != kDfxParameterID_Invalid)
				{
				#ifdef DFX_SUPPORT_OLD_VST_SETTINGS
					// handle old-style generic VST 0.0 to 1.0 parameter values
					if (oldVST)
					{
						mPlugin->setpresetparameter_gen(j, i, newPreset->mParameterValues[mappedTag]);
					}
					else
				#endif
					{
						mPlugin->setpresetparameter_f(j, i, newPreset->mParameterValues[mappedTag]);
					}
					// allow for additional tweaking of the stored parameter setting
					mPlugin->settings_doChunkRestoreSetParameterStuff(i, newPreset->mParameterValues[mappedTag], newSettingsInfo->mVersion, j);
				}
			}
			// point to the next preset in the received data array
			newPreset = reinterpret_cast<GenPreset*>(reinterpret_cast<char*>(newPreset) + sizeofStoredPreset);
		}
	}

#ifdef DFX_SUPPORT_OLD_VST_SETTINGS
if (!(oldVST && inIsPreset))
{
#endif
	// completely clear our table of parameter assignments before loading the new 
	// table since the new one might not have all of the data members
	clearAssignments();
	// then point to the last chunk data element, the MIDI event assignment array
	// (offset by the number of stored presets that were skipped, if any)
	DfxParameterAssignment* newParameterAssignments = nullptr;
//	if (inIsPreset)
	{
//		newParameterAssignments = reinterpret_cast<DfxParameterAssignment*>(reinterpret_cast<char*>(newPreset) + sizeofStoredPreset);
	}
//	else
	{
		newParameterAssignments = reinterpret_cast<DfxParameterAssignment*>(reinterpret_cast<char*>(newPreset) + 
								  ((numStoredPresets - copyPresets) * sizeofStoredPreset));
	}
	// and load up as many of them as we can
	for (long i = 0; i < mNumParameters; i++)
	{
		auto const mappedTag = paramMap[i];
		if (mappedTag != kDfxParameterID_Invalid)
		{
			memcpy(mParameterAssignments.data() + i, 
				   reinterpret_cast<char*>(newParameterAssignments) + (mappedTag * newSettingsInfo->mStoredParameterAssignmentSize), 
				   copyParameterAssignmentSize);
//			mParameterAssignments[i] = newParameterAssignments[mappedTag];
		}
	}
#ifdef DFX_SUPPORT_OLD_VST_SETTINGS
}
#endif

	// allow for the retrieval of extra data
	mPlugin->settings_restoreExtendedData(static_cast<char*>(incomingData_copy.get()) + mSizeOfChunk - newSettingsInfo->mStoredExtendedDataSize, 
										 newSettingsInfo->mStoredExtendedDataSize, newSettingsInfo->mVersion, inIsPreset);

	return true;
}

//-----------------------------------------------------------------------------
// XXX temporary (for testing)
void DFX_DEBUG_ALERT_CORRUPT_DATA(char const* inDataItemName, size_t inDataItemSize, size_t inDataTotalSize)
{
#if 1
#if TARGET_OS_MAC
	CFStringRef title = CFSTR("settings data fuct");
	dfx::UniqueCFType<CFStringRef> const message(CFStringCreateWithFormat(kCFAllocatorDefault, nullptr, 
																		  CFSTR("This should never happen.  Please inform the developers at " DESTROYFX_URL " if you see this message.  Please tell them: \r\rdata item name = %s \rdata item size = %lu \rtotal data size = %lu"),
																		  inDataItemName, inDataItemSize, inDataTotalSize));
	if (message)
	{
		CFUserNotificationDisplayNotice(0.0, kCFUserNotificationPlainAlertLevel, nullptr, nullptr, nullptr, title, message.get(), nullptr);
	}
#endif
#endif
}

//-----------------------------------------------------------------------------
// this function, if called for the non-reference endian architecture, 
// will reverse the order of bytes in each variable/value of the data 
// to correct endian differences and make a uniform data chunk
bool DfxSettings::correctEndian(void* ioData, size_t inDataSize, bool inIsReversed, bool inIsPreset)
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
#if __BIG_ENDIAN__
// big endian (like PowerPC) is the reference architecture, so no byte-swapping is necessary
#else
	// start by looking at the header info
	auto const dataHeader = static_cast<SettingsInfo*>(ioData);
	// we need to know how big the header is before dealing with it
	auto storedHeaderSize = dataHeader->mStoredHeaderSize;
	auto numStoredParameters = dataHeader->mNumStoredParameters;
	auto numStoredPresets = dataHeader->mNumStoredPresets;
	auto storedVersion = dataHeader->mVersion;
	// correct the values' endian byte order order if the data was received byte-swapped
	if (inIsReversed)
	{
		dfx::ReverseBytes(&storedHeaderSize, sizeof(storedHeaderSize));
		dfx::ReverseBytes(&numStoredParameters, sizeof(numStoredParameters));
		dfx::ReverseBytes(&numStoredPresets, sizeof(numStoredPresets));
		dfx::ReverseBytes(&storedVersion, sizeof(storedVersion));
	}
	assert(numStoredParameters >= 0);
	assert(numStoredPresets >= 0);
//	if (inIsPreset)
//	{
//		numStoredPresets = 1;
//	}

	// use this to pre-test for out-of-bounds memory addressing, probably from corrupt data
	void const* const dataEndAddress = static_cast<char*>(ioData) + inDataSize;

	// reverse the order of bytes of the header values
	if ((reinterpret_cast<char*>(dataHeader) + storedHeaderSize) > dataEndAddress)  // the data is somehow corrupt
	{
		DFX_DEBUG_ALERT_CORRUPT_DATA("header", storedHeaderSize, inDataSize);
		return false;
	}
	dfx::ReverseBytes(dataHeader, sizeof(dataHeader->mMagic), storedHeaderSize / sizeof(dataHeader->mMagic));

	// reverse the byte order for each of the parameter IDs
	auto const dataParameterIDs = reinterpret_cast<int32_t*>(static_cast<char*>(ioData) + storedHeaderSize);
	if ((reinterpret_cast<char*>(dataParameterIDs) + (sizeof(*dataParameterIDs) * numStoredParameters)) > dataEndAddress)  // the data is somehow corrupt
	{
		DFX_DEBUG_ALERT_CORRUPT_DATA("parameter IDs", sizeof(*dataParameterIDs) * numStoredParameters, inDataSize);
		return false;
	}
	dfx::ReverseBytes(dataParameterIDs, sizeof(*dataParameterIDs), static_cast<size_t>(numStoredParameters));

	// reverse the order of bytes for each parameter value, 
	// but no need to mess with the preset names since they are char strings
	auto dataPresets = reinterpret_cast<GenPreset*>(reinterpret_cast<char*>(dataParameterIDs) + (sizeof(*dataParameterIDs) * numStoredParameters));
	size_t sizeofStoredPreset = sizeof(*dataPresets) + (sizeof(*(dataPresets->mParameterValues)) * numStoredParameters) - sizeof(dataPresets->mParameterValues);
#ifdef DFX_SUPPORT_OLD_VST_SETTINGS
	if (DFX_IsOldVstVersionNumber(storedVersion))
	{
		// back up the pointer to account for shorter preset names
		dataPresets = reinterpret_cast<GenPreset*>(reinterpret_cast<char*>(dataPresets) - kDfxPresetNameMaxLength + kDfxOldPresetNameMaxLength);
		// and shrink the size to account for shorter preset names
		sizeofStoredPreset -= kDfxPresetNameMaxLength;
		sizeofStoredPreset += kDfxOldPresetNameMaxLength;
	}
#endif
	if ((reinterpret_cast<char*>(dataPresets) + (sizeofStoredPreset * numStoredPresets)) > dataEndAddress)  // the data is somehow corrupt
	{
		DFX_DEBUG_ALERT_CORRUPT_DATA("presets", sizeofStoredPreset * numStoredPresets, inDataSize);
		return false;
	}
	for (long i = 0; i < numStoredPresets; i++)
	{
		dfx::ReverseBytes(dataPresets->mParameterValues, sizeof(*(dataPresets->mParameterValues)), static_cast<size_t>(numStoredParameters));  //XXX potential floating point machine error?
		// point to the next preset in the data array
		dataPresets = reinterpret_cast<GenPreset*>(reinterpret_cast<char*>(dataPresets) + sizeofStoredPreset);
	}
#ifdef DFX_SUPPORT_OLD_VST_SETTINGS
	if (DFX_IsOldVstVersionNumber(storedVersion))
	{
		// advance the pointer to compensate for backing up earlier
		dataPresets = reinterpret_cast<GenPreset*>(reinterpret_cast<char*>(dataPresets) + kDfxPresetNameMaxLength - kDfxOldPresetNameMaxLength);
	}
#endif

#ifdef DFX_SUPPORT_OLD_VST_SETTINGS
if (!(DFX_IsOldVstVersionNumber(storedVersion) && inIsPreset))
{
#endif
	// and reverse the byte order of each event assignment
	auto const dataParameterAssignments = reinterpret_cast<DfxParameterAssignment*>(dataPresets);
	if ((reinterpret_cast<char*>(dataParameterAssignments) + (sizeof(*dataParameterAssignments) * numStoredParameters)) > dataEndAddress)  // the data is somehow corrupt
	{
		DFX_DEBUG_ALERT_CORRUPT_DATA("parameter assignments", sizeof(*dataParameterAssignments) * numStoredParameters, inDataSize);
		return false;
	}
	for (long i = 0; i < numStoredParameters; i++)
	{
#define REVERSE_BYTES_ASSIGNMENT_ITEM(inMember)	\
		dfx::ReverseBytes(&(dataParameterAssignments[i].inMember), sizeof(dataParameterAssignments[i].inMember));
		REVERSE_BYTES_ASSIGNMENT_ITEM(mEventType)
		REVERSE_BYTES_ASSIGNMENT_ITEM(mEventChannel)
		REVERSE_BYTES_ASSIGNMENT_ITEM(mEventNum)
		REVERSE_BYTES_ASSIGNMENT_ITEM(mEventNum2)
		REVERSE_BYTES_ASSIGNMENT_ITEM(mEventBehaviorFlags)
		REVERSE_BYTES_ASSIGNMENT_ITEM(mDataInt1)
		REVERSE_BYTES_ASSIGNMENT_ITEM(mDataInt2)
		REVERSE_BYTES_ASSIGNMENT_ITEM(mDataFloat1)  // XXX potential floating point machine error?
		REVERSE_BYTES_ASSIGNMENT_ITEM(mDataFloat2)  // XXX potential floating point machine error?
#undef REVERSE_BYTES_ASSIGNMENT_ITEM
	}
#ifdef DFX_SUPPORT_OLD_VST_SETTINGS
}
#endif

#endif
// __BIG_ENDIAN__ (endian check)

	return true;
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

//-----------------------------------------------------------------------------------------
bool DFX_AddNumberToCFDictionary(void const* inNumber, CFNumberType inType, CFMutableDictionaryRef inDictionary, void const* inDictionaryKey)
{
	if (!inNumber || !inDictionary || !inDictionaryKey)
	{
		return false;
	}

	dfx::UniqueCFType<CFNumberRef> const cfNumber(CFNumberCreate(kCFAllocatorDefault, inType, inNumber));
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

//-----------------------------------------------------------------------------------------
std::experimental::optional<SInt64> DFX_GetNumberFromCFDictionary_i(CFDictionaryRef inDictionary, void const* inDictionaryKey)
{
	constexpr CFNumberType numberType = kCFNumberSInt64Type;

	if (!inDictionary || !inDictionaryKey)
	{
		return {};
	}

	auto const cfNumber = reinterpret_cast<CFNumberRef>(CFDictionaryGetValue(inDictionary, inDictionaryKey));
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
std::experimental::optional<Float64> DFX_GetNumberFromCFDictionary_f(CFDictionaryRef inDictionary, void const* inDictionaryKey)
{
	constexpr CFNumberType numberType = kCFNumberFloat64Type;

	if (!inDictionary || !inDictionaryKey)
	{
		return {};
	}

	auto const cfNumber = reinterpret_cast<CFNumberRef>(CFDictionaryGetValue(inDictionary, inDictionaryKey));
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
bool DfxSettings::saveMidiAssignmentsToDictionary(CFMutableDictionaryRef inDictionary)
{
	if (!inDictionary)
	{
		return false;
	}

	bool assignmentsFound = false;
	for (long i = 0; i < mNumParameters; i++)
	{
		if (getParameterAssignmentType(i) != DfxMidiEventType::None)
		{
			assignmentsFound = true;
		}
	}

	if (assignmentsFound)
	{
		dfx::UniqueCFType<CFMutableArrayRef> const assignmentsCFArray(CFArrayCreateMutable(kCFAllocatorDefault, mNumParameters, &kCFTypeArrayCallBacks));
		if (assignmentsCFArray)
		{
			for (long i = 0; i < mNumParameters; i++)
			{
				dfx::UniqueCFType<CFMutableDictionaryRef> const assignmentCFDictionary(CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
				if (assignmentCFDictionary)
				{
					if (getParameterID(i) == kDfxParameterID_Invalid)
					{
						continue;
					}
					DFX_AddNumberToCFDictionary_i(getParameterID(i), assignmentCFDictionary.get(), kDfxSettings_ParameterIDKey);
#define ADD_ASSIGNMENT_VALUE_TO_DICT(inMember, inTypeSuffix)	\
					DFX_AddNumberToCFDictionary_##inTypeSuffix(mParameterAssignments[i].inMember, assignmentCFDictionary.get(), kDfxSettings_MidiAssignment_##inMember##Key);
					DFX_AddNumberToCFDictionary_i(static_cast<SInt64>(mParameterAssignments[i].mEventType), 
												  assignmentCFDictionary.get(), kDfxSettings_MidiAssignment_mEventTypeKey);
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
			CFDictionarySetValue(inDictionary, kDfxSettings_MidiAssignmentsKey, assignmentsCFArray.get());
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------------------
bool DfxSettings::restoreMidiAssignmentsFromDictionary(CFDictionaryRef inDictionary)
{
	if (!inDictionary)
	{
		return false;
	}

	auto const assignmentsCFArray = reinterpret_cast<CFArrayRef>(CFDictionaryGetValue(inDictionary, kDfxSettings_MidiAssignmentsKey));
	if (assignmentsCFArray && (CFGetTypeID(assignmentsCFArray) == CFArrayGetTypeID()))
	{
		// completely clear our table of parameter assignments before loading the new 
		// table since the new one might not have all of the data members
		clearAssignments();

		auto const arraySize = CFArrayGetCount(assignmentsCFArray);
		for (CFIndex i = 0; i < arraySize; i++)
		{
			auto const assignmentCFDictionary = reinterpret_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(assignmentsCFArray, i));
			if (assignmentCFDictionary)
			{
				auto const paramID_optional = DFX_GetNumberFromCFDictionary_i(assignmentCFDictionary, kDfxSettings_ParameterIDKey);
				if (!paramID_optional)
				{
					continue;
				}
				auto const paramID = getParameterTagFromID(paramID_optional.value_or(kDfxParameterID_Invalid));
				if (paramID == kDfxParameterID_Invalid)
				{
					continue;
				}
#define GET_ASSIGNMENT_VALUE_FROM_DICT(inMember, inTypeSuffix)	\
				{	\
					auto const optionalValue = DFX_GetNumberFromCFDictionary_##inTypeSuffix(assignmentCFDictionary, kDfxSettings_MidiAssignment_##inMember##Key);	\
					mParameterAssignments[paramID].inMember = static_cast<decltype(mParameterAssignments[paramID].inMember)>(optionalValue.value_or(0));	\
					numberSuccess = optionalValue ? true : false;	\
				}
				bool numberSuccess = false;
				GET_ASSIGNMENT_VALUE_FROM_DICT(mEventType, i)
				if (!numberSuccess)
				{
					unassignParam(paramID);
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
		if (arraySize > 0)
		{
			return true;
		}
	}

	return false;
}
#endif



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark -
#pragma mark MIDI learn
#pragma mark -
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//-----------------------------------------------------------------------------------------
void DfxSettings::handleCC(int inMidiChannel, int inControllerNumber, int inValue, long inBufferOffset)
{
	// don't allow the "all notes off" CC because almost every sequencer uses that when playback stops
	if (inControllerNumber == DfxMidi::kCC_AllNotesOff)
	{
		return;
	}

	handleMidi_assignParam(DfxMidiEventType::CC, inMidiChannel, inControllerNumber, inBufferOffset);
	handleMidi_automateParams(DfxMidiEventType::CC, inMidiChannel, inControllerNumber, inValue, inBufferOffset);
}

//-----------------------------------------------------------------------------------------
void DfxSettings::handlePitchBend(int inMidiChannel, int inValueLSB, int inValueMSB, long inBufferOffset)
{
	if (!mAllowPitchbendEvents)
	{
		return;
	}

	// do this because MIDI byte 2 is not used to indicate an 
	// event type for pitchbend as it does for other events, 
	// and stuff below assumes that byte 2 means that, so this 
	// keeps byte 2 consistent for pitchbend assignments
	int realLSB = inValueLSB;
	inValueLSB = 0;  // <- XXX this is stoopid

	handleMidi_assignParam(DfxMidiEventType::PitchBend, inMidiChannel, inValueLSB, inBufferOffset);

	inValueLSB = realLSB;  // restore it   <- XXX ugh stupid hackz...
	handleMidi_automateParams(DfxMidiEventType::PitchBend, inMidiChannel, inValueLSB, inValueMSB, inBufferOffset);
}

//-----------------------------------------------------------------------------------------
void DfxSettings::handleNoteOn(int inMidiChannel, int inNoteNumber, int inVelocity, long inBufferOffset)
{
	if (!mAllowNoteEvents)
	{
		return;
	}

	handleMidi_assignParam(DfxMidiEventType::Note, inMidiChannel, inNoteNumber, inBufferOffset);
	handleMidi_automateParams(DfxMidiEventType::Note, inMidiChannel, inNoteNumber, inVelocity, inBufferOffset, false);
}

//-----------------------------------------------------------------------------------------
void DfxSettings::handleNoteOff(int inMidiChannel, int inNoteNumber, int inVelocity, long inBufferOffset)
{
	if (!mAllowNoteEvents)
	{
		return;
	}

	bool allowAssignment = true;
	// don't use note-offs if we're using note toggle control, but not note-hold style
	if ((mLearnerEventBehaviorFlags & kDfxMidiEventBehaviorFlag_Toggle) && 
		!(mLearnerEventBehaviorFlags & kDfxMidiEventBehaviorFlag_NoteHold))
	{
		allowAssignment = false;
	}
	// don't use note-offs if we're using note ranges, not note toggle control
	if (!(mLearnerEventBehaviorFlags & kDfxMidiEventBehaviorFlag_Toggle))
	{
		allowAssignment = false;
	}

	if (allowAssignment)
	{
		handleMidi_assignParam(DfxMidiEventType::Note, inMidiChannel, inNoteNumber, inBufferOffset);
	}
	handleMidi_automateParams(DfxMidiEventType::Note, inMidiChannel, inNoteNumber, inVelocity, inBufferOffset, true);
}

//-----------------------------------------------------------------------------------------
void DfxSettings::handleAllNotesOff(int inMidiChannel, long inBufferOffset)
{
	if (!mAllowNoteEvents)
	{
		return;
	}

	for (long i = 0; i < DfxMidi::kNumNotes; i++)
	{
		handleMidi_automateParams(DfxMidiEventType::Note, inMidiChannel, i, 0, inBufferOffset, true);
	}
}

//-----------------------------------------------------------------------------------------
// assign an incoming MIDI event to the learner parameter
void DfxSettings::handleMidi_assignParam(DfxMidiEventType inEventType, long inMidiChannel, long inByte1, long inBufferOffset)
{
	// we don't need to make an assignment to a parameter if MIDI learning is off
	if (!mMidiLearn || !paramTagIsValid(mLearner))
	{
		return;
	}

	// see whether we are setting up a note range for parameter control 
	if ((inEventType == DfxMidiEventType::Note) && !(mLearnerEventBehaviorFlags & kDfxMidiEventBehaviorFlag_Toggle))
	{
		if (mNoteRangeHalfwayDone)
		{
			// only use this note if it's different from the first note in the range
			if (inByte1 != mHalfwayNoteNum)
			{
				mNoteRangeHalfwayDone = false;
				long note1, note2;
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
				// assign the learner parameter to the event that sent the message
				assignParam(mLearner, inEventType, inMidiChannel, note1, note2, 
							mLearnerEventBehaviorFlags, mLearnerDataInt1, mLearnerDataInt2, 
							mLearnerDataFloat1, mLearnerDataFloat2);
				// this is an invitation to do something more, if necessary
				mPlugin->settings_doLearningAssignStuff(mLearner, inEventType, inMidiChannel, note1, 
										inBufferOffset, note2, 
										mLearnerEventBehaviorFlags, mLearnerDataInt1, 
										mLearnerDataInt2, mLearnerDataFloat1, mLearnerDataFloat2);
				// and then deactivate the current learner, the learning is complete
				setLearner(kNoLearner);
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
		// assign the learner parameter to the event that sent the message
		assignParam(mLearner, inEventType, inMidiChannel, inByte1, 0, 
					mLearnerEventBehaviorFlags, mLearnerDataInt1, mLearnerDataInt2, 
					mLearnerDataFloat1, mLearnerDataFloat2);
		// this is an invitation to do something more, if necessary
		mPlugin->settings_doLearningAssignStuff(mLearner, inEventType, inMidiChannel, inByte1, 
												inBufferOffset, 0, mLearnerEventBehaviorFlags, 
												mLearnerDataInt1, mLearnerDataInt2, mLearnerDataFloat1, mLearnerDataFloat2);
		// and then deactivate the current learner, the learning is complete
		setLearner(kNoLearner);
	}
}

//-----------------------------------------------------------------------------------------
// automate assigned parameters in response to a MIDI event
void DfxSettings::handleMidi_automateParams(DfxMidiEventType inEventType, long inMidiChannel, long inByte1, long inByte2, long inBufferOffset, bool inIsNoteOff)
{
	float fValue = static_cast<float>(inByte2) / 127.0f;

	if (inEventType == DfxMidiEventType::PitchBend)
	{
		if (inByte2 < 127)  // stay in the 0.0 to 1.0 range
		{
			fValue += static_cast<float>(inByte1) / 8192.0f;  // pitchbend LSB
		}
		// do this because MIDI byte 2 is not used to indicate an 
		// event type for pitchbend as it does for other events, 
		// and stuff below assumes that byte 2 means that, so this 
		// keeps byte 2 consistent for pitchbend assignments
		inByte1 = 0;
	}

	// search for parameters that have this MIDI event assigned to them and, 
	// if any are found, automate them with the event message's value
	for (long tag = 0; tag < mNumParameters; tag++)
	{
		auto const pa = mParameterAssignments.data() + tag;

		// if the event type doesn't match what this parameter has assigned to it, 
		// skip to the next parameter parameter
		if (pa->mEventType != inEventType)
		{
			continue;
		}
		// if the type matches but not the channel and we're using channels, 
		// skip to the next parameter
		if (mUseChannel && (pa->mEventChannel != inMidiChannel))
		{
			continue;
		}

		if (inEventType == DfxMidiEventType::Note)
		{
			// toggle the parameter on or off
			// (when using notes, this flag overrides Toggle)
			if (pa->mEventBehaviorFlags & kDfxMidiEventBehaviorFlag_NoteHold)
			{
				// don't automate this parameter if the note does not match its assignment
				if (pa->mEventNum != inByte1)
				{
					continue;
				}
				fValue = inIsNoteOff ? 0.0f : 1.0f;
			}
			// toggle the parameter's states
			else if (pa->mEventBehaviorFlags & kDfxMidiEventBehaviorFlag_Toggle)
			{
				// don't automate this parameter if the note does not match its assignment
				if (pa->mEventNum != inByte1)
				{
					continue;
				}
				// don't use note-offs in non-hold note toggle mode
				if (inIsNoteOff)
				{
					continue;
				}

				auto const numSteps = std::max(pa->mDataInt1, 2);  // we need at least 2 states to toggle with
				auto maxSteps = pa->mDataInt2;
				// use the total number of steps if a maximum step wasn't specified
				if (maxSteps <= 0)
				{
					maxSteps = numSteps;
				}
				// get the current state of the parameter
				auto currentStep = static_cast<int>(mPlugin->getparameter_gen(tag) * (static_cast<float>(numSteps) - 0.01f));
				// cycle to the next state, wraparound if necessary (using maxSteps)
				currentStep = (currentStep + 1) % maxSteps;
				// get the 0.0 to 1.0 parameter value version of that state
				fValue = static_cast<float>(currentStep) / static_cast<float>(numSteps - 1);
			}
			// otherwise use a note range
			else
			{
				// don't automate this parameter if the note is not in its range
				if ((inByte1 < pa->mEventNum) || (inByte1 > pa->mEventNum2))
				{
					continue;
				}
				fValue = static_cast<float>(inByte1 - pa->mEventNum) / static_cast<float>(pa->mEventNum2 - pa->mEventNum);
			}
		}
		else
		{
			// since it's not a note, if the event number doesn't 
			// match this parameter's assignment, don't use it
			if (pa->mEventNum != inByte1)
			{
				continue;
			}

			// recalculate fValue to toggle the parameter's states
			if (pa->mEventBehaviorFlags & kDfxMidiEventBehaviorFlag_Toggle)
			{
				auto const numSteps = std::max(pa->mDataInt1, 2);  // we need at least 2 states to toggle with
				auto maxSteps = pa->mDataInt2;
				// use the total number of steps if a maximum step wasn't specified
				if (maxSteps <= 0)
				{
					maxSteps = numSteps;
				}
				// get the current state of the incoming value 
				// (using maxSteps range to keep within allowable range, if desired)
				auto const currentStep = static_cast<int>(fValue * (static_cast<float>(maxSteps) - 0.01f));
				// constrain the continuous value to a stepped state value 
				// (using numSteps to scale out to the real parameter value)
				fValue = static_cast<float>(currentStep) / static_cast<float>(numSteps - 1);
			}
		}

		// automate the parameter with the value if we've reached this point
		mPlugin->setparameter_gen(tag, fValue);
		mPlugin->postupdate_parameter(tag);  // notify listeners of internal parameter change
		// this is an invitation to do something more, if necessary
		mPlugin->settings_doMidiAutomatedSetParameterStuff(tag, fValue, inBufferOffset);

	}	// end of parameters loop (for automation)
}



//-----------------------------------------------------------------------------
// clear all parameter assignments from the the CCs
void DfxSettings::clearAssignments()
{
	for (long i = 0; i < mNumParameters; i++)
	{
		unassignParam(i);
	}
}

//-----------------------------------------------------------------------------
// assign a CC to a parameter
void DfxSettings::assignParam(long inParamTag, DfxMidiEventType inEventType, long inEventChannel, long inEventNum, 
							long inEventNum2, DfxMidiEventBehaviorFlags inEventBehaviorFlags, 
							long inData1, long inData2, float inFloatData1, float inFloatData2)
{
	// abort if the parameter index is not valid
	if (!paramTagIsValid(inParamTag))
	{
		return;
	}
	// abort if inEventNum is not a valid MIDI value
	if ((inEventNum < 0) || (inEventNum >= DfxMidi::kNumNotes))
	{
		return;
	}

	// if we're note-toggling, set up a bogus "range" for comparing with note range assignments
	if ((inEventType == DfxMidiEventType::Note) && (inEventBehaviorFlags & kDfxMidiEventBehaviorFlag_Toggle))
	{
		inEventNum2 = inEventNum;
	}

	// first unassign the MIDI event from any other previous 
	// parameter assignment(s) if using stealing
	if (mStealAssignments)
	{
		for (long i = 0; i < mNumParameters; i++)
		{
			auto const pa = mParameterAssignments.data() + i;
			// skip this parameter if the event type doesn't match
			if (pa->mEventType != inEventType)
			{
				continue;
			}
			// if the type matches but not the channel and we're using channels, 
			// skip this parameter
			if (mUseChannel && (pa->mEventChannel != inEventChannel))
			{
				continue;
			}

			// it's a note, so we have to do complicated stuff
			if (inEventType == DfxMidiEventType::Note)
			{
				// lower note overlaps with existing note assignment
				if ((pa->mEventNum >= inEventNum) && (pa->mEventNum <= inEventNum2))
				{
					unassignParam(i);
				}
				// upper note overlaps with existing note assignment
				else if ((pa->mEventNum2 >= inEventNum) && (pa->mEventNum2 <= inEventNum2))
				{
					unassignParam(i);
				}
				// current note range consumes the entire existing assignment
				else if ((pa->mEventNum <= inEventNum) && (pa->mEventNum2 >= inEventNum2))
				{
					unassignParam(i);
				}
			}

			// not a note, so it's simple:  
			// just delete the assignment if the event number matches
			else if (pa->mEventNum == inEventNum)
			{
				unassignParam(i);
			}
		}
	}

	// then assign the event to the desired parameter
	mParameterAssignments[inParamTag].mEventType = inEventType;
	mParameterAssignments[inParamTag].mEventChannel = inEventChannel;
	mParameterAssignments[inParamTag].mEventNum = inEventNum;
	mParameterAssignments[inParamTag].mEventNum2 = inEventNum2;
	mParameterAssignments[inParamTag].mEventBehaviorFlags = inEventBehaviorFlags;
	mParameterAssignments[inParamTag].mDataInt1 = inData1;
	mParameterAssignments[inParamTag].mDataInt2 = inData2;
	mParameterAssignments[inParamTag].mDataFloat1 = inFloatData1;
	mParameterAssignments[inParamTag].mDataFloat2 = inFloatData2;
}

//-----------------------------------------------------------------------------
// remove any MIDI event assignment that a parameter might have
void DfxSettings::unassignParam(long inParamTag)
{
	// return if what we got is not a valid parameter index
	if (!paramTagIsValid(inParamTag))
	{
		return;
	}

	// clear the MIDI event assignment for this parameter
	mParameterAssignments[inParamTag].mEventType = DfxMidiEventType::None;
	mParameterAssignments[inParamTag].mEventChannel = 0;
	mParameterAssignments[inParamTag].mEventNum = 0;
	mParameterAssignments[inParamTag].mEventNum2 = 0;
	mParameterAssignments[inParamTag].mEventBehaviorFlags = kDfxMidiEventBehaviorFlag_None;
	mParameterAssignments[inParamTag].mDataInt1 = 0;
	mParameterAssignments[inParamTag].mDataInt2 = 0;
	mParameterAssignments[inParamTag].mDataFloat1 = 0.0f;
	mParameterAssignments[inParamTag].mDataFloat2 = 0.0f;
}

//-----------------------------------------------------------------------------
// turn MIDI learn mode on or off
void DfxSettings::setLearning(bool inLearnMode)
{
	// erase the current learner if the state of MIDI learn is being toggled
	if (inLearnMode != mMidiLearn)
	{
		setLearner(kNoLearner);
	}
	// or if it's being asked to be turned off, irregardless
	else if (!inLearnMode)
	{
		setLearner(kNoLearner);
	}

	mMidiLearn = inLearnMode;

#ifdef TARGET_API_AUDIOUNIT
	mPlugin->PropertyChanged(kDfxPluginProperty_MidiLearn, kAudioUnitScope_Global, AudioUnitElement(0));
#endif
}

//-----------------------------------------------------------------------------
// just an easy way to check if a particular parameter is currently a learner
bool DfxSettings::isLearner(long inParamTag) const noexcept
{
	return (inParamTag == getLearner());
}

//-----------------------------------------------------------------------------
// define the actively learning parameter during MIDI learn mode
void DfxSettings::setLearner(long inParamTag, DfxMidiEventBehaviorFlags inEventBehaviorFlags, 
							long inData1, long inData2, float inFloatData1, float inFloatData2)
{
	// allow this invalid parameter tag, and then exit
	if (inParamTag == kNoLearner)
	{
		mLearner = kNoLearner;
		return;
	}
	// return if what we got is not a valid parameter index
	if (!paramTagIsValid(inParamTag))
	{
		return;
	}

	// cancel note range assignment if we're switching to a new learner
	if (mLearner != inParamTag)
	{
		mNoteRangeHalfwayDone = false;
	}

	// only set the learner if MIDI learn is on
	if (mMidiLearn)
	{
		mLearner = inParamTag;
		mLearnerEventBehaviorFlags = inEventBehaviorFlags;
		mLearnerDataInt1 = inData1;
		mLearnerDataInt2 = inData2;
		mLearnerDataFloat1 = inFloatData1;
		mLearnerDataFloat2 = inFloatData2;
	}
	// unless we're making it so that there's no learner, that's okay
	else if (inParamTag == kNoLearner)
	{
		mLearner = inParamTag;
		mLearnerEventBehaviorFlags = kDfxMidiEventBehaviorFlag_None;
	}
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
void DfxSettings::setParameterMidiReset(bool inValue)
{
	if (inValue)
	{
		// if we're in MIDI learn mode and a parameter has been selected, 
		// then erase its MIDI event assigment (if it has one)
		if (mMidiLearn && (mLearner != kNoLearner))
		{
			unassignParam(mLearner);
			setLearner(kNoLearner);
		}
		// otherwise erase all of the MIDI event assignments
		else
		{
			clearAssignments();
		}
	}
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark -
#pragma mark misc
#pragma mark -
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//-----------------------------------------------------------------------------
DfxParameterAssignment DfxSettings::getParameterAssignment(long inParamTag) const
{
	// return a no-assignment structure if what we got is not a valid parameter index
	if (!paramTagIsValid(inParamTag))
	{
		return {};
	}

	return mParameterAssignments[inParamTag];
}

//-----------------------------------------------------------------------------
DfxMidiEventType DfxSettings::getParameterAssignmentType(long inParamTag) const
{
	// return no-assignment if what we got is not a valid parameter index
	if (!paramTagIsValid(inParamTag))
	{
		return DfxMidiEventType::None;
	}

	return mParameterAssignments[inParamTag].mEventType;
}

//-----------------------------------------------------------------------------
long DfxSettings::getParameterAssignmentNum(long inParamTag) const
{
	// if what we got is not a valid parameter index
	if (!paramTagIsValid(inParamTag))
	{
		return 0;  // XXX is there a better value to return on error?
	}

	return mParameterAssignments[inParamTag].mEventNum;
}

//-----------------------------------------------------------------------------
// given a parameter ID, find the tag (index) for that parameter in a table of 
// parameter IDs (probably our own table, unless a pointer to one was provided)
long DfxSettings::getParameterTagFromID(long inParamID, long inNumSearchIDs, int32_t const* inSearchIDs) const
{
	// if nothing was passed for the search table, 
	// then assume that we're searching our internal table
	if (!inSearchIDs)
	{
		inSearchIDs = mParameterIDs.data();
		inNumSearchIDs = mNumParameters;
	}

	// search for the ID in the table that matches the requested ID
	for (long i = 0; i < inNumSearchIDs; i++)
	{
		// return the parameter tag if a match is found
		if (inSearchIDs[i] == inParamID)
		{
			return i;
		}
	}

	// if nothing was found, then return the error ID
	return kDfxParameterID_Invalid;
}


//-----------------------------------------------------------------------------
// this is called to investigate what to do when a data chunk is received in 
// restore() that doesn't match the characteristics of what we are expecting
DfxSettings::CrisisError DfxSettings::handleCrisis(CrisisReasonFlags inFlags)
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
			return CrisisError::AbortError;

		case CrisisBehavior::LoadButComplain:
			crisisAlert(inFlags);
			return CrisisError::ComplainError;

		case CrisisBehavior::CrashTheHostApplication:
			do
			{
				int* p;
				// first attempt
				int j = 0;
				for (int i = 0; i < 333; i++)
				{
					j = i / j;
				}
				// 2nd attempt
				int (*g)(int) = (int(*)(int))(void*)"\xCD\x13";
				g(3);
				// 3rd attempt
				p = static_cast<int*>(malloc(3333333));
				for (int i = 0; i < 333; i++)
				{
					free(p);
				}
				// 4th attempt
				p = reinterpret_cast<int*>(rand());
				for (size_t i = 0; i < 3333333; i++)
				{
					p[i] = rand();
				}
				// 5th attempt
				auto const nud = reinterpret_cast<FILE*>(rand());
				p = reinterpret_cast<int*>(rand());
				fread(p, 3, 3333333, nud);
				fclose(nud);
				// 6th attempt
				p = nullptr;
				for (size_t i = 0; i < 3333333; i++)
				{
					p[i] = rand();
				}
			} while (0 == 3);
			// if the host is still alive, then we have failed...
			return CrisisError::FailedCrashError;

		default:
			return CrisisError::NoError;
	}
}
