/*-------------- by Marc Poirier  ][  December 2000 -------------*/

#include "skidder.hpp"


//-----------------------------------------------------------------------------
void Skidder::resetMidi()
{
	// zero out the whole note table
	for (int i=0; i < NUM_NOTES; i++)
		noteTable[i] = 0;

	waitSamples = 0;
	MIDIin = MIDIout = false;
	mostRecentVelocity = 127;
}

//-----------------------------------------------------------------------------
void Skidder::processMidiNotes()
{
	bool noteIsOn = false;

	for (long i=0; i < midistuff->numBlockEvents; i++)
	{
		int currentNote = midistuff->blockEvents[i].byte1;
		int velocity = midistuff->blockEvents[i].byte2;

		switch (midistuff->blockEvents[i].status)
		{
			// note-on status was received
			case kMidiNoteOn:
				noteTable[currentNote] = mostRecentVelocity = velocity;
				// This is not very accurate.
				// If more than one note-on happens this block, only the last one is effective.
				// This is good enough for this effect, though.
				noteOn(midistuff->blockEvents[i].delta);
				break;

			// note-off status was received
			case kMidiNoteOff:
				noteTable[currentNote] = 0;	// turn off the note
				break;

			// all notes off
			case kMidiCC_AllNotesOff:
				for (currentNote = 0; currentNote < NUM_NOTES; currentNote++)
					noteTable[currentNote] = 0;	// turn off all notes
				noteOff();	// do the notes off Skidder stuff
				break;
		}
	}

	// check every note to see any are still on or if they all got turned off during this block...
	for (int currentNote=0; currentNote < NUM_NOTES; currentNote++)
	{
		if (noteTable[currentNote])
			noteIsOn = true;
	}
	// ...and do the Skidder notes-off stuff if we have no notes remaining on
	if (!noteIsOn)
			noteOff();
}

//-----------------------------------------------------------------------------------------
void Skidder::noteOn(long delta)
{
	switch (midiMode)
	{
		case kMidiMode_trigger:
			waitSamples = delta;
			state = valley;
			valleySamples = 0;
			MIDIin = true;
			break;

		case kMidiMode_apply:
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
	switch (midiMode)
	{
		case kMidiMode_trigger:
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
			// if we're in a slope-out and there's a raised floor, the slope position needs to be rescaled
			else if ( (state == slopeOut) && (floor > 0.0f) )
			{
				slopeSamples = (long) ( (((float)slopeSamples*slopeStep*gainRange)+floor) * (float)slopeDur );
				waitSamples = slopeSamples;
			}

			// just to prevent anything really stupid from happening
			if (waitSamples < 0)
				waitSamples = 0;

			break;

		case kMidiMode_apply:
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
// this gets called when Skidder automates a parameter from CC messages.
// this is where we can link parameter automation for rangeslider points.
void Skidder::settings_doLearningAssignStuff(long tag, long eventType, long eventChannel, long eventNum, 
										long delta, long eventNum2, long eventBehaviourFlags, 
										long data1, long data2, float fdata1, float fdata2)
{
	if ( dfxsettings->getSteal() )
		return;

	switch (tag)
	{
		case kPulsewidth:
			if (pulsewidthDoubleAutomate)
				dfxsettings->assignParam(kPulsewidthRandMin, eventType, eventChannel, eventNum, eventNum2, 
							eventBehaviourFlags, data1, data2, fdata1, fdata2);
			break;
		case kPulsewidthRandMin:
			if (pulsewidthDoubleAutomate)
				dfxsettings->assignParam(kPulsewidth, eventType, eventChannel, eventNum, eventNum2, 
							eventBehaviourFlags, data1, data2, fdata1, fdata2);
			break;
		case kFloor:
			if (floorDoubleAutomate)
				dfxsettings->assignParam(kFloorRandMin, eventType, eventChannel, eventNum, eventNum2, 
							eventBehaviourFlags, data1, data2, fdata1, fdata2);
			break;
		case kFloorRandMin:
			if (floorDoubleAutomate)
				dfxsettings->assignParam(kFloor, eventType, eventChannel, eventNum, eventNum2, 
							eventBehaviourFlags, data1, data2, fdata1, fdata2);
			break;
		default:
			break;
	}
}

/*
//----------------------------------------------------------------------------- 
void Skidder::settings_unassignParam(long tag)
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
*/
