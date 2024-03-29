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

#pragma once

#include <algorithm>
#include <string>
#include <vector>


namespace dfx
{


//-------------------------------------------------------------------------- 
// this holds the beat scalar values and textual displays for the tempo rates
class TempoRateTable
{
public:
	// the number of tempo beat division options
	enum class Rates
	{
		Normal,
		Slow,
		NoExtreme
	};

	explicit TempoRateTable(Rates inRates = Rates::Normal);

	double getScalar(size_t inIndex) const
	{
		return mScalars[boundedIndex(inIndex)];
	}
	std::string const& getDisplay(size_t inIndex) const
	{
		return mDisplays[boundedIndex(inIndex)];
	}
	auto getNumRates() const noexcept
	{
		return mScalars.size();
	}
	size_t getNearestTempoRateIndex(double inTempoRateValue) const;

private:
	size_t boundedIndex(size_t inIndex) const noexcept
	{
		return std::min(inIndex, getNumRates() - 1);
	}

	std::vector<double> mScalars;
	std::vector<std::string> mDisplays;
};


}  // namespace
