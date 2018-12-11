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
// Supply a value for the optional DfxSettings constructor argument 
// sizeofExtendedData.  This should be the size of the extra data 
// (in bytes) of your additional data.  Then implement or override 
// (if inheriting) the saveExtendedData() and restoreExtendedData() 
// methods to take care of your extra data.
//
// If you need to do anything special when parameter settings are restored 
// from particular versions of your plugin (for example, remapping the 
// value to accomodate for a change in range or scaling of the parameter 
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
//
//-------------------------------------------------------------------------
// NOTE July 3rd:  I've recently mucked with this code a lot and added new 
// features with notes, pitchbend, stepped control, limited ranges, etc.  
// I'll document this stuff, later...

#pragma once


#include <vector>

#include "dfxdefines.h"
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
	static constexpr long kNoLearner = -3;  // for when no parameter is activated for learning

	enum class CrisisBehavior
	{
		// crisis behaviors (what to do when restore sends an unexpected buffer size)
		LoadWhatYouCan,
		DontLoad,
		LoadButComplain,
		CrashTheHostApplication
	};


	DfxSettings(long inMagic, DfxPlugin* inPlugin, size_t inSizeofExtendedData = 0);
	virtual ~DfxSettings();


	// - - - - - - - - - API-connect methods - - - - - - - - -

	// for adding to your base plugin class methods
	std::vector<uint8_t> save(bool inIsPreset);
	bool restore(void const* inData, size_t inBufferSize, bool inIsPreset);
#ifdef TARGET_API_AUDIOUNIT
	bool saveMidiAssignmentsToDictionary(CFMutableDictionaryRef inDictionary);
	bool restoreMidiAssignmentsFromDictionary(CFDictionaryRef inDictionary);
#endif

	// handlers for the types of MIDI events that we support
	void handleNoteOn(int inMidiChannel, int inNoteNumber, int inVelocity, unsigned long inOffsetFrames);
	void handleNoteOff(int inMidiChannel, int inNoteNumber, int inVelocity, unsigned long inOffsetFrames);
	void handleAllNotesOff(int inMidiChannel, unsigned long inOffsetFrames);
	void handlePitchBend(int inMidiChannel, int inValueLSB, int inValueMSB, unsigned long inOffsetFrames);
	void handleCC(int inMidiChannel, int inControllerNumber, int inValue, unsigned long inOffsetFrames);


	// - - - - - - - - - MIDI learn - - - - - - - - -

	// deactivate MIDI learn mode
	// call this when your editor window opens and when it closes
	void resetLearning()
	{
		setLearning(false);
	}
	// remove MIDI event assignments from all parameters
	void clearAssignments();
	// assign a MIDI event to a parameter
	void assignParam(long inParamTag, dfx::MidiEventType inEventType, long inEventChannel, 
					 long inEventNum, long inEventNum2 = 0, 
					 dfx::MidiEventBehaviorFlags inEventBehaviorFlags = dfx::kMidiEventBehaviorFlag_None, 
					 long inDataInt1 = 0, long inDataInt2 = 0, 
					 float inDataFloat1 = 0.0f, float inDataFloat2 = 0.0f);
	// remove a parameter's MIDI event assignment
	void unassignParam(long inParamTag);

	// define or report the actively learning parameter during MIDI learn mode
	void setLearner(long inParamTag, dfx::MidiEventBehaviorFlags inEventBehaviorFlags = dfx::kMidiEventBehaviorFlag_None, 
					long inDataInt1 = 0, long inDataInt2 = 0, 
					float inDataFloat1 = 0.0f, float inDataFloat2 = 0.0f);
	auto getLearner() const noexcept
	{
		return mLearner;
	}
	bool isLearner(long inParamTag) const noexcept;

	// turn MIDI learning on or off
	void setLearning(bool inLearnMode);
	// report whether or not MIDI learn mode is active
	auto isLearning() const noexcept
	{
		return mMidiLearn;
	}

	// call these from valueChanged in the plugin editor
	void setParameterMidiLearn(bool inValue);
	void setParameterMidiReset(bool inValue = true);

	// potentially useful accessors
	dfx::ParameterAssignment getParameterAssignment(long inParamTag) const;
	dfx::MidiEventType getParameterAssignmentType(long inParamTag) const;
	long getParameterAssignmentNum(long inParamTag) const;


	// - - - - - - - - - version compatibility management - - - - - - - - -

	// if you set this to something and data is received during restore()
	// which has a version number in its header that's lower than this, 
	// then loading will abort
	void setLowestLoadableVersion(long inVersion) noexcept
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
	void setParameterID(long inParamTag, long inNewID)
	{
		if (paramTagIsValid(inParamTag))
		{
			mParameterIDs[inParamTag] = inNewID;
		}
	}
	long getParameterID(long inParamTag) const
	{
		return (paramTagIsValid(inParamTag)) ? mParameterIDs[inParamTag] : 0;
	}
	long getParameterTagFromID(long inParamID, long inNumSearchIDs = 0, int32_t const* inSearchIDs = nullptr) const;


	// - - - - - - - - - optional settings - - - - - - - - -

	// true means allowing a given MIDI event to be assigned to only one parameter; 
	// false means that a single event can be assigned to more than one parameter
	void setSteal(bool inStealMode) noexcept
	{
		mStealAssignments = inStealMode;
	}
	auto getSteal() const noexcept
	{
		return mStealAssignments;
	}

	// true means that pitchbend events can be assigned to parameters and 
	// used to control those parameters; false means don't use pitchbend like that
	void setAllowPitchbendEvents(bool inNewMode = true) noexcept
	{
		mAllowPitchbendEvents = inNewMode;
	}
	auto getAllowPitchbendEvents() const noexcept
	{
		return mAllowPitchbendEvents;
	}

	// true means that MIDI note events can be assigned to parameters and 
	// used to control those parameters; false means don't use notes like that
	void setAllowNoteEvents(bool inNewMode = true) noexcept
	{
		mAllowNoteEvents = inNewMode;
	}
	auto getAllowNoteEvents() const noexcept
	{
		return mAllowNoteEvents;
	}

	// true means that MIDI channel in events and assignments matters; 
	// false means operate in MIDI omni mode
	void setUseChannel(bool inNewMode = true) noexcept
	{
		mUseChannel = inNewMode;
	}
	auto getUseChannel() const noexcept
	{
		return mUseChannel;
	}

	// this tells DfxSettings what you want it to do if a non-matching 
	// settings data is received in restore()  (see the enum options above)
	void setCrisisBehavior(CrisisBehavior inNewMode) noexcept
	{
		mCrisisBehavior = inNewMode;
	}
	auto getCrisisBehavior() const noexcept
	{
		return mCrisisBehavior;
	}


protected:
	enum class CrisisError
	{
		// crisis-handling errors
		NoError,
		AbortError,
		ComplainError,
		FailedCrashError
	};

	enum
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
	typedef int CrisisReasonFlags;

	// header information for the storage data
	// note:  correctEndian() assumes that the data is all of type 32-bit integer, 
	// so if you change this structure and add different types, 
	// then you will want to modify correctEndian() to handle that.
	// It is also assumed that the first 6 items in this struct 
	// (everything up through mNumStoredPresets) will not change, 
	// so if you alter this header structure at all, keep the 
	// first six items in there and add anything else AFTER those.
	struct SettingsInfo
	{
		// this is a value that you should look at to check for authenticity 
		// of the data and as an identifier of the data's creator 
		// (the easiest thing is probably to use your plugin's ID code)
		int32_t mMagic = 0;
		// the version number of the plugin that is creating the data
		int32_t mVersion = 0;
		// this defaults to 0, but you can set it to be the earliest version 
		// number of your plugin that would be able to handle the storage data
		// (then lower versions of the plugin will fail to load the data)
		int32_t mLowestLoadableVersion = 0;
		// the size (in bytes) of the header data that the plugin 
		// will store in the settings data
		uint32_t mStoredHeaderSize = 0;
		// the number of parameters that the plugin will store in the settings data
		int32_t mNumStoredParameters = 0;
		// the number of presets that the plugin will store in the settings data
		int32_t mNumStoredPresets = 0;
		// the size (in bytes) of each parameter assignment struct 
		// that the plugin will store in the settings data
		uint32_t mStoredParameterAssignmentSize = 0;
		// the size (in bytes) of the extra settings data (if any)
		uint32_t mStoredExtendedDataSize = 0;
	};

	// structure of an API-generic preset
	struct GenPreset
	{
		char mName[dfx::kPresetNameMaxLength];
		float mParameterValues[1];  // can of course be more...
	};

	// reverse the byte order of data
	bool correctEndian(void* ioData, size_t inDataSize, bool inIsReversed, bool inIsPreset = false);

	// investigates what to do when a data is received in 
	// restore() that doesn't match what we are expecting
	CrisisError handleCrisis(CrisisReasonFlags inFlags);
	// can be implemented to display an alert dialogue or something 
	// if CrisisBehavior::LoadButComplain crisis behavior is being used
	virtual void crisisAlert(CrisisReasonFlags /*inFlags*/) {}

	void debugAlertCorruptData(char const* inDataItemName, size_t inDataItemSize, size_t inDataTotalSize);

	// a simple but handy check to see if a parameter tag is valid
	bool paramTagIsValid(long inParamTag) const noexcept
	{
		return (inParamTag >= 0) && (inParamTag < mNumParameters);
	}

	void handleMidi_assignParam(dfx::MidiEventType inEventType, long inMidiChannel, long inByte1, unsigned long inOffsetFrames);
	void handleMidi_automateParams(dfx::MidiEventType inEventType, long inMidiChannel, long inByte1, long inByte2, unsigned long inOffsetFrames, bool inIsNoteOff = false);


	DfxPlugin* const mPlugin;
	long const mNumParameters, mNumPresets;

	bool mMidiLearn = false;  // switch value for MIDI learn mode
	long mLearner = kNoLearner;  // the parameter currently selected for MIDI learning

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
	// It is set with an optional constructor argument (default it 0).
	size_t const mSizeOfExtendedData;

	// the header info for the settings data
	SettingsInfo mSettingsInfo;
	// an ordered table of IDs for each parameter stored in each preset
	// (this is so that non-parameter-compatible plugin versions can load 
	// settings and know which stored parameters correspond to theirs)
	std::vector<int32_t> mParameterIDs;
	// the array of which MIDI event, if any, is assigned to each parameter
	std::vector<dfx::ParameterAssignment> mParameterAssignments;

	// whether to allow only one parameter assignment per MIDI event, or steal them
	bool mStealAssignments = false;
	// whether to allow MIDI note events to be assigned to control parameters
	bool mAllowNoteEvents = false;
	// whether to allow pitchbend events to be assigned to control parameters
	bool mAllowPitchbendEvents = false;
	// what to do if restore() sends data with a mismatched byte size
	CrisisBehavior mCrisisBehavior = CrisisBehavior::LoadWhatYouCan;
	// whether to differentiate events and parameter assignments based 
	// on MIDI channel or whether to ignore channel (omni-style)
	bool mUseChannel = false;

	// this lets the plugin specify any MIDI control behavior characterists 
	// for the current MIDI-learning parameter
	dfx::MidiEventBehaviorFlags mLearnerEventBehaviorFlags = dfx::kMidiEventBehaviorFlag_None;
	// lets the plugin pass along an extra context-specific data bytes
	long mLearnerDataInt1 = 0, mLearnerDataInt2 = 0;
	float mLearnerDataFloat1 = 0.0f, mLearnerDataFloat2 = 0.0f;  // TODO: unused, remove? (but serialized data compatibility)

	// if a note range is being learned for a parameter, this will be true
	bool mNoteRangeHalfwayDone = false;
	// the note that is the first part of the 2-note range being learned
	long mHalfwayNoteNum = 0;
};



/*
Here's what you add to a plugin to make it use DfxSettings and MIDI learn.  
In the header, add these includes:

#include "dfxsettings.h"


Add declarations for these 3 virtual functions:

	virtual long getChunk(void** data, bool isPreset);
	virtual long setChunk(void* data, long byteSize, bool isPreset);
	virtual long processEvents(VstEvents* events);


Add a DfxSettings instance to your plugin class:

	DfxSettings dfxsettings;



Then in the source, add this to the constructor initializer list:

	dfxsettings(PLUGIN_ID, this)


In canDo() add:
	if (strcmp(text, "receiveVstEvents") == 0)
	{
		return 1;
	}
	if (strcmp(text, "receiveVstMidiEvent") == 0)
	{
		return 1;
	}


Also add these functions somewhere, or if they're already implemented, 
add the following stuff into them:

long PLUGIN::getChunk(void** data, bool isPreset)
{	return dfxsettings->save(data, isPreset);	}

long PLUGIN::setChunk(void* data, long byteSize, bool isPreset)
{	return dfxsettings->restore(data, byteSize, isPreset);	}

long PLUGIN::processEvents(VstEvents* events)
{
	dfxsettings->processParameterEvents(events);
	return 1;
}


To resume(), add:

	wantEvents();

*/
