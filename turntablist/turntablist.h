/*------------------------------------------------------------------------
Copyright (c) 2004 bioroid media development & Copyright (C) 2004-2021 Sophia Poirier
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

To contact the developer, use the contact form at http://destroyfx.org/
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
#include <mutex>
#include <vector>

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
enum
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

enum : dfx::PropertyID
{
	kTurntablistProperty_Play = dfx::kPluginProperty_EndOfList,
	kTurntablistProperty_AudioFile
};



//------------------------------------------------------------------------------------------
class Turntablist final : public DfxPlugin
{
public:
	Turntablist(TARGET_API_BASE_INSTANCE_TYPE inInstance);

	void dfx_PostConstructor() override;

	long initialize() override;

	void processaudio(float const* const* inAudio, float* const* outAudio, unsigned long inNumFrames) override;
	void processparameters() override;

	OSStatus SaveState(CFPropertyListRef* outData) override;
	OSStatus RestoreState(CFPropertyListRef inData) override;

	OSStatus GetPropertyInfo(AudioUnitPropertyID inPropertyID, 
							 AudioUnitScope inScope, AudioUnitElement inElement, 
							 UInt32& outDataSize, bool& outWritable) override;
	OSStatus GetProperty(AudioUnitPropertyID inPropertyID, 
						 AudioUnitScope inScope, AudioUnitElement inElement, 
						 void* outData) override;
	OSStatus SetProperty(AudioUnitPropertyID inPropertyID, 
						 AudioUnitScope inScope, AudioUnitElement inElement, 
						 void const* inData, UInt32 inDataSize) override;

	OSStatus GetParameterInfo(AudioUnitScope inScope, 
							  AudioUnitParameterID inParameterID, 
							  AudioUnitParameterInfo& outParameterInfo) override;
	OSStatus CopyClumpName(AudioUnitScope inScope, UInt32 inClumpID, 
						   UInt32 inDesiredNameLength, CFStringRef* outClumpName) override;

private:
	OSStatus loadAudioFile(FSRef const& inFileRef);
	OSStatus createAudioFileAlias(AliasHandle* outAlias, Size* outDataSize = nullptr);
	OSStatus resolveAudioFileAlias(AliasHandle const inAlias);

	void processMidiEvent(long inCurrentEvent);
	void processScratch(bool inSetParameter = false);
	void processScratchStop();
	void processPitch();
	void processDirection();
	void calculateSpinSpeeds();
	void setPlay(bool inPlayState, bool inShouldSendNotification = true);

	void noteOn(int inNote, int inVelocity, unsigned long inOffsetFrames);
	void stopNote(bool inStopPlay = false);
	void playNote(bool inValue);
	long fixMidiData(long inParameterID, char inValue);

	OSStatus PostNotification_AudioFileNotFound(CFStringRef inFileName);


	int m_nCurrentNote;
	int m_nCurrentVelocity;
	bool m_bNoteIsOn;

	FSRef m_fsAudioFile;

#ifdef USE_LIBSNDFILE
	std::vector<float> m_fBuffer;
	float* m_fLeft = nullptr;
	float* m_fRight = nullptr;
#else
	ausdk::AUBufferList m_auBufferList;
#endif

	// our 32-bit floating point audio info
	int m_nNumChannels;	// 1=mono, 2=stereo, 0=yomama
	int m_nNumSamples;	// number of samples per channel
	int m_nSampleRate;
	double m_fSampleRate;

	// optional
	double m_fBasePitch;
	double m_fPlaySampleRate;

	// switches
	bool m_bPower;	// on/off
	bool m_bNotePowerTrack;	// scratch mode on/off
	// TODO: use dfx::SmoothedValue for continuous parameters
	double m_fScratchAmount;
	double m_fLastScratchAmount;
#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
	bool m_bMute;	// on/off
#endif
	double m_fPitchShift;
	long m_nDirection;
//	float m_fScratchSpeed;
	double m_fScratchSpeed_scrub, m_fScratchSpeed_spin;

	// modifiers
	long m_nNoteMode;	// reset/resume
	bool m_bLoop;	// on/off
	double m_fPitchRange;
	double m_fSpinDownSpeed;
	double m_fSpinUpSpeed;
	bool m_bKeyTracking;
#ifdef INCLUDE_SILLY_OUTPUT_PARAMETERS
	float m_fVolume;
#endif


	double m_fUsedSpinDownSpeed;
	double m_fUsedSpinUpSpeed;

	bool m_bPlayedReverse;
	int m_nRootKey;

	double m_fPosition;
	double m_fPosOffset;
	double m_fNumSamples;

	bool m_bPlay;

	bool m_bPitchBendSet;
	double m_fPitchBend;
	int m_nScratchDir;
	bool m_bScratching;
	bool m_bWasScratching;
	int m_nWasScratchingDir;
	int m_nScratchDelay;

	double m_fDesiredPitch;

	long m_nScratchMode;
	long m_nScratchSubMode;


	float m_fNoteVolume;	// recalculated on note on and volume changes

	int m_nPowerIntervalEnd;

	bool m_bPlayForward;


	double m_fDesiredScratchRate;
	int m_nScratchInterval;
	int m_nScratchIntervalEnd;
	int m_nScratchIntervalEndBase;
	bool m_bScratchStop;

	int m_nScratchCenter;	// center position where scratching starts
	double m_fScratchCenter;
	double m_fDesiredPosition;
	double m_fPrevDesiredPosition;

	float m_fScratchVolumeModifier;

	bool m_bScratchAmountSet;

	double m_fDesiredScratchRate2;

	bool m_bAudioFileHasBeenLoaded;

	double m_fTinyScratchAdjust;

	// new power variables to do sample accurate powering up/down
//	double m_fDesiredPowerRate;
//	double m_fTinyPowerAdjust;

	std::mutex m_AudioFileLock;
};
