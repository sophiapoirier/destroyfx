/*--------- by Marc Poirier  ][  April-July + October 2002 ---------*/

#include "dfxsettings.h"
#include "dfxplugin.h"

#include <stdio.h>	// for FILE stuff


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark -
#pragma mark init / destroy
#pragma mark -
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//-----------------------------------------------------------------------------
DfxSettings::DfxSettings(long inMagic, DfxPlugin * inPlugin, unsigned long inSizeofExtendedData)
:	plugin(inPlugin), sizeofExtendedData(inSizeofExtendedData)
{
	sharedChunk = NULL;
	paramAssignments = NULL;
	parameterIDs = NULL;

	// there's nothing we can do without a pointer back to the plugin
	if (plugin == NULL)
		return;

	numParameters = plugin->getnumparameters();
	numPresets = plugin->getnumpresets();

	if (numPresets < 1)
		numPresets = 1;	// we do need at least 1 set of parameters
	if (numParameters < 1)
		numParameters = 1;	// come on now, what are you trying to do?

	paramAssignments = (DfxParameterAssignment*) malloc(numParameters * sizeof(DfxParameterAssignment));
	parameterIDs = (long*) malloc(numParameters * sizeof(long));

	// default to each parameter having its ID equal its index
	// (I haven't implemented anything with parameter IDs yet)
	for (long i=0; i < numParameters; i++)
		parameterIDs[i] = i;

	// calculate some data sizes that are useful to know
	sizeofPreset = sizeof(DfxGenPreset) + (sizeof(float) * (numParameters-2));
	sizeofParameterIDs = sizeof(long) * numParameters;
	sizeofPresetChunk = sizeofPreset 			// 1 preset
						+ sizeof(DfxSettingsInfo) 	// the special data header info
						+ sizeofParameterIDs	// the table of parameter IDs
						+ (sizeof(DfxParameterAssignment)*numParameters);	// the MIDI events assignment array
	sizeofChunk = (sizeofPreset*numPresets)		// all of the presets
					+ sizeof(DfxSettingsInfo)		// the special data header info
					+ sizeofParameterIDs			// the table of parameter IDs
					+ (sizeof(DfxParameterAssignment)*numParameters);	// the MIDI events assignment array

	// increase the allocation sizes if extra data must be stored
	sizeofChunk += sizeofExtendedData;
	sizeofPresetChunk += sizeofExtendedData;

	// this is the shared data that we point **data to in save()
	sharedChunk = (DfxSettingsInfo*) malloc(sizeofChunk);
	// and a few pointers to elements within that data, just for ease of use
	firstSharedParameterID = (long*) ((char*)sharedChunk + sizeof(DfxSettingsInfo));
	firstSharedPreset = (DfxGenPreset*) ((char*)firstSharedParameterID + sizeofParameterIDs);
	firstSharedParamAssignment = (DfxParameterAssignment*) 
									((char*)firstSharedPreset + (sizeofPreset*numPresets));

	// set all of the header infos
	settingsInfo.magic = inMagic;
	settingsInfo.version = plugin->getpluginversion();
	settingsInfo.lowestLoadableVersion = 0;
	settingsInfo.storedHeaderSize = sizeof(DfxSettingsInfo);
	settingsInfo.numStoredParameters = numParameters;
	settingsInfo.numStoredPresets = numPresets;
	settingsInfo.storedParameterAssignmentSize = sizeof(DfxParameterAssignment);
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
	crisisBehaviour = kDfxSettingsCrisis_LoadWhatYouCan;

	// allow for further constructor stuff, if necessary
	plugin->settings_init();
}

//-----------------------------------------------------------------------------
DfxSettings::~DfxSettings()
{
	// wipe out the signature
	settingsInfo.magic = 0;

	// deallocate memories
	if (paramAssignments != NULL)
		free(paramAssignments);
	paramAssignments = NULL;

	if (parameterIDs != NULL)
		free(parameterIDs);
	parameterIDs = NULL;

	if (sharedChunk != NULL)
		free(sharedChunk);
	sharedChunk = NULL;

	// allow for further destructor stuff, if necessary
	plugin->settings_cleanup();
}

//------------------------------------------------------
// this interprets a UNIX environment variable string as a boolean
bool getenvBool(const char * var, bool def)
{
	const char * env = getenv(var);

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
#pragma mark -
#pragma mark settings
#pragma mark -
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//-----------------------------------------------------------------------------
// this gets called when the host wants to save settings data, 
// like when saving a song or preset files
unsigned long DfxSettings::save(void ** outData, bool isPreset)
{
  long i, j;


	if ( (sharedChunk == NULL) || (plugin == NULL) )
	{
		*outData = 0;
		return 1;
	}

	// share with the host
	*outData = sharedChunk;

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

		DfxParameterAssignment * tempSharedParamAssignment = (DfxParameterAssignment*) ((char*)firstSharedPreset + sizeofPreset);
		// store the parameters' MIDI event assignments
		for (i=0; i < numParameters; i++)
			tempSharedParamAssignment[i] = paramAssignments[i];

		// reverse the order of bytes in the data being sent to the host, if necessary
		correctEndian(*outData, false, isPreset);
		// allow for the storage of extra data
		plugin->settings_saveExtendedData((char*)sharedChunk+sizeofPresetChunk-sizeofExtendedData, isPreset);

		return sizeofPresetChunk;
	}

	// otherwise store the entire bank of presets and the MIDI event assignments
	else
	{
		DfxGenPreset * tempSharedPresets = firstSharedPreset;
		for (j=0; j < numPresets; j++)
		{
			// copy the preset name to the chunk
			plugin->getpresetname(j, tempSharedPresets->name);
			// copy all of the parameters for this preset to the chunk
			for (i=0; i < numParameters; i++)
				tempSharedPresets->params[i] = plugin->getpresetparameter_f(j, i);
			// point to the next preset in the data array for the host
			tempSharedPresets = (DfxGenPreset*) ((char*)tempSharedPresets + sizeofPreset);
		}

		// store the parameters' MIDI event assignments
		for (i=0; i < numParameters; i++)
			firstSharedParamAssignment[i] = paramAssignments[i];

		// reverse the order of bytes in the data being sent to the host, if necessary
		correctEndian(*outData, false, isPreset);
		// allow for the storage of extra data
		plugin->settings_saveExtendedData((char*)sharedChunk+sizeofChunk-sizeofExtendedData, isPreset);

		return sizeofChunk;
	}
}


// for backwerds compaxibilitee
#define IS_OLD_VST_VERSION(version)	((version) < 0x00010000)
#define OLD_PRESET_MAX_NAME_LENGTH 32

//-----------------------------------------------------------------------------
// this gets called when the host wants to load settings data, 
// like when restoring settings while opening a song, 
// or loading a preset file
bool DfxSettings::restore(void * inData, unsigned long byteSize, bool isPreset)
{
  DfxSettingsInfo * newSettingsInfo;
  DfxGenPreset * newPreset;
  long * newParameterIDs;
  long i, j;


	if (plugin == NULL)
		return false;

	// un-reverse the order of bytes in the received data, if necessary
	correctEndian(inData, true, isPreset);

	// point to the start of the chunk data:  the settingsInfo header
	newSettingsInfo = (DfxSettingsInfo*)inData;

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

#ifdef DFX_SUPPORT_OLD_VST_SETTINGS
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
	// figure out how much of the DfxParameterAssignment structure we can import
	unsigned long copyParameterAssignmentSize = (newSettingsInfo->storedParameterAssignmentSize < settingsInfo.storedParameterAssignmentSize) ? 
									newSettingsInfo->storedParameterAssignmentSize : settingsInfo.storedParameterAssignmentSize;

	// check for conflicts and keep track of them
	long crisisFlags = 0;
	if (newSettingsInfo->version < settingsInfo.version)
		crisisFlags = crisisFlags | kDfxSettingsCrisis_LowerVersion;
	else if (newSettingsInfo->version > settingsInfo.version)
		crisisFlags = crisisFlags | kDfxSettingsCrisis_HigherVersion;
	if (numStoredParameters < numParameters)
		crisisFlags = crisisFlags | kDfxSettingsCrisis_FewerParameters;
	else if (numStoredParameters > numParameters)
		crisisFlags = crisisFlags | kDfxSettingsCrisis_MoreParameters;
	if (isPreset)
	{
		if (byteSize < sizeofPresetChunk)
			crisisFlags = crisisFlags | kDfxSettingsCrisis_SmallerByteSize;
		else if (byteSize > sizeofPresetChunk)
			crisisFlags = crisisFlags | kDfxSettingsCrisis_LargerByteSize;
	}
	else
	{
		if (byteSize < sizeofChunk)
			crisisFlags = crisisFlags | kDfxSettingsCrisis_SmallerByteSize;
		else if (byteSize > sizeofChunk)
			crisisFlags = crisisFlags | kDfxSettingsCrisis_LargerByteSize;
		if (numStoredPresets < numPresets)
			crisisFlags = crisisFlags | kDfxSettingsCrisis_FewerPresets;
		else if (numStoredPresets > numPresets)
			crisisFlags = crisisFlags | kDfxSettingsCrisis_MorePresets;
	}
	// handle the crisis situations (if any) and abort loading if we're told to
	if (handleCrisis(crisisFlags) == kDfxSettingsCrisis_AbortError)
		return false;

	// point to the next data element after the chunk header:  the first parameter ID
	newParameterIDs = (long*) ((char*)newSettingsInfo + storedHeaderSize);
	// create a mapping table for corresponding the incoming parameters to the 
	// destination parameters (in case the parameter IDs don't all match up)
	//  [ the index of paramMap is the same as our parameter tag/index and the value 
	//     is the tag/index of the incoming parameter that corresponds, if any ]
	long * paramMap = (long*) malloc(numParameters * sizeof(long));
	for (long tag=0; tag < numParameters; tag++)
		paramMap[tag] = getParameterTagFromID(parameterIDs[tag], numStoredParameters, newParameterIDs);

	// point to the next data element after the parameter IDs:  the first preset name
	newPreset = (DfxGenPreset*) ((char*)newParameterIDs + (sizeof(long)*numStoredParameters));
	// handy for incrementing the data pointer
	unsigned long sizeofStoredPreset = sizeof(DfxGenPreset) + (sizeof(float)*(numStoredParameters-2));

	// the chunk being received only contains one preset
	if (isPreset)
	{
		// in Audio Unit, this is handled already in AUBase::RestoreState, 
		// and we are not really loading a "preset,"
		// we are restoring the last user state
		#ifndef TARGET_API_AUDIOUNIT
		// copy the preset name from the chunk
		plugin->setpresetname(plugin->getcurrentpresetnum(), newPreset->name);
		#endif
	#ifdef DFX_SUPPORT_OLD_VST_SETTINGS
		// back up the pointer to account for shorter preset names
		if (oldvst)
			newPreset = (DfxGenPreset*) ((char*)newPreset + (OLD_PRESET_MAX_NAME_LENGTH - DFX_PRESET_MAX_NAME_LENGTH));
	#endif
		// copy all of the parameters that we can for this preset from the chunk
		for (i=0; i < numParameters; i++)
		{
			long mappedTag = paramMap[i];
			if (mappedTag != DFX_PARAM_INVALID_ID)
			{
			#ifdef DFX_SUPPORT_OLD_VST_SETTINGS
				// handle old-style generic VST 0.0 to 1.0 parameter values
				if (oldvst)
					plugin->setparameter_gen(i, newPreset->params[mappedTag]);
				else
			#endif
				plugin->setparameter_f(i, newPreset->params[mappedTag]);
				// allow for additional tweaking of the stored parameter setting
				plugin->settings_doChunkRestoreSetParameterStuff(i, newPreset->params[mappedTag], newSettingsInfo->version);
			}
		}
		// point to the next preset in the received data array
		newPreset = (DfxGenPreset*) ((char*)newPreset + sizeofStoredPreset);
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
		#ifdef DFX_SUPPORT_OLD_VST_SETTINGS
			// back up the pointer to account for shorter preset names
			if (oldvst)
				newPreset = (DfxGenPreset*) ((char*)newPreset + (OLD_PRESET_MAX_NAME_LENGTH - DFX_PRESET_MAX_NAME_LENGTH));
		#endif
			// copy all of the parameters that we can for this preset from the chunk
			for (i=0; i < numParameters; i++)
			{
				long mappedTag = paramMap[i];
				if (mappedTag != DFX_PARAM_INVALID_ID)
				{
				#ifdef DFX_SUPPORT_OLD_VST_SETTINGS
					// handle old-style generic VST 0.0 to 1.0 parameter values
					if (oldvst)
						plugin->setpresetparameter_gen(j, i, newPreset->params[mappedTag]);
					else
				#endif
					plugin->setpresetparameter_f(j, i, newPreset->params[mappedTag]);
					// allow for additional tweaking of the stored parameter setting
					plugin->settings_doChunkRestoreSetParameterStuff(i, newPreset->params[mappedTag], newSettingsInfo->version, j);
				}
			}
			// point to the next preset in the received data array
			newPreset = (DfxGenPreset*) ((char*)newPreset + sizeofStoredPreset);
		}
	}

#ifdef DFX_SUPPORT_OLD_VST_SETTINGS
if ( !(oldvst && isPreset) )
{
#endif
	// completely clear our table of parameter assignments before loading the new 
	// table since the new one might not have all of the data members
	clearAssignments();
	// then point to the last chunk data element, the MIDI event assignment array
	// (offset by the number of stored presets that were skipped, if any)
	DfxParameterAssignment * newParamAssignments;
//	if (isPreset)
//		newParamAssignments = (DfxParameterAssignment*) ((char*)newPreset + sizeofStoredPreset);
//	else
		newParamAssignments = (DfxParameterAssignment*) ((char*)newPreset + 
								((numStoredPresets-copyPresets) * sizeofStoredPreset));
	// and load up as many of them as we can
	for (i=0; i < numParameters; i++)
	{
		long mappedTag = paramMap[i];
		if (mappedTag != DFX_PARAM_INVALID_ID)
			memcpy( &(paramAssignments[i]), 
					(char*)newParamAssignments+(mappedTag*(newSettingsInfo->storedParameterAssignmentSize)), 
					copyParameterAssignmentSize);
//			paramAssignments[i] = newParamAssignments[mappedTag];
	}
#ifdef DFX_SUPPORT_OLD_VST_SETTINGS
}
#endif

	// allow for the retrieval of extra data
	plugin->settings_restoreExtendedData((char*)inData+sizeofChunk-newSettingsInfo->storedExtendedDataSize, 
						newSettingsInfo->storedExtendedDataSize, newSettingsInfo->version, isPreset);

	if (paramMap)
		free(paramMap);

	return true;
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark -
#pragma mark AudioUnit-specific stuff
#pragma mark -
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#ifdef TARGET_API_AUDIOUNIT
	static const CFStringRef kDfxSettings_ParameterIDKey = CFSTR("parameter ID");
static const CFStringRef kDfxSettings_MidiAssignmentsKey = CFSTR("DFX! MIDI assignments");
	static const CFStringRef kDfxSettings_MidiAssignment_eventTypeKey = CFSTR("event type");
	static const CFStringRef kDfxSettings_MidiAssignment_eventChannelKey = CFSTR("event channel");
	static const CFStringRef kDfxSettings_MidiAssignment_eventNumKey = CFSTR("event number");
	static const CFStringRef kDfxSettings_MidiAssignment_eventNum2Key = CFSTR("event number 2");
	static const CFStringRef kDfxSettings_MidiAssignment_eventBehaviourFlagsKey = CFSTR("event behavior flags");
	static const CFStringRef kDfxSettings_MidiAssignment_data1Key = CFSTR("integer data 1");
	static const CFStringRef kDfxSettings_MidiAssignment_data2Key = CFSTR("integer data 2");
	static const CFStringRef kDfxSettings_MidiAssignment_fdata1Key = CFSTR("float data 1");
	static const CFStringRef kDfxSettings_MidiAssignment_fdata2Key = CFSTR("float data 2");

//-----------------------------------------------------------------------------------------
bool DFX_AddNumberToCFDictionary(const void * inNumber, CFNumberType inType, CFMutableDictionaryRef inDictionary, const void * inDictionaryKey)
{
	if ( (inNumber == NULL) || (inDictionary == NULL) || (inDictionaryKey == NULL) )
		return false;

	CFNumberRef cfNumber = CFNumberCreate(kCFAllocatorDefault, inType, inNumber);
	if (cfNumber != NULL)
	{
		CFDictionarySetValue(inDictionary, inDictionaryKey, cfNumber);
		CFRelease(cfNumber);
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------------------
bool DFX_AddNumberToCFDictionary_i(SInt64 inNumber, CFMutableDictionaryRef inDictionary, const void * inDictionaryKey)
{
	return DFX_AddNumberToCFDictionary(&inNumber, kCFNumberSInt64Type, inDictionary, inDictionaryKey);
}

//-----------------------------------------------------------------------------------------
bool DFX_AddNumberToCFDictionary_f(Float64 inNumber, CFMutableDictionaryRef inDictionary, const void * inDictionaryKey)
{
	return DFX_AddNumberToCFDictionary(&inNumber, kCFNumberFloat64Type, inDictionary, inDictionaryKey);
}

//-----------------------------------------------------------------------------------------
SInt64 DFX_GetNumberFromCFDictionary_i(CFDictionaryRef inDictionary, const void * inDictionaryKey, bool * outSuccess = NULL)
{
	SInt64 resultNumber = 0;
	const CFNumberType numberType = kCFNumberSInt64Type;
	if (outSuccess != NULL)
		*outSuccess = false;

	if ( (inDictionary == NULL) || (inDictionaryKey == NULL) )
		return resultNumber;

	CFNumberRef cfNumber = reinterpret_cast<CFNumberRef>( CFDictionaryGetValue(inDictionary, inDictionaryKey) );
	if (cfNumber != NULL)
	{
		if ( CFGetTypeID(cfNumber) == CFNumberGetTypeID() )
		{
			if (CFNumberGetType(cfNumber) == numberType)
			{
				Boolean numberSuccess = CFNumberGetValue(cfNumber, numberType, &resultNumber);
				if (outSuccess != NULL)
					*outSuccess = numberSuccess;
			}
		}
	}

	return resultNumber;
}

//-----------------------------------------------------------------------------------------
Float64 DFX_GetNumberFromCFDictionary_f(CFDictionaryRef inDictionary, const void * inDictionaryKey, bool * outSuccess = NULL)
{
	Float64 resultNumber = 0;
	const CFNumberType numberType = kCFNumberFloat64Type;
	if (outSuccess != NULL)
		*outSuccess = false;

	if ( (inDictionary == NULL) || (inDictionaryKey == NULL) )
		return resultNumber;

	CFNumberRef cfNumber = reinterpret_cast<CFNumberRef>( CFDictionaryGetValue(inDictionary, inDictionaryKey) );
	if (cfNumber != NULL)
	{
		if ( CFGetTypeID(cfNumber) == CFNumberGetTypeID() )
		{
			if (CFNumberGetType(cfNumber) == numberType)
			{
				Boolean numberSuccess = CFNumberGetValue(cfNumber, numberType, &resultNumber);
				if (outSuccess != NULL)
					*outSuccess = numberSuccess;
			}
		}
	}

	return resultNumber;
}

//-----------------------------------------------------------------------------------------
bool DfxSettings::saveMidiAssignmentsToDictionary(CFMutableDictionaryRef inDictionary)
{
	if (inDictionary == NULL)
		return false;

	bool assignmentsFound = false;
	for (long i=0; i < numParameters; i++)
	{
		if (getParameterAssignmentType(i) != kParamEventNone)
			assignmentsFound = true;
	}

	if (assignmentsFound)
	{
		CFMutableArrayRef assignmentsCFArray = CFArrayCreateMutable(kCFAllocatorDefault, numParameters, &kCFTypeArrayCallBacks);
		if (assignmentsCFArray != NULL)
		{
			for (long i=0; i < numParameters; i++)
			{
				CFMutableDictionaryRef assignmentCFDictionary = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
				if (assignmentCFDictionary != NULL)
				{
					if (getParameterID(i) == DFX_PARAM_INVALID_ID)
						continue;
					DFX_AddNumberToCFDictionary_i(getParameterID(i), assignmentCFDictionary, kDfxSettings_ParameterIDKey);
#define ADD_ASSIGNMENT_VALUE_TO_DICT(inMember, inTypeSuffix)	\
					DFX_AddNumberToCFDictionary_##inTypeSuffix(paramAssignments[i].inMember, assignmentCFDictionary, kDfxSettings_MidiAssignment_##inMember##Key);
					ADD_ASSIGNMENT_VALUE_TO_DICT(eventType, i)
					ADD_ASSIGNMENT_VALUE_TO_DICT(eventChannel, i)
printf("event num = %ld, event num 2 = %ld\n", paramAssignments[i].eventNum, paramAssignments[i].eventNum2);
					ADD_ASSIGNMENT_VALUE_TO_DICT(eventNum, i)
					ADD_ASSIGNMENT_VALUE_TO_DICT(eventNum2, i)
					ADD_ASSIGNMENT_VALUE_TO_DICT(eventBehaviourFlags, i)
					ADD_ASSIGNMENT_VALUE_TO_DICT(data1, i)
					ADD_ASSIGNMENT_VALUE_TO_DICT(data2, i)
					ADD_ASSIGNMENT_VALUE_TO_DICT(fdata1, f)
					ADD_ASSIGNMENT_VALUE_TO_DICT(fdata2, f)
#undef ADD_ASSIGNMENT_VALUE_TO_DICT
					CFArraySetValueAtIndex(assignmentsCFArray, i, assignmentCFDictionary);
					CFRelease(assignmentCFDictionary);
				}
			}
			CFDictionarySetValue(inDictionary, kDfxSettings_MidiAssignmentsKey, assignmentsCFArray);
			CFRelease(assignmentsCFArray);
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------------------
bool DfxSettings::restoreMidiAssignmentsFromDictionary(CFDictionaryRef inDictionary)
{
	if (inDictionary == NULL)
		return false;

	CFArrayRef assignmentsCFArray = reinterpret_cast<CFArrayRef>( CFDictionaryGetValue(inDictionary, kDfxSettings_MidiAssignmentsKey) );
	if (assignmentsCFArray != NULL)
	{
		if ( CFGetTypeID(assignmentsCFArray) == CFArrayGetTypeID() )
		{
			// completely clear our table of parameter assignments before loading the new 
			// table since the new one might not have all of the data members
			clearAssignments();

			CFIndex arraySize = CFArrayGetCount(assignmentsCFArray);
			for (CFIndex i=0; i < arraySize; i++)
			{
				CFDictionaryRef assignmentCFDictionary = reinterpret_cast<CFDictionaryRef>( CFArrayGetValueAtIndex(assignmentsCFArray, i) );
				if (assignmentCFDictionary != NULL)
				{
					bool numberSuccess = false;
					long paramID = DFX_GetNumberFromCFDictionary_i(assignmentCFDictionary, kDfxSettings_ParameterIDKey, &numberSuccess);
					if (!numberSuccess)
						continue;
					paramID = getParameterTagFromID(paramID);
					if (paramID == DFX_PARAM_INVALID_ID)
						continue;
#define GET_ASSIGNMENT_VALUE_FROM_DICT(inMember, inTypeSuffix)	\
					paramAssignments[paramID].inMember = DFX_GetNumberFromCFDictionary_##inTypeSuffix(assignmentCFDictionary, kDfxSettings_MidiAssignment_##inMember##Key, &numberSuccess);
					GET_ASSIGNMENT_VALUE_FROM_DICT(eventType, i)
					if (!numberSuccess)
					{
						unassignParam(paramID);
						continue;
					}
					GET_ASSIGNMENT_VALUE_FROM_DICT(eventChannel, i)
					GET_ASSIGNMENT_VALUE_FROM_DICT(eventNum, i)
					GET_ASSIGNMENT_VALUE_FROM_DICT(eventNum2, i)
					GET_ASSIGNMENT_VALUE_FROM_DICT(eventBehaviourFlags, i)
					GET_ASSIGNMENT_VALUE_FROM_DICT(data1, i)
					GET_ASSIGNMENT_VALUE_FROM_DICT(data2, i)
					GET_ASSIGNMENT_VALUE_FROM_DICT(fdata1, f)
					GET_ASSIGNMENT_VALUE_FROM_DICT(fdata2, f)
#undef GET_ASSIGNMENT_VALUE_FROM_DICT
				}
			}
			// this seems like a good enough sign that we at least partially succeeded
			if (arraySize > 0)
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
				plugin->settings_doLearningAssignStuff(learner, eventType, channel, note1, 
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
		plugin->settings_doLearningAssignStuff(learner, eventType, channel, byte1, 
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
		DfxParameterAssignment * pa = &(paramAssignments[tag]);

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
		plugin->postupdate_parameter(tag);	// notify listeners of internal parameter change
		// this is an invitation to do something more, if necessary
		plugin->settings_doMidiAutomatedSetParameterStuff(tag, fValue, frameOffset);

	}	// end of parameters loop (for automation)
}



//-----------------------------------------------------------------------------
// clear all parameter assignments from the the CCs
void DfxSettings::clearAssignments()
{
	for (long i=0; i < numParameters; i++)
		unassignParam(i);
}

//-----------------------------------------------------------------------------
// assign a CC to a parameter
void DfxSettings::assignParam(long tag, long eventType, long eventChannel, long eventNum, 
							long eventNum2, long eventBehaviourFlags, 
							long data1, long data2, float fdata1, float fdata2)
{
	// abort if the parameter index is not valid
	if (! paramTagIsValid(tag) )
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
			DfxParameterAssignment * pa = &(paramAssignments[i]);
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

			// not a note, so it's simple:  
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
	if (! paramTagIsValid(tag) )
		return;

	// clear the MIDI event assignment for this parameter
	paramAssignments[tag].eventType = kParamEventNone;
	paramAssignments[tag].eventChannel = 0;
	paramAssignments[tag].eventNum = 0;
	paramAssignments[tag].eventNum2 = 0;
	paramAssignments[tag].eventBehaviourFlags = 0;
	paramAssignments[tag].data1 = 0;
	paramAssignments[tag].data2 = 0;
	paramAssignments[tag].fdata1 = 0.0f;
	paramAssignments[tag].fdata2 = 0.0f;
}

//-----------------------------------------------------------------------------
// turn MIDI learn mode on or off
void DfxSettings::setLearning(bool newLearn)
{
	// erase the current learner if the state of MIDI learn is being toggled
	if (newLearn != midiLearn)
		setLearner(kNoLearner);
	// or if it's being asked to be turned off, irregardless
	else if (!newLearn)
		setLearner(kNoLearner);

	midiLearn = newLearn;
}

//-----------------------------------------------------------------------------
// just an easy way to check if a particular parameter is currently a learner
bool DfxSettings::isLearner(long tag)
{
	return (tag == getLearner());
}

//-----------------------------------------------------------------------------
// define the actively learning parameter during MIDI learn mode
void DfxSettings::setLearner(long tag, long eventBehaviourFlags, 
							long data1, long data2, float fdata1, float fdata2)
{
	// allow this invalid parameter tag, and then exit
	if (tag == kNoLearner)
	{
		learner = kNoLearner;
		return;
	}
	// return if what we got is not a valid parameter index
	if (! paramTagIsValid(tag)  )
		return;

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
void DfxSettings::setParameterMidiLearn(bool value)
{
	setLearning(value);
}

//-----------------------------------------------------------------------------
// a plugin editor should call this during valueChanged from a control 
// to clear MIDI event assignments, VST parameter style
void DfxSettings::setParameterMidiReset(bool value)
{
	if (value)
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
#pragma mark -
#pragma mark misc
#pragma mark -
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//-----------------------------------------------------------------------------
long DfxSettings::getParameterAssignmentType(long paramTag)
{
	// return no-assignment if what we got is not a valid parameter index
	if (! paramTagIsValid(paramTag) )
		return kParamEventNone;

	return paramAssignments[paramTag].eventType;
}

//-----------------------------------------------------------------------------
long DfxSettings::getParameterAssignmentNum(long paramTag)
{
	// if what we got is not a valid parameter index
	if (! paramTagIsValid(paramTag) )
		return 0;	// XXX is there a better value to return on error?

	return paramAssignments[paramTag].eventNum;
}

//-----------------------------------------------------------------------------
// given a parameter ID, find the tag (index) for that parameter in a table of 
// parameter IDs (probably our own table, unless a pointer to one was provided)
long DfxSettings::getParameterTagFromID(long paramID, long numSearchIDs, long * searchIDs)
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

	// if nothing was found, then return the error ID
	return DFX_PARAM_INVALID_ID;
}


//-----------------------------------------------------------------------------
// this is called to investigate what to do when a data chunk is received in 
// restore() that doesn't match the characteristics of what we are expecting
long DfxSettings::handleCrisis(long flags)
{
	// no need to continue on if there is no crisis situation
	if (flags == 0)
		return kDfxSettingsCrisis_NoError;

	switch (crisisBehaviour)
	{
		case kDfxSettingsCrisis_LoadWhatYouCan:
			return kDfxSettingsCrisis_NoError;
			break;

		case kDfxSettingsCrisis_DontLoad:
			return kDfxSettingsCrisis_AbortError;
			break;

		case kDfxSettingsCrisis_LoadButComplain:
			crisisAlert(flags);
			return kDfxSettingsCrisis_ComplainError;
			break;

		case kDfxSettingsCrisis_CrashTheHostApplication:
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
				FILE * nud = (FILE*)rand();
				p = (int*)rand();
				fread(p, 3, 3333333, nud);
				fclose(nud);
				// 6th attempt
				p = NULL;
				for (i=0; i < 3333333; i++)
					p[i] = rand();
			} while (0 == 3);
			// if the host is still alive, then we have failed...
			return kDfxSettingsCrisis_FailedCrashError;
			break;

		default:
			break;
	}

	return kDfxSettingsCrisis_NoError;
}


//-----------------------------------------------------------------------------
// this function, if called for the non-reference endian architecture, 
// will reverse the order of bytes in each variable/value of the data 
// to correct endian differences and make a uniform data chunk
void DfxSettings::correctEndian(void * data, bool isReversed, bool isPreset)
{
/*
// XXX another idea...
void blah(long long x)
{
	int n = sizeof(x);
	while (n--)
	{
		write(f, x & 255, 1);
		x >>= 8;
	}
}
*/
#if __BIG_ENDIAN__
// big endian (like PowerPC) is the reference architecture, so no byte-swapping is necessary
#else
	// start by looking at the header info
	DfxSettingsInfo * dataHeader = (DfxSettingsInfo*)data;
	// we need to know how big the header is before dealing with it
	unsigned long storedHeaderSize = dataHeader->storedHeaderSize;
	long numStoredParameters = dataHeader->numStoredParameters;
	long numStoredPresets = dataHeader->numStoredPresets;
	long storedVersion = dataHeader->version;
	// correct the values' endian byte order order if the data was received byte-swapped
	if (isReversed)
	{
		reversebytes(&storedHeaderSize, sizeof(storedHeaderSize));
		reversebytes(&numStoredParameters, sizeof(numStoredParameters));
		reversebytes(&numStoredPresets, sizeof(numStoredPresets));
		reversebytes(&storedVersion, sizeof(storedVersion));
	}
//	if (isPreset)
//		numStoredPresets = 1;

	// reverse the order of bytes of the header values
	reversebytes(dataHeader, sizeof(long), storedHeaderSize/sizeof(long));

	// reverse the byte order for each of the parameter IDs
	long * dataParameterIDs = (long*) ((char*)data + storedHeaderSize);
	reversebytes(dataParameterIDs, sizeof(long), numStoredParameters);

	// reverse the order of bytes for each parameter value, 
	// but no need to mess with the preset names since they are char strings
	DfxGenPreset * dataPresets = (DfxGenPreset*) ((char*)dataParameterIDs + (sizeof(long)*numStoredParameters));
	unsigned long sizeofStoredPreset = sizeof(DfxGenPreset) + (sizeof(float) * (numStoredParameters-2));
#ifdef DFX_SUPPORT_OLD_VST_SETTINGS
	if (IS_OLD_VST_VERSION(storedVersion))
	{
		// back up the pointer to account for shorter preset names
		dataPresets = (DfxGenPreset*) ((char*)dataPresets + (OLD_PRESET_MAX_NAME_LENGTH - DFX_PRESET_MAX_NAME_LENGTH));
		// and shrink the size to account for shorter preset names
		sizeofStoredPreset += OLD_PRESET_MAX_NAME_LENGTH - DFX_PRESET_MAX_NAME_LENGTH;
	}
#endif
	for (long iij=0; iij < numStoredPresets; iij++)
	{
		reversebytes(dataPresets->params, sizeof(float), (unsigned)numStoredParameters);
		// point to the next preset in the data array
		dataPresets = (DfxGenPreset*) ((char*)dataPresets + sizeofStoredPreset);
	}
#ifdef DFX_SUPPORT_OLD_VST_SETTINGS
	if (IS_OLD_VST_VERSION(storedVersion))
		// advance the pointer to compensate for backing up earlier
		dataPresets = (DfxGenPreset*) ((char*)dataPresets - (OLD_PRESET_MAX_NAME_LENGTH - DFX_PRESET_MAX_NAME_LENGTH));
#endif

#ifdef DFX_SUPPORT_OLD_VST_SETTINGS
if ( !(IS_OLD_VST_VERSION(storedVersion) && isPreset) )
{
#endif
	// and reverse the byte order of each event assignment
	DfxParameterAssignment * dataParameterAssignments = (DfxParameterAssignment*) dataPresets;
	for (long i=0; i < numStoredParameters; i++)
	{
		reversebytes( &(dataParameterAssignments->eventType), sizeof(long) );
		reversebytes( &(dataParameterAssignments->eventChannel), sizeof(long) );
		reversebytes( &(dataParameterAssignments->eventNum), sizeof(long) );
		reversebytes( &(dataParameterAssignments->eventNum2), sizeof(long) );
		reversebytes( &(dataParameterAssignments->eventBehaviourFlags), sizeof(long) );
		reversebytes( &(dataParameterAssignments->data1), sizeof(long) );
		reversebytes( &(dataParameterAssignments->data2), sizeof(long) );
		reversebytes( &(dataParameterAssignments->fdata1), sizeof(float) );
		reversebytes( &(dataParameterAssignments->fdata2), sizeof(float) );
	}
#ifdef DFX_SUPPORT_OLD_VST_SETTINGS
}
#endif

#endif
// __BIG_ENDIAN__ (endian check)
}
