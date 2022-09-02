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

#pragma once

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
	using Shape = int;

	LFO();

	void reset();

	static std::string getShapeName(Shape inShape);

	double getDepth() const noexcept;
	void setDepth(double inDepth) noexcept;
	Shape getShape() const noexcept;
	void setShape(Shape inShape) noexcept;

	void setStepSize(double inStepSize) noexcept;
	void syncToTheBeat(double inSamplesToBar);

	void updatePosition(size_t inNumSteps = 1);
	double process() const;

	//--------------------------------------------------------------------------------------
	// scales the output of process from 0.0 - 1.0 output to 0.0 - 2.0 (oscillating around 1.0)
	double processZeroToTwo() const
	{
		return (process() * 2.) - mDepth + 1.;
	}

protected:
	static constexpr size_t kSmoothDur = 48;
	[[maybe_unused]] static constexpr double kSmoothStep = 1. / static_cast<double>(kSmoothDur);

	size_t mSmoothSamples = 0;  // TODO: a counter for the position during a smoothing fade

private:
	using Generator = double(*)(double);

	static Generator getGeneratorForShape(Shape inShape) noexcept;
	static double sineGenerator(double inPosition);
	static double triangleGenerator(double inPosition);
	static double squareGenerator(double inPosition);
	static double sawGenerator(double inPosition);
	static double reverseSawGenerator(double inPosition);
	static double thornGenerator(double inPosition);


	double mDepth = 0.;
	Shape mShape {};

	Generator mGenerator = nullptr;  // LFO waveform generator function pointer
	double mPosition = 0.;  // the position in the LFO cycle
	double mStepSize = 0.;  // size of the steps through the LFO cycle
	double mRandomNumber = 0.;  // random values for the random LFO waveforms
	double mPrevRandomNumber = 0.;  // previous random values for the random interpolating LFO waveform
	dfx::math::RandomGenerator<decltype(mRandomNumber)> mRandomGenerator {dfx::math::RandomSeed::Entropic};
};


}  // namespace
