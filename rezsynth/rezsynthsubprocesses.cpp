/*-------------- by Marc Poirier  ][  January - March 2001 -------------*/

#include "rezsynth.hpp"


//-----------------------------------------------------------------------------------------
// this function tries to even out the wildly erratic resonant amplitudes
double RezSynth::processAmpEvener(int numBands, int currentNote)
{
	double baseFreq = midistuff->freqTable[currentNote] * midistuff->pitchbend;
	double ampEvener;

	if (scaleMode <= kScaleMode_none)	// no scaling
//		ampEvener = 0.0000000000009 * pow(((double)fBandwidth+0.72), 1.8) * baseFreq * baseFreq / sqrt((double)numBands*0.0000003);
		ampEvener = 0.0000000000009 * pow(contractparametervalue_index(kBandwidth, bandwidth)+0.72, 1.8) * baseFreq * baseFreq / sqrt((double)numBands*0.0000003);
	else if (scaleMode == kScaleMode_rms)	// RMS scaling
		ampEvener = 0.0003 * baseFreq / sqrt((double)numBands*0.39);
	else if (scaleMode >= kScaleMode_peak)	// peak scaling
		ampEvener = 0.0072 * baseFreq / ((bandwidth-BANDWIDTH_MIN+10.0)/3.0) / ((double)numBands*0.03);
	return ampEvener;
}

//-----------------------------------------------------------------------------------------
// This function calculates the 3 coefficients in the resonant filter equation,
// 1 for the current sample input & 2 for the last 2 samples.
void RezSynth::processCoefficients(int *numbands, int currentNote)
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

// SHRINK THE NUMBER OF BANDS IF MISTAKES ARE "OFF" & THE NEXT BAND WILL EXCEED THE NYQUIST
		if ( (!foldover) && (bandCenterFreq > nyquist) )
		{
			*numbands = bandcount;
			return;	// there's no need to do the calculations if we won't be using this band
		}

// CALCULATE THE COEFFICIENTS FOR THE 2 DELAYED INPUTS IN THE FILTER
		// this value is usually just approaching 1 from below (i.e. 0.999) but gets smaller 
		// & perhaps approaches 0 as bandwidth &/or the center freq distance from base freq grow
		delay2amp[bandcount] = exp(bandCenterFreq/baseFreq * bandwidth * (-twoPiDivSR));
		// this value, at 44.1 kHz, moves in a curve from 2 to -2 as the center frequency goes from 0 to ~20 kHz
		delay1amp[bandcount] = cos(bandCenterFreq*twoPiDivSR) * delay2amp[bandcount]*4.0 / (delay2amp[bandcount]+1.0);

// CALCULATE THE COEFFICIENT FOR THE CURRENT INPUT SAMPLE IN THE FILTER
		// no scaling; input gain = 1
		if (scaleMode <= kScaleMode_none)
			inputAmp[bandcount] = 1.0;
		// RMS gain = 1
		else if (scaleMode == kScaleMode_rms)
			inputAmp[bandcount] = sqrt( (1.0-delay2amp[bandcount]) * ((delay2amp[bandcount]+1.0)*(delay2amp[bandcount]+1.0)-(delay1amp[bandcount]*delay1amp[bandcount])) / (delay2amp[bandcount]+1.0) );
		// peak gain at center = 1
		else if (scaleMode >= kScaleMode_peak)
			inputAmp[bandcount] = sqrt(1.0 - (delay1amp[bandcount]*(delay1amp[bandcount])/(delay2amp[bandcount]*4.0))) * (1.0-delay2amp[bandcount]);
	}	// end of bands loop
}

//-----------------------------------------------------------------------------------------
// This function writes the filtered audio output.
void RezSynth::processFilterOuts(const float *in, float *out, long sampleFrames, double ampEvener, 
							int currentNote, int numBands, double *prevOut, double *prevprevOut)
{
	double totalAmp = ampEvener * (double)(gain*(midistuff->noteTable[currentNote].noteAmp)*wetGain);

	// here we do the resonant filter equation using our filter coefficients, & related stuff
	for (long samplecount=0; samplecount < sampleFrames; samplecount++)
	{
		// see whether attack or release are active & fetch the output scalar
		float envAmp = midistuff->processEnvelope(fades, currentNote);	// the attack/release scalar
		double envedTotalAmp = totalAmp * (double)envAmp;

		for (int bandcount=0; bandcount < numBands; bandcount++)
		{
			// filter using the input, delayed values, & their filter coefficients
			double curBandOutValue = (inputAmp[bandcount] * in[samplecount]) 
							   + (delay1amp[bandcount] * prevOut[bandcount]) 
							   - (delay2amp[bandcount] * prevprevOut[bandcount]);

			// add the latest resonator to the output collection, scaled by my evener & user gain
			out[samplecount] += (float) (curBandOutValue * envedTotalAmp);

			// very old outValue gets old outValue & old outValue gets current outValue (no longer current)
			prevprevOut[bandcount] = prevOut[bandcount];
			prevOut[bandcount] = curBandOutValue;
		}
	}
}
/*
void RezSynth::processFilterOuts(float *in, float *out, long sampleFrames, double ampEvener, 
							int currentNote, int numBands, double *prevOut, double *prevprevOut)
{
  NoteTable *currentNoteTableNote = &noteTable[currentNote];
  double curBandOutValue;	// holds the latest resonator's output value
  double totalAmp;
  long samplecount;
  int bandcount;
  NoteTable noteTemp;	// backup value holder for note envelope information


	totalAmp = ampEvener * (double)(gain*(noteTable[currentNote].noteAmp));

	// most of the note envelope values are liable to change below,
	// so we back them up to allow multi-band repetition
	noteTemp = noteTable[currentNote];

	// here we do the resonant filter equation using our filter coefficients, & related stuff
	for (bandcount=0; (bandcount < numBands); bandcount++)
	{
		// restore the note values before doing the next band
		noteTable[currentNote] = noteTemp;

		for (samplecount=0; (samplecount < sampleFrames); samplecount++)
		{
			// filter using the input, delayed values, & their filter coefficients
			curBandOutValue = (inputAmp[bandcount] * in[samplecount]) + (delay1amp[bandcount] * prevOut[bandcount]) - (delay2amp[bandcount] * prevprevOut[bandcount]);

			// add the latest resonator to the output collection, scaled by my evener & user gain
			out[samplecount] += (float) (curBandOutValue * totalAmp * (double)processEnvelope(currentNoteTableNote, fadeTable, fades));

			// very old outValue gets old outValue & old outValue gets current outValue (no longer current)
			prevprevOut[bandcount] = prevOut[bandcount];
			prevOut[bandcount] = curBandOutValue;
		}
	}
}
*/

//-----------------------------------------------------------------------------------------
// this function outputs the unprocessed audio input between notes, if desired
void RezSynth::processUnaffected(const float *in, float *out, long sampleFrames)
{
	for (long samplecount=0; samplecount < sampleFrames; samplecount++)
	{
		float unEnvAmp = 1.0f;

		// this is the state when all notes just ended & the clean input first kicks in
		if (unaffectedState == unFadeIn)
		{
			// linear fade-in
			unEnvAmp = (float)unaffectedFadeSamples * UNAFFECTED_FADE_STEP;
			unaffectedFadeSamples++;
			// go to the no-gain state if the fade-in is done
			if (unaffectedFadeSamples >= UNAFFECTED_FADE_DUR)
				unaffectedState = unFlat;
		}

		// a note has just begun, so we need to hasily fade out the clean input audio
		else if (unaffectedState == unFadeOut)
		{
			unaffectedFadeSamples--;
			// linear fade-out
			unEnvAmp = (float)unaffectedFadeSamples * UNAFFECTED_FADE_STEP;
			// get ready for the next time & exit this function if the fade-out is done
			if (unaffectedFadeSamples <= 0)
			{
				// ready for the next time
				unaffectedState = unFadeIn;
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

	// if this latest event is a note-on & this note isn't still active 
	// from being previously played, then clear this note's delay buffers
	if ( (midistuff->blockEvents[currentEvent].status == kMidiNoteOn) && 	// it's a note-on
	 		(midistuff->noteTable[currentNote].velocity == 0) )				// the note is currently off
	{
		// wipe out the feedback buffers
		for (unsigned ch=0; ch < numChannels; ch++)
		{
			int bandcount;
			for (bandcount=0; bandcount < MAX_BANDS; bandcount++)
				prevOutValue[ch][currentNote][bandcount] = 0.0;
			for (bandcount=0; bandcount < MAX_BANDS; bandcount++)
				prevprevOutValue[ch][currentNote][bandcount] = 0.0;
		}
	}
}
