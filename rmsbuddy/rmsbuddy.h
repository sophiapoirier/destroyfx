/*--------------- by Marc Poirier  ][  June 2001 + February 2003 --------------*/

#ifndef __RMSBUDDY_H
#define __RMSBUDDY_H

#include "AUEffectBase.h"


enum {
	kAnalysisFrameSize = 0,	// the size, in ms, of the RMS and peak analysis frame / refresh rate
	kTimeToUpdate,	// a fake parameter (really an audio thread GUI notification mechanism)

	// property IDs for allowing the GUI component get DSP information and trigger DSP-related events
	kDynamicsDataProperty = 64000,	// read-only *** get the current dynamics analysis data
	kResetRMSProperty,	// event message *** reset the average RMS values
	kResetPeakProperty,	// event message *** reset the absolute peak values
	kNumChannelsProperty,	// read-only *** get the number of audio channels being analyzed
};

// this is the data structure passed between GUI and DSP components for kDynamicsDataProperty
typedef struct {
	// values filled upon output
	double averageRMS;
	double continualRMS;
	float absolutePeak;
	float continualPeak;
	// value sent in upon input (specifies which audio channel you want data for)
	unsigned long channel;
} RmsBuddyDynamicsData;


//----------------------------------------------------------------------------- 
class RMSbuddy : public AUEffectBase
{
public:
	RMSbuddy(AudioUnit component);

	virtual ComponentResult Initialize();
	virtual void Cleanup();
	virtual ComponentResult Reset(AudioUnitScope inScope, AudioUnitElement inElement);

	virtual OSStatus ProcessBufferLists(AudioUnitRenderActionFlags & ioActionFlags, 
						const AudioBufferList & inBuffer, AudioBufferList & outBuffer, 
						UInt32 inFramesToProcess);

	virtual ComponentResult GetParameterInfo(AudioUnitScope inScope, 
						AudioUnitParameterID inParameterID, AudioUnitParameterInfo & outParameterInfo);
	virtual ComponentResult GetParameter(AudioUnitParameterID inParameterID, AudioUnitScope inScope, 
						AudioUnitElement inElement, Float32 & outValue);

	virtual ComponentResult GetPropertyInfo(AudioUnitPropertyID inPropertyID, AudioUnitScope inScope, 
						AudioUnitElement inElement, UInt32 & outDataSize, Boolean & outWritable);
	virtual ComponentResult GetProperty(AudioUnitPropertyID inPropertyID, AudioUnitScope inScope, 
						AudioUnitElement inElement, void * outData);
	virtual ComponentResult SetProperty(AudioUnitPropertyID inPropertyID, AudioUnitScope inScope, 
						AudioUnitElement inElement, const void * inData, UInt32 inDataSize);
	virtual int GetNumCustomUIComponents();
	virtual void GetUIComponentDescs(ComponentDescription * inDescArray);
	virtual ComponentResult	Version()
		{	return RMS_BUDDY_VERSION;	}
	virtual bool SupportsTail()
		{	return true;	}

private:
	unsigned long numChannels;	// remember the current number of channels being analyzed
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
	RmsBuddyDynamicsData * guiShareDataCache;	// cache stores for the dynamics analysis data to be fetched by the GUI
	void resetGUIcounters();	// reset the GUI-related continual values
	void notifyGUI();	// post notification to the GUI that it's time to re-fetch data and refresh its display
};


#endif
