/*------------------------------------------------------------------------
Copyright (C) 2002-2024  Sophia Poirier

This file is part of Scrubby.

Scrubby is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Scrubby is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Scrubby.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#include "scrubby.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <numbers>

#include "dfxmath.h"


//-----------------------------------------------------------------------------
// computes the principle branch of the Lambert W function
// { LambertW(x) = W(x), where W(x) * exp(W(x)) = x }
inline double LambertW(double inValue)
{
	auto const x = std::fabs(inValue);
	if (x <= 500.)
	{
		return 0.665 * (1. + (0.0195 * std::log(x + 1.))) * std::log(x + 1.) + 0.04;
	}
	else
	{
		return std::log(x - 4.) - ((1. - 1. / std::log(x)) * log(std::log(x)));
	}
}

//-----------------------------------------------------------------------------------------
inline double calculateTargetSpeed(double a, double n, double k)
{
	a = std::fabs(a);
	n = std::fabs(n);
	k = std::fabs(k);

	double const lambertInput = (n * a) / k;
	double b = k * LambertW(lambertInput) / n;  // the target speed
	// cuz I don't totally trust my Lambert W function...
	if (!std::isfinite(b))
	{
		b = 1.0;
	}
//	else
	{
//		b *= 2.0;
	}
	if (b < 1.0)
	{
		b = std::pow(b, 0.63);
	}

/*
	double inc = 0.;
	double testk = k * 2.;
	b = a;
	// b is less than a, guess downwards
	if (k < (n * a))
	{
		inc = 0.5;
	}
	// b is larger than a, guess upwards
	else
	{
		inc = 2.;
	}
	while (std::fabs((testk / k) - 1.) > 0.01)
//	while (testk != k)
	for (int i = 0; i < 90; i++)
	{
		b *= inc;
		testk = std::fabs(((b - a) * n) / std::log(b / a));
		if (testk < k)
		{
			b /= inc;
			inc = std::sqrt(inc);
		}
		else if (testk == k)
		{
			break;
		}
	}
*/
//	b = k / n;

	return b;
}


//-----------------------------------------------------------------------------------------
void Scrubby::checkTempoSyncStuff()
{
	// figure out the current tempo if we're doing tempo sync
	if (mTempoSync)
	{
		// calculate the tempo at the current processing buffer
		if (mUseHostTempo && hostCanDoTempo() && gettimeinfo().mTempoBPS)  // get the tempo from the host
		{
			mCurrentTempoBPS = *gettimeinfo().mTempoBPS;
			// check if audio playback has just restarted and reset buffer stuff if it has (for measure sync)
			if (gettimeinfo().mPlaybackChanged)
			{
				std::ranges::fill(mNeedResync, true);
			}
		}
		else  // get the tempo from the user parameter
		{
			mCurrentTempoBPS = mUserTempo / 60.0;
			std::ranges::fill(mNeedResync, false);  // we don't want it true if we're not syncing to host tempo
		}
	}
	else
	{
		std::ranges::fill(mNeedResync, false);  // we don't want it true if we're not syncing to host tempo
	}

	// reset cycle state stuff if playback has changed (for measure sync)
	auto const numChannels = getnumoutputs();
	for (size_t ch = 0; ch < numChannels; ch++)
	{
		if (mNeedResync[ch])
		{
			mSeekCount[ch] = 0;
		}
	}
}

//-----------------------------------------------------------------------------------------
void Scrubby::generateNewTarget(size_t channel)
{
	double currentSeekRate {};

// CALCULATE THE SEEK CYCLE LENGTH
	// match the seek rate to tempo
	if (mTempoSync)
	{
		// randomize the tempo rate if the random min scalar is lower than the upper bound
		if (mUseSeekRateRandMin)
		{
			auto const randomizedSeekRateIndex = mRandomEngine.next(mSeekRateRandMinIndex, mSeekRateIndex);
			currentSeekRate = mTempoRateTable.getScalar(randomizedSeekRateIndex);
		}
		else
		{
			currentSeekRate = mSeekRateSync;
		}
		// convert the tempo rate into rate in terms of Hz
		currentSeekRate *= mCurrentTempoBPS;
	}
	//
	// use the seek rate in terms of Hz
	else
	{
		if (mUseSeekRateRandMin)
		{
			currentSeekRate = expandparametervalue(kSeekRate_Hz, mRandomEngine.next(mSeekRateRandMinHz_gen, mSeekRateHz_gen));
		}
		else
		{
			currentSeekRate = mSeekRateHz;
		}
	}
	// calculate the length of this seek cycle in seconds
	double cycleDur = 1.0 / currentSeekRate;
	// then calculate the length of this seek cycle in samples
	mSeekCount[channel] = std::lround(cycleDur * getsamplerate());
	//
	// do bar sync if we're in tempo sync and things resyncing is in order
	if (mNeedResync[channel] && gettimeinfo().mSamplesToNextBar)
	{
		auto const samplesUntilBar = std::lround(*gettimeinfo().mSamplesToNextBar);
		// because a 0 for mSeekCount will be soon turned into 1, which is not so good for DJ mode
		if (samplesUntilBar > 0)
		{
			if (mSpeedMode == kSpeedMode_DJ)
			{
				// doubling the length of the first seek after syncing to the measure
				// seems to prevent Cubase from freaking out in tempo sync'd DJ mode
				mSeekCount[channel] = (samplesUntilBar + mSeekCount[channel]) % (mSeekCount[channel] * 2);
			}
			else
			{
				mSeekCount[channel] = samplesUntilBar % mSeekCount[channel];
			}
			cycleDur = static_cast<double>(mSeekCount[channel]) / getsamplerate();
		}
	}
	mSeekCount[channel] = std::max(mSeekCount[channel], 1L);

// CALCULATE THE MOVEMENT CYCLE LENGTH
	auto const currentSeekDur = mUseSeekDurRandMin ? mRandomEngine.next(mSeekDurRandMin, mSeekDur) : mSeekDur;
	// calculate the length of the seeking movement in samples
	mMoveCount[channel] = std::max(std::lround(cycleDur * currentSeekDur * getsamplerate()), 1L);

// FIND THE NEW TARGET POSITION IN THE BUFFER
	// randomly locate a new target position within the buffer seek range
	long const bufferSize = std::lround(mSeekRangeSeconds * getsamplerate());
	// search back from the current writer input point
	long const newTargetPos = mWritePos - mRandomEngine.next<long>(0, bufferSize);
	//
	// calculate the distance between
	auto readPosInt = static_cast<long>(mReadPos[channel]);
	if (readPosInt >= mWritePos)
	{
		readPosInt -= mMaxAudioBufferSize;
	}
//	if (std::abs(readPosInt - mWritePos) > (mMaxAudioBufferSize / 2))
//	{
//		if (readPosInt >= mWritePos)
//		{
//			readPosInt -= mMaxAudioBufferSize;
//		}
//		else
//		{
//			readPosInt += mMaxAudioBufferSize;
//		}
//	}
	long targetDistance = newTargetPos - readPosInt;
	if (targetDistance == 0)
	{
		targetDistance = 1;
	}
	//
	// calculate the step size of playback movement through the buffer
	double newReadStep = static_cast<double>(targetDistance) / static_cast<double>(mMoveCount[channel]);
	// constrain the speed to a semitone step, if that's what we want to do
	if (mPitchConstraint && (mSpeedMode == kSpeedMode_Robot))
	{
		newReadStep = processPitchConstraint(newReadStep);
	}
	// still constrain to octave ranges, even when pitch constraint is disabled
	else
	{
		double const direction_f = (newReadStep < 0.0) ? -1.0 : 1.0;  // direction scalar
		auto const newReadStep_abs = std::fabs(newReadStep);
		// constrain to octaves range, if we're doing that
		if ((mOctaveMin > kOctave_MinValue) && (newReadStep_abs < 1.0))
		{
			double const minstep = std::pow(2.0, static_cast<double>(mOctaveMin));
			if (newReadStep_abs < minstep)
			{
				newReadStep = minstep * direction_f;
			}
		}
		else if ((mOctaveMax < kOctave_MaxValue) && (newReadStep_abs > 1.0))
		{
			auto const maxStep = std::pow(2.0, static_cast<double>(mOctaveMax));
			if (newReadStep_abs > maxStep)
			{
				newReadStep = maxStep * direction_f;
			}
		}
	}
	//
	// calculate the step size of portamento playback incrementation
//	if ((mSpeedMode == kSpeedMode_DJ) && !mNeedResync[channel])
	if (mSpeedMode == kSpeedMode_DJ)
	{
		bool const moveBackwards = (newReadStep < 0.0);  // will we move backwards in the delay buffer?
		double oldReadStep = std::fabs(mReadStep[channel]);
		//
#if USE_LINEAR_ACCELERATION
		auto const absReadStep = std::fabs(newReadStep);
		bool const slowdown = (absReadStep < oldReadStep);  // are we slowing down or speeding up?
		double const stepDifference = absReadStep - oldReadStep;
		bool const crossZero = (slowdown && (stepDifference > absReadStep));  // will we go down to 0 Hz and then back up again?
		mPortamentoStep[channel] = (stepDifference * 2.0) / static_cast<double>(mMoveCount[channel]);
		if (moveBackwards)
		{
			mPortamentoStep[channel] *= -1.0;
		}
//		mPortamentoStep[channel] = std::fabs(mPortamentoStep[channel]);
//		if (slowdown)
//		{
//			mPortamentoStep[channel] *= -1.0;
//		}
#else
		// exponential acceleration
		oldReadStep = std::max(oldReadStep, 0.001);  // protect against a situation impossible to deal with
		double const targetReadStep = calculateTargetSpeed(oldReadStep, static_cast<double>(mMoveCount[channel]), static_cast<double>(targetDistance));
		mPortamentoStep[channel] = std::pow(std::fabs(targetReadStep) / oldReadStep, 1.0 / static_cast<double>(mMoveCount[channel]));
		mPortamentoStep[channel] = std::fabs(mPortamentoStep[channel]);
//auto const ktest = static_cast<long>(std::fabs((targetReadStep * static_cast<double>(mMoveCount[channel])) / std::log(std::fabs(targetReadStep / oldReadStep))));
//std::printf("oldReadStep = %.6f\nreadStep = %.6f\ntargetReadStep = %.6f\nportamentoStep = %.6f\nmovecount = %ld\ntargetDistance = %ld\nktest = %ld\n\n", oldReadStep, newReadStep, targetReadStep, mPortamentoStep[channel], mMoveCount[channel], targetDistance, ktest);
//std::printf("a = %.3f,\tb = %.3f,\tn = %ld,\tk = %ld\tportamentoStep = %.6f\n", oldReadStep, targetReadStep, mMoveCount[channel], targetDistance, mPortamentoStep[channel]);
//std::printf("\noldReadStep = %.6f\treadStep = %.6f\ttargetReadStep = %.6f\tportamentoStep = %.6f\n", oldReadStep, newReadStep, targetReadStep, mPortamentoStep[channel]);

#endif
		newReadStep = oldReadStep;
		if (moveBackwards)
		{
			newReadStep = -newReadStep;
		}
#if USE_LINEAR_ACCELERATION
		if (crossZero)
		{
			newReadStep = -newReadStep;
		}
#endif
	}
	else
	{
#if USE_LINEAR_ACCELERATION
		mPortamentoStep[channel] = 0.0;  // no linear acceleration
#else
		mPortamentoStep[channel] = 1.0;  // no exponential acceleration
#endif
	}

	// assign the new read step value into the correct channel's variable
	mReadStep[channel] = newReadStep;
	mNeedResync[channel] = false;  // untrue this so that we don't do the measure sync calculations again unnecessarily
}

// this is a necessary value for calculating the pitches related to the playback speeds
static double const kLn2ToTheOneTwelfthPower = std::log(std::pow(2., 1. / 12.));  // TODO C++26: constexpr
//constexpr double kLn2ToTheOneTwelfthPower = 0.05776226504666215;

//-----------------------------------------------------------------------------------------
double Scrubby::processPitchConstraint(double readStep) const
{
	bool const backwards = (readStep < 0.0);  // traveling backwards through the buffer?
	double const direction_f = backwards ? -1.0 : 1.0;  // direction scalar

	// check to see if none of the semitones are enabled,
	// in which case we act like they are all enabled.
	// no we don't, we become silent, I changed my mind
	auto const notesActive = std::ranges::any_of(mPitchSteps, std::identity{});

	// determine the lower bound of the number of semitone shifts that this playback speed results in
	// the equation below comes from 2^(semitone/12) = readStep, solving for semitone
	double const semitone_f = std::floor(std::log(std::fabs(readStep)) / kLn2ToTheOneTwelfthPower);
	auto semitone = static_cast<long>(semitone_f + 0.1);  // add a little bit to prevent any float truncation errors

	// . . . search for a possible active note to constrain to . . .
	auto octave = std::div(semitone, 12L);  // the octave and semitone transposition
	// remainder will be used as the index to the mPitchSteps array,
	// so it must be positive, and we compensate for adding 12 to it
	// by subtracting an octave, so it all evens out
	if (octave.rem < 0)
	{
		octave.rem += 12;
		octave.quot -= 1;
	}
	// just in case it fails both searches, which shouldn't happen
	semitone = octave.rem;
	// if no notes are active, then don't play back anything (this basically means silence)
	if (!notesActive)
	{
		return 0.0f;
	}
	// if we are choosing from a selection of semitone steps, then do the selection process
	else
	{
		// start searching for an active pitchStep with the current pitchStep (remainder)
		// and then all those below it
		auto i = octave.rem;
		do
		{
			// we found the pitchstep that we will use, store it and exit this loop
			if (mPitchSteps[i])
			{
				semitone = i;
				break;
			}
			// otherwise keep searching down
			i--;
			if (i < 0)
			{
				// didn't find an active one yet, so wrap around to the top of the pitchStep array
				i = kNumPitchSteps - 1;
				// and compensate for that by subtracting an octave
				octave.quot--;
			}
		} while (i != octave.rem);
	}
	// constrain to octaves range, if we're doing that
	if ((mOctaveMin > kOctave_MinValue) && (octave.quot < mOctaveMin))
	{
		octave.quot = mOctaveMin;
	}
	else if ((mOctaveMax < kOctave_MaxValue) && (octave.quot > mOctaveMax))
	{
		octave.quot = mOctaveMax;
	}
	// add the octave transposition back in to get the correct semitone transposition
	semitone += (octave.quot * 12);

	// return the transposition converted to a playback speed scalar, with direction restored
	return std::pow(2.0, static_cast<double>(semitone) / 12.0) * direction_f;
}


//-----------------------------------------------------------------------------------------
void Scrubby::processaudio(std::span<float const* const> inAudio, std::span<float* const> outAudio, size_t inNumFrames)
{
	auto const numChannels = outAudio.size();

	processMidiNotes();
	checkTempoSyncStuff();

	auto const notesActive = std::ranges::any_of(mPitchSteps, std::identity{});
	// if we're using pitch constraint and the previous block had no notes active,
	// then check to see if new notes have begun and, if so, begin new seeks
	// so that we start producing sound again immediately
	if ((mPitchConstraint && (mSpeedMode == kSpeedMode_Robot)) && !mNotesWereAlreadyActive && notesActive)
	{
		std::ranges::fill(mSeekCount, 0);
	}
	// now remember the current situation for informing the next processing block
	mNotesWereAlreadyActive = notesActive;

	for (size_t sampleIndex = 0; sampleIndex < inNumFrames; sampleIndex++)
	{
		// cache first channel input audio sample so that in-place processing can overwrite it for subsequent channels
		auto const inputValue_firstChannel = inAudio.front()[sampleIndex];

		// update the buffers with the latest samples
		if (!mFreeze)
		{
			for (size_t ch = 0; ch < numChannels; ch++)
			{
				mAudioBuffers[ch][mWritePos] = inAudio[std::min(ch, inAudio.size() - 1)][sampleIndex];
			}
#if 0  // melody test
			for (size_t ch = 0; ch < numChannels; ch++)
			{
				mAudioBuffers[ch][mWritePos] = 0.69f * std::sin(24.f * std::numbers::pi_v<float> * (static_cast<float>(sampleIndex) / static_cast<float>(inNumFrames)));
				mAudioBuffers[ch][mWritePos] = 0.69f * std::sin(2.f * std::numbers::pi_v<float> * (static_cast<float>(mSineCount) / 169.f));
			}
			// produce a sine wave of C4 when using 44.1 kHz sample rate
			if (++mSineCount > 168)
			{
				mSineCount = 0;
			}
#endif
		}

		// write the output to the output streams, interpolated for smoothness
		for (size_t ch = 0; ch < numChannels; ch++)
		{
			auto const inputValue = (ch < inAudio.size()) ? inAudio[ch][sampleIndex] : inputValue_firstChannel;
			auto outputValue = dfx::math::InterpolateHermite(std::span(mAudioBuffers[ch]).subspan(0, mMaxAudioBufferSize), mReadPos[ch]);
			outputValue = mHighpassFilters[ch].process(outputValue);
			outAudio[ch][sampleIndex] = (inputValue * mInputGain.getValue()) + (outputValue * mOutputGain.getValue());
		}

		// increment/decrement the position trackers and counters
		if (!mFreeze)
		{
			mWritePos = (mWritePos + 1) % mMaxAudioBufferSize;
		}
		for (size_t ch = 0; ch < numChannels; ch++)
		{
			mSeekCount[ch]--;
			mMoveCount[ch]--;
		}
		//
		// it's time to find a new target to seek
		if (mSeekCount[0] < 0)
		{
			generateNewTarget(0);

			// copy the left channel's new values if we're in unified channels mode
			if (!mSplitChannels)
			{
				auto const fillWithFirst = [](auto& container)
				{
					std::fill(std::next(container.begin()), container.end(), container.front());
				};
				fillWithFirst(mReadPos);
				fillWithFirst(mReadStep);
				fillWithFirst(mPortamentoStep);
				fillWithFirst(mSeekCount);
				fillWithFirst(mMoveCount);
				fillWithFirst(mNeedResync);
			}
		}
		// find a new target to seek for the right channel if we're in split channels mode
		if (mSplitChannels)
		{
			for (size_t ch = 1; ch < numChannels; ch++)
			{
				if (mSeekCount[ch] < 0)
				{
					generateNewTarget(ch);
				}
			}
		}
		//
		// only increment the read position trackers if we're still moving towards the target
		for (size_t ch = 0; ch < numChannels; ch++)
		{
			if (mMoveCount[ch] >= 0)
			{
				if (mSpeedMode == kSpeedMode_DJ)
				{
#if USE_LINEAR_ACCELERATION
					mReadStep[ch] += mPortamentoStep[ch];
#else
					mReadStep[ch] *= mPortamentoStep[ch];
#endif
				}
				mReadPos[ch] += mReadStep[ch];
				// wraparound the read position tracker if necessary
				if (mReadPos[ch] >= mMaxAudioBufferSize_f)
				{
					mReadPos[ch] = std::fmod(mReadPos[ch], mMaxAudioBufferSize_f);
				}
				while (mReadPos[ch] < 0.0)
				{
					mReadPos[ch] += mMaxAudioBufferSize_f;
				}
				assert(static_cast<long>(mReadPos[ch]) >= 0);
				assert(static_cast<long>(mReadPos[ch]) < mMaxAudioBufferSize);
			}
		}

		incrementSmoothedAudioValues();
	}
}

//-----------------------------------------------------------------------------------------
void Scrubby::processMidiNotes()
{
	std::array<bool, kNumPitchSteps> oldNotes {};
	for (size_t i = 0; i < oldNotes.size(); i++)
	{
		oldNotes[i] = getparameter_b(static_cast<dfx::ParameterID>(i) + kPitchStep0);
	}

	for (size_t i = 0; i < getmidistate().getBlockEventCount(); i++)
	{
		// wrap the note value around to our 1-octave range
		auto const currentNote = dfx::math::ToIndex(getmidistate().getBlockEvent(i).mByte1) % kNumPitchSteps;

		switch (getmidistate().getBlockEvent(i).mStatus)
		{
			case DfxMidi::kStatus_NoteOn:
				// correct any bogus values that would mess up the system
				mActiveNotesTable[currentNote] = std::max(mActiveNotesTable[currentNote], 0L);
				// if there are currently no keys playing this note,
				// then turn its associated pitch constraint parameter on
				if (mActiveNotesTable[currentNote] == 0)
				{
					setparameterquietly_b(static_cast<dfx::ParameterID>(currentNote) + kPitchStep0, true);
				}
				// increment the active notes table for this note
				mActiveNotesTable[currentNote]++;
				break;

			case DfxMidi::kStatus_NoteOff:
				// if this is the last key that was playing this note,
				// then turn its associated pitch constraint parameter off
				if (mActiveNotesTable[currentNote] == 1)
				{
					setparameterquietly_b(static_cast<dfx::ParameterID>(currentNote) + kPitchStep0, false);
				}
				// decrement the active notes table for this note, but don't go below 0
				mActiveNotesTable[currentNote] = std::max(mActiveNotesTable[currentNote] - 1, 0L);
				break;

			case DfxMidi::kStatus_CC:
				if (getmidistate().getBlockEvent(i).mByte1 == DfxMidi::kCC_AllNotesOff)
				{
					for (size_t noteIndex = 0; noteIndex < mActiveNotesTable.size(); noteIndex++)
					{
						// if this note is currently active, then turn its
						// associated pitch constraint parameter off
						// (and reset this note in the table)
						if (std::exchange(mActiveNotesTable[noteIndex], 0) > 0)
						{
							setparameterquietly_b(static_cast<dfx::ParameterID>(noteIndex) + kPitchStep0, false);
						}
					}
				}
				break;

			default:
				break;
		}
	}

	// go through and inform any listeners of any parameter changes that might have just happened
	for (size_t i = 0; i < oldNotes.size(); i++)
	{
		if (oldNotes[i] != getparameter_b(static_cast<dfx::ParameterID>(i) + kPitchStep0))
		{
			postupdate_parameter(static_cast<dfx::ParameterID>(i) + kPitchStep0);
		}
	}
}
