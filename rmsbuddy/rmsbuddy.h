/*------------------------------------------------------------------------
Copyright (C) 2001-2021  Sophia Poirier

This file is part of RMS Buddy.

RMS Buddy is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 2 of the License, or 
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

#include <optional>
#include <utility>
#include <vector>

#include "AUEffectBase.h"


//----------------------------------------------------------------------------- 
class RMSBuddy final : public AUEffectBase
{
public:
	explicit RMSBuddy(AudioComponentInstance inComponentInstance);

	OSStatus Initialize() override;
	void Cleanup() override;
	OSStatus Reset(AudioUnitScope inScope, AudioUnitElement inElement) override;

	OSStatus GetParameterList(AudioUnitScope inScope, AudioUnitParameterID* outParameterList, UInt32& outNumParameters) override;
	OSStatus GetParameterInfo(AudioUnitScope inScope, AudioUnitParameterID inParameterID, 
							  AudioUnitParameterInfo& outParameterInfo) override;
	OSStatus SetParameter(AudioUnitParameterID inParameterID, AudioUnitScope inScope, AudioUnitElement inElement, 
						  Float32 inValue, UInt32 inBufferOffsetInFrames) override;
	OSStatus CopyClumpName(AudioUnitScope inScope, UInt32 inClumpID, UInt32 inDesiredNameLength, CFStringRef* outClumpName) override;

	OSStatus GetPropertyInfo(AudioUnitPropertyID inPropertyID, AudioUnitScope inScope, AudioUnitElement inElement, 
							 UInt32& outDataSize, Boolean& outWritable) override;
	OSStatus GetProperty(AudioUnitPropertyID inPropertyID, AudioUnitScope inScope, AudioUnitElement inElement, 
						 void* outData) override;
	OSStatus SetProperty(AudioUnitPropertyID inPropertyID, AudioUnitScope inScope, AudioUnitElement inElement, 
						 void const* inData, UInt32 inDataSize) override;
	bool SupportsTail() override { return true; }
	CFURLRef CopyIconLocation() override;

	OSStatus ProcessBufferLists(AudioUnitRenderActionFlags& ioActionFlags, 
								AudioBufferList const& inBuffer, AudioBufferList& outBuffer, 
								UInt32 inFramesToProcess) override;

private:
	using ChannelParameterDesc = std::pair<UInt32, AudioUnitParameterID>;

	static AudioUnitParameterID GetParameterIDFromChannelAndID(UInt32 inChannelIndex, AudioUnitParameterID inID);
	static std::optional<ChannelParameterDesc> GetChannelAndIDFromParameterID(AudioUnitParameterID inParameterID);
	static float LinearToDecibels(float inLinearValue);

	void HandleChannelCount();
	void SetMeter(UInt32 inChannelIndex, AudioUnitParameterID inID, float inLinearValue);

	void ResetRMS();
	void ResetPeak();
	void RestartAnalysisWindow();

	float const mMinMeterValueDb;
	UInt32 mChannelCount = 0;
	uintmax_t mTotalSamples {};  // the total samples elapsed since we last started analyzing average RMS and absolute peak
	std::vector<double> mAverageRMS;  // the current average RMS values per-channel
	std::vector<double> mTotalSquaredCollection;  // the current sums of squared input sample values per-channel (used for RMS calculation)
	std::vector<float> mAbsolutePeak;  // the current absolute peak values per-channel

	// the below is all similar to the above stuff, but running on the update schedule of the GUI
	uint64_t mAnalysisWindowSampleCounter {};  // number of samples analyzed since the last GUI refresh
	std::vector<double> mContinualRMS;  // the accumulation for continual RMS for GUI display
	std::vector<float> mContinualPeak;  // the absolute peak value since the last GUI refresh
};
