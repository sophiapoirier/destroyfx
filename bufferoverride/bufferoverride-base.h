/*------------------------------------------------------------------------
Copyright (C) 2001-2021  Sophia Poirier and Tom Murphy VII

This file is part of Buffer Override.

Buffer Override is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 2 of the License, or 
(at your option) any later version.

Buffer Override is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Buffer Override.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/


#pragma once

//----------------------------------------------------------------------------- 
// these are the plugin parameters:
enum
{
	kDivisor,
	kBufferSize_MS,
	kBufferSize_Sync,
	kBufferTempoSync,
	kBufferInterrupt,

	kDivisorLFORate_Hz,
	kDivisorLFORate_Sync,
	kDivisorLFODepth,
	kDivisorLFOShape,
	kDivisorLFOTempoSync,
	kBufferLFORate_Hz,
	kBufferLFORate_Sync,
	kBufferLFODepth,
	kBufferLFOShape,
	kBufferLFOTempoSync,

	kSmooth,
	kDryWetMix,

	kPitchBendRange,
	kMidiMode,

	kTempo,
	kTempoAuto,

	kNumParameters
};
