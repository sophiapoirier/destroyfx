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


//-----------------------------------------------------------------------------------------
// this function tries to even out the wildly erratic resonant amplitudes
double RezSynth::calculateAmpEvener(int numBands, int currentNote)
{
	double baseFreq = midistuff->freqTable[currentNote] * midistuff->pitchbend;
	double ampEvener = 1.0;

	if (scaleMode <= kScaleMode_none)	// no scaling
//		ampEvener = 0.0000000000009 * pow(((double)fBandwidth+0.72), 1.8) * baseFreq * baseFreq / sqrt((double)numBands*0.0000003);
		ampEvener = 0.0000000000009 * pow(contractparametervalue(kBandwidth, bandwidth)+0.72, 1.8) * baseFreq * baseFreq / sqrt((double)numBands*0.0000003);
	else if (scaleMode == kScaleMode_rms)	// RMS scaling
		ampEvener = 0.0003 * baseFreq / sqrt((double)numBands*0.39);
	else if (scaleMode >= kScaleMode_peak)	// peak scaling
		ampEvener = 0.0072 * baseFreq / ((bandwidth-getparametermin_f(kBandwidth)+10.0)/3.0) / ((double)numBands*0.03);

	if (resonAlgorithm != kResonAlg_2poleNoZero)
	{
		ampEvener *= 2.0;
		if (scaleMode <= kScaleMode_none)
			ampEvener *= 6.0;
	}

	return ampEvener;
}

//-----------------------------------------------------------------------------------------
// This function calculates the 3 coefficients in the resonant filter equation,
// 1 for the current sample input and 2 for the last 2 samples.
void RezSynth::calculateCoefficients(int * numbands, int currentNote)
{
	double baseFreq = midistuff->freqTable[currentNote] * midistuff->pitchbend;

	for (int bandcount=0; (bandcount < *numbands); bandcount++)
	{
// GET THE CURRENT BAND'S CENTER FREQUENCY
		double bandCenterFreq;
		// do logarithmic band separation, octave-style
		if (sepMode == kSepMode_octaval)
			bandCenterFreq = baseFreq * pow(2.0, sepAmount_octaval*bandcount);
		// do linear band separation, hertz-style
		else
			bandCenterFreq = baseFreq + (baseFreq*sepAmount_linear*bandcount);

// SHRINK THE NUMBER OF BANDS IF MISTAKES ARE "OFF" AND THE NEXT BAND WILL EXCEED THE NYQUIST
		if ( (!foldover) && (bandCenterFreq > nyquist) )
		{
			*numbands = bandcount;
			return;	// there's no need to do the calculations if we won't be using this band
		}

// CALCULATE THE COEFFICIENTS FOR THE 2 DELAYED INPUTS IN THE FILTER
// and CALCULATE THE COEFFICIENT FOR THE CURRENT INPUT SAMPLE IN THE FILTER
		// kScaleMode_none -> no scaling; input gain = 1
		inputAmp[bandcount] = 1.0;
		double r = exp(bandwidth * (-piDivSR));
		switch (resonAlgorithm)
		{
			// based on the reson opcode in Csound
			case kResonAlg_2poleNoZero:
			default:
				// this value is usually just approaching 1 from below (i.e. 0.999) but gets smaller 
				// and perhaps approaches 0 as bandwidth and/or the center freq distance from base freq grow
				prevprevOutCoeff[bandcount] = exp(bandCenterFreq/baseFreq * bandwidth * (-twoPiDivSR));
				// this value, at 44.1 kHz, moves in a curve from 2 to -2 as the center frequency goes from 0 to ~20 kHz
				prevOutCoeff[bandcount] = prevprevOutCoeff[bandcount]*4.0 * cos(bandCenterFreq*twoPiDivSR) / (prevprevOutCoeff[bandcount]+1.0);
				// unused in this algorithm
				prevprevInCoeff[bandcount] = 0.0;
				//
				// RMS gain = 1
				if (scaleMode == kScaleMode_rms)
					inputAmp[bandcount] = sqrt( ((prevprevOutCoeff[bandcount]+1.0)*(prevprevOutCoeff[bandcount]+1.0)-(prevOutCoeff[bandcount]*prevOutCoeff[bandcount])) * (1.0-prevprevOutCoeff[bandcount]) / (prevprevOutCoeff[bandcount]+1.0) );
				// peak gain at center = 1
				else if (scaleMode == kScaleMode_peak)
					inputAmp[bandcount] = (1.0-prevprevOutCoeff[bandcount]) * sqrt(1.0 - (prevOutCoeff[bandcount]*prevOutCoeff[bandcount]/(prevprevOutCoeff[bandcount]*4.0)));
				break;

			// an implementation of the 2-pole, 2-zero resononant filter 
			// described by Julius O. Smith and James B. Angell in 
			// "A Constant Gain Digital Resonator Tuned By A Single Coefficient," 
			// Computer Music Journal, Vol. 6, No. 4, Winter 1982, p.36-39 
			// (where the zeros are at +/- the square root of r, 
			// where r is the pole radius of the resonant filter)
			case kResonAlg_2pole2zeroR:
				prevOutCoeff[bandcount] = 2.0 * r * cos(bandCenterFreq * twoPiDivSR);
				prevprevOutCoeff[bandcount] = r * r;
				prevprevInCoeff[bandcount] = r;
				//
				if (scaleMode == kScaleMode_peak)
					inputAmp[bandcount] = 1.0 - r;
				else if (scaleMode == kScaleMode_rms)
					inputAmp[bandcount] = sqrt(1.0 - r);
				break;

			// version of the above filter where the zeros are located at z = 1 and z = -1
			case kResonAlg_2pole2zero1:
				prevOutCoeff[bandcount] = 2.0 * r * cos(bandCenterFreq * twoPiDivSR);
				prevprevOutCoeff[bandcount] = r * r;
				prevprevInCoeff[bandcount] = 1.0;
				//
				if (scaleMode == kScaleMode_peak)
					inputAmp[bandcount] = (1.0 - prevprevOutCoeff[bandcount]) * 0.5;
				else if (scaleMode == kScaleMode_rms)
					inputAmp[bandcount] = sqrt((1.0 - prevprevOutCoeff[bandcount]) * 0.5);
				break;
		}
	}	// end of bands loop
}

//-----------------------------------------------------------------------------------------
// This function writes the filtered audio output.
void RezSynth::processFilterOuts(const float * in, float * out, long sampleFrames, double ampEvener, 
							int currentNote, int numBands, 
							double & prevIn, double & prevprevIn, 
							double * prevOut, double * prevprevOut)
{
	double totalAmp = ampEvener * (double)(gain*(midistuff->noteTable[currentNote].noteAmp)*wetGain);

	// here we do the resonant filter equation using our filter coefficients, and related stuff
	for (long samplecount=0; samplecount < sampleFrames; samplecount++)
	{
		// see whether attack or release are active and fetch the output scalar
		float envAmp = midistuff->processEnvelope(fades, currentNote);	// the attack/release scalar
		double envedTotalAmp = totalAmp * (double)envAmp;

		for (int bandcount=0; bandcount < numBands; bandcount++)
		{
			// filter using the input, delayed values, and their filter coefficients
			double curBandOutValue = (inputAmp[bandcount] * (in[samplecount] - prevprevInCoeff[bandcount] * prevprevIn)) 
							   + (prevOutCoeff[bandcount] * prevOut[bandcount]) 
							   - (prevprevOutCoeff[bandcount] * prevprevOut[bandcount]);

			// add the latest resonator to the output collection, scaled by my evener and user gain
			out[samplecount] += (float) (curBandOutValue * envedTotalAmp);

			// very old outValue gets old outValue and old outValue gets current outValue (no longer current)
			prevprevOut[bandcount] = prevOut[bandcount];
			prevOut[bandcount] = curBandOutValue;
		}
		prevprevIn = prevIn;
		prevIn = in[samplecount];
	}
}
/*
void RezSynth::processFilterOuts(float * in, float * out, long sampleFrames, double ampEvener, 
							int currentNote, int numBands, double * prevOut, double * prevprevOut)
{
  NoteTable * currentNoteTableNote = &noteTable[currentNote];
  double curBandOutValue;	// holds the latest resonator's output value
  double totalAmp;
  long samplecount;
  int bandcount;
  NoteTable noteTemp;	// backup value holder for note envelope information


	totalAmp = ampEvener * (double)(gain*(noteTable[currentNote].noteAmp));

	// most of the note envelope values are liable to change below,
	// so we back them up to allow multi-band repetition
	noteTemp = noteTable[currentNote];

	// here we do the resonant filter equation using our filter coefficients, and related stuff
	for (bandcount=0; (bandcount < numBands); bandcount++)
	{
		// restore the note values before doing the next band
		noteTable[currentNote] = noteTemp;

		for (samplecount=0; (samplecount < sampleFrames); samplecount++)
		{
			// filter using the input, delayed values, and their filter coefficients
			curBandOutValue = (inputAmp[bandcount] * (in[samplecount] - prevprevInCoeff[bandcount] * prevprevIn)) + (prevOutCoeff[bandcount] * prevOut[bandcount]) - (prevprevOutCoeff[bandcount] * prevprevOut[bandcount]);

			// add the latest resonator to the output collection, scaled by my evener and user gain
			out[samplecount] += (float) (curBandOutValue * totalAmp * (double)processEnvelope(currentNoteTableNote, fadeTable, fades));

			// very old outValue gets old outValue and old outValue gets current outValue (no longer current)
			prevprevOut[bandcount] = prevOut[bandcount];
			prevOut[bandcount] = curBandOutValue;
		}
	}
}
*/

//-----------------------------------------------------------------------------------------
// this function outputs the unprocessed audio input between notes, if desired
void RezSynth::processUnaffected(const float * in, float * out, long sampleFrames)
{
	for (long samplecount=0; samplecount < sampleFrames; samplecount++)
	{
		float unEnvAmp = 1.0f;

		// this is the state when all notes just ended and the clean input first kicks in
		if (unaffectedState == kUnaffectedState_FadeIn)
		{
			// linear fade-in
			unEnvAmp = (float)unaffectedFadeSamples * kUnaffectedFadeStep;
			unaffectedFadeSamples++;
			// go to the no-gain state if the fade-in is done
			if (unaffectedFadeSamples >= kUnaffectedFadeDur)
				unaffectedState = kUnaffectedState_Flat;
		}

		// a note has just begun, so we need to hasily fade out the clean input audio
		else if (unaffectedState == kUnaffectedState_FadeOut)
		{
			unaffectedFadeSamples--;
			// linear fade-out
			unEnvAmp = (float)unaffectedFadeSamples * kUnaffectedFadeStep;
			// get ready for the next time and exit this function if the fade-out is done
			if (unaffectedFadeSamples <= 0)
			{
				// ready for the next time
				unaffectedState = kUnaffectedState_FadeIn;
				return;	// important!  leave this function or a new fade-in will begin
			}
		}

		out[samplecount] += in[samplecount] * unEnvAmp * betweenGain * wetGain;
	}
}

//-----------------------------------------------------------------------------------------
// This function checks if the latest note message is a note-on for a note that's currently off.  
// If it is, then that note's filter feedback buffers are cleared.
void RezSynth::checkForNewNote(long currentEvent, unsigned long numChannels)
{
	// store the current note MIDI number
	int currentNote = midistuff->blockEvents[currentEvent].byte1;

	// if this latest event is a note-on and this note isn't still active 
	// from being previously played, then clear this note's delay buffers
	if ( (midistuff->blockEvents[currentEvent].status == kMidiNoteOn) && 	// it's a note-on
	 		(midistuff->noteTable[currentNote].velocity == 0) )				// the note is currently off
	{
		// wipe out the feedback buffers
		for (unsigned ch=0; ch < numChannels; ch++)
		{
			int bandcount;
			for (bandcount=0; bandcount < kMaxBands; bandcount++)
				prevOutValue[ch][currentNote][bandcount] = 0.0;
			for (bandcount=0; bandcount < kMaxBands; bandcount++)
				prevprevOutValue[ch][currentNote][bandcount] = 0.0;
		}
	}
}
