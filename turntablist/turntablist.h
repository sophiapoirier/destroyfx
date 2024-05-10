/*------------------------------------------------------------------------
Copyright (c) 2004 bioroid media development & Copyright (C) 2004-2024 Sophia Poirier
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


// 1.1 TO DO:
// 1) fix position based scratching
// how fix
// calculate new desired speed/position
// when processcratch gets called
// 1) missed offset = scratch interval * tiny sample rate
// 2) new offset = blah + missed offset

// 1.2 TO DO:
// 1) put midi learn back in - remove hardcoded midi cc's
// 2) add settings (midi learn) saving/loading
// 3) add spin mode - sub mode 2


#pragma once

#include <CoreServices/CoreServices.h>
#include <atomic>
#include <expected>
#include <vector>

#include "dfxmisc.h"
#include "dfxmutex.h"
#include "dfxplugin.h"



//#define USE_LIBSNDFILE


//------------------------------------------------------------------------------------------
enum
{
	kScratchDirection_Forward,
	kScratchDirection_Backward,
	kNumScratchDirections
};

enum
{
	kScratchMode_Scrub,
	kScratchMode_Spin,
	kNumScratchModes
};

enum
{
	kNoteMode_Reset,
	kNoteMode_Resume,
	kNumNoteModes
};


//------------------------------------------------------------------------------------------
// parameters
enum : dfx::ParameterID
{
	// scratching
	kParam_ScratchAmount,
	kParam_ScratchMode,
	kParam_ScratchSpeed_scrub,
	kParam_ScratchSpeed_spin,

	// turntable power
	kParam_Power,
	kParam_SpinUpSpeed,
	kParam_SpinDownSpeed,
	kParam_NotePowerTrack,

	// pitch
	kParam_PitchShift,
	kParam_PitchRange,
	kParam_KeyTracking,
	kParam_RootKey,

	// audio sample playback
	kParam_Loop,
	kParam_Direction,
	kParam_NoteMode,
	kParam_PlayTrigger,   // play/note activity

#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
	// audio output
	kParam_Mute,
	kParam_Volume,
#endif
	kNumParameters
};

enum : dfx::PropertyID
{
	kTurntablistProperty_Play = dfx::kPluginProperty_EndOfList,
	kTurntablistProperty_AudioFile
};


//------------------------------------------------------------------------------------------
using UniqueAliasHandle = dfx::UniqueOpaqueType<AliasHandle, [](AliasHandle alias)
{
	DisposeHandle(reinterpret_cast<Handle>(alias));
}>;



//------------------------------------------------------------------------------------------
class Turntablist final : public DfxPlugin
{
public:
	explicit Turntablist(TARGET_API_BASE_INSTANCE_TYPE inInstance);

	void dfx_PostConstructor() override;

	void initialize() override;

	void idle() override;

	void processaudio(float const* const* inAudio, float* const* outAudio, size_t inNumFrames) override;
	void processparameters() override;

	dfx::StatusCode dfx_GetPropertyInfo(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned int inItemIndex,
										size_t& outDataSize, dfx::PropertyFlags& outFlags) override;
	dfx::StatusCode dfx_GetProperty(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned int inItemIndex,
									void* outData) override;
	dfx::StatusCode dfx_SetProperty(dfx::PropertyID inPropertyID, dfx::Scope inScope, unsigned int inItemIndex,
									void const* inData, size_t inDataSize) override;

	OSStatus SaveState(CFPropertyListRef* outData) override;
	OSStatus RestoreState(CFPropertyListRef inData) override;

	OSStatus GetParameterInfo(AudioUnitScope inScope, 
							  AudioUnitParameterID inParameterID, 
							  AudioUnitParameterInfo& outParameterInfo) override;
	OSStatus CopyClumpName(AudioUnitScope inScope, UInt32 inClumpID, 
						   UInt32 inDesiredNameLength, CFStringRef* outClumpName) override;

private:
	enum : UInt32
	{
		kParamGroup_BaseID = kAudioUnitClumpID_System + 1,
		kParamGroup_Scratching = kParamGroup_BaseID,
		kParamGroup_Playback,
		kParamGroup_Power,
		kParamGroup_Pitch
#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
		, kParamGroup_Output
#endif
	};

	OSStatus loadAudioFile(FSRef const& inFileRef);
	std::expected<UniqueAliasHandle, OSStatus> createAudioFileAlias() const;
	OSStatus resolveAudioFileAlias(AliasHandle inAlias);

	void processMidiEvent(size_t inEventIndex);
	void processScratch(bool inSetParameter = false);
	void processScratchStop();
	void processPitch();
	void processDirection();
	void calculateSpinSpeeds();
	void setPlay(bool inPlayState, bool inShouldSendNotification = true);

	void noteOn(int inNote, int inVelocity, size_t inOffsetFrames);
	void stopNote(bool inStopPlay = false);
	void playNote(bool inValue);
	static int fixMidiData(dfx::ParameterID inParameterID, int inValue) noexcept;

	OSStatus PostNotification_AudioFileNotFound(CFStringRef inFileName);


	int m_nCurrentNote = 0;
	int m_nCurrentVelocity = 0;
	bool m_bNoteIsOn = false;

	FSRef m_fsAudioFile {};

#ifdef USE_LIBSNDFILE
	std::vector<float> m_fBuffer;
	float* m_fLeft = nullptr;
	float* m_fRight = nullptr;
#else
	ausdk::AUBufferList m_auBufferList;
#endif

	// our 32-bit floating point audio info
	size_t m_nNumChannels = 0;	// 1=mono, 2=stereo, 0=yomama
	size_t m_nNumSamples = 0;	// number of samples per channel
	int m_nSampleRate = 0;
	double m_fSampleRate = 0.;

	// optional
	double m_fBasePitch = 0.;
	double m_fPlaySampleRate = 0.;

	// switches
	bool m_bPower = false;	// on/off
	bool m_bNotePowerTrack = false;	// scratch mode on/off
	// TODO: use dfx::SmoothedValue for continuous parameters
	double m_fScratchAmount = 0.;
	double m_fLastScratchAmount = 0.;
#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
	bool m_bMute = false;	// on/off
#endif
	double m_fPitchShift = 0.;
	long m_nDirection = 0;
	//float m_fScratchSpeed = 0.f;
	double m_fScratchSpeed_scrub = 0., m_fScratchSpeed_spin = 0.;

	// modifiers
	long m_nNoteMode = 0;	// reset/resume
	bool m_bLoop = false;	// on/off
	double m_fPitchRange = 0.;
	double m_fSpinDownSpeed = 0.;
	double m_fSpinUpSpeed = 0.;
	bool m_bKeyTracking = false;
#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
	float m_fVolume = 0.f;
#endif


	double m_fUsedSpinDownSpeed = 0.;
	double m_fUsedSpinUpSpeed = 0.;

	bool m_bPlayedReverse = false;
	int m_nRootKey = 0;

	double m_fPosition = 0.;
	double m_fPosOffset = 0.;
	double m_fNumSamples = 0.;

	bool m_bPlay = false;

	bool m_bPitchBendSet = false;
	double m_fPitchBend = 0.;
	int m_nScratchDir = 0;
	bool m_bScratching = false;
	bool m_bWasScratching = false;
	int m_nWasScratchingDir = 0;

	double m_fDesiredPitch = 0.;

	long m_nScratchMode = 0;
	long m_nScratchSubMode = 0;


	float m_fNoteVolume = 0.f;	// recalculated on note on and volume changes

	int m_nPowerIntervalEnd = 0;

	bool m_bPlayForward = true;


	double m_fDesiredScratchRate = 0.;
	int m_nScratchInterval = 0;
	int m_nScratchIntervalEnd = 0;
	int m_nScratchIntervalEndBase = 0;
	bool m_bScratchStop = false;

	int m_nScratchCenter = 0;	// center position where scratching starts
	double m_fScratchCenter = 0.;
	double m_fDesiredPosition = 0.;
	double m_fPrevDesiredPosition = 0.;

	float m_fScratchVolumeModifier = 0.f;

	bool m_bScratchAmountSet = false;

	double m_fDesiredScratchRate2 = 0.;

	bool m_bAudioFileHasBeenLoaded = false;

	double m_fTinyScratchAdjust = 0.;

	// new power variables to do sample accurate powering up/down
	//double m_fDesiredPowerRate = 0.;
	//double m_fTinyPowerAdjust = 0.;

	dfx::SpinLock m_AudioFileLock;

	std::atomic_flag mPlayChangedInProcessHasPosted;
};
