/*------------------------------------------------------------------------
Destroy FX is a sovereign entity comprised of Marc Poirier & Tom Murphy 7.  
This is our Audio Unit shit.
------------------------------------------------------------------------*/

#ifndef __DFXPLUGIN_H
#include "dfxplugin.h"
#endif



#pragma mark _________init_________

//-----------------------------------------------------------------------------
ComponentResult DfxPlugin::Initialize()
{
	ComponentResult result = noErr;

	result = TARGET_API_BASE_CLASS::Initialize();	// XXX should I do this?

	if (result == noErr)
		result = do_initialize();

	if (result == noErr)
		Reset();

	return result;
}

//-----------------------------------------------------------------------------
void DfxPlugin::Cleanup()
{
	TARGET_API_BASE_CLASS::Cleanup();

	do_cleanup();
}

//-----------------------------------------------------------------------------
void DfxPlugin::Reset()
{
	do_reset();
}



#pragma mark _________info_________

//-----------------------------------------------------------------------------
ComponentResult DfxPlugin::GetPropertyInfo(AudioUnitPropertyID inID, 
					AudioUnitScope inScope, AudioUnitElement inElement, 
					UInt32 &outDataSize, Boolean &outWritable)
{
	ComponentResult result = noErr;

	switch (inID)
	{
	#if TARGET_PLUGIN_HAS_GUI
		case kAudioUnitProperty_GetUIComponentList:
			outDataSize = sizeof(ComponentDescription);
			outWritable = false;
			break;
	#endif

	#if TARGET_PLUGIN_USES_MIDI
//		returns an array of AudioUnitMIDIControlMapping's, specifying a default mapping of
//		MIDI controls and/or NRPN's to AudioUnit scopes/elements/parameters.
//	typedef struct AudioUnitMIDIControlMapping {
//		UInt16	midiNRPN;			// 0xFFFF if none
								// MSB, LSB are in low 14 bits
//		UInt8	midiControl;		// 0xFF if none
								// must not use controls: 
								//		0, 32 (bank select)
								// 		6, 38 (data entry MSB/LSB), 
								// 		96-101 (increment, decrement, RPN/NRPN select),
								// 		120-127 (channel mode messages)
//		UInt8					scope;
//		AudioUnitElement		element;
//		AudioUnitParameterID	parameter;
//	};
		case kAudioUnitProperty_MIDIControlMapping:
			outDataSize = sizeof(AudioUnitMIDIControlMapping*);
			outWritable = false;
			break;
	#endif

		default:
			result = TARGET_API_BASE_CLASS::GetPropertyInfo(inID, inScope, inElement, outDataSize, outWritable);
			break;
	}

	return result;
}

//-----------------------------------------------------------------------------
ComponentResult DfxPlugin::GetProperty(AudioUnitPropertyID inID, 
					AudioUnitScope inScope, AudioUnitElement inElement, 
					void *outData)
{
	ComponentResult result = noErr;

	switch (inID)
	{
	#if TARGET_PLUGIN_HAS_GUI
		case kAudioUnitProperty_GetUIComponentList:
			{
				ComponentDescription *cd = static_cast<ComponentDescription *>(outData);
				cd->componentType = kAudioUnitCarbonViewComponentType;
				#ifdef PLUGIN_EDITOR_ID
					cd->componentSubType = PLUGIN_EDITOR_ID;
				#else
					cd->componentSubType = (PLUGIN_ID & 0xFFFFFF00) + 'V';
				#endif
				cd->componentManufacturer = DESTROYFX_ID;
				cd->componentFlags = 0;
				cd->componentFlagsMask = 0;
			}
			break;
	#endif

	#if TARGET_PLUGIN_USES_MIDI
		case kAudioUnitProperty_MIDIControlMapping:
//			AudioUnitMIDIControlMapping *mcm = (AudioUnitMIDIControlMapping*)outData;
			// XXX do something here
			break;
	#endif

		default:
			result = TARGET_API_BASE_CLASS::GetProperty(inID, inScope, inElement, outData);
			break;
	}

	return result;
}

//-----------------------------------------------------------------------------
UInt32 DfxPlugin::SupportedNumChannels(const AUChannelInfo **outInfo)
{
	if (channelconfigs)
	{
		if (outInfo != NULL)
			*outInfo = channelconfigs;
		return (unsigned)numchannelconfigs;
	}
	return 0;
}

//-----------------------------------------------------------------------------
Float64 DfxPlugin::GetLatency()
{
	return getlatency_seconds();
}

//-----------------------------------------------------------------------------
Float64 DfxPlugin::GetTailTime()
{
	return gettailsize_seconds();
}



#pragma mark _________parameters_________

//-----------------------------------------------------------------------------
ComponentResult DfxPlugin::GetParameterInfo(AudioUnitScope inScope, 
					AudioUnitParameterID inParameterID, 
					AudioUnitParameterInfo &outParameterInfo)
{
	if (inScope != kAudioUnitScope_Global)
		return kAudioUnitErr_InvalidScope;

	if ( !parameterisvalid(inParameterID) )
		return kAudioUnitErr_InvalidParameter;

	getparametername(inParameterID, outParameterInfo.name);
	outParameterInfo.minValue = getparametermin_f(inParameterID);
	outParameterInfo.maxValue = getparametermax_f(inParameterID);
	outParameterInfo.defaultValue = getparameterdefault_f(inParameterID);
	outParameterInfo.flags = kAudioUnitParameterFlag_IsReadable 
							| kAudioUnitParameterFlag_IsWritable;

	// the complicated part:  getting the unit type
	switch (getparameterunit(inParameterID))
	{
		case kDfxParamUnit_percent:
			outParameterInfo.unit = kAudioUnitParameterUnit_Percent;
			break;
		case kDfxParamUnit_lineargain:
			outParameterInfo.unit = kAudioUnitParameterUnit_LinearGain;
			break;
		case kDfxParamUnit_decibles:
			outParameterInfo.unit = kAudioUnitParameterUnit_Decibels;
			break;
		case kDfxParamUnit_drywetmix:
			outParameterInfo.unit = kAudioUnitParameterUnit_EqualPowerCrossfade;
			break;
		case kDfxParamUnit_audiofreq:
		case kDfxParamUnit_lfofreq:
			outParameterInfo.unit = kAudioUnitParameterUnit_Hertz;
			break;
		case kDfxParamUnit_seconds:
		case kDfxParamUnit_ms:	// eh, whatever...
			outParameterInfo.unit = kAudioUnitParameterUnit_Seconds;
			break;
		case kDfxParamUnit_samples:
			outParameterInfo.unit = kAudioUnitParameterUnit_SampleFrames;
			break;
		case kDfxParamUnit_scalar:
			outParameterInfo.unit = kAudioUnitParameterUnit_Rate;
			break;
		case kDfxParamUnit_notes:
			outParameterInfo.unit = kAudioUnitParameterUnit_MIDINoteNumber;
			break;
		case kDfxParamUnit_semitones:
		case kDfxParamUnit_octaves:	// whatever...
			outParameterInfo.unit = kAudioUnitParameterUnit_RelativeSemiTones;
			break;
		case kDfxParamUnit_cents:
			outParameterInfo.unit = kAudioUnitParameterUnit_Cents;
			break;
		case kDfxParamUnit_pan:
			outParameterInfo.unit = kAudioUnitParameterUnit_Pan;
			break;
		case kDfxParamUnit_index:
		case kDfxParamUnit_strings:
			outParameterInfo.unit = kAudioUnitParameterUnit_Indexed;
			break;

		default:
			switch (getparametervaluetype(inParameterID))
			{
				case kDfxParamValueType_boolean:
					outParameterInfo.unit = kAudioUnitParameterUnit_Boolean;
					break;
				case kDfxParamValueType_int:
				case kDfxParamValueType_uint:
				case kDfxParamValueType_char:
				case kDfxParamValueType_uchar:
					outParameterInfo.unit = kAudioUnitParameterUnit_Indexed;
					break;
				default:
					outParameterInfo.unit = kAudioUnitParameterUnit_Generic;
					break;
			}
	}



	return noErr;
}

//-----------------------------------------------------------------------------
ComponentResult DfxPlugin::GetParameterValueStrings(AudioUnitScope inScope, 
					AudioUnitParameterID inParameterID, CFArrayRef *outStrings)
{
	if (inScope != kAudioUnitScope_Global)
		return kAudioUnitErr_InvalidScope;

	if (!parameterisvalid(inParameterID))
		return kAudioUnitErr_InvalidProperty;

	if (getparameterunit(inParameterID) == kDfxParamUnit_strings)
	{
		CFStringRef * outArray = getparametervaluecfstrings(inParameterID);
		if (outArray == NULL)
			return kAudioUnitErr_InvalidProperty;
		long numStrings = getparametermax_i(inParameterID) - getparametermin_i(inParameterID) + 1;
		*outStrings = CFArrayCreate(NULL, (const void**)outArray, numStrings, NULL);
		return noErr;
	}

	return kAudioUnitErr_InvalidProperty;
}

//-----------------------------------------------------------------------------
ComponentResult DfxPlugin::SetParameter(AudioUnitParameterID inID, 
					AudioUnitScope inScope, AudioUnitElement inElement, 
					Float32 inValue, UInt32 inBufferOffsetInFrames)
{
	if ( (inScope == kAudioUnitScope_Global) && (inElement == 0) )
		setparameter_f(inID, inValue);

	return AUBase::SetParameter(inID, inScope, inElement, inValue, inBufferOffsetInFrames);
}



#pragma mark _________presets_________

/*
ComponentResult AUBase::DispatchSetProperty(AudioUnitPropertyID inID, const void *inData)
{
	case kAudioUnitProperty_CurrentPreset:
	{
		AUPreset &newPreset = *(AUPreset*)inData;
		if (newPreset.presetName)
		{
			if (newPreset.presetNumber < 0 || NewFactoryPresetSet(newPreset) == noErr)
			{
				CFRelease(mCurrentPreset.presetName);
				mCurrentPreset = newPreset;
				CFRetain(mCurrentPreset.presetName);
				return noErr;
			}
			else return kAudioUnitErr_InvalidPropertyValue;
		}
		else return kAudioUnitErr_InvalidPropertyValue;
	}
}
*/
//-----------------------------------------------------------------------------
// Returns an array of AUPreset that contain a number and name for each of the presets.  
// The number of each preset must be greater (or equal to) zero.  
// The CFArrayRef should be released by the caller.
ComponentResult DfxPlugin::GetPresets(CFArrayRef *outData) const
{
	// figure out how many valid (loaded) presets we actually have...
	long outNumPresets = 0;
	for (long i=0; i < numPresets; i++)
	{
//		if (presetnameisvalid(i))
		if ( (presets[i].getname_ptr() != NULL) && (presets[i].getname_ptr()[0] != 0) )
			outNumPresets++;
	}
	if (outNumPresets <= 0)	// woops, looks like we don't actually have any presets
		return kAudioUnitErr_InvalidProperty;
	// ...and then allocate a mutable array large enough to hold them all
	CFMutableArrayRef outArray = CFArrayCreateMutable(NULL, outNumPresets, NULL);

	// add the preset data (name and number) into the array
	for (long i=0; i < numPresets; i++)
	{
//		if (presetnameisvalid(i))
		if ( (presets[i].getname_ptr() != NULL) && (presets[i].getname_ptr()[0] != 0) )
		{
			aupresets[i].presetNumber = i;
			//aupresets[i].presetName = getpresetcfname(i);
			aupresets[i].presetName = presets[i].getcfname();
			CFArrayAppendValue(outArray, &(aupresets[i]));
		}
	}

	*outData = (CFArrayRef)outArray;

	return noErr;
}

//-----------------------------------------------------------------------------
OSStatus DfxPlugin::NewFactoryPresetSet(const AUPreset & inNewFactoryPreset)
{
	long newNumber = inNewFactoryPreset.presetNumber;

	if ( !presetisvalid(newNumber) )
		return kAudioUnitErr_InvalidPropertyValue;
	if ( !presetnameisvalid(newNumber) )
		return kAudioUnitErr_InvalidPropertyValue;

	if ( !loadpreset(newNumber) )
		return kAudioUnitErr_InvalidPropertyValue;

	return noErr;
}



#pragma mark _________state_________

/*
//these are the current keys for the class info document
static const CFStringRef kVersionString = CFSTR("version");
static const CFStringRef kTypeString = CFSTR("type");
static const CFStringRef kSubtypeString = CFSTR("subtype");
static const CFStringRef kManufacturerString = CFSTR("manufacturer");
static const CFStringRef kDataString = CFSTR("data");
static const CFStringRef kNameString = CFSTR("name");
static void AddNumToDictionary(CFMutableDictionaryRef dict, CFStringRef key, SInt32 value)
{
	CFNumberRef num = CFNumberCreate(NULL, kCFNumberSInt32Type, &value);
	CFDictionarySetValue(dict, key, num);
	CFRelease(num);
}
#define kCurrentSavedStateVersion 0
*/
static const CFStringRef kDfxDataDictionaryKeyString = CFSTR("destroyfx-data");
//-----------------------------------------------------------------------------
// stores the values of all parameters values, state info, etc. into a CFPropertyListRef
ComponentResult DfxPlugin::SaveState(CFPropertyListRef *outData)
{
	ComponentResult result = TARGET_API_BASE_CLASS::SaveState(outData);
	if (result != noErr)
		return result;

/*
	ComponentDescription desc;
	OSStatus err = GetComponentInfo((Component)GetComponentInstance(), &desc, NULL, NULL, NULL);
	if (err)
		return err;

	CFMutableDictionaryRef dict = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

// first step -> save the version to the data ref
	SInt32 value = kCurrentSavedStateVersion;
	AddNumToDictionary(dict, kVersionString, value);

// second step -> save the component type, subtype, manu to the data ref
	value = desc.componentType;
	AddNumToDictionary(dict, kTypeString, value);

	value = desc.componentSubType;
	AddNumToDictionary(dict, kSubtypeString, value);
	
	value = desc.componentManufacturer;
	AddNumToDictionary(dict, kManufacturerString, value);
	
// third step -> save the state of all parameters on all scopes and elements
	CFMutableDataRef data = CFDataCreateMutable(NULL, NULL);
	for (AudioUnitScope iscope=0; iscope < 4; ++iscope)
	{
		AUScope &scope = GetScope(iscope);
		AudioUnitElement nElems = scope.GetNumberOfElements();
		for (AudioUnitElement ielem=0; ielem < nElems; ++ielem)
		{
			AUElement *element = scope.GetElement(ielem);
			UInt32 nparams = element->GetNumberOfParameters();
			if (nparams > 0)
			{
				struct {
					UInt32	scope;
					UInt32	element;
				} hdr;
				
				hdr.scope = CFSwapInt32HostToBig(iscope);
				hdr.element = CFSwapInt32HostToBig(ielem);
				CFDataAppendBytes(data, (UInt8 *)&hdr, sizeof(hdr));
				
				element->SaveState(data);
			}
		}
	}

	// save all this in the data section of the dictionary
	CFDictionarySetValue(dict, kDataString, data);
	CFRelease(data);

// fourth step - save the name from the current preset
	AUPreset au_preset;
	if (DispatchGetProperty(kAudioUnitProperty_CurrentPreset, kAudioUnitScope_Global, 0, &au_preset) == noErr)
		CFDictionarySetValue(dict, kNameString, au_preset.presetName);
*/

#if TARGET_PLUGIN_USES_MIDI
	// we ought to be able to assume that the data is now a CF mutable dictionary
	CFMutableDictionaryRef dict = (CFMutableDictionaryRef) *outData;
	// create a CF data storage thingy for our special data
	CFMutableDataRef cfdata = CFDataCreateMutable(NULL, NULL);
	UInt8 * dfxdata;	// a pointer to our special data
	unsigned long dfxdatasize;	// the number of bytes of our data
	// fetch our special data
	dfxdatasize = dfxsettings->save( (void**)(&dfxdata), true );
	// put our special data into the CF data storage thingy
	CFDataAppendBytes(cfdata, dfxdata, (signed)dfxdatasize);
	// put the CF data storage thingy into the dfx-data section of the CF dictionary
	CFDictionarySetValue(dict, kDfxDataDictionaryKeyString, cfdata);
	// dfx-data belongs to us no more, bye bye...
	CFRelease(cfdata);
	*outData = dict;
#endif

	return noErr;
}

//-----------------------------------------------------------------------------
// restores all parameter values, state info, etc. from the CFPropertyListRef
ComponentResult DfxPlugin::RestoreState(CFPropertyListRef inData)
{
	ComponentResult result = TARGET_API_BASE_CLASS::RestoreState(inData);
	if (result != noErr)
		return result;

/*
	if (CFGetTypeID(inData) != CFDictionaryGetTypeID())
		return kAudioUnitErr_InvalidPropertyValue;

	ComponentDescription desc;
	OSStatus err = GetComponentInfo((Component)GetComponentInstance(), &desc, NULL, NULL, NULL);
	if (err)
		return err;

// first step -> check the saved version in the data ref
// at this point we're only dealing with version==0
	CFNumberRef cfnum = reinterpret_cast<CFNumberRef>(CFDictionaryGetValue((CFDictionaryRef)inData, kVersionString));
	SInt32 value;
	CFNumberGetValue(cfnum, kCFNumberSInt32Type, &value);
	if (value != kCurrentSavedStateVersion)
		return kAudioUnitErr_InvalidPropertyValue;

// second step -> check that this data belongs to this kind of audio unit
// by checking the component type, subtype and manuID
// they MUST all match
	cfnum = reinterpret_cast<CFNumberRef>(CFDictionaryGetValue((CFDictionaryRef)inData, kTypeString));
	CFNumberGetValue(cfnum, kCFNumberSInt32Type, &value);
	if (UInt32(value) != desc.componentType)
		return kAudioUnitErr_InvalidPropertyValue;

	cfnum = reinterpret_cast<CFNumberRef>(CFDictionaryGetValue((CFDictionaryRef)inData, kSubtypeString));
	CFNumberGetValue(cfnum, kCFNumberSInt32Type, &value);
	if (UInt32(value) != desc.componentSubType)
		return kAudioUnitErr_InvalidPropertyValue;

	cfnum = reinterpret_cast<CFNumberRef>(CFDictionaryGetValue((CFDictionaryRef)inData, kManufacturerString));
	CFNumberGetValue(cfnum, kCFNumberSInt32Type, &value);
	if (UInt32(value) != desc.componentManufacturer)
		return kAudioUnitErr_InvalidPropertyValue;

// third step -> restore the state of all of the parameters for each scope and element	
	CFDataRef data = reinterpret_cast<CFDataRef>(CFDictionaryGetValue((CFDictionaryRef)inData, kDataString));
	if (data == NULL)
		return kAudioUnitErr_InvalidPropertyValue;

	const UInt8 *p, *pend;
	
	p = CFDataGetBytePtr(data);
	pend = p + CFDataGetLength(data);
	
	// we have a zero length data, which may just mean there were no parameters to save!
	//	if (p >= pend) return noErr; 

	while (p < pend)
	{
		struct {
			UInt32	scope;
			UInt32	element;
		} hdr;
		
		hdr.scope = CFSwapInt32BigToHost(*(UInt32 *)p);		p += sizeof(UInt32);
		hdr.element = CFSwapInt32BigToHost(*(UInt32 *)p);	p += sizeof(UInt32);
		
		AUScope &scope = GetScope(hdr.scope);
		AUElement *element = scope.SafeGetElement(hdr.element);
			// $$$ order of operations issue: what if the element does not yet exist?
		p = element->RestoreState(p);
	}
	// XXX a cheap hack for now...
	for (long i=0; i < numParameters; i++)
		setparameter_f(i, TARGET_API_BASE_CLASS::GetParameter(i));

// fourth step - restore the name from the document
	CFStringRef name = reinterpret_cast<CFStringRef>(CFDictionaryGetValue((CFDictionaryRef)inData, kNameString));
	AUPreset au_preset;
	au_preset.presetNumber = -1;
	if (name)
		au_preset.presetName = name;
	else
		// no name entry make the default one
		au_preset.presetName = CFSTR("Untitled");
	// set mCurrentPreset with this new preset data
	DispatchSetProperty(kAudioUnitProperty_CurrentPreset, kAudioUnitScope_Global, 0, &au_preset, sizeof(AUPreset));
*/

#if TARGET_PLUGIN_USES_MIDI
	CFDataRef cfdata = reinterpret_cast<CFDataRef>(CFDictionaryGetValue((CFDictionaryRef)inData, kDfxDataDictionaryKeyString));
	if (cfdata == NULL)
		return kAudioUnitErr_InvalidPropertyValue;

	// a pointer to our special data
	const UInt8 * dfxdata = CFDataGetBytePtr(cfdata);
	// the number of bytes of our data
	unsigned long dfxdatasize = (unsigned) CFDataGetLength(cfdata);
	bool success = dfxsettings->restore((void*)dfxdata, dfxdatasize, true);
	if (!success)
		return kAudioUnitErr_InvalidPropertyValue;
	
#else
	for (long i=0; i < numParameters; i++)
		setparameter_f(i, TARGET_API_BASE_CLASS::GetParameter(i));

#endif
// TARGET_PLUGIN_USES_MIDI


	return noErr;
}



#pragma mark _________dsp_________

//-----------------------------------------------------------------------------
ComponentResult DfxPlugin::ChangeStreamFormat(AudioUnitScope inScope, AudioUnitElement inElement, 
				const CAStreamBasicDescription &inPrevFormat, const CAStreamBasicDescription &inNewFormat)
{
printf("\nDfxPlugin::ChangeStreamFormat,   newsr = %.3f,   oldsr = %.3f\n\n", inNewFormat.mSampleRate, inPrevFormat.mSampleRate);
	ComponentResult result = TARGET_API_BASE_CLASS::ChangeStreamFormat(inScope, inElement, inPrevFormat, inNewFormat);

	if (result == noErr)
	{
//		if (inNewFormat.mSampleRate != inPrevFormat.mSampleRate)
		if ( (inNewFormat.mSampleRate != getsamplerate()) || 
				(inNewFormat.mChannelsPerFrame != inPrevFormat.mChannelsPerFrame) )
		{
//			do_initialize();
			updatesamplerate();
			createbuffers();
		}
	}

	return result;
}

//-----------------------------------------------------------------------------
OSStatus DfxPlugin::ProcessBufferLists(AudioUnitRenderActionFlags &ioActionFlags, 
				const AudioBufferList &inBuffer, AudioBufferList &outBuffer, 
				UInt32 inFramesToProcess)
{
	OSStatus result = noErr;
	preprocessaudio();

#if TARGET_PLUGIN_USES_DSPCORE
	result = TARGET_API_BASE_CLASS::ProcessBufferLists(ioActionFlags, inBuffer, outBuffer, inFramesToProcess);

#else
	UInt32 inNumBuffers = inBuffer.mNumberBuffers;
	UInt32 outNumBuffers = outBuffer.mNumberBuffers;
	// can't have less than 1 in or out stream
	if ( (inNumBuffers < 1) || (outNumBuffers < 1) )
		return kAudioUnitErr_FormatNotSupported;

	for (UInt32 i=0; i < numInputs; i++)
		inputsP[i] = (float*)inBuffer.mBuffers[i].mData;
	for (UInt32 i=0; i < numOutputs; i++)
	{
		outputsP[i] = (float*)outBuffer.mBuffers[i].mData;
		outBuffer.mBuffers[i].mDataByteSize = inFramesToProcess * sizeof(Float32);
	}

	// now do the processing
	processaudio(inputsP, outputsP, inFramesToProcess);

	// I don't know what the hell this is for
	ioActionFlags &= ~kAudioUnitRenderAction_OutputIsSilence;

#endif

	postprocessaudio();

	return result;
}



#pragma mark _________MIDI_________

#if TARGET_PLUGIN_USES_MIDI

//-----------------------------------------------------------------------------
void DfxPlugin::HandleNoteOn(int inChannel, UInt8 inNoteNumber, 
						UInt8 inVelocity, long inFrameOffset)
{
	handlemidi_noteon(inChannel, inNoteNumber, inVelocity, inFrameOffset);
}

//-----------------------------------------------------------------------------
void DfxPlugin::HandleNoteOff(int inChannel, UInt8 inNoteNumber, 
						UInt8 inVelocity, long inFrameOffset)
{
	handlemidi_noteoff(inChannel, inNoteNumber, inVelocity, inFrameOffset);
}

//-----------------------------------------------------------------------------
void DfxPlugin::HandleAllNotesOff(int inChannel)
{
	handlemidi_allnotesoff(inChannel, 0);
}

//-----------------------------------------------------------------------------
void DfxPlugin::HandlePitchWheel(int inChannel, UInt8 inPitchLSB, UInt8 inPitchMSB, 
						long inFrameOffset)
{
	handlemidi_pitchbend(inChannel, inPitchLSB, inPitchMSB, inFrameOffset);
}

//-----------------------------------------------------------------------------
void DfxPlugin::HandleControlChange(int inChannel, UInt8 inController, 
						UInt8 inValue, long inFrameOffset)
{
	handlemidi_cc(inChannel, inController, inValue, inFrameOffset);
}

//-----------------------------------------------------------------------------
void DfxPlugin::HandleProgramChange(int inChannel, UInt8 inProgramNum)
{
	handlemidi_programchange(inChannel, inProgramNum, 0);
	// XXX maybe this is really all we want to do?
	loadpreset(inProgramNum);
}

#endif
// TARGET_PLUGIN_USES_MIDI
