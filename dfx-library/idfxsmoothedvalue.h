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

To contact the author, use the contact form at http://destroyfx.org/

Destroy FX is a sovereign entity comprised of Sophia Poirier and Tom Murphy 7.  
This is our class for doing all kinds of fancy plugin parameter stuff.
------------------------------------------------------------------------*/

#pragma once


#include <cstddef>

#include "dfx-base.h"



namespace dfx
{


//-----------------------------------------------------------------------------
class ISmoothedValue
{
public:
	virtual ~ISmoothedValue() noexcept = default;

	virtual double getSmoothingTime() const noexcept DFX_RT_ATTR = 0;
	virtual void setSmoothingTime(double inSmoothingTimeInSeconds) noexcept DFX_RT_ATTR = 0;
	virtual void setSampleRate(double inSampleRate) noexcept DFX_RT_ATTR = 0;
	virtual void snap() noexcept DFX_RT_ATTR = 0;
	virtual bool isSmoothing() const noexcept DFX_RT_ATTR = 0;
	virtual void inc() noexcept DFX_RT_ATTR = 0;
	virtual void inc(size_t inCount) noexcept DFX_RT_ATTR = 0;
};


}  // namespace
