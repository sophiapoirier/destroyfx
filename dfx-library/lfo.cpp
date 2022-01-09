/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2022  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 2 of the License, or 
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
#include <cmath>

#include "dfxmath.h"


//------------------------------------------------------------------------
dfx::LFO::LFO()
{
	mGenerator = getGeneratorForShape(mShape);
	reset();
}

//------------------------------------------------------------------------
void dfx::LFO::reset()
{
	mPosition = 0.;
	mStepSize = 1.;  // just to avoid stasis
	mPrevRandomNumber = mRandomGenerator.next();
	mRandomNumber = mRandomGenerator.next();
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
		default:
			assert(false);
			return {};
	}
}

//--------------------------------------------------------------------------------------
// this function points the LFO generator function pointer to the correct waveform generator
dfx::LFO::Generator dfx::LFO::getGeneratorForShape(Shape inShape) noexcept
{
	switch (inShape)
	{
		case kShape_Sine:
			return sineGenerator;
		case kShape_Triangle:
			return triangleGenerator;
		case kShape_Square:
			return squareGenerator;
		case kShape_Saw:
			return sawGenerator;
		case kShape_ReverseSaw:
			return reverseSawGenerator;
		case kShape_Thorn:
			return thornGenerator;
		default:
			return nullptr;
	}
}

//--------------------------------------------------------------------------------------
double dfx::LFO::getDepth() const noexcept
{
	return mDepth;
}

//--------------------------------------------------------------------------------------
void dfx::LFO::setDepth(double inDepth) noexcept
{
	mDepth = inDepth;
}

//--------------------------------------------------------------------------------------
dfx::LFO::Shape dfx::LFO::getShape() const noexcept
{
	return mShape;
}

//--------------------------------------------------------------------------------------
void dfx::LFO::setShape(Shape inShape) noexcept
{
	mShape = inShape;
	mGenerator = getGeneratorForShape(inShape);
}

//--------------------------------------------------------------------------------------
void dfx::LFO::setStepSize(double inStepSize) noexcept
{
	mStepSize = inStepSize;
}

//--------------------------------------------------------------------------------------
// calculates the position within an LFO's cycle needed to sync to the song's beat
void dfx::LFO::syncToTheBeat(long inSamplesToBar)
{
	// calculate how many samples long the LFO cycle is
	double const cycleSize = 1. / mStepSize;
	// calculate many more samples it will take for this cycle to coincide with the beat
	double const countdown = std::fmod(static_cast<double>(inSamplesToBar), cycleSize);
	// and convert that into the correct LFO position according to its step size
	mPosition = (cycleSize - countdown) * mStepSize;
	// wrap around the new position if it is beyond the end of the cycle
	mPosition = std::max(mPosition, 0.);
	mPosition = std::fmod(mPosition, 1.);
}

//-----------------------------------------------------------------------------------------
// This function wraps around the LFO cycle position when it passes the cycle end.
// It also sets up the smoothing counter if a discontiguous LFO waveform is being used.
void dfx::LFO::updatePosition(long inNumSteps)
{
	// increment the LFO position tracker
	mPosition += mStepSize * static_cast<double>(inNumSteps);

	if (mPosition >= 1.)
	{
		// wrap around the position tracker if it has made it past the end of the LFO cycle
		mPosition = std::fmod(mPosition, 1.);
		// get new random LFO values, too
		mPrevRandomNumber = std::exchange(mRandomNumber, mRandomGenerator.next());
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
	else if (mPosition < 0.)
	{
		mPosition = 0.;
	}
	// special check for the square waveform - it also needs smoothing at the half point
	else if (mShape == kShape_Square)
	{
		// check to see if it has just passed the halfway point, where the square waveform drops to zero
		constexpr double squareHalfPoint = 0.5; 
		if ((mPosition >= squareHalfPoint) && 
			((mPosition - mStepSize) < squareHalfPoint))
		{
			mSmoothSamples = kSmoothDur;
		}
	}
}

//-----------------------------------------------------------------------------------------
// gets the current 0.0 - 1.0 output value of the LFO and increments its position
double dfx::LFO::process() const
{
	double outValue {};

	if (mShape == kShape_RandomInterpolating)
	{
		// interpolate between the previous random number and the new one
		outValue = (mRandomNumber * mPosition) + (mPrevRandomNumber * (1. - mPosition));
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
double dfx::LFO::sineGenerator(double inPosition)
{
	return (std::sin((inPosition - 0.25) * 2. * dfx::math::kPi<double>) + 1.) * 0.5;
}

//-----------------------------------------------------------------------------------------
// ramp from 0 to 1 for the first half and ramp from 1 to 0 for the second half
double dfx::LFO::triangleGenerator(double inPosition)
{
	if (inPosition < 0.5)
	{
		return inPosition * 2.;
	}
	else
	{
		return 1. - ((inPosition - 0.5) * 2.);
	}
}

//-----------------------------------------------------------------------------------------
// stay at 1 for the first half and 0 for the second half
double dfx::LFO::squareGenerator(double inPosition)
{
	return (inPosition < 0.5) ? 1. : 0.;
}

//-----------------------------------------------------------------------------------------
// ramps from 0 to 1
double dfx::LFO::sawGenerator(double inPosition)
{
	return inPosition;
}

//-----------------------------------------------------------------------------------------
// ramps from 1 to 0
double dfx::LFO::reverseSawGenerator(double inPosition)
{
	return 1. - inPosition;
}

//-----------------------------------------------------------------------------------------
// exponentially slope up from 0 to 1 for the first half and down from 1 to 0 for the second half
double dfx::LFO::thornGenerator(double inPosition)
{
	return std::pow(triangleGenerator(inPosition), 2.);
}
