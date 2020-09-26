/*
Copyright (c) 2004 bioroid media development
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer. 
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution. 
* Neither the name of "bioroid media development" nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission. 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Additional changes since June 15th 2004 have been made by Sophia Poirier.  
Copyright (C) 2004 Sophia Poirier
*/
		
	
// 1.1 TO DO:
// 1) fix position based scratching
// how fix
// calculate new desired speed/position
// when processcratch gets called
// 1) missed offset = scratch interval * tiny sample rate
// 2) new offset = blah + missed offsef

// 1.2 TO DO:
// 1) put midi learn back in - remove hardcoded midi cc's
// 2) add settings (midi learn) saving/loading
// 3) add spin mode - sub mode 2


#ifndef __SCRATCHA__
#define __SCRATCHA__

#include "dfxplugin.h"



/////////////////////////////
// DEFINES

const float MAX_PITCH_RANGE = 0.5f; // 50%


//////////////////////////////////////
enum
{
	kScratchDirection_forward = 0,
	kScratchDirection_backward,
	kNumScratchDirections
};

enum
{
	kScratchMode_scrub = 0,
	kScratchMode_spin,
	kNumScratchModes
};

enum
{
	kNoteMode_reset = 0,
	kNoteMode_resume,
	kNumNoteModes
};


// parameters
enum
{
	// scratching
	kScratchAmount = 0,
	kScratchMode,
	kScratchSpeed_scrub,
	kScratchSpeed_spin,

	// turntable power
	kPower,
	kSpinUpSpeed,
	kSpinDownSpeed,
	kNotePowerTrack,

	// pitch
	kPitchShift,
	kPitchRange,
	kKeyTracking,

	// audio sample playback
	kLoop,
	kDirection,
	kNoteMode,

	// audio output
	kMute,
	kVolume,
	kNumParams,

	kPlay   // play/note activity
};

enum
{
	kTurntablistClump_scratching = kAudioUnitClumpID_System + 1,
	kTurntablistClump_power,
	kTurntablistClump_pitch,
	kTurntablistClump_playback,
	kTurntablistClump_output
};

enum
{
	kTurntablistProperty_Play = 93000,
	kTurntablistProperty_AudioFile
};



//------------------------------------------------------------------------------------------
class Scratcha : public DfxPlugin
{
public:
	Scratcha(TARGET_API_BASE_INSTANCE_TYPE inInstance);
	virtual ~Scratcha();

	virtual long initialize();

	virtual void processaudio(const float ** in, float ** out, unsigned long inNumFrames, bool replacing=true);
	virtual void processparameters();

	virtual ComponentResult SaveState(CFPropertyListRef * outData);
	virtual ComponentResult RestoreState(CFPropertyListRef inData);

	virtual ComponentResult GetPropertyInfo(AudioUnitPropertyID inPropertyID, 
					AudioUnitScope inScope, AudioUnitElement inElement, 
					UInt32 & outDataSize, Boolean & outWritable);
	virtual ComponentResult GetProperty(AudioUnitPropertyID inPropertyID, 
					AudioUnitScope inScope, AudioUnitElement inElement, 
					void * outData);
	virtual ComponentResult SetProperty(AudioUnitPropertyID inPropertyID, 
					AudioUnitScope inScope, AudioUnitElement inElement, 
					const void * inData, UInt32 inDataSize);

	virtual ComponentResult GetParameterInfo(AudioUnitScope inScope, 
					AudioUnitParameterID inParameterID, 
					AudioUnitParameterInfo & outParameterInfo);
	virtual CFStringRef CopyClumpName(UInt32 inClumpID);

private:
	OSStatus loadAudioFile(const FSRef & inFile);
	void	processMidiEvent(long currEvent);
	void	processScratch(bool bSetParameter = false);
	void	processScratchStop();
	void	processPitch();
	void	processDirection();
	void	setRootKey();
	void	calculateSpinSpeeds();

	void noteOn(long note, long velocity, long delta);
	void stopNote();
	void PlayNote(bool value);
	long FixMidiData(long param, char value);

	OSStatus PostPropertyChangeNotificationSafely(AudioUnitPropertyID inPropertyID, 
					AudioUnitScope inScope = kAudioUnitScope_Global, AudioUnitElement inElement = 0);


	float fScaler;

	long currentNote;
	long currentVelocity;
	long currentDelta;
	bool noteIsOn;

	FSRef m_fsAudioFile;

	// SCRATCHA STUFF y0!
	float * m_fBuffer;
	float * m_fLeft;
	float * m_fRight;

	// our 32-bit floating point audio info
	int m_nNumChannels;	// 1=mono, 2=stereo, 0=yomama
	int m_nNumSamples;	// number of samples per channel
	int m_nSampleRate;
	float m_fSampleRate;

	// optional
	float m_fFreq;
	float m_fBasePitch;
	float m_fPlaySampleRate;

	// switches
	bool m_bPower; // on/off
	bool m_bNotePowerTrack; // scratch mode on/off
	float m_fScratchAmount;
	float m_fLastScratchAmount;
	bool m_bMute; // on/off
	float m_fPitchShift;
	long m_nDirection;
//	float m_fScratchSpeed;
	float m_fScratchSpeed_scrub, m_fScratchSpeed_spin;

	// modifiers
	long m_nNoteMode; // reset/resume
	bool m_bLoop; // on/off
	float m_fPitchRange;
	float m_fSpinDownSpeed; // ADD ME
	float m_fSpinUpSpeed;  // ADD ME
	float m_bKeyTracking;  // ADD ME
	float m_fRootKey;  // ADD ME
	float m_fVolume;


	float m_fUsedSpinDownSpeed;
	float m_fUsedSpinUpSpeed; 

	bool m_bPlayedReverse;
	long m_nRootKey;

	double m_fPosition;
	float m_fPosOffset;
	float m_fNumSamples;

	bool m_bPlay;

	bool m_bPitchBendSet;
	int m_nPitchBend;
	int m_nScratchDir;
	float m_fScratchRate;
	bool m_bScratching;
	bool m_bWasScratching;
	int m_nWasScratchingDir;
	float m_fPreScratchRate;
	int m_nScratchDelay;

	float m_fDesiredPitch;

	long m_nScratchMode;
	long m_nScratchSubMode;


	float m_fNoteVolume;  // recalculated on note on and volume changes

	int m_nPowerIntervalEnd;

	bool m_bPlayForward;


	bool m_bProcessing;

	float m_fPrevLeft;
	float m_fPrevRight;

	float m_fDesiredScratchRate;
	int m_nScratchInterval;
	int m_nScratchIntervalEnd;
	int m_nScratchIntervalEndBase;
	bool m_bScratchStop;

	int m_nScratchCenter; // center position where scratching starts
	float m_fScratchCenter;
	float m_fDesiredOffset;
	float m_fDesiredPosition;
	float m_fPrevDesiredOffset;
	float m_fPrevDesiredPosition;
	float m_fPrevDesiredScratchRate2;

	float m_fTemporary;

	float m_fPrevScratchAmount;

	float m_fScratchVolumeModifier;

	bool m_bScratchAmountSet;

	float m_fDesiredScratchRate2;

	bool	m_bDataReady;

	float m_fTinyScratchAdjust;

	// new power variables to do sample accurate powering up/down
	float m_fDesiredPowerRate;
	float m_fTinyPowerAdjust;
};


#endif
