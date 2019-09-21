/*------------------------------------------------------------------------
Copyright (C) 2001-2019  Sophia Poirier

This file is part of RMS Buddy.

RMS Buddy is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

RMS Buddy is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with RMS Buddy.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#pragma once

#include <AudioToolbox/AUComponent.h>


namespace dfx::RMS
{

//----------------------------------------------------------------------------- 
enum : AudioUnitParameterID
{
	kParameter_AnalysisWindowSize = 0,  // the size, in milliseconds, of the RMS and peak analysis window / refresh rate
	kParameter_ResetRMS,  // write-only: message to reset the average RMS values
	kParameter_ResetPeak,  // write-only: message to reset the absolute peak values
	kParameter_BaseCount,

	// read-only per-channel metering parameters
	kChannelParameter_AverageRMS = 0,
	kChannelParameter_ContinualRMS,
	kChannelParameter_AbsolutePeak,
	kChannelParameter_ContinualPeak,
	kChannelParameter_Count,
	kChannelParameter_Base = 100
};

}
