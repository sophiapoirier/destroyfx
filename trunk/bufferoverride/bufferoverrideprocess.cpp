/*------------------- by Marc Poirier  ][  March 2001 -------------------*/

#ifndef __BUFFEROVERRIDE_H
#include "bufferoverride.hpp"
#endif



//-----------------------------------------------------------------------------
void BufferOverride::updateBuffer(unsigned long samplePos)
{
  bool doSmoothing = true;	// but in some situations, we shouldn't
  bool barSync = false;	// true if we need to sync up with the next bar start
  float divisorLFOvalue, bufferLFOvalue;	// the current output values of the LFOs
  long prevForcedBufferSize;	// the previous forced buffer size


	// take care of MIDI
	heedBufferOverrideEvents(samplePos);

	readPos = 0;	// reset for starting a new minibuffer
	prevMinibufferSize = minibufferSize;
	prevForcedBufferSize = currentForcedBufferSize;

	//--------------------------PROCESS THE LFOs----------------------------
	// update the LFOs' positions to the current position
	divisorLFO->updatePosition(prevMinibufferSize);
	bufferLFO->updatePosition(prevMinibufferSize);
	// Then get the current output values of the LFOs, which also updates their positions once more.  
	// Scale the 0.0 - 1.0 LFO output values to 0.0 - 2.0 (oscillating around 1.0).
	divisorLFOvalue = processLFOzero2two(divisorLFO);
	bufferLFOvalue = 2.0f - processLFOzero2two(bufferLFO);	// inverting it makes more pitch sense
	// & then update the stepSize for each LFO, in case the LFO parameters have changed
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

		// check on the previous forced & minibuffers; don't smooth if the last forced buffer wasn't divided
		if (prevMinibufferSize >= currentForcedBufferSize)
			doSmoothing = false;
		else
			doSmoothing = true;

		// now update the the size of the current force buffer
		if ( bufferTempoSync &&	// the user wants to do tempo sync / beat division rate
			 (currentTempoBPS > 0.0f) ) // avoid division by zero
		{
			currentForcedBufferSize = (long) ( getsamplerate_f() / (currentTempoBPS * bufferSizeSync) );
			// set this true so that we make sure to do the measure syncronisation later on
			if (needResync)
				barSync = true;
		}
		else
			currentForcedBufferSize = (long) bufferSize_ms2samples(bufferSizeMs);
		// apply the buffer LFO to the forced buffer size
		currentForcedBufferSize = (long) ((float)currentForcedBufferSize * bufferLFOvalue);
		// really low tempos & tempo rate values can cause huge forced buffer sizes,
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
		long remainingForcedBuffer = currentForcedBufferSize - writePos;
		if ( (minibufferSize*2) >= remainingForcedBuffer )
			minibufferSize = remainingForcedBuffer;
	}
	// this is a new forced buffer just beginning, act accordingly, do bar sync if necessary
	else
	{
		long samplesToBar = timeinfo.samplesToNextBar;
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
			if (barSync)
				minibufferSize = currentForcedBufferSize = samplesToBar % currentForcedBufferSize;
			else
				minibufferSize = currentForcedBufferSize;
		}
		else
		{
			minibufferSize = (long) ( (float)currentForcedBufferSize / currentBufferDivisor );
			if (barSync)
			{
				// calculate how long this forced buffer needs to be
				long countdown = samplesToBar % currentForcedBufferSize;
				// update the forced buffer size & number of minibuffers so that 
				// the forced buffers sync up with the musical measures of the song
				if ( countdown < (minibufferSize*2) )	// extend the buffer if it would be too short...
					currentForcedBufferSize += countdown;
				else	// ...otherwise chop it down to the length of the extra bit needed to sync with the next measure
					currentForcedBufferSize = countdown;
			}
		}
	}

	//-----------------------CALCULATE SMOOTHING DURATION-------------------------
	// no smoothing if the previous forced buffer wasn't divided
	if (!doSmoothing)
		smoothcount = smoothDur = 0;
	else
	{
		smoothDur = (long) (smooth * (float)minibufferSize);
		long maxSmoothDur;
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
		smoothStep = 1.0f / (float)(smoothDur+1);	// the gain increment for each smoothing step

//		sqrtFadeIn = sqrtf(smoothStep);
//		sqrtFadeOut = sqrtf(1.0f - smoothStep);
//		smoothFract = smoothStep;

		fadeOutGain = cosf(PI/(float)(4*smoothDur));
		fadeInGain = sinf(PI/(float)(4*smoothDur));
		realFadePart = (fadeOutGain * fadeOutGain) - (fadeInGain * fadeInGain);	// cosf(3.141592/2/n)
		imaginaryFadePart = 2.0f * fadeOutGain * fadeInGain;	// sinf(3.141592/2/n)
	}
}



//---------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------
void BufferOverride::processaudio(const float **in, float **out, unsigned long inNumFrames, bool replacing)
{
	unsigned long numChannels = getnumoutputs();
	unsigned long ch;
	float oldDivisor = divisor;

//-------------------------SAFETY CHECK----------------------
	// there must have not been available memory or something (like WaveLab goofing up), 
	// so try to allocate buffers now
	if (numBuffers < numChannels)
		createbuffers();
	for (ch=0; ch < numChannels; ch++)
	{
		if (buffers[ch] == NULL)
		{
			// exit the loop if creation succeeded
			if ( createbuffers() )
				break;
			// or abort audio processing if the creation failed
			else return;
		}
	}


//-------------------------INITIALIZATIONS----------------------
	// this is a handy value to have during LFO calculations & wasteful to recalculate at every sample
	numLFOpointsDivSR = NUM_LFO_POINTS_FLOAT / getsamplerate_f();
	divisorLFO->pickTheLFOwaveform();
	bufferLFO->pickTheLFOwaveform();

	// calculate this scaler value to minimize calculations later during processOutput()
	// (square root for equal power mix)
	float inputGain = sqrtf(1.0f - dryWetMix);
	float outputGain = sqrtf(dryWetMix);


//-----------------------TEMPO STUFF---------------------------
	// figure out the current tempo if we're doing tempo sync
	if ( bufferTempoSync || 
			(divisorLFO->bTempoSync || bufferLFO->bTempoSync) )
	{
		// calculate the tempo at the current processing buffer
		if ( useHostTempo && hostCanDoTempo && timeinfo.tempoIsValid )	// get the tempo from the host
		{
			currentTempoBPS = timeinfo.tempo_bps;
			// check if audio playback has just restarted & reset buffer stuff if it has (for measure sync)
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
			currentTempoBPS = userTempo / 60.0f;
			needResync = false;	// we don't want it true if we're not syncing to host tempo
		}
	}


//-----------------------AUDIO STUFF---------------------------
	// here we begin the audio output loop, which has two checkpoints at the beginning
	for (unsigned long samplecount=0; samplecount < inNumFrames; samplecount++)
	{
		// check if it's the end of this minibuffer
		if (readPos >= minibufferSize)
			updateBuffer(samplecount);

		// store the latest input samples into the buffers
		for (ch=0; ch < numChannels; ch++)
			buffers[ch][writePos] = in[ch][samplecount];

		// get the current output without any smoothing
		for (ch=0; ch < numChannels; ch++)
			outval[ch] = buffers[ch][readPos];

		// and if smoothing is taking place, get the smoothed audio output
		if (smoothcount > 0)
		{
			for (ch=0; ch < numChannels; ch++)
			{
			// crossfade between the current input & its corresponding overlap sample
//				outval[ch] *= 1.0f - (smoothStep * (float)smoothcount);	// current
//				outval[ch] += buffers[ch][readPos+prevMinibufferSize] * smoothStep*(float)smoothcount;	// + previous
//				float smoothfract = smoothStep * (float)smoothcount;
//				float newgain = sqrt(1.0f - smoothfract);
//				float oldgain = sqrt(smoothfract);
//				outval[ch] = (outval[ch] * newgain) + (buffers[ch][readPos+prevMinibufferSize] * oldgain);
//				outval[ch] = (outval[ch] * sqrtFadeIn) + (buffers[ch][readPos+prevMinibufferSize] * sqrtFadeOut);
				outval[ch] = (outval[ch] * fadeInGain) + (buffers[ch][readPos+prevMinibufferSize] * fadeOutGain);
			}
			smoothcount--;
//			smoothFract += smoothStep;
//			sqrtFadeIn = 0.5f * (sqrtFadeIn + (smoothFract / sqrtFadeIn));
//			sqrtFadeOut = 0.5f * (sqrtFadeOut + ((1.0f-smoothFract) / sqrtFadeOut));
			fadeInGain = (fadeOutGain * imaginaryFadePart) + (fadeInGain * realFadePart);
			fadeOutGain = (realFadePart * fadeOutGain) - (imaginaryFadePart * fadeInGain);
		}

		// write the output samples into the output stream
	#if TARGET_API_VST
		if (replacing)
		{
	#endif
			for (ch=0; ch < numChannels; ch++)
				out[ch][samplecount] = (outval[ch] * outputGain) + (in[ch][samplecount] * inputGain);
	#if TARGET_API_VST
		}
		else
		{
			for (ch=0; ch < numChannels; ch++)
				out[ch][samplecount] += (outval[ch] * outputGain) + (in[ch][samplecount] * inputGain);
		}
	#endif

		// increment the position trackers
		readPos++;
		writePos++;
	}


//-----------------------MIDI STUFF---------------------------
	// check to see if there may be a note or pitchbend message left over that hasn't been implemented
	if (midistuff->numBlockEvents > 0)
	{
		long eventcount;
		for (eventcount = 0; eventcount < midistuff->numBlockEvents; eventcount++)
		{
			if (isNote(midistuff->blockEvents[eventcount].status))
			{
				// regardless of whether it's a note-on or note-off, we've found some note message
				oldNote = true;
				// store the note & update the notes table if it's a note-on message
				if (midistuff->blockEvents[eventcount].status == kMidiNoteOn)
				{
					midistuff->insertNote(midistuff->blockEvents[eventcount].byte1);
					lastNoteOn = midistuff->blockEvents[eventcount].byte1;
					// since we're not doing the fDivisor updating yet, this needs to be falsed
					divisorWasChangedByHand = false;
				}
				// otherwise remove the note from the notes table
				else
					midistuff->removeNote(midistuff->blockEvents[eventcount].byte1);
			}
			else if (midistuff->blockEvents[eventcount].status == kMidiCC_AllNotesOff)
			{
				oldNote = true;
				midistuff->removeAllNotes();
			}
		}
		for (eventcount = (midistuff->numBlockEvents-1); (eventcount >= 0); eventcount--)
		{
			if (midistuff->blockEvents[eventcount].status == kMidiPitchbend)
			{
				// set this pitchbend message as lastPitchbend
				lastPitchbend = midistuff->blockEvents[eventcount].byte2;
				break;	// leave this for loop
			}
		}
	}

	// make the our parameters storers and the host aware that divisor changed because of MIDI
	if (divisor != oldDivisor)
		setparameter_f(kDivisor, divisor);	// XXX eh?
}
