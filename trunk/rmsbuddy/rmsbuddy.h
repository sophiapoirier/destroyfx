/*--------------- by Marc Poirier  ][  June 2001 + February 2003 --------------*/

#ifndef __RMSBUDDY_H
#define __RMSBUDDY_H

#include "AUInlineEffectBase.h"


enum {
	kAnalysisFrameSize = 0,
	kTimeToUpdate,	// a fake parameter (GUI notification mechanism)

	kDynamicsDataProperty = 64000,
	kResetRMSProperty,
	kResetPeakProperty,
};

struct DynamicsData {
	// output
	double averageRMS;
	double continualRMS;
	float absolutePeak;
	float continualPeak;
	// input
	unsigned long channel;
};


//----------------------------------------------------------------------------- 
class RMSbuddy : public AUInlineEffectBase
{
public:
	RMSbuddy(AudioUnit component);
	virtual ~RMSbuddy();

	virtual ComponentResult Initialize();
	virtual ComponentResult Reset(AudioUnitScope inScope, AudioUnitElement inElement);

	virtual OSStatus ProcessBufferLists(AudioUnitRenderActionFlags &ioActionFlags, 
						const AudioBufferList &inBuffer, AudioBufferList &outBuffer, 
						UInt32 inFramesToProcess);

	virtual ComponentResult GetPropertyInfo(AudioUnitPropertyID inID, AudioUnitScope inScope, 
						AudioUnitElement inElement, UInt32 &outDataSize, Boolean &outWritable);
	virtual ComponentResult GetProperty(AudioUnitPropertyID inID, AudioUnitScope inScope, 
						AudioUnitElement inElement, void *outData);
	virtual ComponentResult SetProperty(AudioUnitPropertyID inID, AudioUnitScope inScope, 
						AudioUnitElement inElement, const void *inData, UInt32 inDataSize);

	virtual ComponentResult GetParameterInfo(AudioUnitScope inScope, 
						AudioUnitParameterID inParameterID, AudioUnitParameterInfo &outParameterInfo);
	virtual ComponentResult GetParameter(AudioUnitParameterID inParameterID, AudioUnitScope inScope, 
						AudioUnitElement inElement, Float32 &outValue);
	virtual UInt32 SupportedNumChannels(const AUChannelInfo **outInfo);
	virtual int GetNumCustomUIComponents();
	virtual void GetUIComponentDescs(ComponentDescription *inDescArray);

private:
	double leftAverageRMS, rightAverageRMS;
	unsigned long totalSamples;
	double totalSquaredCollection1, totalSquaredCollection2;
	float leftAbsolutePeak, rightAbsolutePeak;
	void resetRMS();
	void resetPeak();

	unsigned long GUIsamplesCounter;
	double GUIleftContinualRMS, GUIrightContinualRMS;
	float GUIleftContinualPeak, GUIrightContinualPeak;
	DynamicsData leftData, rightData;
	void resetGUIcounters();
	void notifyGUI();
};


#endif
