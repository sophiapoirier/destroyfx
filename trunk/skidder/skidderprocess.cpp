/*------------------------------------------------------------------------
Copyright (C) 2000-2020  Sophia Poirier

This file is part of Skidder.

Skidder is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Skidder is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Skidder.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "skidder.h"

#include <algorithm>
#include <cmath>

#include "dfxmath.h"


//-----------------------------------------------------------------------------------------
void Skidder::processSlopeIn()
{
	float baseSlopeAmp = static_cast<float>(mSlopeDur - mSlopeSamples) * mSlopeStep;
	baseSlopeAmp *= baseSlopeAmp;  // square-scale the gain scalar

	// dividing the growing mSlopeDur-mSlopeSamples by mSlopeDur makes ascending values
	if (mMidiIn)
	{
		if (mMidiMode == kMidiMode_Trigger)
		{
			// start from a 0.0 floor if we are coming in from silence
			mSampleAmp = baseSlopeAmp;
		}
		else if (mMidiMode == kMidiMode_Apply)
		{
			// no fade-in for the first entry of MIDI apply
			mSampleAmp = 1.0f;
		}
	}
	else if (mUseRandomFloor)
	{
		mSampleAmp = (baseSlopeAmp * mRandomGainRange) + mRandomFloor;
	}
	else
	{
		mSampleAmp = (baseSlopeAmp * mGainRange) + mFloor;
	}

	mSlopeSamples--;

	if (mSlopeSamples <= 0)
	{
		mState = SkidState::Plateau;
		mMidiIn = false;  // make sure it doesn't happen again
	}
}

//-----------------------------------------------------------------------------------------
void Skidder::processPlateau()
{
	mMidiIn = false;  // in case there was no slope-in

	// mSampleAmp in the plateau is 1.0, i.e. this sample is unaffected
	mSampleAmp = 1.0f;

	mPlateauSamples--;

	if (mPlateauSamples <= 0)
	{
#ifdef USE_BACKWARDS_RMS
		// average and then sqare the sample squareroots for the RMS value
		mRMS = std::pow(mRMS / static_cast<double>(mRMSCount), 2.0);
#else
		// average and then get the sqare root of the squared samples for the mRMS value
		mRMS = std::sqrt(mRMS / static_cast<double>(mRMSCount));
#endif
		// because RMS tends to be < 0.5, thus unfairly limiting rupture's range
		mRMS *= 2.0;
		// avoids clipping or illegit values (like from wraparound)
		if ((mRMS > 1.0) || (mRMS < 0.0))
		{
			mRMS = 1.0;
		}
		mRMSCount = 0;  // reset the RMS counter
		//
		// set up the random floor values
		mRandomFloor = static_cast<float>(expandparametervalue(kFloor, dfx::math::InterpolateRandom(mFloorRandMin_gen, mFloor_gen)));
		mRandomGainRange = 1.0f - mRandomFloor;  // the range of the skidding on/off gain
		//
		if (mSlopeDur > 0)
		{
			mState = SkidState::SlopeOut;
			mSlopeSamples = mSlopeDur;  // refill mSlopeSamples
			mSlopeStep = 1.0f / static_cast<float>(mSlopeDur);  // calculate the fade increment scalar
		}
		else
		{
			mState = SkidState::Valley;
		}
	}
}


//-----------------------------------------------------------------------------------------
void Skidder::processSlopeOut()
{
	float baseSlopeAmp = static_cast<float>(mSlopeSamples) * mSlopeStep;
	baseSlopeAmp *= baseSlopeAmp;  // square-scale the gain scalar

	// dividing the decrementing mSlopeSamples by mSlopeDur makes descending values
	if (mMidiOut && (mMidiMode == kMidiMode_Trigger))
	{
		// move towards a 0.0 floor if we are exiting to silence
		mSampleAmp = baseSlopeAmp;
	}
	else if (mUseRandomFloor)
	{
		mSampleAmp = (baseSlopeAmp * mRandomGainRange) + mRandomFloor;
	}
	else
	{
		mSampleAmp = (baseSlopeAmp * mGainRange) + mFloor;
	}

	mSlopeSamples--;

	if (mSlopeSamples <= 0)
	{
		mState = SkidState::Valley;
		mMidiOut = false;  // make sure it doesn't happen again
	}
}

//-----------------------------------------------------------------------------------------
void Skidder::processValley()
{
	if (mMidiIn)
	{
		if (mMidiMode == kMidiMode_Trigger)
		{
			// there's one sample of valley when trigger mode begins, so silence that one
			mSampleAmp = 0.0f;
		}
		else if (mMidiMode == kMidiMode_Apply)
		{
			// there's one sample of valley when apply mode begins, so keep it at full gain
			mSampleAmp = 1.0f;
		}
	}
	// otherwise mSampleAmp in the valley is whatever the floor gain is, the lowest gain value
	else if (mUseRandomFloor)
	{
		mSampleAmp = mRandomFloor;
	}
	else
	{
		mSampleAmp = mFloor;
	}

	mValleySamples--;

	if (mValleySamples <= 0)
	{
		float cycleRate = 0.0f;  // the base current skid rate value
		bool barSync = false;  // true if we need to sync up with the next bar start
		mRMS = 0.0;  // reset mRMS now because valley is over
		//
		// This is where we figure out how many samples long each 
		// envelope section is for the next skid cycle.
		//
		if (mTempoSync)  // the user wants to do tempo sync / beat division rate
		{
			// randomize the tempo rate if the random min scalar is lower than the upper bound
			if (mUseRandomRate)
			{
				auto const randomizedTempoRateIndex = static_cast<long>(dfx::math::InterpolateRandom(static_cast<float>(mRateRandMinIndex), static_cast<float>(mRateIndex) + 0.99f));
				cycleRate = mTempoRateTable.getScalar(randomizedTempoRateIndex);
				// we can't do the bar sync if the skids durations are random
				mNeedResync = false;
			}
			else
			{
				cycleRate = mRate_Sync;
			}
			// convert the tempo rate into rate in terms of Hz
			cycleRate *= mCurrentTempoBPS;
			// set this true so that we make sure to do the measure syncronisation later on
			if (mNeedResync && (mMidiMode == kMidiMode_None))
			{
				barSync = true;
			}
		}
		else
		{
			cycleRate = mUseRandomRate ? static_cast<float>(expandparametervalue(kRate_Hz, dfx::math::InterpolateRandom(mRateRandMinHz_gen, mRateHz_gen))) : mRate_Hz;
		}
		mNeedResync = false;  // reset this so that we don't have any trouble
		mCycleSamples = std::lround(getsamplerate_f() / cycleRate);
		//
		auto const pulsewidth = mUseRandomPulsewidth ? dfx::math::InterpolateRandom(mPulsewidthRandMin, mPulsewidth) : mPulsewidth;
		mPulseSamples = std::lround(static_cast<float>(mCycleSamples) * pulsewidth);
		mValleySamples = mCycleSamples - mPulseSamples;
		mSlopeSamples = std::lround(getsamplerate() * mSlopeSeconds);
		mSlopeDur = mSlopeSamples;
		mSlopeStep = 1.0f / static_cast<float>(mSlopeDur);  // calculate the fade increment scalar
		mPlateauSamples = mPulseSamples - (mSlopeSamples * 2);
		if (mPlateauSamples < 1)  // this shrinks the slope to 1/3 of the pulse if the user sets slope too high
		{
			mSlopeSamples = std::lround(static_cast<float>(mPulseSamples) / 3.0f);
			mSlopeDur = mSlopeSamples;
			mSlopeStep = 1.0f / static_cast<float>(mSlopeDur);  // calculate the fade increment scalar
			mPlateauSamples = mPulseSamples - (mSlopeSamples * 2);
		}

		// go to slopeIn next if slope is not 0.0, otherwise go to plateau
		mState = (mSlopeDur > 0) ? SkidState::SlopeIn : SkidState::Plateau;

		if (barSync)  // we need to adjust this cycle so that a skid syncs with the next bar
		{
			// calculate how long this skid cycle needs to be
			long const countdown = gettimeinfo().mSamplesToNextBar % mCycleSamples;
			// skip straight to the valley and adjust its length
			if (countdown <= (mValleySamples + (mSlopeSamples * 2)))
			{
				mValleySamples = countdown;
				mState = SkidState::Valley;
			}
			// otherwise adjust the plateau if the shortened skid is still long enough for one
			else
			{
				mPlateauSamples -= mCycleSamples - countdown;
			}
		}

		// if MIDI apply mode is just beginning, make things smooth with no panning
		if (mMidiIn && (mMidiMode == kMidiMode_Apply))
		{
			mPanGainL = mPanGainR = 1.0f;
		}
		else
		{
			// this calculates a random float value from -1.0 to 1.0
			float const panRander = (dfx::math::Rand<float>() * 2.0f) - 1.0f;
			// ((panRander * mPanWidth) + 1.0) ranges from 0.0 to 2.0
			mPanGainL = (panRander * mPanWidth) + 1.0f;
			mPanGainR = 2.0f - ((panRander * mPanWidth) + 1.0f);
		}
	}  // end of the "valley is over" if-statement
}

//-----------------------------------------------------------------------------------------
float Skidder::processOutput(float in1, float in2, float panGain)
{
	// output noise
	if ((mState == SkidState::Valley) && (mNoise.getValue() != 0.0f))
	{
		// out gets random noise with samples from -1.0 to 1.0 times the random pan times rupture times the RMS scalar
		return ((dfx::math::Rand<float>() * 2.0f) - 1.0f) * panGain * mNoise.getValue() * static_cast<float>(mRMS);
	}
	// do regular skidding output
	else
	{
		// only output a bit of the first input
		if (panGain <= 1.0f)
		{
			return in1 * panGain * mSampleAmp;
		}
		// output all of the first input and a bit of the second input
		else
		{
			return (in1 + (in2 * (panGain - 1.0f))) * mSampleAmp;
		}
	}
}

//-----------------------------------------------------------------------------------------
void Skidder::processaudio(float const* const* inAudio, float* const* outAudio, unsigned long inNumFrames)
{
	auto const numInputs = getnuminputs();
	auto const numOutputs = getnumoutputs();
	float const channelScalar = 1.0f / static_cast<float>(numOutputs);

	for (unsigned long ch = 0; ch < numOutputs; ch++)
	{
		mOutputAudio[ch] = outAudio[ch];
		if (numInputs == numOutputs)
		{
			mInputAudio[ch] = inAudio[ch];
		}
		else if (ch == 0)
		{
			// handle the special case of mismatched input/output channel counts that we allow 
			// by repeating the mono-input to multiple (faked) input channels
			// (copying to an intermediate input buffer in case processing in-place)
			std::copy_n(inAudio[ch], inNumFrames, mAsymmetricalInputAudioBuffer.data());
			std::fill(mInputAudio.begin(), mInputAudio.end(), mAsymmetricalInputAudioBuffer.data());
		}
	}


// ---------- begin MIDI stuff --------------
	processMidiNotes();

	auto const noteIsOn = isAnyNoteOn();

	switch (mMidiMode)
	{
		case kMidiMode_Trigger:
			// check mWaitSamples also because, if it's zero, we can just move ahead normally
			if (noteIsOn && (mWaitSamples != 0))
			{
				// need to make sure that the skipped part is silent when processing in-place
				for (unsigned long ch = 0; ch < numOutputs; ch++)
				{
					for (unsigned long samp = 0; samp < dfx::math::ToUnsigned(mWaitSamples); samp++)
					{
						mOutputAudio[ch][samp] = 0.0f;
					}

					// jump ahead accordingly in the i/o streams
					mInputAudio[ch] += mWaitSamples;
					mOutputAudio[ch] += mWaitSamples;
				}

				// cut back the number of samples outputted
				inNumFrames -= dfx::math::ToUnsigned(mWaitSamples);

				// reset
				mWaitSamples = 0;
			}

			else if (!noteIsOn)
			{
				// if Skidder currently is in the plateau and has a slow cycle, this could happen
				if (dfx::math::ToUnsigned(mWaitSamples) > inNumFrames)
				{
					mWaitSamples -= dfx::math::ToSigned(inNumFrames);
				}
				else
				{
					for (unsigned long ch = 0; ch < numOutputs; ch++)
					{
						for (unsigned long samp = dfx::math::ToUnsigned(mWaitSamples); samp < inNumFrames; samp++)
						{
							mOutputAudio[ch][samp] = 0.0f;
						}
					}
					if (mWaitSamples > 0)
					{
						inNumFrames = dfx::math::ToUnsigned(mWaitSamples);
						mWaitSamples = 0;
					}
					else
					{
						// that's all we need to do if there are no notes, just silence the output
						return;
					}
				}
			}

			// adjust the floor according to note velocity if velocity mode is on
			if (mUseVelocity)
			{
				mFloor = static_cast<float>(expandparametervalue(kFloor, static_cast<float>(DfxMidi::kMaxValue - mMostRecentVelocity) * DfxMidi::kValueScalar));
				mGainRange = 1.0f - mFloor;  // the range of the skidding on/off gain
				mUseRandomFloor = false;
			}

			break;

		case kMidiMode_Apply:
			// check mWaitSamples also because, if it's zero, we can just move ahead normally
			if (noteIsOn && (mWaitSamples != 0))
			{
				// need to make sure that the skipped part is unprocessed audio
				for (unsigned long ch = 0; ch < numOutputs; ch++)
				{
					std::copy_n(mInputAudio[ch], mWaitSamples, mOutputAudio[ch]);

					// jump ahead accordingly in the i/o streams
					mInputAudio[ch] += mWaitSamples;
					mOutputAudio[ch] += mWaitSamples;
				}

				// cut back the number of samples outputted
				inNumFrames -= dfx::math::ToUnsigned(mWaitSamples);

				// reset
				mWaitSamples = 0;
			}

			else if (!noteIsOn)
			{
				// if Skidder currently is in the plateau and has a slow cycle, this could happen
				if (dfx::math::ToUnsigned(mWaitSamples) > inNumFrames)
				{
					mWaitSamples -= dfx::math::ToSigned(inNumFrames);
				}
				else
				{
					for (unsigned long ch = 0; ch < numOutputs; ch++)
					{
						std::copy_n(mInputAudio[ch] + mWaitSamples, inNumFrames - dfx::math::ToUnsigned(mWaitSamples), mOutputAudio[ch] + mWaitSamples);
					}
					if (mWaitSamples > 0)
					{
						inNumFrames = dfx::math::ToUnsigned(mWaitSamples);
						mWaitSamples = 0;
					}
					else
					{
						// that's all we need to do if there are no notes, 
						// just copy the input to the output
						return;
					}
				}
			}

			// adjust the floor according to note velocity if velocity mode is on
			if (mUseVelocity)
			{
				mFloor = static_cast<float>(expandparametervalue(kFloor, static_cast<float>(DfxMidi::kMaxValue - mMostRecentVelocity) * DfxMidi::kValueScalar));
				mGainRange = 1.0f - mFloor;  // the range of the skidding on/off gain
				mUseRandomFloor = false;
			}

			break;

		default:
			break;
	}
// ---------- end MIDI stuff --------------


	// figure out the current tempo if we're doing tempo sync
	if (mTempoSync)
	{
		// calculate the tempo at the current processing buffer
		if (mUseHostTempo && hostCanDoTempo() && gettimeinfo().mTempoIsValid)  // get the tempo from the host
		{
			mCurrentTempoBPS = gettimeinfo().mTempo_BPS;
			// check if audio playback has just restarted and reset buffer stuff if it has (for measure sync)
			if (gettimeinfo().mPlaybackChanged)
			{
				mNeedResync = true;
				mState = SkidState::Valley;
				mValleySamples = 0;
			}
		}
		else  // get the tempo from the user parameter
		{
			mCurrentTempoBPS = mUserTempo / 60.0;
			mNeedResync = false;  // we don't want it true if we're not syncing to host tempo
		}
		mOldTempoBPS = mCurrentTempoBPS;
	}
	else
	{
		mNeedResync = false;  // we don't want it true if we're not syncing to host tempo
	}


	// stereo processing
	if (numOutputs == 2)
	{
		// this is the per-sample audio processing loop
		for (unsigned long samp = 0; samp < inNumFrames; samp++)
		{
			auto const inputValueL = mInputAudio[0][samp];
			auto const inputValueR = mInputAudio[1][samp];

			// get the average sqare root of the current input samples
			if ((mState == SkidState::SlopeIn) || (mState == SkidState::Plateau))
			{
#ifdef USE_BACKWARDS_RMS
				mRMS += std::sqrt((std::fabs(inputValueL) + std::fabs(inputValueR)) * channelScalar);
#else
				mRMS += ((inputValueL * inputValueL) + (inputValueR * inputValueR)) * channelScalar;
#endif
				mRMSCount++;  // this counter is later used for getting the mean
			}

			switch (mState)
			{
				case SkidState::SlopeIn:
					processSlopeIn();
					break;
				case SkidState::Plateau:
					processPlateau();
					break;
				case SkidState::SlopeOut:
					processSlopeOut();
					break;
				case SkidState::Valley:
				default:
					processValley();
					break;
			}
	
			mOutputAudio[0][samp] = processOutput(inputValueL, inputValueR, mPanGainL);
			mOutputAudio[1][samp] = processOutput(inputValueR, inputValueL, mPanGainR);

			incrementSmoothedAudioValues();
		}
	}

	// independent-channel processing
	else
	{
		// this is the per-sample audio processing loop
		for (unsigned long samp = 0; samp < inNumFrames; samp++)
		{
			// get the average sqare root of the current input samples
			if ((mState == SkidState::SlopeIn) || (mState == SkidState::Plateau))
			{
#ifdef USE_BACKWARDS_RMS
				float tempSum = 0.0f;
				for (unsigned long ch = 0; ch < numOutputs; ch++)
				{
					tempSum += std::fabs(mInputAudio[ch][samp]);
				}
				mRMS += std::sqrt(tempSum * channelScalar);
#else
				for (unsigned long ch = 0; ch < numOutputs; ch++)
				{
					mRMS += mInputAudio[ch][samp] * mInputAudio[ch][samp] * channelScalar;
				}
#endif
				mRMSCount++;  // this counter is later used for getting the mean
			}

			switch (mState)
			{
				case SkidState::SlopeIn:
					processSlopeIn();
					break;
				case SkidState::Plateau:
					processPlateau();
					break;
				case SkidState::SlopeOut:
					processSlopeOut();
					break;
				case SkidState::Valley:
				default:
					processValley();
					break;
			}
	
			for (unsigned long ch = 0; ch < numOutputs; ch++)
			{
				mOutputAudio[ch][samp] = processOutput(mInputAudio[ch][samp], mInputAudio[ch][samp], 1.0f);
			}

			incrementSmoothedAudioValues();
		}
	}
}
