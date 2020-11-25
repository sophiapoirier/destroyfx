/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2009-2020  Sophia Poirier

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
This is our class for doing interpolation of values over time.
------------------------------------------------------------------------*/

#pragma once


#include "idfxsmoothedvalue.h"

#include <stddef.h>



namespace dfx
{


//-----------------------------------------------------------------------------
// A smoothed floating point value (T = float or double).
// The value is linearly interpolated for the given number of seconds.
// Client indicates the passage of time manually by calling inc() each sample.
template <typename T>
class SmoothedValue final : public ISmoothedValue
{
public:
	explicit SmoothedValue(double inSmoothingTimeInSeconds = 0.030);

	void setValue(T inTargetValue) noexcept;
	void setValueNow(T inValue) noexcept;
	// Immediately snap to the target value.
	void snap() noexcept override;
	T getValue() const noexcept
	{
		return mCurrentValue;
	}

	bool isSmoothing() const noexcept override;
	// Advance one sample.
	void inc() noexcept override;
	// advance N samples
	void inc(size_t inCount) noexcept override;

	double getSmoothingTime() const noexcept override
	{
		return mSmoothDur_seconds;
	}
	void setSmoothingTime(double inSmoothingTimeInSeconds) override;
	void setSampleRate(double inSampleRate) override;

	SmoothedValue<T>& operator=(T inValue) noexcept;

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
