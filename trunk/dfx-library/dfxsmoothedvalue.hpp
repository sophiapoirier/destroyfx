/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2009-2016  Sophia Poirier

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
#include <assert.h>
#if (__cplusplus >= 201103L)
	#include <type_traits>
#endif



//-----------------------------------------------------------------------------
template <typename T>
DfxSmoothedValue<T>::DfxSmoothedValue(double inSmoothingTimeInSeconds)
:	mCurrentValue(0),
	mTargetValue(0),
	mValueStep(0),
	mSmoothDur_seconds(0),
	mSmoothDur_samples(0),
	mSmoothCount(0),
	mSampleRate(1.0),
	mReinit(true)
{
#if (__cplusplus >= 201103L)
	static_assert(std::is_floating_point<T>::value, "value type must be floating point");
#endif

	setSmoothingTime(inSmoothingTimeInSeconds);
}

//-----------------------------------------------------------------------------
template <typename T>
void DfxSmoothedValue<T>::setValue(T inTargetValue)
{
	if (mReinit)
	{
		setValueNow(inTargetValue);
		mReinit = false;
	}
	else
	{
		if (inTargetValue == mTargetValue)
			return;
		mTargetValue = inTargetValue;
		mValueStep = (mSmoothDur_samples > 0) ? ((mTargetValue - mCurrentValue) / static_cast<T>(mSmoothDur_samples)) : T(0);
		mSmoothCount = 0;
	}
}

//-----------------------------------------------------------------------------
template <typename T>
void DfxSmoothedValue<T>::setValueNow(T inValue)
{
	mCurrentValue = mTargetValue = inValue;
	mSmoothCount = mSmoothDur_samples;
	mReinit = false;
}

//-----------------------------------------------------------------------------
template <typename T>
void DfxSmoothedValue<T>::inc()
{
	if (mSmoothCount < mSmoothDur_samples)
	{
		mCurrentValue += mValueStep;
		mSmoothCount++;
	}
	else
	{
		mCurrentValue = mTargetValue;
	}
}

//-----------------------------------------------------------------------------
template <typename T>
void DfxSmoothedValue<T>::setSmoothingTime(double inSmoothingTimeInSeconds)
{
	assert(inSmoothingTimeInSeconds >= 0.0);

	mSmoothDur_seconds = inSmoothingTimeInSeconds;
	mSmoothDur_samples = static_cast<size_t>(std::max(mSmoothDur_seconds * mSampleRate, 0.0));
}

//-----------------------------------------------------------------------------
template <typename T>
void DfxSmoothedValue<T>::setSampleRate(double inSampleRate)
{
	assert(inSampleRate > 0.0);

	mSampleRate = inSampleRate;
	setSmoothingTime(mSmoothDur_seconds);

	setValueNow(mTargetValue);
}
