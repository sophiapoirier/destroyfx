// Destroy FX plugin data storage stuff by Sophia Poirier  ][  April-July + October 2002
//
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

#ifndef __DFXSETTINGS_H
#define __DFXSETTINGS_H


#include "dfxdefines.h"

#ifdef TARGET_API_AUDIOUNIT
	#include <CoreFoundation/CoreFoundation.h>
#endif


//------------------------------------------------------
// some constants
enum
{
	// . . . MIDI learn stuff . . .

	kNoLearner = -3,	// for when no parameter is activated for learning
	kNumMidiValues = 128,	// the number of values for MIDI events

	// MIDI event types
	kParamEventNone = 0,	// for when you haven't assigned an event to a parameter
	kParamEventCC,	// CC assignment
	kParamEventPitchbend,	// pitchbend wheel assignment
	kParamEventNote,	// note assignment

	// parameter automation behaviour mode flags
	//
	// use MIDI events to toggle the associated parameter between 0.0 and 1.0 
	// if no data1 value was specified, 
	// (good for controlling on/off buttons)
	// but if data1 is greater than 2, then MIDI events will toggle between 
	// paramater's states.  note-ons will cycle through the values and 
	// continuous MIDI events will move through the values in steps
	// (good for switches)
	// (if using notes events and this flag is not set, and neither is NoteHold, 
	// then note ranges are used to control associated parameters)
	kEventBehaviourToggle = 1,
	// send 1.0 on note on and 0.0 on note off if this flag is on, 
	// otherwise toggle 1.0 and 0.0 at each note on
	// (only relevent when using notes events)
	// (overrides Toggle setting for notes events, but not other events)
	kEventBehaviourNoteHold = 1 << 1,
	// use a range other than 0.0 to 1.0 
	// the range is defined by fdata1 and fdata2
	kEventBehaviourRange = 1 << 2,



	// . . . load crisis stuff . . .

	// crisis behaviours (what to do when restore sends an unexpected buffer size)
	kDfxSettingsCrisis_LoadWhatYouCan = 0,
	kDfxSettingsCrisis_DontLoad,
	kDfxSettingsCrisis_LoadButComplain,
	kDfxSettingsCrisis_CrashTheHostApplication,

	// crisis-handling errors
	kDfxSettingsCrisis_NoError = 0,
	kDfxSettingsCrisis_AbortError,
	kDfxSettingsCrisis_ComplainError,
	kDfxSettingsCrisis_FailedCrashError,

	// crisis situation flags
	kDfxSettingsCrisis_MismatchedMagic		= 1,		// the magic signatures don't match
	kDfxSettingsCrisis_SmallerByteSize		= 1 << 1,	// the incoming data size is smaller
	kDfxSettingsCrisis_LargerByteSize		= 1 << 2,	// the incoming data size is larger
	kDfxSettingsCrisis_FewerParameters		= 1 << 3,	// the incoming data has fewer parameters
	kDfxSettingsCrisis_MoreParameters		= 1 << 4,	// the incoming data has more parameters
	kDfxSettingsCrisis_FewerPresets			= 1 << 5,	// the incoming data has fewer presets
	kDfxSettingsCrisis_MorePresets			= 1 << 6,	// the incoming data has more presets
	kDfxSettingsCrisis_LowerVersion			= 1 << 7,	// the incoming data has a lower version number
	kDfxSettingsCrisis_HigherVersion		= 1 << 8,	// the incoming data has a higher version number
	kDfxSettingsCrisis_VersionIsTooLow		= 1 << 9,	// the incoming data's version number is lower than our lowest loadable version number
	kDfxSettingsCrisis_VersionIsTooHigh		= 1 << 10,	// our version number is lower than the incoming data's lowest loadable version number
	kDfxSettingsCrisis_SmallerHeader		= 1 << 11,	// the incoming data's header size is smaller
	kDfxSettingsCrisis_LargerHeader			= 1 << 12	// the incoming data's header size is larger
};


//------------------------------------------------------
// structure of an API-generic preset
typedef struct
{
	char name[DFX_PRESET_MAX_NAME_LENGTH];
	float params[2];	// can of course be more than 2...
} DfxGenPreset;


//------------------------------------------------------
// header information for the storage data
// note:  correctEndian() assumes that the data is all of type long, 
// so if you change this structure and add different types, 
// then you will want to modify correctEndian() to handle that.
// It is also assumed that the first 6 items in this struct 
// (everything up through numStoredPresets) will not change, 
// so if you alter this header structure at all, keep the 
// first six items in there and add anything else AFTER those.
typedef struct
{
	// this is a value that you should look at to check for authenticity 
	// of the data and as an identifier of the data's creator 
	// (the easiest thing is probably to use your plugin's "unique ID")
	long magic;
	// the version number of the plugin that is creating the data
	long version;
	// this defaults to 0, but you can set it to be the earliest version 
	// number of your plugin that would be able to handle the storage data
	// (then lower versions of the plugin will fail to load the data)
	long lowestLoadableVersion;
	// the size (in bytes) of the header data that the plugin 
	// will store in the settings data
	unsigned long storedHeaderSize;
	// the number of parameters that the plugin will store in the settings data
	long numStoredParameters;
	// the number of presets that the plugin will store in the settings data
	long numStoredPresets;
	// the size (in bytes) of each parameter assignment struct 
	// that the plugin will store in the settings data
	unsigned long storedParameterAssignmentSize;
	// the size (in bytes) of the extra settings data (if any)
	unsigned long storedExtendedDataSize;
} DfxSettingsInfo;


//------------------------------------------------------
typedef struct
{
	// the MIDI event type 
	//    (CC, note, pitchbend, etc.)
	long eventType;
	// the MIDI channel of the MIDI event assignment
	//    (so far, I'm not using channel information for anything)
	long eventChannel;
	// the number of the MIDI event assigned to the parameter 
	//    (CC number, note number, etc.)
	long eventNum;
	// a second MIDI event number for double-value assignments 
	//    (like 2 notes defining a note range)
	long eventNum2;
	// indicating the behaviour of the event, i.e. toggle vs. hold for notes, etc.
	long eventBehaviourFlags;
	// bonus data slots
	// (context-specific)
	// (like for the number of steps in an indexed toggle assignment)
	long data1;
	// (or the maximum step, within that range, to cycle up to)
	long data2;
	// (or the minimum point in a float range)
	float fdata1;
	// (or the maximum point in a float range)
	float fdata2;
} DfxParameterAssignment;


//------------------------------------------------------
// this reverses the bytes in a stream of data, for correcting endian difference
inline void reversebytes(void * inData, unsigned long inItemSize, unsigned long inItemCount = 1)
{
	unsigned long half = (inItemSize / 2) + (inItemSize % 2);
	char * dataBytes = (char*)inData;

	for (unsigned long c=0; c < inItemCount; c++)
	{
		for (unsigned long i=0; i < half; i++)
		{
			char temp = dataBytes[i];
			unsigned long complementIndex = (inItemSize - 1) - i;
			dataBytes[i] = dataBytes[complementIndex];
			dataBytes[complementIndex] = temp;
		}
		dataBytes += inItemSize;
	}
}


//------------------------------------------------------
// this interprets a UNIX environment variable string as a boolean
bool getenvBool(const char * inVarName, bool inFallbackValue);


//------------------------------------------------------
class DfxSettings
{
public:
	DfxSettings(long inMagic, DfxPlugin * inPlugin, unsigned long inSizeofExtendedData = 0);
	~DfxSettings();


	// - - - - - - - - - API-connect methods - - - - - - - - -

	// for adding to your base plugin class methods
	unsigned long save(void ** outData, bool inIsPreset);
	bool restore(void * inData, unsigned long inBufferSize, bool inIsPreset);
#ifdef TARGET_API_AUDIOUNIT
	bool saveMidiAssignmentsToDictionary(CFMutableDictionaryRef inDictionary);
	bool restoreMidiAssignmentsFromDictionary(CFDictionaryRef inDictionary);
#endif

	// handlers for the types of MIDI events that we support
	void handleNoteOn(int inMidiChannel, int inNoteNumber, int inVelocity, long inBufferOffset);
	void handleNoteOff(int inMidiChannel, int inNoteNumber, int inVelocity, long inBufferOffset);
	void handlePitchBend(int inMidiChannel, int inValueLSB, int inValueMSB, long inBufferOffset);
	void handleCC(int inMidiChannel, int inControllerNumber, int inValue, long inBufferOffset);


	// - - - - - - - - - MIDI learn - - - - - - - - -

	// deactivate MIDI learn mode
	// call this when your editor window opens and when it closes
	void resetLearning()
		{	setLearning(false);	}
	// remove MIDI event assignments from all parameters
	void clearAssignments();
	// assign a MIDI event to a parameter
	void assignParam(long inTag, long inEventType, long inEventChannel, 
								long inEventNum, long inEventNum2 = 0, 
								long inEventBehaviourFlags = 0, 
								long inData1 = 0, long inData2 = 0, 
								float inFloatData1 = 0.0f, float inFloatData2 = 0.0f);
	// remove a parameter's MIDI event assignment
	void unassignParam(long inTag);

	// define or report the actively learning parameter during MIDI learn mode
	void setLearner(long inTag, long inEventBehaviourFlags = 0, 
							long inData1 = 0, long inData2 = 0, 
							float inFloatData1 = 0.0f, float inFloatData2 = 0.0f);
	long getLearner()
		{	return learner;	}
	bool isLearner(long inTag);

	// turn MIDI learning on or off
	void setLearning(bool inLearnMode);
	// report whether or not MIDI learn mode is active
	bool isLearning()
		{	return midiLearn;	}

	// call these from valueChanged in the plugin editor
	void setParameterMidiLearn(bool inValue);
	void setParameterMidiReset(bool inValue = true);

	// potentially useful accessors
	long getParameterAssignmentType(long inParamTag);
	long getParameterAssignmentNum(long inParamTag);


	// - - - - - - - - - version compatibility management - - - - - - - - -

	// if you set this to something and data is received during restore()
	// which has a version number in its header that's lower than this, 
	// then loading will abort
	void setLowestLoadableVersion(long inVersion)
		{	settingsInfo.lowestLoadableVersion = inVersion;	}
	long getLowestLoadableVersion()
		{	return settingsInfo.lowestLoadableVersion;	}

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
	void setParameterID(long inTag, long inNewID)
		{	if (paramTagIsValid(inTag)) parameterIDs[inTag] = inNewID;	}
	long getParameterID(long inTag)
		{	if (paramTagIsValid(inTag)) return parameterIDs[inTag]; else return 0;	}
	long getParameterTagFromID(long paramID, long inNumSearchIDs = 0, long * inSearchIDs = NULL);


	// - - - - - - - - - optional settings - - - - - - - - -

	// true means allowing a given MIDI event to be assigned to only one parameter; 
	// false means that a single event can be assigned to more than one parameter
	void setSteal(bool inStealMode)
		{	stealAssignments = inStealMode;	}
	bool getSteal()
		{	return stealAssignments;	}

	// true means that pitchbend events can be assigned to parameters and 
	// used to control those parameters; false means don't use pitchbend like that
	void setAllowPitchbendEvents(bool inNewMode = true)
		{	allowPitchbendEvents = inNewMode;	}
	bool getAllowPitchbendEvents()
		{	return allowPitchbendEvents;	}

	// true means that MIDI note events can be assigned to parameters and 
	// used to control those parameters; false means don't use notes like that
	void setAllowNoteEvents(bool inNewMode = true)
		{	allowNoteEvents = inNewMode;	}
	bool getAllowNoteEvents()
		{	return allowNoteEvents;	}

	// true means that MIDI channel in events and assignments matters; 
	// false means operate in MIDI omni mode
	void setUseChannel(bool inNewMode = true)
		{	useChannel = inNewMode;	}
	bool getUseChannel()
		{	return useChannel;	}

	// this tells DfxSettings what you want it to do if a non-matching 
	// settings data is received in restore()   (see the enum options above)
	void setCrisisBehaviour(bool inNewMode)
		{	crisisBehaviour = inNewMode;	}
	long getCrisisBehaviour()
		{	return crisisBehaviour;	}


protected:
	// reverse the byte order of data
	void correctEndian(void * inData, bool isReversed, bool inIsPreset = false);

	// investigates what to do when a data is received in 
	// restore() that doesn't match what we are expecting
	long handleCrisis(long inFlags);
	// can be implemented to display an alert dialogue or something 
	// if kDfxSettingsCrisis_LoadButComplain crisis behaviour is being used
	void crisisAlert(long inFlags)
		{ }

	// a simple but handy check to see if a parameter tag is valid
	bool paramTagIsValid(long inTag)
		{	return ( (inTag >= 0) && (inTag < numParameters) );	}

	void handleMidi_assignParam(long inEventType, long inMidiChannel, long inByte1, long inBufferOffset);
	void handleMidi_automateParams(long inEventType, long inMidiChannel, long inByte1, long inByte2, long inBufferOffset, bool inIsNoteOff = false);


	long numParameters, numPresets;
	DfxPlugin * plugin;

	bool midiLearn;	// switch value for MIDI learn mode
	long learner;	// the parameter currently selected for MIDI learning

	// size of one preset (32-char preset name + all parameter values)
	unsigned long sizeofPreset;
	// size of the table of parameter IDs (one for each parameter)
	unsigned long sizeofParameterIDs;
	// size of the single-preset "preset" version of the settings data
	unsigned long sizeofPresetChunk;
	// size of the entire settings data (entire bank, not preset)
	unsigned long sizeofChunk;

	// This is how much larger (beyond the regular stuff) the settings data 
	// memory allocation should be (for extending the basic data stuff).
	// It is set with an optional constructor argument (default it 0).
	unsigned long sizeofExtendedData;

	// the header info for the settings data
	DfxSettingsInfo settingsInfo;
	// an ordered table of IDs for each parameter stored in each preset
	// (this is so that non-parameter-compatible plugin versions can load 
	// settings and know which stored parameters correspond to theirs)
	long * parameterIDs;
	// the array of which MIDI event, if any, is assigned to each parameter
	DfxParameterAssignment * paramAssignments;

	// this what we point the host to during save()
	DfxSettingsInfo * sharedChunk;
	// a few handy pointers into sections of our settings data
	long * firstSharedParameterID;
	DfxGenPreset * firstSharedPreset;
	DfxParameterAssignment * firstSharedParamAssignment;

	// whether to allow only one parameter assignment per MIDI event, or steal them
	bool stealAssignments;
	// whether to allow MIDI note events to be assigned to control parameters
	bool allowNoteEvents;
	// whether to allow pitchbend events to be assigned to control parameters
	bool allowPitchbendEvents;
	// what to do if restore() sends data with a mismatched byte size
	long crisisBehaviour;
	// whether to differentiate events and parameter assignments based 
	// on MIDI channel or whether to ignore channel (omni-style)
	bool useChannel;

	// this lets the plugin specify any MIDI control behaviour characterists 
	// for the current MIDI-learning parameter
	long learnerEventBehaviourFlags;
	// let's the plugin pass along an extra context-specific data bytes
	long learnerData1, learnerData2;
	float learnerFData1, learnerFData2;

	// if a note range is being learned for a parameter, this will be true
	bool noteRangeHalfwayDone;
	// the note that is the first part of the 2-note range being learned
	long halfwayNoteNum;
};


#endif
// __DFXSETTINGS_H



/*
Here's what you add to a plugin to make it use DfxSettings and MIDI learn.  
In the header, add these includes:

#include "dfxsettings.h"


Add declarations for these 3 virtual functions:

	virtual long getChunk(void ** data, bool isPreset);
	virtual long setChunk(void * data, long byteSize, bool isPreset);
	virtual long processEvents(VstEvents* events);


Add a DfxSettings pointer to your plugin class:

	DfxSettings * dfxsettings;



Then in the source, make this call in the constructor:

	dfxsettings = new DfxSettings(PLUGIN_ID, this);


In the destructor add:

	if (dfxsettings)
		delete dfxsettings;


In canDo() add:
	if (strcmp(text, "receiveVstEvents") == 0)
		return 1;
	if (strcmp(text, "receiveVstMidiEvent") == 0)
		return 1;


Also add these functions somewhere, or if they're already implemented, 
add the following stuff into them:

long PLUGIN::getChunk(void ** data, bool isPreset)
{	return dfxsettings->save(data, isPreset);	}

long PLUGIN::setChunk(void * data, long byteSize, bool isPreset)
{	return dfxsettings->restore(data, byteSize, isPreset);	}

long PLUGIN::processEvents(VstEvents* events)
{
	dfxsettings->processParameterEvents(events);
	return 1;
}


To resume(), add:

	wantEvents();

*/
