/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2019  Sophia Poirier

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

#include "dfxmath.h"


//------------------------------------------------------------------------
dfx::LFO::LFO()
{
	mGenerator = sineGenerator;  // just to have it pointing to something at least

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

//--------------------------------------------------------------------------------------
std::string dfx::LFO::getShapeName(Shape inShape)
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
// this function points the LFO generator function pointer to the correct waveform generator
void dfx::LFO::pickTheWaveform()
{
	switch (mShape)
	{
		case kShape_Sine:
			mGenerator = sineGenerator;
			break;
		case kShape_Triangle:
			mGenerator = triangleGenerator;
			break;
		case kShape_Square:
			mGenerator = squareGenerator;
			break;
		case kShape_Saw:
			mGenerator = sawGenerator;
			break;
		case kShape_ReverseSaw:
			mGenerator = reverseSawGenerator;
			break;
		case kShape_Thorn:
			mGenerator = thornGenerator;
			break;
		default:
			mGenerator = nullptr;
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
	float const cycleSize = 1.0f / mStepSize;
	// calculate many more samples it will take for this cycle to coincide with the beat
	float const countdown = std::fmod(static_cast<float>(inSamplesToBar), cycleSize);
	// and convert that into the correct LFO position according to its step size
	mPosition = (cycleSize - countdown) * mStepSize;
	// wrap around the new position if it is beyond the end of the cycle
	mPosition = std::max(mPosition, 0.0f);
	mPosition = std::fmod(mPosition, 1.0f);
}

//-----------------------------------------------------------------------------------------
// This function wraps around the LFO cycle position when it passes the cycle end.
// It also sets up the smoothing counter if a discontiguous LFO waveform is being used.
void dfx::LFO::updatePosition(long inNumSteps)
{
	// increment the LFO position tracker
	mPosition += mStepSize * static_cast<float>(inNumSteps);

	if (mPosition >= 1.0f)
	{
		// wrap around the position tracker if it has made it past the end of the LFO cycle
		mPosition = std::fmod(mPosition, 1.0f);
		// get new random LFO values, too
		mPrevRandomNumber = std::exchange(mRandomNumber, dfx::math::Rand<decltype(mRandomNumber)>());
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
		// check to see if it has just passed the halfway point, where the square waveform drops to zero
		constexpr float squareHalfPoint = 0.5f; 
		if ((mPosition >= squareHalfPoint) && 
			((mPosition - mStepSize) < squareHalfPoint))
		{
			mSmoothSamples = kSmoothDur;
		}
	}
}

//-----------------------------------------------------------------------------------------
// gets the current 0.0 - 1.0 output value of the LFO and increments its position
float dfx::LFO::process() const
{
	float outValue {};

	if (mShape == kShape_RandomInterpolating)
	{
		// interpolate between the previous random number and the new one
		outValue = (mRandomNumber * mPosition) + (mPrevRandomNumber * (1.0f - mPosition));
	}
	else if (mShape == kShape_Random)
	{
		outValue = mRandomNumber;
	}
	else
	{
		outValue = mGenerator(mPosition);
	}

	return outValue * mDepth;
}

//-----------------------------------------------------------------------------------------
// oscillates from 0 to 1 and back to 0
float dfx::LFO::sineGenerator(float inPosition)
{
	return (std::sin((inPosition - 0.25f) * 2.0f * dfx::math::kPi<float>) + 1.0f) * 0.5f;
}

//-----------------------------------------------------------------------------------------
// ramp from 0 to 1 for the first half and ramp from 1 to 0 for the second half
float dfx::LFO::triangleGenerator(float inPosition)
{
	if (inPosition < 0.5f)
	{
		return inPosition * 2.0f;
	}
	else
	{
		return 1.0f - ((inPosition - 0.5f) * 2.0f);
	}
}

//-----------------------------------------------------------------------------------------
// stay at 1 for the first half and 0 for the second half
float dfx::LFO::squareGenerator(float inPosition)
{
	return (inPosition < 0.5f) ? 1.0f : 0.0f;
}

//-----------------------------------------------------------------------------------------
// ramps from 0 to 1
float dfx::LFO::sawGenerator(float inPosition)
{
	return inPosition;
}

//-----------------------------------------------------------------------------------------
// ramps from 1 to 0
float dfx::LFO::reverseSawGenerator(float inPosition)
{
	return 1.0f - inPosition;
}

//-----------------------------------------------------------------------------------------
// exponentially slope up from 0 to 1 for the first half and down from 1 to 0 for the second half
float dfx::LFO::thornGenerator(float inPosition)
{
	return std::pow(triangleGenerator(inPosition), 2.0f);
}
