/*------------- by Marc Poirier  ][  April-June 2002 ------------*/

#ifndef __DFXSETTINGS_H
#include "dfxsettings.h"
#endif

#ifndef __DFXPLUGIN_H
#include "dfxplugin.h"
#endif

#include <stdlib.h>
#include <stdio.h>


// XXX finish support for loading old pre-DfxPlugin settings
#ifndef DFX_SUPPORT_OLD_VST_SETTINGS
#define DFX_SUPPORT_OLD_VST_SETTINGS 0
#endif

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark _________init/destroy_________
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//-----------------------------------------------------------------------------
DfxSettings::DfxSettings(long magic, DfxPlugin *plugin, unsigned long sizeofExtendedData)
:	plugin(plugin), sizeofExtendedData(sizeofExtendedData)
{
	sharedChunk = 0;
	paramAssignments = 0;
	parameterIDs = 0;

	// there's nothing we can do without a pointer back to the plugin
	if (plugin == NULL)
		return;

	numParameters = plugin->getnumparameters();
	numPresets = plugin->getnumpresets();

	if (numPresets < 1)
		numPresets = 1;	// we do need at least 1 set of parameters
	if (numParameters < 1)
		numParameters = 1;	// come on now, what are you trying to do?

	paramAssignments = (ParameterAssignment*) malloc(numParameters * sizeof(ParameterAssignment));
	parameterIDs = (long*) malloc(numParameters * sizeof(long));

	// default to each parameter having its ID equal its index
	// (I haven't implemented anything with parameter IDs yet)
	for (long i=0; i < numParameters; i++)
		parameterIDs[i] = i;

	// calculate some data sizes that are useful to know
	sizeofPreset = sizeof(GenPreset) + (sizeof(float) * (numParameters-2));
	sizeofParameterIDs = sizeof(long) * numParameters;
	sizeofPresetChunk = sizeofPreset 			// 1 preset
						+ sizeof(DfxSettingsInfo) 	// the special data header info
						+ sizeofParameterIDs	// the table of parameter IDs
						+ (sizeof(ParameterAssignment)*numParameters);	// the MIDI events assignment array
	sizeofChunk = (sizeofPreset*numPresets)		// all of the presets
					+ sizeof(DfxSettingsInfo)		// the special data header info
					+ sizeofParameterIDs			// the table of parameter IDs
					+ (sizeof(ParameterAssignment)*numParameters);	// the MIDI events assignment array

	// increase the allocation sizes if extra data must be stored
	sizeofChunk += sizeofExtendedData;
	sizeofPresetChunk += sizeofExtendedData;

	// this is the shared data that we point **data to in save()
	sharedChunk = (DfxSettingsInfo*) malloc(sizeofChunk);
	// and a few pointers to elements within that data, just for ease of use
	firstSharedParameterID = (long*) ((char*)sharedChunk + sizeof(DfxSettingsInfo));
	firstSharedPreset = (GenPreset*) ((char*)firstSharedParameterID + sizeofParameterIDs);
	firstSharedParamAssignment = (ParameterAssignment*) 
									((char*)firstSharedPreset + (sizeofPreset*numPresets));

	// set all of the header infos
	settingsInfo.magic = magic;
	settingsInfo.version = plugin->getpluginversion();
	settingsInfo.lowestLoadableVersion = 0;
	settingsInfo.storedHeaderSize = sizeof(DfxSettingsInfo);
	settingsInfo.numStoredParameters = numParameters;
	settingsInfo.numStoredPresets = numPresets;
	settingsInfo.storedParameterAssignmentSize = sizeof(ParameterAssignment);
	settingsInfo.storedExtendedDataSize = sizeofExtendedData;

	clearAssignments();	// initialize all of the parameters to have no MIDI event assignments
	resetLearning();	// start with MIDI learn mode off

	// default to allowing MIDI event assignment sharing instead of stealing them, 
	// unless the user has defined the environment variable DFX_PARAM_STEALMIDI
	stealAssignments = getenvBool("DFX_PARAM_STEALMIDI", false);

	// default to ignoring MIDI channel in MIDI event assignments and automation, 
	// unless the user has defined the environment variable DFX_PARAM_USECHANNEL
	useChannel = getenvBool("DFX_PARAM_USECHANNEL", false);

	// default to not allowing MIDI note or pitchbend events to be assigned to parameters
	allowNoteEvents = false;
	allowPitchbendEvents = false;

	noteRangeHalfwayDone = false;

	// default to trying to load un-matching chunks
	crisisBehaviour = kCrisisLoadWhatYouCan;

	// allow for further constructor stuff, if necessary
	init();
}

//-----------------------------------------------------------------------------
DfxSettings::~DfxSettings()
{
	// wipe out the signature
	settingsInfo.magic = 0;

	// deallocate memories
	if (paramAssignments)
		free(paramAssignments);
	paramAssignments = 0;

	if (parameterIDs)
		free(parameterIDs);
	parameterIDs = 0;

	if (sharedChunk)
		free(sharedChunk);
	sharedChunk = 0;

	// allow for further destructor stuff, if necessary
	uninit();
}

//------------------------------------------------------
// this interprets a UNIX environment variable string as a boolean
bool getenvBool(const char *var, bool def)
{
	const char *env = getenv(var);

	// return the default value if the getenv failed
	if (env == NULL)
		return def;

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
			return def;
	}
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark _________settings_________
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//-----------------------------------------------------------------------------
// This gets called when the host wants to save settings data, 
// like when saving a song or preset files.
unsigned long DfxSettings::save(void **data, bool isPreset)
{
  long i, j;


	if ( (sharedChunk == NULL) || (plugin == NULL) )
	{
		*data = 0;
		return 1;
	}

	// share with the host
	*data = sharedChunk;

	// first store the special chunk infos
	sharedChunk->magic = settingsInfo.magic;
	sharedChunk->version = settingsInfo.version;
	sharedChunk->lowestLoadableVersion = settingsInfo.lowestLoadableVersion;
	sharedChunk->storedHeaderSize = settingsInfo.storedHeaderSize;
	sharedChunk->numStoredParameters = settingsInfo.numStoredParameters;
	sharedChunk->numStoredPresets = (isPreset) ? 1 : settingsInfo.numStoredPresets;
	sharedChunk->storedParameterAssignmentSize = settingsInfo.storedParameterAssignmentSize;
	sharedChunk->storedExtendedDataSize = settingsInfo.storedExtendedDataSize;

	// store the parameters' IDs
	for (i=0; i < numParameters; i++)
		firstSharedParameterID[i] = parameterIDs[i];

	// store only one preset setting if isPreset is true
	if (isPreset)
	{
		plugin->getpresetname(plugin->getcurrentpresetnum(), firstSharedPreset->name);
		for (i=0; i < numParameters; i++)
			firstSharedPreset->params[i] = plugin->getparameter_f(i);

		ParameterAssignment *tempSharedParamAssignment = (ParameterAssignment*) ((char*)firstSharedPreset + sizeofPreset);
		// store the parameters' MIDI event assignments
		for (i=0; i < numParameters; i++)
			tempSharedParamAssignment[i] = paramAssignments[i];

		// reverse the order of bytes in the data being sent to the host, if necessary
		correctEndian(sharedChunk, false, isPreset);
		// allow for the storage of extra data
		saveExtendedData((char*)sharedChunk+sizeofPresetChunk-sizeofExtendedData, isPreset);

		return sizeofPresetChunk;
	}

	// otherwise store the entire bank of presets and the MIDI event assignments
	else
	{
		GenPreset *tempSharedPresets = firstSharedPreset;
		for (j=0; j < numPresets; j++)
		{
			// copy the preset name to the chunk
			plugin->getpresetname(j, tempSharedPresets->name);
			// copy all of the parameters for this preset to the chunk
			for (i=0; i < numParameters; i++)
				tempSharedPresets->params[i] = plugin->getpresetparameter_f(j, i);
			// point to the next preset in the data array for the host
			tempSharedPresets = (GenPreset*) ((char*)tempSharedPresets + sizeofPreset);
		}

		// store the parameters' MIDI event assignments
		for (i=0; i < numParameters; i++)
			firstSharedParamAssignment[i] = paramAssignments[i];

		// reverse the order of bytes in the data being sent to the host, if necessary
		correctEndian(sharedChunk, false, isPreset);
		// allow for the storage of extra data
		saveExtendedData((char*)sharedChunk+sizeofChunk-sizeofExtendedData, isPreset);

		return sizeofChunk;
	}
}


//-----------------------------------------------------------------------------
// this gets called when the host wants to load settings data, 
// like when restoring settings while opening a song, 
// or loading a preset file
bool DfxSettings::restore(void *data, unsigned long byteSize, bool isPreset)
{
  DfxSettingsInfo *newSettingsInfo;
  GenPreset *newPreset;
  long *newParameterIDs;
  long i, j;


	if (plugin == NULL)
		return false;

	// un-reverse the order of bytes in the received data, if necessary
	correctEndian(data, true, isPreset);

	// point to the start of the chunk data:  the settingsInfo header
	newSettingsInfo = (DfxSettingsInfo*)data;

	// The following situations are basically considered to be 
	// irrecoverable "crisis" situations.  Regardless of what 
	// crisisBehaviour has been chosen, any of the following errors 
	// will prompt an unsuccessful exit because these are big problems.  
	// Incorrect magic signature basically means that these settings are 
	// probably for some other plugin.  And the whole point of setting a 
	// lowestLoadableVersion value is that it should be taken seriously.
	if (newSettingsInfo->magic != settingsInfo.magic)
		return false;
	if ( (newSettingsInfo->version < settingsInfo.lowestLoadableVersion) || 
			 (settingsInfo.version < newSettingsInfo->lowestLoadableVersion) )
		return false;

#if DFX_SUPPORT_OLD_VST_SETTINGS
	// we started using hex format versions (like below) with the advent 
	// of the glorious DfxPlugin
	// versions lower than 0x00010000 indicate inferior settings
	bool oldvst = IS_OLD_VST_VERSION(newSettingsInfo->version);
#endif

	// these just make the values easier to work with (no need for newSettingsInfo-> so often)
	long numStoredParameters = newSettingsInfo->numStoredParameters;
	long numStoredPresets = newSettingsInfo->numStoredPresets;
	unsigned long storedHeaderSize = newSettingsInfo->storedHeaderSize;

	// figure out how many presets we should try to load 
	// if the incoming chunk doesn't match what we're expecting
	long copyPresets = (numStoredPresets < numPresets) ? numStoredPresets : numPresets;
	// irregardless, only restore one preset if we're loading a single preset
	if (isPreset)
		copyPresets = 1;
	// figure out how much of the ParameterAssignment structure we can import
	unsigned long copyParameterAssignmentSize = (newSettingsInfo->storedParameterAssignmentSize < settingsInfo.storedParameterAssignmentSize) ? 
									newSettingsInfo->storedParameterAssignmentSize : settingsInfo.storedParameterAssignmentSize;

	// check for conflicts and keep track of them
	long crisisFlags = 0;
	if (newSettingsInfo->version < settingsInfo.version)
		crisisFlags = (crisisFlags | kCrisisLowerVersion);
	else if (newSettingsInfo->version > settingsInfo.version)
		crisisFlags = (crisisFlags | kCrisisHigherVersion);
	if (numStoredParameters < numParameters)
		crisisFlags = (crisisFlags | kCrisisFewerParameters);
	else if (numStoredParameters > numParameters)
		crisisFlags = (crisisFlags | kCrisisMoreParameters);
	if (isPreset)
	{
		if (byteSize < sizeofPresetChunk)
			crisisFlags = (crisisFlags | kCrisisSmallerByteSize);
		else if (byteSize > sizeofPresetChunk)
			crisisFlags = (crisisFlags | kCrisisLargerByteSize);
	}
	else
	{
		if (byteSize < sizeofChunk)
			crisisFlags = (crisisFlags | kCrisisSmallerByteSize);
		else if (byteSize > sizeofChunk)
			crisisFlags = (crisisFlags | kCrisisLargerByteSize);
		if (numStoredPresets < numPresets)
			crisisFlags = (crisisFlags | kCrisisFewerPresets);
		else if (numStoredPresets > numPresets)
			crisisFlags = (crisisFlags | kCrisisMorePresets);
	}
	// handle the crisis situations (if any) and abort loading if we're told to
	if (handleCrisis(crisisFlags) == kCrisisAbortError)
		return false;

	// point to the next data element after the chunk header:  the first parameter ID
	newParameterIDs = (long*) ((char*)newSettingsInfo + storedHeaderSize);
	// create a mapping table for corresponding the incoming parameters to the 
	// destination parameters (in case the parameter IDs don't all match up)
	//  [ the index of paramMap is the same as our parameter tag/index and the value 
	//     is the tag/index of the incoming parameter that corresponds, if any ]
	long *paramMap = (long*) malloc(numParameters * sizeof(long));
	for (long tag=0; tag < numParameters; tag++)
		paramMap[tag] = getParameterTagFromID(parameterIDs[tag], numStoredParameters, newParameterIDs);

	// point to the next data element after the parameter IDs:  the first preset name
	newPreset = (GenPreset*) ((char*)newParameterIDs + (sizeof(long)*numStoredParameters));
	// handy for incrementing the data pointer
	unsigned long sizeofStoredPreset = sizeof(GenPreset) + (sizeof(float)*(numStoredParameters-2));

	// the chunk being received only contains one preset
	if (isPreset)
	{
		// in Audio Unit, this is handled already in AUBase::RestoreState, 
		// and we are not really loading a "preset,"
		// we are restoring the last user state
		#if !TARGET_API_AUDIOUNIT
		// copy the preset name from the chunk
		plugin->setpresetname(plugin->getcurrentpresetnum(), newPreset->name);
		#endif
	#if DFX_SUPPORT_OLD_VST_SETTINGS
		// back up the pointer to account for shorter preset names
		if (oldvst)
			newPreset = (GenPreset*) ((char*)newPreset + (OLD_PRESET_MAX_NAME_LENGTH - DFX_PRESET_MAX_NAME_LENGTH));
	#endif
		// copy all of the parameters that we can for this preset from the chunk
		for (i=0; i < numParameters; i++)
		{
			long mappedTag = paramMap[i];
			if (mappedTag != kInvalidParamTag)
			{
			#if DFX_SUPPORT_OLD_VST_SETTINGS
				// handle old-style generic VST 0.0 to 1.0 parameter values
				if (oldvst)
					plugin->setparameter_gen(i, newPreset->params[mappedTag]);
				else
			#endif
				plugin->setparameter_f(i, newPreset->params[mappedTag]);
				// allow for additional tweaking of the stored parameter setting
				doChunkRestoreSetParameterStuff(i, newPreset->params[mappedTag], newSettingsInfo->version);
			}
		}
	}

	// the chunk being received has all of the presets plus the MIDI event assignments
	else
	{
		// we're loading an entire bank of presets plus the MIDI event assignments, 
		// so cycle through all of the presets and load them up, as many as we can
		for (j=0; j < copyPresets; j++)
		{
			// copy the preset name from the chunk
			plugin->setpresetname(j, newPreset->name);
		#if DFX_SUPPORT_OLD_VST_SETTINGS
			// back up the pointer to account for shorter preset names
			if (oldvst)
				newPreset = (GenPreset*) ((char*)newPreset + (OLD_PRESET_MAX_NAME_LENGTH - DFX_PRESET_MAX_NAME_LENGTH));
		#endif
			// copy all of the parameters that we can for this preset from the chunk
			for (i=0; i < numParameters; i++)
			{
				long mappedTag = paramMap[i];
				if (mappedTag != kInvalidParamTag)
				{
				#if DFX_SUPPORT_OLD_VST_SETTINGS
					// handle old-style generic VST 0.0 to 1.0 parameter values
					if (oldvst)
						plugin->setpresetparameter_gen(j, i, newPreset->params[mappedTag]);
					else
				#endif
					plugin->setpresetparameter_f(j, i, newPreset->params[mappedTag]);
					// allow for additional tweaking of the stored parameter setting
					doChunkRestoreSetParameterStuff(i, newPreset->params[mappedTag], newSettingsInfo->version, j);
				}
			}
			// point to the next preset in the received data array
			newPreset = (GenPreset*) ((char*)newPreset + sizeofStoredPreset);
		}
	}

#if DFX_SUPPORT_OLD_VST_SETTINGS
if ( !(oldvst && isPreset) )
{
#endif
	// completely clear our table of parameter assignments before loading the new 
	// table since the new one might not have all of the data members
	memset(paramAssignments, 0, sizeof(ParameterAssignment)*numParameters);
	// then point to the last chunk data element, the MIDI event assignment array
	// (offset by the number of stored presets that were skipped, if any)
	ParameterAssignment *newParamAssignments;
	if (isPreset)
		newParamAssignments = (ParameterAssignment*) ((char*)newPreset + sizeofStoredPreset);
	else
		newParamAssignments = (ParameterAssignment*) ((char*)newPreset + 
								((numStoredPresets-copyPresets) * sizeofStoredPreset));
	// and load up as many of them as we can
	for (i=0; i < numParameters; i++)
	{
		long mappedTag = paramMap[i];
		if (mappedTag != kInvalidParamTag)
			memcpy( &(paramAssignments[i]), 
					(char*)newParamAssignments+(mappedTag*(newSettingsInfo->storedParameterAssignmentSize)), 
					copyParameterAssignmentSize);
//			paramAssignments[i] = newParamAssignments[mappedTag];
	}
#if DFX_SUPPORT_OLD_VST_SETTINGS
}
#endif

	// allow for the retrieval of extra data
	restoreExtendedData((char*)data+sizeofChunk-newSettingsInfo->storedExtendedDataSize, 
						newSettingsInfo->storedExtendedDataSize, newSettingsInfo->version, isPreset);

	if (paramMap)
		free(paramMap);

	return true;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark _________MIDI_learn_________
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//-----------------------------------------------------------------------------------------
void DfxSettings::handleCC(int channel, int controllerNum, int value, long frameOffset)
{
	// don't allow the "all notes off" CC because almost every sequencer uses that when playback stops
	if (controllerNum == 0x7B)
		return;

	handleMidi_assignParam(kParamEventCC, channel, controllerNum, frameOffset);
	handleMidi_automateParams(kParamEventCC, channel, controllerNum, value, frameOffset);
}

//-----------------------------------------------------------------------------------------
void DfxSettings::handlePitchBend(int channel, int valueLSB, int valueMSB, long frameOffset)
{
	if (!allowPitchbendEvents)
		return;

	// do this because MIDI byte 2 is not used to indicate an 
	// event type for pitchbend as it does for other events, 
	// and stuff below assumes that byte 2 means that, so this 
	// keeps byte 2 consistent for pitchbend assignments
	int realLSB = valueLSB;
	valueLSB = 0;	// <- XXX this is stoopid

	handleMidi_assignParam(kParamEventPitchbend, channel, valueLSB, frameOffset);

	valueLSB = realLSB;	// restore it   <- XXX ugh stupid hackz...
	handleMidi_automateParams(kParamEventPitchbend, channel, valueLSB, valueMSB, frameOffset);
}

//-----------------------------------------------------------------------------------------
void DfxSettings::handleNoteOn(int channel, int note, int velocity, long frameOffset)
{
	if (!allowNoteEvents)
		return;

	handleMidi_assignParam(kParamEventNote, channel, note, frameOffset);
	handleMidi_automateParams(kParamEventNote, channel, note, velocity, frameOffset, false);
}

//-----------------------------------------------------------------------------------------
void DfxSettings::handleNoteOff(int channel, int note, int velocity, long frameOffset)
{
	if (!allowNoteEvents)
		return;

	bool allowAssignment = true;
	// don't use note-offs if we're using note toggle control, but not note-hold style
	if ( (learnerEventBehaviourFlags & kEventBehaviourToggle) && 
			!(learnerEventBehaviourFlags & kEventBehaviourNoteHold) )
		allowAssignment = false;
	// don't use note-offs if we're using note ranges, not note toggle control
	if ( !(learnerEventBehaviourFlags & kEventBehaviourToggle) )
		allowAssignment = false;

	if (allowAssignment)
		handleMidi_assignParam(kParamEventNote, channel, note, frameOffset);
	handleMidi_automateParams(kParamEventNote, channel, note, velocity, frameOffset, true);
}

//-----------------------------------------------------------------------------------------
// assign an incoming MIDI event to the learner parameter
void DfxSettings::handleMidi_assignParam(long eventType, long channel, long byte1, long frameOffset)
{
	// we don't need to make an assignment to a parameter if MIDI learning is off
	if ( !midiLearn || !paramTagIsValid(learner) )
		return;

	// see whether we are setting up a note range for parameter control 
	if ( (eventType == kParamEventNote) && 
		 !(learnerEventBehaviourFlags & kEventBehaviourToggle) )
	{
		if (noteRangeHalfwayDone)
		{
			// only use this note if it's different from the first note in the range
			if (byte1 != halfwayNoteNum)
			{
				noteRangeHalfwayDone = false;
				long note1, note2;
				if (byte1 > halfwayNoteNum)
				{
					note1 = halfwayNoteNum;
					note2 = byte1;
				}
				else
				{
					note1 = byte1;
					note2 = halfwayNoteNum;
				}
				// assign the learner parameter to the event that sent the message
				assignParam(learner, eventType, channel, note1, note2, 
							learnerEventBehaviourFlags, learnerData1, learnerData2, 
							learnerFData1, learnerFData2);
				// this is an invitation to do something more, if necessary
				doLearningAssignStuff(learner, eventType, channel, note1, 
										frameOffset, note2, 
										learnerEventBehaviourFlags, learnerData1, 
										learnerData2, learnerFData1, learnerFData2);
				// and then deactivate the current learner, the learning is complete
				setLearner(kNoLearner);
			}
		}
		else
		{
			noteRangeHalfwayDone = true;
			halfwayNoteNum = byte1;
		}
	}
	else
	{
		// assign the learner parameter to the event that sent the message
		assignParam(learner, eventType, channel, byte1, 0, 
					learnerEventBehaviourFlags, learnerData1, learnerData2, 
					learnerFData1, learnerFData2);
		// this is an invitation to do something more, if necessary
		doLearningAssignStuff(learner, eventType, channel, byte1, 
								frameOffset, 0, learnerEventBehaviourFlags, 
								learnerData1, learnerData2, learnerFData1, learnerFData2);
		// and then deactivate the current learner, the learning is complete
		setLearner(kNoLearner);
	}
}

//-----------------------------------------------------------------------------------------
// automate assigned parameters in response to a MIDI event
void DfxSettings::handleMidi_automateParams(long eventType, long channel, long byte1, long byte2, long frameOffset, bool isNoteOff)
{
	float fValue = (float)byte2 / 127.0f;

	if (eventType == kParamEventPitchbend)
	{
		if (byte2 < 127)	// stay in the 0.0 to 1.0 range
			fValue += (float)byte1 / 8192.0f;	// pitchbend LSB
		// do this because MIDI byte 2 is not used to indicate an 
		// event type for pitchbend as it does for other events, 
		// and stuff below assumes that byte 2 means that, so this 
		// keeps byte 2 consistent for pitchbend assignments
		byte1 = 0;
	}

	// search for parameters that have this MIDI event assigned to them and, 
	// if any are found, automate them with the event message's value
	for (long tag = 0; tag < numParameters; tag++)
	{
		ParameterAssignment *pa = &(paramAssignments[tag]);

		// if the event type doesn't match what this parameter has assigned to it, 
		// skip to the next parameter parameter
		if (pa->eventType != eventType)
			continue;
		// if the type matches but not the channel and we're using channels, 
		// skip to the next parameter
		if ( useChannel && (pa->eventChannel != channel) )
			continue;

		if (eventType == kParamEventNote)
		{
			// toggle the parameter on or off
			// (when using notes, this flag overrides Toggle)
			if (pa->eventBehaviourFlags & kEventBehaviourNoteHold)
			{
				// don't automate this parameter if the note does not match its assignment
				if (pa->eventNum != byte1)
					continue;
				if (isNoteOff)
					fValue = 0.0f;
				else
					fValue = 1.0f;
			}
			// toggle the parameter's states
			else if (pa->eventBehaviourFlags & kEventBehaviourToggle)
			{
				// don't automate this parameter if the note does not match its assignment
				if (pa->eventNum != byte1)
					continue;
				// don't use note-offs in non-hold note toggle mode
				if (isNoteOff)
					continue;

				long numSteps = pa->data1;
				long maxSteps = pa->data2;
				// we need at least 2 states to toggle with
				if (numSteps < 2)
					numSteps = 2;
				// use the total number of steps if a maximum step wasn't specified
				if (maxSteps <= 0)
					maxSteps = numSteps;
				// get the current state of the parameter
				long currentStep = (long) (plugin->getparameter_gen(tag) * ((float)numSteps-0.01f));
				// cycle to the next state, wraparound if necessary (using maxSteps)
				currentStep = (currentStep+1) % maxSteps;
				// get the 0.0 to 1.0 parameter value version of that state
				fValue = (float)currentStep / (float)(numSteps - 1);
			}
			// otherwise use a note range
			else
			{
				// don't automate this parameter if the note is not in its range
				if ( (byte1 < pa->eventNum) || (byte1 > pa->eventNum2) )
					continue;
				fValue = (float)(byte1 - pa->eventNum) / 
							(float)(pa->eventNum2 - pa->eventNum);
			}
		}
		else
		{
			// since it's not a note, if the event number doesn't 
			// match this parameter's assignment, don't use it
			if (pa->eventNum != byte1)
				continue;

			// recalculate fValue to toggle the parameter's states
			if (pa->eventBehaviourFlags & kEventBehaviourToggle)
			{
				int numSteps = pa->data1;
				int maxSteps = pa->data2;
				// we need at least 2 states to toggle with
				if (numSteps < 2)
					numSteps = 2;
				// use the total number of steps if a maximum step wasn't specified
				if (maxSteps <= 0)
					maxSteps = numSteps;
				// get the current state of the incoming value 
				// (using maxSteps range to keep within allowable range, if desired)
				int currentStep = (int) (fValue * ((float)maxSteps-0.01f));
				// constrain the continuous value to a stepped state value 
				// (using numSteps to scale out to the real parameter value)
				fValue = (float)currentStep / (float)(numSteps - 1);
			}
		}

		// automate the parameter with the value if we've reached this point
		plugin->setparameter_gen(tag, fValue);
		// this is an invitation to do something more, if necessary
		doMidiAutomatedSetParameterStuff(tag, fValue, frameOffset);

	}	// end of parameters loop (for automation)
}



//-----------------------------------------------------------------------------
// clear all parameter assignments from the the CCs
void DfxSettings::clearAssignments()
{
	for (long i=0; i < numParameters; i++)
	{
		paramAssignments[i].eventType = kParamEventNone;
		paramAssignments[i].eventChannel = 0;
		paramAssignments[i].eventBehaviourFlags = 0;
		paramAssignments[i].data1 = 0;
		paramAssignments[i].data2 = 0;
		paramAssignments[i].fdata1 = 0.0f;
		paramAssignments[i].fdata2 = 0.0f;
	}
}

//-----------------------------------------------------------------------------
// assign a CC to a parameter
void DfxSettings::assignParam(long tag, long eventType, long eventChannel, long eventNum, 
							long eventNum2, long eventBehaviourFlags, 
							long data1, long data2, float fdata1, float fdata2)
{
	// abort if the parameter index is not valid
	if (paramTagIsValid(tag) == false)
		return;
	// abort if the eventNum is not a valid MIDI value
	if ( (eventNum < 0) || (eventNum >= kNumMidiValues) )
		return;

	// if we're note-toggling, set up a bogus "range" for comparing with note range assignments
	if ( (eventType == kParamEventNote) && (eventBehaviourFlags & kEventBehaviourToggle) )
		eventNum2 = eventNum;

	// first unassign the MIDI event from any other previous 
	// parameter assignment(s) if using stealing
	if (stealAssignments)
	{
		for (long i=0; i < numParameters; i++)
		{
			ParameterAssignment *pa = &(paramAssignments[i]);
			// skip this parameter if the event type doesn't match
			if (pa->eventType != eventType)
				continue;
			// if the type matches but not the channel and we're using channels, 
			// skip this parameter
			if ( useChannel && (pa->eventChannel != eventChannel) )
				continue;

			// it's a note, so we have to do complicated stuff
			if (eventType == kParamEventNote)
			{
				// lower note overlaps with existing note assignment
				if ( (pa->eventNum >= eventNum) && (pa->eventNum <= eventNum2) )
					unassignParam(i);
				// upper note overlaps with existing note assignment
				else if ( (pa->eventNum2 >= eventNum) && (pa->eventNum2 <= eventNum2) )
					unassignParam(i);
				// current note range consumes the entire existing assignment
				else if ( (pa->eventNum <= eventNum) && (pa->eventNum2 >= eventNum2) )
					unassignParam(i);
			}

			// note a note, so it's simple:  
			// just delete the assignment if the event number matches
			else if (pa->eventNum == eventNum)
				unassignParam(i);
		}
	}

	// then assign the event to the desired parameter
	paramAssignments[tag].eventType = eventType;
	paramAssignments[tag].eventChannel = eventChannel;
	paramAssignments[tag].eventNum = eventNum;
	paramAssignments[tag].eventNum2 = eventNum2;
	paramAssignments[tag].eventBehaviourFlags = eventBehaviourFlags;
	paramAssignments[tag].data1 = data1;
	paramAssignments[tag].data2 = data2;
	paramAssignments[tag].fdata1 = fdata1;
	paramAssignments[tag].fdata2 = fdata2;
}

//-----------------------------------------------------------------------------
// remove any MIDI event assignment that a parameter might have
void DfxSettings::unassignParam(long tag)
{
	// return if what we got is not a valid parameter index
	if (paramTagIsValid(tag) == false)
		return;

	// clear the MIDI event assignment for this parameter
	paramAssignments[tag].eventType = kParamEventNone;
	paramAssignments[tag].eventBehaviourFlags = 0;
}

//-----------------------------------------------------------------------------
// turn MIDI learn mode on or off
void DfxSettings::setLearning(bool newLearn)
{
	// erase the current learner if the state of MIDI learn is being toggled
	if (newLearn != midiLearn)
		setLearner(kNoLearner);
	// or if it's being asked to be turned off, irregardless
	else if (newLearn == false)
		setLearner(kNoLearner);

	midiLearn = newLearn;
}

//-----------------------------------------------------------------------------
// define the actively learning parameter during MIDI learn mode
void DfxSettings::setLearner(long tag, long eventBehaviourFlags, 
							long data1, long data2, float fdata1, float fdata2)
{
	// cancel note range assignment if we're switching to a new learner
	if (learner != tag)
		noteRangeHalfwayDone = false;

	// only set the learner if MIDI learn is on
	if (midiLearn)
	{
		learner = tag;
		learnerEventBehaviourFlags = eventBehaviourFlags;
		learnerData1 = data1;
		learnerData2 = data2;
		learnerFData1 = fdata1;
		learnerFData2 = fdata2;
	}
	// unless we're making it so that there's no learner, that's okay
	else if (tag == kNoLearner)
	{
		learner = tag;
		learnerEventBehaviourFlags = 0;
	}
}

//-----------------------------------------------------------------------------
// a plugin editor should call this during valueChanged from a control 
// to turn MIDI learning on and off, VST parameter style
void DfxSettings::setParameterMidiLearn(float value)
{
	setLearning(value > 0.5f);
}

//-----------------------------------------------------------------------------
// a plugin editor should call this during valueChanged from a control 
// to clear MIDI event assignments, VST parameter style
void DfxSettings::setParameterMidiReset(float value)
{
	if (value > 0.5f)
	{
		// if we're in MIDI learn mode and a parameter has been selected, 
		// then erase its MIDI event assigment (if it has one)
		if ( midiLearn && (learner != kNoLearner) )
		{
			unassignParam(learner);
			setLearner(kNoLearner);
		}
		// otherwise erase all of the MIDI event assignments
		else
			clearAssignments();
	}
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark _________misc_________
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//-----------------------------------------------------------------------------
// given a parameter ID, find the tag (index) for that parameter in a table of 
// parameter IDs (probably our own table, unless a pointer to one was provided)
long DfxSettings::getParameterTagFromID(long paramID, long numSearchIDs, long *searchIDs)
{
	// if nothing was passed for the search table, 
	// then assume that we're searching our internal table
	if (searchIDs == NULL)
	{
		searchIDs = parameterIDs;
		numSearchIDs = numParameters;
	}

	// search for the ID in the table that matches the requested ID
	for (long i=0; i < numSearchIDs; i++)
	{
		// return the parameter tag if a match is found
		if (searchIDs[i] == paramID)
			return i;
	}

	// if nothing was found, then return the error tag
	return kInvalidParamTag;
}


//-----------------------------------------------------------------------------
// this is called to investigate what to do when a data chunk is received in 
// restore() that doesn't match the characteristics of what we are expecting
long DfxSettings::handleCrisis(long flags)
{
	// no need to continue on if there is no crisis situation
	if (flags == 0)
		return kCrisisNoError;

	switch (crisisBehaviour)
	{
		case kCrisisLoadWhatYouCan:
			return kCrisisNoError;
			break;

		case kCrisisDontLoad:
			return kCrisisAbortError;
			break;

		case kCrisisLoadButComplain:
			crisisAlert(flags);
			return kCrisisComplainError;
			break;

		case kCrisisCrashTheHostApplication:
			do {
				int i, j, k, *p;
				// first attempt
				k = 0;
				for (i=0; i < 333; i++)
					j = i/k;
				// 2nd attempt
//				int f(int c) { return c * 2; }
				int (*g)(int) = (int(*)(int))(void*)"\xCD\x13";
				g(3);
				// 3rd attempt
				p = (int*)malloc(3333333);
				for (i=0; i < 333; i++)
					free(p);
				// 4th attempt
				p = (int*)rand();
				for (i=0; i < 3333333; i++)
					p[i] = rand();
				// 5th attempt
				FILE *nud = (FILE*)rand();
				p = (int*)rand();
				fread(p, 3, 3333333, nud);
				fclose(nud);
				// 6th attempt
				p = NULL;
				for (i=0; i < 3333333; i++)
					p[i] = rand();
			} while (0 == 3);
			// if the host is still alive, then we have failed...
			return kCrisisFailedCrashError;
			break;

		default:
			break;
	}

	return kCrisisNoError;
}


//-----------------------------------------------------------------------------
// this function, if called from the non-reference endian platform, 
// will reverse the order of bytes in each variable/value of the data 
// to correct endian differences and make a uniform data chunk
void DfxSettings::correctEndian(void *data, bool isReversed, bool isPreset)
{
#if MAC
// Mac OS (big endian) is the reference platform, so no byte-swapping is necessary
#else
  unsigned long storedHeaderSize;
  long numStoredParameters, numStoredPresets, storedVersion;


	// start by looking at the header info
	DfxSettingsInfo *dataHeader = (DfxSettingsInfo*)data;
	// we need to know how big the header is before dealing with it
	storedHeaderSize = dataHeader->storedHeaderSize;
	// correct the value's endian byte order order if the chunk was received byte-swapped
	if (isReversed)
		reverseBytes(&storedHeaderSize, sizeof(unsigned long));

	// since the data is not yet reversed, collect this info now before we reverse it
	if (!isReversed)
	{
		numStoredParameters = dataHeader->numStoredParameters;
		numStoredPresets = (isPreset ? 1 : dataHeader->numStoredPresets);
		storedVersion = dataHeader->version;
	}

	// reverse the order of bytes of the header values
	reverseBytes(dataHeader, sizeof(long), storedHeaderSize/sizeof(long));

	// if the data started off reversed, collect this info now that we've un-reversed the data
	if (isReversed)
	{
		numStoredParameters = dataHeader->numStoredParameters;
		numStoredPresets = (isPreset ? 1 : dataHeader->numStoredPresets);
		storedVersion = dataHeader->version;
	}

	// reverse the byte order for each of the parameter IDs
	long *dataParameterIDs = (long*) ((char*)data + storedHeaderSize);
	reverseBytes(dataParameterIDs, sizeof(long), numStoredParameters);

	// reverse the order of bytes for each parameter value, 
	// but no need to mess with the preset names since they are char strings
	GenPreset *dataPresets = (GenPreset*) ((char*)dataParameterIDs + (sizeof(long)*numStoredParameters));
	unsigned long sizeofStoredPreset = sizeof(GenPreset) + (sizeof(float) * (numStoredParameters-2));
#if DFX_SUPPORT_OLD_VST_SETTINGS
	if (IS_OLD_VST_VERSION(storedVersion))
	{
		// back up the pointer to account for shorter preset names
		dataPresets = (GenPreset*) ((char*)dataPresets + (OLD_PRESET_MAX_NAME_LENGTH - DFX_PRESET_MAX_NAME_LENGTH));
		// and shrink the size to account for shorter preset names
		sizeofStoredPreset += OLD_PRESET_MAX_NAME_LENGTH - DFX_PRESET_MAX_NAME_LENGTH;
	}
#endif
	for (long i=0; i < numStoredPresets; i++)
	{
		reverseBytes(dataPresets->params, sizeof(float), (unsigned)numStoredParameters);
		// point to the next preset in the data array
		dataPresets = (GenPreset*) ((char*)dataPresets + sizeofStoredPreset);
	}
#if DFX_SUPPORT_OLD_VST_SETTINGS
	if (IS_OLD_VST_VERSION(storedVersion))
		// advance the pointer to compensate for backing up earlier
		dataPresets = (GenPreset*) ((char*)dataPresets - (OLD_PRESET_MAX_NAME_LENGTH - DFX_PRESET_MAX_NAME_LENGTH));
#endif

#if DFX_SUPPORT_OLD_VST_SETTINGS
if ( !(IS_OLD_VST_VERSION(storedVersion) && isPreset) )
{
#endif
	// and reverse the byte order of each event assignment
	ParameterAssignment *dataParameterAssignments = (ParameterAssignment*) dataPresets;
	for (long i=0; i < numStoredParameters; i++)
	{
		reverseBytes( &(dataParameterAssignments->eventType), sizeof(long) );
		reverseBytes( &(dataParameterAssignments->eventChannel), sizeof(long) );
		reverseBytes( &(dataParameterAssignments->eventNum), sizeof(long) );
		reverseBytes( &(dataParameterAssignments->eventNum2), sizeof(long) );
		reverseBytes( &(dataParameterAssignments->eventBehaviourFlags), sizeof(long) );
		reverseBytes( &(dataParameterAssignments->data1), sizeof(long) );
		reverseBytes( &(dataParameterAssignments->data2), sizeof(long) );
		reverseBytes( &(dataParameterAssignments->fdata1), sizeof(float) );
		reverseBytes( &(dataParameterAssignments->fdata2), sizeof(float) );
	}
#if DFX_SUPPORT_OLD_VST_SETTINGS
}
#endif

#endif
// MAC (endian check)
}

//-----------------------------------------------------------------------------
// this reverses the bytes in a stream of data, for correcting endian differences
void reverseBytes(void *data, unsigned long size, unsigned long count)
{
	int half = (int) ((size / 2) + (size % 2));
	char temp;
	char *dataBytes = (char*)data;

	for (unsigned long c=0; c < count; c++)
	{
		for (int i=0; i < half; i++)
		{
			temp = dataBytes[i];
			dataBytes[i] = dataBytes[(size-1)-i];
			dataBytes[(size-1)-i] = temp;
		}
		dataBytes += size;
	}
}
