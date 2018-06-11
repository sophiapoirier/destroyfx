/*---------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2001-2018  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Destroy FX Library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Destroy FX Library.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/

Sophia's Destroy FX MIDI stuff
---------------------------------------------------------------*/

#include "dfxmidi.h"

#include <algorithm>
#include <cmath>

#include "dfxmath.h"


//------------------------------------------------------------------------
DfxMidi::DfxMidi()
{
	fillFrequencyTable();

	setResumedAttackMode(false);

	reset();
}

//------------------------------------------------------------------------
void DfxMidi::reset()
{
	// zero out the note table, or what's important at least
	for (auto& note : mNoteTable)
	{
		note.mVelocity = 0;
		note.mEnvelope.setInactive();
		note.mLastOutValue = 0.0f;
		note.mSmoothSamples = 0;
		note.clearTail();
	}
	mSustainQueue.fill(false);

	// clear the ordered note queue
	removeAllNotes();

	// reset this counter since processEvents may not be called during the first block
	mNumBlockEvents = 0;
	// reset the pitchbend value to no bend
	mPitchBend = 1.0;
	// turn sustain pedal off
	mSustain = false;
}

//------------------------------------------------------------------------
void DfxMidi::setSampleRate(double inSampleRate)
{
	for (auto& note : mNoteTable)
	{
		note.mEnvelope.setSampleRate(inSampleRate);
	}
}

//------------------------------------------------------------------------
void DfxMidi::setEnvParameters(double inAttackDur, double inDecayDur, double inSustainLevel, double inReleaseDur)
{
	for (auto& note : mNoteTable)
	{
		note.mEnvelope.setParameters(inAttackDur, inDecayDur, inSustainLevel, inReleaseDur);
	}
}

//------------------------------------------------------------------------
void DfxMidi::setEnvCurveType(DfxEnvelope::CurveType inCurveType)
{
	for (auto& note : mNoteTable)
	{
		note.mEnvelope.setCurveType(inCurveType);
	}
}

//------------------------------------------------------------------------
void DfxMidi::setResumedAttackMode(bool inNewMode)
{
	for (auto& note : mNoteTable)
	{
		note.mEnvelope.setResumedAttackMode(inNewMode);
	}
}

//------------------------------------------------------------------------
void DfxMidi::preprocessEvents()
{
	// Sort the events in our queue so that they are in chronological order.
	// The host is supposed to send them in order, but just in case...
	std::stable_sort(mBlockEvents.begin(), std::next(mBlockEvents.begin(), mNumBlockEvents), [](auto const& a, auto const& b)
					 {
						 return a.mDelta < b.mDelta;
					 });
}

//------------------------------------------------------------------------
// zeroing mNumBlockEvents must be done at the end of each audio processing block 
// so that events are not reused in the next block(s) if no new events arrive
void DfxMidi::postprocessEvents()
{
	mNumBlockEvents = 0;
}

//-----------------------------------------------------------------------------
// this function inserts a new note into the beginning of the active notes queue
void DfxMidi::insertNote(int inMidiNote)
{
	// first check whether this note is already active (could happen in weird sequencers, like Max for example)
	auto const nonmatchPortion = std::stable_partition(mNoteQueue.begin(), mNoteQueue.end(), [inMidiNote](auto const& note)
													   {
														   return note == inMidiNote;
													   });
	// if the note is not already active, shift every note up a position (normal scenario)
	if (nonmatchPortion == mNoteQueue.begin())
	{
		std::rotate(mNoteQueue.begin(), std::prev(mNoteQueue.end()), mNoteQueue.end());
		// then place the new note into the first position
		mNoteQueue.front() = inMidiNote;
	}
}

//-----------------------------------------------------------------------------
// this function removes a note from the active notes queue
void DfxMidi::removeNote(int inMidiNote)
{
	auto const nonmatchPortion = std::stable_partition(mNoteQueue.begin(), mNoteQueue.end(), [inMidiNote](auto const& note)
													   {
														   return note != inMidiNote;
													   });
	std::fill(nonmatchPortion, mNoteQueue.end(), kInvalidValue);
}

//-----------------------------------------------------------------------------
// this function cancels all of the notes in the active notes queue
void DfxMidi::removeAllNotes()
{
	mNoteQueue.fill(kInvalidValue);
}

//-----------------------------------------------------------------------------
void DfxMidi::handleNoteOn(int inMidiChannel, int inNoteNumber, int inVelocity, long inBufferOffset)
{
	mBlockEvents[mNumBlockEvents].mStatus = kStatus_NoteOn;
	mBlockEvents[mNumBlockEvents].mChannel = inMidiChannel;
	mBlockEvents[mNumBlockEvents].mByte1 = inNoteNumber;
	mBlockEvents[mNumBlockEvents].mByte2 = inVelocity;
	mBlockEvents[mNumBlockEvents].mDelta = inBufferOffset;
	incNumEvents();
}

//-----------------------------------------------------------------------------
void DfxMidi::handleNoteOff(int inMidiChannel, int inNoteNumber, int inVelocity, long inBufferOffset)
{
	mBlockEvents[mNumBlockEvents].mStatus = kStatus_NoteOff;
	mBlockEvents[mNumBlockEvents].mChannel = inMidiChannel;
	mBlockEvents[mNumBlockEvents].mByte1 = inNoteNumber;
	mBlockEvents[mNumBlockEvents].mByte2 = inVelocity;
	mBlockEvents[mNumBlockEvents].mDelta = inBufferOffset;
	incNumEvents();
}

//-----------------------------------------------------------------------------
void DfxMidi::handleAllNotesOff(int inMidiChannel, long inBufferOffset)
{
	mBlockEvents[mNumBlockEvents].mStatus = kStatus_CC;
	mBlockEvents[mNumBlockEvents].mByte1 = kCC_AllNotesOff;
	mBlockEvents[mNumBlockEvents].mChannel = inMidiChannel;
	mBlockEvents[mNumBlockEvents].mDelta = inBufferOffset;
	incNumEvents();
}

//-----------------------------------------------------------------------------
void DfxMidi::handlePitchBend(int inMidiChannel, int inValueLSB, int inValueMSB, long inBufferOffset)
{
	mBlockEvents[mNumBlockEvents].mStatus = kStatus_PitchBend;
	mBlockEvents[mNumBlockEvents].mChannel = inMidiChannel;
	mBlockEvents[mNumBlockEvents].mByte1 = inValueLSB;
	mBlockEvents[mNumBlockEvents].mByte2 = inValueMSB;
	mBlockEvents[mNumBlockEvents].mDelta = inBufferOffset;
	incNumEvents();
}

//-----------------------------------------------------------------------------
void DfxMidi::handleCC(int inMidiChannel, int inControllerNumber, int inValue, long inBufferOffset)
{
	// only handling sustain pedal for now...
	if (inControllerNumber == kCC_SustainPedalOnOff)
	{
		mBlockEvents[mNumBlockEvents].mStatus = kStatus_CC;
		mBlockEvents[mNumBlockEvents].mByte1 = inControllerNumber;
		mBlockEvents[mNumBlockEvents].mChannel = inMidiChannel;
		mBlockEvents[mNumBlockEvents].mByte2 = inValue;
		mBlockEvents[mNumBlockEvents].mDelta = inBufferOffset;
		incNumEvents();
	}
}

//-----------------------------------------------------------------------------
void DfxMidi::handleProgramChange(int inMidiChannel, int inProgramNumber, long inBufferOffset)
{
	mBlockEvents[mNumBlockEvents].mStatus = kStatus_ProgramChange;
	mBlockEvents[mNumBlockEvents].mChannel = inMidiChannel;
	mBlockEvents[mNumBlockEvents].mByte1 = inProgramNumber;
	mBlockEvents[mNumBlockEvents].mDelta = inBufferOffset;
	incNumEvents();
}


//-----------------------------------------------------------------------------------------
// this function is called during process() when MIDI events need to be attended to
void DfxMidi::heedEvents(long inEventNum, double inPitchBendRange, bool inLegato, 
						 float inVelocityCurve, float inVelocityInfluence)
{
	switch (mBlockEvents[inEventNum].mStatus)
	{

// --- NOTE-ON RECEIVED ---
		case kStatus_NoteOn:
		{
			auto const currentNote = mBlockEvents[inEventNum].mByte1;
			mNoteTable[currentNote].mVelocity = mBlockEvents[inEventNum].mByte2;
			mNoteTable[currentNote].mNoteAmp = (std::pow(kValueScalar * static_cast<float>(mNoteTable[currentNote].mVelocity), inVelocityCurve) * 
												inVelocityInfluence) + (1.0f - inVelocityInfluence);
			//
			if (inLegato)  // legato is on, fade out the last note and fade in the new one, supershort
			{
/* XXX TODO: implement real legato
				// this is false until we find some already active note
				bool legatoNoteFound = false;
				// find the previous note and set it to fade out
				for (int notecount = 0; notecount < kNumNotes; notecount++)
				{
					// we want to find the active note, but not this new one
					if ((mNoteTable[notecount].mVelocity) && (notecount != currentNote) && (mNoteTable[notecount].mEnvelope.isInactive()))
					{
						// if the note is currently fading in, pick up where it left off
						if (mNoteTable[notecount].mEnvelope.getState() == DfxEnvelope::State::Attack)
						{
							mNoteTable[notecount].releaseSamples = mNoteTable[notecount].attackSamples;
						}
						// otherwise do the full fade out duration, if the note is not already fading out
						else if (mNoteTable[notecount].mEnvelope.getState() == DfxEnvelope::State::Release)
						{
							mNoteTable[notecount].releaseSamples = kLegatoFadeDur;
						}
						mNoteTable[notecount].releaseDur = kLegatoFadeDur;
						// we found an active note (that's the same as the new incoming note)
						legatoNoteFound = true;
					}
				}
				// don't start a new note fade-in if the currently active note is the same as this new note
				if (!(!legatoNoteFound && mNoteTable[currentNote].mVelocity))
				{
					// legato mode always uses this short fade
					mNoteTable[currentNote].attackDur = kLegatoFadeDur;
					// attackSamples starts counting from zero, so set it to zero
					mNoteTable[currentNote].attackSamples = 0;
					// calculate how far this fade must "step" through at each sample.
					// Since legato mode overrides the fades parameter and only does cheap fades, this 
					// isn't really necessary, but since I don't trust that everything will work right...
					mNoteTable[currentNote].linearFadeStep = kLegatoFadeStep;
				}
*/
			}
			//
			else  // legato is off, so set up for the attack envelope
			{
				mNoteTable[currentNote].mEnvelope.beginAttack();
				// if the note is still sounding and in release, then smooth the end of that last note
				if (!(mNoteTable[currentNote].mEnvelope.isResumedAttackMode()) && (mNoteTable[currentNote].mEnvelope.getState() == DfxEnvelope::State::Release))
				{
					mNoteTable[currentNote].mSmoothSamples = kStolenNoteFadeDur;
				}
			}
			break;
		}



// --- NOTE-OFF RECEIVED ---
		case kStatus_NoteOff:
		{
			auto const currentNote = mBlockEvents[inEventNum].mByte1;
			// don't process this note off, but do remember it, if the sustain pedal is on
			if (mSustain)
			{
				mSustainQueue[currentNote] = true;
			}
			else
			{
				turnOffNote(currentNote, inLegato);
			}
			break;
		}



// --- PITCHBEND RECEIVED ---
		case kStatus_PitchBend:
		{
			constexpr auto upperRange = static_cast<double>(kPitchBendMaxValue - kPitchBendMidpointValue);
			constexpr auto lowerRange = static_cast<double>(kPitchBendMidpointValue);
			int const pitchbend14bit = (mBlockEvents[inEventNum].mByte2 * (kMaxValue + 1)) + mBlockEvents[inEventNum].mByte1;
			// bend pitch up
			if (pitchbend14bit >= kPitchBendMidpointValue)
			{
				// scale the MIDI value from 0.0 to 1.0
				mPitchBend = static_cast<double>(pitchbend14bit - kPitchBendMidpointValue) / upperRange;
			}
			// bend pitch down
			else
			{
				// scale the MIDI value from -1.0 to 0.0
				mPitchBend = static_cast<double>(pitchbend14bit - kPitchBendMidpointValue) / lowerRange;
			}
			// then scale it according to tonal steps and the user defined range
			mPitchBend = dfx::math::FrequencyScalarBySemitones(mPitchBend * inPitchBendRange);
			break;
		}



// --- CONTROLLER CHANGE RECEIVED ---
		case kStatus_CC:
		{
			switch (mBlockEvents[inEventNum].mByte1)
			{
	// --- SUSTAIN PEDAL RECEIVED ---
				case kCC_SustainPedalOnOff:
					if (mSustain && (mBlockEvents[inEventNum].mByte2 <= 63))
					{
						for (int i = 0; i < static_cast<int>(mSustainQueue.size()); i++)
						{
							if (mSustainQueue[i])
							{
								turnOffNote(i, inLegato);
								mSustainQueue[i] = false;
							}
						}
					}
					mSustain = (mBlockEvents[inEventNum].mByte2 >= kMidpointValue);
					break;

	// --- ALL-NOTES-OFF RECEIVED ---
				// all sound off, so call suspend() to wipe out all of the notes and buffers
				case kCC_AllNotesOff:
				{
					// and zero out the note table, or what's important at least
					for (auto& note : mNoteTable)
					{
						note.mVelocity = 0;
						note.mEnvelope.setInactive();
					}
					break;
				}

				default:
					break;
			}
			break;  // kStatus_CC
		}

		default:
			break;
	}
}

//-------------------------------------------------------------------------
float DfxMidi::processEnvelope(int inMidiNote)
{
	auto const outputAmp = mNoteTable[inMidiNote].mEnvelope.process();
	if (mNoteTable[inMidiNote].mEnvelope.getState() == DfxEnvelope::State::Dormant)
	{
		mNoteTable[inMidiNote].mVelocity = 0;
	}

	return outputAmp;
}

//-------------------------------------------------------------------------
// this function writes the audio output for smoothing the tips of cut-off notes
// by sloping down from the last sample outputted by the note
void DfxMidi::processSmoothingOutputSample(float* outAudio, long inNumSamples, int inMidiNote)
{
	for (long sampleIndex = 0; sampleIndex < inNumSamples; sampleIndex++)
	{
		// add the latest sample to the output collection, scaled by the note envelope and user gain
		float outputFadeScalar = static_cast<float>(mNoteTable[inMidiNote].mSmoothSamples * kStolenNoteFadeStep);
		outputFadeScalar = outputFadeScalar * outputFadeScalar * outputFadeScalar;
		outAudio[sampleIndex] += mNoteTable[inMidiNote].mLastOutValue * outputFadeScalar;
		// decrement the smoothing counter
		(mNoteTable[inMidiNote].mSmoothSamples)--;
		// exit this function if we've done all of the smoothing necessary
		if (mNoteTable[inMidiNote].mSmoothSamples <= 0)
		{
			return;
		}
	}
}

//-------------------------------------------------------------------------
// this function writes the audio output for smoothing the tips of cut-off notes
// by fading out the samples stored in the tail buffers
void DfxMidi::processSmoothingOutputBuffer(float* outAudio, long inNumSamples, int inMidiNote, int inMidiChannel)
{
	auto& smoothsamples = mNoteTable[inMidiNote].mSmoothSamples;
	auto const& tail = (inMidiChannel == 1) ? mNoteTable[inMidiNote].mTail1 : mNoteTable[inMidiNote].mTail2;

	for (long sampleIndex = 0; (sampleIndex < inNumSamples) && (smoothsamples > 0); sampleIndex++, smoothsamples--)
	{
		outAudio[sampleIndex] += tail[kStolenNoteFadeDur - smoothsamples] * 
		static_cast<float>(smoothsamples) * kStolenNoteFadeStep;
	}
}

//-----------------------------------------------------------------------------------------
// this function fills a table with the correct frequency for every MIDI note
void DfxMidi::fillFrequencyTable()
{
	constexpr double standardConcertA = 440.0;
	int semitonesFromA = -69;
	for (auto& freq : mNoteFrequencyTable)
	{
		freq = standardConcertA * dfx::math::FrequencyScalarBySemitones(static_cast<double>(semitonesFromA));
		semitonesFromA++;
	}
}

//-----------------------------------------------------------------------------
bool DfxMidi::incNumEvents()
{
	mNumBlockEvents++;
	// don't go past the allocated space for the events queue
	// TODO: actually truncating capacity by one event because of the way that events are added, could be fixed
	if (mNumBlockEvents >= static_cast<long>(mBlockEvents.size()))
	{
		mNumBlockEvents = static_cast<long>(mBlockEvents.size()) - 1;
		return false;  // ! abort !
	}
	return true;  // successful increment
}

//-----------------------------------------------------------------------------------------
void DfxMidi::turnOffNote(int inMidiNote, bool inLegato)
{
	// legato is off (note-offs are ignored when it's on)
	// go into the note release if legato is off and the note isn't already off
	if (!inLegato && (mNoteTable[inMidiNote].mVelocity > 0))
	{
		mNoteTable[inMidiNote].mEnvelope.beginRelease();
		// make sure to turn the note off now if there is no release
		if (mNoteTable[inMidiNote].mEnvelope.isInactive())
		{
			mNoteTable[inMidiNote].mVelocity = 0;
		}
	}
}



//-----------------------------------------------------------------------------------------



/* I wrote this in an email to someone explaining how my MIDI handling works.  
   I figured it was worth throwing in here for anyone else who might look at my source code.
   It's a step by step explanation of what happens.

NOTE:  I've made many changes to this code but haven't updated the info below, 
so don't be surprised if it doesn't match up with the current code.

	In processEvents(), I receive a VstEvent array (which includes a 
counter value for how many items there are in that particular array) and 
then I look at every item.  First I check if it's a MIDI event (like in 
the SDK).  If it is, then I cast it into my VstMidiEvent variable (like in 
the SDK) and then start examining it.  I only want to deal with it if is 
one of 3 types of MIDI events:  a note, a pitchbend message, or a panic 
message.  That is what each of the next three "if"s are all about.  If the 
event is any one of those 3 things, then it gets worked on.
	You are right that the struct array mBlockEvents[] is my queue.  I fill 
up one item in that array for each interesting event that I receive during 
that processing block.  I store the status in my own way (using an enum 
that's in my header) as either kStatus_NoteOn, kStatus_NoteOff, kStatus_PitchBend, 
or kCC_AllNotesOff.  This is what goes in the mBlockEvents.mStatus field.  
Then I take MIDI bytes 1 and 2 and put them into mBlockEvents.mByte1 and .mByte2.  
If it's a note off message, then I put 0 into mByte2 (velocity).  By the 
way, with pitchbend, byte 1 is the LSB, but since LSB is, so far as I know, 
inconsistantly implemented by different MIDI devices, my plugin doesn't 
actually use that value in any way.  At the end of each of these "if"s, I 
update mNumBlockEvents because that is my counter that I look at during 
process() to see how many events I have to deal with during that block.
	And for each event, I store the deltaFrames value.  deltaFrames is the 
number of samples into that processing block that the event occurs.  This 
is what makes sample accurate timing (on MIDI data playback, not with live 
playing, of course) possible.  A VST host will send all of the upcoming 
events for a giving processing block to processEvents() and then the exact 
position within that processing block is given by deltaFrames.  If 
deltaFrames is 0, then the event occurs at the very beginning of the block.  
If deltaFrames is 333, then it occurs 333 samples into that processing 
block.  While it is not required, the SDK claims that hosts generally, as 
a matter of convention, send VstEvents to processEvents() in chronological 
order.  My plugin assumes that they are received in order.  There is no 
sorting done in my plugin.
	Now on to process().  Basically, I divide my process up into 
sub-chunks according to when events occur (which means according to the 
deltaFrames values).  I have two important variables here:   eventcount 
and currentBlockPosition.  The eventcount keeps track of how many of the 
events for that block I have addressed.  I initialize it to -1 so that 
first it will do processing up until the first event and then it will 
start counting events at 0 with the first event.  This is because there 
most likely will be audio to process before any events occur during that 
block (unless the block began with all notes off).  currentBlockPosition 
stores the sample start position of the current sub-chunk.  Basically it 
is the deltaFrames values of the latest event that I am working on.  It 
obviously starts out at 0.
	Next I start a "do" loop that cycles for every event.  First it 
evaluates the duration of the current processing sub-chunk, so it first 
checks to see if there are any more upcoming events in the queue.  If not, 
then the current chunk processes to the end of the processing block, if 
yes, then the current sub-chunk position is subtracted from the upcoming 
event's deltaFrames value.  I move up inputs and outputs accordingly, etc.
	Next comes a "for" loop that goes through my mNoteTable struct array 
and looks for any active notes (i.e. notes with a non-zero velocity value).  
All that I do during this loop is check the velocity of each note and then 
process the audio for that note if it has a non-zero velocity.
	After that "for" loop, I increment eventcount, leave the events loop 
if events are done, update currentBlockPosition, and then call heedEvents().  
heedEvents() is important.  heedEvents() is where I take in the effects of 
the next MIDI event.  Basically I tell heedEvents() which event number I 
am looking at and then it updates any vital stuff so that, when going 
through the next processing sub-chunk, all necessary changes have been 
made to make the next batch of processing take into account the impact of 
the latest event.  So heedEvents() is pretty much just a switch statement 
checking out which type of event is being analyzed and then implementing it 
in the appropriate way.  It would take quite a while to fully explain what 
happens in there and why, so I'll leave it up to you to determine whichever 
parts of it you want to figure out.  I don't think that it is very 
complicated (just many steps), but I could be wrong and if you can't figure 
out any parts of it, let me know and I'll explain.
	There is one more really important thing in process().  At the of the 
whole function, I reset mNumBlockEvents (the global events counter) to 0.  
This is extremely important because processEvents() only gets called if 
new events are received during a processing block.  If not, then 
processEvents() does not get called, mNumBlockEvents does not get zeroed 
at the beginning of processEvents(), and process() will process the same 
MIDI events over and over and over for every processing block until a new 
MIDI event is received.  This fact about processEvents() is not explained 
in the SDK and I spent FOREVER with a malfunctioning plugin until I 
figured this out.
*/
