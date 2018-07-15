/*------------------------------------------------------------------------
Copyright (C) 2001-2018  Sophia Poirier

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

#include <cmath>

#include "dfxmath.h"


void RezSynth::processaudio(float const* const* inAudio, float* const* outAudio, unsigned long inNumFrames, bool replacing)
{
	auto const numChannels = getnumoutputs();
	auto numFramesToProcess = inNumFrames;  // for dividing up the block accoring to events
	float dryGain = 1.0f - mDryWetMix;  // the gain of the input audio in the dry/wet mix
	mWetGain = mDryWetMix;
	if (mDryWetMixMode == kDryWetMixMode_EqualPower)
	{
		// sqare root for equal power blending of non-correlated signals
		dryGain = std::sqrt(dryGain);
		mWetGain = std::sqrt(mWetGain);
	}


#ifndef TARGET_API_AUDIOUNIT
	// mix very quiet noise (-300 dB) into the input signal to hopefully avoid any denormal values
	float quietNoise = 1.0e-15f;
	for (unsigned long ch = 0; ch < numChannels; ch++)
	{
		auto const volatileIn = const_cast<float*>(inAudio[ch]);
		for (unsigned long samplecount = 0; samplecount < inNumFrames; samplecount++)
		{
			volatileIn[samplecount] += quietNoise;
			quietNoise = -quietNoise;
		}
	}
#endif


	// counter for the number of MIDI events this block
	// start at -1 because the beginning stuff has to happen
	long eventcount = -1;
	unsigned long currentBlockPosition = 0;  // we are at sample 0


	// now we're ready to start looking at MIDI messages and processing sound and such
	do
	{
		// check for an upcoming event and decrease this block chunk size accordingly 
		// if there won't be another event, go all the way to the end of the block
		if ((eventcount + 1) >= getmidistate().getBlockEventCount())
		{
			numFramesToProcess = inNumFrames - currentBlockPosition;
		}
		// else there will be and this chunk goes up to the next delta position
		else
		{
			numFramesToProcess = getmidistate().getBlockEvent(eventcount + 1).mOffsetFrames - currentBlockPosition;
		}

		// this means that 2 (or more) events occur simultaneously, 
		// so there's no need to do calculations during this round
		if (numFramesToProcess == 0)
		{
			eventcount++;
			checkForNewNote(eventcount, numChannels);  // and attend to related issues if necessary
			// take in the effects of the next event
			getmidistate().heedEvents(eventcount, mPitchBendRange, mLegato, mVelocityCurve, mVelocityInfluence);
			continue;
		}

		// test for whether or not all notes are off and unprocessed audio can be outputted
		bool noNotes = true;  // none yet for this chunk

		for (int notecount = 0; notecount < DfxMidi::kNumNotes; notecount++)
		{
			// only go into the output processing cycle if a note is happening
			if (getmidistate().getNoteState(notecount).mVelocity)
			{
				noNotes = false;  // we have a note

				double const ampEvener = mWiseAmp ? calculateAmpEvener(notecount) : 1.0;  // a scalar for balancing outputs from the 3 normalizing modes

				// this is the resonator stuff
				auto const activeNumBands = calculateCoefficients(notecount);

				// most of the note values are liable to change during processFilterOuts,
				// so we back them up to allow multi-channel repetition
				DfxMidi::MusicNote const noteState_temp = getmidistate().getNoteState(notecount);
				// render the filtered audio output for the note for each audio channel
				for (unsigned long ch = 0; ch < numChannels; ch++)
				{
					// restore the note values before doing processFilterOuts for the next channel
					getmidistate().setNoteState(notecount, noteState_temp);

					processFilterOuts(&(inAudio[ch][currentBlockPosition]), &(outAudio[ch][currentBlockPosition]), 
									  numFramesToProcess, ampEvener, notecount, activeNumBands, 
									  mPrevInValue[ch][notecount], mPrevPrevInValue[ch][notecount], 
									  mPrevOutValue[ch][notecount].data(), mPrevPrevOutValue[ch][notecount].data());
				}
			}
		}  // end of notes loop

		// we had notes this chunk, but the unaffected processing hasn't faded out, so change its state to fade-out
		if (!noNotes && (mUnaffectedState == UnaffectedState::Flat))
		{
			mUnaffectedState = UnaffectedState::FadeOut;
		}

		// we can output unprocessed audio if no notes happened during this block chunk
		// or if the unaffected fade-out still needs to be finished
		if (noNotes || (mUnaffectedState == UnaffectedState::FadeOut))
		{
			auto const entryUnaffectedState = mUnaffectedState;
			auto const entryUnaffectedFadeSamples = mUnaffectedFadeSamples;
			for (unsigned long ch = 0; ch < numChannels; ch++)
			{
				mUnaffectedState = entryUnaffectedState;
				mUnaffectedFadeSamples = entryUnaffectedFadeSamples;
				processUnaffected(&(inAudio[ch][currentBlockPosition]), &(outAudio[ch][currentBlockPosition]), numFramesToProcess);
			}
		}

		eventcount++;
		// don't do the event processing below if there are no more events
		if (eventcount >= getmidistate().getBlockEventCount())
		{
			continue;
		}

		// jump our position value forward
		currentBlockPosition = getmidistate().getBlockEvent(eventcount).mOffsetFrames;

		checkForNewNote(eventcount, numChannels);  // and attend to related issues if necessary
		// take in the effects of the next event
		getmidistate().heedEvents(eventcount, mPitchBendRange, mLegato, mVelocityCurve, mVelocityInfluence);

	} while (eventcount < getmidistate().getBlockEventCount());

	// mix in the dry input (only if there is supposed to be some dry; let's not waste calculations)
	if (mDryWetMix < 1.0f)
	{
		for (unsigned long ch = 0; ch < numChannels; ch++)
		{
			for (unsigned long samplecount = 0; samplecount < inNumFrames; samplecount++)
			{
				outAudio[ch][samplecount] += inAudio[ch][samplecount] * dryGain;
			}
		}
	}
}
