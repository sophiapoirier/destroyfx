/*------------------------------------------------------------------------
Copyright (C) 2001-2022  Sophia Poirier

This file is part of Buffer Override.

Buffer Override is free software:  you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Buffer Override is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Buffer Override.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#include "bufferoverride-base.h"
#include "bufferoverride.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "dfxmath.h"



//-----------------------------------------------------------------------------
template <typename T>
static void updateViewCacheValue(std::atomic<T>& ioAtomicValue, T const inReplacementValue, bool& ioChanged)
{
	ioChanged |= ioAtomicValue.exchange(inReplacementValue, std::memory_order_relaxed) != inReplacementValue;
};

//-----------------------------------------------------------------------------
long BufferOverride::ms2samples(double inSizeMS) const
{
	return std::lround(inSizeMS * getsamplerate() * 0.001);
}

//-----------------------------------------------------------------------------
long BufferOverride::beat2samples(double inBeatScalar, double inTempoBPS) const
{
	return std::lround(getsamplerate() / (inTempoBPS * inBeatScalar));
}

//-----------------------------------------------------------------------------
void BufferOverride::updateBuffer(size_t samplePos, bool& ioViewDataChanged)
{
	bool doSmoothing = true;  // but in some situations, we shouldn't
	bool barSync = false;  // true if we need to sync up with the next bar start


	heedMidiEvents(samplePos);

	mReadPos = 0;  // reset for starting a new minibuffer
	mPrevMinibufferSize = mMinibufferSize;
	auto const prevForcedBufferSize = mCurrentForcedBufferSize;

	//--------------------------PROCESS THE LFOs----------------------------
	// update the LFOs' positions to the current position
	mDivisorLFO.updatePosition(mPrevMinibufferSize);
	mBufferLFO.updatePosition(mPrevMinibufferSize);
	// Then get the current output values of the LFOs, which also updates their positions once more.
	// Scale the 0 - 1 LFO output values to 0 - 2 (oscillating around 1).
	auto const divisorLFOValue = static_cast<float>(mDivisorLFO.processZeroToTwo());
	double const bufferLFOValue = 2. - mBufferLFO.processZeroToTwo();  // inverting it makes more pitch sense
	updateViewCacheValue(mDivisorLFOValue_viewCache, divisorLFOValue, ioViewDataChanged);
	updateViewCacheValue(mBufferLFOValue_viewCache, static_cast<float>(bufferLFOValue), ioViewDataChanged);
	// and then update the step size for each LFO, in case the LFO parameters have changed
	if (mDivisorLFOTempoSync)
	{
		mDivisorLFO.setStepSize(mCurrentTempoBPS * mDivisorLFOTempoRate * mOneDivSR);
	}
	else
	{
		mDivisorLFO.setStepSize(mDivisorLFORateHz * mOneDivSR);
	}
	if (mBufferLFOTempoSync)
	{
		mBufferLFO.setStepSize(mCurrentTempoBPS * mBufferLFOTempoRate * mOneDivSR);
	}
	else
	{
		mBufferLFO.setStepSize(mBufferLFORateHz * mOneDivSR);
	}

	//---------------------------CALCULATE FORCED BUFFER SIZE----------------------------
	// check if it's the end of this forced buffer
	if (mWritePos >= mCurrentForcedBufferSize)
	{
		mWritePos = 0;  // start up a new forced buffer

		// check on the previous forced and minibuffers; don't smooth if the last forced buffer wasn't divided
		doSmoothing = (mPrevMinibufferSize < mCurrentForcedBufferSize);

		// now update the the size of the current force buffer
		if (mBufferTempoSync &&  // the user wants to do tempo sync / beat division rate
			(mCurrentTempoBPS > 0.0))  // avoid division by zero
		{
			mCurrentForcedBufferSize = beat2samples(mBufferSizeSync, mCurrentTempoBPS);
			// set this true so that we make sure to do the measure syncronisation later on
			if (mNeedResync)
			{
				barSync = true;
			}
		}
		else
		{
			mCurrentForcedBufferSize = ms2samples(mBufferSizeMS);
		}
		// apply the buffer LFO to the forced buffer size
		mCurrentForcedBufferSize = std::lround(static_cast<double>(mCurrentForcedBufferSize) * bufferLFOValue);
		// really low tempos and tempo rate values can cause huge forced buffer sizes,
		// so prevent going outside of the allocated buffer space
		mCurrentForcedBufferSize = std::clamp(mCurrentForcedBufferSize, 2L, static_cast<long>(mBuffers.front().size()));

		// untrue this so that we don't do the measure sync calculations again unnecessarily
		mNeedResync = false;
	}

	//-----------------------CALCULATE THE DIVISOR-------------------------
	auto currentBufferDivisor = mDivisor;
	// apply the divisor LFO to the divisor value if there's an "active" divisor
	if (currentBufferDivisor >= kActiveDivisorMinimum)
	{
		// now it's possible that the LFO could make the divisor less than the active minimum,
		// which will essentially turn the effect off, so we limit the modulation there
		currentBufferDivisor = std::max(currentBufferDivisor * divisorLFOValue, kActiveDivisorMinimum);
	}

	//-----------------------CALCULATE THE MINIBUFFER SIZE-------------------------
	// this is not a new forced buffer starting up
	if (mWritePos > 0)
	{
		// if it's allowed, update the minibuffer size midway through this forced buffer
		if (mBufferInterrupt)
		{
			mMinibufferSize = std::lround(static_cast<float>(mCurrentForcedBufferSize) / currentBufferDivisor);
		}
		// if it's the last minibuffer, then fill up the forced buffer to the end
		// by extending this last minibuffer to fill up the end of the forced buffer
		long const remainingForcedBuffer = mCurrentForcedBufferSize - mWritePos;
		if ((mMinibufferSize * 2) >= remainingForcedBuffer)
		{
			mMinibufferSize = remainingForcedBuffer;
		}
	}
	// this is a new forced buffer just beginning, act accordingly, do bar sync if necessary
	else
	{
		auto const samplesPerBeat = TimeInfo::samplesPerBeat(mCurrentTempoBPS, getsamplerate());
		auto const timeSigNumerator = gettimeinfo().timeSignatureNumerator().value_or(4.);
		auto const samplesPerBar = samplesPerBeat * timeSigNumerator;
		double samplesToNextBar = gettimeinfo().mSamplesToNextBar ? (*gettimeinfo().mSamplesToNextBar - static_cast<double>(samplePos)) : 0.;
		while ((samplesToNextBar < 0.) && (samplesPerBar > 0.))
		{
			samplesToNextBar += samplesPerBar;
		}
		samplesToNextBar = std::max(samplesToNextBar, 0.);
		if (barSync)
		{
			// do beat sync for each LFO if it ought to be done
			if (mDivisorLFOTempoSync)
			{
				mDivisorLFO.syncToTheBeat(samplesToNextBar);
			}
			if (mBufferLFOTempoSync)
			{
				mBufferLFO.syncToTheBeat(samplesToNextBar);
			}
		}
		// because there isn't really any division (given my implementation)
		if (currentBufferDivisor < kActiveDivisorMinimum)
		{
			long const samplesToAlignForcedBufferToBar = std::lround(samplesToNextBar) % mCurrentForcedBufferSize;
			if (barSync && (samplesToAlignForcedBufferToBar > 0))
			{
				mCurrentForcedBufferSize = samplesToAlignForcedBufferToBar;
			}
			mMinibufferSize = mCurrentForcedBufferSize;
		}
		else
		{
			mMinibufferSize = std::lround(static_cast<float>(mCurrentForcedBufferSize) / currentBufferDivisor);
			if (barSync)
			{
				// calculate how long this forced buffer needs to be
				long const countdown = std::lround(samplesToNextBar) % mCurrentForcedBufferSize;
				// update the forced buffer size and number of minibuffers so that
				// the forced buffers sync up with the musical measures of the song
				if (countdown < (mMinibufferSize * 2))  // extend the buffer if it would be too short...
				{
					mCurrentForcedBufferSize += countdown;
				}
				else  // ...otherwise chop it down to the length of the extra bit needed to sync with the next measure
				{
					mCurrentForcedBufferSize = countdown;
				}
			}
		}
	}
	// avoid zero-sized minibuffers
	mMinibufferSize = std::max(mMinibufferSize, 1L);

	//-----------------------CALCULATE SMOOTHING DURATION-------------------------
	// no smoothing if the previous forced buffer wasn't divided
	if (!doSmoothing)
	{
		mSmoothCount = mSmoothDur = 0;
	}
	else
	{
		mSmoothDur = std::lround(mSmoothPortion * static_cast<float>(mMinibufferSize));
		long maxSmoothDur = 0;
		// if we're just starting a new forced buffer,
		// then the samples beyond the end of the previous one are not valid
		if (mWritePos <= 0)
		{
			maxSmoothDur = prevForcedBufferSize - mPrevMinibufferSize;
		}
		// otherwise just make sure that we don't go outside of the allocated arrays
		else
		{
			maxSmoothDur = std::lround(mBuffers.front().size()) - mPrevMinibufferSize;
		}
		mSmoothDur = std::min(mSmoothDur, maxSmoothDur);
		mSmoothCount = mSmoothDur;
//		mSmoothStep = 1.0f / static_cast<float>(mSmoothDur + 1);  // the gain increment for each smoothing step

//		mSqrtFadeIn = std::sqrt(mSmoothStep);
//		mSqrtFadeOut = std::sqrt(1.0f - mSmoothStep);
//		mSmoothFract = mSmoothStep;

		mFadeOutGain = std::cos(std::numbers::pi_v<float> / static_cast<float>(4 * mSmoothDur));
		mFadeInGain = std::sin(std::numbers::pi_v<float> / static_cast<float>(4 * mSmoothDur));
		mRealFadePart = (mFadeOutGain * mFadeOutGain) - (mFadeInGain * mFadeInGain);  // std::cos(std::numbers::pi_v<float> / 2.f / n)
		mImaginaryFadePart = 2.f * mFadeOutGain * mFadeInGain;  // std::sin(std::numbers::pi_v<float> / 2.f / n)
	}
}



//---------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------
void BufferOverride::processaudio(float const* const* inAudio, float* const* outAudio, size_t inNumFrames)
{
	auto const numChannels = getnumoutputs();
	auto const entryDivisor = mDivisor;

	bool viewDataChanged = false;


//-----------------------TEMPO STUFF---------------------------
	// figure out the current tempo if we're doing tempo sync
	if (mBufferTempoSync || mDivisorLFOTempoSync || mBufferLFOTempoSync)
	{
		// calculate the tempo at the current processing buffer
		if (mUseHostTempo && hostCanDoTempo() && gettimeinfo().mTempoBPS)  // get the tempo from the host
		{
			mCurrentTempoBPS = *gettimeinfo().mTempoBPS;
			updateViewCacheValue(mHostTempoBPS_viewCache, mCurrentTempoBPS, viewDataChanged);
			// check if audio playback has just restarted and reset buffer stuff if it has (for measure sync)
			if (gettimeinfo().mPlaybackChanged)
			{
				mNeedResync = true;
				mCurrentForcedBufferSize = 1;
				mWritePos = 1;
				mMinibufferSize = 1;
				mPrevMinibufferSize = 0;
				mSmoothCount = mSmoothDur = 0;
			}
		}
		else  // get the tempo from the user parameter
		{
			mCurrentTempoBPS = mUserTempoBPM / 60.0;
			mNeedResync = false;  // we don't want it true if we're not syncing to host tempo
		}
	}


//-----------------------AUDIO STUFF---------------------------
	// here we begin the audio output loop, which has two checkpoints at the beginning
	for (size_t sampleIndex = 0; sampleIndex < inNumFrames; sampleIndex++)
	{
		// check if it's the end of this minibuffer
		if (mReadPos >= mMinibufferSize)
		{
			updateBuffer(sampleIndex, viewDataChanged);
		}

		// store the latest input samples into the buffers
		for (size_t ch = 0; ch < numChannels; ch++)
		{
			mBuffers[ch][mWritePos] = inAudio[ch][sampleIndex];
		}

		// get the current output without any smoothing
		for (size_t ch = 0; ch < numChannels; ch++)
		{
			mAudioOutputValues[ch] = mBuffers[ch][mReadPos];
		}

		// and if smoothing is taking place, get the smoothed audio output
		if (mSmoothCount > 0)
		{
			for (size_t ch = 0; ch < numChannels; ch++)
			{
				// crossfade between the current input and its corresponding overlap sample
//				mAudioOutputValues[ch] *= 1.0f - (mSmoothStep * static_cast<float>(mSmoothCount));  // current
//				mAudioOutputValues[ch] += mBuffers[ch][mReadPos + mPrevMinibufferSize] * mSmoothStep * static_cast<float>(mSmoothCount);  // + previous
//				float const mSmoothFract = mSmoothStep * static_cast<float>(mSmoothCount);
//				float const newgain = std::sqrt(1.0f - mSmoothFract);
//				float const oldgain = std::sqrt(mSmoothFract);
//				mAudioOutputValues[ch] = (mAudioOutputValues[ch] * newgain) + (mBuffers[ch][mReadPos + mPrevMinibufferSize] * oldgain);
//				mAudioOutputValues[ch] = (mAudioOutputValues[ch] * mSqrtFadeIn) + (mBuffers[ch][mReadPos + mPrevMinibufferSize] * mSqrtFadeOut);
				mAudioOutputValues[ch] = (mAudioOutputValues[ch] * mFadeInGain) + (mBuffers[ch][mReadPos + mPrevMinibufferSize] * mFadeOutGain);
			}
			mSmoothCount--;
//			mSmoothFract += mSmoothStep;
//			mSqrtFadeIn = 0.5f * (mSqrtFadeIn + (mSmoothFract / mSqrtFadeIn));
//			mSqrtFadeOut = 0.5f * (mSqrtFadeOut + ((1.0f - mSmoothFract) / mSqrtFadeOut));
			mFadeInGain = (mFadeOutGain * mImaginaryFadePart) + (mFadeInGain * mRealFadePart);
			mFadeOutGain = (mRealFadePart * mFadeOutGain) - (mImaginaryFadePart * mFadeInGain);
		}

		// write the output samples into the output stream
		for (size_t ch = 0; ch < numChannels; ch++)
		{
			outAudio[ch][sampleIndex] = (mAudioOutputValues[ch] * mOutputGain.getValue()) + (inAudio[ch][sampleIndex] * mInputGain.getValue());
		}

		// increment the position trackers
		mReadPos++;
		mWritePos++;

		incrementSmoothedAudioValues();
	}


//-----------------------MIDI STUFF---------------------------
	// check to see if there may be a note or pitchbend message left over that hasn't been implemented
	auto& midiState = getmidistate();
	if (midiState.getBlockEventCount() > 0)
	{
		for (size_t eventIndex = 0; eventIndex < midiState.getBlockEventCount(); eventIndex++)
		{
			if (DfxMidi::isNote(midiState.getBlockEvent(eventIndex).mStatus))
			{
				// regardless of whether it's a note-on or note-off, we've found some note message
				mOldNote = true;
				// store the note and update the notes table if it's a note-on message
				if (midiState.getBlockEvent(eventIndex).mStatus == DfxMidi::kStatus_NoteOn)
				{
					midiState.insertNote(midiState.getBlockEvent(eventIndex).mByte1);
					mLastNoteOn = midiState.getBlockEvent(eventIndex).mByte1;
					// since we're not doing the divisor updating yet, this needs to be falsed
					mDivisorWasChangedByHand = false;
				}
				// otherwise remove the note from the notes table
				else
				{
					midiState.removeNote(midiState.getBlockEvent(eventIndex).mByte1);
				}
			}
			else if (midiState.getBlockEvent(eventIndex).mStatus == DfxMidi::kStatus_CC)
			{
				if (midiState.getBlockEvent(eventIndex).mByte1 == DfxMidi::kCC_AllNotesOff)
				{
					mOldNote = true;
					midiState.removeAllNotes();
				}
			}
		}
		for (size_t eventIndex = midiState.getBlockEventCount() - 1; true; eventIndex--)
		{
			if (midiState.getBlockEvent(eventIndex).mStatus == DfxMidi::kStatus_PitchBend)
			{
				mLastPitchbendLSB = midiState.getBlockEvent(eventIndex).mByte1;
				mLastPitchbendMSB = midiState.getBlockEvent(eventIndex).mByte2;
				break;  // leave this for-loop
			}
			if (eventIndex == 0)
			{
				break;
			}
		}
	}

	// make our parameters storers and the host aware that divisor changed because of MIDI
	if ((mDivisor != entryDivisor)/* || mDivisorWasChangedByMIDI*/)
	{
		setparameterquietly_f(kDivisor, mDivisor);
		postupdate_parameter(kDivisor);  // inform listeners of change
	}
	mDivisorWasChangedByMIDI = false;

	if (viewDataChanged)
	{
		updateViewDataCache();
	}
}
