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
This is our Destroy FX plugin data storage stuff
------------------------------------------------------------------------*/

// This is a class, some functions, and other relevant stuff for dealing 
// with Destroy FX plugin settings data.
//
// My basic purpose for using the storage feature is for storing MIDI event 
// assignments for parameter modulation.  Rather than double the number of 
// regular parameters and have a MIDI event number parameter that 
// corresponds to each functional parameter, I decided to use special data 
// storage.  
//
// This has several benefits:
//	*	For one thing, you can store ints rather than using floats and 
//		scaling and casting to int (which is fudgy).
//	*	Also, if you randomize parameters in your plugin host software, 
//		you won't randomize MIDI event assignments, as that would most 
//		likely not be a desirable result of randomization.
//	*	You can also assign ID numbers to each parameter so that if you 
//		add or remove or rearrange parameters in later versions of your 
//		plugin, you can still map the new parameters to the old set and 
//		therefore load old settings into new versions of your plugin and 
//		vice versa (parameter IDs are possible in Audio Unit but not VST).
//
//
// If you have other stuff that you need to store as special data aside 
// from MIDI event assignments, it is quite easy to extend this class.  
// Supply the size of the extra data (in bytes) of your additional data 
// by overriding settings_sizeOfExtendedData.  Then override the 
// settings_saveExtendedData and settings_restoreExtendedData methods 
// to handle your extra data.
//
// If you need to do anything special when parameter settings are restored 
// from particular versions of your plugin (for example, remapping the 
// value to accommodate for a change in range or scaling of the parameter 
// in a more recent version of your plugin), I've provided a hook for 
// extending parameter restoration during restore().  Just take care of 
// your special cases in doChunkRestoreSetParameterStuff().
// 
// If you want to do any extra stuff when handling MIDI event assignment 
// and automation, I've provided a couple of hooks from the event 
// processing routine in order to make that easy:
// doLearningAssignStuff() is called during processParameterEvents() 
// whenever a MIDI event is assigned to a parameter via MIDI learn.
// doSetParameterStuff() is called during processParameterEvents() 
// whenever an incoming MIDI event automates a parameter to which it is 
// assigned.

#pragma once


#include <atomic>
#include <bit>
#include <cstddef>
#include <span>
#include <type_traits>
#include <vector>

#include "dfx-base.h"
#include "dfxpluginproperties.h"

#ifdef TARGET_API_AUDIOUNIT
	#include <CoreFoundation/CoreFoundation.h>
#endif


//------------------------------------------------------
class DfxPlugin;



//------------------------------------------------------
class DfxSettings
{
public:
	enum class CrisisBehavior
	{
		// crisis behaviors (what to do when restore sends an unexpected buffer size)
		LoadWhatYouCan,
		DontLoad,
		LoadButComplain,
		CrashTheHostApplication
	};

	using CrisisReasonFlags = unsigned int;
	enum : CrisisReasonFlags
	{
		// crisis situation flags
		kCrisisReasonFlag_None				= 0,
		kCrisisReasonFlag_MismatchedMagic	= 1,		// the magic signatures don't match
		kCrisisReasonFlag_SmallerByteSize	= 1 << 1,	// the incoming data size is smaller
		kCrisisReasonFlag_LargerByteSize	= 1 << 2,	// the incoming data size is larger
		kCrisisReasonFlag_FewerParameters	= 1 << 3,	// the incoming data has fewer parameters
		kCrisisReasonFlag_MoreParameters	= 1 << 4,	// the incoming data has more parameters
		kCrisisReasonFlag_FewerPresets		= 1 << 5,	// the incoming data has fewer presets
		kCrisisReasonFlag_MorePresets		= 1 << 6,	// the incoming data has more presets
		kCrisisReasonFlag_LowerVersion		= 1 << 7,	// the incoming data has a lower version number
		kCrisisReasonFlag_HigherVersion		= 1 << 8,	// the incoming data has a higher version number
		kCrisisReasonFlag_VersionIsTooLow	= 1 << 9,	// the incoming data's version number is lower than our lowest loadable version number
		kCrisisReasonFlag_VersionIsTooHigh	= 1 << 10,	// our version number is lower than the incoming data's lowest loadable version number
		kCrisisReasonFlag_SmallerHeader		= 1 << 11,	// the incoming data's header size is smaller
		kCrisisReasonFlag_LargerHeader		= 1 << 12	// the incoming data's header size is larger
	};

#if TARGET_OS_MAC
	static inline CFStringRef const kDfxDataAUClassInfoKeyString = CFSTR("destroyfx-data");
#endif


	DfxSettings(uint32_t inMagic, DfxPlugin& inPlugin, size_t inSizeofExtendedData);


	// - - - - - - - - - plugin API glue methods - - - - - - - - -

	// for adding to your base plugin class methods
	[[nodiscard]] std::vector<std::byte> save(bool inIsPreset) const;
	[[nodiscard]] bool restore(void const* inData, size_t inDataSize, bool inIsPreset);
	[[nodiscard]] bool minimalValidate(void const* inData, size_t inDataSize) const noexcept;

#if TARGET_PLUGIN_USES_MIDI
#ifdef TARGET_API_AUDIOUNIT
	[[nodiscard]] bool saveMidiAssignmentsToDictionary(CFMutableDictionaryRef inDictionary) const;
	[[nodiscard]] bool restoreMidiAssignmentsFromDictionary(CFDictionaryRef inDictionary);
#endif

	// handlers for the types of MIDI events that we support
	void handleNoteOn(int inMidiChannel, int inNoteNumber, int inVelocity, size_t inOffsetFrames);
	void handleNoteOff(int inMidiChannel, int inNoteNumber, int inVelocity, size_t inOffsetFrames);
	void handleAllNotesOff(int inMidiChannel, size_t inOffsetFrames);
	void handleChannelAftertouch(int inMidiChannel, int inValue, size_t inOffsetFrames);
	void handlePitchBend(int inMidiChannel, int inValueLSB, int inValueMSB, size_t inOffsetFrames);
	void handleCC(int inMidiChannel, int inControllerNumber, int inValue, size_t inOffsetFrames);


	// - - - - - - - - - MIDI learn - - - - - - - - -

	// remove MIDI event assignments from all parameters
	void clearAssignments();
	// assign a MIDI event to a parameter
	void assignParameter(dfx::ParameterID inParameterID, dfx::MidiEventType inEventType, int inEventChannel, 
						 int inEventNum, int inEventNum2 = 0, 
						 dfx::MidiEventBehaviorFlags inEventBehaviorFlags = dfx::kMidiEventBehaviorFlag_None, 
						 int inDataInt1 = 0, int inDataInt2 = 0, 
						 float inDataFloat1 = 0.0f, float inDataFloat2 = 0.0f);
	// remove a parameter's MIDI event assignment
	void unassignParameter(dfx::ParameterID inParameterID);

	// define or report the actively learning parameter during MIDI learn mode
	void setLearner(dfx::ParameterID inParameterID, dfx::MidiEventBehaviorFlags inEventBehaviorFlags = dfx::kMidiEventBehaviorFlag_None, 
					int inDataInt1 = 0, int inDataInt2 = 0, 
					float inDataFloat1 = 0.0f, float inDataFloat2 = 0.0f);
	auto getLearner() const noexcept
	{
		return mLearner.load();
	}

	// turn MIDI learning on or off
	void setLearning(bool inLearnMode);
	// report whether or not MIDI learn mode is active
	auto isLearning() const noexcept
	{
		return mMidiLearn.load();
	}

	// call these from valueChanged in the plugin editor
	void setParameterMidiLearn(bool inValue);
	void setParameterMidiReset();

	// potentially useful accessors
	dfx::ParameterAssignment getParameterAssignment(dfx::ParameterID inParameterID) const;
	dfx::MidiEventType getParameterAssignmentType(dfx::ParameterID inParameterID) const;
#endif // TARGET_PLUGIN_USES_MIDI


	// - - - - - - - - - version compatibility management - - - - - - - - -

	// if you set this to something and data is received during restore()
	// which has a version number in its header that is lower than this, 
	// then loading will fail
	void setLowestLoadableVersion(unsigned int inVersion) noexcept
	{
		mSettingsInfo.mLowestLoadableVersion = inVersion;
	}
	auto getLowestLoadableVersion() const noexcept
	{
		return mSettingsInfo.mLowestLoadableVersion;
	}

	// This stuff manages the parameter IDs.  Each parameter has an ID number 
	// so that, if you later add parameters and their tags are inserted within 
	// the old set (rather than tacked on at the end of the old order), 
	// or you remove parameters, or you switch around their order, 
	// then you can just make sure to keep the ID for each old parameter 
	// the same so that old versions of the plugin can load new settings and 
	// vice versa.  By default, a parameter ID table is constructed in which 
	// the ID for each parameter is the same as the parameter's tag/index.  
	// If your plugin's parameter IDs need to be different than that, then 
	// you use these functions to manage the IDs and change them if necessary.
	// If you make any changes, they should really be done immediately after 
	// creating the DfxSettings object, or at least before your plugin's 
	// constructor returns, because you don't want any set or save calls 
	// made before you have your parameter ID map finalized.
	void setMappedParameterID(dfx::ParameterID inParameterIndex, dfx::ParameterID inMappedParameterID)
	{
		if (isValidParameterID(inParameterIndex))
		{
			mParameterIDMap[inParameterIndex] = inMappedParameterID;
		}
	}
	dfx::ParameterID getMappedParameterID(dfx::ParameterID inParameterIndex) const
	{
		return (isValidParameterID(inParameterIndex)) ? mParameterIDMap[inParameterIndex] : dfx::kParameterID_Invalid;
	}


	// - - - - - - - - - optional settings - - - - - - - - -

#if TARGET_PLUGIN_USES_MIDI
	// true means allowing a given MIDI event to be assigned to only one parameter; 
	// false means that a single event can be assigned to more than one parameter
	void setSteal(bool inMode) noexcept;
	auto getSteal() const noexcept
	{
		return mStealAssignments.load();
	}

	void setDeactivateLearningUponLearnt(bool inMode) noexcept
	{
		mDeactivateLearningUponLearnt = inMode;
	}
	auto getDeactivateLearningUponLearnt() const noexcept
	{
		return mDeactivateLearningUponLearnt.load();
	}

	void setAllowChannelAftertouchEvents(bool inMode = true) noexcept
	{
		mAllowChannelAftertouchEvents = inMode;
	}
	auto getAllowChannelAftertouchEvents() const noexcept
	{
		return mAllowChannelAftertouchEvents;
	}

	// true means that pitchbend events can be assigned to parameters and 
	// used to control those parameters; false means don't use pitchbend like that
	void setAllowPitchbendEvents(bool inMode = true) noexcept
	{
		mAllowPitchbendEvents = inMode;
	}
	auto getAllowPitchbendEvents() const noexcept
	{
		return mAllowPitchbendEvents;
	}

	// true means that MIDI note events can be assigned to parameters and 
	// used to control those parameters; false means don't use notes like that
	void setAllowNoteEvents(bool inMode = true) noexcept
	{
		mAllowNoteEvents = inMode;
	}
	auto getAllowNoteEvents() const noexcept
	{
		return mAllowNoteEvents;
	}

	// true means that MIDI channel in events and assignments matters; 
	// false means operate in MIDI omni mode
	void setUseChannel(bool inMode) noexcept;
	auto getUseChannel() const noexcept
	{
		return mUseChannel.load();
	}
#endif // TARGET_PLUGIN_USES_MIDI

	// this tells DfxSettings what you want it to do if a non-matching 
	// settings data is received in restore()  (see the enum options above)
	void setCrisisBehavior(CrisisBehavior inMode) noexcept
	{
		mCrisisBehavior = inMode;
	}
	auto getCrisisBehavior() const noexcept
	{
		return mCrisisBehavior;
	}

	static consteval bool serializationIsNativeEndian() noexcept
	{
		return std::endian::native == std::endian::big;
	}


private:
	enum class CrisisError
	{
		// crisis-handling errors
		NoError,
		QuitError,
		ComplainError,
		FailedCrashError
	};

	enum GlobalBehaviorFlags : uint32_t
	{
		kGlobalBehaviorFlag_UseChannel = 1,
		kGlobalBehaviorFlag_StealAssignments = 1 << 1
	};

#pragma pack(push, 4)
	// header information for the storage data
	// note:  correctEndian() assumes that the data is all of type 32-bit integer, 
	// so if you change this structure and add different types, 
	// then you will want to modify correctEndian() to handle that.
	// It is also assumed that the first six items in this struct 
	// (everything up through mNumStoredPresets) will not change, 
	// so if you alter this header structure at all, keep the 
	// first six items in there and add anything else AFTER those.
	struct SettingsInfo
	{
		// this is a value that you should look at to check for authenticity 
		// of the data and as an identifier of the data's creator 
		// (the easiest thing is probably to use your plugin's ID code)
		uint32_t mMagic = 0;
		// the version number of the plugin that is creating the data
		uint32_t mVersion = 0;
		// this defaults to 0, but you can set it to be the earliest version 
		// number of your plugin that would be able to handle the storage data
		// (then lower versions of the plugin will fail to load the data)
		uint32_t mLowestLoadableVersion = 0;
		// the size (in bytes) of the header data that the plugin 
		// will store in the settings data
		uint32_t mStoredHeaderSize = 0;
		// the number of parameters that the plugin will store in the settings data
		uint32_t mNumStoredParameters = 0;
		// the number of presets that the plugin will store in the settings data
		uint32_t mNumStoredPresets = 0;
		// the size (in bytes) of each parameter assignment struct 
		// that the plugin will store in the settings data
		uint32_t mStoredParameterAssignmentSize = 0;
		// the size (in bytes) of the extra settings data (if any)
		uint32_t mStoredExtendedDataSize = 0;
		// behaviors that are global to plugin operation
		uint32_t mGlobalBehaviorFlags = 0;
	};
	static_assert(dfx::IsTriviallySerializable<SettingsInfo>);

	// structure of an API-generic preset
	struct GenPreset
	{
		char mName[dfx::kPresetNameMaxLength] {};  // must include null-terminator byte
		float mParameterValues[1] {};  // can of course be more...
	};
	static_assert(dfx::IsTriviallySerializable<GenPreset>);
#pragma pack(pop)
	// some extracted type definitions for convenience during byte meddling
	using GenPresetNameT = decltype(GenPreset::mName);
	using GenPresetNameElementT = std::remove_extent_t<GenPresetNameT>;
	static_assert(!std::is_same_v<GenPresetNameT, GenPresetNameElementT>);
	static_assert(sizeof(GenPresetNameElementT) == 1);  // allows assumption of no serialized memory alignment concerns
	using GenPresetParameterValueT = std::remove_extent_t<decltype(GenPreset::mParameterValues)>;
	static_assert(!std::is_same_v<decltype(GenPreset::mParameterValues), GenPresetParameterValueT>);

	// reverse the byte order of data
	[[nodiscard]] bool correctEndian(void* ioData, size_t inDataSize, bool inIsReversed, bool inIsPreset) const;

	// investigates what to do when a data is received in 
	// restore() that doesn't match what we are expecting
	CrisisError handleCrisis(CrisisReasonFlags inFlags) const;

	void validateRange(void const* inData, size_t inDataSize, void const* inAddress, size_t inAddressSize, char const* inDataItemName) const;
	void debugAlertCorruptData(char const* inDataItemName, size_t inDataItemSize, size_t inDataTotalSize) const;

	// a simple but handy check to see if a parameter tag is valid
	bool isValidParameterID(dfx::ParameterID inParameterID) const noexcept
	{
		return (inParameterID < mNumParameters);
	}

	static dfx::ParameterID getParameterIndexFromMap(dfx::ParameterID inParameterID, std::span<uint32_t const> inSearchIDs);
	dfx::ParameterID getParameterIndexFromMap(dfx::ParameterID inParameterID) const;

	static size_t sizeOfGenPreset(size_t inParameterCount) noexcept;

#if TARGET_PLUGIN_USES_MIDI
	void handleMidi_assignParameter(dfx::MidiEventType inEventType, int inMidiChannel, int inByte1, size_t inOffsetFrames);
	void handleMidi_automateParameters(dfx::MidiEventType inEventType, int inMidiChannel, int inByte1, int inByte2, size_t inOffsetFrames, bool inIsNoteOn = false);
#endif // TARGET_PLUGIN_USES_MIDI


	DfxPlugin& mPlugin;
	size_t const mNumParameters, mNumPresets;

	// size of one preset (preset name + all parameter values)
	size_t mSizeOfPreset = 0;
	// size of the table of parameter IDs (one for each parameter)
	size_t mSizeOfParameterIDs = 0;
	// size of the single-preset "preset" version of the settings data
	size_t mSizeOfPresetChunk = 0;
	// size of the entire settings data (entire bank, not preset)
	size_t mSizeOfChunk = 0;

	// This is how much larger (beyond the regular stuff) the settings data 
	// memory allocation should be (for extending the basic data stuff).
	// It is set with a constructor argument.
	size_t const mSizeOfExtendedData;

	// the header info for the settings data
	SettingsInfo mSettingsInfo;
	// an ordered table of IDs for each parameter stored in each preset
	// (this is so that non-parameter-compatible plugin versions can load 
	// settings and know which stored parameters correspond to theirs)
	std::vector<uint32_t> mParameterIDMap;

	// what to do if restore() sends data with a mismatched byte size
	CrisisBehavior mCrisisBehavior = CrisisBehavior::LoadWhatYouCan;

#if TARGET_PLUGIN_USES_MIDI
	std::atomic<bool> mMidiLearn {false};  // switch value for MIDI learn mode
	std::atomic<dfx::ParameterID> mLearner {dfx::kParameterID_Invalid};  // the parameter currently selected for MIDI learning

	// the array of which MIDI event, if any, is assigned to each parameter
	std::vector<dfx::ParameterAssignment> mParameterAssignments;

	// whether to allow only one parameter assignment per MIDI event, or steal them
	std::atomic<bool> mStealAssignments {false};
	std::atomic<bool> mDeactivateLearningUponLearnt {true};
	bool mAllowChannelAftertouchEvents = true;
	// whether to allow pitchbend events to be assigned to control parameters
	bool mAllowPitchbendEvents = false;
	// whether to allow MIDI note events to be assigned to control parameters
	bool mAllowNoteEvents = false;
	// whether to differentiate events and parameter assignments based 
	// on MIDI channel or whether to ignore channel (omni-style)
	std::atomic<bool> mUseChannel {false};

	// this lets the plugin specify any MIDI control behavior characterists 
	// for the current MIDI-learning parameter
	dfx::MidiEventBehaviorFlags mLearnerEventBehaviorFlags = dfx::kMidiEventBehaviorFlag_None;
	// lets the plugin pass along an extra context-specific data bytes
	std::atomic<int> mLearnerDataInt1 {0}, mLearnerDataInt2 {0};
	std::atomic<float> mLearnerDataFloat1 {0.0f}, mLearnerDataFloat2 {0.0f};  // TODO: unused, remove? (but serialized data compatibility)

	// if a note range is being learned for a parameter, this will be true
	std::atomic<bool> mNoteRangeHalfwayDone {false};
	// the note that is the first part of the 2-note range being learned
	std::atomic<int> mHalfwayNoteNum {0};
#endif // TARGET_PLUGIN_USES_MIDI
};
