/*
Copyright (c) 2004 bioroid media development
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer. 
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution. 
* Neither the name of "bioroid media development" nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission. 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Additional changes since June 15th 2004 have been made by Sophia Poirier.  
Copyright (C) 2004-2007 Sophia Poirier
*/

#include "turntablist.h"



/////////////////////////////
// DEFINES

// allow pitch bend scratching
#define USE_MIDI_PITCH_BEND
// use hard-coded MIDI CCs for parameters
//#define USE_MIDI_CC

const long k_nScratchInterval = 16; //24
const long k_nPowerInterval = 120;
const long k_nRootKey_default = 60;   // middle C

const double k_fScratchAmountMiddlePoint = 0.0;
const double k_fScratchAmountLaxRange = 0.000000001;
const double k_fScratchAmountMiddlePoint_UpperLimit = k_fScratchAmountMiddlePoint + k_fScratchAmountLaxRange;
const double k_fScratchAmountMiddlePoint_LowerLimit = k_fScratchAmountMiddlePoint - k_fScratchAmountLaxRange;



#ifdef USE_LIBSNDFILE
	#include "sndfile.h"
#else
	#include <AudioToolbox/ExtendedAudioFile.h>
#endif



//-----------------------------------------------------------------------------------------
// Turntablist
//-----------------------------------------------------------------------------------------
// this macro does boring entry point stuff for us
DFX_ENTRY(Turntablist)

//-----------------------------------------------------------------------------------------
Turntablist::Turntablist(TARGET_API_BASE_INSTANCE_TYPE inInstance)
	: DfxPlugin(inInstance, kNumParameters, 0)
{
	initparameter_f(kParam_ScratchAmount, "scratch amount", k_fScratchAmountMiddlePoint, k_fScratchAmountMiddlePoint, -1.0, 1.0, kDfxParamUnit_generic);	// float2string(m_fPlaySampleRate, text);
//	initparameter_f(kScratchSpeed, "scratch speed", 0.33333333, 0.33333333, 0.0, 1.0, kDfxParamUnit_scalar);
	initparameter_f(kParam_ScratchSpeed_scrub, "scratch speed (scrub mode)", 2.0, 2.0, 0.5, 5.0, kDfxParamUnit_seconds);
	initparameter_f(kParam_ScratchSpeed_spin, "scratch speed (spin mode)", 3.0, 3.0, 1.0, 8.0, kDfxParamUnit_scalar);

	initparameter_b(kParam_Power, "power", true, true);
	initparameter_f(kParam_SpinUpSpeed, "spin up speed", 0.063, 0.05, 0.0001, 1.0, kDfxParamUnit_scalar, kDfxParamCurve_log);
	initparameter_f(kParam_SpinDownSpeed, "spin down speed", 0.09, 0.05, 0.0001, 1.0, kDfxParamUnit_scalar, kDfxParamCurve_log);
	initparameter_b(kParam_NotePowerTrack, "note-power track", false, false);

	initparameter_f(kParam_PitchShift, "pitch shift", 0.0, 0.0, -100.0, 100.0, kDfxParamUnit_percent);
	initparameter_f(kParam_PitchRange, "pitch range", 12.0, 12.0, 1.0, 36.0, kDfxParamUnit_semitones);
	initparameter_b(kParam_KeyTracking, "key track", false, false);
	initparameter_i(kParam_RootKey, "root key", k_nRootKey_default, k_nRootKey_default, 0, 0x7F, kDfxParamUnit_notes);

	initparameter_b(kParam_Loop, "loop", true, false);

	initparameter_indexed(kParam_ScratchMode, "scratch mode", kScratchMode_Scrub, kScratchMode_Scrub, kNumScratchModes);
	setparametervaluestring(kParam_ScratchMode, kScratchMode_Scrub, "scrub");
	setparametervaluestring(kParam_ScratchMode, kScratchMode_Spin, "spin");

	initparameter_indexed(kParam_Direction, "playback direction", kScratchDirection_Forward, kScratchDirection_Forward, kNumScratchDirections);
	setparametervaluestring(kParam_Direction, kScratchDirection_Forward, "forward");
	setparametervaluestring(kParam_Direction, kScratchDirection_Backward, "reverse");

	initparameter_indexed(kParam_NoteMode, "note mode", kNoteMode_Reset, kNoteMode_Reset, kNumNoteModes);
	setparametervaluestring(kParam_NoteMode, kNoteMode_Reset, "reset");
	setparametervaluestring(kParam_NoteMode, kNoteMode_Resume, "resume");

	initparameter_b(kParam_PlayTrigger, "playback trigger", false, false);

#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
	initparameter_b(kParam_Mute, "mute", false, false);
	initparameter_f(kParam_Volume, "volume", 1.0, 0.5, 0.0, 1.0, kDfxParamUnit_lineargain, kDfxParamCurve_cubed);
#endif


#ifdef USE_LIBSNDFILE
	addchannelconfig(0, 2);	// stereo-out
	addchannelconfig(0, 1);	// mono-out
#else
	addchannelconfig(0, -1);	// 0-in / N-out
#endif

	// XXX This plugin doesn't support nodes because of the problem of passing a file reference 
	// as custom property data between different machines on a network.  
	// There is no reliable way to do that, so far as I know.
	setSupportedLogicNodeOperationMode(0);

#ifdef USE_LIBSNDFILE
	m_fBuffer = NULL;
	m_fLeft = NULL;
	m_fRight = NULL;
#endif

	m_nNumChannels = 0;
	m_nNumSamples = 0;
	m_nSampleRate = 0;
	m_fSampleRate = 0.0;


	m_bPlayedReverse = false;
	m_bPlay = false;

	m_fPosition = 0.0;
	m_fPosOffset = 0.0;
	m_fNumSamples = 0.0;

	m_fLastScratchAmount = getparameter_f(kParam_ScratchAmount);
	m_fPitchBend = k_fScratchAmountMiddlePoint;
	m_bPitchBendSet = false;
	m_bScratching = false;
	m_bWasScratching = false;
	m_bPlayForward = true;
	m_nScratchSubMode = 1;	// speed based from scrub scratch mode

	m_nScratchDelay = 0;

	m_nRootKey = getparameter_i(kParam_RootKey);

	m_nCurrentNote = m_nRootKey;
	m_nCurrentVelocity = 0x7F;

	m_bAudioFileHasBeenLoaded = false;
	m_bScratchStop = false;

	memset(&m_fsAudioFile, 0, sizeof(m_fsAudioFile));

	m_fScratchVolumeModifier = 0.0f;

	m_bScratchAmountSet = false;
	m_fDesiredScratchRate = 0.0;

	m_nScratchInterval = 0;

	m_fDesiredPosition = 0.0;
	m_fPrevDesiredPosition = 0.0;

	// XXX yah?  for now...
	if (dfxsettings != NULL)
	{
		dfxsettings->setSteal(true);
		dfxsettings->setUseChannel(false);
	}
}

//-----------------------------------------------------------------------------------------
Turntablist::~Turntablist()
{
#ifdef USE_LIBSNDFILE
	if (m_fBuffer != NULL)
		free(m_fBuffer);
	m_fBuffer = NULL;
#else
	m_auBufferList.Deallocate();
#endif
}


//-----------------------------------------------------------------------------------------
long Turntablist::initialize()
{
	if (sampleratechanged)
	{
		m_nPowerIntervalEnd = (int)getsamplerate() / k_nPowerInterval;	// set process interval to 10 times a second - should maybe update it to 60?
		m_nScratchIntervalEnd = (int)getsamplerate() / k_nScratchInterval; // set scratch interval end to 1/16 second
		m_nScratchIntervalEndBase = m_nScratchIntervalEnd;
	}

	stopNote();

	return kDfxErr_NoError;
}


//-----------------------------------------------------------------------------------------
void Turntablist::processparameters()
{
	m_bPower = getparameter_b(kParam_Power);
	m_bNotePowerTrack = getparameter_b(kParam_NotePowerTrack);
	m_bLoop = getparameter_b(kParam_Loop);
	m_bKeyTracking = getparameter_b(kParam_KeyTracking);

	m_fPitchShift = getparameter_scalar(kParam_PitchShift);
#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
	m_fVolume = getparameter_f(kParam_Volume);
#endif
	m_fPitchRange = getparameter_f(kParam_PitchRange);

	m_fScratchAmount = getparameter_f(kParam_ScratchAmount);
//	m_fScratchSpeed = getparameter_f(kScratchSpeed);
	m_fScratchSpeed_scrub = getparameter_f(kParam_ScratchSpeed_scrub);
	m_fScratchSpeed_spin = getparameter_f(kParam_ScratchSpeed_spin);
	m_fSpinUpSpeed = getparameter_f(kParam_SpinUpSpeed);
	m_fSpinDownSpeed = getparameter_f(kParam_SpinDownSpeed);

	m_nRootKey = getparameter_i(kParam_RootKey);

	m_nDirection = getparameter_i(kParam_Direction);
	m_nScratchMode = getparameter_i(kParam_ScratchMode);
	m_nNoteMode = getparameter_i(kParam_NoteMode);

#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
	m_bMute = getparameter_b(kParam_Mute);
#endif
	bool playbackTrigger = getparameter_b(kParam_PlayTrigger);


	if (getparameterchanged(kParam_PitchShift))
		processPitch();

	if (getparameterchanged(kParam_ScratchAmount) || m_bPitchBendSet)	// XXX checking m_bPitchBendSet until I fix getparameterchanged()
	{
		m_bScratchAmountSet = true;
		if ( (m_fScratchAmount > k_fScratchAmountMiddlePoint_LowerLimit) && (m_fScratchAmount < k_fScratchAmountMiddlePoint_UpperLimit) )
			m_bScratching = false;
		else
		{
			if (!m_bScratching) // first time in so init stuff here
			{
				m_fPrevDesiredPosition = m_fPosition;
				m_fDesiredScratchRate2 = m_fPlaySampleRate;
				m_fScratchCenter = m_fPosition;
				m_nScratchCenter = (int)m_fPosition;
			}
			m_bScratching = true;
		}
		processScratch();
	}

#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
	m_fNoteVolume = m_fVolume * (float)m_nCurrentVelocity * MIDI_SCALAR;
#else
	m_fNoteVolume = (float)m_nCurrentVelocity * MIDI_SCALAR;
#endif

	if (getparameterchanged(kParam_PitchRange))
		processPitch();

	if (getparameterchanged(kParam_Direction))
		processDirection();

	calculateSpinSpeeds();

	if ( getparametertouched(kParam_PlayTrigger) )
		playNote(playbackTrigger);
}



#pragma mark -
#pragma mark plugin state

static const CFStringRef kTurntablistPreset_AudioFileReferenceKey = CFSTR("audiofile");
static const CFStringRef kTurntablistPreset_AudioFileAliasKey = CFSTR("DestroyFX-audiofile-alias");

//-----------------------------------------------------------------------------------------
bool FSRefIsValid(const FSRef & inFileRef)
{
	// this function only is available on Mac OS X 10.4 or higher
	if (FSIsFSRefValid != NULL)
		return FSIsFSRefValid(&inFileRef);
	else
		return ( FSGetCatalogInfo(&inFileRef, kFSCatInfoNone, NULL, NULL, NULL, NULL) == noErr );
} 

//-----------------------------------------------------------------------------------------
ComponentResult Turntablist::SaveState(CFPropertyListRef * outData)
{
	ComponentResult result = AUBase::SaveState(outData);
	if (result != noErr)
		return result;

	CFMutableDictionaryRef dict = (CFMutableDictionaryRef) (*outData);


// save the path of the loaded audio file, if one is currently loaded
	if ( FSRefIsValid(m_fsAudioFile) )
	{
		CFURLRef audioFileUrl = CFURLCreateFromFSRef(kCFAllocatorDefault, &m_fsAudioFile);
		if (audioFileUrl != NULL)
		{
			CFStringRef audioFileUrlString = CFURLCopyFileSystemPath(audioFileUrl, kCFURLPOSIXPathStyle);
			if (audioFileUrlString != NULL)
			{
				CFDictionaryRef fileReferencesDictionary = CFDictionaryCreate(kCFAllocatorDefault, (const void **)(&kTurntablistPreset_AudioFileReferenceKey), (const void **)(&audioFileUrlString), 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
				if (fileReferencesDictionary != NULL)
				{
					CFDictionarySetValue(dict, (const void *)CFSTR(kAUPresetExternalFileRefs), (const void *)fileReferencesDictionary);
					CFRelease(fileReferencesDictionary);
				}
				CFRelease(audioFileUrlString);
			}
			CFRelease(audioFileUrl);
		}

		// also save an alias of the loaded audio file, as a fall-back
		AliasHandle aliasHandle = NULL;
		Size aliasSize = 0;
		OSStatus aliasStatus = createAudioFileAlias(&aliasHandle, &aliasSize);
		if (aliasStatus == noErr)
		{
			if (GetAliasSize != NULL)
				aliasSize = GetAliasSize(aliasHandle);
			else
				aliasSize = GetHandleSize((Handle)aliasHandle);
			if (aliasSize > 0)
			{
				CFDataRef aliasCFData = CFDataCreate(kCFAllocatorDefault, (UInt8*)(*aliasHandle), (CFIndex)aliasSize);
				if (aliasCFData != NULL)
				{
					CFDictionarySetValue(dict, kTurntablistPreset_AudioFileAliasKey, aliasCFData);
					CFRelease(aliasCFData);
				}
			}
			DisposeHandle((Handle)aliasHandle);
		}
	}


// save the MIDI CC -> parameter assignments
	if (dfxsettings != NULL)
	{
		dfxsettings->saveMidiAssignmentsToDictionary(dict);
	}

	return noErr;
}

//-----------------------------------------------------------------------------------------
ComponentResult Turntablist::RestoreState(CFPropertyListRef inData)
{
	ComponentResult result = AUBase::RestoreState(inData);
	if (result != noErr)
		return result;

	CFDictionaryRef dict = static_cast<CFDictionaryRef>(inData);


// restore the previously loaded audio file
	bool foundAudioFilePathString = false;
	bool failedToResolveAudioFile = false;
	CFStringRef audioFileNameString = NULL;
	CFDictionaryRef fileReferencesDictionary = reinterpret_cast<CFDictionaryRef>( CFDictionaryGetValue(dict, CFSTR(kAUPresetExternalFileRefs)) );
	if (fileReferencesDictionary != NULL)
	{
		CFStringRef audioFileUrlString = reinterpret_cast<CFStringRef>( CFDictionaryGetValue(fileReferencesDictionary, kTurntablistPreset_AudioFileReferenceKey) );
		if (audioFileUrlString != NULL)
		{
			CFURLRef audioFileUrl = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, audioFileUrlString, kCFURLPOSIXPathStyle, false);
			if (audioFileUrl != NULL)
			{
				FSRef audioFileRef;
				Boolean gotFileRef = CFURLGetFSRef(audioFileUrl, &audioFileRef);
				if (gotFileRef)
				{
					loadAudioFile(audioFileRef);
					foundAudioFilePathString = true;
					failedToResolveAudioFile = false;
				}
				else
				{
					failedToResolveAudioFile = true;
					// we can't get the proper LaunchServices "display name" if the file does not exist, so do this instead
					audioFileNameString = CFURLCopyLastPathComponent(audioFileUrl);
				}
				CFRelease(audioFileUrl);
			}
		}
	}

	// if resolving the audio file from the stored file path failed, then try resolving from the stored file alias
	if (!foundAudioFilePathString)
	{
		CFDataRef aliasCFData = reinterpret_cast<CFDataRef>( CFDictionaryGetValue(dict, kTurntablistPreset_AudioFileAliasKey) );
		if (aliasCFData != NULL)
		{
			if ( CFGetTypeID(aliasCFData) == CFDataGetTypeID() )
			{
				CFIndex aliasDataSize = CFDataGetLength(aliasCFData);
				const UInt8 * aliasData = CFDataGetBytePtr(aliasCFData);
				if ( (aliasData != NULL) && (aliasDataSize > 0) )
				{
					AliasHandle aliasHandle = NULL;
					OSErr aliasError = PtrToHand(aliasData, (Handle*)(&aliasHandle), aliasDataSize);
					if ( (aliasError == noErr) && (aliasHandle != NULL) )
					{
						FSRef audioFileRef;
						Boolean wasChanged;
						aliasError = FSResolveAlias(NULL, aliasHandle, &audioFileRef, &wasChanged);
						if (aliasError == noErr)
						{
							loadAudioFile(audioFileRef);
							failedToResolveAudioFile = false;
						}
						else
						{
							failedToResolveAudioFile = true;
							// if we haven't gotten it already...
							if (audioFileNameString == NULL)
							{
								HFSUniStr255 fileNameUniString;
								OSStatus aliasStatus = FSCopyAliasInfo(aliasHandle, &fileNameUniString, NULL, NULL, NULL, NULL);
								if (aliasStatus == noErr)
									audioFileNameString = CFStringCreateWithCharacters(kCFAllocatorDefault, fileNameUniString.unicode, fileNameUniString.length);
							}
						}
						DisposeHandle((Handle)aliasHandle);
					}
				}
			}
		}
	}

	if (failedToResolveAudioFile)
		PostNotification_AudioFileNotFound(audioFileNameString);
	if (audioFileNameString != NULL)
		CFRelease(audioFileNameString);


// restore the MIDI CC -> parameter assignments
	if (dfxsettings != NULL)
	{
		dfxsettings->restoreMidiAssignmentsFromDictionary(dict);
	}

	return noErr;
}

//-----------------------------------------------------------------------------
OSStatus Turntablist::PostNotification_AudioFileNotFound(CFStringRef inFileName)
{
	if (inFileName == NULL)
		inFileName = CFSTR("");

	CFBundleRef pluginBundleRef = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));

	CFStringRef titleString_base = CFCopyLocalizedStringFromTableInBundle(CFSTR("%@:  Audio file not found."), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("the title of the notification dialog when an audio file referenced in stored session data cannot be found"));
	CFStringRef identifyingNameString = (mContextName != NULL) ? mContextName : CFSTR(PLUGIN_NAME_STRING);
	CFStringRef titleString = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, titleString_base, identifyingNameString);
	CFRelease(titleString_base);

	CFStringRef messageString_base = CFCopyLocalizedStringFromTableInBundle(CFSTR("The previously used file \"%@\" could not be found."), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("the detailed message of the notification dialog when an audio file referenced in stored session data cannot be found"));
	CFStringRef messageString = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, messageString_base, inFileName);
	CFRelease(messageString_base);

	OSStatus status = CFUserNotificationDisplayNotice(0.0, kCFUserNotificationPlainAlertLevel, NULL, NULL, NULL, titleString, messageString, NULL);
	if (titleString != NULL)
		CFRelease(titleString);
	if (messageString != NULL)
		CFRelease(messageString);

	return status;
}

//-----------------------------------------------------------------------------
ComponentResult Turntablist::GetPropertyInfo(AudioUnitPropertyID inPropertyID, 
					AudioUnitScope inScope, AudioUnitElement inElement, 
					UInt32 & outDataSize, Boolean & outWritable)
{
	ComponentResult result = noErr;

	switch (inPropertyID)
	{
		case kTurntablistProperty_Play:
			outDataSize = sizeof(Boolean);
			outWritable = true;
			break;

		case kTurntablistProperty_AudioFile:
			if ( FSRefIsValid(m_fsAudioFile) )
			{
				AliasHandle alias = NULL;
				Size aliasSize = 0;
				result = createAudioFileAlias(&alias, &aliasSize);
				if (result == noErr)
				{
					outDataSize = aliasSize;
					outWritable = true;
					DisposeHandle((Handle)alias);
				}
			}
			else
			{
				result = errFSBadFSRef;
			}
			break;

		default:
			result = DfxPlugin::GetPropertyInfo(inPropertyID, inScope, inElement, outDataSize, outWritable);
			break;
	}

	return result;
}

//-----------------------------------------------------------------------------
ComponentResult Turntablist::GetProperty(AudioUnitPropertyID inPropertyID, 
					AudioUnitScope inScope, AudioUnitElement inElement, 
					void * outData)
{
	ComponentResult result = noErr;

	switch (inPropertyID)
	{
		case kTurntablistProperty_Play:
			*(Boolean*)outData = m_bPlay;
			break;

		case kTurntablistProperty_AudioFile:
			if ( FSRefIsValid(m_fsAudioFile) )
			{
				AliasHandle alias = NULL;
				Size aliasSize = 0;
				result = createAudioFileAlias(&alias, &aliasSize);
				if (result == noErr)
				{
					memcpy(outData, *alias, aliasSize);
					DisposeHandle((Handle)alias);
				}
			}
			else
				result = errFSBadFSRef;
			break;

		default:
			result = DfxPlugin::GetProperty(inPropertyID, inScope, inElement, outData);
			break;
	}

	return result;
}

//-----------------------------------------------------------------------------
ComponentResult Turntablist::SetProperty(AudioUnitPropertyID inPropertyID, 
					AudioUnitScope inScope, AudioUnitElement inElement, 
					const void * inData, UInt32 inDataSize)
{
	ComponentResult result = noErr;

	switch (inPropertyID)
	{
		case kTurntablistProperty_Play:
			m_bPlay = *(Boolean*)inData;
			playNote(m_bPlay);
			break;

		case kTurntablistProperty_AudioFile:
			{
				AliasHandle alias = NULL;
				result = PtrToHand(inData, (Handle*)(&alias), inDataSize);
				if ( (result == noErr) && (alias != NULL) )
				{
					result = resolveAudioFileAlias(alias);
					DisposeHandle((Handle)alias);
				}
				else
					result = memFullErr;
			}
			break;

		default:
			result = DfxPlugin::SetProperty(inPropertyID, inScope, inElement, inData, inDataSize);
			break;
	}

	return result;
}

//-----------------------------------------------------------------------------------------
ComponentResult Turntablist::GetParameterInfo(AudioUnitScope inScope, 
							AudioUnitParameterID inParameterID, 
							AudioUnitParameterInfo & outParameterInfo)
{
	ComponentResult result = DfxPlugin::GetParameterInfo(inScope, inParameterID, outParameterInfo);
	if (result != noErr)
		return result;

	outParameterInfo.flags |= kAudioUnitParameterFlag_HasClump;
	switch (inParameterID)
	{
		case kParam_ScratchAmount:
		case kParam_ScratchMode:
		case kParam_ScratchSpeed_scrub:
		case kParam_ScratchSpeed_spin:
			outParameterInfo.clumpID = kParamGroup_Scratching;
			break;

		case kParam_Power:
		case kParam_SpinUpSpeed:
		case kParam_SpinDownSpeed:
		case kParam_NotePowerTrack:
			outParameterInfo.clumpID = kParamGroup_Power;
			break;

		case kParam_PitchShift:
		case kParam_PitchRange:
		case kParam_KeyTracking:
		case kParam_RootKey:
			outParameterInfo.clumpID = kParamGroup_Pitch;
			break;

		case kParam_Loop:
		case kParam_Direction:
		case kParam_NoteMode:
		case kParam_PlayTrigger:
			outParameterInfo.clumpID = kParamGroup_Playback;
			break;

#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
		case kParam_Mute:
		case kParam_Volume:
			outParameterInfo.clumpID = kParamGroup_Output;
			break;
#endif

		default:
			outParameterInfo.flags &= ~kAudioUnitParameterFlag_HasClump;
			break;
	}

	return noErr;
}

//-----------------------------------------------------------------------------------------
CFStringRef Turntablist::CopyParameterGroupName(UInt32 inParameterGroupID)
{
	CFStringRef clumpName = NULL;
	switch (inParameterGroupID)
	{
		case kParamGroup_Scratching:
			clumpName = CFSTR("scratching");
			break;
		case kParamGroup_Playback:
			clumpName = CFSTR("audio sample playback");
			break;
		case kParamGroup_Power:
			clumpName = CFSTR("turntable power");
			break;
		case kParamGroup_Pitch:
			clumpName = CFSTR("pitch");
			break;
#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
		case kParamGroup_Output:
			clumpName = CFSTR("audio output");
			break;
#endif
		default:
			break;
	}

	if (clumpName != NULL)
		CFRetain(clumpName);
	return clumpName;
}



//-----------------------------------------------------------------------------------------
void Turntablist::setPlay(bool inPlayState, bool inShouldSendNotification)
{
	bool play_old = m_bPlay;
	m_bPlay = inPlayState;
	if (m_bPlay != play_old)
		PropertyChanged(kTurntablistProperty_Play, kAudioUnitScope_Global, 0);
}

//-----------------------------------------------------------------------------
OSStatus Turntablist::createAudioFileAlias(AliasHandle * outAlias, Size * outDataSize)
{
	if (outAlias == NULL)
		return paramErr;

	OSErr error = FSNewAlias(NULL, &m_fsAudioFile, outAlias);
	if (error != noErr)
		return error;
	if (*outAlias == NULL)
		return nilHandleErr;

	if (outDataSize != NULL)
	{
		if (GetAliasSize != NULL)
			*outDataSize = GetAliasSize(*outAlias);
		else
			*outDataSize = GetHandleSize((Handle)(*outAlias));
	}

	return error;
}

//-----------------------------------------------------------------------------
OSStatus Turntablist::resolveAudioFileAlias(const AliasHandle inAlias)
{
	FSRef audioFileRef;
	Boolean wasChanged;
	OSStatus status = FSResolveAlias(NULL, inAlias, &audioFileRef, &wasChanged);
	if (status == noErr)
	{
		status = loadAudioFile(audioFileRef);
	}

	return status;
}



#pragma mark -
#pragma mark audio processing

//-----------------------------------------------------------------------------------------
OSStatus Turntablist::loadAudioFile(const FSRef & inFileRef)
{
// ExtAudioFile
#ifndef USE_LIBSNDFILE
	OSStatus status;
	UInt32 dataSize;

	if (ExtAudioFileOpen == NULL)
		return unsupportedOSErr;
	ExtAudioFileRef audioFileRef = NULL;
	status = ExtAudioFileOpen(&inFileRef, &audioFileRef);
	if (status != noErr)
		return status;

	SInt64 audioFileNumFrames = 0;
	dataSize = sizeof(audioFileNumFrames);
	status = ExtAudioFileGetProperty(audioFileRef, kExtAudioFileProperty_FileLengthFrames, &dataSize, &audioFileNumFrames);
	if (status != noErr)
		return status;

	AudioStreamBasicDescription audioFileStreamFormat;
	dataSize = sizeof(audioFileStreamFormat);
	status = ExtAudioFileGetProperty(audioFileRef, kExtAudioFileProperty_FileDataFormat, &dataSize, &audioFileStreamFormat);
	if (status != noErr)
		return status;

	CAStreamBasicDescription clientStreamFormat;
	clientStreamFormat.SetCanonical(audioFileStreamFormat.mChannelsPerFrame, false);
	clientStreamFormat.mSampleRate = audioFileStreamFormat.mSampleRate;
	status = ExtAudioFileSetProperty(audioFileRef, kExtAudioFileProperty_ClientDataFormat, sizeof(clientStreamFormat), &clientStreamFormat);
	if (status != noErr)
		return status;

	m_AudioFileLock.grab();

	m_bAudioFileHasBeenLoaded = false;	// XXX cuz we're about to possibly re-allocate the audio buffer and invalidate what might already be there

	m_auBufferList.Allocate(clientStreamFormat, (UInt32)audioFileNumFrames);
	AudioBufferList & abl = m_auBufferList.PrepareBuffer(clientStreamFormat, (UInt32)audioFileNumFrames);
	UInt32 audioFileNumFrames_temp = (UInt32)audioFileNumFrames;
	status = ExtAudioFileRead(audioFileRef, &audioFileNumFrames_temp, &abl);
	if (status != noErr)
	{
		m_auBufferList.Deallocate();
		return status;
	}
	if (audioFileNumFrames_temp != (UInt32)audioFileNumFrames)	// XXX do something?
	{
		// XXX error?
		fprintf(stderr, PLUGIN_NAME_STRING":  audio data size mismatch!\nsize requested: %lu, size read: %lu\n\n", (UInt32)audioFileNumFrames, audioFileNumFrames_temp);
	}

	status = ExtAudioFileDispose(audioFileRef);

	m_nNumChannels = clientStreamFormat.mChannelsPerFrame;
	m_nSampleRate = clientStreamFormat.mSampleRate;
	m_nNumSamples = (int) audioFileNumFrames;

	m_fPlaySampleRate = (double) m_nSampleRate;
	m_fSampleRate = (double) m_nSampleRate;
	calculateSpinSpeeds();
	m_fPosition = 0.0;
	m_fPosOffset = 0.0;
	m_fNumSamples = (double) m_nNumSamples;



// libsndfile
#else
	UInt8 file[2048];
	memset(file, 0, sizeof(file));
	OSStatus status = FSRefMakePath(&inFileRef, file, sizeof(file));
	if (status != noErr)
		return status;
//fprintf(stderr, PLUGIN_NAME_STRING" audio file:  %s\n", file);

	SF_INFO sfInfo;
	memset(&sfInfo, 0, sizeof(SF_INFO));
	SNDFILE * sndFile = NULL;

	sndFile = sf_open((const char*)file, SFM_READ, &sfInfo);

	if (sndFile == NULL)
	{
		// print error
		char buffer[256];
		memset(buffer, 0, sizeof(buffer));
		sf_error_str(sndFile, buffer, sizeof(buffer) - 1);
		fprintf(stderr, "\n"PLUGIN_NAME_STRING" could not open the audio file:  %s\nlibsndfile error message:  %s\n", file, buffer);
		return sf_error(sndFile);
	}

#if 0
	fprintf(stderr, "**** SF_INFO dump for:  %s\n", file);
	fprintf(stderr, "     samplerate:  %d\n", sfInfo.samplerate);
	fprintf(stderr, "     frames:  %d (0x%08x)\n", sfInfo.frames, sfInfo.frames);
	fprintf(stderr, "     channels:  %d\n", sfInfo.channels);
	fprintf(stderr, "     format:  %d\n", sfInfo.format);
	fprintf(stderr, "     sections:  %d\n", sfInfo.sections);
	fprintf(stderr, "     seekable:  %d\n", sfInfo.seekable);
#endif

	sf_command(sndFile, SFC_SET_NORM_FLOAT, NULL, SF_TRUE);

	m_AudioFileLock.grab();

	m_nNumChannels = sfInfo.channels;
	m_nSampleRate = sfInfo.samplerate;
	m_nNumSamples = (int) sfInfo.frames;

	m_fPlaySampleRate = (double) m_nSampleRate;
	m_fSampleRate = (double) m_nSampleRate;
	calculateSpinSpeeds();
	m_fPosition = 0.0;
	m_fPosOffset = 0.0;
	m_fNumSamples = (double) m_nNumSamples;

	if (m_fBuffer != NULL)
		free(m_fBuffer);
	m_fBuffer = NULL;
	m_fLeft = NULL;
	m_fRight = NULL;

	m_fBuffer = (float*) malloc(m_nNumChannels * m_nNumSamples * sizeof(float));

	m_fLeft = m_fBuffer;
	if (m_nNumChannels == 1)
		m_fRight = m_fBuffer;
	else
		m_fRight = &(m_fBuffer[m_nNumSamples]);

	// do file loading here!!
	sf_count_t sizein = sfInfo.frames * sfInfo.channels;
	sf_count_t sizeout = sf_read_float(sndFile, m_fBuffer, sizein);
	if (sizeout != sizein)
	{
		// XXX error?
		fprintf(stderr, PLUGIN_NAME_STRING":  audio data size mismatch!\nsize-in: %ld, size-out: %ld\n\n", (long)sizein, (long)sizeout);
	}

	if (m_nNumChannels >= 2)
	{
		float * tempBuffer = (float*) malloc(m_nNumSamples * sizeof(float));
		if (tempBuffer != NULL)
		{
			// do swaps
			for (int z=0; z < m_nNumSamples; z++)
			{
				tempBuffer[z] = m_fBuffer[(z*m_nNumChannels)+1];	// copy channel 2 into buffer
				m_fBuffer[z] = m_fBuffer[z*m_nNumChannels];	// handle channel 1
			}

			for (int z=0; z < m_nNumSamples; z++)
				m_fBuffer[m_nNumSamples+z] = tempBuffer[z];	// make channel 2

			free(tempBuffer);
		}
	}

	// end file loading here!!

	if (sf_close(sndFile) != 0)
	{
		// error closing
	}

	sndFile = NULL;
#endif


	processPitch();	// set up stuff

	// ready to play
	m_bAudioFileHasBeenLoaded = true;
	m_AudioFileLock.release();

	m_fsAudioFile = inFileRef;
	PropertyChanged(kTurntablistProperty_AudioFile, kAudioUnitScope_Global, 0);

	return noErr;
}

//-----------------------------------------------------------------------------------------
void Turntablist::calculateSpinSpeeds()
{
//	m_fUsedSpinUpSpeed = (((exp(10.0*m_fSpinUpSpeed)-1.0)/(exp(10.0)-1.0)) * m_fSampleRate) / (double)m_nPowerIntervalEnd;
//	m_fUsedSpinDownSpeed = (((exp(10.0*m_fSpinDownSpeed)-1.0)/(exp(10.0)-1.0)) * m_fSampleRate) / (double)m_nPowerIntervalEnd;
	m_fUsedSpinUpSpeed = m_fSpinUpSpeed * m_fSampleRate / (double)m_nPowerIntervalEnd;
	m_fUsedSpinDownSpeed = m_fSpinDownSpeed * m_fSampleRate / (double)m_nPowerIntervalEnd;
}

//-----------------------------------------------------------------------------------------
void Turntablist::processaudio(const float ** inStreams, float ** outStreams, unsigned long inNumFrames, bool replacing)
{
	long eventFrame = -1; // -1 = no events
	long numEvents = midistuff->numBlockEvents;
	long currEvent = 0;

	unsigned long numOutputs = getnumoutputs();
//	float (*interpolateHermiteFunctionPtr)(float *, double, long) = m_bLoop ? DFX_InterpolateHermite : DFX_InterpolateHermite_NoWrap;


	if (numEvents == 0)
		eventFrame = -1;
	else
		eventFrame = (midistuff->blockEvents[currEvent]).delta;
	for (long currFrame=0; currFrame < (signed)inNumFrames; currFrame++)
	{
		//
		// process MIDI events if any
		//
		while (currFrame == eventFrame)
		{
			processMidiEvent(currEvent);
			currEvent++;	// check next event

			if (currEvent >= numEvents)
				eventFrame = -1;	// no more events
		}
		
		if (m_bScratching)  // handle scratching
		{
			m_nScratchInterval++;
			if (m_nScratchInterval > m_nScratchIntervalEnd)
			{
				if (m_nScratchMode == kScratchMode_Spin)
					processScratch();
				else
					processScratchStop();
			}
			else
			{
				if (m_nScratchMode == kScratchMode_Scrub)
				{
					// nudge samplerate
					m_fPlaySampleRate += m_fTinyScratchAdjust;
					// speed mode
					if (m_fTinyScratchAdjust > 0.0)	// positive
					{
						if (m_fPlaySampleRate > m_fDesiredScratchRate2)
							m_fPlaySampleRate = m_fDesiredScratchRate2;
					}
					else	// negative
					{
						if (m_fPlaySampleRate < m_fDesiredScratchRate2)
							m_fPlaySampleRate = m_fDesiredScratchRate2;
					}					
				}				
			}
		}
		else // not scratching so just handle power
		{
			if (!m_bPower)	// power off - spin down
			{
				if (m_fPlaySampleRate > 0.0)	// too high, spin down
				{
					m_fPlaySampleRate -= m_fUsedSpinDownSpeed;	// adjust
					if (m_fPlaySampleRate < 0.0)	// too low so we past it
						m_fPlaySampleRate = 0.0;	// fix it
				}
			}
			else // power on - spin up
			{
				if (m_fPlaySampleRate < m_fDesiredPitch) // too low so bring up
				{
					m_fPlaySampleRate += m_fUsedSpinUpSpeed; // adjust
					if (m_fPlaySampleRate > m_fDesiredPitch) // too high so we past it
						m_fPlaySampleRate = m_fDesiredPitch; // fix it
				}
				else if (m_fPlaySampleRate > m_fDesiredPitch) // too high so bring down
				{
					m_fPlaySampleRate -= m_fUsedSpinUpSpeed; // adjust
					if (m_fPlaySampleRate < m_fDesiredPitch) // too low so we past it
						m_fPlaySampleRate = m_fDesiredPitch; // fix it
				}
			}
		}

		// handle rest of processing here
		if (m_bNoteIsOn)
		{
			// adjust gnPosition for new sample rate

			// set pos offset based on current sample rate

			// safety check
			if (m_fPlaySampleRate <= 0.0)
			{
				m_fPlaySampleRate = 0.0;
				if (m_bNotePowerTrack)
				{
					if (!m_bScratching)
					{
						stopNote(true);
						if (m_nNoteMode == kNoteMode_Reset)
							m_fPosition = 0.0;
					}
				}
			}

			m_fPosOffset = m_fPlaySampleRate / getsamplerate();   //m_fPlaySampleRate/m_fSampleRate;

			int lockResult = EAGAIN;
			if (m_bAudioFileHasBeenLoaded)
				lockResult = m_AudioFileLock.try_grab();
			if (lockResult == 0)
			{
				if (m_bNoteIsOn)
				{

					if (!m_bPlayForward) // if play direction = reverse
					{
						if (m_fPlaySampleRate != 0.0)
						{
							m_fPosition -= m_fPosOffset;
							while (m_fPosition < 0.0)	// was if
							{
								if (!m_bLoop)	// off
								{
									if (m_bPlayedReverse)
									{
										stopNote(true);
										m_fPosition = 0.0;
									}
									else
									{
										m_fPosition += m_fNumSamples; // - 1;
										m_bPlayedReverse = true;
									}									
								}
								else
									m_fPosition += m_fNumSamples; // - 1;
							}
						}
					}   // if (!bPlayForward)

#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
					if (!m_bMute)   // if audio on
					{		
#endif
						if (m_fPlaySampleRate == 0.0)
						{
							for (unsigned long ch=0; ch < numOutputs; ch++)
								outStreams[ch][currFrame] = 0.0f;
						}
						else
						{
//#define NO_INTERPOLATION
//#define LINEAR_INTERPOLATION
#define CUBIC_INTERPOLATION

							for (unsigned long ch=0; ch < numOutputs; ch++)
							{
							#ifdef USE_LIBSNDFILE
								float * output = (ch == 0) ? m_fLeft : m_fRight;
							#else
								AudioBufferList & abl = m_auBufferList.GetBufferList();
								unsigned long ablChannel = ch;
								if (ch >= abl.mNumberBuffers)
								{
									ablChannel = abl.mNumberBuffers - 1;
									// XXX only do the channel remapping for mono->stereo upmixing special case (?)
									if (ch > 1)
										break;
								}
								float * output = (float*) (abl.mBuffers[ablChannel].mData);
							#endif

#ifdef NO_INTERPOLATION
								float outval = output[(long)m_fPosition];
#endif NO_INTERPOLATION

#ifdef LINEAR_INTERPOLATION						
								float floating_part = m_fPosition - (double)((long)m_fPosition);
								long big_part1 = (long)m_fPosition;
								long big_part2 = big_part1 + 1;
								if (big_part2 > m_nNumSamples)
									big_part2 = 0;
								float outval = (floating_part * output[big_part1]) + ((1.0f-floating_part) * output[big_part2]);
#endif LINEAR_INTERPOLATION

#ifdef CUBIC_INTERPOLATION
								float outval;
							#if 0
								// XXX is this a silly optimization to avoid another branch?
								// XXX can this even work for an inline function?
								outval = interpolateHermiteFunctionPtr(output, m_fPosition, m_nNumSamples);
							#else
								if (m_bLoop)
									outval = DFX_InterpolateHermite(output, m_fPosition, m_nNumSamples);
								else
									outval = DFX_InterpolateHermite_NoWrap(output, m_fPosition, m_nNumSamples);
							#endif
#endif CUBIC_INTERPOLATION

								outStreams[ch][currFrame] = outval * m_fNoteVolume;
							}
						}
#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
					}   // if (!m_bMute)
					else
					{
						for (unsigned long ch=0; ch < numOutputs; ch++)
							outStreams[ch][currFrame] = 0.0f;
					}
#endif

					
					if (m_bPlayForward)	// if play direction = forward
					{
						m_bPlayedReverse = false;
						if (m_fPlaySampleRate != 0.0)
						{
							m_fPosition += m_fPosOffset;

							while (m_fPosition >= m_fNumSamples)
							{
								m_fPosition -= m_fNumSamples;
								if (!m_bLoop) // off
									stopNote(true);
							}
						}
					}   // if (bPlayForward)

				}   // if (bNoteIsOn)
				else
				{
					for (unsigned long ch=0; ch < numOutputs; ch++)
						outStreams[ch][currFrame] = 0.0f;
				}

				m_AudioFileLock.release();
			}   // if (lockResult == 0)
			else
			{
				for (unsigned long ch=0; ch < numOutputs; ch++)
					outStreams[ch][currFrame] = 0.0f;
			}
		}   // if (bNoteIsOn)
		else
		{
			for (unsigned long ch=0; ch < numOutputs; ch++)
				outStreams[ch][currFrame] = 0.0f;
		}
	}   // (currFrame < inNumFrames)
}



#pragma mark -
#pragma mark scratch processing

//-----------------------------------------------------------------------------------------
void Turntablist::processScratchStop()
{
	m_nScratchDir = kScratchDirection_Forward;
	m_fPlaySampleRate = 0.0;
	if (!m_bScratchStop)
	{
		m_bScratchStop = true;
//		m_nScratchInterval = m_nScratchIntervalEnd / 2;
	}
	else
	{
		// TO DO:
		// 1. set the position to where it should be to adjust for errors

		m_fPlaySampleRate = 0.0;
		m_fDesiredScratchRate = 0.0;
		m_fDesiredScratchRate2 = 0.0;
		m_fTinyScratchAdjust = 0.0;
		m_nScratchInterval = 0;
	}

	processDirection();
}

//-----------------------------------------------------------------------------------------
void Turntablist::processScratch(bool inSetParameter)
{
	double fIntervalScaler = 0.0;
	
	if (m_bPitchBendSet)
	{
/*
		// set scratch amount to scaled pitchbend
		if (inSetParameter)
		{
			setparameter_f(kParam_ScratchAmount, m_fPitchBend);
			// XXX post notification?  not that this ever gets called with inSetParameter true...
			m_fScratchAmount = m_fPitchBend;
		}
		else
			m_fScratchAmount = m_fPitchBend;
*/
		m_bScratchAmountSet = true;

		m_bPitchBendSet = false;
	}


	if (m_bScratching)  // scratching
	{
		if (m_nScratchMode == kScratchMode_Spin)
		{
			if (m_fScratchAmount >= k_fScratchAmountMiddlePoint)
				m_nScratchDir = kScratchDirection_Forward;
			else
				m_nScratchDir = kScratchDirection_Backward;
		}

	// todo:
	// handle scratching like power
	// scratching will set target sample rate and system will catch up based on scratch speed parameter
	// sort of like spin up/down speed param..

	// if curr rate below target rate then add samplespeed2
	// it gone over then set to target rate
	// if curr rate above target rate then sub samplespeed2
	// if gone below then set to target rate


		// m_fScratchSpeed_spin is the hand size
		// set target sample rate
		m_fDesiredScratchRate = fabs(m_fScratchAmount * m_fScratchSpeed_spin * m_fBasePitch);
		
		if (m_nScratchMode == kScratchMode_Spin)	// mode 2
		{
			m_fPlaySampleRate = m_fDesiredScratchRate;
			m_bScratchStop = false;
			m_nScratchInterval = 0;
		}
		else	// mode 1
		{
		//	int oldtime = timeGetTime();  //time in nanoseconds

			//	NEWEST MODE 2
			// TO DO:
			// make position based
			// use torque to set the spin down time
			// and make the torque speed be samplerate/spindown time

			if (m_nScratchSubMode == 0)
			{
// POSITION BASED:
				if (m_bScratchAmountSet)
				{
					m_bScratchStop = false;

					fIntervalScaler = ((double)m_nScratchInterval / (double)m_nScratchIntervalEnd) + 1.0;

					m_fDesiredPosition = m_fScratchCenter + (contractparametervalue_index(kParam_ScratchAmount, m_fScratchAmount) * m_fScratchSpeed_scrub * m_fSampleRate);

					double fDesiredDelta;
					
					if (m_nScratchInterval == 0)
					{
						m_fPosition = m_fDesiredPosition;
						m_fTinyScratchAdjust = 0.0;
					}
					else
					{
						fDesiredDelta = (m_fDesiredPosition - m_fPosition); //m_nScratchInterval;
						m_fTinyScratchAdjust = (fDesiredDelta - m_fPlaySampleRate) / m_fSampleRate;
					}








					// do something with desireddelta and scratchinterval

					// figure out direction
					double fDiff = contractparametervalue_index(kParam_ScratchAmount, m_fScratchAmount) - contractparametervalue_index(kParam_ScratchAmount, m_fLastScratchAmount);

					if (fDiff < 0.0)
					{
						fDiff = -fDiff;
						m_nScratchDir = kScratchDirection_Backward;
					}
					else
						m_nScratchDir = kScratchDirection_Forward;

					m_fDesiredScratchRate2 = m_fSampleRate * m_fScratchSpeed_scrub * (double)m_nScratchInterval;

					// figure out destination position and current position
					// figure out distance between the two
					// samples to cover = fDiff * m_nNumSamples
					// need algorithm to get the acceleration needed to cover the distance given current speed
					// calculate the sample rate needed to cover that distance in that amount of time (interval end)
					// figure out desiredscratchrate2
					// m_nScratchIntervalEnd/m_nSampleRate = m_nPosDiff/x
					//m_fDesiredScratchRate2 = (m_nSampleRate/m_nScratchIntervalEnd)*m_nPosDiff;
					//if fDiff == 1.0 then it means ur moving thru the entire sample in one quick move
					//
					//
					// desired rate = distance / time
					// distance is samples to cover, time is interval
					//accel = (desired rate - curr rate)/ time
					// accel is tiny scratch adjust

					// TO DO:
					// 1. figure out how to handle wrapping around the sample

					//new
					// add fScaler = (m_fScratchSpeed*7.0f)
					


					m_fDesiredScratchRate2 = (m_fDesiredScratchRate2 + m_fPlaySampleRate) * 0.5;

					m_bScratchAmountSet = false;

					m_fPrevDesiredPosition = m_fDesiredPosition;
				}
			}
			else
			{
// SPEED BASED:
				if (m_bScratchAmountSet)
				{
					m_bScratchStop = false;

					double fDiff = contractparametervalue_index(kParam_ScratchAmount, m_fScratchAmount) - contractparametervalue_index(kParam_ScratchAmount, m_fLastScratchAmount);

					if (fDiff < 0.0)
					{
						fDiff = -fDiff;
						m_nScratchDir = kScratchDirection_Backward;
					}
					else
					{
						m_nScratchDir = kScratchDirection_Forward;
					}


					// new
					fIntervalScaler = ((double)m_nScratchInterval / (double)m_nScratchIntervalEnd) + 1.0;

					m_fDesiredScratchRate2 = (fDiff * m_fSampleRate * m_fScratchSpeed_scrub*(double)k_nScratchInterval) / fIntervalScaler;

					m_fTinyScratchAdjust = (m_fDesiredScratchRate2 - m_fPlaySampleRate)/(double)m_nScratchIntervalEnd; 


					m_fPlaySampleRate += m_fTinyScratchAdjust;
					m_bScratchAmountSet = false;
				}
			}

			m_fLastScratchAmount = m_fScratchAmount;
		}
		m_fScratchVolumeModifier = m_fPlaySampleRate / m_fSampleRate;
		if (m_fScratchVolumeModifier > 1.5f)
			m_fScratchVolumeModifier = 1.5f;

		if (!m_bScratching)
			m_bScratching = true;

		if (m_bScratchStop)
		{
			m_fPlaySampleRate = 0.0;
			m_fDesiredScratchRate = 0.0;
			m_fDesiredScratchRate2 = 0.0;
		}

		m_nScratchInterval = 0;


		processDirection();
	}
	else
	{
		m_nWasScratchingDir = m_nScratchDir;
		m_bScratching = false;
		m_bWasScratching = true;
		processDirection();
	}
}

//-----------------------------------------------------------------------------------------
void Turntablist::processPitch()
{
	double temp = m_fDesiredPitch;
	if (m_bKeyTracking)
	{
		int note2play = m_nCurrentNote - m_nRootKey;
		m_fBasePitch = m_fSampleRate * pow(2.0, (double)note2play/12.0);
	}
	else
	{
		m_fBasePitch = m_fSampleRate;
	}
	double fPitchAdjust = m_fPitchShift * m_fPitchRange;	// shift in semitones
	fPitchAdjust = pow(2.0, fPitchAdjust/12.0); // shift as a scalar of the playback rate
	m_fDesiredPitch = m_fBasePitch * fPitchAdjust;

	if (temp == m_fPlaySampleRate)
		m_fPlaySampleRate = m_fDesiredPitch;	// do instant adjustment
}

//-----------------------------------------------------------------------------------------
void Turntablist::processDirection()
{
	m_bPlayForward = true;

	if (m_bScratching)
	{
		if (m_nScratchDir == kScratchDirection_Backward)
			m_bPlayForward = false;
	}
	else
	{
		if (m_bWasScratching)
		{
			if (m_nDirection == kScratchDirection_Backward)
				m_bPlayForward = false;
		}
		else
		{
			if (m_nDirection == kScratchDirection_Backward)
				m_bPlayForward = false;
		}
	}
}



#pragma mark -
#pragma mark MIDI processing

//-----------------------------------------------------------------------------------------
void Turntablist::processMidiEvent(long inCurrentEvent)
{
	DfxMidiEvent event = midistuff->blockEvents[inCurrentEvent];

	if ( isNote(event.status) )
	{
		long note = event.byte1;
		long velocity = event.byte2;
		if (event.status == kMidiNoteOff)
			velocity = 0;

		noteOn(note, velocity, event.delta); // handle note on or off
	}
	else if (event.status == kMidiCC) // controller change
	{
		if (event.byte1 == kMidiCC_AllNotesOff)	// all notes off
		{
			stopNote(true);
			if (m_nNoteMode == kNoteMode_Reset)
				m_fPosition = 0.0;
		}

#ifdef USE_MIDI_CC
		if ( (event.byte1 >= 64) && (event.byte1 <= (64 + kNumParameters - 1)) )
		{
			long param = event.byte1 - 64;
			long new_data = fixMidiData(param, event.byte2);
			float value = (float)new_data * MIDI_SCALAR;
			setparameter_f(param, value);
			postupdate_parameter(param);
		}
#endif
	}
#ifdef USE_MIDI_PITCH_BEND
	else if (event.status == kMidiPitchbend) // pitch bend
	{
		// handle pitch bend here
		int pitchBendValue = (event.byte2 << 7) | event.byte1;
		m_bPitchBendSet = true;

		if (pitchBendValue == kDfxMidi_PitchbendMiddleValue)
			m_fPitchBend = k_fScratchAmountMiddlePoint;
		else
		{
			m_fPitchBend = (double)pitchBendValue / (double)((kDfxMidi_PitchbendMiddleValue * 2) - 1);
			if (m_fPitchBend > 1.0)
				m_fPitchBend = 1.0;
			else if (m_fPitchBend < 0.0)
				m_fPitchBend = 0.0;
			m_fPitchBend = expandparametervalue_index(kParam_ScratchAmount, m_fPitchBend);
		}

		setparameter_f(kParam_ScratchAmount, m_fPitchBend);
//		postupdate_parameter(kParam_ScratchAmount);	// XXX post notification? for some reason it makes the results sound completely different
		m_fScratchAmount = m_fPitchBend;
	}
#endif
}

//-----------------------------------------------------------------------------------------
void Turntablist::noteOn(long inNote, long inVelocity, long inDelta)
{
	bool power_old = m_bPower;

	m_nCurrentNote = inNote;
	m_nCurrentVelocity = inVelocity;
	if (inVelocity == 0)
	{
		if (m_bNotePowerTrack)
		{
			setparameter_b(kParam_Power, false);
			m_bPower = false;
			m_bNoteIsOn = true;
			setPlay(false);
		}
		else
		{
			m_bNoteIsOn = false;
			setPlay(false);
			if (m_nNoteMode == kNoteMode_Reset)
				m_fPosition = 0.0;
			m_bPlayedReverse = false;
		}
	}
	else
	{
		m_bNoteIsOn = true;
		setPlay(true);

		if (m_nNoteMode == kNoteMode_Reset)
			m_fPosition = 0.0;
		// calculate note volume
#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
		m_fNoteVolume = m_fVolume * (float)m_nCurrentVelocity * MIDI_SCALAR;
#else
		m_fNoteVolume = (float)m_nCurrentVelocity * MIDI_SCALAR;
#endif
		//44100*(2^(0/12)) = C-3
		processPitch();

		if (m_bNotePowerTrack)
		{
			setparameter_b(kParam_Power, true);
			m_bPower = true;
		}
	}

	// XXX post notification yes?
	if (m_bPower != power_old)
		postupdate_parameter(kParam_Power);
}

//-----------------------------------------------------------------------------------------
void Turntablist::stopNote(bool inStopPlay)
{
	m_bNoteIsOn = false;
	m_bPlayedReverse = false;

	if (inStopPlay)
		setPlay(false);
}

//-----------------------------------------------------------------------------------------
void Turntablist::playNote(bool inValue)
{
	if (inValue)
	{
		// m_nCurrentNote - m_nRootKey = ?
		// - 64       - 0          = -64
		// 64          - 64         = 0;
		// 190           - 127        = 63;
		m_nCurrentNote = m_nRootKey;
		noteOn(m_nCurrentNote, 0x7F, 0);	// note on
	}
	else
		noteOn(m_nCurrentNote, 0, 0);	// note off
}

//-----------------------------------------------------------------------------------------
long Turntablist::fixMidiData(long inParameterID, char inValue)
{
	switch (inParameterID)
	{
		case kParam_Power:
		case kParam_NotePowerTrack:
#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
		case kParam_Mute:
#endif
		case kParam_PlayTrigger:
		case kParam_NoteMode:
		case kParam_Direction:
		case kParam_ScratchMode:
		case kParam_Loop:
		case kParam_KeyTracking:
			// <64 = 0ff, >=64 = 0n
			if (inValue < 64)
				return 0;
			else
				return 0x7F;
			break;
		default:
			break;
	}

	return inValue;
}
