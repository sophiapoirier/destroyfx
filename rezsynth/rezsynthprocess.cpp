/*------------------------------------------------------------------------
Copyright (C) 2001-2010  Sophia Poirier

This file is part of Rez Synth.

Rez Synth is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Rez Synth is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Rez Synth.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "rezsynth.h"


void RezSynth::processaudio(const float **in, float **out, unsigned long inNumFrames, bool replacing)
{
	unsigned long numChannels = getnumoutputs();
	long numFramesToProcess = (signed)inNumFrames, totalSampleFrames = (signed)inNumFrames;	// for dividing up the block accoring to events
	float SAMPLERATE = getsamplerate_f();
	float dryGain = 1.0f - dryWetMix;	// the gain of the input audio in the dry/wet mix
	wetGain = dryWetMix;
	if (dryWetMixMode == kDryWetMixMode_equalpower)
	{
		// sqare root for equal power blending of non-correlated signals
		dryGain = sqrtf(dryGain);
		wetGain = sqrtf(wetGain);
	}
	unsigned long ch;


	// mix very quiet noise (-300 dB) into the input signal to hopefully avoid any denormal values
	float quietNoise = 1.0e-15f;
	for (ch=0; ch < numChannels; ch++)
	{
		float *volatileIn = (float*) in[ch];
		for (long samplecount = 0; samplecount < totalSampleFrames; samplecount++)
		{
			volatileIn[samplecount] += quietNoise;
			quietNoise = -quietNoise;
		}
	}


	// these are values that are always needed during calculateCoefficients
	piDivSR = kDFX_PI_d / getsamplerate();
	twoPiDivSR = piDivSR * 2.0;
	nyquist = (getsamplerate()-bandwidth) / 2.0;	// adjusted for bandwidth to accomodate the filter's frequency range

	// counter for the number of MIDI events this block
	// start at -1 because the beginning stuff has to happen
	long eventcount = -1;
	long currentBlockPosition = 0;	// we are at sample 0


	// now we're ready to start looking at MIDI messages and processing sound and such
	do
	{
		// check for an upcoming event and decrease this block chunk size accordingly 
		// if there won't be another event, go all the way to the end of the block
		if ( (eventcount+1) >= midistuff->numBlockEvents )
			numFramesToProcess = totalSampleFrames - currentBlockPosition;
		// else there will be and this chunk goes up to the next delta position
		else
			numFramesToProcess = midistuff->blockEvents[eventcount+1].delta - currentBlockPosition;

		// this means that 2 (or more) events occur simultaneously, 
		// so there's no need to do calculations during this round
		if (numFramesToProcess == 0)
		{
			eventcount++;
			checkForNewNote(eventcount, numChannels);	// and attend to related issues if necessary
			// take in the effects of the next event
			midistuff->heedEvents(eventcount, SAMPLERATE, pitchbendRange, attack, release, legato, velCurve, velInfluence);
			continue;
		}

		// test for whether or not all notes are off and unprocessed audio can be outputted
		bool noNotes = true;	// none yet for this chunk

		for (int notecount=0; notecount < NUM_NOTES; notecount++)
		{
			// only go into the output processing cycle if a note is happening
			if (midistuff->noteTable[notecount].velocity)
			{
				noNotes = false;	// we have a note

				double ampEvener = 1.0;	// a scalar for balancing outputs from the 3 normalizing modes
				// do the smart gain control thing if the user says so
				if (wiseAmp)
					ampEvener = calculateAmpEvener(numBands, notecount);

				// store before processing the note's coefficients
				int tempNumBands = numBands;
				// this is the resonator stuff
				calculateCoefficients(&numBands, notecount);

				// most of the note values are liable to change during processFilterOuts,
				// so we back them up to allow multi-band repetition
				NoteTable noteTemp = midistuff->noteTable[notecount];
				// render the filtered audio output for the note for each audio channel
				for (ch=0; ch < numChannels; ch++)
				{
					// restore the note values before doing processFilterOuts for the next channel
					midistuff->noteTable[notecount] = noteTemp;
					processFilterOuts(&(in[ch][currentBlockPosition]), &(out[ch][currentBlockPosition]), 
								numFramesToProcess, ampEvener, notecount, numBands, 
								prevInValue[ch][notecount], prevprevInValue[ch][notecount], 
								prevOutValue[ch][notecount], prevprevOutValue[ch][notecount]);
				}

				// restore the number of bands before moving on to the next note
				numBands = tempNumBands;
			}
		}	// end of notes loop

		// we had notes this chunk, but the unaffected processing hasn't faded out, so change its state to fade-out
		if ( (!noNotes) && (unaffectedState == kUnaffectedState_Flat) )
			unaffectedState = kUnaffectedState_FadeOut;

		// we can output unprocessed audio if no notes happened during this block chunk
		// or if the unaffected fade-out still needs to be finished
		if ( noNotes || (unaffectedState == kUnaffectedState_FadeOut) )
		{
			int tempUnState = unaffectedState;
			int tempUnSamples = unaffectedFadeSamples;
			for (ch=0; ch < numChannels; ch++)
			{
				unaffectedState = tempUnState;
				unaffectedFadeSamples = tempUnSamples;
				processUnaffected(&(in[ch][currentBlockPosition]), &(out[ch][currentBlockPosition]), numFramesToProcess);
			}
		}

		eventcount++;
		// don't do the event processing below if there are no more events
		if (eventcount >= midistuff->numBlockEvents)
			continue;

		// jump our position value forward
		currentBlockPosition = midistuff->blockEvents[eventcount].delta;

		checkForNewNote(eventcount, numChannels);	// and attend to related issues if necessary
		// take in the effects of the next event
		midistuff->heedEvents(eventcount, SAMPLERATE, pitchbendRange, attack, release, legato, velCurve, velInfluence);

	} while (eventcount < midistuff->numBlockEvents);

	// processEvents() is only called when new VstEvents are sent,
	// so zeroing numBlockEvents right here is critical
	midistuff->numBlockEvents = 0;

	// mix in the dry input (only if there is supposed to be some dry; let's not waste calculations)
	if (dryWetMix < 1.0f)
	{
		for (ch=0; ch < numChannels; ch++)
		{
			for (long samplecount=0; samplecount < totalSampleFrames; samplecount++)
				out[ch][samplecount] += in[ch][samplecount] * dryGain;
		}
	}
}

