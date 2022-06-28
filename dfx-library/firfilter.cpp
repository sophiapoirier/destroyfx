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

#include "firfilter.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <functional>
#include <numbers>



//-----------------------------------------------------------------------------
float besselIZero(float input);
//float besselIZero2(float input);


//-----------------------------------------------------------------------------
// you're supposed to use use an odd number of taps
void dfx::FIRFilter::calculateIdealLowpassCoefficients(double inCutoff, double inSampleRate, 
													   std::span<float> outCoefficients)
{
	assert(!outCoefficients.empty());
	assert(outCoefficients.size() % 2);

	// get the cutoff as a ratio of cutoff to Nyquist, scaled from 0 to Pi
	double const corner = (inCutoff / (inSampleRate * 0.5)) * std::numbers::pi_v<double>;

	size_t middleCoeff {};
	if (outCoefficients.size() % 2)
	{
		middleCoeff = (outCoefficients.size() - 1) / 2;
		outCoefficients[middleCoeff] = corner / std::numbers::pi_v<double>;
	}
	else
	{
		middleCoeff = outCoefficients.size() / 2;
	}

	for (size_t n = 0; n < middleCoeff; n++)
	{
		double const value = static_cast<double>(n) - (static_cast<double>(outCoefficients.size() - 1) * 0.5);
		outCoefficients[n] = std::sin(value * corner) / (value * std::numbers::pi_v<double>);
		outCoefficients[outCoefficients.size() - 1 - n] = outCoefficients[n];
	}
}

//-----------------------------------------------------------------------------
void dfx::FIRFilter::calculateIdealLowpassCoefficients(double inCutoff, double inSampleRate, 
													   std::span<float> outCoefficients, 
													   std::span<float const> inCoefficientsWindow)
{
	assert(outCoefficients.size() == inCoefficientsWindow.size());
	calculateIdealLowpassCoefficients(inCutoff, inSampleRate, outCoefficients);
	std::transform(outCoefficients.begin(), outCoefficients.end(), inCoefficientsWindow.begin(), outCoefficients.begin(), std::multiplies<>{});
}

//-----------------------------------------------------------------------------
void dfx::FIRFilter::applyKaiserWindow(std::span<float> ioCoefficients, float inAttenuation)
{
	assert(!ioCoefficients.empty());

	// beta is 0 if the attenuation is less than 21 dB
	float beta = 0.0f;
	if (inAttenuation >= 50.0f)
	{
		beta = 0.1102f * (inAttenuation - 8.71f);
	}
	else if ((inAttenuation < 50.0f) && (inAttenuation >= 21.0f))
	{
		beta = 0.5842f * std::pow((inAttenuation - 21.0f), 0.4f);
		beta += 0.07886f * (inAttenuation - 21.0f);
	}

	size_t const halfLength = (ioCoefficients.size() + 1) / 2;
	auto const oneDivBesselIZeroOfBeta = 1.0f / besselIZero(beta);
	for (size_t n = 0; n < halfLength; n++)
	{
		ioCoefficients[n] *= besselIZero(beta * std::sqrt(1.0f - std::pow((1.0f - ((2.0f * n) / static_cast<float>(ioCoefficients.size() - 1))), 2.0f))) * oneDivBesselIZeroOfBeta;
		ioCoefficients[ioCoefficients.size() - 1 - n] = ioCoefficients[n];
	}
}

//-----------------------------------------------------------------------------
std::vector<float> dfx::FIRFilter::generateKaiserWindow(size_t inNumTaps, float inAttenuation)
{
	std::vector<float> coefficientsWindow(inNumTaps, 1.0f);
	applyKaiserWindow(coefficientsWindow, inAttenuation);
	return coefficientsWindow;
}

//-----------------------------------------------------------------------------
float besselIZero(float input)
{
	float sum = 1.0f;
	float const halfIn = input * 0.5f;
	float denominator = 1.0f;
	float numerator = 1.0f;
	for (int m = 1; m <= 32; m++)
	{
		numerator *= halfIn;
		denominator *= static_cast<float>(m);
		float const term = numerator / denominator;
		sum += term * term;
	}
	return sum;
}

//-----------------------------------------------------------------------------
#if 0
float besselIZero2(float input)
{
	float sum = 1.0f;
	float ds = 1.0f;
	float d = 0.0f;

	do
	{
		d += 2.0f;
		ds *= (input * input) / (d * d);
		sum += ds;
	}
	while (ds > (1E-7f * sum));

	return sum;
}
#endif
