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

#include "bufferoverride.h"

#include <cmath>



//-----------------------------------------------------------------------------
static inline long bufferSize_ms2samples(double inSizeMS, double inSampleRate)
{
	return (long) (inSizeMS * inSampleRate * 0.001);
}

//-----------------------------------------------------------------------------
void BufferOverride::updateBuffer(unsigned long samplePos)
{
	bool doSmoothing = true;	// but in some situations, we shouldn't
	bool barSync = false;	// true if we need to sync up with the next bar start


	// take care of MIDI
	heedBufferOverrideEvents(samplePos);

	readPos = 0;	// reset for starting a new minibuffer
	prevMinibufferSize = minibufferSize;
	const long prevForcedBufferSize = currentForcedBufferSize;

	//--------------------------PROCESS THE LFOs----------------------------
	// update the LFOs' positions to the current position
	divisorLFO->updatePosition(prevMinibufferSize);
	bufferLFO->updatePosition(prevMinibufferSize);
	// Then get the current output values of the LFOs, which also updates their positions once more.  
	// Scale the 0.0 - 1.0 LFO output values to 0.0 - 2.0 (oscillating around 1.0).
	const float divisorLFOvalue = divisorLFO->processLFOzero2two();
	float bufferLFOvalue = 2.0f - bufferLFO->processLFOzero2two();	// inverting it makes more pitch sense
	// and then update the stepSize for each LFO, in case the LFO parameters have changed
	if (divisorLFO->bTempoSync)
		divisorLFO->stepSize = currentTempoBPS * divisorLFO->fTempoRate * numLFOpointsDivSR;
	else
		divisorLFO->stepSize = divisorLFO->fRate * numLFOpointsDivSR;
	if (bufferLFO->bTempoSync)
		bufferLFO->stepSize = currentTempoBPS * bufferLFO->fTempoRate * numLFOpointsDivSR;
	else
		bufferLFO->stepSize = bufferLFO->fRate * numLFOpointsDivSR;

	//---------------------------CALCULATE FORCED BUFFER SIZE----------------------------
	// check if it's the end of this forced buffer
	if (writePos >= currentForcedBufferSize)
	{
		writePos = 0;	// start up a new forced buffer

		// check on the previous forced and minibuffers; don't smooth if the last forced buffer wasn't divided
		if (prevMinibufferSize >= currentForcedBufferSize)
			doSmoothing = false;
		else
			doSmoothing = true;

		// now update the the size of the current force buffer
		if ( bufferTempoSync &&	// the user wants to do tempo sync / beat division rate
			 (currentTempoBPS > 0.0) ) // avoid division by zero
		{
			currentForcedBufferSize = (long) ( getsamplerate() / (currentTempoBPS * bufferSizeSync) );
			// set this true so that we make sure to do the measure syncronisation later on
			if (needResync)
				barSync = true;
		}
		else
			currentForcedBufferSize = bufferSize_ms2samples(bufferSizeMs, getsamplerate());
		// apply the buffer LFO to the forced buffer size
		currentForcedBufferSize = (long) ((float)currentForcedBufferSize * bufferLFOvalue);
		// really low tempos and tempo rate values can cause huge forced buffer sizes,
		// so prevent going outside of the allocated buffer space
		if (currentForcedBufferSize > SUPER_MAX_BUFFER)
			currentForcedBufferSize = SUPER_MAX_BUFFER;
		if (currentForcedBufferSize < 2)
			currentForcedBufferSize = 2;

		// untrue this so that we don't do the measure sync calculations again unnecessarily
		needResync = false;
	}

	//-----------------------CALCULATE THE DIVISOR-------------------------
	currentBufferDivisor = divisor;
	// apply the divisor LFO to the divisor value if there's an "active" divisor (i.e. 2 or greater)
	if (currentBufferDivisor >= 2.0f)
	{
		currentBufferDivisor *= divisorLFOvalue;
		// now it's possible that the LFO could make the divisor less than 2, 
		// which will essentially turn the effect off, so we stop the modulation at 2
		if (currentBufferDivisor < 2.0f)
			currentBufferDivisor = 2.0f;
	}

	//-----------------------CALCULATE THE MINIBUFFER SIZE-------------------------
	// this is not a new forced buffer starting up
	if (writePos > 0)
	{
		// if it's allowed, update the minibuffer size midway through this forced buffer
		if (bufferInterrupt)
			minibufferSize = (long) ( (float)currentForcedBufferSize / currentBufferDivisor );
		// if it's the last minibuffer, then fill up the forced buffer to the end 
		// by extending this last minibuffer to fill up the end of the forced buffer
		const long remainingForcedBuffer = currentForcedBufferSize - writePos;
		if ( (minibufferSize * 2) >= remainingForcedBuffer )
			minibufferSize = remainingForcedBuffer;
	}
	// this is a new forced buffer just beginning, act accordingly, do bar sync if necessary
	else
	{
		const long samplesPerBar = lrint(static_cast<double>(timeinfo.samplesPerBeat) * timeinfo.numerator);
		long samplesToBar = timeinfo.samplesToNextBar - samplePos;
		while ((samplesToBar < 0) && (samplesPerBar > 0))
			samplesToBar += samplesPerBar;
		if (barSync)
		{
			// do beat sync for each LFO if it ought to be done
			if (divisorLFO->bTempoSync)
				divisorLFO->syncToTheBeat(samplesToBar);
			if (bufferLFO->bTempoSync)
				bufferLFO->syncToTheBeat(samplesToBar);
		}
		// because there isn't really any division (given my implementation) when the divisor is < 2
		if (currentBufferDivisor < 2.0f)
		{
			const long samplesToAlignForcedBufferToBar = samplesToBar % currentForcedBufferSize;
			if (barSync && (samplesToAlignForcedBufferToBar > 0))
				currentForcedBufferSize = samplesToAlignForcedBufferToBar;
			minibufferSize = currentForcedBufferSize;
		}
		else
		{
			minibufferSize = (long) ( (float)currentForcedBufferSize / currentBufferDivisor );
			if (barSync)
			{
				// calculate how long this forced buffer needs to be
				long countdown = samplesToBar % currentForcedBufferSize;
				// update the forced buffer size and number of minibuffers so that 
				// the forced buffers sync up with the musical measures of the song
				if ( countdown < (minibufferSize * 2) )	// extend the buffer if it would be too short...
					currentForcedBufferSize += countdown;
				else	// ...otherwise chop it down to the length of the extra bit needed to sync with the next measure
					currentForcedBufferSize = countdown;
			}
		}
	}
	// avoid madness such as 0-sized minibuffers
	if (minibufferSize < 1)
		minibufferSize = 1;

	//-----------------------CALCULATE SMOOTHING DURATION-------------------------
	// no smoothing if the previous forced buffer wasn't divided
	if (!doSmoothing)
		smoothcount = smoothDur = 0;
	else
	{
		smoothDur = static_cast<long>(smooth * static_cast<float>(minibufferSize));
		long maxSmoothDur = 0;
		// if we're just starting a new forced buffer, 
		// then the samples beyond the end of the previous one are not valid
		if (writePos <= 0)
			maxSmoothDur = prevForcedBufferSize - prevMinibufferSize;
		// otherwise just make sure that we don't go outside of the allocated arrays
		else
			maxSmoothDur = SUPER_MAX_BUFFER - prevMinibufferSize;
		if (smoothDur > maxSmoothDur)
			smoothDur = maxSmoothDur;
		smoothcount = smoothDur;
		smoothStep = 1.0f / static_cast<float>(smoothDur + 1);	// the gain increment for each smoothing step

//		sqrtFadeIn = std::sqrt(smoothStep);
//		sqrtFadeOut = std::sqrt(1.0f - smoothStep);
//		smoothFract = smoothStep;

		fadeOutGain = std::cos(kDFX_PI_f / static_cast<float>(4 * smoothDur));
		fadeInGain = std::sin(kDFX_PI_f / static_cast<float>(4 * smoothDur));
		realFadePart = (fadeOutGain * fadeOutGain) - (fadeInGain * fadeInGain);	// std::cos(3.141592/2.0/n)
		imaginaryFadePart = 2.0f * fadeOutGain * fadeInGain;	// std::sin(3.141592/2.0/n)
	}
}



//---------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------
void BufferOverride::processaudio(const float ** in, float ** out, unsigned long inNumFrames, bool replacing)
{
	const unsigned long numChannels = getnumoutputs();
	unsigned long ch;
	const float oldDivisor = divisor;

//-------------------------SAFETY CHECK----------------------
	// there must have not been available memory or something (like WaveLab goofing up), 
	// so try to allocate buffers now
	if (numBuffers < numChannels)
		createbuffers();
	for (ch = 0; ch < numChannels; ch++)
	{
		if (buffers[ch] == NULL)
		{
			// exit the loop if creation succeeded
			if ( createbuffers() )
				break;
			// or abort audio processing if the creation failed
			else
				return;
		}
	}


//-------------------------INITIALIZATIONS----------------------
	// this is a handy value to have during LFO calculations and wasteful to recalculate at every sample
	numLFOpointsDivSR = NUM_LFO_POINTS_FLOAT / getsamplerate_f();
	divisorLFO->pickTheLFOwaveform();
	bufferLFO->pickTheLFOwaveform();

	// calculate this scaler value to minimize calculations later during processOutput()
	// (square root for equal power mix)
	inputGain.setValue(std::sqrt(1.0f - dryWetMix));
	outputGain.setValue(std::sqrt(dryWetMix));


//-----------------------TEMPO STUFF---------------------------
	// figure out the current tempo if we're doing tempo sync
	if ( bufferTempoSync || 
			(divisorLFO->bTempoSync || bufferLFO->bTempoSync) )
	{
		// calculate the tempo at the current processing buffer
		if ( useHostTempo && hostCanDoTempo && timeinfo.tempoIsValid )	// get the tempo from the host
		{
			currentTempoBPS = timeinfo.tempo_bps;
			// check if audio playback has just restarted and reset buffer stuff if it has (for measure sync)
			if (timeinfo.playbackChanged)
			{
				needResync = true;
				currentForcedBufferSize = 1;
				writePos = 1;
				minibufferSize = 1;
				prevMinibufferSize = 0;
				smoothcount = smoothDur = 0;
			}
		}
		else	// get the tempo from the user parameter
		{
			currentTempoBPS = userTempo / 60.0;
			needResync = false;	// we don't want it true if we're not syncing to host tempo
		}
	}


//-----------------------AUDIO STUFF---------------------------
	// here we begin the audio output loop, which has two checkpoints at the beginning
	for (unsigned long samplecount = 0; samplecount < inNumFrames; samplecount++)
	{
		// check if it's the end of this minibuffer
		if (readPos >= minibufferSize)
			updateBuffer(samplecount);

		// store the latest input samples into the buffers
		for (ch = 0; ch < numChannels; ch++)
			buffers[ch][writePos] = in[ch][samplecount];

		// get the current output without any smoothing
		for (ch = 0; ch < numChannels; ch++)
			outval[ch] = buffers[ch][readPos];

		// and if smoothing is taking place, get the smoothed audio output
		if (smoothcount > 0)
		{
			for (ch = 0; ch < numChannels; ch++)
			{
			// crossfade between the current input and its corresponding overlap sample
//				outval[ch] *= 1.0f - (smoothStep * (float)smoothcount);	// current
//				outval[ch] += buffers[ch][readPos + prevMinibufferSize] * smoothStep * (float)smoothcount;	// + previous
//				const float smoothfract = smoothStep * (float)smoothcount;
//				const float newgain = std::sqrt(1.0f - smoothfract);
//				const float oldgain = std::sqrt(smoothfract);
//				outval[ch] = (outval[ch] * newgain) + (buffers[ch][readPos+prevMinibufferSize] * oldgain);
//				outval[ch] = (outval[ch] * sqrtFadeIn) + (buffers[ch][readPos+prevMinibufferSize] * sqrtFadeOut);
				outval[ch] = (outval[ch] * fadeInGain) + (buffers[ch][readPos+prevMinibufferSize] * fadeOutGain);
			}
			smoothcount--;
//			smoothFract += smoothStep;
//			sqrtFadeIn = 0.5f * (sqrtFadeIn + (smoothFract / sqrtFadeIn));
//			sqrtFadeOut = 0.5f * (sqrtFadeOut + ((1.0f - smoothFract) / sqrtFadeOut));
			fadeInGain = (fadeOutGain * imaginaryFadePart) + (fadeInGain * realFadePart);
			fadeOutGain = (realFadePart * fadeOutGain) - (imaginaryFadePart * fadeInGain);
		}

		// write the output samples into the output stream
	#ifdef TARGET_API_VST
		if (replacing)
		{
	#endif
			for (ch = 0; ch < numChannels; ch++)
				out[ch][samplecount] = (outval[ch] * outputGain.getValue()) + (in[ch][samplecount] * inputGain.getValue());
	#ifdef TARGET_API_VST
		}
		else
		{
			for (ch = 0; ch < numChannels; ch++)
				out[ch][samplecount] += (outval[ch] * outputGain.getValue()) + (in[ch][samplecount] * inputGain.getValue());
		}
	#endif

		// increment the position trackers
		readPos++;
		writePos++;

		inputGain.inc();
		outputGain.inc();
	}


//-----------------------MIDI STUFF---------------------------
	// check to see if there may be a note or pitchbend message left over that hasn't been implemented
	if (midistuff->numBlockEvents > 0)
	{
		for (long eventcount = 0; eventcount < midistuff->numBlockEvents; eventcount++)
		{
			if (isNote(midistuff->blockEvents[eventcount].status))
			{
				// regardless of whether it's a note-on or note-off, we've found some note message
				oldNote = true;
				// store the note and update the notes table if it's a note-on message
				if (midistuff->blockEvents[eventcount].status == kMidiNoteOn)
				{
					midistuff->insertNote(midistuff->blockEvents[eventcount].byte1);
					lastNoteOn = midistuff->blockEvents[eventcount].byte1;
					// since we're not doing the divisor updating yet, this needs to be falsed
					divisorWasChangedByHand = false;
				}
				// otherwise remove the note from the notes table
				else
					midistuff->removeNote(midistuff->blockEvents[eventcount].byte1);
			}
			else if (midistuff->blockEvents[eventcount].status == kMidiCC)
			{
				if (midistuff->blockEvents[eventcount].byte1 == kMidiCC_AllNotesOff)
				{
					oldNote = true;
					midistuff->removeAllNotes();
				}
			}
		}
		for (long eventcount = midistuff->numBlockEvents - 1; eventcount >= 0; eventcount--)
		{
			if (midistuff->blockEvents[eventcount].status == kMidiPitchbend)
			{
				// set this pitchbend message as lastPitchbend
				lastPitchbend = midistuff->blockEvents[eventcount].byte2;
				break;	// leave this for loop
			}
		}
	}

	// make our parameters storers and the host aware that divisor changed because of MIDI
	if (divisor != oldDivisor)
	{
		setparameter_f(kDivisor, divisor);	// XXX eh?
		postupdate_parameter(kDivisor);	// inform listeners of change
	}
}
