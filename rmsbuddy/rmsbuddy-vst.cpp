/*---------------------- by Sophia Poirier  ][  June 2001 ---------------------*/

#include "rmsbuddy-vst.h"
#include "rmsbuddyeditor-vst.h"

#include <stdio.h>
#include <math.h>


//-----------------------------------------------------------------------------
// initializations & such

RMSbuddy::RMSbuddy(audioMasterCallback audioMaster)
	: AudioEffectX(audioMaster, 0, 0)	// no programs, no parameters
{
	setNumInputs(2);	// stereo in
	setNumOutputs(2);	// stereo out
	setUniqueID(RMS_BUDDY_PLUGIN_ID);	// identify
	canMono();	// it's okay to feed both inputs with the same signal
	canProcessReplacing();	// supports both accumulating and replacing output

	resume();

	editor = new RMSbuddyEditor(this);
}

//-----------------------------------------------------------------------------------------
RMSbuddy::~RMSbuddy()
{
	// nud
}

//-----------------------------------------------------------------------------------------
// this gets called when the plugin is activated

void RMSbuddy::resume()
{
	// reset all of these things
	totalSamples = 0;
	totalSquaredCollection1 = totalSquaredCollection2 = 0.0f;
	leftContinualRMS = rightContinualRMS = 0.0f;
	leftAverageRMS = rightAverageRMS = 0.0f;
	leftAbsolutePeak = rightAbsolutePeak = 0.0f;
	leftContinualPeak = rightContinualPeak = 0.0f;
	resetGUIcounters();
}

//-----------------------------------------------------------------------------
// Destroy FX infos

bool RMSbuddy::getEffectName(char *name) {
	strcpy(name, "RMS Buddy");	// name max 32 char
	return true; }

long RMSbuddy::getVendorVersion() {
	return RMS_BUDDY_VERSION; }

bool RMSbuddy::getErrorText(char *text) {
	strcpy(text, "I've stopped counting.");	// max 256 char
	return true; }

bool RMSbuddy::getVendorString(char *text) {
	strcpy(text, "Destroy FX");	// a string identifying the vendor (max 64 char)
	return true; }

bool RMSbuddy::getProductString(char *text) {
	// a string identifying the product name (max 64 char)
	strcpy(text, "Super Destroy FX bipolar VST plugin pack");
	return true; }

//------------------------------------------------------------------------
bool RMSbuddy::getInputProperties(long index, VstPinProperties* properties)
{
	if ( (index >= 0) && (index < 2) )
	{
		sprintf(properties->label, "RMS Buddy input %ld", index+1);
		sprintf(properties->shortLabel, "in %ld", index+1);
		properties->flags = kVstPinIsStereo | kVstPinIsActive;
		return true;
	}
	return false;
}

//------------------------------------------------------------------------
bool RMSbuddy::getOutputProperties(long index, VstPinProperties* properties)
{
	if ( (index >= 0) && (index < 2) )
	{
		sprintf (properties->label, "RMS Buddy output %ld", index+1);
		sprintf (properties->shortLabel, "out %ld", index+1);
		properties->flags = kVstPinIsStereo | kVstPinIsActive;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------------------
void RMSbuddy::process(float **inputs, float **outputs, long sampleFrames)
{
	processaudio(inputs, outputs, sampleFrames, false);
}

//-----------------------------------------------------------------------------------------
void RMSbuddy::processReplacing(float **inputs, float **outputs, long sampleFrames)
{
	processaudio(inputs, outputs, sampleFrames, true);
}

//-----------------------------------------------------------------------------------------
void RMSbuddy::processaudio(float **inputs, float **outputs, long sampleFrames, bool replacing)
{
	long samplecount;
	leftContinualRMS = rightContinualRMS = 0.0f;
	leftContinualPeak = rightContinualPeak = 0.0f;

	for (samplecount = 0; samplecount < sampleFrames; samplecount++)
	{
		if (replacing)
			outputs[0][samplecount] = inputs[0][samplecount];
		else
			outputs[0][samplecount] += inputs[0][samplecount];
		float absInputValue = fabsf(inputs[0][samplecount]);
		leftContinualRMS += absInputValue * absInputValue;
		if (absInputValue > leftContinualPeak)
			leftContinualPeak = absInputValue;
	}

	for (samplecount = 0; samplecount < sampleFrames; samplecount++)
	{
		if (replacing)
			outputs[1][samplecount] = inputs[1][samplecount];
		else
			outputs[1][samplecount] += inputs[1][samplecount];
		float absInputValue = fabsf(inputs[1][samplecount]);
		rightContinualRMS += absInputValue * absInputValue;
		if (absInputValue > rightContinualPeak)
			rightContinualPeak = absInputValue;
	}

	totalSamples += sampleFrames;
	totalSquaredCollection1 += leftContinualRMS;
	totalSquaredCollection2 += rightContinualRMS;

	leftAverageRMS = sqrtf(totalSquaredCollection1 / (float)totalSamples);
	rightAverageRMS = sqrtf(totalSquaredCollection2 / (float)totalSamples);

	// update absolute peak values
	if (leftContinualPeak > leftAbsolutePeak)
		leftAbsolutePeak = leftContinualPeak;
	if (rightContinualPeak > rightAbsolutePeak)
		rightAbsolutePeak = rightContinualPeak;

	// increment the RMS informations for the GUI displays
	GUIsamplesCounter += sampleFrames;
	GUIleftContinualRMS += leftContinualRMS;
	GUIrightContinualRMS += rightContinualRMS;
	// update the GUI continual peak values
	if (leftContinualPeak > GUIleftContinualPeak)
		GUIleftContinualPeak = leftContinualPeak;
	if (rightContinualPeak > GUIrightContinualPeak)
		GUIrightContinualPeak = rightContinualPeak;
}

//-----------------------------------------------------------------------------------------
void RMSbuddy::resetGUIcounters()
{
	GUIsamplesCounter = 0;
	GUIleftContinualRMS = GUIrightContinualRMS = 0.0f;
	GUIleftContinualPeak = GUIrightContinualPeak = 0.0f;
}

//-----------------------------------------------------------------------------------------
// this tells the host the plugin can do, which is nothing special in this case

long RMSbuddy::canDo(char* text)
{
	if (strcmp(text, "plugAsChannelInsert") == 0)
		return 1;
	if (strcmp(text, "plugAsSend") == 0)
		return 1;
	if (strcmp(text, "mixDryWet") == 0)
		return 1;
	if (strcmp(text, "1in1out") == 0)
		return 1;
	if (strcmp(text, "1in2out") == 0)
		return 1;
	if (strcmp(text, "2in2out") == 0)
		return 1;

	return -1;	// explicitly can't do; 0 => don't know
}



//-----------------------------------------------------------------------------
//                                   main()                                   |
//-----------------------------------------------------------------------------

// prototype of the export function main
#if BEOS
	#define main main_plugin
	extern "C" __declspec(dllexport) AEffect *main_plugin(audioMasterCallback audioMaster);
#elif MACX
	#define main main_macho
	extern "C" AEffect *main_macho(audioMasterCallback audioMaster);
#else
	AEffect *main(audioMasterCallback audioMaster);
#endif

AEffect *main(audioMasterCallback audioMaster)
{
	// get vst version
	if ( !audioMaster(0, audioMasterVersion, 0, 0, 0, 0) )
		return NULL;  // old version

	AudioEffect *effect = new RMSbuddy(audioMaster);
	if (effect == NULL)
		return NULL;
	return effect->getAeffect();
}

#if WIN32
void *hInstance;
BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved)
{
	hInstance = hInst;
	return 1;
}
#endif
