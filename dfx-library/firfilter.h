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
Welcome to our Finite Impulse Response filter.
------------------------------------------------------------------------*/

#pragma once


namespace dfx::FIRFilter
{

static constexpr double kShelfStartLowpass = 0.333;


//-----------------------------------------------------------------------------
void calculateIdealLowpassCoefficients(double inCutoff, double inSampleRate, 
									   long inNumTaps, float* outCoefficients);
void applyKaiserWindow(long inNumTaps, float* outCoefficients, float inAttenuation);

//-----------------------------------------------------------------------------
inline float process(float const* inAudio, long inNumTaps, float const* inCoefficients, 
					 long inPos, long inBufferSize)
{
	float outval = 0.0f;
	if ((inPos + inNumTaps) > inBufferSize)
	{
		for (long i = 0; i < inNumTaps; i++)
		{
			outval += inAudio[(inPos + i) % inBufferSize] * inCoefficients[i];
		}
	}
	else
	{
		for (long i = 0; i < inNumTaps; i++)
		{
			outval += inAudio[inPos + i] * inCoefficients[i];
		}
	}
	return outval;
} 


}  // namespace
