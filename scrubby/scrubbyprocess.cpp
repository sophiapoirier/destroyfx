/*-------------- by Marc Poirier  ][  February 2002 -------------*/

#ifndef __scrubby
#include "scrubby.hpp"
#endif

#include <stdlib.h>
#include <math.h>
#if MAC
	#include <fp.h>
#endif
#if WIN32
	#include <float.h>
#endif


//-----------------------------------------------------------------------------------------
inline double calculateTargetSpeed(double a, double n, double k)
{
  double b;	// the target speed

	a = fabs(a);
	n = fabs(n);
	k = fabs(k);


	double lambertInput = (n * a) / k;
	b = k * LambertW(lambertInput) / n;
	// cuz I don't totally trust my LambertW function...
#if MAC
	if ( isnan(b) || (! __isfinite(b)) )
#else
	if ( _isnan(b) || (! _finite(b)) )
#endif
		b = 1.0;
//	else
//		b *= 2.0;
	if (b < 1.0)
		b = pow(b, 0.63);


/*
	float inc;
	float testk = k * 2.0f;
	b = a;
	// b is less than a, guess downwards
	if (k < (n*a))
		inc = 0.5f;
	// b is larger than a, guess upwards
	else
		inc = 2.0f;
	while ( fabsf((testk/k) - 1.0f) > 0.01f )
//	while (testk != k)
	for (int i=0; i < 90; i++)
	{
		b *= inc;
		testk = fabsf( ((b-a) * n) / logf(b/a) );
		if (testk < k)
		{
			b /= inc;
			inc = sqrtf(inc);
		}
		else if (testk == k)
			break;
	}
*/
//	b = k / n;

	return b;
}


//-----------------------------------------------------------------------------------------
void Scrubby::checkTempoSyncStuff()
{
	// figure out the current tempo if we're doing tempo sync
	if (tempoSync)
	{
		// calculate the tempo at the current processing buffer
		if ( (fTempo > 0.0f) || (hostCanDoTempo != 1) )	// get the tempo from the user parameter
		{
			currentTempoBPS = tempoScaled(fTempo) / 60.0f;
			needResync1 = needResync2 = false;	// we don't want it true if we're not syncing to host tempo
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
				// check if audio playback has just restarted or playback position has changed
				if (timeInfo->flags & kVstTransportChanged)
					needResync1 = needResync2 = true;
			}
			else	// do the same stuff as above if the timeInfo gets a null pointer
			{
				currentTempoBPS = tempoScaled(fTempo) / 60.0f;
				needResync1 = needResync2 = false;	// we don't want it true if we're not syncing to host tempo
			}
		}
	}
	else
		needResync1 = needResync2 = false;

	// reset cycle state stuff if playback has changed (for measure sync)
	if (needResync1)
		seekcount1 = 0;
	if (needResync2)
		seekcount2 = 0;
}

//-----------------------------------------------------------------------------------------
void Scrubby::generateNewTarget(int channel)
{
  float currentSeekRate, currentSeekDur;
  double readStep, portamentoStep;
  long seekcount, movecount;


// CALCULATE THE SEEK CYCLE LENGTH
	// match the seek rate to tempo
	if (tempoSync)
	{
		float tempoRateMin = tempoRateTable->getScalar(fSeekRateRandMin);
		float tempoRateMax = tempoRateTable->getScalar(fSeekRate);
		// randomize the tempo rate if the random min scalar is lower than the upper bound
		if (tempoRateMin < tempoRateMax)
		{
			currentSeekRate = tempoRateTable->getScalar(interpolateRandom(fSeekRateRandMin,fSeekRate));
			// don't do musical bar sync if we're using randomized tempo rate
			needResync1 = needResync2 = false;
		}
		else
			currentSeekRate = tempoRateMax;
		// convert the tempo rate into rate in terms of Hz
		currentSeekRate *= currentTempoBPS;
	}
	//
	// use the seek rate in terms of Hz
	else
	{
		if (useSeekRateRandMin)
			currentSeekRate = seekRateScaled(interpolateRandom(fSeekRateRandMin, fSeekRate));
		else
			currentSeekRate = seekRateScaled(fSeekRate);
		needResync1 = needResync2 = false;
	}
	// calculate the length of this seek cycle in seconds
	float cycleDur = 1.0f / currentSeekRate;
	// then calculate the length of this seek cycle in samples
	seekcount = (long) (cycleDur * SAMPLERATE);
	//
	bool needResync = (channel == 1) ? needResync1 : needResync2;
	// do bar sync if we're in tempo sync & things resyncing is in order
	if (needResync)
	{
		long samplesUntilBar = samplesToNextBar(timeInfo);
		// because a 0 for seekcount will be soon turned into 1, which is not so good for DJ mode
		if (samplesUntilBar > 0)
		{
			if (portamento)
				// doubling the length of the first seek after syncing to the measure 
				// seems to prevent Cubase from freaking out in tempo sync'd DJ mode
				seekcount = (samplesUntilBar+seekcount) % (seekcount*2);
			else
				seekcount = samplesUntilBar % seekcount;
			cycleDur = (float)seekcount / SAMPLERATE;
		}
	}
	// avoid bad values
	if (seekcount < 1)
		seekcount = 1;

// CALCULATE THE MOVEMENT CYCLE LENGTH
	if (useSeekDurRandMin)
		currentSeekDur = seekDurScaled(interpolateRandom(fSeekDurRandMin, fSeekDur));
	else
		currentSeekDur = seekDurScaled(fSeekDur);
	// calculate the length of the seeking movement in samples
	movecount = (long) (cycleDur * currentSeekDur * SAMPLERATE);
	// avoid bad values
	if (movecount < 1)
		movecount = 1;

// FIND THE NEW TARGET POSITION IN THE BUFFER
	// randomly locate a new target position within the buffer seek range
	float randy = (float)rand() * ONE_DIV_RAND_MAX;
	float bufferSizeFloat = seekRangeScaled(fSeekRange) * 0.001f * SAMPLERATE;
	// search back from the current writer input point
	long newTargetPos = writePos - (long)(bufferSizeFloat * randy);
	//
	// calculate the distance between
	long readPosInt = (channel == 1) ? (long)readPos1 : (long)readPos2;
	if (readPosInt >= writePos)
		readPosInt -= MAX_BUFFER;
//	if ( abs(readPosInt-writePos) > (MAX_BUFFER/2) )
//	{
//		if (readPosInt >= writePos)
//			readPosInt -= MAX_BUFFER;
//		else
//			readPosInt += MAX_BUFFER;
//	}
	long targetDistance = newTargetPos - readPosInt;
	if (targetDistance == 0)
		targetDistance = 1;	
	//
	// calculate the step size of playback movement through the buffer
	readStep = (double)targetDistance / (double)movecount;
	// constrain the speed to a semitone step, if that's what we want to do
	if (pitchConstraint && !portamento)
		readStep = processPitchConstraint(readStep);
	//
	// calculate the step size of portamento playback incrementation
//	if ( portamento && (!needResync) )
	if (portamento)
	{
		bool moveBackwards = (readStep < 0.0);	// will we move backwards in the delay buffer?
		double oldReadStep = (channel == 1) ? readStep1 : readStep2;
		oldReadStep = fabs(oldReadStep);
		//
#ifdef USE_LINEAR_ACCELERATION
		double absReadStep = fabs(readStep);
		bool slowdown = (absReadStep < oldReadStep);	// are we slowing down or speeding up?
		double stepDifference = absReadStep - oldReadStep;
		bool cross0 = ( slowdown && (stepDifference > absReadStep) );	// will we go down to 0 Hz & then back up again?
		portamentoStep = (stepDifference * 2.0) / (double)movecount;
		if (moveBackwards)
			portamentoStep = -portamentoStep;
//		portamentoStep = fabs(portamentoStep);
//		if (slowdown)
//			portamentoStep = -portamentoStep;
#else
		// exponential acceleration
		if (oldReadStep < 0.001)	// protect against a situation impossible to deal with
			oldReadStep = 0.001;
		double targetReadStep = calculateTargetSpeed(oldReadStep, (double)movecount, (double)targetDistance);
		portamentoStep = pow( fabs(targetReadStep)/oldReadStep, 1.0/(double)movecount );
		portamentoStep = fabs(portamentoStep);
//showme = targetReadStep;
//long ktest = (long) fabsf( (targetReadStep * (float)movecount) / logf(fabsf(targetReadStep/oldReadStep)) );
//fprintf(f, "oldReadStep = %.6f\nreadStep = %.6f\ntargetReadStep = %.6f\nportamentoStep = %.6f\nmovecount = %ld\ntargetDistance = %ld\nktest = %ld\n\n", oldReadStep,readStep,targetReadStep,portamentoStep, movecount, targetDistance,ktest);
//fprintf(f, "a = %.3f,\tb = %.3f,\tn = %ld,\tk = %ld\tportamentoStep = %.6f\n", oldReadStep, targetReadStep, movecount, targetDistance, portamentoStep);
//fprintf(f, "\noldReadStep = %.6f\treadStep = %.6f\ttargetReadStep = %.6f\tportamentoStep = %.6f\n", oldReadStep,readStep,targetReadStep,portamentoStep);

#endif
		readStep = oldReadStep;
		if (moveBackwards)
			readStep = -readStep;
#ifdef USE_LINEAR_ACCELERATION
		if (cross0)
			readStep = -readStep;
#endif
	}
	else
#ifdef USE_LINEAR_ACCELERATION
		portamentoStep = 0.0;	// for no linear acceleration
#else
		portamentoStep = 1.0;	// for no exponential acceleration
#endif

// ASSIGN ALL OF THESE NEW VALUES INTO THE CORRECT CHANNEL'S VARIABLES
	if (channel == 1)
	{
		seekcount1 = seekcount;
		movecount1 = movecount;
		readStep1 = readStep;
		portamentoStep1 = portamentoStep;
		needResync1 = false;	// untrue this so that we don't do the measure sync calculations again unnecessarily
	}
	else
	{
		seekcount2 = seekcount;
		movecount2 = movecount;
		readStep2 = readStep;
		portamentoStep2 = portamentoStep;
		needResync2 = false;	// untrue this so that we don't do the measure sync calculations again unnecessarily
	}
}

//-----------------------------------------------------------------------------------------
double Scrubby::processPitchConstraint(double readStep)
{
  long i;


	bool backwards = (readStep < 0.0);	// traveling backwards through the buffer?
	double fdirection = backwards ? -1.0 : 1.0;	// direction scalar

	// check to see if none of the semitones are enabled, 
	// in which case we act like they are all enabled.
	// no we don't, we become silent, I changed my mind
	bool noNotesActive = true;
	for (i=0; i < NUM_PITCH_STEPS; i++)
	{
		if (pitchSteps[i])
			noNotesActive = false;
	}

	// determine the lower bound of the number of semitone shifts that this playback speed results in
	// the equation below comes from 2^(semitone/12) = readStep, solving for semitone
	double fsemitone = log(fabs(readStep)) / LN2TO1_12TH;
	fsemitone = floor(fsemitone);
	long semitone = (long) (fsemitone + 0.1);	// add a little bit to prevent any float truncation errors

	// . . . search for a possible active note to constrain to . . .
	long octave = semitone / 12;	// the octave transposition
	long remainder = semitone % 12;	// the semitone transposition
	// remainder will be used as the index to the pitchSteps array, 
	// so it must be positive, & we compensate for adding 12 to it 
	// by subtracting 1 octave, so it all evens out
	if (remainder < 0)
	{
		remainder += 12;
		octave -= 1;
	}
	// just in case it fails both searches, which shouldn't happen
	semitone = remainder;
	// start searching for an active pitchStep with the current pitchStep (remainder) 
	// & then all those below it
	i = remainder;
	// if no notes are active, then don't play back anything (this basically means silence)
	if (noNotesActive)
		return 0.0f;
	// if we are choosing from a selection of semitone steps, then do the selection process
	else
	{
		do
		{
			// we found the pitchstep that we will use, store it & exit this loop
			if (pitchSteps[i])
			{
				semitone = i;
				break;
			}
			// otherwise keep searching down
			i--;
			if (i < 0)
			{
				// didn't find an active one yet, so wrap around to the top of the pitchStep array
				i = NUM_PITCH_STEPS - 1;
				// & compensate for that by subtracting an octave
				octave--;
			}
		} while (i != remainder);
	}
	// constrain to octaves range, if we're doing that
	long octavemin = octaveMinScaled(fOctaveMin);
	long octavemax = octaveMaxScaled(fOctaveMax);
	if ( (octavemin > OCTAVE_MIN) && (octave < octavemin) )
		octave = octavemin;
	else if ( (octavemax < OCTAVE_MAX) && (octave > octavemax) )
		octave = octavemax;
	// add the octave transposition back in to get the correct semitone transposition
	semitone += (octave * 12);

	// return the transposition converted to a playback speed scalar, with direction restored
	return pow(2.0, (double)semitone/12.0) * fdirection;
}


//-----------------------------------------------------------------------------------------
void Scrubby::doTheProcess(float **inputs, float **outputs, long sampleFrames, bool replacing)
{
#if MAC
	// no memory allocations at during interrupt
#else
	// there must have not been available memory or something (like WaveLab goofing up), 
	// so try to allocate buffers now
	if ( (buffer1 == NULL) || (buffer2 == NULL) )
		createAudioBuffers();
#endif
	// if the creation failed, then abort audio processing
	if ( (buffer1 == NULL) || (buffer2 == NULL) )
		return;

	getRealValues();

	checkTempoSyncStuff();

	// if we're using pitch constraint & the previous block had no notes active, 
	// then check to see if new notes have begun &, if so, begin new seeks 
	// so that we start producing sound again immediately
	int i;
	if ( (pitchConstraint && !portamento) && !notesWereAlreadyActive )
	{
		for (i=0; i < NUM_PITCH_STEPS; i++)
		{
			if (pitchSteps[i])
			{
				seekcount1 = 0;
				seekcount2 = 0;
				break;
			}
		}
	}
	// now remember the current situation for informing the next processing block
	notesWereAlreadyActive = false;
	for (i=0; i < NUM_PITCH_STEPS; i++)
	{
		if (pitchSteps[i])
			notesWereAlreadyActive = true;
	}

	for (long samplecount=0; (samplecount < sampleFrames); samplecount++)
	{
		// update the buffers with the latest samples
		if (!freeze)
		{
			buffer1[writePos] = inputs[0][samplecount];
			buffer2[writePos] = inputs[1][samplecount];
// melody test
//			buffer1[writePos] = buffer2[writePos] = 0.69f*sinf(24.0f*PI*((float)samplecount/(float)sampleFrames));
//			buffer1[writePos] = buffer2[writePos] = 0.69f*sinf(2.0f*PI*((float)sinecount/169.0f));
//			if (++sinecount > 168)   sinecount = 0;	// produce a sine wave of C4 when using 44.1 kHz
		}

		// get the current output values, interpolated for smoothness
		float out1 = interpolateHermite(buffer1, readPos1, MAX_BUFFER);
		float out2 = interpolateHermite(buffer2, readPos2, MAX_BUFFER);

		// write the output to the output streams
		if (replacing)
		{
			outputs[0][samplecount] = out1;
			outputs[1][samplecount] = out2;
		}
		else
		{
			outputs[0][samplecount] += out1;
			outputs[1][samplecount] += out2;
		}

		// increment/decrement the position trackers & counters
		if (!freeze)
			writePos = (writePos+1) % MAX_BUFFER;
		seekcount1--;
		seekcount2--;
		movecount1--;
		movecount2--;
		//
		// it's time to find a new target to seek
		if (seekcount1 < 0)
		{
			generateNewTarget(1);

			// copy the left channel's new values if we're in unified stereo mode
			if (!splitStereo)
			{
				readPos2 = readPos1;
				readStep2 = readStep1;
				portamentoStep2 = portamentoStep1;
				seekcount2 = seekcount1;
				movecount2 = movecount1;
				needResync2 = needResync1;
			}
		}
		// find a new target to seek for the right channel if we're in split stereo mode
		if ( splitStereo && (seekcount2 < 0) )
			generateNewTarget(2);
		//
		// only increment the read position trackers if we're still moving towards the target
		if (movecount1 >= 0)
		{
			if (portamento)
#ifdef USE_LINEAR_ACCELERATION
				readStep1 += portamentoStep1;
#else
				readStep1 *= portamentoStep1;
#endif
			readPos1 += readStep1;
			long readPosInt = (long)readPos1;
			// wraparound the read position tracker if necessary
			if (readPosInt >= MAX_BUFFER)
				readPos1 = fmod(readPos1, MAX_BUFFER_FLOAT);
			else if (readPosInt < 0)
			{
				while (readPos1 < 0.0)
					readPos1 += MAX_BUFFER_FLOAT;
			}
//fprintf(f, "readStep1 = %.6f\treadPos1 = %.9f\tmovecount1 = %ld\n", readStep1, readPos1, movecount1);
		}
		if (movecount2 >= 0)
		{
			if (portamento)
#ifdef USE_LINEAR_ACCELERATION
				readStep2 += portamentoStep2;
#else
				readStep2 *= portamentoStep2;
#endif
			readPos2 += readStep2;
			long readPosInt = (long)readPos2;
			// wraparound the read position tracker if necessary
			if (readPosInt >= MAX_BUFFER)
				readPos2 = fmod(readPos2, MAX_BUFFER_FLOAT);
			else if (readPosInt < 0)
			{
				while (readPos2 < 0.0)
					readPos2 += MAX_BUFFER_FLOAT;
			}
		}
	}
}

//-----------------------------------------------------------------------------------------
void Scrubby::process(float **inputs, float **outputs, long sampleFrames)
{
	doTheProcess(inputs, outputs, sampleFrames, false);
}

//-----------------------------------------------------------------------------------------
void Scrubby::processReplacing(float **inputs, float **outputs, long sampleFrames)
{
	doTheProcess(inputs, outputs, sampleFrames, true);
}


//-----------------------------------------------------------------------------------------
long Scrubby::processEvents(VstEvents* events)
{
  int currentNote, notecount;
  long i;


	midistuff->processEvents(events, this);	// make sense of the current block's MIDI data
	chunk->processParameterEvents(events);

	for (i=0; i < midistuff->numBlockEvents; i++)
	{
		// wrap the note value around to our 1-octave range
		currentNote = (midistuff->blockEvents[i].byte1) % NUM_PITCH_STEPS;

		switch (midistuff->blockEvents[i].status)
		{
			case kMidiNoteOn:
				// correct any bogus values that would mess up the system
				if (activeNotesTable[currentNote] < 0)
					activeNotesTable[currentNote] = 0;
				// if there are currently no keys playing this note, 
				// then turn its associated pitch constraint parameter on
				if (activeNotesTable[currentNote] == 0)
				{
					fPitchSteps[currentNote] = 1.0f;
					keyboardWasPlayedByMidi = true;
				}
				// increment the active notes table for this note
				activeNotesTable[currentNote] += 1;
				break;

			case kMidiNoteOff:
				// if this is the last key that was playing this note, 
				// then turn its associated pitch constraint parameter off
				if (activeNotesTable[currentNote] == 1)
				{
					fPitchSteps[currentNote] = 0.0f;
					keyboardWasPlayedByMidi = true;
				}
				// decrement the active notes table for this note, but don't go below 0
				if (activeNotesTable[currentNote] > 0)
					activeNotesTable[currentNote] -= 1;
				else
					activeNotesTable[currentNote] = 0;
				break;

			case ccAllNotesOff:
				for (notecount=0; notecount < NUM_PITCH_STEPS; notecount++)
				{
					// if this note is currently active, then turn its 
					// associated pitch constraint parameter off
					if (activeNotesTable[notecount] > 0)
					{
						fPitchSteps[notecount] = 0.0f;
						keyboardWasPlayedByMidi = true;
					}
					// reset this note in the table
					activeNotesTable[notecount] = 0;
				}
				break;

			default:
				break;
		}
	}

	// tells the host to keep calling this function in the future; 0 means stop
	return 1;
}
