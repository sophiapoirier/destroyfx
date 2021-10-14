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
#include "thrush.h"
#endif

#include <stdio.h>
#include <math.h>
#include <stdlib.h>


//--------------------------------------------------------------------------------------
// initializations & such

Thrush::Thrush(audioMasterCallback audioMaster)
	: AudioEffectX(audioMaster, NUM_PROGRAMS, NUM_PARAMETERS)	// 16 programs, 27 parameters
{
	// allocate memory for these arrays
	delayBuffer = new float[DELAY_BUFFER_SIZE];
	delayBuffer2 = new float[DELAY_BUFFER_SIZE];
	tempoRateTable = new TempoRateTable(kSlowTempoRates);

	lfo1 = new LFO;
	lfo2 = new LFO;
	lfo1_2 = new LFO;
	lfo2_2 = new LFO;

	programs = new ThrushProgram[NUM_PROGRAMS];
	setProgram(0);
	strcpy(programs[0].name, "thrush");	// default program name
	initPresets();

	setNumInputs(2);	// stereo in
	setNumOutputs(2);	// stereo out
	canMono();	// it's okay to feed both inputs with the same signal
	canProcessReplacing();	// supports both accumulating and replacing output
	setUniqueID(PLUGIN_ID);	// identify

	// check to see if the host supports sending tempo & time information to VST plugins
	hostCanDoTempo = canHostDo("sendVstTimeInfo");
	// default the tempo to something more reasonable than 39 bmp
	// also give currentTempoBPS a value in case that's useful for a freshly opened GUI
	if (hostCanDoTempo != 1)
	{
		setParameter( kTempo, tempoUnscaled(120.0f) );
		currentTempoBPS = tempoScaled(fTempo) / 60.0f;
	}
	else
		currentTempoBPS = (float)tempoAt(0) / 600000.0f;

	suspend();
}

//-----------------------------------------------------------------------------------------
// this is the destructor - here we would clean up for the host

Thrush::~Thrush()
{
	if (programs)
		delete[] programs;

	if (lfo1)
		delete lfo1;
	if (lfo2)
		delete lfo2;
	if (lfo1_2)
		delete lfo1_2;
	if (lfo2_2)
		delete lfo2_2;

	// deallocate the memory from these arrays
	if (delayBuffer)
		delete[] delayBuffer;
	if (delayBuffer2)
		delete[] delayBuffer2;
	if (tempoRateTable)
		delete tempoRateTable;
}

//-----------------------------------------------------------------------------------------
// this gets called when the plugin is de-activated

void Thrush::suspend()
{
	inputPosition = 0;
	lfo1->reset();
	lfo2->reset();
	lfo1_2->reset();
	lfo2_2->reset();
	delayPosition = delayPosition2 = oldDelayPosition = oldDelayPosition2 = 0;

	for (long i=0; (i < DELAY_BUFFER_SIZE); i++)
	{
		delayBuffer[i] = 0.0f;
		delayBuffer2[i] = 0.0f;
	}
}

//-----------------------------------------------------------------------------
// this gets called when the plugin is activated
void Thrush::resume()
{
	needResync = true;	// some hosts may call resume when restarting playback
}

//-----------------------------------------------------------------------------------------
// Destroy FX infos

bool Thrush::getEffectName(char *name) {
	strcpy (name, "Thrush");	// name max 32 char
	return true; }

long Thrush::getVendorVersion() {
	return PLUGIN_VERSION; }

bool Thrush::getErrorText(char *text) {
	strcpy (text, "I told you not to touch me.");	// max 256 char
	return true; }

bool Thrush::getVendorString(char *text) {
	strcpy (text, "Hrvatski / Destroy FX");	// a string identifying the vendor (max 64 char)
	return true; }

bool Thrush::getProductString(char *text) {
	// a string identifying the product name (max 64 char)
	strcpy (text, "Super Destroy FX upsetting+delightful plugin pack");
	return true; }

//-----------------------------------------------------------------------------------------
// this tells the host to keep calling process() for the duration of the maximum delay time  
// even if the audio input has ended

long Thrush::getTailSize()
{
	return (long)DELAY_BUFFER_SIZE;
}

//------------------------------------------------------------------------
bool Thrush::getInputProperties(long index, VstPinProperties* properties)
{
	if ( (index >= 0) && (index < 2) )
	{
		sprintf(properties->label, "Thrush input %ld", index+1);
		sprintf(properties->shortLabel, "in %ld", index+1);
		properties->flags = kVstPinIsStereo | kVstPinIsActive;
		return true;
	}
	return false;
}

//------------------------------------------------------------------------
bool Thrush::getOutputProperties(long index, VstPinProperties* properties)
{
	if ( (index >= 0) && (index < 2) )
	{
		sprintf (properties->label, "Thrush output %ld", index+1);
		sprintf (properties->shortLabel, "out %ld", index+1);
		properties->flags = kVstPinIsStereo | kVstPinIsActive;
		return true;
	}
	return false;
}

//----------------------------------------------------------------------------- 
ThrushProgram::ThrushProgram()
{
	name = new char[32];
	param = new float[NUM_PARAMETERS];

	param[kDelay] = delayUnscaled(9);
	param[kTempo] = 0.0f;	// default to "auto"
	param[kLFOonOff] = 0.0f;	// default to off
	param[kLFO1rate] = LFOrateUnscaled(3.0f);
	param[kLFO1tempoSync] = 0.0f;	// default to off
	param[kLFO1depth] = 0.81f;
	param[kLFO1shape] = LFOshapeUnscaled(kLFOshape_thorn);	// default to thorn
	param[kLFO2rate] = LFOrateUnscaled(6.0f);
	param[kLFO2tempoSync] = 0.0f;	// default to off
	param[kLFO2depth] = 0.0f;
	param[kLFO2shape] = LFOshapeUnscaled(kLFOshape_triangle);	// default to triangle
	param[kStereoLink] = 1.0f;	// default to on
	param[kDelay2] = delayUnscaled(3);
	param[kLFOonOff2] = 0.0f;	// default to off
	param[kLFO1rate2] = LFOrateUnscaled(9.0f);
	param[kLFO1tempoSync2] = 0.0f;	// default to off
	param[kLFO1depth2] = 0.3f;
	param[kLFO1shape2] = LFOshapeUnscaled(kLFOshape_square);	// default to square
	param[kLFO2rate2] = LFOrateUnscaled(12.0f);
	param[kLFO2tempoSync2] = 0.0f;	// default to off
	param[kLFO2depth2] = 0.0f;
	param[kLFO2shape2] = LFOshapeUnscaled(kLFOshape_saw);	// default to sawtooth
	param[kDryWetMix] = 1.0f;

	strcpy(name, "default");
}

//----------------------------------------------------------------------------- 
ThrushProgram::~ThrushProgram()
{
	if (name)
		delete[] name;
	if (param)
		delete[] param;
}


//--------- programs --------
//----------------------------------------------------------------------------- 
void Thrush::setProgram(long programNum)
{
	if ( (programNum < NUM_PROGRAMS) && (programNum >= 0) )
	{
		AudioEffectX::setProgram(programNum);

		for (int i=0; i < NUM_PARAMETERS; i++)
			setParameter(i, programs[programNum].param[i]);
	}
	// tell the host to update the editor display with the new settings
	AudioEffectX::updateDisplay();
}

//-----------------------------------------------------------------------------------------
void Thrush::setProgramName(char *name)
{
	strcpy(programs[curProgram].name, name);
}

//-----------------------------------------------------------------------------------------
void Thrush::getProgramName(char *name)
{
	if ( !strcmp(programs[curProgram].name, "default") )
		sprintf(name, "default %ld", curProgram+1);
	else
		strcpy(name, programs[curProgram].name);
}

//-----------------------------------------------------------------------------------------
bool Thrush::getProgramNameIndexed(long category, long index, char *text)
{
	if ( (index < NUM_PROGRAMS) && (index >= 0) )
	{
		strcpy(text, programs[index].name);
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------------------
bool Thrush::copyProgram(long destination)
{
	if ( (destination < NUM_PROGRAMS) && (destination >= 0) )
	{
		if (destination != curProgram)	// only copy it if it's a different program slot
		{
			for (long i = 0; i < NUM_PARAMETERS; i++)
				programs[destination].param[i] = programs[curProgram].param[i];
			strcpy(programs[destination].name, programs[curProgram].name);
		}
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------------------
void Thrush::initPresets() {
  int i = 1;

//	programs[i].param[kDelay] = delayUnscaled();
	programs[i].param[kLFOonOff] = 1.0f;
	programs[i].param[kLFO1tempoSync] = 0.0f;
//	programs[i].param[kLFO1rate] = LFOrateUnscaled(f);
//	programs[i].param[kLFO1depth] = f;
	programs[i].param[kLFO1shape] = LFOshapeUnscaled(kLFOshape_saw);
	programs[i].param[kStereoLink] = 0.0f;	// off
//	programs[i].param[kDelay2] = delayUnscaled();
	programs[i].param[kLFOonOff2] = 1.0f;
	programs[i].param[kLFO1tempoSync2] = 0.0f;
//	programs[i].param[kLFO1rate2] = LFOrateUnscaled(f);
//	programs[i].param[kLFO1depth2] = f;
	programs[i].param[kLFO1shape2] = LFOshapeUnscaled(kLFOshape_saw);
	programs[i].param[kDryWetMix] = 1.0f;
	strcpy(programs[i].name, "vibber");
	i++;

/*
	programs[i].param[kDelay] = delayUnscaled();
	programs[i].param[kTempo] = 0.0f;
	programs[i].param[kLFOonOff] = .0f;
	programs[i].param[kLFO1rate] = LFOrateUnscaled(f);
	programs[i].param[kLFO1tempoSync] = .0f;
	programs[i].param[kLFO1depth] = f;
	programs[i].param[kLFO1shape] = LFOshapeUnscaled(kLFOshape_);
	programs[i].param[kLFO2rate] = LFOrateUnscaled(f);
	programs[i].param[kLFO2tempoSync] = .0f;
	programs[i].param[kLFO2depth] = f;
	programs[i].param[kLFO2shape] = LFOshapeUnscaled(kLFOshape_);
	programs[i].param[kStereoLink] = .0f;
	programs[i].param[kDelay2] = delayUnscaled();
	programs[i].param[kLFOonOff2] = .0f;
	programs[i].param[kLFO1rate2] = LFOrateUnscaled(f);
	programs[i].param[kLFO1tempoSync2] = .0f;
	programs[i].param[kLFO1depth2] = f;
	programs[i].param[kLFO1shape2] = LFOshapeUnscaled(kLFOshape_);
	programs[i].param[kLFO2rate2] = LFOrateUnscaled(f);
	programs[i].param[kLFO2tempoSync2] = .0f;
	programs[i].param[kLFO2depth2] = f;
	programs[i].param[kLFO2shape2] = LFOshapeUnscaled(kLFOshape_);
	programs[i].param[kDryWetMix] = 1.0f;
	strcpy(programs[i].name, "");
	i++;
*/
};

//--------- programs (end) --------


//-----------------------------------------------------------------------------------------
void Thrush::setParameter(long index, float value)
{
	switch (index)
	{
		case kDelay          : fDelay = value;				break;
		case kTempo          : fTempo = value;				break;
		case kLFOonOff       : lfo1->fOnOff = value;		break;
		case kLFO1rate:
			// make sure the cycles match up if the tempo rate has changed
			if (tempoRateTable->getScalar(lfo1->fRate) != tempoRateTable->getScalar(value))
				needResync = true;
			lfo1->fRate = value;
			break;
		case kLFO1tempoSync :
			// set needResync true if tempo sync mode has just been switched on
			if ( onOffTest(value) && !onOffTest(lfo1->fTempoSync) )
				needResync = true;
			lfo1->fTempoSync = value;
			break;
		case kLFO1depth      : lfo1->fDepth = value;		break;
		case kLFO1shape      : lfo1->fShape = value;		break;
		case kLFO2rate:
			// make sure the cycles match up if the tempo rate has changed
			if (tempoRateTable->getScalar(lfo2->fRate) != tempoRateTable->getScalar(value))
				needResync = true;
			lfo2->fRate = value;
			break;
		case kLFO2tempoSync :
			// set needResync true if tempo sync mode has just been switched on
			if ( onOffTest(value) && !onOffTest(lfo2->fTempoSync) )
				needResync = true;
			lfo2->fTempoSync = value;
			break;
		case kLFO2depth      : lfo2->fDepth = value;		break;
		case kLFO2shape      : lfo2->fShape = value;		break;
		case kStereoLink :
			fStereoLink = value;
			stereoLink = onOffTest(value);
			break;
		case kDelay2         : fDelay2 = value;				break;
		case kLFOonOff2      : lfo1_2->fOnOff = value; 		break;
		case kLFO1rate2      : lfo1_2->fRate = value;		break;
		case kLFO1tempoSync2 :
			// set needResync true if tempo sync mode has just been switched on
			if ( !onOffTest(fStereoLink) && (onOffTest(value) && !onOffTest(lfo1_2->fTempoSync)) )
				needResync = true;
			lfo1_2->fTempoSync = value;
			break;
		case kLFO1depth2     : lfo1_2->fDepth = value;		break;
		case kLFO1shape2     : lfo1_2->fShape = value;		break;
		case kLFO2rate2      : lfo2_2->fRate = value;		break;
		case kLFO2tempoSync2 :
			// set needResync true if tempo sync mode has just been switched on
			if ( !onOffTest(fStereoLink) && (onOffTest(value) && !onOffTest(lfo2_2->fTempoSync)) )
				needResync = true;
			lfo2_2->fTempoSync = value;
			break;
		case kLFO2depth2     : lfo2_2->fDepth = value;		break;
		case kLFO2shape2     : lfo2_2->fShape = value;		break;
		case kDryWetMix      : fDryWetMix = value;			break;
	}

	if ( (index >= 0) && (index < NUM_PARAMETERS) )
		programs[curProgram].param[index] = value;
}

//-----------------------------------------------------------------------------------------
float Thrush::getParameter(long index)
{
	switch (index)
	{
		default:
		case kDelay          : return fDelay;
		case kTempo          : return fTempo;
		case kLFOonOff       : return lfo1->fOnOff;
		case kLFO1rate       : return lfo1->fRate;
		case kLFO1tempoSync  : return lfo1->fTempoSync;
		case kLFO1depth      : return lfo1->fDepth;
		case kLFO1shape      : return lfo1->fShape;
		case kLFO2rate       : return lfo2->fRate;
		case kLFO2tempoSync  : return lfo2->fTempoSync;
		case kLFO2depth      : return lfo2->fDepth;
		case kLFO2shape      : return lfo2->fShape;
		case kStereoLink     : return fStereoLink;
		case kDelay2         : return fDelay2;
		case kLFOonOff2      : return lfo1_2->fOnOff;
		case kLFO1rate2      : return lfo1_2->fRate;
		case kLFO1tempoSync2 : return lfo1_2->fTempoSync;
		case kLFO1depth2     : return lfo1_2->fDepth;
		case kLFO1shape2     : return lfo1_2->fShape;
		case kLFO2rate2      : return lfo2_2->fRate;
		case kLFO2tempoSync2 : return lfo2_2->fTempoSync;
		case kLFO2depth2     : return lfo2_2->fDepth;
		case kLFO2shape2     : return lfo2_2->fShape;
		case kDryWetMix      : return fDryWetMix;
	}
}

//-----------------------------------------------------------------------------------------
// titles of each parameter

void Thrush::getParameterName(long index, char *label)
{
	switch (index)
	{
		case kDelay          : strcpy(label, "inverse delay");			break;
		case kTempo          : strcpy(label, "tempo");					break;
		case kLFOonOff       : strcpy(label, "LFO");					break;
		case kLFO1rate       : strcpy(label, "LFO1 rate");				break;
		case kLFO1tempoSync  : strcpy(label, "LFO1 tempo sync");		break;
		case kLFO1depth      : strcpy(label, "LFO1 depth");				break;
		case kLFO1shape      : strcpy(label, "LFO1 shape");				break;
		case kLFO2rate       : strcpy(label, "LFO2 rate");				break;
		case kLFO2tempoSync  : strcpy(label, "LFO2 tempo sync");		break;
		case kLFO2depth      : strcpy(label, "LFO2 depth");				break;
		case kLFO2shape      : strcpy(label, "LFO2 shape");				break;
		case kStereoLink     : strcpy(label, "stereo link");			break;
		case kDelay2         : strcpy(label, "inverse delay R");		break;
		case kLFOonOff2      : strcpy(label, "LFO R");					break;
		case kLFO1rate2      : strcpy(label, "LFO1 rate R");			break;
		case kLFO1tempoSync2 : strcpy(label, "LFO1 tempo sync R");		break;
		case kLFO1depth2     : strcpy(label, "LFO1 depth R");			break;
		case kLFO1shape2     : strcpy(label, "LFO1 shape R");			break;
		case kLFO2rate2      : strcpy(label, "LFO2 rate R");			break;
		case kLFO2tempoSync2 : strcpy(label, "LFO2 tempo sync R");		break;
		case kLFO2depth2     : strcpy(label, "LFO2 depth R");			break;
		case kLFO2shape2     : strcpy(label, "LFO2 shape R");			break;
		case kDryWetMix      : strcpy(label, "dry/wet mix");			break;
	}
}

//-----------------------------------------------------------------------------------------
// numerical display of each parameter's gradiations

void Thrush::getParameterDisplay(long index, char *text)
{
	switch (index)
	{
		case kDelay  :
		case kDelay2 :
			sprintf(text, "%ld", delayScaled(getParameter(index)));
			break;

		case kTempo :
			if ( (fTempo > 0.0f) || (hostCanDoTempo != 1) )
				sprintf(text, "%.3f", tempoScaled(fTempo));
			else
				strcpy(text, "auto");
			break;

		case kLFOonOff       :
		case kLFOonOff2      :
		case kLFO1tempoSync  :
		case kLFO2tempoSync  :
		case kLFO1tempoSync2 :
		case kLFO2tempoSync2 :
		case kStereoLink     :
			if (onOffTest(getParameter(index)))
				strcpy(text, "on");
			else
				strcpy(text, "off");
			break;

		case kLFO1rate :
			if (onOffTest(lfo1->fTempoSync))
				strcpy(text, tempoRateTable->getDisplay(lfo1->fRate));
			else
				sprintf(text, "%.3f", LFOrateScaled(lfo1->fRate));
			break;
		case kLFO2rate :
			if (onOffTest(lfo2->fTempoSync))
				strcpy(text, tempoRateTable->getDisplay(lfo2->fRate));
			else
				sprintf(text, "%.3f", LFOrateScaled(lfo2->fRate));
			break;
		case kLFO1rate2 :
			if (onOffTest(lfo1_2->fTempoSync))
				strcpy(text, tempoRateTable->getDisplay(lfo1_2->fRate));
			else
				sprintf(text, "%.3f", LFOrateScaled(lfo1_2->fRate));
			break;
		case kLFO2rate2 :
			if (onOffTest(lfo2_2->fTempoSync))
				strcpy(text, tempoRateTable->getDisplay(lfo2_2->fRate));
			else
				sprintf(text, "%.3f", LFOrateScaled(lfo2_2->fRate));
			break;

		case kLFO1depth  :
		case kLFO1depth2 :
			sprintf(text, "%ld %%", (long)(getParameter(index) * 100.0f));
			break;

		case kLFO2depth  :
		case kLFO2depth2 :
			sprintf(text, "%.3f x", LFO2depthScaled(getParameter(index)));
			break;

		case kLFO1shape :
			lfo1->getShapeName(text);
			break;
		case kLFO2shape :
			lfo2->getShapeName(text);
			break;
		case kLFO1shape2 :
			lfo1_2->getShapeName(text);
			break;
		case kLFO2shape2 :
			lfo2_2->getShapeName(text);
			break;

		case kDryWetMix :
			sprintf(text, "%ld %%", (long)(fDryWetMix * 100.0f));
			break;
	}
}

//-----------------------------------------------------------------------------------------
// unit of measure for each parameter

void Thrush::getParameterLabel(long index, char *label)
{
	switch (index)
	{
		case kDelay  :
		case kDelay2 :
			if (delayScaled(getParameter(index)) == 1) strcpy(label, "sample");
			else strcpy(label, "samples");
			break;

		case kTempo : strcpy(label, "bpm");	break;

		case kLFOonOff       :
		case kLFOonOff2      :
		case kLFO1tempoSync  :
		case kLFO2tempoSync  :
		case kLFO1tempoSync2 :
		case kLFO2tempoSync2 :
		case kStereoLink     :
			strcpy(label, " ");
			break;

		case kLFO1rate :
			if (onOffTest(lfo1->fTempoSync)) strcpy(label, "cycles/beat");
			else strcpy(label, "Hz");
			break;
		case kLFO2rate :
			if (onOffTest(lfo2->fTempoSync)) strcpy(label, "cycles/beat");
			else strcpy(label, "Hz");
			break;
		case kLFO1rate2 :
			if (onOffTest(lfo1_2->fTempoSync)) strcpy(label, "cycles/beat");
			else strcpy(label, "Hz");
			break;
		case kLFO2rate2 :
			if (onOffTest(lfo2_2->fTempoSync)) strcpy(label, "cycles/beat");
			else strcpy(label, "Hz");
			break;

		case kLFO1depth  :
		case kLFO1depth2 :
			strcpy(label, " ");
			break;

		case kLFO2depth  :
		case kLFO2depth2 :
			strcpy(label, " ");
			break;

		case kLFO1shape  :
		case kLFO2shape  :
		case kLFO1shape2 :
		case kLFO2shape2 :
			strcpy(label, " ");
			break;

		case kDryWetMix : strcpy(label, " ");	break;
	}
}

//-----------------------------------------------------------------------------------------
// this just tells the host that Thrush can handle tempo & time information

long Thrush::canDo(char* text)
{
	if (strcmp(text, "receiveVstTimeInfo") == 0)
		return 1;
	if (strcmp(text, "plugAsChannelInsert") == 0)
		return 1;
	if (strcmp(text, "plugAsSend") == 0)
		return 1;
	if (strcmp(text, "mixDryWet") == 0)
		return 1;
	if (strcmp(text, "1in1out") == 0)
		return 1;
	if (strcmp(text, "1in2out") == 0)
		return 1;
	if (strcmp(text, "2in2out") == 0)
		return 1;

	return -1;	// explicitly can't do; 0 => don't know
}


//--------------------------------------------------------------------------------------
// this function calculates the tempo, if needed

void Thrush::calculateTheTempo()
{
	// figure out the current tempo if any of our LFOs are tempo synced
	if ( (onOffTest(lfo1->fTempoSync) || onOffTest(lfo2->fTempoSync)) || 
		 ( !stereoLink && (onOffTest(lfo1_2->fTempoSync) || onOffTest(lfo2_2->fTempoSync)) ) )
	{
		// calculate the tempo at the current processing buffer
		if ( (fTempo > 0.0f) || (hostCanDoTempo != 1) )	// get the tempo from the user parameter
		{
			currentTempoBPS = tempoScaled(fTempo) / 60.0f;
			needResync = false;	// we don't want it true if we're not syncing to host tempo
		}
		else	// get the tempo from the host
		{
			timeInfo = getTimeInfo(kBeatSyncTimeInfoFlags);
			if (timeInfo)
			{
				if (kVstTempoValid & timeInfo->flags)
					currentTempoBPS = (float)timeInfo->tempo / 60.0f;
				else
					currentTempoBPS = tempoScaled(fTempo) / 60.0f;
				// but zero & negative tempos are bad, so get the user tempo value instead if that happens
				if (currentTempoBPS <= 0.0f)
					currentTempoBPS = tempoScaled(fTempo) / 60.0f;
				//
				// check if audio playback has just restarted & reset cycle state stuff if it has (for measure sync)
				if (timeInfo->flags & kVstTransportChanged)
					needResync = true;
			}
			else	// do the same stuff as above if the timeInfo gets a null pointer
			{
				currentTempoBPS = tempoScaled(fTempo) / 60.0f;
				needResync = false;	// we don't want it true if we're not syncing to host tempo
			}
		}
	}
}

//--------------------------------------------------------------------------------------
// this function calculates the duration, in samples, of each LFO cycle

void Thrush::calculateTheLFOcycleSize(LFO *lfo)
{
	// calculate the size of the LFO cycle
	if (onOffTest(lfo->fTempoSync))	// the user wants to do tempo sync / beat division rate
		lfo->cycleRate = currentTempoBPS * (tempoRateTable->getScalar(lfo->fRate));
	else	// use the manual/Hz rate
		lfo->cycleRate = LFOrateScaled(lfo->fRate);
	// get a float increment value for each step through the LFO table at the current LFO frequency
	lfo->stepSize = lfo->cycleRate * numLFOpointsDivSR;
}

//--------------------------------------------------------------------------------------
// this function modulates the first layer LFO with the second layer & then gets the first layer LFO output

float Thrush::processTheLFOs(LFO *lfoLayer1, LFO *lfoLayer2)
{
  float LFOoffset;	// this is the offset from the first layer LFO's rate, caused by the second layer LFO


	// process the LFOs if the first layer LFO is turned on
	if (onOffTest(lfoLayer1->fOnOff))
	{
		// do beat sync if it ought to be done
		if ( needResync && onOffTest(lfoLayer2->fTempoSync) )
			lfoLayer2->syncToTheBeat(samplesToNextBar(timeInfo));

		lfoLayer2->updatePosition();	// move forward another sample position
		LFOoffset = lfoLayer2->processLFO();

		// scale the 0.0 - 1.0 LFO output value to the depth range of the second layer LFO
		LFOoffset = LFO2depthScaled(LFOoffset);

		// update the first layer LFO's table step size as modulated by the second layer LFO
		lfoLayer1->stepSize = lfoLayer1->cycleRate * LFOoffset * numLFOpointsDivSR;
		// do beat sync if it must be done (don't do it if the 2nd layer LFO is active; that's too much to deal with)
		if ( needResync && (onOffTest(lfoLayer1->fTempoSync) && (lfoLayer2->fDepth < 0.0001f)) )
			lfoLayer1->syncToTheBeat(samplesToNextBar(timeInfo));

		lfoLayer1->updatePosition();	// move forward another sample position
		// scale the 0.0 - 1.0 LFO output value to 0.0 - 2.0 (oscillating around 1.0)
		return lfoLayer1->processLFOzero2two();
	}

	// return a scalar of 1.0 if the LFO is not on, i.e. a scalar that doesn't change anything
	else return 1.0f;
}

//--------------------------------------------------------------------------------------
void Thrush::doTheProcess(float **inputs, float **outputs, long sampleFrames, bool replacing)
{
  long samplecount;
  float inputGain = 1.0f - (fDryWetMix * 0.5f);
  float inverseGain = fDryWetMix * 0.5f;
  float oldGain, newGain;
  float delayOffset;	// a scalar that is the offset of the inverse signals' delay caused by the LFOs
  float SAMPLERATE = getSampleRate();


	// just in case the host responds with something wacky
	if (SAMPLERATE <= 0.0f)   SAMPLERATE = 44100.0f;
	// this is a handy value to have during LFO calculations & wasteful to recalculate at every sample
	numLFOpointsDivSR = NUM_LFO_POINTS_FLOAT / SAMPLERATE;

	// set up the basic startup conditions of all of the LFOs if any LFOs are turned on
	if ( (onOffTest(lfo1->fOnOff)) || (!stereoLink && onOffTest(lfo1_2->fOnOff)) )
	{
		calculateTheTempo();
		calculateTheLFOcycleSize(lfo1);
		calculateTheLFOcycleSize(lfo2);
		calculateTheLFOcycleSize(lfo1_2);
		calculateTheLFOcycleSize(lfo2_2);
		lfo1->pickTheLFOwaveform();
		lfo2->pickTheLFOwaveform();
		lfo1_2->pickTheLFOwaveform();
		lfo2_2->pickTheLFOwaveform();
	}

	for (samplecount=0; (samplecount < sampleFrames); samplecount++)
	{
		// evaluate the sample-by-sample output of the LFOs
		delayOffset = processTheLFOs(lfo1, lfo2);
		// update the delay position(s)   (this is done every sample in case LFOs are active)
		delayPosition = (inputPosition - delayScaled(fDelay*delayOffset) + DELAY_BUFFER_SIZE) % DELAY_BUFFER_SIZE;
		if (stereoLink)
			delayPosition2 = delayPosition;
		else
		{
			delayOffset = processTheLFOs(lfo1_2, lfo2_2);
			delayPosition2 = (inputPosition - delayScaled(fDelay2*delayOffset) + DELAY_BUFFER_SIZE) % DELAY_BUFFER_SIZE;
		}
		needResync = false;	// make sure it gets set false so that it doesn't happen again when it shouldn't

		// audio output ... first reckon with the anything that needs to be smoothed (this is a mess)
		if ( (lfo1->smoothSamples > 0) || (!stereoLink && (lfo1_2->smoothSamples > 0)) )
		{
			// give the right channel the same fade position if stereo link is on
			if (stereoLink)
				lfo1_2->smoothSamples = lfo1->smoothSamples;

			////////////   doing the left channel first   ////////////
			// If the fade is at the very beginning, don't actually start fading anything yet.
			// Instead take the current output sample & store it as the blending out value.
			if (lfo1->smoothSamples == LFO_SMOOTH_DUR)
			{
				oldDelayPosition = delayPosition;
				lastSample = (inputs[0][samplecount] * inputGain) - (delayBuffer[delayPosition] * inverseGain);
				if (replacing)
					outputs[0][samplecount] = lastSample;
				else
					outputs[0][samplecount] += lastSample;
			}
			// do regular fade-out/mixing stuff
			else if (lfo1->smoothSamples > 0)
			{
				oldDelayPosition = (oldDelayPosition+1) % DELAY_BUFFER_SIZE;
				oldGain = (float)lfo1->smoothSamples * LFO_SMOOTH_STEP;
				newGain = 1.0f - oldGain;
				if (replacing)
					outputs[0][samplecount] = (inputs[0][samplecount] * inputGain) - 
												((delayBuffer[delayPosition] * inverseGain) * newGain) - 
												((delayBuffer[oldDelayPosition] * inverseGain) * oldGain);
				else
					outputs[0][samplecount] += (inputs[0][samplecount] * inputGain) - 
												((delayBuffer[delayPosition] * inverseGain) * newGain) - 
												((delayBuffer[oldDelayPosition] * inverseGain) * oldGain);
			}
			// if neither of the first 2 conditions were true, then the left channel 
			// is not fading/blending at all - the right channel must be - do regular output
			else
			{
				if (replacing)
					outputs[0][samplecount] = (inputs[0][samplecount] * inputGain) - 
												(delayBuffer[delayPosition] * inverseGain);
				else
					outputs[0][samplecount] += (inputs[0][samplecount] * inputGain) - 
												(delayBuffer[delayPosition] * inverseGain);
			}

			////////////   now the right channel - same stuff as above   ////////////
			if (lfo1_2->smoothSamples == LFO_SMOOTH_DUR)
			{
				oldDelayPosition2 = delayPosition2;
				lastSample2 = (inputs[1][samplecount] * inputGain) - (delayBuffer2[delayPosition2] * inverseGain);
				if (replacing)
					outputs[1][samplecount] = lastSample2;
				else
					outputs[1][samplecount] += lastSample2;
			}
			else if (lfo1_2->smoothSamples > 0)
			{
				oldDelayPosition2 = (oldDelayPosition2+1) % DELAY_BUFFER_SIZE;
				oldGain = (float)lfo1_2->smoothSamples * LFO_SMOOTH_STEP;
				newGain = 1.0f - oldGain;
				if (replacing)
					outputs[1][samplecount] = (inputs[1][samplecount] * inputGain) - 
												((delayBuffer2[delayPosition2] * inverseGain) * newGain) - 
												((delayBuffer2[oldDelayPosition2] * inverseGain) * oldGain);
				else
					outputs[1][samplecount] += (inputs[1][samplecount] * inputGain) - 
												((delayBuffer2[delayPosition2] * inverseGain) * newGain) - 
												((delayBuffer2[oldDelayPosition2] * inverseGain) * oldGain);
			}
			else
			{
				if (replacing)
					outputs[1][samplecount] = (inputs[1][samplecount] * inputGain) - 
												(delayBuffer2[delayPosition2] * inverseGain);
				else
					outputs[1][samplecount] += (inputs[1][samplecount] * inputGain) - 
												(delayBuffer2[delayPosition2] * inverseGain);
			}

			// decrement the counters if they are currently counting
			if (lfo1->smoothSamples > 0)
				(lfo1->smoothSamples)--;
			if (lfo1_2->smoothSamples > 0)
				(lfo1_2->smoothSamples)--;
		}


		// no smoothing is going on   (this is much simpler)
		else
		{
			if (replacing)
			{
				outputs[0][samplecount] = (inputs[0][samplecount] * inputGain) - 
										(delayBuffer[delayPosition] * inverseGain);
				outputs[1][samplecount] = (inputs[1][samplecount] * inputGain) - 
										(delayBuffer2[delayPosition2] * inverseGain);
			}
			else
			{
				outputs[0][samplecount] += (inputs[0][samplecount] * inputGain) - 
										(delayBuffer[delayPosition] * inverseGain);
				outputs[1][samplecount] += (inputs[1][samplecount] * inputGain) - 
										(delayBuffer2[delayPosition2] * inverseGain);
			}
		}

		// put the latest input samples into the delay buffer
		delayBuffer[inputPosition] = inputs[0][samplecount];
		delayBuffer2[inputPosition] = inputs[1][samplecount];

		// incremement the input position in the delay buffer & wrap it around 
		// if it has reached the end of the buffer
		inputPosition = (inputPosition+1) % DELAY_BUFFER_SIZE;
	}
}

//--------------------------------------------------------------------------------------
void Thrush::process(float **inputs, float **outputs, long sampleFrames)
{
	doTheProcess(inputs, outputs, sampleFrames, false);
}

//--------------------------------------------------------------------------------------
void Thrush::processReplacing(float **inputs, float **outputs, long sampleFrames)
{
	doTheProcess(inputs, outputs, sampleFrames, true);
}
