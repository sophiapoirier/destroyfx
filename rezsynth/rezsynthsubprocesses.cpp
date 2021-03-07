/*------------------------------------------------------------------------
Copyright (C) 2001-2021  Sophia Poirier

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

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "rezsynth.h"

#include <cmath>


//-----------------------------------------------------------------------------------------
// this function tries to even out the wildly erratic resonant amplitudes
double RezSynth::calculateAmpEvener(int currentNote) const
{
	if (!mWiseAmp)
	{
		return 1.0;
	}

	double const baseFreq = getmidistate().getNoteFrequency(currentNote) * getmidistate().getPitchBend();
	auto const noteBandwidth = getBandwidthForFreq(baseFreq);
	double ampEvener = 1.0;

	if (mScaleMode <= kScaleMode_None)
	{
//		ampEvener = 0.0000000000009 * std::pow((static_cast<double>(fBandwidth) + 0.72), 1.8) * baseFreq * baseFreq / std::sqrt(static_cast<double>(mNumBands) * 0.0000003);
		ampEvener = 0.0000000000009 * std::pow(contractparametervalue(kBandwidthAmount_Hz, noteBandwidth) + 0.72, 1.8) * baseFreq * baseFreq / std::sqrt(static_cast<double>(mNumBands) * 0.0000003);
	}
	else if (mScaleMode == kScaleMode_RMS)
	{
		ampEvener = 0.0003 * baseFreq / std::sqrt(static_cast<double>(mNumBands) * 0.39);
	}
	else if (mScaleMode >= kScaleMode_Peak)
	{
		ampEvener = 0.0072 * baseFreq / ((noteBandwidth - getparametermin_f(kBandwidthAmount_Hz) + 10.0) / 3.0) / (static_cast<double>(mNumBands) * 0.03);
	}

	if (mResonAlgorithm != kResonAlg_2PoleNoZero)
	{
		ampEvener *= 2.0;
		if (mScaleMode <= kScaleMode_None)
		{
			ampEvener *= 6.0;
		}
	}

	return ampEvener;
}

//-----------------------------------------------------------------------------------------
// This function calculates the 3 coefficients in the resonant filter equation,
// 1 for the current sample input and 2 for the last 2 samples.
int RezSynth::calculateCoefficients(int currentNote)
{
	mBaseFreq[currentNote] = getmidistate().getNoteFrequency(currentNote) * getmidistate().getPitchBend();
	auto const baseFreq = mBaseFreq[currentNote].getValue();

	for (int bandcount = 0; bandcount < mNumBands; bandcount++)
	{
// GET THE CURRENT BAND'S CENTER FREQUENCY
		mBandCenterFreq[currentNote][bandcount] = [this, baseFreq, bandcount]()
		{
			// do logarithmic band separation, octave-style
			if (mSepMode == kSeparationMode_Octaval)
			{
				return baseFreq * std::pow(2.0, mSepAmount_Octaval * bandcount);
			}
			// do linear band separation, hertz-style
			return baseFreq + (baseFreq * mSepAmount_Linear * bandcount);
		}();
		auto const bandCenterFreq = mBandCenterFreq[currentNote][bandcount].getValue();

// SHRINK THE NUMBER OF BANDS IF MISTAKES ARE "OFF" AND THE NEXT BAND WILL EXCEED THE NYQUIST
		if (!mFoldover && (bandCenterFreq > mNyquist))
		{
			clearFilterOutputForBands(bandcount);
			return bandcount;  // there's no need to do the calculations if we won't be using this or the remaining bands
		}

		mBandBandwidth[currentNote][bandcount] = getBandwidthForFreq(bandCenterFreq);
	
// CALCULATE THE COEFFICIENTS FOR THE 2 DELAYED INPUTS IN THE FILTER
// and CALCULATE THE COEFFICIENT FOR THE CURRENT INPUT SAMPLE IN THE FILTER
		// kScaleMode_None -> no scaling; input gain = 1
		mInputAmp[bandcount] = 1.0;
		auto const r = std::exp(mBandBandwidth[currentNote][bandcount].getValue() * -mPiDivSR);
		switch (mResonAlgorithm)
		{
			// based on the reson opcode in Csound
			case kResonAlg_2PoleNoZero:
			default:
				// this value is usually just approaching 1 from below (i.e. 0.999) but gets smaller 
				// and perhaps approaches 0 as bandwidth and/or the center freq distance from base freq grow
				mPrevPrevOutCoeff[bandcount] = std::exp((bandCenterFreq / baseFreq) * mBandBandwidth[currentNote][bandcount].getValue() * -mTwoPiDivSR);
				// this value, at 44.1 kHz, moves in a curve from 2 to -2 as the center frequency goes from 0 to ~20 kHz
				mPrevOutCoeff[bandcount] = mPrevPrevOutCoeff[bandcount] * 4.0 * std::cos(bandCenterFreq * mTwoPiDivSR) / (mPrevPrevOutCoeff[bandcount] + 1.0);
				// unused in this algorithm
				mPrevPrevInCoeff[bandcount] = 0.0;
				//
				// RMS gain = 1
				if (mScaleMode == kScaleMode_RMS)
				{
					mInputAmp[bandcount] = std::sqrt(((mPrevPrevOutCoeff[bandcount] + 1.0) * (mPrevPrevOutCoeff[bandcount] + 1.0) - (mPrevOutCoeff[bandcount] * mPrevOutCoeff[bandcount])) * (1.0 - mPrevPrevOutCoeff[bandcount]) / (mPrevPrevOutCoeff[bandcount] + 1.0));
				}
				// peak gain at center = 1
				else if (mScaleMode == kScaleMode_Peak)
				{
					mInputAmp[bandcount] = (1.0 - mPrevPrevOutCoeff[bandcount]) * std::sqrt(1.0 - (mPrevOutCoeff[bandcount] * mPrevOutCoeff[bandcount] / (mPrevPrevOutCoeff[bandcount] * 4.0)));
				}
				break;

			// an implementation of the 2-pole, 2-zero resononant filter 
			// described by Julius O. Smith and James B. Angell in 
			// "A Constant Gain Digital Resonator Tuned By A Single Coefficient," 
			// Computer Music Journal, Vol. 6, No. 4, Winter 1982, p.36-39 
			// (where the zeros are at +/- the square root of r, 
			// where r is the pole radius of the resonant filter)
			case kResonAlg_2Pole2ZeroR:
				mPrevOutCoeff[bandcount] = 2.0 * r * std::cos(bandCenterFreq * mTwoPiDivSR);
				mPrevPrevOutCoeff[bandcount] = r * r;
				mPrevPrevInCoeff[bandcount] = r;
				//
				if (mScaleMode == kScaleMode_Peak)
				{
					mInputAmp[bandcount] = 1.0 - r;
				}
				else if (mScaleMode == kScaleMode_RMS)
				{
					mInputAmp[bandcount] = std::sqrt(1.0 - r);
				}
				break;

			// version of the above filter where the zeros are located at z = 1 and z = -1
			case kResonAlg_2Pole2Zero1:
				mPrevOutCoeff[bandcount] = 2.0 * r * std::cos(bandCenterFreq * mTwoPiDivSR);
				mPrevPrevOutCoeff[bandcount] = r * r;
				mPrevPrevInCoeff[bandcount] = 1.0;
				//
				if (mScaleMode == kScaleMode_Peak)
				{
					mInputAmp[bandcount] = (1.0 - mPrevPrevOutCoeff[bandcount]) * 0.5;
				}
				else if (mScaleMode == kScaleMode_RMS)
				{
					mInputAmp[bandcount] = std::sqrt((1.0 - mPrevPrevOutCoeff[bandcount]) * 0.5);
				}
				break;
		}
	}  // end of bands loop

	return mNumBands;  // unchanged
}

//-----------------------------------------------------------------------------------------
// This function writes the filtered audio output.
void RezSynth::processFilterOuts(float const* const* inAudio, float* const* outAudio, 
								 unsigned long sampleFrameOffset, unsigned long sampleFrames, 
								 int currentNote, int numBands)
{
	auto const numChannels = getnumoutputs();
	float envAmp = 1.f;
	auto& channelFilters = mLowpassGateFilters[currentNote];

	// here we do the resonant filter equation using our filter coefficients, and related stuff
	for (unsigned long sampleIndex = sampleFrameOffset; sampleIndex < sampleFrames + sampleFrameOffset; sampleIndex++)
	{
		auto const noteAmp = getmidistate().getNoteState(currentNote).mNoteAmp.getValue();
		auto const ampEvener = mAmpEvener[currentNote].getValue();
		// see whether attack or release are active and fetch the output scalar
		if (mFadeType == kCurveType_Lowpass)
		{
			if (((sampleIndex - sampleFrameOffset) % mFreqSmoothingStride) == 0)
			{
				dfx::IIRFilter::Coefficients lpCoeff;
				std::tie(lpCoeff, envAmp) = getmidistate().processEnvelopeLowpassGate(currentNote);
				std::for_each(channelFilters.begin(), channelFilters.end(), [&lpCoeff](auto& filter)
				{
					filter.setCoefficients(lpCoeff);
				});
			}
			else
			{
				getmidistate().processEnvelope(currentNote);  // to temporally progress the envelope's state
			}
		}
		else
		{
			envAmp = getmidistate().processEnvelope(currentNote);
		}
		float const envedTotalAmp = noteAmp * envAmp * mWetGain.getValue() * mOutputGain.getValue();

		for (unsigned long ch = 0; ch < numChannels; ch++)
		{
			double bandOutputSum = 0.;
			for (int bandIndex = 0; bandIndex < numBands; bandIndex++)
			{
				// filter using the input, delayed values, and their filter coefficients
				double const curBandOutValue = (mInputAmp[bandIndex] * (inAudio[ch][sampleIndex] - mPrevPrevInCoeff[bandIndex] * mPrevPrevInValue[ch][currentNote])) 
											   + (mPrevOutCoeff[bandIndex] * mPrevOutValue[ch][currentNote][bandIndex]) 
											   - (mPrevPrevOutCoeff[bandIndex] * mPrevPrevOutValue[ch][currentNote][bandIndex]);

				bandOutputSum += curBandOutValue;
				// very old outValue gets old outValue and old outValue gets current outValue (no longer current)
				mPrevPrevOutValue[ch][currentNote][bandIndex] = std::exchange(mPrevOutValue[ch][currentNote][bandIndex], curBandOutValue);
			}
			auto const scaledBandOutputSum = static_cast<float>(bandOutputSum * ampEvener);

			// add the latest resonator to the output collection, scaled by my evener and user gain
			auto const entryOutput = outAudio[ch][sampleIndex];
			if (mFadeType == kCurveType_Lowpass)
			{
				outAudio[ch][sampleIndex] += channelFilters[ch].process(scaledBandOutputSum) * envedTotalAmp;
			}
			else
			{
				outAudio[ch][sampleIndex] += scaledBandOutputSum * envedTotalAmp;
			}

#if __GNUC__
			if (__builtin_expect(std::isinf(outAudio[ch][sampleIndex]), 0))  // TODO: C++20 [[unlikely]]
#else
			if (std::isinf(outAudio[ch][sampleIndex]))
#endif
			{
				outAudio[ch][sampleIndex] = entryOutput;
			}

			mPrevPrevInValue[ch][currentNote] = std::exchange(mPrevInValue[ch][currentNote], inAudio[ch][sampleIndex]);
		}

		mOutputGain.inc();
		mWetGain.inc();
		mAmpEvener[currentNote].inc();
	}
}

//-----------------------------------------------------------------------------------------
// this function outputs the unprocessed audio input between notes, if desired
void RezSynth::processUnaffected(float const* inAudio, float* outAudio, unsigned long sampleFrames)
{
	for (unsigned long sampleIndex = 0; sampleIndex < sampleFrames; sampleIndex++)
	{
		float unEnvAmp = 1.0f;

		// this is the state when all notes just ended and the clean input first kicks in
		if (mUnaffectedState == UnaffectedState::FadeIn)
		{
			// linear fade-in
			unEnvAmp = static_cast<float>(mUnaffectedFadeSamples) * mUnaffectedFadeStep;
			mUnaffectedFadeSamples++;
			// go to the no-gain state if the fade-in is done
			if (mUnaffectedFadeSamples >= mUnaffectedFadeDur)
			{
				mUnaffectedState = UnaffectedState::Flat;
			}
		}

		// a note has just begun, so we need to hasily fade out the clean input audio
		else if (mUnaffectedState == UnaffectedState::FadeOut)
		{
			mUnaffectedFadeSamples--;
			// linear fade-out
			unEnvAmp = static_cast<float>(mUnaffectedFadeSamples) * mUnaffectedFadeStep;
			// get ready for the next time and exit this function if the fade-out is done
			if (mUnaffectedFadeSamples <= 0)
			{
				// ready for the next time
				mUnaffectedState = UnaffectedState::FadeIn;
				return;  // important!  exit this function or a new fade-in will begin
			}
		}

		outAudio[sampleIndex] += inAudio[sampleIndex] * unEnvAmp * mBetweenGain.getValue() * mWetGain.getValue();

		mBetweenGain.inc();
		mWetGain.inc();
	}
}

//-----------------------------------------------------------------------------------------
double RezSynth::getBandwidthForFreq(double inFreq) const
{
	return (mBandwidthMode == kBandwidthMode_Q) ? (inFreq / mBandwidthAmount_Q) : mBandwidthAmount_Hz;
}

//-----------------------------------------------------------------------------------------
// This function checks if the latest note message is a note-on for a note that's currently off.  
// If it is, then that note's filter feedback buffers are cleared.
void RezSynth::checkForNewNote(long currentEvent)
{
	// store the current note MIDI number
	auto const currentNote = getmidistate().getBlockEvent(currentEvent).mByte1;

	// if this latest event is a note-on and this note isn't still active 
	// from being previously played, then clear this note's delay buffers
	if ((getmidistate().getBlockEvent(currentEvent).mStatus == DfxMidi::kStatus_NoteOn)  // it's a note-on
		&& !getmidistate().isNoteActive(currentNote))  // this note is currently off
	{
		// wipe out the feedback buffers
		for (auto& values : mPrevOutValue)
		{
			values[currentNote].fill(0.0);
		}
		for (auto& values : mPrevPrevOutValue)
		{
			values[currentNote].fill(0.0);
		}
	}
}
