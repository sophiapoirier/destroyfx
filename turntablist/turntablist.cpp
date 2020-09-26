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

#include "turntablist.h"

#include <AudioToolbox/AudioToolbox.h>	// for AUEventListenerNotify and AudioFile stuff
//#include <CoreAudio/CoreAudio.h>



/////////////////////////////
// DEFINES

#define USE_LIBSNDFILE

// allow pitch bend scratching
#define _MIDI_PITCH_BEND_
// allow midi cc for controls
#define _MIDI_CC_

const long SCRATCH_INTERVAL = 16; //24
const long POWER_INTERVAL = 120;
const double midiScaler = 1.0 / 127.0;
const long ROOT_KEY = 60;



#ifdef USE_LIBSNDFILE
	#include "sndfile.h"
#endif



//-----------------------------------------------------------------------------------------
// Scratcha
//-----------------------------------------------------------------------------------------

// this macro does boring entry point stuff for us
DFX_ENTRY(Scratcha);

//-----------------------------------------------------------------------------------------
Scratcha::Scratcha(TARGET_API_BASE_INSTANCE_TYPE inInstance)
	: DfxPlugin(inInstance, kNumParams, 0)
{
	initparameter_f(kPitchAmount, "pitch amount", 0.0, 0.0, -50.0, 50.0, kDfxParamUnit_percent);   // (m_fPitchAmount * 2.0f) * (m_fPitchRange * 100.0f)
	initparameter_f(kPitchRange, "pitch range", MAX_PITCH_RANGE * 100.0, MAX_PITCH_RANGE * 100.0, 0.0, MAX_PITCH_RANGE * 100.0, kDfxParamUnit_percent);
	initparameter_f(kVolume, "volume", 1.0, 0.5, 0.0, 1.0, kDfxParamUnit_lineargain);

	initparameter_b(kKeyTracking, "key track", false, false);
	initparameter_b(kLoop, "loop", false, false);
	initparameter_b(kPower, "power", true, true);
	initparameter_b(kNotePowerTrack, "note-power track", false, false);

	initparameter_f(kScratchAmount, "scratch amount", 0.0, 0.0, -1.0, 1.0, kDfxParamUnit_generic);   // float2string(m_fPlaySampleRate, text);
//	initparameter_f(kScratchSpeed, "scratch speed", 0.33333333, 0.33333333, 0.0, 1.0, kDfxParamUnit_scalar);
	initparameter_f(kScratchSpeed_realtime, "scratch speed (realtime mode)", 2.0, 2.0, 0.5, 5.0, kDfxParamUnit_seconds);
	initparameter_f(kScratchSpeed_sequence, "scratch speed (sequence mode)", 3.0, 3.0, 1.0, 8.0, kDfxParamUnit_scalar);
	initparameter_f(kSpinUpSpeed, "spin up speed", 1.0, 1.0, 0.0001, 1.0, kDfxParamUnit_scalar, kDfxParamCurve_log);
	initparameter_f(kSpinDownSpeed, "spin down speed", 1.0, 1.0, 0.0001, 1.0, kDfxParamUnit_scalar, kDfxParamCurve_log);

	initparameter_indexed(kDirection, "playback direction", kScratchDirection_forward, kScratchDirection_forward, kNumScratchDirections);
	setparametervaluestring(kDirection, kScratchDirection_forward, "forward");
	setparametervaluestring(kDirection, kScratchDirection_backward, "reverse");

	initparameter_indexed(kScratchMode, "scratch mode", kScratchMode_realtime, kScratchMode_realtime, kNumScratchModes);
	setparametervaluestring(kScratchMode, kScratchMode_realtime, "realtime");
	setparametervaluestring(kScratchMode, kScratchMode_sequence, "sequence");

	initparameter_indexed(kNoteMode, "note mode", kNoteMode_reset, kNoteMode_reset, kNumNoteModes);
	setparametervaluestring(kNoteMode, kNoteMode_reset, "reset");
	setparametervaluestring(kNoteMode, kNoteMode_resume, "resume");

	initparameter_b(kMute, "mute", false, false);	// XXX this should not be a parameter, right?  or actually with MusicDevices, there is no Bypass property?
//	initparameter_b(kPlay, "play", false, false);   // XXX should "play" be a parameter, in order to allow it to be automated?


	addchannelconfig(0, 2);	// 0-in/2-out

	// SCRATCHA
	m_fBuffer = NULL;
	m_fLeft = NULL;
	m_fRight = NULL;

	m_nNumChannels = 0;
	m_nNumSamples = 0;
	m_nSampleRate = 0;
	

	// modifiers
	m_bPlayedReverse = false;

	m_fPosition = 0.0f;
	m_fPosOffset = 0.0f;
	m_fNumSamples = 0.0f;

	m_fLastScratchAmount = 0.0f;
	m_nPitchBend = 0x2000;
	m_bPitchBendSet = false;
	m_bScratching = false;
	m_bWasScratching = false;
	m_bPlayForward = true;
	m_nScratchSubMode = 1; // speed based from realtime scratch mode

	m_nScratchDelay = 0;
	m_bProcessing = false;

	m_fPrevLeft = 0.0f;
	m_fPrevRight = 0.0f;

	setRootKey();

	currentNote = (m_nRootKey - ROOT_KEY) + m_nRootKey;
	currentVelocity = 127;

	m_bDataReady = false;
	m_bScratchStop = false;

	memset(&m_fsAudioFile, 0, sizeof(m_fsAudioFile));

	m_fTemporary = 0.0f;

	m_fScratchVolumeModifier = 0.0f;

	m_bScratchAmountSet = false;
	m_fDesiredScratchRate = 0.0f;

	m_nScratchInterval = 0;

	m_fDesiredOffset = 0.0f;
	m_fPrevDesiredOffset = 0.0f;
	m_fDesiredPosition = 0.0f;
	m_fPrevDesiredPosition = 0.0f;

	// XXX yah?  for now...
	if (dfxsettings != NULL)
	{
		dfxsettings->setSteal(true);
		dfxsettings->setUseChannel(false);
	}
}

//-----------------------------------------------------------------------------------------
Scratcha::~Scratcha()
{
	if (m_fBuffer != NULL)
		free(m_fBuffer);
	m_fBuffer = NULL;
}


//-----------------------------------------------------------------------------------------
long Scratcha::initialize()
{
	if (sampleratechanged)
	{
		m_nPowerIntervalEnd = (int)getsamplerate() / POWER_INTERVAL;	// set process interval to 10 times a second - should maybe update it to 60?
		m_nScratchIntervalEnd = (int)getsamplerate() / SCRATCH_INTERVAL; // set scratch interval end to 1/16 second
		m_nScratchIntervalEndBase = m_nScratchIntervalEnd;
	}

	m_bPlayedReverse = false;
	stopNote();
	currentDelta = 0;

	return kDfxErr_NoError;
}


//-----------------------------------------------------------------------------------------
void Scratcha::processparameters()
{
	m_bPower = getparameter_b(kPower);
	m_bNotePowerTrack = getparameter_b(kNotePowerTrack);
	m_bLoop = getparameter_b(kLoop);
	m_bKeyTracking = getparameter_b(kKeyTracking);

	m_fPitchAmount = getparameter_scalar(kPitchAmount);
	m_fVolume = getparameter_f(kVolume);
	m_fPitchRange = getparameter_scalar(kPitchRange);

	m_fScratchAmount = getparameter_f(kScratchAmount);
//	m_fScratchSpeed = getparameter_f(kScratchSpeed);
	m_fScratchSpeed_realtime = getparameter_f(kScratchSpeed_realtime);
	m_fScratchSpeed_sequence = getparameter_f(kScratchSpeed_sequence);
	m_fSpinUpSpeed = getparameter_f(kSpinUpSpeed);
	m_fSpinDownSpeed = getparameter_f(kSpinDownSpeed);

	m_nDirection = getparameter_i(kDirection);
	m_nScratchMode = getparameter_i(kScratchMode);
	m_nNoteMode = getparameter_i(kNoteMode);

	m_bMute = getparameter_b(kMute);
//	m_bPlay = getparameter_b(kPlay);


	if (getparameterchanged(kPitchAmount))
		processPitch();

	if (getparameterchanged(kScratchAmount))
	{
		m_bScratchAmountSet = true;
		if (m_fScratchAmount == 0.0f)
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

	m_fNoteVolume = (float)(m_fVolume * (double)currentVelocity * midiScaler);

	if (getparameterchanged(kPitchRange))
		processPitch();

	if (getparameterchanged(kDirection))
		processDirection();

	calculateSpinSpeeds();

//	if (getparameterchanged(kPlay))
//		PlayNote(m_bPlay);
}



#pragma mark -
#pragma mark plugin state

static const CFStringRef kTurntablistPresetAudioFileReferenceKey = CFSTR("audiofile");
static const CFStringRef kTurntablistMidiAssignmentsClassInfoKey = CFSTR("DFX!-midi-assignments");
static const UInt8 kTurntablistAssignment_None = 0xFF;

//-----------------------------------------------------------------------------------------
bool FSRefIsValid(const FSRef & inFileRef)
{
	return ( FSGetCatalogInfo(&inFileRef, kFSCatInfoNone, NULL, NULL, NULL, NULL) == noErr );
} 

//-----------------------------------------------------------------------------------------
ComponentResult Scratcha::SaveState(CFPropertyListRef * outData)
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
				CFDictionaryRef fileReferencesDictionary = CFDictionaryCreate(kCFAllocatorDefault, (const void **)(&kTurntablistPresetAudioFileReferenceKey), (const void **)(&audioFileUrlString), 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
				if (fileReferencesDictionary != NULL)
				{
					CFDictionarySetValue(dict, (const void *)CFSTR(kAUPresetExternalFileRefs), (const void *)fileReferencesDictionary);
					CFRelease(fileReferencesDictionary);
				}
				CFRelease(audioFileUrlString);
			}
			CFRelease(audioFileUrl);
		}
	}

// save the MIDI CC -> parameter assignments
	if (dfxsettings != NULL)
	{
		UInt8 assignmentsToStore[kNumParams];
		bool assignmentsFound = false;
		for (int i=0; i < kNumParams; i++)
		{
			if (dfxsettings->getParameterAssignmentType(i) == kParamEventCC)
			{
				assignmentsToStore[i] = (unsigned char) (dfxsettings->getParameterAssignmentNum(i));
				assignmentsFound = true;
			}
			else
				assignmentsToStore[i] = kTurntablistAssignment_None;
		}
		if (assignmentsFound)
		{
			CFDataRef assignmentsCFData = CFDataCreate(kCFAllocatorDefault, (const UInt8*)assignmentsToStore, (CFIndex)sizeof(assignmentsToStore));
			if (assignmentsCFData != NULL)
			{
				CFDictionarySetValue(dict, kTurntablistMidiAssignmentsClassInfoKey, assignmentsCFData);
				CFRelease(assignmentsCFData);
			}
		}
	}

	return noErr;
}

//-----------------------------------------------------------------------------------------
ComponentResult Scratcha::RestoreState(CFPropertyListRef inData)
{
	ComponentResult result = AUBase::RestoreState(inData);
	if (result != noErr)
		return result;

	CFDictionaryRef dict = static_cast<CFDictionaryRef>(inData);

// restore the previously loaded audio file
	CFDictionaryRef fileReferencesDictionary = reinterpret_cast<CFDictionaryRef>( CFDictionaryGetValue(dict, CFSTR(kAUPresetExternalFileRefs)) );
	if (fileReferencesDictionary != NULL)
	{
		CFStringRef audioFileUrlString = reinterpret_cast<CFStringRef>( CFDictionaryGetValue(fileReferencesDictionary, kTurntablistPresetAudioFileReferenceKey) );
		if (audioFileUrlString != NULL)
		{
			CFURLRef audioFileUrl = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, audioFileUrlString, kCFURLPOSIXPathStyle, false);
			if (audioFileUrl != NULL)
			{
				FSRef audioFileRef;
				Boolean gotFileRef = CFURLGetFSRef(audioFileUrl, &audioFileRef);
				CFRelease(audioFileUrl);
				if (gotFileRef)
					loadAudioFile(audioFileRef);
			}
		}
	}

// restore the MIDI CC -> parameter assignments
	if (dfxsettings != NULL)
	{
		CFDataRef assignmentsCFData = reinterpret_cast<CFDataRef>( CFDictionaryGetValue(dict, kTurntablistMidiAssignmentsClassInfoKey) );
		if (assignmentsCFData != NULL)
		{
			CFIndex dataSize = CFDataGetLength(assignmentsCFData);
			const UInt8 * assignments = CFDataGetBytePtr(assignmentsCFData);
			if ( (assignments != NULL) && (dataSize > 0) )
			{
				for (int i=0; (i < kNumParams) && (i < dataSize); i++)
				{
					if (assignments[i] == kTurntablistAssignment_None)
						dfxsettings->unassignParam(i);
					else
						dfxsettings->assignParam(i, kParamEventCC, 0, assignments[i]);
				}
			}
		}
	}

	return noErr;
}

//-----------------------------------------------------------------------------
ComponentResult Scratcha::GetPropertyInfo(AudioUnitPropertyID inPropertyID, 
					AudioUnitScope inScope, AudioUnitElement inElement, 
					UInt32 & outDataSize, Boolean & outWritable)
{
	ComponentResult result = noErr;

	switch (inPropertyID)
	{
		case kAudioUnitProperty_ParameterClumpName:
			outDataSize = sizeof(AudioUnitParameterNameInfo);
			outWritable = false;
			return noErr;

		case kTurntablistProperty_Play:
			outDataSize = sizeof(m_bPlay);
			outWritable = true;
			break;

		case kTurntablistProperty_AudioFile:
			outDataSize = sizeof(m_fsAudioFile);
			outWritable = true;
			break;

		default:
			result = DfxPlugin::GetPropertyInfo(inPropertyID, inScope, inElement, outDataSize, outWritable);
			break;
	}

	return result;
}

//-----------------------------------------------------------------------------
ComponentResult Scratcha::GetProperty(AudioUnitPropertyID inPropertyID, 
					AudioUnitScope inScope, AudioUnitElement inElement, 
					void * outData)
{
	ComponentResult result = noErr;

	switch (inPropertyID)
	{
		case kTurntablistProperty_Play:
			*(bool*)outData = m_bPlay;
			break;

		case kTurntablistProperty_AudioFile:
			if ( FSRefIsValid(m_fsAudioFile) )
				*(FSRef*)outData = m_fsAudioFile;
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
ComponentResult Scratcha::SetProperty(AudioUnitPropertyID inPropertyID, 
					AudioUnitScope inScope, AudioUnitElement inElement, 
					const void * inData, UInt32 inDataSize)
{
	ComponentResult result = noErr;

	switch (inPropertyID)
	{
		case kTurntablistProperty_Play:
			m_bPlay = *(bool*)inData;
			PlayNote(m_bPlay);
			break;

		case kTurntablistProperty_AudioFile:
			result = loadAudioFile(*(FSRef*)inData);
			break;

		default:
			result = DfxPlugin::SetProperty(inPropertyID, inScope, inElement, inData, inDataSize);
			break;
	}

	return result;
}

//-----------------------------------------------------------------------------------------
OSStatus Scratcha::PostPropertyChangeNotificationSafely(AudioUnitPropertyID inPropertyID, AudioUnitScope inScope, AudioUnitElement inElement)
{
	if (AUEventListenerNotify != NULL)
	{
		AudioUnitEvent auEvent;
		memset(&auEvent, 0, sizeof(auEvent));
		auEvent.mEventType = kAudioUnitEvent_PropertyChange;
		auEvent.mArgument.mProperty.mAudioUnit = GetComponentInstance();
		auEvent.mArgument.mProperty.mPropertyID = inPropertyID;
		auEvent.mArgument.mProperty.mScope = inScope;
		auEvent.mArgument.mProperty.mElement = inElement;
		return AUEventListenerNotify(NULL, NULL, &auEvent);
	}
	else
	{
		PropertyChanged(inPropertyID, inScope, inElement);
		return noErr;
	}
}

//-----------------------------------------------------------------------------------------
ComponentResult Scratcha::GetParameterInfo(AudioUnitScope inScope, 
							AudioUnitParameterID inParameterID, 
							AudioUnitParameterInfo & outParameterInfo)
{
	ComponentResult result = DfxPlugin::GetParameterInfo(inScope, inParameterID, outParameterInfo);
	if (result != noErr)
		return result;

	outParameterInfo.flags |= kAudioUnitParameterFlag_HasClump;
	switch (inParameterID)
	{
		case kScratchAmount:
		case kScratchMode:
		case kScratchSpeed_realtime:
		case kScratchSpeed_sequence:
			outParameterInfo.clumpID = kTurntablistClump_scratching;
			break;

		case kPower:
		case kSpinUpSpeed:
		case kSpinDownSpeed:
		case kNotePowerTrack:
			outParameterInfo.clumpID = kTurntablistClump_power;
			break;

		case kPitchAmount:
		case kPitchRange:
		case kKeyTracking:
			outParameterInfo.clumpID = kTurntablistClump_pitch;
			break;

		case kLoop:
		case kDirection:
		case kNoteMode:
			outParameterInfo.clumpID = kTurntablistClump_playback;
			break;

		case kMute:
		case kVolume:
			outParameterInfo.clumpID = kTurntablistClump_output;
			break;

		default:
			outParameterInfo.flags &= ~kAudioUnitParameterFlag_HasClump;
			break;
	}

	return noErr;
}

//-----------------------------------------------------------------------------------------
CFStringRef Scratcha::CopyClumpName(UInt32 inClumpID)
{
	CFStringRef clumpName = NULL;
	switch (inClumpID)
	{
		case kTurntablistClump_scratching:
			clumpName = CFSTR("scratching");
			break;
		case kTurntablistClump_power:
			clumpName = CFSTR("turntable power");
			break;
		case kTurntablistClump_pitch:
			clumpName = CFSTR("pitch");
			break;
		case kTurntablistClump_playback:
			clumpName = CFSTR("audio sample playback");
			break;
		case kTurntablistClump_output:
			clumpName = CFSTR("audio output");
			break;
		default:
			break;
	}

	if (clumpName != NULL)
		return CFStringCreateCopy(kCFAllocatorDefault, clumpName);
	else
		return NULL;
}



#pragma mark -
#pragma mark audio processing

//-----------------------------------------------------------------------------------------
OSStatus Scratcha::loadAudioFile(const FSRef & inFile)
{
	UInt8 file[2048];
	memset(file, 0, sizeof(file));
	OSStatus status = FSRefMakePath(&inFile, file, sizeof(file));
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
		char  buffer[256] ;
		memset(buffer, 0, sizeof(buffer));
		sf_error_str(sndFile, buffer, sizeof(buffer));
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

	m_bDataReady = false;
	sf_command(sndFile, SFC_SET_NORM_FLOAT, NULL, SF_TRUE) ;

	m_nNumChannels = (int)sfInfo.channels;
	m_nSampleRate = (int)sfInfo.samplerate;
	m_nNumSamples  = (int)sfInfo.frames;

	m_fPlaySampleRate = (float) m_nSampleRate;
	m_fSampleRate = (float) m_nSampleRate;
	calculateSpinSpeeds();
	m_fPosition = 0.0f;
	m_fPosOffset = 0.0f;
	m_fNumSamples = (float)m_nNumSamples;

	if (m_fBuffer != NULL)
	{
		free(m_fBuffer);
		m_fBuffer = NULL;
		m_fLeft = NULL;
		m_fRight = NULL;
	}

	m_fBuffer = (float*) malloc(m_nNumChannels * m_nNumSamples * sizeof(float));
	float * tempBuffer = (float*) malloc(m_nNumSamples * sizeof(float));

	m_fLeft = (float*)&m_fBuffer[0];
	m_fRight = (float*)&m_fBuffer[m_nNumSamples * (m_nNumChannels-1)];

	// do file loading here!!
	sf_count_t sizein, sizeout;
	sizein = sfInfo.frames * sfInfo.channels;
	sizeout = sf_read_float(sndFile, &m_fBuffer[0], sizein);
	if (sizeout != sizein)
	{
		// error?
		fprintf(stderr, "sizein: %d, sizeout: %d\n", sizein, sizeout);
	}

	if (m_nNumChannels == 2)
	{
		if (tempBuffer != NULL)
		{
			// do swaps
			for (int z=0; z < m_nNumSamples; z++)
			{
				m_fBuffer[z] = 0.0f;
				tempBuffer[z] = m_fBuffer[(z*2)+1]; // copy channel 1 into buffer
				m_fBuffer[z] = m_fBuffer[z*2]; // handle channel 0
			}

			for (int z=0; z < m_nNumSamples; z++)
				m_fBuffer[m_nNumSamples+z] = tempBuffer[z]; // make channel 1
		}
	}

	if (tempBuffer != NULL)
		free(tempBuffer);
	tempBuffer = NULL;

	// end file loading here!!

	if (sf_close(sndFile) != 0)
	{
		// error closing
	}

	sndFile = NULL;

	m_fsAudioFile = inFile;

	processPitch(); // set up stuff
	m_bDataReady = true;	// ready to play

	PostPropertyChangeNotificationSafely(kTurntablistProperty_AudioFile);

	return noErr;
}

//-----------------------------------------------------------------------------------------
void Scratcha::calculateSpinSpeeds()
{
//	m_fUsedSpinUpSpeed = (((exp(10.0*m_fSpinUpSpeed)-1.0)/(exp(10.0)-1.0)) * (double)m_nSampleRate) / (double)m_nPowerIntervalEnd;
//	m_fUsedSpinDownSpeed = (((exp(10.0*m_fSpinDownSpeed)-1.0)/(exp(10.0)-1.0)) * (double)m_nSampleRate) / (double)m_nPowerIntervalEnd;
	m_fUsedSpinUpSpeed = m_fSpinUpSpeed * (double)m_nSampleRate / (double)m_nPowerIntervalEnd;
	m_fUsedSpinDownSpeed = m_fSpinDownSpeed * (double)m_nSampleRate / (double)m_nPowerIntervalEnd;
}

//-----------------------------------------------------------------------------------------
void Scratcha::processaudio(const float ** inStreams, float ** outStreams, unsigned long inNumFrames, bool replacing)
{
	m_bProcessing = true;

	float * out1 = outStreams[0];
	float * out2 = outStreams[1];

	long eventFrame = -1; // -1 = no events
	long numEvents = midistuff->numBlockEvents;
	long currEvent = 0;


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
				if (m_nScratchMode == kScratchMode_sequence)
					processScratch();
				else
					processScratchStop();
			}
			else
			{
				if (m_nScratchMode == kScratchMode_realtime)
				{
					// nudge samplerate
					m_fPlaySampleRate += m_fTinyScratchAdjust;
					// speed mode
					if (m_fTinyScratchAdjust > 0.0f)	// positive
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
			if (!m_bPower) // power off - spin down
			{
				if (m_fPlaySampleRate > 0.0f) // too high, spin down
				{
					m_fPlaySampleRate -= m_fUsedSpinDownSpeed; // adjust
					if (m_fPlaySampleRate < 0.0f) // too low so we past it
						m_fPlaySampleRate = 0.0f; // fix it
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
		if (noteIsOn)
		{
			// adjust gnPosition for new sample rate

			// set pos offset based on current sample rate

			// safey check
			if (m_fPlaySampleRate <= 0.0f)
			{
				m_fPlaySampleRate = 0.0f;
				if (m_bNotePowerTrack)
				{
					if (!m_bScratching)
					{
						stopNote();
						if (m_nNoteMode == kNoteMode_reset)
							m_fPosition = 0.0f;
						m_bPlay = false;
						m_bPlayedReverse = false;
					}
				}
			}

			m_fPosOffset = m_fPlaySampleRate/getsamplerate_f();  //m_fPlaySampleRate/m_fSampleRate;

			if (m_bDataReady)
			{
				if (noteIsOn)
				{

					if (!m_bPlayForward) // if play direction = reverse
					{
						if (m_fPlaySampleRate != 0.0f)
						{
							m_fPosition -= m_fPosOffset;
							while (m_fPosition < 0) // was if
							{									
								if (!m_bLoop) // off
								{
									if (m_bPlayedReverse)
									{
										stopNote();
										m_fPosition = 0.0f;
										m_bPlayedReverse = false;

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

					if (!m_bMute)   // if audio on
					{		
//#define _NO_INTERPOLATION_
//#define _LINEAR_INTERPOLATION_
#define _CUBIC_INTERPOLATION_
						// no interpolation start
#ifdef _NO_INTERPOLATION_
						float fLeft = m_fLeft[(long)m_fPosition];
						float fRight = m_fRight[(long)m_fPosition];
						//no interpolation end
#endif _NO_INTERPOLATION_

						//linear interpolation start
#ifdef _LINEAR_INTERPOLATION_						
						float floating_part = (float)(m_fPosition - (long)m_fPosition);
						long big_part1 = (long)m_fPosition;
						long big_part2 = big_part1+1;
						if (big_part2 > m_nNumSamples)
							big_part2 = 0;
						float fLeft = (floating_part * m_fLeft[big_part1]) + ((1-floating_part) * m_fLeft[big_part2]);
						float fRight = (floating_part * m_fRight[big_part1]) + ((1-floating_part) * m_fRight[big_part2]);
#endif _LINEAR_INTERPOLATION_
						//linear interpolation end

#ifdef _CUBIC_INTERPOLATION_
						//cubic interpolation start		
						long inpos = (long) m_fPosition;
						float finpos = (float)(m_fPosition - (long)(m_fPosition));
						//float xm1, x0, x1, x2;
						float a, b, c;
						float xarray[4]; //0 = xm1, 1 = x0, 2 = x1, 3 = x2
						int posarray[4];
						posarray[0] = inpos - 1;
						posarray[1] = inpos;
						posarray[2] = inpos + 1;
						posarray[3] = inpos + 2;
						for (int pos = 0; pos < 4; pos++)
						{
							if (!m_bLoop)   // off - set to -1/0
							{
								if ( (posarray[pos] < 0) || (posarray[pos] >= m_nNumSamples) )
									posarray[pos] = -1;
							}
							else	// on - set to new pos
							{
								if (posarray[pos] < 0)
									posarray[pos] += m_nNumSamples;
								else if (posarray[pos] >= m_nNumSamples)
									posarray[pos] -= m_nNumSamples;
							}
						}
						// left channel
						for (int pos2 = 0; pos2 < 4; pos2++)
						{
							if (posarray[pos2] == -1)
								xarray[pos2] = 0.0f;
							else
								xarray[pos2] = m_fLeft[posarray[pos2]];
						}
						a = (3 * (xarray[1]-xarray[2]) - xarray[0] + xarray[3]) / 2;
						b = 2*xarray[2] + xarray[0] - (5*xarray[1] + xarray[2]) / 2;
						c = (xarray[2] - xarray[0]) / 2;
						float fLeft = (((a * finpos) + b) * finpos + c) * finpos + xarray[1];

						// right channel
						for (int pos3 = 0; pos3 < 4; pos3++)
						{
							if (posarray[pos3] == -1)
								xarray[pos3] = 0.0f;
							else
								xarray[pos3] = m_fRight[posarray[pos3]];
						}
						a = (3 * (xarray[1]-xarray[2]) - xarray[0] + xarray[3]) / 2;
						b = 2*xarray[2] + xarray[0] - (5*xarray[1] + xarray[2]) / 2;
						c = (xarray[2] - xarray[0]) / 2;
						float fRight = (((a * finpos) + b) * finpos + c) * finpos + xarray[1];
#endif _CUBIC_INTERPOLATION_
						/*
						Name: Cubic interpollation
						Type: Cubic Hermite interpolation
						References: posted by Olli Niemitalo
						Notes: 
						finpos is the fractional, inpos the integer part.
						Allso see other001.gif 

						xm1 = x [inpos - 1];
						x0  = x [inpos + 0];
						x1  = x [inpos + 1];
						x2  = x [inpos + 2];
						a = (3 * (x0-x1) - xm1 + x2) / 2;
						b = 2*x1 + xm1 - (5*x0 + x2) / 2;
						c = (x1 - xm1) / 2;
						y [outpos] = (((a * finpos) + b) * finpos + c) * finpos + x0;
						*/
						//m_nNumSamples
						//cubic interpolation end



						if (m_fPlaySampleRate == 0.0f)
						{
							out1[currFrame] = 0.0f;
							out2[currFrame] = 0.0f;
						}
						else
						{
							out1[currFrame] = fLeft * m_fNoteVolume;
							out2[currFrame] = fRight * m_fNoteVolume;
						}
					}   // if (!m_bMute)
					else
						out1[currFrame] = out2[currFrame] = 0.0f;

					
					if (m_bPlayForward)	// if play direction = forward
					{
						m_bPlayedReverse = false;
						if (m_fPlaySampleRate != 0.0f)
						{
							m_fPosition += m_fPosOffset;

							while (m_fPosition > m_fNumSamples)
							{
								m_fPosition -= m_fNumSamples;
								if (!m_bLoop) // off
								{
									stopNote();
									m_bPlayedReverse = false;
								}

							}
						}
					}   // if (bPlayForward)

				}   // if (noteIsOn)
				else
					out1[currFrame] = out2[currFrame] = 0.0f;

			}   // if (m_bDataReady)
			else
				out1[currFrame] = out2[currFrame] = 0.0f;
		}   // if (noteIsOn)
		else
			out1[currFrame] = out2[currFrame] = 0.0f;
	}   // (currFrame < inNumFrames)

	m_bProcessing = false;
}



#pragma mark -
#pragma mark scratch processing

//-----------------------------------------------------------------------------------------
void Scratcha::processScratchStop()
{
	m_nScratchDir = kScratchDirection_forward;
	m_fPlaySampleRate = 0.0f;
	if (!m_bScratchStop)
	{
		m_bScratchStop = true;
//		m_nScratchInterval = m_nScratchIntervalEnd / 2;
	}
	else
	{
		// TO DO:
		// 1. set the position to where it should be to adjust for errors

		m_fPlaySampleRate = 0.0f;
		m_fDesiredScratchRate = 0.0f;
		m_fDesiredScratchRate2 = 0.0f;
		m_fTinyScratchAdjust = 0.0f;
		m_nScratchInterval = 0;
	}

	processDirection();
}

//-----------------------------------------------------------------------------------------
void Scratcha::processScratch(bool bSetParameter)
{
	int bLogSetParameter;
	float fIntervalScaler = 0.0f;
	
	if (bSetParameter)
		bLogSetParameter = 1;
	else
		bLogSetParameter = 0;

	float fLogDiff;

	if (m_bPitchBendSet)
	{
		// set scratch amount to scaled pitchbend
		float fPitchBend = (float)m_nPitchBend / 16383.0f;	// was 16384
		fPitchBend = expandparametervalue_index(kScratchAmount, fPitchBend);
		if (bSetParameter)
			setparameter_f(kScratchAmount, fPitchBend);
			// XXX post notification?
		else
			m_fScratchAmount = fPitchBend;
		m_bScratchAmountSet = true;

		// XXX todo fix pitchbend
		/*
<agoz4> i noticed that with pitch bend at maximum , value is not 1.000000
<agoz4> but 0.9999
<agoz4> division by 128/ 127 something like that?
<agoz4> ok, now that i moved the fader by mouse instead of using pitch wheel, 
		it doesn't reset to full speed at half fader
		*/

		if (m_fScratchAmount > 1.0f)
			m_fScratchAmount = 1.0f;
		if (m_fScratchAmount < -1.0f)
			m_fScratchAmount = -1.0f;
		m_bPitchBendSet = false;
	}


	if (m_bScratching) // scratching
	{
		if (m_nScratchMode == kScratchMode_sequence)
		{
			if (m_fScratchAmount == 0.0f)
				m_nScratchDir = kScratchDirection_forward;
			else if (m_fScratchAmount > 0.0f)
				m_nScratchDir = kScratchDirection_forward;
			else
				m_nScratchDir = kScratchDirection_backward;
		}

	// todo:
	// handle scratching like power
	// scratching will set target sample rate and system will catch up based on scratch speed parameter
	// sort of like spin up/down speed param..

	// if curr rate below target rate then add samplespeed2
	// it gone over then set to target rate
	// if curr rate above target rate then sub samplespeed2
	// if gone below then set to target rate


		// calculate hand size
		float fScaler = m_fScratchSpeed_sequence;

		// set target sample rate
		m_fDesiredScratchRate = fabsf(m_fScratchAmount * fScaler * m_fBasePitch);
		
		if (m_nScratchMode == kScratchMode_sequence)	// mode 2
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
			// use torue to set the spin down time
			// and make the torque speed be samplerate/spindown time

			if (m_nScratchSubMode == 0)
			{
// POSITION BASED:
				if (m_bScratchAmountSet)
				{
					m_bScratchStop = false;

					fScaler = 1.0f;
					fIntervalScaler = ((float)m_nScratchInterval/(float)m_nScratchIntervalEnd) + 1.0f;

// XXX m_fScratchAmount here is normalized?  should do contractparametervalue_index(kScratchAmount, m_fScratchAmount) ?
					m_fDesiredPosition = m_fScratchCenter + (m_fScratchAmount * m_fScratchSpeed_realtime * m_nSampleRate);

					m_fTemporary = m_fDesiredPosition;

					float fDesiredDelta;
					
					if (m_nScratchInterval == 0)
					{
						m_fPosition = m_fDesiredPosition;
						m_fTinyScratchAdjust = 0;
					}
					else
					{
						fDesiredDelta = (m_fDesiredPosition - m_fPosition); //m_nScratchInterval;
						m_fTinyScratchAdjust = (fDesiredDelta - m_fPlaySampleRate)/m_nSampleRate;						
					}

					


					



					// do something with desireddelta and scratchinterval

					// figure out direction
					float fDiff = m_fScratchAmount - m_fLastScratchAmount;

					if (fDiff < 0.0)
					{
						fDiff = -fDiff;
						m_nScratchDir = kScratchDirection_backward;
					}
					else
						m_nScratchDir = kScratchDirection_forward;

					m_fDesiredScratchRate2 = m_nSampleRate * m_fScratchSpeed_realtime * m_nScratchInterval;

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
					


					m_fDesiredScratchRate2 = (m_fDesiredScratchRate2+m_fPlaySampleRate)/2;

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

					float fDiff = m_fScratchAmount - m_fLastScratchAmount;
					fLogDiff = fDiff;

					if (fDiff < 0.0)
					{
						fDiff = -fDiff;
						m_nScratchDir = kScratchDirection_backward;
					}
					else
					{
						m_nScratchDir = kScratchDirection_forward;
					}


					//new
					fScaler = 1.0f;
					fIntervalScaler = ((float)m_nScratchInterval/(float)m_nScratchIntervalEnd) + 1.0f;

					m_fDesiredScratchRate2 = (fDiff * (float)m_nSampleRate * m_fScratchSpeed_realtime*(float)SCRATCH_INTERVAL) / fIntervalScaler;

					m_fTinyScratchAdjust = (m_fDesiredScratchRate2 - m_fPlaySampleRate)/m_nScratchIntervalEnd; 


					m_fPlaySampleRate += m_fTinyScratchAdjust;
					m_bScratchAmountSet = false;
				}
			}

			m_fLastScratchAmount = m_fScratchAmount;
		}
		m_fScratchVolumeModifier = (m_fPlaySampleRate/m_fSampleRate);
		if (m_fScratchVolumeModifier > 1.5f)
			m_fScratchVolumeModifier = 1.5f;

		if (!m_bScratching)
			m_bScratching = true;

		if (m_bScratchStop)
		{
			m_fPlaySampleRate = 0.0f;
			m_fDesiredScratchRate = 0.0f;
			m_fDesiredScratchRate2 = 0.0f;
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
void Scratcha::processPitch()
{
	float temp = m_fDesiredPitch;
	if (m_bKeyTracking)
	{
		int note2play = currentNote - m_nRootKey;
		m_fBasePitch = ((double)m_nSampleRate) * pow(2.0, ((double)note2play)/12.0);
		float pitchAdjust = m_fPitchAmount * m_fPitchRange;
		m_fDesiredPitch = m_fBasePitch + (m_fBasePitch * pitchAdjust);
	}
	else
	{
		m_fBasePitch = (float)m_nSampleRate;
		float pitchAdjust = m_fPitchAmount * m_fPitchRange;
		m_fDesiredPitch = m_fBasePitch + (m_fBasePitch * pitchAdjust);
	}

	if (temp == m_fPlaySampleRate)
		m_fPlaySampleRate = m_fDesiredPitch; // do instant adjustment
}

//-----------------------------------------------------------------------------------------
void Scratcha::processDirection()
{
	m_bPlayForward = true;

	if (m_bScratching)
	{
		if (m_nScratchDir == kScratchDirection_backward)
			m_bPlayForward = false;
	}
	else
	{
		if (m_bWasScratching)
		{
			if (m_nDirection == kScratchDirection_backward)
				m_bPlayForward = false;
		}
		else
		{
			if (m_nDirection == kScratchDirection_backward)
				m_bPlayForward = false;
		}
	}
}



#pragma mark -
#pragma mark MIDI processing

//-----------------------------------------------------------------------------------------
void Scratcha::processMidiEvent(long currEvent)
{
	DfxMidiEvent event = midistuff->blockEvents[currEvent];

	if ( isNote(event.status) )	// we only look at notes
	{
		long note = event.byte1;
		long velocity = event.byte2;
		if (event.status == kMidiNoteOff)
			velocity = 0;

		noteOn(note, velocity, event.delta); // handle note on or off
	}
#ifdef _MIDI_CC_
	else if (event.status == kMidiCC) // controller change
	{
		if (event.byte1 == kMidiCC_AllNotesOff)	// all notes off
		{
			stopNote();
			if (m_nNoteMode == kNoteMode_reset)
				m_fPosition = 0.0f;
			m_bPlayedReverse = false;
		}

		if ( (event.byte1 >= 64) && (event.byte1 <= (64 + kNumParams - 1)) )
		{
			long param = event.byte1 - 64;
			long new_data = FixMidiData(param, event.byte2);
			float value = ((float)new_data)/127.f;
			setparameter_f(param, value);
			postupdate_parameter(param);
		}
	}
#endif
#ifdef _MIDI_PITCH_BEND_
	else if (event.status == kMidiPitchbend) // pitch bend
	{
		// handle pitch bend here
		unsigned short pb14bit;
		pb14bit = (unsigned short)event.byte2; 
		pb14bit <<= 7;
		pb14bit |= (unsigned short)event.byte1;
		m_nPitchBend = pb14bit;
		m_bPitchBendSet = true;

		float fPitchBend;
		if (m_nPitchBend == 0x2000)
			fPitchBend = 0.5f;
		else
		{
			fPitchBend = (float)m_nPitchBend / 16383.0f;
			if (fPitchBend > 1.0f)
				fPitchBend = 1.0f;
			else if (fPitchBend < 0.0f)
				fPitchBend = 0.0f;	
		}
	
		fPitchBend = expandparametervalue_index(kScratchAmount, fPitchBend);
		setparameter_f(kScratchAmount, fPitchBend);
		// XXX post notification?
	}
#endif
}

//-----------------------------------------------------------------------------------------
void Scratcha::setRootKey()
{
	m_nRootKey = (int)(0.5f * 127.f);
}

//-----------------------------------------------------------------------------------------
void Scratcha::noteOn(long note, long velocity, long delta)
{
	currentNote = note;
	currentVelocity = velocity;
	currentDelta = delta;
	if (velocity == 0)
	{
		if (m_bNotePowerTrack)
		{
			setparameter_b(kPower, false);
			// XXX post notification?
			noteIsOn = true;
			m_bPlay = false;
		}
		else
		{
			noteIsOn = false;
			m_bPlay = false;
			if (m_nNoteMode == kNoteMode_reset)
				m_fPosition = 0.0f;
			m_bPlayedReverse = false;
		}
	}
	else
	{
		noteIsOn = true;
		m_bPlay = true;

		if (m_nNoteMode == kNoteMode_reset)
			m_fPosition = 0.0f;
		// calculate note volume
		m_fNoteVolume = (float)(m_fVolume * (double)currentVelocity * midiScaler);
		//44100*(2^(0/12)) = C-3
		processPitch();

		if (m_bNotePowerTrack)
		{
			setparameter_b(kPower, false);
			// XXX post notification?
		}
	}

	PostPropertyChangeNotificationSafely(kTurntablistProperty_Play);
}

//-----------------------------------------------------------------------------------------
void Scratcha::stopNote()
{
	bool play_old = m_bPlay;
	m_bPlay = false;
	noteIsOn = false;
	if (m_bPlay != play_old)
		PostPropertyChangeNotificationSafely(kTurntablistProperty_Play);
}

//-----------------------------------------------------------------------------------------
void Scratcha::PlayNote(bool value)
{
	if (!value)
		noteOn(currentNote, 0, 0); // note off
	else
	{
		//currentNote - m_nRootKey = ?
		// - 64       - 0          = -64
		// 64          - 64         = 0;
		// 190           - 127        = 63;
		currentNote = (m_nRootKey - ROOT_KEY) + m_nRootKey;
		noteOn(currentNote, 127, 0); // note on
	}
}

//-----------------------------------------------------------------------------------------
long Scratcha::FixMidiData(long param, char value)
{
	switch (param)
	{
		case kPower:
		case kNotePowerTrack:
		case kMute:
		case kPlay:
		case kNoteMode:
		case kDirection:
		case kScratchMode:
		case kLoop:
		case kKeyTracking:
			// <64 = 0ff, >=64 = 0n
			if (value < 64)
				return 0;
			else
				return 127;
			break;
		default:
			break;
	}

	return value;
}
