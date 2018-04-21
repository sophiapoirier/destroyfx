/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2018  Sophia Poirier

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

Destroy FX is a sovereign entity comprised of Sophia Poirier and Tom Murphy 7.
Welcome to our Low Frequency Oscillator.
------------------------------------------------------------------------*/

#include "lfo.h"

#include <algorithm>
#include <cassert>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "dfxmath.h"


//------------------------------------------------------------------------
dfx::LFO::LFO()
{
	fillLFOtables();
	mTable = mSineTable.data();  // just to have it pointing to something at least

	srand(static_cast<unsigned int>(time(nullptr)));  // sets a seed value for rand() from the system clock

	reset();
}

//------------------------------------------------------------------------
void dfx::LFO::reset()
{
	mPosition = 0.0f;
	mStepSize = 1.0f;  // just to avoid anything really screwy
	mPrevRandomNumber = dfx::math::Rand<decltype(mPrevRandomNumber)>();
	mRandomNumber = dfx::math::Rand<decltype(mRandomNumber)>();
	mSmoothSamples = 0;
}

//-----------------------------------------------------------------------------------------
// this function creates tables for mapping out the sine, triangle, and saw LFO shapes
void dfx::LFO::fillLFOtables()
{
	long i {};


	// fill the sine waveform table (oscillates from 0 to 1 and back to 0)
	for (i = 0; i < kNumPoints; i++)
	{
		mSineTable[i] = (std::sin(((static_cast<float>(i) * kTableStep) - 0.25f) * 2.0f * dfx::math::kPi<float>) + 1.0f) * 0.5f;
	}

	// fill the triangle waveform table
	// ramp from 0 to 1 for the first half
	for (i = 0; i < (kNumPoints / 2); i++)
	{
		mTriangleTable[i] = static_cast<float>(i) / static_cast<float>(kNumPoints / 2);
	}
	// and ramp from 1 to 0 for the second half
	for (long n = 0; i < kNumPoints; n++)
	{
		mTriangleTable[i] = 1.0f - (static_cast<float>(n) / static_cast<float>(kNumPoints / 2));
		i++;
	}

	// fill the square waveform table
	// stay at 1 for the first half
	for (i = 0; i < (kNumPoints / 2); i++)
	{
		mSquareTable[i] = 1.0f;
	}
	// and 0 for the second half
	for (long n = 0; i < kNumPoints; n++)
	{
		mSquareTable[i] = 0.0f;
		i++;
	}

	// fill the sawtooth waveform table (ramps from 0 to 1)
	for (i = 0; i < kNumPoints; i++)
	{
		mSawTable[i] = static_cast<float>(i) / static_cast<float>(kNumPoints - 1);
	}

	// fill the reverse sawtooth waveform table (ramps from 1 to 0)
	for (i = 0; i < kNumPoints; i++)
	{
		mReverseSawTable[i] = static_cast<float>(kNumPoints - i - 1) / static_cast<float>(kNumPoints - 1);
	}

	// fill the thorn waveform table
	// exponentially slope up from 0 to 1 for the first half
	for (i = 0; i < (kNumPoints / 2); i++)
	{
		mThornTable[i] = std::pow((static_cast<float>(i) / static_cast<float>(kNumPoints / 2)), 2.0f);
	}
	// and exponentially slope down from 1 to 0 for the second half
	for (long n = 0; i < kNumPoints; n++)
	{
		mThornTable[i] = std::pow((1.0f - (static_cast<float>(n) / static_cast<float>(kNumPoints / 2))), 2.0f);
		i++;
	}
}

//--------------------------------------------------------------------------------------
std::string dfx::LFO::getShapeName(Shape inShape) const
{
	switch (inShape)
	{
		case kShape_Sine                : return "sine";
		case kShape_Triangle            : return "triangle";
		case kShape_Square              : return "square";
		case kShape_Saw                 : return "sawtooth";
		case kShape_ReverseSaw          : return "reverse sawtooth";
		case kShape_Thorn               : return "thorn";
		case kShape_Random              : return "random";
		case kShape_RandomInterpolating : return "random interpolating";
		default: assert(false); return {};
	}
}

//--------------------------------------------------------------------------------------
// this function points the LFO table pointers to the correct waveform tables
void dfx::LFO::pickTheWaveform()
{
	switch (mShape)
	{
		case kShape_Sine:
			mTable = mSineTable.data();
			break;
		case kShape_Triangle:
			mTable = mTriangleTable.data();
			break;
		case kShape_Square:
			mTable = mSquareTable.data();
			break;
		case kShape_Saw:
			mTable = mSawTable.data();
			break;
		case kShape_ReverseSaw:
			mTable = mReverseSawTable.data();
			break;
		case kShape_Thorn:
			mTable = mThornTable.data();
			break;
		default:
			mTable = nullptr;
			break;
	}
}

//--------------------------------------------------------------------------------------
void dfx::LFO::setDepth(float inDepth)
{
	mDepth = inDepth;
}

//--------------------------------------------------------------------------------------
void dfx::LFO::setShape(Shape inShape)
{
	mShape = inShape;
}

//--------------------------------------------------------------------------------------
void dfx::LFO::setStepSize(float inStepSize)
{
	mStepSize = inStepSize;
}

//--------------------------------------------------------------------------------------
// calculates the position within an LFO's cycle needed to sync to the song's beat
void dfx::LFO::syncToTheBeat(long inSamplesToBar)
{
	// calculate how many samples long the LFO cycle is
	float const cyclesize = kNumPoints_f / mStepSize;
	// calculate many more samples it will take for this cycle to coincide with the beat
	float const countdown = std::fmod(static_cast<float>(inSamplesToBar), cyclesize);
	// and convert that into the correct LFO position according to its table step size
	mPosition = (cyclesize - countdown) * mStepSize;
	// wrap around the new position if it is beyond the end of the LFO table
	if (mPosition >= kNumPoints_f)
	{
		mPosition = std::fmod(mPosition, kNumPoints_f);
	}
	mPosition = std::max(mPosition, 0.0f);
}
