/*------------------------------------------------------------------------
Copyright (c) 2004 bioroid media development & Copyright (C) 2004-2023 Sophia Poirier
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer. 
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution. 
* Neither the name of "bioroid media development" nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission. 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Additional changes since June 15th 2004 have been made by Sophia Poirier.  

This file is part of Turntablist.

Turntablist is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 2 of the License, or 
(at your option) any later version.

You should have received a copy of the GNU General Public License 
along with Turntablist.  If not, see <http://www.gnu.org/licenses/>.

To contact the developer, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#include "turntablist.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <mutex>
#include <optional>

#include "dfxmath.h"
#include "dfxmisc.h"



/////////////////////////////
// DEFINES

// allow pitch bend scratching
#define USE_MIDI_PITCH_BEND
// use hard-coded MIDI CCs for parameters
//#define USE_MIDI_CC

constexpr int k_nScratchInterval = 16;//24
constexpr int k_nPowerInterval = 120;
constexpr int k_nRootKey_default = 60;   // middle C

constexpr double k_fScratchAmountMiddlePoint = 0.0;
constexpr double k_fScratchAmountLaxRange = 0.000000001;
constexpr double k_fScratchAmountMiddlePoint_UpperLimit = k_fScratchAmountMiddlePoint + k_fScratchAmountLaxRange;
constexpr double k_fScratchAmountMiddlePoint_LowerLimit = k_fScratchAmountMiddlePoint - k_fScratchAmountLaxRange;



#ifdef USE_LIBSNDFILE
	#include "sndfile.h"
#else
	#include <AudioToolbox/ExtendedAudioFile.h>
#endif



//-----------------------------------------------------------------------------------------
// Turntablist
//-----------------------------------------------------------------------------------------
// this macro does boring entry point stuff for us
DFX_EFFECT_ENTRY(Turntablist)

//-----------------------------------------------------------------------------------------
Turntablist::Turntablist(TARGET_API_BASE_INSTANCE_TYPE inInstance)
:	DfxPlugin(inInstance, kNumParameters, 0)
{
	initparameter_f(kParam_ScratchAmount, {"scratch amount"}, k_fScratchAmountMiddlePoint, k_fScratchAmountMiddlePoint, -1.0, 1.0, DfxParam::Unit::Generic);	// float2string(m_fPlaySampleRate, text);
	//initparameter_f(kScratchSpeed, {"scratch speed"}, 0.33333333, 0.33333333, 0.0, 1.0, DfxParam::Unit::Scalar);
	initparameter_f(kParam_ScratchSpeed_scrub, {"scratch speed (scrub mode)"}, 2.0, 2.0, 0.5, 5.0, DfxParam::Unit::Seconds);
	initparameter_f(kParam_ScratchSpeed_spin, {"scratch speed (spin mode)"}, 3.0, 3.0, 1.0, 8.0, DfxParam::Unit::Scalar);

	initparameter_b(kParam_Power, {"power"}, true);
	initparameter_f(kParam_SpinUpSpeed, {"spin up speed"}, 0.063, 0.05, 0.0001, 1.0, DfxParam::Unit::Scalar, DfxParam::Curve::Log);
	initparameter_f(kParam_SpinDownSpeed, {"spin down speed"}, 0.09, 0.05, 0.0001, 1.0, DfxParam::Unit::Scalar, DfxParam::Curve::Log);
	initparameter_b(kParam_NotePowerTrack, {"note-power track"}, false);

	initparameter_f(kParam_PitchShift, {"pitch shift"}, 0.0, 0.0, -100.0, 100.0, DfxParam::Unit::Percent);
	initparameter_f(kParam_PitchRange, {"pitch range"}, 12.0, 12.0, 1.0, 36.0, DfxParam::Unit::Semitones);
	initparameter_b(kParam_KeyTracking, {"key track"}, false);
	initparameter_i(kParam_RootKey, {"root key"}, k_nRootKey_default, k_nRootKey_default, 0, 0x7F, DfxParam::Unit::Notes);

	initparameter_b(kParam_Loop, {"loop"}, true, false);

	initparameter_list(kParam_ScratchMode, {"scratch mode"}, kScratchMode_Scrub, kScratchMode_Scrub, kNumScratchModes);
	setparametervaluestring(kParam_ScratchMode, kScratchMode_Scrub, "scrub");
	setparametervaluestring(kParam_ScratchMode, kScratchMode_Spin, "spin");

	initparameter_list(kParam_Direction, {"playback direction"}, kScratchDirection_Forward, kScratchDirection_Forward, kNumScratchDirections);
	setparametervaluestring(kParam_Direction, kScratchDirection_Forward, "forward");
	setparametervaluestring(kParam_Direction, kScratchDirection_Backward, "reverse");

	initparameter_list(kParam_NoteMode, {"note mode"}, kNoteMode_Reset, kNoteMode_Reset, kNumNoteModes);
	setparametervaluestring(kParam_NoteMode, kNoteMode_Reset, "reset");
	setparametervaluestring(kParam_NoteMode, kNoteMode_Resume, "resume");

	initparameter_b(kParam_PlayTrigger, {"playback trigger"}, false);

#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
	initparameter_b(kParam_Mute, {"mute"}, false);
	initparameter_f(kParam_Volume, {"volume"}, 1.0, 0.5, 0.0, 1.0, DfxParam::Unit::LinearGain, DfxParam::Curve::Cubed);
#endif

	setparameterenforcevaluelimits(kParam_RootKey, true);

#ifdef USE_LIBSNDFILE
	addchannelconfig(0, 2);	// stereo-out
	addchannelconfig(0, 1);	// mono-out
#else
	addchannelconfig(0, -1);	// 0-in / N-out
#endif

#if LOGIC_AU_PROPERTIES_AVAILABLE
	// XXX This plugin doesn't support nodes because of the problem of passing a file reference 
	// as custom property data between different machines on a network.  
	// There is no reliable way to do that, so far as I know.
	setSupportedLogicNodeOperationMode(0);
#endif


	m_fLastScratchAmount = getparameter_f(kParam_ScratchAmount);
	m_fPitchBend = k_fScratchAmountMiddlePoint;
	m_nScratchSubMode = 1;	// speed based from scrub scratch mode

	m_nRootKey = static_cast<int>(getparameter_i(kParam_RootKey));

	m_nCurrentNote = m_nRootKey;
	m_nCurrentVelocity = 0x7F;

	mPlayChangedInProcessHasPosted.test_and_set();
}

//-----------------------------------------------------------------------------------------
void Turntablist::dfx_PostConstructor()
{
	// XXX yah?  for now...
	getsettings().setSteal(true);
	getsettings().setUseChannel(false);
}


//-----------------------------------------------------------------------------------------
void Turntablist::initialize()
{
	if (sampleRateChanged())
	{
		m_nPowerIntervalEnd = static_cast<int>(getsamplerate()) / k_nPowerInterval;  // set process interval to 10 times a second - should maybe update it to 60?
		m_nScratchIntervalEnd = static_cast<int>(getsamplerate()) / k_nScratchInterval;  // set scratch interval end to 1/16 second
		m_nScratchIntervalEndBase = m_nScratchIntervalEnd;
	}

	stopNote();
}


//-----------------------------------------------------------------------------------------
void Turntablist::idle()
{
	if (!mPlayChangedInProcessHasPosted.test_and_set(std::memory_order_relaxed))
	{
		dfx_PropertyChanged(kTurntablistProperty_Play);
	}
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

	m_nRootKey = static_cast<int>(getparameter_i(kParam_RootKey));

	m_nDirection = getparameter_i(kParam_Direction);
	m_nScratchMode = getparameter_i(kParam_ScratchMode);
	m_nNoteMode = getparameter_i(kParam_NoteMode);

#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
	m_bMute = getparameter_b(kParam_Mute);
#endif
	auto const playbackTrigger = getparameter_b(kParam_PlayTrigger);


	if (getparameterchanged(kParam_PitchShift))
		processPitch();

	if (getparameterchanged(kParam_ScratchAmount) || m_bPitchBendSet)	// XXX checking m_bPitchBendSet until I fix getparameterchanged()
	{
		m_bScratchAmountSet = true;
		if ((m_fScratchAmount > k_fScratchAmountMiddlePoint_LowerLimit)
			&& (m_fScratchAmount < k_fScratchAmountMiddlePoint_UpperLimit))
		{
			m_bScratching = false;
		}
		else
		{
			if (!m_bScratching) // first time in so init stuff here
			{
				m_fPrevDesiredPosition = m_fPosition;
				m_fDesiredScratchRate2 = m_fPlaySampleRate;
				m_fScratchCenter = m_fPosition;
				m_nScratchCenter = static_cast<int>(m_fPosition);
			}
			m_bScratching = true;
		}
		processScratch();
	}

#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
	m_fNoteVolume = m_fVolume * static_cast<float>(m_nCurrentVelocity) * DfxMidi::kValueScalar;
#else
	m_fNoteVolume = static_cast<float>(m_nCurrentVelocity) * DfxMidi::kValueScalar;
#endif

	if (getparameterchanged(kParam_PitchRange))
	{
		processPitch();
	}

	if (getparameterchanged(kParam_Direction))
	{
		processDirection();
	}

	calculateSpinSpeeds();

	if (getparametertouched(kParam_PlayTrigger))
	{
		playNote(playbackTrigger);
	}
}



#pragma mark -
#pragma mark plugin state

static CFStringRef const kTurntablistPreset_AudioFileReferenceKey = CFSTR("audiofile");
static CFStringRef const kTurntablistPreset_AudioFileAliasKey = CFSTR("DestroyFX-audiofile-alias");

//-----------------------------------------------------------------------------------------
OSStatus Turntablist::SaveState(CFPropertyListRef* outData)
{
	AUSDK_Require_noerr(AUBase::SaveState(outData));

	auto const dict = const_cast<CFMutableDictionaryRef>(reinterpret_cast<CFDictionaryRef>(*outData));


// save the path of the loaded audio file, if one is currently loaded
	if (FSIsFSRefValid(&m_fsAudioFile))
	{
		auto const audioFileUrl = dfx::MakeUniqueCFType(CFURLCreateFromFSRef(kCFAllocatorDefault, &m_fsAudioFile));
		if (audioFileUrl)
		{
			auto const audioFileUrlString = dfx::MakeUniqueCFType(CFURLCopyFileSystemPath(audioFileUrl.get(), kCFURLPOSIXPathStyle));
			if (audioFileUrlString)
			{
				auto const audioFileUrlString_ptr = audioFileUrlString.get();
				auto const fileReferencesDictionary = dfx::MakeUniqueCFType(CFDictionaryCreate(kCFAllocatorDefault, (void const**)(&kTurntablistPreset_AudioFileReferenceKey), (void const**)(&audioFileUrlString_ptr), 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
				if (fileReferencesDictionary)
				{
					CFDictionarySetValue(dict, CFSTR(kAUPresetExternalFileRefs), fileReferencesDictionary.get());
				}
			}
		}

		// also save an alias of the loaded audio file, as a fall-back
		AliasHandle aliasHandle = nullptr;
		Size aliasSize = 0;
		auto const aliasStatus = createAudioFileAlias(&aliasHandle, &aliasSize);
		if (aliasStatus == noErr)
		{
			aliasSize = GetAliasSize(aliasHandle);
			if (aliasSize > 0)
			{
				auto const aliasCFData = dfx::MakeUniqueCFType(CFDataCreate(kCFAllocatorDefault, (UInt8*)(*aliasHandle), (CFIndex)aliasSize));
				if (aliasCFData)
				{
					CFDictionarySetValue(dict, kTurntablistPreset_AudioFileAliasKey, aliasCFData.get());
				}
			}
			DisposeHandle(reinterpret_cast<Handle>(aliasHandle));
		}
	}


// save the MIDI CC -> parameter assignments
	[[maybe_unused]] auto const midiSuccess = getsettings().saveMidiAssignmentsToDictionary(dict);
	assert(midiSuccess);

	return noErr;
}

//-----------------------------------------------------------------------------------------
OSStatus Turntablist::RestoreState(CFPropertyListRef inData)
{
	AUSDK_Require_noerr(AUBase::RestoreState(inData));

	auto const dict = reinterpret_cast<CFDictionaryRef>(inData);


// restore the previously loaded audio file
	bool foundAudioFilePathString = false;
	bool failedToResolveAudioFile = false;
	dfx::UniqueCFType<CFStringRef> audioFileNameString;
	auto const fileReferencesDictionary = reinterpret_cast<CFDictionaryRef>(CFDictionaryGetValue(dict, CFSTR(kAUPresetExternalFileRefs)));
	if (fileReferencesDictionary)
	{
		auto const audioFileUrlString = reinterpret_cast<CFStringRef>(CFDictionaryGetValue(fileReferencesDictionary, kTurntablistPreset_AudioFileReferenceKey));
		if (audioFileUrlString)
		{
			auto const audioFileUrl = dfx::MakeUniqueCFType(CFURLCreateWithFileSystemPath(kCFAllocatorDefault, audioFileUrlString, kCFURLPOSIXPathStyle, false));
			if (audioFileUrl)
			{
				FSRef audioFileRef {};
				if (CFURLGetFSRef(audioFileUrl.get(), &audioFileRef))
				{
					loadAudioFile(audioFileRef);
					foundAudioFilePathString = true;
					failedToResolveAudioFile = false;
				}
				else
				{
					failedToResolveAudioFile = true;
					// we can't get the proper LaunchServices "display name" if the file does not exist, so do this instead
					audioFileNameString.reset(CFURLCopyLastPathComponent(audioFileUrl.get()));
				}
			}
		}
	}

	// if resolving the audio file from the stored file path failed, then try resolving from the stored file alias
	if (!foundAudioFilePathString)
	{
		auto const aliasCFData = reinterpret_cast<CFDataRef>(CFDictionaryGetValue(dict, kTurntablistPreset_AudioFileAliasKey));
		if (aliasCFData)
		{
			if (CFGetTypeID(aliasCFData) == CFDataGetTypeID())
			{
				auto const aliasDataSize = CFDataGetLength(aliasCFData);
				auto const aliasData = CFDataGetBytePtr(aliasCFData);
				if (aliasData && (aliasDataSize > 0))
				{
					AliasHandle aliasHandle = nullptr;
					auto aliasError = PtrToHand(aliasData, reinterpret_cast<Handle*>(&aliasHandle), aliasDataSize);
					if ((aliasError == noErr) && aliasHandle)
					{
						FSRef audioFileRef {};
						Boolean wasChanged {};
						aliasError = FSResolveAlias(nullptr, aliasHandle, &audioFileRef, &wasChanged);
						if (aliasError == noErr)
						{
							loadAudioFile(audioFileRef);
							failedToResolveAudioFile = false;
						}
						else
						{
							failedToResolveAudioFile = true;
							// if we haven't gotten it already...
							if (!audioFileNameString)
							{
								HFSUniStr255 fileNameUniString {};
								auto const aliasStatus = FSCopyAliasInfo(aliasHandle, &fileNameUniString, nullptr, nullptr, nullptr, nullptr);
								if (aliasStatus == noErr)
								{
									audioFileNameString.reset(CFStringCreateWithCharacters(kCFAllocatorDefault, fileNameUniString.unicode, fileNameUniString.length));
								}
							}
						}
						DisposeHandle(reinterpret_cast<Handle>(aliasHandle));
					}
				}
			}
		}
	}

	if (failedToResolveAudioFile)
	{
		PostNotification_AudioFileNotFound(audioFileNameString.get());
	}


// restore the MIDI CC -> parameter assignments
	[[maybe_unused]] auto const midiSuccess = getsettings().restoreMidiAssignmentsFromDictionary(dict);
	assert(midiSuccess);

	return noErr;
}

//-----------------------------------------------------------------------------
OSStatus Turntablist::PostNotification_AudioFileNotFound(CFStringRef inFileName)
{
	if (!inFileName)
	{
		inFileName = CFSTR("");
	}

	auto const pluginBundleRef = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));

	auto const titleString_base = dfx::MakeUniqueCFType(CFCopyLocalizedStringFromTableInBundle(CFSTR("%@:  Audio file not found."), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("the title of the notification dialog when an audio file referenced in stored session data cannot be found")));
	auto const contextName = GetContextName();
	auto const identifyingNameString = contextName ? contextName : CFSTR(PLUGIN_NAME_STRING);
	auto const titleString = dfx::MakeUniqueCFType(CFStringCreateWithFormat(kCFAllocatorDefault, nullptr, titleString_base.get(), identifyingNameString));

	auto const messageString_base = dfx::MakeUniqueCFType(CFCopyLocalizedStringFromTableInBundle(CFSTR("The previously used file \"%@\" could not be found."), 
					CFSTR("Localizable"), pluginBundleRef, CFSTR("the detailed message of the notification dialog when an audio file referenced in stored session data cannot be found")));
	auto const messageString = dfx::MakeUniqueCFType(CFStringCreateWithFormat(kCFAllocatorDefault, nullptr, messageString_base.get(), inFileName));

	OSStatus status = CFUserNotificationDisplayNotice(0.0, kCFUserNotificationPlainAlertLevel, nullptr, nullptr, nullptr, titleString.get(), messageString.get(), nullptr);

	return status;
}

//-----------------------------------------------------------------------------
dfx::StatusCode Turntablist::dfx_GetPropertyInfo(dfx::PropertyID inPropertyID,
												 dfx::Scope inScope, unsigned int inItemIndex,
												 size_t& outDataSize, dfx::PropertyFlags& outFlags)
{
	switch (inPropertyID)
	{
		case kTurntablistProperty_Play:
			outDataSize = sizeof(Boolean);
			outFlags = dfx::kPropertyFlag_Readable | dfx::kPropertyFlag_Writable;
			return dfx::kStatus_NoError;

		case kTurntablistProperty_AudioFile:
		{
			if (!FSIsFSRefValid(&m_fsAudioFile))
			{
				return errFSBadFSRef;
			}
			AliasHandle alias = nullptr;
			Size aliasSize = 0;
			AUSDK_Require_noerr(createAudioFileAlias(&alias, &aliasSize));
			outDataSize = aliasSize;
			outFlags = dfx::kPropertyFlag_Readable | dfx::kPropertyFlag_Writable;
			DisposeHandle(reinterpret_cast<Handle>(alias));
			return dfx::kStatus_NoError;
		}

		default:
			return DfxPlugin::dfx_GetPropertyInfo(inPropertyID, inScope, inItemIndex, outDataSize, outFlags);
	}
}

//-----------------------------------------------------------------------------
dfx::StatusCode Turntablist::dfx_GetProperty(dfx::PropertyID inPropertyID,
											 dfx::Scope inScope, unsigned int inItemIndex,
											 void* outData)
{
	switch (inPropertyID)
	{
		case kTurntablistProperty_Play:
			dfx::MemCpyObject(static_cast<Boolean>(m_bPlay), outData);
			return dfx::kStatus_NoError;

		case kTurntablistProperty_AudioFile:
		{
			if (!FSIsFSRefValid(&m_fsAudioFile))
			{
				return errFSBadFSRef;
			}
			AliasHandle alias = nullptr;
			Size aliasSize = 0;
			AUSDK_Require_noerr(createAudioFileAlias(&alias, &aliasSize));
			std::memcpy(outData, *alias, aliasSize);
			DisposeHandle(reinterpret_cast<Handle>(alias));
			return dfx::kStatus_NoError;
		}

		default:
			return DfxPlugin::dfx_GetProperty(inPropertyID, inScope, inItemIndex, outData);
	}
}

//-----------------------------------------------------------------------------
dfx::StatusCode Turntablist::dfx_SetProperty(dfx::PropertyID inPropertyID,
											 dfx::Scope inScope, unsigned int inItemIndex,
											 void const* inData, size_t inDataSize)
{
	switch (inPropertyID)
	{
		case kTurntablistProperty_Play:
			m_bPlay = dfx::Enliven<Boolean>(inData);
			playNote(m_bPlay);
			return dfx::kStatus_NoError;

		case kTurntablistProperty_AudioFile:
		{
			AliasHandle alias = nullptr;
			AUSDK_Require_noerr(PtrToHand(inData, reinterpret_cast<Handle*>(&alias), inDataSize));
			if (!alias)
			{
				return memFullErr;
			}
			auto const status = resolveAudioFileAlias(alias);
			DisposeHandle(reinterpret_cast<Handle>(alias));
			return status;
		}

		default:
			return DfxPlugin::dfx_SetProperty(inPropertyID, inScope, inItemIndex, inData, inDataSize);
	}
}

//-----------------------------------------------------------------------------------------
OSStatus Turntablist::GetParameterInfo(AudioUnitScope inScope, 
									   AudioUnitParameterID inParameterID, 
									   AudioUnitParameterInfo& outParameterInfo)
{
	AUSDK_Require_noerr(DfxPlugin::GetParameterInfo(inScope, inParameterID, outParameterInfo));

	switch (inParameterID)
	{
		case kParam_ScratchAmount:
		case kParam_ScratchMode:
		case kParam_ScratchSpeed_scrub:
		case kParam_ScratchSpeed_spin:
			HasClump(outParameterInfo, kParamGroup_Scratching);
			break;

		case kParam_Power:
		case kParam_SpinUpSpeed:
		case kParam_SpinDownSpeed:
		case kParam_NotePowerTrack:
			HasClump(outParameterInfo, kParamGroup_Power);
			break;

		case kParam_PitchShift:
		case kParam_PitchRange:
		case kParam_KeyTracking:
		case kParam_RootKey:
			HasClump(outParameterInfo, kParamGroup_Pitch);
			break;

		case kParam_Loop:
		case kParam_Direction:
		case kParam_NoteMode:
		case kParam_PlayTrigger:
			HasClump(outParameterInfo, kParamGroup_Playback);
			break;

#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
		case kParam_Mute:
		case kParam_Volume:
			HasClump(outParameterInfo, kParamGroup_Output);
			break;
#endif

		default:
			break;
	}

	return noErr;
}

//-----------------------------------------------------------------------------------------
OSStatus Turntablist::CopyClumpName(AudioUnitScope inScope, UInt32 inClumpID, 
									UInt32 inDesiredNameLength, CFStringRef* outClumpName)
{
	if (inClumpID < kParamGroup_BaseID)
	{
		return TARGET_API_BASE_CLASS::CopyClumpName(inScope, inClumpID, inDesiredNameLength, outClumpName);
	}
	if (inScope != kAudioUnitScope_Global)
	{
		return kAudioUnitErr_InvalidScope;
	}

	auto const pluginBundleRef = CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));
	switch (inClumpID)
	{
		case kParamGroup_Scratching:
			*outClumpName = CFCopyLocalizedStringFromTableInBundle(CFSTR("scratching"), CFSTR("Localizable"), pluginBundleRef, CFSTR("parameter clump name"));
			return noErr;
		case kParamGroup_Playback:
			*outClumpName = CFCopyLocalizedStringFromTableInBundle(CFSTR("audio sample playback"), CFSTR("Localizable"), pluginBundleRef, CFSTR("parameter clump name"));
			return noErr;
		case kParamGroup_Power:
			*outClumpName = CFCopyLocalizedStringFromTableInBundle(CFSTR("turntable power"), CFSTR("Localizable"), pluginBundleRef, CFSTR("parameter clump name"));
			return noErr;
		case kParamGroup_Pitch:
			*outClumpName = CFCopyLocalizedStringFromTableInBundle(CFSTR("pitch"), CFSTR("Localizable"), pluginBundleRef, CFSTR("parameter clump name"));
			return noErr;
#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
		case kParamGroup_Output:
			*outClumpName = CFCopyLocalizedStringFromTableInBundle(CFSTR("audio output"), CFSTR("Localizable"), pluginBundleRef, CFSTR("parameter clump name"));
			return noErr;
#endif
		default:
			return kAudioUnitErr_InvalidPropertyValue;
	}
}



//-----------------------------------------------------------------------------------------
void Turntablist::setPlay(bool inPlayState, bool inShouldSendNotification)
{
	if (std::exchange(m_bPlay, inPlayState) != inPlayState)
	{
		if (isrenderthread())
		{
			mPlayChangedInProcessHasPosted.clear(std::memory_order_relaxed);
		}
		else
		{
			dfx_PropertyChanged(kTurntablistProperty_Play);
		}
	}
}

//-----------------------------------------------------------------------------
OSStatus Turntablist::createAudioFileAlias(AliasHandle* outAlias, Size* outDataSize)
{
	if (!outAlias)
	{
		return paramErr;
	}

	AUSDK_Require_noerr(FSNewAlias(nullptr, &m_fsAudioFile, outAlias));
	if (*outAlias == nullptr)
	{
		return nilHandleErr;
	}

	if (outDataSize)
	{
		*outDataSize = GetAliasSize(*outAlias);
	}

	return noErr;
}

//-----------------------------------------------------------------------------
OSStatus Turntablist::resolveAudioFileAlias(AliasHandle const inAlias)
{
	FSRef audioFileRef {};
	Boolean wasChanged {};
	auto status = FSResolveAlias(nullptr, inAlias, &audioFileRef, &wasChanged);
	if (status == noErr)
	{
		status = loadAudioFile(audioFileRef);
	}

	return status;
}



#pragma mark -
#pragma mark audio processing

//-----------------------------------------------------------------------------------------
OSStatus Turntablist::loadAudioFile(FSRef const& inFileRef)
{
// ExtAudioFile
#ifndef USE_LIBSNDFILE
	ExtAudioFileRef audioFileRef = nullptr;
	AUSDK_Require_noerr(ExtAudioFileOpen(&inFileRef, &audioFileRef));

	SInt64 audioFileNumFrames = 0;
	UInt32 dataSize = sizeof(audioFileNumFrames);
	AUSDK_Require_noerr(ExtAudioFileGetProperty(audioFileRef, kExtAudioFileProperty_FileLengthFrames, &dataSize, &audioFileNumFrames));

	AudioStreamBasicDescription audioFileStreamFormat {};
	dataSize = sizeof(audioFileStreamFormat);
	AUSDK_Require_noerr(ExtAudioFileGetProperty(audioFileRef, kExtAudioFileProperty_FileDataFormat, &dataSize, &audioFileStreamFormat));

	auto const clientStreamFormat = ausdk::ASBD::CreateCommonFloat32(audioFileStreamFormat.mSampleRate, audioFileStreamFormat.mChannelsPerFrame);
	AUSDK_Require_noerr(ExtAudioFileSetProperty(audioFileRef, kExtAudioFileProperty_ClientDataFormat, sizeof(clientStreamFormat), &clientStreamFormat));

	std::unique_lock guard(m_AudioFileLock);

	m_bAudioFileHasBeenLoaded = false;	// XXX cuz we're about to possibly re-allocate the audio buffer and invalidate what might already be there

	m_auBufferList.Allocate(clientStreamFormat, static_cast<UInt32>(audioFileNumFrames));
	auto& abl = m_auBufferList.PrepareBuffer(clientStreamFormat, static_cast<UInt32>(audioFileNumFrames));
	auto audioFileNumFrames_temp = static_cast<UInt32>(audioFileNumFrames);
	auto status = ExtAudioFileRead(audioFileRef, &audioFileNumFrames_temp, &abl);
	if (status != noErr)
	{
		m_auBufferList.Deallocate();
		return status;
	}
	if (audioFileNumFrames_temp != static_cast<UInt32>(audioFileNumFrames))  // XXX do something?
	{
		// XXX error?
		std::fprintf(stderr, PLUGIN_NAME_STRING ":  audio data size mismatch!\nsize requested: %ld, size read: %u\n\n", static_cast<long>(audioFileNumFrames), static_cast<unsigned int>(audioFileNumFrames_temp));
	}

	status = ExtAudioFileDispose(audioFileRef);

	m_nNumChannels = clientStreamFormat.mChannelsPerFrame;
	m_nSampleRate = dfx::math::IRound(clientStreamFormat.mSampleRate);
	m_nNumSamples = dfx::math::ToUnsigned(audioFileNumFrames);

	m_fPlaySampleRate = static_cast<double>(m_nSampleRate);
	m_fSampleRate = static_cast<double>(m_nSampleRate);
	calculateSpinSpeeds();
	m_fPosition = 0.0;
	m_fPosOffset = 0.0;
	m_fNumSamples = static_cast<double>(m_nNumSamples);



// libsndfile
#else
	std::array<UInt8, 2048> file {};
	AUSDK_Require_noerr(FSRefMakePath(&inFileRef, file.data(), file.size()));
//std::fprintf(stderr, PLUGIN_NAME_STRING " audio file:  %s\n", file.data());

	SF_INFO sfInfo {};
	SNDFILE* const sndFile = sf_open(reinterpret_cast<char const*>(file.data()), SFM_READ, &sfInfo);

	if (!sndFile)
	{
		// print error
		std::array<char, 256> buffer {};
		sf_error_str(sndFile, buffer.data(), buffer.size() - 1);
		std::fprintf(stderr, "\n" PLUGIN_NAME_STRING " could not open the audio file:  %s\nlibsndfile error message:  %s\n", file.data(), buffer.data());
		return sf_error(sndFile);
	}

#if 0
	std::fprintf(stderr, "**** SF_INFO dump for:  %s\n", file.data());
	std::fprintf(stderr, "     samplerate:  %d\n", sfInfo.samplerate);
	std::fprintf(stderr, "     frames:  %d (0x%08x)\n", sfInfo.frames, sfInfo.frames);
	std::fprintf(stderr, "     channels:  %d\n", sfInfo.channels);
	std::fprintf(stderr, "     format:  %d\n", sfInfo.format);
	std::fprintf(stderr, "     sections:  %d\n", sfInfo.sections);
	std::fprintf(stderr, "     seekable:  %d\n", sfInfo.seekable);
#endif

	sf_command(sndFile, SFC_SET_NORM_FLOAT, nullptr, SF_TRUE);

	std::unique_lock guard(m_AudioFileLock);

	m_nNumChannels = dfx::math::ToUnsigned(sfInfo.channels);
	m_nSampleRate = sfInfo.samplerate;
	m_nNumSamples = dfx::math::ToUnsigned(sfInfo.frames);

	m_fPlaySampleRate = static_cast<double>(m_nSampleRate);
	m_fSampleRate = static_cast<double>(m_nSampleRate);
	calculateSpinSpeeds();
	m_fPosition = 0.0;
	m_fPosOffset = 0.0;
	m_fNumSamples = static_cast<double>(m_nNumSamples);

	m_fLeft = nullptr;
	m_fRight = nullptr;

	m_fBuffer.assign(m_nNumChannels * m_nNumSamples, 0.0f);

	m_fLeft = m_fBuffer.data();
	m_fRight = m_fBuffer.data() + ((m_nNumChannels == 1) ? 0 : m_nNumSamples);

	// do file loading here!!
	sf_count_t const sizein = sfInfo.frames * sfInfo.channels;
	sf_count_t sizeout = sf_read_float(sndFile, m_fBuffer.data(), sizein);
	if (sizeout != sizein)
	{
		// XXX error?
		std::fprintf(stderr, PLUGIN_NAME_STRING ":  audio data size mismatch!\nsize-in: %ld, size-out: %ld\n\n", static_cast<long>(sizein), static_cast<long>(sizeout));
	}

	if (m_nNumChannels >= 2)
	{
		std::vector<float> tempBuffer(m_nNumSamples, 0.0f);

		// do swaps
		for (size_t z = 0; z < tempBuffer.size(); z++)
		{
			tempBuffer[z] = m_fBuffer[(z * m_nNumChannels) + 1];	// copy channel 2 into buffer
			m_fBuffer[z] = m_fBuffer[z * m_nNumChannels];	// handle channel 1
		}

		for (size_t z = 0; z < tempBuffer.size(); z++)
		{
			m_fBuffer[m_nNumSamples + z] = tempBuffer[z];	// make channel 2
		}
	}

	// end file loading here!!

	if (sf_close(sndFile) != 0)
	{
		// error closing
	}
#endif


	processPitch();	// set up stuff

	// ready to play
	m_bAudioFileHasBeenLoaded = true;
	guard.unlock();

	m_fsAudioFile = inFileRef;  // XXX TODO: this object needs its own serialized lock access as well
	dfx_PropertyChanged(kTurntablistProperty_AudioFile);

	return noErr;
}

//-----------------------------------------------------------------------------------------
void Turntablist::calculateSpinSpeeds()
{
//	m_fUsedSpinUpSpeed = (((std::exp(10.0 * m_fSpinUpSpeed) - 1.0) / (std::exp(10.0) - 1.0)) * m_fSampleRate) / static_cast<double>(m_nPowerIntervalEnd);
//	m_fUsedSpinDownSpeed = (((std::exp(10.0 * m_fSpinDownSpeed) - 1.0) / (std::exp(10.0) - 1.0)) * m_fSampleRate) / static_cast<double>(m_nPowerIntervalEnd);
	m_fUsedSpinUpSpeed = m_fSpinUpSpeed * m_fSampleRate / static_cast<double>(m_nPowerIntervalEnd);
	m_fUsedSpinDownSpeed = m_fSpinDownSpeed * m_fSampleRate / static_cast<double>(m_nPowerIntervalEnd);
}

//-----------------------------------------------------------------------------------------
void Turntablist::processaudio(float const* const* /*inAudio*/, float* const* outAudio, size_t inNumFrames)
{
	std::optional<size_t> eventFrame;
	auto const numEvents = getmidistate().getBlockEventCount();
	size_t eventIndex = 0;

	auto const numOutputs = getnumoutputs();
	auto interpolateHermiteFunction = dfx::math::InterpolateHermite_NoWrap;
	if (m_bLoop)
	{
		interpolateHermiteFunction = dfx::math::InterpolateHermite;
	}


	if (numEvents == 0)
	{
		eventFrame.reset();
	}
	else
	{
		eventFrame = getmidistate().getBlockEvent(eventIndex).mOffsetFrames;
	}
	for (size_t frameIndex = 0; frameIndex < inNumFrames; frameIndex++)
	{
		//
		// process MIDI events if any
		//
		while (eventFrame && (frameIndex == *eventFrame))
		{
			processMidiEvent(eventIndex);
			eventIndex++;	// check next event

			if (eventIndex >= numEvents)
			{
				eventFrame.reset();	// no more events
			}
		}

		if (m_bScratching)  // handle scratching
		{
			m_nScratchInterval++;
			if (m_nScratchInterval > m_nScratchIntervalEnd)
			{
				if (m_nScratchMode == kScratchMode_Spin)
				{
					processScratch();
				}
				else
				{
					processScratchStop();
				}
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
						m_fPlaySampleRate = std::min(m_fPlaySampleRate, m_fDesiredScratchRate2);
					}
					else	// negative
					{
						m_fPlaySampleRate = std::max(m_fPlaySampleRate, m_fDesiredScratchRate2);
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
					m_fPlaySampleRate = std::max(m_fPlaySampleRate - m_fUsedSpinDownSpeed, 0.);
				}
			}
			else // power on - spin up
			{
				if (m_fPlaySampleRate < m_fDesiredPitch) // too low so bring up
				{
					m_fPlaySampleRate = std::min(m_fPlaySampleRate + m_fUsedSpinUpSpeed, m_fDesiredPitch);
				}
				else if (m_fPlaySampleRate > m_fDesiredPitch) // too high so bring down
				{
					m_fPlaySampleRate = std::max(m_fPlaySampleRate - m_fUsedSpinUpSpeed, m_fDesiredPitch);
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
						{
							m_fPosition = 0.0;
						}
					}
				}
			}

			m_fPosOffset = m_fPlaySampleRate / getsamplerate();   //m_fPlaySampleRate / m_fSampleRate;

			std::unique_lock const guard(m_AudioFileLock, std::try_to_lock);
			if (guard.owns_lock() && m_bAudioFileHasBeenLoaded && m_bNoteIsOn)
			{
				if (!m_bPlayForward) // if play direction = reverse
				{
					if (m_fPlaySampleRate != 0.)
					{
						m_fPosition -= m_fPosOffset;
						while (m_fPosition < 0.)	// was if
						{
							if (!m_bLoop)	// off
							{
								if (m_bPlayedReverse)
								{
									stopNote(true);
									m_fPosition = 0.;
								}
								else
								{
									m_fPosition += m_fNumSamples; // - 1;
									m_bPlayedReverse = true;
								}
							}
							else
							{
								m_fPosition += m_fNumSamples; // - 1;
							}
						}
					}
				}  // if (!bPlayForward)

#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
				if (!m_bMute)   // if audio on
				{
#endif
					if (dfx::math::IsZero(m_fPlaySampleRate))
					{
						for (size_t ch = 0; ch < numOutputs; ch++)
						{
							outAudio[ch][frameIndex] = 0.f;
						}
					}
					else
					{
//#define NO_INTERPOLATION
//#define LINEAR_INTERPOLATION
#define CUBIC_INTERPOLATION

						for (size_t ch = 0; ch < numOutputs; ch++)
						{
						#ifdef USE_LIBSNDFILE
							auto const output = (ch == 0) ? m_fLeft : m_fRight;
						#else
							AudioBufferList const& abl = m_auBufferList.GetBufferList();
							size_t ablChannel = ch;
							if (ch >= abl.mNumberBuffers)
							{
								ablChannel = abl.mNumberBuffers - 1;
								// XXX only do the channel remapping for mono->stereo upmixing special case (?)
								if (ch > 1)
								{
									break;
								}
							}
							auto const output = static_cast<float const*>(abl.mBuffers[ablChannel].mData);
						#endif

#ifdef NO_INTERPOLATION
							auto const outputValue = output[static_cast<size_t>(m_fPosition)];
#endif  // NO_INTERPOLATION

#ifdef LINEAR_INTERPOLATION
							auto const [pos_fractional, pos_integral] = dfx::math::ModF<size_t>(m_fPosition);
							assert(pos_integral < m_nNumSamples);
							auto const pos_integral_next = (pos_integral + 1) % m_nNumSamples;
							float const outputValue = std::lerp(output[pos_integral], output[pos_integral_next], static_cast<float>(pos_fractional));
#endif  // LINEAR_INTERPOLATION

#ifdef CUBIC_INTERPOLATION
							auto const outputValue = interpolateHermiteFunction({output, m_nNumSamples}, m_fPosition);
#endif  // CUBIC_INTERPOLATION

							outAudio[ch][frameIndex] = outputValue * m_fNoteVolume;
						}
					}
#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
				}  // if (!m_bMute)
				else
				{
					for (size_t ch = 0; ch < numOutputs; ch++)
					{
						outAudio[ch][frameIndex] = 0.f;
					}
				}
#endif


				if (m_bPlayForward)	// if play direction = forward
				{
					m_bPlayedReverse = false;
					if (!dfx::math::IsZero(m_fPlaySampleRate))
					{
						m_fPosition += m_fPosOffset;

						while (m_fPosition >= m_fNumSamples)
						{
							m_fPosition -= m_fNumSamples;
							if (!m_bLoop) // off
							{
								stopNote(true);
							}
						}
					}
				}  // if (bPlayForward)
			}  // if (owns_lock && m_bAudioFileHasBeenLoaded && m_bNoteIsOn)
			else
			{
				for (size_t ch = 0; ch < numOutputs; ch++)
				{
					outAudio[ch][frameIndex] = 0.f;
				}
			}
		}  // if (bNoteIsOn)
		else
		{
			for (size_t ch = 0; ch < numOutputs; ch++)
			{
				outAudio[ch][frameIndex] = 0.f;
			}
		}
	}  // per frame loop
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
		{
			m_fScratchAmount = m_fPitchBend;
		}
*/
		m_bScratchAmountSet = true;

		m_bPitchBendSet = false;
	}


	if (m_bScratching)  // scratching
	{
		if (m_nScratchMode == kScratchMode_Spin)
		{
			m_nScratchDir = (m_fScratchAmount >= k_fScratchAmountMiddlePoint) ? kScratchDirection_Forward : kScratchDirection_Backward;
		}

	// todo:
	// handle scratching like power
	// scratching will set target sample rate and system will catch up based on scratch speed parameter
	// sort of like spin up/down speed param..

	// if current rate below target rate then add samplespeed2
	// it gone over then set to target rate
	// if current rate above target rate then sub samplespeed2
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

					fIntervalScaler = (static_cast<double>(m_nScratchInterval) / static_cast<double>(m_nScratchIntervalEnd)) + 1.0;

					m_fDesiredPosition = m_fScratchCenter + (contractparametervalue(kParam_ScratchAmount, m_fScratchAmount) * m_fScratchSpeed_scrub * m_fSampleRate);

					double fDesiredDelta {};

					if (m_nScratchInterval == 0)
					{
						m_fPosition = m_fDesiredPosition;
						assert(m_fPosition >= 0.);
						assert(m_fPosition < m_fNumSamples);
						m_fTinyScratchAdjust = 0.0;
					}
					else
					{
						fDesiredDelta = (m_fDesiredPosition - m_fPosition); //m_nScratchInterval;
						m_fTinyScratchAdjust = (fDesiredDelta - m_fPlaySampleRate) / m_fSampleRate;
					}








					// do something with desireddelta and scratchinterval

					// figure out direction
					double fDiff = contractparametervalue(kParam_ScratchAmount, m_fScratchAmount) - contractparametervalue(kParam_ScratchAmount, m_fLastScratchAmount);

					if (fDiff < 0.0)
					{
						fDiff = -fDiff;
						m_nScratchDir = kScratchDirection_Backward;
					}
					else
					{
						m_nScratchDir = kScratchDirection_Forward;
					}

					m_fDesiredScratchRate2 = m_fSampleRate * m_fScratchSpeed_scrub * static_cast<double>(m_nScratchInterval);

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
					//accel = (desired rate - current rate)/ time
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

					double fDiff = contractparametervalue(kParam_ScratchAmount, m_fScratchAmount) - contractparametervalue(kParam_ScratchAmount, m_fLastScratchAmount);

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
					fIntervalScaler = (static_cast<double>(m_nScratchInterval) / static_cast<double>(m_nScratchIntervalEnd)) + 1.0;

					m_fDesiredScratchRate2 = (fDiff * m_fSampleRate * m_fScratchSpeed_scrub * static_cast<double>(k_nScratchInterval)) / fIntervalScaler;

					m_fTinyScratchAdjust = (m_fDesiredScratchRate2 - m_fPlaySampleRate) / static_cast<double>(m_nScratchIntervalEnd); 


					m_fPlaySampleRate += m_fTinyScratchAdjust;
					m_bScratchAmountSet = false;
				}
			}

			m_fLastScratchAmount = m_fScratchAmount;
		}
		m_fScratchVolumeModifier = std::min(m_fPlaySampleRate / m_fSampleRate, 1.5);

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
	auto const entryDesiredPitch = m_fDesiredPitch;
	if (m_bKeyTracking)
	{
		auto const note2play = m_nCurrentNote - m_nRootKey;
		m_fBasePitch = m_fSampleRate * std::pow(2.0, static_cast<double>(note2play) / 12.0);
	}
	else
	{
		m_fBasePitch = m_fSampleRate;
	}
	double fPitchAdjust = m_fPitchShift * m_fPitchRange;  // shift in semitones
	fPitchAdjust = std::pow(2.0, fPitchAdjust / 12.0);  // shift as a scalar of the playback rate
	m_fDesiredPitch = m_fBasePitch * fPitchAdjust;

	if (entryDesiredPitch == m_fPlaySampleRate)
	{
		m_fPlaySampleRate = m_fDesiredPitch;  // do instant adjustment
	}
}

//-----------------------------------------------------------------------------------------
void Turntablist::processDirection()
{
	m_bPlayForward = true;

	if (m_bScratching)
	{
		if (m_nScratchDir == kScratchDirection_Backward)
		{
			m_bPlayForward = false;
		}
	}
	else
	{
		if (m_bWasScratching)
		{
			if (m_nDirection == kScratchDirection_Backward)
			{
				m_bPlayForward = false;
			}
		}
		else
		{
			if (m_nDirection == kScratchDirection_Backward)
			{
				m_bPlayForward = false;
			}
		}
	}
}



#pragma mark -
#pragma mark MIDI processing

//-----------------------------------------------------------------------------------------
void Turntablist::processMidiEvent(size_t inEventIndex)
{
	auto const& event = getmidistate().getBlockEvent(inEventIndex);

	if (DfxMidi::isNote(event.mStatus))
	{
		auto const note = event.mByte1;
		auto const velocity = (event.mStatus == DfxMidi::kStatus_NoteOff) ? 0 : event.mByte2;
		noteOn(note, velocity, event.mOffsetFrames);  // handle note on or off
	}
	else if (event.mStatus == DfxMidi::kStatus_CC)
	{
		if (event.mByte1 == DfxMidi::kCC_AllNotesOff)
		{
			stopNote(true);
			if (m_nNoteMode == kNoteMode_Reset)
			{
				m_fPosition = 0.0;
			}
		}

#ifdef USE_MIDI_CC
		constexpr int dataMidpoint = 64;
		if ((event.mByte1 >= dataMidpoint) && (event.mByte1 <= (dataMidpoint + kNumParameters - 1)))
		{
			dfx::ParameterID const parameterID = event.mByte1 - dataMidpoint;
			auto const correctedData = fixMidiData(parameterID, event.mByte2);
			float const value = static_cast<float>(correctedData) * DfxMidi::kValueScalar;
			setparameter_f(parameterID, value);
			postupdate_parameter(parameterID);
		}
#endif
	}
#ifdef USE_MIDI_PITCH_BEND
	else if (event.mStatus == DfxMidi::kStatus_PitchBend)
	{
		// handle pitch bend here
		int const pitchBendValue = (event.mByte2 << 7) | event.mByte1;
		m_bPitchBendSet = true;

		if (pitchBendValue == DfxMidi::kPitchBendMidpointValue)
		{
			m_fPitchBend = k_fScratchAmountMiddlePoint;
		}
		else
		{
			m_fPitchBend = static_cast<double>(pitchBendValue) / static_cast<double>((DfxMidi::kPitchBendMidpointValue * 2) - 1);
			m_fPitchBend = std::clamp(m_fPitchBend, 0.0, 1.0);
			m_fPitchBend = expandparametervalue(kParam_ScratchAmount, m_fPitchBend);
		}

		setparameter_f(kParam_ScratchAmount, m_fPitchBend);
//		postupdate_parameter(kParam_ScratchAmount);	// XXX post notification? for some reason it makes the results sound completely different
		m_fScratchAmount = m_fPitchBend;
	}
#endif
}

//-----------------------------------------------------------------------------------------
void Turntablist::noteOn(int inNote, int inVelocity, size_t /*inOffsetFrames*/)
{
	auto const power_old = m_bPower;

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
			{
				m_fPosition = 0.0;
			}
			m_bPlayedReverse = false;
		}
	}
	else
	{
		m_bNoteIsOn = true;
		setPlay(true);

		if (m_nNoteMode == kNoteMode_Reset)
		{
			m_fPosition = 0.0;
		}
		// calculate note volume
#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
		m_fNoteVolume = m_fVolume * static_cast<float>(m_nCurrentVelocity) * DfxMidi::kValueScalar;
#else
		m_fNoteVolume = static_cast<float>(m_nCurrentVelocity) * DfxMidi::kValueScalar;
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
	{
		postupdate_parameter(kParam_Power);
	}
}

//-----------------------------------------------------------------------------------------
void Turntablist::stopNote(bool inStopPlay)
{
	m_bNoteIsOn = false;
	m_bPlayedReverse = false;

	if (inStopPlay)
	{
		setPlay(false);
	}
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
	{
		noteOn(m_nCurrentNote, 0, 0);  // note off
	}
}

//-----------------------------------------------------------------------------------------
int Turntablist::fixMidiData(dfx::ParameterID inParameterID, int inValue) noexcept
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
			return (inValue < 64) ? 0 : 0x7F;
		default:
			return inValue;
	}
}
