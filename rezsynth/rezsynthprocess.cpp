/*------------------------------------------------------------------------
Copyright (C) 2001-2023  Sophia Poirier

This file is part of Rez Synth.

Rez Synth is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Rez Synth is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rez Synth.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#include "rezsynth.h"

#include <algorithm>

#include "dfxmath.h"


void RezSynth::processaudio(float const* const* inAudio, float* const* outAudio, size_t inNumFrames)
{
	auto const numChannels = getnumoutputs();
	auto numFramesToProcess = inNumFrames;  // for dividing up the block according to events


	if constexpr (dfx::math::kDenormalProblem)
	{
		// mix very quiet noise (-300 dB) into the input signal to hopefully avoid any denormal values
		float quietNoise = 1.0e-15f;
		for (size_t ch = 0; ch < numChannels; ch++)
		{
			auto const volatileIn = const_cast<float*>(inAudio[ch]);
			for (size_t samplecount = 0; samplecount < inNumFrames; samplecount++)
			{
				volatileIn[samplecount] += quietNoise;
				quietNoise = -quietNoise;
			}
		}
	}


	// counter for the number of MIDI events this block
	// start at -1 because the beginning stuff has to happen
	long eventcount = -1;
	size_t currentBlockPosition = 0;  // we are at sample 0


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

		// this means that two (or more) events occur simultaneously,
		// so there's no need to do calculations during this round
		if (numFramesToProcess == 0)
		{
			eventcount++;
			checkForNewNote(eventcount);  // and attend to related issues if necessary
			// take in the effects of the next event
			getmidistate().heedEvents(eventcount, mVelocityCurve, mVelocityInfluence);
			continue;
		}

		// test for whether all notes are off and unprocessed audio can be outputted
		bool notesActive = false;  // none yet for this chunk

		// these will increment for every note on every channel, so restore before iterations
		auto const entryOutputGain = mOutputGain;
		auto const entryWetGain = mWetGain;
		for (int noteIndex = 0; noteIndex < DfxMidi::kNumNotesWithLegatoVoice; noteIndex++)
		{
			// only go into the output processing cycle if a note is happening
			if (getmidistate().isNoteActive(noteIndex))
			{
				notesActive = true;

				mAmpEvener[noteIndex] = calculateAmpEvener(noteIndex);  // a scalar for balancing outputs from the normalizing modes

				for (size_t subSlicePosition = 0; subSlicePosition < numFramesToProcess; )
				{
					// this is the resonator stuff
					auto const activeNumBands = calculateCoefficients(noteIndex);

					// if a note is just begin, skip smoothing for all of the per-note smoothed parameter values
					if (!mNoteActiveLastRender[noteIndex] && (subSlicePosition == 0))
					{
						mAmpEvener[noteIndex].snap();
						mBaseFreq[noteIndex].snap();
						std::ranges::for_each(mBandCenterFreq[noteIndex], [](auto& value){ value.snap(); });
						std::ranges::for_each(mBandBandwidth[noteIndex], [](auto& value){ value.snap(); });
					}

					auto const subSliceFrameCount = [this, numFramesToProcess, subSlicePosition, noteIndex, activeNumBands]
					{
						auto const valueIsSmoothing = [](auto const& value){ return value.isSmoothing(); };
						auto const freqIsSmoothing = mBaseFreq[noteIndex].isSmoothing()
						|| std::any_of(mBandCenterFreq[noteIndex].cbegin(),
									   std::next(mBandCenterFreq[noteIndex].cbegin(), activeNumBands),
									   valueIsSmoothing)
						|| std::any_of(mBandBandwidth[noteIndex].cbegin(),
									   std::next(mBandBandwidth[noteIndex].cbegin(), activeNumBands),
									   valueIsSmoothing);
						auto const remainingFrames = numFramesToProcess - subSlicePosition;
						return freqIsSmoothing ? std::min(mFreqSmoothingStride, remainingFrames) : remainingFrames;
					}();

					// restore values before doing processFilterOuts for the next channel
					mOutputGain = entryOutputGain;
					mWetGain = entryWetGain;

					// render the filtered audio output for the note
					processFilterOuts(inAudio, outAudio,
									  currentBlockPosition + subSlicePosition, subSliceFrameCount,
									  noteIndex, activeNumBands);

					mBaseFreq[noteIndex].inc(subSliceFrameCount);
					std::ranges::for_each(mBandCenterFreq[noteIndex], [subSliceFrameCount](auto& value){ value.inc(subSliceFrameCount); });
					std::ranges::for_each(mBandBandwidth[noteIndex], [subSliceFrameCount](auto& value){ value.inc(subSliceFrameCount); });

					subSlicePosition += subSliceFrameCount;
				}
			}

			// could be because it already was inactive, or because we just completed articulation of the note
			if (!getmidistate().isNoteActive(noteIndex))
			{
				std::ranges::for_each(mLowpassGateFilters[noteIndex], [](auto& filter){ filter.reset(); });
			}

			mNoteActiveLastRender[noteIndex] = getmidistate().isNoteActive(noteIndex);
		}  // end of notes loop

		if (!notesActive)
		{
			mOutputGain.snap();
		}

		// we had notes this chunk, but the unaffected processing hasn't faded out, so change its state to fade-out
		if (notesActive && (mUnaffectedState == UnaffectedState::Flat))
		{
			mUnaffectedState = UnaffectedState::FadeOut;
		}

		// we can output unprocessed audio if no notes happened during this block chunk
		// or if the unaffected fade-out still needs to be finished
		if (!notesActive || (mUnaffectedState == UnaffectedState::FadeOut))
		{
			auto const entryUnaffectedState = mUnaffectedState;
			auto const entryUnaffectedFadeSamples = mUnaffectedFadeSamples;
			auto const entryBetweenGain = mBetweenGain;
			auto const entryUnaffectedWetGain = notesActive ? entryWetGain : mWetGain;
			for (size_t ch = 0; ch < numChannels; ch++)
			{
				mUnaffectedState = entryUnaffectedState;
				mUnaffectedFadeSamples = entryUnaffectedFadeSamples;
				mBetweenGain = entryBetweenGain;
				mWetGain = entryUnaffectedWetGain;
				processUnaffected(&(inAudio[ch][currentBlockPosition]), &(outAudio[ch][currentBlockPosition]), numFramesToProcess);
			}
		}
		else
		{
			mBetweenGain.snap();
			if (!notesActive)
			{
				mWetGain.snap();
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

		checkForNewNote(eventcount);  // and attend to related issues if necessary
		// take in the effects of the next event
		getmidistate().heedEvents(eventcount, mVelocityCurve, mVelocityInfluence);

	} while (eventcount < getmidistate().getBlockEventCount());

	// mix in the dry input (only if there is supposed to be some dry; let's not waste calculations)
	if ((mDryGain.getValue() > 0.0f) || mDryGain.isSmoothing())
	{
		auto const entryDryGain = mDryGain;
		for (size_t ch = 0; ch < numChannels; ch++)
		{
			mDryGain = entryDryGain;
			for (size_t samplecount = 0; samplecount < inNumFrames; samplecount++)
			{
				outAudio[ch][samplecount] += inAudio[ch][samplecount] * mDryGain.getValue();
				mDryGain.inc();
			}
		}
	}
}
