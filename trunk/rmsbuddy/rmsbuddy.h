/*------------------------------------------------------------------------
Copyright (C) 2001-2010  Sophia Poirier

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

#ifndef __RMS_BUDDY_H
#define __RMS_BUDDY_H

#include "AUEffectBase.h"


enum {
	kRMSBuddyParameter_AnalysisWindowSize = 0,	// the size, in ms, of the RMS and peak analysis window / refresh rate
	kRMSBuddyParameter_ResetRMS,	// message *** reset the average RMS values
	kRMSBuddyParameter_ResetPeak,	// message *** reset the absolute peak values
	kRMSBuddyParameter_NumParameters,

	// custom property IDs for allowing the GUI component get DSP information
	kRMSBuddyProperty_DynamicsData = 64000	// read-only *** get the current dynamics analysis data
};

// this is the data structure passed between GUI and DSP components for kRMSBuddyProperty_DynamicsData
typedef struct {
	double averageRMS;
	double continualRMS;
	double absolutePeak;
	double continualPeak;
} RMSBuddyDynamicsData;


//----------------------------------------------------------------------------- 
class RMSBuddy : public AUEffectBase
{
public:
	RMSBuddy(AudioComponentInstance inComponentInstance);

	virtual OSStatus Initialize();
	virtual void Cleanup();
	virtual OSStatus Reset(AudioUnitScope inScope, AudioUnitElement inElement);

	virtual OSStatus ProcessBufferLists(AudioUnitRenderActionFlags & ioActionFlags, 
						const AudioBufferList & inBuffer, AudioBufferList & outBuffer, 
						UInt32 inFramesToProcess);

	virtual OSStatus GetParameterInfo(AudioUnitScope inScope, 
						AudioUnitParameterID inParameterID, AudioUnitParameterInfo & outParameterInfo);
	virtual OSStatus SetParameter(AudioUnitParameterID inParameterID, AudioUnitScope inScope, 
						AudioUnitElement inElement, Float32 inValue, UInt32 inBufferOffsetInFrames);

	virtual OSStatus GetPropertyInfo(AudioUnitPropertyID inPropertyID, AudioUnitScope inScope, 
						AudioUnitElement inElement, UInt32 & outDataSize, Boolean & outWritable);
	virtual OSStatus GetProperty(AudioUnitPropertyID inPropertyID, AudioUnitScope inScope, 
						AudioUnitElement inElement, void * outData);
	virtual OSStatus SetProperty(AudioUnitPropertyID inPropertyID, AudioUnitScope inScope, 
						AudioUnitElement inElement, const void * inData, UInt32 inDataSize);
	virtual int GetNumCustomUIComponents();
	virtual void GetUIComponentDescs(ComponentDescription * inDescArray);
	virtual OSStatus Version()
		{	return RMS_BUDDY_VERSION;	}
	virtual bool SupportsTail()
		{	return true;	}


private:
	UInt32 numChannels;	// remember the current number of channels being analyzed
	unsigned long totalSamples;	// the total sample count since we last started analyzing average RMS and absolute peak
	double * averageRMS;	// array of the current average RMS values for each channel
	double * totalSquaredCollection;	// array of the current sums of squared input sample values for each channel (used for RMS calculation)
	float * absolutePeak;	// array of the current absolute peak values for each channel
	void resetRMS();	// reset the average RMS-related values and restart calculation of average RMS
	void resetPeak();	// reset the absolute peak-related values and restart calculation of absolute peak

	// the below is all similar to the above stuff, but running on the update schedule of the GUI
	unsigned long guiSamplesCounter;	// number of samples since the last GUI refresh
	double * guiContinualRMS;	// the accumulation for continual RMS for GUI display
	float * guiContinualPeak;	// the peak value since the last GUI refresh
	RMSBuddyDynamicsData * guiShareDataCache;	// cache stores for the dynamics analysis data to be fetched by the GUI
	void resetGUIcounters();	// reset the GUI-related continual values
	void notifyGUI();	// post notification to the GUI that it's time to re-fetch data and refresh its display
};


#endif
