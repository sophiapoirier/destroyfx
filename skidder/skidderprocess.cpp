/*-------------- by Marc Poirier  ][  December 2000 -------------*/

#ifndef __skidder
#include "skidder.hpp"
#endif

#include <stdlib.h>
#include <math.h>


//-----------------------------------------------------------------------------------------
void Skidder::processSlopeIn()
{
	// dividing the growing slopeDur-slopeSamples by slopeDur makes ascending values
#ifdef MSKIDDER
	if (MIDIin)
	{
		if (midiModeScaled(fMidiMode) == kMidiTrigger)
			// start from a 0.0 floor if we are coming in from silence
			amp = ((float)(slopeDur-slopeSamples)) * slopeStep;
		else if (midiModeScaled(fMidiMode) == kMidiApply)
			// no fade-in for the first entry of MIDI apply
			amp = 1.0f;
	}
	else
#endif
	if (useRandomFloor)
		amp = ( ((float)(slopeDur-slopeSamples)) * slopeStep * randomGainRange ) + randomFloor;
	else
		amp = ( ((float)(slopeDur-slopeSamples)) * slopeStep * gainRange ) + floor;

	slopeSamples--;

	if (slopeSamples <= 0)
	{
		state = plateau;
		MIDIin = false;	// make sure it doesn't happen again
	}
}

//-----------------------------------------------------------------------------------------
void Skidder::processPlateau()
{
#ifdef MSKIDDER
	MIDIin = false;	// in case there was no slope-in
#endif

	// amp in the plateau is 1.0, i.e. this sample is unaffected
	amp = 1.0f;

	plateauSamples--;

	if (plateauSamples <= 0)
	{
		// average & then sqare the sample squareroots for the RMS value
		rms = powf((rms/(float)rmscount), 2.0f);
		// because RMS tends to be < 0.5, thus unfairly limiting rupture's range
		rms *= 2.0f;
		// avoids clipping or unexpected values (like wraparound)
		if ( (rms > 1.0f) || (rms < 0.0f) )
			rms = 1.0f;
		rmscount = 0;	// reset the RMS counter
		//
		// set up the random floor values
		useRandomFloor = fFloorRandMin < fFloor;
		randomFloor = ( ((float)rand()*ONE_DIV_RAND_MAX) * gainScaled(fFloor-fFloorRandMin) ) 
							+ gainScaled(fFloorRandMin);
		randomGainRange = 1.0f - randomFloor;	// the range of the skidding on/off gain
		//
		if (slopeDur > 0)
		{
			state = slopeOut;
			slopeSamples = slopeDur; // refill slopeSamples
			slopeStep = 1.0f / (float)slopeDur;	// calculate the fade increment scalar
		}
		else
			state = valley;
	}
}


//-----------------------------------------------------------------------------------------
void Skidder::processSlopeOut()
{
	// dividing the decrementing slopeSamples by slopeDur makes descending values
#ifdef MSKIDDER
	if ( (MIDIout) && (midiModeScaled(fMidiMode) == kMidiTrigger) )
		// start from a 0.0 floor if we are coming in from silence
		amp = ((float)slopeSamples) * slopeStep;
	else
#endif
	if (useRandomFloor)
		amp = ( ((float)slopeSamples) * slopeStep * randomGainRange ) + randomFloor;
	else
		amp = ( ((float)slopeSamples) * slopeStep * gainRange ) + floor;

	slopeSamples--;

	if (slopeSamples <= 0)
	{
		state = valley;
		MIDIout = false;	// make sure it doesn't happen again
	}
}

//-----------------------------------------------------------------------------------------
void Skidder::processValley(float SAMPLERATE)
{
  float rateRandFactor = rateRandFactorScaled(fRateRandFactor);	// stores the real value
  float cycleRate;	// the base current skid rate value
  float randFloat, randomRate;	// the current randomized rate value
  float fPulsewidthRandomized;	// stores the latest randomized pulsewidth 0.0 - 1.0 value
  bool barSync = false;	// true if we need to sync up with the next bar start
  long countdown;


#ifdef MSKIDDER
	if (MIDIin)
	{
		if (midiModeScaled(fMidiMode) == kMidiTrigger)
			// there's one sample of valley when trigger mode begins, so silence that one
			amp = 0.0f;
		else if (midiModeScaled(fMidiMode) == kMidiApply)
			// there's one sample of valley when apply mode begins, so keep it at full gain
			amp = 1.0f;
	}
	// otherwise amp in the valley is whatever the floor gain is, the lowest gain value
	else
#endif
	if (useRandomFloor)
		amp = randomFloor;
	else
		amp = floor;

	valleySamples--;

	if (valleySamples <= 0)
	{
		rms = 0.0f;	// reset rms now because valley is over
		//
		// This is where we figure out how many samples long each 
		// envelope section is for the next skid cycle.
		//
		if (onOffTest(fTempoSync))	// the user wants to do tempo sync / beat division rate
		{
			cycleRate = currentTempoBPS * (tempoRateTable->getScalar(fTempoRate));
			// set this true so that we make sure to do the measure syncronisation later on
			if ( needResync && (midiModeScaled(fMidiMode) == kNoMidiMode) )
				barSync = true;
		}
		else
			cycleRate = rateScaled(fRate);
		needResync = false;	// reset this so that we don't have any trouble
		//
		if (fRateRandFactor > 0.0f)
		{
			// get a random value from 0.0 to 1.0
			randFloat = (float)rand() * ONE_DIV_RAND_MAX;
			// square-scale the random value & then scale it with the random rate range
			randomRate = ( randFloat * randFloat * 
							((cycleRate*rateRandFactor)-(cycleRate/rateRandFactor)) ) + 
							(cycleRate/rateRandFactor);
			cycleSamples = (long) (SAMPLERATE / randomRate);
			barSync = false;	// we can't do the bar sync if the skids durations are random
		}
		else
			cycleSamples = (long) (SAMPLERATE / cycleRate);
		//
		if (fPulsewidth > fPulsewidthRandMin)
		{
			fPulsewidthRandomized = ( ((float)rand()*ONE_DIV_RAND_MAX) * (fPulsewidth-fPulsewidthRandMin) ) + fPulsewidthRandMin;
			pulseSamples = (long) ( ((float)cycleSamples) * pulsewidthScaled(fPulsewidthRandomized) );
		}
		else
			pulseSamples = (long) ( ((float)cycleSamples) * pulsewidthScaled(fPulsewidth) );
		valleySamples = cycleSamples - pulseSamples;
		slopeSamples = (long) ((SAMPLERATE/1000.0f)*(fSlope*(SLOPEMAX)));
		slopeDur = slopeSamples;
		slopeStep = 1.0f / (float)slopeDur;	// calculate the fade increment scalar
		plateauSamples = pulseSamples - (slopeSamples * 2);
		if (plateauSamples < 1)	// this shrinks the slope to 1/3 of the pulse if the user sets slope too high
		{
			slopeSamples = (long) (((float)pulseSamples) / 3.0f);
			slopeDur = slopeSamples;
			slopeStep = 1.0f / (float)slopeDur;	// calculate the fade increment scalar
			plateauSamples = pulseSamples - (slopeSamples * 2);
		}

		// go to slopeIn next if slope is not 0.0, otherwise go to plateau
		if (slopeDur > 0)
			state = slopeIn;
		else
			state = plateau;

		if (barSync)	// we need to adjust this cycle so that a skid syncs with the next bar
		{
			// calculate how long this skid cycle needs to be
			countdown = samplesToNextBar(timeInfo) % cycleSamples;
			// skip straight to the valley & adjust its length
			if ( countdown <= (valleySamples+(slopeSamples*2)) )
			{
				valleySamples = countdown;
				state = valley;
			}
			// otherwise adjust the plateau if the shortened skid is still long enough for one
			else
				plateauSamples -= cycleSamples - countdown;
		}

	#ifdef MSKIDDER
		// if MIDI apply mode is just beginning, make things smooth with no panning
		if ( (MIDIin) && (midiModeScaled(fMidiMode) == kMidiApply) )
			panRander = 0.0f;
		else
	#endif
		// this puts random float values from -1.0 to 1.0 into panRander
		panRander = ( ((float)rand()*ONE_DIV_RAND_MAX) * 2.0f ) - 1.0f;

	} //end of the "valley is over" if-statement
}

//-----------------------------------------------------------------------------------------
float Skidder::processOutput(float in1, float in2, float pan)
{
	// output noise
	if ( (state == valley) && (fNoise != 0.0f) )
		// out gets random noise with samples from -1.0 to 1.0 times the random pan times rupture times the RMS scalar
		return ((((float)rand()*ONE_DIV_RAND_MAX)*2.0f)-1.0f) * pan * fNoise_squared * rms;

	// do regular skidding output
	else
	{
		// only output a bit of the first input
		if (pan <= 1.0f)
			return in1 * pan * amp;
		// output all of the first input & a bit of the second input
		else
			return ( in1 + (in2*(pan-1.0f)) ) * amp;
	}
}

//-----------------------------------------------------------------------------------------
void Skidder::doTheProcess(float **inputs, float **outputs, long sampleFrames, bool replacing)
{
/* begin inter-plugin audio sharing stuff */
#ifdef HUNGRY
	if ( ! (foodEater->setupProcess(inputs, sampleFrames)) )
		return;
#endif
/* end inter-plugin audio sharing stuff */

  float *in1  = inputs[0];
  float *in2  = inputs[1];
  float *out1 = outputs[0];
  float *out2 = outputs[1];

  float SAMPLERATE = getSampleRate();
	// just in case the host responds with something wacky
	if (SAMPLERATE <= 0.0f)   SAMPLERATE = 44100.0f;

  long samplecount;


	floor = gainScaled(fFloor);	// the parameter scaled real value
	gainRange = 1.0f - floor;	// the range of the skidding on/off gain
	useRandomFloor = (fFloorRandMin < fFloor);


// ---------- begin MIDI stuff --------------
#ifdef MSKIDDER
  bool noteIsOn = false;

	for (int notecount=0; (notecount < NUM_NOTES); notecount++)
	{
		if (noteTable[notecount])
		{
			noteIsOn = true;
			break;	// we only need to find one active note
		}
	}

	switch (midiModeScaled(fMidiMode))
	{
		case kMidiTrigger:
			// check waitSamples also because, if it's zero, we can just move ahead normally
			if ( (noteIsOn) && (waitSamples) )
			{
				// cut back the number of samples outputted
				sampleFrames -= waitSamples;
				// & jump ahead accordingly in the i/o streams
				in1 += waitSamples;
				in2 += waitSamples;
				out1 += waitSamples;
				out2 += waitSamples;

				// need to make sure that the skipped part is silent if we're processReplacing
				if (replacing)
				{
					for (samplecount = 0; (samplecount < waitSamples); samplecount++)
						outputs[0][samplecount] = 0.0f;
					for (samplecount = 0; (samplecount < waitSamples); samplecount++)
						outputs[1][samplecount] = 0.0f;
				}

				// reset
				waitSamples = 0;
			}

			else if (!noteIsOn)
			{
				// if Skidder currently is in the plateau & has a slow cycle, this could happen
				if (waitSamples > sampleFrames)
					waitSamples -= sampleFrames;
				else
				{
					if (replacing)
					{
						for (samplecount = waitSamples; (samplecount < sampleFrames); samplecount++)
							outputs[0][samplecount] = 0.0f;
						for (samplecount = waitSamples; (samplecount < sampleFrames); samplecount++)
							outputs[1][samplecount] = 0.0f;
					}
					sampleFrames = waitSamples;
					waitSamples = 0;
				}
			}

			// adjust the floor according to note velocity if velocity mode is on
			if (onOffTest(fVelocity))
			{
				floor = gainScaled((float)(127-mostRecentVelocity)/127.0f);
				gainRange = 1.0f - floor;	// the range of the skidding on/off gain
				useRandomFloor = false;
			}

			break;

		case kMidiApply:
			// check waitSamples also because, if it's zero, we can just move ahead normally
			if ( (noteIsOn) && (waitSamples) )
			{
				// cut back the number of samples outputted
				sampleFrames -= waitSamples;
				// & jump ahead accordingly in the i/o streams
				in1 += waitSamples;
				in2 += waitSamples;
				out1 += waitSamples;
				out2 += waitSamples;

				// need to make sure that the skipped part is unprocessed audio
				if (replacing)
				{
					for (samplecount = 0; (samplecount < waitSamples); samplecount++)
						outputs[0][samplecount] = inputs[0][samplecount];
					for (samplecount = 0; (samplecount < waitSamples); samplecount++)
						outputs[1][samplecount] = inputs[1][samplecount];
				}
				else
				{
					for (samplecount = 0; (samplecount < waitSamples); samplecount++)
						outputs[0][samplecount] += inputs[0][samplecount];
					for (samplecount = 0; (samplecount < waitSamples); samplecount++)
						outputs[1][samplecount] += inputs[1][samplecount];
				}

				// reset
				waitSamples = 0;
			}

			else if (!noteIsOn)
			{
				// if Skidder currently is in the plateau & has a slow cycle, this could happen
				if (waitSamples)
				{
					if (waitSamples > sampleFrames)
						waitSamples -= sampleFrames;
					else
					{
						if (replacing)
						{
							for (samplecount = waitSamples; (samplecount < sampleFrames); samplecount++)
								outputs[0][samplecount] = inputs[0][samplecount];
							for (samplecount = waitSamples; (samplecount < sampleFrames); samplecount++)
								outputs[1][samplecount] = inputs[1][samplecount];
						}
						else
						{
							for (samplecount = waitSamples; (samplecount < sampleFrames); samplecount++)
								outputs[0][samplecount] += inputs[0][samplecount];
							for (samplecount = waitSamples; (samplecount < sampleFrames); samplecount++)
								outputs[1][samplecount] += inputs[1][samplecount];
						}
						sampleFrames = waitSamples;
						waitSamples = 0;
					}
				}
				else
				{
					if (replacing)
					{
						for (samplecount = 0; (samplecount < sampleFrames); samplecount++)
							outputs[0][samplecount] = inputs[0][samplecount];
						for (samplecount = 0; (samplecount < sampleFrames); samplecount++)
							outputs[1][samplecount] = inputs[1][samplecount];
					}
					else
					{
						for (samplecount = 0; (samplecount < sampleFrames); samplecount++)
							outputs[0][samplecount] += inputs[0][samplecount];
						for (samplecount = 0; (samplecount < sampleFrames); samplecount++)
							outputs[1][samplecount] += inputs[1][samplecount];
					}
					// that's all we need to do if there are no notes, 
					// just copy the input to the output
					return;
				}
			}

			// adjust the floor according to note velocity if velocity mode is on
			if (onOffTest(fVelocity))
			{
				floor = gainScaled((float)(127-mostRecentVelocity)/127.0f);
				gainRange = 1.0f - floor;	// the range of the skidding on/off gain
				useRandomFloor = false;
			}

			break;

		default:
			break;
	}
#endif
// ---------- end MIDI stuff --------------


	// figure out the current tempo if we're doing tempo sync
	if (onOffTest(fTempoSync))
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
//					currentTempoBPS = ((float)tempoAt(reportCurrentPosition())) / 600000.0f;
				else
					currentTempoBPS = tempoScaled(fTempo) / 60.0f;
				// but zero & negative tempos are bad, so get the user tempo value instead if that happens
				if (currentTempoBPS <= 0.0f)
					currentTempoBPS = tempoScaled(fTempo) / 60.0f;
				//
				// check if audio playback has just restarted & reset cycle state stuff if it has (for measure sync)
				if (timeInfo->flags & kVstTransportChanged)
				{
					needResync = true;
					state = valley;
					valleySamples = 0;
				}
			}
			else	// do the same stuff as above if the timeInfo gets a null pointer
			{
				currentTempoBPS = tempoScaled(fTempo) / 60.0f;
				needResync = false;	// we don't want it true if we're not syncing to host tempo
			}
		}
		//
		// this is for notifying the GUI about updating the rate random range display
		if ( (currentTempoBPS != oldTempoBPS) || (mustUpdateTempoHasChanged) )
		{
			tempoHasChanged = true;
			mustUpdateTempoHasChanged = false;	// reset it now
		}
		oldTempoBPS = currentTempoBPS;
	}

	for (samplecount=0; (samplecount < sampleFrames); samplecount++)
	{
		switch (state)
		{
			case slopeIn:
				// get the average sqareroot of the current input samples
				rms += sqrtf( fabsf(((*in1)+(*in2))*0.5f) );
				rmscount++;	// this counter is later used for getting the mean
				processSlopeIn();
				break;
			case plateau:
				rms += sqrtf( fabsf(((*in1)+(*in2))*0.5f) );
				rmscount++;
				processPlateau();
				break;
			case slopeOut:
				processSlopeOut();
				break;
			case valley:
				processValley(SAMPLERATE);
				break;
		}

		// ((panRander*fPan)+1.0) ranges from 0.0 to 2.0
		if (replacing)
		{
			*out1 = processOutput( *in1, *in2, ((panRander*fPan)+1.0f) );
			*out2 = processOutput( *in2, *in1, (2.0f - ((panRander*fPan)+1.0f)) );
		}
		else
		{
			*out1 += processOutput( *in1, *in2, ((panRander*fPan)+1.0f) );
			*out2 += processOutput( *in2, *in1, (2.0f - ((panRander*fPan)+1.0f)) );
		}
		// move forward in the i/o sample streams
		in1++;
		in2++;
		out1++;
		out2++;
	}
}

//-----------------------------------------------------------------------------------------
void Skidder::process(float **inputs, float **outputs, long sampleFrames)
{
	doTheProcess(inputs, outputs, sampleFrames, false);
}

//-----------------------------------------------------------------------------------------
void Skidder::processReplacing(float **inputs, float **outputs, long sampleFrames)
{
	doTheProcess(inputs, outputs, sampleFrames, true);
}
