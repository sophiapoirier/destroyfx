/*-------------- by Marc Poirier  ][  December 2000 -------------*/

#include "skidder.hpp"


//-----------------------------------------------------------------------------------------
void Skidder::processSlopeIn()
{
	// dividing the growing slopeDur-slopeSamples by slopeDur makes ascending values
	if (MIDIin)
	{
		if (midiMode == kMidiMode_trigger)
			// start from a 0.0 floor if we are coming in from silence
			sampleAmp = ((float)(slopeDur-slopeSamples)) * slopeStep;
		else if (midiMode == kMidiMode_apply)
			// no fade-in for the first entry of MIDI apply
			sampleAmp = 1.0f;
	}
	else if (useRandomFloor)
		sampleAmp = ( ((float)(slopeDur-slopeSamples)) * slopeStep * randomGainRange ) + randomFloor;
	else
		sampleAmp = ( ((float)(slopeDur-slopeSamples)) * slopeStep * gainRange ) + floor;

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
	MIDIin = false;	// in case there was no slope-in

	// sampleAmp in the plateau is 1.0, i.e. this sample is unaffected
	sampleAmp = 1.0f;

	plateauSamples--;

	if (plateauSamples <= 0)
	{
#ifdef USE_BACKWARDS_RMS
		// average and then sqare the sample squareroots for the RMS value
		rms = powf((rms/(float)rmscount), 2.0f);
#else
		// average and then get the sqare root of the squared samples for the RMS value
		rms = sqrtf( rms / (float)(rmscount*2) );
#endif
		// because RMS tends to be < 0.5, thus unfairly limiting rupture's range
		rms *= 2.0f;
		// avoids clipping or illegit values (like from wraparound)
		if ( (rms > 1.0f) || (rms < 0.0f) )
			rms = 1.0f;
		rmscount = 0;	// reset the RMS counter
		//
		// set up the random floor values
		randomFloor = (float) expandparametervalue_index(kFloor, interpolateRandom(floorRandMin_gen, floor_gen));
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
	if ( MIDIout && (midiMode == kMidiMode_trigger) )
		// start from a 0.0 floor if we are coming in from silence
		sampleAmp = ((float)slopeSamples) * slopeStep;
	else if (useRandomFloor)
		sampleAmp = ( ((float)slopeSamples) * slopeStep * randomGainRange ) + randomFloor;
	else
		sampleAmp = ( ((float)slopeSamples) * slopeStep * gainRange ) + floor;

	slopeSamples--;

	if (slopeSamples <= 0)
	{
		state = valley;
		MIDIout = false;	// make sure it doesn't happen again
	}
}

//-----------------------------------------------------------------------------------------
void Skidder::processValley()
{
	float cycleRate;	// the base current skid rate value
	bool barSync = false;	// true if we need to sync up with the next bar start
	float SAMPLERATE = getsamplerate_f();


	if (MIDIin)
	{
		if (midiMode == kMidiMode_trigger)
			// there's one sample of valley when trigger mode begins, so silence that one
			sampleAmp = 0.0f;
		else if (midiMode == kMidiMode_apply)
			// there's one sample of valley when apply mode begins, so keep it at full gain
			sampleAmp = 1.0f;
	}
	// otherwise sampleAmp in the valley is whatever the floor gain is, the lowest gain value
	else if (useRandomFloor)
		sampleAmp = randomFloor;
	else
		sampleAmp = floor;

	valleySamples--;

	if (valleySamples <= 0)
	{
		rms = 0.0f;	// reset rms now because valley is over
		//
		// This is where we figure out how many samples long each 
		// envelope section is for the next skid cycle.
		//
		if (tempoSync)	// the user wants to do tempo sync / beat division rate
		{
			// randomize the tempo rate if the random min scalar is lower than the upper bound
			if (useRandomRate)
			{
				cycleRate = tempoRateTable->getScalar((long)interpolateRandom((float)rateRandMinIndex,(float)rateIndex+0.99f));
				// we can't do the bar sync if the skids durations are random
				needResync = false;
			}
			else
				cycleRate = rateSync;
			// convert the tempo rate into rate in terms of Hz
			cycleRate *= currentTempoBPS;
			// set this true so that we make sure to do the measure syncronisation later on
			if ( needResync && (midiMode == kMidiMode_none) )
				barSync = true;
		}
		else
		{
			if (useRandomRate)
				cycleRate = (float) expandparametervalue_index(kRate_abs, interpolateRandom(rateRandMinHz_gen, rateHz_gen));
			else
				cycleRate = rateHz;
		}
		needResync = false;	// reset this so that we don't have any trouble
		cycleSamples = (long) (SAMPLERATE / cycleRate);
		//
		if (useRandomPulsewidth)
			pulseSamples = (long) ( (float)cycleSamples * interpolateRandom(pulsewidthRandMin, pulsewidth) );
		else
			pulseSamples = (long) ( (float)cycleSamples * pulsewidth );
		valleySamples = cycleSamples - pulseSamples;
		slopeSamples = (long) (getsamplerate() * slopeSeconds);
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
			long countdown = timeinfo.samplesToNextBar % cycleSamples;
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

		// if MIDI apply mode is just beginning, make things smooth with no panning
		if ( (MIDIin) && (midiMode == kMidiMode_apply) )
			panGainL = panGainR = 1.0f;
		else
		{
			// this calculates a random float value from -1.0 to 1.0
			float panRander = (randFloat() * 2.0f) - 1.0f;
			// ((panRander*panWidth)+1.0) ranges from 0.0 to 2.0
			panGainL = (panRander*panWidth) + 1.0f;
			panGainR = 2.0f - ((panRander*panWidth) + 1.0f);
		}

	} //end of the "valley is over" if-statement
}

//-----------------------------------------------------------------------------------------
float Skidder::processOutput(float in1, float in2, float panGain)
{
	// output noise
	if ( (state == valley) && (noise != 0.0f) )
		// out gets random noise with samples from -1.0 to 1.0 times the random pan times rupture times the RMS scalar
		return ((randFloat()*2.0f)-1.0f) * panGain * noise * rms;

	// do regular skidding output
	else
	{
		// only output a bit of the first input
		if (panGain <= 1.0f)
			return in1 * panGain * sampleAmp;
		// output all of the first input & a bit of the second input
		else
			return ( in1 + (in2*(panGain-1.0f)) ) * sampleAmp;
	}
}

//-----------------------------------------------------------------------------------------
void Skidder::processaudio(const float ** inputs, float ** outputs, unsigned long inNumFrames, bool replacing)
{
	unsigned long numInputs = getnuminputs(), numOutputs = getnumoutputs();
	const float * in1  = inputs[0];
	const float * in2  = (numInputs < 2) ? inputs[0] : inputs[1];	// support 1 or 2 inputs
	float * out1 = outputs[0];
	float * out2 = (numOutputs < 2) ? outputs[0] : outputs[1];
	unsigned long samplecount;


// ---------- begin MIDI stuff --------------
	processMidiNotes();
	bool noteIsOn = false;

	for (int notecount=0; notecount < NUM_NOTES; notecount++)
	{
		if (noteTable[notecount])
		{
			noteIsOn = true;
			break;	// we only need to find one active note
		}
	}

	switch (midiMode)
	{
		case kMidiMode_trigger:
			// check waitSamples also because, if it's zero, we can just move ahead normally
			if ( noteIsOn && (waitSamples != 0) )
			{
				// need to make sure that the skipped part is silent if we're processing in-place
				if (replacing)
				{
					for (samplecount = 0; samplecount < (unsigned)waitSamples; samplecount++)
						out1[samplecount] = 0.0f;
					if (numOutputs > 1)
					{
						for (samplecount = 0; samplecount < (unsigned)waitSamples; samplecount++)
							out2[samplecount] = 0.0f;
					}
				}

				// cut back the number of samples outputted
				inNumFrames -= (unsigned)waitSamples;
				// and jump ahead accordingly in the i/o streams
				in1 += waitSamples;
				in2 += waitSamples;
				out1 += waitSamples;
				out2 += waitSamples;

				// reset
				waitSamples = 0;
			}

			else if (!noteIsOn)
			{
				// if Skidder currently is in the plateau & has a slow cycle, this could happen
				if ((unsigned)waitSamples > inNumFrames)
					waitSamples -= (signed)inNumFrames;
				else
				{
					if (replacing)
					{
						for (samplecount = (unsigned)waitSamples; samplecount < inNumFrames; samplecount++)
							out1[samplecount] = 0.0f;
						if (numOutputs > 1)
						{
							for (samplecount = (unsigned)waitSamples; samplecount < inNumFrames; samplecount++)
								out2[samplecount] = 0.0f;
						}
					}
					inNumFrames = (unsigned)waitSamples;
					waitSamples = 0;
				}
			}

			// adjust the floor according to note velocity if velocity mode is on
			if (useVelocity)
			{
				floor = (float) expandparametervalue_index(kFloor, (float)(127-mostRecentVelocity)/127.0f);
				gainRange = 1.0f - floor;	// the range of the skidding on/off gain
				useRandomFloor = false;
			}

			break;

		case kMidiMode_apply:
			// check waitSamples also because, if it's zero, we can just move ahead normally
			if ( noteIsOn && (waitSamples != 0) )
			{
				// need to make sure that the skipped part is unprocessed audio
				if (replacing)
				{
					memcpy(out1, in1, waitSamples * sizeof(float));
					if (numOutputs > 1)
						memcpy(out2, in2, waitSamples * sizeof(float));
				}
				else
				{
					for (samplecount = 0; samplecount < (unsigned)waitSamples; samplecount++)
						out1[samplecount] += in1[samplecount];
					if (numOutputs > 1)
					{
						for (samplecount = 0; samplecount < (unsigned)waitSamples; samplecount++)
							out2[samplecount] += in2[samplecount];
					}
				}

				// cut back the number of samples outputted
				inNumFrames -= (unsigned)waitSamples;
				// and jump ahead accordingly in the i/o streams
				in1 += waitSamples;
				in2 += waitSamples;
				out1 += waitSamples;
				out2 += waitSamples;

				// reset
				waitSamples = 0;
			}

			else if (!noteIsOn)
			{
				// if Skidder currently is in the plateau & has a slow cycle, this could happen
				if (waitSamples != 0)
				{
					if ((unsigned)waitSamples > inNumFrames)
						waitSamples -= (signed)inNumFrames;
					else
					{
						if (replacing)
						{
							memcpy( &(out1[waitSamples]), &(in1[waitSamples]), (inNumFrames - (unsigned)waitSamples) * sizeof(float) );
							if (numOutputs > 1)
								memcpy( &(out2[waitSamples]), &(in2[waitSamples]), (inNumFrames - (unsigned)waitSamples) * sizeof(float) );
						}
						else
						{
							for (samplecount = (unsigned)waitSamples; samplecount < inNumFrames; samplecount++)
								out1[samplecount] += in1[samplecount];
							if (numOutputs > 1)
							{
								for (samplecount = (unsigned)waitSamples; samplecount < inNumFrames; samplecount++)
									out2[samplecount] += in2[samplecount];
							}
						}
						inNumFrames = (unsigned)waitSamples;
						waitSamples = 0;
					}
				}
				else
				{
					if (replacing)
					{
						memcpy(out1, in1, inNumFrames * sizeof(float));
						if (numOutputs > 1)
							memcpy(out2, in2, inNumFrames * sizeof(float));
					}
					else
					{
						for (samplecount = 0; samplecount < inNumFrames; samplecount++)
							out1[samplecount] += in1[samplecount];
						if (numOutputs > 1)
						{
							for (samplecount = 0; samplecount < inNumFrames; samplecount++)
								out2[samplecount] += in2[samplecount];
						}
					}
					// that's all we need to do if there are no notes, 
					// just copy the input to the output
					return;
				}
			}

			// adjust the floor according to note velocity if velocity mode is on
			if (useVelocity)
			{
				floor = (float) expandparametervalue_index(kFloor, (float)(127-mostRecentVelocity)/127.0f);
				gainRange = 1.0f - floor;	// the range of the skidding on/off gain
				useRandomFloor = false;
			}

			break;

		default:
			break;
	}
// ---------- end MIDI stuff --------------


	// figure out the current tempo if we're doing tempo sync
	if (tempoSync)
	{
		// calculate the tempo at the current processing buffer
		if ( useHostTempo && hostCanDoTempo && timeinfo.tempoIsValid )	// get the tempo from the host
		{
			currentTempoBPS = (float) timeinfo.tempo_bps;
			// check if audio playback has just restarted & reset buffer stuff if it has (for measure sync)
			if (timeinfo.playbackChanged)
			{
				needResync = true;
				state = valley;
				valleySamples = 0;
			}
		}
		else	// get the tempo from the user parameter
		{
			currentTempoBPS = userTempo / 60.0f;
			needResync = false;	// we don't want it true if we're not syncing to host tempo
		}
		oldTempoBPS = currentTempoBPS;
	}
	else
		needResync = false;	// we don't want it true if we're not syncing to host tempo


	// stereo processing
	if (numOutputs >= 2)
	{
		// this is the per-sample audio processing loop
		for (samplecount=0; samplecount < inNumFrames; samplecount++)
		{
			switch (state)
			{
				case slopeIn:
					// get the average sqareroot of the current input samples
#ifdef USE_BACKWARDS_RMS
					rms += sqrtf( fabsf(((*in1)+(*in2))*0.5f) );
#else
					rms += ((*in1)*(*in1)) + ((*in2)*(*in2));
#endif
					rmscount++;	// this counter is later used for getting the mean
					processSlopeIn();
					break;
				case plateau:
#ifdef USE_BACKWARDS_RMS
					rms += sqrtf( fabsf(((*in1)+(*in2))*0.5f) );
#else
					rms += ((*in1)*(*in1)) + ((*in2)*(*in2));
#endif
					rmscount++;
					processPlateau();
					break;
				case slopeOut:
					processSlopeOut();
					break;
				case valley:
					processValley();
					break;
			}
	
		#ifdef TARGET_API_VST
			if (replacing)
			{
		#endif
				*out1 = processOutput(*in1, *in2, panGainL);
				*out2 = processOutput(*in2, *in1, panGainR);
		#ifdef TARGET_API_VST
			}
			else
			{
				*out1 += processOutput(*in1, *in2, panGainL);
				*out2 += processOutput(*in2, *in1, panGainR);
			}
		#endif
			// move forward in the i/o sample streams
			in1++;
			in2++;
			out1++;
			out2++;
		}
	}

	// mono processing
	else
	{
		// this is the per-sample audio processing loop
		for (samplecount=0; samplecount < inNumFrames; samplecount++)
		{
			switch (state)
			{
				case slopeIn:
					// get the average sqareroot of the current input samples
#ifdef USE_BACKWARDS_RMS
					rms += sqrtf( fabsf(*in1) );
#else
					rms += (*in1) * (*in1);
#endif
					rmscount++;	// this counter is later used for getting the mean
					processSlopeIn();
					break;
				case plateau:
#ifdef USE_BACKWARDS_RMS
					rms += sqrtf( fabsf(*in1) );
#else
					rms += (*in1) * (*in1);
#endif
					rmscount++;
					processPlateau();
					break;
				case slopeOut:
					processSlopeOut();
					break;
				case valley:
					processValley();
					break;
			}
	
		#ifdef TARGET_API_VST
			if (replacing)
		#endif
				*out1 = processOutput(*in1, *in1, 1.0f);
		#ifdef TARGET_API_VST
			else
				*out1 += processOutput(*in1, *in1, 1.0f);
		#endif
			// move forward in the i/o sample streams
			in1++;
			out1++;
		}
	}
}
