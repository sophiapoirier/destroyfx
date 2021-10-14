/*------------------------------------------------------------------------
Copyright (C) 2001-2021  Sophia Poirier and Keith Fullerton Whitman

This file is part of Thrush.

Thrush is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Thrush is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Thrush.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#ifndef __thrush
#define __thrush

#include "dfxmisc.h"
#include "lfo.h"


//-------------------------------------------------------------------------------------
// these are the plugin parameters:
enum
{
	kDelay,
	kTempo,

	kLFOonOff,
	kLFO1tempoSync,
	kLFO1rate,
	kLFO1depth,
	kLFO1shape,

	kLFO2tempoSync,
	kLFO2rate,
	kLFO2depth,
	kLFO2shape,

	kStereoLink,
	kDelay2,

	kLFOonOff2,
	kLFO1tempoSync2,
	kLFO1rate2,
	kLFO1depth2,
	kLFO1shape2,

	kLFO2tempoSync2,
	kLFO2rate2,
	kLFO2depth2,
	kLFO2shape2,

	kDryWetMix,

	NUM_PARAMETERS
};


//-------------------------------------------------------------------------------------
// constants

#define DELAY_MIN 1
#define DELAY_MAX 128
const long DELAY_BUFFER_SIZE = DELAY_MAX * 2;
#define delayScaled(A)   ( (long)((A)*(float)(DELAY_MAX-DELAY_MIN)) + DELAY_MIN )
#define delayUnscaled(A)   ( (float)((A) - DELAY_MIN) / (float)(DELAY_MAX-DELAY_MIN) )

#define TEMPO_MIN 39.0f
#define TEMPO_MAX 480.0f
// this is for converting from parameter entries to the real values
#define tempoScaled(A) ( paramRangeScaled((A), TEMPO_MIN, TEMPO_MAX) )
#define tempoUnscaled(A) ( paramRangeUnscaled((A), TEMPO_MIN, TEMPO_MAX) )

#define LFO_RATE_MIN 0.09f
#define LFO_RATE_MAX 21.0f
#define LFOrateScaled(A)   ( paramRangeScaled((A), LFO_RATE_MIN, LFO_RATE_MAX) )
#define LFOrateUnscaled(A)   ( paramRangeUnscaled((A), LFO_RATE_MIN, LFO_RATE_MAX) )

#define LFO2_DEPTH_MIN 1.0f
#define LFO2_DEPTH_MAX 9.0f
#define LFO2depthScaled(A) ( paramRangeScaled((A), LFO2_DEPTH_MIN, LFO2_DEPTH_MAX) )
#define LFO2depthUnscaled(A) ( paramRangeUnscaled((A), LFO2_DEPTH_MIN, LFO2_DEPTH_MAX) )

#define NUM_PROGRAMS 16
#define PLUGIN_VERSION 1000
#define PLUGIN_ID 'thsh'


//----------------------------------------------------------------------------- 
class ThrushProgram
{
friend class Thrush;
public:
	ThrushProgram();
	~ThrushProgram();
private:
	float *param;
	char *name;
};



//-------------------------------------------------------------------------------------

class Thrush : public AudioEffectX
{
friend class ThrushEditor;
public:
	Thrush(audioMasterCallback audioMaster);
	~Thrush();

	virtual void process(float **inputs, float **outputs, long sampleFrames);
	virtual void processReplacing(float **inputs, float **outputs, long sampleFrames);

	virtual void suspend();
	virtual void resume();

	virtual void setProgram(long programNum);
	virtual void setProgramName(char *name);
	virtual void getProgramName(char *name);
	virtual bool getProgramNameIndexed(long category, long index, char *text);
	virtual bool copyProgram(long destination);

	virtual void setParameter(long index, float value);
	virtual float getParameter(long index);
	virtual void getParameterLabel(long index, char *label);
	virtual void getParameterDisplay(long index, char *text);
	virtual void getParameterName(long index, char *text);

	virtual long getTailSize();
	// there was a typo in the VST header files versions 2.0 through 2.2, 
	// so some hosts will still call this incorrectly named version...
	virtual long getGetTailSize() { return getTailSize(); }
	virtual bool getInputProperties(long index, VstPinProperties* properties);
	virtual bool getOutputProperties(long index, VstPinProperties* properties);

	virtual bool getEffectName(char *name);
	virtual long getVendorVersion();
	virtual bool getErrorText(char *text);
	virtual bool getVendorString(char *text);
	virtual bool getProductString(char *text);

	virtual long canDo(char* text);

protected:
	// the parameters
	float fDelay, fTempo, fStereoLink, fDelay2, fDryWetMix;

	LFO *lfo1, *lfo2, *lfo1_2, *lfo2_2;

	void doTheProcess(float **inputs, float **outputs, long sampleFrames, bool replacing);
	void calculateTheTempo();
	void calculateTheLFOcycleSize(LFO *lfo);
	float processTheLFOs(LFO *lfoLayer1, LFO *lfoLayer2);

	void initPresets();
	ThrushProgram *programs;

	// these track the input & delay buffer positions
	long inputPosition, delayPosition, delayPosition2, oldDelayPosition, oldDelayPosition2;
	float *delayBuffer, *delayBuffer2;	// left & right channel delay buffers

	float numLFOpointsDivSR;	// the number of LFO table points divided by the sampling rate

	bool stereoLink;	// onOffTest(fStereoLink) stored as a bool to save on calculations

	bool needLastSample, needLastSample2;
	float lastSample, lastSample2;

	TempoRateTable *tempoRateTable;	// a table of tempo rate values
	float currentTempoBPS;	// tempo in beats per second
	long hostCanDoTempo;	// my semi-booly dude who knows something about the host's VstTimeInfo implementation
	VstTimeInfo *timeInfo;
	bool needResync;	// true when playback has just started up again
};

#endif
