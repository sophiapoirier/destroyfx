/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2009-2026  Sophia Poirier

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

To contact the author, use the contact form at http://destroyfx.org

Destroy FX is a sovereign entity comprised of Sophia Poirier and Tom Murphy 7.  
This is our class for doing interpolation of values over time.
------------------------------------------------------------------------*/

#pragma once


#include "idfxsmoothedvalue.h"

#include <concepts>
#include <cstddef>



namespace dfx
{


//-----------------------------------------------------------------------------
// A smoothed floating point value (T = float or double).
// The value is linearly interpolated for the given number of seconds.
// Client indicates the passage of time manually by calling inc() each sample.
template <std::floating_point T>
class SmoothedValue final : public ISmoothedValue
{
public:
	constexpr explicit SmoothedValue(double inSmoothingTimeInSeconds = 0.030) noexcept DFX_RT_ATTR;

	constexpr void setValue(T inTargetValue) noexcept DFX_RT_ATTR;
	constexpr void setValueNow(T inValue) noexcept DFX_RT_ATTR;
	// Immediately snap to the target value.
	constexpr void snap() noexcept DFX_RT_ATTR override;
	constexpr T getValue() const noexcept DFX_RT_ATTR
	{
		return mCurrentValue;
	}

	constexpr bool isSmoothing() const noexcept DFX_RT_ATTR override;
	// Advance one sample.
	constexpr void inc() noexcept DFX_RT_ATTR override;
	// advance N samples
	constexpr void inc(size_t inCount) noexcept DFX_RT_ATTR override;

	constexpr double getSmoothingTime() const noexcept DFX_RT_ATTR override
	{
		return mSmoothDur_seconds;
	}
	constexpr void setSmoothingTime(double inSmoothingTimeInSeconds) noexcept DFX_RT_ATTR override;
	constexpr void setSampleRate(double inSampleRate) noexcept DFX_RT_ATTR override;

	constexpr SmoothedValue<T>& operator=(T inValue) noexcept DFX_RT_ATTR;

private:
	T mCurrentValue {}, mTargetValue {};
	T mValueStep {};
	double mSmoothDur_seconds = 0.0;
	size_t mSmoothDur_samples = 0, mSmoothCount = 0;
	double mSampleRate = 1.0;
	bool mReinitialize = true;
};


}  // namespace



#include "dfxsmoothedvalue.hpp"
