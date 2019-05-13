/*------------------------------------------------------------------------
Copyright (C) 2001-2019  Sophia Poirier

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


//-----------------------------------------------------------------------------------------
// this function tries to even out the wildly erratic resonant amplitudes
double RezSynth::calculateAmpEvener(int currentNote) const
{
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
	double const baseFreq = getmidistate().getNoteFrequency(currentNote) * getmidistate().getPitchBend();

	for (int bandcount = 0; bandcount < mNumBands; bandcount++)
	{
// GET THE CURRENT BAND'S CENTER FREQUENCY
		double bandCenterFreq {};
		// do logarithmic band separation, octave-style
		if (mSepMode == kSeparationMode_Octaval)
		{
			bandCenterFreq = baseFreq * std::pow(2.0, mSepAmount_Octaval * bandcount);
		}
		// do linear band separation, hertz-style
		else
		{
			bandCenterFreq = baseFreq + (baseFreq * mSepAmount_Linear * bandcount);
		}

		auto const bandBandwidth = getBandwidthForFreq(bandCenterFreq);

// SHRINK THE NUMBER OF BANDS IF MISTAKES ARE "OFF" AND THE NEXT BAND WILL EXCEED THE NYQUIST
		if (!mFoldover && (bandCenterFreq > mNyquist))
		{
			return bandcount;  // there's no need to do the calculations if we won't be using this or the remaining bands
		}

// CALCULATE THE COEFFICIENTS FOR THE 2 DELAYED INPUTS IN THE FILTER
// and CALCULATE THE COEFFICIENT FOR THE CURRENT INPUT SAMPLE IN THE FILTER
		// kScaleMode_None -> no scaling; input gain = 1
		mInputAmp[bandcount] = 1.0;
		auto const r = std::exp(bandBandwidth * -mPiDivSR);
		switch (mResonAlgorithm)
		{
			// based on the reson opcode in Csound
			case kResonAlg_2PoleNoZero:
			default:
				// this value is usually just approaching 1 from below (i.e. 0.999) but gets smaller 
				// and perhaps approaches 0 as bandwidth and/or the center freq distance from base freq grow
				mPrevPrevOutCoeff[bandcount] = std::exp(bandCenterFreq/baseFreq * bandBandwidth * -mTwoPiDivSR);
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
void RezSynth::processFilterOuts(float const* inAudio, float* outAudio, 
								 unsigned long sampleFrames, double ampEvener, 
								 int currentNote, int numBands, 
								 double& prevIn, double& prevprevIn, 
								 double* prevOut, double* prevprevOut)
{
	double const noteAmp = ampEvener * static_cast<double>(getmidistate().getNoteState(currentNote).mNoteAmp);

	// here we do the resonant filter equation using our filter coefficients, and related stuff
	for (unsigned long samplecount = 0; samplecount < sampleFrames; samplecount++)
	{
		// see whether attack or release are active and fetch the output scalar
		auto const envAmp = getmidistate().processEnvelope(currentNote);  // the attack/release scalar
		double const envedTotalAmp = noteAmp * static_cast<double>(envAmp * mWetGain.getValue()) * mOutputGain.getValue();

		for (int bandcount = 0; bandcount < numBands; bandcount++)
		{
			// filter using the input, delayed values, and their filter coefficients
			double const curBandOutValue = (mInputAmp[bandcount] * (inAudio[samplecount] - mPrevPrevInCoeff[bandcount] * prevprevIn)) 
										   + (mPrevOutCoeff[bandcount] * prevOut[bandcount]) 
										   - (mPrevPrevOutCoeff[bandcount] * prevprevOut[bandcount]);

			// add the latest resonator to the output collection, scaled by my evener and user gain
			outAudio[samplecount] += static_cast<float>(curBandOutValue * envedTotalAmp);

			// very old outValue gets old outValue and old outValue gets current outValue (no longer current)
			prevprevOut[bandcount] = prevOut[bandcount];
			prevOut[bandcount] = curBandOutValue;
		}
		prevprevIn = prevIn;
		prevIn = inAudio[samplecount];
		mOutputGain.inc();
		mWetGain.inc();
	}
}
/*
void RezSynth::processFilterOuts(float const* inAudio, float* outAudio, unsigned long sampleFrames, double ampEvener, 
								 int currentNote, int numBands, double* prevOut, double* prevprevOut)
{
	double const totalAmp = ampEvener * static_cast<double>(mOutputGain * noteTable[currentNote].mNoteAmp);

	// most of the note envelope values are liable to change below,
	// so we back them up to allow multi-band repetition
	auto const noteTemp = noteTable[currentNote];  // backup value holder for note envelope information

	// here we do the resonant filter equation using our filter coefficients, and related stuff
	for (int bandcount = 0; bandcount < numBands; bandcount++)
	{
		// restore the note values before doing the next band
		noteTable[currentNote] = noteTemp;

		for (unsigned long samplecount = 0; samplecount < sampleFrames; samplecount++)
		{
			// filter using the input, delayed values, and their filter coefficients
			double const curBandOutValue = (mInputAmp[bandcount] * (inAudio[samplecount] - mPrevPrevInCoeff[bandcount] * prevprevIn)) + (mPrevOutCoeff[bandcount] * prevOut[bandcount]) - (mPrevPrevOutCoeff[bandcount] * prevprevOut[bandcount]);

			// add the latest resonator to the output collection, scaled by my evener and user gain
			outAudio[samplecount] += static_cast<float>(curBandOutValue * totalAmp * static_cast<double>(processEnvelope(&noteTable[currentNote])));

			// very old outValue gets old outValue and old outValue gets current outValue (no longer current)
			prevprevOut[bandcount] = prevOut[bandcount];
			prevOut[bandcount] = curBandOutValue;
		}
	}
}
*/

//-----------------------------------------------------------------------------------------
// this function outputs the unprocessed audio input between notes, if desired
void RezSynth::processUnaffected(float const* inAudio, float* outAudio, unsigned long sampleFrames)
{
	for (unsigned long samplecount = 0; samplecount < sampleFrames; samplecount++)
	{
		float unEnvAmp = 1.0f;

		// this is the state when all notes just ended and the clean input first kicks in
		if (mUnaffectedState == UnaffectedState::FadeIn)
		{
			// linear fade-in
			unEnvAmp = static_cast<float>(mUnaffectedFadeSamples) * kUnaffectedFadeStep;
			mUnaffectedFadeSamples++;
			// go to the no-gain state if the fade-in is done
			if (mUnaffectedFadeSamples >= kUnaffectedFadeDur)
			{
				mUnaffectedState = UnaffectedState::Flat;
			}
		}

		// a note has just begun, so we need to hasily fade out the clean input audio
		else if (mUnaffectedState == UnaffectedState::FadeOut)
		{
			mUnaffectedFadeSamples--;
			// linear fade-out
			unEnvAmp = static_cast<float>(mUnaffectedFadeSamples) * kUnaffectedFadeStep;
			// get ready for the next time and exit this function if the fade-out is done
			if (mUnaffectedFadeSamples <= 0)
			{
				// ready for the next time
				mUnaffectedState = UnaffectedState::FadeIn;
				return;  // important!  exit this function or a new fade-in will begin
			}
		}

		outAudio[samplecount] += inAudio[samplecount] * unEnvAmp * mBetweenGain.getValue() * mWetGain.getValue();

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
void RezSynth::checkForNewNote(long currentEvent, unsigned long numChannels)
{
	// store the current note MIDI number
	auto const currentNote = getmidistate().getBlockEvent(currentEvent).mByte1;

	// if this latest event is a note-on and this note isn't still active 
	// from being previously played, then clear this note's delay buffers
	if ((getmidistate().getBlockEvent(currentEvent).mStatus == DfxMidi::kStatus_NoteOn)  // it's a note-on
		&& (getmidistate().getNoteState(currentNote).mVelocity == 0))  // the note is currently off
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
