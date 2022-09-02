/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2001-2022  Sophia Poirier

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
Welcome to our Infinite Impulse Response filter.
------------------------------------------------------------------------*/

#pragma once


#include <array>
#include <vector>

#include "dfxmath.h"


// too unstable when crossover frequency is modulated, though performance is faster
#define DFX_CROSSOVER_LINKWITZ_RILEY_MUSICDSP 0


namespace dfx
{


//-----------------------------------------------------------------------------
class IIRFilter
{
public:
	enum class FilterType
	{
		Lowpass,
		Highpass,
		Bandpass,
		Peak,
		Notch,
		LowShelf,
		HighShelf
	};

	struct Coefficients
	{
		float mIn = 0.0f;
		float mPrevIn = 0.0f;
		float mPrevPrevIn = 0.0f;
		float mPrevOut = 0.0f;
		float mPrevPrevOut = 0.0f;
	};

	static constexpr Coefficients kZeroCoeff = { 0.0, 0.0, 0.0, 0.0, 0.0 };
	static constexpr Coefficients kUnityCoeff = { 1.0, 0.0, 0.0, 0.0, 0.0 };
	static_assert(kUnityCoeff.mIn == 1.0);  // protect against possible future member reordering
	static constexpr double kShelfStartLowpass = 0.333;

	IIRFilter() = default;
	explicit IIRFilter(double inSampleRate);

	void setCoefficients(Coefficients const& inCoefficients);
	Coefficients const& setCoefficients(FilterType inFilterType, double inFrequency, double inQ, double inGain);
	Coefficients const& setCoefficients(FilterType inFilterType, double inFrequency, double inQ);
	Coefficients const& setLowpassCoefficients(double inCutoffFrequency);
	Coefficients const& setHighpassCoefficients(double inCutoffFrequency);
	Coefficients const& setBandpassCoefficients(double inCenterFrequency, double inQ);
	void copyCoefficients(IIRFilter const& inSourceFilter) noexcept;
	auto getCoefficients() const noexcept { return mCoeff; }
	void setSampleRate(double inSampleRate);

	void reset() noexcept;


	[[nodiscard]] float process(float inSample)
	{
		mPrevPrevOut = mPrevOut;
		mPrevOut = mCurrentOut;

#ifdef DFX_IIRFILTER_USE_OPTIMIZATION_FOR_EXCLUSIVELY_LP_HP_NOTCH  // one fewer multiplication
		mCurrentOut = ((inSample + mPrevPrevIn) * mCoeff.mIn) + (mPrevIn * mCoeff.mPrevIn) 
						- (mPrevOut * mCoeff.mPrevOut) - (mPrevPrevOut * mCoeff.mPrevPrevOut);
#else
		mCurrentOut = (inSample * mCoeff.mIn) + (mPrevIn * mCoeff.mPrevIn) + (mPrevPrevIn * mCoeff.mPrevPrevIn) 
						- (mPrevOut * mCoeff.mPrevOut) - (mPrevPrevOut * mCoeff.mPrevPrevOut);
#endif
		mCurrentOut = dfx::math::ClampDenormal(mCurrentOut);

		mPrevPrevIn = mPrevIn;
		mPrevIn = inSample;

		return mCurrentOut;
	}

	void processToCache(float inSample)
	{
		// store four samples of history if we're preprocessing for Hermite interpolation
		mPrevPrevPrevOut = mPrevPrevOut;
		std::ignore = process(inSample);
	}

// start of pre-Hermite-specific functions
// there are four versions, three of which unroll for loops of two, three, and four iterations

	void processToCacheH1(float inSample)
	{
		mPrevPrevPrevOut = mPrevPrevOut;
		mPrevPrevOut = mPrevOut;
		mPrevOut = mCurrentOut;
		//
		// XXX this uses an optimization that only works for LP, HP, and notch filters
		mCurrentOut = ((inSample + mPrevPrevIn) * mCoeff.mIn) + (mPrevIn * mCoeff.mPrevIn)
						- (mPrevOut * mCoeff.mPrevOut) - (mPrevPrevOut * mCoeff.mPrevPrevOut);
		mCurrentOut = dfx::math::ClampDenormal(mCurrentOut);
		//
		mPrevPrevIn = mPrevIn;
		mPrevIn = inSample;
	}

	void processToCacheH2(float * inAudio, long inPos, long inBufferSize)
	{
		auto const in0 = inAudio[inPos];
		auto const in1 = inAudio[(inPos + 1) % inBufferSize];

		mPrevPrevPrevOut = mPrevPrevOut;
		mPrevPrevOut = mPrevOut;
		mPrevOut = mCurrentOut;
		// XXX this uses an optimization that only works for LP, HP, and notch filters
		mCurrentOut = ((in0 + mPrevPrevIn) * mCoeff.mIn) + (mPrevIn * mCoeff.mPrevIn)
						- (mPrevOut * mCoeff.mPrevOut) - (mPrevPrevOut * mCoeff.mPrevPrevOut);
		//
		mPrevPrevPrevOut = mPrevPrevOut;
		mPrevPrevOut = mPrevOut;
		mPrevOut = mCurrentOut;
		mCurrentOut = ((in1 + mPrevIn) * mCoeff.mIn) + (in0 * mCoeff.mPrevIn)
						- (mPrevOut * mCoeff.mPrevOut) - (mPrevPrevOut * mCoeff.mPrevPrevOut);
		//
		mCurrentOut = dfx::math::ClampDenormal(mCurrentOut);
		mPrevPrevIn = in0;
		mPrevIn = in1;
	}

	void processToCacheH3(float * inAudio, long inPos, long inBufferSize)
	{
		auto const in0 = inAudio[inPos];
		auto const in1 = inAudio[(inPos + 1) % inBufferSize];
		auto const in2 = inAudio[(inPos + 2) % inBufferSize];

		// XXX this uses an optimization that only works for LP, HP, and notch filters
		mPrevPrevPrevOut = ((in0 + mPrevPrevIn) * mCoeff.mIn) + (mPrevIn * mCoeff.mPrevIn)
							- (mCurrentOut * mCoeff.mPrevOut) - (mPrevOut * mCoeff.mPrevPrevOut);
		mPrevPrevOut = ((in1 + mPrevIn) * mCoeff.mIn) + (in0 * mCoeff.mPrevIn)
						- (mPrevPrevPrevOut * mCoeff.mPrevOut) - (mCurrentOut * mCoeff.mPrevPrevOut);
		mPrevOut = ((in2 + in0) * mCoeff.mIn) + (in1 * mCoeff.mPrevIn)
					- (mPrevPrevOut * mCoeff.mPrevOut) - (mPrevPrevPrevOut * mCoeff.mPrevPrevOut);
		//
		mCurrentOut = mPrevOut;
		mCurrentOut = dfx::math::ClampDenormal(mCurrentOut);
		mPrevOut = mPrevPrevOut;
		mPrevPrevOut = mPrevPrevPrevOut;
		mPrevPrevPrevOut = mCurrentOut;
		//
		mPrevPrevIn = in1;
		mPrevIn = in2;
	}

	void processToCacheH4(float * inAudio, long inPos, long inBufferSize)
	{
		auto const in0 = inAudio[inPos];
		auto const in1 = inAudio[(inPos + 1) % inBufferSize];
		auto const in2 = inAudio[(inPos + 2) % inBufferSize];
		auto const in3 = inAudio[(inPos + 3) % inBufferSize];

		// XXX this uses an optimization that only works for LP, HP, and notch filters
		mPrevPrevPrevOut = ((in0 + mPrevPrevIn) * mCoeff.mIn) + (mPrevIn * mCoeff.mPrevIn)
							- (mCurrentOut * mCoeff.mPrevOut) - (mPrevOut * mCoeff.mPrevPrevOut);
		mPrevPrevOut = ((in1 + mPrevIn) * mCoeff.mIn) + (in0 * mCoeff.mPrevIn)
						- (mPrevPrevPrevOut * mCoeff.mPrevOut) - (mCurrentOut * mCoeff.mPrevPrevOut);
		mPrevOut = ((in2 + in0) * mCoeff.mIn) + (in1 * mCoeff.mPrevIn)
					- (mPrevPrevOut * mCoeff.mPrevOut) - (mPrevPrevPrevOut * mCoeff.mPrevPrevOut);
		mCurrentOut = ((in3 + in1) * mCoeff.mIn) + (in2 * mCoeff.mPrevIn)
						- (mPrevOut * mCoeff.mPrevOut) - (mPrevPrevOut * mCoeff.mPrevPrevOut);
		mCurrentOut = dfx::math::ClampDenormal(mCurrentOut);
		//
		mPrevPrevIn = in2;
		mPrevIn = in3;
	}

	// 4-point Hermite spline interpolation for use with IIR filter output histories
	float interpolateHermitePostFilter(double inPos) const
	{
		auto const posFract = static_cast<float>(std::fmod(inPos, 1.0));

		float const a = ((3.0f * (mPrevPrevOut-mPrevOut)) - mPrevPrevPrevOut + mCurrentOut) * 0.5f;
		float const b = (2.0f * mPrevOut) + mPrevPrevPrevOut - (2.5f * mPrevPrevOut) - (mCurrentOut * 0.5f);
		float const c = (mPrevOut - mPrevPrevPrevOut) * 0.5f;

		return ((((a * posFract) + b) * posFract + c) * posFract) + mPrevPrevOut;
	}


private:
	FilterType mFilterType {};
	double mFilterFrequency = 1.0;
	double mFilterQ = 1.0;
	double mFilterGain = 1.0;
	double mSampleRate = 1.0;

	float mPrevIn = 0.0f, mPrevPrevIn = 0.0f;
	float mPrevOut = 0.0f, mPrevPrevOut = 0.0f, mPrevPrevPrevOut = 0.0f, mCurrentOut = 0.0f;
	Coefficients mCoeff;
};



//-----------------------------------------------------------------------------
class Crossover
{
public:
	Crossover(size_t inChannelCount, double inSampleRate, double inFrequency);

	// the Linkwitz–Riley 4th-order filters are not stable with quickly changing cutoff frequency, 
	// so if changes can be modulated, smooth the changes per-sample (no striding)
	void setFrequency(double inFrequency);
	void reset();
	// result contains the low audio portion followed by the high
	std::pair<float, float> process(size_t inChannel, float inSample);

private:
	double const mSampleRate;

#if DFX_CROSSOVER_LINKWITZ_RILEY_MUSICDSP
	struct InputCoeff
	{
		double mA0 {}, mA1 {}, mA2 {};
	} mLowpassCoeff, mHighpassCoeff;
	double mB1 {}, mB2 {}, mB3 {}, mB4 {};

	struct History
	{
		double mX1 {}, mX2 {}, mX3 {}, mX4 {}, mY1 {}, mY2 {}, mY3 {}, mY4 {};
		void reset() noexcept { mX1 = mX2 = mX3 = mX4 = mY1 = mY2 = mY3 = mY4 = 0.f; }
	};
	std::vector<History> mLowpassHistories, mHighpassHistories;
#else
	// cascade two 2nd-order Butterworth lowpass and highpass filters  
	// in series for the low and high output (respectively) to create  
	// 4th-order Linkwitz-Riley filters with flat summed output
	std::vector<std::array<IIRFilter, 2>> mLowpassFilters, mHighpassFilters;
#endif
};


}  // namespace
