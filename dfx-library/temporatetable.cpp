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

To contact the author, use the contact form at http://destroyfx.org

Destroy FX is a sovereign entity comprised of Sophia Poirier and Tom Murphy 7.
Welcome to our tempo rate table.
------------------------------------------------------------------------*/

#include "temporatetable.h"

#include <cassert>
#include <cmath>


//-----------------------------------------------------------------------------
dfx::TempoRateTable::TempoRateTable(Rates inRates)
{
	auto const addTempoRate = [this](int numerator, int denominator = 1)
	{
		mScalars.push_back(static_cast<double>(numerator));
		mDisplays.push_back(std::to_string(numerator));
		if (denominator > 1)
		{
			mScalars.back() /= static_cast<double>(denominator);
			mDisplays.back().append("/").append(std::to_string(denominator));
		}
	};

	if (inRates == Rates::Slow)
	{
		addTempoRate(1, 12);
		addTempoRate(1, 8);
		addTempoRate(1, 7);
	}
	if (inRates != Rates::NoExtreme)
	{
		addTempoRate(1, 6);
		addTempoRate(1, 5);
	}
	addTempoRate(1, 4);
	addTempoRate(1, 3);
	addTempoRate(1, 2);
	addTempoRate(2, 3);
	addTempoRate(3, 4);
	addTempoRate(1);
	addTempoRate(2);
	addTempoRate(3);
	addTempoRate(4);
	addTempoRate(5);
	addTempoRate(6);
	addTempoRate(7);
	addTempoRate(8);
	addTempoRate(12);
	addTempoRate(16);
	addTempoRate(24);
	addTempoRate(32);
	addTempoRate(48);
	addTempoRate(64);
	addTempoRate(96);
	if (inRates != Rates::Slow)
	{
		addTempoRate(333);
	}
	if ((inRates != Rates::Slow) && (inRates != Rates::NoExtreme))
	{
		mScalars.push_back(3000.);
		mDisplays.push_back("infinity");
	}

	assert(mScalars.size() == mDisplays.size());
}

//-----------------------------------------------------------------------------
// given a tempo rate value, return the index of the tempo rate 
// that is closest to that requested value
size_t dfx::TempoRateTable::getNearestTempoRateIndex(double inTempoRateValue) const
{
	auto bestDiff = mScalars.back();
	size_t bestIndex = 0;
	for (size_t i = 0; i < getNumRates(); i++)
	{
		auto const diff = std::fabs(inTempoRateValue - mScalars[i]);
		if (diff < bestDiff)
		{
			bestDiff = diff;
			bestIndex = i;
		}
	}
	return bestIndex;
}
