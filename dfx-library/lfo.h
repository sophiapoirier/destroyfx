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

#pragma once

#include <string>


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

	LFO();

	void reset();

	void pickTheWaveform();  // TODO: omigoddess please remove this horrid method and handle state changes automatically
	static std::string getShapeName(Shape inShape);

	void setDepth(float inDepth);
	void setShape(Shape inShape);

	void setStepSize(float inStepSize);
	void syncToTheBeat(long inSamplesToBar);

	void updatePosition(long inNumSteps = 1);
	float process() const;

	//--------------------------------------------------------------------------------------
	// scales the output of process from 0.0 - 1.0 output to 0.0 - 2.0 (oscillating around 1.0)
	float processZeroToTwo() const
	{
		return (process() * 2.0f) - mDepth + 1.0f;
	}


private:
	static constexpr long kSmoothDur = 48;

	static float sineGenerator(float inPosition);
	static float triangleGenerator(float inPosition);
	static float squareGenerator(float inPosition);
	static float sawGenerator(float inPosition);
	static float reverseSawGenerator(float inPosition);
	static float thornGenerator(float inPosition);


	float(*mGenerator)(float) = nullptr;  // LFO waveform generator function pointer
	float mPosition = 0.0f;  // the position in the LFO cycle
	float mStepSize = 0.0f;  // size of the steps through the LFO cycle
	float mRandomNumber = 0.0f;  // random values for the random LFO waveforms
	float mPrevRandomNumber = 0.0f;  // previous random values for the random interpolating LFO waveform
	long mSmoothSamples = 0;  // TODO: a counter for the position during a smoothing fade

	float mDepth = 0.0f;
	Shape mShape {};
};


}  // namespace
