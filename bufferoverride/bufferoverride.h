/*------------------------------------------------------------------------
Copyright (C) 2001-2016  Sophia Poirier

This file is part of Buffer Override.

Buffer Override is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Buffer Override is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Buffer Override.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#pragma once


#include "dfxplugin.h"
#include "dfxsmoothedvalue.h"
#include "lfo.h"


//----------------------------------------------------------------------------- 
// these are the plugin parameters:
enum
{
	kDivisor,
	kBufferSize_abs,
	kBufferSize_sync,
	kBufferTempoSync,
	kBufferInterrupt,

	kDivisorLFOrate_abs,
	kDivisorLFOrate_sync,
	kDivisorLFOdepth,
	kDivisorLFOshape,
	kDivisorLFOtempoSync,
	kBufferLFOrate_abs,
	kBufferLFOrate_sync,
	kBufferLFOdepth,
	kBufferLFOshape,
	kBufferLFOtempoSync,

	kSmooth,
	kDryWetMix,

	kPitchbend,
	kMidiMode,

	kTempo,
	kTempoAuto,

	kNumParameters
};

//----------------------------------------------------------------------------- 
// constants

const long kNumPresets = 16;

// We need this stuff to get some maximum buffer size and allocate for that.
// This is 42 bpm, which should be sufficient.
const double kMinAllowableBPS = 0.7;

enum {
	kMidiMode_nudge,
	kMidiMode_trigger,
	kNumMidiModes
};


//----------------------------------------------------------------------------- 
class BufferOverride : public DfxPlugin
{
public:
	BufferOverride(TARGET_API_BASE_INSTANCE_TYPE inInstance);
	virtual ~BufferOverride();

	virtual long initialize();
	virtual void cleanup();
	virtual void reset();

	virtual void processaudio(const float ** in, float ** out, unsigned long inNumFrames, bool replacing = true);
	virtual void processparameters();

	virtual bool createbuffers();
	virtual void releasebuffers();


private:
	void updateBuffer(unsigned long samplePos);

	void heedBufferOverrideEvents(unsigned long samplePos);
	float getDivisorParameterFromNote(int currentNote);
	float getDivisorParameterFromPitchbend(int pitchbendByte);

	void initPresets();

	// the parameters
	float divisor, bufferSizeMs, bufferSizeSync;
	bool bufferTempoSync, bufferInterrupt, useHostTempo;
	float smooth, dryWetMix;
	double pitchbendRange, userTempo;
	long midiMode;

	DfxSmoothedValue<float> inputGain, outputGain;  // the effective states of the dry/wet mix

	long currentForcedBufferSize;	// the size of the larger, imposed buffer
	float ** buffers;	// this stores the forced buffer
	float * outval;	// array of current audio output values (1 for each channel)
	unsigned long numBuffers;	// how many buffers we have allocated at the moment
	long writePos;	// the current sample position within the forced buffer

	long minibufferSize;	// the current size of the divided "mini" buffer
	long prevMinibufferSize;	// the previous size
	long readPos;	// the current sample position within the minibuffer
	float currentBufferDivisor;	// the current value of the divisor with LFO possibly applied

	float numLFOpointsDivSR;	// the number of LFO table points divided by the sampling rate

	double currentTempoBPS;	// tempo in beats per second
	bool needResync;

	long SUPER_MAX_BUFFER;

	long smoothDur, smoothcount;	// total duration and sample counter for the minibuffer transition smoothing period
	float smoothStep;	// the gain increment for each sample "step" during the smoothing period
	float sqrtFadeIn, sqrtFadeOut;	// square root of the smoothing gains, for equal power crossfading
	float smoothFract;

	double pitchbend, oldPitchbend;	// pitchbending scalar values
	bool oldNote;	// says if there was an old, unnatended note-on or note-off from a previous block
	int lastNoteOn, lastPitchbend;	// these carry over the last events from a previous processing block
	bool divisorWasChangedByHand;	// for MIDI trigger mode - tells us to respect the fDivisor value
	bool divisorWasChangedByMIDI;	// tells the GUI that the divisor displays need updating

	LFO * divisorLFO, * bufferLFO;

	float fadeOutGain, fadeInGain, realFadePart, imaginaryFadePart;	// for trig crossfading
};
