/*----------------------- by Marc Poirier  ][  June 2001 ----------------------*/

#ifndef __RMSBUDDY_VST_H
#define __RMSBUDDY_VST_H

#include "audioeffectx.h"


//----------------------------------------------------------------------------- 
class RMSbuddy : public AudioEffectX
{
friend class RMSbuddyEditor;
public:
	RMSbuddy(audioMasterCallback audioMaster);
	virtual ~RMSbuddy();

	virtual void resume();

	virtual void process(float **inputs, float **outputs, long sampleFrames);
	virtual void processReplacing(float **inputs, float **outputs, long sampleFrames);

	virtual bool getEffectName(char *name);
	virtual long getVendorVersion();
	virtual bool getErrorText(char *text);
	virtual bool getVendorString(char *text);
	virtual bool getProductString(char *text);

	virtual bool getInputProperties(long index, VstPinProperties* properties);
	virtual bool getOutputProperties(long index, VstPinProperties* properties);
	virtual long canDo(char* text);

private:
	void processaudio(float **inputs, float **outputs, long sampleFrames, bool replacing);

	float leftAverageRMS, rightAverageRMS;
	float leftContinualRMS, rightContinualRMS;
	long totalSamples;
	float totalSquaredCollection1, totalSquaredCollection2;
	float leftAbsolutePeak, rightAbsolutePeak;
	float leftContinualPeak, rightContinualPeak;

	long GUIsamplesCounter;
	float GUIleftContinualRMS, GUIrightContinualRMS;
	float GUIleftContinualPeak, GUIrightContinualPeak;
	void resetGUIcounters();
};

#endif
