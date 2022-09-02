/*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2022  Sophia Poirier

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

This is a template for making a DfxPlugin.
------------------------------------------------------------------------*/

#pragma once


#include <vector>

#include "dfxplugin.h"


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
class DfxStub final : public DfxPlugin
{
public:
	explicit DfxStub(TARGET_API_BASE_INSTANCE_TYPE inInstance);

	void dfx_PostConstructor() override;

#if !TARGET_PLUGIN_USES_DSPCORE
	long initialize() override;
	void cleanup() override;
	void reset() override;

	void processaudio(float const* const* in, float** out, size_t inNumFrames) override;
	void processparameters() override;
#endif

private:
	static constexpr size_t kNumPresets = 16;
	static constexpr double kBufferSize_Seconds = 3.0;  // audio buffer size in seconds

	// the different states of the indexed parameter
	enum : long
	{
		kIndexParamState1,
		kIndexParamState2,
		kIndexParamState3,
		kNumIndexParamStates
	};

	void initPresets();

#if !TARGET_PLUGIN_USES_DSPCORE
	// handy usable copies of the parameters
	float floatParam = 0.0f;
	long intParam = 0, indexParam = 0;
	bool booleanParam = false;

	// stuff needed for audio processing
	std::vector<std::vector<float>> buffers;  // a two-dimensional array of audio buffers
	size_t bufferpos = 0;  // position in the buffer
#endif
};



#if TARGET_PLUGIN_USES_DSPCORE
//----------------------------------------------------------------------------- 
class DfxStubDSP final : public DfxPluginCore
{
public:
	explicit DfxStubDSP(DfxPlugin* inInstance);

	void reset() override;

	void processparameters() override;
	void process(float const* inStream, float* outStream, size_t inNumFrames) override;

private:
	// handy usable copies of the parameters
	float floatParam = 0.0f;
	long intParam = 0, indexParam = 0;
	bool booleanParam = false;

	// stuff needed for audio processing
	std::vector<float> buffer;  // an audio buffer
	size_t bufferpos = 0;  // position in the buffer
};
#endif  // TARGET_PLUGIN_USES_DSPCORE
