/*-------------- by Marc Poirier  ][  December 2000 -------------*/

#ifndef __skidder
#include "skidder.hpp"
#endif


//-----------------------------------------------------------------------------
void Skidder::resetMidi()
{
	// & zero out the whole note table
	for (int i=0; (i < NUM_NOTES); i++)
		noteTable[i] = 0;

	waitSamples = 0;
	MIDIin = MIDIout = false;
	mostRecentVelocity = 127;
}

//-----------------------------------------------------------------------------
long Skidder::processEvents(VstEvents* newEvents)
{
  VstMidiEvent *midiEvent;
  int status, currentNote, velocity;
  char *midiData;
  bool noteIsOn = false;
  long newProgramNumber = -1, newProgramDelta = 0;


	for (long i = 0; (i < newEvents->numEvents); i++)
	{
		// check to see if this event is MIDI; if no, then we try the for-loop again
		if ( ((newEvents->events[i])->type) != kVstMidiType )
			continue;

		// it's okay, so cast the incoming event as a VstMidiEvent
		midiEvent = (VstMidiEvent*)newEvents->events[i];

		// address the midiData[4] string from the event to this temp data pointer
		midiData = midiEvent->midiData;

		// ignore the channel (lower 4 bits)
		status = midiData[0] & 0xf0;
		currentNote = midiData[1] & 0x7f;
		velocity = midiData[2] & 0x7f;

		// note-on status was received
		if (status == 0x90)
		{
			if (velocity > 0)
			{
				noteTable[currentNote] = mostRecentVelocity = velocity;
				// This is not very accurate.
				// If more than one note-on happens this block, only the last one is effective.
				// This is good enough for this effect, though.
				noteOn(midiEvent->deltaFrames);
			}
			// this is actually a note-off message if velocity is 0
			else
				noteTable[currentNote] = 0;	// turn off the note
		}

		// note-off status was received
		if (status == 0x80)
			noteTable[currentNote] = 0;	// turn off the note

		// all notes off
		if ( (status == 0xb0) && (midiData[1] == 0x7b) )
		{
			for (currentNote = 0; (currentNote < NUM_NOTES); currentNote++)
				noteTable[currentNote] = 0;	// turn off all notes
			noteOff();	// do the notes off Skidder stuff
		}

		midiEvent++;

		// program change was received
		if (status == 0xC0)
		{
			if (midiEvent->deltaFrames >= newProgramDelta)
			{
				newProgramNumber = midiData[1] & 0x7F;	// program number
				newProgramDelta = midiEvent->deltaFrames;	// timing offset
			}
		}
	}

	// change the current Skidder preset if a program change message was received
	if (newProgramNumber >= 0)
		setProgram(newProgramNumber);

	// check every note to see any are still on or if they all got turned off during this block
	for (currentNote=0; (currentNote < NUM_NOTES); currentNote++)
	{
		if (noteTable[currentNote])
			noteIsOn = true;
	}
	// & do the Skidder notes off stuff if we have no notes remaining on
	if (!noteIsOn)
			noteOff();

	// process CC parameter automation
	chunk->processParameterEvents(newEvents);

	// says that we want more events; 0 means stop sending them
	return 1;
}

//-----------------------------------------------------------------------------------------
void Skidder::noteOn(long delta)
{
	switch (midiModeScaled(fMidiMode))
	{
		case kMidiTrigger:
			waitSamples = delta;
			state = valley;
			valleySamples = 0;
			MIDIin = true;
			break;

		case kMidiApply:
			waitSamples = delta;
			state = valley;
			valleySamples = 0;
			MIDIin = true;
			break;

		default:
			break;
	}
}

//-----------------------------------------------------------------------------------------
void Skidder::noteOff()
{
	switch (midiModeScaled(fMidiMode))
	{
		case kMidiTrigger:
			switch (state)
			{
				case slopeIn:
					state = slopeOut;
					slopeSamples = slopeDur - slopeSamples;
					waitSamples = slopeSamples;
					break;

				case plateau:
					plateauSamples = 1;
					waitSamples = slopeDur + 1;
					break;

				case slopeOut:
					waitSamples = slopeSamples;
					break;

				case valley:
					waitSamples = 0;
					break;

				default:
					break;
			}

			MIDIout = true;
			// in case we're still in a MIDI-in phase; it won't get falsed otherwise
			if (MIDIin)
				MIDIin = false;
			// if we're in a slope-out & there's a raised floor, the slope position needs to be rescaled
			else if ( (state == slopeOut) && (floor > 0.0f) )
			{
				slopeSamples = (long) ( (((float)slopeSamples*slopeStep*gainRange)+floor) * (float)slopeDur );
				waitSamples = slopeSamples;
			}

			// just to prevent anything really stupid from happening
			if (waitSamples < 0)
				waitSamples = 0;

			break;

		case kMidiApply:
			switch (state)
			{
				case slopeIn:
					waitSamples = slopeSamples;
					break;

				case plateau:
					waitSamples = 0;
					break;

				case slopeOut:
					state = slopeIn;
					slopeSamples = slopeDur - slopeSamples;
					waitSamples = slopeSamples;
					break;

				case valley:
					valleySamples = 1;
					waitSamples = slopeDur + 1;
					break;

				default:
					break;
			}

			MIDIout = true;
			// in case we're still in a MIDI-in phase; it won't get falsed otherwise
			if (MIDIin)
				MIDIin = false;
			// just to prevent anything really stupid from happening
			if (waitSamples < 0)
				waitSamples = 0;

			break;

		default:
			break;
	}
}


//-----------------------------------------------------------------------------
long Skidder::getChunk(void **data, bool isPreset)
{	return chunk->getChunk(data, isPreset);	}

//-----------------------------------------------------------------------------
long Skidder::setChunk(void *data, long byteSize, bool isPreset)
{	return chunk->setChunk(data, byteSize, isPreset);	}


//----------------------------------------------------------------------------- 
SkidderChunk::SkidderChunk(long numParameters, long numPrograms, long magic, AudioEffectX *effect)
	: VstChunk (numParameters, numPrograms, magic, effect)
{
	// start off with split CC automation of both range slider points
	pulsewidthDoubleAutomate = floorDoubleAutomate = false;
}

//----------------------------------------------------------------------------- 
// this gets called when Skidder automates a parameter from CC messages.
// this is where we can link parameter automation for rangeslider points.
void SkidderChunk::doLearningAssignStuff(long tag, long eventType, long eventChannel, long eventNum, 
										long delta, long eventNum2, long eventBehaviourFlags, 
										long data1, long data2, float fdata1, float fdata2)
{
	if ( getSteal() )
		return;

	switch (tag)
	{
		case kPulsewidth:
			if (pulsewidthDoubleAutomate)
				assignParam(kPulsewidthRandMin, eventType, eventChannel, eventNum, eventNum2, 
							eventBehaviourFlags, data1, data2, fdata1, fdata2);
			break;
		case kPulsewidthRandMin:
			if (pulsewidthDoubleAutomate)
				assignParam(kPulsewidth, eventType, eventChannel, eventNum, eventNum2, 
							eventBehaviourFlags, data1, data2, fdata1, fdata2);
			break;
		case kFloor:
			if (floorDoubleAutomate)
				assignParam(kFloorRandMin, eventType, eventChannel, eventNum, eventNum2, 
							eventBehaviourFlags, data1, data2, fdata1, fdata2);
			break;
		case kFloorRandMin:
			if (floorDoubleAutomate)
				assignParam(kFloor, eventType, eventChannel, eventNum, eventNum2, 
							eventBehaviourFlags, data1, data2, fdata1, fdata2);
			break;
		default:
			break;
	}
}

//----------------------------------------------------------------------------- 
void SkidderChunk::unassignParam(long tag)
{
	VstChunk::unassignParam(tag);

	switch (tag)
	{
		case kPulsewidth:
			if (pulsewidthDoubleAutomate)
				VstChunk::unassignParam(kPulsewidthRandMin);
			break;
		case kPulsewidthRandMin:
			if (pulsewidthDoubleAutomate)
				VstChunk::unassignParam(kPulsewidth);
			break;
		case kFloor:
			if (floorDoubleAutomate)
				VstChunk::unassignParam(kFloorRandMin);
			break;
		case kFloorRandMin:
			if (floorDoubleAutomate)
				VstChunk::unassignParam(kFloor);
			break;
		default:
			break;
	}
}
