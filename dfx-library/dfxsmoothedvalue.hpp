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


#include "dfxsmoothedvalue.h"

#include <algorithm>
#include <cassert>
#include <type_traits>
#include <utility>



//-----------------------------------------------------------------------------
template <typename T>
dfx::SmoothedValue<T>::SmoothedValue(double inSmoothingTimeInSeconds)
:	mCurrentValue(0),
	mTargetValue(0),
	mValueStep(0),
	mSmoothDur_seconds(0),
	mSmoothDur_samples(0),
	mSmoothCount(0),
	mSampleRate(1.0),
	mReinitialize(true)
{
	static_assert(std::is_floating_point_v<T>);

	setSmoothingTime(inSmoothingTimeInSeconds);
}

//-----------------------------------------------------------------------------
template <typename T>
void dfx::SmoothedValue<T>::setValue(T inTargetValue) noexcept
{
	if (std::exchange(mReinitialize, false))
	{
		setValueNow(inTargetValue);
	}
	else
	{
		if (inTargetValue == mTargetValue)
		{
			return;
		}
		mTargetValue = inTargetValue;
		mValueStep = (mSmoothDur_samples > 0) ? ((mTargetValue - mCurrentValue) / static_cast<T>(mSmoothDur_samples)) : T(0);
		mCurrentValue += mValueStep;
		mSmoothCount = 0;
	}
}

//-----------------------------------------------------------------------------
template <typename T>
void dfx::SmoothedValue<T>::setValueNow(T inValue) noexcept
{
	mCurrentValue = mTargetValue = inValue;
	mSmoothCount = mSmoothDur_samples;
	mReinitialize = false;
}

//-----------------------------------------------------------------------------
template <typename T>
void dfx::SmoothedValue<T>::snap() noexcept
{
	setValueNow(mTargetValue);
}

//-----------------------------------------------------------------------------
template <typename T>
bool dfx::SmoothedValue<T>::isSmoothing() const noexcept
{
	return mSmoothCount < mSmoothDur_samples;
}

//-----------------------------------------------------------------------------
template <typename T>
void dfx::SmoothedValue<T>::inc() noexcept
{
	if (isSmoothing())
	{
		mSmoothCount++;
	}
	if (isSmoothing())
	{
		mCurrentValue += mValueStep;
	}
	else
	{
		mCurrentValue = mTargetValue;
	}
}

//-----------------------------------------------------------------------------
template <typename T>
void dfx::SmoothedValue<T>::setSmoothingTime(double inSmoothingTimeInSeconds)
{
	assert(inSmoothingTimeInSeconds >= 0.0);

	mSmoothDur_seconds = inSmoothingTimeInSeconds;
	mSmoothDur_samples = static_cast<size_t>(std::max(mSmoothDur_seconds * mSampleRate, 0.0));
}

//-----------------------------------------------------------------------------
template <typename T>
void dfx::SmoothedValue<T>::setSampleRate(double inSampleRate)
{
	assert(inSampleRate > 0.0);

	mSampleRate = inSampleRate;
	setSmoothingTime(mSmoothDur_seconds);

	setValueNow(mTargetValue);
}

//-----------------------------------------------------------------------------
template <typename T>
dfx::SmoothedValue<T>& dfx::SmoothedValue<T>::operator=(T inValue) noexcept
{
	setValue(inValue);
	return *this;
}
