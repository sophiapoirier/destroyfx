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
Welcome to our Finite Impulse Response filter.
------------------------------------------------------------------------*/

#pragma once


#include <cstddef>
#include <span>
#include <vector>


namespace dfx::FIRFilter
{

static constexpr double kShelfStartLowpass = 0.333;


//-----------------------------------------------------------------------------
void calculateIdealLowpassCoefficients(double inCutoff, double inSampleRate, 
									   std::span<float> outCoefficients);
void calculateIdealLowpassCoefficients(double inCutoff, double inSampleRate, 
									   std::span<float> outCoefficients, 
									   std::span<float const> inCoefficientsWindow);
void applyKaiserWindow(std::span<float> ioCoefficients, float inAttenuation);
std::vector<float> generateKaiserWindow(size_t inNumTaps, float inAttenuation);

//-----------------------------------------------------------------------------
inline float process(std::span<float const> inAudio, std::span<float const> inCoefficients, size_t inPos)
{
	float outval = 0.0f;
	if ((inPos + inCoefficients.size()) > inAudio.size())
	{
		for (size_t i = 0; i < inCoefficients.size(); i++)
		{
			outval += inAudio[(inPos + i) % inAudio.size()] * inCoefficients[i];
		}
	}
	else
	{
		for (size_t i = 0; i < inCoefficients.size(); i++)
		{
			outval += inAudio[inPos + i] * inCoefficients[i];
		}
	}
	return outval;
} 


}  // namespace
