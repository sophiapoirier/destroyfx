/*------------------------------------------------------------------------
Destroy FX is a sovereign entity comprised of Marc Poirier & Tom Murphy 7.  
This is our class for E-Z plugin-making and E-Z multiple-API support.
This is where we connect the Audio Unit API to our DfxPlugin system.
written by Marc Poirier, October 2002
------------------------------------------------------------------------*/

#include "dfxplugin.h"



#pragma mark _________init_________

//-----------------------------------------------------------------------------
// this is called immediately after an instance of the plugin class is created
void DfxPlugin::PostConstructor()
{
	TARGET_API_BASE_CLASS::PostConstructor();

	dfxplugin_postconstructor();

	// make host see that current preset is 0
	update_preset(0);
}

//-----------------------------------------------------------------------------
// this is like a second constructor, kind of
// it is called when the Audio Unit is expected to be ready to process audio
// this is where DSP-specific resources should be allocated
ComponentResult DfxPlugin::Initialize()
{
	// call the inherited class' Initialize routine
	ComponentResult result = TARGET_API_BASE_CLASS::Initialize();

	// call our initialize routine
	if (result == noErr)
		result = do_initialize();

	// AU hosts aren't required to call Reset between Initialize and 
	// beginning audio processing, so this makes sure that Reset happens, 
	// in case the plugin depends on that
	if (result == noErr)
		Reset();

	return result;
}

//-----------------------------------------------------------------------------
// this is the "destructor" partner to Initialize
// any DSP-specific resources should be released here
void DfxPlugin::Cleanup()
{
	TARGET_API_BASE_CLASS::Cleanup();	// XXX doesn't actually do anything now, but maybe at some point?

	do_cleanup();
}

//-----------------------------------------------------------------------------
// this is called when an audio stream is broken somehow 
// (playback stop/restart, change of playback position, etc.)
// any DSP state variables should be reset here 
// (contents of buffers, position trackers, IIR filter histories, etc.)
void DfxPlugin::Reset()
{
	do_reset();
}



#pragma mark _________info_________

//-----------------------------------------------------------------------------
// get basic information about Audio Unit Properties 
// (whether they're supported, writability, and data size)
// most properties are handled by inherited base class implementations
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
		case kAudioUnitProperty_MIDIControlMapping:
			outDataSize = sizeof(AudioUnitMIDIControlMapping*);
			outWritable = false;
			break;
	#endif

		// this allows a GUI component to get a pointer to our DfxPlugin class instance
		case kDfxPluginProperty_PluginPtr:
			outDataSize = sizeof(DfxPlugin*);
			outWritable = false;
			break;

		// randomize the parameters
		case kDfxPluginProperty_RandomizeParameters:
			// when you "set" this "property", you send a bool to say whether or not to write automation data
			outDataSize = sizeof(bool);
			outWritable = true;
			break;

	#if TARGET_PLUGIN_USES_MIDI
		// get/set the MIDI learn state
		case kDfxPluginProperty_MidiLearn:
			outDataSize = sizeof(bool);
			outWritable = true;
			break;
		// clear MIDI parameter assignments
		case kDfxPluginProperty_ResetMidiLearn:
			outDataSize = sizeof(bool);	// whatever, you don't need an input value for this property
			outWritable = true;
			break;
		// get/set the current MIDI learner parameter
		case kDfxPluginProperty_MidiLearner:
			outDataSize = sizeof(long);
			outWritable = true;
			break;
	#endif

		default:
			result = TARGET_API_BASE_CLASS::GetPropertyInfo(inID, inScope, inElement, outDataSize, outWritable);
			break;
	}

	return result;
}

//-----------------------------------------------------------------------------
// get specific information about Audio Unit Properties
// most properties are handled by inherited base class implementations
ComponentResult DfxPlugin::GetProperty(AudioUnitPropertyID inID, 
					AudioUnitScope inScope, AudioUnitElement inElement, 
					void *outData)
{
	ComponentResult result = noErr;

	switch (inID)
	{
	#if TARGET_PLUGIN_USES_MIDI
		case kAudioUnitProperty_MIDIControlMapping:
			if (outData == NULL)
				result = kAudioUnitErr_InvalidPropertyValue;
			else
			{
				for (long i=0; i < numParameters; i++)
				{
					aumidicontrolmap[i].midiNRPN = 0xFFFF;	// this means "none"
					aumidicontrolmap[i].midiControl = 0xFF;	// this means "none"
					// but we might still have a CC assignment to share...
					if (dfxsettings != NULL)
					{
						if (dfxsettings->getParameterAssignmentType(i) == kParamEventCC)
							aumidicontrolmap[i].midiControl = dfxsettings->getParameterAssignmentNum(i);
					}
					aumidicontrolmap[i].scope = kAudioUnitScope_Global;
					aumidicontrolmap[i].element = (AudioUnitElement) 0;
					aumidicontrolmap[i].parameter = (AudioUnitParameterID) i;
				}
				*(AudioUnitMIDIControlMapping**)outData = aumidicontrolmap;
			}
			break;
	#endif

		// this allows a GUI component to get a pointer to our DfxPlugin class instance
		case kDfxPluginProperty_PluginPtr:
			if (outData == NULL)
				result = kAudioUnitErr_InvalidPropertyValue;
			else
				*(DfxPlugin**)outData = this;
			break;

	#if TARGET_PLUGIN_USES_MIDI
		// get the MIDI learn state
		case kDfxPluginProperty_MidiLearn:
			if (outData == NULL)
				result = kAudioUnitErr_InvalidPropertyValue;
			else
				*(bool*)outData = dfxsettings->isLearning();
			break;
		// get the current MIDI learner parameter
		case kDfxPluginProperty_MidiLearner:
			if (outData == NULL)
				result = kAudioUnitErr_InvalidPropertyValue;
			else
				*(long*)outData = dfxsettings->getLearner();
			break;
	#endif

		default:
			result = TARGET_API_BASE_CLASS::GetProperty(inID, inScope, inElement, outData);
			break;
	}

	return result;
}

//-----------------------------------------------------------------------------
ComponentResult DfxPlugin::SetProperty(AudioUnitPropertyID inID, 
					AudioUnitScope inScope, AudioUnitElement inElement, 
					const void *inData, UInt32 inDataSize)
{
	ComponentResult result = noErr;

	switch (inID)
	{
	#if TARGET_PLUGIN_USES_MIDI
		case kAudioUnitProperty_MIDIControlMapping:
			result = kAudioUnitErr_PropertyNotWritable;
			break;
	#endif

		case kDfxPluginProperty_PluginPtr:
			result = kAudioUnitErr_PropertyNotWritable;
			break;

		// randomize the parameters
		case kDfxPluginProperty_RandomizeParameters:
			if (inData == NULL)
				result = kAudioUnitErr_InvalidPropertyValue;
			else
				// when you "set" this "property", you send a bool to say whether or not to write automation data
				randomizeparameters( *(bool*)inData );
			break;

	#if TARGET_PLUGIN_USES_MIDI
		// set the MIDI learn state
		case kDfxPluginProperty_MidiLearn:
			if (inData == NULL)
				result = kAudioUnitErr_InvalidPropertyValue;
			else
				dfxsettings->setParameterMidiLearn( *(bool*)inData );
			break;
		// clear MIDI parameter assignments
		case kDfxPluginProperty_ResetMidiLearn:
			// you don't need an input value for this property
			dfxsettings->setParameterMidiReset();
			break;
		// set the current MIDI learner parameter
		case kDfxPluginProperty_MidiLearner:
			if (inData == NULL)
				result = kAudioUnitErr_InvalidPropertyValue;
			else
				dfxsettings->setLearner( *(long*)inData );
			break;
	#endif

		default:
			result = TARGET_API_BASE_CLASS::SetProperty(inID, inScope, inElement, inData, inDataSize);
			break;
	}

	return result;
}

//-----------------------------------------------------------------------------
// give the host an array of the audio input/output channel configurations 
// that the plugin supports
// if the pointer passed in is NULL, then simply return the number of supported configurations
// if any n-to-n configuration (i.e. same number of ins and outs) is supported, return 0
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

#if TARGET_PLUGIN_HAS_GUI
//-----------------------------------------------------------------------------
int DfxPlugin::GetNumCustomUIComponents()
{
	return 1;
}

//-----------------------------------------------------------------------------
void DfxPlugin::GetUIComponentDescs(ComponentDescription *inDescArray)
{
	inDescArray->componentType = kAudioUnitCarbonViewComponentType;
	#ifdef PLUGIN_EDITOR_ID
		inDescArray->componentSubType = PLUGIN_EDITOR_ID;
	#else
		inDescArray->componentSubType = PLUGIN_ID;
	#endif
	inDescArray->componentManufacturer = DESTROYFX_ID;
	inDescArray->componentFlags = 0;
	inDescArray->componentFlagsMask = 0;
}
#endif



#pragma mark _________parameters_________

//-----------------------------------------------------------------------------
// get specific information about the properties of a parameter
ComponentResult DfxPlugin::GetParameterInfo(AudioUnitScope inScope, 
					AudioUnitParameterID inParameterID, 
					AudioUnitParameterInfo &outParameterInfo)
{
	// we're only handling the global scope
	if (inScope != kAudioUnitScope_Global)
		return kAudioUnitErr_InvalidScope;

	if ( !parameterisvalid(inParameterID) )
		return kAudioUnitErr_InvalidParameter;

	getparametername(inParameterID, outParameterInfo.name);
	outParameterInfo.minValue = getparametermin_f(inParameterID);
	outParameterInfo.maxValue = getparametermax_f(inParameterID);
	outParameterInfo.defaultValue = getparameterdefault_f(inParameterID);
	// if the parameter is hidden, then indicate that it's not readable or writable...
	if (getparameterhidden(inParameterID))
		outParameterInfo.flags = 0;
	// ...otherwise all parameters are readable and writable
	else
		outParameterInfo.flags = kAudioUnitParameterFlag_IsReadable 
								| kAudioUnitParameterFlag_IsWritable;

	// the complicated part:  getting the unit type, 
	// but easy if we use value strings for value display
	if (getparameterusevaluestrings(inParameterID))
		outParameterInfo.unit = kAudioUnitParameterUnit_Indexed;
	else
	{
		switch (getparameterunit(inParameterID))
		{
			case kDfxParamUnit_generic:
				outParameterInfo.unit = kAudioUnitParameterUnit_Generic;
				break;
			case kDfxParamUnit_quantity:
				outParameterInfo.unit = kAudioUnitParameterUnit_Indexed;	// XXX assume it's an integer?
				break;
			case kDfxParamUnit_percent:
				outParameterInfo.unit = kAudioUnitParameterUnit_Percent;
				break;
			case kDfxParamUnit_portion:
				outParameterInfo.unit = kAudioUnitParameterUnit_Generic;
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
			case kDfxParamUnit_hz:
				outParameterInfo.unit = kAudioUnitParameterUnit_Hertz;
				break;
			case kDfxParamUnit_seconds:
				outParameterInfo.unit = kAudioUnitParameterUnit_Seconds;
				break;
			case kDfxParamUnit_ms:
				outParameterInfo.unit = kAudioUnitParameterUnit_Milliseconds;
				break;
			case kDfxParamUnit_samples:
				outParameterInfo.unit = kAudioUnitParameterUnit_SampleFrames;
				break;
			case kDfxParamUnit_scalar:
				outParameterInfo.unit = kAudioUnitParameterUnit_Rate;
				break;
			case kDfxParamUnit_divisor:
				outParameterInfo.unit = kAudioUnitParameterUnit_Generic;
				break;
			case kDfxParamUnit_exponent:
				outParameterInfo.unit = kAudioUnitParameterUnit_Generic;
				break;
			case kDfxParamUnit_semitones:
				outParameterInfo.unit = kAudioUnitParameterUnit_RelativeSemiTones;
				break;
			case kDfxParamUnit_octaves:
				outParameterInfo.unit = kAudioUnitParameterUnit_Octaves;
				break;
			case kDfxParamUnit_cents:
				outParameterInfo.unit = kAudioUnitParameterUnit_Cents;
				break;
			case kDfxParamUnit_notes:
				outParameterInfo.unit = kAudioUnitParameterUnit_MIDINoteNumber;
				break;
			case kDfxParamUnit_pan:
				outParameterInfo.unit = kAudioUnitParameterUnit_Pan;
				break;
			case kDfxParamUnit_bpm:
				outParameterInfo.unit = kAudioUnitParameterUnit_BPM;
				break;
			case kDfxParamUnit_beats:
				outParameterInfo.unit = kAudioUnitParameterUnit_Beats;
				break;
			case kDfxParamUnit_index:
				outParameterInfo.unit = kAudioUnitParameterUnit_Indexed;
				break;
			case kDfxParamUnit_custom:
				outParameterInfo.unit = kAudioUnitParameterUnit_Generic;
				break;

			default:
				// if we got to this point, try using the value type to determine the unit type
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
	}


	return noErr;
}

//-----------------------------------------------------------------------------
// give the host an array of CFStrings with the display values for an indexed parameter
ComponentResult DfxPlugin::GetParameterValueStrings(AudioUnitScope inScope, 
					AudioUnitParameterID inParameterID, CFArrayRef *outStrings)
{
	// we're only handling the global scope
	if (inScope != kAudioUnitScope_Global)
		return kAudioUnitErr_InvalidScope;

	if (!parameterisvalid(inParameterID))
		return kAudioUnitErr_InvalidProperty;

	if (getparameterusevaluestrings(inParameterID))
	{
		CFStringRef * outArray = getparametervaluecfstrings(inParameterID);
		// if we don't actually have strings set, exit with an error
		if (outArray == NULL)
			return kAudioUnitErr_InvalidProperty;
		// in case the min is not 0, get the total count of items in the array
		long numStrings = getparametermax_i(inParameterID) - getparametermin_i(inParameterID) + 1;
		// create a CFArray of the strings (the host will destroy the CFArray)
		*outStrings = CFArrayCreate(kCFAllocatorDefault, (const void**)outArray, numStrings, NULL);
		return noErr;
	}

	return kAudioUnitErr_InvalidProperty;
}

//-----------------------------------------------------------------------------
ComponentResult DfxPlugin::SetParameter(AudioUnitParameterID inID, 
					AudioUnitScope inScope, AudioUnitElement inElement, 
					Float32 inValue, UInt32 inBufferOffsetInFrames)
{
	if (inScope != kAudioUnitScope_Global)
		return kAudioUnitErr_InvalidScope;
	if (inElement != 0)
		return kAudioUnitErr_InvalidElement;
	if ( !parameterisvalid(inID) )
		return kAudioUnitErr_InvalidParameter;

	setparameter_f(inID, inValue);
	return noErr;
//	return AUBase::SetParameter(inID, inScope, inElement, inValue, inBufferOffsetInFrames);
}



#pragma mark _________presets_________

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
	CFMutableArrayRef outArray = CFArrayCreateMutable(kCFAllocatorDefault, outNumPresets, NULL);

	// add the preset data (name and number) into the array
	for (long i=0; i < numPresets; i++)
	{
//		if (presetnameisvalid(i))
		if ( (presets[i].getname_ptr() != NULL) && (presets[i].getname_ptr()[0] != 0) )
		{
			aupresets[i].presetNumber = i;
//			aupresets[i].presetName = getpresetcfname(i);
			aupresets[i].presetName = presets[i].getcfname();
			CFArrayAppendValue(outArray, &(aupresets[i]));
		}
	}

	*outData = (CFArrayRef)outArray;

	return noErr;
}

//-----------------------------------------------------------------------------
// this is called as a request to load a preset
OSStatus DfxPlugin::NewFactoryPresetSet(const AUPreset & inNewFactoryPreset)
{
	long newNumber = inNewFactoryPreset.presetNumber;

	if ( !presetisvalid(newNumber) )
		return kAudioUnitErr_InvalidPropertyValue;
	// for AU, we are using invalid preset names as a way of saying "not a real preset," 
	// even though it might be a valid (allocated) preset number
	if ( !presetnameisvalid(newNumber) )
		return kAudioUnitErr_InvalidPropertyValue;

	// try to load the preset
	if ( !loadpreset(newNumber) )
		return kAudioUnitErr_InvalidPropertyValue;

	return noErr;
}



#pragma mark _________state_________

static const CFStringRef kDfxDataDictionaryKeyString = CFSTR("destroyfx-data");
//-----------------------------------------------------------------------------
// stores the values of all parameters values, state info, etc. into a CFPropertyListRef
ComponentResult DfxPlugin::SaveState(CFPropertyListRef *outData)
{
	ComponentResult result = TARGET_API_BASE_CLASS::SaveState(outData);
	if (result != noErr)
		return result;

#if TARGET_PLUGIN_USES_MIDI
	// create a CF data storage thingy for our special data
	CFMutableDataRef cfdata = CFDataCreateMutable(kCFAllocatorDefault, 0);
	void * dfxdata;	// a pointer to our special data
	unsigned long dfxdatasize;	// the number of bytes of our data
	// fetch our special data
	dfxdatasize = dfxsettings->save( &dfxdata, true );
	// put our special data into the CF data storage thingy
	CFDataAppendBytes(cfdata, (UInt8*)dfxdata, (signed)dfxdatasize);
	// put the CF data storage thingy into the dfx-data section of the CF dictionary
	CFDictionarySetValue((CFMutableDictionaryRef)(*outData), kDfxDataDictionaryKeyString, cfdata);
	// cfdata belongs to us no more, bye bye...
	CFRelease(cfdata);
#endif

	return noErr;
}

#define DEBUG_VST_SETTINGS_IMPORT	0
//-----------------------------------------------------------------------------
// restores all parameter values, state info, etc. from the CFPropertyListRef
ComponentResult DfxPlugin::RestoreState(CFPropertyListRef inData)
{
	ComponentResult result = TARGET_API_BASE_CLASS::RestoreState(inData);

#if TARGET_PLUGIN_USES_MIDI
#if DEBUG_VST_SETTINGS_IMPORT
printf("\nresult from AUBase::RestoreState was %ld\n", result);
#endif
	CFDataRef cfdata = NULL;

	if (result == noErr)
	{
		// look for a data section keyed with our custom data key
		cfdata = reinterpret_cast<CFDataRef>(CFDictionaryGetValue((CFDictionaryRef)inData, kDfxDataDictionaryKeyString));
		// failing that, try to see if old VST chunk data is being fed to us
		if (cfdata == NULL)
{
			cfdata = reinterpret_cast<CFDataRef>(CFDictionaryGetValue((CFDictionaryRef)inData, CFSTR("vstdata")));
#if DEBUG_VST_SETTINGS_IMPORT
printf("destroyfx-data was not there, trying vstdata...\n");
if (cfdata == NULL) printf("vstdata was not there\n");
else printf("vstdata was there, loading...\n");
#endif
}
	}
	// there was an error in AUBas::RestoreState, but maybe some keys were missing and "vstdata" is there...
	else
{
		cfdata = reinterpret_cast<CFDataRef>(CFDictionaryGetValue((CFDictionaryRef)inData, CFSTR("vstdata")));
#if DEBUG_VST_SETTINGS_IMPORT
printf("AUBase::RestoreState failed, trying vstdata...\n");
if (cfdata == NULL) printf("vstdata was not there\n");
else printf("vstdata was there, loading...\n");
#endif
}

	// if we couldn't get any data, abort with an error
	if (cfdata == NULL)
		return kAudioUnitErr_InvalidPropertyValue;

	// a pointer to our special data
	const UInt8 * dfxdata = CFDataGetBytePtr(cfdata);
	// the number of bytes of our data
	unsigned long dfxdatasize = (unsigned) CFDataGetLength(cfdata);
	// try to restore the saved settings data
	bool success = dfxsettings->restore((void*)dfxdata, dfxdatasize, true);
#if DEBUG_VST_SETTINGS_IMPORT
if (success) printf("settings data was successfully loaded\n");
else printf("settings data failed to load\n");
#endif
	if (!success)
		return kAudioUnitErr_InvalidPropertyValue;

#else
	// abort if the base implementation of RestoreState failed
	if (result != noErr)
		return result;
// XXX should we rethink this and load parameter settings if dfxsettings->restore() fails, or always before dfxsettings->restore()?
	// load the parameter settings that were restored 
	// by the inherited base class implementation of RestoreState
	for (long i=0; i < numParameters; i++)
		setparameter_f(i, TARGET_API_BASE_CLASS::GetParameter(i));

#endif
// TARGET_PLUGIN_USES_MIDI


	return noErr;
}



#pragma mark _________dsp_________

//-----------------------------------------------------------------------------
// the host calls this to inform the plugin that it wants to start using 
// a different audio stream format (sample rate, num channels, etc.)
ComponentResult DfxPlugin::ChangeStreamFormat(AudioUnitScope inScope, AudioUnitElement inElement, 
				const CAStreamBasicDescription &inPrevFormat, const CAStreamBasicDescription &inNewFormat)
{
//printf("\nDfxPlugin::ChangeStreamFormat,   newsr = %.3f,   oldsr = %.3f\n\n", inNewFormat.mSampleRate, inPrevFormat.mSampleRate);
	// just use the inherited base class implementation
	ComponentResult result = TARGET_API_BASE_CLASS::ChangeStreamFormat(inScope, inElement, inPrevFormat, inNewFormat);

	if ( (result == noErr) && IsInitialized() )
	{
		// react to changes in the number of channels or sampling rate
		if ( (inNewFormat.mSampleRate != inPrevFormat.mSampleRate) || 
//		if ( (inNewFormat.mSampleRate != getsamplerate()) || 
				(inNewFormat.mChannelsPerFrame != inPrevFormat.mChannelsPerFrame) )
		{
			updatesamplerate();
			updatenumchannels();
			createbuffers();
		}
	}

	return result;
}

//-----------------------------------------------------------------------------
// this is the audio processing routine
OSStatus DfxPlugin::ProcessBufferLists(AudioUnitRenderActionFlags &ioActionFlags, 
				const AudioBufferList &inBuffer, AudioBufferList &outBuffer, 
				UInt32 inFramesToProcess)
{
	OSStatus result = noErr;

	// do any pre-DSP prep
	preprocessaudio();

#if TARGET_PLUGIN_USES_DSPCORE
	// if the plugin uses DSP cores, then we just call the 
	// inherited base class implementation, which handles "Kernels"
	result = TARGET_API_BASE_CLASS::ProcessBufferLists(ioActionFlags, inBuffer, outBuffer, inFramesToProcess);

#else
	UInt32 inNumBuffers = inBuffer.mNumberBuffers;
	UInt32 outNumBuffers = outBuffer.mNumberBuffers;
	// can't have less than 1 in or out stream
	if ( (inNumBuffers < 1) || (outNumBuffers < 1) )
		result = kAudioUnitErr_FormatNotSupported;

	if (result == noErr)
	{
		// set up our more convenient audio stream pointers
		for (UInt32 i=0; i < numInputs; i++)
			inputsP[i] = (float*) (inBuffer.mBuffers[i].mData);
		for (UInt32 i=0; i < numOutputs; i++)
		{
			outputsP[i] = (float*) (outBuffer.mBuffers[i].mData);
			outBuffer.mBuffers[i].mDataByteSize = inFramesToProcess * sizeof(Float32);
		}

		// now do the processing
		processaudio((const float**)inputsP, outputsP, inFramesToProcess);

		// I don't know what the hell this is for
		ioActionFlags &= ~kAudioUnitRenderAction_OutputIsSilence;
	}

#endif
// end of if/else TARGET_PLUGIN_USES_DSPCORE

	// do any post-DSP stuff
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
