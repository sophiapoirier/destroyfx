/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2018  Sophia Poirier

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Destroy FX Library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Destroy FX Library.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/

This is a template for making a DfxPlugin.
------------------------------------------------------------------------*/

#pragma once


#include "dfxplugin.h"

#include <vector>


//----------------------------------------------------------------------------- 
// these are the plugin parameters:
enum
{
	kFloatParam,
	kIntParam,
	kIndexParam,
	kBooleanParam,

	kNumParameters
};


//----------------------------------------------------------------------------- 
// constants and helpfuls

constexpr long kNumPresets = 16;

// audio buffer size in seconds
constexpr double kBufferSize_Seconds = 3.0;

// the different states of the indexed parameter
enum
{
	kIndexParamState1,
	kIndexParamState2,
	kIndexParamState3,
	kNumIndexParamStates
};


//----------------------------------------------------------------------------- 
class DfxStub : public DfxPlugin
{
public:
	DfxStub(TARGET_API_BASE_INSTANCE_TYPE inInstance);
	virtual ~DfxStub();

	void dfx_PostConstructor() override;
	long initialize() override;
	void cleanup() override;
	void reset() override;

#if !TARGET_PLUGIN_USES_DSPCORE
	void processaudio(float const* const* in, float** out, unsigned long inNumFrames, bool replacing = true) override;
	void processparameters() override;

	bool createbuffers() override;
	void releasebuffers() override;
	void clearbuffers() override;
#endif

private:
	void initPresets();

#if !TARGET_PLUGIN_USES_DSPCORE
	// handy usable copies of the parameters
	float floatParam = 0.0f;
	long intParam = 0, indexParam = 0;
	bool booleanParam = false;

	// stuff needed for audio processing
	std::vector<std::vector<float>> buffers;  // a two-dimensional array of audio buffers
	long bufferpos = 0;  // position in the buffer
#endif

};



#if TARGET_PLUGIN_USES_DSPCORE
//----------------------------------------------------------------------------- 
class DfxStubDSP : public DfxPluginCore
{
public:
	DfxStubDSP(DfxPlugin* inInstance);
	virtual ~DfxStubDSP();

	void reset() override;
	bool createbuffers() override;
	void releasebuffers() override;
	void clearbuffers() override;

	void processparameters() override;
	void process(float const* inStream, float* outStream, unsigned long inNumFrames, bool replacing = true) override;

private:
	// handy usable copies of the parameters
	float floatParam = 0.0f;
	long intParam = 0, indexParam = 0;
	bool booleanParam = false;

	// stuff needed for audio processing
	std::vector<float> buffer;  // an audio buffer
	long bufferpos = 0;  // position in the buffer
};
#endif  // TARGET_PLUGIN_USES_DSPCORE
