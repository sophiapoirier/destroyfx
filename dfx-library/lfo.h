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

#pragma once

#include <algorithm>
#include <array>
#include <string>

#include "dfxmath.h"


namespace dfx
{


//----------------------------------------------------------------------------- 
class LFO
{
public:
	enum
	{
		kShape_Sine,
		kShape_Triangle,
		kShape_Square,
		kShape_Saw,
		kShape_ReverseSaw,
		kShape_Thorn,
		kShape_Random,
		kShape_RandomInterpolating,
		kNumShapes
	};
	typedef int Shape;

	static constexpr long kNumPoints = 512;
	static constexpr auto kNumPoints_f = static_cast<float>(kNumPoints);  // to reduce casting later on
	static constexpr float kTableStep = 1.0f / kNumPoints_f;  // to reduce division and encourage multiplication
	static constexpr long kSmoothDur = 48;

	LFO();

	void reset();

	void pickTheWaveform();  // TODO: omigoddess please remove this horrid method and handle state changes automatically
	std::string getShapeName(Shape inShape) const;

	void setDepth(float inDepth);
	void setShape(Shape inShape);

	void setStepSize(float inStepSize);
	void syncToTheBeat(long inSamplesToBar);


	//--------------------------------------------------------------------------------------
	// This function wraps around the LFO table position when it passes the cycle end.
	// It also sets up the smoothing counter if a discontiguous LFO waveform is being used.
	void updatePosition(long inNumSteps = 1)
	{
		// increment the LFO position tracker
		mPosition += mStepSize * static_cast<float>(inNumSteps);

		if (mPosition >= kNumPoints_f)
		{
			// wrap around the position tracker if it has made it past the end of the LFO table
			mPosition = std::fmod(mPosition, kNumPoints_f);
			// get new random LFO values, too
			mPrevRandomNumber = mRandomNumber;
			mRandomNumber = dfx::math::Rand<decltype(mRandomNumber)>();
			// set up the sample smoothing if a discontiguous waveform's cycle just ended
			switch (mShape)
			{
				case kShape_Square:
				case kShape_Saw:
				case kShape_ReverseSaw:
				case kShape_Random:
					mSmoothSamples = kSmoothDur;
					break;
				default:
					break;
			}
		}
		else if (mPosition < 0.0f)
		{
			mPosition = 0.0f;
		}
		// special check for the square waveform - it also needs smoothing at the half point
		else if (mShape == kShape_Square)
		{
			// check to see if it has just passed the halfway point
			constexpr long squareHalfPoint = kNumPoints / 2;  // the point in the table when the square waveform drops to zero
			if ((static_cast<long>(mPosition) >= squareHalfPoint) && 
				(static_cast<long>(mPosition - mStepSize) < squareHalfPoint))
			{
				mSmoothSamples = kSmoothDur;
			}
		}
	}

	//--------------------------------------------------------------------------------------
	// gets the current 0.0 - 1.0 output value of the LFO and increments its position
	float process() const
	{
		float outValue {};

		if (mShape == kShape_RandomInterpolating)
		{
			// calculate how far into this LFO cycle we are so far, scaled from 0.0 to 1.0
			float const randiScalar = mPosition * kTableStep;
			// interpolate between the previous random number and the new one
			outValue = (mRandomNumber * randiScalar) + (mPrevRandomNumber * (1.0f - randiScalar));
		}
		else if (mShape == kShape_Random)
		{
			outValue = mRandomNumber;
		}
		else
		{
			outValue = mTable[static_cast<long>(mPosition)];
		}

		return outValue * mDepth;
	}

	//--------------------------------------------------------------------------------------
	// scales the output of process from 0.0 - 1.0 output to 0.0 - 2.0 (oscillating around 1.0)
	float processZeroToTwo() const
	{
		return (process() * 2.0f) - mDepth + 1.0f;
	}


private:
	void fillLFOtables();

	// LFO waveform tables
	std::array<float, kNumPoints> mSineTable, mTriangleTable, mSquareTable, mSawTable, mReverseSawTable, mThornTable;
	float const* mTable = nullptr;  // pointer to the LFO table

	float mPosition = 0.0f;  // the position in the LFO table
	float mStepSize = 0.0f;  // size of the steps through the LFO table
	float mRandomNumber = 0.0f;  // random values for the random LFO waveforms
	float mPrevRandomNumber = 0.0f;  // previous random values for the random interpolating LFO waveform
//	float mCycleRate = 0.0f;  // the rate in Hz of the LFO (only used for first layer LFOs)
	long mSmoothSamples = 0.0f;  // a counter for the position during a smoothing fade

	float mDepth = 0.0f;
	Shape mShape {};
};


}  // namespace
