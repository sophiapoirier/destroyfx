/*------------------------------------------------------------------------
Copyright (C) 2001-2023  Sophia Poirier

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
#include <cassert>
#include <cmath>
#include <numbers>

#include "dfxmath.h"



//-----------------------------------------------------------------------------
constexpr float getStepAmount(long const inLength)
{
	assert(inLength >= 0);
	return 1.f / static_cast<float>(inLength + 1);
}

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
	auto const prevMinibufferAudibleLength = mMinibufferAudibleLength;
	bool const forcedBufferIsBeginning = (mWritePos >= mCurrentForcedBufferSize);  // check if this forced buffer ended
	bool const prevMinibufferShortened = (prevMinibufferAudibleLength < mPrevMinibufferSize);
	bool const randomizeMinibufferPortion = (mMinibufferPortionRandomMin < mMinibufferPortion);

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
	if (forcedBufferIsBeginning)
	{
		mWritePos = 0;  // start up a new forced buffer

		// check on the previous forced and minibuffers; don't smooth if the last forced buffer wasn't divided or shortened
		doSmoothing = (prevMinibufferAudibleLength < prevForcedBufferSize);

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

		ioViewDataChanged |= (mDecayShape == kDecayShape_Random) || randomizeMinibufferPortion;
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
	if (!forcedBufferIsBeginning)
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
	if (forcedBufferIsBeginning || mBufferInterrupt)
	{
		mEffectiveMinibufferPortion = randomizeMinibufferPortion ? mRandomEngine.next(mMinibufferPortionRandomMin, mMinibufferPortion) : mMinibufferPortion;
	}
	mMinibufferAudibleLength = std::max(std::lround(static_cast<double>(mMinibufferSize) * mEffectiveMinibufferPortion), 1L);

	//-----------------------CALCULATE BUFFER DECAY-------------------------
	mPrevMinibufferDecayGain = mMinibufferDecayGain;
	std::swap(mCurrentDecayFilters, mPrevDecayFilters);
	std::ranges::for_each(mCurrentDecayFilters, [](auto& filter){ filter.reset(); });
	auto& firstFilter = mCurrentDecayFilters.front();

	auto const positionNormalized = static_cast<float>(mWritePos) / static_cast<float>(mCurrentForcedBufferSize);
	auto decay = GetBufferDecay(positionNormalized, mDecayDepth, mDecayShape, mRandomEngine);
	if (mDecayMode == kDecayMode_Gain)
	{
		mMinibufferDecayGain = decay * decay;
		firstFilter.setCoefficients(dfx::IIRFilter::kUnityCoeff);
	}
	else
	{
		mMinibufferDecayGain = 1.f;
		constexpr float decayMax = 1.f;
		auto const decayMin = decayMax - std::fabs(mDecayDepth);
		auto const decayReachedMidpoint = decay <= std::midpoint(decayMin, decayMax);
		// special decay value mapping fix-up for this mode (bifurcated value range)
		if ((mDecayMode == kDecayMode_HP_To_LP) && (mDecayShape != kDecayShape_Random))
		{
			if (decayReachedMidpoint)  // LP portion
			{
				decay = ((decay - decayMin) * 2.f) + decayMin;
			}
			else  // HP portion
			{
				decay = ((decayMax - decay) * 2.f) + decayMin;
			}
		}
		mDecayFilterIsLowpass = [this, positionNormalized, decayReachedMidpoint]
		{
			switch (mDecayMode)
			{
				case kDecayMode_Lowpass:
					return true;
				case kDecayMode_Highpass:
					return false;
				case kDecayMode_HP_To_LP:
					if (mDecayShape == kDecayShape_Random)
					{
						bool const positionReachedMidpoint = positionNormalized >= 0.5f;
						auto const invertedDepth = std::signbit(mDecayDepth);
						return invertedDepth ? !positionReachedMidpoint : positionReachedMidpoint;
					}
					return decayReachedMidpoint;
				case kDecayMode_LP_HP_Alternating:
					return !mDecayFilterIsLowpass;
				default:
					assert(false);  // TODO C++23: std::unreachable
					return false;
			}
		}();
		if (decay >= decayMax)
		{
			firstFilter.setCoefficients(dfx::IIRFilter::kUnityCoeff);
		}
		else if (mDecayFilterIsLowpass)
		{
			firstFilter.setLowpassGateCoefficients(decay);
		}
		else
		{
			firstFilter.setHighpassGateCoefficients(decay);
		}
	}
	auto const filterCoefficients = firstFilter.getCoefficients();
	std::for_each(std::next(mCurrentDecayFilters.begin()), mCurrentDecayFilters.end(), [filterCoefficients](auto& filter)
	{
		filter.setCoefficients(filterCoefficients);
	});

	//-----------------------CALCULATE SMOOTHING DURATION-------------------------
	auto const accelerateFadeToMinibufferPortion = [remainderLength = mMinibufferAudibleLength](long& length, long& countDown)
	{
		assert(length > 0);
		assert(countDown > 0);
		auto const countDownProgress = static_cast<float>(countDown) / static_cast<float>(length);
		countDown = remainderLength;
		length = std::lround(static_cast<float>(countDown) / countDownProgress);
	};

	mFadeInSmoothLength = std::lround(mSmoothPortion * static_cast<float>(mMinibufferSize));
	// if the previous forced buffer wasn't divided but was shortened, fade-in from the position of the fade-out
	if ((mPrevMinibufferSize == prevForcedBufferSize) && prevMinibufferShortened)
	{
		float const countDownProgress = (mFadeOutSmoothLength > 0) ? (static_cast<float>(mFadeOutSmoothCountDown) / static_cast<float>(mFadeOutSmoothLength)) : 0.f;
		mFadeInSmoothCountDown = std::lround((1.f - countDownProgress) * static_cast<float>(mFadeInSmoothLength));
		if (mFadeInSmoothCountDown > mMinibufferAudibleLength)
		{
			accelerateFadeToMinibufferPortion(mFadeInSmoothLength, mFadeInSmoothCountDown);
		}
	}
	else if (doSmoothing)
	{
		long maxSmoothLength {};
		// if we're just starting a new forced buffer,
		// then the samples beyond the end of the previous one are not valid
		if (forcedBufferIsBeginning)
		{
			maxSmoothLength = prevForcedBufferSize - mPrevMinibufferSize;
		}
		// otherwise just make sure that we don't go outside of the allocated arrays
		else
		{
			maxSmoothLength = std::ssize(mBuffers.front()) - mPrevMinibufferSize;
		}
		mFadeInSmoothLength = std::min({mFadeInSmoothLength, maxSmoothLength, mMinibufferAudibleLength});
		mFadeInSmoothCountDown = mFadeInSmoothLength;
	}
	// no smoothing if the previous forced buffer wasn't divided or shortened
	else
	{
		mFadeInSmoothLength = mFadeInSmoothCountDown = 0;
	}
	mFadeInSmoothStep = getStepAmount(mFadeInSmoothLength);

	// an early fade-out from reduced portion minibuffer is not in progress or completed
	if (!prevMinibufferShortened)
	{
		mFadeOutSmoothLength = mFadeInSmoothLength;
		mFadeOutSmoothCountDown = mFadeInSmoothCountDown;
		mFadeOutSmoothStep = mFadeInSmoothStep;
	}
	else if (mFadeOutSmoothCountDown > mMinibufferAudibleLength)
	{
		accelerateFadeToMinibufferPortion(mFadeOutSmoothLength, mFadeOutSmoothCountDown);
		mFadeOutSmoothStep = getStepAmount(mFadeOutSmoothLength);
	}
}



//---------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------
void BufferOverride::processaudio(float const* const* inAudio, float* const* outAudio, size_t inNumFrames)
{
	auto const numChannels = getnumoutputs();
	auto const entryDivisor = mDivisor;
	constexpr float halfPi = std::numbers::pi_v<float> / 2.f;

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
				mFadeInSmoothLength = mFadeInSmoothCountDown = 0;
				mFadeOutSmoothLength = mFadeOutSmoothCountDown = 0;
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

		if (mReadPos == mMinibufferAudibleLength)
		{
			mFadeOutSmoothLength = std::lround(mSmoothPortion * static_cast<float>(mMinibufferSize));
			long const maxSmoothLength = std::ssize(mBuffers.front()) - mMinibufferAudibleLength;
			mFadeOutSmoothLength = std::min({mFadeOutSmoothLength, maxSmoothLength, mMinibufferAudibleLength});
			mFadeOutSmoothCountDown = mFadeOutSmoothLength;
			mFadeOutSmoothStep = getStepAmount(mFadeOutSmoothLength);
		}

		// get the current output without any smoothing
		bool const minibufferIsAudiblePortion = (mReadPos < mMinibufferAudibleLength);
		if (minibufferIsAudiblePortion || (mFadeOutSmoothCountDown > 0))
		{
			for (size_t ch = 0; ch < numChannels; ch++)
			{
				mAudioOutputValues[ch] = mCurrentDecayFilters[ch].process(mBuffers[ch][mReadPos]) * mMinibufferDecayGain;
			}
		}
		else
		{
			std::ranges::fill(mAudioOutputValues, 0.f);
		}

		constexpr auto normalizePosition = [](long length, long countDown, float stepAmount)
		{
			return static_cast<float>((length - countDown) + 1) * stepAmount;
		};
		// and if smoothing is taking place, get the smoothed audio output
		if (mFadeInSmoothCountDown > 0)
		{
			auto const normalizedPosition = normalizePosition(mFadeInSmoothLength, mFadeInSmoothCountDown, mFadeInSmoothStep);
#if 1  // sine/cosine crossfade
			auto const fadeInGain = std::sin(halfPi * normalizedPosition);
#else  // square root crossfade
			auto const fadeInGain = std::sqrt(normalizedPosition);
#endif
			// crossfade in the current input
			std::ranges::transform(mAudioOutputValues, mAudioOutputValues.begin(),
								   [fadeInGain](auto value){ return value * fadeInGain; });
			mFadeInSmoothCountDown--;
		}
		if (mFadeOutSmoothCountDown > 0)
		{
			auto const normalizedPosition = normalizePosition(mFadeOutSmoothLength, mFadeOutSmoothCountDown, mFadeOutSmoothStep);
#if 1  // sine/cosine crossfade
			auto const fadeOutGain = std::cos(halfPi * normalizedPosition);
#else  // square root crossfade
			auto const fadeOutGain = std::sqrt(1.f - normalizedPosition);
#endif
			if (minibufferIsAudiblePortion)
			{
				// crossfade out the overlap sample
				for (size_t ch = 0; ch < numChannels; ch++)
				{
					auto const tailOutputValue = mPrevDecayFilters[ch].process(mBuffers[ch][mReadPos + mPrevMinibufferSize]) * mPrevMinibufferDecayGain;
					mAudioOutputValues[ch] += tailOutputValue * fadeOutGain;
				}
			}
			else
			{
				// fade-out the of end of the shortened minibuffer
				std::ranges::transform(mAudioOutputValues, mAudioOutputValues.begin(),
									   [fadeOutGain](auto value){ return value * fadeOutGain; });
			}
			mFadeOutSmoothCountDown--;
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
