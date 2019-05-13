/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2009-2019  Sophia Poirier

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
This is our class for doing all kinds of fancy plugin parameter stuff.
------------------------------------------------------------------------*/

#pragma once


#include "idfxsmoothedvalue.h"

#include <stddef.h>



namespace dfx
{


//-----------------------------------------------------------------------------
template <typename T>
class SmoothedValue final : public ISmoothedValue
{
public:
	explicit SmoothedValue(double inSmoothingTimeInSeconds = 0.030);

	void setValue(T inTargetValue) noexcept;
	void setValueNow(T inValue) noexcept;
	void snap() noexcept override;
	T getValue() const noexcept
	{
		return mCurrentValue;
	}

	bool isSmoothing() const noexcept override;
	void inc() noexcept override;

	void setSmoothingTime(double inSmoothingTimeInSeconds) override;
	void setSampleRate(double inSampleRate) override;

	SmoothedValue<T>& operator=(T inValue) noexcept;

private:
	T mCurrentValue, mTargetValue;
	T mValueStep;
	double mSmoothDur_seconds;
	size_t mSmoothDur_samples, mSmoothCount;
	double mSampleRate;
	bool mReinitialize;
};


}  // namespace



#include "dfxsmoothedvalue.hpp"
